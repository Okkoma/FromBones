#include "implicitclamp.h"
#include "../VCommon/utility.h"

namespace anl
{
CImplicitClamp::CImplicitClamp(float low, float high) : m_source(0), m_low(low), m_high(high) {}
CImplicitClamp::~CImplicitClamp() {}

void CImplicitClamp::setRange(float low, float high)
{
    m_low=low;
    m_high=high;
}
void CImplicitClamp::setSource(CImplicitModuleBase *b, int num) // Modified For FromBones Project.
{
    m_source=b;
}

float CImplicitClamp::get(float x, float y)
{
    if(!m_source) return 0;

    return clamp(m_source->get(x,y),m_low,m_high);
}

float CImplicitClamp::get(float x, float y, float z)
{
    if(!m_source) return 0;

    return clamp(m_source->get(x,y,z),m_low,m_high);
}

float CImplicitClamp::get(float x, float y, float z, float w)
{
    if(!m_source) return 0;

    return clamp(m_source->get(x,y,z,w),m_low,m_high);
}

float CImplicitClamp::get(float x, float y, float z, float w, float u, float v)
{
    if(!m_source) return 0;

    return clamp(m_source->get(x,y,z,w,u,v),m_low,m_high);
}
};
