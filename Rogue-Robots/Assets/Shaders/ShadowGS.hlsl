
struct GSOutput
{
    float4 pos : SV_POSITION;
    uint targetSlice : SV_RenderTargetArrayIndex;
};

struct PerLightData
{
    matrix view;
    matrix proj;
    //----
    float4 position;
    float3 color;
    float cutoffAngle;
    float3 direction;
    float strength;
};

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
      
    uint perDrawLight;
    uint wireframe;
    uint smIdx;
};
ConstantBuffer<PushConstantElement> constants : register(b0, space0);

[maxvertexcount(3)]
void main(
	triangle float4 input[3] : SV_POSITION,
	inout TriangleStream<GSOutput> output
)
{    
    for (uint i = 0; i < 3; i++)
    {
        GSOutput element;
        //float4 clip_space = mul(perLightData.proj, mul(perLightData.view, input[i]));
        float4 clip_space = input[i];
        element.pos = clip_space;
        element.targetSlice = constants.smIdx;
        output.Append(element);
    }
}