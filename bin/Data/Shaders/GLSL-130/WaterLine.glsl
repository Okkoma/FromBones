#include "Uniforms.glsl"
#include "Transform.glsl"

varying vec4 vColor;

void VS()
{
    vColor = iColor;
    vec3 worldPos = (iPos * iModelMatrix).xyz;
    gl_Position = GetClipPos(worldPos);
}

void PS()
{
    gl_FragColor = vec4(vColor.rgb, 1.0);
}
