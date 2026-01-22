struct VS_OUT
{
    float4 position : SV_Position;
    float4 color : VS_Color;
    float2 uv : VS_UV;
    float blend : VS_Blend;
};

float AR;

Texture2D ATLAS_TEXTURE : register(t0);

SamplerState ATLAS_SAMPLER : register(s0);

VS_OUT v_shader(uint layer : KL_Layer, float3 position : KL_Position, float4 color : KL_Color, float2 uv : KL_UV, float blend : KL_Blend)
{
    VS_OUT data;
    data.position = float4(position.x / AR, position.y,
        layer / 256.0f, // divide by 256 and not 255 because 1.0 is not visible
        1.0f);
    data.color = color;
    data.uv = uv;
    data.blend = blend;
    return data;
}

float4 p_shader(VS_OUT data) : SV_Target0
{
    float4 texture_color = ATLAS_TEXTURE.Sample(ATLAS_SAMPLER, data.uv);
    return lerp(data.color, texture_color, data.blend);
}
