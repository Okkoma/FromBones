#ifndef IMPLICIT_CURVE_H
#define IMPLICIT_CURVE_H
#include "implicitmodulebase.h"
#include "implicitbasisfunction.h"
#include "templates/tcurve.h"
namespace anl
{
class CImplicitCurve : public CImplicitModuleBase
{
public:
    CImplicitCurve();
    ~CImplicitCurve();

    void pushPoint(float t, float v);
    void clearCurve();
    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.
    void setInterpType(int type);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    TCurve<float> m_curve;
    CScalarParameter m_source;
    int m_type;
};
};

#endif
