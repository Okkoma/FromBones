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
uniform vec4 cTerrainTile;
uniform vec2 cMapWorldSize;
#else
uniform mediump vec4 cTerrainTile;
uniform mediump vec2 cMapWorldSize;
#endif

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);

    vTexCoord = vec2(iTexCoord.x, 1.0-iTexCoord.y);
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
    // Reduce Texture Coord to the tile Coordinate
    // the position of the tile terrain in the texture is cTerrainTile.xy
    // the size of the tile terrains in the texture is cTerrainTile.zw
    vec2 tilecoord = cTerrainTile.xy + cTerrainTile.zw * fract(vTexCoord);

#ifndef GL_ES
    // Get explicit gradient at the TextureCoord to use in textureGrad
    vec2 fdx = dFdx(vTexCoord);
    vec2 fdy = dFdy(vTexCoord);

    vec4 diffColor  = weights.r * textureGrad(sDetailMap1, tilecoord, fdx, fdy)
                    + weights.g * textureGrad(sDetailMap2, tilecoord, fdx, fdy)
                    + weights.b * textureGrad(sDetailMap3, tilecoord, fdx, fdy);
#else
    vec4 diffColor = weights.r * texture2D(sDetailMap1, tilecoord)
                   + weights.g * texture2D(sDetailMap2, tilecoord)
                   + weights.b * texture2D(sDetailMap3, tilecoord);
#endif
    gl_FragColor = vColor * diffColor;
}
