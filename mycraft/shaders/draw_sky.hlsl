struct VS_OUT
{
    float4 position : SV_Position;
    float3 textur : VS_Texture;
};

static const float PI = 3.1415926535897932384626433832795f;

static const float3 DAY_TOP_COLOR = float3(0, 150, 240) / 255.0f;
static const float3 DAY_MIDDLE_COLOR = float3(225, 180, 135) / 255.0f;
static const float3 DAY_BOTTOM_COLOR = float3(205, 110, 110) / 255.0f;

static const float3 NIGHT_TOP_COLOR = float3(0, 0, 15) / 255.0f;
static const float3 NIGHT_MIDDLE_COLOR = float3(0, 15, 40) / 255.0f;
static const float3 NIGHT_BOTTOM_COLOR = float3(0, 30, 60) / 255.0f;

static const float3 SUN_COLOR = float3(220, 210, 155) / 255.0f;
static const float SUN_MIN = 1.0f;
static const float SUN_MAX = 2.0f;

static const float3 MOON_COLOR = float3(190, 190, 190) / 255.0f;
static const float MOON_MIN = 0.75f;
static const float MOON_MAX = 1.0f;

float4x4 VP;
float3 SUN_DIRECTION;

float sky_body_power(float3 direction, float3 body_direction, float body_min, float body_max)
{
    float angle = acos(dot(direction, -body_direction)) * (180.0f / PI);
    return 1.0f - smoothstep(body_min, body_max, angle);
}

VS_OUT v_shader(float3 position : KL_Position)
{
    VS_OUT data;
    data.position = mul(float4(position, 0.0f), VP).xyww;
    data.textur = position;
    return data;
}

float4 p_shader(VS_OUT data) : SV_Target0
{
    float3 direction = normalize(data.textur);
    
    float3 sky_top_color = lerp(NIGHT_TOP_COLOR, DAY_TOP_COLOR, saturate(-SUN_DIRECTION.y));
    float3 sky_middle_color = lerp(NIGHT_MIDDLE_COLOR, DAY_MIDDLE_COLOR, saturate(-SUN_DIRECTION.y));
    float3 sky_bottom_color = lerp(NIGHT_BOTTOM_COLOR, DAY_BOTTOM_COLOR, saturate(-SUN_DIRECTION.y));
    
    float3 color;
    if (direction.y >= 0.0f)
    {
        color = lerp(sky_middle_color, sky_top_color, direction.y);
    }
    else
    {
        color = lerp(sky_middle_color, sky_bottom_color, -direction.y);
    }
    
    color = lerp(color, SUN_COLOR, sky_body_power(direction, SUN_DIRECTION, SUN_MIN, SUN_MAX));
    color = lerp(color, MOON_COLOR, sky_body_power(direction, -SUN_DIRECTION, MOON_MIN, MOON_MAX));
    return float4(color, 1.0f);
}
