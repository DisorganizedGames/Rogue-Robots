struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 nor : NORMAL;
    float3 tan : TANGENT;
    float3 bitan : BITANGENT;
    float3 wsPos : WS_POSITION;
};

struct PerFrameData
{
    matrix world;
    matrix view;
    matrix proj;
    float4 camPos;
    matrix joints[130];
};

struct SubmeshMetadata
{
    uint vertStart;
    uint vertCount;
    uint indexStart;
    uint indexCount;
    uint blendStart;
    uint blendCount;
};

struct BlendWeight
{
    int idx;
    float weight;
};

struct Blend
{
    BlendWeight iw[4];
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
    uint blendTable;
};
ConstantBuffer<PushConstantElement> constants : register(b0, space0);


VS_OUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUT output = (VS_OUT) 0;
    
    StructuredBuffer<SubmeshMetadata> mds = ResourceDescriptorHeap[constants.submeshTable];
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[constants.posTable];
    StructuredBuffer<float2> uvs = ResourceDescriptorHeap[constants.uvTable];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[constants.norTable];
    StructuredBuffer<float3> tangents = ResourceDescriptorHeap[constants.tanTable];
    StructuredBuffer<Blend> blendData = ResourceDescriptorHeap[constants.blendTable];
    
    SubmeshMetadata md = mds[constants.submeshID];
    Blend bw = blendData[vertexID + md.blendStart];
    vertexID += md.vertStart;

    float3 pos = positions[vertexID];
    float2 uv = uvs[vertexID];
    float3 nor = normals[vertexID];
    float3 tan = tangents[vertexID];
    //float3 bitan = normalize(cross(nor, tan));  // not sure if this is correct-handed
    float3 bitan = normalize(cross(tan, nor));  // not sure if this is correct-handed
   
    ConstantBuffer<PerFrameData> pfData = ResourceDescriptorHeap[constants.perFrameCB];

    if (md.blendCount > 0)
    {
        matrix mat = pfData.joints[bw.iw[0].idx] * bw.iw[0].weight;
        mat += pfData.joints[bw.iw[1].idx] * bw.iw[1].weight;
        mat += pfData.joints[bw.iw[2].idx] * bw.iw[2].weight;
        mat += pfData.joints[bw.iw[3].idx] * bw.iw[3].weight;
        pos = (float3) mul(float4(pos, 1.0f), mat);
    }
    
    output.wsPos = mul(pfData.world, float4(pos, 1.f)).xyz;
    output.pos = mul(pfData.proj, mul(pfData.view, float4(output.wsPos, 1.f)));

    output.nor = mul(pfData.world, float4(nor, 0.f)).xyz;
    output.tan = mul(pfData.world, float4(tan, 0.f)).xyz;
    output.bitan = bitan;
    output.uv = uv;
 
    return output;
}