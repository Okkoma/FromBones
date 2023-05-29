#pragma once

class Noise
{
public:
    Noise() { }

    enum NoiseType
    {
        VALUE = 0,
        PERLIN,
        SIMPLEX,
    };

    float Noise1D(NoiseType type, float x, float persistence=1.f, int octaves=1);
    float Noise2D(NoiseType type, float x, float y, float persistence=1.f, int octaves=1);

    void SetValueSeed(int seed);
    float ValueNoise1D(float arg);

    float PerlinNoise1D(float arg);
    float PerlinNoise2D(float x, float y);
    float SimplexNoise1D(float x);
    float SimplexNoise2D(float x, float y);

private:
    float value[256];

    static unsigned char perm[512];
};
