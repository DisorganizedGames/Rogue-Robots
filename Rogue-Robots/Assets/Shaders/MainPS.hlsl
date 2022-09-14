struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 nor : NORMAL;
    float3 tan : TANGENT;
    float3 bitan : BITANGENT;
    float3 wsPos : WS_POSITION;
};

struct MaterialElement
{
    uint albedo;
    uint metallicRoughness;
    uint normal;
    uint emissive;
};

struct PushConstantElement
{
    uint perFrameCB;
    
    uint submeshID;
    
    uint submeshTable;
    uint posTable;
    uint uvTable;
    uint norTable;
    uint tanTable;
    
    uint matTable;
    uint matID;
};
ConstantBuffer<PushConstantElement> constants : register(b0, space0);

SamplerState g_aniso_samp : register(s0, space1);

float4 main(VS_OUT input) : SV_TARGET
{
    StructuredBuffer<MaterialElement> mats = ResourceDescriptorHeap[constants.matTable];
    MaterialElement mat = mats[constants.matID];
    
    //return float4(mat.albedo.x, mat.emissive.x, mat.metallicRoughness.x, mat.normal.x);
    
    Texture2D albedo = ResourceDescriptorHeap[mat.albedo];
    return float4(albedo.Sample(g_aniso_samp, input.uv));

    
    return float4(normalize(input.nor), 1.f);
}