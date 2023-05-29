#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include "implicitcache.h"

typedef Urho3D::HashMap<anl::CImplicitCacheArray*, Urho3D::PODVector<float> > AnlCacheStorage;   // Added For FromBones Project.
static AnlCacheStorage ANL_ImplicitCacheStorage;                                                 // Added For FromBones Project.

namespace anl
{

/// CImplicitCache

CImplicitCache::CImplicitCache() :m_source(0) {}
CImplicitCache::~CImplicitCache() {}

void CImplicitCache::setSource(CImplicitModuleBase *b, int num) // Modified For FromBones Project.
{
    m_source.set(b);
}

void CImplicitCache::setSource(float v)
{
    m_source.set(v);
}

float CImplicitCache::get(float x, float y)
{
    if(!m_c2.valid || m_c2.x!=x || m_c2.y!=y)
    {
        m_c2.x=x;
        m_c2.y=y;
        m_c2.valid=true;
        m_c2.val=m_source.get(x,y);
    }
    return m_c2.val;
}

float CImplicitCache::get(float x, float y, float z)
{
    if(!m_c3.valid || m_c3.x!=x || m_c3.y!=y || m_c3.z!=z)
    {
        m_c3.x=x;
        m_c3.y=y;
        m_c3.z=z;
        m_c3.valid=true;
        m_c3.val=m_source.get(x,y,z);
    }
    return m_c3.val;
}

float CImplicitCache::get(float x, float y, float z, float w)
{
    if(!m_c4.valid || m_c4.x!=x || m_c4.y!=y || m_c4.z!=z || m_c4.w!=w)
    {
        m_c4.x=x;
        m_c4.y=y;
        m_c4.z=z;
        m_c4.w=w;
        m_c4.valid=true;
        m_c4.val=m_source.get(x,y,z,w);
    }
    return m_c4.val;
}

float CImplicitCache::get(float x, float y, float z, float w, float u, float v)
{
    if(!m_c6.valid || m_c6.x!=x || m_c6.y!=y || m_c6.z!=z || m_c6.w!=w || m_c6.u!=u || m_c6.v!=v)
    {
        m_c6.x=x;
        m_c6.y=y;
        m_c6.z=z;
        m_c6.w=w;
        m_c6.u=u;
        m_c6.v=v;
        m_c6.valid=true;
        m_c6.val=m_source.get(x,y,z,w,u,v);
    }
    return m_c6.val;
}

/// All After That has been added for FromBones Project.
/// TODO : allow index_get, index_set to be specific at each module using the cachearray

/// CacheArray

void CacheArray::allocate(CImplicitCacheArray* cache, unsigned size)
{
    arraysize = size;

    if (size)
    {
        ANL_ImplicitCacheStorage[cache].Resize(size);
        storage = &(ANL_ImplicitCacheStorage[cache][0]);
    }
    else
    {
        ANL_ImplicitCacheStorage.Erase(cache);
        storage = 0;
    }

    clear();
}

void CacheArray::clear()
{
    index_get = -1;
    index_set = -1;
    valid = false;
}

void CacheArray::set(float value)
{
    if (!storage)
        return;

    if (index_set < arraysize-1)
    {
        index_set++;
        storage[index_set] = value;

        if (index_set == arraysize-1)
            valid = true;
    }
}

float CacheArray::get()
{
    index_get++;

    if (index_get >= arraysize)
        index_get = 0;

    return storage[index_get];
}

/// CImplicitCacheArray

void CImplicitCacheArray::startStaticCache(unsigned size)
{
    for (AnlCacheStorage::Iterator c=ANL_ImplicitCacheStorage.Begin(); c!=ANL_ImplicitCacheStorage.End(); ++c)
        c->first_->m_cache.allocate(c->first_, size);
}

void CImplicitCacheArray::stopStaticCache()
{
    ANL_ImplicitCacheStorage.Clear();
}

CImplicitCacheArray::CImplicitCacheArray() :m_source(0)
{
    // activate CacheArray with a small amount, wait for reentrance with startStaticCache
    ANL_ImplicitCacheStorage[this].Resize(1);
}

CImplicitCacheArray::~CImplicitCacheArray() {}

void CImplicitCacheArray::setSource(CImplicitModuleBase *b, int num)
{
    m_source.set(b);
}

void CImplicitCacheArray::setSource(float v)
{
    m_source.set(v);
}

float CImplicitCacheArray::get(float x, float y)
{
    if (m_cache.valid)
        return m_cache.get();

    float value = m_source.get(x,y);
    m_cache.set(value);
    return value;
}

float CImplicitCacheArray::get(float x, float y, float z)
{
    if (m_cache.valid)
        return m_cache.get();

    float value = m_source.get(x,y,z);
    m_cache.set(value);
    return value;
}

float CImplicitCacheArray::get(float x, float y, float z, float w)
{
    if (m_cache.valid)
        return m_cache.get();

    float value = m_source.get(x,y,z,w);
    m_cache.set(value);
    return value;
}

float CImplicitCacheArray::get(float x, float y, float z, float w, float u, float v)
{
    if (m_cache.valid)
        return m_cache.get();

    float value = m_source.get(x,y,z,w,u,v);
    m_cache.set(value);
    return value;
}

};
