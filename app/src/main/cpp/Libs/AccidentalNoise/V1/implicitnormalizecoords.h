#ifndef IMPLICIT_NORMALIZE_COORDS_H
#define IMPLICIT_NORMALIZE_COORDS_H

#include "implicitmodulebase.h"

namespace anl
{
class CImplicitNormalizeCoords : public CImplicitModuleBase
{
public:
    CImplicitNormalizeCoords();
    CImplicitNormalizeCoords(float length);

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setLength(float v);
    void setLength(CImplicitModuleBase *v);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    CScalarParameter m_length;
};
};

#endif
