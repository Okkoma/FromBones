#version 450

layout(location=0) in vec2 inPosition;
layout(location=1) in vec2 inTextCoord;
layout(location=2) in vec4 inColor;
layout(location=3) in float inZ;
layout(location=4) in int inTextCoord2;
layout(location=5) in int inTextCoord3;

layout(location=0) out vec4 vColor;
layout(location=1) out vec2 vTextCoord;
flat layout(location=2) out uint fTextureId;
flat layout(location=3) out uint fTexEffect1;
flat layout(location=4) out uint fTexEffect2;
flat layout(location=5) out uint fTexEffect3;
layout(location=6) out vec3 vVertexLight;

layout(set=0, binding=0) uniform CameraVS
{
    mat4 cViewProj;
};

layout(set=1, binding=0) uniform LightVS
{
    vec4 cVertexLights[4 * 3];
    int cNumVertexLights;
};

const int TXM_UNIT = 0;
const int TXM_FX_LIT = 1;
const int TXM_FX_EFFECTS = 2;
const int TXM_FX_TILEINDEX = 3;

const int TEXTUREMODEMASK[] =
{
    0x0000000F, // TXM_UNIT          : 0000001111
    0x00000010, // TXM_FX_LIT        : 0000010000
    0x000000E0, // TXM_FX_EFFECTS    : 0011100000
    0x00000300, // TXM_FX_TILEINDEX  : 1100000000
};

const int TEXTUREMODEOFFSET[] =
{
    0, // TXM_UNIT
    4, // TXM_FX_LIT
    5, // TXM_FX_EFFECTS
    8, // TXM_FX_TILEINDEX
};

uint ExtractTextureMode(int flag, uint texmode)
{
    return ((texmode & TEXTUREMODEMASK[flag]) >> TEXTUREMODEOFFSET[flag]);
}

float GetVertexLightVolumetric(int index, vec3 worldPos)
{
    vec3 lightDir = cVertexLights[index * 3 + 1].xyz;
    vec3 lightPos = cVertexLights[index * 3 + 2].xyz;
    float invRange = cVertexLights[index * 3].w;
    float cutoff = cVertexLights[index * 3 + 1].w;
    float invCutoff = cVertexLights[index * 3 + 2].w;

    // Directional light
    if (invRange == 0.0)
        return 1.0;
    // Point/spot light
    else
    {
        vec3 lightVec = (lightPos - worldPos) * invRange;
        float lightDist = length(lightVec);
        vec3 localDir = lightVec / lightDist;
        float atten = clamp(1.0 - lightDist * lightDist, 0.0, 1.0);
        float spotEffect = dot(localDir, lightDir);
        float spotAtten = clamp((spotEffect - cutoff) * invCutoff, 0.0, 1.0);
        return atten * spotAtten;
    }
}

void main()
{
    gl_Position   = vec4(inPosition, 0.0, 1.0) * cViewProj;

    vTextCoord    = inTextCoord;
    vColor        = inColor;

    fTextureId    = ExtractTextureMode(TXM_UNIT, inTextCoord2);
    fTexEffect1   = ExtractTextureMode(TXM_FX_LIT, inTextCoord2);
    fTexEffect2   = ExtractTextureMode(TXM_FX_EFFECTS, inTextCoord2);
    fTexEffect3   = ExtractTextureMode(TXM_FX_TILEINDEX, inTextCoord2);

    // Ambient & per-vertex lighting
    vVertexLight = vec3(0.1);
        
    if (fTexEffect1 == uint(0))
    {
        for (int i = 0; i < cNumVertexLights; ++i)
            vVertexLight += GetVertexLightVolumetric(i, vec3(inPosition, 0.0)) * cVertexLights[i * 3].rgb;
    }

}
