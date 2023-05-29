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

vec4 ApplyBackShape(sampler2D texSampler)
{
    // InnerLayer (0,0)
    if (fTexEffect3 < 1.0)
        return texture2D(texSampler, vec2(0.5, 0.5) * fract(vTexCoord));
    // BackGroundLayer (0,1)
    else if (fTexEffect3 < 2.0)
        return texture2D(texSampler, vec2(0.0, 0.5) + vec2(0.5, 0.5) * fract(vTexCoord));
    // BackViewLayer (1,0)
    else if (fTexEffect3 < 3.0)
        return texture2D(texSampler, vec2(0.5, 0.0) + vec2(0.5, 0.5) * fract(vTexCoord));
    // RockLayer (1,1)
    return texture2D(texSampler, vec2(0.5, 0.5) + vec2(0.5, 0.5) * fract(vTexCoord));
}

vec4 ApplyTextureEffects(sampler2D texSampler)
{
    vec4 value = fTextureId == 0.0 ? ApplyBackShape(texSampler) : texture2D(texSampler, vTexCoord);

    // LIT or UNLIT=1

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

// remove the interpolation
float Round(float a)
{
    float f = fract(a);
    a -= f;
    if (f > 0.5)
        return a + 1.0;
    return a;
}

#endif

void PS()
{
	vec4 diffInput;

    float tid = Round(fTextureId);

    if (tid == 3.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[3]);
    else if (tid == 2.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[2]);
    else if (tid == 1.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[1]);
    else
        diffInput = ApplyTextureEffects(sUrho2DTextures[0]);

    gl_FragColor = vColor * diffInput;

//#ifdef GL_ES
//    // apply gamma correction
//    float gamma = 1.5;
//    gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0/gamma));
//#endif    
}
