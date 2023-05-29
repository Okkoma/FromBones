#include "implicitscaledomain.h"

namespace anl
{
CImplicitScaleDomain::CImplicitScaleDomain() : m_source(0),m_sx(1),m_sy(1),m_sz(1),m_sw(1),m_su(1),m_sv(1) {}
CImplicitScaleDomain::CImplicitScaleDomain(float x, float y, float z, float w, float u, float v):m_source(0),
    m_sx(x),m_sy(y),m_sz(z),m_sw(w),m_su(u),m_sv(v) {}

void CImplicitScaleDomain::setScale(float x, float y, float z, float w, float u, float v)
{
    m_sx.set(x);
    m_sy.set(y);
    m_sz.set(z);
    m_sw.set(w);
    m_su.set(u);
    m_sv.set(v);
}

void CImplicitScaleDomain::setXScale(float x)
{
    m_sx.set(x);
}
void CImplicitScaleDomain::setYScale(float x)
{
    m_sy.set(x);
}
void CImplicitScaleDomain::setZScale(float x)
{
    m_sz.set(x);
}
void CImplicitScaleDomain::setWScale(float x)
{
    m_sw.set(x);
}
void CImplicitScaleDomain::setUScale(float x)
{
    m_su.set(x);
}
void CImplicitScaleDomain::setVScale(float x)
{
    m_sv.set(x);
}
void CImplicitScaleDomain::setXScale(CImplicitModuleBase *x)
{
    m_sx.set(x);
}
void CImplicitScaleDomain::setYScale(CImplicitModuleBase *y)
{
    m_sy.set(y);
}
void CImplicitScaleDomain::setZScale(CImplicitModuleBase *z)
{
    m_sz.set(z);
}
void CImplicitScaleDomain::setWScale(CImplicitModuleBase *w)
{
    m_sw.set(w);
}
void CImplicitScaleDomain::setUScale(CImplicitModuleBase *u)
{
    m_su.set(u);
}
void CImplicitScaleDomain::setVScale(CImplicitModuleBase *v)
{
    m_sv.set(v);
}


void CImplicitScaleDomain::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

void CImplicitScaleDomain::setSource(float v)
{
    m_source.set(v);
}

float CImplicitScaleDomain::get(float x, float y)
{
    return m_source.get(x*m_sx.get(x,y), y*m_sy.get(x,y));
}

float CImplicitScaleDomain::get(float x, float y, float z)
{
    return m_source.get(x*m_sx.get(x,y,z), y*m_sy.get(x,y,z), z*m_sz.get(x,y,z));
}

float CImplicitScaleDomain::get(float x, float y, float z, float w)
{
    return m_source.get(x*m_sx.get(x,y,z,w), y*m_sy.get(x,y,z,w), z*m_sz.get(x,y,z,w), w*m_sw.get(x,y,z,w));
}

float CImplicitScaleDomain::get(float x, float y, float z, float w, float u, float v)
{
    return m_source.get(x*m_sx.get(x,y,z,w,u,v), y*m_sy.get(x,y,z,w,u,v), z*m_sz.get(x,y,z,w,u,v),
                        w*m_sw.get(x,y,z,w,u,v), u*m_su.get(x,y,z,w,u,v), v*m_sv.get(x,y,z,w,u,v));
}
};
