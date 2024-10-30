struct VS_OUT
{
    float4 position : SV_Position;
    float2 textur : VS_Texture;
};

static const float PI = 3.1415926535897932384626433832795f;
static const float2 DEFINED_TEXTURES[4] =
{
	{ 0.0f, 1.0f },
	{ 0.0f, 0.0f },
	{ 1.0f, 0.0f },
	{ 1.0f, 1.0f },
};

float4x4 VP;
float3 CAMERA_ORIGIN;
float ELAPSED_TIME;
float RENDER_DISTANCE;

Texture2D ATLAS_TEXTURE : register(t0);

SamplerState ATLAS_SAMPLER : register(s0);

int2 atlas_pos(uint block)
{
    return int2((block >> 4) & 0xF, block & 0xF);
}

float2 atlas_uv(uint block, uint textur)
{
    return (DEFINED_TEXTURES[textur] + atlas_pos(block)) * (1.0f / 16.0f);
}

VS_OUT v_shader(float3 position : KL_Position, uint textur : KL_Texture, uint block : KL_Block)
{
    VS_OUT data;
    data.position = mul(float4(position, 1.0f), VP);
    data.textur = atlas_uv(block, textur);
    return data;
}

void p_shader(VS_OUT data)
{
    float alpha = ATLAS_TEXTURE.Sample(ATLAS_SAMPLER, data.textur).a;
    if (alpha < 1.0f)
        discard;
}
