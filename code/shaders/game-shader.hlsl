cbuffer ConstantStore0 : register(b0)
{
  float4x4 proj;
};

cbuffer ConstantStore1 : register(b1)
{
  uint   enable_texture;
  float3 inv_sprite_sheet_dims;
};

struct Quad_Instance
{
  float3 p;
  float3 dims;  
  float4 colour;
  
  float2 atlas_p;
  float2 atlas_dims;
  uint tex_on_me;
};

struct VertexShader_Output
{
  float4 p          : SV_Position;
  float4 colour     : Colour_Mod;
  float2 uv         : Tex_UV;
  uint tex_on_me    : Tex_On_Me;
};

StructuredBuffer<Quad_Instance>       g_quad_instances           : register(t0);
Texture2D<float4>                     g_sprite_sheet_diffuse     : register(t1);

SamplerState                          g_sampler                  : register(s0);

static float3 g_quad_vertices[] =
{
  float3(+0.0f, +0.0f, 0.0f),
  float3(+0.0f, +1.0f, 0.0f),
  float3(+1.0f, +0.0f, 0.0f),
  float3(+1.0f, +1.0f, 0.0f),
};

VertexShader_Output
vs_main(uint iid : SV_InstanceID, uint vid : SV_VertexID)
{
  VertexShader_Output result = (VertexShader_Output)0;

  Quad_Instance instances   = g_quad_instances[iid];
  float3 vertex             = g_quad_vertices[vid] * instances.dims + instances.p;
  
  result.p        = mul(proj, float4(vertex, 1.0f));
  result.colour   = instances.colour;
  
  if (vid == 0)
  {
    result.uv = float2(instances.atlas_p.x, instances.atlas_p.y + instances.atlas_dims.y);
  }
  else if (vid == 1)
  {
    result.uv = instances.atlas_p;
  }
  else if (vid == 2)
  {
    result.uv = instances.atlas_p + instances.atlas_dims;
  }
  else
  {
    result.uv = float2(instances.atlas_p.x + instances.atlas_dims.x, instances.atlas_p.y);
  }
  
  result.tex_on_me = instances.tex_on_me;
  return(result);
}

float4
linear_to_srgb(float4 c)
{
  return float4(c.xyz*c.xyz, c.a);
}

float4
ps_main(VertexShader_Output inp) : SV_Target
{
  float4 sample = linear_to_srgb(inp.colour);
  
  if (inp.tex_on_me)
  {
    float2 uv     = (floor(inp.uv) + min(frac(inp.uv) / fwidth(inp.uv), 1.0f) - 0.5f) * inv_sprite_sheet_dims.xy;
    float4 texel  = linear_to_srgb(g_sprite_sheet_diffuse.Sample(g_sampler, uv));
    sample *= texel;
  }
  
  if (sample.a == 0)
  {
    discard;
  }
  
  return sample;
}