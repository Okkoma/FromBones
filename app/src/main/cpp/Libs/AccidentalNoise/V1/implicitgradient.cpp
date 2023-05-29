#include "implicitgradient.h"
#include "../VCommon/utility.h"
#include <math.h>

namespace anl
{
CImplicitGradient::CImplicitGradient()
{
    setGradient(0,1,0,1,0,0,0,0,0,0,0,0);
}
CImplicitGradient::~CImplicitGradient() {}

void CImplicitGradient::setGradient(float x1, float x2, float y1, float y2, float z1, float z2,
                                    float w1, float w2, float u1, float u2, float v1, float v2)
{
    m_gx1=x1;
    m_gx2=x2;
    m_gy1=y1;
    m_gy2=y2;
    m_gz1=z1;
    m_gz2=z2;
    m_gw1=w1;
    m_gw2=w2;
    m_gu1=u1;
    m_gu2=u2;
    m_gv1=v1;
    m_gv2=v2;

    m_x=x2-x1;
    m_y=y2-y1;
    m_z=z2-z1;
    m_w=w2-w1;
    m_u=u2-u1;
    m_v=v2-v1;

    m_vlen=(m_x*m_x+m_y*m_y+m_z*m_z+m_w*m_w+m_u*m_u+m_v*m_v);
}

float CImplicitGradient::get(float x, float y)
{
    // Subtract from (1) and take dotprod
    float dx=x-m_gx1;
    float dy=y-m_gy1;
    float dp=dx*m_x+dy*m_y;
    dp/=m_vlen;
    //dp=clamp(dp/m_vlen,0.0,1.0);
    //return lerp(dp,1.0,-1.0);
    return dp;
}

float CImplicitGradient::get(float x, float y, float z)
{
    float dx=x-m_gx1;
    float dy=y-m_gy1;
    float dz=z-m_gz1;
    float dp=dx*m_x+dy*m_y+dz*m_z;
    dp/=m_vlen;
    //dp=clamp(dp/m_vlen,0.0,1.0);
    //return lerp(dp,1.0,-1.0);
    return dp;
}

float CImplicitGradient::get(float x, float y, float z, float w)
{
    float dx=x-m_gx1;
    float dy=y-m_gy1;
    float dz=z-m_gz1;
    float dw=w-m_gw1;
    float dp=dx*m_x+dy*m_y+dz*m_z+dw*m_w;
    dp/=m_vlen;
    //dp=clamp(dp/m_vlen,0.0,1.0);
    //return lerp(dp,1.0,-1.0);
    return dp;
}

float CImplicitGradient::get(float x, float y, float z, float w, float u, float v)
{
    float dx=x-m_gx1;
    float dy=y-m_gy1;
    float dz=z-m_gz1;
    float dw=w-m_gw1;
    float du=u-m_gu1;
    float dv=v-m_gv1;
    float dp=dx*m_x+dy*m_y+dz*m_z+dw*m_w+du*m_u+dv*m_v;
    dp/=m_vlen;
    //dp=clamp(dp/m_vlen,0.0,1.0);
    //return lerp(clamp(dp,0.0,1.0),1.0,-1.0);
    return dp;
}
};
