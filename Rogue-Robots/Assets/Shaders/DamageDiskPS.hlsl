struct PS_IN
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

struct PushConstantElement
{
    float directionX;        // Reinterpret this as float
    float directionY;        // Reinterpret this as float
    float intensity;
    float visibility;        // Reinterpret this as float
    float colorR;
    float colorG;
    float colorB;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

float4 main(PS_IN input) : SV_TARGET0
{
    float2 direction = float2(float(g_constants.directionX), float(g_constants.directionY));
    direction *= 2.f.rr;
    direction -= 1.f.rr;
    
    float visIn = float(g_constants.visibility);
    
    
    // Work in GL style
    float2 uv = input.uv;
    uv.y = 1.f - uv.y;
    
    // Offset (0, 0) to center of screen
    uv -= 0.5.rr;
    uv *= 2.f;
    
    // Visibility of effect: Should be tweaked dynamically
    //const float visibility = 1.f;
    float visibility = visIn;
    
    // Direction of the effect in 2D cartesian space
    //float2 effectDirection = float2(0.f, 1.f);
    float2 effectDirection = normalize(direction);
    
    // Setup new space (X)..
    float2 xAxis = -effectDirection;
    float2 o = float2(0.f, 0.f);
    float2 pOnEllipse = uv - o;
    float lenToEllipse = length(pOnEllipse); 
    
    // Bow angle
    const float b = 7.f;
    const float bowOffset = 0.85f;
    const float bowThickness = 0.025f;
    const float aoa = 180.0;        // Static in space
    
    // Angle to point
    float a = acos(dot(pOnEllipse, xAxis) / (length(xAxis) * length(pOnEllipse)));
    float aDeg = a * 180.f / 3.1415f;
    
    // Resolve disk (bow) endpoints
    float leftDeg = aoa - b;
    float rightDeg = aoa + b;
    bool onBow = aDeg > leftDeg && aDeg < rightDeg;
    
    // Circular length from center outwards
    float lenFromCenter = saturate(length(effectDirection * bowOffset - pOnEllipse)) * 5.f;
    //return float4(lenFromCenter.rrr, 1.f);
    
    bool onEllipse = lenToEllipse > bowOffset - bowThickness * lenFromCenter &&
                     lenToEllipse < bowOffset + bowThickness * lenFromCenter;
    
    bool onMidEllipse = lenToEllipse > bowOffset - bowThickness &&
                        lenToEllipse < bowOffset + bowThickness;
    
    
    float3 color = float3(g_constants.colorR, g_constants.colorG, g_constants.colorB) * g_constants.intensity;
    float vis = visibility <= 0.6f ? 0.f : visibility;
    // Opacity below 0.5 seems to just make it disappear, so we disappear early at 0.6
    if (onEllipse && onBow)
        return float4(color, vis);
    else if (onMidEllipse && onBow && lenFromCenter < 0.1f)
        return float4(color, vis);
    else
        return float4(0.f.rrr, 0.f);
}

