float4x4 WVP;

float4 v_shader(float3 position : KL_Position) : SV_Position
{
    return mul(float4(position, 1.0f), WVP);
}

float p_shader(float4 position : SV_Position) : SV_Depth
{
    return 1.0f;
}
