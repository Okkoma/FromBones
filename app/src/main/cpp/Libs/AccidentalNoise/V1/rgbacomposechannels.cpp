#include "rgbacomposechannels.h"
#include "hsv.h"

namespace anl
{
CRGBACompositeChannels::CRGBACompositeChannels() : m_c1(0), m_c2(0), m_c3(0), m_c4(1), m_mode(RGB) {};
CRGBACompositeChannels::CRGBACompositeChannels(int mode) : m_c1(0), m_c2(0), m_c3(0), m_c4(1), m_mode(mode) {}
CRGBACompositeChannels::~CRGBACompositeChannels() {}

void CRGBACompositeChannels::setRedSource(CImplicitModuleBase *m)
{
    m_c1.set(m);
}
void CRGBACompositeChannels::setGreenSource(CImplicitModuleBase *m)
{
    m_c2.set(m);
}
void CRGBACompositeChannels::setBlueSource(CImplicitModuleBase *m)
{
    m_c3.set(m);
}
void CRGBACompositeChannels::setHueSource(CImplicitModuleBase *m)
{
    m_c1.set(m);
}
void CRGBACompositeChannels::setSatSource(CImplicitModuleBase *m)
{
    m_c2.set(m);
}
void CRGBACompositeChannels::setValSource(CImplicitModuleBase *m)
{
    m_c3.set(m);
}
void CRGBACompositeChannels::setAlphaSource(CImplicitModuleBase *m)
{
    m_c4.set(m);
}
void CRGBACompositeChannels::setRedSource(float r)
{
    m_c1.set(r);
}
void CRGBACompositeChannels::setGreenSource(float g)
{
    m_c2.set(g);
}
void CRGBACompositeChannels::setBlueSource(float b)
{
    m_c3.set(b);
}
void CRGBACompositeChannels::setAlphaSource(float a)
{
    m_c4.set(a);
}
void CRGBACompositeChannels::setHueSource(float h)
{
    m_c1.set(h);
}
void CRGBACompositeChannels::setSatSource(float s)
{
    m_c2.set(s);
}
void CRGBACompositeChannels::setValSource(float v)
{
    m_c3.set(v);
}

ANLV1_SRGBA CRGBACompositeChannels::get(float x, float y)
{
    float r=(float)m_c1.get(x,y);
    float g=(float)m_c2.get(x,y);
    float b=(float)m_c3.get(x,y);
    float a=(float)m_c4.get(x,y);
    if(m_mode==RGB) return ANLV1_SRGBA(r,g,b,a);
    else
    {
        ANLV1_SRGBA hsv(r,g,b,a);
        ANLV1_SRGBA rgb;
        HSVtoRGBA(hsv,rgb);
        return rgb;
    }
}
ANLV1_SRGBA CRGBACompositeChannels::get(float x, float y, float z)
{
    float r=(float)m_c1.get(x,y,z);
    float g=(float)m_c2.get(x,y,z);
    float b=(float)m_c3.get(x,y,z);
    float a=(float)m_c4.get(x,y,z);
    if(m_mode==RGB) return ANLV1_SRGBA(r,g,b,a);
    else
    {
        ANLV1_SRGBA hsv(r,g,b,a);
        ANLV1_SRGBA rgb;
        HSVtoRGBA(hsv,rgb);
        return rgb;
    }
}
ANLV1_SRGBA CRGBACompositeChannels::get(float x, float y, float z, float w)
{
    float r=(float)m_c1.get(x,y,z,w);
    float g=(float)m_c2.get(x,y,z,w);
    float b=(float)m_c3.get(x,y,z,w);
    float a=(float)m_c4.get(x,y,z,w);
    if(m_mode==RGB) return ANLV1_SRGBA(r,g,b,a);
    else
    {
        ANLV1_SRGBA hsv(r,g,b,a);
        ANLV1_SRGBA rgb;
        HSVtoRGBA(hsv,rgb);
        return rgb;
    }
}
ANLV1_SRGBA CRGBACompositeChannels::get(float x, float y, float z, float w, float u, float v)
{
    float r=(float)m_c1.get(x,y,z,w,u,v);
    float g=(float)m_c2.get(x,y,z,w,u,v);
    float b=(float)m_c3.get(x,y,z,w,u,v);
    float a=(float)m_c4.get(x,y,z,w,u,v);
    if(m_mode==RGB) return ANLV1_SRGBA(r,g,b,a);
    else
    {
        ANLV1_SRGBA hsv(r,g,b,a);
        ANLV1_SRGBA rgb;
        HSVtoRGBA(hsv,rgb);
        return rgb;
    }
}
};
