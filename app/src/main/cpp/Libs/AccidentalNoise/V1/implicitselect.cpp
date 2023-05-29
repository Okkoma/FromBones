#include "implicitselect.h"
#include "../VCommon/utility.h"

namespace anl
{
CImplicitSelect::CImplicitSelect() : CImplicitModuleBase(), m_low(0), m_high(0), m_control(0),
    m_threshold(0.0), m_falloff(0.0)
{
}
CImplicitSelect::~CImplicitSelect() {}

void CImplicitSelect::setLowSource(CImplicitModuleBase *b)
{
    m_low.set(b);
}
void CImplicitSelect::setHighSource(CImplicitModuleBase *b)
{
    m_high.set(b);
}
void CImplicitSelect::setControlSource(CImplicitModuleBase *b)
{
    m_control.set(b);
}

void CImplicitSelect::setLowSource(float b)
{
    m_low.set(b);
}
void CImplicitSelect::setHighSource(float b)
{
    m_high.set(b);
}
void CImplicitSelect::setControlSource(float b)
{
    m_control.set(b);
}

void CImplicitSelect::setThreshold(float t)
{
    //m_threshold=t;
    m_threshold.set(t);
}
void CImplicitSelect::setFalloff(float f)
{
    //m_falloff=f;
    m_falloff.set(f);
}
void CImplicitSelect::setThreshold(CImplicitModuleBase *m)
{
    m_threshold.set(m);
}
void CImplicitSelect::setFalloff(CImplicitModuleBase *m)
{
    m_falloff.set(m);
}

float CImplicitSelect::get(float x, float y)
{
    float control=m_control.get(x,y);
    float falloff=m_falloff.get(x,y);
    float threshold=m_threshold.get(x,y);

    if(falloff>0.0)
    {
        if(control < (threshold-falloff))
        {
            // Lies outside of falloff area below threshold, return first source
            return m_low.get(x,y);
        }
        else if(control > (threshold+falloff))
        {
            // Lise outside of falloff area above threshold, return second source
            return m_high.get(x,y);
        }
        else
        {
            // Lies within falloff area.
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float blend=quintic_blend((control-lower)/(upper-lower));
            return lerp(blend,m_low.get(x,y),m_high.get(x,y));
        }
    }
    else
    {
        if(control<threshold) return m_low.get(x,y);
        else return m_high.get(x,y);
    }
}

float CImplicitSelect::get(float x, float y, float z)
{
    float control=m_control.get(x,y,z);
    float falloff=m_falloff.get(x,y,z);
    float threshold=m_threshold.get(x,y,z);

    if(falloff>0.0)
    {
        if(control < (threshold-falloff))
        {
            // Lies outside of falloff area below threshold, return first source
            return m_low.get(x,y,z);
        }
        else if(control > (threshold+falloff))
        {
            // Lise outside of falloff area above threshold, return second source
            return m_high.get(x,y,z);
        }
        else
        {
            // Lies within falloff area.
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float blend=quintic_blend((control-lower)/(upper-lower));
            return lerp(blend,m_low.get(x,y,z),m_high.get(x,y,z));
        }
    }
    else
    {
        if(control<threshold) return m_low.get(x,y,z);
        else return m_high.get(x,y,z);
    }
}

float CImplicitSelect::get(float x, float y, float z, float w)
{
    float control=m_control.get(x,y,z,w);
    float falloff=m_falloff.get(x,y,z,w);
    float threshold=m_threshold.get(x,y,z,w);

    if(falloff>0.0)
    {
        if(control < (threshold-falloff))
        {
            // Lies outside of falloff area below threshold, return first source
            return m_low.get(x,y,z,w);
        }
        else if(control > (threshold+falloff))
        {
            // Lise outside of falloff area above threshold, return second source
            return m_high.get(x,y,z,w);
        }
        else
        {
            // Lies within falloff area.
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float blend=quintic_blend((control-lower)/(upper-lower));
            return lerp(blend,m_low.get(x,y,z,w),m_high.get(x,y,z,w));
        }
    }
    else
    {
        if(control<threshold) return m_low.get(x,y,z,w);
        else return m_high.get(x,y,z,w);
    }
}

float CImplicitSelect::get(float x, float y, float z, float w, float u, float v)
{
    float control=m_control.get(x,y,z,w,u,v);
    float falloff=m_falloff.get(x,y,z,w,u,v);
    float threshold=m_threshold.get(x,y,z,w,u,v);

    if(falloff>0.0)
    {
        if(control < (threshold-falloff))
        {
            // Lies outside of falloff area below threshold, return first source
            return m_low.get(x,y,z,w,u,v);
        }
        else if(control > (threshold+falloff))
        {
            // Lise outside of falloff area above threshold, return second source
            return m_high.get(x,y,z,w,u,v);
        }
        else
        {
            // Lies within falloff area.
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float blend=quintic_blend((control-lower)/(upper-lower));
            return lerp(blend,m_low.get(x,y,z,w,u,v),m_high.get(x,y,z,w,u,v));
        }
    }
    else
    {
        if(control<threshold) return m_low.get(x,y,z,w,u,v);
        else return m_high.get(x,y,z,w,u,v);
    }
}
};
