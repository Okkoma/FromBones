#include "implicitscaleoffset.h"

namespace anl
{
CImplicitScaleOffset::CImplicitScaleOffset(float scale, float offset):m_source(0), m_scale(scale), m_offset(offset) {}
CImplicitScaleOffset::~CImplicitScaleOffset() {}

void CImplicitScaleOffset::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}

void CImplicitScaleOffset::setSource(float v)
{
    m_source.set(v);
}

void CImplicitScaleOffset::setScale(float scale)
{
    m_scale.set(scale);
}
void CImplicitScaleOffset::setOffset(float offset)
{
    m_offset.set(offset);
}
void CImplicitScaleOffset::setScale(CImplicitModuleBase *scale)
{
    m_scale.set(scale);
}
void CImplicitScaleOffset::setOffset(CImplicitModuleBase *offset)
{
    m_offset.set(offset);
}

float CImplicitScaleOffset::get(float x, float y)
{

    return m_source.get(x,y)*m_scale.get(x,y)+m_offset.get(x,y);
}

float CImplicitScaleOffset::get(float x, float y, float z)
{


    return m_source.get(x,y,z)*m_scale.get(x,y,z)+m_offset.get(x,y,z);
}

float CImplicitScaleOffset::get(float x, float y, float z, float w)
{


    return m_source.get(x,y,z,w)*m_scale.get(x,y,z,w)+m_offset.get(x,y,z,w);
}

float CImplicitScaleOffset::get(float x, float y, float z, float w, float u, float v)
{


    return m_source.get(x,y,z,w,u,v)*m_scale.get(x,y,z,w,u,v)+m_offset.get(x,y,z,w,u,v);
}
};
