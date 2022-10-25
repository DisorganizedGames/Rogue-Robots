#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Material.h"
#include "ShaderInterop_Samplers.hlsli"
#include "ShaderInterop_Math.hlsli"
#include "ShaderInterop_Lights.hlsli"
#include "PBRHelpers.hlsli"

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float4 shadowPos : SHADOW_POSITION;
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

struct PerLightDataForShadows
{
    PerLightData lightData[12];
    uint actualNrOfSpotlights;
};

struct Shadow
{
    uint shadowMaps[12];
};

struct PushConstantElement
{
    uint gdDescriptor;
    uint perFrameOffset;
    
    uint perDrawCB;
    uint wireframe;
    uint perDrawLight;
    uint depth;
};

CONSTANTS(g_constants, PushConstantElement)

static const uint NO_TEXTURE = 0xffffffff;

float CalculateShadowFactor(Texture2D depth, float3 worldPosition, float3 worldNormal, float3 surfaceToLightDir, matrix view, matrix proj)
{
    float shadowmapTexelSize = 2.0f / 1024.0f;
    float4 plv = mul(view, float4(worldPosition, 1.0f));
    float shadowFOVFactor = max(proj[0].x, proj[1].y);
    shadowmapTexelSize *= abs(plv.z) * shadowFOVFactor;
        
    float cosLight = dot(surfaceToLightDir, worldNormal);
    float slopeScale = saturate(1 - cosLight);
        
    float normalOffsetScale = slopeScale * shadowmapTexelSize;
        
    float3 normalOffset = float3(worldNormal * normalOffsetScale);
    float3 shadowPositionTemp = worldPosition + normalOffset;
            
    float4 shadowPosition = mul(proj, mul(view, float4(shadowPositionTemp, 1.f)));
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
        for (int x = -2; x <= 2; x++)
        {
            [unroll]
            for (int y = -2; y <= 2; y++)
            {
                shadowFactor += depth.SampleCmpLevelZero(g_linear_PCF_sampler, shadowMapTextureCoords, actualDepth + 0.0001f, int2(x, y));
            }
        }
        shadowFactor /= (5 * 5);
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
    ConstantBuffer<PerLightDataForShadows> perLightData = ResourceDescriptorHeap[g_constants.perDrawLight];
    ConstantBuffer<Shadow> shadowMaps = ResourceDescriptorHeap[g_constants.depth];
    for (int k = 0; k < perLightData.actualNrOfSpotlights; ++k)
    {
        if (perLightData.lightData[k].strength == 0)
            continue;
        // check contribution from based on spotlight angle
            float3 lightToPosDir = normalize(input.wsPos - perLightData.lightData[k].position.xyz);
        float theta = dot(normalize(perLightData.lightData[k].direction), lightToPosDir);
    
        float contrib = 0.f;
        if (acos(theta) > perLightData.lightData[k].cutoffAngle * 3.1415f / 180.f)
            contrib = 0.0f;
        else
            contrib = 1.f;
        
        // calculate per-light radiance
        float3 L = normalize(perLightData.lightData[k].position.xyz - input.wsPos);
        float3 H = normalize(V + L);
        float3 radiance = perLightData.lightData[k].color * perLightData.lightData[k].strength;
        
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
        
        //float shadowFactor = 1.0f;
        //if (k == 0)
        //{
        //    Texture2D shadowMap = ResourceDescriptorHeap[g_constants.depth1];
        //    shadowFactor = CalculateShadowFactor(shadowMap, input.wsPos, N, -lightToPosDir, perLightData.lightData[k].view, perLightData.lightData[k].proj);
        //}
        //else if (k == 1)
        //{
        //    Texture2D shadowMap = ResourceDescriptorHeap[g_constants.depth2];
        //    shadowFactor = CalculateShadowFactor(shadowMap, input.wsPos, N, -lightToPosDir, perLightData.lightData[k].view, perLightData.lightData[k].proj);
        //}
        
        
        //Texture2D shadowMap = ResourceDescriptorHeap[NonUniformResourceIndex(shadowMaps.shadowMaps[k])];
        Texture2D shadowMap = ResourceDescriptorHeap[shadowMaps.shadowMaps[k]];
        float shadowFactor = CalculateShadowFactor(shadowMap, input.wsPos, N, -lightToPosDir, perLightData.lightData[k].view, perLightData.lightData[k].proj);
        Lo += (kD * albedoInput / 3.1415 + specular) * radiance * NdotL * (contrib) * shadowFactor;
    }
    
    //Texture2D sm = ResourceDescriptorHeap[g_constants.depth1];
    
    
    //return float4(sm.Sample(g_point_samp, 0.5.rr).rrr, 1.f);
    
    
    // calculate dynamic spot lights
    //for (int i = 0; i < perLightData.actualNrOfSpotlights; ++i)
    //{
    //    ShaderInterop_SpotLight spotLight = spotLights[pfData.spotLightOffsets.dynOffset + i];
    //    
    //    
    //    
    //    // check contribution from based on spotlight angle
    //    float3 lightToPosDir = normalize(input.wsPos - spotLight.position.xyz);
    //    float theta = dot(normalize(spotLight.direction), lightToPosDir);
    //
    //    float contrib = 0.f;
    //    if (acos(theta) > spotLight.cutoffAngle * 3.1415f / 180.f)
    //        contrib = 0.0f;
    //    else
    //        contrib = 1.f;
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
    //    
    //    Texture2D shadowMap;
    //    float shadowFactor = 1.0f;
    //    if (i == 1)
    //        shadowMap = ResourceDescriptorHeap[g_constants.depth2];
    //    else
    //        shadowMap = ResourceDescriptorHeap[g_constants.depth1];
    //    
    //    shadowFactor = CalculateShadowFactor(shadowMap, input.wsPos, N, -lightToPosDir, perLightData.lightData[i].view, perLightData.lightData[i].proj);
    // 
    //    Lo += (kD * albedoInput / 3.1415 + specular) * radiance * NdotL * (contrib) * shadowFactor;
    //}
        
    float3 hdr = amb + Lo;
    return float4(hdr, albedoAlpha);
    
}


