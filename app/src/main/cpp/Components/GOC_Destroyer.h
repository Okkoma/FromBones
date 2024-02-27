#pragma once


#include <Urho3D/Scene/Component.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "DefsCore.h"
#include "DefsEntityInfo.h"
#include "DefsMove.h"

#include "TimerRemover.h"

namespace Urho3D
{
class RigidBody2D;
}

class MapBase;
class Map;
struct FluidCell;

using namespace Urho3D;


enum UpdatePositionMode
{
    UPDATEPOS_AUTO = 0,
    UPDATEPOS_FORCE = 1,
    UPDATEPOS_NOCHANGEMAP = 2,
};


class GOC_Destroyer : public Component
{
    URHO3D_OBJECT(GOC_Destroyer, Component);

public :
    GOC_Destroyer(Context* context);
    virtual ~GOC_Destroyer();

    static void RegisterObject(Context* context);

    void Reset(bool set);
    void ResetViewZ();

    static void SetEnableWorld2D(bool enable)
    {
        useWorld2D_ = enable;
    }
    void SetLifeTime(float delay);
    void SetEnableLifeTimer(bool enable);
    void SetEnableLifeNotifier(bool enable);
    void SetEnableUnstuck(bool enable);
    void SetEnablePositionUpdate(bool enable);
    void SetDelegateNode(Node* node);

    void CheckLifeTime();

    /// for Pool of Object
    void SetDestroyMode(int removestate);

    /// Set viewZ and attributes for body and shape
    void SetViewZ(int viewZ=-1, unsigned viewMask=0, int draworder=-1);
    /// Set WorldMapPosition
    void SetWorldMapPosition(const WorldMapPosition& wmPosition);

    float GetLifeTime() const
    {
        return lifeTime_;
    }
    int GetDestroyMode() const
    {
        return destroyMode_;
    }
    bool GetCheckUnstuck() const
    {
        return checkUnstuck_;
    }
    bool AllowWallSpawning() const
    {
        return allowWallSpawning_;
    }

    bool GetUpdatedWorldPosition2D(Vector2& position);
    const WorldMapPosition& GetWorldMapPosition() const
    {
        return mapWorldPosition_;
    }
    WorldMapPosition& GetWorldMapPosition()
    {
        return mapWorldPosition_;
    }
    const Vector2& GetWorldMassCenter() const
    {
        return mapWorldPosition_.position_;
    }
    const Vector2& GetPositionInTile() const
    {
        return mapWorldPosition_.positionInTile_;
    }
    const Rect& GetShapesRect() const
    {
        return shapesRect_;
    }

    Rect GetWorldShapesRect() const;
    void GetWorldShapesRect(Rect& rect);

    const Rect& GetShapeRectInTile() const
    {
        return mapWorldPosition_.shapeRectInTile_;
    }
    Map* GetCurrentMap() const
    {
        return currentMap_;
    }
    FluidCell* GetCurrentCell() const
    {
        return currentCell_;
    }
    bool IsOnFreeTiles(int viewZ) const;
    bool IsInWalls(MapBase* map, int viewZ);
    bool IsInsideWorld();
    bool HasCellInFront(bool direction);
    bool HasWallInFront(bool direction);

    FeatureType GetFeatureOnViewId(int viewId) const;
    FeatureType GetFeatureOnViewZ(int viewZ) const;

    virtual void ApplyAttributes();
    virtual void OnSetEnabled();

    void Destroy(float delay=0.f, bool reset=true);

    bool UpdateShapesRect();

    void UpdateFilterBits(int viewZ=-1);
    void UpdateAreaStates(bool sendfluidevent=false);

    void UpdatePositions(UpdatePositionMode mode=UPDATEPOS_AUTO);
    bool UpdatePositions(VariantMap& eventData, UpdatePositionMode mode=UPDATEPOS_AUTO);

    void AdjustPosition(const ShortIntVector2& mpoint, int viewZ, Vector2& worldmassposition);
    void AdjustPositionInTile(int viewZ);
    void AdjustPositionInTile(const WorldMapPosition& initialposition);

    void WorldAppearCallBack();

    void DumpWorldMapPositions();

    virtual void DrawDebugGeometry(DebugRenderer* debugRenderer, bool depthTest) const;

private :
    bool Unstuck();

    void OnWorldEntityCreate(StringHash eventType, VariantMap& eventData);
    void OnWorldEntityDestroy(StringHash eventType, VariantMap& eventData);

    void HandleUpdateWorld2D(StringHash eventType, VariantMap& eventData);
    void HandleUpdateTime(StringHash eventType, VariantMap& eventData);
    void HandleUpdateTimePeriod(StringHash eventType, VariantMap& eventData);

    float lifeTime_, elapsedTime_, elapsedTimeLockView_;
    float buoyancy_;
    bool worldUpdatePosition_, lifeNotifier_, checkUnstuck_, allowWallSpawning_;
    bool activeTimer_, switchViewEnable_, viewZDirty_;
    bool hasWallInFront_[2];
    int destroyMode_;
    int viewZToCheck_;
    int lastUnblockedTile_;

    /// mapworld coords
    WorldMapPosition mapWorldPosition_;

    WeakPtr<Node> delegateNode_;
    RigidBody2D* body_;
    Map* currentMap_;
    MapBase* lastObjectMaped_;
    FluidCell* currentCell_;
    FluidCell* lastcurrentCell_[2];

    unsigned tileIndexes_[2];
    FeatureType cellCheckFeature_[2];
    FeatureType lastCellCheckFeature_[2];

    Rect shapesRect_;
    static bool useWorld2D_;
};


