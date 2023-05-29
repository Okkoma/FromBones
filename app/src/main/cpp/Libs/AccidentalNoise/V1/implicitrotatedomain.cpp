#include "implicitrotatedomain.h"
#include <cmath>

namespace anl
{
CImplicitRotateDomain::CImplicitRotateDomain(float ax, float ay, float az, float angle_deg) : CImplicitModuleBase(),m_ax(ax), m_ay(ay), m_az(az), m_angledeg(angle_deg), m_source(0)
{
    //setAxisAngle(ax,ay,az,angle_deg);

}
CImplicitRotateDomain::~CImplicitRotateDomain() {}

void CImplicitRotateDomain::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}
void CImplicitRotateDomain::setSource(float v)
{
    m_source.set(v);
}

void CImplicitRotateDomain::calculateRotMatrix(float x, float y)
{
    float angle=m_angledeg.get(x,y)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y);
    float ay=m_ay.get(x,y);
    float az=m_az.get(x,y);

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

void CImplicitRotateDomain::calculateRotMatrix(float x, float y, float z)
{
    float angle=m_angledeg.get(x,y,z)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y,z);
    float ay=m_ay.get(x,y,z);
    float az=m_az.get(x,y,z);

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
void CImplicitRotateDomain::calculateRotMatrix(float x, float y, float z, float w)
{
    float angle=m_angledeg.get(x,y,z,w)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y,z,w);
    float ay=m_ay.get(x,y,z,w);
    float az=m_az.get(x,y,z,w);

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
void CImplicitRotateDomain::calculateRotMatrix(float x, float y, float z, float w, float u, float v)
{
    float angle=m_angledeg.get(x,y,z,w,u,v)*360.0 * 3.14159265/180.0;
    float ax=m_ax.get(x,y,z,w,u,v);
    float ay=m_ay.get(x,y,z,w,u,v);
    float az=m_az.get(x,y,z,w,u,v);

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
void CImplicitRotateDomain::setAxis(float ax, float ay, float az)
{
    m_ax.set(ax);
    m_ay.set(ay);
    m_az.set(az);
}

void CImplicitRotateDomain::setAxis(CImplicitModuleBase *ax, CImplicitModuleBase *ay, CImplicitModuleBase *az)
{
    m_ax.set(ax);
    m_ay.set(ay);
    m_az.set(az);
}
void CImplicitRotateDomain::setAxisX(float ax)
{
    m_ax.set(ax);
}
void CImplicitRotateDomain::setAxisY(float ay)
{
    m_ay.set(ay);
}
void CImplicitRotateDomain::setAxisZ(float az)
{
    m_az.set(az);
}
void CImplicitRotateDomain::setAxisX(CImplicitModuleBase *ax)
{
    m_ax.set(ax);
}
void CImplicitRotateDomain::setAxisY(CImplicitModuleBase *ay)
{
    m_ay.set(ay);
}
void CImplicitRotateDomain::setAxisZ(CImplicitModuleBase *az)
{
    m_az.set(az);
}

void CImplicitRotateDomain::setAngle(float a)
{
    m_angledeg.set(a);
}
void CImplicitRotateDomain::setAngle(CImplicitModuleBase *a)
{
    m_angledeg.set(a);
}

float CImplicitRotateDomain::get(float x, float y)
{
    float nx,ny;
    float angle=m_angledeg.get(x,y)*360.0 * 3.14159265/180.0;
    float cos2d=cos(angle);
    float sin2d=sin(angle);
    nx = x*cos2d-y*sin2d;
    ny = y*cos2d+x*sin2d;
    return m_source.get(nx,ny);
}

float CImplicitRotateDomain::get(float x, float y, float z)
{
    calculateRotMatrix(x,y,z);
    float nx, ny, nz;
    nx = (m_rotmatrix[0][0]*x) + (m_rotmatrix[1][0]*y) + (m_rotmatrix[2][0]*z);
    ny = (m_rotmatrix[0][1]*x) + (m_rotmatrix[1][1]*y) + (m_rotmatrix[2][1]*z);
    nz = (m_rotmatrix[0][2]*x) + (m_rotmatrix[1][2]*y) + (m_rotmatrix[2][2]*z);
    return m_source.get(nx,ny,nz);
}
float CImplicitRotateDomain::get(float x, float y, float z, float w)
{

    calculateRotMatrix(x,y,z,w);
    float nx, ny, nz;
    nx = (m_rotmatrix[0][0]*x) + (m_rotmatrix[1][0]*y) + (m_rotmatrix[2][0]*z);
    ny = (m_rotmatrix[0][1]*x) + (m_rotmatrix[1][1]*y) + (m_rotmatrix[2][1]*z);
    nz = (m_rotmatrix[0][2]*x) + (m_rotmatrix[1][2]*y) + (m_rotmatrix[2][2]*z);
    return m_source.get(nx,ny,nz,w);
}
float CImplicitRotateDomain::get(float x, float y, float z, float w, float u, float v)
{

    calculateRotMatrix(x,y,z,w,u,v);
    float nx, ny, nz;
    nx = (m_rotmatrix[0][0]*x) + (m_rotmatrix[1][0]*y) + (m_rotmatrix[2][0]*z);
    ny = (m_rotmatrix[0][1]*x) + (m_rotmatrix[1][1]*y) + (m_rotmatrix[2][1]*z);
    nz = (m_rotmatrix[0][2]*x) + (m_rotmatrix[1][2]*y) + (m_rotmatrix[2][2]*z);
    return m_source.get(nx,ny,nz,w,u,v);
}
};
