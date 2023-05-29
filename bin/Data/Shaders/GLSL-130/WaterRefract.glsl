#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"
#include "ScreenPos.glsl"

varying vec2 vTexCoord;
varying vec2 vRefractUV;
varying vec4 vColor;

void VS()
{
    vec3 worldPos = (iPos * iModelMatrix).xyz;

	vTexCoord = iTexCoord + vec2(0.25 * sin(cElapsedTime), 0.0);
    vColor = iColor;

    gl_Position = GetClipPos(worldPos);
    vRefractUV = GetQuadTexCoord(gl_Position);
}

void PS()
{
	const float freq = 35.0;
	const float accel = 5.0;
	const float amplitude = 0.0002;

    float variation = amplitude * sin(freq * vRefractUV.x + accel * cElapsedTimePS) + amplitude * cos(freq * vRefractUV.y + accel * cElapsedTimePS);

    // refracted color from the refraction map (it's the viewport getted at refract pass : see ForwardUrho2D renderpath)
    gl_FragColor = vec4(cMatDiffColor.rgb * texture2D(sDiffMap, vRefractUV + vec2(variation)).rgb * vColor.rgb, 1.0);
}
