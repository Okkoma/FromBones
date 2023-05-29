#define Urho2DSamplers

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "PostProcess.glsl"
#include "FXAA2n.glsl"

#ifdef VERTEXLIGHT
#include "Lighting.glsl"
#endif

varying vec4 vColor;
varying vec2 vTexCoord;

#ifdef VERTEXLIGHT
varying vec3 vVertexLight;
#endif

varying float fTextureId;
varying float fTexEffect1;
varying float fTexEffect2;
varying float fTexEffect3;

/*
    /// fTexEffect1 Values ///
    // LIT   = 0
    // UNLIT = 1

    /// fTexEffect2 Values ///
    // CROPALPHA = 1
    // BLUR = 2
    // FXAA = 4

    /// fTexEffect2 Combinaisons ///
    // CROPALPHA        = 1
    // CROPALPHA + BLUR = 3
    // CROPALPHA + FXAA = 5
    // BLUR             = 2
    // BLUR + CROPALPHA = 3
    // FXAA             = 4
    // FXAA + CROPALPHA = 5

    /// fTexEffect3 Values ///
    // TILE(0,0) = 0
    // TILE(0,1) = 1
    // TILE(1,0) = 2
    // TILE(1,1) = 3
*/

void VS()
{
    vec3 worldPos = vec3((iPos * iModelMatrix).xy, 1.0);
    gl_Position = GetClipPos(worldPos);
    
    vTexCoord = iTexCoord;
    vColor = iColor;
	fTextureId   = iTangent.x;
    fTexEffect1  = iTangent.y; // unlit
    fTexEffect2  = iTangent.z; // other effects (cropalpha, blur, fxaa2)
    fTexEffect3  = iTangent.w; // tile index

#ifdef VERTEXLIGHT
    // Ambient & per-vertex lighting
    vVertexLight = GetAmbient(GetZonePos(worldPos));
    
    // Lit Cases
    if (fTexEffect1 == 0.0)
    {
        #ifdef NUMVERTEXLIGHTS
        for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
            vVertexLight += GetVertexLightVolumetric(i, worldPos) * cVertexLights[i * 3].rgb;
        #endif
    }
#endif
}

#ifdef COMPILEPS
// remove the variation
float Round(float a)
{
    float f = fract(a);
    a -= f;
    if (f > 0.5)
        return a + 1.0;
    return a;
}

vec4 ApplyTextureEffects(sampler2D texSampler)
{
    vec4 value = texture2D(texSampler, vTexCoord);

    float effect = Round(fTexEffect2);

    // BLUR=4
    //if (effect == 2.0 || effect == 3.0)
    //    value *= GaussianBlur(5, vec2(1.0, 1.0), cGBufferInvSize.xy * 2.0, 2.0, texSampler, vTexCoord);
    // FXAA=8
    //else if (effect == 4.0 || effect ==5.0)
    //    value *= FXAA2(texSampler, vTexCoord);

    // CROPALPHA cases
    if (effect == 1.0 || effect == 3.0 || effect == 5.0)
    {
        const float minval = 0.13;
        if (value.r < minval && value.g < minval && value.b > 1.0-minval)
            value.a = (value.r + value.g + 1.0-value.b) / 3.0;
    }

#ifdef VERTEXLIGHT
    vec4 litColor;

    // USE LIGHT (by default)
    if (fTexEffect1 < 1.0)
        litColor = vec4(vVertexLight, 1.0);
    // UNLIT
    else
        litColor = vec4(1.0);

    value *= litColor;
#endif

    return value;
}
#endif

void PS()
{
    vec4 diffInput;

    float tid = Round(fTextureId);

    if (tid == 0.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[0]);
    else if (tid == 1.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[1]);
    else if (tid == 2.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[2]);
    else if (tid == 3.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[3]);
    else if (tid == 4.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[4]);
    else if (tid == 5.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[5]);
    else if (tid == 6.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[6]);
    if (tid == 7.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[7]);

    gl_FragColor = cMatDiffColor * vColor * diffInput;

// #ifdef GL_ES
//     // apply gamma correction
//     float gamma = 1.5;
//     gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0/gamma));
// #endif
}
