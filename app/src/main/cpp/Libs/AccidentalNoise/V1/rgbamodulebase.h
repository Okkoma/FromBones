#ifndef RGBA_MODULE_BASE_H
#define RGBA_MODULE_BASE_H

#include "vectortypes.h"

namespace anl
{
class CRGBAModuleBase
{
public:
    CRGBAModuleBase() {}
    virtual ~CRGBAModuleBase() {}

    void setSeed(unsigned int) {};

    virtual ANLV1_SRGBA get(float x, float y)=0;
    virtual ANLV1_SRGBA get(float x, float y, float z)=0;
    virtual ANLV1_SRGBA get(float x, float y, float z, float w)=0;
    virtual ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v)=0;
};

class CRGBAParameter
{
public:
    CRGBAParameter() : m_source(0), m_constant(0,0,0,0) {}
    CRGBAParameter(float r, float g, float b, float a) : m_source(0), m_constant(r,g,b,a) {}

    void set(float r, float g, float b, float a)
    {
        m_source=0;
        m_constant=ANLV1_SRGBA(r,g,b,a);
    }
    void set(CRGBAModuleBase *m)
    {
        m_source=m;
    }

    ANLV1_SRGBA get(float x, float y)
    {
        if(m_source) return m_source->get(x,y);
        else return m_constant;
    }

    ANLV1_SRGBA get(float x, float y, float z)
    {
        if(m_source) return m_source->get(x,y,z);
        else return m_constant;
    }

    ANLV1_SRGBA get(float x, float y, float z, float w)
    {
        if(m_source) return m_source->get(x,y,z,w);
        else return m_constant;
    }

    ANLV1_SRGBA get(float x, float y, float z, float w, float u, float v)
    {
        if(m_source) return m_source->get(x,y,z,w,u,v);
        else return m_constant;
    }


protected:
    CRGBAModuleBase *m_source;
    ANLV1_SRGBA m_constant;

};

};

#endif
