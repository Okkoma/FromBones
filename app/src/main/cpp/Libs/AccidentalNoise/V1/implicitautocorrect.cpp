#include "implicitautocorrect.h"
#include "../VCommon/random_gen.h"

namespace anl
{

CImplicitAutoCorrect::CImplicitAutoCorrect() : m_source(0), m_low(-1.0), m_high(1.0) {}
CImplicitAutoCorrect::CImplicitAutoCorrect(float low, float high) : m_source(0), m_low(low), m_high(high) {}

void CImplicitAutoCorrect::setSource(CImplicitModuleBase *b, int num) // Modified For FromBones Project.
{
    m_source=b;
    calculate();
}

void CImplicitAutoCorrect::setRange(float low, float high)
{
    m_low=low;
    m_high=high;
    calculate();
}

void CImplicitAutoCorrect::calculate()
{
    if(!m_source) return;
    float mn,mx;
    LCG lcg;
    //lcg.setSeedTime();

    // Calculate 2D
    mn=10000.0;
    mx=-10000.0;
    for(int c=0; c<10000; ++c)
    {
        float nx=lcg.get01()*4.0-2.0;
        float ny=lcg.get01()*4.0-2.0;

        float v=m_source->get(nx,ny);
        if(v<mn) mn=v;
        if(v>mx) mx=v;
    }
    m_scale2=(m_high-m_low) / (mx-mn);
    m_offset2=m_low-mn*m_scale2;

    // Calculate 3D
    mn=10000.0;
    mx=-10000.0;
    for(int c=0; c<10000; ++c)
    {
        float nx=lcg.get01()*4.0-2.0;
        float ny=lcg.get01()*4.0-2.0;
        float nz=lcg.get01()*4.0-2.0;

        float v=m_source->get(nx,ny,nz);
        if(v<mn) mn=v;
        if(v>mx) mx=v;
    }
    m_scale3=(m_high-m_low) / (mx-mn);
    m_offset3=m_low-mn*m_scale3;

    // Calculate 4D
    mn=10000.0;
    mx=-10000.0;
    for(int c=0; c<10000; ++c)
    {
        float nx=lcg.get01()*4.0-2.0;
        float ny=lcg.get01()*4.0-2.0;
        float nz=lcg.get01()*4.0-2.0;
        float nw=lcg.get01()*4.0-2.0;

        float v=m_source->get(nx,ny,nz,nw);
        if(v<mn) mn=v;
        if(v>mx) mx=v;
    }
    m_scale4=(m_high-m_low) / (mx-mn);
    m_offset4=m_low-mn*m_scale4;

    // Calculate 6D
    mn=10000.0;
    mx=-10000.0;
    for(int c=0; c<10000; ++c)
    {
        float nx=lcg.get01()*4.0-2.0;
        float ny=lcg.get01()*4.0-2.0;
        float nz=lcg.get01()*4.0-2.0;
        float nw=lcg.get01()*4.0-2.0;
        float nu=lcg.get01()*4.0-2.0;
        float nv=lcg.get01()*4.0-2.0;

        float v=m_source->get(nx,ny,nz,nw,nu,nv);
        if(v<mn) mn=v;
        if(v>mx) mx=v;
    }
    m_scale6=(m_high-m_low) / (mx-mn);
    m_offset6=m_low-mn*m_scale6;
}


float CImplicitAutoCorrect::get(float x, float y)
{
    if(!m_source) return 0.0;

    float v=m_source->get(x,y);
    return std::max(m_low, std::min(m_high, v*m_scale2+m_offset2));
}

float CImplicitAutoCorrect::get(float x, float y, float z)
{
    if(!m_source) return 0.0;

    float v=m_source->get(x,y,z);
    return std::max(m_low, std::min(m_high, v*m_scale3+m_offset3));
}
float CImplicitAutoCorrect::get(float x, float y, float z, float w)
{
    if(!m_source) return 0.0;

    float v=m_source->get(x,y,z,w);
    return std::max(m_low, std::min(m_high, v*m_scale4+m_offset4));
}

float CImplicitAutoCorrect::get(float x, float y, float z, float w, float u, float v)
{
    if(!m_source) return 0.0;

    float val=m_source->get(x,y,z,w,u,v);
    return std::max(m_low, std::min(m_high, val*m_scale6+m_offset6));
}
};
