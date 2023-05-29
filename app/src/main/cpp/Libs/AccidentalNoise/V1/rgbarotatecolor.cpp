#include "rgbarotatecolor.h"
#include <math.h>
#include "../VCommon/utility.h"

namespace anl
{
CRGBARotateColor::CRGBARotateColor() : m_ax(0), m_ay(0), m_az(1), m_angledeg(0), m_source(0,0,0,0), m_normalizeaxis(false) {}

void CRGBARotateColor::setAxis(float ax, float ay, float az)
{
    m_ax.set(ax);
    m_ay.set(ay);
    m_az.set(az);
}
void CRGBARotateColor::setAxis(CImplicitModuleBase *ax, CImplicitModuleBase *ay, CImplicitModuleBase *az)
{
    m_ax.set(ax);
    m_ay.set(ay);
    m_az.set(az);
}
void CRGBARotateColor::setAxisX(float ax)
{
    m_ax.set(ax);
}
void CRGBARotateColor::setAxisY(float ay)
{
    m_ay.set(ay);
}
void CRGBARotateColor::setAxisZ(float az)
{
    m_az.set(az);
}
void CRGBARotateColor::setAxisX(CImplicitModuleBase *ax)
{
    m_ax.set(ax);
}
void CRGBARotateColor::setAxisY(CImplicitModuleBase *ay)
{
    m_ay.set(ay);
}
void CRGBARotateColor::setAxisZ(CImplicitModuleBase *az)
{
    m_az.set(az);
}
void CRGBARotateColor::setAngle(float a)
{
    m_angledeg.set(a);
}
void CRGBARotateColor::setAngle(CImplicitModuleBase *a)
{
    m_angledeg.set(a);
}
void CRGBARotateColor::setSource(CRGBAModuleBase *m)
{
    m_source.set(m);
}
void CRGBARotateColor::setSource(float r, float g, float b, float a)
{
    m_source.set(r,g,b,a);
}
ANLV1_SRGBA CRGBARotateColor::get(float x, float y)
{
    ANLV1_SRGBA s=m_source.get(x,y);
    calculateRotMatrix(x,y);

    s[0]=s[0]*2.0f-1.0f;
    s[1]=s[1]*2.0f-1.0f;
    s[2]=s[2]*2.0f-1.0f;


    s=ANLV1_SRGBA(
          (float)((m_rotmatrix[0][0]*s[0]) + (m_rotmatrix[1][0]*s[1]) + (m_rotmatrix[2][0]*s[2])),
          (float)((m_rotmatrix[0][1]*s[0]) + (m_rotmatrix[1][1]*s[1]) + (m_rotmatrix[2][1]*s[2])),
          (float)((m_rotmatrix[0][2]*s[0]) + (m_rotmatrix[1][2]*s[1]) + (m_rotmatrix[2][2]*s[2])),
          s[3]);

    s[0]=(float)clamp(s[0]*0.5+0.5,0.0,1.0);
    s[1]=(float)clamp(s[1]*0.5+0.5,0.0,1.0);
    s[2]=(float)clamp(s[2]*0.5+0.5,0.0,1.0);

    return s;

}
ANLV1_SRGBA CRGBARotateColor::get(float x, float y, float z)
{
    ANLV1_SRGBA s=m_source.get(x,y,z);
    calculateRotMatrix(x,y,z);

    s[0]=s[0]*2.0f-1.0f;
    s[1]=s[1]*2.0f-1.0f;
    s[2]=s[2]*2.0f-1.0f;


    s=ANLV1_SRGBA(
          (float)((m_rotmatrix[0][0]*s[0]) + (m_rotmatrix[1][0]*s[1]) + (m_rotmatrix[2][0]*s[2])),
          (float)((m_rotmatrix[0][1]*s[0]) + (m_rotmatrix[1][1]*s[1]) + (m_rotmatrix[2][1]*s[2])),
          (float)((m_rotmatrix[0][2]*s[0]) + (m_rotmatrix[1][2]*s[1]) + (m_rotmatrix[2][2]*s[2])),
          s[3]);

    s[0]=(float)clamp(s[0]*0.5+0.5,0.0,1.0);
    s[1]=(float)clamp(s[1]*0.5+0.5,0.0,1.0);
    s[2]=(float)clamp(s[2]*0.5+0.5,0.0,1.0);

    return s;
}
ANLV1_SRGBA CRGBARotateColor::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w);
    calculateRotMatrix(x,y,z,w);

    s[0]=s[0]*2.0f-1.0f;
    s[1]=s[1]*2.0f-1.0f;
    s[2]=s[2]*2.0f-1.0f;


    s=ANLV1_SRGBA(
          (float)((m_rotmatrix[0][0]*s[0]) + (m_rotmatrix[1][0]*s[1]) + (m_rotmatrix[2][0]*s[2])),
          (float)((m_rotmatrix[0][1]*s[0]) + (m_rotmatrix[1][1]*s[1]) + (m_rotmatrix[2][1]*s[2])),
          (float)((m_rotmatrix[0][2]*s[0]) + (m_rotmatrix[1][2]*s[1]) + (m_rotmatrix[2][2]*s[2])),
          s[3]);

    s[0]=(float)clamp(s[0]*0.5+0.5,0.0,1.0);
    s[1]=(float)clamp(s[1]*0.5+0.5,0.0,1.0);
    s[2]=(float)clamp(s[2]*0.5+0.5,0.0,1.0);

    return s;
}
ANLV1_SRGBA CRGBARotateColor::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w,u,v);
    calculateRotMatrix(x,y,z,w,u,v);

    s[0]=s[0]*2.0f-1.0f;
    s[1]=s[1]*2.0f-1.0f;
    s[2]=s[2]*2.0f-1.0f;


    s=ANLV1_SRGBA(
          (float)((m_rotmatrix[0][0]*s[0]) + (m_rotmatrix[1][0]*s[1]) + (m_rotmatrix[2][0]*s[2])),
          (float)((m_rotmatrix[0][1]*s[0]) + (m_rotmatrix[1][1]*s[1]) + (m_rotmatrix[2][1]*s[2])),
          (float)((m_rotmatrix[0][2]*s[0]) + (m_rotmatrix[1][2]*s[1]) + (m_rotmatrix[2][2]*s[2])),
          s[3]);

    s[0]=(float)clamp(s[0]*0.5+0.5,0.0,1.0);
    s[1]=(float)clamp(s[1]*0.5+0.5,0.0,1.0);
    s[2]=(float)clamp(s[2]*0.5+0.5,0.0,1.0);

    return s;
}

void CRGBARotateColor::calculateRotMatrix(float x, float y)
{
    float angle=m_angledeg.get(x,y)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y);
    float ay=m_ay.get(x,y);
    float az=m_az.get(x,y);

    if(m_normalizeaxis)
    {
        float len=sqrt(ax*ax+ay*ay+az*az);
        ax/=len;
        ay/=len;
        az/=len;
    }

    float cosangle=cos(angle);
    float sinangle=sin(angle);

    m_rotmatrix[0][0] = 1.0 + (1.0-cosangle)*(ax*ax-1.0);
    m_rotmatrix[1][0] = -az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[2][0] = ay*sinangle+(1.0-cosangle)*ax*az;

    m_rotmatrix[0][1] = az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[1][1] = 1.0 + (1.0-cosangle)*(ay*ay-1.0);
    m_rotmatrix[2][1] = -ax*sinangle+(1.0-cosangle)*ay*az;

    m_rotmatrix[0][2] = -ay*sinangle+(1.0-cosangle)*ax*az;
    m_rotmatrix[1][2] = ax*sinangle+(1.0-cosangle)*ay*az;
    m_rotmatrix[2][2] = 1.0 + (1.0-cosangle)*(az*az-1.0);
}

void CRGBARotateColor::calculateRotMatrix(float x, float y, float z)
{
    float angle=m_angledeg.get(x,y,z)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y,z);
    float ay=m_ay.get(x,y,z);
    float az=m_az.get(x,y,z);
    if(m_normalizeaxis)
    {
        float len=sqrt(ax*ax+ay*ay+az*az);
        ax/=len;
        ay/=len;
        az/=len;
    }

    float cosangle=cos(angle);
    float sinangle=sin(angle);

    m_rotmatrix[0][0] = 1.0 + (1.0-cosangle)*(ax*ax-1.0);
    m_rotmatrix[1][0] = -az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[2][0] = ay*sinangle+(1.0-cosangle)*ax*az;

    m_rotmatrix[0][1] = az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[1][1] = 1.0 + (1.0-cosangle)*(ay*ay-1.0);
    m_rotmatrix[2][1] = -ax*sinangle+(1.0-cosangle)*ay*az;

    m_rotmatrix[0][2] = -ay*sinangle+(1.0-cosangle)*ax*az;
    m_rotmatrix[1][2] = ax*sinangle+(1.0-cosangle)*ay*az;
    m_rotmatrix[2][2] = 1.0 + (1.0-cosangle)*(az*az-1.0);
}

void CRGBARotateColor::calculateRotMatrix(float x, float y, float z, float w)
{
    float angle=m_angledeg.get(x,y,z,w)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y,z,w);
    float ay=m_ay.get(x,y,z,w);
    float az=m_az.get(x,y,z,w);
    if(m_normalizeaxis)
    {
        float len=sqrt(ax*ax+ay*ay+az*az);
        ax/=len;
        ay/=len;
        az/=len;
    }

    float cosangle=cos(angle);
    float sinangle=sin(angle);

    m_rotmatrix[0][0] = 1.0 + (1.0-cosangle)*(ax*ax-1.0);
    m_rotmatrix[1][0] = -az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[2][0] = ay*sinangle+(1.0-cosangle)*ax*az;

    m_rotmatrix[0][1] = az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[1][1] = 1.0 + (1.0-cosangle)*(ay*ay-1.0);
    m_rotmatrix[2][1] = -ax*sinangle+(1.0-cosangle)*ay*az;

    m_rotmatrix[0][2] = -ay*sinangle+(1.0-cosangle)*ax*az;
    m_rotmatrix[1][2] = ax*sinangle+(1.0-cosangle)*ay*az;
    m_rotmatrix[2][2] = 1.0 + (1.0-cosangle)*(az*az-1.0);
}
void CRGBARotateColor::calculateRotMatrix(float x, float y, float z, float w, float u, float v)
{
    float angle=m_angledeg.get(x,y,z,w,u,v)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y,z,w,u,v);
    float ay=m_ay.get(x,y,z,w,u,v);
    float az=m_az.get(x,y,z,w,u,v);
    if(m_normalizeaxis)
    {
        float len=sqrt(ax*ax+ay*ay+az*az);
        ax/=len;
        ay/=len;
        az/=len;
    }

    float cosangle=cos(angle);
    float sinangle=sin(angle);

    m_rotmatrix[0][0] = 1.0 + (1.0-cosangle)*(ax*ax-1.0);
    m_rotmatrix[1][0] = -az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[2][0] = ay*sinangle+(1.0-cosangle)*ax*az;

    m_rotmatrix[0][1] = az*sinangle+(1.0-cosangle)*ax*ay;
    m_rotmatrix[1][1] = 1.0 + (1.0-cosangle)*(ay*ay-1.0);
    m_rotmatrix[2][1] = -ax*sinangle+(1.0-cosangle)*ay*az;

    m_rotmatrix[0][2] = -ay*sinangle+(1.0-cosangle)*ax*az;
    m_rotmatrix[1][2] = ax*sinangle+(1.0-cosangle)*ay*az;
    m_rotmatrix[2][2] = 1.0 + (1.0-cosangle)*(az*az-1.0);
}
};
