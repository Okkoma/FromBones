#ifndef IMPLICIT_TRANSLATE_DOMAIN_H
#define IMPLICIT_TRANSLATE_DOMAIN_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitTranslateDomain : public CImplicitModuleBase
{
public:
    CImplicitTranslateDomain();
    ~CImplicitTranslateDomain();

    void setXAxisSource(CImplicitModuleBase *m);
    void setYAxisSource(CImplicitModuleBase *m);
    void setZAxisSource(CImplicitModuleBase *m);
    void setWAxisSource(CImplicitModuleBase *m);
    void setUAxisSource(CImplicitModuleBase *m);
    void setVAxisSource(CImplicitModuleBase *m);

    void setXAxisSource(float v);
    void setYAxisSource(float v);
    void setZAxisSource(float v);
    void setWAxisSource(float v);
    void setUAxisSource(float v);
    void setVAxisSource(float v);

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    float getXTranslate() const
    {
        return m_ax.get();
    }
    float getYTranslate() const
    {
        return m_ay.get();
    }

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source, m_ax, m_ay, m_az, m_aw, m_au, m_av;
};
};

#endif
