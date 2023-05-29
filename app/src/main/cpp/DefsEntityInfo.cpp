#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>


#include "DefsWorld.h"
#include "ViewManager.h"

#include "DefsEntityInfo.h"


/// WorldMapPosition

const WorldMapPosition WorldMapPosition::EMPTY = WorldMapPosition();

WorldMapPosition::WorldMapPosition(World2DInfo* winfo, const Vector2& position) :
    defined_(false), viewMask_(0), drawOrder_(-1), viewZIndex_(-1)
{
    winfo->Convert2WorldMapPosition(position, *this, positionInTile_);
}

WorldMapPosition::WorldMapPosition(World2DInfo* winfo, const IntVector2& mpoint, const IntVector2& mposition, int viewZ) :
    defined_(false), viewMask_(0), drawOrder_(-1), viewZIndex_(-1), viewZ_(viewZ)
{
    mPoint_.x_ = mpoint.x_;
    mPoint_.y_ = mpoint.y_;
    mPosition_ = mposition;
    tileIndex_ = winfo->GetTileIndex(mposition);

    if (ViewManager::Get())
        viewZIndex_ = ViewManager::Get()->GetViewZIndex(viewZ);

    winfo->Convert2WorldPosition(*this, position_);
}

String WorldMapPosition::ToString() const
{
    return "(wpos:" + position_.ToString() + "|viewz:" + String(viewZ_) + "|mpoint:" + mPoint_.ToString() +
           "|mpos:" + mPosition_.ToString() + "|tindex:" + String(tileIndex_) + "|intile:" + positionInTile_.ToString() + ")";
}

const SceneEntityInfo SceneEntityInfo::EMPTY = SceneEntityInfo();

const PhysicEntityInfo PhysicEntityInfo::EMPTY = PhysicEntityInfo();
