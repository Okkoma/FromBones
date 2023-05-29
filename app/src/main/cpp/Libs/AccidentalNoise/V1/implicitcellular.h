#ifndef IMPLICIT_CELLULAR_H
#define IMPLICIT_CELLULAR_H
#include "implicitmodulebase.h"
#include "cellulargen.h"

namespace anl
{
class CImplicitCellular : public CImplicitModuleBase
{
public:
    CImplicitCellular();
    CImplicitCellular(float a, float b, float c, float d);
    ~CImplicitCellular() {}

    void setCoefficients(float a, float b, float c, float d);
    void setCellularSource(CCellularGenerator *m);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CCellularGenerator *m_generator;
    float m_coefficients[4];
};
};

#endif
