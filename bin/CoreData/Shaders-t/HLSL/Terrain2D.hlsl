#include "Uniforms.hlsl"
#include "Samplers.hlsl"
#include "Transform.hlsl"

#ifndef D3D11

// D3D9 uniforms and samplers
#ifdef COMPILEVS
uniform float4 cTerrainTile;
uniform float2 cMapWorldSize;
#else
sampler2D sWeightMap0 : register(s0);
sampler2D sDetailMap1 : register(s1);
sampler2D sDetailMap2 : register(s2);
sampler2D sDetailMap3 : register(s3);
#endif

#else

// D3D11 constant buffers and samplers
#ifdef COMPILEVS
cbuffer CustomVS : register(b6)
{
	float4 cTerrainTile;
    float2 cMapWorldSize;
}
#else
Texture2D tWeightMap0 : register(t0);
Texture2D tDetailMap1 : register(t1);
Texture2D tDetailMap2 : register(t2);
Texture2D tDetailMap3 : register(t3);
SamplerState sWeightMap0 : register(s0);
SamplerState sDetailMap1 : register(s1);
SamplerState sDetailMap2 : register(s2);
SamplerState sDetailMap3 : register(s3);
#endif

#endif

void VS(float4 iPos : POSITION,
    float2 iTexCoord : TEXCOORD0,
    float4 iColor : COLOR0,
    out float4 oPos : OUTPOSITION,    
    out float2 oTexCoord : TEXCOORD0,
    out float2 oMapTexCoord : TEXCOORD1,
    out float4 oColor : COLOR0
    )
{
    float4x3 modelMatrix = iModelMatrix;
    float3 worldPos = GetWorldPos(modelMatrix);
    oPos = GetClipPos(worldPos);
    
    oTexCoord = iTexCoord;
    oMapTexCoord = worldPos.xy / cMapWorldSize;
    oColor = iColor;    
}

void PS(float4 iColor : COLOR0,
        float2 iTexCoord : TEXCOORD0,
        float2 iMapTexCoord : TEXCOORD1,
        out float4 oColor : OUTCOLOR0)
{
    // Get terrain Map Weights
    float3 weights = Sample2D(WeightMap0, iMapTexCoord).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;
    
    // Mix terrains
    // Reduce Texture Coord to the tile Coordinate
    // the position of the tile terrain in the texture is cTerrainTile.xy
    // the size of the tile terrains in the texture is cTerrainTile.zw
    float2 tilecoord = cTerrainTile.xy + cTerrainTile.zw * frac(iTexCoord);
        
    float4 diffColor = weights.r * Sample2D(DetailMap1, tilecoord)
        			 + weights.g * Sample2D(DetailMap2, tilecoord)
        			 + weights.b * Sample2D(DetailMap3, tilecoord);
    
    oColor = iColor * diffColor;
}

