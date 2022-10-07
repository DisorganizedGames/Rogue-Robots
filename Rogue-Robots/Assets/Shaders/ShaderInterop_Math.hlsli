#pragma once

static const float PI = 3.1415f;

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
