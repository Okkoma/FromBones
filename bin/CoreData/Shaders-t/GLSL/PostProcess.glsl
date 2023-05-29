#ifdef COMPILEPS
const float PI = 3.14159265;

vec2 Noise(vec2 coord)
{
    float noiseX = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233))) * 43758.5453), 0.0, 1.0);
    float noiseY = clamp(fract(sin(dot(coord, vec2(12.9898, 78.233) * 2.0)) * 43758.5453), 0.0, 1.0);
    return vec2(noiseX, noiseY);
}

// Adapted: http://callumhay.blogspot.com/2010/09/gaussian-blur-shader-glsl.html
vec4 GaussianBlur(int blurKernelSize, vec2 blurDir, vec2 blurRadius, float sigma, sampler2D texSampler, vec2 texCoord)
{
    int blurKernelSizeHalfSize = blurKernelSize / 2;

    // Incremental Gaussian Coefficent Calculation (See GPU Gems 3 pp. 877 - 889)
    vec3 gaussCoeff;
    gaussCoeff.x = 1.0 / (sqrt(2.0 * PI) * sigma);
    gaussCoeff.y = exp(-0.5 / (sigma * sigma));
    gaussCoeff.z = gaussCoeff.y * gaussCoeff.y;

    vec2 blurVec = blurRadius * blurDir;
    vec4 avgValue = vec4(0.0);
    float gaussCoeffSum = 0.0;

    avgValue += texture2D(texSampler, texCoord) * gaussCoeff.x;
    gaussCoeffSum += gaussCoeff.x;
    gaussCoeff.xy *= gaussCoeff.yz;

    for (int i = 1; i <= blurKernelSizeHalfSize; i++)
    {
        avgValue += texture2D(texSampler, texCoord - float(i) * blurVec) * gaussCoeff.x;
        avgValue += texture2D(texSampler, texCoord + float(i) * blurVec) * gaussCoeff.x;

        gaussCoeffSum += 2.0 * gaussCoeff.x;
        gaussCoeff.xy *= gaussCoeff.yz;
    }

    return avgValue / gaussCoeffSum;
}

const vec3 LumWeights = vec3(0.2126, 0.7152, 0.0722);

vec3 ReinhardEq3Tonemap(vec3 x)
{
    return x / (1.0 + x);
}

vec3 ReinhardEq4Tonemap(vec3 x, float white)
{
    return x * (1.0 + x / white) / (1.0 + x);
}

// Unchared2 tone mapping (See http://filmicgames.com)
const float A = 0.15;
const float B = 0.50;
const float C = 0.10;
const float D = 0.20;
const float E = 0.02;
const float F = 0.30;

vec3 Uncharted2Tonemap(vec3 x)
{
   return ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F))-E/F;
}

#ifndef GL_ES
vec3 ColorCorrection(vec3 color, sampler3D lut)
{
    float lutSize = 16.0;
    float scale = (lutSize - 1.0) / lutSize;
    float offset = 1.0 / (2.0 * lutSize);
    return texture3D(lut, clamp(color, 0.0, 1.0) * scale + offset).rgb;
}
#endif

const float Gamma = 2.2;
const float InverseGamma = 1.0 / 2.2;

vec3 ToGamma(vec3 color)
{
    return vec3(pow(color.r, Gamma), pow(color.g, Gamma), pow(color.b, Gamma));
}

vec3 ToInverseGamma(vec3 color)
{
    return vec3(pow(color.r, InverseGamma), pow(color.g, InverseGamma), pow(color.b, InverseGamma));
}

vec4 FXAA2(sampler2D texSampler, vec2 texCoord)
{
    vec4 value = vec4(0.0);

    float FXAA_SUBPIX_SHIFT = 1.0/4.0; // Not used
    float FXAA_SPAN_MAX = 8.0;
    float FXAA_REDUCE_MUL = 1.0/8.0;
    float FXAA_REDUCE_MIN = 1.0/128.0;

    vec2 posOffset = cGBufferInvSize.xy * texCoord.x;

    vec3 rgbNW = texture2D(texSampler, texCoord + vec2(-posOffset.x, -posOffset.y)).rgb;
    vec3 rgbNE = texture2D(texSampler, texCoord + vec2(posOffset.x, -posOffset.y)).rgb;
    vec3 rgbSW = texture2D(texSampler, texCoord + vec2(-posOffset.x, posOffset.y)).rgb;
    vec3 rgbSE = texture2D(texSampler, texCoord + vec2(posOffset.x, posOffset.y)).rgb;
    vec3 rgbM  = texture2D(texSampler, texCoord).rgb;

    vec3 luma = vec3(0.299, 0.587, 0.114);
    float lumaNW = dot(rgbNW, luma);
    float lumaNE = dot(rgbNE, luma);
    float lumaSW = dot(rgbSW, luma);
    float lumaSE = dot(rgbSE, luma);
    float lumaM  = dot(rgbM,  luma);

    float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));
    float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));

    if (((lumaMax - lumaMin) / lumaMin) >= texCoord.y)
    {
        vec2 dir;
        dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));
        dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));

        float dirReduce = max(
            (lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),
            FXAA_REDUCE_MIN);
        float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);
        dir = min(vec2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX),
              max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX),
              dir * rcpDirMin)) * cGBufferInvSize.xy;

        vec3 rgbA = (1.0/2.0) * (
            texture2D(texSampler, texCoord + dir * (1.0/3.0 - 0.5)).xyz +
            texture2D(texSampler, texCoord + dir * (2.0/3.0 - 0.5)).xyz);
        vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (
            texture2D(texSampler, texCoord + dir * (0.0/3.0 - 0.5)).xyz +
            texture2D(texSampler, texCoord + dir * (3.0/3.0 - 0.5)).xyz);
        float lumaB = dot(rgbB, luma);

        vec3 rgbOut;
        if((lumaB < lumaMin) || (lumaB > lumaMax))
            rgbOut = rgbA;
        else
            rgbOut = rgbB;

        value = vec4(rgbOut, 1.0);
    }
    else
        value = vec4(rgbM, 1.0);

    return value;
}

#endif
