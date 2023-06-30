#pragma once

#include <Urho3D/Container/List.h>

namespace Urho3D
{
class Scene;
class Node;
class Camera;
class TmxFile2D;
class Text;
class CollisionChain2D;
class Connection;
}

#include "GameOptionsTest.h"

#include "DefsNetwork.h"
#include "DefsWorld.h"

class Map;
class MapStorage;
class MapSimulatorLiquid;
class ViewManager;
class Actor;
struct ActorInfo;

using namespace Urho3D;

struct MapFurnitureRef
{
    MapFurnitureRef() : nodeid_(0), ref_(0) { }
    MapFurnitureRef(unsigned nodeid, EntityData* ref) : nodeid_(nodeid), ref_(ref) { }
    MapFurnitureRef(const MapFurnitureRef& m) : nodeid_(m.nodeid_), ref_(m.ref_) { }

    unsigned nodeid_;
    EntityData* ref_;
};

// indexed par tileindex
typedef HashMap<unsigned, List<MapFurnitureRef> > MapFurnitureLocation;

struct WorldViewInfo
{
    WorldViewInfo();

    void Clear();

    int viewport_;

    Camera* camera_;
    Map* currentMap_;
    CollisionChain2D* visibleCollideBorder_;

    float camOrtho_;
    float camRatio_;
    bool cameraFocusEnabled_;

    /// the current map point of the camera in this World
    ShortIntVector2 mPoint_;
    IntVector2 mPosition_;

    /// the Default Map coord
    ShortIntVector2 dMapPoint_;
    Vector2 dMapPosition_;

    IntRect visibleMapArea_;
    IntRect visibleMapAreaMinimized_;
    Rect visibleRect_;
    Rect extVisibleRect_;
    Rect tiledVisibleRect_;

    bool visibleRectInFullBackground_;
    bool isUnderground_;
    bool needUpdateCurrentMap_;

    Vector<ShortIntVector2> mapsToShow_;
    Vector<ShortIntVector2> mapsToHide_;
    Vector<Map*> visibleAreaMaps_;
};

class FROMBONES_API World2D : public Component
{
    URHO3D_OBJECT(World2D, Component);

public:
    static void RegisterObject(Context* context);

    World2D(Context* context);
    ~World2D();

/// Attributes Setters
    void SetWorldPoint(const IntVector2& point);
    void SetWorldSize(const IntVector2& size = IntVector2::ZERO);
    void SetWorldMapSize(const IntVector2& size);
    void SetChunkNum(const IntVector2& chunkNum);

    void SetDefaultMapPoint(const ShortIntVector2& point, bool checkInsideWorld=true); // TODO : viewport
    void SetDefaultMapPoint(const IntVector2& point); // TODO : viewport
    void SetDefaultMapPoint(const Vector2& point); // TODO : viewport

    void SetMapGroundLevelAttr(int level);
    void SetColliderForceShapeTypeAttr(bool force);
    void SetColliderShapeTypeAttr(ColliderShapeTypeMode stype);
    void SetMapAddObjectAttr(bool addObject);
    void SetMapAddFurnitureAttr(bool addFurniture);
    void SetMapAddBorderAttr(bool addBorder);
    void SetMapAddImageLayerAttr(const String& texture);
    void SetMapAddImageLayersAttr(const StringVector& textureInfos);
    void SetAnlWorldModelAttr(const String& modelfile);
    void SetMapGeneratorAttr(const String& gen);
    void SetMapGeneratorParams(const String& params);
    void SetGeneratorSeed(unsigned seed);
    void SetGeneratorRandomSeed();
    void SetGeneratorNumEntities(const IntVector2& numEntities);
    void SetGeneratorAuthorizedCategories(const String& authorizedCategories);

/// Attributes Getters
    const IntVector2& GetWorldPoint() const
    {
        return worldPoint_;
    }
    const IntVector2& GetChunkNum() const
    {
        return chunkNum_;
    }
    IntVector2 GetDefaultMapPoint() const
    {
        return IntVector2(viewinfos_[0].dMapPoint_.x_, viewinfos_[0].dMapPoint_.y_);    // TODO : viewport
    }
    const ShortIntVector2& GetDefaultMapPointShort() const
    {
        return viewinfos_[0].dMapPoint_;    // TODO : viewport
    }
    IntVector2 GetWorldMapSize() const
    {
        return IntVector2(info_->mapWidth_,info_->mapHeight_);
    }
    const IntVector2& GetWorldSize() const
    {
        return worldSize_;
    }
    int GetMapGroundLevelAttr() const
    {
        return info_->simpleGroundLevel_;
    }
    bool GetColliderForceShapeTypeAttr() const
    {
        return info_->forcedShapeType_;
    }
    ColliderShapeTypeMode GetColliderShapeTypeAttr() const
    {
        return info_->shapeType_;
    }
    bool GetMapAddObjectAttr() const
    {
        return info_->addObject_;
    }
    bool GetMapAddFurnitureAttr() const
    {
        return info_->addFurniture_;
    }
    bool GetMapAddBorderAttr() const
    {
        return addBorder_;
    }
    const String& GetMapAddImageLayerAttr() const
    {
        return String::EMPTY;
    }
    const StringVector& GetMapAddImageLayersAttr() const
    {
        return imageLayersInfos_;
    }
    const String& GetAnlWorldModelAttr() const;
    const String& GetMapGeneratorAttr() const;
    const String& GetMapGeneratorParams() const;
    unsigned GetGeneratorSeed() const
    {
        return generatorSeed_;
    }
    const IntVector2& GetGeneratorNumEntities() const;
    const String& GetGeneratorAuthorizedCategories() const
    {
        return generatorAuthorizedCategories_;
    }
    const String& GetEmptyStr() const
    {
        return String::EMPTY;
    }

/// Getters
    static World2D* GetWorld()
    {
        return world_;
    }
    static World2DInfo* GetWorldInfo()
    {
        return info_;
    }
    static MapStorage* GetStorage()
    {
        return mapStorage_;
    }
    static ViewManager* GetViewManager()
    {
        return world_->viewManager_;
    }

    bool IsVisible() const;

    static void SetAllowClearMaps(bool state)
    {
        world_->allowClearMaps_ = state;
    }
    static bool AllowClearMaps()
    {
        return world_->allowClearMaps_;
    }

    static bool IsInfinite()
    {
        return noBounds_;
    }
    static bool IsInsideWorldBounds(const Vector2& worldPosition);
    static bool IsInsideWorldBounds(const ShortIntVector2& mPoint);
    static bool IsInsideVisibleAreas(const ShortIntVector2& mPoint);
    static bool IsInsideVisibleAreasMinimized(const ShortIntVector2& mPoint);
    static bool IsVisibleRectInFullBackGround(int viewport=0)
    {
        return world_->viewinfos_[viewport].visibleRectInFullBackground_;
    }
    static bool IsUnderground(int viewport=0)
    {
        return world_->viewinfos_[viewport].isUnderground_;
    }

    static Rect GetMapBounds(const Vector2& worldPosition);
    static const IntRect& GetWorldBounds()
    {
        return worldBounds_;
    }
    static const Rect& GetWorldFloatBounds()
    {
        return worldFloatBounds_;
    }

    static const Rect& GetExtendedVisibleRect(int viewport=0);
    static const Rect& GetTiledVisibleRect(int viewport=0);
    static Rect GetVisibleRect(int viewport=0);
    static const IntRect& GetVisibleAreas(int viewport=0)
    {
        return world_->viewinfos_[viewport].visibleMapAreaMinimized_;
    }
    static IntRect GetVisibleAreas(const Vector2& wposition);
    static const Vector<ShortIntVector2>& GetKeepedVisibleMaps()
    {
        return keepedVisibleMaps_;
    }

    static const ShortIntVector2& GetCurrentMapPoint(int viewport=0)
    {
        return world_->viewinfos_[viewport].mPoint_;
    }
    static Map* GetCurrentMap(int viewport=0)
    {
        return world_->viewinfos_[viewport].currentMap_;
    }
    static const Vector2& GetCurrentMapOrigin(int viewport=0);

    static Map* GetMapAt(const ShortIntVector2& mPoint, bool createIfMissing=false);
    static Map* GetMapAt(const Vector2& wPosition);
    static Map* GetAvailableMapAt(const ShortIntVector2& mPoint);
    static unsigned GetNumMapsDrawn(int viewport);
    static bool GetNearestBlockTile(const Vector2& wposition, int viewZ, ShortIntVector2& mpoint, unsigned& tileindex);
    static void GetCollisionShapesAt(const ShortIntVector2& mpoint, unsigned tileindex, int viewZ, Vector<CollisionShape2D*>& cshapes);

    static int GetTileGid(const WorldMapPosition& mapPosition);
    static unsigned GetTileProperty(const WorldMapPosition& mapPosition);

    /// get the LOS value for world position : realize the conversion world to map positions
    static unsigned GetLineOfSight(const Vector2& worldPosition);
    /// prefer this one : no conversion done
    static unsigned GetLineOfSight(const WorldMapPosition& position);
    static PODVector<unsigned>* GetLosTable(const ShortIntVector2& mPoint);

    static Node* GetEntitiesRootNode(Scene* scene, CreateMode mode=LOCAL);
    static Node* GetEntitiesNode(const ShortIntVector2& mPoint, CreateMode mode=LOCAL);
    static void GetFilteredEntities(const ShortIntVector2& mPoint, PODVector<Node*>& entities, int skipControllerType);
    static void GetEntities(const ShortIntVector2& mPoint, PODVector<Node*>& entities, const StringHash& type);
    static void GetEntities(const ShortIntVector2& mPoint, PODVector<Node*>& entities, const char* name);
    static List<unsigned>& GetEntities(const ShortIntVector2& mPoint)
    {
        return mapEntities_[mPoint].entities_;
    }
    static MapEntityInfo& GetEntitiesInfo(const ShortIntVector2& mPoint)
    {
        return mapEntities_[mPoint];
    }
    static void GetVisibleEntities(PODVector<Node*>& entities); // TODO : viewport
    static PODVector<Node*> FindEntitiesAt(const String& entityName, const ShortIntVector2& mPoint, int viewZ);
    static List<MapFurnitureRef> FindFurnituresAt(const ShortIntVector2& mPoint, unsigned tileindex);

/// Setters
    void Start();
    void Stop();
    void Set(bool load);
    void SetInfo(World2DInfo* info);
    void SetUpdateLoadingDelay(int delay = MAP_MAXDELAY);
    void SetCamera(float zoom=1.f, const Vector2& focus=Vector2::ZERO);
    void ResetPosition(const Vector2& focus=Vector2::ZERO); // TODO : viewport
    void AddScrollers(int viewport);

    void GoToMap(const ShortIntVector2& mpoint, const IntVector2& mposition, int viewZ=-1, int viewport=0);
    void GoToMap(const ShortIntVector2& mpoint, int viewport=0);
    void GoCameraToDestinationMap(int viewport=0, bool updateinstant=true);

    static void SetKeepedVisibleMaps(bool state);
    static void ResetPositionFocus();
    static void SavePositionFocus();

    static void ReinitWorld(const IntVector2& wPoint);
    static void ReinitAllWorlds();
    static void SaveWorld(bool saveEntities=false);

    static void AttachEntityToMapNode(Node* entity, const ShortIntVector2& mPoint, CreateMode mode=LOCAL);
    static void AddEntity(const ShortIntVector2& mPoint, unsigned id);
    static void RemoveEntity(const ShortIntVector2& mPoint, unsigned id);
    static void DestroyEntity(MapBase* map, Node* node);
    static void PurgeEntities(const ShortIntVector2& mPoint);
    static void AddStaticFurniture(const ShortIntVector2& mPoint, Node* node, EntityData& furniture);
    static void DestroyFurnituresAt(MapBase* map, unsigned tileindex);

    static Node* SpawnFurniture(const StringHash& got, Vector2 position, int viewZ, bool isabiome=true, bool checkpositionintile=true, bool findfloor=false);
    static Node* NetSpawnEntity(ObjectControlInfo& info, Node* holder=0, VariantMap* slotData=0);
    static Node* SpawnEntity(const StringHash& got, int entityid, int id, unsigned holderid, int viewZ, const PhysicEntityInfo& physicInfo, const SceneEntityInfo& sceneInfo=SceneEntityInfo::EMPTY, VariantMap* slotData=0, bool outsidePool=false);
    static Actor* SpawnActor(const String& name, const StringHash& got, unsigned char entityid, const StringHash& dialogue, int viewZ, const Vector2& position);
    static Actor* SpawnActor(unsigned actorid, const Vector2& position, int viewZ);

/// For Editor
    Node* SpawnEntity(const StringHash& got);
    Node* SpawnFurniture(const StringHash& got);
    Node* SpawnActor();

/// Serializers
private:
    void LoadActors();
    void SaveActors();

/// Updaters
public:
    /// Update Loading Maps in memory
    bool UpdateLoading();
    /// Update Step buffer and visible maps.
    void UpdateStep(float timestep=0.f);
    /// Update visible maps
    bool UpdateVisibility(float timestep);
    /// Update the Viewport at the worldposition or by default at the default viewport position WorldViewInfo::dMapPosition_
    void UpdateInstant(int viewport=0, const Vector2& position=Vector2::ZERO, float timestep=0.f, bool sendevent=true);
    /// Update buffer and visible maps : no timer
    void UpdateAll();
    void UpdateVisibleRectInfos(int viewport=0);

private:
    void UpdateWorldBounds();
    void UpdateTextureLevels();
    void UpdateVisibleLists(int viewport=0);
    void UpdateVisibleArea(int viewport=0, HiresTimer* timer=0);
    void UpdateVisibleCollideBorders();
    void UpdateActors(HiresTimer* timer);
    void UpdateMaps(HiresTimer* timer);

/// Handlers
public:
    virtual void OnSetEnabled();
    void OnViewportUpdated();
    void OnMapVisibleChanged(Map* map);

private:
    virtual void OnNodeSet(Node* node);
private:
    void HandleChunksDirty(StringHash eventType, VariantMap& eventData);
    void HandleObjectAppear(StringHash eventType, VariantMap& eventData);
    void HandleObjectChangeMap(StringHash eventType, VariantMap& eventData);
    void HandleObjectDestroy(StringHash eventType, VariantMap& eventData);

/// Debug & Dump
public:
    void SetEnableDrawDebug(bool enable);
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool activeTagText);
private:
    void DrawDebugTaggedInfos(DebugRenderer* debug, bool activeTagText);
    void DrawDebugRect(const Rect& rect, DebugRenderer* debugRenderer, bool depthTest, const Color& color);
    void DrawDebugTile(Map* map, unsigned tileindex, DebugRenderer* debugRenderer, bool depthTest, const Color& color);
    void DrawDebugBiomeTiles(Map* map, DebugRenderer* debugRenderer, bool depthTest);
public:
    void DumpMapsInMemory() const;
    void DumpEntitiesInMemory() const;
    void DumpMapVisibilityProgress() const;
private:
    void DumpNodeList(const List<unsigned>& list, String title) const;

private :
    bool addBorder_, allowClearMaps_;
    bool isSet_;
    IntVector2 chunkNum_;

    StringVector imageLayersInfos_;

    unsigned generatorSeed_;
    IntVector2 generatorNumEntities_;
    String generatorAuthorizedCategories_;
    String genParams_;

    Rect originMapRect_; /// DrawDebug : origin Map Marker

    WorldViewInfo viewinfos_[MAX_VIEWPORTS];

    Vector<ShortIntVector2> mapsToShow_;
    Vector<ShortIntVector2> mapsToHide_;

    MapSimulatorLiquid* mapLiquid_;
    ViewManager* viewManager_;

    /// Cached extended visible rect
    static Rect extVisibleRectCached_;
    /// Save Position (for createmode)
    static Vector2 focusPosition_;

    /// The Point of this World On the WorldMap
    static IntVector2 worldPoint_;
    static float mWidth_;
    static float mHeight_;
    static float mTileWidth_;
    static float mTileHeight_;

    static IntRect worldBounds_;
    static Rect worldFloatBounds_;
    static bool worldMapUpdate_;
    static IntVector2 worldSize_;
    static bool noBounds_;

    static WeakPtr<Node> entitiesRootNodes_[2];
    static Vector<ShortIntVector2> keepedVisibleMaps_;
    static Vector<Map*> effectiveVisibleMaps_[MAX_VIEWPORTS];
    static HashMap<ShortIntVector2, MapEntityInfo > mapEntities_;
    static HashMap<ShortIntVector2, MapFurnitureLocation> mapFurnitures_;

    static World2D* world_;
    static World2DInfo* info_;
    static MapStorage* mapStorage_;
    static Text* world2DDebugPoolText_;
};





