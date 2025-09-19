#pragma once

#include "DefsGame.h"
#include "DefsMap.h"
#include "DefsFluids.h"
#include "DefsEntityInfo.h"
#include "GameOptions.h"

#ifdef USE_TILERENDERING
#include "ObjectTiled.h"
#else
#include "ObjectSkinned.h"
#endif // USE_TILERENDERING

#include "Map.h"


using namespace Urho3D;

static const short int fixedObjeMapedPointY = -32767;

class MapCreator;
class MapColliderGenerator;
struct ChunkInfo;
class ViewManager;
class GOC_Destroyer;


class ObjectMaped : public Component, public MapBase
{
    URHO3D_OBJECT(ObjectMaped, Component);

public:
    ObjectMaped(Context* context);
    ~ObjectMaped();

    static void RegisterObject(Context* context);
    static void Reset();
    static bool SetPhysicObjectID(ObjectMaped* object);
    static void FreePhysicObjectID(int id);

    /// Initializers
public:
    void SetSize(int width, int height, int numviews);
    bool Clear(HiresTimer* timer=0);
private:
    void Init();

    /// Setters
public:
    void SetMapDataPoint(int point=0);
    void SetPhysicEnabled(bool enable);
    void SetChangeViewZEnabled(bool enable);
    void CreateFrom(const ObjectMapedInfo& info);

protected:
    void HandleSet(StringHash eventType, VariantMap& eventData);
    virtual void OnTileModified(int x, int y);
    virtual void OnPhysicsSetted();
    virtual bool OnUpdateMapData(HiresTimer* timer);
    
    virtual void OnMarkedDirty(Node* node);

private:
    void CreateWheel(Node* root, Node* vehicleNode, const Vector2& center, float scale);
    bool RemoveNodes(HiresTimer* timer=0);

    void HandleChangeViewIndex(StringHash eventType, VariantMap& eventData);

    /// Getters
public:
    virtual bool AllowDynamicFurnitures(int viewZ) const
    {
        return true;
    }

    int GetCreateMode() const
    {
        return info_.createmode_;
    }
    int GetPhysicObjectID() const
    {
        return physicObjectId_;
    }
    int GetMapDataPoint() const
    {
        return dataId_;
    }

    Node* GetVehicleWheels() const
    {
        return vehicleWheels_;
    }

    static const PODVector<ObjectMaped* >& GetPhysicObjects();
    static void GetPhysicObjectAt(const Vector2& wposition, MapBase*& map, bool calculateMapPosition=false);
    static const WorldMapPosition& GetWorldMapPositionResult()
    {
        return sObjectMapPositionResult_;
    }
    static void GetVisibleFurnitures(PODVector<Node*>& furnitureRootNodes);

    /// Updaters
    bool Update(HiresTimer* timer);
    void MarkDirty();
    virtual void OnSetEnabled();
    void OnNodeRemoved();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    void Dump() const;

protected :
    virtual void OnNodeSet(Node* node);

    /// VARS
private:
    ObjectMapedInfo info_;

    int physicObjectId_;
    int dataId_;
    int currentViewId_;
    bool physicsEnabled_;
    bool switchViewZEnabled_;
    int dirtyRecurse_{0};
    WeakPtr<Node> vehicleWheels_;

    static WorldMapPosition sObjectMapPositionResult_;
};

