#ifndef IMPLICIT_SIN_H
#define IMPLICIT_SIN_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitSin : public CImplicitModuleBase
{
public:
    CImplicitSin();
    ~CImplicitSin();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
};
};

#endif

