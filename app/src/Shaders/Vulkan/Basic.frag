#version 450

layout(location=0) in vec4 vColor;
layout(location=1) in vec2 inTextCoord;

layout(location=0) out vec4 fragColor;

void main()
{
    fragColor = vColor;
}

