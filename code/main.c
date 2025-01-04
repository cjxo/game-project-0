#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <timeapi.h>

#define COBJMACROS
#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <dxgidebug.h>
#include <d3d11sdklayers.h>
#include <d3dcompiler.h>

#include "base.h"
#include "my_math.h"
#include "game.h"

#include "my_math.c"
#include "game.c"

static HWND   g_w32_window;
static s32    g_w32_window_width;
static s32    g_w32_window_height;

#if defined(GAME_DEBUG)
# define DX11_ShaderCompileFlags (D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR|D3DCOMPILE_SKIP_OPTIMIZATION|D3DCOMPILE_ENABLE_STRICTNESS|D3DCOMPILE_WARNINGS_ARE_ERRORS)
#else
# define DX11_ShaderCompileFlags (D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR|D3DCOMPILE_OPTIMIZATION_LEVEL3)
#endif

static s32                               g_dx11_resolution_width  = 1280;
static s32                               g_dx11_resolution_height = 720;
static ID3D11Device                     *g_dx11_dev;
static ID3D11DeviceContext              *g_dx11_dev_cont;
static ID3D11Texture2D                  *g_dx11_back_buffer_tex;
static ID3D11RenderTargetView           *g_dx11_back_buffer_rtv;
static ID3D11BlendState                 *g_dx11_blend_state;
static ID3D11RasterizerState            *g_dx11_rasterizer_fill_nocull_ccw;
static ID3D11SamplerState               *g_dx11_point_sampler_all;
static ID3D11Texture2D                  *g_dx11_main_depth_stencil_tex;
static ID3D11DepthStencilView           *g_dx11_main_depth_stencil_dsv;
static ID3D11DepthStencilState          *g_dx11_depth_less_stencil_nope;

static IDXGISwapChain1           *g_dxgi_swap_chain;

static ID3D11VertexShader         *g_dx11_game_vshader;
static ID3D11PixelShader          *g_dx11_game_pshader;
static ID3D11Buffer               *g_dx11_game_cbuffer0;
static ID3D11Buffer               *g_dx11_game_cbuffer1;
static ID3D11Buffer               *g_dx11_game_quad_sbuffer0;
static ID3D11ShaderResourceView   *g_dx11_game_quad_sbuffer0_srv;
//static ID3D11Texture2D           *g_game_sprite_sheet_diffuse;

typedef struct
{
  m44 proj;
} DX11_Game_CBuffer0;

typedef struct
{
  u32 enable_texture;
  f32 _pad_0[3];
} DX11_Game_CBuffer1;

static Game_Input g_game_input;

static Input_KeyType
w32_map_wparam_to_keytype(WPARAM wParam)
{
  Input_KeyType key = Input_KeyType_Count;
  switch (wParam)
  {
    case VK_ESCAPE:
    {
      key = Input_KeyType_Escape;
    } break;

    case VK_SPACE:
    {
      key = Input_KeyType_Space;
    } break;
    
    default:
    {
      if ((wParam >= 'A') && (wParam <= 'Z'))
      {
        key = (Input_KeyType)(Input_KeyType_A + (wParam - 'A'));
      }
      else if ((wParam >= '0') && (wParam <= '9'))
      {
        key = (Input_KeyType)(Input_KeyType_0 + (wParam - '0'));
      }
    } break;
  }

  return(key);
}

static LRESULT
w32_window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
  LRESULT result = 0;

  switch (message)
  {
    case WM_CLOSE:
    {
      DestroyWindow(window);
    } break;
    
    case WM_DESTROY:
    {
      PostQuitMessage(0);
    } break;
    
    default:
    {
      result = DefWindowProc(window, message, wparam, lparam);
    } break;
  }
  
  return(result);
}

static void
dx11_create_device(void)
{
  b32 success = false;
  UINT flags = (D3D11_CREATE_DEVICE_SINGLETHREADED);
#if defined(GAME_DEBUG)
  flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif        
  D3D_FEATURE_LEVEL feature_levels = D3D_FEATURE_LEVEL_11_0;
  if(SUCCEEDED(D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, flags, &feature_levels, 1,
                                 D3D11_SDK_VERSION, &g_dx11_dev, 0, &g_dx11_dev_cont)))
  {
#if defined(GAME_DEBUG)
    ID3D11InfoQueue *dx11_infoq;
    IDXGIInfoQueue *dxgi_infoq;
    
    if (SUCCEEDED(ID3D11Device_QueryInterface(g_dx11_dev, &IID_ID3D11InfoQueue, &dx11_infoq)))
    {
      ID3D11InfoQueue_SetBreakOnSeverity(dx11_infoq, D3D11_MESSAGE_SEVERITY_CORRUPTION, TRUE);
      ID3D11InfoQueue_SetBreakOnSeverity(dx11_infoq, D3D11_MESSAGE_SEVERITY_ERROR, TRUE);
      ID3D11InfoQueue_Release(dx11_infoq);
      
      typedef HRESULT (* DXGIGetDebugInterfaceFN)(REFIID, void **);

      HMODULE lib = LoadLibraryA("Dxgidebug.dll");
      if (lib)
      {
        DXGIGetDebugInterfaceFN fn = (DXGIGetDebugInterfaceFN)GetProcAddress(lib, "DXGIGetDebugInterface");
        if (fn)
        {
          if (SUCCEEDED(fn(&IID_IDXGIInfoQueue, &dxgi_infoq)))
          {
            IDXGIInfoQueue_SetBreakOnSeverity(dxgi_infoq, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, TRUE);
            IDXGIInfoQueue_SetBreakOnSeverity(dxgi_infoq, DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, TRUE);
            IDXGIInfoQueue_Release(dxgi_infoq);
            success = true;
          }
        }
        
        FreeLibrary(lib);
      }
    }
#else
    success = true;
#endif
  }
  
  if (!success)
  {
    // TODO: Error 
    ExitProcess(1);
  }
}

static void
dx11_create_swap_chain(void)
{
  b32 success = false;
  IDXGIDevice2 *dxgi_device;
  IDXGIAdapter *dxgi_adapter;
  IDXGIFactory2 *dxgi_factory;
  
  if (SUCCEEDED(ID3D11Device_QueryInterface(g_dx11_dev, &IID_IDXGIDevice2, &dxgi_device)))
  {
    if (SUCCEEDED(IDXGIDevice2_GetAdapter(dxgi_device, &dxgi_adapter)))
    {
      if (SUCCEEDED(IDXGIAdapter_GetParent(dxgi_adapter, &IID_IDXGIFactory2, &dxgi_factory)))
      {
      // https://learn.microsoft.com/en-us/windows/win32/api/dxgi/ne-dxgi-dxgi_swap_effect
        DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {0};
        swap_chain_desc.Width              = g_dx11_resolution_width;
        swap_chain_desc.Height             = g_dx11_resolution_height;
        swap_chain_desc.Format             = DXGI_FORMAT_B8G8R8A8_UNORM;
        swap_chain_desc.Stereo             = FALSE;
        swap_chain_desc.SampleDesc.Count   = 1;
        swap_chain_desc.SampleDesc.Quality = 0;
        swap_chain_desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swap_chain_desc.BufferCount        = 2;
        swap_chain_desc.Scaling            = DXGI_SCALING_STRETCH;
        swap_chain_desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swap_chain_desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
        swap_chain_desc.Flags              = 0;
        
        if (SUCCEEDED(IDXGIFactory2_CreateSwapChainForHwnd(dxgi_factory, (IUnknown *)g_dx11_dev, g_w32_window,
                                                           &swap_chain_desc, 0, 0,
                                                           &g_dxgi_swap_chain)))
        {
          IDXGIFactory2_MakeWindowAssociation(dxgi_factory, g_w32_window, DXGI_MWA_NO_ALT_ENTER);
          
          if (SUCCEEDED(IDXGISwapChain1_GetBuffer(g_dxgi_swap_chain, 0, &IID_ID3D11Texture2D, &g_dx11_back_buffer_tex)))
          {
            D3D11_RENDER_TARGET_VIEW_DESC rtv_desc = { 0 };
            rtv_desc.Format              = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
            rtv_desc.ViewDimension       = D3D11_RTV_DIMENSION_TEXTURE2D;
            rtv_desc.Texture2D.MipSlice  = 0;
            if (SUCCEEDED(ID3D11Device_CreateRenderTargetView(g_dx11_dev, (ID3D11Resource *)g_dx11_back_buffer_tex,
                                                              &rtv_desc, &g_dx11_back_buffer_rtv)))
            {
              success = true;
            }
          }
        }
        
        IDXGIFactory2_Release(dxgi_factory);
      }
      
      IDXGIAdapter_Release(dxgi_adapter);
    }
    
    IDXGIDevice2_Release(dxgi_device);
  }
  
  if (!success)
  {
    // TODO: Error 
    ExitProcess(1);
  }
}

static void
dx11_create_states(void)
{
  b32 success = false;
  
  D3D11_RASTERIZER_DESC raster_desc;
  raster_desc.FillMode                     = D3D11_FILL_SOLID;
  raster_desc.CullMode                     = D3D11_CULL_NONE;
  raster_desc.FrontCounterClockwise        = TRUE;
  raster_desc.DepthBias                    = 0;
  raster_desc.DepthBiasClamp               = 0.0f;
  raster_desc.SlopeScaledDepthBias         = 0.0f;
  raster_desc.DepthClipEnable              = FALSE;
  raster_desc.ScissorEnable                = FALSE;
  raster_desc.MultisampleEnable            = FALSE;
  raster_desc.AntialiasedLineEnable        = FALSE;
  
  D3D11_BLEND_DESC blend_desc;
  blend_desc.AlphaToCoverageEnable                 = FALSE;
  blend_desc.IndependentBlendEnable                = FALSE;
  blend_desc.RenderTarget[0].BlendEnable           = TRUE;
  blend_desc.RenderTarget[0].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOp               = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].SrcBlendAlpha         = D3D11_BLEND_SRC_ALPHA;
  blend_desc.RenderTarget[0].DestBlendAlpha        = D3D11_BLEND_INV_SRC_ALPHA;
  blend_desc.RenderTarget[0].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
  blend_desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
  
  D3D11_SAMPLER_DESC sam_desc;
  sam_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
  //sam_desc.Filter              = D3D11_FILTER_MIN_MAG_POINT_MIP_LINEAR;
  //sam_desc.Filter              = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  //sam_desc.Filter = D3D11_FILTER_MIN_LINEAR_MAG_POINT_MIP_LINEAR;
  //sam_desc.Filter = D3D11_FILTER_ANISOTROPIC;
  sam_desc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
  sam_desc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
  sam_desc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
  sam_desc.MipLODBias = 0;
  //sam_desc.MaxAnisotropy = D3D11_REQ_MAXANISOTROPY;
  sam_desc.MaxAnisotropy = 1;
  sam_desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sam_desc.BorderColor[0] = 1.0f;
  sam_desc.BorderColor[1] = 1.0f;
  sam_desc.BorderColor[2] = 1.0f;
  sam_desc.BorderColor[3] = 1.0f;
  sam_desc.MinLOD = 0;
  sam_desc.MaxLOD = D3D11_FLOAT32_MAX;
  
  if (SUCCEEDED(ID3D11Device1_CreateRasterizerState(g_dx11_dev, &raster_desc, &g_dx11_rasterizer_fill_nocull_ccw)))
  {
    if (SUCCEEDED(ID3D11Device_CreateBlendState(g_dx11_dev, &blend_desc, &g_dx11_blend_state)))
    {
      if (SUCCEEDED(ID3D11Device1_CreateSamplerState(g_dx11_dev, &sam_desc, &g_dx11_point_sampler_all)))
      {
        success = true;
      }
    }
  }
  
  if (!success)
  {
    // TODO: error...
    ExitProcess(1);
  }
}

static void
dx11_create_depth_stencils(void)
{
  b32 success = false;
  
  D3D11_DEPTH_STENCIL_DESC depth_stencil_desc =
  {
    .DepthEnable           = TRUE,
    .DepthWriteMask        = D3D11_DEPTH_WRITE_MASK_ALL,
    .DepthFunc             = D3D11_COMPARISON_LESS,
    .StencilEnable         = FALSE,
    .StencilReadMask       = D3D11_DEFAULT_STENCIL_READ_MASK,
    .StencilWriteMask      = D3D11_DEFAULT_STENCIL_WRITE_MASK,
  };
  
  if (SUCCEEDED(ID3D11Device1_CreateDepthStencilState(g_dx11_dev, &depth_stencil_desc, &g_dx11_depth_less_stencil_nope)))
  {
    D3D11_TEXTURE2D_DESC tex_desc =
    {
      .Width             = g_dx11_resolution_width,
      .Height            = g_dx11_resolution_height,
      .MipLevels         = 1,
      .ArraySize         = 1,
      .Format            = DXGI_FORMAT_D24_UNORM_S8_UINT,
      .SampleDesc        = { 1, 0 },
      .Usage             = D3D11_USAGE_DEFAULT,
      .BindFlags         = D3D11_BIND_DEPTH_STENCIL,
      .CPUAccessFlags    = 0,
      .MiscFlags         = 0
    };
    
    if (SUCCEEDED(ID3D11Device1_CreateTexture2D(g_dx11_dev, &tex_desc, 0, &g_dx11_main_depth_stencil_tex)))
    {
      if (SUCCEEDED(ID3D11Device1_CreateDepthStencilView(g_dx11_dev, (ID3D11Resource *)g_dx11_main_depth_stencil_tex, 0, &g_dx11_main_depth_stencil_dsv)))
      {
        success = true;
      }
    }
  }
  

  if (!success)
  {
    // TODO: error...
    ExitProcess(1);
  }
}

static void
dx11_create_game_shaders(void)
{
  b32 success = false;
  ID3DBlob *bytecode, *errmsg;
  D3DCompileFromFile(L"..\\code\\shaders\\game-shader.hlsl", 0, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                     "vs_main", "vs_5_0", DX11_ShaderCompileFlags, 0, &bytecode, &errmsg);
  
  if (errmsg)
  {
    OutputDebugStringA(ID3D10Blob_GetBufferPointer(errmsg));
    Assert(0);
  }
  
  if (SUCCEEDED(ID3D11Device1_CreateVertexShader(g_dx11_dev, ID3D10Blob_GetBufferPointer(bytecode),
                                                 ID3D10Blob_GetBufferSize(bytecode),
                                                 0, &g_dx11_game_vshader)))
  {
    ID3D10Blob_Release(bytecode);
    
    D3DCompileFromFile(L"..\\code\\shaders\\game-shader.hlsl", 0, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                     "ps_main", "ps_5_0", DX11_ShaderCompileFlags, 0, &bytecode, &errmsg);
                     
    if (errmsg)
    {
      OutputDebugStringA(ID3D10Blob_GetBufferPointer(errmsg));
      Assert(0);
    }
    
    if (SUCCEEDED(ID3D11Device1_CreatePixelShader(g_dx11_dev, ID3D10Blob_GetBufferPointer(bytecode),
                                                  ID3D10Blob_GetBufferSize(bytecode),
                                                  0, &g_dx11_game_pshader)))
    {
      ID3D10Blob_Release(bytecode);
      
      D3D11_BUFFER_DESC cbuf_desc =
      {
        .ByteWidth           = sizeof(DX11_Game_CBuffer0),
        .Usage               = D3D11_USAGE_DYNAMIC,
        .BindFlags           = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags           = 0,
        .StructureByteStride = 0,
      };
      
      if (SUCCEEDED(ID3D11Device1_CreateBuffer(g_dx11_dev, &cbuf_desc, 0, &g_dx11_game_cbuffer0)))
      {
        cbuf_desc.ByteWidth           = sizeof(DX11_Game_CBuffer1);
        if (SUCCEEDED(ID3D11Device1_CreateBuffer(g_dx11_dev, &cbuf_desc, 0, &g_dx11_game_cbuffer1)))
        {
          D3D11_BUFFER_DESC sbuffer_desc =
          {
            .ByteWidth				    = sizeof(Game_QuadInstance) * Game_MaxQuadInstances,
            .Usage					      = D3D11_USAGE_DYNAMIC,
            .BindFlags				    = D3D11_BIND_SHADER_RESOURCE,
            .CPUAccessFlags		    = D3D11_CPU_ACCESS_WRITE,
            .MiscFlags				    = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
            .StructureByteStride	= sizeof(Game_QuadInstance),
          };
          if (SUCCEEDED(ID3D11Device1_CreateBuffer(g_dx11_dev, &sbuffer_desc, 0, &g_dx11_game_quad_sbuffer0)))
          {
            D3D11_SHADER_RESOURCE_VIEW_DESC sbuffer_srv_desc =
            {
              .Format           = DXGI_FORMAT_UNKNOWN,
              .ViewDimension    = D3D11_SRV_DIMENSION_BUFFER,
              .Buffer           = { .NumElements = Game_MaxQuadInstances }
            };
            AssertHR(ID3D11Device1_CreateShaderResourceView(g_dx11_dev, (ID3D11Resource *)g_dx11_game_quad_sbuffer0, 
                                                            &sbuffer_srv_desc, &g_dx11_game_quad_sbuffer0_srv));
            success = true;
          }
        }
      }
    }
  }
  
  if (!success)
  {
    // TODO: error...
    ExitProcess(1);
  }
}

int __stdcall
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        PSTR lpCmdLine, int nCmdShow)
{
  (void)hInstance;
  (void)hPrevInstance;
  (void)lpCmdLine;
  (void)nCmdShow;
  
  WNDCLASS wnd_class =
  {
    .style           = 0,
    .lpfnWndProc     = &w32_window_proc,
    .cbClsExtra      = 0,
    .cbWndExtra      = 0,
    .hInstance       = GetModuleHandleA(0),
    .hIcon           = LoadIconA(0, IDI_APPLICATION),
    .hCursor         = LoadCursorA(0, IDC_ARROW),
    .hbrBackground   = GetStockObject(BLACK_BRUSH),
    .lpszMenuName    = 0,
    .lpszClassName   = "Game Project",
  };

  g_w32_window;
  g_w32_window_width   = 1280;
  g_w32_window_height  = 720;
  if (RegisterClassA(&wnd_class))
  {
    RECT w32_client_rect =
    {
      .left       = 0,
      .top        = 0,
      .right      = g_w32_window_width,
      .bottom     = g_w32_window_height,
    };
    AdjustWindowRect(&w32_client_rect, WS_OVERLAPPEDWINDOW, FALSE);
    
    g_w32_window  = CreateWindowA(wnd_class.lpszClassName, "D3D11 - HLSL", WS_OVERLAPPEDWINDOW,
                                  0, 0, w32_client_rect.right - w32_client_rect.left, w32_client_rect.bottom - w32_client_rect.top,
                                  0, 0, wnd_class.hInstance, 0);
    if (IsWindow(g_w32_window))
    {
      ShowWindow(g_w32_window, SW_SHOW);
      
      dx11_create_device();
      dx11_create_swap_chain();
      dx11_create_states();
      dx11_create_depth_stencils();
      dx11_create_game_shaders();
      
      Game_State game_state = {0};
      
      b32 is_running = true;
      TIMECAPS tc;
      timeGetDevCaps(&tc, sizeof(tc));
      timeBeginPeriod(tc.wPeriodMin);
      
      DEVMODE devmode;
      EnumDisplaySettingsA(0, ENUM_CURRENT_SETTINGS, &devmode);
      f32 seconds_per_frame   = 1.0f / (f32)devmode.dmDisplayFrequency;
      f32 game_update_secs    = 1.0f / (f32)devmode.dmDisplayFrequency;
      
      while (is_running)
      {
        for (u32 key = 0; key < Input_KeyType_Count; ++key)
        {
          g_game_input.key[key] &= ~(Input_InteractFlag_Pressed | Input_InteractFlag_Released);
        }
        
        for (u32 button = 0; button < Input_ButtonType_Count; ++button)
        {
          g_game_input.button[button] &= ~(Input_InteractFlag_Pressed | Input_InteractFlag_Released);
        }
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) != 0)
        {
          switch (msg.message)
          {
            case WM_QUIT:
            {
              is_running = false;
            } break;
            
            case WM_KEYDOWN:
            {
              Input_KeyType key = w32_map_wparam_to_keytype(msg.wParam);
              if (key != Input_KeyType_Count)
              {
                g_game_input.key[key] |= Input_InteractFlag_Pressed | Input_InteractFlag_Held;
              }
            } break;
            
            case WM_KEYUP:
            {
              Input_KeyType key = w32_map_wparam_to_keytype(msg.wParam);
              if (key != Input_KeyType_Count)
              {
                g_game_input.key[key] |= Input_InteractFlag_Released;
                g_game_input.key[key] &= ~Input_InteractFlag_Held;
              }
            } break;
        
            default:
            {
              TranslateMessage(&msg); 
              DispatchMessage(&msg);
            } break;
          }
        } 
        
        if (g_game_input.key[Input_KeyType_Escape] & Input_InteractFlag_Released)
        {
          is_running = false;
        }
        
        game_update_and_render(&game_state, &g_game_input, game_update_secs);
        
        D3D11_VIEWPORT viewport =
        {
          .Width     = (f32)g_dx11_resolution_width,
          .Height    = (f32)g_dx11_resolution_height,
          .TopLeftX  = 0,
          .TopLeftY  = 0,
          .MinDepth  = 0,
          .MaxDepth  = 1,
        };
        
        f32 clear_colour[4] = {0};
        ID3D11DeviceContext_ClearRenderTargetView(g_dx11_dev_cont, g_dx11_back_buffer_rtv, clear_colour);
        ID3D11DeviceContext_ClearDepthStencilView(g_dx11_dev_cont, g_dx11_main_depth_stencil_dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        
        // buffer mapping
        {
          D3D11_MAPPED_SUBRESOURCE mapped_subresource;
          ID3D11DeviceContext_Map(g_dx11_dev_cont, (ID3D11Resource *)g_dx11_game_cbuffer0, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
          DX11_Game_CBuffer0 new_cbuf0 =
          {
            .proj = m44_make_ortho_z01(-viewport.Width * 0.5f, viewport.Width * 0.5f, viewport.Height * 0.5f, -viewport.Height * 0.5f, 0.0f, 1.0f),
          };
          
          CopyMemory(mapped_subresource.pData, &new_cbuf0, sizeof(new_cbuf0));
          ID3D11DeviceContext_Unmap(g_dx11_dev_cont, (ID3D11Resource *)g_dx11_game_cbuffer0, 0);
          
          ID3D11DeviceContext_Map(g_dx11_dev_cont, (ID3D11Resource *)g_dx11_game_cbuffer1, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
          DX11_Game_CBuffer1 new_cbuf1 =
          {
            .enable_texture = 0
          };
          
          CopyMemory(mapped_subresource.pData, &new_cbuf1, sizeof(new_cbuf1));
          ID3D11DeviceContext_Unmap(g_dx11_dev_cont, (ID3D11Resource *)g_dx11_game_cbuffer1, 0);
          
          ID3D11DeviceContext_Map(g_dx11_dev_cont, (ID3D11Resource *)g_dx11_game_quad_sbuffer0, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_subresource);
          CopyMemory(mapped_subresource.pData, game_state.game_quad_instances, sizeof(Game_QuadInstance) * Game_MaxQuadInstances);
          ID3D11DeviceContext_Unmap(g_dx11_dev_cont, (ID3D11Resource *)g_dx11_game_quad_sbuffer0, 0);
        }

        {        
          ID3D11DeviceContext_IASetPrimitiveTopology(g_dx11_dev_cont, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        }
        
        {
          ID3D11DeviceContext_VSSetShader(g_dx11_dev_cont, g_dx11_game_vshader, 0, 0);
          ID3D11DeviceContext_VSSetConstantBuffers(g_dx11_dev_cont, 0, 1, &g_dx11_game_cbuffer0);
          ID3D11DeviceContext_VSSetShaderResources(g_dx11_dev_cont, 0, 1, &g_dx11_game_quad_sbuffer0_srv);
        }
        
        {
          ID3D11DeviceContext_RSSetState(g_dx11_dev_cont, g_dx11_rasterizer_fill_nocull_ccw);
          ID3D11DeviceContext_RSSetViewports(g_dx11_dev_cont, 1, &viewport);
        }
        
        {
          ID3D11DeviceContext_PSSetShader(g_dx11_dev_cont, g_dx11_game_pshader, 0, 0);
          ID3D11DeviceContext_PSSetConstantBuffers(g_dx11_dev_cont, 1, 1, &g_dx11_game_cbuffer1);
        }
        
        {
          ID3D11DeviceContext_OMSetBlendState(g_dx11_dev_cont, g_dx11_blend_state, 0, 0xFFFFFFFF);
          ID3D11DeviceContext_OMSetRenderTargets(g_dx11_dev_cont, 1, &g_dx11_back_buffer_rtv, 0);
        }
        
        {
          ID3D11DeviceContext_DrawInstanced(g_dx11_dev_cont, 4, (UINT)game_state.game_quad_instance_count, 0, 0);
        }
        
        ID3D11DeviceContext_ClearState(g_dx11_dev_cont);
        IDXGISwapChain1_Present(g_dxgi_swap_chain, 1, 0);
        
        game_state.game_quad_instance_count = 0;
      }
    }
  }

  ExitProcess(0);
}