#define Urho2DSamplers

#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec4 vColor;

void VS()
{
    vec3 worldPos = (iPos * iModelMatrix).xyz;

    //vTexCoord = iTexCoord + vec2(0.25 * sin(cElapsedTime), 0.0);
    vTexCoord = iTexCoord;
    vColor = iColor;

    gl_Position = GetClipPos(worldPos);
}

void PS()
{
    gl_FragColor = texture2D(sUrho2DTextures[1] , vTexCoord) * vColor;
}
