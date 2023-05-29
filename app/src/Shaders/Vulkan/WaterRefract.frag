#version 450

layout(location=0) in vec4 vColor;
layout(location=1) in vec2 vRefractUV;

layout(set=1, binding=1) uniform FramePS
{
	float cElapsedTimePS;
};

layout(set=2, binding=0) uniform sampler2D sWaterMap[1];

layout(location=0) out vec4 fragColor;

void main()
{
	const float freq = 35.0;
	const float accel = 5.0;
	const float amplitude = 0.0002;

    float variation = amplitude * sin(freq * vRefractUV.x + accel * cElapsedTimePS) + amplitude * cos(freq * vRefractUV.y + accel * cElapsedTimePS);
	
    // refracted color from the refraction map (the viewport textured getted at the previous pass in ForwardUrho2D renderpath)
    fragColor = vec4(texture(sWaterMap[0], vRefractUV + vec2(variation)).rgb * vColor.rgb, 1.0);
}
