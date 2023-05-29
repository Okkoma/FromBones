#ifndef IMPLICIT_ROTATEDOMAIN_H
#define IMPLICIT_ROTATEDOMAIN_H
#include "implicitmodulebase.h"

namespace anl
{
/*
Given angle r in radians and unit vector u = ai + bj + ck or [a,b,c]', define:

q0 = cos(r/2),  q1 = sin(r/2) a,  q2 = sin(r/2) b,  q3 = sin(r/2) c

and construct from these values the rotation matrix:

     (q0² + q1² - q2² - q3²)      2(q1q2 - q0q3)          2(q1q3 + q0q2)

Q  =      2(q2q1 + q0q3)     (q0² - q1² + q2² - q3²)      2(q2q3 - q0q1)

          2(q3q1 - q0q2)          2(q3q2 + q0q1)     (q0² - q1² - q2² + q3²)

Multiplication by Q then effects the desired rotation, and in particular:

Q u = u
*/

class CImplicitRotateDomain : public CImplicitModuleBase
{
public:
    CImplicitRotateDomain(float ax, float ay, float az, float angle_deg);
    ~CImplicitRotateDomain();
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Added For FromBones Project.
    virtual void setSource(float v);
    void setAxis(float ax, float ay, float az);
    void setAxis(CImplicitModuleBase *ax, CImplicitModuleBase *ay, CImplicitModuleBase *az);
    void setAxisX(float ax);
    void setAxisY(float ay);
    void setAxisZ(float az);
    void setAxisX(CImplicitModuleBase *ax);
    void setAxisY(CImplicitModuleBase *ay);
    void setAxisZ(CImplicitModuleBase *az);

    void setAngle(float a);
    void setAngle(CImplicitModuleBase *a);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    float m_rotmatrix[3][3];
    CScalarParameter m_ax,m_ay,m_az, m_angledeg;
    CScalarParameter m_source;

    void calculateRotMatrix(float x, float y);
    void calculateRotMatrix(float x, float y, float z);
    void calculateRotMatrix(float x, float y, float z, float w);
    void calculateRotMatrix(float x, float y, float z, float w, float u, float v);
};
};

#endif
