#ifndef IMPLICIT_CACHE_H
#define IMPLICIT_CACHE_H
#include "implicitmodulebase.h"


namespace anl
{
struct SCache
{
    float x,y,z,w,u,v;
    float val;
    bool valid;

    SCache() : valid(false) {}
};
class CImplicitCache : public CImplicitModuleBase
{
public:
    CImplicitCache();
    ~CImplicitCache();

    virtual void setSource(float b);	                        // Modified For FromBones Project.
    virtual void setSource(CImplicitModuleBase *b, int num=0); 	// Modified For FromBones Project.

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    SCache m_c2, m_c3, m_c4, m_c6;
};

class CImplicitCacheArray; // Added For FromBones Project.

struct CacheArray // Added For FromBones Project.
{
    CacheArray() : storage(0), valid(false) {}
    void allocate(CImplicitCacheArray* cache, unsigned size);
    void clear();
    void set(float value);
    float get();

    float* storage;
    int arraysize;
    int index_set;
    int index_get;
    bool valid;
};

class CImplicitCacheArray : public CImplicitModuleBase  // Added For FromBones Project.
{
public:

    static void startStaticCache(unsigned size);
    static void stopStaticCache();

    CImplicitCacheArray();
    ~CImplicitCacheArray();

    virtual void setSource(float b);
    virtual void setSource(CImplicitModuleBase *b, int num=0);

    float get(float x, float y);
    float get(float x, float y, float z);
    float get(float x, float y, float z, float w);
    float get(float x, float y, float z, float w, float u, float v);

protected:
    CScalarParameter m_source;
    CacheArray m_cache;
};
};

#endif
