#ifndef IMPLICIT_Pow_H
#define IMPLICIT_Pow_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitPow : public CImplicitModuleBase
{
public:
    CImplicitPow();
    ~CImplicitPow();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setPower(float v);
    void setPower(CImplicitModuleBase *m);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);


protected:
    CScalarParameter m_source, m_power;
};
};

#endif
