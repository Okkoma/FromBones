#ifndef IMPLICIT_RADIAL_H
#define IMPLICIT_RADIAL_H
#include "implicitmodulebase.h"
#include <cmath>

namespace anl
{
class CImplicitRadial : public CImplicitModuleBase
{
public:

    CImplicitRadial() : CImplicitModuleBase() { }
    ~CImplicitRadial() { }

    void setRadius(float radius)
    {
        m_radius = radius;
    }
    float getRadius() const
    {
        return m_radius;
    }

    float get(float x, float y)
    {
        return m_radius-sqrt(x*x+y*y);
    }
    float get(float x, float y, float z)
    {
        return m_radius-sqrt(x*x+y*y+z*z);
    }
    float get(float x, float y, float z, float w)
    {
        return m_radius-sqrt(x*x+y*y+z*z+w*w);
    }
    float get(float x, float y, float z, float w, float u, float v)
    {
        return m_radius-sqrt(x*x+y*y+z*z+w*w+u*u+v*v);
    }

protected:
    float m_radius;
};
};

#endif
