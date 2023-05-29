#ifndef RGBA_BLENDOPS_H
#define RGBA_BLENDOPS_H
#include "rgbamodulebase.h"

/*********************************************************
CRGBABlendOps

    Provide raster-ops-style blending of two RGBA channels.
**********************************************************/

namespace anl
{
enum EBlendOps
{
    SRC1_ALPHA,
    SRC2_ALPHA,
    ONE_MINUS_SRC1_ALPHA,
    ONE_MINUS_SRC2_ALPHA,
    ONE,
    ZERO
};

class CRGBABlendOps : public CRGBAModuleBase
{
public:
    CRGBABlendOps();
    CRGBABlendOps(int src1mode, int src2mode);
    ~CRGBABlendOps();

    void setSrc1Mode(int mode);
    void setSrc2Mode(int mode);
    void setSource1(CRGBAModuleBase *m);
    void setSource2(CRGBAModuleBase *m);
    void setSource1(float r, float g, float b, float a);
    void setSource2(float r, float g, float b, float a);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_source1, m_source2;
    int m_src1blend, m_src2blend;

    ANLV1_SRGBA blendRGBAs(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
};
};

#endif
