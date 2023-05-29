#include "rgbaconstant.h"

namespace anl
{
CRGBAConstant::CRGBAConstant() : m_rgba(0) {}
CRGBAConstant::CRGBAConstant(ANLV1_SRGBA &r) : m_rgba(r) {}
CRGBAConstant::CRGBAConstant(float r, float g, float b, float a) : m_rgba(r,g,b,a) {}
CRGBAConstant::~CRGBAConstant() {}
void CRGBAConstant::set(float r, float g, float b, float a)
{
    m_rgba=ANLV1_SRGBA(r,g,b,a);
}
void CRGBAConstant::set(ANLV1_SRGBA &r)
{
    m_rgba=ANLV1_SRGBA(r);
}
ANLV1_SRGBA CRGBAConstant::get(float x, float y)
{
    return m_rgba;
}
ANLV1_SRGBA CRGBAConstant::get(float x, float y, float z)
{
    return m_rgba;
}
ANLV1_SRGBA CRGBAConstant::get(float x, float y, float z, float w)
{
    return m_rgba;
}
ANLV1_SRGBA CRGBAConstant::get(float x, float y, float z, float w, float u, float v)
{
    return m_rgba;
}
};
