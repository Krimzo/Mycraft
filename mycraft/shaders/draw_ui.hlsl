struct VS_OUT
{
    float4 position : SV_Position;
    float4 color : VS_Color;
    float2 textur : VS_Texture;
    float tex_blend : VS_TexBlend;
};

float AR;

Texture2D ATLAS_TEXTURE : register(t0);

SamplerState ATLAS_SAMPLER : register(s0);

VS_OUT v_shader(float3 position : KL_Position, float4 color : KL_Color, float2 textur : KL_Texture, float tex_blend : KL_Blend)
{
    VS_OUT data;
    data.position = float4(position.x / AR, position.y, 0.0f, 1.0f);
    data.color = color;
    data.textur = textur;
    data.tex_blend = tex_blend;
    return data;
}

float4 p_shader(VS_OUT data) : SV_Target0
{
    float4 texture_color = ATLAS_TEXTURE.Sample(ATLAS_SAMPLER, data.textur);
    return lerp(data.color, texture_color, data.tex_blend);
}
