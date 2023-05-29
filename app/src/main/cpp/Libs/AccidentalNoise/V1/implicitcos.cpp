#include "implicitcos.h"
#include <math.h>

namespace anl
{
CImplicitCos::CImplicitCos() : m_source(0) {}
CImplicitCos::~CImplicitCos() {}

void CImplicitCos::setSource(float v)
{
    m_source.set(v);
}
void CImplicitCos::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

float CImplicitCos::get(float x, float y)
{
    return cos(m_source.get(x,y));
}
float CImplicitCos::get(float x, float y, float z)
{
    return cos(m_source.get(x,y,z));
}
float CImplicitCos::get(float x, float y, float z, float w)
{
    return cos(m_source.get(x,y,z,w));
}
float CImplicitCos::get(float x, float y, float z, float w, float u, float v)
{
    return cos(m_source.get(x,y,z,w,u,v));
}
};
