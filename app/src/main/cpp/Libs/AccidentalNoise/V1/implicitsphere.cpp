#include "implicitsphere.h"

namespace anl
{
CImplicitSphere::CImplicitSphere() : CImplicitModuleBase(),m_source(0), m_cx(0), m_cy(0), m_cz(0), m_cw(0), m_cu(0), m_cv(0), m_radius(1) {}
CImplicitSphere::~CImplicitSphere() {}

void CImplicitSphere::setCenter(float cx,float cy,float cz,float cw,float cu,float cv)
{
    m_cx=cx;
    m_cy=cy;
    m_cz=cz;
    m_cw=cw;
    m_cu=cu;
    m_cv=cv;
}
void CImplicitSphere::setCenterX(float cx)
{
    m_cx.set(cx);
}
void CImplicitSphere::setCenterY(float cy)
{
    m_cy.set(cy);
}
void CImplicitSphere::setCenterZ(float cz)
{
    m_cz.set(cz);
}
void CImplicitSphere::setCenterW(float cw)
{
    m_cw.set(cw);
}
void CImplicitSphere::setCenterU(float cu)
{
    m_cu.set(cu);
}
void CImplicitSphere::setCenterV(float cv)
{
    m_cv.set(cv);
}
void CImplicitSphere::setCenterX(CImplicitModuleBase *cx)
{
    m_cx.set(cx);
}
void CImplicitSphere::setCenterY(CImplicitModuleBase *cy)
{
    m_cy.set(cy);
}
void CImplicitSphere::setCenterZ(CImplicitModuleBase *cz)
{
    m_cz.set(cz);
}
void CImplicitSphere::setCenterW(CImplicitModuleBase *cw)
{
    m_cw.set(cw);
}
void CImplicitSphere::setCenterU(CImplicitModuleBase *cu)
{
    m_cu.set(cu);
}
void CImplicitSphere::setCenterV(CImplicitModuleBase *cv)
{
    m_cv.set(cv);
}
void CImplicitSphere::setRadius(float r)
{
    m_radius.set(r);
}
void CImplicitSphere::setRadius(CImplicitModuleBase *r)
{
    m_radius.set(r);
}

float CImplicitSphere::get(float x, float y)
{
    float dx=x-m_cx.get(x,y), dy=y-m_cy.get(x,y);
    float len=sqrt(dx*dx+dy*dy);
    float radius=m_radius.get(x,y);
    float i=(radius-len)/radius;
    if(i<0) i=0;
    if(i>1) i=1;

    return i;
}

float CImplicitSphere::get(float x, float y, float z)
{
    float dx=x-m_cx.get(x,y,z), dy=y-m_cy.get(x,y,z), dz=z-m_cz.get(x,y,z);
    float len=sqrt(dx*dx+dy*dy+dz*dz);
    float radius=m_radius.get(x,y,z);
    float i=(radius-len)/radius;
    if(i<0) i=0;
    if(i>1) i=1;

    return i;
}

float CImplicitSphere::get(float x, float y, float z, float w)
{
    float dx=x-m_cx.get(x,y,z,w), dy=y-m_cy.get(x,y,z,w), dz=z-m_cz.get(x,y,z,w), dw=w-m_cw.get(x,y,z,w);
    float len=sqrt(dx*dx+dy*dy+dz*dz+dw*dw);
    float radius=m_radius.get(x,y,z,w);
    float i=(radius-len)/radius;
    if(i<0) i=0;
    if(i>1) i=1;

    return i;
}

float CImplicitSphere::get(float x, float y, float z, float w, float u, float v)
{
    float dx=x-m_cx.get(x,y,z,w,u,v), dy=y-m_cy.get(x,y,z,w,u,v), dz=z-m_cz.get(x,y,z,w,u,v), dw=w-m_cw.get(x,y,z,w,u,v),
          du=u-m_cu.get(x,y,z,w,u,v), dv=v-m_cv.get(x,y,z,w,u,v);
    float len=sqrt(dx*dx+dy*dy+dz*dz+dw*dw+du*du+dv*dv);
    float radius=m_radius.get(x,y,z,w,u,v);
    float i=(radius-len)/radius;
    if(i<0) i=0;
    if(i>1) i=1;

    return i;
}
};
