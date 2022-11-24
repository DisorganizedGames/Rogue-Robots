struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

struct VS_OUT
{
    float4 position : SV_Position;
    float2 uv : TEXCOORD;
    float3 color : COLOR;
};

float4 main(VS_OUT input) : SV_TARGET
{
    return float4(input.color, 1);
}