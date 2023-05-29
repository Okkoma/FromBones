#ifndef IMPLICIT_AUTOCORRECT_H
#define IMPLICIT_AUTOCORRECT_H
#include "implicitmodulebase.h"
/*************************************************
CImplicitAutoCorrect

    Define a function that will attempt to correct the range of its input to fall within a desired output
range.
    Operates by sampling the input function a number of times across an section of the domain, and using the
calculated min/max values to generate scale/offset pair to apply to the input. The calculate() function performs
this sampling and calculation, and is called automatically whenever a source is set, or whenever the desired output
ranges are set. Also, the function may be called manually, if desired.
***************************************************/

namespace anl
{
class CImplicitAutoCorrect : public CImplicitModuleBase
{
public:
    CImplicitAutoCorrect();
    CImplicitAutoCorrect(float low, float high);
    ~CImplicitAutoCorrect() {}

    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.
    void setRange(float low, float high);
    void calculate();

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CImplicitModuleBase *m_source;
    float m_low, m_high;
    float m_scale2, m_offset2;
    float m_scale3, m_offset3;
    float m_scale4, m_offset4;
    float m_scale6, m_offset6;

};
};


#endif
