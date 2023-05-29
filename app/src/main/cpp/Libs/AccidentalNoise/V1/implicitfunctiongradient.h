#ifndef IMPLICIT_FUNCTIONGRADIENT_H
#define IMPLICIT_FUNCTIONGRADIENT_H
#include "implicitmodulebase.h"

namespace anl
{
enum EFunctionGradientAxis
{
    X_AXIS,
    Y_AXIS,
    Z_AXIS,
    W_AXIS,
    U_AXIS,
    V_AXIS
};

class CImplicitFunctionGradient : public CImplicitModuleBase
{
public:
    CImplicitFunctionGradient();
    ~CImplicitFunctionGradient();

    virtual void setSource(float b);           	        	// Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    void setAxis(int axis);
    void setSpacing(float s);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    int m_axis;
    float m_spacing;

};
};

#endif
