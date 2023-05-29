#include "rgbaimplicitgrayscale.h"

namespace anl
{
CRGBAImplicitGrayscale::CRGBAImplicitGrayscale() : m_source(0) {}
CRGBAImplicitGrayscale::~CRGBAImplicitGrayscale() {}
void CRGBAImplicitGrayscale::setSource(CImplicitModuleBase *m)
{
    m_source=m;
}

ANLV1_SRGBA CRGBAImplicitGrayscale::get(float x, float y)
{
    if(m_source==0) return ANLV1_SRGBA(0,0,0,0);

    float val=m_source->get(x,y);
    return ANLV1_SRGBA((float)val,(float)val,(float)val,1.0f);
}
ANLV1_SRGBA CRGBAImplicitGrayscale::get(float x, float y, float z)
{
    if(m_source==0) return ANLV1_SRGBA(0,0,0,0);

    float val=m_source->get(x,y,z);
    return ANLV1_SRGBA((float)val,(float)val,(float)val,1.0f);
}

ANLV1_SRGBA CRGBAImplicitGrayscale::get(float x, float y, float z, float w)
{
    if(m_source==0) return ANLV1_SRGBA(0,0,0,0);

    float val=m_source->get(x,y,z,w);
    return ANLV1_SRGBA((float)val,(float)val,(float)val,1.0f);
}
ANLV1_SRGBA CRGBAImplicitGrayscale::get(float x, float y, float z, float w, float u, float v)
{
    if(m_source==0) return ANLV1_SRGBA(0,0,0,0);

    float val=m_source->get(x,y,z,w,u,v);
    return ANLV1_SRGBA((float)val,(float)val,(float)val,1.0f);
}
};
