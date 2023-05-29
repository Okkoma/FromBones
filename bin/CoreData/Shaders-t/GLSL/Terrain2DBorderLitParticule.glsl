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
    vec3 weights = texture2D(sWeightMap0, vMapTexCoord).rgb;
    float sumWeights = weights.r + weights.g + weights.b;
    weights /= sumWeights;

    // Mix terrains
    vec4 diffColor = weights.r * texture2D(sDetailMap1, vTexCoord)
                   + weights.g * texture2D(sDetailMap2, vTexCoord)
                   + weights.b * texture2D(sDetailMap3, vTexCoord);

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
