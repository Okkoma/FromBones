#ifndef IMPLICIT_SELECT_H
#define IMPLICIT_SELECT_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitSelect : public CImplicitModuleBase
{
public:
    CImplicitSelect();
    ~CImplicitSelect();
    void setLowSource(CImplicitModuleBase *b);
    void setHighSource(CImplicitModuleBase *b);
    void setControlSource(CImplicitModuleBase *b);
    void setThreshold(CImplicitModuleBase *m);
    void setFalloff(CImplicitModuleBase *m);
    void setLowSource(float v);
    void setHighSource(float v);
    void setControlSource(float v);
    void setThreshold(float t);
    void setFalloff(float f);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_low, m_high, m_control;
    CScalarParameter m_threshold, m_falloff;

};
};
#endif
