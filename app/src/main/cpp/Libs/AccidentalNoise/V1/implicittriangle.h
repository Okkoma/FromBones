#ifndef IMPLICIT_TRIANGLE_H
#define IMPLICIT_TRIANGLE_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitTriangle : public CImplicitModuleBase
{
public:
    CImplicitTriangle(float period, float offset);
    ~CImplicitTriangle();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setPeriod(float p);
    void setPeriod(CImplicitModuleBase *p);
    void setOffset(float o);
    void setOffset(CImplicitModuleBase *o);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source, m_period, m_offset;
};
};

#endif
