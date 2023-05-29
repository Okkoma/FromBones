#ifndef HSV_H
#define HSV_H

#include "vectortypes.h"

// Utility functions to handle HSV to RGB conversions
namespace anl
{
void HSVtoRGBA(ANLV1_SRGBA &hsv, ANLV1_SRGBA &rgb);
void RGBAtoHSV(ANLV1_SRGBA &rgb, ANLV1_SRGBA &hsv);
};


#endif
