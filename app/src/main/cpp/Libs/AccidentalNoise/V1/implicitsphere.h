#ifndef IMPLICIT_SPHERE_H
#define IMPLICIT_SPHERE_H
#include "implicitmodulebase.h"
#include <cmath>

namespace anl
{
class CImplicitSphere : public CImplicitModuleBase
{
public:
    CImplicitSphere();
    ~CImplicitSphere();
    void setCenter(float cx,float cy,float cz=0,float cw=0,float cu=0,float cv=0);
    void setCenterX(float cx);
    void setCenterY(float cy);
    void setCenterZ(float cz);
    void setCenterW(float cw);
    void setCenterU(float cu);
    void setCenterV(float cv);
    void setCenterX(CImplicitModuleBase *cx);
    void setCenterY(CImplicitModuleBase *cy);
    void setCenterZ(CImplicitModuleBase *cz);
    void setCenterW(CImplicitModuleBase *cw);
    void setCenterU(CImplicitModuleBase *cu);
    void setCenterV(CImplicitModuleBase *cv);

    void setRadius(float r);
    void setRadius(CImplicitModuleBase *r);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CImplicitModuleBase *m_source;
    CScalarParameter m_cx, m_cy, m_cz, m_cw, m_cu, m_cv;
    CScalarParameter m_radius;

};
};

#endif
