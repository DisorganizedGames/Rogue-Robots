struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};


struct PushConstantElement
{
    uint table;    
    uint offset;
    uint dynTest;
    
    uint hvTable;
    uint hvOffset;
};

ConstantBuffer<PushConstantElement> pushConstant : register(b0, space0);

struct TestData
{
    float a, b, c, d;
};

float4 main(VS_OUT input) : SV_TARGET
{
    StructuredBuffer<TestData> testDatas = ResourceDescriptorHeap[pushConstant.table];
        
    TestData data = testDatas[pushConstant.offset];
    
    ConstantBuffer<TestData> dynTest = ResourceDescriptorHeap[pushConstant.dynTest];
    
    StructuredBuffer<TestData> hvTable = ResourceDescriptorHeap[pushConstant.hvTable];
    TestData hvData = hvTable[pushConstant.hvOffset];
    
    
    //return float4(hvData.a, hvData.b, hvData.c, hvData.d);
    //return float4(dynTest.a, dynTest.b, dynTest.c, dynTest.d);
    //return float4(data.a, data.b, data.c, data.d);
    
    return float4(input.uv, 0.f, 1.f);
    
}