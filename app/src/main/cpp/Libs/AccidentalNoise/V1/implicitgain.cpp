#include "implicitgain.h"
#include "../VCommon/utility.h"

namespace anl
{
CImplicitGain::CImplicitGain(float b) : m_source(0), m_gain(b) {}
CImplicitGain::~CImplicitGain() {}

void CImplicitGain::setSource(float v)
{
    m_source.set(v);
}
void CImplicitGain::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}
void CImplicitGain::setGain(float b)
{
    m_gain.set(b);
}
void CImplicitGain::setGain(CImplicitModuleBase *m)
{
    m_gain.set(m);
}

float CImplicitGain::get(float x, float y)
{
    float v=m_source.get(x,y);
    return gain(m_gain.get(x,y), v);
}

float CImplicitGain::get(float x, float y, float z)
{

    float v=m_source.get(x,y,z);
    return gain(m_gain.get(x,y,z), v);
}

float CImplicitGain::get(float x, float y, float z, float w)
{

    float v=m_source.get(x,y,z,w);
    return gain(m_gain.get(x,y,z,w), v);
}

float CImplicitGain::get(float x, float y, float z, float w, float u, float v)
{
    float va=m_source.get(x,y,z,w, u, v);
    return gain(m_gain.get(x,y,z,w,u,v), va);
}
};
