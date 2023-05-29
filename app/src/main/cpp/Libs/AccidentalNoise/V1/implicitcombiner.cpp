#include "implicitcombiner.h"

namespace anl
{
CImplicitCombiner::CImplicitCombiner(unsigned int type)
{
    m_type=type;
    clearAllSources();
}

void CImplicitCombiner::setType(unsigned int type)
{
    m_type=type;
}

void CImplicitCombiner::clearAllSources()
{
    for(int c=0; c<MaxSources; ++c) m_sources[c]=0;
}

void CImplicitCombiner::setSource(CImplicitModuleBase *b, int which) // Modified For FromBones Project.
{
    if(which<0 || which>=MaxSources) return;
    m_sources[which]=b;
}

float CImplicitCombiner::get(float x, float y)
{
    switch(m_type)
    {
    case ADD:
        return Add_get(x,y);
        break;
    case SUB:
        return Sub_get(x,y);
        break;
    case MULT:
        return Mult_get(x,y);
        break;
    case MAX:
        return Max_get(x,y);
        break;
    case MIN:
        return Min_get(x,y);
        break;
    case AVG:
        return Avg_get(x,y);
        break;
    default:
        return 0.0;
        break;
    }
}

float CImplicitCombiner::get(float x, float y, float z)
{
    switch(m_type)
    {
    case ADD:
        return Add_get(x,y,z);
        break;
    case MULT:
        return Mult_get(x,y,z);
        break;
    case MAX:
        return Max_get(x,y,z);
        break;
    case MIN:
        return Min_get(x,y,z);
        break;
    case AVG:
        return Avg_get(x,y,z);
        break;
    default:
        return 0.0;
        break;
    }
}

float CImplicitCombiner::get(float x, float y, float z, float w)
{
    switch(m_type)
    {
    case ADD:
        return Add_get(x,y,z,w);
        break;
    case MULT:
        return Mult_get(x,y,z,w);
        break;
    case MAX:
        return Max_get(x,y,z,w);
        break;
    case MIN:
        return Min_get(x,y,z,w);
        break;
    case AVG:
        return Avg_get(x,y,z,w);
        break;
    default:
        return 0.0;
        break;
    }
}

float CImplicitCombiner::get(float x, float y, float z, float w, float u, float v)
{
    switch(m_type)
    {
    case ADD:
        return Add_get(x,y,z,w,u,v);
        break;
    case MULT:
        return Mult_get(x,y,z,w,u,v);
        break;
    case MAX:
        return Max_get(x,y,z,w,u,v);
        break;
    case MIN:
        return Min_get(x,y,z,w,u,v);
        break;
    case AVG:
        return Avg_get(x,y,z,w,u,v);
        break;
    default:
        return 0.0;
        break;
    }
}


float CImplicitCombiner::Add_get(float x, float y)
{
    float value=0.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value+=m_sources[c]->get(x,y);
    }
    return value;
}
float CImplicitCombiner::Add_get(float x, float y, float z)
{
    float value=0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value+=m_sources[c]->get(x,y,z);
    }
    return value;
}
float CImplicitCombiner::Add_get(float x, float y, float z, float w)
{
    float value=0.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value+=m_sources[c]->get(x,y,z,w);
    }
    return value;
}
float CImplicitCombiner::Add_get(float x, float y, float z, float w, float u, float v)
{
    float value=0.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value+=m_sources[c]->get(x,y,z,w,u,v);
    }
    return value;
}
float CImplicitCombiner::Sub_get(float x, float y)
{
    float value=m_sources[0]->get(x,y);
    for(int c=1; c<MaxSources; ++c)
    {
        if(m_sources[c]) value-=m_sources[c]->get(x,y);
    }
    return value;
}
float CImplicitCombiner::Mult_get(float x, float y)
{
    float value=1.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value*=m_sources[c]->get(x,y);
    }
    return value;
}
float CImplicitCombiner::Mult_get(float x, float y, float z)
{
    float value=1.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value*=m_sources[c]->get(x,y,z);
    }
    return value;
}
float CImplicitCombiner::Mult_get(float x, float y, float z, float w)
{
    float value=1.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value*=m_sources[c]->get(x,y,z,w);
    }
    return value;
}
float CImplicitCombiner::Mult_get(float x, float y, float z, float w, float u, float v)
{
    float value=1.0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c]) value*=m_sources[c]->get(x,y,z,w,u,v);
    }
    return value;
}

float CImplicitCombiner::Min_get(float x, float y)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float v=m_sources[d]->get(x,y);
            if(v<mn) mn=v;
        }
    }

    return mn;
}

float CImplicitCombiner::Min_get(float x, float y, float z)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y,z);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float v=m_sources[d]->get(x,y,z);
            if(v<mn) mn=v;
        }
    }

    return mn;
}

float CImplicitCombiner::Min_get(float x, float y, float z, float w)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y,z,w);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float v=m_sources[d]->get(x,y,z,w);
            if(v<mn) mn=v;
        }
    }

    return mn;
}

float CImplicitCombiner::Min_get(float x, float y, float z, float w, float u, float v)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y,z,w,u,v);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float val=m_sources[d]->get(x,y,z,w,u,v);
            if(val<mn) mn=val;
        }
    }

    return mn;
}

float CImplicitCombiner::Max_get(float x, float y)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float v=m_sources[d]->get(x,y);
            if(v>mn) mn=v;
        }
    }

    return mn;
}

float CImplicitCombiner::Max_get(float x, float y, float z)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y,z);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float v=m_sources[d]->get(x,y,z);
            if(v>mn) mn=v;
        }
    }

    return mn;
}

float CImplicitCombiner::Max_get(float x, float y, float z, float w)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y,z,w);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float v=m_sources[d]->get(x,y,z,w);
            if(v>mn) mn=v;
        }
    }

    return mn;
}

float CImplicitCombiner::Max_get(float x, float y, float z, float w, float u, float v)
{
    float mn;
    int c=0;
    while(c<MaxSources && !m_sources[c]) ++c;
    if(c==MaxSources) return 0.0;
    mn=m_sources[c]->get(x,y,z,w,u,v);

    for(int d=c; d<MaxSources; ++d)
    {
        if(m_sources[d])
        {
            float val=m_sources[d]->get(x,y,z,w,u,v);
            if(val>mn) mn=val;
        }
    }

    return mn;
}

float CImplicitCombiner::Avg_get(float x, float y)
{
    float count=0;
    float value=0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c])
        {
            value+=m_sources[c]->get(x,y);
            count+=1.0;
        }
    }
    if(count==0.0) return 0.0;
    return value/count;
}

float CImplicitCombiner::Avg_get(float x, float y, float z)
{
    float count=0;
    float value=0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c])
        {
            value+=m_sources[c]->get(x,y,z);
            count+=1.0;
        }
    }
    if(count==0.0) return 0.0;
    return value/count;
}

float CImplicitCombiner::Avg_get(float x, float y, float z, float w)
{
    float count=0;
    float value=0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c])
        {
            value+=m_sources[c]->get(x,y,z,w);
            count+=1.0;
        }
    }
    if(count==0.0) return 0.0;
    return value/count;
}

float CImplicitCombiner::Avg_get(float x, float y, float z, float w, float u, float v)
{
    float count=0;
    float value=0;
    for(int c=0; c<MaxSources; ++c)
    {
        if(m_sources[c])
        {
            value+=m_sources[c]->get(x,y,z,w,u,v);
            count+=1.0;
        }
    }
    if(count==0.0) return 0.0;
    return value/count;
}
};
