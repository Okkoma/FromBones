#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec2 vMapTexCoord;
varying vec4 vColor;


#ifndef GL_ES
uniform float cTerrainTileSize;
uniform float cTerrainOffset;
uniform vec2 cMapWorldSize;
#else
uniform mediump float cTerrainTileSize;
uniform mediump float cTerrainOffset;
uniform mediump vec2 cMapWorldSize;
#endif

// DONT WORK ON RPI
//vec4 fourTapSample(sampler2D atlas, vec2 texcoord, vec2 tileoffset)
//{
//    // Initialize accumulators
//    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
//    float totalWeight = 0.0;
//
//    for (int dx=0; dx<2; ++dx)
//    for (int dy=0; dy<2; ++dy)
//    {
//        // Compute coordinate in 2x2 tile patch
//        vec2 tilecoord = fract(texcoord + 0.7*vec2(dx, dy));
//
//        // Weight sample based on distance to center
//        float w = pow(0.5 - max(abs(tilecoord.x-0.5), abs(tilecoord.y-0.5)), 24.0);
//        // Sample and accumulate
//        color += w * texture2D(atlas, tileoffset + tilecoord * cTerrainTileSize);
//        totalWeight += w;
//    }
//
//    // Return weighted color
//    return color / totalWeight;
//}

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);
    vTexCoord = vec2(iTexCoord.x, 1.0 - iTexCoord.y);
    vMapTexCoord = worldPos.xy / cMapWorldSize;
    vColor = iColor;
}

void PS()
{
    // Get terrain Map Weights
    vec3 weights = texture2D(sNormalMap, vMapTexCoord).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;

    // Reduce Texture Coord to the tile Coordinate
    vec2 tilecoord = cTerrainTileSize * fract(vTexCoord);

//#ifndef GL_ES
//    // Get explicit gradient at the TextureCoord to use in textureGrad (to prevent seam with TEXTURE_REPEAT)
//    vec2 fdx = dFdx(vTexCoord);
//    vec2 fdy = dFdy(vTexCoord);

//    // Mix terrains
//    vec4 diffColor = weights.r * textureGrad(sDiffMap, tilecoord, fdx, fdy)
//                   + weights.g * textureGrad(sDiffMap, tilecoord + vec2(cTerrainOffset, 0), fdx, fdy)
//                   + weights.b * textureGrad(sDiffMap, tilecoord + vec2(0, cTerrainOffset), fdx, fdy);
//#else
    vec4 diffColor = weights.r * texture2D(sDiffMap, tilecoord + vec2(0, 0))
                   + weights.g * texture2D(sDiffMap, tilecoord + vec2(cTerrainOffset, 0))
                   + weights.b * texture2D(sDiffMap, tilecoord + vec2(0, cTerrainOffset));
//#endif
    gl_FragColor = vColor*diffColor;
}
