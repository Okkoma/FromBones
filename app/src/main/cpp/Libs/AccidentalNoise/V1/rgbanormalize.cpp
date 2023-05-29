#include "rgbanormalize.h"

#include <math.h>

namespace anl
{
CRGBANormalize::CRGBANormalize() : m_source(0,0,0,0) {}
CRGBANormalize::~CRGBANormalize() {}

void CRGBANormalize::setSource(CRGBAModuleBase *m)
{
    m_source.set(m);
}
void CRGBANormalize::setSource(float r, float g, float b, float a)
{
    m_source.set(r,g,b,a);
}

ANLV1_SRGBA CRGBANormalize::get(float x, float y)
{
    ANLV1_SRGBA s=m_source.get(x,y);
    float len=s[0]*s[0] + s[1]*s[1] + s[2]*s[2];
    if(len==0.0)
    {
        return ANLV1_SRGBA(0,0,0,0);
    }
    len=sqrtf(len);
    return ANLV1_SRGBA(s[0]/len, s[1]/len, s[2]/len, s[3]);
}
ANLV1_SRGBA CRGBANormalize::get(float x, float y, float z)
{
    ANLV1_SRGBA s=m_source.get(x,y,z);
    float len=s[0]*s[0] + s[1]*s[1] + s[2]*s[2];
    if(len==0.0)
    {
        return ANLV1_SRGBA(0,0,0,0);
    }
    len=sqrtf(len);
    return ANLV1_SRGBA(s[0]/len, s[1]/len, s[2]/len, s[3]);
}
ANLV1_SRGBA CRGBANormalize::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w);
    float len=s[0]*s[0] + s[1]*s[1] + s[2]*s[2];
    if(len==0.0)
    {
        return ANLV1_SRGBA(0,0,0,0);
    }
    len=sqrtf(len);
    return ANLV1_SRGBA(s[0]/len, s[1]/len, s[2]/len, s[3]);
}
ANLV1_SRGBA CRGBANormalize::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w,u,v);
    float len=s[0]*s[0] + s[1]*s[1] + s[2]*s[2];
    if(len==0.0)
    {
        return ANLV1_SRGBA(0,0,0,0);
    }
    len=sqrtf(len);
    return ANLV1_SRGBA(s[0]/len, s[1]/len, s[2]/len, s[3]);
}
};
