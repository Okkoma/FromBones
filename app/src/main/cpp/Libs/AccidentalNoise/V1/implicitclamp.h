#ifndef IMPLICIT_CLAMP_H
#define IMPLICIT_CLAMP_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitClamp : public CImplicitModuleBase
{
public:
    CImplicitClamp(float low, float high);
    ~CImplicitClamp();

    void setRange(float low, float high);
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CImplicitModuleBase *m_source;
    float m_low, m_high;
};
};
#endif
