#include "implicitblend.h"
#include "../VCommon/utility.h"

namespace anl
{

CImplicitBlend::CImplicitBlend(): m_low(0), m_high(0), m_control(0) {}
CImplicitBlend::~CImplicitBlend() {}

void CImplicitBlend::setLowSource(CImplicitModuleBase *b)
{
    m_low.set(b);
}

void CImplicitBlend::setHighSource(CImplicitModuleBase *b)
{
    m_high.set(b);
}

void CImplicitBlend::setControlSource(CImplicitModuleBase *b)
{
    m_control.set(b);
}

void CImplicitBlend::setLowSource(float v)
{
    m_low.set(v);
}

void CImplicitBlend::setHighSource(float v)
{
    m_high.set(v);
}

void CImplicitBlend::setControlSource(float v)
{
    m_control.set(v);
}

float CImplicitBlend::get(float x, float y)
{
    float v1=m_low.get(x,y);
    float v2=m_high.get(x,y);
    float blend=m_control.get(x,y);
    blend=(blend+1.0) * 0.5;

    return lerp(blend,v1,v2);
}

float CImplicitBlend::get(float x, float y, float z)
{
    float v1=m_low.get(x,y,z);
    float v2=m_high.get(x,y,z);
    float blend=m_control.get(x,y,z);
    return lerp(blend,v1,v2);
}

float CImplicitBlend::get(float x, float y, float z, float w)
{
    float v1=m_low.get(x,y,z,w);
    float v2=m_high.get(x,y,z,w);
    float blend=m_control.get(x,y,z,w);
    return lerp(blend,v1,v2);
}

float CImplicitBlend::get(float x, float y, float z, float w, float u, float v)
{
    float v1=m_low.get(x,y,z,w,u,v);
    float v2=m_high.get(x,y,z,w,u,v);
    float blend=m_control.get(x,y,z,w,u,v);
    return lerp(blend,v1,v2);
}
};
