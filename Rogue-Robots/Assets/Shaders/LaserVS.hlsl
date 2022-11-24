#include "ShaderInterop_Renderer.h"
#include "ShaderInterop_Mesh.h"

struct PushConstantElement
{
	uint gdDescriptor;
	uint perFrameOffset;
};

ConstantBuffer<PushConstantElement> g_constants : register(b0, space0);

struct LaserBeamBuffer
{
	matrix worldMatrix;
	float3 color;
	float length;
};
ConstantBuffer<LaserBeamBuffer> laserBeamCB : register(b1, space0);

struct VS_OUT
{
	float4 position : SV_Position;
	float2 uv : TEXCOORD;
	float3 color : COLOR;
};

static const float2 uvs[6] = { float2(0, 0), float2(1, 0), float2(0, 1), float2(1, 0), float2(1, 1), float2(0, 1) };


VS_OUT main(uint vertexID : SV_VERTEXID)
{
	StructuredBuffer<ShaderInterop_GlobalData> gds = ResourceDescriptorHeap[g_constants.gdDescriptor];
	ShaderInterop_GlobalData gd = gds[0];
    
	StructuredBuffer<ShaderInterop_PerFrameData> pfDatas = ResourceDescriptorHeap[gd.perFrameTable];
	ShaderInterop_PerFrameData pfData = pfDatas[g_constants.perFrameOffset];
	
	VS_OUT output;
	output.uv = uvs[vertexID];
	output.position = float4(output.uv.x * 2.0f - 1.0f, 0.0f, -output.uv.y, 1.0f);
	output.position.x *= 0.05f;
	output.position.z *= laserBeamCB.length;
	output.color = laserBeamCB.color;
	
	output.position = mul(laserBeamCB.worldMatrix, float4(output.position.xyz, 1));
	output.position = mul(pfData.viewMatrix, float4(output.position.xyz, 1));
	output.position = mul(pfData.projMatrix, float4(output.position.xyz, 1));
	
	return output;
}