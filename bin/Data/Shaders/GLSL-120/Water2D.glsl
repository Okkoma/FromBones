#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec4 vColor;

void VS()
{
    vec3 worldPos = vec3((iPos * iModelMatrix).xy, 0.0);

    vTexCoord = iTexCoord;
    vColor = iColor;

    gl_Position = GetClipPos(worldPos);
}

void PS()
{
	vec2 sinTexCoord = vTexCoord;
	sinTexCoord.x += 0.1 * sin(vTexCoord.y + cElapsedTimePS);

//    vec3 refractColor = texture2D(sEnvMap, sinTexCoord).rgb;
//    vec3 reflectColor = texture2D(sDiffMap, sinTexCoord).rgb * vColor.rgb;
//    vec3 finalColor = mix(refractColor, reflectColor, 0.5);	
    
    vec3 finalColor = texture2D(sDiffMap, sinTexCoord).rgb;
    gl_FragColor.rgb = cMatDiffColor.rgb * finalColor;
    gl_FragColor.a = 0.3;
}
