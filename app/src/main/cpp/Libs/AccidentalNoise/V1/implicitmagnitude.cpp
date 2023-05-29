#include "implicitmagnitude.h"

namespace anl
{
CImplicitMagnitude::CImplicitMagnitude() : m_x(0), m_y(0), m_z(0), m_w(0), m_u(0), m_v(0) {}
CImplicitMagnitude::~CImplicitMagnitude() {}

void CImplicitMagnitude::setX(float f)
{
    m_x.set(f);
}
void CImplicitMagnitude::setY(float f)
{
    m_y.set(f);
}
void CImplicitMagnitude::setZ(float f)
{
    m_z.set(f);
}
void CImplicitMagnitude::setW(float f)
{
    m_w.set(f);
}
void CImplicitMagnitude::setU(float f)
{
    m_u.set(f);
}
void CImplicitMagnitude::setV(float f)
{
    m_v.set(f);
}

void CImplicitMagnitude::setX(CImplicitModuleBase *f)
{
    m_x.set(f);
}
void CImplicitMagnitude::setY(CImplicitModuleBase *f)
{
    m_y.set(f);
}
void CImplicitMagnitude::setZ(CImplicitModuleBase *f)
{
    m_z.set(f);
}
void CImplicitMagnitude::setW(CImplicitModuleBase *f)
{
    m_w.set(f);
}
void CImplicitMagnitude::setU(CImplicitModuleBase *f)
{
    m_u.set(f);
}
void CImplicitMagnitude::setV(CImplicitModuleBase *f)
{
    m_v.set(f);
}

float CImplicitMagnitude::get(float x, float y)
{
    float xx=m_x.get(x,y);
    float yy=m_y.get(x,y);
    return sqrt(xx*xx+yy*yy);
}

float CImplicitMagnitude::get(float x, float y, float z)
{
    float xx=m_x.get(x,y,z);
    float yy=m_y.get(x,y,z);
    float zz=m_z.get(x,y,z);
    return sqrt(xx*xx+yy*yy+zz*zz);
}

float CImplicitMagnitude::get(float x, float y, float z, float w)
{
    float xx=m_x.get(x,y,z,w);
    float yy=m_y.get(x,y,z,w);
    float zz=m_z.get(x,y,z,w);
    float ww=m_w.get(x,y,z,w);
    return sqrt(xx*xx+yy*yy+zz*zz+ww*ww);
}

float CImplicitMagnitude::get(float x, float y, float z, float w, float u, float v)
{
    float xx=m_x.get(x,y,z,w,u,v);
    float yy=m_y.get(x,y,z,w,u,v);
    float zz=m_z.get(x,y,z,w,u,v);
    float ww=m_w.get(x,y,z,w,u,v);
    float uu=m_u.get(x,y,z,w,u,v);
    float vv=m_v.get(x,y,z,w,u,v);
    return sqrt(xx*xx+yy*yy+zz*zz+ww*ww+uu*uu+vv*vv);
}

};
