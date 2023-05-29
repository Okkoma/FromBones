#ifndef IMPLICIT_SCALEDOMAIN_H
#define IMPLICIT_SCALEDOMAIN_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitScaleDomain : public CImplicitModuleBase
{
public:
    CImplicitScaleDomain();
    CImplicitScaleDomain(float x, float y, float z=1, float w=1, float u=1, float v=1);
    void setScale(float x, float y, float z=1, float w=1, float u=1, float v=1);
    void setXScale(float x);
    void setYScale(float x);
    void setZScale(float x);
    void setWScale(float x);
    void setUScale(float x);
    void setVScale(float x);
    void setXScale(CImplicitModuleBase *x);
    void setYScale(CImplicitModuleBase *y);
    void setZScale(CImplicitModuleBase *z);
    void setWScale(CImplicitModuleBase *w);
    void setUScale(CImplicitModuleBase *u);
    void setVScale(CImplicitModuleBase *v);
    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    float getXScale() const
    {
        return m_sx.get();
    }
    float getYScale() const
    {
        return m_sy.get();
    }

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    //CImplicitModuleBase *m_source;
    CScalarParameter m_source;
    CScalarParameter m_sx, m_sy, m_sz, m_sw, m_su, m_sv;
};
};

#endif
