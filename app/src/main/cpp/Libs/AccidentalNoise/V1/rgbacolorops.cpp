#include "rgbacolorops.h"
#include <algorithm>
#include <cmath>

namespace anl
{
CRGBAColorOps::CRGBAColorOps() : m_source1(0,0,0,0), m_source2(0,0,0,0), m_op(SOFTLIGHT) {}
CRGBAColorOps::CRGBAColorOps(int op) : m_source1(0,0,0,0), m_source2(0,0,0,0)
{
    setOperation(op);
}
CRGBAColorOps::~CRGBAColorOps() {}
void CRGBAColorOps::setOperation(int op)
{
    m_op=op;
    if(m_op<0) m_op=0;
    if(m_op>LINEARBURN) m_op=LINEARBURN;
}

void CRGBAColorOps::setSource1(float r, float g, float b, float a)
{
    m_source1.set(r,g,b,a);
}
void CRGBAColorOps::setSource1(CRGBAModuleBase *m)
{
    m_source1.set(m);
}
void CRGBAColorOps::setSource2(float r, float g, float b, float a)
{
    m_source2.set(r,g,b,a);
}
void CRGBAColorOps::setSource2(CRGBAModuleBase *m)
{
    m_source2.set(m);
}

ANLV1_SRGBA CRGBAColorOps::multiply(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    return ANLV1_SRGBA(s1[0]*s2[0], s1[1]*s2[1], s1[2]*s2[2], s1[3]);
}
ANLV1_SRGBA CRGBAColorOps::add(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    return ANLV1_SRGBA(s1[0]+s2[0], s1[1]+s2[1], s1[2]+s2[2], s1[3]);
}
ANLV1_SRGBA CRGBAColorOps::screen(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    r=s1[0] + s2[0] - s1[0]*s2[0];
    g=s1[1] + s2[1] - s1[1]*s2[1];
    b=s1[2] + s2[2] - s1[2]*s2[2];
    return ANLV1_SRGBA(r,g,b,s1[3]);
}
ANLV1_SRGBA CRGBAColorOps::overlay(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b,a;
    r = (s2[0] < 0.5f) ? (2.0f * s1[0] * s2[0]) : (1.0f-2.0f * (1.0f-s1[0]) * (1.0f-s2[0]));
    g = (s2[1] < 0.5f) ? (2.0f * s1[1] * s2[1]) : (1.0f-2.0f * (1.0f-s1[1]) * (1.0f-s2[1]));
    b = (s2[2] < 0.5f) ? (2.0f * s1[2] * s2[2]) : (1.0f-2.0f * (1.0f-s1[2]) * (1.0f-s2[2]));
    a=s1[3];
    return ANLV1_SRGBA(r,g,b,a);
}

ANLV1_SRGBA CRGBAColorOps::softLight(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    if(s1[0] > 0.5f)
    {
        r=s2[0]+(1.0f-s2[0])*((s1[0]-0.5f) / 0.5f) * (0.5 - std::abs(s2[0]-0.5f));
    }
    else
    {
        r= s2[0] - s2[0] * ((0.5f-s1[0]) / 0.5f) * (0.5f - std::abs(s2[0]-0.5f));
    }

    if(s1[1] > 0.5f)
    {
        g=s2[1]+(1.0f-s2[1])*((s1[1]-0.5f) / 0.5f) * (0.5 - std::abs(s2[1]-0.5f));
    }
    else
    {
        g= s2[1] - s2[1] * ((0.5f-s1[1]) / 0.5f) * (0.5f - std::abs(s2[1]-0.5f));
    }

    if(s1[2] > 0.5f)
    {
        b=s2[2]+(1.0f-s2[2])*((s1[2]-0.5f) / 0.5f) * (0.5 - std::abs(s2[2]-0.5f));
    }
    else
    {
        b= s2[2] - s2[2] * ((0.5f-s1[2]) / 0.5f) * (0.5f - std::abs(s2[2]-0.5f));
    }

    return ANLV1_SRGBA(r,g,b,s1[3]);
}

ANLV1_SRGBA CRGBAColorOps::hardLight(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    if(s1[0] > 0.5f)
    {
        r = s2[0] + (1.0f - s2[0]) * ((s1[0] - 0.5f) / 0.5f);
    }
    else
    {
        r = s2[0] * s1[0] / 0.5f;
    }

    if(s1[1] > 0.5f)
    {
        g = s2[1] + (1.0f - s2[1]) * ((s1[1] - 0.5f) / 0.5f);
    }
    else
    {
        g = s2[1] * s1[1] / 0.5f;
    }

    if(s1[2] > 0.5f)
    {
        b = s2[2] + (1.0f - s2[2]) * ((s1[2] - 0.5f) / 0.5f);
    }
    else
    {
        b = s2[2] * s1[2] / 0.5f;
    }

    return ANLV1_SRGBA(r,g,b,s1[3]);
}

ANLV1_SRGBA CRGBAColorOps::dodge(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    r = (s1[0] == 1.0f) ? s1[0] : std::min(1.0f, ((s2[0]) / (1.0f-s1[0])));
    g = (s1[1] == 1.0f) ? s1[1] : std::min(1.0f, ((s2[1]) / (1.0f-s1[1])));
    b = (s1[2] == 1.0f) ? s1[2] : std::min(1.0f, ((s2[2]) / (1.0f-s1[2])));

    return ANLV1_SRGBA(r,g,b,s1[3]);
}

ANLV1_SRGBA CRGBAColorOps::burn(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    r = (s1[0]==1.0f) ? s1[0] : std::max(0.0f, (1.0f - ((1.0f-s2[0])) / s1[0]));
    g = (s1[1]==1.0f) ? s1[1] : std::max(0.0f, (1.0f - ((1.0f-s2[1])) / s1[1]));
    b = (s1[2]==1.0f) ? s1[2] : std::max(0.0f, (1.0f - ((1.0f-s2[2])) / s1[2]));

    return ANLV1_SRGBA(r,g,b,s1[3]);
}

ANLV1_SRGBA CRGBAColorOps::linearDodge(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    r = std::min(s1[0]+s2[0], 1.0f);
    g = std::min(s1[1]+s2[1], 1.0f);
    b = std::min(s1[2]+s2[2], 1.0f);

    return ANLV1_SRGBA(r,g,b,s1[3]);
}

ANLV1_SRGBA CRGBAColorOps::linearBurn(ANLV1_SRGBA &s1, ANLV1_SRGBA &s2)
{
    float r,g,b;
    r = ((s1[0] + s2[0]) < 1.0f) ? 0.0f : (s1[0] + s2[0] - 1.0f);
    g = ((s1[1] + s2[1]) < 1.0f) ? 0.0f : (s1[1] + s2[1] - 1.0f);
    b = ((s1[2] + s2[2]) < 1.0f) ? 0.0f : (s1[2] + s2[2] - 1.0f);

    return ANLV1_SRGBA(r,g,b,s1[3]);
}

ANLV1_SRGBA CRGBAColorOps::get(float x, float y)
{
    ANLV1_SRGBA s1=m_source1.get(x,y);
    ANLV1_SRGBA s2=m_source2.get(x,y);
    switch(m_op)
    {
    case COLORMULTIPLY:
        return multiply(s1,s2);
        break;
    case COLORADD:
        return add(s1,s2);
        break;
    case SCREEN:
        return screen(s1,s2);
        break;
    case OVERLAY:
        return overlay(s1,s2);
        break;
    case SOFTLIGHT:
        return softLight(s1,s2);
        break;
    case HARDLIGHT:
        return hardLight(s1,s2);
        break;
    case DODGE:
        return dodge(s1,s2);
        break;
    case BURN:
        return burn(s1,s2);
        break;
    case LINEARDODGE:
        return linearDodge(s1,s2);
        break;
    case LINEARBURN:
        return linearBurn(s1,s2);
        break;
    default:
        return softLight(s1,s2);
        break;
    }
}

ANLV1_SRGBA CRGBAColorOps::get(float x, float y, float z)
{
    ANLV1_SRGBA s1=m_source1.get(x,y,z);
    ANLV1_SRGBA s2=m_source2.get(x,y,z);
    switch(m_op)
    {
    case COLORMULTIPLY:
        return multiply(s1,s2);
        break;
    case COLORADD:
        return add(s1,s2);
        break;
    case SCREEN:
        return screen(s1,s2);
        break;
    case OVERLAY:
        return overlay(s1,s2);
        break;
    case SOFTLIGHT:
        return softLight(s1,s2);
        break;
    case HARDLIGHT:
        return hardLight(s1,s2);
        break;
    case DODGE:
        return dodge(s1,s2);
        break;
    case BURN:
        return burn(s1,s2);
        break;
    case LINEARDODGE:
        return linearDodge(s1,s2);
        break;
    case LINEARBURN:
        return linearBurn(s1,s2);
        break;
    default:
        return softLight(s1,s2);
        break;
    }
}

ANLV1_SRGBA CRGBAColorOps::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA s1=m_source1.get(x,y,z,w);
    ANLV1_SRGBA s2=m_source2.get(x,y,z,w);
    switch(m_op)
    {
    case COLORMULTIPLY:
        return multiply(s1,s2);
        break;
    case COLORADD:
        return add(s1,s2);
        break;
    case SCREEN:
        return screen(s1,s2);
        break;
    case OVERLAY:
        return overlay(s1,s2);
        break;
    case SOFTLIGHT:
        return softLight(s1,s2);
        break;
    case HARDLIGHT:
        return hardLight(s1,s2);
        break;
    case DODGE:
        return dodge(s1,s2);
        break;
    case BURN:
        return burn(s1,s2);
        break;
    case LINEARDODGE:
        return linearDodge(s1,s2);
        break;
    case LINEARBURN:
        return linearBurn(s1,s2);
        break;
    default:
        return softLight(s1,s2);
        break;
    }
}

ANLV1_SRGBA CRGBAColorOps::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA s1=m_source1.get(x,y,z,w,u,v);
    ANLV1_SRGBA s2=m_source2.get(x,y,z,w,u,v);
    switch(m_op)
    {
    case COLORMULTIPLY:
        return multiply(s1,s2);
        break;
    case COLORADD:
        return add(s1,s2);
        break;
    case SCREEN:
        return screen(s1,s2);
        break;
    case OVERLAY:
        return overlay(s1,s2);
        break;
    case SOFTLIGHT:
        return softLight(s1,s2);
        break;
    case HARDLIGHT:
        return hardLight(s1,s2);
        break;
    case DODGE:
        return dodge(s1,s2);
        break;
    case BURN:
        return burn(s1,s2);
        break;
    case LINEARDODGE:
        return linearDodge(s1,s2);
        break;
    case LINEARBURN:
        return linearBurn(s1,s2);
        break;
    default:
        return softLight(s1,s2);
        break;
    }
}

};
