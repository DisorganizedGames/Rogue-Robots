#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Material.h"
#include "ShaderInterop_Samplers.hlsli"
#include "ShaderInterop_Math.hlsli"
#include "ShaderInterop_Lights.hlsli"
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

struct SpotlightData
{
    matrix viewMatrix;
    matrix projectionMatrix;
    float4 worldPosition;
    float3 color;
    float cutoffAngle;
    float3 direction;
    float strength;
};

struct SpotlightMetaData
{
    SpotlightData spotlightArray[12];
    uint currentNrOfSpotlights;
};

/*
    Dont use arrays of elements which are not 16-byte aligned, i.e: arrays of uint3 is NO NO (12 bytes), or array of uints (4 bytes)
*/

struct Shadow
{
    uint4 shadowMapArray[3]; //Allows for 12 "shadow-generating" spotlights (3 groups of 4 each).
};

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    
    uint perDrawCB;
    uint spotlightArrayStructureIndex;
    uint shadowMapDepthIndex;
    uint wireframe;
};

CONSTANTS(g_constants, PushConstantElement)

static const uint NO_TEXTURE = 0xffffffff;

#define SHADOWMAP_CONSTANT_BIAS 0.000015f
#define PCF_SAMPLE_COUNT 2

float CalculateShadowFactor(Texture2D shadowMap, float3 worldPosition, float3 worldNormal, float3 surfaceToLightDirection, matrix viewMatrix, matrix projectionMatrix)
{
    float shadowmapTexelSize = 2.0f / 1024.0f; //Adjusted from (1 / 1024) -> (2 / 1024)
    float4 plv = mul(viewMatrix, float4(worldPosition, 1.0f));
    float shadowFOVFactor = max(projectionMatrix[0].x, projectionMatrix[1].y);
    shadowmapTexelSize *= abs(plv.z) * shadowFOVFactor;
        
    float cosLight = dot(surfaceToLightDirection, worldNormal);
    float slopeScale = saturate(1 - cosLight);
        
    float normalOffsetScale = slopeScale * shadowmapTexelSize;
    float3 normalOffset = float3(worldNormal * normalOffsetScale);
    float3 normalOffsetWorldPosition = worldPosition + normalOffset;
            
    float4 shadowPosition = mul(projectionMatrix, mul(viewMatrix, float4(normalOffsetWorldPosition, 1.f)));
    float3 positionLightSS = shadowPosition.xyz / shadowPosition.w;
    float2 shadowMapTextureCoords = float2(0.5f * positionLightSS.x + 0.5f, -0.5f * positionLightSS.y + 0.5f);
    float actualDepth = positionLightSS.z;
    
    float shadowFactor = 0.0f;
    if (positionLightSS.z > 1.0f || positionLightSS.z < 0.0f)
    {
        shadowFactor = 1.0f;
    }
    else
    {
        [unroll]
        for (int x = -PCF_SAMPLE_COUNT; x <= PCF_SAMPLE_COUNT; x++)
        {
            [unroll]
            for (int y = -PCF_SAMPLE_COUNT; y <= PCF_SAMPLE_COUNT; y++)
            {
                shadowFactor += shadowMap.SampleCmpLevelZero(g_linear_PCF_sampler, shadowMapTextureCoords, actualDepth + SHADOWMAP_CONSTANT_BIAS, int2(x, y));
            }
        }
        shadowFactor /= ((PCF_SAMPLE_COUNT * 2 + 1) * (PCF_SAMPLE_COUNT * 2 + 1));
        shadowFactor = (1 - shadowFactor);
    }
    return shadowFactor;
}

float4 main(VS_OUT input) : SV_TARGET
{    
    // Get per draw data
    ConstantBuffer<PerDrawData> perDrawData = ResourceDescriptorHeap[g_constants.perDrawCB];
    
    if (g_constants.wireframe == 1)
        return float4(0.f, 1.f, 0.f, 1.f);
        
    
    StructuredBuffer<ShaderInterop_GlobalData> gds = ResourceDescriptorHeap[g_constants.gdDescriptor];
    ShaderInterop_GlobalData gd = gds[0];
    
    // Get lights metadata
    StructuredBuffer<ShaderInterop_LightsMetadata> lightsMDs = ResourceDescriptorHeap[gd.lightTableMD];
    ShaderInterop_LightsMetadata lightsMD = lightsMDs[0];
    
    // Get per frame data
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
    
    // Grab normal from normal map if available
    float3 N = normalize(input.nor);
    if (mat.normal != NO_TEXTURE)
        N = normalize(GetFinalNormal(g_aniso_samp, ResourceDescriptorHeap[mat.normal], normalize(input.tan), normalize(input.bitan), normalize(input.nor), input.uv));
    
    
    float3 emissiveInput = mat.emissiveFactor.rgb;
    if (mat.emissive != NO_TEXTURE)
    {
        Texture2D emissive = ResourceDescriptorHeap[mat.emissive];
        emissiveInput *= emissive.Sample(g_aniso_samp, input.uv).rgb;
    }
    
    float3 amb = 0.03f * albedoInput + emissiveInput;
    

    
    // ========= PBR 
    float3 camPos = pfData.camPos;
    float3 V = normalize(camPos - input.wsPos);
    float3 F0 = float3(0.04f, 0.04f, 0.04f);
    F0 = lerp(F0, albedoInput, metallicInput);
    

    //// Get spotlights
    //StructuredBuffer<ShaderInterop_SpotLight> spotlights = ResourceDescriptorHeap[gd.spotLightTable];
    //uint lightID = 0;
    //ShaderInterop_SpotLight spotlight = spotlights[pfData.spotLightOffsets.dynOffset + lightID];

    //float3 lightToPosDir = normalize(input.wsPos - spotlight.position.xyz);
    //float3 lightToPosDist = length(input.wsPos - spotlight.position.xyz);
    //float theta = dot(normalize(spotlight.direction), lightToPosDir);
    
    //float SPOTLIGHT_DISTANCE = 30.f;
    //float distanceFallOffFactor = (1.f - clamp(lightToPosDist, 0.f, SPOTLIGHT_DISTANCE) / SPOTLIGHT_DISTANCE);
    //distanceFallOffFactor *= distanceFallOffFactor;     // quadratic falloff
    
    //float contrib = 0.f;    
    //contrib = distanceFallOffFactor;
    ////if (acos(theta) > spotlight.cutoffAngle * 3.1415f / 180.f)
    ////{
    ////    contrib = 0.0f;
    ////}
    ////else
    ////    contrib = 1.f * distanceFallOffFactor;
    
    
    
    float3 Lo = float3(0.f, 0.f, 0.f);
    
    // calculate static point lights
    StructuredBuffer<ShaderInterop_PointLight> pointLights = ResourceDescriptorHeap[gd.pointLightTable];
    for (int i = 0; i < lightsMD.staticPointLightRange.count; ++i)
    {
        ShaderInterop_PointLight pointLight = pointLights[pfData.pointLightOffsets.staticOffset + i];
        if (pointLight.strength == 0)
            continue;

        // calculate per-light radiance
        float3 L = normalize(pointLight.position.xyz - input.wsPos);
        float3 H = normalize(V + L);
        float distance = length(pointLight.position.xyz - input.wsPos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = pointLight.color.xyz * attenuation * pointLight.strength;
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughnessInput);
        float G = GeometrySmith(N, V, L, roughnessInput);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // energy conservation (diff/spec)
        float3 kS = F;
        float3 kD = float3(1.f, 1.f, 1.f) - kS;
        kD *= 1.0 - metallicInput;
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedoInput / 3.1415f + specular) * radiance * NdotL;
        
    }

    // calculate dyn point lights
    for (int i = 0; i < lightsMD.dynPointLightRange.count; ++i)
    {
        ShaderInterop_PointLight pointLight = pointLights[pfData.pointLightOffsets.dynOffset + i];
        if (pointLight.strength == 0)
            continue;

        // calculate per-light radiance
        float3 L = normalize(pointLight.position.xyz - input.wsPos);
        float3 H = normalize(V + L);
        float distance = length(pointLight.position.xyz - input.wsPos);
        float attenuation = 1.0 / (distance * distance);
        float3 radiance = pointLight.color.xyz * attenuation * pointLight.strength;
        
        // cook-torrance brdf
        float NDF = DistributionGGX(N, H, roughnessInput);
        float G = GeometrySmith(N, V, L, roughnessInput);
        float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // energy conservation (diff/spec)
        float3 kS = F;
        float3 kD = float3(1.f, 1.f, 1.f) - kS;
        kD *= 1.0 - metallicInput;
        
        float3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        float3 specular = numerator / max(denominator, 0.001);
            
        // add to outgoing radiance Lo
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedoInput / 3.1415f + specular) * radiance * NdotL;
    }
    
    // calculate static spot lights
    //StructuredBuffer<ShaderInterop_SpotLight> spotLights = ResourceDescriptorHeap[gd.spotLightTable];
    //for (int i = 0; i < lightsMD.staticSpotLightRange.count; ++i)
    //{
    //    ShaderInterop_SpotLight spotLight = spotLights[pfData.spotLightOffsets.staticOffset + i];
    //    
    //    // check contribution from based on spotlight angle
    //    float3 lightToPosDir = normalize(input.wsPos - spotLight.position.xyz);
    //    float3 lightToPosDist = length(input.wsPos - spotLight.position.xyz);
    //    float theta = dot(normalize(spotLight.direction), lightToPosDir);
    //
    //    float contrib = 0.f;
    //    if (acos(theta) > spotLight.cutoffAngle * 3.1415f / 180.f)
    //        contrib = 0.0f;
    //    else
    //    {
    //        contrib = 1.f;
    //    }
    //    
    //    // calculate per-light radiance
    //    float3 L = normalize(spotLight.position.xyz - input.wsPos);
    //    float3 H = normalize(V + L);
    //    float3 radiance = spotLight.color * spotLight.strength;
    //    
    //    // cook-torrance brdf
    //    float NDF = DistributionGGX(N, H, roughnessInput);
    //    float G = GeometrySmith(N, V, L, roughnessInput);
    //    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    //    
    //    float3 kS = F;
    //    float3 kD = float3(1.f, 1.f, 1.f) - kS;
    //    kD *= 1.0 - metallicInput;
    //    
    //    float3 numerator = NDF * G * F;
    //    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    //    float3 specular = numerator / max(denominator, 0.001);
    //   
    //    // add to outgoing radiance Lo
    //    float NdotL = max(dot(N, L), 0.0);
    //    Lo += (kD * albedoInput / 3.1415 + specular) * radiance * NdotL * (contrib);
    //}
    
    //calculate dynamic spot lights
    ConstantBuffer<SpotlightMetaData> perSpotlightData = ResourceDescriptorHeap[g_constants.spotlightArrayStructureIndex];
    ConstantBuffer<Shadow> shadowMapArrayStruct = ResourceDescriptorHeap[g_constants.shadowMapDepthIndex];
    
    for (int k = 0; k < perSpotlightData.currentNrOfSpotlights; ++k)
    {
        if (perSpotlightData.spotlightArray[k].strength == 0)
            continue;
        // check contribution from based on spotlight angle
        float3 lightToPosDir = normalize(input.wsPos - perSpotlightData.spotlightArray[k].worldPosition.xyz);
        float theta = dot(normalize(perSpotlightData.spotlightArray[k].direction), lightToPosDir);
    
        float contrib = 0.f;
        if (acos(theta) > perSpotlightData.spotlightArray[k].cutoffAngle * 3.1415f / 180.f)
            contrib = 0.0f;
        else
            contrib = 1.f;
        
        // calculate per-light radiance
        float3 L = normalize(perSpotlightData.spotlightArray[k].worldPosition.xyz - input.wsPos);
        float3 H = normalize(V + L);
        float3 radiance = perSpotlightData.spotlightArray[k].color * perSpotlightData.spotlightArray[k].strength;
        
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
        
        const uint groupIndex = k / 4;
        const uint offsetInGroup = k;
        
        Texture2D shadowMap = ResourceDescriptorHeap[shadowMapArrayStruct.shadowMapArray[groupIndex][offsetInGroup]];
        float shadowFactor = CalculateShadowFactor(shadowMap, input.wsPos, N, -lightToPosDir, perSpotlightData.spotlightArray[k].viewMatrix, perSpotlightData.spotlightArray[k].projectionMatrix);
        Lo += (kD * albedoInput / 3.1415 + specular) * radiance * NdotL * contrib * shadowFactor;
    }
    
    // add temp directional light
    //{
    //    // calculate per-light radiance
    //    float3 L = normalize(-float3(-1.f, -1.f, 1.f));
    //    float3 H = normalize(V + L);
    //    float3 radiance = float3(1.f, 1.f, 1.f); // no attenuation
    
    //    // cook-torrance brdf
    //    float NDF = DistributionGGX(N, H, roughnessInput);
    //    float G = GeometrySmith(N, V, L, roughnessInput);
    //    float3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);
    
    //    float3 kS = F;
    //    float3 kD = float3(1.f, 1.f, 1.f) - kS;
    //    kD *= 1.0 - metallicInput;
    
    //    float3 numerator = NDF * G * F;
    //    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
    //    float3 specular = numerator / max(denominator, 0.001);
    
    
    //    // add to outgoing radiance Lo
    //    float NdotL = max(dot(N, L), 0.0);
    //    Lo += (kD * albedoInput / 3.1415 + specular) * radiance * NdotL;
    //}
    
    float3 hdr = amb + Lo;
    return float4(hdr, albedoAlpha);
    
}


