#include "implicitbias.h"
#include "../VCommon/utility.h"

namespace anl
{
CImplicitBias::CImplicitBias(float b) : m_source(0), m_bias(b) {}
CImplicitBias::~CImplicitBias() {}

void CImplicitBias::setSource(CImplicitModuleBase *b, int num)
{
    m_source.set(b);
}
void CImplicitBias::setSource(float v)
{
    m_source.set(v);
}

void CImplicitBias::setBias(float b)
{
    m_bias.set(b);
}
void CImplicitBias::setBias(CImplicitModuleBase *m)
{
    m_bias.set(m);
}

float CImplicitBias::get(float x, float y)
{
    float va=m_source.get(x,y);
    return bias(m_bias.get(x,y), va);
}

float CImplicitBias::get(float x, float y, float z)
{
    float va=m_source.get(x,y,z);
    return bias(m_bias.get(x,y,z), va);
}

float CImplicitBias::get(float x, float y, float z, float w)
{
    float va=m_source.get(x,y,z,w);
    return bias(m_bias.get(x,y,z,w), va);
}

float CImplicitBias::get(float x, float y, float z, float w, float u, float v)
{
    float va=m_source.get(x,y,z,w, u, v);
    return bias(m_bias.get(x,y,z,w,u,v), va);
}
};
