#ifndef IMPLICIT_SCALEOFFSET_H
#define IMPLICIT_SCALEOFFSET_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitScaleOffset : public CImplicitModuleBase
{
public:
    CImplicitScaleOffset(float scale, float offset);
    ~CImplicitScaleOffset();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setScale(float scale);
    void setOffset(float offset);
    void setScale(CImplicitModuleBase *scale);
    void setOffset(CImplicitModuleBase *offset);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    //CImplicitModuleBase *m_source;
    CScalarParameter m_source;
    //float m_scale, m_offset;
    CScalarParameter m_scale, m_offset;
};
};

#endif

