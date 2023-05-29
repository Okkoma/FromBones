#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/PListFile.h>
#include <Urho3D/Resource/XMLFile.h>

#include "DefsWorld.h"

#include "GameHelpers.h"

#include "TileSheet2D.h"

#include "TerrainAtlas.h"

extern const char* NeighborModeStr[];

/// TerrainAtlas class

TerrainAtlas::TerrainAtlas(Context* context) :
    Resource(context),
    neighborMode_(Connected0),
    dirtyTiles_(false),
    dirtyWorldInfo_(false)
{
    Init();
}

TerrainAtlas::~TerrainAtlas()
{
    UnregisterTiles();
    URHO3D_LOGINFOF("~TerrainAtlas() - this = %u !", this);
}

bool TerrainAtlas::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    Clear();

    Init();

    String extension = GetExtension(source.GetName());
    if (extension == ".plist")
        return BeginLoadFromPListFile(source);

    if (extension == ".xml")
        return BeginLoadFromXMLFile(source);

    URHO3D_LOGERROR("Unsupported file type");
    return false;
}

bool TerrainAtlas::EndLoad()
{
    if (loadPListFile_)
        return EndLoadFromPListFile();

    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    return false;
}

void TerrainAtlas::Clear()
{
//    URHO3D_LOGINFOF("TerrainAtlas() - Clear this = %u !", this);

    UnregisterTiles();

    tileSheets_.Clear();
    registeredTileSheets_.Clear();
    terrains_.Clear();
    terrainhashes_.Clear();
    biomeTerrains_.Clear();
    skins_.Clear();
    skinhashes_.Clear();
    skinrefs_.Clear();
    skinsByCategory_.Clear();

    compatibleTerrainsByFeature_.Clear();
}

void TerrainAtlas::Init()
{
    if (!tiles_.Size())
    {
        tiles_.Push(Tile::EMPTYPTR);
    }
    if (!decals_.Size())
    {
        decals_.Push(Tile::EMPTYPTR);
    }

    terrains_.Reserve(16);
}

void TerrainAtlas::RegisterObject(Context* context)
{
//    URHO3D_LOGINFOF("TerrainAtlas() - RegisterObject() : ... ");

    context->RegisterFactory<TerrainAtlas>();

    URHO3D_LOGINFOF("TerrainAtlas() - RegisterObject() : ... OK !");
}



/// add new or get existing terrain and return index
unsigned char TerrainAtlas::GetOrCreateTerrain(const String& name, unsigned property)
{
    StringHash hashname = StringHash(name);
    Vector<StringHash>::ConstIterator it = terrainhashes_.Find(hashname);

    unsigned char index;

    if (it != terrainhashes_.End())
    {
        index = it-terrainhashes_.Begin();
//        URHO3D_LOGINFOF("TerrainAtlas() - GetOrCreateTerrain : existing name=%s id=%u", name.CString(), index);
    }
    else
    {
        index = terrains_.Size();

        URHO3D_LOGINFOF("TerrainAtlas() - GetOrCreateTerrain : new name=%s id=%u property=%u ...", name.CString(), index, property);

        terrains_.Push(MapTerrain(name, property));
        terrainhashes_.Push(hashname);

        if (property & TERRAINPROPERTY_BIOME)
        {
            biomeTerrains_.Push(&terrains_.Back());
        }
    }

    return index;
}

/// add new or get existing skin and return index
unsigned char TerrainAtlas::GetOrCreateSkin(const String& name)
{
    StringHash hashname = StringHash(name);
    Vector<StringHash>::ConstIterator it = skinhashes_.Find(hashname);

    unsigned char index;

    if (it != skinhashes_.End())
    {
        index = it-skinhashes_.Begin();
    }
    else
    {
        index = skins_.Size();
        skins_.Push(MapSkin());
        skins_.Back().name_ = name;
        skinrefs_.Push(HashMap<unsigned char, unsigned char>());
        skinhashes_.Push(hashname);
    }

//    URHO3D_LOGINFOF("TerrainAtlas() - GetOrCreateSkin : name=%s id=%u", name.CString(), index);

    return index;
}

signed char TerrainAtlas::GetSkinId(const String& name)
{
    Vector<StringHash>::ConstIterator it = skinhashes_.Find(StringHash(name));
    return it != skinhashes_.End() ? it-skinhashes_.Begin() : -1;
}

void TerrainAtlas::Update()
{
    if (dirtyTiles_)
    {
        tiles_.Compact();

        /// Update use Dimension Tiles Tag
        useDimensionTiles_ = false;
        for (unsigned i=0; i < terrains_.Size(); i++)
        {
            if (terrains_[i].UseDimensionTiles())
            {
                useDimensionTiles_ = true;
                break;
            }
        }
        /// Update compatible Terrains for each feature
        compatibleTerrainsByFeature_.Resize(MapFeatureType::GetListSize());

        FeatureType j=0;
        for (Vector<Vector<int > >::Iterator it=compatibleTerrainsByFeature_.Begin(); it!=compatibleTerrainsByFeature_.End(); ++it,++j)
            for (unsigned i=0; i < terrains_.Size(); i++)
            {
                if (terrains_[i].HasCompatibleFeature(j))
                    (*it).Push(i);
            }

        /// Update Skins Data Pointers
        for (unsigned i=0; i < skins_.Size(); i++)
        {
            SkinData& skindata = skins_[i].skinData_;
            HashMap<unsigned char, unsigned char>& skinref = skinrefs_[i];

            for (HashMap<unsigned char, unsigned char>::Iterator it=skinref.Begin(); it!=skinref.End(); ++it)
                skindata[it->first_] = &(terrains_[it->second_]);
        }

        dirtyTiles_ = false;

        /// Update Skin Categories
        skinsByCategory_.Clear();
        for (unsigned i=0; i < skins_.Size(); i++)
        {
            MapSkin& skin = skins_[i];
            StringHash key(skin.name_.Split('_')[0]);
            skinsByCategory_[key].Push(&skin);
        }

//        URHO3D_LOGINFOF("TerrainAtlas() - Update Terrain Features !");
    }

    if (dirtyWorldInfo_ && info_ && tileSheets_.Size())
    {
//        URHO3D_LOGINFOF("TerrainAtlas() - %s Update Scaled Tiles ... ", GetName().CString());

        Vector2 scale(info_->tileWidth_, info_->tileHeight_);

        for (unsigned i=0; i<tileSheets_.Size(); i++)
            tileSheets_[i]->UpdateTileScale(scale);

        dirtyWorldInfo_ = false;

//        URHO3D_LOGINFOF("TerrainAtlas() - Update Scaled Tiles OK !");
    }
}

bool TerrainAtlas::IsRegistered(const String& name) const
{
    return IsRegistered(StringHash(name));
}

bool TerrainAtlas::IsRegistered(const StringHash& hashname) const
{
    Vector<StringHash>::ConstIterator it = registeredTileSheets_.Find(hashname);
    return (it != registeredTileSheets_.End());
}

void TerrainAtlas::SetWorldInfo(World2DInfo* winfo)
{
    if (winfo)
    {
        info_ = winfo;
        dirtyWorldInfo_ = true;

        Update();
    }
}


bool TerrainAtlas::BeginLoadFromPListFile(Deserializer& source)
{
    return true;
}

bool TerrainAtlas::EndLoadFromPListFile()
{
    return true;
}

bool TerrainAtlas::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XMLFile(context_);
    if (!loadXMLFile_->Load(source))
    {
        URHO3D_LOGERRORF("TerrainAtlas() - BeginLoadFromXMLFile : Could not load TerrainAtlas %s", source.GetName().CString());
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    XMLElement rootElem = loadXMLFile_->GetRoot("TerrainAtlas");
    if (!rootElem)
    {
        URHO3D_LOGERRORF("TerrainAtlas() - BeginLoadFromXMLFile : Invalid TerrainAtlas", source.GetName().CString());
        loadXMLFile_.Reset();
        return false;
    }

//    if (GetAsyncLoadState() == ASYNC_LOADING)
//        GetSubsystem<ResourceCache>()->BackgroundLoadResource<Texture2D>(rootElem.GetAttribute("texture"), true, this);

//    URHO3D_LOGINFOF("TerrainAtlas() - Loading From XML : %s ...", source.GetName().CString());
    return true;
}

bool TerrainAtlas::EndLoadFromXMLFile()
{
//    URHO3D_LOGINFOF("TerrainAtlas() - EndLoadFromXMLFile : ...");

    XMLElement rootElem = loadXMLFile_->GetRoot("TerrainAtlas");

    neighborMode_ = (NeighborMode)GetEnumValue(rootElem.GetAttribute("NghMode"), NeighborModeStr);

    for (XMLElement tileSheetElem = rootElem.GetChild("TileSheet"); tileSheetElem; tileSheetElem = tileSheetElem.GetNext("TileSheet"))
        RegisterTileSheet(tileSheetElem.GetAttribute("name"));

    loadXMLFile_.Reset();

    URHO3D_LOGINFOF("TerrainAtlas() - Loading From XML %s ... OK !", GetName().CString());
//    URHO3D_LOGINFOF("TerrainAtlas() - EndLoadFromXMLFile : ... OK !");

//    Dump();

    return true;
}

void TerrainAtlas::RegisterTileSheet(const String& name, bool updateScale)
{
    TileSheet2D::sAtlas_ = this;

//    URHO3D_LOGINFO("TerrainAtlas() - RegisterTileSheet ... " + name);

    SharedPtr<TileSheet2D> tilesheet(context_->GetSubsystem<ResourceCache>()->GetTempResource<TileSheet2D>(name));
    if (!tilesheet)
    {
        URHO3D_LOGERRORF("TerrainAtlas() - RegisterTileSheet : No TileSheet2D File %s !", name.CString());
        return;
    }

    StringHash hashname(tilesheet->GetName());
    if (!IsRegistered(hashname))
    {
        tileSheets_.Push(tilesheet);
        registeredTileSheets_.Push(hashname);

        dirtyTiles_ = true;

        Update();

        if (updateScale)
            tilesheet->UpdateTileScale(Vector2(info_->tileWidth_, info_->tileHeight_));

        URHO3D_LOGINFOF("TerrainAtlas() - RegisterTileSheet Atlas=%s TileSheet=%s UpdateScaleNow=%s ... OK !", GetName().CString(), tilesheet->GetName().CString(), updateScale?"true":"false");
    }
}

void TerrainAtlas::RegisterTile(Sprite2D* sprite, unsigned properties)
{
    // don't check for existing sprite => create always new tile
    // => because of unicity of Tile property Tile::terrain_
    Tile* tile = 0;
//    Tile* tile = GetTileWithSprite(sprite);

    int gid = tile ? tile->GetGid() : tiles_.Size();

    if (!tile)
    {
        tile = new Tile(gid);
        tile->AddSprite(sprite);

        // Set Properties (block, terrain, connectiontype)
        tile->SetProperties(properties);

        tiles_.Push(tile);
        tile = tiles_.Back();
        URHO3D_LOGINFOF("TerrainAtlas() - RegisterTile : New gid=%d terrain=%u, connectIndex=%u, dimensions=%u ptr=%u",
                        tile->GetGid(), tile->GetTerrain(), tile->GetConnectivity(), tile->GetDimensions(), tile);
    }
    else
    {
        URHO3D_LOGINFOF("TerrainAtlas() - RegisterTile : Existing gid=%d terrain=%u, connectIndex=%u, dimensions=%u ptr=%u",
                        tile->GetGid(), tile->GetTerrain(), tile->GetConnectivity(), tile->GetDimensions(), tile);
    }

    // Add Connection Type for this tile to MapTerrain
    MapTerrain& terrain = GetTerrain(tile->GetTerrain());
    terrain.AddConnectIndexToTileGid(gid, tile->GetConnectivity());

    if (tile->GetDimensions())
        terrain.AddGidToDimension(gid, tile->GetDimensions());
    // correct dimension for Connect4,Connect8 base TILE_1X1
    else
        tile->SetDimensions(TILE_1X1);
}

void TerrainAtlas::RegisterDecal(Sprite2D* sprite, unsigned properties)
{
    int newgid = decals_.Size();
    Tile* tile = new Tile(newgid);

    tile->AddSprite(sprite);

    // Set Properties (block, terrain, connectiontype)
    tile->SetProperties(properties);

    // Add Connection Type for this decal to MapTerrain
    MapTerrain& terrain = GetTerrain(tile->GetTerrain());
    terrain.AddConnectIndexToDecalGid(newgid, tile->GetConnectivity());

//    URHO3D_LOGINFOF("TerrainAtlas() - this=%u RegisterDecal : gid=%d terrain=%s, connectIndex=%u",
//                    this, newgid, terrain->GetName().CString(), tile->GetConnectivity());

    decals_.Push(tile);
}

void TerrainAtlas::RegisterTilesFrom(const TmxFile2D& tmxFile)
{
    URHO3D_LOGINFOF("TerrainAtlas() - RegisterTilesFrom(TMX) : ...");
//
//    // Set Tiles if need
//    const Vector<SharedPtr<Tile2D> >& tmxTiles = tmxFile.GetAllTiles();
//
//    if (tmxTiles.Size() <= 1)
//    {
//        URHO3D_LOGERRORF("TerrainAtlas() - RegisterTilesFrom(TMX) : tmxTiles Size < 2 ! ");
//        return;
//    }
//
//    if (!tiles_.Size())
//    {
//        URHO3D_LOGINFOF("TerrainAtlas() - RegisterTilesFrom(TMX) : New Tiles => Tiles num=%u", tmxTiles.Size());
//        tiles_.Reserve(tmxTiles.Size()+1);
//        tiles_.Push(0);//[0] = 0;
//    }
//    else
//    {
//        URHO3D_LOGINFOF("TerrainAtlas() - RegisterTilesFrom(TMX) : Add To Existing Tiles");
//    }
//
//    // Start at index 1 : index 0 = empty tile
//    int currentgid = 0;
//    int newgid = 1;
//    int registergid = 0;
//
//    // Set Tiles
//    char terrain = 0, connect, flags;
//    for (Vector<SharedPtr<Tile2D> >::ConstIterator it=tmxTiles.Begin()+1; it!=tmxTiles.End(); ++it)
//    {
//        const Tile2D* tile2D = *it;
//        Tile* tile;
//
//        /// Get Terrain Name && Connection
//        if (tile2D->HasProperty(TileTerrainProperty))
//        {
//            Vector<String> terrainProps = tile2D->GetProperty(TileTerrainProperty).Split('|');
//            terrain = AddTerrain(terrainProps[0]);
//            connect = MapTilesConnectType::GetIndex(terrainProps[1].CString());
//
////            Vector<int>& gtable = GetGidTable(terrain, connect);
////            registergid = gtable.Size() ? gtable.Back() : 0;
//            registergid = 0;
//        }
//        else
//            registergid = 0;
//
//        /// Not Register Tile
//        if (!registergid)
//        {
//            URHO3D_LOGINFOF("TerrainAtlas() - RegisterTilesFrom(TMX) : Register NewTile ... gid=%d", currentgid);
//            // Set New Typed Tile
//            // Animated Tile
//            if (tile2D->HasProperty(TileAnimatedProperty))
//            {
//                Vector<String> values = tile2D->GetProperty(TileAnimatedProperty).Split(':');
//                int toGid =  newgid + ToInt(values[0]) - 1;
//                int tickDelay = ANIMATEDTILE_DEFAULT_TICKDELAY;
//                int loopDelay = ANIMATEDTILE_DEFAULT_LOOPDELAY;
//                if (values.Size()>1)
//                    tickDelay = ToInt(values[1]);
//                if (values.Size()>2)
//                    loopDelay = ToInt(values[2]);
//
////                tiles_[gid] = new AnimatedTile(gid, toGid, tickDelay, loopDelay);
//                tile = new AnimatedTile(newgid, toGid, tickDelay, loopDelay);
//                tiles_.Push(tile);
//                currentgid = newgid;
////                URHO3D_LOGINFOF("=> new AnimatedTile gid=%d togid=%d", gid, toGid);
//                newgid++;
//            }
////            // Breakable Tile
////            else if (tile2D->HasProperty(TileBreakableProperty))
////            {
//////                tiles_[gid] = new BreakableTile(newgid);
////                tile = new BreakableTile(newgid);
////                tiles_.Push(tile);
////                newgid++;
////            }
//            // Default Tile
//            else
//            {
////                tiles_[gid] = new Tile(gid);
//                tile = new Tile(newgid);
//                tiles_.Push(tile);
//                currentgid = newgid;
//
//                newgid++;
//            }
//        }
//        /// Already registered
//        else
//        {
//            tile = tiles_[registergid];
//            currentgid = registergid;
//            URHO3D_LOGINFOF("TerrainAtlas() - RegisterTilesFrom(TMX) : Existing Tile ... gid=%d", currentgid);
//        }
//
//        /// Add Properties (Features, Furnitures, Block)
//        if (currentgid)
//        {
//    //        if (tiles_[gid]->AddSprite(tile2D->GetSprite()))
//            if (tile->AddSprite(tile2D->GetSprite()) && !registergid)
//            {
//                // Set Tile Properties
//                if (tile2D->HasProperty(TileTerrainProperty))
//                {
//                    MapTerrain& mapterrain = GetTerrain(terrain);
//                    // Add Connection Type for this tile to MapTerrain
//                    mapterrain.AddConnectivityToGid(currentgid, connect);
//    //                URHO3D_LOGINFOF("=> AddConnectivityToGid gid=%d string=%s terrain=%u, connect=%u",
//    //                         gid, tile2D->GetProperty(TileTerrainProperty).CString(), terrain, connect);
//
//                    // Add Features Compatibility to MapTerrain
//                    if (tile2D->HasProperty(TileTerrainFeatureCompatiblity))
//                    {
//                        Vector<String> features = tile2D->GetProperty(TileTerrainFeatureCompatiblity).Split('|');
//                        for (int i=0;i<features.Size();++i)
//                        {
////                            mapterrain.AddCompatibleFeature(MapFeatureType::GetFeature(features[i]));
//                            mapterrain.AddCompatibleFeature(MapFeatureType::GetIndex(features[i].CString()));
//                            URHO3D_LOGINFOF("=> AddCompatibleFeature To terrain=%s(%d) feature=%s(%d)",
//                                     mapterrain.GetName().CString(), terrain, features[i].CString(),
//                                     MapFeatureType::GetIndex(features[i].CString()));
//                        }
//                    }
//                    // Add Furnitures Compatibility to MapTerrain
//                    if (tile2D->HasProperty(TileTerrainFurnitureCompatiblity))
//                    {
//                        Vector<String> values = tile2D->GetProperty(TileTerrainFurnitureCompatiblity).Split('|');
//                        for (int i=0;i<values.Size();i++)
//                        {
//                            Vector<String> str = values[i].Split(':');
//
//                            if (str.Size() > 1)
//                            {
//                                for (int j=1;j<str.Size();j++)
//                                {
//                                    mapterrain.AddCompatibleFurniture(MapFurnitureType::GetIndex(str[0].CString()), MapTilesConnectType::GetIndex(str[j].CString()));
//                                    URHO3D_LOGINFOF("=> AddCompatibleFurniture To terrain=%s(%d) furniture=%s(%d) connect=%s(%d)",
//                                             mapterrain.GetName().CString(), terrain,  str[0].CString(), MapFurnitureType::GetIndex(str[0].CString()),
//                                             str[j].CString(), MapTilesConnectType::GetIndex(str[j].CString()));
//                                }
//                            }
//                            else
//                            {
//                                mapterrain.AddCompatibleFurniture(MapFurnitureType::GetIndex(str[0].CString()));
//                                URHO3D_LOGINFOF("=> AddCompatibleFurniture To terrain=%s(%d) furniture=%s(%d) connect=0",
//                                         mapterrain.GetName().CString(), terrain,  str[0].CString(), MapFurnitureType::GetIndex(str[0].CString()));
//                            }
//                        }
//                    }
//                }
//                else
//                {
//                    terrain = 0;
//                    connect = 0;
//                }
//
//                // Set Tile Properties
//                flags = 0;
//                if (tile2D->HasProperty(TileBlockedProperty))
//                {
//                    flags = ToUInt(tile2D->GetProperty(TileBlockedProperty)) | BlockProperty;
//    //                URHO3D_LOGINFOF("=> AddProperty block flags=%d", flags);
//                }
//    //            else
//    //                URHO3D_LOGINFOF("=> AddProperty flags=%d", flags);
//
//                tile->SetProperties(TileProperty(flags, terrain, connect));
//            }
//        }
//    }
//
//    Update();
//
//    // Dump Terrains
//    for (unsigned i=0;i<terrains_.Size();++i)
//        terrains_[i].Dump();
//
//    URHO3D_LOGINFOF("TerrainAtlas() - RegisterTilesFrom(TMX) : ... OK !");
}

void TerrainAtlas::UnregisterTiles()
{
//    URHO3D_LOGINFOF("TerrainAtlas() - UnregisterTiles() : this = %u...", this);

    for (Vector<Tile* >::Iterator it=tiles_.Begin(); it!=tiles_.End(); ++it)
        if (*it != Tile::EMPTYPTR) delete (*it);

    tiles_.Clear();

    for (Vector<Tile* >::Iterator it=decals_.Begin(); it!=decals_.End(); ++it)
        if (*it != Tile::EMPTYPTR) delete (*it);

    decals_.Clear();

//    URHO3D_LOGINFOF("TerrainAtlas() - UnregisterTiles() : this = %u ... OK !", this);
}

MapTerrain& TerrainAtlas::GetTerrain(const StringHash& hashname)
{
    // warning : no vector end() check
    Vector<StringHash>::ConstIterator it = terrainhashes_.Find(hashname);
    return terrains_[it-terrainhashes_.Begin()];
}

unsigned char TerrainAtlas::GetTerrainId(const StringHash& hashname) const
{
    Vector<StringHash>::ConstIterator it = terrainhashes_.Find(hashname);

    if (it != terrainhashes_.End())
        return (unsigned char)(it-terrainhashes_.Begin());

    URHO3D_LOGWARNINGF("TerrainAtlas() - GetTerrainId : not find terrain for hashname=%u", hashname.Value());

    return 0;
}

MapSkin& TerrainAtlas::GetSkin(const StringHash& hashname)
{
    // warning : no vector end() check
    Vector<StringHash>::ConstIterator it = skinhashes_.Find(hashname);
    return skins_[it-skinhashes_.Begin()];
}

MapSkin* TerrainAtlas::GetSkin(const StringHash& category, int index) const
{
    HashMap<StringHash, Vector<MapSkin* > >::ConstIterator it = skinsByCategory_.Find(category);
    if (it != skinsByCategory_.End())
    {
        const Vector<MapSkin* >& skins = it->second_;
        if (skins.Size())
            return skins[index % skins.Size()];
    }

    return 0;
}

Vector<int>& TerrainAtlas::GetTileGidTable(unsigned char terrain, int connectIndex)
{
    return terrains_[terrain].GetTileGidTableForConnectIndex(connectIndex);
}

int TerrainAtlas::GetTileGid(unsigned char terrain, int connectIndex) const
{
//    return terrains_[terrain].GetGidForConnectivity(connect);
    return terrains_[terrain].GetRandomTileGidForConnectIndex(connectIndex);
}

int TerrainAtlas::GetTileGid(const StringHash& hashname, int connectIndex) const
{
    unsigned char terrain = GetTerrainId(hashname);
    return terrains_[terrain].GetRandomTileGidForConnectIndex(connectIndex);
}

int TerrainAtlas::GetDecalGid(unsigned char terrain, int connectIndex, int rand) const
{
    const Vector<int>& decalGids = terrains_[terrain].GetDecalGidTableForConnectIndex(connectIndex);
    return decalGids[decalGids.Size() > 1 ? rand % decalGids.Size() : 0];
}

Tile* TerrainAtlas::GetTile(int gid) const
{
    return tiles_[gid];
}

Tile* TerrainAtlas::GetTile(unsigned char terrain, int connectIndex) const
{
    return tiles_[terrains_[terrain].GetRandomTileGidForConnectIndex(connectIndex)];
}

Tile* TerrainAtlas::GetTileFromConnectValue(unsigned char terrain, int connectValue) const
{
    return tiles_[terrains_[terrain].GetRandomTileGidForConnectValue(connectValue)];
}

bool CompareSprite2D(Sprite2D* spr1, Sprite2D* spr2)
{
    if (spr1 == spr2)
        return true;

    if (spr1->GetTexture() != spr2->GetTexture())
        return false;

    if (spr1->GetRectangle() != spr2->GetRectangle())
        return false;

    if (spr1->GetHotSpot() != spr2->GetHotSpot())
        return false;

    if (spr1->GetOffset() != spr2->GetOffset())
        return false;

    return true;
}

Tile* TerrainAtlas::GetTileWithSprite(Sprite2D* sprite) const
{
    Tile* tile;
    for (Vector<Tile* >::ConstIterator it=tiles_.Begin(); it!=tiles_.End(); ++it)
    {
        tile = *it;
        if (tile == Tile::EMPTYPTR)
            continue;

        if (CompareSprite2D(tile->GetSprite(), sprite))
            return tile;
    }
    return 0;
}

Sprite2D* TerrainAtlas::GetTileSprite(int gid) const
{
    return tiles_[gid]->GetSprite();
}

Sprite2D* TerrainAtlas::GetTileSprite(unsigned char terrainid, int connectIndex) const
{
    return tiles_[GetTileGid(terrainid, connectIndex)]->GetSprite();
}

Sprite2D* TerrainAtlas::GetTileSprite(const MapTerrain* terrain, int connectIndex) const
{
    return tiles_[terrain->GetRandomTileGidForConnectIndex(connectIndex)]->GetSprite();
}

Sprite2D* TerrainAtlas::GetTileSprite(const MapTerrain* terrain, int connectIndex, int spriteid) const
{
    return tiles_[terrain->GetTileGidTableForConnectIndex(connectIndex)[spriteid]]->GetSprite();
}

Sprite2D* TerrainAtlas::GetDecalSprite(int gid) const
{
    return decals_[gid]->GetSprite();
}

Sprite2D* TerrainAtlas::GetDecalSprite(unsigned char terrainid, int connectIndex, int rand) const
{
    return decals_[GetDecalGid(terrainid, connectIndex, rand)]->GetSprite();
}


//int Map::GetTileConnectionIndex(int connectValue)
//{
//    return tileConnectIndexbyValue[connectValue];
//}

int TerrainAtlas::GetRandomTerrainForFeature(FeatureType feature) const
{
    if (feature < compatibleTerrainsByFeature_.Size())
    {
        int size = (int) compatibleTerrainsByFeature_[feature].Size();
        return size > 0 ? compatibleTerrainsByFeature_[feature][Random(size)] : -1;
    }
    return -1;
}

void TerrainAtlas::Dump() const
{
    URHO3D_LOGINFOF("TerrainAtlas() - Dump : Name=%s ptr=%u Num Terrains=%u Num Skins=%u ...", GetName().CString(), this, terrains_.Size(), skins_.Size());

    for (int i = 0; i < GetNumBiomeTerrains(); ++i)
    {
        const MapTerrain& terrain = GetBiomeTerrain(i);
        URHO3D_LOGINFOF("Dump BiomeTerrain biomeid=%d => terrain=%s ptr=%u", i, terrain.GetName().CString(), &terrain);
    }

    for (int i = 0; i < terrains_.Size(); ++i)
    {
        const MapTerrain& terrain = terrains_[i];
        URHO3D_LOGINFOF("Dump Terrains index=%u => terrain=%s ptr=%u", i, terrain.GetName().CString(), &terrain);
    }

    /*
        // Dump Terrains
        for (unsigned i = 0; i < terrains_.Size(); ++i)
            terrains_[i].Dump();

        // Dump Skins
        for (HashMap<StringHash, Vector<MapSkin* > >::ConstIterator cat=skinsByCategory_.Begin(); cat != skinsByCategory_.End(); ++cat)
        {
            URHO3D_LOGINFOF("Dump SkinCategory hash=%u :", cat->first_.Value());
            const Vector<MapSkin* >& skins = cat->second_;
            for (Vector<MapSkin* >::ConstIterator skn = skins.Begin(); skn != skins.End(); ++skn)
            {
                MapSkin* skin = *skn;
                URHO3D_LOGINFOF(" => Skin name=%s(ptr=%u) nghMode=%s :", skin->name_.CString(), skin, NeighborModeStr[skin->neighborMode_]);
                for (SkinData::ConstIterator it= skin->skinData_.Begin(); it != skin->skinData_.End(); ++it)
                {
                    URHO3D_LOGINFOF("   => associate Feature=%s(%u) With Terrain=%s(Ptr=%u)", MapFeatureType::GetName(it->first_), it->first_, (it->second_)->GetName().CString(), it->second_);
                }
            }
        }
    */
    URHO3D_LOGINFOF("TerrainAtlas() - Dump : Name=%s ... OK !", GetName().CString());
}
