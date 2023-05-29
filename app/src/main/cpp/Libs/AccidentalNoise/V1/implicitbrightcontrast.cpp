#include "implicitbrightcontrast.h"

namespace anl
{
CImplicitBrightContrast::CImplicitBrightContrast() : m_source(0), m_bright(0), m_threshold(0), m_factor(1) {}
CImplicitBrightContrast::~CImplicitBrightContrast() {}

void CImplicitBrightContrast::setSource(CImplicitModuleBase *b, int num)
{
    m_source.set(b);
}
void CImplicitBrightContrast::setSource(float v)
{
    m_source.set(v);
}

void CImplicitBrightContrast::setBrightness(float b)
{
    m_bright.set(b);
}
void CImplicitBrightContrast::setContrastThreshold(float t)
{
    m_threshold.set(t);
}
void CImplicitBrightContrast::setContrastFactor(float t)
{
    m_factor.set(t);
}
void CImplicitBrightContrast::setBrightness(CImplicitModuleBase *b)
{
    m_bright.set(b);
}
void CImplicitBrightContrast::setContrastThreshold(CImplicitModuleBase *t)
{
    m_threshold.set(t);
}
void CImplicitBrightContrast::setContrastFactor(CImplicitModuleBase *t)
{
    m_factor.set(t);
}

float CImplicitBrightContrast::get(float x, float y)
{
    float v=m_source.get(x,y);
    // Apply brightness
    v+=m_bright.get(x,y);

    // Subtract threshold, scale by factor, add threshold
    float threshold=m_threshold.get(x,y);
    v-=threshold;
    v*=m_factor.get(x,y);
    v+=threshold;
    return v;
}

float CImplicitBrightContrast::get(float x, float y, float z)
{
    float v=m_source.get(x,y,z);
    // Apply brightness
    v+=m_bright.get(x,y,z);

    // Subtract threshold, scale by factor, add threshold
    float threshold=m_threshold.get(x,y,z);
    v-=threshold;
    v*=m_factor.get(x,y,z);
    v+=threshold;
    return v;
}

float CImplicitBrightContrast::get(float x, float y, float z, float w)
{
    float v=m_source.get(x,y,z,w);
    // Apply brightness
    v+=m_bright.get(x,y,z,w);

    // Subtract threshold, scale by factor, add threshold
    float threshold=m_threshold.get(x,y,z,w);
    v-=threshold;
    v*=m_factor.get(x,y,z,w);
    v+=threshold;
    return v;
}

float CImplicitBrightContrast::get(float x, float y, float z, float w, float u, float v)
{
    float c=m_source.get(x,y,z,w,u,v);
    // Apply brightness
    c+=m_bright.get(x,y,z,w,u,v);

    // Subtract threshold, scale by factor, add threshold
    float threshold=m_threshold.get(x,y,z,w,u,v);
    c-=threshold;
    c*=m_factor.get(x,y,z,w,u,v);
    c+=threshold;
    return c;
}
};
