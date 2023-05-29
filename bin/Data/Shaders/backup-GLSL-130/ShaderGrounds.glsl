#version 130

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

flat varying int fTexId;
flat varying uint fTexEffect;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    gl_Position = GetClipPos(worldPos);

    vTexCoord = iTexCoord;
    vColor = iColor;
	fTexId = int(iTexCoord1.x);
    fTexEffect = uint(iTexCoord1.y);

#ifdef VERTEXLIGHT
    if ((fTexEffect & uint(2)) != uint(2))
    {
        // Ambient & per-vertex lighting
        vVertexLight = GetAmbient(GetZonePos(worldPos));

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
    //value = texture2D(texSampler, (cTileIndex / cNumTiles) + (1.0 / cNumTiles) * fract(vTexCoord));
    // extract tileindex on bits 4 & 5
    int tileindex = int(fTexEffect >> 4);
    // InnerLayer (0,0)
    if (tileindex == 0)
        return texture2D(texSampler, vec2(0.5, 0.5) * fract(vTexCoord));
    // BackGroundLayer (0,1)
    else if (tileindex == 1)
        return texture2D(texSampler, vec2(0, 0.5) + vec2(0.5, 0.5) * fract(vTexCoord));
    // BackViewLayer (1,0)
    else if (tileindex == 2)
        return texture2D(texSampler, vec2(0.5, 0) + vec2(0.5, 0.5) * fract(vTexCoord));
    // RockLayer (1,1)
    return texture2D(texSampler, vec2(0.5, 0.5) + vec2(0.5, 0.5) * fract(vTexCoord));
}

vec4 ApplyTextureEffects(sampler2D texSampler)
{
    vec4 value = fTexId == 0 ? ApplyBackShape(texSampler) : texture2D(texSampler, vTexCoord);

    // BLUR=4
    if ((fTexEffect & uint(4)) == uint(4))
        value *= GaussianBlur(5, vec2(1.0, 1.0), cGBufferInvSize.xy * 2.0, 2.0, texSampler, vTexCoord);
    // FXAA=8
    else if ((fTexEffect & uint(8)) == uint(8))
        value *= FXAA2(texSampler, vTexCoord);

    // CROPALPHA=1
    if ((fTexEffect & uint(1)) == uint(1))
    {
        const float minval = 0.13;
        if (value.r < minval && value.g < minval && value.b > 1-minval)
            value.a = (value.r + value.g + 1.0-value.b) / 3.0;
    }

    // LIT or UNLIT=2
#ifdef VERTEXLIGHT
    vec4 litColor;
    // UNLIT
    if ((fTexEffect & uint(2)) == uint(2))
        litColor = vec4(1);
    // USE LIGHT (by default)
    else
        litColor = vec4(vVertexLight,1);

    value *= litColor;
#endif

    return value;
}
#endif

void PS()
{
    vec4 diffInput;

	switch(fTexId)
	{
        case -1: diffInput = vec4(1); break;
		case  0: diffInput = ApplyTextureEffects(sUrho2DTextures[ 0]); break;
        case  1: diffInput = ApplyTextureEffects(sUrho2DTextures[ 1]); break;
        case  2: diffInput = ApplyTextureEffects(sUrho2DTextures[ 2]); break;
        case  3: diffInput = ApplyTextureEffects(sUrho2DTextures[ 3]); break;
        case  4: diffInput = ApplyTextureEffects(sUrho2DTextures[ 4]); break;
        case  5: diffInput = ApplyTextureEffects(sUrho2DTextures[ 5]); break;
        case  6: diffInput = ApplyTextureEffects(sUrho2DTextures[ 6]); break;
        case  7: diffInput = ApplyTextureEffects(sUrho2DTextures[ 7]); break;
	}

    gl_FragColor = cMatDiffColor * vColor * diffInput;
}
