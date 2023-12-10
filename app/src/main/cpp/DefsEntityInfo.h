#pragma once

#include "ShortIntVector2.h"
#include "DefsNetwork.h"
#include "DefsViews.h"

namespace Urho3D
{
class Node;
class Connection;
}

using namespace Urho3D;


class World2DInfo;

struct WorldMapPosition
{
    WorldMapPosition() : defined_(false), viewMask_(0), drawOrder_(-1), viewZIndex_(-1) { }
    WorldMapPosition(World2DInfo* winfo, const Vector2& position, int viewZ=FRONTVIEW);
    WorldMapPosition(World2DInfo* winfo, const IntVector2& mpoint, const IntVector2& mposition, int viewZ=FRONTVIEW);
    WorldMapPosition(const WorldMapPosition& position)
    {
        /*defined_ = true;*/ *this = position;
    }
    WorldMapPosition& operator = (const WorldMapPosition& p)
    {
        defined_ = true;
//        defined_ = p.defined_;
        viewMask_ = p.viewMask_;
        drawOrder_ = p.drawOrder_;
        viewZIndex_ = p.viewZIndex_;
        viewZ_ = p.viewZ_;
        position_ = p.position_;
        shapeRectInTile_ = p.shapeRectInTile_;
        positionInTile_ = p.positionInTile_;
        mPoint_ = p.mPoint_;
        mPosition_ = p.mPosition_;
        tileIndex_ = p.tileIndex_;
        return *this;
    }
    bool operator != (const WorldMapPosition& p) const
    {
        return mPoint_ != p.mPoint_ || tileIndex_ != p.tileIndex_ || viewZIndex_ != p.viewZIndex_;
    }

    String ToString() const;

    bool defined_;
    unsigned viewMask_;
    int drawOrder_;
    int viewZIndex_;
    int viewZ_;

    Vector2 position_;
    /// rect of the collisionshapes in mass center origin and in tilesize normalized => set by gocdestroyer
    Rect shapeRectInTile_;
    /// Position in the tile (0.f-mTileWidth,0.f-mTileHeight) (0.f,0.f) => BottomLeft
    Vector2 positionInTile_;
    ShortIntVector2 mPoint_;
    IntVector2 mPosition_;
    unsigned tileIndex_;

    static const WorldMapPosition EMPTY;
};

struct MapEntityInfo
{
    MapEntityInfo() { }

    WeakPtr<Node> entitiesNode_[2];
    List<unsigned> entities_;
};

struct EquipmentPart
{
    EquipmentPart() { }
    EquipmentPart(const String& mapname, const StringHash& objecttype) : mapname_(mapname), objecttype_(objecttype) { }
    EquipmentPart(const EquipmentPart& part) : mapname_(part.mapname_), objecttype_(part.objecttype_) { }

    String mapname_;
    StringHash objecttype_;
};

typedef Vector<EquipmentPart> EquipmentList;

struct SceneEntityInfo
{
    SceneEntityInfo()
    {
        Reset();
    }
    void Reset()
    {
        memset(this, 0, sizeof(SceneEntityInfo));
    }

    int zindex_;
    unsigned faction_;
    bool deferredAdd_;
    bool skipNetSpawn_;
    Node* attachNode_;
    int clientId_;
    EquipmentList* equipment_;
    ObjectControlInfo* objectControlInfo_;
    Node* ownerNode_;
    static const SceneEntityInfo EMPTY;
};

struct PhysicEntityInfo
{
    PhysicEntityInfo()
    {
        Reset();
    }
    PhysicEntityInfo(float x, float y) : positionx_(x), positiony_(y), velx_(0.f), vely_(0.f), rotation_(0.f), direction_(1.f) { }

    void Reset()
    {
        memset(this, 0, sizeof(PhysicEntityInfo));
    }

    float positionx_, positiony_;
    float velx_, vely_;
    float rotation_, direction_;

    static const PhysicEntityInfo EMPTY;
};

class Map;
class MapModel;

enum
{
    CREATEAUTO = 0,
    CREATEFROMMAPDATA = 1,
    CREATEFROMMAP = 2,
    CREATEFROMGENERATOR = 3,
};

struct ObjectMapedInfo
{
    ObjectMapedInfo() : createmode_(CREATEAUTO), ref_(0) { }
    ObjectMapedInfo(int id, const Vector2& position) : createmode_(CREATEFROMMAPDATA), position_(position), ref_(0)
    {
        rect_.top_ = id;
    }
    ObjectMapedInfo(Map* map, const IntRect& rect, const Vector2& position) : createmode_(CREATEFROMMAP), position_(position), rect_(rect), ref_(map) { }
    ObjectMapedInfo(MapModel* model, int width, int height, unsigned seed, const Vector2& position) : createmode_(CREATEFROMGENERATOR), position_(position), ref_(model)
    {
        rect_.left_ = width;
        rect_.top_ = height;
        rect_.right_ = seed;
    }

    int createmode_;
    Vector2 position_;
    Vector2 scale_;
    IntRect rect_;

    void* ref_;
};


