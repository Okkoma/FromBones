#ifndef IMPLICIT_RGBADOTPRODUCT_H
#define IMPLICIT_RGBADOTPRODUCT_H
#include "implicitmodulebase.h"
#include "rgbamodulebase.h"

namespace anl
{
class CImplicitRGBADotProduct : public CImplicitModuleBase
{
public:
    CImplicitRGBADotProduct();
    ~CImplicitRGBADotProduct();

    void setSource1(CRGBAModuleBase *m);
    void setSource1(float r, float g, float b, float a);
    void setSource2(CRGBAModuleBase *m);
    void setSource2(float r, float g, float b, float a);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_source1, m_source2;
};
};

#endif
