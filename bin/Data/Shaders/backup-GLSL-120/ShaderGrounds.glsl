#version 120

#define Urho2DSamplers

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

#include "PostProcess.glsl"
#include "FXAA2n.glsl"

#ifdef VERTEXLIGHT
#include "Lighting.glsl"
#endif

varying vec2 vTexCoord;
varying vec4 vColor;

#ifdef VERTEXLIGHT
varying vec3 vVertexLight;
#endif

varying float fTexId;
varying float fTexEffect;

/*
    /// fTexEffect Values ///
    
    // CROPALPHA = 1
    // UNLIT = 2
    // BLUR = 4
    // FXAA = 8
    // TILEINDEX_X = 16
    // TILEINDEX_Y = 32
    
    // CROPALPHA = 1
    // CROPALPHA + UNLIT = 3
    // CROPALPHA + BLUR = 5
    // CROPALPHA + UNLIT + BLUR =  7 
    // CROPALPHA + FXAA = 9
    // CROPALPHA + UNLIT + FXAA = 11

    // UNLIT = 2
    // UNLIT + CROPALPHA = 3
    // UNLIT + BLUR = 6
    // UNLIT + CROPALPHA + BLUR = 7
    // UNLIT + FXAA = 10
    // UNLIT + CROPALPHA + FXAA = 11
    
    // BLUR = 4
    // BLUR + CROPALPHA = 5
    // BLUR + UNLIT = 6
    // BLUR + CROPALPHA + UNLIT = 7
    
    // FXAA = 8
    // FXAA + CROPALPHA = 9
    // FXAA + UNLIT = 10
    // FXAA + CROPALPHA + UNLIT = 11
    
    // TILE(0,0) = 0-15
    // TILE(0,1) = 16-31
    // TILE(1,0) = 32-47
    // TILE(1,1) = 48-53
*/
 
void VS()
{
    vec3 worldPos = vec3((iPos * iModelMatrix).xy, 0.0);
    
    vTexCoord = iTexCoord;
    vColor = iColor;
    fTexId = iTexCoord1.x;
    fTexEffect = iTexCoord1.y;

    gl_Position = GetClipPos(worldPos);
        
#ifdef VERTEXLIGHT
    float effect = fTexEffect;
    
    // Remove the TileCoord values 48,32,16
    if (effect >= 16.0)
    {
        effect -= 16.0;
        if (effect >= 16.0)
        {
            effect -= 16.0;
            if (effect >= 16.0)
            {
                effect -= 16.0;
            }
        }
    }
    
    // Lit Cases
    if (effect !=  2.0 && effect != 3.0 && 
        effect !=  6.0 && effect != 7.0 &&
        effect != 10.0 && effect != 11.0)
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
vec4 ApplyBackShapeEffects(sampler2D texSampler, float effect)
{
    vec4 value;

    vec2 f = fract(vTexCoord);
 
    // InnerLayer (0,0)
    if (effect <= 15.0)
        value = texture2D(texSampler, vec2(0.5, 0.5) * f);
    // RockLayer (1,1)
    else if (effect >= 48.0)
        value = texture2D(texSampler, vec2(0.5, 0.5) + vec2(0.5, 0.5) * f);
    // BackViewLayer (1,0)
    else if (effect >= 32.0)
        value = texture2D(texSampler, vec2(0.5, 0.0) + vec2(0.5, 0.5) * f);
    // BackGroundLayer (0,1)
    else
        value = texture2D(texSampler, vec2(0.0, 0.5) + vec2(0.5, 0.5) * f);

#ifdef VERTEXLIGHT
    vec4 litColor;
    
    // Remove the TileCoord values 48,32,16
    if (effect >= 16.0)
    {
        effect -= 16.0;
        if (effect >= 16.0)
        {
            effect -= 16.0;
            if (effect >= 16.0)
            {
                effect -= 16.0;
            }
        }
    }
            
    // UNLIT
    if (effect ==  2.0 || effect == 3.0 ||
        effect ==  6.0 || effect == 7.0 ||
        effect == 10.0 || effect == 11.0)
        litColor = vec4(1.0);
    // USE LIGHT (by default)
    else
        litColor = vec4(vVertexLight, 1.0);

    value *= litColor;
#endif

    return value;
}

vec4 ApplyTextureEffects(sampler2D texSampler, float effect)
{
    vec4 value = texture2D(texSampler, vTexCoord);

#ifdef VERTEXLIGHT
    vec4 litColor;
    
    // Remove the TileCoord values
    if (effect >= 48.0)
        effect -= 48.0;
    else if (effect >= 32.0)
        effect -= 32.0;
    else if (effect >= 16.0)
        effect -= 16.0;
            
    // UNLIT
    if (effect ==  2.0 || effect == 3.0 ||
        effect ==  6.0 || effect == 7.0 ||
        effect == 10.0 || effect == 11.0)
        litColor = vec4(1.0);
    // USE LIGHT (by default)
    else
        litColor = vec4(vVertexLight, 1.0);

    value *= litColor;
#endif

    return value;
}

// remove the variation
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

    float tid = Round(fTexId);
    float effect = Round(fTexEffect);
    
//    if (tid >= 7.0)
//        diffInput = ApplyTextureEffects(sUrho2DTextures[7], effect);
//    else if (tid >= 6.0)
//        diffInput = ApplyTextureEffects(sUrho2DTextures[6], effect);
//    else
    if (tid == 5.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[5], effect);
    else if (tid == 4.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[4], effect);
    else if (tid == 3.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[3], effect);
    else if (tid == 2.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[2], effect);
    else if (tid == 1.0)
        diffInput = ApplyTextureEffects(sUrho2DTextures[1], effect);
    else
        diffInput = ApplyBackShapeEffects(sUrho2DTextures[0], effect);
         
    gl_FragColor = cMatDiffColor * vColor * diffInput;
}
