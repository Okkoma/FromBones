#include "implicitextractrgbachannel.h"

namespace anl
{
CImplicitExtractRGBAChannel::CImplicitExtractRGBAChannel() : m_source(0,0,0,0), m_channel(RED) {}
CImplicitExtractRGBAChannel::CImplicitExtractRGBAChannel(int channel) : m_source(0,0,0,0), m_channel(channel) {}

void CImplicitExtractRGBAChannel::setSource(CRGBAModuleBase *m)
{
    m_source.set(m);
}
void CImplicitExtractRGBAChannel::setSource(float r, float g, float b, float a)
{
    m_source.set(r,g,b,a);
}
void CImplicitExtractRGBAChannel::setChannel(int channel)
{
    m_channel=channel;
    if(channel<RED) channel=RED;
    if(channel>ALPHA) channel=ALPHA;
}
float CImplicitExtractRGBAChannel::get(float x, float y)
{
    ANLV1_SRGBA s=m_source.get(x,y);

    return s[m_channel];
}

float CImplicitExtractRGBAChannel::get(float x, float y, float z)
{
    ANLV1_SRGBA s=m_source.get(x,y,z);

    return s[m_channel];
}
float CImplicitExtractRGBAChannel::get(float x, float y, float z, float w)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w);

    return s[m_channel];
}
float CImplicitExtractRGBAChannel::get(float x, float y, float z, float w, float u, float v)
{
    ANLV1_SRGBA s=m_source.get(x,y,z,w,u,v);

    return s[m_channel];
}
};
