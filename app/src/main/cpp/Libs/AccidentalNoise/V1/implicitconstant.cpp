#include "implicitconstant.h"

namespace anl
{
CImplicitConstant::CImplicitConstant() : m_constant(0) {}
CImplicitConstant::CImplicitConstant(float c) : m_constant(c) {}
CImplicitConstant::~CImplicitConstant() {}

void CImplicitConstant::setConstant(float c)
{
    m_constant=c;
}

float CImplicitConstant::get(float x, float y)
{
    return m_constant;
}
float CImplicitConstant::get(float x, float y, float z)
{
    return m_constant;
}
float CImplicitConstant::get(float x, float y, float z, float w)
{
    return m_constant;
}
float CImplicitConstant::get(float x, float y, float z, float w, float u, float v)
{
    return m_constant;
}
};
