#include "implicitpow.h"
#include <math.h>

namespace anl
{
CImplicitPow::CImplicitPow() : m_source(0), m_power(1) {}
CImplicitPow::~CImplicitPow() {}

void CImplicitPow::setSource(float v)
{
    m_source.set(v);
}
void CImplicitPow::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}
void CImplicitPow::setPower(float v)
{
    m_power.set(v);
}
void CImplicitPow::setPower(CImplicitModuleBase *m)
{
    m_power.set(m);
}

float CImplicitPow::get(float x, float y)
{
    return pow(m_source.get(x,y), m_power.get(x,y));
}
float CImplicitPow::get(float x, float y, float z)
{
    return pow(m_source.get(x,y,z), m_power.get(x,y,z));
}
float CImplicitPow::get(float x, float y, float z, float w)
{
    return pow(m_source.get(x,y,z,w), m_power.get(x,y,z,w));
}
float CImplicitPow::get(float x, float y, float z, float w, float u, float v)
{
    return pow(m_source.get(x,y,z,w,u,v), m_power.get(x,y,z,w,u,v));
}
};
