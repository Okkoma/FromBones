#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec2 vMapTexCoord;
varying vec4 vColor;

#ifndef GL_ES
uniform float cTileOffset;
uniform vec2 cMapWorldSize;
#else
uniform mediump float cTileOffset;
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
    vec3 weights = texture2D(sNormalMap, vMapTexCoord).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;

    // Mix terrains
    vec4 diffColor = weights.r * texture2D(sDiffMap, vTexCoord)
                   + weights.g * texture2D(sDiffMap, vTexCoord + vec2(cTileOffset, 0.0))
                   + weights.b * texture2D(sDiffMap, vTexCoord + vec2(0.0, cTileOffset));

    gl_FragColor = vColor * diffColor;
}
