#ifndef IMPLICIT_BASIS_FUNCTION_H
#define IMPLICIT_BASIS_FUNCTION_H
#include "../VCommon/noise_gen.h"
#include "implicitmodulebase.h"

namespace anl
{

enum EBasisTypes
{
    VALUE,
    GRADIENT,
    GRADVAL,
    SIMPLEX,
    WHITE
};

enum EInterpTypes
{
    NONE,
    LINEAR,
    CUBIC,
    QUINTIC
};


class CImplicitBasisFunction : public CImplicitModuleBase
{
public:
    CImplicitBasisFunction();
    CImplicitBasisFunction(int type, int interp);

    void setType(int type);
    void setInterp(int interp);
    void setRotationAngle(float ax, float ay, float az, float angle);

    void setSeed(unsigned int seed);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);


protected:

    float m_scale[4], m_offset[4];
    interp_func m_interp;
    noise_func2 m_2d;
    noise_func3 m_3d;
    noise_func4 m_4d;
    noise_func6 m_6d;
    unsigned int m_seed;

    float m_rotmatrix[3][3];
    float cos2d,sin2d;

    void setMagicNumbers(int type);

};

};

#endif
