#include "rgbacurve.h"

namespace anl
{
CRGBACurve::CRGBACurve() : m_source(0), m_type(LINEAR) {}
CRGBACurve::~CRGBACurve() {}

void CRGBACurve::pushPoint(float t,float r, float g, float b, float a)
{
    m_curve.pushPoint(t,ANLV1_SRGBA(r,g,b,a));
}

void CRGBACurve::clearCurve()
{
    m_curve.clear();
}

void CRGBACurve::setInterpType(int t)
{
    m_type=t;
    if(m_type<0) m_type=0;
    if(m_type>QUINTIC) m_type=QUINTIC;
}

void CRGBACurve::setSource(float t)
{
    m_source.set(t);
}
void CRGBACurve::setSource(CImplicitModuleBase *m)
{
    m_source.set(m);
}

ANLV1_SRGBA CRGBACurve::get(float x, float y)
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

ANLV1_SRGBA CRGBACurve::get(float x, float y, float z)
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

ANLV1_SRGBA CRGBACurve::get(float x, float y, float z, float w)
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

ANLV1_SRGBA CRGBACurve::get(float x, float y, float z, float w, float u, float v)
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

