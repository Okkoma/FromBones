#include "rgbaselect.h"
#include "../VCommon/utility.h"

namespace anl
{
CRGBASelect::CRGBASelect() : m_low(0,0,0,0), m_high(0,0,0,0), m_control(0), m_threshold(0), m_falloff(0) {}
CRGBASelect::CRGBASelect(float thresh, float fall) : m_low(0,0,0,0), m_high(0,0,0,0), m_control(0), m_threshold(thresh), m_falloff(fall) {}

void CRGBASelect::setLowSource(CRGBAModuleBase *m)
{
    m_low.set(m);
}
void CRGBASelect::setHighSource(CRGBAModuleBase *m)
{
    m_high.set(m);
}
void CRGBASelect::setLowSource(float r, float g, float b, float a)
{
    m_low.set(r,g,b,a);
}
void CRGBASelect::setHighSource(float r, float g, float b, float a)
{
    m_high.set(r,g,b,a);
}
void CRGBASelect::setControlSource(CImplicitModuleBase *m)
{
    m_control.set(m);
}
void CRGBASelect::setThreshold(CImplicitModuleBase *m)
{
    m_threshold.set(m);
}
void CRGBASelect::setFalloff(CImplicitModuleBase *m)
{
    m_falloff.set(m);
}
void CRGBASelect::setControlSource(float v)
{
    m_control.set(v);
}
void CRGBASelect::setThreshold(float v)
{
    m_threshold.set(v);
}
void CRGBASelect::setFalloff(float v)
{
    m_falloff.set(v);
}
ANLV1_SRGBA CRGBASelect::get(float x, float y)
{
    ANLV1_SRGBA s1=m_low.get(x,y);
    ANLV1_SRGBA s2=m_high.get(x,y);
    float control=m_control.get(x,y);
    float threshold=m_threshold.get(x,y);
    float falloff=m_falloff.get(x,y);

    if(falloff>0.0)
    {
        if(control<threshold-falloff)
        {
            return s1;
        }
        else if(control>threshold+falloff)
        {
            return s2;
        }
        else
        {
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float t=quintic_blend((control-lower)/(upper-lower));
            return ANLV1_SRGBA(
                       (float)(s1[0]+t*(s2[0]-s1[0])),
                       (float)(s1[1]+t*(s2[1]-s1[1])),
                       (float)(s1[2]+t*(s2[2]-s1[2])),
                       (float)(s1[3]+t*(s2[3]-s1[3])));
        }
    }
    else
    {
        if(control<threshold) return s1;
        else return s2;
    }
}
ANLV1_SRGBA CRGBASelect::get(float x, float y, float z)
{
    ANLV1_SRGBA s1=m_low.get(x,y,z);
    ANLV1_SRGBA s2=m_high.get(x,y,z);
    float control=m_control.get(x,y,z);
    float threshold=m_threshold.get(x,y,z);
    float falloff=m_falloff.get(x,y,z);

    if(falloff>0.0)
    {
        if(control<threshold-falloff)
        {
            return s1;
        }
        else if(control>threshold+falloff)
        {
            return s2;
        }
        else
        {
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float t=quintic_blend((control-lower)/(upper-lower));
            return ANLV1_SRGBA(
                       (float)(s1[0]+t*(s2[0]-s1[0])),
                       (float)(s1[1]+t*(s2[1]-s1[1])),
                       (float)(s1[2]+t*(s2[2]-s1[2])),
                       (float)(s1[3]+t*(s2[3]-s1[3])));
        }
    }
    else
    {
        if(control<threshold) return s1;
        else return s2;
    }
}

ANLV1_SRGBA CRGBASelect::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA s1=m_low.get(x,y,z,w);
    ANLV1_SRGBA s2=m_high.get(x,y,z,w);
    float control=m_control.get(x,y,z,w);
    float threshold=m_threshold.get(x,y,z,w);
    float falloff=m_falloff.get(x,y,z,w);

    if(falloff>0.0)
    {
        if(control<threshold-falloff)
        {
            return s1;
        }
        else if(control>threshold+falloff)
        {
            return s2;
        }
        else
        {
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float t=quintic_blend((control-lower)/(upper-lower));
            return ANLV1_SRGBA(
                       (float)(s1[0]+t*(s2[0]-s1[0])),
                       (float)(s1[1]+t*(s2[1]-s1[1])),
                       (float)(s1[2]+t*(s2[2]-s1[2])),
                       (float)(s1[3]+t*(s2[3]-s1[3])));
        }
    }
    else
    {
        if(control<threshold) return s1;
        else return s2;
    }
}

ANLV1_SRGBA CRGBASelect::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA s1=m_low.get(x,y,z,w,u,v);
    ANLV1_SRGBA s2=m_high.get(x,y,z,w,u,v);
    float control=m_control.get(x,y,z,w,u,v);
    float threshold=m_threshold.get(x,y,z,w,u,v);
    float falloff=m_falloff.get(x,y,z,w,u,v);

    if(falloff>0.0)
    {
        if(control<threshold-falloff)
        {
            return s1;
        }
        else if(control>threshold+falloff)
        {
            return s2;
        }
        else
        {
            float lower=threshold-falloff;
            float upper=threshold+falloff;
            float t=quintic_blend((control-lower)/(upper-lower));
            return ANLV1_SRGBA(
                       (float)(s1[0]+t*(s2[0]-s1[0])),
                       (float)(s1[1]+t*(s2[1]-s1[1])),
                       (float)(s1[2]+t*(s2[2]-s1[2])),
                       (float)(s1[3]+t*(s2[3]-s1[3])));
        }
    }
    else
    {
        if(control<threshold) return s1;
        else return s2;
    }
}
};
