#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

#define ACTIVE_BARYCENTRICEFFECTS
#define ACTIVE_BARYCENTRICEFFECTS_IDENTIFY_ADJACENTEDGES

varying vec2 vTexCoord;
varying vec4 vColor;
varying vec2 vPosition;

#ifdef ACTIVE_BARYCENTRICEFFECTS
varying vec3 vBarycentric;
varying vec3 vBaryAdj;
#endif

#ifndef GL_ES
uniform vec2 cNumTiles;
uniform vec2 cTileIndex;
uniform vec4 cWireEffects;
#else
uniform mediump vec2 cNumTiles;
uniform mediump vec2 cTileIndex;
uniform mediump vec4 cWireEffects;
#endif

#define PI 3.1415926538
#define EPSILON 0.2
#define ADJACENTEDGE 100.0

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    gl_Position = GetClipPos(vec3((iPos * iModelMatrix).xy, 0.0));

    vTexCoord = GetTexCoord(iTexCoord);
    vColor = iColor;

    vPosition = iPos.xy;

#ifdef ACTIVE_BARYCENTRICEFFECTS
    if (cWireEffects.x > 0.0)
    {
        if (iPos.z > 1.f - EPSILON && iPos.z < 1.f + EPSILON)
        {
            vBarycentric = vec3(1, 0, 0);
            vBaryAdj = vec3(1, 0, 0);
        }
        else if (iPos.z > 100.f - EPSILON && iPos.z < 100.f + EPSILON)
        {
            vBarycentric = vec3(0, 1, 0);
            vBaryAdj = vec3(0, 1, 0);
        }
        else if (iPos.z > 10000.f - EPSILON && iPos.z < 10000.f + EPSILON)
        {
            vBarycentric = vec3(0, 0, 1);
            vBaryAdj = vec3(0, 0, 1);
        }
    #ifdef ACTIVE_BARYCENTRICEFFECTS_IDENTIFY_ADJACENTEDGES
        else if (iPos.z > 9901.0 - EPSILON && iPos.z < 9901.0 + EPSILON)
        {
            vBarycentric = vec3(1, 0, 0);
            vBaryAdj = vec3(1, ADJACENTEDGE, 0);
        }
        else if (iPos.z > 990001.0 - EPSILON && iPos.z < 990001.0 + EPSILON)
        {
            vBarycentric = vec3(1, 0, 0);
            vBaryAdj = vec3(1, 0, ADJACENTEDGE);
        }
        else if (iPos.z > 999901.0 - EPSILON && iPos.z < 999901.0 + EPSILON)
        {
            vBarycentric = vec3(1, 0, 0);
            vBaryAdj = vec3(1, ADJACENTEDGE, ADJACENTEDGE);
        }
        else if (iPos.z > 199.0 - EPSILON && iPos.z < 199.0 + EPSILON)
        {
            vBarycentric = vec3(0, 1, 0);
            vBaryAdj = vec3(ADJACENTEDGE, 1, 0);
        }
        else if (iPos.z > 990100.0 - EPSILON && iPos.z < 990100.0 + EPSILON)
        {
            vBarycentric = vec3(0, 1, 0);
            vBaryAdj = vec3(0, 1, ADJACENTEDGE);
        }
        else if (iPos.z > 990199.0 - EPSILON && iPos.z < 990199.0 + EPSILON)
        {
            vBarycentric = vec3(0, 1, 0);
            vBaryAdj = vec3(ADJACENTEDGE, 1, ADJACENTEDGE);
        }
        else if (iPos.z > 10099.0 - EPSILON && iPos.z < 10099.0 + EPSILON)
        {
            vBarycentric = vec3(0, 0, 1);
            vBaryAdj = vec3(ADJACENTEDGE, 0, 1);
        }
        else if (iPos.z > 19900.0 - EPSILON && iPos.z < 19900.0 + EPSILON)
        {
            vBarycentric = vec3(0, 0, 1);
            vBaryAdj = vec3(0, ADJACENTEDGE, 1);
        }
        else if (iPos.z > 19999.0 - EPSILON && iPos.z < 19999.0 + EPSILON)
        {
            vBarycentric = vec3(0, 0, 1);
            vBaryAdj = vec3(ADJACENTEDGE, ADJACENTEDGE, 1);
        }
    #endif
    }
#endif
}

float grid(vec2 parameter, float width, float feather)
{
    float w1 = width - feather * 0.5;
    vec2 d = fwidth(parameter);
    vec2 looped = 0.5 - abs(mod(parameter, 1.0) - 0.5);
    vec2 a2 = smoothstep(d * w1, d * (w1 + feather), looped);
    return min(a2.x, a2.y);
}

float grid(vec2 parameter, float width)
{
    vec2 d = fwidth(parameter);
    vec2 looped = 0.5 - abs(mod(parameter, 1.0) - 0.5);
    vec2 a2 = smoothstep(d * (width - 0.5), d * (width + 0.5), looped);
    return min(a2.x, a2.y);
}

float wireframe(vec3 vBC, float width, float feather)
{
    float w1 = width - feather * 0.5;
    vec3 d = fwidth(vBC);
    vec3 a3 = smoothstep(d * w1, d * (w1 + feather), vBC);
    return min(min(a3.x, a3.y), a3.z);
}

float wireframe(vec3 vBC, float width)
{
    vec3 d = fwidth(vBC);
    vec3 a3 = smoothstep(d * (width - 0.5), d * (width + 0.5), vBC);
    return min(min(a3.x, a3.y), a3.z);
}

//uniform float gsize; //size of the griduniform
//float gwidth; //grid lines'width in pixels
//varying vec3 P;
//
//void main()
//{
//    vec3 f = abs(fract (P * gsize)-0.5);
//    vec3 df = fwidth(P * gsize);
//    float mi = max(0.0, gwidth-1.0), ma=max(1.0, gwidth);//should be uniforms
//    vec3 g = clamp((f-df*mi)/(df*(ma-mi)),max(0.0,1.0-gwidth),1.0);//max(0.0,1.0-gwidth) should also be sent as uniform
//    float c = g.x * g.y * g.z;
//    gl_FragColor = vec4(c, c, c, 1.0);
//    gl_FragColor = gl_FragColor * gl_Color;
//}

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

	gl_FragColor = vColor * texture2D(sDiffMap, (cTileIndex / cNumTiles) + (1.0 / cNumTiles) * fract(vTexCoord));

#ifdef ACTIVE_BARYCENTRICEFFECTS
    // Barycentric WireFrame
    if (cWireEffects.x > 0.0)
    {
        //gl_FragColor.a *= wireframe(vBarycentric, cWireEffects.x, cWireEffects.y);
        //gl_FragColor.a *= wireframe(vBarycentric, (abs(sin(length(vBarycentric) * PI))+1.0) * cWireEffects.x, cWireEffects.y);
        //gl_FragColor.r = wireframe(vBarycentric, (abs(sin(length(vBarycentric) * PI))+1.0) * cWireEffects.x, cWireEffects.y);

    #ifdef ACTIVE_BARYCENTRICEFFECTS_IDENTIFY_ADJACENTEDGES
        //gl_FragColor.r = wireframe(vBaryAdj, cWireEffects.x, cWireEffects.y);
        //gl_FragColor.r = min(min(vBaryAdj.x, vBaryAdj.y), vBaryAdj.z);
        gl_FragColor.rgb = fwidth(vBaryAdj);
        gl_FragColor.a = 1.f;
    #else
        float bary = min(min(vBarycentric.x, vBarycentric.y), vBarycentric.z);
        gl_FragColor.r += 0.25f * bary;
        gl_FragColor.a *= 5.f *bary;
    #endif
    }
#endif

    // Grid XY
    if (cWireEffects.z > 0.0)
        gl_FragColor.a *= 2 * grid(vPosition.xy * cWireEffects.z, cWireEffects.w, 10.0);
}
