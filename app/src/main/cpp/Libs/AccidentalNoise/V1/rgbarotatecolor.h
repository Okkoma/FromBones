#ifndef RGBA_ROTATECOLOR_H
#define RGBA_ROTATECOLOR_H
#include "rgbamodulebase.h"
#include "implicitmodulebase.h"

namespace anl
{
class CRGBARotateColor : public CRGBAModuleBase
{
public:
    CRGBARotateColor();
    void setAxis(float ax, float ay, float az);
    void setAxis(CImplicitModuleBase *ax, CImplicitModuleBase *ay, CImplicitModuleBase *az);
    void setAxisX(float ax);
    void setAxisY(float ay);
    void setAxisZ(float az);
    void setAxisX(CImplicitModuleBase *ax);
    void setAxisY(CImplicitModuleBase *ay);
    void setAxisZ(CImplicitModuleBase *az);

    void setNormalizeAxis(bool n)
    {
        m_normalizeaxis=n;
    }

    void setAngle(float a);
    void setAngle(CImplicitModuleBase *a);

    void setSource(CRGBAModuleBase *m);
    void setSource(float r, float g, float b, float a);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_ax, m_ay, m_az, m_angledeg;
    CRGBAParameter m_source;
    bool m_normalizeaxis;
    float m_rotmatrix[3][3];
    void calculateRotMatrix(float x, float y);
    void calculateRotMatrix(float x, float y, float z);
    void calculateRotMatrix(float x, float y, float z, float w);
    void calculateRotMatrix(float x, float y, float z, float w, float u, float v);
};
};

#endif
