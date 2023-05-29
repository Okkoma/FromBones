#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec4 vColor;

void VS()
{
    vec3 worldPos = (iPos * iModelMatrix).xyz;

	vTexCoord = iTexCoord + vec2(0.25 * sin(cElapsedTime), 0.0);
    vColor = iColor;

    gl_Position = GetClipPos(worldPos);
}

void PS()
{
    gl_FragColor = vec4(cMatDiffColor.rgb * texture2D(sDiffMap , vTexCoord).rgb * vColor.rgb, 1.0);
}
