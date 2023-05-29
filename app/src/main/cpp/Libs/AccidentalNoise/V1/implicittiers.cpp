#include "implicittiers.h"
#include "../VCommon/utility.h"
#include <math.h>

namespace anl
{
CImplicitTiers::CImplicitTiers() : m_source(0), m_numtiers(0), m_smooth(true) {}
CImplicitTiers::CImplicitTiers(int numtiers, bool smooth) : m_source(0), m_numtiers(numtiers), m_smooth(smooth) {}
CImplicitTiers::~CImplicitTiers() {}

void CImplicitTiers::setSource(float v)
{
    m_source.set(v);
}
void CImplicitTiers::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    m_source.set(b);
}
void CImplicitTiers::setNumTiers(int numtiers)
{
    m_numtiers=numtiers;
}
void CImplicitTiers::setSmooth(bool smooth)
{
    m_smooth=smooth;
}

float CImplicitTiers::get(float x, float y)
{
    int numsteps=m_numtiers;
    if(m_smooth) --numsteps;
    float val=m_source.get(x,y);
    float Tb=floor(val*(float)(numsteps));
    float Tt=Tb+1.0;
    float t=val*(float)(numsteps)-Tb;
    Tb/=(float)(numsteps);
    Tt/=(float)(numsteps);
    float u;
    if(m_smooth) u=quintic_blend(t);
    else u=0.0;
    return Tb+u*(Tt-Tb);
}
float CImplicitTiers::get(float x, float y, float z)
{
    int numsteps=m_numtiers;
    if(m_smooth) --numsteps;
    float val=m_source.get(x,y,z);
    float Tb=floor(val*(float)(numsteps));
    float Tt=Tb+1.0;
    float t=val*(float)(numsteps)-Tb;
    Tb/=(float)(numsteps);
    Tt/=(float)(numsteps);
    float u;
    if(m_smooth) u=quintic_blend(t);
    else u=0.0;
    return Tb+u*(Tt-Tb);
}
float CImplicitTiers::get(float x, float y, float z, float w)
{
    int numsteps=m_numtiers;
    if(m_smooth) --numsteps;
    float val=m_source.get(x,y,z,w);
    float Tb=floor(val*(float)(numsteps));
    float Tt=Tb+1.0;
    float t=val*(float)(numsteps)-Tb;
    Tb/=(float)(numsteps);
    Tt/=(float)(numsteps);
    float u;
    if(m_smooth) u=quintic_blend(t);
    else u=0.0;
    return Tb+u*(Tt-Tb);
}

float CImplicitTiers::get(float x, float y, float z, float w, float u, float v)
{
    int numsteps=m_numtiers;
    if(m_smooth) --numsteps;
    float val=m_source.get(x,y,z,w,u,v);
    float Tb=floor(val*(float)(numsteps));
    float Tt=Tb+1.0;
    float t=val*(float)(numsteps)-Tb;
    Tb/=(float)(numsteps);
    Tt/=(float)(numsteps);
    float s;
    if(m_smooth) s=quintic_blend(t);
    else s=0.0;
    return Tb+s*(Tt-Tb);
}
};
