#pragma once

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Urho2D/Sprite2D.h>

#include "MapFeatureTypes.h"
#include "MapFurnitures.h"
#include "MapTilesConnect.h"

using namespace Urho3D;


typedef unsigned char ConnectIndex;

//enum TilePropertiesFlag
//{
//    ShapeType         = 0x0003,
//    BlockProperty     = 0x0004,
//    MovableProperty   = 0x0008,
//    BreakableProperty = 0x0010,
//    TransparentProperty = 0x0020,
//};

//static const String TileBlockedProperty = "blocked";
//static const String TileAnimatedProperty = "animated";
//static const String TileBreakableProperty = "breakable";
//static const String TileTerrainProperty = "terrain";
//static const String TileTerrainFeatureCompatiblity = "terrainFeatures";
//static const String TileTerrainFurnitureCompatiblity = "terrainFurnitures";

struct TileProperty
{
    TileProperty(unsigned props) : property_(props) { ; }
    TileProperty(unsigned char flags, unsigned char terrain, ConnectIndex connect, unsigned char dimensions=0)
        : properties_(flags), terrain_(terrain), connect_(connect), dimensions_(dimensions) { ; }

    union
        {
            struct
            {
                unsigned property_;
            };
            struct
            {
                unsigned char properties_;
                unsigned char terrain_;
                ConnectIndex connect_;
                unsigned char dimensions_;
            };
        };
};

enum TileDimensions
{
    TILE_0 = 0,
    /* TILE_1X2_PART =
    ---
    | |
    ---
    |X|
    ---
    */
    TILE_1X2_PART,
    /* TILE_2X1_PART =
    -----
    | |X|
    -----
    */
    TILE_2X1_PART,
    /* TILE_2X2_PARTR =
    -----
    | |X|
    -----
    | | |
    -----
    */
    TILE_2X2_PARTR,
    /* TILE_2X2_PARTB =
    -----
    | | |
    -----
    |X| |
    -----
    */
    TILE_2X2_PARTB,
    /* TILE_2X2_PARTBR =
    -----
    | | |
    -----
    | |X|
    -----
    */
    TILE_2X2_PARTBR,
    TILE_RENDER,
    TILE_1X1,
    TILE_1X2,
    TILE_2X1,
    TILE_2X2
};



enum TileType// : int
{
    TILE_SIMPLE = 0,
    TILE_ANIMATED,
    TILE_BREAKABLE
};

/// Tiles Define
class Tile;

typedef Tile *TilePtr;

class Tile
{
public:
    Tile(int gid) : gid_(gid) { ; }
    Tile(int gid, unsigned char dimensions) : gid_(gid), dimensions_(dimensions) { ; }
//    virtual ~Tile() {;}

    virtual const TileType GetType() const
    {
        return TILE_SIMPLE;
    }

    bool AddSprite(Sprite2D* sprite);
    void SetProperties(const TileProperty& prop)
    {
        property_ = prop.property_;
    }
    void SetProperties(unsigned prop)
    {
        property_ = prop;
    }
    void SetDimensions(unsigned char dim)
    {
        dimensions_ = dim;
    }

    int GetGid() const
    {
        return gid_;
    }
    unsigned char GetPropertyFlags() const
    {
        return properties_;
    }
    unsigned char GetTerrain() const
    {
        return terrain_;
    }
    unsigned char GetDimensions() const
    {
        return dimensions_;
    }
    ConnectIndex GetConnectivity() const
    {
        return connect_;
    }
    int GetConnectValue() const
    {
        return MapTilesConnectType::GetValue(connect_);
    }

    int GetTileOffset(short int* nghTable) const
    {
        return nghTable[DimensionNghIndex[dimensions_]];
    }

    int GetNumSprite() const
    {
        return (int)sprites_.Size();
    }
    Sprite2D* GetSprite(unsigned char version = 0) const
    {
        return sprites_[version];
    }

    static int GetConnectValue(ConnectIndex connect)
    {
        return MapTilesConnectType::GetValue(connect);
    }
    static int GetConnectIndex(int value)
    {
        return MapTilesConnectType::GetIndex(value);
    }
    static int GetMaxDimension(ConnectIndex index);
    static int GetMaxDimension(int value);

    template< typename T > static int GetConnectIndexNghb2(const T* dataMap, unsigned addr, short int nghb1, short int nghb2);
    template< typename T > static int GetConnectIndexNghb3(const T* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3);
    template< typename T > static int GetConnectIndexNghb4(const T* dataMap, unsigned addr);
    template< typename T > static void GetCornerIndex(const T* dataMap, unsigned addr, ConnectIndex& index);

    template< typename T > static int GetConnectIndexNghb2(short int* nghTable, const T* dataMap, unsigned addr, short int nghb1, short int nghb2);
    template< typename T > static int GetConnectIndexNghb3(short int* nghTable, const T* dataMap, unsigned addr, short int nghb1, short int nghb2, short int nghb3);
    template< typename T > static int GetConnectIndexNghb4(short int* nghTable, const T* dataMap, unsigned addr);
    template< typename T > static void GetCornerIndex(short int* nghTable, const T* dataMap, unsigned addr, ConnectIndex& index);

    template< typename T > static int GetMaxDimensionRight(short int* nghTable, const T* dataMap, const TilePtr* render, unsigned addr);
    template< typename T > static int GetMaxDimensionBottom(short int* nghTable, const T* dataMap, const TilePtr* render, unsigned addr);
    template< typename T > static int GetMaxDimensionBottomRight(short int* nghTable, const T* dataMap, const TilePtr* render, unsigned addr);

//    static void SetTile(Tile* tile, sTile*& stile, bool furnish=true);
//    static void SetConnectedTile(sTile*& stile, int feature, int connectValue);

    static const Tile EMPTY;
    static const Tile INITTILE;
    static const Tile DIMPART1X2;
    static const Tile DIMPART2X1;
    static const Tile DIMPART2X2R;
    static const Tile DIMPART2X2B;
    static const Tile DIMPART2X2BR;

    static TilePtr EMPTYPTR;
    static TilePtr INITTILEPTR;
    static TilePtr DIMPART1X2P;
    static TilePtr DIMPART2X1P;
    static TilePtr DIMPART2X2RP;
    static TilePtr DIMPART2X2BP;
    static TilePtr DIMPART2X2BRP;

    static const int DimensionNghIndex[11];

private:
    friend class sTile;

    int gid_;

    union
    {
        struct
        {
            unsigned property_;
        };
        struct
        {
            unsigned char properties_;     /// hardcoded flags (blockproperty etc...)
            unsigned char terrain_;
            ConnectIndex connect_;        /// connection index (0-15)
            unsigned char dimensions_;
        };
    };

    /// Sprites by version id
    Vector<SharedPtr<Sprite2D> > sprites_;
};

class AnimatedTile : public Tile
{
public:
    AnimatedTile(int fromGid, int toGid, int tickDelay, int loopDelay)
        : Tile(fromGid), toGid_(toGid), tickDelay_(tickDelay), loopDelay_(loopDelay) { ; }
//    virtual ~AnimatedTile() {;}

    virtual const TileType GetType() const
    {
        return TILE_ANIMATED;
    }

private:
    friend class sAnimatedTile;

    int toGid_;
    int tickDelay_, loopDelay_;
};

//class BreakableTile : public AnimatedTile
//{
//public:
//    BreakableTile(float maxHitPoint, );
//    virtual const TileType GetType() const { return TILE_BREAKABLE; }
//
//private:
//    float maxHitPoint_;
//};

/// Tiles Instance
class TerrainAtlas;

class sTile
{
public:
    sTile() : tile_(0), node_(0), furniture_(-1), spriteId_(0) { ; }
    sTile(Tile* tile, bool randSprite=false);

    TileType GetType()
    {
        return tile_->GetType();
    }

    void SetTile(Tile* tile)
    {
        tile_ = tile;
    }
    void SetNode(Node* node)
    {
        if (node) node_ = node;
    }
    void SetFurniture(int furniture)
    {
        furniture_ = furniture;
    }
    void SetSpriteID(char version)
    {
        spriteId_ = version;
    }
    void SetRandomFurniture();
    void SetRandomSpriteID();

    const Tile& GetTile() const
    {
        return *tile_;
    }
    const Node* GetNode() const
    {
        return node_;
    }
    Sprite2D* GetSprite() const
    {
        return tile_->sprites_[spriteId_];
    }

    int GetFurniture()
    {
        return furniture_;
    }
    unsigned char GetSpriteID()
    {
        return spriteId_;
    }
    unsigned char GetProperties() const
    {
        return tile_ ? tile_->GetPropertyFlags() : 0;
    } //M_MAX_UNSIGNED; }
    unsigned char GetTerrain() const
    {
        return tile_ ? tile_->GetTerrain() : 0;
    }
    ConnectIndex GetConnectivity() const
    {
        return tile_ ? tile_->GetConnectivity() : 0;
    }

    virtual void Update() { ; }

    static const sTile EMPTY;
    static TerrainAtlas* atlas_;

private:
    Tile* tile_;
    Node* node_;
    int furniture_;
    unsigned char spriteId_;
};

class sAnimatedTile : public sTile
{
public:
    sAnimatedTile(AnimatedTile* tile) : sTile(tile), tile_(tile), currentGid_(tile->GetGid()), currentDelay_(0) { ; }

//    virtual ~sAnimatedTile() {;}

    virtual void Update();
    bool IsPlaying()
    {
        return (GetNode() != 0);
    }

private:
    AnimatedTile* tile_;
    int currentGid_;
    int currentDelay_;
};

//class sBreakableTile : public sAnimatedTile
//{
//public:
//    sBreakableTile(BreakableTile* tile);

//    virtual void Update();
//    bool IsBroken() { return hitPoint_ < 0.f; }
//
//private:
//    float hitPoint_;
//};

