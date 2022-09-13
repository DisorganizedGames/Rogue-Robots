struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 nor : NORMAL;
    float3 tan : TANGENT;
    float3 bitan : BITANGENT;
    float3 wsPos : WS_POSITION;
};
float4 main(VS_OUT input) : SV_TARGET
{
    return float4(normalize(input.nor), 1.f);
}