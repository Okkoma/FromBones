#include "implicitfractal.h"
#include <cmath>
#include <iostream>

using namespace anl;
CImplicitFractal::CImplicitFractal(unsigned int type, unsigned int basistype, unsigned int interptype)
{
    setNumOctaves(8);
    setFrequency(1.0);
    setLacunarity(2.0);
    setType(type);
    setAllSourceTypes(basistype, interptype);
    resetAllSources();
}

void CImplicitFractal::setNumOctaves(int n)
{
    if(n>=MaxSources) n=MaxSources-1;
    m_numoctaves=n;
}
void CImplicitFractal::setFrequency(float f)
{
    m_frequency=f;
}
void CImplicitFractal::setLacunarity(float l)
{
    m_lacunarity=l;
}
void CImplicitFractal::setGain(float g)
{
    m_gain=g;
}
void CImplicitFractal::setOffset(float o)
{
    m_offset=o;
}
void CImplicitFractal::setH(float h)
{
    m_H=h;
}

void CImplicitFractal::setType(unsigned int t)
{
    m_type=t;
    switch(t)
    {
    case FBM:
        m_H=1.0;
        m_gain=0.5;
        m_offset=0;
        fBm_calcWeights();
        break;
    case RIDGEDMULTI:
        m_H=0.9;
        m_gain=0.5;
        m_offset=1;
        RidgedMulti_calcWeights();
        break;
    case BILLOW:
        m_H=1;
        m_gain=0.5;
        m_offset=0;
        Billow_calcWeights();
        break;
    case MULTI:
        m_H=1;
        m_offset=0;
        m_gain=0;
        Multi_calcWeights();
        break;
    case HYBRIDMULTI:
        m_H=0.25;
        m_gain=1;
        m_offset=0.7;
        HybridMulti_calcWeights();
        break;
    case DECARPENTIERSWISS:
        m_H=0.9;
        m_gain=0.6;
        m_offset=0.15;
        DeCarpentierSwiss_calcWeights();
        break;
    default:
        m_H=1.0;
        m_gain=0;
        m_offset=0;
        fBm_calcWeights();
        break;
    };
}

void CImplicitFractal::setAllSourceTypes(unsigned int basis_type, unsigned int interp)
{
    for(int i=0; i<MaxSources; ++i)
    {
        m_basis[i].setType(basis_type);
        m_basis[i].setInterp(interp);
    }
}

void CImplicitFractal::setSourceType(int which, unsigned int type, unsigned int interp)
{
    if(which>=MaxSources || which<0) return;
    m_basis[which].setType(type);
    m_basis[which].setInterp(interp);
}

void CImplicitFractal::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    if(which<0 || which>=MaxSources) return;
    m_source[which]=b;
}

void CImplicitFractal::overrideSource(int which, CImplicitModuleBase *b)
{
    if(which<0 || which>=MaxSources) return;
    m_source[which]=b;
}

void CImplicitFractal::resetSource(int which)
{
    if(which<0 || which>=MaxSources) return;
    m_source[which]=&m_basis[which];
}

void CImplicitFractal::resetAllSources()
{
    for(int c=0; c<MaxSources; ++c) m_source[c] = &m_basis[c];
}


void CImplicitFractal::setSeed(unsigned int seed)
{
    for(int c=0; c<MaxSources; ++c) m_source[c]->setSeed(seed+c*300);
}

CImplicitBasisFunction *CImplicitFractal::getBasis(int which)
{
    if(which<0 || which>=MaxSources) return 0;
    return &m_basis[which];
}

float CImplicitFractal::get(float x, float y)
{
    float v;
    switch(m_type)
    {
    case FBM:
        v=fBm_get(x,y);
        break;
    case RIDGEDMULTI:
        v=RidgedMulti_get(x,y);
        break;
    case BILLOW:
        v=Billow_get(x,y);
        break;
    case MULTI:
        v=Multi_get(x,y);
        break;
    case HYBRIDMULTI:
        v=HybridMulti_get(x,y);
        break;
    case DECARPENTIERSWISS:
        v=DeCarpentierSwiss_get(x,y);
        break;
    default:
        v=fBm_get(x,y);
        break;
    }
    //return clamp(v,-1.0,1.0);
    return v;
}

float CImplicitFractal::get(float x, float y, float z)
{
    float val;
    switch(m_type)
    {
    case FBM:
        val=fBm_get(x,y,z);
        break;
    case RIDGEDMULTI:
        val=RidgedMulti_get(x,y,z);
        break;
    case BILLOW:
        val=Billow_get(x,y,z);
        break;
    case MULTI:
        val=Multi_get(x,y,z);
        break;
    case HYBRIDMULTI:
        val=HybridMulti_get(x,y,z);
        break;
    case DECARPENTIERSWISS:
        val=DeCarpentierSwiss_get(x,y,z);
        break;
    default:
        val=fBm_get(x,y,z);
        break;
    }
    //return clamp(val,-1.0,1.0);
    return val;
}

float CImplicitFractal::get(float x, float y, float z, float w)
{
    float val;
    switch(m_type)
    {
    case FBM:
        val=fBm_get(x,y,z,w);
        break;
    case RIDGEDMULTI:
        val=RidgedMulti_get(x,y,z,w);
        break;
    case BILLOW:
        val=Billow_get(x,y,z,w);
        break;
    case MULTI:
        val=Multi_get(x,y,z,w);
        break;
    case HYBRIDMULTI:
        val=HybridMulti_get(x,y,z,w);
        break;
    case DECARPENTIERSWISS:
        val=DeCarpentierSwiss_get(x,y,z,w);
        break;
    default:
        val=fBm_get(x,y,z,w);
        break;
    }
    return val;
}

float CImplicitFractal::get(float x, float y, float z, float w, float u, float v)
{
    float val;
    switch(m_type)
    {
    case FBM:
        val=fBm_get(x,y,z,w,u,v);
        break;
    case RIDGEDMULTI:
        val=RidgedMulti_get(x,y,z,w,u,v);
        break;
    case BILLOW:
        val=Billow_get(x,y,z,w,u,v);
        break;
    case MULTI:
        val=Multi_get(x,y,z,w,u,v);
        break;
    case HYBRIDMULTI:
        val=HybridMulti_get(x,y,z,w,u,v);
        break;
    case DECARPENTIERSWISS:
        val=DeCarpentierSwiss_get(x,y,z,w,u,v);
        break;
    default:
        val=fBm_get(x,y,z,w,u,v);
        break;
    }

    return val;
}

void CImplicitFractal::fBm_calcWeights()
{
    //std::cout << "Weights: ";
    for(int i=0; i<(int)MaxSources; ++i)
    {
        m_exparray[i]= pow(m_lacunarity, -i*m_H);
    }

    // Calculate scale/bias pairs by guessing at minimum and maximum values and remapping to [-1,1]
    float minvalue=0.0, maxvalue=0.0;
    for(int i=0; i<MaxSources; ++i)
    {
        minvalue += -1.0 * m_exparray[i];
        maxvalue += 1.0 * m_exparray[i];

        float A=-1.0, B=1.0;
        float scale = (B-A) / (maxvalue-minvalue);
        float bias = A - minvalue*scale;
        m_correct[i][0]=scale;
        m_correct[i][1]=bias;

        //std::cout << minvalue << " " << maxvalue << " " << scale << " " << bias << std::endl;
    }
}

void CImplicitFractal::RidgedMulti_calcWeights()
{
    for(int i=0; i<(int)MaxSources; ++i)
    {
        m_exparray[i]= pow(m_lacunarity, -i*m_H);
    }

    // Calculate scale/bias pairs by guessing at minimum and maximum values and remapping to [-1,1]
    float minvalue=0.0, maxvalue=0.0;
    for(int i=0; i<MaxSources; ++i)
    {
        minvalue += (m_offset-1.0)*(m_offset-1.0)*m_exparray[i];
        maxvalue += (m_offset)*(m_offset) * m_exparray[i];

        float A=-1.0, B=1.0;
        float scale = (B-A) / (maxvalue-minvalue);
        float bias = A - minvalue*scale;
        m_correct[i][0]=scale;
        m_correct[i][1]=bias;
    }

}

void CImplicitFractal::DeCarpentierSwiss_calcWeights()
{
    for(int i=0; i<(int)MaxSources; ++i)
    {
        m_exparray[i]= pow(m_lacunarity, -i*m_H);
    }

    // Calculate scale/bias pairs by guessing at minimum and maximum values and remapping to [-1,1]
    float minvalue=0.0, maxvalue=0.0;
    for(int i=0; i<MaxSources; ++i)
    {
        minvalue += (m_offset-1.0)*(m_offset-1.0)*m_exparray[i];
        maxvalue += (m_offset)*(m_offset) * m_exparray[i];

        float A=-1.0, B=1.0;
        float scale = (B-A) / (maxvalue-minvalue);
        float bias = A - minvalue*scale;
        m_correct[i][0]=scale;
        m_correct[i][1]=bias;
    }

}

void CImplicitFractal::Billow_calcWeights()
{
    for(int i=0; i<(int)MaxSources; ++i)
    {
        m_exparray[i]= pow(m_lacunarity, -i*m_H);
    }

    // Calculate scale/bias pairs by guessing at minimum and maximum values and remapping to [-1,1]
    float minvalue=0.0, maxvalue=0.0;
    for(int i=0; i<MaxSources; ++i)
    {
        minvalue += -1.0 * m_exparray[i];
        maxvalue += 1.0 * m_exparray[i];

        float A=-1.0, B=1.0;
        float scale = (B-A) / (maxvalue-minvalue);
        float bias = A - minvalue*scale;
        m_correct[i][0]=scale;
        m_correct[i][1]=bias;
    }

}

void CImplicitFractal::Multi_calcWeights()
{
    for(int i=0; i<(int)MaxSources; ++i)
    {
        m_exparray[i]= pow(m_lacunarity, -i*m_H);
    }

    // Calculate scale/bias pairs by guessing at minimum and maximum values and remapping to [-1,1]
    float minvalue=1.0, maxvalue=1.0;
    for(int i=0; i<MaxSources; ++i)
    {
        minvalue *= -1.0*m_exparray[i]+1.0;
        maxvalue *= 1.0*m_exparray[i]+1.0;

        float A=-1.0, B=1.0;
        float scale = (B-A) / (maxvalue-minvalue);
        float bias = A - minvalue*scale;
        m_correct[i][0]=scale;
        m_correct[i][1]=bias;
    }

}

void CImplicitFractal::HybridMulti_calcWeights()
{
    for(int i=0; i<(int)MaxSources; ++i)
    {
        m_exparray[i]= pow(m_lacunarity, -i*m_H);
    }

    // Calculate scale/bias pairs by guessing at minimum and maximum values and remapping to [-1,1]
    float minvalue=1.0, maxvalue=1.0;
    float weightmin, weightmax;
    float A=-1.0, B=1.0, scale, bias;

    minvalue = m_offset - 1.0;
    maxvalue = m_offset + 1.0;
    weightmin = m_gain*minvalue;
    weightmax = m_gain*maxvalue;

    scale = (B-A) / (maxvalue-minvalue);
    bias = A - minvalue*scale;
    m_correct[0][0]=scale;
    m_correct[0][1]=bias;


    for(int i=1; i<MaxSources; ++i)
    {
        if(weightmin>1.0) weightmin=1.0;
        if(weightmax>1.0) weightmax=1.0;

        float signal=(m_offset-1.0)*m_exparray[i];
        minvalue += signal*weightmin;
        weightmin *=m_gain*signal;

        signal=(m_offset+1.0)*m_exparray[i];
        maxvalue += signal*weightmax;
        weightmax *=m_gain*signal;


        scale = (B-A) / (maxvalue-minvalue);
        bias = A - minvalue*scale;
        m_correct[i][0]=scale;
        m_correct[i][1]=bias;
    }

}


float CImplicitFractal::fBm_get(float x, float y)
{
    float sum=0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y);
        sum+=n*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::fBm_get(float x, float y, float z)
{
    float sum=0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z);
        sum+=n*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::fBm_get(float x, float y, float z, float w)
{
    float sum=0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z,w);
        sum+=n*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::fBm_get(float x, float y, float z, float w, float u, float v)
{
    float sum=0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;
    u*=m_frequency;
    v*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z,w);
        sum+=n*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
        u*=m_lacunarity;
        v*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::Multi_get(float x, float y)
{
    float value=1.0;
    x*=m_frequency;
    y*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        value *= m_source[i]->get(x,y)*m_exparray[i]+1.0;
        x*=m_lacunarity;
        y*=m_lacunarity;

    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::Multi_get(float x, float y, float z, float w)
{
    float value=1.0;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        value *= m_source[i]->get(x,y,z,w)*m_exparray[i]+1.0;
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::Multi_get(float x, float y, float z)
{
    float value=1.0;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        value *= m_source[i]->get(x,y,z)*m_exparray[i]+1.0;
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}


float CImplicitFractal::Multi_get(float x, float y, float z, float w, float u, float v)
{
    float value=1.0;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;
    u*=m_frequency;
    v*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        value *= m_source[i]->get(x,y,z,w,u,v)*m_exparray[i]+1.0;
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
        u*=m_lacunarity;
        v*=m_lacunarity;
    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}


float CImplicitFractal::Billow_get(float x, float y)
{
    float sum=0.0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y);
        sum+=(2.0 * fabs(n)-1.0)*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::Billow_get(float x, float y, float z, float w)
{
    float sum=0.0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z,w);
        sum+=(2.0 * fabs(n)-1.0)*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::Billow_get(float x, float y, float z)
{
    float sum=0.0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z);
        sum+=(2.0 * fabs(n)-1.0)*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::Billow_get(float x, float y, float z, float w, float u, float v)
{
    float sum=0.0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;
    u*=m_frequency;
    v*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z,w,u,v);
        sum+=(2.0 * fabs(n)-1.0)*amp;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
        u*=m_lacunarity;
        v*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::RidgedMulti_get(float x, float y)
{
    float sum=0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y);
        n=1.0-fabs(n);
        sum+=amp*n;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
    }
    return sum;
    /*float result=0.0, signal;
    x*=m_frequency;
    y*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        signal=m_source[i]->get(x,y);
        signal=m_offset-fabs(signal);
        signal *= signal;
        result +=signal*m_exparray[i];

        x*=m_lacunarity;
        y*=m_lacunarity;

    }

    return result*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];*/
}

float CImplicitFractal::RidgedMulti_get(float x, float y, float z, float w)
{
    float result=0.0, signal;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        signal=m_source[i]->get(x,y,z,w);
        signal=m_offset-fabs(signal);
        signal *= signal;
        result +=signal*m_exparray[i];

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
    }

    return result*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::RidgedMulti_get(float x, float y, float z)
{
    float sum=0;
    float amp=1.0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x,y,z);
        n=1.0-fabs(n);
        sum+=amp*n;
        amp*=m_gain;

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::RidgedMulti_get(float x, float y, float z, float w, float u, float v)
{
    float result=0.0, signal;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;
    u*=m_frequency;
    v*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        signal=m_source[i]->get(x,y,z,w,u,v);
        signal=m_offset-fabs(signal);
        signal *= signal;
        result +=signal*m_exparray[i];

        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
        u*=m_lacunarity;
        v*=m_lacunarity;
    }

    return result*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::HybridMulti_get(float x, float y)
{
    float value, signal, weight;
    x*=m_frequency;
    y*=m_frequency;


    value = m_source[0]->get(x,y) + m_offset;
    weight = m_gain * value;
    x*=m_lacunarity;
    y*=m_lacunarity;

    for(unsigned int i=1; i<m_numoctaves; ++i)
    {
        if(weight>1.0) weight=1.0;
        signal = (m_source[i]->get(x,y)+m_offset)*m_exparray[i];
        value += weight*signal;
        weight *=m_gain * signal;
        x*=m_lacunarity;
        y*=m_lacunarity;

    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::HybridMulti_get(float x, float y, float z)
{
    float value, signal, weight;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;

    value = m_source[0]->get(x,y,z) + m_offset;
    weight = m_gain * value;
    x*=m_lacunarity;
    y*=m_lacunarity;
    z*=m_lacunarity;

    for(unsigned int i=1; i<m_numoctaves; ++i)
    {
        if(weight>1.0) weight=1.0;
        signal = (m_source[i]->get(x,y,z)+m_offset)*m_exparray[i];
        value += weight*signal;
        weight *=m_gain * signal;
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::HybridMulti_get(float x, float y, float z, float w)
{
    float value, signal, weight;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;

    value = m_source[0]->get(x,y,z,w) + m_offset;
    weight = m_gain * value;
    x*=m_lacunarity;
    y*=m_lacunarity;
    z*=m_lacunarity;
    w*=m_lacunarity;

    for(unsigned int i=1; i<m_numoctaves; ++i)
    {
        if(weight>1.0) weight=1.0;
        signal = (m_source[i]->get(x,y,z,w)+m_offset)*m_exparray[i];
        value += weight*signal;
        weight *=m_gain * signal;
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::HybridMulti_get(float x, float y, float z, float w, float u, float v)
{
    float value, signal, weight;
    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;
    u*=m_frequency;
    v*=m_frequency;

    value = m_source[0]->get(x,y,z,w,u,v) + m_offset;
    weight = m_gain * value;
    x*=m_lacunarity;
    y*=m_lacunarity;
    z*=m_lacunarity;
    w*=m_lacunarity;
    u*=m_lacunarity;
    v*=m_lacunarity;

    for(unsigned int i=1; i<m_numoctaves; ++i)
    {
        if(weight>1.0) weight=1.0;
        signal = (m_source[i]->get(x,y,z,w,u,v)+m_offset)*m_exparray[i];
        value += weight*signal;
        weight *=m_gain * signal;
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
        u*=m_lacunarity;
        v*=m_lacunarity;
    }

    return value*m_correct[m_numoctaves-1][0] + m_correct[m_numoctaves-1][1];
}

float CImplicitFractal::DeCarpentierSwiss_get(float x, float y)
{
    float sum=0;
    float amp=1.0;

    float dx_sum=0;
    float dy_sum=0;

    x*=m_frequency;
    y*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x+m_offset*dx_sum,y+m_offset*dy_sum);
        float dx=m_source[i]->get_dx(x+m_offset*dx_sum,y+m_offset*dy_sum);
        float dy=m_source[i]->get_dy(x+m_offset*dx_sum,y+m_offset*dy_sum);
        sum+=amp * (1.0-fabs(n));
        dx_sum+=amp*dx*-n;
        dy_sum+=amp*dy*-n;
        amp*=m_gain*clamp(sum,(float)0.0f,(float)1.0f);
        x*=m_lacunarity;
        y*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::DeCarpentierSwiss_get(float x, float y, float z, float w)
{
    float sum=0;
    float amp=1.0;

    float dx_sum=0;
    float dy_sum=0;
    float dz_sum=0;
    float dw_sum=0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum);
        float dx=m_source[i]->get_dx(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum);
        float dy=m_source[i]->get_dy(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum);
        float dz=m_source[i]->get_dz(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum);
        float dw=m_source[i]->get_dw(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum);
        sum+=amp * (1.0-fabs(n));
        dx_sum+=amp*dx*-n;
        dy_sum+=amp*dy*-n;
        dz_sum+=amp*dz*-n;
        dw_sum+=amp*dw*-n;
        amp*=m_gain*clamp(sum,0.f,1.f);
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::DeCarpentierSwiss_get(float x, float y, float z)
{
    float sum=0;
    float amp=1.0;

    float dx_sum=0;
    float dy_sum=0;
    float dz_sum=0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum);
        float dx=m_source[i]->get_dx(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum);
        float dy=m_source[i]->get_dy(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum);
        float dz=m_source[i]->get_dz(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum);
        sum+=amp * (1.0-fabs(n));
        dx_sum+=amp*dx*-n;
        dy_sum+=amp*dy*-n;
        dz_sum+=amp*dz*-n;
        amp*=m_gain*clamp(sum,0.f,1.f);
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
    }
    return sum;
}

float CImplicitFractal::DeCarpentierSwiss_get(float x, float y, float z, float w, float u, float v)
{
    float sum=0;
    float amp=1.0;

    float dx_sum=0;
    float dy_sum=0;
    float dz_sum=0;
    float dw_sum=0;
    float du_sum=0;
    float dv_sum=0;

    x*=m_frequency;
    y*=m_frequency;
    z*=m_frequency;
    w*=m_frequency;
    u*=m_frequency;
    v*=m_frequency;

    for(unsigned int i=0; i<m_numoctaves; ++i)
    {
        float n=m_source[i]->get(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum);
        float dx=m_source[i]->get_dx(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dx_sum, w+m_offset*dw_sum, u+m_offset*du_sum, v+m_offset*dv_sum);
        float dy=m_source[i]->get_dy(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum, u+m_offset*du_sum, v+m_offset*dv_sum);
        float dz=m_source[i]->get_dz(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum, u+m_offset*du_sum, v+m_offset*dv_sum);
        float dw=m_source[i]->get_dw(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum, u+m_offset*du_sum, v+m_offset*dv_sum);
        float du=m_source[i]->get_du(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum, u+m_offset*du_sum, v+m_offset*dv_sum);
        float dv=m_source[i]->get_dv(x+m_offset*dx_sum,y+m_offset*dy_sum,z+m_offset*dz_sum, w+m_offset*dw_sum, u+m_offset*du_sum, v+m_offset*dv_sum);
        sum+=amp * (1.0-fabs(n));
        dx_sum+=amp*dx*-n;
        dy_sum+=amp*dy*-n;
        dz_sum+=amp*dz*-n;
        dw_sum+=amp*dw*-n;
        du_sum+=amp*du*-n;
        dv_sum+=amp*dv*-n;
        amp*=m_gain*clamp(sum,0.f,1.f);
        x*=m_lacunarity;
        y*=m_lacunarity;
        z*=m_lacunarity;
        w*=m_lacunarity;
        u*=m_lacunarity;
        v*=m_lacunarity;
    }
    return sum;
}
