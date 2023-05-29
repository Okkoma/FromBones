#ifndef IMPLICIT_TIERS_H
#define IMPLICIT_TIERS_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitTiers : public CImplicitModuleBase
{
public:
    CImplicitTiers();
    CImplicitTiers(int numtiers, bool smooth);
    ~CImplicitTiers();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setNumTiers(int numtiers);
    void setSmooth(bool smooth);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    int m_numtiers;
    bool m_smooth;
};
};

#endif
