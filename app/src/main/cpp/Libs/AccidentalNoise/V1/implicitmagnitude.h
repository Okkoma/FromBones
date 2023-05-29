#ifndef C_IMPLICIT_MAGNITUDE_H
#define C_IMPLICIT_MAGNITUDE_H

#include "implicitmodulebase.h"
#include <cmath>
namespace anl
{
class CImplicitMagnitude : public CImplicitModuleBase
{
public:
    CImplicitMagnitude();
    ~CImplicitMagnitude();

    void setX(float f);
    void setY(float f);
    void setZ(float f);
    void setW(float f);
    void setU(float f);
    void setV(float f);

    void setX(CImplicitModuleBase *f);
    void setY(CImplicitModuleBase *f);
    void setZ(CImplicitModuleBase *f);
    void setW(CImplicitModuleBase *f);
    void setU(CImplicitModuleBase *f);
    void setV(CImplicitModuleBase *f);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

private:
    CScalarParameter m_x, m_y, m_z, m_w, m_u, m_v;
};
};
#endif

