#ifndef IMPLICIT_MODIFIER_H
#define IMPLICIT_MODIFIER_H
#include "implicitmodulebase.h"
#include "../utility.h"
#include "../curvetypes.h"

// Modules that modify the output of a function

class CImplicitCurve : public CImplicitModuleBase
{
public:
    CImplicitCurve() : m_source(0) {}
    ~CImplicitCurve() {}

    virtual void setSource(CImplicitModuleBase *b, int num=0) 	// Modified For FromBones Project.
    {
        m_source=b;
    }

    void setControlPoint(float v, float p)
    {
        m_curve.pushPoint(v,p);
    }

    void clearControlPoints()
    {
        m_curve.clear();
    }

    float get(float x, float y)
    {
        if(!m_source) return 0.0;

        float v=m_source->get(x,y);
        // Should clamp; make sure inputs are in range
        //v=(v+1.0 ) * 0.5;
        v=clamp(v,0.0,1.0);
        return m_curve.linearInterp(v);
    }

    float get(float x, float y, float z)
    {
        if(!m_source) return 0;
        float v=m_source->get(x,y,z);
        //v=(v+1)*0.5;
        v=clamp(v,0.0,1.0);
        return m_curve.linearInterp(v);
    }

    float get(float x, float y, float z, float w)
    {
        if(!m_source) return 0.0;

        float v=m_source->get(x,y,z,w);
        // Should clamp; make sure inputs are in range
        //v=(v+1.0 ) * 0.5;
        v=clamp(v,0.0,1.0);
        return m_curve.linearInterp(v);
    }

    float get(float x, float y, float z, float w, float u, float v)
    {
        if(!m_source) return 0.0;

        float val=m_source->get(x,y,z,w,u,v);
        // Should clamp; make sure inputs are in range
        //val=(val+1.0 ) * 0.5;
        val=clamp(val,0.0,1.0);
        return m_curve.linearInterp(val);
    }



protected:
    CImplicitModuleBase *m_source;
    CCurved m_curve;

};


#endif
