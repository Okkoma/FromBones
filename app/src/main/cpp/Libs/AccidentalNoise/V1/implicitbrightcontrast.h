#ifndef IMPLICIT_BRIGHTCONTRAST_H
#define IMPLICIT_BRIGHTCONTRAST_H

#include "implicitmodulebase.h"
namespace anl
{
class CImplicitBrightContrast : public CImplicitModuleBase
{
public:
    CImplicitBrightContrast();
    ~CImplicitBrightContrast();

    virtual void setSource(float b);	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setBrightness(float b);
    void setContrastThreshold(float t);
    void setContrastFactor(float t);
    void setBrightness(CImplicitModuleBase *m);
    void setContrastThreshold(CImplicitModuleBase *m);
    void setContrastFactor(CImplicitModuleBase *m);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);
protected:
    CScalarParameter m_source;
    CScalarParameter m_bright, m_threshold, m_factor;
};
};

#endif
