#version 450

layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vTextCoord;

layout(set=1, binding=0) uniform sampler2D diffmap;

//layout (input_attachment_index = 0, set = 0, binding = 0) uniform subpassInput inputColor;
//layout (input_attachment_index = 1, set = 1, binding = 1) uniform subpassInput inputDepth;

layout(location=0) out vec4 fragColor1;
//layout(location=1) out vec4 fragColor2;

void main()
{
    fragColor1 = texture(diffmap, vTextCoord) * vColor;
//	fragColor1 = fragColor2 = texture(diffmap, vTextCoord) * vColor;
//	fragColor = subpassLoad(inputColor) * vColor;
}

