#ifndef IMPLICIT_BIAS_H
#define IMPLICIT_BIAS_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitBias : public CImplicitModuleBase
{
public:
    CImplicitBias(float b);
    ~CImplicitBias();

    virtual void setSource(float b);	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setBias(float b);
    void setBias(CImplicitModuleBase *m);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    CScalarParameter m_bias;
};
};

#endif
