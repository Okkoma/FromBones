#include "implicitcellular.h"

namespace anl
{
CImplicitCellular::CImplicitCellular() : m_generator(0)
{
    setCoefficients(1,0,0,0);
}
CImplicitCellular::CImplicitCellular(float a, float b, float c, float d) : m_generator(0)
{
    setCoefficients(a,b,c,d);
}

void CImplicitCellular::setCoefficients(float a, float b, float c, float d)
{
    m_coefficients[0]=a;
    m_coefficients[1]=b;
    m_coefficients[2]=c;
    m_coefficients[3]=d;
}

void CImplicitCellular::setCellularSource(CCellularGenerator *m)
{
    m_generator=m;
}

float CImplicitCellular::get(float x, float y)
{
    if(!m_generator) return 0.0;

    SCellularCache &c=m_generator->get(x,y);
    return c.f[0]*m_coefficients[0] + c.f[1]*m_coefficients[1] + c.f[2]*m_coefficients[2] + c.f[3]*m_coefficients[3];
}

float CImplicitCellular::get(float x, float y, float z)
{
    if(!m_generator) return 0.0;

    SCellularCache &c=m_generator->get(x,y,z);
    return c.f[0]*m_coefficients[0] + c.f[1]*m_coefficients[1] + c.f[2]*m_coefficients[2] + c.f[3]*m_coefficients[3];
}
float CImplicitCellular::get(float x, float y, float z, float w)
{
    if(!m_generator) return 0.0;

    SCellularCache &c=m_generator->get(x,y,z,w);
    return c.f[0]*m_coefficients[0] + c.f[1]*m_coefficients[1] + c.f[2]*m_coefficients[2] + c.f[3]*m_coefficients[3];
}
float CImplicitCellular::get(float x, float y, float z, float w, float u, float v)
{
    if(!m_generator) return 0.0;

    SCellularCache &c=m_generator->get(x,y,z,w,u,v);
    return c.f[0]*m_coefficients[0] + c.f[1]*m_coefficients[1] + c.f[2]*m_coefficients[2] + c.f[3]*m_coefficients[3];
}

};
