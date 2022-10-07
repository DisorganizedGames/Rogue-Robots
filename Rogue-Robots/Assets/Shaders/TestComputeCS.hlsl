
struct PushConstantElement
{
    uint tex;
    //uint buf;
    uint rContrib;
    uint gContrib;
    uint bContrib;
    uint step;
};
ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

[numthreads(8, 8, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID)
{
    RWTexture2D<float4> tex = ResourceDescriptorHeap[g_constants.tex];
    //RWBuffer<float4> buf = ResourceDescriptorHeap[g_constants.buf];
    
    //buf[0] = float4(g_constants.rContrib, g_constants.gContrib, g_constants.bContrib, 1.f);
    
    tex[globalId.xy * g_constants.step.rr] = float4(g_constants.rContrib, g_constants.gContrib, g_constants.bContrib, 1.f);
}