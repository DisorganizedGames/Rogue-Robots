struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};


struct PushConstantElement
{
    uint dynConst;
};

ConstantBuffer<PushConstantElement> pushConstant : register(b0, space0);

struct TestData
{
    float a, b, c, d;
};

float4 main(VS_OUT input) : SV_TARGET
{
    ConstantBuffer<TestData> dynTest = ResourceDescriptorHeap[pushConstant.dynConst];
    return float4(dynTest.a, dynTest.b, dynTest.c, dynTest.d);
    
    return float4(input.uv, 0.f, 1.f);
    
}