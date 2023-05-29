#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec2 vMapTexCoord;
varying vec4 vColor;

uniform sampler2D sWeightMap0;
uniform sampler2D sDetailMap1;
uniform sampler2D sDetailMap2;
uniform sampler2D sDetailMap3;

#ifndef GL_ES
uniform vec2 cMapWorldSize;
#else
uniform mediump vec2 cMapWorldSize;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);

    vTexCoord = iTexCoord;

    vMapTexCoord = worldPos.xy / cMapWorldSize;
    vColor = iColor;
}

void PS()
{
    // Get terrain Map Weights
    vec3 weights = texture2D(sWeightMap0, vMapTexCoord).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;

    // Mix terrains
    vec4 diffColor  = weights.r * texture2D(sDetailMap1, vTexCoord)
                    + weights.g * texture2D(sDetailMap2, vTexCoord)
                    + weights.b * texture2D(sDetailMap3, vTexCoord);

    gl_FragColor = vColor * diffColor;
}
