#include "implicitsawtooth.h"
#include <cmath>
namespace anl
{
CImplicitSawtooth::CImplicitSawtooth(float period) :
    m_source(0), m_period(period)
{
}
CImplicitSawtooth::~CImplicitSawtooth()
{
}

void CImplicitSawtooth::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

void CImplicitSawtooth::setSource(float s)
{
    m_source.set(s);
}

void CImplicitSawtooth::setPeriod(CImplicitModuleBase *p)
{
    m_period.set(p);
}

void CImplicitSawtooth::setPeriod(float p)
{
    m_period.set(p);
}

float CImplicitSawtooth::get(float x, float y)
{
    float val=m_source.get(x,y);
    float p=m_period.get(x,y);

    return 2.0*(val/p - floor(0.5 + val/p));
}

float CImplicitSawtooth::get(float x, float y, float z)
{
    float val=m_source.get(x,y,z);
    float p=m_period.get(x,y,z);

    return 2.0*(val/p - floor(0.5 + val/p));
}

float CImplicitSawtooth::get(float x, float y, float z, float w)
{
    float val=m_source.get(x,y,z,w);
    float p=m_period.get(x,y,z,w);

    return 2.0*(val/p - floor(0.5 + val/p));
}

float CImplicitSawtooth::get(float x, float y, float z, float w, float u, float v)
{
    float val=m_source.get(x,y,z,w,u,v);
    float p=m_period.get(x,y,z,w,u,v);

    return 2.0*(val/p - floor(0.5 + val/p));
}
};
