#include "implicitcurve.h"

namespace anl
{
CImplicitCurve::CImplicitCurve() : m_source(0), m_type(LINEAR) {}
CImplicitCurve::~CImplicitCurve() {}

void CImplicitCurve::pushPoint(float t, float v)
{
    m_curve.pushPoint(t,v);
}

void CImplicitCurve::clearCurve()
{
    m_curve.clear();
}

void CImplicitCurve::setInterpType(int t)
{
    m_type=t;
    if(m_type<0) m_type=0;
    if(m_type>QUINTIC) m_type=QUINTIC;
}

void CImplicitCurve::setSource(float t)
{
    m_source.set(t);
}
void CImplicitCurve::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

float CImplicitCurve::get(float x, float y)
{
    float t=m_source.get(x,y);
    switch(m_type)
    {
    case NONE:
        return m_curve.noInterp(t);
        break;
    case LINEAR:
        return m_curve.linearInterp(t);
        break;
    case CUBIC:
        return m_curve.cubicInterp(t);
        break;
    case QUINTIC:
        return m_curve.quinticInterp(t);
        break;
    default:
        return m_curve.linearInterp(t);
        break;
    }
}

float CImplicitCurve::get(float x, float y, float z)
{
    float t=m_source.get(x,y,z);
    switch(m_type)
    {
    case NONE:
        return m_curve.noInterp(t);
        break;
    case LINEAR:
        return m_curve.linearInterp(t);
        break;
    case CUBIC:
        return m_curve.cubicInterp(t);
        break;
    case QUINTIC:
        return m_curve.quinticInterp(t);
        break;
    default:
        return m_curve.linearInterp(t);
        break;
    }
}

float CImplicitCurve::get(float x, float y, float z, float w)
{
    float t=m_source.get(x,y,z,w);
    switch(m_type)
    {
    case NONE:
        return m_curve.noInterp(t);
        break;
    case LINEAR:
        return m_curve.linearInterp(t);
        break;
    case CUBIC:
        return m_curve.cubicInterp(t);
        break;
    case QUINTIC:
        return m_curve.quinticInterp(t);
        break;
    default:
        return m_curve.linearInterp(t);
        break;
    }
}

float CImplicitCurve::get(float x, float y, float z, float w, float u, float v)
{
    float t=m_source.get(x,y,z,w,u,v);
    switch(m_type)
    {
    case NONE:
        return m_curve.noInterp(t);
        break;
    case LINEAR:
        return m_curve.linearInterp(t);
        break;
    case CUBIC:
        return m_curve.cubicInterp(t);
        break;
    case QUINTIC:
        return m_curve.quinticInterp(t);
        break;
    default:
        return m_curve.linearInterp(t);
        break;
    }
}


};
