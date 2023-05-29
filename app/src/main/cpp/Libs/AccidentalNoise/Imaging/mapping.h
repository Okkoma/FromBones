#ifndef ANLMAPPING_H
#define ANLMAPPING_H

#include "../templates/tarrays.h"

namespace anl
{
typedef TArray2D<float> CArray2Dd;
typedef TArray3D<float> CArray3Dd;
typedef TArray2D<SRGBA> CArray2Drgba;
typedef TArray3D<SRGBA> CArray3Drgba;
typedef TArray2D<ANLV1_SRGBA> CArray2Drgbav1;

enum EMappingModes
{
    SEAMLESS_NONE,
    SEAMLESS_X,
    SEAMLESS_Y,
    SEAMLESS_Z,
    SEAMLESS_XY,
    SEAMLESS_XZ,
    SEAMLESS_YZ,
    SEAMLESS_XYZ
};

struct SMappingRanges
{
    float mapx0,mapy0,mapz0, mapx1,mapy1,mapz1;
    float loopx0,loopy0,loopz0, loopx1,loopy1,loopz1;

    SMappingRanges()
    {
        mapx0=mapy0=mapz0=loopx0=loopy0=loopz0=-1;
        mapx1=mapy1=mapz1=loopx1=loopy1=loopz1=1;
    };

    SMappingRanges(const anl::SMappingRanges &rhs)
    {
        mapx0=rhs.mapx0;
        mapx1=rhs.mapx1;
        mapy0=rhs.mapy0;
        mapy1=rhs.mapy1;
        mapz0=rhs.mapz0;
        mapz1=rhs.mapz1;

        loopx0=rhs.loopx0;
        loopx1=rhs.loopx1;
        loopy0=rhs.loopy0;
        loopy1=rhs.loopy1;
        loopz0=rhs.loopz0;
        loopz1=rhs.loopz1;
    }

    SMappingRanges(float x0, float x1, float y0, float y1, float z0=0.0, float z1=1.0)
    {
        mapx0=x0;
        mapx1=x1;
        mapy0=y0;
        mapy1=y1;
        mapz0=z0;
        mapz1=z1;

        loopx0=x0;
        loopx1=x1;
        loopy0=y0;
        loopy1=y1;
        loopz0=z0;
        loopz1=z1;
    }
};
};
#endif
