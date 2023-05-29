/*
Copyright (c) 2008 Joshua Tippetts

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/
#ifndef UTILITY_H
#define UTILITY_H
#include <cmath>

template<typename TYPE> TYPE clamp(TYPE v, TYPE l, TYPE h)
{
    if(v<l) v=l;
    if(v>h) v=h;

    return v;
};

template<typename TYPE> TYPE lerp(float t, TYPE a, TYPE b)
{
    return a+t*(b-a);
}

template<typename TYPE> bool isPowerOf2(TYPE n)
{
    return !((n-1) & n);
}

inline float hermite_blend(float t)
{
    return (t*t*(3-2*t));
}

inline float quintic_blend(float t)
{
    return t*t*t*(t*(t*6-15)+10);
}

inline int fast_floor(float t)
{
    return (t>0 ? (int)t : (int)t - 1);
}

inline float array_dot(float *arr, float a, float b)
{
    return a*arr[0] + b*arr[1];
}

inline float array_dot3(float *arr, float a, float b, float c)
{
    return a*arr[0] + b*arr[1] + c*arr[2];
}

inline float bias(float b, float t)
{
    return pow(t, log(b)/log(0.5));
}

inline float gain(float g, float t)
{
    if(t<0.5)
    {
        return bias(1.0-g, 2.0*t)/2.0;
    }
    else
    {
        return 1.0 - bias(1.0-g, 2.0 - 2.0*t)/2.0;
    }
}

struct TileCoord
{
    unsigned int x,y;
};

struct CoordPair
{
    float x,y;
};

CoordPair closest_point(float vx, float vy, float x, float  y);

float deg_to_rad(float deg);

float rad_to_deg(float rad);

float hex_function(float x, float y);

TileCoord calcHexPointTile(float px, float py);

CoordPair calcHexTileCenter(int tx, int ty);

#endif

