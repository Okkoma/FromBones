#include "Uniforms.glsl"
#include "Samplers.glsl"
#include "Transform.glsl"

#ifdef VERTEXLIGHT
#include "Lighting.glsl"
#endif

varying vec2 vTexCoord;
varying vec4 vColor;

#ifdef VERTEXLIGHT
varying vec3 vVertexLight;
#endif

varying float fTexEffect1;

/*
    /// fTexEffect1 Values ///
    // LIT   = 0
    // UNLIT = 1
*/

void VS()
{
    vec3 worldPos = vec3((iPos * iModelMatrix).xy, 0.0);

    vTexCoord = iTexCoord;
    vColor = iColor;

    fTexEffect1  = iTangent.y; // unlit

    gl_Position = GetClipPos(worldPos);

#ifdef VERTEXLIGHT
    // Lit Cases
    if (fTexEffect1 == 0.0)
    {
        // Ambient & per-vertex lighting
        vVertexLight = GetAmbient(GetZonePos(worldPos));

        #ifdef NUMVERTEXLIGHTS
        for (int i = 0; i < NUMVERTEXLIGHTS; ++i)
            vVertexLight += GetVertexLightVolumetric(i, worldPos) * cVertexLights[i * 3].rgb;
        #endif
    }
#endif
}

void PS()
{
    vec4 diffInput = texture2D(sDiffMap, vTexCoord);

#ifdef VERTEXLIGHT
    if (fTexEffect1 < 1.0)
        diffInput *= vec4(vVertexLight, 1.0);
#endif

    gl_FragColor = cMatDiffColor * vColor * diffInput;

#ifdef GL_ES
    // apply gamma correction
    float gamma = 1.2;
    gl_FragColor.rgb = pow(gl_FragColor.rgb, vec3(1.0/gamma));
#endif
}
