#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/Engine/Engine.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/VectorBuffer.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/SceneResolver.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/TmxFile2D.h>

#include "Predefined.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"
#include "GameNetwork.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"

#include "MapPool.h"
#include "MapGenerator.h"
#include "MapColliderGenerator.h"
#include "MapCreator.h"
#include "MapWorld.h"
#include "ViewManager.h"
#include "ObjectMaped.h"

#include "MapStorage.h"

extern const char* mapStatusNames[];
extern const char* mapAsynStateNames[];

#define MAPSTORAGE_ONLYSERIALIZEATSTARTANDEND


/// MapSerializer

MapSerializer::MapSerializer(Context* context) :
    Object(context),
    run_(false)
{
//	workInfos_.Reserve(100);
}

MapSerializer::~MapSerializer()
{ }

void SerializerThread(const WorkItem* item, unsigned threadIndex)
{
    if (!static_cast<Object*>(item->aux_)->IsInstanceOf<SerializerWorkInfo>())
    {
        URHO3D_LOGERRORF("SerializerThread : thread=%u ... Not a SerializerWorkInfo !", threadIndex);
        return;
    }

    SerializerWorkInfo& info = *static_cast<SerializerWorkInfo*>(item->aux_);

//	URHO3D_LOGINFOF("SerializerThread : thread=%u ... Start Work mpoint=%s ... state=%s ...", threadIndex, info.mapdata_ ? info.mapdata_->mpoint_.ToString().CString() : ".", mapAsynStateNames[info.state_]);

    if (info.state_ == MAPASYNC_LOADQUEUED)
    {
        info.state_ = MAPASYNC_LOADING;

//		URHO3D_LOGINFOF("SerializerThread : thread=%u ... MAPASYNC_LOADING mapdata=%u mpoint=%s ... ", threadIndex, info.mapdata_, info.mapdata_ ? info.mapdata_->mpoint_.ToString().CString() : ".");
        MapData* mapdata = info.mapdata_;
        if (mapdata)
        {
            File file(GameContext::Get().context_, mapdata->mapfilename_, FILE_READ);
            info.state_ = mapdata->Load(file) ? MAPASYNC_LOADSUCCESS : MAPASYNC_LOADFAIL;
        }
        else
        {
            info.state_ = MAPASYNC_LOADFAIL;
        }
    }
    else if (info.state_ == MAPASYNC_SAVEQUEUED)
    {
        info.state_ = MAPASYNC_SAVING;

//		URHO3D_LOGINFOF("SerializerThread : thread=%u ... MAPASYNC_SAVING mapdata=%u mpoint=%s ... ", threadIndex, info.mapdata_, info.mapdata_ ? info.mapdata_->mpoint_.ToString().CString() : ".");

        MapData* mapdata = info.mapdata_;
        if (mapdata)
        {
            File file(GameContext::Get().context_, mapdata->mapfilename_, FILE_WRITE);
            info.state_ = mapdata->Save(file) ? MAPASYNC_SAVESUCCESS : MAPASYNC_SAVEFAIL;
        }
        else
        {
            info.state_ = MAPASYNC_SAVEFAIL;
        }
    }

    URHO3D_LOGINFOF("SerializerThread : thread=%u ... End Work mpoint=%s ... state=%s !", threadIndex, info.mapdata_ ? info.mapdata_->mpoint_.ToString().CString() : ".", mapAsynStateNames[info.state_]);
}

void MapSerializer::Start()
{
    // don't restart
    if (run_)
        return;

    // Clean before start
    List<SerializerWorkInfo>::Iterator it = workInfos_.Begin();
    while (it != workInfos_.End())
    {
        if (it->finished_)
            it = workInfos_.Erase(it);
        else
            it++;
    }

    run_ = true;
    SubscribeToEvent(GameContext::Get().gameWorkQueue_, E_WORKITEMCOMPLETED, URHO3D_HANDLER(MapSerializer, HandleWorkItemComplete));

    URHO3D_LOGINFOF("MapSerializer() - Start ...");

    GameContext::Get().gameWorkQueue_->Resume();
}

void MapSerializer::Stop()
{
    if (!run_)
        return;

    run_ = false;

    UnsubscribeFromEvent(GameContext::Get().gameWorkQueue_, E_WORKITEMCOMPLETED);

    URHO3D_LOGINFOF("MapSerializer() - Stop !");
}

bool MapSerializer::LoadMapData(MapData* mapdata, bool async)
{
    if (!mapdata)
        return false;

    if (mapdata->mapfilename_.Empty())
        mapdata->mapfilename_ = MapStorage::GetMapFileName(MapStorage::Get()->GetCurrentWorldName(), mapdata->mpoint_, ".dat");

    if (!GetSubsystem<FileSystem>()->FileExists(mapdata->mapfilename_))
        return false;

    bool success = false;

    if (async)
    {
        // Check if Already in Serialization
        if (mapdata->state_ >= MAPASYNC_LOADING)
        {
            URHO3D_LOGERRORF("MapSerializer() - LoadMapData : mapdata=%u mpoint=%s ... Can't Queue it : already in Serialization !",
                             mapdata, mapdata ? mapdata->mpoint_.ToString().CString() : ".");
        }
        else
        {
            if (mapdata->map_)
            {
                mapdata->savedmapstate_ = mapdata->map_->GetStatus();
                mapdata->map_->SetStatus(Loading_Map);
            }

            // Use WorkQueue
            WorkQueue* queue = GameContext::Get().gameWorkQueue_;

            queue->Pause();

            // Set WorkItem
            SerializerWorkInfo& workInfo = GetFreeWorkInfo();
            workInfo.finished_ = false;
            workInfo.mapdata_ = mapdata;
            workInfo.state_ = MAPASYNC_LOADQUEUED;

            // Add WorkItem
            SharedPtr<WorkItem> item = queue->GetFreeItem();
            item->sendEvent_ = true;
            item->priority_ = SERIALIZER_WORKITEM_PRIORITY;
            item->workFunction_ = SerializerThread;
            item->aux_ = &workInfo;
            queue->AddWorkItem(item);

            queue->Resume();

            URHO3D_LOGINFOF("MapSerializer() - LoadMapData mPoint=%s(map=%u) ... numInQueue=%u ", mapdata->mpoint_.ToString().CString(), mapdata->map_, queue->GetNumInQueue());

            Start();
        }
    }
    else
    {
        SharedPtr<File> file;

        if (GetSubsystem<FileSystem>()->FileExists(mapdata->mapfilename_))
            file = SharedPtr<File>(new File(context_, mapdata->mapfilename_, FILE_READ));

        if (file)
        {
            mapdata->state_ = MAPASYNC_LOADING;
            success = mapdata->Load(*file);
        }

        mapdata->state_ = success ? MAPASYNC_LOADSUCCESS : MAPASYNC_LOADFAIL;
//        mapdata->Dump();

        URHO3D_LOGINFOF("MapSerializer() - LoadMapData mPoint=%s(map=%u) file=%s ... seed=%u ... %s !", mapdata->mpoint_.ToString().CString(), mapdata->map_, mapdata->mapfilename_.CString(), mapdata->seed_, success ? "OK":"NOK");
    }

    return success;
}

bool MapSerializer::SaveMapData(MapData* mapdata, bool async)
{
    if (!mapdata)
        return false;

    if (mapdata->mapfilename_.Empty())
        mapdata->mapfilename_ = MapStorage::GetMapFileName(MapStorage::Get()->GetCurrentWorldName(), mapdata->mpoint_, ".dat");

    // don't save again if no linked map and already an existing mapfile : the datas in the existing mapfile has been created during UnLoadMapAt
    if (!mapdata->map_ && GetSubsystem<FileSystem>()->FileExists(mapdata->mapfilename_))
        return false;

    bool success = false;

    if (async)
    {
        // Check if Already in Serialization
        if (mapdata->state_ >= MAPASYNC_SAVING)
        {
            URHO3D_LOGERRORF("MapSerializer() - SaveMapData : mapdata=%u mpoint=%s ... Can't Queue it : already in Serialization !",
                             mapdata, mapdata ? mapdata->mpoint_.ToString().CString() : ".");
        }
        else
        {
            if (mapdata->map_)
                mapdata->savedmapstate_ = mapdata->map_->GetStatus();

            // Use WorkQueue
            WorkQueue* queue = GameContext::Get().gameWorkQueue_;

            queue->Pause();

            // Set WorkItem
            SerializerWorkInfo& workInfo = GetFreeWorkInfo();
            workInfo.finished_ = false;
            workInfo.mapdata_ = mapdata;
            workInfo.state_ = MAPASYNC_SAVEQUEUED;

            // Add WorkItem
            SharedPtr<WorkItem> item = queue->GetFreeItem();
            item->sendEvent_ = true;
            item->priority_ = SERIALIZER_WORKITEM_PRIORITY;
            item->workFunction_ = SerializerThread;
            item->aux_ = &workInfo;
            queue->AddWorkItem(item);

            queue->Resume();

            URHO3D_LOGINFOF("MapSerializer() - SaveMapData mPoint=%s(map=%u) ...", mapdata->mpoint_.ToString().CString(), mapdata->map_);

            Start();
        }
    }
    else
    {
        SharedPtr<File> file = SharedPtr<File>(new File(context_, mapdata->mapfilename_, FILE_WRITE));
        if (file)
        {
            mapdata->state_ = MAPASYNC_SAVING;
            success = mapdata->Save(*file);
        }

        mapdata->state_ = success ? MAPASYNC_SAVESUCCESS : MAPASYNC_SAVEFAIL;
//        mapdata->Dump();

        URHO3D_LOGINFOF("MapSerializer() - SaveMapData mPoint=%s(map=%u) ... %s !", mapdata->mpoint_.ToString().CString(), mapdata->map_, success ? "OK":"NOK");
    }

    return success;
}

void MapSerializer::HandleWorkItemComplete(StringHash eventType, VariantMap& eventData)
{
    WorkItem* item = static_cast<WorkItem*>(eventData[WorkItemCompleted::P_ITEM].GetPtr());

    if (!static_cast<Object*>(item->aux_)->IsInstanceOf<SerializerWorkInfo>())
        return;

    SerializerWorkInfo* info = static_cast<SerializerWorkInfo*>(item->aux_);

    if (info->mapdata_)
    {
        if (info->mapdata_->map_)
        {
            // Restore previous state before serializing
            info->mapdata_->map_->SetStatus(info->mapdata_->savedmapstate_);

            // if Loading, set to next state
            if (info->mapdata_->map_->GetStatus() == Loading_Map)
                info->mapdata_->map_->SetStatus(Generating_Map);
        }

        URHO3D_LOGINFOF("MapSerializer() - HandleWorkItemComplete ... %s state=%s mapstatus=%s !",
                        info->mapdata_->mpoint_.ToString().CString(), mapAsynStateNames[info->state_], info->mapdata_->map_ ? mapStatusNames[info->mapdata_->map_->GetStatus()] : "none");

        info->mapdata_->state_ = info->state_;
    }

    // Reset workinfo
    info->mapdata_ = 0;
    info->state_ = MAPASYNC_NONE;
    info->finished_ = true;

    // the WorkItems are not all finished => continue
    for (List<SerializerWorkInfo>::ConstIterator it = workInfos_.Begin(); it != workInfos_.End(); ++it)
    {
        if (!it->finished_)
            return;
    }

    //  All WorkItems are finished => stop
    URHO3D_LOGINFOF("MapSerializer() - HandleWorkItemComplete ... All WorkItems Finished !");

    Stop();
}

SerializerWorkInfo& MapSerializer::GetFreeWorkInfo()
{
    for (List<SerializerWorkInfo>::Iterator it = workInfos_.Begin(); it != workInfos_.End(); ++it)
    {
        if (it->finished_)
            return *it;
    }

    workInfos_.Resize(workInfos_.Size()+1);

    return workInfos_.Back();
}

unsigned MapSerializer::GetNumQueuedMaps() const
{
    return workInfos_.Size();
}

bool MapSerializer::IsInQueue(const ShortIntVector2& mpoint) const
{
    for (List<SerializerWorkInfo>::ConstIterator it = workInfos_.Begin(); it != workInfos_.End(); ++it)
    {
        if (it->mapdata_ && it->mapdata_->mpoint_ == mpoint && !it->finished_)
            return true;
    }

    return false;
}

bool MapSerializer::IsRunning() const
{
    return run_;
}


/// MapStorage

const String            MapStorage::storagePrefixDir = DATALEVELSDIR;
const String            MapStorage::storagePrefixFile = String("map");
String                  MapStorage::storageDir_;
Vector<String>          MapStorage::registeredWorldName_;
Vector<World2DInfo>     MapStorage::registeredWorld2DInfo_;
Vector<IntVector2>      MapStorage::registeredWorldPoint_;
WeakPtr<TerrainAtlas>   MapStorage::atlas_;
Vector<MapModel>        MapStorage::mapModels_;
MapStorage*             MapStorage::storage_ = 0;
MapCreator*             MapStorage::mapCreator_ = 0;
MapPool*                MapStorage::mapPool_ = 0;
MapSerializer*          MapStorage::mapSerializer_ = 0;
Vector<MapData>         MapStorage::mapDatasPool_;
HashMap<ShortIntVector2, MapData* > MapStorage::mapDatas_;


void MapStorage::InitTable(Context* context)
{
    URHO3D_LOGINFOF("MapStorage() - InitTable ...");

#ifdef LOCAL_STORAGE
    storageDir_ = context->GetSubsystem<FileSystem>()->GetCurrentDir() + storagePrefixDir;
#else
    storageDir_ = GameContext::Get().gameConfig_.saveDir_ + storagePrefixDir;
#endif

    GAME_SETGAMELOGENABLE(GAMELOG_MAPPRELOAD, false);

    // Register Worlds Path
    URHO3D_LOGINFOF("MapStorage() - InitTable ...Register Worlds ...");

//    RegisterWorldPath(context, "World_1", IntVector2(0, 0), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_world_1.xml");
//    RegisterWorldPath(context, "World_2", IntVector2(0, 1), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_world_2.xml");
//    RegisterWorldPath(context, "World_3", IntVector2(0, 2), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_world_3.xml");
    RegisterWorldPath(context, "ArenaZone", IntVector2(0, 3), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_arena.xml");
    RegisterWorldPath(context, "TestZone1", IntVector2(0, 4), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_world_1.xml", "anlworldVM-ellipsoid-zone1.xml");
    RegisterWorldPath(context, "TestZone2", IntVector2(0, 5), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_world_1.xml", "anlworldVM-ellipsoid-zone2.xml");
    RegisterWorldPath(context, "Custom", IntVector2(0, 6), WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT, "atlas_world_1.xml", "nodefault");

    URHO3D_LOGINFOF("MapStorage() - InitTable ... Tileset Water ...");

    String waterset = storagePrefixDir + "tilesetwater.xml";
    for (Vector<World2DInfo>::Iterator it = registeredWorld2DInfo_.Begin(); it != registeredWorld2DInfo_.End(); ++it)
        it->atlas_->RegisterTileSheet(waterset, false);

#ifndef USE_TILERENDERING
    if (GameContext::Get().gameConfig_.renderShapes_)
    {
        URHO3D_LOGINFOF("MapStorage() - InitTable ... Tileset Border ...");

#ifdef USE_RENDERSHAPE_WITH_TERRAINATLAS
        String borderset = storagePrefixDir + "tileset_borderA.xml";
#else
        String borderset = storagePrefixDir + "tileset_border.xml";
#endif
        for (Vector<World2DInfo>::Iterator it = registeredWorld2DInfo_.Begin(); it != registeredWorld2DInfo_.End(); ++it)
            it->atlas_->RegisterTileSheet(borderset, true);
    }
#endif

    GAME_SETGAMELOGENABLE(GAMELOG_MAPPRELOAD, true);

#ifdef DUMP_ATLASES
    // Dump Atlases
    for (Vector<World2DInfo>::Iterator it = registeredWorld2DInfo_.Begin(); it != registeredWorld2DInfo_.End(); ++it)
        it->atlas_->Dump();
#endif

    URHO3D_LOGINFOF("MapStorage() - InitTable ... Create Map Models ...");

    // Create Map Models
    mapModels_.Resize(MAPMODEL_NUM);
    {
        MapModel& model = mapModels_[MAPMODEL_DUNGEON];
        model.numviews_ = MAP_NUMMAXVIEWS;
        model.anlModel_ = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>("anlworldVM-ellipsoid-zone2.xml");
        model.hasPhysics_ = true;
        model.colliderBodyRotate_ = false;
        model.colliderBodyType_ = BT_DYNAMIC;
        model.colliderType_ = DUNGEONINNERTYPE;
        model.genType_ = GEN_DUNGEON;
    }
    {
        MapModel& model = mapModels_[MAPMODEL_CAVE];

    }
    {
        MapModel& model = mapModels_[MAPMODEL_ASTEROID];
        model.numviews_ = 1;
        model.anlModel_ = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(DATALEVELSDIR + "anlworldVM-asteroid1.xml");
        model.hasPhysics_ = true;
        model.colliderBodyRotate_ = true;
        model.colliderBodyType_ = BT_DYNAMIC;
        model.colliderType_ = ASTEROIDTYPE;
        model.genType_ = GEN_ASTEROID;

//        Vector<ColliderParams > physicsParameters_;
//        Vector<ColliderParams > rendersParameters_;
    }
    {
        MapModel& model = mapModels_[MAPMODEL_BACKASTEROID];
        model.numviews_ = 1;
        model.anlModel_ = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(DATALEVELSDIR + "anlworldVM-asteroid1.xml");
        model.hasPhysics_ = false;
        model.colliderBodyRotate_ = false;
        model.colliderBodyType_ = BT_STATIC;
        model.colliderType_ = ASTEROIDTYPE;
        model.genType_ = GEN_ASTEROID;
    }
    {
        MapModel& model = mapModels_[MAPMODEL_MOBILECASTLE];
        model.numviews_ = 3;
        model.anlModel_ = 0;
        model.hasPhysics_ = true;
        model.colliderBodyRotate_ = true;
        model.colliderBodyType_ = BT_DYNAMIC;
        model.colliderType_ = MOBILECASTLETYPE;
        model.genType_ = GEN_MOBILECASTLE;
    }

    World2DInfo::WATERMATERIAL_TILE 	= context->GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/watertiles.xml");
    World2DInfo::WATERMATERIAL_REFRACT  = context->GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/waterrefract.xml");
    World2DInfo::WATERMATERIAL_LINE     = context->GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/waterlines.xml");

    // TODO : Reserve the max maps in the world (cf worldmap)
    mapDatasPool_.Reserve(500);

    URHO3D_LOGINFOF("MapStorage() - InitTable ... OK !");
}

void MapStorage::DeInitTable(Context* context)
{
    URHO3D_LOGINFOF("MapStorage() - DeInitTable ...");

    if (mapCreator_)
    {
        delete mapCreator_;
        mapCreator_ = 0;
    }

    atlas_.Reset();

    for (unsigned i=0; i < registeredWorld2DInfo_.Size(); i++)
    {
        registeredWorld2DInfo_[i].atlas_.Reset();
        registeredWorld2DInfo_[i].worldModel_.Reset();
    }

    context->GetSubsystem<ResourceCache>()->ReleaseResources(TerrainAtlas::GetTypeStatic());
    context->GetSubsystem<ResourceCache>()->ReleaseResources(AnlWorldModel::GetTypeStatic());

    registeredWorldName_.Clear();
    registeredWorld2DInfo_.Clear();
    registeredWorldPoint_.Clear();

    World2DInfo::WATERMATERIAL_TILE.Reset();
    World2DInfo::WATERMATERIAL_REFRACT.Reset();
    World2DInfo::WATERMATERIAL_LINE.Reset();

    URHO3D_LOGINFOF("MapStorage() - DeInitTable ... OK !");
}



MapStorage::MapStorage(Context* context) :
    Object(context),
    forcedMapToUnload_(UNDEFINED_MAPPOINT),
    creatingMode_(MCM_ASYNCHRONOUS),
    delayUpdateUsec_(World2DInfo::delayUpdateUsec_)
{ }

MapStorage::MapStorage(Context* context, const IntVector2& wPoint) :
    Object(context),
    forcedMapToUnload_(UNDEFINED_MAPPOINT),
    sseed_(1),
    delayUpdateUsec_(World2DInfo::delayUpdateUsec_)
{
    if (!registeredWorldPoint_.Contains(wPoint))
    {
        URHO3D_LOGERRORF("MapStorage() - World Point %s Not Registered ! Stop !", wPoint.ToString().CString());
        return;
    }

    currentWorldIndex_ = registeredWorldPoint_.Find(wPoint) - registeredWorldPoint_.Begin();
    worldName_ = registeredWorldName_[currentWorldIndex_];
    currentWorld2DInfo_ = &registeredWorld2DInfo_[currentWorldIndex_];
    ObjectTiled::RegisterWorldInfo(currentWorld2DInfo_);

    width_ = currentWorld2DInfo_->mapWidth_;
    height_ = currentWorld2DInfo_->mapHeight_;
    wBounds_ = currentWorld2DInfo_->wBounds_;
    defaultAtlasSet_ = currentWorld2DInfo_->atlasSetFile_;
    atlas_ = currentWorld2DInfo_->atlas_;

    Clear();

//    storageDir_ = currentWorld2DInfo_->storageDir_;

    if (!storage_)
        storage_ = this;

    SetMapSeed(sseed_);

    if (!mapCreator_)
        mapCreator_ = new MapCreator(context);

    mapCreator_->SetWorldInfo(currentWorld2DInfo_);

    if (!mapPool_)
        mapPool_ = new MapPool(context);

    SetMapBufferOffset(1);

    if (!mapSerializer_)
    {
        mapSerializer_ = new MapSerializer(context);
    }

    URHO3D_LOGINFOF("MapStorage() - storageDir_ = %s", storageDir_.CString());
}

MapStorage::~MapStorage()
{
    URHO3D_LOGINFOF("~MapStorage() ... ");

    if (mapPool_)
    {
        delete mapPool_;
        mapPool_ = 0;
    }

    if (mapCreator_)
    {
        delete mapCreator_;
        mapCreator_ = 0;
    }

    if (mapSerializer_)
    {
        delete mapSerializer_;
        mapSerializer_ = 0;
    }

    if (storage_ == this)
        storage_ = 0;

    URHO3D_LOGINFOF("~MapStorage() ... OK !");
}

void MapStorage::Clear()
{
    URHO3D_LOGINFOF("MapStorage() - Clear() ...");

    creatingMode_ = MCM_ASYNCHRONOUS;

    mapsInMemory_.Clear();
    mapsToLoadInMemory_.Clear();
    mapsToUnloadFromMemory_.Clear();

//    savedEntities_.Clear();
//    savedTiles_.Clear();
//    seeds_.Clear();
    mapDatas_.Clear();
    mapDatasPool_.Clear();

    if (mapPool_)
        mapPool_->Clear();

    URHO3D_LOGINFOF("MapStorage() - Clear() ... OK !");
}

void MapStorage::SetMapSeed(unsigned seed)
{
    if (!seed)
        seed = GameRand::GetTimeSeed();

    GameRand::SetSeedRand(ALLRAND, seed);

    sseed_ = seed;

    URHO3D_LOGINFOF("MapStorage() - SetMapSeed() ... Set Seed=%u !", sseed_);
}

void MapStorage::SetBufferDirty(bool dirty)
{
    bufferedAreaDirty_ = dirty;
}

/// num offset maps around centerMapPoint
void MapStorage::SetMapBufferOffset(int offset)
{
    URHO3D_LOGINFOF("MapStorage() - SetMapBufferOffset : offset = %d ...", offset);

    bOffset_ = offset;
    /*
        offset = 2
        z = buffer area perimeter => zsize = 1 + 2*offset = 5
        zzzzz
        zyyyz
        zyxyz
        zyyyz
        zzzzz

        num maps = squared(1 + 2*offset)
    */
    maxMapsInMemory_ = 1 + 2*bOffset_;
    maxMapsInMemory_ *= maxMapsInMemory_;

    if (mapPool_)
        mapPool_->Resize(GameContext::Get().ServerMode_ ? maxMapsInMemory_ * GameContext::MAX_NUMNETPLAYERS : maxMapsInMemory_ * MAX_VIEWPORTS);

#ifndef USE_LOADINGLISTS
    mapsToLoadInMemory_.Reserve(maxMapsInMemory_ * MAX_VIEWPORTS);
    mapsToUnloadFromMemory_.Reserve(maxMapsInMemory_ * MAX_VIEWPORTS);
#endif

    SetBufferDirty(true);

    URHO3D_LOGINFOF("MapStorage() - SetMapBufferOffset : offset = %d numMaxMapsInMemory = %d ... OK !", bOffset_, maxMapsInMemory_);
}

// Set the World Node
void MapStorage::SetNode(Node* node)
{
    URHO3D_LOGINFOF("MapStorage() - SetNode : node = %s(%u)", node->GetName().CString(), node->GetID());

    node_ = node;
}




const IntVector2& MapStorage::CheckWorld2DPoint(Context* context, const IntVector2& worldPoint, World2DInfo* info)
{
    if (info)
    {
        for (unsigned i=0; i<registeredWorld2DInfo_.Size(); ++i)
        {
            if (&registeredWorld2DInfo_[i] == info)
                return registeredWorldPoint_[i];
        }

        URHO3D_LOGWARNINGF("MapStorage() - WorldInfo not found in static : create a new one with theses infos in worldPoint=%s!", worldPoint.ToString().CString());

        unsigned index = RegisterWorldPath(context, "World", worldPoint, 0, 0, String::EMPTY, String::EMPTY, info);
        return registeredWorldPoint_[index];
    }
    else
    {
        URHO3D_LOGWARNINGF("MapStorage() - No WorldInfo At Entry : create a standard new one !");
        unsigned index = RegisterWorldPath(context, "World", worldPoint);
        return worldPoint;
    }
}

unsigned MapStorage::RegisterWorldPath(Context* context, const String& shortPathName, const IntVector2& worldPoint, float tilewidth, float tileheight,
                                       const String& atlasSet, const String& worldmodelfile, World2DInfo* pinfo)
{
    unsigned index = 0;

//    if (!registeredWorldPoint_.Contains(worldPoint))
    {
        FileSystem* fs = context->GetSubsystem<FileSystem>();
        if (!fs)
        {
            URHO3D_LOGERROR("MapStorage() - RegisterWorldPath : No FileSystem Found");
            return index;
        }

//#ifdef LOCAL_STORAGE
//        String storageDir = context->GetSubsystem<FileSystem>()->GetCurrentDir() + storagePrefixDir + shortPathName;
//#else
//        String storageDir = GameContext::Get().saveDir_ + storagePrefixDir + shortPathName;
//#endif
        String storageDir = storageDir_ + shortPathName;

        if (!fs->DirExists(storageDir))
            fs->CreateDir(storageDir);

        URHO3D_LOGINFOF("MapStorage() - -------------------------------------------------------------------");
        URHO3D_LOGINFOF("MapStorage() - RegisterWorldPath %s at worldPoint=%s atlas=%s ...",
                        shortPathName.CString(), worldPoint.ToString().CString(), atlasSet.CString());

        {
            if (pinfo)
            {
                tilewidth = pinfo->tileWidth_;
                tileheight = pinfo->tileHeight_;
            }

            World2DInfo info(storageDir, tilewidth, tileheight, atlasSet, worldmodelfile);

            if (!pinfo)
                pinfo = &info;
            else
                pinfo->storageDir_ = storageDir;

            if (pinfo->atlas_ == 0)
            {
                TerrainAtlas* atlas = 0;
                if (!atlasSet.Empty())
                {
                    URHO3D_LOGINFO("MapStorage() - RegisterWorldPath : Get TerrainAtlas Set " + World2DInfo::ATLASSETDIR + atlasSet);
                    atlas = context->GetSubsystem<ResourceCache>()->GetResource<TerrainAtlas>(World2DInfo::ATLASSETDIR + atlasSet);
                    pinfo->atlasSetFile_ = atlasSet;
                }
                if (!atlas)
                {
                    URHO3D_LOGINFO("MapStorage() - RegisterWorldPath : Apply Default TerrainAtlas Set " + World2DInfo::ATLASSETDIR + World2DInfo::ATLASSETDEFAULT);
                    atlas = context->GetSubsystem<ResourceCache>()->GetResource<TerrainAtlas>(World2DInfo::ATLASSETDIR + World2DInfo::ATLASSETDEFAULT);
                    pinfo->atlasSetFile_ = World2DInfo::ATLASSETDEFAULT;
                }

                if (atlas)
                    pinfo->atlas_ = atlas;
                else
                    URHO3D_LOGWARNINGF("MapStorage() - RegisterWorldPath : No Default Atlas Set to Apply !!!");
            }

            if (pinfo->worldModel_ == 0)
            {
                AnlWorldModel* worldModel = 0;
                if (worldmodelfile != "nodefault")
                {
                    if (!worldmodelfile.Empty())
                    {
                        URHO3D_LOGINFO("MapStorage() - RegisterWorldPath : Get AnlWorldModel " + World2DInfo::ATLASSETDIR + worldmodelfile);
                        worldModel = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(World2DInfo::ATLASSETDIR + worldmodelfile);
                        pinfo->worldModelFile_ = worldmodelfile;
                    }
                    if (!worldmodelfile.Empty() && !worldModel)
                    {
                        URHO3D_LOGINFO("MapStorage() - RegisterWorldPath : Apply Default AnlWorldModel " + World2DInfo::ATLASSETDIR + World2DInfo::WORLDMODELDEFAULT);
                        worldModel = context->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(World2DInfo::ATLASSETDIR + World2DInfo::WORLDMODELDEFAULT);
                        pinfo->worldModelFile_ = World2DInfo::WORLDMODELDEFAULT;
                    }
                }

                if (worldModel)
                    pinfo->worldModel_ = worldModel;
                else
                    URHO3D_LOGWARNINGF("MapStorage() - RegisterWorldPath : No Default AnlWorldModel to Apply !!!");
            }

            if (!registeredWorldPoint_.Contains(worldPoint))
            {
                index = registeredWorldPoint_.Size();
//                URHO3D_LOGINFOF("MapStorage() - RegisterWorldPath : Register WorldInfo in New WorldPoint index=%d ...", index);
                if (index >= registeredWorld2DInfo_.Size())
                    registeredWorld2DInfo_.Resize(index+1);
                if (index >= registeredWorldPoint_.Size())
                    registeredWorldPoint_.Resize(index+1);
                if (index >= registeredWorldName_.Size())
                    registeredWorldName_.Resize(index+1);
            }
            else
            {
                index = registeredWorldPoint_.Find(worldPoint) - registeredWorldPoint_.Begin();
            }
//                URHO3D_LOGINFOF("MapStorage() - RegisterWorldPath : Register WorldInfo in Existing WorldPoint index=%d ...", index);
            registeredWorld2DInfo_[index] = *pinfo;
            registeredWorld2DInfo_[index].atlas_->SetWorldInfo(&registeredWorld2DInfo_[index]);
            registeredWorldPoint_[index] = worldPoint;
            registeredWorldName_[index] = shortPathName;
        }

        URHO3D_LOGINFOF("MapStorage() - RegisterWorldPath %s at %s index=%u info_=%u ... OK !",
                        storageDir. CString(), worldPoint.ToString().CString(), index, &(registeredWorld2DInfo_[index]));
        URHO3D_LOGINFOF("MapStorage() - -------------------------------------------------------------------");
    }

    return index;
}

int MapStorage::GetWorldIndex(const IntVector2& worldPoint)
{
    Vector<IntVector2>::ConstIterator it = registeredWorldPoint_.Find(worldPoint);
    return it != registeredWorldPoint_.End() ? it-registeredWorldPoint_.Begin() : -1;
}

Map* MapStorage::GetMapAt(const ShortIntVector2& mPoint) const
{
    HashMap<ShortIntVector2, Map* >::ConstIterator it = mapsInMemory_.Find(mPoint);
    return it != mapsInMemory_.End() ? it->second_ : 0;
}

Map* MapStorage::GetAvailableMapAt(const ShortIntVector2& mPoint) const
{
    HashMap<ShortIntVector2, Map* >::ConstIterator it = mapsInMemory_.Find(mPoint);
    return it != mapsInMemory_.End() ? (it->second_->IsAvailable() ? it->second_ : 0) : 0;
}

Map* MapStorage::InitializeMap(const ShortIntVector2& mPoint, HiresTimer* timer)
{
//    URHO3D_LOGINFOF("MapStorage() - InitializeMap : mPoint=%s ...", mPoint.ToString().CString());
    Map* map = 0;

    // Find Map in memory
    HashMap<ShortIntVector2, Map* >::ConstIterator it = mapsInMemory_.Find(mPoint);
    if (it != mapsInMemory_.End())
        map = it->second_;
    else
        map = mapPool_->Get();

    if (!map)
    {
//        URHO3D_LOGERRORF("MapStorage() - InitializeMap : No Allocation for %s", mPoint.ToString().CString());
        return 0;
    }

    // Check if mapdata is being serialized
    if (mapSerializer_->GetNumQueuedMaps() > 0)
    {
        if (mapSerializer_->IsInQueue(mPoint))
        {
            URHO3D_LOGERRORF("MapStorage() - InitializeMap mPoint=%s(map=%u) ... Is Serializing the MapData ... Wait 1 !", mPoint.ToString().CString(), map);
            return 0;
        }
    }

    if (map->GetStatus() == Uninitialized)
    {
        map->Initialize(mPoint, sseed_);
        // set rect bounds here for world draw debug
        map->GetBounds() = Map::GetMapRect(mPoint);
        mapsInMemory_[mPoint] = map;

        URHO3D_LOGINFOF("MapStorage() - InitializeMap : mPoint=%s map=%s ptr=%u state=%s ... OK !",
                        mPoint.ToString().CString(), map->GetMapPoint().ToString().CString(), map, mapStatusNames[map->GetStatus()]);
    }

    if (map->GetStatus() == Initializing)
    {
//        URHO3D_LOGINFOF("MapStorage() - InitializeMap : mPoint=%s map=%u ... status=%s ...", mPoint.ToString().CString(), map, mapStatusNames[map->GetStatus()]);

#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapStore_Create);
#endif

        // Get or Create the mapdata for the point
        MapData*& mapdata = mapDatas_[mPoint];
        if (!mapdata)
        {
            if (mapDatasPool_.Capacity() <= mapDatasPool_.Size())
                URHO3D_LOGERRORF("MapDatasPool Size Over defined Capacity !");
            mapDatasPool_.Resize(mapDatasPool_.Size()+1);
            mapdata = &mapDatasPool_.Back();
        }

        // Associate map with mapdata
        mapdata->mpoint_ = mPoint;
        mapdata->width_ = map->GetWidth();
        mapdata->height_ = map->GetHeight();
        mapdata->seed_ = map->GetMapSeed();
        mapdata->SetMap(map);
        map->SetMapData(mapdata);

        // be sure the mapdata state is well reinitialized
        mapdata->state_ = MAPASYNC_NONE;

        // the mapdata is not in serializing state
//        if (mapdata->state_ == MAPASYNC_NONE)
        {
            // Set Status to Generating_Map
            // if the map will be loaded, the next state of the map will be "Generating_Map"
            map->SetStatus(Generating_Map);

            if (GameContext::Get().ClientMode_)
            {
//                if (!mapdata->IsLoaded())
                {
                    mapdata->savedmapstate_ = Generating_Map;
                    mapdata->state_ = MAPASYNC_LOADING;
                    map->SetStatus(Loading_Map);
                    URHO3D_LOGERRORF("MapStorage() - InitializeMap mPoint=%s(map=%u) ... ClientMode ... put map in loading state ... Wait !", mPoint.ToString().CString(), map);

                    // Add Map Request
                    VariantMap& eventdata = GameNetwork::Get()->GetClientEventData();
                    eventdata.Clear();
                    eventdata[Net_ObjectCommand::P_TILEMAP] = mPoint.ToHash();
                    GameNetwork::Get()->PushObjectCommand(REQUESTMAP, &eventdata, false, GameNetwork::Get()->GetClientID());
                }
            }
            else
            {
            #ifdef MAPSTORAGE_ONLYSERIALIZEATSTARTANDEND
                if (!mapdata->IsLoaded() || (GameContext::Get().allMapsPrefab_ && !mapdata->IsSectionSet(MAPDATASECTION_LAYER)))
            #else
                if (!mapdata->IsSectionSet(MAPDATASECTION_LAYER))
            #endif
                {
                    // Load the map data if a map file exists and the mapdata is new
                    // Change to Loading_Map status
                    bool success = mapSerializer_->LoadMapData(mapdata);
                }
            }
        }
//        // the mapdata is currently in a serializing state
//        else if (mapdata->savedmapstate_ != Generating_Map)
//        {
//            // After serializing, force the next state to be "Generating_Map"
//            mapdata->savedmapstate_ = Generating_Map;
//
//            URHO3D_LOGERRORF("MapStorage() - InitializeMap mPoint=%s(map=%u) ... Is Serializing MapData ... Wait 2 !", mPoint.ToString().CString(), map);
//        }
    }

    mapCreator_->AddMapToCreate(mPoint);

    return map;
}

void MapStorage::ForceMapToUnload(Map* map)
{
    forcedMapToUnload_ = map->GetMapPoint();

    if (!mapsToUnloadFromMemory_.Contains(forcedMapToUnload_))
    {
        URHO3D_LOGERRORF("MapStorage() - ForceMapToUnload %s map=%u !", forcedMapToUnload_.ToString().CString(), map);
        mapsToUnloadFromMemory_.Push(forcedMapToUnload_);
    }
}

bool MapStorage::UnloadMapAt(const ShortIntVector2& mPoint, HiresTimer* timer)
{
    URHO3D_LOGINFOF("MapStorage() - UnloadMapAt %s ...", mPoint.ToString().CString());

    HashMap<ShortIntVector2, Map*>::Iterator it = mapsInMemory_.Find(mPoint);
    if (it == mapsInMemory_.End())
    {
        URHO3D_LOGERRORF("MapStorage() - UnloadMapAt %s ... no map found in mapsInMemory_ ... NOK !", mPoint.ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);
        return true;
    }

    Map* map = it->second_;
    if (!map)
    {
        URHO3D_LOGERRORF("MapStorage() - UnloadMapAt %s ... map=0 ... NOK !", mPoint.ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mapCreator_->PurgeMap(mPoint);
        return true;
    }

    int& mcount = map->GetMapCounter(MAP_GENERAL);
    if (mcount > 2)
        mcount = 0;

    URHO3D_LOGINFOF("MapStorage() - UnloadMapAt %s ... map=%u mcount=%d status=%s ...", mPoint.ToString().CString(), map, mcount, mapStatusNames[map->GetStatus()]);

    if (map->IsVisible())
        map->HideMap(timer);

    if (map->GetStatus() < Unloading_Map)
    {
        // normally don't use this code : it's already made in UpdateBufferedArea()
        // log this
        URHO3D_LOGERRORF("MapStorage() - UnloadMapAt %s was not in Unloading state ...", mPoint.ToString().CString());

        mapCreator_->PurgeMap(mPoint);

        map->SetStatus(Unloading_Map);
        mcount = 0;
    }

    if (map->GetStatus() > Unloading_Map)
    {
        // in saving state waiting ...
        return false;
    }

    if (mcount == 0)
    {
        if (!map->UpdateVisibility(timer))
            return false;

        map->GetMapCounter(MAP_FUNC1) = 0;

        mcount = map->IsSerializable() ? 1 : 2;
    }

    if (mcount == 1)
    {
        // save mapdata in memory
        if (!map->UpdateMapData(timer))
            return false;

        URHO3D_LOGINFOF("MapStorage() - UnloadMapAt %s ... Saving MapData ... status=%s timer=%d/%d msec !", mPoint.ToString().CString(),
                        map ? mapStatusNames[map->GetStatus()]:"", timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

#ifndef MAPSTORAGE_ONLYSERIALIZEATSTARTANDEND
        // save mapdata in file
        // mapserializer (async saving) restores the map status to Unloading_Map when it has finished
        if (!mapSerializer_->SaveMapData(map->GetMapData(), true))
            URHO3D_LOGERRORF("MapStorage() - UnloadMapAt %s ... Saving MapData ... status=%s can't save the map !", mPoint.ToString().CString(), map ? mapStatusNames[map->GetStatus()] : "");
#endif

        map->GetMapCounter(MAP_FUNC1) = 0;
        mcount++;
    }

    if (mcount == 2)
    {
        if (!mapPool_->Free(map, timer))
            return false;

        mapsInMemory_.Erase(it);

        if (forcedMapToUnload_ == mPoint)
            forcedMapToUnload_ = UNDEFINED_MAPPOINT;

        map->SetStatus(Uninitialized);

        GAME_SETGAMELOGENABLE(GAMELOG_MAPUNLOAD, true);
        URHO3D_LOGINFOF("MapStorage() - UnloadMapAt map=%s(%s) ... Free Pool OK ... timer=%d/%d msec !", mPoint.ToString().CString(), mapStatusNames[Uninitialized], timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);
        GAME_SETGAMELOGENABLE(GAMELOG_MAPUNLOAD, false);
    }

    return true;
}


/// TODO : asynchronous
bool MapStorage::LoadMaps(bool loadEntities, bool async)
{
    URHO3D_LOGINFOF("MapStorage() - LoadMaps ...");

    /// Load World File

    Vector<ShortIntVector2> mappoints;
    File file(context_, GetWorldFileName(currentWorldIndex_), FILE_READ);
    if (file.IsOpen())
    {
        unsigned nummaps = file.ReadVLE();

        mappoints.Resize(nummaps);

        URHO3D_LOGINFOF("MapStorage() - LoadMaps ... worldfilename = %s (index=%u) ... nummaps=%u ...", file.GetName().CString(), currentWorldIndex_, nummaps);

        for (unsigned i=0; i < nummaps; i++)
        {
            IntVector2 coord = file.ReadIntVector2();
            ShortIntVector2& mappoint = mappoints[i];
            mappoint.x_ = coord.x_;
            mappoint.y_ = coord.y_;
        }

        URHO3D_LOGINFOF("MapStorage() - LoadMaps ... worldfilename = %s (index=%u) ... OK !", file.GetName().CString(), currentWorldIndex_);

        file.Close();
    }
    else
    {
        return false;
    }

    if (!mappoints.Size())
        return false;

    if (async && mapSerializer_->IsRunning())
        return false;

    /// Clear the MapDatas
    mapDatas_.Clear();
    mapDatasPool_.Clear();

    /// Allocate MapDatas
    mapDatasPool_.Resize(mappoints.Size());
    for (unsigned i=0; i < mappoints.Size(); i++)
    {
        const ShortIntVector2& mpoint = mappoints[i];

        MapData*& mapdata = mapDatas_[mpoint];
        mapdata = &mapDatasPool_[i];
        mapdata->mpoint_ = mappoints[i];

        URHO3D_LOGINFOF("MapStorage() - LoadMaps ... allocate mapdata=%u mappoint=%s", mapdata, mpoint.ToString().CString());
    }

    /// Load Map Datas
    for (unsigned i=0; i < mappoints.Size(); i++)
    {
        bool success = mapSerializer_->LoadMapData(mapDatas_[mappoints[i]], async);
    }

    if (async)
    {
        /// TODO : subscribe to an handle finish serializing

        /// Wait for the thread finishes
        while (mapSerializer_->IsRunning())
            Time::Sleep(5);
    }

    URHO3D_LOGINFOF("MapStorage() - LoadMaps ... OK !");

    return true;
}

/// TODO : asynchronous
bool MapStorage::SaveMaps(bool saveEntities, bool async)
{
    URHO3D_LOGINFOF("MapStorage() - SaveMaps ...");

    /// Save World File
    File file(context_, GetWorldFileName(currentWorldIndex_), FILE_WRITE);
    if (file.IsOpen())
    {
        file.WriteVLE(mapDatas_.Size());

        IntVector2 mPoint;
        for (HashMap<ShortIntVector2, MapData* >::ConstIterator it=mapDatas_.Begin(); it!=mapDatas_.End(); ++it)
        {
            mPoint.x_ = it->first_.x_;
            mPoint.y_ = it->first_.y_;
            file.WriteIntVector2(mPoint);
        }

        URHO3D_LOGINFOF("MapStorage() - SaveMaps ... WorldFile = %s (index=%u)", file.GetName().CString(), currentWorldIndex_);

        file.Close();
    }

    /// Update Map Datas for Maps in Memory
    for (HashMap<ShortIntVector2, Map* >::Iterator it=mapsInMemory_.Begin(); it!=mapsInMemory_.End(); ++it)
    {
        Map* map = it->second_;
        if (map && map->IsSerializable())
            map->UpdateMapData(0);
    }

    /// Save Map Datas
    for (HashMap<ShortIntVector2, MapData* >::Iterator it=mapDatas_.Begin(); it!=mapDatas_.End(); ++it)
    {
        MapData*& mapdata = it->second_;
        if (mapdata->map_ && !mapdata->map_->IsSerializable())
            continue;

        mapSerializer_->SaveMapData(mapdata, async);
    }

    if (async)
    {
        /// TODO : subscribe to an handle finish serializing

        /// Wait for the thread finishes
        while (mapSerializer_->IsRunning())
            Time::Sleep(5);
    }

    URHO3D_LOGINFOF("MapStorage() - SaveMaps ... OK !");

    return true;
}

MapData* MapStorage::GetMapDataAt(const ShortIntVector2& mpoint, bool createIfMissing)
{
    MapData*& mapdata = mapDatas_[mpoint];
    if (!mapdata && createIfMissing)
    {
        if (mapDatasPool_.Capacity() <= mapDatasPool_.Size())
            URHO3D_LOGERRORF("MapStorage() - GetMapDataAt : MapDatasPool Size Over defined Capacity !");
        mapDatasPool_.Resize(mapDatasPool_.Size()+1);
        mapdata = &mapDatasPool_.Back();
    }

    return mapdata;
}

const IntVector2& MapStorage::GetWorldPoint(int worldindex)
{
    return registeredWorldPoint_[worldindex];
}

const String& MapStorage::GetWorldName(const IntVector2& worldPoint)
{
    int index = MapStorage::GetWorldIndex(worldPoint);
    return index != -1 ? registeredWorldName_[index] : String::EMPTY;
}


/// for saved file
String MapStorage::GetWorldDirName(const String& worldName)
{
    return storageDir_ + worldName;
}

String MapStorage::GetMapFileName(const String& worldName, const ShortIntVector2& mPoint, const char* ext)
{
//    return storagePrefixDir + worldName + "/" + storagePrefixFile + "_" + String(mPoint.x_) + "_" + String(mPoint.y_) + ext;
    return storageDir_ + worldName + "/" + storagePrefixFile + "_" + String(mPoint.x_) + "_" + String(mPoint.y_) + ext;
}

String MapStorage::GetWorldFileName(int worldIndex)
{
//    return storagePrefixDir + GetWorldName(worldIndex) + ".wld";
    return storageDir_ + GetWorldName(worldIndex) + ".wld";
}

/// for saved file
bool MapStorage::MapFileExist(const String& worldName, const ShortIntVector2& mPoint, const char* ext)
{
    return storage_->GetContext()->GetSubsystem<FileSystem>()->FileExists(GetMapFileName(worldName, mPoint, ext));
}

/// for resource file
SharedPtr<TmxFile2D> MapStorage::GetTmxFile(const String& worldName, const ShortIntVector2& mPoint)
{
    return storage_->GetContext()->GetSubsystem<ResourceCache>()->GetTempResource<TmxFile2D>(storagePrefixDir + worldName + "/" + storagePrefixFile + "_" + String(mPoint.x_) + "_" + String(mPoint.y_) + ".tmx");
}

/// delete world maps by scan the world directory
void MapStorage::DeleteWorldFiles(const IntVector2& wPoint, const char* ext)
{
    FileSystem* fs = GameContext::Get().context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        URHO3D_LOGERROR("MapStorage() - DeleteWorldFiles : FileSystem Not Found !");
        return;
    }

    const String& worldName = GetWorldName(wPoint).Empty() ? "World" : GetWorldName(wPoint);
    String worlddir = GetWorldDirName(worldName);
    String filter = ext == 0 ? "*" : ext;

    Vector<String> filenames;
    fs->ScanDir(filenames, worlddir, filter, SCAN_FILES, false);

    URHO3D_LOGINFOF("MapStorage() - DeleteWorldFiles = %s in %s ... found %u files to delete !", ext, worlddir.CString(), filenames.Size());

    for (unsigned i=0; i < filenames.Size(); i++)
    {
        if (fs->Delete(worlddir + "/" + filenames[i]))
            URHO3D_LOGINFOF(" delete -> %s OK !", filenames[i].CString());
        else
            URHO3D_LOGWARNINGF(" delete -> %s NOK !", filenames[i].CString());
    }

    // erase worldfile
    if (filter == "*")
    {
        String worldfilename = storageDir_ + worldName + ".wld";
        if (fs->Delete(worldfilename))
            URHO3D_LOGINFOF(" delete -> %s OK !", worldfilename.CString());
    }
}

/// save world files to the save folder
void MapStorage::SaveWorldFiles(const IntVector2& wPoint)
{
    FileSystem* fs = GameContext::Get().context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        URHO3D_LOGERROR("MapStorage() - SaveWorldFiles : FileSystem Not Found !");
        return;
    }

    Vector<String> filenames;
    const String& worldName = GetWorldName(wPoint).Empty() ? "World" : GetWorldName(wPoint);
    String savedir = GameContext::Get().gameConfig_.saveDir_ + SAVELEVELSDIR + worldName;
    String worlddir = GetWorldDirName(worldName);

    // check the save folder
    if (!fs->DirExists(savedir))
        fs->CreateDir(savedir);
    // purge the save folder
    else
    {
        fs->ScanDir(filenames, savedir, "*", SCAN_FILES, false);
        for (unsigned i=0; i < filenames.Size(); i++)
            bool del = fs->Delete(savedir + "/" + filenames[i]);
    }

    URHO3D_LOGINFOF("MapStorage() - SaveWorldFiles ... ");

    // copy to the save folder
    filenames.Clear();
    fs->ScanDir(filenames, worlddir, "*", SCAN_FILES, false);

    for (unsigned i=0; i < filenames.Size(); i++)
    {
        if (fs->Copy(worlddir + "/" + filenames[i], savedir + "/" + filenames[i]))
            URHO3D_LOGINFOF(" save %s in %s OK !", String(worlddir + "/" +filenames[i]).CString(), savedir.CString());
        else
            URHO3D_LOGWARNINGF(" save %s NOK !", filenames[i].CString());
    }

    String worldFileName = storageDir_ + worldName + ".wld";
    String saveWorldFileName = savedir + "/" + worldName + ".wld";
    if (fs->Copy(worldFileName, saveWorldFileName))
        URHO3D_LOGINFOF(" save %s in %s OK !", worldFileName.CString(), saveWorldFileName.CString());
    else
        URHO3D_LOGWARNINGF(" save %s NOK !", worldFileName.CString());
}

/// load world files from the save folder
void MapStorage::LoadWorldFiles(const IntVector2& wPoint)
{
    FileSystem* fs = GameContext::Get().context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        URHO3D_LOGERROR("MapStorage() - LoadWorldFiles : FileSystem Not Found !");
        return;
    }

    Vector<String> filenames;
    const String& worldName = GetWorldName(wPoint).Empty() ? "World" : GetWorldName(wPoint);
    String savedir = GameContext::Get().gameConfig_.saveDir_ + SAVELEVELSDIR + worldName;
    String worlddir = GetWorldDirName(worldName);

    // purge the world folder
    fs->ScanDir(filenames, worlddir, "*", SCAN_FILES, false);
    for (unsigned i=0; i < filenames.Size(); i++)
        bool del = fs->Delete(worlddir + "/" + filenames[i]);
    if (fs->FileExists(storageDir_ + worldName + ".wld"))
        bool del = fs->Delete(storageDir_ + worldName + ".wld");

    URHO3D_LOGINFOF("MapStorage() - LoadWorldFiles ... ");

    // copy from the save folder
    filenames.Clear();
    fs->ScanDir(filenames, savedir, "*", SCAN_FILES, false);

    for (unsigned i=0; i < filenames.Size(); i++)
    {
        if (fs->Copy(savedir + "/" + filenames[i], worlddir + "/" + filenames[i]))
            URHO3D_LOGINFOF(" load %s in %s OK !", String(savedir + "/" + filenames[i]).CString(), worlddir.CString());
        else
            URHO3D_LOGWARNINGF(" load %s NOK !", filenames[i].CString());
    }

    String worldFileName = storageDir_ + worldName + ".wld";
    String saveWorldFileName = savedir + "/" + worldName + ".wld";
    if (fs->Copy(saveWorldFileName, worldFileName))
        URHO3D_LOGINFOF(" load %s in %s OK !", saveWorldFileName.CString(), worldFileName.CString());
    else
        URHO3D_LOGWARNINGF(" load %s NOK !", saveWorldFileName.CString());
}

/// copy world files from base distribution folder
void MapStorage::CopyInitialWorldFiles(const IntVector2& wPoint)
{
    FileSystem* fs = GameContext::Get().context_->GetSubsystem<FileSystem>();
    if (!fs)
    {
        URHO3D_LOGERROR("MapStorage() - CopyInitialWorldFiles : FileSystem Not Found !");
        return;
    }

    Vector<String> filenames;
    const String& worldName = GetWorldName(wPoint).Empty() ? "World" : GetWorldName(wPoint);
    String distribdir = fs->GetProgramDir() + DATALEVELSDIR + worldName;
    String worlddir = GetWorldDirName(worldName);

    // copy from the save folder
    fs->ScanDir(filenames, distribdir, "*", SCAN_FILES, false);

    URHO3D_LOGINFOF("MapStorage() - CopyInitialWorldFiles ... %u files from distrib=%s to worlddir=%s ...", filenames.Size(), distribdir.CString(), worlddir.CString());

    for (unsigned i=0; i < filenames.Size(); i++)
    {
        if (fs->Copy(distribdir + "/" + filenames[i], worlddir + "/" + filenames[i]))
            URHO3D_LOGINFOF(" copy %s OK in %s!", String(distribdir + "/" + filenames[i]).CString(), worlddir.CString());
        else
            URHO3D_LOGWARNINGF(" copy %s NOK !", filenames[i].CString());
    }

    String worldFileName = storageDir_ + worldName + ".wld";
    String saveWorldFileName = distribdir + "/" + worldName + ".wld";
    if (fs->Copy(saveWorldFileName, worldFileName))
        URHO3D_LOGINFOF(" copy %s in %s OK !", saveWorldFileName.CString(), worldFileName.CString());

    URHO3D_LOGINFOF("MapStorage() - CopyInitialWorldFiles ... OK !");
}


inline void MapStorage::PushMapToLoad(const ShortIntVector2& mPoint)
{
    if (!World2D::IsInsideWorldBounds(mPoint))
    {
        URHO3D_LOGWARNINGF("MapStorage() - PushMapToLoad %s : not inside World2D bounds !", mPoint.ToString().CString());
        return;
    }

    /// if not in memory, push it
    if (!mapsInMemory_.Contains(mPoint) && !mapsToLoadInMemory_.Contains(mPoint))
    {
        mapsToLoadInMemory_.Push(mPoint);
        URHO3D_LOGINFOF("MapStorage() - PushMapToLoad mPoint=%s", mPoint.ToString().CString());
    }

    /// Never remove in the mapsToUnloadFromMemory_, let the process to do it
}

bool MapStorage::IsInsideBufferedArea(const ShortIntVector2& mPoint) const
{
    for (Vector<IntRect>::ConstIterator it = bufferAreas_.Begin(); it != bufferAreas_.End(); ++it)
    {
        if (it->IsInside(mPoint.x_, mPoint.y_) == INSIDE)
            return true;
    }
    return false;
}

void MapStorage::UpdateBufferedArea(bool maximizeMapsToLoad)
{
    if (!bufferedAreaDirty_)
        return;

    World2D::GetWorld()->GetBufferExpandInfos(bufferExpandInfos_, maximizeMapsToLoad);

    // Set the new buffer Areas
    bufferAreas_.Resize(bufferExpandInfos_.Size());
    for (unsigned i = 0; i < bufferAreas_.Size(); i++)
    {
        const BufferExpandInfo& expand = bufferExpandInfos_[i];
        IntRect& newBufferedArea = bufferAreas_[i];
        newBufferedArea.left_   = expand.x_ - bOffset_;
        newBufferedArea.top_    = expand.y_ - bOffset_;
        newBufferedArea.right_  = expand.x_ + bOffset_;
        newBufferedArea.bottom_ = expand.y_ + bOffset_;
    }

//    currentWorld2DInfo_->ConvertMapRect2WorldRect(bufferedArea_[viewport], bufferedAreaRect_[viewport]);

    /// Set List to Unload
    if (mapsInMemory_.Size())
    {
        for (HashMap<ShortIntVector2, Map* >::Iterator it=mapsInMemory_.Begin(); it!=mapsInMemory_.End(); ++it)
        {
            const ShortIntVector2& mPoint = it->first_;
            Map* map = it->second_;
            if (map == 0)
                continue;

#ifdef USE_LOADINGLISTS
            List<ShortIntVector2>::Iterator jt;
#else
            Vector<ShortIntVector2>::Iterator jt;
#endif
            jt = mapsToUnloadFromMemory_.Find(mPoint);

            if (IsInsideBufferedArea(mPoint))
            {
                // remove it from unloading the process.
                if (jt != mapsToUnloadFromMemory_.End())
                {
                    mapsToUnloadFromMemory_.Erase(jt);
                }
            }
            else if (!World2D::GetKeepedVisibleMaps().Contains(mPoint) && World2D::AllowClearMaps())
            {
                // remove it from creation list because unloading process can be slow
                URHO3D_LOGERRORF("MapStorage() - UpdateBufferedArea : mPoint=%s to Unload ...", mPoint.ToString().CString());
                mapCreator_->PurgeMap(mPoint);

                if (jt == mapsToUnloadFromMemory_.End())
                {
                    URHO3D_LOGINFOF("MapStorage() - UpdateBufferedArea : Push mPoint=%s to Unload !", mPoint.ToString().CString());
                    mapsToUnloadFromMemory_.Push(mPoint);
                }
            }
        }
    }

    /// Purge List to Load
    if (mapsToLoadInMemory_.Size())
    {
#ifdef USE_LOADINGLISTS
        List<ShortIntVector2>::Iterator it;
#else
        Vector<ShortIntVector2>::Iterator it;
#endif
        it = mapsToLoadInMemory_.Begin();
        while (it != mapsToLoadInMemory_.End())
        {
            if (!IsInsideBufferedArea(*it) && !World2D::GetKeepedVisibleMaps().Contains(*it) && World2D::AllowClearMaps())
            {
                // if not in the load list, remove it from creation list
                URHO3D_LOGERRORF("MapStorage() - UpdateBufferedArea : mPoint=%s purge load list ...", it->ToString().CString());
                mapCreator_->PurgeMap(*it);

                it = mapsToLoadInMemory_.Erase(it);
            }
            else
            {
                it++;
            }
        }
    }

    /// Set List to Load (In Reverse Order : first pushed, last loaded)
    for (Vector<BufferExpandInfo>::ConstIterator it = bufferExpandInfos_.Begin(); it != bufferExpandInfos_.End(); ++it)
    {
        const int centerx = it->x_;
        const int centery = it->y_;
        const int expX = it->dx_;
        const int expY = it->dy_;

        const int dirX = expX ? expX / Abs(expX) : 1;
        const int dirY = expY ? expY / Abs(expY) : 1;

        // if expX != 0 it's an first expansion following X otherwise is an first expansion following Y
        int expDirX;
        int expDirY;
        int& exp1 = expX ? expDirX : expDirY; // First Expansion Direction
        int& exp2 = expX ? expDirY : expDirX; // Second Expansion Direction

        // rear expansion only if new position is far from current position
        if ((!expX && !expY) || (Abs(expX) >= 3*bOffset_) || (Abs(expY) >= 3*bOffset_))
        {
            if (expX)
            {
                for (exp2 = bOffset_; exp2 > 0; exp2--)
                    for (exp1 = bOffset_; exp1 > 0; exp1--)
                    {
                        PushMapToLoad(ShortIntVector2(centerx - dirX*expDirX, centery - dirY*expDirY));
                        PushMapToLoad(ShortIntVector2(centerx - dirX*expDirX, centery + dirY*expDirY));
                    }
            }
            else
            {
                for (exp2 = bOffset_; exp2 > 0; exp2--)
                    for (exp1 = bOffset_; exp1 > 0; exp1--)
                    {
                        PushMapToLoad(ShortIntVector2(centerx - dirX*expDirX, centery - dirY*expDirY));
                        PushMapToLoad(ShortIntVector2(centerx + dirX*expDirX, centery - dirY*expDirY));
                    }
            }

            exp2 = 0;
            for (exp1 = bOffset_; exp1 > 0; exp1--)
                PushMapToLoad(ShortIntVector2(centerx - dirX*expDirX, centery - dirY*expDirY));
        }

        // sides expansion
        {
            if (expX)
            {
                for (exp2 = bOffset_; exp2 > 0; exp2--)
                    for (exp1 = bOffset_; exp1 >= 0; exp1--)
                    {
                        PushMapToLoad(ShortIntVector2(centerx + dirX*expDirX, centery - dirY*expDirY));
                        PushMapToLoad(ShortIntVector2(centerx + dirX*expDirX, centery + dirY*expDirY));
                    }
            }
            else
            {
                for (exp2 = bOffset_; exp2 > 0; exp2--)
                    for (exp1 = bOffset_; exp1 >= 0; exp1--)
                    {
                        PushMapToLoad(ShortIntVector2(centerx - dirX*expDirX, centery + dirY*expDirY));
                        PushMapToLoad(ShortIntVector2(centerx + dirX*expDirX, centery + dirY*expDirY));
                    }
            }
        }

        // front expansion, including New Centered Map
        exp2 = 0;
        for (exp1 = bOffset_; exp1 >= 0; exp1--)
            PushMapToLoad(ShortIntVector2(centerx + dirX*expDirX, centery + dirY*expDirY));
    }

    bufferedAreaDirty_ = false;
    //DumpMapsMemory();
}

void MapStorage::UpdateAllMaps()
{
    bool updated = UpdateMapsInMemory();
}

bool MapStorage::UpdateMapsInMemory(HiresTimer* timer)
{
//    URHO3D_LOGERRORF("MapStorage() - UpdateMapsInMemory ...");

    unsigned numUnloadedMaps = 0;

    /// Priorize the Unloading of a Map (MapCreator wait for this mPoint)
    if (forcedMapToUnload_ != UNDEFINED_MAPPOINT)
    {
#ifdef USE_LOADINGLISTS
        List<ShortIntVector2>::Iterator it;
#else
        Vector<ShortIntVector2>::Iterator it;
#endif
        it = mapsToUnloadFromMemory_.Find(forcedMapToUnload_);
        if (it != mapsToUnloadFromMemory_.End())
        {
            URHO3D_LOGERRORF("MapStorage() - UpdateMapsInMemory : Forced Unload mPoint = %s", it->ToString().CString());
            if (UnloadMapAt(*it, timer))
            {
                it = mapsToUnloadFromMemory_.Erase(it);
                forcedMapToUnload_ = UNDEFINED_MAPPOINT;
                numUnloadedMaps++;
            }
        }
        else
        {
            forcedMapToUnload_ = UNDEFINED_MAPPOINT;
        }
    }

    /// Priorize the Update Loaded Maps (Visibility)
    if (creatingMode_ != MCM_INSTANT)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapStore_Loaded);
#endif

        /// Update Current Maps
        unsigned numviewports = ViewManager::Get()->GetNumViewports();
        for (int viewport=0; viewport < numviewports; viewport++)
        {
            Map* map = World2D::GetWorld()->GetCurrentMap(viewport);
            if (map)
                map->Update(timer);
        }

        /// Update Maps
        for (HashMap<ShortIntVector2, Map* >::ConstIterator it=mapsInMemory_.Begin(); it!=mapsInMemory_.End(); ++it)
        {
            if (!it->second_->Update(timer))
                break;
        }

        /// Update ObjectMaped
        const PODVector<ObjectMaped* >& objectmapeds = ObjectMaped::GetPhysicObjects();
        for (unsigned i=0; i < objectmapeds.Size(); i++)
        {
            ObjectMaped* objectmaped = objectmapeds[i];
            if (!objectmaped || !objectmaped->GetNode()->IsEnabled())
                continue;

            if (!objectmaped->Update(timer))
                break;
        }
    }

    /// New Maps to Load
    {
        while (mapsToLoadInMemory_.Size())
        {
            URHO3D_LOGINFOF("MapStorage() - UpdateMapsInMemory : ToLoad mPoint = %s ... mpoint=%s",
                            mapsToLoadInMemory_.Back().ToString().CString(), mapsToLoadInMemory_.Back().ToString().CString());

            if (!InitializeMap(mapsToLoadInMemory_.Back(), timer))
                break;

            mapsToLoadInMemory_.Pop();

//            if (timer && timer->GetUSec(false) > delayUpdateUsec_)
//                return false;
        }
    }

//    URHO3D_LOGERRORF("MapStorage() - UpdateMapsInMemory : MapsInMemory=%u (max=%u) ", mapsInMemory_.Size(), maxMapsInMemory_);

    /// Unload Maps
    if (mapsInMemory_.Size() > maxMapsInMemory_ && mapsToUnloadFromMemory_.Size())
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapStore_Unloading);
#endif

//        URHO3D_LOGINFOF("MapStorage() - %d maps in memory > maxMaps (%d) ; num mapsToUnLoad = %d => Drop Maps (t=%d on %d msec)",
//                        mapsInMemory_.Size(), maxMapsInMemory_, mapsToUnloadFromMemory_.Size(), timer ? timer->GetUSec(false)/1000 : 0, delayUpdateUsec_/1000);

        GAME_SETGAMELOGENABLE(GAMELOG_MAPUNLOAD, false);

#ifdef USE_LOADINGLISTS
        List<ShortIntVector2>::Iterator it = mapsToUnloadFromMemory_.Begin();
        while (it != mapsToUnloadFromMemory_.End())
        {
            // Skip the Maps To Keep Visible
            if (World2D::GetKeepedVisibleMaps().Contains(*it))
            {
//                URHO3D_LOGINFOF("MapStorage() - UpdateMapsInMemory : keep visible => skip unload mPoint = %s", it->ToString().CString());
                it++;
                continue;
            }

            URHO3D_LOGINFOF("MapStorage() - UpdateMapsInMemory : Unload mPoint = %s", it->ToString().CString());
            if (UnloadMapAt(*it, timer))
            {
                it = mapsToUnloadFromMemory_.Erase(it);
                numUnloadedMaps++;
            }

            // Just one unloadmap if timer
            if (timer)
                break;
        }
#else
        int i = mapsToUnloadFromMemory_.Size();
        while (--i >= 0)
        {
            // Skip the Maps To Keep Visible
            if (World2D::GetKeepedVisibleMaps().Contains(mapsToUnloadFromMemory_[i]))
            {
//                URHO3D_LOGINFOF("MapStorage() - UpdateMapsInMemory : keep visible => skip unload mPoint = %s", mapsToUnloadFromMemory_[i].ToString().CString());
                continue;
            }

            URHO3D_LOGINFOF("MapStorage() - UpdateMapsInMemory : Unload mPoint = %s", mapsToUnloadFromMemory_[i].ToString().CString());
            if (UnloadMapAt(mapsToUnloadFromMemory_[i], timer))
            {
                mapsToUnloadFromMemory_.Pop();
                numUnloadedMaps++;
            }

            // Just one unloadmap if timer
            if (timer)
                break;
        }
#endif
        GAME_SETGAMELOGENABLE(GAMELOG_MAPUNLOAD, true);

//        if (timer && numUnloadedMaps > 0)
//            return false;
    }

    /// Update Loading Maps
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapStore_Loading);
#endif

//        URHO3D_LOGERRORF("MapStorage() - UpdateMapsInMemory ... MapCreator Update ... timer=%u", timer);

        int numloaded = 0;

        if (timer || creatingMode_ == MCM_INSTANT)
        {
            if (mapCreator_->Update(timer, delayUpdateUsec_) && mapCreator_->IsRunning())
                numloaded++;
        }
        else
        {
            while (mapCreator_->Update() && mapCreator_->IsRunning())
                numloaded++;
        }

//        if (numloaded > 0)
//            SendEvent(WORLD_MAPUPDATE);
    }

    // Necessary to not Hang the program when using World2D::UpdateInstant() or World2D::UpdateAll()
    if (GameContext::Get().gameWorkQueue_ && !timer)
    {
//        URHO3D_LOGERRORF("MapStorage() - UpdateMapsInMemory ... !timer don't hang program !");
        GameContext::Get().gameWorkQueue_->Complete(0);
    }

//    URHO3D_LOGERRORF("MapStorage() - UpdateMapsInMemory ... OK !");

    if (!mapCreator_->IsRunning())
    {
//        GameHelpers::ResetGameLog(GetContext());
        return true;
    }

    return false;
}


void MapStorage::MarkMapDirty()
{
    for (HashMap<ShortIntVector2, Map* >::ConstIterator it=mapsInMemory_.Begin(); it!=mapsInMemory_.End(); ++it)
        it->second_->MarkDirty();

    const PODVector<ObjectMaped* >& physicObjects = ObjectMaped::GetPhysicObjects();
    for (PODVector<ObjectMaped* >::ConstIterator it=physicObjects.Begin(); it!=physicObjects.End(); ++it)
        if (*it)
            (*it)->MarkDirty();
}


void MapStorage::DumpMapsMemory() const
{
    URHO3D_LOGINFOF("MapStorage() - DumpMapsMemory : mapsInMemory=%u/%u mapsToLoadInMemory=%u mapsToUnloadFromMemory_=%u",
                    mapsInMemory_.Size(), maxMapsInMemory_, mapsToLoadInMemory_.Size(), mapsToUnloadFromMemory_.Size());

//    URHO3D_LOGINFOF("centeredPoint_=%s", centeredPoint_[0].ToString().CString());
//    URHO3D_LOGINFOF("bufferedArea_=%s", bufferedArea_[0].ToString().CString());
//    URHO3D_LOGINFOF("bufferedAreaRect_=%s", bufferedAreaRect_[0].ToString().CString());

    mapCreator_->Dump();

    unsigned i=0;
    for (HashMap<ShortIntVector2, Map* >::ConstIterator it=mapsInMemory_.Begin(); it!=mapsInMemory_.End(); ++it,++i)
    {
        Map* map = it->second_;
        URHO3D_LOGINFOF("mapsInMemory_[%u] : mPoint=%s Map*=%u status=%s(%d) visible=%s counters=%s mapdataSize=%u=>%u",
                        i, it->first_.ToString().CString(), map, mapStatusNames[map->mapStatus_.status_], map->mapStatus_.status_,
                        map->GetVisibleState(), map->mapStatus_.DumpMapCounters().CString(), map->GetMapData() ? map->GetMapData()->GetDataSize() : 0, map->GetMapData() ? map->GetMapData()->GetCompressedDataSize() : 0);
    }

    i=0;

#ifdef USE_LOADINGLISTS
    for (List<ShortIntVector2>::ConstIterator it=mapsToLoadInMemory_.Begin(); it!=mapsToLoadInMemory_.End(); ++it,++i)
        URHO3D_LOGINFOF("mapsToLoadInMemory_[%u] : mPoint=%s", i, (*it).ToString().CString());
    i=0;
    for (List<ShortIntVector2>::ConstIterator it=mapsToUnloadFromMemory_.Begin(); it!=mapsToUnloadFromMemory_.End(); ++it,++i)
        URHO3D_LOGINFOF("mapsToUnloadFromMemory_[%u] : mPoint=%s", i, (*it).ToString().CString());
#else
    for (Vector<ShortIntVector2>::ConstIterator it=mapsToLoadInMemory_.Begin(); it!=mapsToLoadInMemory_.End(); ++it,++i)
        URHO3D_LOGINFOF("mapsToLoadInMemory_[%u] : mPoint=%s", i, (*it).ToString().CString());
    i=0;
    for (Vector<ShortIntVector2>::ConstIterator it=mapsToUnloadFromMemory_.Begin(); it!=mapsToUnloadFromMemory_.End(); ++it,++i)
        URHO3D_LOGINFOF("mapsToUnloadFromMemory_[%u] : mPoint=%s", i, (*it).ToString().CString());
#endif

    mapPool_->Dump();
}

void MapStorage::DumpRegisterWorldPath()
{
    unsigned size = registeredWorld2DInfo_.Size();

    URHO3D_LOGINFOF("MapStorage() - DumpRegisterWorldPath : Size=%u ...", size);
    for (unsigned i = 0; i < size; ++i)
    {
        const World2DInfo& winfo = registeredWorld2DInfo_[i];

        URHO3D_LOGINFOF("-> World2DInfo %u : Point=%s Ptr=%u StorageDir=%s Atlas=%u", i, registeredWorldPoint_[i].ToString().CString(),
                        &winfo, winfo.storageDir_.CString(), winfo.atlas_.Get());

        winfo.atlas_->Dump();
    }
    URHO3D_LOGINFOF("MapStorage() - DumpRegisterWorldPath : ... OK !");
}
