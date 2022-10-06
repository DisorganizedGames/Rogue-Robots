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
    matrix viewMatrix;
    matrix projMatrix;
    matrix invProjMatrix;
    float4 camPos;
    float time;
};

struct MaterialElement
{
    uint albedo;
    uint metallicRoughness;
    uint normal;
    uint emissive;
    
    float4 albedoFactor;
    float4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
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


struct PerDrawData
{
    matrix world;
    uint submeshID;
    uint materialID;
    uint jointsDescriptor;
};

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    
    uint perDrawCB;
};

ConstantBuffer<PushConstantElement> constants : register(b0, space0);

SamplerState g_aniso_samp : register(s0, space1);

// PBR Functions
float DistributionGGX(float3 N, float3 H, float roughness);
float GeometrySchlickGGX(float NdotV, float roughness);
float GeometrySmith(float3 N, float3 V, float3 L, float roughness);
float3 FresnelSchlick(float cosTheta, float3 F0);
float3 GetFinalNormal(uint normalId, float3 tangent, float3 bitangent, float3 inputNormal, float2 uv);

static const float PI = 3.1415f;
static const float NO_TEXTURE = 0xffffffff;

float4 main(VS_OUT input) : SV_TARGET
{    
    ConstantBuffer<PerDrawData> perDrawData = ResourceDescriptorHeap[constants.perDrawCB];
    
    StructuredBuffer<GlobalData> gds = ResourceDescriptorHeap[constants.gdDescriptor];
    GlobalData gd = gds[0];
    
    StructuredBuffer<PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
    PerFrameData pfData = pfDatas[constants.perFrameOffset];
    
    StructuredBuffer<MaterialElement> mats = ResourceDescriptorHeap[gd.materialTable];
    MaterialElement mat = mats[perDrawData.materialID];

    float4 albedoInput4 = mat.albedoFactor;
    float albedoAlpha = albedoInput4.w;
    float3 albedoInput = albedoInput4.xyz;
    if (mat.albedo != NO_TEXTURE)
    {
        Texture2D albedo = ResourceDescriptorHeap[mat.albedo];

        albedoInput4 = albedo.Sample(g_aniso_samp, input.uv) * mat.albedoFactor;
        albedoInput = albedoInput4.xyz;
        albedoAlpha = albedoInput4.w;
    }

    float metallicInput = mat.metallicFactor;
    float roughnessInput = mat.roughnessFactor;
    if (mat.metallicRoughness != NO_TEXTURE)
    {
        Texture2D metallicRoughness = ResourceDescriptorHeap[mat.metallicRoughness];

        metallicInput = metallicRoughness.Sample(g_aniso_samp, input.uv).b * mat.metallicFactor;
        roughnessInput = metallicRoughness.Sample(g_aniso_samp, input.uv).g * mat.roughnessFactor;
    }

    float3 N = normalize(input.nor);
    if (mat.normal != NO_TEXTURE)
    {
        N = normalize(GetFinalNormal(mat.normal, normalize(input.tan), normalize(input.bitan), normalize(input.nor), input.uv));
    }
    
    float3 camPos = pfData.camPos;
    
    float3 amb = 0.03f * albedoInput;
    
      
    float3 V = normalize(camPos - input.wsPos);
    
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedoInput, metallicInput);
    
    //col = pow(col, (1.f / 2.22f).rrr);
    
    //return float4(col, 1.f);
    
    
    

        
    // Add directional Light
    float3 Lo = float3(0.f, 0.f, 0.f);
    {
        // calculate per-light radiance
        float3 L = normalize(-float3(-1.f, -1.f, 1.f));
        float3 H = normalize(V + L);
        float3 radiance = float3(1.f, 1.f, 1.f); // no attenuation
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughnessInput);
        float G = GeometrySmith(N, V, L, roughnessInput);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        float3 kS = F;
        float3 kD = float3(1.f, 1.f, 1.f) - kS;
        kD *= 1.0 - metallicInput;
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);
        
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedoInput / PI + specular) * radiance * NdotL;
    }
    
    float3 hdr = amb + Lo;
    return float4(hdr, 1.f);
    
}


float3 GetFinalNormal(uint normalId, float3 tangent, float3 bitangent, float3 inputNormal, float2 uv)
{
    Texture2D normalTex = ResourceDescriptorHeap[normalId];
    
    float3 tanSpaceNor = normalTex.Sample(g_aniso_samp, uv).xyz;
    
    // If no normal map --> Use default input normal
    if (length(tanSpaceNor) <= 0.005f)  // epsilon: 0.005f
        return inputNormal;
    
    float3x3 tbn = float3x3(tangent, bitangent, inputNormal); // matrix to orient our tangent space normal with
    tbn = transpose(tbn);
    
    // Normal map is in [0, 1] space so we need to transform it to [-1, 1] space
    float3 mappedSpaceNor = normalize(tanSpaceNor * 2.f - 1.f);
    
    // Orient the tangent space correctly in world space
    float3 mapNorWorld = normalize(mul(tbn, mappedSpaceNor));
       
    // Assuming always on
    return mapNorWorld;

}

float DistributionGGX(float3 N, float3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
	
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
	
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
	
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
	
    return ggx1 * ggx2;
}

float3 FresnelSchlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(max(1.0 - cosTheta, 0.0), 5.0);
}