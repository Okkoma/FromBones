#pragma once

#include <Urho3D/Resource/Resource.h>

#include "DefsMap.h"

namespace Urho3D
{
class PListFile;
class Sprite2D;
class Texture2D;
class XMLFile;
class TmxFile2D;
}

using namespace Urho3D;


class TileSheet2D;
class World2DInfo;


class TerrainAtlas : public Resource
{
    URHO3D_OBJECT(TerrainAtlas, Resource);

public:
    TerrainAtlas(Context* context);
    virtual ~TerrainAtlas();
    static void RegisterObject(Context* context);

    virtual bool BeginLoad(Deserializer& source);
    virtual bool EndLoad();

    int GetNeighborMode() const
    {
        return neighborMode_;
    }
    unsigned char GetOrCreateTerrain(const String& name, unsigned property=0);
    unsigned char GetOrCreateSkin(const String& name);
    signed char GetSkinId(const String& name);
    void RegisterTile(Sprite2D* sprite, unsigned properties);
//    void RegisterTile(Sprite2D* sprite, unsigned char terrain, ConnectIndex connect, unsigned char dimensions=0);
    void RegisterDecal(Sprite2D* sprite, unsigned properties);
//    void RegisterDecal(Sprite2D* sprite, unsigned char terrain, ConnectIndex connect);
    void RegisterTileSheet(const String& xmlfile, bool updateScale=false);
    void RegisterTilesFrom(const TmxFile2D& tmxFile);
    void UnregisterTiles();

    void SetWorldInfo(World2DInfo* winfo);
    const World2DInfo* GetWorldInfo() const
    {
        return info_;
    }

    void Clear();

    void Update();

    bool IsRegistered(const String& name) const;
    bool IsRegistered(const StringHash& hashname) const;
    bool UseDecals() const
    {
        return decals_.Size() > 1;
    }
    bool UseDimensionTiles() const
    {
        return useDimensionTiles_;
    }

    unsigned char GetNumBiomeTerrains() const
    {
        return (unsigned char)biomeTerrains_.Size();
    }
    const MapTerrain& GetBiomeTerrain(int biomeid) const
    {
        //assert(biomeid < biomeTerrains_.Size());
        return *(biomeTerrains_[biomeid%biomeTerrains_.Size()]);
    }
    const Vector<MapTerrain>& GetAllTerrains() const
    {
        return terrains_;
    }

    MapTerrain& GetTerrain(unsigned char index)
    {
        return terrains_[index];
    }
    const MapTerrain& GetTerrain(unsigned char index) const
    {
        return terrains_[index];
    }
    MapTerrain& GetTerrain(const StringHash& hashname);
    unsigned char GetTerrainId(const StringHash& hashname) const;

    MapSkin& GetSkin(int index)
    {
        return skins_[index];
    }
    MapSkin& GetSkin(const StringHash& hashname);
    MapSkin* GetSkin(const StringHash& category, int index) const;
    HashMap<unsigned char, unsigned char>& GetSkinRef(int index)
    {
        return skinrefs_[index];
    }

    Vector<int>& GetTileGidTable(unsigned char terrainid, int connectIndex);
    int GetTileGid(unsigned char terrainid, int connectIndex) const;
    int GetTileGid(const StringHash& hashname, int connectIndex) const;
    int GetDecalGid(unsigned char terrainid, int connectIndex, int rand) const;
    Tile* GetTile(int gid) const;
    Tile* GetTile(unsigned char terrainid, int connectIndex) const;
    Tile* GetTileFromConnectValue(unsigned char terrainid, int connectValue) const;
    Tile* GetTileWithSprite(Sprite2D* sprite) const;
    Sprite2D* GetTileSprite(int gid) const;
    Sprite2D* GetTileSprite(unsigned char terrainid, int connectIndex) const;
    Sprite2D* GetTileSprite(const MapTerrain* terrain, int connectIndex) const;
    Sprite2D* GetTileSprite(const MapTerrain* terrain, int connectIndex, int spriteid) const;
    Sprite2D* GetDecalSprite(int gid) const;
    Sprite2D* GetDecalSprite(unsigned char terrainid, int connectIndex, int rand) const;
    int GetRandomTerrainForFeature(FeatureType feature) const;

    void Dump() const;

private :
    bool BeginLoadFromPListFile(Deserializer& source);
    bool EndLoadFromPListFile();
    bool BeginLoadFromXMLFile(Deserializer& source);
    bool EndLoadFromXMLFile();

    void Init();

    Vector<SharedPtr<TileSheet2D> > tileSheets_;
    Vector<StringHash > registeredTileSheets_;

    World2DInfo* info_;
    NeighborMode neighborMode_;
    bool dirtyWorldInfo_;
    bool dirtyTiles_;
    bool useDimensionTiles_;

    /// Tiles
    Vector<Tile* > tiles_;
    /// Decals
    Vector<Tile* > decals_;

    /// Terrains
    Vector<MapTerrain > terrains_;
    Vector<StringHash > terrainhashes_;
    Vector<MapTerrain* > biomeTerrains_;

    /// Skins
    Vector<MapSkin > skins_;
    Vector<StringHash > skinhashes_;
    HashMap<StringHash, Vector<MapSkin* > > skinsByCategory_;

    /// skinrefs[skinid] = table des terrains par features
    /// => skinrefs[1][MapFeatureType::OuterGround] = TerrainID pour le type OuterGround de la skin 01
    Vector<HashMap<unsigned char, unsigned char> > skinrefs_;

    /// Compatible Terrains (terrainId) by feature
    Vector<Vector<int > > compatibleTerrainsByFeature_;

    /// PList file used while loading.
    SharedPtr<PListFile> loadPListFile_;
    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;
};
