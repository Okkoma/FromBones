#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

varying vec2 vTexCoord;
varying vec4 vColor;
varying vec2 vPosition;

#ifndef GL_ES
uniform vec2 cNumTiles;
uniform vec2 cTileIndex;
#else
uniform mediump vec2 cNumTiles;
uniform mediump vec2 cTileIndex;
#endif

//#ifndef GL_ES
//#define BARYCENTRIC_EFFECTS
//#endif

#ifdef BARYCENTRIC_EFFECTS

#define PI 3.14159

varying vec3 vBarycentric;

#ifndef GL_ES
uniform vec4 cWireEffects;
#else
uniform mediump vec4 cWireEffects;
#endif

float grid(vec2 parameter, float width, float feather)
{
    float w1 = width - feather * 0.5;
    vec2 d = fwidth(parameter);
    vec2 looped = 0.5 - abs(mod(parameter, 1.0) - 0.5);
    vec2 a2 = smoothstep(d * w1, d * (w1 + feather), looped);
    return min(a2.x, a2.y);
}

float wireframe(vec3 vBC, float width, float feather)
{
    float w1 = width - feather * 0.5;
    vec3 d = fwidth(vBC);
    vec3 a3 = smoothstep(d * w1, d * (w1 + feather), vBC);
    return min(min(a3.x, a3.y), a3.z);
}

#endif

void VS()
{
    gl_Position = GetClipPos(vec3((iPos * iModelMatrix).xy, 0.0));

    vTexCoord = iTexCoord;
    vColor = iColor;

    vPosition = iPos.xy;

#ifdef BARYCENTRIC_EFFECTS
    if (cWireEffects.x > 0.0)
    {
        if (iPos.z == 0.f)
            vBarycentric = vec3(1, 0, 0);
        else if (iPos.z == 1.f)
            vBarycentric = vec3(0, 1, 0);
        else
            vBarycentric = vec3(0, 0, 1);
    }
#endif
}



void PS()
{
    // Reduce Texture Coord to the tile Coordinate
    //vec2 size = (1.0 / cNumTiles);
    //vec2 position = (cTileIndex / cNumTiles);
    //vec2 framecoord = position + size * fract(vTexCoord);
    //vec4 diffColor = texture2D(sDiffMap, framecoord);

    //vec2 framecoord = (cTileIndex + fract(vTexCoord)) / cNumTiles;
    //vec4 diffColor = texture2D(sDiffMap, framecoord);
    //gl_FragColor = vColor * diffColor;

//	gl_FragColor = vColor * texture2D(sDiffMap, (cTileIndex / cNumTiles) + (1.0 / cNumTiles) * fract(vTexCoord));
	gl_FragColor = vColor * texture2D(sDiffMap, (cTileIndex + fract(vTexCoord)) / cNumTiles);
	
#ifdef BARYCENTRIC_EFFECTS
    // Barycentric WireFrame
    if (cWireEffects.x > 0.0)
        gl_FragColor.a *= wireframe(vBarycentric, (abs(sin(length(vBarycentric) * PI))+1.0) * cWireEffects.x, cWireEffects.y);

    // Grid XY
    if (cWireEffects.z > 0.0)
        gl_FragColor.a *= 2 * grid(vPosition.xy * cWireEffects.z, cWireEffects.w, 10.0);
#endif

#ifdef GL_ES
    // apply gamma correction
    float gamma = 2.2;
    gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0/gamma));
#endif
}
