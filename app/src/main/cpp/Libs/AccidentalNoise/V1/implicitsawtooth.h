#ifndef IMPLICIT_SAWTOOTH_H
#define IMPLICIT_SAWTOOTH_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitSawtooth : public CImplicitModuleBase
{
public:
    CImplicitSawtooth(float period);
    ~CImplicitSawtooth();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setPeriod(CImplicitModuleBase *p);
    void setPeriod(float p);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    CScalarParameter m_period;

};
};
#endif
