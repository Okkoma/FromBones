#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "Lighting.glsl"
#include "Fog.glsl"

varying vec2 vTexCoord;
varying vec2 vMapTexCoord;
varying vec4 vWorldPos;
varying vec4 vColor;

#ifdef PERPIXEL
    #ifdef SHADOW
        varying vec4 vShadowPos[NUMCASCADES];
    #endif
    #ifdef SPOTLIGHT
        varying vec4 vSpotPos;
    #endif
    #ifdef POINTLIGHT
        varying vec3 vCubeMaskVec;
    #endif
#else
    varying vec3 vVertexLight;
#endif

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
    vWorldPos = vec4(worldPos, GetDepth(gl_Position));
    vColor = iColor;

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        vec4 projWorldPos = vec4(worldPos, 1.0);

        #ifdef SHADOW
            // Shadow projection: transform from world space to shadow space
            for (int i = 0; i < NUMCASCADES; i++)
                vShadowPos[i] = GetShadowPos(i, vec3(0, 0, 0), projWorldPos);
        #endif

        #ifdef SPOTLIGHT
            // Spotlight projection: transform from world space to projector texture coordinates
            vSpotPos = projWorldPos * cLightMatrices[0];
        #endif

        #ifdef POINTLIGHT
            vCubeMaskVec = (worldPos - cLightPos.xyz) * mat3(cLightMatrices[0][0].xyz, cLightMatrices[0][1].xyz, cLightMatrices[0][2].xyz);
        #endif
    #else
        // Ambient & per-vertex lighting
        vVertexLight = GetAmbient(GetZonePos(worldPos));

        #ifdef NUMVERTEXLIGHTS
            for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
                vVertexLight += GetVertexLightVolumetric(i, worldPos) * cVertexLights[i * 3].rgb;
        #endif
    #endif
}

void PS()
{
    // Get terrain Map Weights
    vec3 weights = texture2D(sNormalMap, vMapTexCoord).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;

    // Reduce Texture Coord to the tile Coordinate
    vec2 tilecoord = cTerrainTileSize * fract(vTexCoord);

#ifndef GL_ES
    // Get explicit gradient at the TextureCoord to use in textureGrad (to prevent seam with TEXTURE_REPEAT)
    vec2 fdx = dFdx(vTexCoord);
    vec2 fdy = dFdy(vTexCoord);

    // Mix terrains
    vec4 diffColor = weights.r * textureGrad(sDiffMap, tilecoord, fdx, fdy)
                   + weights.g * textureGrad(sDiffMap, tilecoord + vec2(cTerrainOffset, 0), fdx, fdy)
                   + weights.b * textureGrad(sDiffMap, tilecoord + vec2(0, cTerrainOffset), fdx, fdy);
#else
    vec4 diffColor = weights.r * texture2D(sDiffMap, tilecoord + vec2(0, 0))
                   + weights.g * texture2D(sDiffMap, tilecoord + vec2(cTerrainOffset, 0))
                   + weights.b * texture2D(sDiffMap, tilecoord + vec2(0, cTerrainOffset));
#endif

    #ifdef VERTEXCOLOR
        diffColor *= vColor;
    #endif

    // Get fog factor
    #ifdef HEIGHTFOG
        float fogFactor = GetHeightFogFactor(vWorldPos.w, vWorldPos.y);
    #else
        float fogFactor = GetFogFactor(vWorldPos.w);
    #endif

    #ifdef PERPIXEL
        // Per-pixel forward lighting
        vec3 lightColor;
        vec3 lightDir;
        vec3 finalColor;

        float diff = GetDiffuseVolumetric(vWorldPos.xyz);

        #ifdef SHADOW
            diff *= GetShadow(vShadowPos, vWorldPos.w);
        #endif

        #if defined(SPOTLIGHT)
            lightColor = vSpotPos.w > 0.0 ? texture2DProj(sLightSpotMap, vSpotPos).rgb * cLightColor.rgb : vec3(0.0, 0.0, 0.0);
        #elif defined(CUBEMASK)
            lightColor = textureCube(sLightCubeMap, vCubeMaskVec).rgb * cLightColor.rgb;
        #else
            lightColor = cLightColor.rgb;
        #endif

        finalColor = diff * lightColor * diffColor.rgb;
        gl_FragColor = vec4(GetLitFog(finalColor, fogFactor), diffColor.a);
    #else
        // Ambient & per-vertex lighting
        vec3 finalColor = vVertexLight * diffColor.rgb;

        gl_FragColor = vec4(GetFog(finalColor, fogFactor), diffColor.a);
    #endif
}
