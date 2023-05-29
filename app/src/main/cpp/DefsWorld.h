#pragma once

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Graphics/DebugRenderer.h>

namespace Urho3D
{
class Node;
class Material;
}

using namespace Urho3D;

#include "DefsGame.h"
#include "DefsEntityInfo.h"

#include "TerrainAtlas.h"
#include "AnlWorldModel.h"


#define WORLD_MAP_WIDTH 64
#define WORLD_MAP_HEIGHT 64
//#define WORLD_TILE_WIDTH 64.f * PIXEL_SIZE
//#define WORLD_TILE_HEIGHT 64.f * PIXEL_SIZE
#define WORLD_TILE_WIDTH 32.f * PIXEL_SIZE
#define WORLD_TILE_HEIGHT 32.f * PIXEL_SIZE

#define WORLD_TINY_1 IntRect(0,0,1,1)
#define WORLD_TINY_2 IntRect(-1,-1,1,1)
#define WORLD_SMALL IntRect(-5,-5,5,5)
#define WORLD_MEDIUM IntRect(-10,-10,10,10)
#define WORLD_HUGE IntRect(-20,-20,20,20)
#define WORLD_INFINITE IntRect(0,0,0,0)

static const IntVector2 UndefinedMapPosition(-1,-1);
static const float VisibleRectOverscan = 4.f;

//const float WorldMappingStep = 0.5f;

struct AngleInfo
{
    Vector2 angle_;
    float pente_;
};

struct EllipseW
{
    EllipseW() { }
    EllipseW(const Vector2& center, const Vector2& radius) : center_(center), radius_(radius), aabb_(center - radius, center + radius) { }

    void Define(const Vector2& center, const Vector2& radius)
    {
        center_ = center;
        radius_ = radius;
        aabb_.Define(center - radius, center + radius);
    }

    void GenerateShape(int numpoints=64);

    inline float ClampX(float x) const { return Clamp(x, center_.x_ - radius_.x_, center_.x_ + radius_.x_); }
    inline float ClampY(float y) const { return Clamp(y, center_.y_ - radius_.y_, center_.y_ + radius_.y_); }
    inline bool IsInBounds(float x) const { return x >= center_.x_ - radius_.x_ && x <= center_.x_ + radius_.x_; }

    inline float GetMinX() const { return center_.x_ - radius_.x_; }
    inline float GetMaxX() const { return center_.x_ + radius_.x_; }
    inline float GetMinY() const { return center_.y_ - radius_.y_; }
    inline float GetMaxY() const { return center_.y_ + radius_.y_; }

    float GetX(float y) const;
    float GetY(float x) const;

    inline float GetDeltaX(float y) const
    {
        y -= center_.y_;
        return (radius_.x_/radius_.y_) * sqrtf(radius_.y_*radius_.y_ - y*y);
    }

    inline float GetDeltaY(float x) const
    {
        x -= center_.x_;
        return (radius_.y_/radius_.x_) * sqrtf(radius_.x_*radius_.x_ - x*x);
    }

    inline void GetPointsInRectAtX(float x, const Rect& rect, Vector<Vector2>& points) const;
    inline void GetPointsInRectAtY(float y, const Rect& rect, Vector<Vector2>& points) const;
    void GetIntersections(const Rect& rect, Vector<Vector2>& points, bool sortX) const;

    void GetPositionOn(float xi, float sidey, Vector2& position);
    void GetPositionOn(float x, float sidey, Vector2& position, Vector2& angleinfo);
    void GetPositionOnShape(float x, float sidey, Vector2& position);
    void GetPositionOnShape(float x, float sidey, Vector2& position, Vector2& angleinfo);

    void DrawDebugGeometry(DebugRenderer* debug, const Color& color, bool depthTest);

    Vector2 center_;
    Vector2 radius_;
    Rect aabb_;

    PODVector<float> ellipseXValues_;
    PODVector<Vector2> ellipseShape_;
    PODVector<AngleInfo> ellipseAngles_;
};

struct GeneratorParams
{
    PODVector<int>   params_;
    String ToString() const;
};


class World2DInfo
{
public :
    World2DInfo() :
        mapWidth_(WORLD_MAP_WIDTH), mapHeight_(WORLD_MAP_HEIGHT),
        tileWidth_(WORLD_TILE_WIDTH), tileHeight_(WORLD_TILE_HEIGHT),
        mWidth_(WORLD_MAP_WIDTH*WORLD_TILE_WIDTH), mHeight_(WORLD_MAP_HEIGHT*WORLD_TILE_HEIGHT),
        mTileWidth_(WORLD_TILE_WIDTH), mTileHeight_(WORLD_TILE_HEIGHT), mTileRect_(0.f, 0.f, WORLD_TILE_WIDTH, WORLD_TILE_HEIGHT), simpleGroundLevel_(50),
        isScaled_(false), defaultGenerator_(0), forcedShapeType_(true),
        shapeType_(SHT_CHAIN), addObject_(true), addFurniture_(true), storageDir_(ATLASSETDIR), atlasSetFile_(ATLASSETDEFAULT),
        worldModelFile_(String::EMPTY), wBounds_(WORLD_INFINITE), node_(0) { ; }

    World2DInfo(const String& storageDir, float tileWidth=WORLD_TILE_WIDTH, float tileHeight=WORLD_TILE_HEIGHT,
                const String& atlasFile = ATLASSETDEFAULT, const String& worldModel = String::EMPTY, const IntRect& wBounds=WORLD_INFINITE,
                int mapWidth=WORLD_MAP_WIDTH, int mapHeight=WORLD_MAP_HEIGHT,
                bool forcedShapeType=true, ColliderShapeTypeMode shapeType=SHT_CHAIN, bool addObject=true, bool addFurniture=true, unsigned defaultGenerator=0) :
        mapWidth_(mapWidth), mapHeight_(mapHeight), tileWidth_(tileWidth), tileHeight_(tileHeight),
        mWidth_(mapWidth*tileWidth), mHeight_(mapHeight*tileHeight), mTileWidth_(tileWidth), mTileHeight_(tileHeight), mTileRect_(0.f, 0.f, tileWidth, tileHeight),
        simpleGroundLevel_(50), isScaled_(false), defaultGenerator_(defaultGenerator),
        forcedShapeType_(forcedShapeType), shapeType_(shapeType), addObject_(addObject), addFurniture_(addFurniture), storageDir_(storageDir),
        atlasSetFile_(atlasFile), worldModelFile_(worldModel), wBounds_(wBounds), node_(0) { ; }

    World2DInfo(const World2DInfo& winfo);

    ~World2DInfo() { ; }

    World2DInfo& operator=(const World2DInfo& rhs);

    void Update(const Vector3& scale = Vector3::ONE);

    void SetMapSize(int mapWidth, int mapHeight);
    void SetGroundLevel(int groundLevel)
    {
        simpleGroundLevel_ = groundLevel;
    }
    void ParseParams(const String& params);

    void ClearImageLayer()
    {
        imageLayerResources_.Clear();
        imageLayerHotSpots_.Clear();
    }

    const GeneratorParams* GetMapGenParams(int z) const;

    /// Get Position2D from Tile Index Position
    Vector2 GetPosition(int x, int y) const;
    /// Get Position2D from Tile Index Position (World Scaled)
    Vector2 GetWorldPositionFromMapCenter(int x, int y, bool centertile=false) const;
    /// Get Centerd World Position from map point
    Vector2 GetCenteredWorldPosition(const ShortIntVector2& mPoint) const;
    /// Return the corresponding Map Point for the given world position
    ShortIntVector2 GetMapPoint(const Vector2& position) const;
    short int GetMapPointCoordX(float x) const;
    short int GetMapPointCoordY(float y) const;

    /// Convert position to tile index position.
    void Convert2WorldTileCoords(const Vector2& position, int& x, int& y) const;
    void Convert2WorldTileIndex(const ShortIntVector2& mPoint, const Vector2& position, unsigned& index) const;
    /// Convert World Map Position to world position.
    void Convert2WorldPosition(const ShortIntVector2& mPoint, const IntVector2& mPosition, Vector2& position) const;
    void Convert2WorldPosition(const ShortIntVector2& mPoint, const IntVector2& mPosition, const Vector2& posInTile, Vector2& position) const;
    void Convert2WorldPosition(const ShortIntVector2& mPoint, const Vector2& mPosition, Vector2& position) const;
    void Convert2WorldPosition(const WorldMapPosition& wmPosition, Vector2& position);
    void UpdateWorldPosition(WorldMapPosition& wmPosition);
    /// Convert world position to world Map point
    void Convert2WorldMapPoint(const Vector2& worldPosition, ShortIntVector2& mPoint) const;
    void Convert2WorldMapPoint(float worldX, float worldY, ShortIntVector2& mPoint) const;
    /// Convert world position to world Map position decomposed variables
    void Convert2WorldMapPosition(const ShortIntVector2& mPoint, float worldX, float worldY, IntVector2& mPosition) const;
    void Convert2WorldMapPosition(const ShortIntVector2& mPoint, const Vector2& worldPosition, IntVector2& mPosition) const;
    void Convert2WorldMapPosition(const ShortIntVector2& mPoint, const Vector2& worldPosition, IntVector2& mPosition, Vector2& tilePosition) const;
    /// Convert world position to world Map position
    void Convert2WorldMapPosition(float worldX, float worldY, WorldMapPosition& wmPosition) const;
    void Convert2WorldMapPosition(const Vector2& worldPosition, WorldMapPosition& wmPosition) const;
    void Convert2WorldMapPosition(const Vector2& worldPosition, WorldMapPosition& wmPosition, Vector2& positionInTile) const;
    void Convert2WorldMapPosition(const Vector2& worldPosition, ShortIntVector2& mPoint, IntVector2& mPosition) const;
    void Convert2WorldMapPosition(const Vector2& worldPosition, ShortIntVector2& mPoint, unsigned& tileindex) const;

    unsigned GetTileIndex(int xi, int yi) const;
    unsigned GetTileIndex(const IntVector2& mPosition) const;
    IntVector2 GetTileCoords(unsigned tileindex) const;

    /// Get Position In a Tile (in Scaled Tile Size) for a Given World Position2D
    Vector2 GetPositionInTile(const Vector2& worldposition) const;

    /// Convert a Map IntRect to World Map Coords
    void ConvertMapRect2WorldRect(const IntRect& rect, Rect& wRect) const;
    /// Convert a Tile IntRect to World Map Coords
    void Convert2WorldRect(const IntRect& rect, const Vector2& startwposition, Rect& wRect) const;

    void Dump() const;

    /// dimension of a map in tiles
    int mapWidth_, mapHeight_;
    /// dimension of a tile in urho3D scene units
    float tileWidth_, tileHeight_;

    /// dimension of a map in urho3D scene units with scale world node
    float mWidth_, mHeight_;
    float mTileWidth_, mTileHeight_;
    Rect mTileRect_;

    int simpleGroundLevel_;
    bool isScaled_;
    Vector2 worldScale_;

    bool forcedShapeType_;
    ColliderShapeTypeMode shapeType_;
    bool addObject_;
    bool addFurniture_;
    String storageDir_;
    String atlasSetFile_;
    String worldModelFile_;

    IntRect wBounds_;

    EllipseW worldGroundRef_, worldAtmosphere_, worldHillTop_, worldGround_, worldCenter_;

    Node* node_;
    Vector<ResourceRef > imageLayerResources_;
    Vector<Vector2> imageLayerHotSpots_;

    unsigned defaultGenerator_;
    HashMap<int, GeneratorParams > genParams_;

    SharedPtr<AnlWorldModel> worldModel_;
    SharedPtr<TerrainAtlas> atlas_;

    static WeakPtr<TerrainAtlas> currentAtlas_;
    static long long delayUpdateUsec_;
    static const String ATLASSETDIR;
    static const String ATLASSETDEFAULT;
    static const String WORLDMODELDEFAULT;

    static SharedPtr<Material> WATERMATERIAL_TILE;
    static SharedPtr<Material> WATERMATERIAL_REFRACT;
    static SharedPtr<Material> WATERMATERIAL_LINE;
};


inline bool HalfTimeOver(HiresTimer* timer, const long long& delay=World2DInfo::delayUpdateUsec_)
{
    if (!timer)
        return false;

    return timer->GetUSec(false) > delay/2;
}

inline bool TimeOverMaximized(HiresTimer* timer, const long long& delay=World2DInfo::delayUpdateUsec_)
{
    if (!timer)
        return false;

    return delay <= MAP_MAXDELAY || timer->GetUSec(false) > delay;
}

inline bool TimeOver(HiresTimer* timer, const long long& delay=World2DInfo::delayUpdateUsec_)
{
    if (!timer)
        return false;

    return timer->GetUSec(false) > delay;
}

inline void LogTimeOver(const String& str, HiresTimer* timer, const long long& delay=World2DInfo::delayUpdateUsec_)
{
    if (timer->GetUSec(false) >= delay + delay/2)
        URHO3D_LOGERRORF("%s ... TimeOver=%d/%d msec ...", str.CString(), timer ? (int)(timer->GetUSec(false) / 1000) : 0, (int)(delay/1000));
}
