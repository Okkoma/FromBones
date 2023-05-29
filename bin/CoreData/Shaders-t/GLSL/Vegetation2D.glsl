#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

uniform float cWindHeightFactor;
uniform float cWindHeightPivot;
uniform float cWindPeriod;
uniform vec2 cWindWorldSpacing;

varying vec2 vTexCoord;
varying vec4 vColor;

void VS()
{
    mat4 modelMatrix = iModelMatrix;
    vec3 worldPos = GetWorldPos(modelMatrix);
    float height = worldPos.y - cModel[1][3];

    float windStrength = max(height - cWindHeightPivot, 0.0) * cWindHeightFactor;
    float windPeriod = cElapsedTime * cWindPeriod + dot(worldPos.xz, cWindWorldSpacing);
    worldPos.x += windStrength * sin(windPeriod);

    gl_Position = GetClipPos(worldPos);

    vColor = iColor;
    vTexCoord = GetTexCoord(iTexCoord);
}
