struct VS_OUT
{
    float4 pos : SV_POSITION;
};

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    uint jointOffset;
};

struct PerDrawData
{
    matrix world;
    float3 color;
    uint submeshID;
    uint materialID;
    uint jointsDescriptor;
};

ConstantBuffer<PushConstantElement> constants : register(b0, space0);
ConstantBuffer<PerDrawData> perDrawData : register(b1, space0);

float4 main(VS_OUT input) : SV_TARGET0
{
    return float4(perDrawData.color, 0.f);
}
