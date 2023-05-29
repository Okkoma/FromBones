// Lookup tables for 2D, 4D and 6D gradient oise.
// Generated with boost::random, using a lagged Fibonacci generator and a uniform_on_sphere distribution.


#ifndef NOISE_LUT_H
#define NOISE_LUT_H

extern float gradient2D_lut[8][2];
extern float gradient3D_lut[24][3];
extern float gradient4D_lut[64][4];
extern float gradient6D_lut[192][6];
extern float whitenoise_lut[256];


#endif
