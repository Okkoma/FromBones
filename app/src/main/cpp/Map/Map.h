#pragma once

#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Resource/Resource.h>

#include "DefsCore.h"
#include "DefsGame.h"
#include "DefsMap.h"
#include "DefsFluids.h"
#include "DefsEntityInfo.h"
#include "GameOptionsTest.h"

#ifdef USE_TILERENDERING
#include "ObjectTiled.h"
#else
#include "ObjectSkinned.h"
#endif // USE_TILERENDERING



namespace Urho3D
{
class Serializer;
class Deserializer;
class File;
class TmxFile2D;
class TmxLayer2D;
class TmxTileLayer2D;
class Image;
class XMLFile;
class VectorBuffer;
class CollisionBox2D;
class CollisionChain2D;
}

using namespace Urho3D;

class MapCreator;
class MapData;
class MapStorage;
class MapColliderGenerator;
class World2DInfo;
struct ChunkInfo;
class ViewManager;
class GOC_Destroyer;


class FROMBONES_API MapBase
{
    friend class Map;
    friend class ObjectMaped;
    friend class MapCreator;
    friend class MapStorage;

public:
    MapBase();

    virtual StringHash GetType() const = 0;

    void SetSerializable(bool state);
    void SetSize(int width, int height);
    void SetStatus(int status);
    void SetMapPoint(const ShortIntVector2& mpoint)
    {
        mapStatus_.mappoint_ = mpoint;
    }
    void SetMapData(MapData* mapdata)
    {
        mapData_ = mapdata;
    }
    void SetMapModel(MapModel* model)
    {
        mapModel_ = model;
    }

    /// SETTERS
    // Collider Setters
    void SetPhysicProperties(BodyType2D type, float mass, int grpindex = -1, bool allowrotation = false);
    void SetColliderType(ColliderType collidertype);
    void SetColliderParameters(MapColliderType paramtype, int num, const ColliderParams* params);

    // Entity Setters
    void AddSpots(const PODVector<MapSpot>& spots, bool adjustPositions);
    void PopulateEntities(const IntVector2& numEntities, const Vector<StringHash>& authorizedCategories);

    // Furniture Setters
    void SetFurnitureNode(Node* node)
    {
        nodeFurniture_ = node;
    };
    void AddFurnitures(const PODVector<EntityData>& furnitures);
    Node* AddFurniture(const EntityData& furniture);
    Node* AddFurniture(EntityData& furniture);
    void SetFurnitures();
    bool SetFurnitures(HiresTimer* timer, bool checkusable);
    bool AnchorEntityOnTileAt(EntityData& entitydata, Node* node=0, bool isabiome=true, bool checkpositionintile=true);
    // just for test : to remove after test
    void RefreshEntityPosition(Node* node);

    // Tile Modifiers
    bool FindSolidTile(unsigned index, FeatureType& feat, int& layerZ);
    bool FindSolidTile(unsigned index, int viewZ);
    bool FindEmptyTile(unsigned index, FeatureType& feat, int& layerZ);
    bool FindEmptySpace(int numtilex, int numtiley, int viewId, IntVector2& position, int maxAcceptedBlocks=0);
    void FindTileIndexesWithPixelShapeAt(bool block, int centerx, int centery, int viewid, int pixelshapeid, Vector<unsigned>& tileindexes);
    bool CanSetTile(FeatureType feat, int x, int y, int viewZ, bool permutesametiles=false);
    void SetTile(FeatureType feat, int x, int y, int viewZ, Tile** removedtile=0);
    void SetTiles(FeatureType feat, int viewZ, const Vector<unsigned>& tileIndexes);
    void SetTiles(const PODVector<TileModifier>& tileModifiers);
    bool SetTileEntity(FeatureType feature, unsigned tileindex, int viewZ, bool dynamic=false);
    bool SetTileModifiers(const PODVector<TileModifier>& tileModifiers, HiresTimer* timer=0, long long delay=0L);
    void SetCachedTileModifiers(const PODVector<TileModifier>& tileModifiers);
    void GetCachedTileModifiers(PODVector<TileModifier>& tileModifiers);

    void SetTileModifiersDirty(bool dirty) { tileModifiersDirty_ = dirty; }
    void SetEntitiesDirty(bool dirty) { entitiesDirty_ = dirty; }
    bool IsTileModifiersDirty() const { return tileModifiersDirty_; }
    bool IsEntitiesDirty() const { return entitiesDirty_; }

protected:
    virtual void OnTileModified(int x, int y) { }

    // MapData Updaters
    bool UpdateMapData(HiresTimer* timer);
    virtual bool OnUpdateMapData(HiresTimer* timer);

private:
    // Colliders Generators
    bool CreateColliders(HiresTimer* timer, bool setPhysic=true);
    bool GenerateColliders(HiresTimer* timer, bool setPhysic=true);
    bool UpdatePhysicColliders(HiresTimer* timer);
    bool UpdatePhysicCollider(PhysicCollider& collider, int x=-1, int y=-1, bool box=false);
    bool UpdatePhysicCollider(PhysicCollider& collider, const Vector<unsigned>& tileIndexes, bool box=false);

    bool UpdateRenderShapes(HiresTimer* timer);
    bool UpdateRenderColliders(HiresTimer* timer);
    void UpdateRenderCollider(RenderCollider& collider);

    bool UpdateAllColliders(HiresTimer* timer);

    // Shape Setters
    bool SetCollisionShapes(HiresTimer* timer);
    bool AddCollisionChain2D(HiresTimer* timer);
    bool AddCollisionBox2D(HiresTimer* timer);
    bool AddCollisionEdge2D(HiresTimer* timer);
    bool SetCollisionChain2D(PhysicCollider& collider, HiresTimer* timer=0);
    bool UpdateCollisionChain(PhysicCollider& collider, unsigned tileindex);
    bool UpdateCollisionBox(PhysicCollider& collider, unsigned tileindex, bool add, bool addnode=false, bool dynamic=false);
    bool UpdatePlateformBoxes(PhysicCollider& collider, unsigned tileindex, bool add);
    bool UpdatePlateformBoxes(PhysicCollider& collider, const Vector<unsigned>& addedtiles, const Vector<unsigned>& removedtiles);

protected:
    virtual void OnPhysicsSetted() { }

public:
    /// GETTERS
    virtual bool AllowEntities(int viewZ) const
    {
        return true;
    }
    virtual bool AllowFurnitures(int viewZ) const
    {
        return true;
    }
    virtual bool AllowDynamicFurnitures(int viewZ) const
    {
        return true;
    }

    bool IsSerializable() const
    {
        return serializable_;
    }

    // Entities Getters
//    const PODVector<WorldEntityInfo>& GetEntityInfos() const { return mapData_->entities_; }
    const PODVector<MapSpot>& GetSpots() const
    {
        return mapData_->spots_;
    }

    // Furniture Getters
    void GetFurnitures(const IntRect& rect, PODVector<EntityData>& furnitures, bool newOrigin);

    // Tile Modifier Getters
    const TileModifier* GetTileModifier(int x, int y, int viewZ) const;

    // Status / Info Getters
    int GetStatus() const
    {
        return mapStatus_.status_;
    }
    MapGeneratorStatus& GetMapGeneratorStatus()
    {
        return mapStatus_;
    }
    int& GetMapCounter(MapProgressCounterId id)
    {
        return mapStatus_.mapcount_[id];
    }
    bool IsAvailable() const
    {
        return mapStatus_.status_ == Available;
    }
    unsigned GetMapSeed() const
    {
        return mapStatus_.wseed_+mapStatus_.cseed_;
    }
    unsigned GetWorldSeed() const
    {
        return mapStatus_.wseed_;
    }
    const ShortIntVector2& GetMapPoint() const
    {
        return mapStatus_.mappoint_;
    }
    MapData* GetMapData() const
    {
        return mapData_;
    }
    virtual bool IsEffectiveVisible() const
    {
        return true;
    }
    virtual bool IsVisible() const
    {
        return true;
    }

    // Coordinate Getters
    inline int GetWidth() const
    {
        return mapStatus_.width_;
    }
    inline int GetHeight() const
    {
        return mapStatus_.height_;
    }
    inline unsigned GetSize() const
    {
        return GetWidth() * GetHeight();
    }
    inline unsigned GetTileIndex(int x, int y) const
    {
        return y * GetWidth() + x;
    }
    inline int GetTileCoordX(unsigned index) const
    {
        return index%GetWidth();
    }
    inline int GetTileCoordY(unsigned index) const
    {
        return index/GetWidth();
    }
    inline IntVector2 GetTileCoords(unsigned index) const
    {
        return IntVector2(index%GetWidth(), index/GetWidth());
    }
    Vector2 GetWorldTilePosition(const IntVector2& coord, const Vector2& localoffset=Vector2::ZERO) const;
    Vector2 GetPositionInTile(const Vector2& worldposition, unsigned tileindex);
    void GetMapTilePosition(const Vector2& worldposition, unsigned tileindex, Vector2& localoffset);
    unsigned GetTileIndexAt(const Vector2& worldposition) const;
    IntVector2 GetTileCoords(const Vector2& worldposition) const
    {
        return GetTileCoords(GetTileIndexAt(worldposition));
    }

    void GetBlockPositionAt(int direction, int viewid, IntVector2& position, int bound=-1);

    // Node Getters
    Node* GetRootNode() const
    {
        return nodeRoot_;
    }
    Node* GetStaticNode() const
    {
        return nodeStatic_;
    }
    Node* GetFurnitureNode() const
    {
        return nodeFurniture_;
    }

    // Map Objects Getters
    ObjectFeatured* GetObjectFeatured() const
    {
        return featuredMap_.Get();
    }
    ObjectSkinned* GetObjectSkinned() const
    {
        return skinnedMap_.Get();
    }
#ifdef USE_TILERENDERING
    ObjectTiled* GetObjectTiled() const
    {
        return objectTiled_.Get();
    }
#endif

    // Collider Getters
    int GetColliderType() const
    {
        return colliderType_;
    }
    int GetColliderIndex(unsigned viewZ) const
    {
        return featuredMap_->GetColliderIndex(viewZ);
    }
    int GetColliderIndex(unsigned viewZ, int viewId) const
    {
        return featuredMap_->GetColliderIndex(viewZ, viewId);
    }
    void GetColliders(MapColliderType type, int indv, PODVector<MapCollider*>& mapcolliders);
    void GetCollidersAtZIndex(MapColliderType type, int indz, PODVector<MapCollider*>& mapcolliders);
    const Vector<PhysicCollider >& GetPhysicColliders() const
    {
        return physicColliders_;
    }
    const Vector<RenderCollider >& GetRenderColliders() const
    {
        return renderColliders_;
    }

    BoundingBox GetWorldBoundingBox2D();

    // Views Getters
    const Vector<int>& GetViewIDs(int viewZ) const
    {
        return featuredMap_->GetViewIDs(viewZ);
    }
    int GetViewId(unsigned viewZ) const
    {
        return featuredMap_->GetViewId(viewZ);
    }
    int GetNearestViewId(unsigned layerZ) const
    {
        return featuredMap_->GetNearestViewId(layerZ);
    }
    int GetViewZ(int viewId) const
    {
        return featuredMap_->GetViewZ(viewId);
    }
    const FeaturedMap& GetFeatureView(int viewId) const
    {
        return featuredMap_->GetFeatureView(viewId);
    }
    FeaturedMap& GetFeaturedMap(int viewId)
    {
        return featuredMap_->GetFeatureView(viewId);
    }
//    const FeaturedMap& GetFeatureViewAtZ(int layerZ) const { return featuredMap_->GetFeatureView(featuredMap_->GetViewId(layerZ)); }
    const FeaturedMap& GetFeatureViewAtZ(int layerZ) const
    {
        return featuredMap_->GetFeatureView(featuredMap_->GetNearestViewId(layerZ));
    }
    const FeaturedMap& GetMaskedView(unsigned indexZ, unsigned viewId) const
    {
        return featuredMap_->GetMaskedView(indexZ, viewId);
    }
    FluidDatas& GetFluidDatas(int indexFluidZ)
    {
        return featuredMap_->GetFluidView(indexFluidZ);
    }
    FeatureType* GetTerrainMap() const;
    FeatureType* GetBiomeMap() const;
    bool CanSwitchViewZ() const
    {
        return canSwitchViewZ_;
    }
    bool IsMaskedByOtherMap(int viewZ) const;
//    bool IsMaskedBy(MapBase* map, int viewZ) const;
//    IntRect GetRectIn(MapBase* map) const;
    unsigned GetNumViews() const
    {
        return skinnedMap_->GetNumViews();
    }
    const TiledMap& GetTiledView(int viewId) const
    {
        return skinnedMap_->GetTiledView(viewId);
    }
    TiledMap& GetTiledView(int viewId)
    {
        return skinnedMap_->GetTiledView(viewId);
    }
    const Vector<TiledMap>& GetTiledViews() const
    {
        return skinnedMap_->GetTiledViews();
    }
    ConnectedMap& GetConnectedView(int viewId)
    {
        return skinnedMap_->GetConnectedView(viewId);
    }
    const Vector<ConnectedMap>& GetConnectedViews() const
    {
        return skinnedMap_->GetConnectedViews();
    }
    NeighborMode GetConnectedMode() const
    {
        return skinnedMap_->GetSkin() ? skinnedMap_->GetSkin()->neighborMode_ : Connected0;
    }
    virtual Map* GetConnectedMap(int direction) const
    {
        return 0;
    }

    // Features Getters
    FeatureType* GetFeatures(int viewId) const
    {
        return &featuredMap_->GetFeatureView(viewId)[0];
    }
    inline FeatureType GetFeature(unsigned index, int viewId) const
    {
        return featuredMap_->GetFeatureView(viewId)[index];
    }
    inline FeatureType& GetFeatureRef(unsigned index, int viewId)
    {
        return featuredMap_->GetFeatureView(viewId)[index];
    }
    FeatureType GetFeatureType(unsigned index, int viewId) const
    {
        return GetFeature(index, viewId);
    }
    FeatureType GetFeatureAtZ(unsigned index, int layerZ) const;
    FluidCell* GetFluidCellPtr(unsigned index, int indexZ) const;

    // Tile Getters
    bool HasTileProperties(unsigned index, int viewId, unsigned properties) const;
    bool HasTileProperties(int x, int y, int viewId, unsigned properties) const;
    unsigned GetTileProperties(unsigned index, int viewId) const;
    unsigned GetTilePropertiesAtZ(unsigned index, int viewZ) const;
    unsigned GetTileProperties(int x, int y, int viewId) const;
    unsigned char GetTerrain(unsigned index, int viewId) const;
    Tile* GetTile(unsigned index, int viewId) const;

    // Area Properties
    unsigned GetAreaProps(unsigned index, int indexZ, int jumpheight=3) const;

    inline bool AreCoordsOutside(int x, int y) const
    {
        if (x < 0) return true;
        if (y < 0) return true;
        if (x >= featuredMap_->width_) return true;
        if (y >= featuredMap_->height_) return true;
        return false;
    }

    void Dump() const;

    static long long& delayUpdateUsec_;

private:
    inline Tile* InGetTile(unsigned index, int viewId) const
    {
        return skinnedMap_->GetTiledView(viewId)[index];
    }
    inline Tile* InGetTile(int x, int y, int viewId) const
    {
        return skinnedMap_->GetTiledView(viewId)[GetTileIndex(x, y)];
    }
    inline int InGetGid(unsigned index, int viewId) const
    {
        Tile* tile = skinnedMap_->GetTiledView(viewId)[index];
        return tile ? tile->GetGid() : 0;
    }
    inline int InGetGid(int x, int y, int viewId) const
    {
        Tile* tile = skinnedMap_->GetTiledView(viewId)[GetTileIndex(x, y)];
        return tile ? tile->GetGid() : 0;
    }

private:
    static const unsigned gotStart;
    static const unsigned gotExit;

    MapGeneratorStatus mapStatus_;

    MapData* mapData_;
    MapModel* mapModel_;

    // Nodes
    WeakPtr<Node> nodeRoot_;
    SharedPtr<Node> nodeFurniture_;
    WeakPtr<Node> nodeStatic_;
    Vector<Node* > nodeBoxes_;
    Vector<Node* > nodePlateforms_;
    Vector<Node* > nodeChains_;

    // Map Objects
#ifdef USE_TILERENDERING
    SharedPtr<ObjectTiled> objectTiled_;
    WeakPtr<ObjectSkinned> skinnedMap_;
#else
    SharedPtr<ObjectSkinned> skinnedMap_;
#endif
    WeakPtr<ObjectFeatured> featuredMap_;

    // Physic/Render Parameters
    BodyType2D colliderBodyType_;
    int colliderType_;
    float colliderBodyMass_;
    bool colliderBodyRotate_;
    int colliderGroupIndex_;
    int colliderNumParams_[2];
    const ColliderParams* colliderParams_[2];

    bool canSwitchViewZ_;
    bool skipInitialTopBorder_;
    bool serializable_;
    bool tileModifiersDirty_, entitiesDirty_;

    Vector2 center_;

    // Physics Colliders for each view
    Vector<PhysicCollider> physicColliders_;
//#ifdef USE_RENDERCOLLIDERS
    /// Render Colliders for each view
    Vector<RenderCollider> renderColliders_;
//#endif

    // Shapes
    PODVector<CollisionBox2D*> sboxes_;
    PODVector<CollisionBox2D*> splateforms_;
    PODVector<CollisionChain2D*> schains_;

    // Cached Tile Modifiers
    List<TileModifier > cacheTileModifiers_;
};


/// => Sender : Map
/// => Subscribers : GOC_Portal
URHO3D_EVENT(MAP_AVAILABLE, Map_Available) { }

enum
{
    MAP_NOVISIBLE = 0,
    MAP_VISIBLE = 1,
    MAP_CHANGETOVISIBLE = 2,
    MAP_CHANGETONOVISIBLE = 3,
};


class FROMBONES_API Map : public Object, public MapBase
{
    URHO3D_OBJECT(Map, Object);

    friend class MapCreator;
    friend class MapStorage;
    friend struct MapTopography;

public:
    Map(Context* context);
    Map();
    ~Map();

    static void Initialize(World2DInfo* info, ViewManager* viewManager, int chunknumx=0, int chunknumy=0);
    static void Reset();
    static Rect GetMapRect(const ShortIntVector2& mpoint);

    /// INITIALIZERS
public:
    void Resize();
    bool Clear(HiresTimer* timer=0);
    void PullFluids();
    void Initialize(const ShortIntVector2& mPoint, unsigned wseed);
private:
    void Init();

    /// SETTERS
public:
    void SetTagNode(Node* node);
    bool Set(Node* node, HiresTimer* timer);

private:
    // Collider Setters
public:
private:
    void CreateTopBorderCollisionShapes();

    // MapData Updaters
protected:
    virtual bool OnUpdateMapData(HiresTimer* timer);

    // Entities / Furnitures Setters
public:
    Node* AddEntity(const StringHash& got, int entityid, int id, unsigned holderid, int viewZ, const PhysicEntityInfo& physic,
                    const SceneEntityInfo& sceneInfo=SceneEntityInfo::EMPTY, VariantMap* slotData=0, bool outsidePool=false);
    bool SetEntities_Load(HiresTimer* timer);
    bool SetEntities_Add(HiresTimer* timer);

private:
    bool RemoveNodes(bool removeentities = false, HiresTimer* timer=0);

    // Tile Modifiers
public:
protected:
    virtual void OnTileModified(int x, int y);

    // Visibility Setters
public:
    bool SetVisibleStatic(bool visible, HiresTimer* timer=0);
    bool SetVisibleEntities(bool visible, HiresTimer* timer=0);
    bool SetVisibleTiles(bool visible, bool forced=false, HiresTimer* timer=0);
    void ShowMap(HiresTimer* timer);
    void HideMap(HiresTimer* timer);
private:
    bool UpdateVisibility(HiresTimer* timer=0);

    void HandleChangeViewIndex(StringHash eventType, VariantMap& eventData);

    // Connected Maps Setters
public:
    void SetConnectedMap(int direction, Map* map);
    static void ConnectVerticalMaps(Map* maptop, Map* mapbottom);
    static void ConnectHorizontalMaps(Map* mapleft, Map* mapright);
private:
    void UpdateRenderShapeBorders();
    void ClearConnectedMaps();

    // Background Setters
public:
    bool AddBackGroundLayers(HiresTimer* timer);

    // Minimap Setters
public:
private:
    void SetMiniMapAt(int x, int y);
    bool SetMiniMapLayer(int indexViewZ, HiresTimer* timer=0);
    bool SetMiniMapImage(int indexViewZ, HiresTimer* timer=0);
    bool SetMiniMap(HiresTimer* timer);
    void UpdateMiniMap(int indexViewZ);

    /// GETTERS
public:
    // Status Getters
    virtual bool IsEffectiveVisible() const
    {
        return visible_ == MAP_VISIBLE;
    }
    virtual bool IsVisible() const
    {
        return visible_ == MAP_VISIBLE || visible_ == MAP_CHANGETOVISIBLE;
    }
    const char* GetVisibleState() const;

    // Node Getters
    Node* GetLocalEntitiesNode() const
    {
        return localEntitiesNode_;
    }
    Node* GetReplicatedEntitiesNode() const
    {
        return replicatedEntitiesNode_;
    }
    Node* GetTagNode() const
    {
        return nodeTag_;
    }
    const Vector<Node*>& GetImageNodes() const
    {
        return nodeImages_;
    }

    // Coordinate Getters
    Rect& GetBounds()
    {
        return bounds_;
    }
    const Rect& GetBounds() const
    {
        return bounds_;
    }
    bool IsInBounds(const IntVector2& mposition) const
    {
        return mposition.x_ >= 0 && mposition.x_ < MapInfo::info.width_ && mposition.y_ >= 0 && mposition.y_ < MapInfo::info.height_;
    }
    bool IsInXBound(int mpositionx) const
    {
        return mpositionx >= 0 && mpositionx < MapInfo::info.width_;
    }
    bool IsInYBound(int mpositiony) const
    {
        return mpositiony >= 0 && mpositiony < MapInfo::info.height_;
    }
    bool IsInXBound(float x) const
    {
        return x >= bounds_.min_.x_ && x <= bounds_.max_.x_;
    }
    bool IsInYBound(float y) const
    {
        return y >= bounds_.min_.y_ && y <= bounds_.max_.y_;
    }
    Rect GetTileRect(const IntVector2& coords) const;
    Rect GetTileRect(unsigned index) const;

    // View & Environemental Getters
    Image* GetMiniMap(int viewzindex) const
    {
        return miniMap_[viewzindex].Get();
    }
    virtual Map* GetConnectedMap(int direction) const
    {
        return connectedMaps_[direction];
    }
    MapTopography& GetTopography()
    {
        return mapTopography_;
    }
    int GetRealViewZ(int viewZ) const
    {
        return featuredMap_->GetRealViewZAt(viewZ);
    }

    // Tile Getters
    int GetGid(unsigned index, int viewId) const;
    int GetGidAtZ(unsigned index, int layerZ) const;
    int GetGid(int x, int y, int viewId) const;
    unsigned GetNumNeighborTiles(int x, int y, int viewId, unsigned properties);
    int GetMaterialType(unsigned index, int indexZ) const;

    // LOS Getters
    unsigned LineOfSightValue(const IntVector2& pos) const
    {
        return los_[pos.y_ * MapInfo::info.width_ + pos.x_];
    }
    unsigned LineOfSightValue(unsigned index) const
    {
        return los_[index];
    }
    PODVector<unsigned>& GetLosTable()
    {
        return los_;
    }

    // Entity Getters
    virtual bool AllowEntities(int viewZ) const;
    virtual bool AllowFurnitures(int viewZ) const;

    /// UPDATERS
    bool Update(HiresTimer* timer);
    void MarkDirty();

    /// VARS
public:
    static World2DInfo* info_;
    static ViewManager* viewManager_;

private:
    // Status
    Rect bounds_;
    MapTopography mapTopography_;
    int visible_;

    Map* connectedMaps_[4];

    // Nodes
    WeakPtr<Node> node_;
    WeakPtr<Node> nodeTag_;
    Node* localEntitiesNode_;
    Node* replicatedEntitiesNode_;
    Vector<Node* > nodeImages_;

    // Temporary Entities container used in RemoveNodes
    PODVector<Node*> removableEntities_;

    // line of sight by tile
    PODVector<unsigned> los_;

    // Minimap
    SharedPtr<Image> miniMap_[2];
    Vector<SharedPtr<Image> > miniMapLayers_;
    Vector<Vector<Image*> > miniMapLayersByViewZIndex_;

    static CreateMode replicationMode_;
};

