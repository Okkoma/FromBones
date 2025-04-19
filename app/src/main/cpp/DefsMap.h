#pragma once

#include <Urho3D/IO/AbstractFile.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>

#include "ShortIntVector2.h"

#include "GameOptions.h"

#include "DefsViews.h"
#include "DefsColliders.h"

#include "MapFeatureTypes.h"
#include "MapTiles.h"

#include "GameRand.h"
#include "Spline2D.h"
#include "Libs/AccidentalNoise/Imaging/mapping.h"

using namespace Urho3D;

struct ChunkInfo;
struct ObjectFeatured;
class MapBase;
class Map;
struct MapTerrain;
class World2DInfo;
class TerrainAtlas;
class RenderShape;
class AnlWorldModel;

/// CONSTANTS

#define MAP_BACKGROUND2 5.f
#define MAP_BACKGROUND1 2.f
#define MAP_GROUND 0.f
#define MAP_FOREGROUND1 -1.f
#define MAP_FOREGROUND2 -2.f
#define MAP_NUMMAXVIEWS 5
#define MAP_NUMMAXCOLLIDERS 7
#define MAP_NUMMAXRENDERCOLLIDERS 6
#define MAP_BASEDRAWORDER 1003

#define ANIMATEDTILE_DEFAULT_TICKDELAY 25
#define ANIMATEDTILE_DEFAULT_LOOPDELAY ANIMATEDTILE_DEFAULT_TICKDELAY

//const long long MAP_MAXDELAYUPDATE = 6667;
const long long MAP_MAXDELAY = 4500;
const long long MAP_MAXDELAY_ASYNCLOADING = 45000;
//const long long MAP_MAXDELAY_ASYNCLOADING = 100000;
const long long MAP_MAXDELAY_NOASYNCLOADING = 60000;

/* 1 sec = 1000msec = 1000000 usec

    200 frames/sec = 5000 usec / frame
    150 frames/sec = 6667 usec / frame
    75 frames/sec  = 13333 usec / frame
*/

static const ShortIntVector2 UNDEFINED_MAPPOINT(-1000, -1000);

static const unsigned NoMarkPattern    = 0xffff;
static const unsigned blockMapBorderID = 0xffff;
static const unsigned MapEntityDefaultObject = StringHash("GOT_Goblin").Value();

/// Minimap
static const int MINIMAP_PIXBYTILE = 3;
static const int MiniMapSelectedViewIndex[2][4] = { { 2, 1, 0, 0 }, { 4, 3, 1, 0 } };
static const unsigned MiniMapColorFrontBlocked = Color(145.f / 255, 124.f / 255, 111.f / 255, 1.f).ToUInt();
static const unsigned MiniMapColorBack1Blocked = Color(108.f/255, 93.f/255, 83.f/255, 1.f).ToUInt();
static const unsigned MiniMapColorBack2Blocked = Color(72.f/255, 62.f/255, 55.f/255, 1.f).ToUInt();
static const unsigned MiniMapColorBack3Blocked = Color(36.f/255, 31.f/255, 28.f/255, 1.f).ToUInt();
//static const unsigned MiniMapColorFrontBlocked = Color(160.f / 255, 75.f / 255, 15.f / 255, 1.f).ToUInt();
//static const unsigned MiniMapColorBack1Blocked = Color(90.f/255, 35.f/255, 10.f/255, 1.f).ToUInt();
//static const unsigned MiniMapColorBack2Blocked = Color(60.f/255, 25.f/255, 5.f/255, 1.f).ToUInt();
//static const unsigned MiniMapColorBack3Blocked = Color(50.f/255, 15.f/255, 0.f/255, 1.f).ToUInt();
static const unsigned MiniMapColorEmpty = Color(0.f, 0.f, 0.f, 0.f).ToUInt();
static const unsigned MiniMapColorsByLayer[4] = { MiniMapColorFrontBlocked, MiniMapColorBack1Blocked, MiniMapColorBack2Blocked, MiniMapColorBack3Blocked };

/// TYPES DEFINES

// skinData = for each feature type is corresponding a terrain
typedef HashMap<FeatureType, MapTerrain* > SkinData;
typedef PODVector<FeatureType> FeaturedMap;
typedef PODVector<ConnectIndex> ConnectedMap;
typedef PODVector<Tile* > TiledMap;

typedef anl::SMappingRanges AnlMappingRange;

const float MAPMASS = 100.f;

/// ENUMERATIONS

enum Direction
{
    NoneDir = 0,
    LeftDir = 1 << 0,                   // 1
    RightDir = 1 << 1,                  // 2
    LeftRightDir = LeftDir | RightDir,  // 3
    UpDir = 1 << 2,                     // 4
    DownDir = 1 << 3,                   // 8
    AllDir = 15
};

enum AreaFlag
{
    nomoveFlag         = 0x0000,
    walkableFlag       = 0x0001,
    jumpableRightFlag  = 0x0002,
    jumpableLeftFlag   = 0x0004,
    jumpableFlag       = jumpableRightFlag | jumpableLeftFlag,
    flyableFlag        = 0x0008,
    swimmableFlag      = 0x0010
};

enum WallType
{
    Wall_Ground = 0,
    Wall_Border,
    Wall_Roof,
};

enum MapTerrainProperty
{
    TERRAINPROPERTY_BIOME = 0x1,
};

enum NeighborMode
{
    Connected4 = 0,
    Connected8,
    Connected0
};

enum MapType
{
    GROUNDMAP = 0,
    CAVEMAP = 1,
    TERRAINMAP = 2,
    BIOMEMAP = 3,
    POPULATIONMAP = 4,
    TESTMAP = 5,
    MAXMAPTYPES
};

enum MapTypeGenMode
{
    TILES = 0,
    ENTITIES
};

enum MapGenMode
{
    GENERATED   = 0,
    FROMMEMORY  = 1,
    FROMBINFILE = 2,
    FROMXMLFILE = 3
};

enum MapGeneratorType
{
    GEN_RANDOM  = 0,
    GEN_DUNGEON = 1,
    GEN_CAVE  = 2,
    GEN_MAZE = 3,
    GEN_WORLD  = 4,
    GEN_ASTEROID = 5,
    GEN_MOBILECASTLE = 6,
};

//enum MapMode
//{
//    MapNotSet = 0,
//    MapFileDatMode,
//    MapFileXmlMode,
//    MapFileTmxMode,
//    MapCreateMode,
//    MapMemoryMode,
//};

enum MapStatus
{
    Uninitialized = 0,

    Initializing = 1,
    Loading_Map,

    Generating_Map,
    Creating_Map_Layers,
    Creating_Map_Colliders,
    Creating_Map_Entities,
    Setting_Map,
    Setting_Layers,
    Setting_Colliders,
    Updating_ViewBatches,
    Updating_RenderShapes,
    Setting_Areas,
    Setting_BackGround,
    Setting_MiniMap,
    Setting_Furnitures,
    Setting_Entities,
    Setting_Borders,
    Available,

    Unloading_Map,
    Saving_Map
};

enum MapProgressCounterId
{
    MAP_GENERAL = 0,
    MAP_FUNC1,
    MAP_FUNC2,
    MAP_FUNC3,
    MAP_FUNC4,
    MAP_FUNC5,
    MAP_FUNC6,
    MAP_VISIBILITY,
};

enum MapSpotType
{
    SPOT_NONE = 0,
    SPOT_ROOM = 1,
    SPOT_TUNNEL = 2,
    SPOT_CAVE = 3,
    SPOT_LAND = 4,
    SPOT_LIQUID = 5,
    SPOT_START = 6,
    SPOT_EXIT = 7,
    SPOT_BOSS = 8
};

/// MapData Section
enum
{
    MAPDATASECTION_LAYER = 0,
    MAPDATASECTION_TILEMODIFIER,
    MAPDATASECTION_FLUIDVALUE,
    MAPDATASECTION_SPOT,
    MAPDATASECTION_ZONE,
    MAPDATASECTION_NODEIDS,
    MAPDATASECTION_FURNITURE,
    MAPDATASECTION_ENTITY,
    MAPDATASECTION_ENTITYATTR,
    MAPDATASECTION_MAX
};

/// Asynchronous serializing state of a MapData
enum MapAsynState
{
    /// No async operation in progress.
    MAPASYNC_NONE = 0,
    /// Queued for asynchronous loading.
    MAPASYNC_LOADQUEUED = 1,
    /// Queued for asynchronous saving.
    MAPASYNC_SAVEQUEUED = 2,
    /// In progress of calling MapData::Load() in a thread.
    MAPASYNC_LOADING = 3,
    /// In progress of calling MapData::Save() in a thread.
    MAPASYNC_SAVING = 4,
    /// Load() succeeded.
    MAPASYNC_LOADSUCCESS = 5,
    /// Load() failed.
    MAPASYNC_LOADFAIL = 6,
    /// Save() succeeded.
    MAPASYNC_SAVESUCCESS = 7,
    /// Save() failed.
    MAPASYNC_SAVEFAIL = 8
};


/// CLASSES DEFINITIONS

struct MapTerrain
{
    MapTerrain();
    MapTerrain(const String& name, unsigned property=0, const Color& color = Color::WHITE);
    MapTerrain(const MapTerrain& t);

    void AddCompatibleFeature(FeatureType feature)
    {
        if (!compatibleFeatures_.Contains(feature)) compatibleFeatures_.Push(feature);
    }

    void AddConnectIndexToTileGid(int gid, ConnectIndex connectIndex);
    void AddGidToDimension(int gid, int dimension);
    void AddConnectIndexToDecalGid(int gid, ConnectIndex connectIndex);
    void AddCompatibleFurniture(int furniture, ConnectIndex connectIndex=0);
    void SetColor(const Color& color)
    {
        color_ = color;
    }

    const String& GetName() const
    {
        return name_;
    }
    const Color& GetColor() const
    {
        return color_;
    }
    unsigned GetProperty() const
    {
        return property_;
    }

    inline Vector<int>& GetTileGidTableForConnectValue(int connectValue)
    {
        return tileGidConnectivity_[MapTilesConnectType::GetIndex(connectValue)];
    }
    inline const Vector<int>& GetTileGidTableForConnectValue(int connectValue) const
    {
        return tileGidConnectivity_[MapTilesConnectType::GetIndex(connectValue)];
    }
    inline Vector<int>& GetTileGidTableForConnectIndex(ConnectIndex connectIndex)
    {
        return tileGidConnectivity_[connectIndex];
    }
    inline const Vector<int>& GetTileGidTableForConnectIndex(ConnectIndex connectIndex) const
    {
        return tileGidConnectivity_[connectIndex];
    }
    inline Vector<int>& GetDecalGidTableForConnectIndex(ConnectIndex connectIndex)
    {
        return decalGidConnectivity_[connectIndex];
    }
    inline const Vector<int>& GetDecalGidTableForConnectIndex(ConnectIndex connectIndex) const
    {
        return decalGidConnectivity_[connectIndex];
    }

    int GetRandomTileGidForConnectValue(int connectValue) const;
    int GetRandomTileGidForConnectIndex(ConnectIndex connectIndex) const;
    int GetRandomDecalGidForConnectIndex(ConnectIndex connectIndex) const;
    int GetRandomTileGidForDimension(int dimension) const;
    int GetRandomFurniture(ConnectIndex connectIndex=0) const;

    bool operator == (const MapTerrain& rhs) const
    {
        return name_ == rhs.name_;
    }
    bool HasCompatibleFeature(FeatureType feature) const
    {
        return compatibleFeatures_.Contains(feature);
    }
    bool UseDimensionTiles() const
    {
        return useDimension_;
    }
    bool UseDecals() const
    {
        return decalGidConnectivity_.Size() > 0;
    }

//    static const MapTerrain EMPTY;

    void Dump() const;

private :

    String name_;
    unsigned property_;
    Color color_;
    bool useDimension_;

    Vector<FeatureType> compatibleFeatures_;

    // tile gids applicables par connectivite (ConnectIndex) (top, bottom ...)
    Vector<Vector<int > > tileGidConnectivity_;
    Vector<Vector<int > > decalGidConnectivity_;
    // tile gids applicables par dimension (1x1, 1x2, 2x1 ...)
    Vector<Vector<int > > tileGidByDimension_;
    // fournitures par connectivite (ConnectIndex)
    Vector<Vector<int > > compatibleFurnitures_;

    GameRand& Random;
};

struct MapSkin
{
    String name_;
    NeighborMode neighborMode_;
    SkinData skinData_;
};

struct MapInfo
{
    static void Reset();
    static void Initialize(int mapwidth, int mapheight, int chunknumx, int chunknumy, float tilewidth, float tileheight, float mwidth, float mheight);

    ChunkInfo* chinfo_;
    int width_;
    int height_;
    unsigned mapsize_;
    float tileWidth_;
    float tileHeight_;
    Vector2 tileHalfSize_;

    float mWidth_;
    float mHeight_;
    Vector2 mTileHalfSize_;

    short int neighborInd[9];
    short int neighbor4Ind[4];
    short int cornerInd[4];

    static const short int neighborOffX[9];
    static const short int neighborOffY[9];
//    static const short int neighborCardX[4];
//    static const short int neighborCardY[4];

    static MapInfo info;
};

class MapCreator;
typedef bool (MapCreator::*genFuncPtr)(MapBase*, HiresTimer*);

struct MapGeneratorStatus
{
    void ResetCounters(int startindex=0);

    void Clear();

    int GetActiveViewZIndex()
    {
        return viewZindexes_[activeslot_];
    }
    FeatureType* GetMap(int index)
    {
        return (FeatureType*)features_[index];
    }

    void Dump() const;
    String DumpMapCounters() const;

    int status_;
    int x_, y_;
    int width_, height_;
    int mapcount_[8];
//	int viewZindex_;
    int time_;
    bool genSpots_;
    ShortIntVector2 mappoint_;

    int generator_;
    int genFront_, genInner_;
    genFuncPtr genFrontFunc_, genBackFunc_, genInnerFunc_;

    unsigned wseed_, cseed_, rseed_;
    PODVector<int> genparams_;
    PODVector<float> genparamsf_;

    AnlWorldModel* model_;
    AnlMappingRange mappingRange_;

    int activeslot_;
    void* currentMap_;
    Vector<int> viewZindexes_;
    Vector<void*> features_;

    MapBase* map_;
};

struct MapTopography
{
    MapTopography();

    void Clear();

    void Generate(MapGeneratorStatus& genStatus);
    void GenerateFreeSideTiles(MapGeneratorStatus& genStatus, int viewZ);
    void GenerateWorldEllipseInfos(MapGeneratorStatus& genStatus);

    int GetFloorTileY(int tilex) const;
    int GetEllipseTileY(int tilex) const;
    float GetFloorY(float x) const;

    float GetEllipseY(int tilex) const;
    float GetEllipseY(float x) const;
    float GetY(float x) const;

#ifdef USE_TOPOGRAPHY_BACKCURVEFLOOR
    Vector2 GetBackFloorPoint(float t) const;
    float GetBackFloorNearestYAt(float x) const;
#endif

    bool HasNoFreeSides() const
    {
        return !freeSideTiles_[0].Size() && !freeSideTiles_[1].Size() && !freeSideTiles_[2].Size() && !freeSideTiles_[3].Size();
    }
//    bool IsFullSky() const { return HasNoFreeSides() && !numGroundTiles_; }
//    bool IsFullGround() const { return HasNoFreeSides() && numGroundTiles_; }

    // Evaluate on Background view only
    bool IsFullSky() const
    {
        return fullSky_;
    }
    bool IsFullGround() const
    {
        return fullGround_;
    }

    bool IsWorldEllipseVisible() const
    {
        return worldEllipseVisible_;
    }
    bool IsWorldEllipseOver() const
    {
        return worldEllipseOver_;
    }

    int flatmode_, generatedView_;
    unsigned numGroundTiles_;
    float groundDensity_;
    Vector2 maporigin_;
    bool generatedBackCurves_;
    bool worldEllipseVisible_, worldEllipseOver_;
    bool fullSky_, fullGround_;

#ifdef USE_TOPOGRAPHY_BACKCURVEFLOOR
    Spline2D backFloorCurve_;
#endif

    PODVector<unsigned> freeSideTiles_[4];
    PODVector<int> floorTileY_;
    PODVector<int> ellipseTileY_;
    PODVector<float> ellipseY_;
};

struct MapDirection
{
    static int Inverse(int dir)
    {
        switch (dir)
        {
        case North :
            return South;
        case South :
            return North;
        case East :
            return West;
        case West :
            return East;
        default :
            return North;
        }
    }
    enum
    {
        North = 0,
        South,
        East,
        West,
        NoBorders,
        AllBorders,
        All,
    };
    static const char* GetName(int dir)
    {
        return dir < 7 ? names_[dir] : 0;
    }
    static int GetDx(int dir) { return sDirectionDx_[dir]; }
    static int GetDy(int dir) { return sDirectionDy_[dir]; }

    static const char* names_[7];
    static const int sDirectionDx_[4];
    static const int sDirectionDy_[4];
};

struct MapSpot
{
    MapSpot(MapSpotType type, int centerx, int centery, int viewZindex, int width=1, int height=1, int geom=0) :
        type_(type), position_(centerx, centery), viewZIndex_(viewZindex), width_(width), height_(height), geom_(geom) { }

    MapSpotType type_;

    IntVector2 position_;
    int viewZIndex_;
    int width_, height_, geom_;

    String GetName(int index) const;

    static void GetSpotsOfType(MapSpotType type, const PODVector<MapSpot>& spots, PODVector<const MapSpot*>& typedspots);
    static int GetNumSpotsOfType(const PODVector<MapSpot>& spots, MapSpotType type);
    static bool HasSpotOfType(const PODVector<MapSpot>& spots, MapSpotType type);
    static unsigned GetRandomEntityForSpotType(const MapSpot& spot, const Vector<StringHash>& authorizedCategories);
};

struct TileModifier
{
    TileModifier() { }
    TileModifier(int x, int y, int z, FeatureType feat, FeatureType orig) : x_(x), y_(y), z_(z), feat_(feat), oFeat_(orig) { }
    TileModifier(const TileModifier& t) : value_(t.value_), oFeat_(t.oFeat_) { }

    bool operator == (const TileModifier& rhs) const
    {
        return x_ == rhs.x_ && y_ == rhs.y_ && z_ == rhs.z_;
    }
    bool operator != (const TileModifier& rhs) const
    {
        return x_ != rhs.x_ || y_ != rhs.y_ || z_ != rhs.z_;
    }

    unsigned ToHash() const
    {
        return (unsigned) ( (z_ | 0xffff00) & (y_ << 8 | 0xff00ff) & (x_ << 16 | 0x00ffff) );
    }

    static unsigned GetKey(int x, int y, int z)
    {
        return (unsigned) ( (z | 0xffff00) & (y << 8 | 0xff00ff) & (x << 16 | 0x00ffff) );
    }

    union
    {
        struct
        {
            unsigned value_;
        };
        struct
        {
            unsigned char x_;
            unsigned char y_;
            unsigned char z_;
            FeatureType feat_;
        };
    };

    FeatureType oFeat_;
};

struct FeatureFilterInfo
{
    int filter_;
    FeatureType feature1_;
    FeatureType feature2_;
    FeatureType featureBack_;
    FeatureType featureReplace_;
    unsigned viewId1_;
    unsigned viewId2_;
    Vector<unsigned> *storeIndexes_;
};

enum MapModelType
{
    MAPMODEL_DUNGEON = 0,
    MAPMODEL_CAVE = 1,
    MAPMODEL_ASTEROID = 2,
    MAPMODEL_BACKASTEROID = 3,
    MAPMODEL_MOBILECASTLE = 4,
    MAPMODEL_NUM,
};

struct MapModel
{
public:
    MapModel() { }

//private:
    /// Views
    unsigned numviews_;
    // correspondance des viewId et des viewZ
    HashMap<unsigned, unsigned> viewZ_;
    // table des id feature views (tries suivant Z ascendant) pour chaque zview
    HashMap<unsigned, Vector<int> > viewIds_;
    // table des id fluid views pour chaque zview standard
    HashMap<unsigned, Vector<int> > fluidViewIds_;
    // table des feature filters pour chaque view (key1=viewID key2=filterID data=FeatureInfos)
    Vector<FeatureFilterInfo > featurefilters_;

    /// View Generator
    AnlWorldModel* anlModel_;

    /// Physic/Render Parameters
    bool hasPhysics_;
    bool colliderBodyRotate_;
    int colliderBodyType_, colliderType_, genType_;

    Vector<ColliderParams > physicsParameters_;
    Vector<ColliderParams > rendersParameters_;
};


/// MapData : serializable datas for a map

enum EntityDataDrawableFlag
{
    FlagLayerZIndex_ = 0x1F,
    FlagRotate_      = 1 << 5,
    FlagFlipX_       = 1 << 6,
    FlagFlipY_       = 1 << 7,
};

// 16 values for entityid, 4 flags
enum EntityTypeFlag
{
    EntityValue      = 0xF,
    MaxEntityIds     = EntityValue + 1,

    SetEntityFlag     = 1 << 4,
    BossFlag          = 1 << 5,
    RandomEntityFlag  = 1 << 6,
    RandomMappingFlag = 1 << 7,

    EntityFlags		  = SetEntityFlag | BossFlag | RandomEntityFlag | RandomMappingFlag,
};

struct EntityData                   // 8 bytes
{
    EntityData() : gotindex_(0U), tileindex_(USHRT_MAX) { }
    EntityData(const StringHash& got, const Vector2& position, int layerZ);

    void Clear() { gotindex_ = 0U; tileindex_ = USHRT_MAX; properties_[1] = 0U; }
    void Set(unsigned short gotindex=0, short unsigned tileindex=USHRT_MAX, signed char tilepositionx=0, signed char tilepositiony=0, unsigned char sstype=RandomEntityFlag|RandomMappingFlag, int layerZ=0, bool rotate=false, bool flipX=false, bool flipY=false);

    void SetDrawableProps(int layerZ, bool rotate=false, bool flipX=false, bool flipY=false);
    void SetLayerZ(int layerZ);
    void SetPositionProps(const Vector2& positionInTile, ContactSide anchor=NoneSide);
    Vector2 GetNormalizedPositionInTile() const;
    int GetLayerZIndex() const
    {
        return (drawableprops_ & FlagLayerZIndex_);
    }
    unsigned char GetEntityValue() const
    {
        return sstype_ & EntityValue;
    }
    bool IsBoss() const
    {
        return sstype_ & BossFlag;
    }

    String Dump() const;

    union
    {
        struct
        {
            unsigned int properties_[2];
        };
        struct
        {
            // entity type
            short unsigned gotindex_;       // 2 bytes // got
            // map position
            short unsigned tileindex_;      // 2 bytes // 0 to 65535 (max 256x256 tiles in a map)
            // inside tile position
            signed char tilepositionx_;     // 1 bytes // -127 to 127 (-127 = rightalign  ; center = 0; 127 = leftalign)
            signed char tilepositiony_;     // 1 bytes // -127 to 127 (-127 = bottomalign ; center = 0; 127 = topalign)
            // entity animation index
            unsigned char sstype_;  	    // 1 bytes // entityid
            // hardcoded drawable props
            unsigned char drawableprops_;   // 1 bytes // bit1-5=layerzindex; bit6=rotate; bit7=flipx; bit8=flipy;
        };
    };


};

struct ZoneData
{
    bool IsInside(const IntVector2& position, int zindex) const;

    int GetNumPlayersInside() const;

    // zone type
    unsigned char type_;
    // position of the zone center
    unsigned char centerx_;
    unsigned char centery_;
    unsigned char zindex_;

    // shape info : geomid, height and width
    int shapeinfo_;
    void SetShapeInfo(int geom, int width, int height)
    {
        shapeinfo_ = (geom << 16) + (width << 8) + height;
    }
    int GetShape() const
    {
        return (shapeinfo_ >> 16) & 0xFF;
    }
    int GetWidth() const
    {
        return (shapeinfo_ >> 8) & 0xFF;
    }
    int GetHeight() const
    {
        return shapeinfo_ & 0xFF;
    }

    // states
    unsigned lastVisit_;
    unsigned lastEnabled_;
    unsigned nodeid_;
    int state_;

    // actions applied to the zone
    unsigned goInAction_;
    unsigned goOutAction_;

    // entity type
    unsigned char entityIndex_;
    unsigned char sstype_;
};

class MapData
{
    friend class MapBase;
    friend class Map;

public:
    MapData();
    ~MapData();

    void Clear();

    void SetMap(MapBase* base);
    void SetMapsLinks();
    void SetSection(int sid, bool ok=true);

    void CopyPrefabLayer(int i);
    void CopyPrefabLayers();
    void UpdatePrefabLayer(int i);

    EntityData* GetEntityData(Node* node);
    int GetEntityDataID(EntityData* entitydata, bool furnituretype=true);
    void AddEntityData(Node* node, EntityData* entitydata=0, bool forceToSet=true, bool priorizeEntityData=false);
    void RemoveEntityData(EntityData* entitydata, bool furnituretype);
    void RemoveEntityData(Node* node);
    void RemoveEffectActions();

    bool Load(Deserializer& source);
    bool Save(Serializer& destination);

    bool IsLoaded() const
    {
        return dataSize_ != 0U;
    }
    bool IsPrefab() const
    {
        return prefab_;
    }
    bool IsSectionSet(int sid) const
    {
        return sectionSet_[sid];
    }
    bool IsBeingSerialized() const
    {
        return state_ >= MAPASYNC_NONE;
    }
    unsigned GetDataSize() const
    {
        return dataSize_;
    }
    unsigned GetCompressedDataSize() const
    {
        return compresseddataSize_;
    }

    void Dump() const;

    MapBase* map_;

    /// Map data states
    String mapfilename_;
    MapAsynState state_;
    int savedmapstate_;

    /// Serialized datas
    // map informations
    ShortIntVector2 mpoint_;
    int width_, height_;
    unsigned numviews_;
    unsigned seed_;
    int collidertype_;
    int gentype_;
    int skinid_;
    int prefab_;

    // the features ref for each featured view (indexed by viewid) + terrain + biome maps
    Vector<FeaturedMap* > maps_;

    // the tile modifiers to apply to the featured views
    PODVector<TileModifier > tilesModifiers_;

    // the fluidcell values by indexfluidview
    Vector<PODVector<FeatureType> > fluidValues_;

    // the spots informations
    PODVector<MapSpot > spots_;

    // the furnitures informations
    PODVector<EntityData > furnitures_;
    // the entities informations
    PODVector<EntityData > entities_;

    // the zone informations
    PODVector<ZoneData > zones_;

    // the entity ids
    PODVector<unsigned> entitiesIds_;
    // the entities attributes
    Vector<NodeAttributes > entitiesAttributes_;

protected :
    void PurgeEntityDatas();
    void SetEntityData(Node* node, EntityData* entitydata, bool priorizeEntityData=false);
    void UpdateEntityNode(Node* node, EntityData* entitydata);

    bool sectionSet_[MAPDATASECTION_MAX];
    unsigned dataSize_;
    unsigned compresseddataSize_;

private :
    // the ObjectInfos for Furnitures & Entities
    HashMap<unsigned, EntityData* > entityInfos_;
    PODVector<EntityData* > freeFurnitureDatas_;
    PODVector<EntityData* > freeEntityDatas_;

    /// Features Storage for Prefab
    Vector<FeaturedMap > prefabMaps_;
};
