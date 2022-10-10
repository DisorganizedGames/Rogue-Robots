#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Material.h"
#include "ShaderInterop_Samplers.hlsli"
#include "ShaderInterop_Math.hlsli"
#include "PBRHelpers.hlsli"

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
    float3 nor : NORMAL;
    float3 tan : TANGENT;
    float3 bitan : BITANGENT;
    float3 wsPos : WS_POSITION;
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
CONSTANTS(g_constants, PushConstantElement)


static const uint NO_TEXTURE = 0xffffffff;

float4 main(VS_OUT input) : SV_TARGET
{    
    //return float4(input.nor, 1.f);
    ConstantBuffer<PerDrawData> perDrawData = ResourceDescriptorHeap[g_constants.perDrawCB];
    
    StructuredBuffer<ShaderInterop_GlobalData> gds = ResourceDescriptorHeap[g_constants.gdDescriptor];
    ShaderInterop_GlobalData gd = gds[0];
    
    StructuredBuffer<ShaderInterop_PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
    ShaderInterop_PerFrameData pfData = pfDatas[g_constants.perFrameOffset];
    
    StructuredBuffer<ShaderInterop_MaterialElement> mats = ResourceDescriptorHeap[gd.materialTable];
    ShaderInterop_MaterialElement mat = mats[perDrawData.materialID];

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
        N = normalize(GetFinalNormal(g_aniso_samp, ResourceDescriptorHeap[mat.normal], normalize(input.tan), normalize(input.bitan), normalize(input.nor), input.uv));
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
        Lo += (kD * albedoInput / 3.1415 + specular) * radiance * NdotL;
    }
    
    float3 hdr = amb + Lo;
    return float4(hdr, 1.f);
    
}


