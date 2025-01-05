cbuffer ConstantStore0 : register(b0)
{
  float4x4 proj;
};

cbuffer ConstantStore1 : register(b1)
{
  uint   enable_texture;
  float  _pad[3];
};

struct Quad_Instance
{
  float3 p;
  float3 dims;
  
  float4 colour;
};

struct VertexShader_Output
{
  float4 p          : SV_Position;
  float4 colour     : Colour_Mod;
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
  
  return sample;
}