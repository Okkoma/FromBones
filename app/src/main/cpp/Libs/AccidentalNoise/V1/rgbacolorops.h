#ifndef RGBA_COLOR_OPS_H
#define RGBA_COLOR_OPS_H
#include "rgbamodulebase.h"

namespace anl
{
enum EColorOperations
{
    COLORMULTIPLY,
    COLORADD,
    SCREEN,
    OVERLAY,
    SOFTLIGHT,
    HARDLIGHT,
    DODGE,
    BURN,
    LINEARDODGE,
    LINEARBURN
};
class CRGBAColorOps : public CRGBAModuleBase
{
public:
    CRGBAColorOps();
    CRGBAColorOps(int op);
    ~CRGBAColorOps();

    void setOperation(int op);

    void setSource1(float r, float g, float b, float a);
    void setSource1(CRGBAModuleBase *m);
    void setSource2(float r, float g, float b, float a);
    void setSource2(CRGBAModuleBase *m);

    ANLV1_SRGBA get(float x, float y);
    ANLV1_SRGBA get(float x, float y, float z);
    ANLV1_SRGBA get(float x, float y, float z, float w);
    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v);

protected:
    CRGBAParameter m_source1, m_source2;
    int m_op;

    ANLV1_SRGBA multiply(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA add(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA screen(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA overlay(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA softLight(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA hardLight(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA dodge(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA burn(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA linearDodge(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
    ANLV1_SRGBA linearBurn(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2);
};
};

#endif
