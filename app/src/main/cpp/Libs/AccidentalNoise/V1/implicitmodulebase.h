#ifndef IMPLICIT_MODULE_BASE_H
#define IMPLICIT_MODULE_BASE_H

// Base class of implicit (2D, 4D, 6D) noise functions
#define MaxSources 20
namespace anl
{
class CImplicitModuleBase
{
public:
    CImplicitModuleBase() : m_spacing(0.0001f) {}
    virtual ~CImplicitModuleBase() {}
    void setDerivSpacing(float s)
    {
        m_spacing=s;
    }
    virtual void setSeed(unsigned int seed) {}
    virtual void setSource(float b) { } 	        // Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0) { } 	// Modified For FromBones Project.

    virtual float get(float x, float y)=0;
    virtual float get(float x, float y, float z)=0;
    virtual float get(float x, float y, float z, float w)=0;
    virtual float get(float x, float y, float z, float w, float u, float v)=0;

    float get_dx(float x, float y);
    float get_dy(float x, float y);

    float get_dx(float x, float y, float z);
    float get_dy(float x, float y, float z);
    float get_dz(float x, float y, float z);

    float get_dx(float x, float y, float z, float w);
    float get_dy(float x, float y, float z, float w);
    float get_dz(float x, float y, float z, float w);
    float get_dw(float x, float y, float z, float w);

    float get_dx(float x, float y, float z, float w, float u, float v);
    float get_dy(float x, float y, float z, float w, float u, float v);
    float get_dz(float x, float y, float z, float w, float u, float v);
    float get_dw(float x, float y, float z, float w, float u, float v);
    float get_du(float x, float y, float z, float w, float u, float v);
    float get_dv(float x, float y, float z, float w, float u, float v);

protected:
    float m_spacing;
};

// Scalar parameter class
class CScalarParameter
{
public:
    CScalarParameter(float v) : m_val(v), m_source(0) {}
    ~CScalarParameter() {}

    void set(float v)
    {
        m_source=0;
        m_val=v;
    }

    void set(CImplicitModuleBase *m)
    {
        m_source=m;
    }

    float get() const
    {
        return m_val;
    }
    float get(float x, float y)
    {
        if(m_source) return m_source->get(x,y);
        else return m_val;
    }

    float get(float x, float y, float z)
    {
        if(m_source) return m_source->get(x,y,z);
        else return m_val;
    }

    float get(float x, float y, float z, float w)
    {
        if(m_source) return m_source->get(x,y,z,w);
        else return m_val;
    }

    float get(float x, float y, float z, float w, float u, float v)
    {
        if(m_source) return m_source->get(x,y,z,w,u,v);
        else return m_val;
    }

protected:
    float m_val;
    CImplicitModuleBase *m_source;
};
};
#endif
