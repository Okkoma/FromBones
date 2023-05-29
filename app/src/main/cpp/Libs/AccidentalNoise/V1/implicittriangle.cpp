#include "implicittriangle.h"
#include <cmath>

namespace anl
{
float sawtooth(float x, float p)
{
    return (2.0*(x/p-floor(0.5+x/p)))*0.5+0.5;
}

CImplicitTriangle::CImplicitTriangle(float period, float offset) : m_source(0), m_period(period), m_offset(offset) {}
CImplicitTriangle::~CImplicitTriangle() {}

void CImplicitTriangle::setSource(float s)
{
    m_source.set(s);
}
void CImplicitTriangle::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}
void CImplicitTriangle::setPeriod(float p)
{
    m_period.set(p);
}
void CImplicitTriangle::setPeriod(CImplicitModuleBase *p)
{
    m_period.set(p);
}
void CImplicitTriangle::setOffset(float o)
{
    m_offset.set(o);
}
void CImplicitTriangle::setOffset(CImplicitModuleBase *o)
{
    m_offset.set(o);
}

float CImplicitTriangle::get(float x, float y)
{
    float val=m_source.get(x,y);
    float period=m_period.get(x,y);
    float offset=m_offset.get(x,y);

    if (offset>=1) return sawtooth(val, period);
    else if (offset<=0) return 1.0-sawtooth(val, period);
    else
    {
        float s1=(offset-sawtooth(val,period))>=0 ? 1.0 : 0.0;
        float s2=((1.0-offset)-(sawtooth(-val,period)))>=0 ? 1.0 : 0.0;
        return sawtooth(val,period) * s1/offset + sawtooth(-val,period)*s2/(1.0-offset);
    }
}

float CImplicitTriangle::get(float x, float y, float z)
{
    float val=m_source.get(x,y,z);
    float period=m_period.get(x,y,z);
    float offset=m_offset.get(x,y,z);

    if (offset>=1) return sawtooth(val, period);
    else if (offset<=0) return 1.0-sawtooth(val, period);
    else
    {
        float s1=(offset-sawtooth(val,period))>=0 ? 1.0 : 0.0;
        float s2=((1.0-offset)-(sawtooth(-val,period)))>=0 ? 1.0 : 0.0;
        return sawtooth(val,period) * s1/offset + sawtooth(-val,period)*s2/(1.0-offset);
        //return sawtooth(val,period)*s1/offset;
    }
}

float CImplicitTriangle::get(float x, float y, float z, float w)
{
    float val=m_source.get(x,y,z,w);
    float period=m_period.get(x,y,z,w);
    float offset=m_offset.get(x,y,z,w);

    if (offset>=1) return sawtooth(val, period);
    else if (offset<=0) return 1.0-sawtooth(val, period);
    else
    {
        float s1=(offset-sawtooth(val,period))>=0 ? 1.0 : 0.0;
        float s2=((1.0-offset)-(sawtooth(-val,period)))>=0 ? 1.0 : 0.0;
        return sawtooth(val,period) * s1/offset + sawtooth(-val,period)*s2/(1.0-offset);
    }
}

float CImplicitTriangle::get(float x, float y, float z, float w, float u, float v)
{
    float val=m_source.get(x,y,z,w,u,v);
    float period=m_period.get(x,y,z,w,u,v);
    float offset=m_offset.get(x,y,z,w,u,v);

    if (offset>=1) return sawtooth(val, period);
    else if (offset<=0) return 1.0-sawtooth(val, period);
    else
    {
        float s1=(offset-sawtooth(val,period))>=0 ? 1.0 : 0.0;
        float s2=((1.0-offset)-(sawtooth(-val,period)))>=0 ? 1.0 : 0.0;
        return sawtooth(val,period) * s1/offset + sawtooth(-val,period)*s2/(1.0-offset);
    }
}
};
