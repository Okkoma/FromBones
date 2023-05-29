#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>

#include "GameOptions.h"

#include "MapPool.h"


void MapPool::RegisterObject(Context* context)
{
    context->RegisterFactory<MapPool>();
}

MapPool::MapPool(Context* context)
    : Object(context)
{
    URHO3D_LOGINFOF("MapPool()");
}

MapPool::~MapPool()
{
    URHO3D_LOGINFOF("~MapPool() ...");

    maps_.Clear();

    URHO3D_LOGINFOF("~MapPool() ... OK !");
}


void MapPool::Resize(unsigned size)
{
    unsigned oldsize = GetSize();

    maps_.Reserve(size);
    freemaps_.Reserve(size);

//    URHO3D_LOGINFOF("MapPool() - Resize ... actualsize=%u", oldsize);
    if (oldsize < size)
    {
        for (unsigned i=oldsize; i<size; ++i)
        {
            maps_.Push(SharedPtr<Map>(new Map()));
            freemaps_.Push(maps_[i].Get());
        }
        URHO3D_LOGINFOF("MapPool() - Resize size=%u", freemaps_.Size());
    }

    // Force Map Resize for any change (width,height,numviews)
    for (unsigned i=0; i<size; ++i)
        maps_[i]->Resize();
}

bool MapPool::Free(Map* map, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(MapStore_RestoreMap);
#endif

    if (!freemaps_.Contains(map))
    {
        if (!map->Clear(timer))
            return false;

        URHO3D_LOGINFOF("MapPool() - Free point=%s ... OK !", map->GetMapPoint().ToString().CString());
        freemaps_.Push(map);
    }

    return true;
}

void MapPool::Clear()
{
    for (Vector<SharedPtr<Map> >::ConstIterator it=maps_.Begin(); it != maps_.End(); ++it)
    {
        URHO3D_LOGINFOF("MapPool : Clear Map %s", (*it)->GetMapPoint().ToString().CString());
        Free(it->Get());
    }
}

Map* MapPool::Get()
{
    if (!freemaps_.Empty())
    {
        Map* map = freemaps_.Back();
//        URHO3D_LOGINFOF("MapPool() - GetMap ... index=%u addr=%u", index, map);

        freemaps_.Pop();

        return map;
    }
    return 0;
}

void MapPool::Dump() const
{
    URHO3D_LOGINFOF("MapPool - Dump() : numFreeMaps = %u/%u", freemaps_.Size(), maps_.Size());
}

