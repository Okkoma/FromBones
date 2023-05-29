#include "implicitnormalizecoords.h"
#include <cmath>

namespace anl
{
CImplicitNormalizeCoords::CImplicitNormalizeCoords() : m_source(0), m_length(1) {}
CImplicitNormalizeCoords::CImplicitNormalizeCoords(float length) : m_source(0), m_length(length) {}

void CImplicitNormalizeCoords::setSource(float v)
{
    m_source.set(v);
}
void CImplicitNormalizeCoords::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

void CImplicitNormalizeCoords::setLength(float v)
{
    m_length.set(v);
}
void CImplicitNormalizeCoords::setLength(CImplicitModuleBase *v)
{
    m_length.set(v);
}

float CImplicitNormalizeCoords::get(float x, float y)
{
    if(x==0 && y==0) return m_source.get(x,y);

    float len=sqrt(x*x+y*y);
    float r=m_length.get(x,y);
    return m_source.get(x/len*r, y/len*r);
}

float CImplicitNormalizeCoords::get(float x, float y, float z)
{
    if(x==0 && y==0 && z==0) return m_source.get(x,y,z);

    float len=sqrt(x*x+y*y+z*z);
    float r=m_length.get(x,y,z);
    return m_source.get(x/len*r, y/len*r, z/len*r);
}
float CImplicitNormalizeCoords::get(float x, float y, float z, float w)
{
    if(x==0 && y==0 && z==0 && w==0) return m_source.get(x,y,z,w);

    float len=sqrt(x*x+y*y+z*z+w*w);
    float r=m_length.get(x,y,z,w);
    return m_source.get(x/len*r, y/len*r, z/len*r, w/len*r);
}

float CImplicitNormalizeCoords::get(float x, float y, float z, float w, float u, float v)
{
    if(x==0 && y==0 && z==0 && w==0 && u==0 && v==0) return m_source.get(x,y,z,w,u,v);

    float len=sqrt(x*x+y*y+z*z+w*w+u*u+v*v);
    float r=m_length.get(x,y,z,w,u,v);
    return m_source.get(x/len*r, y/len*r, z/len*r, w/len*r, u/len*r, v/len*r);
}
};

