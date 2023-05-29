#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"

uniform float2 cNumTiles;
uniform float2 cTileIndex;

void VS(float4 iPos : POSITION,
		float2 iTexCoord : TEXCOORD0,
		float4 iColor : COLOR0,
		out float4 oColor : COLOR0,
		out float2 oTexCoord : TEXCOORD0,
		out float4 oPos : OUTPOSITION)
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
	oTexCoord = GetTexCoord(iTexCoord);
    oColor = iColor;
}

void PS(float4 iColor : COLOR0,
        float2 iTexCoord : TEXCOORD0,
        out float4 oColor : OUTCOLOR0)
{
	oColor = iColor * Sample2D(DiffMap, (cTileIndex / cNumTiles) + (1.0 / cNumTiles) * frac(iTexCoord));
}