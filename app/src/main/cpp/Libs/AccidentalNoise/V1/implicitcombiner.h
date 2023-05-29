#ifndef IMPLICIT_COMBINER_H
#define IMPLICIT_COMBINER_H
#include "implicitmodulebase.h"

namespace anl
{
enum ECombinerTypes
{
    ADD,
    SUB,
    MULT,
    MAX,
    MIN,
    AVG
};

class CImplicitCombiner : public CImplicitModuleBase
{
public:
    CImplicitCombiner(unsigned int type);
    void setType(unsigned int type);
    void clearAllSources();
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.
    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CImplicitModuleBase *m_sources[MaxSources];
    unsigned int m_type;

    float Add_get(float x, float y);
    float Add_get(float x, float y, float z);
    float Add_get(float x, float y, float z, float w);
    float Add_get(float x, float y, float z, float w, float u, float v);
    float Sub_get(float x, float y);
    float Mult_get(float x, float y);
    float Mult_get(float x, float y, float z);
    float Mult_get(float x, float y, float z, float w);
    float Mult_get(float x, float y, float z, float w, float u, float v);
    float Min_get(float x, float y);
    float Min_get(float x, float y, float z);
    float Min_get(float x, float y, float z, float w);
    float Min_get(float x, float y, float z, float w, float u, float v);
    float Max_get(float x, float y);
    float Max_get(float x, float y, float z);
    float Max_get(float x, float y, float z, float w);
    float Max_get(float x, float y, float z, float w, float u, float v);
    float Avg_get(float x, float y);
    float Avg_get(float x, float y, float z);
    float Avg_get(float x, float y, float z, float w);
    float Avg_get(float x, float y, float z, float w, float u, float v);
};
};

#endif
