#ifndef IMPLICIT_BLEND_H
#define IMPLICIT_BLEND_H
#include "implicitmodulebase.h"

namespace anl
{

class CImplicitBlend : public CImplicitModuleBase
{
public:
    CImplicitBlend();
    ~CImplicitBlend();

    void setLowSource(CImplicitModuleBase *b);
    void setHighSource(CImplicitModuleBase *b);
    void setControlSource(CImplicitModuleBase *b);
    void setLowSource(float v);
    void setHighSource(float v);
    void setControlSource(float v);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);
protected:
    CScalarParameter m_low,m_high,m_control;

};
};
#endif
