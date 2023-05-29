#include "rgbablend.h"

namespace anl
{
CRGBABlend::CRGBABlend() : m_low(0,0,0,0), m_high(0,0,0,0), m_control(0) {}
CRGBABlend::~CRGBABlend() {}

void CRGBABlend::setLowSource(CRGBAModuleBase *m)
{
    m_low.set(m);
}

void CRGBABlend::setHighSource(CRGBAModuleBase *m)
{
    m_high.set(m);
}

void CRGBABlend::setLowSource(float r, float g, float b, float a)
{
    m_low.set(r,g,b,a);
}

void CRGBABlend::setHighSource(float r, float g, float b, float a)
{
    m_high.set(r,g,b,a);
}

void CRGBABlend::setControlSource(CImplicitModuleBase *m)
{
    m_control.set(m);
}

void CRGBABlend::setControlSource(float v)
{
    m_control.set(v);
}

ANLV1_SRGBA CRGBABlend::get(float x, float y)
{
    ANLV1_SRGBA low=m_low.get(x,y);
    ANLV1_SRGBA high=m_high.get(x,y);
    float control=m_control.get(x,y);

    return ANLV1_SRGBA(
               (float)(low[0]+control*(high[0]-low[0])),
               (float)(low[1]+control*(high[1]-low[1])),
               (float)(low[2]+control*(high[2]-low[2])),
               (float)(low[3]+control*(high[3]-low[3])));
}

ANLV1_SRGBA CRGBABlend::get(float x, float y, float z)
{
    ANLV1_SRGBA low=m_low.get(x,y,z);
    ANLV1_SRGBA high=m_high.get(x,y,z);
    float control=m_control.get(x,y,z);

    return ANLV1_SRGBA(
               (float)(low[0]+control*(high[0]-low[0])),
               (float)(low[1]+control*(high[1]-low[1])),
               (float)(low[2]+control*(high[2]-low[2])),
               (float)(low[3]+control*(high[3]-low[3])));
}

ANLV1_SRGBA CRGBABlend::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA low=m_low.get(x,y,z,w);
    ANLV1_SRGBA high=m_high.get(x,y,z,w);
    float control=m_control.get(x,y,z,w);

    return ANLV1_SRGBA(
               (float)(low[0]+control*(high[0]-low[0])),
               (float)(low[1]+control*(high[1]-low[1])),
               (float)(low[2]+control*(high[2]-low[2])),
               (float)(low[3]+control*(high[3]-low[3])));
}
ANLV1_SRGBA CRGBABlend::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA low=m_low.get(x,y,z,w,u,v);
    ANLV1_SRGBA high=m_high.get(x,y,z,w,u,v);
    float control=m_control.get(x,y,z,w,u,v);

    return ANLV1_SRGBA(
               (float)(low[0]+control*(high[0]-low[0])),
               (float)(low[1]+control*(high[1]-low[1])),
               (float)(low[2]+control*(high[2]-low[2])),
               (float)(low[3]+control*(high[3]-low[3])));
}
};
