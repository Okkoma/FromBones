#ifndef RGBA_CURVE_H
#define RGBA_CURVE_H
#include "rgbamodulebase.h"
#include "implicitbasisfunction.h"
#include "templates/tcurve.h"

namespace anl
{
class CRGBACurve : public CRGBAModuleBase
{
public:
    CRGBACurve();
    ~CRGBACurve();

    void pushPoint(float t, float r, float g, float b, float a);
    void clearCurve();
    void setSource(float t);
    void setSource(CImplicitModuleBase *m);
    void setInterpType(int type);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    TCurve<ANLV1_SRGBA> m_curve;
    CScalarParameter m_source;
    int m_type;
};
};

#endif
