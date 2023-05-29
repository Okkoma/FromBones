#pragma once

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Urho2D/Drawable2D.h>

#include "DefsMap.h"
#include "DefsChunks.h"
#include "DefsFluids.h"

#include "GameOptions.h"

#include "ObjectFeatured.h"


class World2DInfo;
class Map;

struct ObjectSkinned : public RefCounted
{
    ObjectSkinned();
    ObjectSkinned(const ObjectSkinned& t) { }
    ObjectSkinned(ObjectFeatured* feature, TerrainAtlas* atlas, MapTerrain* terrain, MapSkin* skin=0);
    ObjectSkinned(unsigned width, unsigned height, unsigned numviews=0);
    ~ObjectSkinned();

    void Clear();
    void Resize(unsigned width, unsigned height, unsigned numviews);//, bool featureShared=false);

    void Set(ObjectFeatured* feature, TerrainAtlas* atlas, MapTerrain* terrain, MapSkin* skin=0);
    void SetNumViews(unsigned numviews);
    void SetFeature(ObjectFeatured* feature=0);
    void SetTerrain(MapTerrain* terrain);
    void SetSkin(MapSkin* skin);
    void SetSkin(const String& name);
    void SetSkin(const String& category, unsigned index);
    void SetAtlas(TerrainAtlas* atlas);

    bool UseDimensionTiles() const
    {
        return (skin_ ? skin_->neighborMode_ == Connected0 : (terrain_ ? terrain_->UseDimensionTiles() : true));
    }
    FeaturedMap& GetFeaturedView(unsigned id);
    ConnectedMap& GetConnectedView(unsigned id);

    TiledMap& GetTiledView(unsigned id);
    const Vector<ConnectedMap>& GetConnectedViews() const;
    const Vector<TiledMap>& GetTiledViews() const;
    ObjectFeatured* GetFeatureData() const
    {
        return feature_;
    }

    TerrainAtlas* GetAtlas() const
    {
        return atlas_;
    }
    MapSkin* GetSkin() const
    {
        return skin_;
    }
    MapTerrain* GetTerrain() const
    {
        return terrain_;
    }

    const unsigned& GetWidth() const
    {
        return feature_->width_;
    }
    const unsigned& GetHeight() const
    {
        return feature_->height_;
    }
    const unsigned& GetNumViews() const
    {
        return numviews_;
    }

    bool SetViews(HiresTimer* timer=0, const long long& delay=0);
    void SetView(int viewid);

    void SetTile(int x, int y, int viewid, int maxdim=0);

    void Dump() const;

    MapBase* map_;
    unsigned indexToSet_, indexVToSet_;

private :
#ifdef USE_TILERENDERING
    friend class ObjectTiled;

#endif
    void Init();

    inline int GetGid42FromSkin(const SkinData& sdata, const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const;
    inline int GetGid43FromSkin(const SkinData& sdata, const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const;
    inline int GetGid44FromSkin(const SkinData& sdata, const FeatureType* features, unsigned addr, ConnectIndex& index) const;
    inline int GetGid42FromTerrain(const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const;
    inline int GetGid43FromTerrain(const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const;
    inline int GetGid44FromTerrain(const FeatureType* features, unsigned addr, ConnectIndex& index) const;

    inline int GetGid82FromSkin(const SkinData& sdata, const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const;
    inline int GetGid83FromSkin(const SkinData& sdata, const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const;
    inline int GetGid84FromSkin(const SkinData& sdata, const FeatureType* features, unsigned addr, ConnectIndex& index) const;
    inline int GetGid82FromTerrain(const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2) const;
    inline int GetGid83FromTerrain(const FeatureType* features, unsigned addr, ConnectIndex& index, const short int& nghb1, const short int& nghb2, const short int& nghb3) const;
    inline int GetGid84FromTerrain(const FeatureType* features, unsigned addr, ConnectIndex& index) const;

    inline int GetGid0_1X1FromSkin(const MapSkin& skin, const FeatureType* features, unsigned addr) const;
    inline int GetGid0_2X1FromSkin(const MapSkin& skin, const FeatureType* features, const TilePtr* render, unsigned addr) const;
    inline int GetGid0_1X2FromSkin(const MapSkin& skin, const FeatureType* features, const TilePtr* render, unsigned addr) const;
    inline int GetGid0_2X2FromSkin(const MapSkin& skin, const FeatureType* features, const TilePtr* render, unsigned addr) const;
    inline int GetGid0_1X1FromTerrain(const FeatureType* features, unsigned addr) const;
    inline int GetGid0_2X1FromTerrain(const FeatureType* features, const TilePtr* render, unsigned addr) const;
    inline int GetGid0_1X2FromTerrain(const FeatureType* features, const TilePtr* render, unsigned addr) const;
    inline int GetGid0_2X2FromTerrain(const FeatureType* features, const TilePtr* render, unsigned addr) const;

    inline void SetConnectIndex_4(const FeatureType* features, ConnectedMap& connections, Tile** tiles, unsigned addr, int x, int y);
//    inline void SetConnectIndex_0(const FeatureType* features, ConnectedMap& connections, unsigned addr, int x, int y);
    inline void SetConnectIndex_0(const FeatureType* features, ConnectedMap& connections, unsigned addr, int x, int y, int viewid);

    void ReduceDimensionTile_Skin(int dimension, const FeatureType* features, Tile** tiles, unsigned addr);
    void ReduceDimensionTile_Terrain(int dimension, const FeatureType* features, Tile** tiles, unsigned addr);
    void SetTileFromSkin(int x, int y, int viewid, int maxdim=0);
    void SetTileFromTerrain(int x, int y, int viewid, int maxdim=0);
    void SetViewFromSkin(unsigned viewid);
    void SetViewFromTerrain( unsigned viewid);

    SharedPtr<ObjectFeatured> feature_;

    MapSkin* skin_;
    MapTerrain* terrain_;
    TerrainAtlas* atlas_;

    unsigned numviews_;

    Vector<ConnectedMap> connectedViews_;
    Vector<TiledMap> tiledViews_;
};

