#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Texture2D.h>

#include "DefsViews.h"
#include "DefsWorld.h"

#include "Map.h"

#include "GameOptionsTest.h"
#include "GameRand.h"
#include "GameHelpers.h"

#include "TerrainAtlas.h"

#include "ObjectSkinned.h"


#define NOVIEW -1
#define TILEMAPSEED 1000


extern const char* mapViewsNames[];



/// ObjectSkinned Implementation

ObjectSkinned::ObjectSkinned() :
    terrain_(0),
    skin_(0)
{
    Init();
}

ObjectSkinned::ObjectSkinned(ObjectFeatured* feature, TerrainAtlas* atlas, MapTerrain* terrain, MapSkin* skin)
{
    Init();

    Set(feature, atlas, terrain, skin);
}

ObjectSkinned::ObjectSkinned(unsigned width, unsigned height, unsigned numviews) :
    terrain_(0),
    skin_(0)
{
    Init();

    Resize(width, height, numviews);
}

ObjectSkinned::~ObjectSkinned()
{
//    URHO3D_LOGINFOF("~ObjectSkinned() ...");

    feature_.Reset();

//    URHO3D_LOGINFOF("~ObjectSkinned() ... OK !");
}

void ObjectSkinned::Init()
{
    skin_ = 0;
    terrain_ = 0;
    atlas_ = 0;
    indexToSet_ = indexVToSet_ = 0;
    numviews_ = 0;
    map_ = 0;
}

void ObjectSkinned::Clear()
{
    // Clear Features
    if (feature_)
        feature_->Clear();

//    for (unsigned i=0; i < tiledViews_.Size(); ++i)
//        memset(&tiledViews_[i][0], 0, sizeof(Tile*) * tiledViews_[i].Size());

    Init();
//	URHO3D_LOGINFOF("ObjectSkinned() - Clear : this=%u ... OK !", this);
}

void ObjectSkinned::Resize(unsigned width, unsigned height, unsigned numviews)//, bool featureShared)
{
    if (!feature_)
        feature_ = SharedPtr<ObjectFeatured>(new ObjectFeatured());

    feature_->Resize(width, height, numviews);

    if (numviews != connectedViews_.Size())
    {
        SetNumViews(numviews);
        connectedViews_.Resize(numviews_);
        tiledViews_.Resize(numviews_);
    }

    unsigned size = width * height;

    for (unsigned i=0; i < numviews_; ++i)
        connectedViews_[i].Resize(size);

    for (unsigned i=0; i < numviews_; ++i)
        tiledViews_[i].Resize(size);
}

// StandAlone Use : Not used in MapWorld
void ObjectSkinned::Set(ObjectFeatured* feature, TerrainAtlas* atlas, MapTerrain* terrain, MapSkin* skin)
{
    SetSkin(skin);
    SetTerrain(terrain);
    SetAtlas(atlas);

    SetFeature(feature);
    SetViews();
}

void ObjectSkinned::SetNumViews(unsigned numviews)
{
//    URHO3D_LOGINFOF("ObjectSkinned() - SetNumViews :  numviews=%u", numviews);

    numviews_ = numviews;
    indexToSet_ = 0;
}

void ObjectSkinned::SetFeature(ObjectFeatured* feature)
{
    if (!feature)
    {
        feature_ = SharedPtr<ObjectFeatured>(new ObjectFeatured());
    }
    else
    {
        feature_ = SharedPtr<ObjectFeatured>(feature);
        Resize(feature->width_, feature->height_, feature->GetNumViews());//, true);
    }
}

void ObjectSkinned::SetTerrain(MapTerrain* terrain)
{
    terrain_ = terrain;
}

void ObjectSkinned::SetSkin(MapSkin* skin)
{
    skin_ = skin;
}

void ObjectSkinned::SetSkin(const String& name)
{
    if (!atlas_)
    {
        URHO3D_LOGWARNINGF("ObjectSkinned() - SetSkin : name=%s ... No ATLAS !", name.CString());
        return;
    }

    MapSkin& skin = atlas_->GetSkin(StringHash(name));
    SetSkin(&skin);
}

void ObjectSkinned::SetSkin(const String& category, unsigned index)
{
    if (!atlas_)
    {
        URHO3D_LOGWARNINGF("ObjectSkinned() - SetSkin : category=%s ... No ATLAS !", category.CString());
        return;
    }

    MapSkin* skin = atlas_->GetSkin(category, index);
    if (!skin)
    {
        URHO3D_LOGWARNINGF("ObjectSkinned() - SetSkin : category=%s No Skin !", category.CString());
        return;
    }

//    URHO3D_LOGINFOF("ObjectSkinned() - SetSkin : category=%s skin=%s !", category.CString(), skin->name_.CString());

    SetSkin(skin);
}

void ObjectSkinned::SetAtlas(TerrainAtlas* atlas)
{
    atlas_ = atlas;

//    URHO3D_LOGINFOF("ObjectSkinned() - SetAtlas : name=%s ptr=%u !", atlas ? atlas->GetName().CString() : "NOATLAS", atlas);

    indexToSet_ = 0;
}


FeaturedMap& ObjectSkinned::GetFeaturedView(unsigned id)
{
    return feature_->GetFeatureView(id);
}

ConnectedMap& ObjectSkinned::GetConnectedView(unsigned id)
{
    return connectedViews_[id];
}

TiledMap& ObjectSkinned::GetTiledView(unsigned id)
{
    return tiledViews_[id];
}

const Vector<ConnectedMap>& ObjectSkinned::GetConnectedViews() const
{
    return connectedViews_;
}

const Vector<TiledMap>& ObjectSkinned::GetTiledViews() const
{
    return tiledViews_;
}


///
/// Get Gid for 4-Neighbors Mode
///

inline int ObjectSkinned::GetGid42FromSkin(const SkinData& sdata, const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    SkinData::ConstIterator it = sdata.Find(fdata[addr]);
    if (it == sdata.End())
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, addr, nghb1, nghb2);

    return it->second_ ? it->second_->GetRandomTileGidForConnectIndex(index) : 0;
}

inline int ObjectSkinned::GetGid43FromSkin(const SkinData& sdata, const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }
    SkinData::ConstIterator it = sdata.Find(fdata[addr]);
    if (it == sdata.End())
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, nghb1, nghb2, nghb3);

    return it->second_ ? it->second_->GetRandomTileGidForConnectIndex(index) : 0;
}

inline int ObjectSkinned::GetGid44FromSkin(const SkinData& sdata, const FeatureType* fdata, unsigned addr, ConnectIndex& index) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    SkinData::ConstIterator it = sdata.Find(fdata[addr]);
    if (it == sdata.End())
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb4(feature_->nghTable4_, fdata, addr);

    return it->second_ ? it->second_->GetRandomTileGidForConnectIndex(index) : 0;
}

inline int ObjectSkinned::GetGid42FromTerrain(const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, addr, nghb1, nghb2);

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForConnectIndex(index);
}

inline int ObjectSkinned::GetGid43FromTerrain(const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, nghb1, nghb2, nghb3);

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForConnectIndex(index);
}

inline int ObjectSkinned::GetGid44FromTerrain(const FeatureType* fdata, unsigned addr, ConnectIndex& index) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb4(feature_->nghTable4_, fdata, addr);
    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForConnectIndex(index);
}

///
/// Get Gid for 8-Neighbors Mode
///

inline int ObjectSkinned::GetGid82FromSkin(const SkinData& sdata, const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    SkinData::ConstIterator it = sdata.Find(fdata[addr]);
    if (it == sdata.End())
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, addr, nghb1, nghb2);

    if (index > 2)
        Tile::GetCornerIndex(feature_->nghCorner_, fdata, addr, index);

    return it->second_ ? it->second_->GetRandomTileGidForConnectIndex(index) : 0;
}

inline int ObjectSkinned::GetGid83FromSkin(const SkinData& sdata, const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    SkinData::ConstIterator it = sdata.Find(fdata[addr]);
    if (it == sdata.End())
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, nghb1, nghb2, nghb3);

    if (index > 2)
        Tile::GetCornerIndex(feature_->nghCorner_, fdata, addr, index);

    return it->second_ ? it->second_->GetRandomTileGidForConnectIndex(index) : 0;
}

inline int ObjectSkinned::GetGid84FromSkin(const SkinData& sdata, const FeatureType* fdata, unsigned addr, ConnectIndex& index) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    SkinData::ConstIterator it = sdata.Find(fdata[addr]);
    if (it == sdata.End())
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb4(feature_->nghTable4_, fdata, addr);

    if (index > 2)
        Tile::GetCornerIndex(feature_->nghCorner_, fdata, addr, index);

    return it->second_ ? it->second_->GetRandomTileGidForConnectIndex(index) : 0;
}

inline int ObjectSkinned::GetGid82FromTerrain(const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, addr, nghb1, nghb2);

    if (index > 2)
        Tile::GetCornerIndex(feature_->nghCorner_, fdata, addr, index);

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForConnectIndex(index);
}

inline int ObjectSkinned::GetGid83FromTerrain(const FeatureType* fdata, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, nghb1, nghb2, nghb3);

    if (index > 2)
        Tile::GetCornerIndex(feature_->nghCorner_, fdata, addr, index);

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForConnectIndex(index);
}

inline int ObjectSkinned::GetGid84FromTerrain(const FeatureType* fdata, unsigned addr, ConnectIndex& index) const
{
    if (fdata[addr] < MapFeatureType::NoRender)
    {
        index = MapTilesConnectType::Void;
        return 0;
    }

    index = Tile::GetConnectIndexNghb4(feature_->nghTable4_, fdata, addr);

    if (index > 2)
        Tile::GetCornerIndex(feature_->nghCorner_, fdata, addr, index);

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForConnectIndex(index);
}

///
/// Get Gid for NoConnected Mode
///

inline int ObjectSkinned::GetGid0_1X1FromSkin(const MapSkin& skin, const FeatureType* fdata, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    SkinData::ConstIterator it = skin.skinData_.Find(fdata[addr]);
    const MapTerrain* terrain = it == skin.skinData_.End() ? &atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)) : it->second_;
    if (!terrain)
        return 0;

    return terrain->GetRandomTileGidForDimension(TILE_1X1);
}

inline int ObjectSkinned::GetGid0_2X1FromSkin(const MapSkin& skin, const FeatureType* fdata, const TilePtr* render, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    SkinData::ConstIterator it = skin.skinData_.Find(fdata[addr]);
    if (it == skin.skinData_.End())
        return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(Tile::GetMaxDimensionRight(feature_->nghTable4_, fdata, render, addr));

    return it->second_ ? it->second_->GetRandomTileGidForDimension(Tile::GetMaxDimensionRight(feature_->nghTable4_, fdata, render, addr)) : 0;
}

inline int ObjectSkinned::GetGid0_1X2FromSkin(const MapSkin& skin, const FeatureType* fdata, const TilePtr* render, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    SkinData::ConstIterator it = skin.skinData_.Find(fdata[addr]);
    if (it == skin.skinData_.End())
        return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(Tile::GetMaxDimensionBottom(feature_->nghTable4_, fdata, render, addr));

    return it->second_ ? it->second_->GetRandomTileGidForDimension(Tile::GetMaxDimensionBottom(feature_->nghTable4_, fdata, render, addr)) : 0;
}

inline int ObjectSkinned::GetGid0_2X2FromSkin(const MapSkin& skin, const FeatureType* fdata, const TilePtr* render, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    SkinData::ConstIterator it = skin.skinData_.Find(fdata[addr]);
    if (it == skin.skinData_.End())
        return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(Tile::GetMaxDimensionBottomRight(feature_->nghTable8_, fdata, render, addr));

    // need nghTable8_ here to check corner
    return it->second_ ? it->second_->GetRandomTileGidForDimension(Tile::GetMaxDimensionBottomRight(feature_->nghTable8_, fdata, render, addr)) : 0;
}


inline int ObjectSkinned::GetGid0_1X1FromTerrain(const FeatureType* fdata, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(TILE_1X1);
}

inline int ObjectSkinned::GetGid0_2X1FromTerrain(const FeatureType* fdata, const TilePtr* render, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(Tile::GetMaxDimensionRight(feature_->nghTable4_, fdata, render, addr));
}

inline int ObjectSkinned::GetGid0_1X2FromTerrain(const FeatureType* fdata, const TilePtr* render, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(Tile::GetMaxDimensionBottom(feature_->nghTable4_, fdata, render, addr));
}

inline int ObjectSkinned::GetGid0_2X2FromTerrain(const FeatureType* fdata, const TilePtr* render, unsigned addr) const
{
    if (!fdata[addr])
        return 0;

    return atlas_->GetBiomeTerrain(feature_->GetTerrainValue(addr)).GetRandomTileGidForDimension(Tile::GetMaxDimensionBottomRight(feature_->nghTable8_, fdata, render, addr));
}

///
/// Setting Views
///

void ObjectSkinned::SetViewFromSkin(unsigned viewid)
{
    const FeatureType* fdata = &(feature_->GetFeatureView(viewid)[0]);

    ConnectedMap& connectedView = GetConnectedView(viewid);
    TiledMap& tiledView = GetTiledView(viewid);
    const unsigned width = GetWidth();
    const unsigned height = GetHeight();

    unsigned addr = 0;
    SkinData& sdata = skin_->skinData_;

    GameRand::SetSeedRand(TILRAND, TILEMAPSEED+viewid);

    if (skin_->neighborMode_ == Connected4)
    {
//        URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromSkin : id=%u skin=%u featureData=%u 4-ConnectedTiles ...", viewid, skin_, fdata);

        // Set inner blocks
        for (unsigned y=1; y < height-1; y++)
            for (unsigned x=1; x < width-1; x++)
            {
                addr = width*y + x;
                tiledView[addr] = atlas_->GetTile(GetGid44FromSkin(sdata, fdata, addr, connectedView[addr]));
            }

        // borders left & right
        for (unsigned y=1; y < height-1; y++)
        {
            // border left
            addr = width*y;
            tiledView[addr] = atlas_->GetTile(GetGid43FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, RightBit, BottomBit));
            // border right
            addr += width - 1;
            tiledView[addr] = atlas_->GetTile(GetGid43FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, BottomBit, LeftBit));
        }

        // borders top & bottom
        const unsigned lastRowAddr = (height-1) * width;
        const unsigned lastColAddr = width - 1;

        for (unsigned x=1; x < width-1; x++)
        {
            // border top
            tiledView[x] = atlas_->GetTile(GetGid43FromSkin(sdata, fdata, x, connectedView[x], RightBit, BottomBit, LeftBit));
            // border bottom
            addr = lastRowAddr + x;
            tiledView[addr] = atlas_->GetTile(GetGid43FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, RightBit, LeftBit));
        }

        // Set corner blocks
        tiledView[0] = atlas_->GetTile(GetGid42FromSkin(sdata, fdata, 0, connectedView[0], RightBit, BottomBit));
        tiledView[lastColAddr] = atlas_->GetTile(GetGid42FromSkin(sdata, fdata, lastColAddr, connectedView[lastColAddr], BottomBit, LeftBit));
        tiledView[lastRowAddr] = atlas_->GetTile(GetGid42FromSkin(sdata, fdata, lastRowAddr, connectedView[lastRowAddr], TopBit, RightBit));
        addr = lastRowAddr + lastColAddr;
        tiledView[addr] = atlas_->GetTile(GetGid42FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, LeftBit));
    }
    else if (skin_->neighborMode_ == Connected8)
    {
//        URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromSkin : id=%u skin=%u featureData=%u 8-ConnectedTiles ...", viewid, skin_, fdata);

        // Set inner blocks
        for (unsigned y=1; y < height-1; y++)
            for (unsigned x=1; x < width-1; x++)
            {
                addr = width*y + x;
                tiledView[addr] = atlas_->GetTile(GetGid84FromSkin(sdata, fdata, addr, connectedView[addr]));
            }

        // borders left & right
        for (unsigned y=1; y < height-1; y++)
        {
            // border left
            addr = width*y;
            tiledView[addr] = atlas_->GetTile(GetGid83FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, RightBit, BottomBit));
            // border right
            addr += width - 1;
            tiledView[addr] = atlas_->GetTile(GetGid83FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, BottomBit, LeftBit));
        }

        // borders top & bottom
        const unsigned lastRowAddr = (height-1) * width;
        const unsigned lastColAddr = width - 1;

        for (unsigned x=1; x < width-1; x++)
        {
            // border top
            tiledView[x] = atlas_->GetTile(GetGid83FromSkin(sdata, fdata, x, connectedView[x], RightBit, BottomBit, LeftBit));
            // border bottom
            addr = lastRowAddr + x;
            tiledView[addr] = atlas_->GetTile(GetGid83FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, RightBit, LeftBit));
        }

        // Set corner blocks
        tiledView[0] = atlas_->GetTile(GetGid82FromSkin(sdata, fdata, 0, connectedView[0], RightBit, BottomBit));
        tiledView[lastColAddr] = atlas_->GetTile(GetGid82FromSkin(sdata, fdata, lastColAddr, connectedView[lastColAddr], BottomBit, LeftBit));
        tiledView[lastRowAddr] = atlas_->GetTile(GetGid82FromSkin(sdata, fdata, lastRowAddr, connectedView[lastRowAddr], TopBit, RightBit));
        addr = lastRowAddr + lastColAddr;
        tiledView[addr] = atlas_->GetTile(GetGid82FromSkin(sdata, fdata, addr, connectedView[addr], TopBit, LeftBit));
    }
    else
    {
        URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromSkin : view=%s Connected0 ...", mapViewsNames[viewid]);

        const MapSkin& skin = *skin_;
        Tile* tile;
        Tile** tiles = &(tiledView[0]);

        const unsigned lastColAddr = width - 1;

        // init tiles
        {
            unsigned size = height*width;
            for (unsigned i = 0; i < size; i++)
                tiles[i] = Tile::INITTILEPTR;
        }

        /// Important : For Dimension Tiles, Don't change this order : top, left, inner, right and bottom
        // border top
        {
            // corner top left
            {
                connectedView[0] = fdata[0] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, 0, RightBit, BottomBit) : MapTilesConnectType::Void;

                tile = atlas_->GetTile(GetGid0_2X2FromSkin(skin, fdata, tiles, 0));
                tiles[0] = tile;

                if (tile != Tile::EMPTYPTR)
                {
                    switch (tile->GetDimensions())
                    {
                    case TILE_2X1:
                        tiles[1] = Tile::DIMPART2X1P;
                        break;
                    case TILE_1X2:
                        tiles[width] = Tile::DIMPART1X2P;
                        break;
                    case TILE_2X2:
                        tiles[1] = Tile::DIMPART2X2RP;
                        tiles[width] = Tile::DIMPART2X2BP;
                        tiles[width+1] = Tile::DIMPART2X2BRP;
                        break;
                    }
                }
            }
            // top
            for (unsigned x=1; x < width-1; x++)
            {
                connectedView[x] = fdata[x] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, x, RightBit, BottomBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[x]->GetDimensions() < TILE_RENDER)
                    continue;

                tile = atlas_->GetTile(GetGid0_2X2FromSkin(skin, fdata, tiles, x));
                tiles[x] = tile;

                if (tile != Tile::EMPTYPTR)
                {
                    switch (tile->GetDimensions())
                    {
                    case TILE_2X1:
                        tiles[x+1] = Tile::DIMPART2X1P;
                        break;
                    case TILE_1X2:
                        tiles[x+width] = Tile::DIMPART1X2P;
                        break;
                    case TILE_2X2:
                        tiles[x+1] = Tile::DIMPART2X2RP;
                        tiles[x+width] = Tile::DIMPART2X2BP;
                        tiles[x+width+1] = Tile::DIMPART2X2BRP;
                        break;
                    }
                }
            }
            // corner top right
            {
                connectedView[lastColAddr] = fdata[lastColAddr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, lastColAddr,  BottomBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[lastColAddr]->GetDimensions() > TILE_RENDER)
                {
                    tile = atlas_->GetTile(GetGid0_1X2FromSkin(skin, fdata, tiles, lastColAddr));
                    tiles[lastColAddr] = tile;

                    if (tile != Tile::EMPTYPTR)
                        if (tile->GetDimensions() == TILE_1X2)
                            tiles[lastColAddr+width] = Tile::DIMPART1X2P;
                }
            }
        }

        // border left
        for (unsigned y=1; y < height-1; y++)
        {
            addr = width*y;
            connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, TopBit, RightBit, BottomBit) : MapTilesConnectType::Void;

            if (tiles[addr]->GetDimensions() < TILE_RENDER)
                continue;

            tile = atlas_->GetTile(GetGid0_2X2FromSkin(skin, fdata, tiles, addr));
            tiles[addr] = tile;

            if (tile != Tile::EMPTYPTR)
            {
                switch (tile->GetDimensions())
                {
                case TILE_2X1:
                    tiles[addr+1] = Tile::DIMPART2X1P;
                    break;
                case TILE_1X2:
                    tiles[addr+width] = Tile::DIMPART1X2P;
                    break;
                case TILE_2X2:
                    tiles[addr+1] = Tile::DIMPART2X2RP;
                    tiles[addr+width] = Tile::DIMPART2X2BP;
                    tiles[addr+width+1] = Tile::DIMPART2X2BRP;
                    break;
                }
            }
        }

        // inner blocks
        for (unsigned y=1; y < height-1; y++)
            for (unsigned x=1; x < width-1; x++)
            {
                addr = width*y + x;
                connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb4(feature_->nghTable4_, fdata, addr) : MapTilesConnectType::Void;

                if (tiles[addr]->GetDimensions() < TILE_RENDER)
                    continue;

                tile = atlas_->GetTile(GetGid0_2X2FromSkin(skin, fdata, tiles, addr));
                tiles[addr] = tile;

                if (tile != Tile::EMPTYPTR)
                {
                    switch (tile->GetDimensions())
                    {
                    case TILE_2X1:
                        tiles[addr+1] = Tile::DIMPART2X1P;
                        break;
                    case TILE_1X2:
                        tiles[addr+width] = Tile::DIMPART1X2P;
                        break;
                    case TILE_2X2:
                        tiles[addr+1] = Tile::DIMPART2X2RP;
                        tiles[addr+width] = Tile::DIMPART2X2BP;
                        tiles[addr+width+1] = Tile::DIMPART2X2BRP;
                        break;
                    }
                }
            }

        // border right
        for (unsigned y=1; y < height-1; y++)
        {
            // border right
            addr = width*(y+1) - 1;
            connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, TopBit, BottomBit, LeftBit) : MapTilesConnectType::Void;

            if (tiles[addr]->GetDimensions() < TILE_RENDER)
                continue;

            tile = atlas_->GetTile(GetGid0_1X2FromSkin(skin, fdata, tiles, addr));
            tiles[addr] = tile;

            if (tile != Tile::EMPTYPTR)
                if (tile->GetDimensions() == TILE_1X2)
                    tiles[addr+width] = Tile::DIMPART1X2P;
        }

        // border bottom
        {
            const unsigned lastRowAddr = (height-1) * width;
            // corner left bottom
            {
                connectedView[lastRowAddr] = fdata[lastRowAddr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, lastRowAddr, TopBit, RightBit) : MapTilesConnectType::Void;

                if (tiles[lastRowAddr]->GetDimensions() > TILE_RENDER)
                {
                    tile = atlas_->GetTile(GetGid0_2X1FromSkin(skin, fdata, tiles, lastRowAddr));
                    tiles[lastRowAddr] = tile;

                    if (tile != Tile::EMPTYPTR)
                        if (tile->GetDimensions() == TILE_2X1)
                            tiles[lastRowAddr+1] = Tile::DIMPART2X1P;
                }
            }
            // bottom
            for (unsigned x=1; x < width-1; x++)
            {
                addr = lastRowAddr + x;
                connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, TopBit, RightBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[addr]->GetDimensions() < TILE_RENDER)
                    continue;

                tile = atlas_->GetTile(GetGid0_2X1FromSkin(skin, fdata, tiles, addr));
                tiles[addr] = tile;

                if (tile != Tile::EMPTYPTR)
                    if (tile->GetDimensions() == TILE_2X1)
                        tiles[addr+1] = Tile::DIMPART2X1P;
            }
            // corner right bottom
            {
                addr = lastRowAddr + lastColAddr;
                connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, addr, TopBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[addr]->GetDimensions() > TILE_RENDER)
                    tiles[addr] = atlas_->GetTile(GetGid0_1X1FromSkin(skin, fdata, addr));
            }
        }

        URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromSkin : view=%s Connected0 ... OK !", mapViewsNames[viewid]);
    }
}

void ObjectSkinned::SetViewFromTerrain(unsigned viewid)
{
    const FeatureType* fdata = &(feature_->GetFeatureView(viewid)[0]);
    ConnectedMap& connectedView = GetConnectedView(viewid);
    TiledMap& tiledView = GetTiledView(viewid);
    const unsigned& width = GetWidth();
    const unsigned& height = GetHeight();

    URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromTerrain : view=%s(%u) featureData=%u ...", mapViewsNames[viewid], viewid, fdata);

    // Set inner blocks
    unsigned addr = 0;

    GameRand::SetSeedRand(TILRAND, TILEMAPSEED+viewid);

    if (atlas_->GetNeighborMode() == Connected4)
    {
        for (unsigned y=1; y < height-1; y++)
            for (unsigned x=1; x < width-1; x++)
            {
                addr = width*y + x;
                tiledView[addr] = atlas_->GetTile(GetGid44FromTerrain(fdata, addr, connectedView[addr]));
            }

        // borders left & right
        for (unsigned y=1; y < height-1; y++)
        {
            // border left
            addr = width*y;
            tiledView[addr] = atlas_->GetTile(GetGid43FromTerrain(fdata, addr, connectedView[addr], TopBit, RightBit, BottomBit));
            // border right
            addr += width - 1;
            tiledView[addr] = atlas_->GetTile(GetGid43FromTerrain(fdata, addr, connectedView[addr], TopBit, BottomBit, LeftBit));
        }

        // borders top & bottom
        const unsigned lastRowAddr = (height-1) * width;
        const unsigned lastColAddr = width - 1;

        for (unsigned x=1; x < width-1; x++)
        {
            // border top
            tiledView[x] = atlas_->GetTile(GetGid43FromTerrain(fdata, x, connectedView[x], RightBit, BottomBit, LeftBit));
            // border bottom
            addr = lastRowAddr + x;
            tiledView[addr] = atlas_->GetTile(GetGid43FromTerrain(fdata, addr, connectedView[addr], TopBit, RightBit, LeftBit));
        }

        // Set corner blocks
        tiledView[0] = atlas_->GetTile(GetGid42FromTerrain(fdata, 0, connectedView[0], RightBit, BottomBit));
        tiledView[lastColAddr] = atlas_->GetTile(GetGid42FromTerrain(fdata, lastColAddr, connectedView[lastColAddr], BottomBit, LeftBit));
        tiledView[lastRowAddr] = atlas_->GetTile(GetGid42FromTerrain(fdata, lastRowAddr, connectedView[lastRowAddr], TopBit, RightBit));
        addr = lastRowAddr + lastColAddr;
        tiledView[addr] = atlas_->GetTile(GetGid42FromTerrain(fdata, addr, connectedView[addr], TopBit, LeftBit));
    }
    else if (atlas_->GetNeighborMode() == Connected8)
    {

    }
    else
    {
        URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromTerrain : view=%s Connected0 ... ", mapViewsNames[viewid]);

        Tile* tile;
        Tile** tiles = &(tiledView[0]);

        const unsigned lastColAddr = width - 1;

        // init tiles
        {
            unsigned size = height*width;
            for (unsigned i = 0; i < size; i++)
                tiles[i] = Tile::INITTILEPTR;
        }

        /// Important : For Dimension Tiles, Don't change this order : top, left, inner, right and bottom
        // border top
        {
            // corner top left
            {
                connectedView[0] = fdata[0] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, 0, RightBit, BottomBit) : MapTilesConnectType::Void;

                tile = atlas_->GetTile(GetGid0_2X2FromTerrain(fdata, tiles, 0));
                tiles[0] = tile;

                if (tile != Tile::EMPTYPTR)
                {
                    switch (tile->GetDimensions())
                    {
                    case TILE_2X1:
                        tiles[1] = Tile::DIMPART2X1P;
                        break;
                    case TILE_1X2:
                        tiles[width] = Tile::DIMPART1X2P;
                        break;
                    case TILE_2X2:
                        tiles[1] = Tile::DIMPART2X2RP;
                        tiles[width] = Tile::DIMPART2X2BP;
                        tiles[width+1] = Tile::DIMPART2X2BRP;
                        break;
                    }
                }
            }
            // top
            for (unsigned x=1; x < width-1; x++)
            {
                connectedView[x] = fdata[x] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, x, RightBit, BottomBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[x]->GetDimensions() < TILE_RENDER)
                    continue;

                tile = atlas_->GetTile(GetGid0_2X2FromTerrain(fdata, tiles, x));
                tiles[x] = tile;

                if (tile != Tile::EMPTYPTR)
                {
                    switch (tile->GetDimensions())
                    {
                    case TILE_2X1:
                        tiles[x+1] = Tile::DIMPART2X1P;
                        break;
                    case TILE_1X2:
                        tiles[x+width] = Tile::DIMPART1X2P;
                        break;
                    case TILE_2X2:
                        tiles[x+1] = Tile::DIMPART2X2RP;
                        tiles[x+width] = Tile::DIMPART2X2BP;
                        tiles[x+width+1] = Tile::DIMPART2X2BRP;
                        break;
                    }
                }
            }
            // corner top right
            {
                connectedView[lastColAddr] = fdata[lastColAddr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, lastColAddr,  BottomBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[lastColAddr]->GetDimensions() > TILE_RENDER)
                {
                    tile = atlas_->GetTile(GetGid0_1X2FromTerrain(fdata, tiles, lastColAddr));
                    tiles[lastColAddr] = tile;

                    if (tile != Tile::EMPTYPTR)
                        if (tile->GetDimensions() == TILE_1X2)
                            tiles[lastColAddr+width] = Tile::DIMPART1X2P;
                }
            }
        }

        // border left
        for (unsigned y=1; y < height-1; y++)
        {
            addr = width*y;
            connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, TopBit, RightBit, BottomBit) : MapTilesConnectType::Void;

            if (tiles[addr]->GetDimensions() < TILE_RENDER)
                continue;

            tile = atlas_->GetTile(GetGid0_2X2FromTerrain(fdata, tiles, addr));
            tiles[addr] = tile;

            if (tile != Tile::EMPTYPTR)
            {
                switch (tile->GetDimensions())
                {
                case TILE_2X1:
                    tiles[addr+1] = Tile::DIMPART2X1P;
                    break;
                case TILE_1X2:
                    tiles[addr+width] = Tile::DIMPART1X2P;
                    break;
                case TILE_2X2:
                    tiles[addr+1] = Tile::DIMPART2X2RP;
                    tiles[addr+width] = Tile::DIMPART2X2BP;
                    tiles[addr+width+1] = Tile::DIMPART2X2BRP;
                    break;
                }
            }
        }

        // inner blocks
        for (unsigned y=1; y < height-1; y++)
        {
            for (unsigned x=1; x < width-1; x++)
            {
                addr = width*y + x;
                connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb4(feature_->nghTable4_, fdata, addr) : MapTilesConnectType::Void;

                if (tiles[addr]->GetDimensions() < TILE_RENDER)
                    continue;

                tile = atlas_->GetTile(GetGid0_2X2FromTerrain(fdata, tiles, addr));
                tiles[addr] = tile;

                if (tile != Tile::EMPTYPTR)
                {
                    switch (tile->GetDimensions())
                    {
                    case TILE_2X1:
                        tiles[addr+1] = Tile::DIMPART2X1P;
                        break;
                    case TILE_1X2:
                        tiles[addr+width] = Tile::DIMPART1X2P;
                        break;
                    case TILE_2X2:
                        tiles[addr+1] = Tile::DIMPART2X2RP;
                        tiles[addr+width] = Tile::DIMPART2X2BP;
                        tiles[addr+width+1] = Tile::DIMPART2X2BRP;
                        break;
                    }
                }
            }
        }

        // border right
        for (unsigned y=1; y < height-1; y++)
        {
            // border right
            addr = width*(y+1) - 1;
            connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, TopBit, BottomBit, LeftBit) : MapTilesConnectType::Void;

            if (tiles[addr]->GetDimensions() < TILE_RENDER)
                continue;

            tile = atlas_->GetTile(GetGid0_1X2FromTerrain(fdata, tiles, addr));
            tiles[addr] = tile;

            if (tile != Tile::EMPTYPTR)
                if (tile->GetDimensions() == TILE_1X2)
                    tiles[addr+width] = Tile::DIMPART1X2P;
        }

        // border bottom
        {
            const unsigned lastRowAddr = (height-1) * width;
            // corner left bottom
            {
                connectedView[lastRowAddr] = fdata[lastRowAddr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, lastRowAddr, TopBit, RightBit) : MapTilesConnectType::Void;

                if (tiles[lastRowAddr]->GetDimensions() > TILE_RENDER)
                {
                    tile = atlas_->GetTile(GetGid0_2X1FromTerrain(fdata, tiles, lastRowAddr));
                    tiles[lastRowAddr] = tile;

                    if (tile != Tile::EMPTYPTR)
                        if (tile->GetDimensions() == TILE_2X1)
                            tiles[lastRowAddr+1] = Tile::DIMPART2X1P;
                }
            }
            // bottom
            for (unsigned x=1; x < width-1; x++)
            {
                addr = lastRowAddr + x;
                connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, fdata, addr, TopBit, RightBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[addr]->GetDimensions() < TILE_RENDER)
                    continue;

                tile = atlas_->GetTile(GetGid0_2X1FromTerrain(fdata, tiles, addr));
                tiles[addr] = tile;

                if (tile != Tile::EMPTYPTR)
                    if (tile->GetDimensions() == TILE_2X1)
                        tiles[addr+1] = Tile::DIMPART2X1P;
            }
            // corner right bottom
            {
                addr = lastRowAddr + lastColAddr;
                connectedView[addr] = fdata[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, fdata, addr, TopBit, LeftBit) : MapTilesConnectType::Void;

                if (tiles[addr]->GetDimensions() > TILE_RENDER)
                    tiles[addr] = atlas_->GetTile(GetGid0_1X1FromTerrain(fdata, addr));
            }
        }

        URHO3D_LOGINFOF("ObjectSkinned() - SetViewFromTerrain : view=%s Connected0 ... OK !", mapViewsNames[viewid]);
    }
}

bool ObjectSkinned::SetViews(HiresTimer* timer, const long long& delay)
{
//    URHO3D_LOGINFOF("ObjectSkinned() - SetViews ... indexToSet_=%u ...", indexToSet_);

    if (indexToSet_ == 0)
    {
        /*
        if (!skin_ && !terrain_)
        {
            URHO3D_LOGERROR("ObjectSkinned() - SetViews ... NO SKIN AND NO TERRAIN !");
            return true;
        }
        */

        indexToSet_++;
        indexVToSet_ = 0;
    }

    if (indexToSet_ == 1)
    {
        URHO3D_LOGINFOF("ObjectSkinned() - SetViews ... startTimer=%d", timer ? timer->GetUSec(false)/1000 : 0);
        for (;;)
        {
            if (indexVToSet_ >= numviews_)
            {
                indexToSet_++;

                URHO3D_LOGINFOF("ObjectSkinned() - SetViews ... OK !");
                indexVToSet_ = 0;
                indexToSet_ = 0;
                return true;
            }

            if (skin_)
                SetViewFromSkin(indexVToSet_);
            else
                SetViewFromTerrain(indexVToSet_);

#ifdef DUMP_SKINWARNINGS
            GameHelpers::DumpData(&GetConnectedView(indexVToSet_)[0], -1, 2, GetWidth(), GetHeight());
#endif

            indexVToSet_++;

            URHO3D_LOGINFOF("ObjectSkinned() - SetViews ... view=%u/%u ... timer=%d msec", indexVToSet_, numviews_, timer ? timer->GetUSec(false) / 1000 : 0);

            if (HalfTimeOver(timer))
                return false;
        }
    }

    return false;
}

void ObjectSkinned::SetView(int viewid)
{
    if (skin_)
        SetViewFromSkin(viewid);
    else
        SetViewFromTerrain(viewid);
}

inline void ObjectSkinned::SetConnectIndex_4(const FeatureType* features, ConnectedMap& connections, Tile** tiles, unsigned addr, int x, int y)
{
    // inner
    if (y > 0 && y < GetHeight()-1)
    {
        if (x > 0 && x < GetWidth()-1)
            tiles[addr] = atlas_->GetTile(GetGid44FromSkin(skin_->skinData_, features, addr, connections[addr]));
        else if (x == 0)
            tiles[addr] = atlas_->GetTile(GetGid43FromSkin(skin_->skinData_, features, addr, connections[addr], TopBit, RightBit, BottomBit));
        else
            tiles[addr] = atlas_->GetTile(GetGid43FromSkin(skin_->skinData_, features, addr, connections[addr], TopBit, BottomBit, LeftBit));
    }
    // top
    else if (y == 0)
    {
        if (x > 0 && x < GetWidth()-1)
            tiles[addr] = atlas_->GetTile(GetGid43FromSkin(skin_->skinData_, features, addr, connections[addr], RightBit, BottomBit, LeftBit));
        else if (x == 0)
            tiles[addr] = atlas_->GetTile(GetGid42FromSkin(skin_->skinData_, features, addr, connections[addr], RightBit, BottomBit));
        else
            tiles[addr] = atlas_->GetTile(GetGid42FromSkin(skin_->skinData_, features, addr, connections[addr], BottomBit, LeftBit));
    }
    // bottom
    else
    {
        if (x > 0 && x < GetWidth()-1)
            tiles[addr] = atlas_->GetTile(GetGid43FromSkin(skin_->skinData_, features, addr, connections[addr], TopBit, RightBit, LeftBit));
        else if (x == 0)
            tiles[addr] = atlas_->GetTile(GetGid42FromSkin(skin_->skinData_, features, addr, connections[addr], TopBit, RightBit));
        else
            tiles[addr] = atlas_->GetTile(GetGid42FromSkin(skin_->skinData_, features, addr, connections[addr], TopBit, LeftBit));
    }
}

//inline void ObjectSkinned::SetConnectIndex_0(const FeatureType* features, ConnectedMap& connections, unsigned addr, int x, int y)
//{
//    if (y > 0 && y < GetHeight()-1)
//    {
//        // inner
//        if (x > 0 && x < GetWidth()-1)
//        {
//            connections[addr] = Tile::GetConnectIndexNghb4(feature_->nghTable4_, features, addr);
//        }
//        // on left border
//        else if (x == 0)
//        {
//            connections[addr] = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, RightBit, BottomBit);
//        }
//        // on right border
//        else
//        {
//            connections[addr] = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, BottomBit, LeftBit);
//        }
//    }
//    // top
//    else if (y == 0)
//    {
//        if (x > 0 && x < GetWidth()-1)
//            connections[addr] = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, RightBit, BottomBit, LeftBit);
//        else if (x == 0)
//            connections[addr] = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, 0, RightBit, BottomBit);
//        else
//            connections[addr] = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, BottomBit, LeftBit);
//    }
//    // bottom
//    else
//    {
//        if (x > 0 && x < GetWidth()-1)
//            connections[addr] = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, RightBit, LeftBit);
//        else if (x == 0)
//            connections[addr] = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, TopBit, RightBit);
//        else
//            connections[addr] = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, TopBit, LeftBit);
//    }
//}

// Set connections at (x,y) with take care of connectedmaps
inline void ObjectSkinned::SetConnectIndex_0(const FeatureType* features, ConnectedMap& connections, unsigned addr, int x, int y, int viewid)
{
    ConnectIndex& connect1 = connections[addr];

//    URHO3D_LOGINFOF("ObjectSkinned() - SetConnectIndex_0 x=%d y=%d viewid=%d", x, y, viewid);

    if (y > 0 && y < GetHeight()-1)
    {
        // inner
        if (x > 0 && x < GetWidth()-1)
        {
//            connect1 = Tile::GetConnectIndexNghb4(feature_->nghTable4_, features, addr);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb4(feature_->nghTable4_, features, addr) : MapTilesConnectType::Void;
        }
        // on left border
        else if (x == 0)
        {
//            connect1 = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, RightBit, BottomBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, RightBit, BottomBit) : MapTilesConnectType::Void;

            Map* cmap = map_ ? map_->GetConnectedMap(MapDirection::West) : 0;
            // be sure that the viewid exists for cmap too : check numviews (see UpdateSewingBatches)
            if (cmap && viewid < cmap->GetNumViews() && cmap->GetFeatureType(addr + GetWidth()-1, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | LeftSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[addr + GetWidth()-1];
                connect2 = (connect1 & LeftSide) != 0 ? connect2 | RightSide : connect2 & ~RightSide;

                //                URHO3D_LOGINFOF("ObjectSkinned() - SetConnectIndex_0 x=%d y=%d viewid=%d map => connect1=%s(%d) maponleft => connect2=%s(%d)", x, y, viewid,
                //                                MapTilesConnectType::GetName(connect1), connect1, MapTilesConnectType::GetName(connect2), connect2);
            }
        }
        // on right border
        else
        {
//            connect1 = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, BottomBit, LeftBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, BottomBit, LeftBit) : MapTilesConnectType::Void;

            Map* cmap = map_ ? map_->GetConnectedMap(MapDirection::East) : 0;
            if (cmap && viewid < cmap->GetNumViews() && cmap->GetFeatureType(addr - GetWidth()+1, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | RightSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[addr - GetWidth()+1];
                connect2 = (connect1 & RightSide) != 0 ? connect2 | LeftSide : connect2 & ~LeftSide;
            }
        }
    }
    // top
    else if (y == 0)
    {
        Map* cmap;

        // border top
        if (x > 0 && x < GetWidth()-1)
        {
//            connect1 = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, RightBit, BottomBit, LeftBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, RightBit, BottomBit, LeftBit) : MapTilesConnectType::Void;
        }
        // corner top-left
        else if (x == 0)
        {
            connect1 = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, 0, RightBit, BottomBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, 0, RightBit, BottomBit) : MapTilesConnectType::Void;

            cmap = map_ ? map_->GetConnectedMap(MapDirection::West) : 0;
            if (cmap && viewid < cmap->GetNumViews() && cmap->GetFeatureType(GetWidth()-1, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | LeftSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[GetWidth()-1];
                connect2 = (connect1 & LeftSide) != 0 ? connect2 | RightSide : connect2 & ~RightSide;
            }
        }
        // corner top-right
        else
        {
//            connect1 = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, BottomBit, LeftBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, BottomBit, LeftBit) : MapTilesConnectType::Void;

            cmap = map_ ? map_->GetConnectedMap(MapDirection::East) : 0;
            if (cmap && viewid < cmap->GetNumViews() && cmap->GetFeatureType(0, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | RightSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[0];
                connect2 = (connect1 & RightSide) != 0 ? connect2 | LeftSide : connect2 & ~LeftSide;
            }
        }
        // for all border top
        cmap = map_ ? map_->GetConnectedMap(MapDirection::North) : 0;
        if (cmap && viewid < cmap->GetNumViews())
        {
            unsigned addr2 = GetWidth() * (GetHeight()-1) + x;
            if (cmap->GetFeatureType(addr2, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | TopSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[addr2];
                connect2 = (connect1 & TopSide) != 0 ? connect2 | BottomSide : connect2 & ~BottomSide;
            }
        }
    }
    else
    {
        Map* cmap;

        // bottom
        if (x > 0 && x < GetWidth()-1)
        {
//            connect1 = Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, RightBit, LeftBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb3(feature_->nghTable4_, features, addr, TopBit, RightBit, LeftBit) : MapTilesConnectType::Void;
        }
        // bottom left
        else if (x == 0)
        {
//            connect1 = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, TopBit, RightBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, TopBit, RightBit) : MapTilesConnectType::Void;

            Map* cmap = map_ ? map_->GetConnectedMap(MapDirection::West) : 0;
            if (cmap && viewid < cmap->GetNumViews() && cmap->GetFeatureType(GetWidth() * GetHeight()-1, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | LeftSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[GetWidth() * GetHeight()-1];
                connect2 = (connect1 & LeftSide) != 0 ? connect2 | RightSide : connect2 & ~RightSide;
            }
        }
        // bottom right
        else
        {
//            connect1 = Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, TopBit, LeftBit);
            connect1 = features[addr] >= MapFeatureType::NoRender ? Tile::GetConnectIndexNghb2(feature_->nghTable4_, features, addr, TopBit, LeftBit) : MapTilesConnectType::Void;

            Map* cmap = map_ ? map_->GetConnectedMap(MapDirection::East) : 0;
            if (cmap && viewid < cmap->GetNumViews() && cmap->GetFeatureType(GetWidth() * (GetHeight() -1), viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | RightSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[GetWidth() * (GetHeight() -1)];
                connect2 = (connect1 & RightSide) != 0 ? connect2 | LeftSide : connect2 & ~LeftSide;
            }
        }
        // for all border bottom
        cmap = map_ ? map_->GetConnectedMap(MapDirection::South) : 0;
        if (cmap && viewid < cmap->GetNumViews())
        {
            if (cmap->GetFeatureType(x, viewid) > MapFeatureType::InnerSpace)
            {
                if (features[addr] > MapFeatureType::InnerSpace)
                    connect1 = connect1 | BottomSide;

                ConnectIndex& connect2 = cmap->GetConnectedView(viewid)[x];
                connect2 = (connect1 & BottomSide) != 0 ? connect2 | TopSide : connect2 & ~TopSide;
            }
        }
    }
}

// Recursive dimension Tile Changer
void ObjectSkinned::ReduceDimensionTile_Skin(int dimension, const FeatureType* features, Tile** tiles, unsigned addr)
{
    // Remove current dimension part by a TILE_1X1
    switch (tiles[addr]->GetDimensions())
    {
    case TILE_1X2_PART:
        tiles[addr-GetWidth()] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-GetWidth()));
        break;
    case TILE_2X1_PART:
        tiles[addr-1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-1));
        break;
    case TILE_2X2_PARTR:
        tiles[addr-1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-1));
        tiles[addr+GetWidth()] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+GetWidth()));
        tiles[addr+GetWidth()-1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+GetWidth()-1));
        break;
    case TILE_2X2_PARTB:
        tiles[addr+1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+1));
        tiles[addr-GetWidth()] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-GetWidth()));
        tiles[addr-GetWidth()+1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-GetWidth()+1));
        break;
    case TILE_2X2_PARTBR:
        tiles[addr-1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-1));
        tiles[addr-GetWidth()] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-GetWidth()));
        tiles[addr-GetWidth()-1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr-GetWidth()-1));
        break;
    case TILE_1X2:
        tiles[addr+GetWidth()] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+GetWidth()));
        break;
    case TILE_2X1:
        tiles[addr+1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+1));
        break;
    case TILE_2X2:
        tiles[addr+1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+1));
        tiles[addr+GetWidth()] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+GetWidth()));
        tiles[addr+GetWidth()+1] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr+GetWidth()+1));
        break;
    }

    // Change to a new dimension tile
    switch (dimension)
    {
    case TILE_1X1:
        tiles[addr] = atlas_->GetTile(GetGid0_1X1FromSkin(*skin_, features, addr));
        break;
    case TILE_1X2:
        tiles[addr] = atlas_->GetTile(GetGid0_1X2FromSkin(*skin_, features, tiles, addr));
        break;
    case TILE_2X1:
        tiles[addr] = atlas_->GetTile(GetGid0_2X1FromSkin(*skin_, features, tiles, addr));
        break;
    case TILE_2X2:
        tiles[addr] = atlas_->GetTile(GetGid0_2X2FromSkin(*skin_, features, tiles, addr));
        break;
    }
#ifdef DUMP_MAPDEBUG_SETTILE
    if (tiles[addr] == Tile::EMPTYPTR && features[addr] > 0)
        URHO3D_LOGWARNINGF("ObjectSkinned() - ReduceDimensionTile_Skin : Tile Empty feature=%s(%u)", MapFeatureType::GetName(features[addr]), features[addr]);
#endif
    // Reduce dimension on impacted tiles
    switch (tiles[addr]->GetDimensions())
    {
    case TILE_1X2:
//        URHO3D_LOGINFOF("ObjectSkinned() - ReduceDimensionTile_Skin : Add a TILE_1X2 at %u => impact on %u", addr, addr+GetWidth());
        ReduceDimensionTile_Skin(TILE_1X1, features, tiles, addr+GetWidth());
        tiles[addr+GetWidth()] = Tile::DIMPART1X2P;
        break;
    case TILE_2X1:
//        URHO3D_LOGINFOF("ObjectSkinned() - ReduceDimensionTile_Skin : Add a TILE_2X1 at %u => impact on %u", addr, addr+1);
        ReduceDimensionTile_Skin(TILE_1X1, features, tiles, addr+1);
        tiles[addr+1] = Tile::DIMPART2X1P;
        break;
    case TILE_2X2:
//        URHO3D_LOGINFOF("ObjectSkinned() - ReduceDimensionTile_Skin : Add a TILE_2X1 at %u => impact on %u, %u, %u", addr, addr+1, addr+GetWidth(), addr+GetWidth()+1);
        ReduceDimensionTile_Skin(TILE_1X1, features, tiles, addr+1);
        ReduceDimensionTile_Skin(TILE_1X1, features, tiles, addr+GetWidth());
        ReduceDimensionTile_Skin(TILE_1X1, features, tiles, addr+GetWidth()+1);
        tiles[addr+1] = Tile::DIMPART2X2RP;
        tiles[addr+GetWidth()] = Tile::DIMPART2X2BP;
        tiles[addr+GetWidth()+1] = Tile::DIMPART2X2BRP;
        break;
    }
}

// Recursive dimension Tile Changer
void ObjectSkinned::ReduceDimensionTile_Terrain(int dimension, const FeatureType* features, Tile** tiles, unsigned addr)
{
    // Remove current dimension part by a TILE_1X1
    switch (tiles[addr]->GetDimensions())
    {
    case TILE_1X2_PART:
        tiles[addr-GetWidth()] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-GetWidth()));
        break;
    case TILE_2X1_PART:
        tiles[addr-1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-1));
        break;
    case TILE_2X2_PARTR:
        tiles[addr-1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-1));
        tiles[addr+GetWidth()] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+GetWidth()));
        tiles[addr+GetWidth()-1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+GetWidth()-1));
        break;
    case TILE_2X2_PARTB:
        tiles[addr+1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+1));
        tiles[addr-GetWidth()] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-GetWidth()));
        tiles[addr-GetWidth()+1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-GetWidth()+1));
        break;
    case TILE_2X2_PARTBR:
        tiles[addr-1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-1));
        tiles[addr-GetWidth()] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-GetWidth()));
        tiles[addr-GetWidth()-1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr-GetWidth()-1));
        break;
    case TILE_1X2:
        tiles[addr+GetWidth()] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+GetWidth()));
        break;
    case TILE_2X1:
        tiles[addr+1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+1));
        break;
    case TILE_2X2:
        tiles[addr+1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+1));
        tiles[addr+GetWidth()] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+GetWidth()));
        tiles[addr+GetWidth()+1] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr+GetWidth()+1));
        break;
    }

    // Change to a new dimension tile
    switch (dimension)
    {
    case TILE_1X1:
        tiles[addr] = atlas_->GetTile(GetGid0_1X1FromTerrain(features, addr));
        break;
    case TILE_1X2:
        tiles[addr] = atlas_->GetTile(GetGid0_1X2FromTerrain(features, tiles, addr));
        break;
    case TILE_2X1:
        tiles[addr] = atlas_->GetTile(GetGid0_2X1FromTerrain(features, tiles, addr));
        break;
    case TILE_2X2:
        tiles[addr] = atlas_->GetTile(GetGid0_2X2FromTerrain(features, tiles, addr));
        break;
    }
#ifdef DUMP_MAPDEBUG_SETTILE
    if (tiles[addr] == Tile::EMPTYPTR && features[addr] > 0)
        URHO3D_LOGWARNINGF("ObjectSkinned() - ReduceDimensionTile_Terrain : Tile Empty feature=%s(%u)", MapFeatureType::GetName(features[addr]), features[addr]);
#endif
    // Reduce dimension on impacted tiles
    switch (tiles[addr]->GetDimensions())
    {
    case TILE_1X2:
//        URHO3D_LOGINFOF("ObjectSkinned() - ReduceDimensionTile_Terrain : Add a TILE_1X2 at %u => impact on %u", addr, addr+GetWidth());
        ReduceDimensionTile_Terrain(TILE_1X1, features, tiles, addr+GetWidth());
        tiles[addr+GetWidth()] = Tile::DIMPART1X2P;
        break;
    case TILE_2X1:
//        URHO3D_LOGINFOF("ObjectSkinned() - ReduceDimensionTile_Terrain : Add a TILE_2X1 at %u => impact on %u", addr, addr+1);
        ReduceDimensionTile_Terrain(TILE_1X1, features, tiles, addr+1);
        tiles[addr+1] = Tile::DIMPART2X1P;
        break;
    case TILE_2X2:
//        URHO3D_LOGINFOF("ObjectSkinned() - ReduceDimensionTile_Terrain : Add a TILE_2X1 at %u => impact on %u, %u, %u", addr, addr+1, addr+GetWidth(), addr+GetWidth()+1);
        ReduceDimensionTile_Terrain(TILE_1X1, features, tiles, addr+1);
        ReduceDimensionTile_Terrain(TILE_1X1, features, tiles, addr+GetWidth());
        ReduceDimensionTile_Terrain(TILE_1X1, features, tiles, addr+GetWidth()+1);
        tiles[addr+1] = Tile::DIMPART2X2RP;
        tiles[addr+GetWidth()] = Tile::DIMPART2X2BP;
        tiles[addr+GetWidth()+1] = Tile::DIMPART2X2BRP;
        break;
    }
}

void ObjectSkinned::SetTileFromSkin(int x, int y, int viewid, int maxdim)
{
    const unsigned width = GetWidth();
    const unsigned height = GetHeight();
    const FeatureType* features = &(feature_->GetFeatureView(viewid)[0]);

    ConnectedMap& connectedView = GetConnectedView(viewid);
    Tile** tiles = &(GetTiledView(viewid)[0]);

    unsigned addr = y*width + x;

    if (skin_->neighborMode_ == Connected4)
    {
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromSkin mode=Connected4 x=%d y=%d viewid=%d", x, y, viewid);
#endif
        SetConnectIndex_4(features, connectedView, tiles, addr, x, y);

        if (x > 0)
            SetConnectIndex_4(features, connectedView, tiles, addr-1, x-1, y);
        if (y > 0)
            SetConnectIndex_4(features, connectedView, tiles, addr-width, x, y-1);
        if (x < width-1)
            SetConnectIndex_4(features, connectedView, tiles, addr+1, x+1, y);
        if (y < height-1)
            SetConnectIndex_4(features, connectedView, tiles, addr+width, x, y-1);
    }
    else if (skin_->neighborMode_ == Connected8)
    {
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromSkin mode=Connected8 x=%d y=%d viewid=%d NOT IMPLEMENTED", x, y, viewid);
    }
    else
    {
        // set the max dimension
        if (!maxdim)
            maxdim = y < height-1 ? (x < width-1 ? TILE_2X2 : TILE_1X2) : (x < width-1 ? TILE_2X1 : TILE_1X1);
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromSkin mode=Connected0 x=%d y=%d viewid=%d feat=%s(%u) maxdim=%d", x, y, viewid, MapFeatureType::GetName(features[addr]), features[addr], maxdim);
#endif
        // reduce dimension
        ReduceDimensionTile_Skin(maxdim, features, tiles, addr);

        // update connectedView (cell + neighborhood)
        SetConnectIndex_0(features, connectedView, addr, x, y, viewid);

        if (x > 0)
            SetConnectIndex_0(features, connectedView, addr-1, x-1, y, viewid);
        if (y > 0)
            SetConnectIndex_0(features, connectedView, addr-width, x, y-1, viewid);
        if (x+1 < width)
            SetConnectIndex_0(features, connectedView, addr+1, x+1, y, viewid);
        if (y+1 < height)
            SetConnectIndex_0(features, connectedView, addr+width, x, y+1, viewid);
    }

    if (features[addr] <= MapFeatureType::InnerSpace && tiles[addr] != Tile::EMPTYPTR)
    {
        tiles[addr] = Tile::EMPTYPTR;
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromSkin x=%d y=%d viewid=%d feat=%s(%u) => cleaning tile !",
                        x, y, viewid, MapFeatureType::GetName(features[addr]), features[addr]);
#endif
    }
}

void ObjectSkinned::SetTileFromTerrain(int x, int y, int viewid, int maxdim)
{
    const unsigned width = GetWidth();
    const unsigned height = GetHeight();
    const FeatureType* features = &(feature_->GetFeatureView(viewid)[0]);

    ConnectedMap& connectedView = GetConnectedView(viewid);
    Tile** tiles = &(GetTiledView(viewid)[0]);

    unsigned addr = y*width + x;

    if (atlas_->GetNeighborMode() == Connected4)
    {
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromTerrain mode=Connected4 x=%d y=%d viewid=%d", x, y, viewid);
#endif
        SetConnectIndex_4(features, connectedView, tiles, addr, x, y);

        if (x > 0)
            SetConnectIndex_4(features, connectedView, tiles, addr-1, x-1, y);
        if (y > 0)
            SetConnectIndex_4(features, connectedView, tiles, addr-width, x, y-1);
        if (x < width-1)
            SetConnectIndex_4(features, connectedView, tiles, addr+1, x+1, y);
        if (y < height-1)
            SetConnectIndex_4(features, connectedView, tiles, addr+width, x, y-1);
    }
    else if (atlas_->GetNeighborMode() == Connected8)
    {
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromTerrain mode=Connected8 x=%d y=%d viewid=%d NOT IMPLEMENTED", x, y, viewid);
    }
    else
    {
        // set the max dimension
        if (!maxdim)
            maxdim = y < height-1 ? (x < width-1 ? TILE_2X2 : TILE_1X2) : (x < width-1 ? TILE_2X1 : TILE_1X1);
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromTerrain mode=Connected0 x=%d y=%d viewid=%d feat=%s(%u) maxdim=%d",
                        x, y, viewid, MapFeatureType::GetName(features[addr]), features[addr], maxdim);
#endif
        // reduce dimension
        ReduceDimensionTile_Terrain(maxdim, features, tiles, addr);

        // update connectedView (cell + neighborhood)
        SetConnectIndex_0(features, connectedView, addr, x, y, viewid);

        if (x > 0)
            SetConnectIndex_0(features, connectedView, addr-1, x-1, y, viewid);
        if (y > 0)
            SetConnectIndex_0(features, connectedView, addr-width, x, y-1, viewid);
        if (x+1 < width)
            SetConnectIndex_0(features, connectedView, addr+1, x+1, y, viewid);
        if (y+1 < height)
            SetConnectIndex_0(features, connectedView, addr+width, x, y+1, viewid);
    }

    if (features[addr] <= MapFeatureType::InnerSpace && tiles[addr] != Tile::EMPTYPTR)
    {
        tiles[addr] = Tile::EMPTYPTR;
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("ObjectSkinned() - SetTileFromTerrain x=%d y=%d viewid=%d feat=%s(%u) => cleaning tile !",
                        x, y, viewid, MapFeatureType::GetName(features[addr]), features[addr]);
#endif
    }
}

void ObjectSkinned::SetTile(int x, int y, int viewid, int maxdim)
{
    assert(x < feature_->width_ && y < feature_->height_);
    assert(viewid < numviews_);

    if (skin_)
        SetTileFromSkin(x, y, viewid, maxdim);
    else
        SetTileFromTerrain(x, y, viewid, maxdim);
}

void ObjectSkinned::Dump() const
{
//    URHO3D_LOGINFOF("Features : FRONTVIEW ViewID=%d ", FrontView_ViewId);
//    GameHelpers::DumpData(&feature_->GetFeatureView(FrontView_ViewId)[0], 1, 2, feature_->width_, feature_->height_);
//    if (GetNumViews() > 3)
//    {
//        URHO3D_LOGINFOF("Features : OUTERVIEW ViewID=%d ", OuterView_ViewId);
//        GameHelpers::DumpData(&feature_->GetFeatureView(OuterView_ViewId)[0], 1, 2, feature_->width_, feature_->height_);
//    }
//    URHO3D_LOGINFOF("Features : INNERVIEW ViewID=%d ", InnerView_ViewId);
//    GameHelpers::DumpData(&feature_->GetFeatureView(InnerView_ViewId)[0], 1, 2, feature_->width_, feature_->height_);
//    if (GetNumViews() > 3)
//    {
//        URHO3D_LOGINFOF("Features : BACKVIEW ViewID=%d ", BackView_ViewId);
//        GameHelpers::DumpData(&feature_->GetFeatureView(BackView_ViewId)[0], 1, 2, feature_->width_, feature_->height_);
//    }
//    URHO3D_LOGINFOF("Features : BACKGROUNDVIEW ViewID=%d ", BackGround_ViewId);
//    GameHelpers::DumpData(&feature_->GetFeatureView(BackGround_ViewId)[0], 1, 2, feature_->width_, feature_->height_);

    const Vector<int>& viewIds = feature_->GetViewIDs(FRONTVIEW);
    for (int i=viewIds.Size()-1; i >= 0; --i)
    {
        int viewid = viewIds[i];

        if (viewid == NOVIEW)
            continue;

        URHO3D_LOGINFOF("ObjectSkinned() - Dump ViewID=%d viewZ=%u: ...", viewid, feature_->viewZ_[viewid]);
        URHO3D_LOGINFOF("Features : ");
        GameHelpers::DumpData(&feature_->GetFeatureView(viewid)[0], 1, 2, feature_->width_, feature_->height_);
        URHO3D_LOGINFOF("Connections : ");
        GameHelpers::DumpData(&connectedViews_[viewid][0], -1, 2, feature_->width_, feature_->height_);
        URHO3D_LOGINFOF("Tiles : ");
        GameHelpers::DumpData(&tiledViews_[viewid][0], -1, 2, feature_->width_, feature_->height_);
    }

    URHO3D_LOGINFOF("Terrains : ");
    GameHelpers::DumpData(&feature_->GetTerrainMap()[0], -1, 2, feature_->width_, feature_->height_);
    URHO3D_LOGINFOF("Biomes : ");
    GameHelpers::DumpData(&feature_->GetBiomeMap()[0], -1, 2, feature_->width_, feature_->height_);

    URHO3D_LOGINFOF("ObjectSkinned() - Dump : DumpViewIDs=%s", feature_->GetDumpViewIds().CString());

    /*
        URHO3D_LOGINFOF("ObjectFeatured() - Dump viewZ=FRONTVIEW FLUID: ...");
        GameHelpers::DumpData(&feature_->GetFluidView(fluidViewIds_[FRONTVIEW].Front()).fluidmap_[0], 1, 2, feature_->width_, feature_->height_);
        URHO3D_LOGINFOF("ObjectFeatured() - Dump viewZ=INNERVIEW FLUID: ...");
        GameHelpers::DumpData(&feature_->GetFluidView(fluidViewIds_[INNERVIEW].Front()).fluidmap_[0], 1, 2, feature_->width_, feature_->height_);
    */
}
