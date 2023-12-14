#pragma once

namespace Urho3D
{
class Node;
class CollisionBox2D;
}

using namespace Urho3D;

#include "DefsCore.h"

static void* NOCOLLIDER = (void*)0;
static void* WALLCOLLIDER = (void*)1;
static void* PLATEFORMCOLLIDER = (void*)2;
static void* WATERCOLLIDER = (void*)3;

/// ColliderCategory means "Is a ..."
enum ColliderCategory
{
    CC_NONE              = 0,
    CC_TRIGGER           = 1 << 0, // 1

    CC_INSIDEWALL        = 1 << 1, // 2
    CC_OUTSIDEWALL       = 1 << 2, // 4

    CC_INSIDEPLATEFORM   = CC_INSIDEWALL,
    CC_OUTSIDEPLATEFORM  = CC_OUTSIDEWALL,

    CC_INSIDEPROJECTILE  = 1 << 3, // 8
    CC_OUTSIDEPROJECTILE = 1 << 4, // 16

    CC_INSIDEAVATAR      = 1 << 5, // 32
    CC_OUTSIDEAVATAR     = 1 << 6, // 64

    CC_INSIDEMONSTER     = 1 << 7, // 128
    CC_OUTSIDEMONSTER    = 1 << 8, // 256

    CC_INSIDEOBJECT      = 1 << 9, // 512
    CC_OUTSIDEOBJECT     = 1 << 10, // 1024
    CC_OBJECT			 = CC_INSIDEOBJECT + CC_OUTSIDEOBJECT, // 1536
    CC_INSIDEEFFECT      = 1 << 11, // 2048
    CC_OUTSIDEEFFECT     = 1 << 12, // 4096

    CC_SCENEUI           = 1 << 13, // 8192

    CC_INSIDESTATICFURNITURE   = 1 << 14, // 16384
    CC_OUTSIDESTATICFURNITURE  = 1 << 15,

    /// BOX2D can't admit more than 16 bits
    CC_ALLPROJECTILES    = CC_INSIDEPROJECTILE | CC_OUTSIDEPROJECTILE,

    CC_INSIDEWALLS       = CC_INSIDEWALL | CC_INSIDEPLATEFORM,   // 2
    CC_OUTSIDEWALLS      = CC_OUTSIDEWALL | CC_OUTSIDEPLATEFORM, // 4
    CC_ALLWALLS          = CC_INSIDEWALL | CC_OUTSIDEWALL,       // 2 + 4 = 6

    CC_INSIDEENTITIES    = CC_INSIDEAVATAR | CC_INSIDEMONSTER | CC_INSIDEOBJECT, // 32 + 128 + 512 = 672
    CC_OUTSIDEENTITIES   = CC_OUTSIDEAVATAR | CC_OUTSIDEMONSTER | CC_OUTSIDEOBJECT, // 64 + 256 + 1024 = 1344

    // define the mapborder like as an projectile/effect => so the projectiles/effects dont' collide with it
    CC_MAPBORDER         = CC_ALLPROJECTILES,
};

/// ColliderMask means "Collides With ..."
enum ColliderMask
{
    CM_NONE              = 0,
    CM_INSIDEWALL        = CC_INSIDEENTITIES | CC_INSIDEEFFECT | CC_INSIDEPROJECTILE | CC_TRIGGER, // 672 + 2048 + 8 + 1 = 2729
    CM_OUTSIDEWALL       = CC_OUTSIDEENTITIES | CC_OUTSIDEEFFECT | CC_OUTSIDEPROJECTILE | CC_TRIGGER,
    CM_ALLWALL           = CC_INSIDEENTITIES | CC_OUTSIDEENTITIES, // 672 + 1344 = 2016

    CM_INSIDEPLATEFORM   = CC_INSIDEENTITIES,// | CC_INSIDEWALL,
    CM_OUTSIDEPLATEFORM  = CC_OUTSIDEENTITIES,// | CC_OUTSIDEWALL,

    CM_DETECTPLAYER      = CC_INSIDEAVATAR | CC_OUTSIDEAVATAR | CC_SCENEUI, // 96 + 8192 = 8288
    CM_DETECTMONSTER     = CC_INSIDEMONSTER | CC_OUTSIDEMONSTER, // 128 + 256 = 384
    CM_DETECTOBJECT      = CC_INSIDEOBJECT | CC_OUTSIDEOBJECT, // 1536
    CM_DETECTOR          = CM_DETECTPLAYER | CM_DETECTMONSTER | CM_DETECTOBJECT | CC_TRIGGER | CC_ALLWALLS,

    CM_INSIDEAVATAR      = CC_TRIGGER | CC_INSIDEENTITIES | CC_INSIDEWALLS | CC_INSIDEPROJECTILE | CC_INSIDESTATICFURNITURE,     // 1 + 672 + 2 + 8 + 16384 = 17067
    CM_OUTSIDEAVATAR     = CC_TRIGGER | CC_OUTSIDEENTITIES | CC_OUTSIDEWALLS | CC_OUTSIDEPROJECTILE | CC_OUTSIDESTATICFURNITURE,

    CM_INSIDEAVATAR_DETECTOR  = CM_INSIDEAVATAR | CC_SCENEUI, // 691 + 8192 = 8883
    CM_OUTSIDEAVATAR_DETECTOR = CM_OUTSIDEAVATAR | CC_SCENEUI,

    CM_INSIDEMONSTER     = CC_TRIGGER | CC_INSIDEENTITIES | CC_INSIDEWALLS | CC_INSIDEPROJECTILE, // 1 + 672 + 2 + 8 = 683
    CM_OUTSIDEMONSTER    = CC_TRIGGER | CC_OUTSIDEENTITIES | CC_OUTSIDEWALLS | CC_OUTSIDEPROJECTILE,

    CM_INSIDEOBJECT      = CC_TRIGGER | CC_INSIDEENTITIES | CC_INSIDEWALLS, // 1 + 672 + 2 = 675
    CM_OUTSIDEOBJECT     = CC_TRIGGER | CC_OUTSIDEENTITIES | CC_OUTSIDEWALLS, // 1 + 1344 + 4 = 1349
    CM_OBJECT  			 = CM_INSIDEOBJECT | CM_OUTSIDEOBJECT, // 1 + 672 + 1344 + 2 + 4 = 2023

    CM_INSIDEPROJECTILE  = CC_INSIDEENTITIES | CC_INSIDEWALLS,
    CM_OUTSIDEPROJECTILE = CC_OUTSIDEENTITIES | CC_OUTSIDEWALLS,   // 1344 + 4 = 1348
    CM_ALLPROJECTILES    = CM_INSIDEPROJECTILE | CM_OUTSIDEPROJECTILE,

    CM_MAPBORDER         = CC_INSIDEENTITIES | CC_OUTSIDEENTITIES,

    CM_INSIDESTATICFURNITURE   = CC_TRIGGER | CC_INSIDEENTITIES,
    CM_OUTSIDESTATICFURNITURE  = CC_TRIGGER | CC_OUTSIDEENTITIES,

    CM_DEADINSIDEENTITY  = CC_INSIDEWALLS,
    CM_DEADOUTSIDEENTITY = CC_OUTSIDEWALLS,

    CM_INSIDEEFFECT      = CC_INSIDEWALLS | CC_INSIDEOBJECT,
    CM_OUTSIDEEFFECT     = CC_OUTSIDEWALLS | CC_OUTSIDEOBJECT, // 4 + 1024 = 1028

    CM_INSIDERAIN        = CC_INSIDEWALLS | CC_INSIDEENTITIES,
    CM_OUTSIDERAIN       = CC_OUTSIDEWALLS | CC_OUTSIDEENTITIES,

    CM_INSIDEBODILESSPART   = CC_INSIDEWALLS,
    CM_OUTSIDEBODILESSPART  = CC_OUTSIDEWALLS,

    CM_SCENEUI           = CC_SCENEUI, // 8192

    CM_INSIDEFURNITURE   = CC_INSIDEWALLS,  // 2
    CM_OUTSIDEFURNITURE  = CC_OUTSIDEWALLS,  // 4
};

enum ExtraContactBits
{
    CONTACT_ISSTABLE = 1,
    CONTACT_TOP      = 2,
    CONTACT_BOTTOM   = 4,
    CONTACT_LEFT     = 8,
    CONTACT_RIGHT    = 16,
    CONTACT_BODYLESS = 32,
    CONTACT_WALLONLY = 64,
    CONTACT_STAIRS   = 128,

    CONTACT_ALL       = CONTACT_TOP | CONTACT_BOTTOM | CONTACT_LEFT | CONTACT_RIGHT, // 30
    CONTACT_UNSTABLE  = CONTACT_ALL,
    CONTACT_STABLE    = CONTACT_ALL | CONTACT_ISSTABLE, // 31
    CONTACT_PLATEFORM = CONTACT_TOP | CONTACT_ISSTABLE | CONTACT_STAIRS

};

enum MapColliderMode
{
    FrontMode = 0,
    BackMode,
    BackRenderMode,
    TopBorderBackMode,
    InnerMode,
    WallMode,
    WallAndPlateformMode,
    PlateformMode,
    BorderMode,
    TopBackMode,
    EmptyMode
};

enum ColliderShapeTypeMode
{
    SHT_ALL = 0,
    SHT_BOX,
    SHT_CHAIN,
    SHT_EDGE,
    NUM_COLLIDERSHAPETYPEMODES,
};

enum ColliderType
{
    DUNGEONINNERTYPE = 0,
    CAVEINNERTYPE = 1,
    ASTEROIDTYPE = 2,
    MOBILECASTLETYPE = 3,
};

enum MapColliderType
{
    PHYSICCOLLIDERTYPE = 0,
    RENDERCOLLIDERTYPE = 1,
};

class MapBase;
class RenderShape;

struct InfoVertex
{
    unsigned side;
    unsigned indexTile;
};

struct ColliderParams
{
    int indz_, indv_, colliderz_, layerz_, mode_, shapetype_;
    unsigned bits1_, bits2_;
};

static const unsigned NUMDUNGEONPHYSICCOLLIDERS = 7;
static const unsigned NUMCAVEPHYSICCOLLIDERS = 4;
static const unsigned NUMASTEROIDPHYSICCOLLIDERS = 2;
static const unsigned NUMCASTLEPHYSICCOLLIDERS = 5;

static const unsigned NUMDUNGEONRENDERCOLLIDERS = 6;
static const unsigned NUMCAVERENDERCOLLIDERS = 4;
static const unsigned NUMASTEROIDRENDERCOLLIDERS = 1;
static const unsigned NUMCASTLERENDERCOLLIDERS = 4;

extern const ColliderParams dungeonPhysicColliderParams[];
extern const ColliderParams dungeonRenderColliderParams[];
extern const ColliderParams cavePhysicColliderParams[];
extern const ColliderParams caveRenderColliderParams[];
extern const ColliderParams asteroidPhysicColliderParams[];
extern const ColliderParams asteroidRenderColliderParams[];
extern const ColliderParams castlePhysicColliderParams[];
extern const ColliderParams castleRenderColliderParams[];

struct MapCollider
{
    MapCollider() : map_(0) { }
    virtual ~MapCollider() { }

    virtual int GetType() const
    {
        return -1;
    }

    virtual void Clear();

    void SetParams(const ColliderParams* params)
    {
        params_ = params;
    }

    bool IsInside(const Vector2& point, bool checkhole) const;

    const ColliderParams* params_;

    // size for each shape type (ColliderShapeTypeMode)
    unsigned numShapes_[NUM_COLLIDERSHAPETYPEMODES];

    Vector<IntRect> contourBorders_;

    // vertices for chains
    Vector<PODVector<Vector2> > contourVertices_;
    Vector<Vector<PODVector<Vector2> > > holeVertices_;

    // map of the contourid by tile
    PODVector<unsigned char> contourIds_;

    void DumpContour() const;

    MapBase* map_;
};

struct Plateform
{
    Plateform(unsigned tileindex) : box_(0), tileleft_(tileindex), size_(1) { }

    CollisionBox2D* box_;
    unsigned tileleft_;
    unsigned char size_;
};

struct PhysicCollider : public MapCollider
{
    PhysicCollider();
    virtual ~PhysicCollider() { }

    virtual int GetType() const
    {
        return PHYSICCOLLIDERTYPE;
    }

    virtual void Clear();
    void ClearBlocks();
    void ClearPlateforms();

    void ReservePhysic(unsigned maxNumBlocks);

    unsigned GetInfoVertex(unsigned index_shape, unsigned index_vertex) const;

    int id_;

    // box indexed by tileindex
    HashMap<unsigned, CollisionBox2D* > blocks_;
    // plateform rects
    HashMap<unsigned, Plateform* > plateforms_;

    Vector<PODVector<InfoVertex> > infoVertices_;
    List<void* > chains_;
    List<void* > holes_;
};

struct FROMBONES_API RenderCollider : public MapCollider
{
    RenderCollider();
    virtual ~RenderCollider() { }

    virtual int GetType() const
    {
        return RENDERCOLLIDERTYPE;
    }

    virtual void Clear();

    void CreateRenderShape(Node* rootnode, bool mapborder=true, bool dynamic=false);

    void UpdateRenderShape();

    WeakPtr<Node> node_;
    WeakPtr<RenderShape> renderShape_;
};


enum PixelShapeType
{
    PIXSHAPE_RECT = 0,
    PIXSHAPE_SUPERELLIPSE,
    PIXSHAPE_SPIRAL,
    MAX_PIXSHAPETYPE
};

struct PixelShape
{
    const PODVector<unsigned char>& GetData() const
    {
        return data_;
    };
    const Vector<IntVector2>& GetPixelCoords() const
    {
        return pixelcoords_;
    }

    int centerx_, centery_;
    int width_, height_;

    PODVector<unsigned char> data_;
    Vector<IntVector2> pixelcoords_;
};

