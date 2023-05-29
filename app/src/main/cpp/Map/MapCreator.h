#pragma once

#include "DefsMap.h"

#include "GameRand.h"


class World2DInfo;
class Map;
class MapStorage;
class MapGenerator;

using namespace Urho3D;

class MapCreator : public Object
{
    URHO3D_OBJECT(MapCreator, Object);

public:
    MapCreator(Context* context);
    ~MapCreator();

    static void InitTable();
    static void DeInitTable();

    static MapCreator* Get()
    {
        return creator_;
    }
    static MapGenerator* GetGenerator(int gentype)
    {
        return generators_[gentype];
    }
    static const String& GetTypeName(unsigned type)
    {
        return typenames_[typeindexes_[type]];
    }
    static const unsigned GEN_RANDOM_TYPE;
    static const unsigned GEN_DUNGEON_TYPE;
    static const unsigned GEN_CAVE_TYPE;
    static const unsigned GEN_MAZE_TYPE;
    static const unsigned GEN_WORLD_TYPE;
    static const unsigned GEN_ASTEROID_TYPE;
    static const unsigned GEN_MOBILECASTLE_TYPE;

    void SetWorldInfo(const World2DInfo* info);
    void SetDefaultGenerator(const String& name);
    void SetDefaultGenerator(unsigned type=GEN_RANDOM_TYPE);
    void SetGenerator(MapGeneratorStatus& genStatus, MapGeneratorType type=GEN_RANDOM);
    void SetNumEntities(const IntVector2& numEntities=IntVector2::ZERO);
    void SetAuthorizedCategories(const String& value);

    void AddMapToCreate(const ShortIntVector2& mpoint);

    void PurgeMap(const ShortIntVector2& mpoint);
    void PurgeMapsOutsideVisibleAreas();

    bool CreateMap(Map* map, HiresTimer* timer=0, const long long& delay=0);
    bool Update(HiresTimer* timer=0, const long long& delay=0);

    bool IsRunning() const
    {
        return mapsToCreate_.Size() > 0;
    }

    void Dump() const;

    bool GenerateLayersBase(MapBase* map, HiresTimer* timer=0);
    bool GenerateColliders(MapBase* map, HiresTimer* timer=0);
    bool GenerateEntities(MapBase* map, HiresTimer* timer=0);

private:
    bool GenerateSimpleGroundMap(MapBase* map, HiresTimer* timer=0);
    bool GenerateWorldGroundMap(MapBase* map, HiresTimer* timer=0);
    bool GenerateAsteroid(MapBase* map, HiresTimer* timer=0);
    bool GenerateBackGroundMap(MapBase* map, HiresTimer* timer=0);
    bool GenerateDungeonMap(MapBase* map, HiresTimer* timer=0);
    bool GenerateCaveMap(MapBase* map, HiresTimer* timer=0);

    bool GenerateLayers(Map* map, HiresTimer* timer=0);

    const World2DInfo* info_;

    MapGeneratorType defaultGenerator_;

    IntVector2 numEntities_;
    float simpleGroundLevel_;
    long long delay_;
    bool sortMaps_;

    Vector<ShortIntVector2> mapsToCreate_;
    Vector<StringHash> authorizedCategories_;

    GameRand& Random;

    static MapCreator* creator_;
    static HashMap<unsigned, int> typeindexes_;
    static Vector<String> typenames_;
    static Vector<MapGenerator* > generators_;
};

