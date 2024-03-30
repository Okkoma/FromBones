#version 450
#extension GL_EXT_nonuniform_qualifier : require

layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vTextCoord;
flat layout(location=2) in uint fTextureId;
flat layout(location=3) in uint fTexEffect1;
flat layout(location=4) in uint fTexEffect2;
flat layout(location=5) in uint fTexEffect3;

layout(set=1, binding=0) uniform sampler2D sGroundsMaps[8];

layout(location=0) out vec4 fragColor;

vec4 ApplyBackShape(sampler2D texSampler)
{
    const float cuttingedge = 0.001;
    const vec2 tilesize = vec2(0.5 - 2.0*cuttingedge, 0.5 - 2.0*cuttingedge);
    
    vec4 value;
    
    // InnerLayer (0,0)
    if (fTexEffect3 == uint(0))
       value = texture(texSampler, vec2(cuttingedge, cuttingedge) + tilesize * fract(vTextCoord));
    // BackGroundLayer (0,1)
    else if (fTexEffect3 == uint(1))
        value = texture(texSampler, vec2(cuttingedge, 0.5 + cuttingedge) + tilesize * fract(vTextCoord));
    // BackViewLayer (1,0)
    else if (fTexEffect3 == uint(2))
        value = texture(texSampler, vec2(0.5 + cuttingedge, cuttingedge) + tilesize * fract(vTextCoord));
    // RockLayer (1,1)
    else
    	value = texture(texSampler, vec2(0.5 + cuttingedge, 0.5 + cuttingedge) + tilesize * fract(vTextCoord));    
    
    value.a += 0.2;
    	    
    return value;
}

vec4 ApplyTextureEffects(sampler2D texSampler)
{
    vec4 value = fTextureId == 0 ? ApplyBackShape(texSampler) : texture(texSampler, vTextCoord);
		
//    // BLUR=2
//    if ((fTexEffect2 & uint(2)) == uint(2))
//        value *= GaussianBlur(5, vec2(1.0, 1.0), cGBufferInvSize.xy * 2.0, 2.0, texSampler, vTextCoord);
//    // FXAA=4
//    else if ((fTexEffect2 & uint(4)) == uint(4))
//        value *= FXAA2(texSampler, vTextCoord);

    // CROPALPHA=1
//    if ((fTexEffect2 & uint(1)) == uint(1))
//    {
//        const float minval = 0.13;
//        if (value.r < minval && value.g < minval && value.b > 1-minval)
//            value.a = (value.r + value.g + 1.0-value.b) / 3.0;
//    }
    	
    return value;
}

void main()
{
    vec4 diffInput = fTextureId < 8 ? ApplyTextureEffects(sGroundsMaps[nonuniformEXT(fTextureId)]) : vec4(1.0);
    fragColor = vColor * diffInput;
}

