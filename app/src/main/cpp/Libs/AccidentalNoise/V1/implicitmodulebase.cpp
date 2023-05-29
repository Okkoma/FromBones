#include "implicitmodulebase.h"

namespace anl
{
float CImplicitModuleBase::get_dx(float x, float y)
{
    return (get(x-m_spacing,y)-get(x+m_spacing,y))/m_spacing;
}

float CImplicitModuleBase::get_dy(float x, float y)
{
    return (get(x,y-m_spacing)-get(x,y+m_spacing))/m_spacing;
}

float CImplicitModuleBase::get_dx(float x, float y, float z)
{
    return (get(x-m_spacing,y,z)-get(x+m_spacing,y,z))/m_spacing;
}

float CImplicitModuleBase::get_dy(float x, float y, float z)
{
    return (get(x,y-m_spacing,z)-get(x,y+m_spacing,z))/m_spacing;
}

float CImplicitModuleBase::get_dz(float x, float y, float z)
{
    return (get(x,y,z-m_spacing)-get(x,y,z+m_spacing))/m_spacing;
}

float CImplicitModuleBase::get_dx(float x, float y, float z, float w)
{
    return (get(x-m_spacing,y,z,w)-get(x+m_spacing,y,z,w))/m_spacing;
}

float CImplicitModuleBase::get_dy(float x, float y, float z, float w)
{
    return (get(x,y-m_spacing,z,w)-get(x,y+m_spacing,z,w))/m_spacing;
}

float CImplicitModuleBase::get_dz(float x, float y, float z, float w)
{
    return (get(x,y,z-m_spacing,w)-get(x,y,z+m_spacing,w))/m_spacing;
}

float CImplicitModuleBase::get_dw(float x, float y, float z, float w)
{
    return (get(x,y,z,w-m_spacing)-get(x,y,z,w+m_spacing))/m_spacing;
}

float CImplicitModuleBase::get_dx(float x, float y, float z, float w, float u, float v)
{
    return (get(x-m_spacing,y,z,w,u,v)-get(x+m_spacing,y,z,w,u,v))/m_spacing;
}

float CImplicitModuleBase::get_dy(float x, float y, float z, float w, float u, float v)
{
    return (get(x,y-m_spacing,z,w,u,v)-get(x,y+m_spacing,z,w,u,v))/m_spacing;
}

float CImplicitModuleBase::get_dz(float x, float y, float z, float w, float u, float v)
{
    return (get(x,y,z-m_spacing,w,u,v)-get(x,y,z+m_spacing,w,u,v))/m_spacing;
}

float CImplicitModuleBase::get_dw(float x, float y, float z, float w, float u, float v)
{
    return (get(x,y,z,w-m_spacing,u,v)-get(x,y,z,w+m_spacing,u,v))/m_spacing;
}

float CImplicitModuleBase::get_du(float x, float y, float z, float w, float u, float v)
{
    return (get(x,y,z,w,u-m_spacing,v)-get(x,y,z,w,u+m_spacing,v))/m_spacing;
}

float CImplicitModuleBase::get_dv(float x, float y, float z, float w, float u, float v)
{
    return (get(x,y,z,w,u,v-m_spacing)-get(x,y,z,w,u,v+m_spacing))/m_spacing;
}

};
