#include "implicitfloor.h"
#include <math.h>

namespace anl
{
CImplicitFloor::CImplicitFloor() : m_source(0) {}
CImplicitFloor::~CImplicitFloor() {}

void CImplicitFloor::setSource(float v)
{
    m_source.set(v);
}
void CImplicitFloor::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}
float CImplicitFloor::get(float x, float y)
{
    return floor(m_source.get(x,y));
}
float CImplicitFloor::get(float x, float y, float z)
{
    return floor(m_source.get(x,y,z));
}
float CImplicitFloor::get(float x, float y, float z, float w)
{
    return floor(m_source.get(x,y,z,w));
}
float CImplicitFloor::get(float x, float y, float z, float w, float u, float v)
{
    return floor(m_source.get(x,y,z,w,u,v));
}
};
