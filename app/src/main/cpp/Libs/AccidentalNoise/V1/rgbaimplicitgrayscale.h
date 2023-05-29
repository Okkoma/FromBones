#ifndef RGBA_IMPLICITGRAYSCALE_H
#define RGBA_IMPLICITGRAYSCALE_H
#include "rgbamodulebase.h"
#include "implicitmodulebase.h"

/***************************************************************
CRGBAImplicitGrayscale

    Given an input source (CImplicitModuleBase), output an RGBA value
that represents the grayscale value of the input, with an alpha of 1
****************************************************************/

namespace anl
{
class CRGBAImplicitGrayscale : public CRGBAModuleBase
{
public:
    CRGBAImplicitGrayscale();
    ~CRGBAImplicitGrayscale();

    void setSource(CImplicitModuleBase *m);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CImplicitModuleBase *m_source;
};
};

#endif
