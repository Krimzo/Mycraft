struct VS_OUT
{
    float3 world : VS_World;
    float4 position : SV_Position;
    float2 textur : VS_Texture;
    float ambient : VS_Ambient;
    float4 sun : VS_Sun;
};

static const float PI = radians(180);
static const float2 DEFINED_TEXTURES[4] = {
    { 0.0f, 1.0f },
    { 0.0f, 0.0f },
    { 1.0f, 0.0f },
    { 1.0f, 1.0f },
};
static const float AMBIENT_FACTOR = 0.2f;

float4x4 VP;
float4x4 SUN_VP;
float3 CAMERA_POSITION;
float ELAPSED_TIME;
float3 SUN_DIRECTION;
float RENDER_DISTANCE;
float2 SHADOW_TEXEL_SIZE;

Texture2D SHADOW_TEXTURE : register(t0);
Texture2D ATLAS_TEXTURE : register(t1);

SamplerState SHADOW_SAMPLER : register(s0);
SamplerState ATLAS_SAMPLER : register(s1);

int2 atlas_pos(uint block)
{
    return int2((block >> 4) & 0xF, block & 0xF);
}

float2 atlas_uv(uint block, uint textur)
{
    return (DEFINED_TEXTURES[textur] + atlas_pos(block)) * (1.0f / 16.0f);
}

float get_pcf_shadow(float3 light_coords, int half_kernel_size)
{
    float shadow_factor = 0.0f;
    [unroll]
    for (int y = -half_kernel_size; y <= half_kernel_size; y++)
    {
        [unroll]
        for (int x = -half_kernel_size; x <= half_kernel_size; x++)
        {
            float2 kernel_coords = float2(x, y) + 0.25f;
            float2 altered_coords = light_coords.xy + kernel_coords * SHADOW_TEXEL_SIZE;
            float shadow_depth = SHADOW_TEXTURE.Sample(SHADOW_SAMPLER, altered_coords).r;
            shadow_factor += (light_coords.z <= shadow_depth) ? 0.0f : 1.0f;
        }
    }
    return shadow_factor / ((half_kernel_size * 2 + 1) * (half_kernel_size * 2 + 1));
}

VS_OUT v_shader(float3 position : KL_Position, uint textur : KL_Texture, uint ambient : KL_Ambient, uint block : KL_Block)
{
    VS_OUT data;
    data.world = position;
    data.position = mul(float4(data.world, 1.0f), VP);
    data.textur = atlas_uv(block, textur);
    data.ambient = ambient * (AMBIENT_FACTOR / 255.0f);
    
    data.sun = mul(float4(position, 1.0f), SUN_VP);
    data.sun.xy *= float2(0.5f, -0.5f);
    data.sun.xy += 0.5f;
    
    return data;
}

float4 p_shader(VS_OUT data) : SV_Target0
{
    float4 color = ATLAS_TEXTURE.Sample(ATLAS_SAMPLER, data.textur);
    if (color.a < 1.0f)
        discard;
    
    float3 a = data.world;
    float3 b = a + ddx(a);
    float3 c = a + ddy(a);
    float3 normal = normalize(cross(b - a, c - a));
    
    data.sun.z = min(data.sun.z, 1.0f);
    
    float diffuse = saturate(dot(-SUN_DIRECTION, normal));
    float shadow = get_pcf_shadow(data.sun.xyz, 1);
    float light = max(diffuse * (1.0f - shadow), data.ambient);
    
    color.xyz *= light;
    return color;
}
