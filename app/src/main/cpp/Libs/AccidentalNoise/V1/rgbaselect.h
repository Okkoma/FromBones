#ifndef RGBA_SELECT_H
#define RGBA_SELECT_H
#include "rgbamodulebase.h"
#include "implicitmodulebase.h"

namespace anl
{
class CRGBASelect : public CRGBAModuleBase
{
public:
    CRGBASelect();
    CRGBASelect(float thresh, float fall);

    void setLowSource(CRGBAModuleBase *m);
    void setHighSource(CRGBAModuleBase *m);
    void setLowSource(float r, float g, float b, float a);
    void setHighSource(float r, float g, float b, float a);
    void setControlSource(CImplicitModuleBase *m);
    void setThreshold(CImplicitModuleBase *m);
    void setFalloff(CImplicitModuleBase *m);
    void setControlSource(float v);
    void setThreshold(float v);
    void setFalloff(float v);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_low, m_high;
    CScalarParameter m_control, m_threshold, m_falloff;
};
};

#endif
