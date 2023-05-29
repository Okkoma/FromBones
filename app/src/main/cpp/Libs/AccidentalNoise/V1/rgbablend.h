#ifndef RGBA_BLEND_H
#define RGBA_BLEND_H
#include "rgbamodulebase.h"
#include "implicitmodulebase.h"

namespace anl
{
class CRGBABlend : public CRGBAModuleBase
{
public:
    CRGBABlend();
    ~CRGBABlend();

    void setLowSource(CRGBAModuleBase *m);
    void setHighSource(CRGBAModuleBase *m);
    void setLowSource(float r, float g, float b, float a);
    void setHighSource(float r, float g, float b, float a);
    void setControlSource(CImplicitModuleBase *m);
    void setControlSource(float v);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_low, m_high;
    CScalarParameter m_control;
};
};

#endif
