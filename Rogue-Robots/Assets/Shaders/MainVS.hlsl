struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 nor : NORMAL;
    float3 tan : TANGENT;
    float3 bitan : BITANGENT;
    float3 wsPos : WS_POSITION;
};

struct GlobalData
{
    uint meshTableSubmeshMD;
    uint meshTablePos;
    uint meshTableUV;
    uint meshTableNor;
    uint meshTableTan;
    uint meshTableBlend;
    
    uint perFrameTable;
    uint materialTable;
};

struct PerFrameData
{
    matrix viewMatrix;
    matrix projMatrix;
    matrix invProjMatrix;
    float4 camPos;
    float time;
};


struct PerDrawData
{
    matrix world;
    uint submeshID;
    uint materialID;
    uint jointsDescriptor;
};

struct JointsData
{
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
    uint gdDescriptor;
    uint perFrameOffset;
    
    uint perDrawCB;
    
};
ConstantBuffer<PushConstantElement> constants : register(b0, space0);


VS_OUT main(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    VS_OUT output = (VS_OUT) 0;
    
    ConstantBuffer<PerDrawData> perDrawData = ResourceDescriptorHeap[constants.perDrawCB];
    
    StructuredBuffer<GlobalData> gds = ResourceDescriptorHeap[constants.gdDescriptor];
    GlobalData gd = gds[0];
    
    StructuredBuffer<PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
    PerFrameData pfData = pfDatas[constants.perFrameOffset];
    
    StructuredBuffer<SubmeshMetadata> mds = ResourceDescriptorHeap[gd.meshTableSubmeshMD];
    StructuredBuffer<float3> positions = ResourceDescriptorHeap[gd.meshTablePos];
    StructuredBuffer<float2> uvs = ResourceDescriptorHeap[gd.meshTableUV];
    StructuredBuffer<float3> normals = ResourceDescriptorHeap[gd.meshTableNor];
    StructuredBuffer<float3> tangents = ResourceDescriptorHeap[gd.meshTableNor];
    StructuredBuffer<Blend> blendData = ResourceDescriptorHeap[gd.meshTableBlend];

    
    SubmeshMetadata md = mds[perDrawData.submeshID];
    Blend bw = blendData[vertexID + md.blendStart];
    vertexID += md.vertStart;

    float3 pos = positions[vertexID];
    float2 uv = uvs[vertexID];
    float3 nor = normals[vertexID];
    float3 tan = tangents[vertexID];
    float3 bitan = normalize(cross(tan, nor));
    
    if (md.blendCount > 0)
    {
        ConstantBuffer<JointsData> jointsData = ResourceDescriptorHeap[perDrawData.jointsDescriptor];
        
        matrix mat = jointsData.joints[bw.iw[0].idx] * bw.iw[0].weight;
        mat += jointsData.joints[bw.iw[1].idx] * bw.iw[1].weight;
        mat += jointsData.joints[bw.iw[2].idx] * bw.iw[2].weight;
        mat += jointsData.joints[bw.iw[3].idx] * bw.iw[3].weight;
        pos = (float3) mul(float4(pos, 1.0f), mat);
    }
    
    output.wsPos = mul(perDrawData.world, float4(pos, 1.f)).xyz;
    output.pos = mul(pfData.projMatrix, mul(pfData.viewMatrix, float4(output.wsPos, 1.f)));

    output.nor = mul(perDrawData.world, float4(nor, 0.f)).xyz;
    output.tan = mul(perDrawData.world, float4(tan, 0.f)).xyz;
    output.bitan = bitan;
    output.uv = uv;
 
    return output;
}