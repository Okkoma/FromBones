#include <cmath>

#include <Urho3D/Urho3D.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/IO/Log.h>

#include "DefsViews.h"
#include "DefsWorld.h"

#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameContext.h"

#include "Map.h"
#include "MapStorage.h"
#include "MapPool.h"
#include "MapGenerator.h"
#include "MapWorld.h"
#include "ViewManager.h"

#include "MapGeneratorDungeon.h"
#include "MapGeneratorGround.h"
#include "MapGeneratorMaze.h"
#include "MapGeneratorWorld.h"
#include "MapColliderGenerator.h"

#include "MapCreator.h"

#define MAP_GENERATECOLLIDERS
#define MAP_ACTIVE_POPULATE

//#define DUMP_VIEWIDS

//#define ACTIVE_MAZEBACKGROUND

extern const char* mapStatusNames[];

MapCreator* MapCreator::creator_ = 0;
HashMap<unsigned, int> MapCreator::typeindexes_;
Vector<String> MapCreator::typenames_;
Vector<MapGenerator* > MapCreator::generators_;

const unsigned MapCreator::GEN_RANDOM_TYPE = 0;
const unsigned MapCreator::GEN_DUNGEON_TYPE = StringHash("MapGeneratorDungeon").Value();
const unsigned MapCreator::GEN_CAVE_TYPE = StringHash("MapGeneratorCave").Value();
const unsigned MapCreator::GEN_MAZE_TYPE = StringHash("MapGeneratorMaze").Value();
const unsigned MapCreator::GEN_WORLD_TYPE = StringHash("MapGeneratorWorld").Value();
const unsigned MapCreator::GEN_ASTEROID_TYPE = StringHash("MapGeneratorAsteroid").Value();
const unsigned MapCreator::GEN_MOBILECASTLE_TYPE = StringHash("MapGeneratorMobileCastle").Value();

MapGeneratorDungeon* genDungeon;
MapGeneratorGround* genGround;
MapGeneratorMaze* genMaze;
MapGeneratorWorld* genWorld;


void MapCreator::InitTable()
{
    MapGenerator::InitTable();

    genDungeon = new MapGeneratorDungeon();
    genGround = new MapGeneratorGround();
    genMaze = new MapGeneratorMaze();
    genWorld = new MapGeneratorWorld();

    typeindexes_.Clear();
    typeindexes_[GEN_RANDOM_TYPE] = GEN_RANDOM;
    typeindexes_[GEN_DUNGEON_TYPE] = GEN_DUNGEON;
    typeindexes_[GEN_CAVE_TYPE] = GEN_CAVE;
    typeindexes_[GEN_MAZE_TYPE] = GEN_MAZE;
    typeindexes_[GEN_WORLD_TYPE] = GEN_WORLD;
    typeindexes_[GEN_ASTEROID_TYPE] = GEN_ASTEROID;
    typeindexes_[GEN_MOBILECASTLE_TYPE] = GEN_MOBILECASTLE;

    typenames_.Clear();
    typenames_.Push("MapGeneratorRandom");
    typenames_.Push("MapGeneratorDungeon");
    typenames_.Push("MapGeneratorCave");
    typenames_.Push("MapGeneratorMaze");
    typenames_.Push("MapGeneratorWorld");
    typenames_.Push("MapGeneratorAsteroid");
    typenames_.Push("MapGeneratorMobileCastle");

    generators_.Clear();
    generators_.Push(0);
    generators_.Push(genDungeon);
    generators_.Push(genGround);
    generators_.Push(genMaze);
    generators_.Push(genWorld);
    generators_.Push(0);
    generators_.Push(0);
}

void MapCreator::DeInitTable()
{
    for (unsigned i = 1; i < generators_.Size(); ++i)
    {
        if (generators_[i])
        {
            delete generators_[i];
            generators_[i] = 0;
        }
    }

    generators_.Clear();
}



MapCreator::MapCreator(Context* context) :
    Object(context),
    info_(0),
    defaultGenerator_(GEN_RANDOM),
    Random(GameRand::GetRandomizer(MAPRAND))
{
    URHO3D_LOGINFOF("MapCreator()");

    if (!creator_)
        creator_ = this;
}

MapCreator::~MapCreator()
{
    if (creator_ == this)
        creator_ = 0;

    URHO3D_LOGINFOF("~MapCreator() ... OK !");
}


void MapCreator::SetWorldInfo(const World2DInfo* info)
{
    if (!info)
        return;

    info_ = info;
    SetDefaultGenerator(info->defaultGenerator_);
    simpleGroundLevel_ = info->simpleGroundLevel_;

    MapGenerator::SetWorldInfo(info);
}

void MapCreator::SetDefaultGenerator(const String& name)
{
    SetDefaultGenerator(StringHash(name).Value());
}

void MapCreator::SetDefaultGenerator(unsigned type)
{
    if (typeindexes_.Find(type) != typeindexes_.End())
        defaultGenerator_ = (MapGeneratorType)typeindexes_[type];
}

void MapCreator::SetGenerator(MapGeneratorStatus& genStatus, MapGeneratorType type)
{
    URHO3D_LOGINFOF("MapCreator() - SetGenerator type=%s(%u) ...", typenames_[type].CString(), type);

    genStatus.generator_ = type;

    if (genStatus.generator_ == GEN_RANDOM)
    {
        int r = Random.Get(100);

        if (r < 45)
            genStatus.generator_ = GEN_DUNGEON;
        else
            genStatus.generator_ = GEN_CAVE;
    }

    genStatus.genFrontFunc_ = genStatus.genBackFunc_ = genStatus.genInnerFunc_ = 0;

    switch (genStatus.generator_)
    {
    case GEN_DUNGEON :
        genStatus.genFrontFunc_ = info_->worldModel_ || genStatus.model_ ? &MapCreator::GenerateWorldGroundMap : &MapCreator::GenerateSimpleGroundMap;
        genStatus.genBackFunc_ = &MapCreator::GenerateBackGroundMap;
        genStatus.genInnerFunc_ = &MapCreator::GenerateDungeonMap;
        break;
    case GEN_CAVE :
        genStatus.genFrontFunc_ = info_->worldModel_ || genStatus.model_ ? &MapCreator::GenerateWorldGroundMap : &MapCreator::GenerateSimpleGroundMap;
        genStatus.genBackFunc_ = &MapCreator::GenerateBackGroundMap;
        genStatus.genInnerFunc_ = &MapCreator::GenerateCaveMap;
        break;
    case GEN_ASTEROID :
        genStatus.genFrontFunc_ = &MapCreator::GenerateAsteroid;
        break;
    case GEN_MOBILECASTLE :
        genStatus.genInnerFunc_ = &MapCreator::GenerateDungeonMap;
        break;
    }

    URHO3D_LOGINFOF("MapCreator() - SetGenerator : map=%s type=%s ... OK !", genStatus.mappoint_.ToString().CString(), typenames_[genStatus.generator_].CString());
}

void MapCreator::SetNumEntities(const IntVector2& numEntities)
{
    numEntities_ = numEntities;

    URHO3D_LOGINFOF("MapCreator() - SetNumEntities = %s ... OK !", numEntities_.ToString().CString());
}

void MapCreator::SetAuthorizedCategories(const String& value)
{
    authorizedCategories_.Clear();

    if (value.Empty())
        return;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
        authorizedCategories_.Push(StringHash(*it));

    URHO3D_LOGINFOF("MapCreator() - SetAuthorizedCategories = %s ... OK !", value.CString());
}

void MapCreator::PurgeMapsOutsideVisibleAreas()
{
    if (!mapsToCreate_.Size() || !World2D::AllowClearMaps())
        return;

    Vector<ShortIntVector2>::Iterator it = mapsToCreate_.Begin();
    while (it != mapsToCreate_.End())
    {
        if (World2D::GetKeepedVisibleMaps().Contains(*it))
        {
            it++;
            continue;
        }

        if (World2D::GetStorage()->IsInsideBufferedAreas(*it))
        {
            it++;
            continue;
        }

        if (World2D::IsInsideVisibleAreasMinimized(*it))
        {
            it++;
            continue;
        }

        Map* map = World2D::GetMapAt(*it, false);
        if (map)
        {
            MapColliderGenerator::Get()->ResetMapGeneration(map);
            map->SetStatus(Unloading_Map);
//            MapStorage::Get()->ForceMapToUnload(map);
        }

        mapsToCreate_.EraseSwap(it - mapsToCreate_.Begin());

        URHO3D_LOGINFOF("MapCreator() - PurgeMapsOutsideVisibleAreas : %s !", it->ToString().CString());
    }
}

void MapCreator::PurgeMap(const ShortIntVector2& mpoint)
{
    URHO3D_LOGINFOF("MapCreator() - PurgeMap : %s !", mpoint.ToString().CString());
    mapsToCreate_.RemoveSwap(mpoint);

    Map* map = World2D::GetMapAt(mpoint, false);
    if (map)
        MapColliderGenerator::Get()->ResetMapGeneration(map);
}


void MapCreator::AddMapToCreate(const ShortIntVector2& mpoint)
{
    if (!mapsToCreate_.Contains(mpoint))
    {
        if (World2D::GetStorage()->GetAvailableMapAt(mpoint))
            return;

        URHO3D_LOGINFOF("MapCreator() - AddMapToCreate : %s !", mpoint.ToString().CString());

        mapsToCreate_.Push(mpoint);
        sortMaps_ = true;
    }
}

bool MapCreator::CreateMap(Map* map, HiresTimer* timer, const long long& delay)
{
    if (map->GetStatus() == Available)
    {
        mapsToCreate_.RemoveSwap(map->GetMapPoint());
        return true;
    }

    if (map->GetStatus() < Generating_Map || map->GetStatus() > Available)
    {
        if (map->GetStatus() == Uninitialized)
        {
            if (MapStorage::Get()->InitializeMap(map->GetMapPoint()))
            {
                URHO3D_LOGERRORF("MapCreator() - CreateMap at %s map=%u ... status=%s[%d] ... Can't Initialize the Map !", map->GetMapPoint().ToString().CString(), map, mapStatusNames[map->GetStatus()], map->GetStatus());
                return false;
            }
        }
        else if (map->GetStatus() == Unloading_Map)
        {
            URHO3D_LOGERRORF("MapCreator() - CreateMap at %s map=%u ... status=%s[%d] ... Force To Unload !", map->GetMapPoint().ToString().CString(), map, mapStatusNames[map->GetStatus()], map->GetStatus());

            MapStorage::Get()->ForceMapToUnload(map);
            return false;
        }
        else if (map->GetStatus() == Loading_Map)
        {
            if (!MapStorage::GetMapSerializer()->IsInQueue(map->GetMapPoint()))
            {
                map->SetStatus(Generating_Map);
                URHO3D_LOGWARNINGF("MapCreator() - CreateMap at %s map=%u ... status=%s[%d] ... Skip Loading ...", map->GetMapPoint().ToString().CString(), map, mapStatusNames[map->GetStatus()], map->GetStatus());
            }
            else
            {
                URHO3D_LOGINFOF("MapCreator() - CreateMap at %s map=%u ... status=%s[%d] ... WAITING changing status to Generating_Map ... serializerStarted=%s isInQueue=%s numInQueue=%u",
                                map->GetMapPoint().ToString().CString(), map, mapStatusNames[map->GetStatus()], map->GetStatus(), MapStorage::GetMapSerializer()->IsRunning() ? "true":"false",
                                MapStorage::GetMapSerializer()->IsInQueue(map->GetMapPoint()) ? "true":"false", GameContext::Get().gameWorkQueue_->GetNumInQueue());
            }
        }
    }

    if (MapStorage::GetMapSerializer()->IsRunning() && MapStorage::GetMapSerializer()->IsInQueue(map->GetMapPoint()))
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... MapSerializer Running ... Skip Creating Map ...",
                         map->GetMapPoint().ToString().CString());
#endif
        return false;
    }

    if (map->GetStatus() == Generating_Map)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_GenMap);
#endif

//        URHO3D_LOGINFOF("MapCreator() - CreateMap at %s ...", map->GetMapPoint().ToString().CString());

        GameRand::SetSeedRand(ALLRAND, map->GetMapGeneratorStatus().rseed_);
        map->GetMapGeneratorStatus().mappoint_ = map->GetMapPoint();
        SetGenerator(map->GetMapGeneratorStatus(), defaultGenerator_);
        MapGenerator::SetSize(map->GetWidth(), map->GetHeight());

        delay_ = timer ? delay : MAP_MAXDELAY_NOASYNCLOADING;

        map->SetPhysicProperties(BT_STATIC, MAPMASS);
//        bool dungeon = map->GetMapGeneratorStatus().generator_ == GEN_DUNGEON;
//        map->featuredMap_->SetViewConfiguration(info_->worldModel_, dungeon, dungeon ? &genDungeon->GetDungeonInfo() : 0);
        map->featuredMap_->SetViewConfiguration(map->GetMapGeneratorStatus().generator_);
        map->SetStatus(Creating_Map_Layers);

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Generating Map seed=%u(w=%u+inc=%u) type=%s ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), map->GetMapSeed(), map->GetWorldSeed(), map->GetMapSeed()-map->GetWorldSeed(),
                         typenames_[map->GetMapGeneratorStatus().generator_].CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
//        GameRand::Dump10Value();
    }

    if (map->GetStatus() == Creating_Map_Layers)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_GenLayers);
#endif

//        URHO3D_LOGINFOF("MapCreator() - CreateMap at %s ... Creating_Map_Layers ...", map->GetMapPoint().ToString().CString());

        bool state = GenerateLayers(map, timer);

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - CreateMap : map=%s ... Generating Layers", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        map->SetStatus(Creating_Map_Colliders);

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Creating Map Layers ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        /// reset for GenerateColliders
        map->GetObjectFeatured()->indexToSet_ = 0;
        map->GetMapCounter(MAP_GENERAL) = 0;
    }

    if (map->GetStatus() == Creating_Map_Colliders)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_GenColliders);
#endif

//        URHO3D_LOGINFOF("MapCreator() - CreateMap at %s ... Creating_Map_Colliders ...", map->GetMapPoint().ToString().CString());

        bool state = GenerateColliders(map, timer);

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - CreateMap : map=%s ... Generating Colliders", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        map->SetStatus(Creating_Map_Entities);

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Creating Map Colliders ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
    }

    if (map->GetStatus() == Creating_Map_Entities)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_GenEntities);
#endif

//        URHO3D_LOGINFOF("MapCreator() - CreateMap at %s ... Creating_Map_Entities ...", map->GetMapPoint().ToString().CString());

        bool state = GenerateEntities(map, timer);

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - CreateMap : map=%s ... Generating Entities", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Creating Map Entities ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        map->SetStatus(Setting_Map);
    }

    if (map->GetStatus() >= Setting_Map && map->GetStatus() < Setting_Furnitures)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_SetMap);
#endif

        bool state = map->Set(MapStorage::Get()->GetNode(), timer);
        if (!state)
            return false;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Setting Map ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        map->SetStatus(Setting_Furnitures);
    }

    if (map->GetStatus() == Setting_Furnitures)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_SetFurnitures);
#endif

#ifdef HANDLE_FURNITURES
        bool state = map->SetFurnitures(timer, map->GetMapData()->IsSectionSet(MAPDATASECTION_ENTITYATTR));

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - CreateMap : map=%s ... Setting Furnitures", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }
#endif

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Setting Furnitures ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        map->SetStatus(Setting_Entities);
    }

    if (map->GetStatus() == Setting_Entities)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_SetEntities);
#endif

#ifdef HANDLE_ENTITIES
        bool state;

        if (map->GetMapData()->IsSectionSet(MAPDATASECTION_ENTITYATTR))
            state = map->SetEntities_Load(timer);

        else if (map->GetMapData()->IsSectionSet(MAPDATASECTION_SPOT))
            state = map->SetEntities_Add(timer);

        else
            state = true;

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - CreateMap : map=%s ... Setting Entities", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Setting Entities ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
#endif
        map->HideMap(timer);

        map->SetStatus(Setting_Borders);
    }

    if (map->GetStatus() == Setting_Borders)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(MapCreator_SetBorders);
#endif
#ifdef HANDLE_CONNECTED_BORDERS
//        if (!MapStorage::Get()->GetAtlas()->UseDimensionTiles())
        {
            URHO3D_PROFILE(MapCreator_SetBorders);
            // Horizontal Connections
            Map::ConnectHorizontalMaps(map, MapStorage::Get()->GetAvailableMapAt(ShortIntVector2(map->GetMapPoint().x_+1, map->GetMapPoint().y_)));
            Map::ConnectHorizontalMaps(MapStorage::Get()->GetAvailableMapAt(ShortIntVector2(map->GetMapPoint().x_-1, map->GetMapPoint().y_)), map);
            // Vertical Connections
            Map::ConnectVerticalMaps(MapStorage::Get()->GetAvailableMapAt(ShortIntVector2(map->GetMapPoint().x_, map->GetMapPoint().y_+1)), map);
            Map::ConnectVerticalMaps(map, MapStorage::Get()->GetAvailableMapAt(ShortIntVector2(map->GetMapPoint().x_, map->GetMapPoint().y_-1)));
        }
#endif

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Status=Available ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        map->SetSerializable(true);
        map->SetStatus(Available);
        map->GetMapGeneratorStatus().ResetCounters();

        map->SendEvent(MAP_AVAILABLE);

//#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - CreateMap at %s ... Status=Available ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
//#endif
        return true;
    }

    return false;
}

#include <Urho3D/Container/Sort.h>

// Sort from the farest to the nearest map
static inline bool CompareMapCenter(const ShortIntVector2& mapl, const ShortIntVector2& mapr)
{
    Vector2 center = World2D::GetExtendedVisibleRect().Center();
    Vector2 lcenter = Map::GetMapRect(mapl).Center();
    Vector2 rcenter = Map::GetMapRect(mapr).Center();

    return (lcenter - center).LengthSquared() > (rcenter - center).LengthSquared();
}

/// TODO : multiviews
bool MapCreator::Update(HiresTimer* timer, const long long& delay)
{
    URHO3D_PROFILE(MapCreator);

#ifdef DUMP_MAPCREATOR_LOGS
    URHO3D_LOGERRORF("MapCreator() - Update : numMapsToCreate=%u ..." , mapsToCreate_.Size());
#endif
    {
        URHO3D_PROFILE(MapCreator_Purge);
        PurgeMapsOutsideVisibleAreas();
    }

    // Always prior visibleArea
    if (MapStorage::Get()->GetCreatingMode() != MCM_INSTANT)
    {
        // TODO : MultiViewport !
        const IntRect& visibleArea = World2D::GetVisibleAreas();
        for (int y = visibleArea.top_; y <= visibleArea.bottom_; y++)
        {
            for (int x = visibleArea.left_; x <= visibleArea.right_; x++)
            {
                AddMapToCreate(ShortIntVector2(x, y));
            }
        }
    }

    // Sort the maps here
    if (sortMaps_ && mapsToCreate_.Size() > 1)
    {
        URHO3D_PROFILE(MapCreator_Sort);
        Sort(mapsToCreate_.Begin(), mapsToCreate_.End(), CompareMapCenter);
        sortMaps_ = false;
    }

    // Always prior currentMap
    if (MapStorage::Get()->GetCreatingMode() != MCM_INSTANT)
    {
        // TODO : MultiViewport !
        if (World2D::GetCurrentMap() && !World2D::GetCurrentMap()->IsAvailable())
        {
            AddMapToCreate(World2D::GetCurrentMap()->GetMapPoint());
            sortMaps_ = false;
        }
    }

    if (!mapsToCreate_.Size())
    {
//#ifdef DUMP_MAPCREATOR_LOGS
//        URHO3D_LOGERROR("MapCreator() - Update : no maps to create !");
//#endif
        return true;
    }

    GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);

    for (;;)
    {
        if (!mapsToCreate_.Size())
            break;

        Map* map = World2D::GetMapAt(mapsToCreate_.Back(), true);
        if (!map)
            break;

#ifdef ACTIVE_WORLD2D_DEBUG
        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);

        URHO3D_LOGINFOF("MapCreator() - Update ... map=%s state=%s MapCounters=%d,%d,%d,%d,%d,%d,%d,%d (NumMapsToCreate=%u) ...",
                        map->GetMapPoint().ToString().CString(), mapStatusNames[map->GetStatus()],
                        map->GetMapCounter(MAP_GENERAL), map->GetMapCounter(MAP_FUNC1), map->GetMapCounter(MAP_FUNC2),
                        map->GetMapCounter(MAP_FUNC3), map->GetMapCounter(MAP_FUNC4), map->GetMapCounter(MAP_FUNC5),
                        map->GetMapCounter(MAP_FUNC6), map->GetMapCounter(MAP_VISIBILITY), mapsToCreate_.Size());

        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
#endif

        if (CreateMap(map, timer, delay))
        {
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);

#ifdef DUMP_CREATED_MAP
            map->Dump();
#endif

            mapsToCreate_.Pop();

            URHO3D_LOGINFOF("MapCreator() - Update map=%s OK ! maxdelayupdate=%dmsec (NumMapsRemainToCreate=%u) ...", map->GetMapPoint().ToString().CString(), (int)(delay_/1000), mapsToCreate_.Size());

            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
        else
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - CreateMap : map=%s ... status=%s mapcount=%d,%d,%d,%d", map->GetMapPoint().ToString().CString(), mapStatusNames[map->GetStatus()],
                                 map->GetMapCounter(MAP_GENERAL), map->GetMapCounter(MAP_FUNC1), map->GetMapCounter(MAP_FUNC2), map->GetMapCounter(MAP_FUNC2)), timer, delay_);
#endif
            break;
        }
    }

    GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);

    return mapsToCreate_.Size() == 0;
}


/// FRONTVIEW GENERATING FUNCTIONS

bool MapCreator::GenerateSimpleGroundMap(MapBase* map, HiresTimer* timer)
{
    int& mcount = map->GetMapCounter(MAP_FUNC1);

//    URHO3D_LOGINFOF("MapCreator() - GenerateSimpleGround(%u) : ... MAP_FUNC1=%d", currentGenIndex_, mcount);

    if (mcount == 0)
    {
        genGround->SetGeneratorMap(map->GetMapGeneratorStatus(), GROUNDMAP, FRONTVIEW, map->GetFeatures(FrontView_ViewId), true, !map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER));
        genGround->SetGeneratorMap(map->GetMapGeneratorStatus(), TERRAINMAP, FRONTVIEW, map->GetTerrainMap(), false);
        genGround->SetGeneratorMap(map->GetMapGeneratorStatus(), BIOMEMAP, FRONTVIEW, map->GetBiomeMap(), false);

        const GeneratorParams* gparams = info_->GetMapGenParams(FRONTVIEW);
        if (gparams)
        {
            genGround->SetParamsInt(map->GetMapGeneratorStatus(), gparams->params_);
            URHO3D_LOGINFOF("MapCreator() - GenerateSimpleGroundMap(%d) : ... indexToSet=%d useparams=%s !",
                            map->GetMapGeneratorStatus().generator_, mcount, gparams->ToString().CString());
        }
        else
        {
            // Bug dans le SetParams par args values
            // => remplacement par PODVector methode
            PODVector<int> defaultParams;
            defaultParams.Resize(10);
            defaultParams[0] = map->featuredMap_->height_/4;
            defaultParams[1] = map->featuredMap_->height_/2;
            defaultParams[2] = 1;
            defaultParams[3] = 6;
            defaultParams[4] = 1;
            defaultParams[5] = 0;
            defaultParams[6] = Max(0, simpleGroundLevel_-10);
            defaultParams[7] = Min(simpleGroundLevel_+10, 100);
            defaultParams[8] = 10;
            defaultParams[9] = 60;
            genGround->SetParamsInt(map->GetMapGeneratorStatus(), defaultParams);
            URHO3D_LOGINFOF("MapCreator() - GenerateSimpleGroundMap(%d) : ... indexToSet = %d use default params !",
                            map->GetMapGeneratorStatus().generator_, mcount);
        }

        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
            genGround->Generate(map->GetMapGeneratorStatus());

        // copy frontview to innerview/background
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            map->featuredMap_->CopyFeatureView(FrontView_ViewId, InnerView_ViewId);
            map->featuredMap_->CopyFeatureView(FrontView_ViewId, BackGround_ViewId);
        }

        mcount++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateSimpleGroundMap : map=%s ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }

    return true;
}

bool MapCreator::GenerateWorldGroundMap(MapBase* map, HiresTimer* timer)
{
    int& mcount1 = map->GetMapCounter(MAP_FUNC1);
    int& mcount2 = map->GetMapCounter(MAP_FUNC2);

//    URHO3D_LOGINFOF("MapCreator() - GenerateWorldGroundMap : ... ");

    /// INITIALIZING
    if (mcount1 == 0)
    {
        // for WorldGround, Use WorldSeed
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), GROUNDMAP, FRONTVIEW, map->GetFeatures(FrontView_ViewId), true);
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), CAVEMAP, INNERVIEW, map->GetFeatures(InnerView_ViewId), true);
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), TERRAINMAP, FRONTVIEW, map->GetTerrainMap(), false);
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), BIOMEMAP, FRONTVIEW, map->GetBiomeMap(), false);

        genWorld->SetParamsInt(map->GetMapGeneratorStatus(), 1, 0);

//        GameHelpers::DumpData(&map->featuredMap_->GetFeatureView(FrontView_ViewId)[0], 0, 2, map->featuredMap_->width_, map->featuredMap_->height_);

        mcount1++;
        mcount2 = map->GetMapCounter(MAP_FUNC3) = map->GetMapCounter(MAP_FUNC4) = 0;
    }

    /// FRONTVIEW WORLDGROUND
    if (mcount1 == 1)
    {
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            if (!genWorld->Generate(map->GetMapGeneratorStatus(), timer, delay_))
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateWorldGroundMap : map=%s ... Features for FRONTVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }

            map->featuredMap_->CopyFeatureView(FrontView_ViewId, BackGround_ViewId);
        }

        mcount1 = mcount2 = 0;

//        GameHelpers::DumpData(&map->featuredMap_->GetFeatureView(FrontView_ViewId)[0], 0, 2, map->featuredMap_->width_, map->featuredMap_->height_);

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateWorldGroundMap : map=%s ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
        return true;
    }

    return false;
}

bool MapCreator::GenerateAsteroid(MapBase* map, HiresTimer* timer)
{
    int& mcount1 = map->GetMapCounter(MAP_FUNC1);
    int& mcount2 = map->GetMapCounter(MAP_FUNC2);

//    URHO3D_LOGINFOF("MapCreator() - GenerateAsteroid : ... ");

    /// INITIALIZING
    if (mcount1 == 0)
    {
        // for WorldGround, Use WorldSeed
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), GROUNDMAP, FRONTVIEW, map->GetFeatures(0), true);
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), TERRAINMAP, FRONTVIEW, map->GetTerrainMap(), false);
        genWorld->SetGeneratorMap(map->GetMapGeneratorStatus(), BIOMEMAP, FRONTVIEW, map->GetBiomeMap(), false);

        genWorld->SetParamsInt(map->GetMapGeneratorStatus(), 1, 0);

        mcount1++;
        mcount2 = map->GetMapCounter(MAP_FUNC3) = map->GetMapCounter(MAP_FUNC4) = 0;
    }

    /// INNERVIEW/OUTERVIEW GROUND
    if (mcount1 == 1)
    {
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            if (!genWorld->Generate(map->GetMapGeneratorStatus(), timer, delay_))
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateAsteroid : map=%s ... Features for OUTERVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }

//            map->featuredMap_->CopyFeatureView(1, 0);
        }

        mcount1 = mcount2 = 0;

//        GameHelpers::DumpData(&map->featuredMap_->GetFeatureView(1)[0], 0, 2, map->featuredMap_->width_, map->featuredMap_->height_);

        map->skinnedMap_->SetAtlas(World2DInfo::currentAtlas_);
        map->skinnedMap_->SetNumViews(1);

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateAsteroid : map=%s ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
        return true;
    }

    return false;
}


/// BACKGROUNDVIEW GENERATING FUNCTIONS

bool MapCreator::GenerateBackGroundMap(MapBase* map, HiresTimer* timer)
{
    int& mcount1 = map->GetMapCounter(MAP_FUNC1);
    int& mcount2 = map->GetMapCounter(MAP_FUNC2);

    if (mcount1 == 0)
    {
//        URHO3D_LOGINFOF("MapCreator() - GenerateBackGroundMap : map=%s Copy FrontView ... timer=%d/%d msec ...",
//                        map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));

        // copy frontview in background
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
            map->featuredMap_->CopyFeatureView(FrontView_ViewId, BackGround_ViewId);

        // add some random blocks
        genMaze->SetGeneratorMap(map->GetMapGeneratorStatus(), GROUNDMAP, BACKGROUND, map->GetFeatures(BackGround_ViewId));

        const GeneratorParams* gparams = info_->GetMapGenParams(BACKGROUND);
        if (gparams)
            genMaze->SetParamsInt(map->GetMapGeneratorStatus(), gparams->params_);
        else
        {
            // Bug dans le SetParams par args values
            // => remplacement par PODVector methode
            PODVector<int> defaultParams;
            defaultParams.Resize(4);
            defaultParams[0] = 3;
            defaultParams[1] = 4;
            defaultParams[2] = 2;
            defaultParams[3] = 1;

            genMaze->SetParamsInt(map->GetMapGeneratorStatus(), defaultParams);
//            genMaze->SetParamsInt(4, 3, 4, 2, true);
        }

        mcount1++;
        mcount2 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateBackGroundMap : map=%s ... Copy FrontView ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount1 == 1)
    {
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            bool state = genMaze->Generate(map->GetMapGeneratorStatus(), timer, delay_);
            if (!state)
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateBackGroundMap : map=%s ... Generate Maze", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }
        }

        mcount1++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateBackGroundMap : map=%s ... Generate Maze ... time=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount1 > 1)
    {
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateBackGroundMap : map=%s ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

#ifdef DUMP_MAPDEBUG_BACKGROUND
        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
        GameHelpers::DumpData(&map->featuredMap_->GetFeatureView(BackGround_ViewId)[0], 0, 2, map->featuredMap_->width_, map->featuredMap_->height_);
        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
#endif

        return true;
    }

    return false;
}

/// INNERVIEW GENERATING FUNCTIONS

bool MapCreator::GenerateDungeonMap(MapBase* map, HiresTimer* timer)
{
    int& mcount = map->GetMapCounter(MAP_FUNC1);

//    URHO3D_LOGINFOF("MapCreator() - GenerateDungeonMap : ... MAP_FUNC1=%d info_=%u", mcount, info_);

    /// DUNGEON INNERVIEW
    if (mcount == 0)
    {
        int innerviewid = map->GetViewId(INNERVIEW);
        int frontviewid = map->GetViewId(FRONTVIEW);

        map->SetColliderType(map->GetViewIDs(FRONTVIEW).Size() == 5 ? DUNGEONINNERTYPE : MOBILECASTLETYPE);

        int caveslot = CAVEMAP;
        if (map->GetMapGeneratorStatus().model_ && !map->GetMapGeneratorStatus().model_->HasRenderableModule(caveslot))
        {
            if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER) && innerviewid != frontviewid)
                map->featuredMap_->CopyFeatureView(frontviewid, innerviewid);

            caveslot = GROUNDMAP;
        }

        genDungeon->SetGeneratorMap(map->GetMapGeneratorStatus(), caveslot, INNERVIEW, map->GetFeatures(innerviewid), true, map->GetColliderType() == MOBILECASTLETYPE);

        const GeneratorParams* gparams = info_->GetMapGenParams(INNERVIEW);
        if (gparams)
        {
            genDungeon->SetParamsInt(map->GetMapGeneratorStatus(), gparams->params_);
        }
        else
        {
            // Bug dans le SetParams par args values
            // => remplacement par PODVector methode
            PODVector<int> defaultParams;
            defaultParams.Resize(6);
            defaultParams[0] = Random.Get(1, map->featuredMap_->width_);
            defaultParams[1] = 90;
            defaultParams[2] = 10;
            defaultParams[3] = Random.Get(map->featuredMap_->width_ / 16, map->featuredMap_->width_ / 8);
            defaultParams[4] = Random.Get(map->featuredMap_->width_ / 8, map->featuredMap_->width_ / 4);
            defaultParams[5] = -1; // Random Dungeon Type;

            genDungeon->SetParamsInt(map->GetMapGeneratorStatus(), defaultParams);
        }

        if (map->GetColliderType() == MOBILECASTLETYPE)
        {
            // set the min and max size for rooms
            map->GetMapGeneratorStatus().genparams_[3] = 3;
            map->GetMapGeneratorStatus().genparams_[4] = Min(Min(map->featuredMap_->width_, map->featuredMap_->height_), 8);
            // set the dungeon type to MobileCastle
            map->GetMapGeneratorStatus().genparams_[5] = 3;
        }

        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            genDungeon->Generate(map->GetMapGeneratorStatus(), timer, delay_);
        }

        mcount++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateDungeonMap : map=%s ... Features INNERVIEW ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// DUNGEON BACKVIEW
    if (mcount == 1)
    {
        int innerviewid = map->GetViewId(INNERVIEW);
        int backviewid = map->GetViewId(BACKVIEW);

        if (backviewid == -1)
            URHO3D_LOGERRORF("MapCreator() - GenerateDungeonMap : map=%s ... NO BACKVIEW ... ERROR => DumpViews=%s", map->GetMapPoint().ToString().CString(), map->featuredMap_->GetDumpViewIds().CString());

        if (backviewid != -1 && !map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
            map->featuredMap_->CopyFeatureView(innerviewid, backviewid);

        mcount++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateDungeonMap : map=%s ... Features BACKVIEW ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// DUNGEON OUTERVIEW
    if (mcount == 2)
    {
        int innerviewid = map->GetViewId(INNERVIEW);
        int outerviewid = map->GetViewId(OUTERVIEW);

        if (outerviewid == -1)
            URHO3D_LOGERRORF("MapCreator() - GenerateDungeonMap : map=%s ... NO OUTERVIEW ... ERROR !", map->GetMapPoint().ToString().CString());

        if (outerviewid != -1 && !map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
            map->featuredMap_->CopyFeatureView(innerviewid, outerviewid);

        map->skinnedMap_->SetAtlas(World2DInfo::currentAtlas_);

        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
            map->GetMapData()->skinid_ = GameRand::GetRand(MAPRAND, 100);

        map->skinnedMap_->SetSkin("dungeon", map->GetMapData()->skinid_);

//        map->skinnedMap_->SetSkin("dungeon_06");
        map->skinnedMap_->SetNumViews(map->GetViewIDs(FRONTVIEW).Size());

        mcount++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateDungeonMap : map=%s ... Features OUTERVIEW ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }

    return true;
}


bool MapCreator::GenerateCaveMap(MapBase* map, HiresTimer* timer)
{
    int& mcount = map->GetMapCounter(MAP_FUNC1);

//    URHO3D_LOGINFOF("MapCreator() - GenerateCaveMap : ... MAP_FUNC1=%d", mcount);

    /// CAVE INNERVIEW
    if (mcount == 0)
    {
        map->SetColliderType(CAVEINNERTYPE);

        int caveslot = CAVEMAP;
        if (map->GetMapGeneratorStatus().model_ && !map->GetMapGeneratorStatus().model_->HasRenderableModule(caveslot))
        {
            if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
                map->featuredMap_->CopyFeatureView(FrontView_ViewId, InnerView_ViewId);

            caveslot = GROUNDMAP;
        }

        genGround->SetGeneratorMap(map->GetMapGeneratorStatus(), caveslot, INNERVIEW, map->GetFeatures(InnerView_ViewId));

//        GameHelpers::DumpData(map->GetMapGeneratorStatus().GetMap(1), 0, 2, info_->mapWidth_, info_->mapHeight_);

        map->skinnedMap_->SetAtlas(World2DInfo::currentAtlas_);
//        map->skinnedMap_->SetSkin("ground", GameRand::GetRand(MAPRAND,1000));
//        map->skinnedMap_->SetSkin("ground", 0);
        map->skinnedMap_->SetNumViews(3);

        mcount++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateCaveMap : map=%s ... Features INNERVIEW ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
//    /// CAVE BACKVIEW
//    if (mcount == 1)
//    {
//        map->featuredMap_->SetFeatureView(BackView_ViewId, BACKVIEW, mapFrontView, true);
//        map->skinnedMap_->SetAtlas(World2DInfo::currentAtlas_);
//        map->skinnedMap_->SetSkin("ground_02");
////        map->skinnedMap_->SetRandomSkin("ground");
//        map->skinnedMap_->SetNumViews(map->GetMapCounter(MAP_GENERAL)+mcount);
//
//        mcount++;
//
//        if (TimeOver(timer))
//            return false;
//    }
    return true;
}


/// GENERAL LAYER GENERATING


bool MapCreator::GenerateLayersBase(MapBase* map, HiresTimer* timer)
{
    int& mcount0 = map->GetMapCounter(MAP_GENERAL);
    int& mcount1 = map->GetMapCounter(MAP_FUNC1);

//    URHO3D_LOGINFOF("MapCreator() - GenerateLayersBase ... MAP_GENERAL=%d MAP_FUNC1=%d", mcount0, mcount1);

    /// FRONTVIEW
    if (mcount0 == 0)
    {
        bool state = map->GetMapGeneratorStatus().genFrontFunc_ ? (this->*(map->GetMapGeneratorStatus().genFrontFunc_))(map, timer) : true;
        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - GenerateLayersBase : map=%s ... FRONTVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... FRONTVIEW ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// TERRAINMAP (IF NOT YET GENERATED : needed here for example by MAPMODEL_MOBILECASTLE)
    if (mcount0 == 1)
    {
        if (!map->GetMapGeneratorStatus().GetMap(TERRAINMAP))
        {
            MapGenerator::GenerateDefaultTerrainMap(map->GetMapGeneratorStatus());
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... TERRAINMAP ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// BACKGROUND
    if (mcount0 == 2)
    {
        bool state = map->GetMapGeneratorStatus().genBackFunc_ ? (this->*(map->GetMapGeneratorStatus().genBackFunc_))(map, timer) : true;
        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - GenerateLayersBase : map=%s ... BACKGROUND", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... BACKGROUND ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// Generate Base Topography : helpful for Dungeon Generating
    if (mcount0 == 3)
    {
        if (map->GetType() == Map::GetTypeStatic())
            static_cast<Map*>(map)->mapTopography_.Generate(map->mapStatus_);

        mcount0++;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... Generate Topography ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
        if (TimeOverMaximized(timer))
            return false;
    }
    /// SPECIFIC LAYERS
    if (mcount0 == 4)
    {
        bool state = map->GetMapGeneratorStatus().genInnerFunc_ ? (this->*(map->GetMapGeneratorStatus().genInnerFunc_))(map, timer) : true;
        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - GenerateLayersBase : map=%s ... INNERVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... INNERVIEW ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// Apply Feature Filters
    if (mcount0 == 5)
    {
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            bool state = map->featuredMap_->ApplyFeatureFilters(timer, delay_);

            if (!state)
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateLayersBase : map=%s ... Apply Filters for Feature Maps", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... Apply Filters for Feature Maps ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    /// Add Modified Tiles
    if (mcount0 == 6)
    {
#ifdef HANDLE_TILEMODIFIERS
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_LAYER))
        {
            bool state = map->SetTileModifiers(map->GetMapData()->tilesModifiers_, timer, delay_);
            if (!state)
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateLayersBase : map=%s ... Set Tile Modifiers", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }
        }
#endif
        map->GetMapData()->SetSection(MAPDATASECTION_LAYER);
        map->GetMapData()->SetMapsLinks();

//        map->Dump();

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayersBase : map=%s ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        mcount1 = 0;
        map->featuredMap_->indexToSet_ = 0;

        return true;
    }

    return false;
}

bool MapCreator::GenerateLayers(Map* map, HiresTimer* timer)
{
    int& mcount0 = map->GetMapCounter(MAP_GENERAL);
    int& mcount1 = map->GetMapCounter(MAP_FUNC1);

//    URHO3D_LOGINFOF("MapCreator() - GenerateLayers ... MAP_GENERAL=%u", mcount0);

    if (mcount0 < 7)
    {
        if (!GenerateLayersBase(map, timer))
            return false;

        mcount0++;
        mcount1 = 0;
    }
    /// Set Fluid Cells
    if (mcount0 == 7)
    {
        bool state = map->featuredMap_->SetFluidCells(timer, delay_);
        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - GenerateLayers : map=%s ... Set Fluid Cells", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        map->skinnedMap_->indexToSet_ = 0;

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayers : map=%s ... Set Fluid Cells  ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
    }
    /// Set Skinned Views
    if (mcount0 == 8)
    {
        bool state = map->skinnedMap_->SetViews(timer, delay_);

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - GenerateLayers : map=%s ... Set Skinned Views", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayers : map=%s ... Set Skinned Views ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
    }
    /// Set Mask Views
    if (mcount0 == 9)
    {
        bool state = map->featuredMap_->UpdateMaskViews(MapInfo::info.chinfo_->GetDefaultTileGroup(MapDirection::All), timer, delay_, map->GetConnectedMode());

        if (!state)
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("MapCreator() - GenerateLayers : map=%s ... Set Masked Views", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
            return false;
        }

        mcount0++;
        mcount1 = 0;

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateLayers : map=%s ... Set Masked Views ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
    }
    /// Set Topography
    if (mcount0 > 9)
    {
        mcount0 = 0;
        mcount1 = 0;

//        map->mapTopography_.Generate(map->mapStatus_);

//#ifdef DUMP_MAPCREATOR_LOGS
//        URHO3D_LOGERRORF("MapCreator() - GenerateLayers : map=%s ... Generate Topography ... timer=%d/%d msec ... OK !",
//                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
//#endif

        return true;
    }

    return false;
}


/// GENERAL MAPCOLLIDER GENERATION

bool MapCreator::GenerateColliders(MapBase* map, HiresTimer* timer)
{
#ifdef MAP_GENERATECOLLIDERS

#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_GenerateColliders);
#endif

    if (!map->CreateColliders(timer))
    {
#ifdef DUMP_ERROR_ON_TIMEOVER
        LogTimeOver(ToString("MapCreator() - GenerateColliders : map=%s ... status_=%d", map->GetMapPoint().ToString().CString(), map->GetMapGeneratorStatus().status_), timer, delay_);
#endif
        return false;
    }

#ifdef DUMP_MAPCREATOR_LOGS
    URHO3D_LOGERRORF("MapCreator() - GenerateColliders : map=%s ... timer=%d/%d msec ... OK !",
                     map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
#endif

    return true;
}


/// ENTITIES & FURNITURES GENERATION

bool MapCreator::GenerateEntities(MapBase* map, HiresTimer* timer)
{
    int& mcount0 = map->GetMapCounter(MAP_GENERAL);

//    URHO3D_LOGINFOF("MapCreator() - GenerateEntities ... mcount0=%d ...", mcount0);

    /// INITIALIZE ENTITIES & FURNITURES GENERATORS
    if (mcount0 == 0)
    {
        map->GetMapGeneratorStatus().genFront_ = 0;

        if (map->GetMapGeneratorStatus().GetMap(GROUNDMAP))
        {
            int frontviewid = map->GetViewId(FRONTVIEW);
            if (frontviewid == NOVIEW)
            {
                URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s frontviewid=-1 !", map->GetMapPoint().ToString().CString());
                return true;
            }
            map->GetMapGeneratorStatus().genFront_ = map->GetMapGeneratorStatus().genFrontFunc_ == &MapCreator::GenerateWorldGroundMap ? GEN_WORLD : GEN_CAVE;
            // reset slot GROUNDMAP to FRONTVIEW
            generators_[map->GetMapGeneratorStatus().genFront_]->SetGeneratorMap(map->GetMapGeneratorStatus(), GROUNDMAP, FRONTVIEW, map->GetFeatures(frontviewid), true);
            generators_[map->GetMapGeneratorStatus().genFront_]->ClearSpots();
            generators_[map->GetMapGeneratorStatus().genFront_]->ClearFurnitures();
        }

        map->GetMapGeneratorStatus().genInner_ = map->GetMapGeneratorStatus().genInnerFunc_ == &MapCreator::GenerateDungeonMap ? GEN_DUNGEON : GEN_CAVE;
        // Be careful to keep Room Spots from DungeonGenerator
        generators_[map->GetMapGeneratorStatus().genInner_]->ClearSpots();
        generators_[map->GetMapGeneratorStatus().genInner_]->ClearFurnitures();

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... FRONTVIEW=%s INNERVIEW=%s timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), typenames_[map->GetMapGeneratorStatus().genFront_].CString(), typenames_[map->GetMapGeneratorStatus().genInner_].CString(),  timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

#if defined(HANDLE_ENTITIES)
        mcount0 = 1;
#elif defined(HANDLE_FURNITURES)
        mcount0 = 3;
#else
        mcount0 = 7;
#endif
    }
#ifdef HANDLE_ENTITIES
    /// ENTITIES in FRONTVIEW
    if (mcount0 == 1)
    {
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_ENTITY) && map->GetMapGeneratorStatus().genFront_ > 0 && map->AllowEntities(FRONTVIEW))
        {
            // don't forget to active the good slot
            map->GetMapGeneratorStatus().activeslot_ = GROUNDMAP;

            bool state = generators_[map->GetMapGeneratorStatus().genFront_]->GenerateSpots(map->GetMapGeneratorStatus(), timer, delay_);

            if (!state)
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateEntities : map=%s ... FRONTVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }

#ifdef DUMP_MAPCREATOR_LOGS
            URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... FRONTVIEW spotsize=%u timer=%d/%d msec ... OK !",
                             map->GetMapPoint().ToString().CString(), generators_[map->GetMapGeneratorStatus().genFront_]->GetSpots().Size(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
        }

        mcount0++;

        if (TimeOverMaximized(timer))
            return false;
    }
    /// ENTITIES in INNERVIEW
    if (mcount0 == 2)
    {
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_ENTITY) && map->GetMapGeneratorStatus().genInner_ > 0 && map->AllowEntities(INNERVIEW))
        {
            // don't forget to active the good slot
            map->GetMapGeneratorStatus().activeslot_ = CAVEMAP;

            bool state = generators_[map->GetMapGeneratorStatus().genInner_]->GenerateSpots(map->GetMapGeneratorStatus(), timer, delay_);

            if (!state)
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateEntities : map=%s ... INNERVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }
        }

#ifdef HANDLE_FURNITURES
        mcount0 = 3;
#else
        mcount0 = 5;
#endif

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... INNERVIEW spotsize=%u timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), generators_[map->GetMapGeneratorStatus().genInner_]->GetSpots().Size(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
#endif
#ifdef HANDLE_FURNITURES
    /// FURNITURES in INNERVIEW (in FIRST)
    if (mcount0 == 3)
    {
        if (map->GetMapGeneratorStatus().genInner_ == GEN_DUNGEON && !map->GetMapData()->IsSectionSet(MAPDATASECTION_FURNITURE))
        {
            // don't forget to active the good slot
            map->GetMapGeneratorStatus().activeslot_ = CAVEMAP;

            bool state = generators_[map->GetMapGeneratorStatus().genInner_]->GenerateFurnitures(map->GetMapGeneratorStatus(), timer, delay_);

            if (!state)
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("MapCreator() - GenerateEntities : map=%s ... Furniture INNERVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                return false;
            }

#ifdef DUMP_MAPCREATOR_LOGS
            URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... Furniture INNERVIEW ... timer=%d/%d msec ... OK !",
                             map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
        }
        mcount0++;
    }
    /// FURNITURES in FRONTVIEW (in SECOND to restore MapTopoGraphy)
    if (mcount0 == 4)
    {
        if (map->GetMapGeneratorStatus().genFront_ > 0)
        {
            if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_FURNITURE))
            {
                // don't forget to active the good slot
                map->GetMapGeneratorStatus().activeslot_ = GROUNDMAP;

                bool state = generators_[map->GetMapGeneratorStatus().genFront_]->GenerateFurnitures(map->GetMapGeneratorStatus(), timer, delay_);

                if (!state)
                {
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("MapCreator() - GenerateEntities : map=%s ... Furniture FRONTVIEW", map->GetMapPoint().ToString().CString()), timer, delay_);
#endif
                    return false;
                }

#ifdef DUMP_MAPCREATOR_LOGS
                URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... Furniture FRONTVIEW ... timer=%d/%d msec ... OK !",
                                 map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
            }
            // 04/04/2023 : maptopography FreeSideTiles must be set in any case (MapTopography used by DrawableScroller)
            else if (map->GetType() == Map::GetTypeStatic())
            {
                static_cast<Map*>(map)->GetTopography().GenerateFreeSideTiles(map->GetMapGeneratorStatus(), FRONTVIEW);
            }
        }

        mcount0++;
    }
#endif
    if (mcount0 == 5)
    {
#ifdef MAP_ACTIVE_POPULATE
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_SPOT))
        {
            if (map->GetMapGeneratorStatus().genFront_ > 0)
                map->AddSpots(generators_[map->GetMapGeneratorStatus().genFront_]->GetSpots(), false);
            if (map->GetMapGeneratorStatus().genInner_ > 0)
                map->AddSpots(generators_[map->GetMapGeneratorStatus().genInner_]->GetSpots(), false);

            map->GetMapData()->SetSection(MAPDATASECTION_SPOT);

//            URHO3D_LOGINFOF("MapCreator() - GenerateEntities : map=%s ... Add Spots ... spotsize=%u OK !", map->GetMapPoint().ToString().CString(), map->GetSpots().Size());
        }

        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_ENTITY))
        {
            map->PopulateEntities(numEntities_, authorizedCategories_);

            map->GetMapData()->SetSection(MAPDATASECTION_ENTITY);

//            URHO3D_LOGINFOF("MapCreator() - GenerateEntities : map=%s ... Populate Entities ... OK !", map->GetMapPoint().ToString().CString());
        }

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... Populate Entities ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
#endif
        mcount0++;

        if (TimeOverMaximized(timer))
            return false;
    }
    if (mcount0 == 6)
    {
#ifdef HANDLE_FURNITURES
        if (!map->GetMapData()->IsSectionSet(MAPDATASECTION_FURNITURE))
        {
            if (map->GetMapGeneratorStatus().genInner_ > 0)
                map->AddFurnitures(generators_[map->GetMapGeneratorStatus().genInner_]->GetFurnitures());
            if (map->GetMapGeneratorStatus().genFront_ > 0)
                map->AddFurnitures(generators_[map->GetMapGeneratorStatus().genFront_]->GetFurnitures());

            map->GetMapData()->SetSection(MAPDATASECTION_FURNITURE);

//            URHO3D_LOGINFOF("MapCreator() - GenerateEntities : map=%s ... Add Furnitures... OK !", map->GetMapPoint().ToString().CString());
        }
        // 04/04/2023 : maptopography FreeSideTiles must be set in any case (MapTopography used by DrawableScroller)
        else if (map->GetType() == Map::GetTypeStatic())
        {
            static_cast<Map*>(map)->GetTopography().GenerateFreeSideTiles(map->GetMapGeneratorStatus(), INNERVIEW);
        }

#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("MapCreator() - GenerateEntities : map=%s ... Add Furnitures ... timer=%d/%d msec ... OK !",
                         map->GetMapPoint().ToString().CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay_/1000));
#endif
#endif
        mcount0++;
    }
    if (mcount0 == 7)
    {
        mcount0 = 0;
        return true;
    }

    return false;
}

void MapCreator::Dump() const
{
    unsigned i = 0;
    for (Vector<ShortIntVector2>::ConstIterator it = mapsToCreate_.Begin(); it != mapsToCreate_.End(); ++it, ++i)
        URHO3D_LOGINFOF("mapsToCreate_[%u] : mPoint=%s", i, it->ToString().CString());
}
