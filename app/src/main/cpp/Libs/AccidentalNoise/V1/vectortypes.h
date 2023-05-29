#ifndef ANLV1_VECTOR_TYPES
#define ANLV1_VECTOR_TYPES

#include "templates/tvec2d.h"
#include "templates/tvec3d.h"
#include "templates/tvec4d.h"

namespace anl
{
typedef TVec2D<float> CVec2f;
typedef TVec3D<float> CVec3f;
typedef TVec4D<float> CVec4f;

typedef TVec2D<int> CVec2i;
typedef TVec3D<int> CVec3i;
typedef TVec4D<int> CVec4i;

typedef TVec2D<unsigned int> CVec2ui;
typedef TVec3D<unsigned int> CVec3ui;
typedef TVec4D<unsigned int> CVec4ui;

typedef TVec3D<unsigned char> SRGBuc;
typedef TVec4D<unsigned char> SRGBAuc;

typedef TVec3D<float> SRGB;
typedef TVec4D<float> ANLV1_SRGBA;
};

#endif
