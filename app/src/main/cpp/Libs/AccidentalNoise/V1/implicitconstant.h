#ifndef IMPLICITCONSTANT_H
#define IMPLICITCONSTANT_H
#include "implicitmodulebase.h"

namespace anl
{
class CImplicitConstant : public CImplicitModuleBase
{
public:
    CImplicitConstant();
    CImplicitConstant(float c);
    ~CImplicitConstant();

    void setConstant(float c);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    float m_constant;
};
};

#endif
