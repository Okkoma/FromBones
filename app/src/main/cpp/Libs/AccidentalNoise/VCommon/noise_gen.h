#ifndef NOISE_GEN_H
#define NOISE_GEN_H

#include "coordinate.h"

namespace anl
{

// Interpolation functions

typedef float (*interp_func)(float);

float noInterp(float t);
float linearInterp(float t);
float hermiteInterp(float t);
float quinticInterp(float t);

interp_func Interpolation(int type);

static const interp_func InterPolationFunctions[] = { noInterp, linearInterp, hermiteInterp, quinticInterp };


// Distance functions

typedef float (*dist_func2)(float, float, float, float);
typedef float (*dist_func3)(float, float, float, float, float, float);
typedef float (*dist_func4)(float, float, float, float, float, float, float, float);
typedef float (*dist_func6)(float, float, float, float, float, float, float, float, float, float, float, float);

float distFast2(float x1, float y1, float x2, float y2);
float distFast3(float x1, float y1, float z1, float x2, float y2, float z2);
float distFast4(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2);
float distFast6(float x1, float y1, float z1, float w1, float u1, float v1, float x2, float y2, float z2, float w2, float u2, float v2);

float distEuclid2(float x1, float y1, float x2, float y2);
float distEuclid3(float x1, float y1, float z1, float x2, float y2, float z2);
float distEuclid4(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2);
float distEuclid6(float x1, float y1, float z1, float w1, float u1, float v1, float x2, float y2, float z2, float w2, float u2, float v2);

float distManhattan2(float x1, float y1, float x2, float y2);
float distManhattan3(float x1, float y1, float z1, float x2, float y2, float z2);
float distManhattan4(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2);
float distManhattan6(float x1, float y1, float z1, float w1, float u1, float v1, float x2, float y2, float z2, float w2, float u2, float v2);

float distGreatestAxis2(float x1, float y1, float x2, float y2);
float distGreatestAxis3(float x1, float y1, float z1, float x2, float y2, float z2);
float distGreatestAxis4(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2);
float distGreatestAxis6(float x1, float y1, float z1, float w1, float u1, float v1, float x2, float y2, float z2, float w2, float u2, float v2);

float distLeastAxis2(float x1, float y1, float x2, float y2);
float distLeastAxis3(float x1, float y1, float z1, float x2, float y2, float z2);
float distLeastAxis4(float x1, float y1, float z1, float w1, float x2, float y2, float z2, float w2);
float distLeastAxis6(float x1, float y1, float z1, float w1, float u1, float v1, float x2, float y2, float z2, float w2, float u2, float v2);

dist_func2 Distance2(int distancetype);
dist_func3 Distance3(int distancetype);
dist_func4 Distance4(int distancetype);
dist_func6 Distance6(int distancetype);

static const dist_func2 DistanceFunctions2[] = { distEuclid2, distManhattan2, distGreatestAxis2, distLeastAxis2 };
static const dist_func3 DistanceFunctions3[] = { distEuclid3, distManhattan3, distGreatestAxis3, distLeastAxis3 };
static const dist_func4 DistanceFunctions4[] = { distEuclid4, distManhattan4, distGreatestAxis4, distLeastAxis4 };
static const dist_func6 DistanceFunctions6[] = { distEuclid6, distManhattan6, distGreatestAxis6, distLeastAxis6 };


// Noise generators

typedef float (*noise_func2)(float, float, unsigned int, interp_func);
typedef float (*noise_func3)(float, float, float, unsigned int, interp_func);
typedef float (*noise_func4)(float, float, float, float, unsigned int, interp_func);
typedef float (*noise_func6)(float, float, float, float, float, float, unsigned int, interp_func);

float value_noise2D(float x, float y, unsigned int seed, interp_func interp);
float value_noise3D(float x, float y, float z, unsigned int seed, interp_func interp);
float value_noise4D(float x, float y, float z, float w, unsigned int seed, interp_func interp);
float value_noise6D(float x, float y, float z, float w, float u, float v, unsigned int seed, interp_func interp);

float gradient_noise2D(float x, float y, unsigned int seed, interp_func interp);
float gradient_noise3D(float x, float y, float z, unsigned int seed, interp_func interp);
float gradient_noise4D(float x, float y, float z, float w, unsigned int seed, interp_func interp);
float gradient_noise6D(float x, float y, float z, float w, float u, float v, unsigned int seed, interp_func interp);

float gradval_noise2D(float x, float y, unsigned int seed, interp_func interp);
float gradval_noise3D(float x, float y, float z, unsigned int seed, interp_func interp);
float gradval_noise4D(float x, float y, float z, float w, unsigned int seed, interp_func interp);
float gradval_noise6D(float x, float y, float z, float w, float u, float v, unsigned int seed, interp_func interp);

float white_noise2D(float x, float y, unsigned int seed, interp_func interp);
float white_noise3D(float x, float y, float z, unsigned int seed, interp_func interp);
float white_noise4D(float x, float y, float z, float w, unsigned int seed, interp_func interp);
float white_noise6D(float x, float y, float z, float w, float u, float v, unsigned int seed, interp_func interp);

float simplex_noise2D(float x, float y, unsigned int seed, interp_func interp);
float simplex_noise3D(float x, float y, float z, unsigned int seed, interp_func interp);
float simplex_noise4D(float x, float y, float z, float w, unsigned int seed, interp_func interp);
float simplex_noise6D(float x, float y, float z, float w, float u, float v, unsigned int seed, interp_func interp);
float new_simplex_noise4D(float x, float y, float z, float w, unsigned int seed, interp_func interp);

void cellular_function2D(float x, float y, unsigned int seed, float *f, float *disp, dist_func2 dist);
void cellular_function3D(float x, float y, float z, unsigned int seed, float *f, float *disp, dist_func3 dist);
void cellular_function4D(float x, float y, float z, float w, unsigned int seed, float *f, float *disp, dist_func4 dist);
void cellular_function6D(float x, float y, float z, float w, float u, float v, unsigned int seed, float *f, float *disp, dist_func6 dist);

void value_noise(const CCoordinate& coord, unsigned seed, interp_func interp, float *value);
void gradient_noise(const CCoordinate& coord, unsigned seed, interp_func interp, float *value);
void gradval_noise(const CCoordinate& coord, unsigned seed, interp_func interp, float *value);
void white_noise(const CCoordinate& coord, unsigned seed, interp_func interp, float *value);
void simplex_noise(const CCoordinate& coord, unsigned seed, interp_func interp, float *value);


// Hash

unsigned int FNV1A_3d(float x, float y, float z, unsigned int seed);


};

#endif
