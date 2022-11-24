struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PushConstantElement
{
    uint renderWidth;
    uint renderHeight;
    float effect;               // normalized [0, n]    // expected smth like:  cos(iTime * 5.0) * 0.5 + 0.5
    float transitionFactor;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

float4 main(PS_IN input) : SV_TARGET0
{
    // Work in GL style
    float2 uv = input.uv;
    uv.y = 1.f - uv.y;
    
    // Offset (0, 0) to center of screen
    uv -= 0.5.rr;
    uv *= 1.f;
    
    float2 o = float2(0.0, 0.0);
    float2 pOnEl = uv - o;
        
    float effect = g_constants.effect;
    float transitionFactor = g_constants.transitionFactor;
    
    const float ellipseStartOffset = 0.35f;
    
    float3 diff = float3(pOnEl, 0.0) - float3(pOnEl * ellipseStartOffset, 0.0);
    float len = clamp(length(diff), 0.f, 1.f);
    len *= (len + transitionFactor) / len;
    
    float intensity = len * effect;
    
    
    float3 color = float3(1.0, 0.0, 0.0);
    color *= max(intensity, 0.f);
    
    return float4(color, 0.6);
}

