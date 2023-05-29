#include "rgbargbatohsv.h"
#include "hsv.h"

namespace anl
{
CRGBARGBAToHSV::CRGBARGBAToHSV() : m_source(0,0,0,0) {}

void CRGBARGBAToHSV::setSource(CRGBAModuleBase *m)
{
    m_source.set(m);
}
void CRGBARGBAToHSV::setSource(float r, float g, float b, float a)
{
    m_source.set(r,g,b,a);
}

ANLV1_SRGBA CRGBARGBAToHSV::get(float x, float y)
{
    ANLV1_SRGBA s=m_source.get(x,y);
    ANLV1_SRGBA d;
    RGBAtoHSV(s,d);
    return d;
}
ANLV1_SRGBA CRGBARGBAToHSV::get(float x, float y, float z)
{
    ANLV1_SRGBA s=m_source.get(x,y,z);
    ANLV1_SRGBA d;
    RGBAtoHSV(s,d);
    return d;
}
ANLV1_SRGBA CRGBARGBAToHSV::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w);
    ANLV1_SRGBA d;
    RGBAtoHSV(s,d);
    return d;
}
ANLV1_SRGBA CRGBARGBAToHSV::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w,u,v);
    ANLV1_SRGBA d;
    RGBAtoHSV(s,d);
    return d;
}
};
