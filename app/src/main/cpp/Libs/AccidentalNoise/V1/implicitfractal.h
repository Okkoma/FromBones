#ifndef IMPLICIT_FRACTAL_H
#define IMPLICIT_FRACTAL_H
#include "implicitmodulebase.h"
#include "implicitbasisfunction.h"
#include "../VCommon/utility.h"

namespace anl
{

enum EFractalTypes
{
    FBM,
    RIDGEDMULTI,
    BILLOW,
    MULTI,
    HYBRIDMULTI,
    DECARPENTIERSWISS
};



class CImplicitFractal : public CImplicitModuleBase
{
public:
    CImplicitFractal(unsigned int type, unsigned int basistype, unsigned int interptype);

    void setNumOctaves(int n);
    void setFrequency(float f);
    void setLacunarity(float l);
    void setGain(float g);
    void setOffset(float o);
    void setH(float h);

    void setType(unsigned int t);
    void setAllSourceTypes(unsigned int basis_type, unsigned int interp);
    void setSourceType(int which, unsigned int type, unsigned int interp);
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.
    void overrideSource(int which, CImplicitModuleBase *b);
    void resetSource(int which);
    void resetAllSources();
    void setSeed(unsigned int seed);
    CImplicitBasisFunction *getBasis(int which);
    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CImplicitBasisFunction m_basis[MaxSources];
    CImplicitModuleBase *m_source[MaxSources];
    float m_exparray[MaxSources];
    float m_correct[MaxSources][2];

    float m_offset, m_gain, m_H;
    float m_frequency, m_lacunarity;
    unsigned int m_numoctaves;
    unsigned int m_type;

    void fBm_calcWeights();
    void RidgedMulti_calcWeights();
    void Billow_calcWeights();
    void Multi_calcWeights();
    void HybridMulti_calcWeights();
    void DeCarpentierSwiss_calcWeights();

    float fBm_get(float x, float y);
    float fBm_get(float x, float y, float z);
    float fBm_get(float x, float y, float z, float w);
    float fBm_get(float x, float y, float z, float w, float u, float v);

    float RidgedMulti_get(float x, float y);
    float RidgedMulti_get(float x, float y, float z);
    float RidgedMulti_get(float x, float y, float z, float w);
    float RidgedMulti_get(float x, float y, float z, float w, float u, float v);

    float Billow_get(float x, float y);
    float Billow_get(float x, float y, float z);
    float Billow_get(float x, float y, float z, float w);
    float Billow_get(float x, float y, float z, float w, float u, float v);

    float Multi_get(float x, float y);
    float Multi_get(float x, float y, float z);
    float Multi_get(float x, float y, float z, float w);
    float Multi_get(float x, float y, float z, float w, float u, float v);

    float HybridMulti_get(float x, float y);
    float HybridMulti_get(float x, float y, float z);
    float HybridMulti_get(float x, float y, float z, float w);
    float HybridMulti_get(float x, float y, float z, float w, float u, float v);

    float DeCarpentierSwiss_get(float x, float y);
    float DeCarpentierSwiss_get(float x, float y, float z);
    float DeCarpentierSwiss_get(float x, float y, float z, float w);
    float DeCarpentierSwiss_get(float x, float y, float z, float w, float u, float v);


};

};
#endif
