#include "implicitsin.h"
#include <math.h>

namespace anl
{
CImplicitSin::CImplicitSin() : m_source(0) {}
CImplicitSin::~CImplicitSin() {}

void CImplicitSin::setSource(float v)
{
    m_source.set(v);
}
void CImplicitSin::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

float CImplicitSin::get(float x, float y)
{
    return sin(m_source.get(x,y));
}
float CImplicitSin::get(float x, float y, float z)
{
    return sin(m_source.get(x,y,z));
}
float CImplicitSin::get(float x, float y, float z, float w)
{
    return sin(m_source.get(x,y,z,w));
}
float CImplicitSin::get(float x, float y, float z, float w, float u, float v)
{
    return sin(m_source.get(x,y,z,w,u,v));
}
};

