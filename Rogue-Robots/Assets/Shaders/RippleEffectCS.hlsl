#include "ShaderInterop_Samplers.hlsli"

// based on https://www.shadertoy.com/view/llj3Dz

struct PushConstantElement
{
    uint tex;
    uint width;
    uint height;
    float x;
    float y;
    float z;
    float time;
    float centerX;
    float centerY;
};
ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

[numthreads(16, 16, 1)]
void main(uint3 globalId : SV_DispatchThreadID, uint3 threadId : SV_GroupThreadID)
{
    float2 uv = float2((globalId.x + 0.5f) / g_constants.width, (globalId.y + 0.5f) / g_constants.height);
    float r = float(g_constants.height) / float(g_constants.width);
    float3 waveParams = float3(g_constants.x, g_constants.y, g_constants.z);
    float2 center = float2(g_constants.centerX, g_constants.centerY);

    float2 uvd = uv;
    float2 centerd = center;

    uvd.y *= r;
    centerd.y *= r;

    float d = distance(uvd, centerd);
    float t = g_constants.time;
    if (d <= t + waveParams.z && d >= t - waveParams.z)
    {
        float diff = d - t;
        float powDiff = 1.0 - pow(abs(diff * waveParams.x), waveParams.y);
        float diffTime = diff * powDiff;

        float2 diffUV = normalize(uv - center);
        uv += (diffUV * diffTime);

        Texture2D texR = ResourceDescriptorHeap[g_constants.tex];
        RWTexture2D<float4> texW = ResourceDescriptorHeap[g_constants.tex];
        float4 val = texR.Sample(g_bilinear_clamp_samp, uv);
        val.rgb += (val.rgb * powDiff);


        texW[globalId.xy] = val;
    }
}
