#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vTextCoord;
flat layout(location=2) in uint fTextureId;
flat layout(location=3) in uint fTexEffect1;
flat layout(location=4) in uint fTexEffect2;
flat layout(location=5) in uint fTexEffect3;
layout(location=6) in vec3 vVertexLight;

layout(set=2, binding=0) uniform sampler2D sActorsMaps[8];

layout(location=0) out vec4 fragColor;
layout(location=1) out vec4 fragColor2;

vec4 ApplyTextureEffects(sampler2D texSampler)
{
    vec4 value = texture(texSampler, vTextCoord);

//    // BLUR=4
//    if ((fTexEffect2 & uint(2)) == uint(2))
//        value *= GaussianBlur(5, vec2(1.0, 1.0), cGBufferInvSize.xy * 2.0, 2.0, texSampler, vTexCoord);
//    // FXAA=8
//    else if ((fTexEffect2 & uint(4)) == uint(4))
//        value *= FXAA2(texSampler, vTexCoord);

    // CROPALPHA=1
    if ((fTexEffect2 & uint(1)) == uint(1))
    {
        const float minval = 0.13;
        if (value.r < minval && value.g < minval && value.b > 1-minval)
            value.a = (value.r + value.g + 1.0-value.b) / 3.0;
    }

    // LIT or UNLIT=1
    if (fTexEffect1 == uint(0))
    	value *= vec4(vVertexLight, 1.0);

    return value;
}

void main()
{
    vec4 diffInput = fTextureId < 8 ? ApplyTextureEffects(sActorsMaps[nonuniformEXT(fTextureId)]) : vec4(1.0);
    fragColor = fragColor2 = vColor * diffInput;
}

