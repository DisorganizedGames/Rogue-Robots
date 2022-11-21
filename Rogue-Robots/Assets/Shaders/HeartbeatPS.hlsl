struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PushConstantElement
{
    float effect; // normalized [0, n[    // expected smth like:  cos(iTime * 5.0) * 0.5 + 0.5
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

float4 main(PS_IN input) : SV_TARGET0
{
    // Work in GL style
    float2 uv = input.uv;
    uv.y = 1.f - uv.y;
    
    // Offset (0, 0) to center of screen
    uv -= 0.5.rr;
    uv *= 2.f;
    
    float2 o = float2(0.0, 0.0);
    float2 pOnEl = uv - o;
    
    float iTime = 10.f;
    
    float effect = g_constants.effect;
    //float effect = cos(iTime * 5.0) * 0.5 + 0.5; // --> [0, 1]
    //effect *= 0.20;
    
      
    float intensity = length(float3(pOnEl, 0.0)) * effect;
    
    //vec3 color = vec3(rand, 0.0, 0.0);
    float3 color = float3(1.0, 0.0, 0.0);
    color *= intensity;
    
    return float4(color, 0.6);
}

