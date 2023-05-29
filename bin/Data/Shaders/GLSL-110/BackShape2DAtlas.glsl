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

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    gl_Position = GetClipPos(vec3((iPos * iModelMatrix).xy, 0.0));

    vTexCoord = GetTexCoord(iTexCoord);
    vColor = iColor;

    vPosition = iPos.xy;
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

	gl_FragColor = vColor * texture2D(sDiffMap, (cTileIndex / cNumTiles) + (1.0 / cNumTiles) * fract(vTexCoord));
}
