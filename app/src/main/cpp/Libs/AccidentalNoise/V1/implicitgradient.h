#ifndef IMPLICIT_GRADIENT_H
#define IMPLICIT_GRADIENT_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitGradient : public CImplicitModuleBase
{
public:
    CImplicitGradient();
    ~CImplicitGradient();

    void setGradient(float x1, float x2, float y1, float y2, float z1=0, float z2=0,
                     float w1=0, float w2=0, float u1=0, float u2=0, float v1=0, float v2=0);


    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);


protected:
    float m_gx1, m_gy1, m_gz1, m_gw1, m_gu1, m_gv1;
    float m_gx2, m_gy2, m_gz2, m_gw2, m_gu2, m_gv2;
    float m_x, m_y, m_z, m_w, m_u, m_v;
    float m_vlen;
};
};
#endif
