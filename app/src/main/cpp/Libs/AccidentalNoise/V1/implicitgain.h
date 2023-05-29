#ifndef IMPLICIT_GAIN_H
#define IMPLICIT_GAIN_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitGain : public CImplicitModuleBase
{
public:
    CImplicitGain(float b);
    ~CImplicitGain();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setGain(float b);
    void setGain(CImplicitModuleBase *m);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    CScalarParameter m_gain;
};
};

#endif

