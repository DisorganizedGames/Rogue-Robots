
struct PushConstantElement
{
    uint test;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

[numthreads(8, 8, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID)
{
    RWTexture2D<float4> tex = ResourceDescriptorHeap[g_constants.test];
        
    tex[globalId.xy * float2(10, 10)] = float4(0.f, 1.f, 0.f, 1.f);
}