#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/PListFile.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Urho2D/Sprite2D.h>

#include "GameHelpers.h"
#include "GameContext.h"
#include "ScrapsEmitter.h"
#include "TerrainAtlas.h"

#include "TileSheet2D.h"


extern const char* NeighborModeStr[];


// Tile spanOffset : Prevent holes between tiles (sprite2D::SetFixedRectangles)
// good value 0.25f to 0.35f (0.25f some gap)
//const float TILESPANNING = 0.3f*PIXEL_SIZE;
//const float TILESPANNING = 0.15f*PIXEL_SIZE;

TerrainAtlas* TileSheet2D::sAtlas_ = 0;


TileSheet2D::TileSheet2D(Context* context) :
    Resource(context)
{
}

TileSheet2D::~TileSheet2D()
{
    URHO3D_LOGDEBUG("~TileSheet2D() - " + GetName());
}

void TileSheet2D::RegisterObject(Context* context)
{
//    URHO3D_LOGINFO("TileSheet2D() - RegisterObject() : ... ");
    context->RegisterFactory<TileSheet2D>();
    URHO3D_LOGINFO("TileSheet2D() - RegisterObject() : ... OK !");
}

bool TileSheet2D::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    sprites_.Clear();
    flippings_.Clear();

    String extension = GetExtension(source.GetName());

    if (extension == ".xml")
        return BeginLoadFromXMLFile(source);

    URHO3D_LOGERRORF("TileSheet2D() - BeginLoad : Unsupported file type (%s) !", source.GetName().CString());
    return false;
}

bool TileSheet2D::EndLoad()
{
    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    return false;
}

Sprite2D* TileSheet2D::GetTileSprite(unsigned id) const
{
    return id < sprites_.Size() ? sprites_[id].Get() : 0;
}

Sprite2D* TileSheet2D::DefineTileSprite(const IntRect& rectangle, const Vector2& hotSpot, const IntVector2& offset, float edge, bool flipx, bool flipy)
{
    if (!texture_)
        return 0;

    SharedPtr<Sprite2D> sprite(new Sprite2D(context_));

    sprite->SetTexture(texture_);
    sprite->SetRectangle(rectangle);
    sprite->SetSourceSize(rectangle.Width(), rectangle.Height());
    sprite->SetOffset(offset);
    sprite->SetHotSpot(hotSpot);
    sprite->SetTextureEdgeOffset(Min(edge, edge/GameContext::Get().dpiScale_));
    sprite->SetSpriteSheet(0);

    sprites_.Push(sprite);

    unsigned char flip = 0;
    if (flipx)
        flip += 1;
    if (flipy)
        flip += 2;

    flippings_.Push(flip);

    return sprites_.Back().Get();
}

void TileSheet2D::UpdateTileScale(const Vector2& worldtilesize)
{
    if (sprites_.Size())
    {
        const Vector2 scale = (worldtilesize / tileSize_) * scaleFactor_;

        URHO3D_LOGDEBUGF("TileSheet2D() - UpdateScale : %s TileSize=%s WorldTileSize=%s ScaleFactor=%f (Scale = %s)",
                         GetName().CString(), tileSize_.ToString().CString(), worldtilesize.ToString().CString(), scaleFactor_, scale.ToString().CString());

        const float spanning = GameContext::Get().gameConfig_.tileSpanning_*PIXEL_SIZE;
        for (unsigned i=0; i < sprites_.Size(); i++)
        {
            sprites_[i]->SetFixedRectangles(scale, spanning, (flippings_[i] & 0x01) > 0, (flippings_[i] & 0x02) > 0);
//            URHO3D_LOGERRORF("sprite[%u] : %s flipx=%s flipy=%s", i, sprites_[i]->Dump().CString(), (flippings_[i] & 0x01) > 0 ? "true":"false", (flippings_[i] & 0x02) > 0 ? "true":"false");
        }
    }
}

bool TileSheet2D::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XMLFile(context_);
    if (!loadXMLFile_->Load(source))
    {
        URHO3D_LOGERROR("Could not load tilesheet");
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    XMLElement rootElem = loadXMLFile_->GetRoot("TileSheet");
    if (!rootElem)
    {
        URHO3D_LOGERROR("Invalid tile sheet");
        loadXMLFile_.Reset();
        return false;
    }

    // If we're async loading, request the texture now. Finish during EndLoad().
    loadTextureName_ = /*GetParentPath(GetName()) + */rootElem.GetAttribute("texture");
    if (GetAsyncLoadState() == ASYNC_LOADING)
        GetSubsystem<ResourceCache>()->BackgroundLoadResource<Texture2D>(loadTextureName_, true, this);

    return true;
}

bool TileSheet2D::EndLoadFromXMLFile()
{
//    URHO3D_LOGINFOF("TileSheet2D() - EndLoadFromXMLFile : %s dpiScale=%f ...", GetName().CString(), GameContext::Get().dpiScale_);

    // keep trace of already registered Tiles and decals (col,row,w,h = key)
    HashMap<unsigned, Sprite2D* > spriteIndexes_;

    XMLElement rootElem = loadXMLFile_->GetRoot("TileSheet");

    texture_ = GetSubsystem<ResourceCache>()->GetResource<Texture2D>(loadTextureName_);
    if (!texture_)
    {
        URHO3D_LOGERROR("TileSheet2D() - EndLoadFromXML : Could not load texture " + loadTextureName_);
        loadXMLFile_.Reset();
        return false;
    }
    if (!sAtlas_)
    {
        URHO3D_LOGERROR("TileSheet2D() - EndLoadFromXML : No Atlas for registering !");
        loadXMLFile_.Reset();
        return false;
    }
    if (sAtlas_->IsRegistered(StringHash(GetName())))
    {
        URHO3D_LOGWARNING("TileSheet2D() - EndLoadFromXML : Atlas has already registered tilesheet " + GetName());
        loadXMLFile_.Reset();
        return true;
    }

    const float texturescale = texture_->GetDpiRatio();

    int basewidth = rootElem.GetInt("basewidth");
    int baseheight = rootElem.GetInt("baseheight");

    if (!basewidth || !baseheight)
        URHO3D_LOGERRORF("TileSheet2D() - EndLoadFromXMLFile : %s ERROR => define basewidth & baseheight must be setted different than 0 !", GetName().CString());

    tileSize_ = Vector2((float) basewidth*PIXEL_SIZE, (float) baseheight*PIXEL_SIZE);

//    Vector2 hotSpot(0.5f, 0.5f);
    scaleFactor_ = rootElem.GetFloat("scalefactor");
    if (!scaleFactor_)
        scaleFactor_ = 1.f;

    for (XMLElement terrainElem = rootElem.GetChild("Terrain"); terrainElem; terrainElem = terrainElem.GetNext("Terrain"))
    {
        String propertyStr = terrainElem.GetAttribute("property");

        // TODO : decode property from String
        unsigned property = propertyStr == "Biome" ? 1 : 0; //GetEnumValue(terrainElem.GetAttribute("property"), MapTerrainTypeStr);

        unsigned char terrainId = sAtlas_->GetOrCreateTerrain(terrainElem.GetAttribute("name"), property);

        MapTerrain& terrain = sAtlas_->GetTerrain(terrainId);

        terrain.SetColor(terrainElem.GetColor("color"));

        for (XMLElement featureElem = terrainElem.GetChild("Features"); featureElem; featureElem = featureElem.GetNext("Features"))
        {
            Vector<String> features = featureElem.GetAttribute("name").Split('|');
            for (unsigned i=0; i<features.Size(); ++i)
            {
                terrain.AddCompatibleFeature(MapFeatureType::GetIndex(features[i].CString()));
//                URHO3D_LOGINFOF("=> AddCompatibleFeature To terrain=%s(%u) feature=%s(%d)",
//                        terrain.GetName().CString(), terrainId, features[i].CString(),
//                        MapFeatureType::GetIndex(features[i].CString()));
            }
        }

        for (XMLElement tilesElem = terrainElem.GetChild("Tiles"); tilesElem; tilesElem = tilesElem.GetNext("Tiles"))
        {
            const int width = (float)tilesElem.GetInt("width") * texturescale;
            const int height = (float)tilesElem.GetInt("height") * texturescale;
            const int startx = (float)tilesElem.GetInt("startx") * texturescale;
            const int starty = (float)tilesElem.GetInt("starty") * texturescale;
            const int spacing = (float)tilesElem.GetInt("spacing") * texturescale;

            const float edgeoffset = tilesElem.GetFloat("edgeoffset");

            Vector2 hotspot(0.f, 1.f);
            if (tilesElem.HasAttribute("hotspotx"))
                hotspot.x_ = tilesElem.GetFloat("hotspotx");
            if (tilesElem.HasAttribute("hotspoty"))
                hotspot.y_ = tilesElem.GetFloat("hotspoty");

            for (XMLElement tileElem = tilesElem.GetChild("Tile"); tileElem; tileElem = tileElem.GetNext("Tile"))
            {
                Sprite2D* sprite;
                // Find for a register sprite in this tilesheet
//                unsigned key = tileElem.GetInt("col") << 24 + tileElem.GetInt("row") << 16 + tileElem.GetInt("width") << 8 + tileElem.GetInt("height");
//                HashMap<unsigned , Sprite2D* >::ConstIterator it = spriteIndexes_.Find(key);
//                Sprite2D* sprite = it != spriteIndexes_.End() ? it->second_ : 0;
                unsigned char dimensions;
                // new sprite
//                if (!sprite)
                {
                    int x = startx + (tileElem.GetInt("col") * (width + 2*spacing));
                    int y = starty + (tileElem.GetInt("row") * (height + 2*spacing));
                    int w = tileElem.GetInt("width");
                    int h = tileElem.GetInt("height");
                    dimensions = (unsigned char)(w == 0 ? TILE_0 : w == 1 ? (h == 1 ? TILE_1X1 : TILE_1X2) : (h == 1 ? TILE_2X1 : TILE_2X2));
                    if (dimensions == TILE_0) w = h = 1;
                    bool flipx = tileElem.HasAttribute("flipx") ? tileElem.GetBool("flipx") : false;
                    bool flipy = tileElem.HasAttribute("flipy") ? tileElem.GetBool("flipy") : false;
                    sprite = DefineTileSprite(IntRect(x, y, x + w * width, y + h * height), hotspot, IntVector2::ZERO, edgeoffset, flipx, flipy);
                    //                spriteIndexes_[key] = sprite;
                }
                // register in atlas
                unsigned char connect = MapTilesConnectType::GetIndex(tileElem.GetAttribute("connection").CString());
                unsigned char propflag = tileElem.HasAttribute("property") ? TilePropertiesFlag::GetValue(tileElem.GetAttribute("property")) : TilePropertiesFlag::Blocked;
                sAtlas_->RegisterTile(sprite, TileProperty(propflag, terrainId, connect, dimensions).property_);
            }
        }

        for (XMLElement decalsElem = terrainElem.GetChild("Decals"); decalsElem; decalsElem = decalsElem.GetNext("Decals"))
        {
            const int width = (float)decalsElem.GetInt("width") * texturescale;
            const int height = (float)decalsElem.GetInt("height") * texturescale;
            const int startx = (float)decalsElem.GetInt("startx") * texturescale;
            const int starty = (float)decalsElem.GetInt("starty") * texturescale;
            const int spacing = (float)decalsElem.GetInt("spacing") * texturescale;

            const float edgeoffset = decalsElem.GetFloat("edgeoffset");

            Vector2 hotspot(0.5f, 0.5f);
            if (decalsElem.HasAttribute("hotspotx"))
                hotspot.x_ = decalsElem.GetFloat("hotspotx");
            if (decalsElem.HasAttribute("hotspoty"))
                hotspot.y_ = decalsElem.GetFloat("hotspoty");

            for (XMLElement decalElem = decalsElem.GetChild("Decal"); decalElem; decalElem = decalElem.GetNext("Decal"))
            {
                Sprite2D* sprite;
//                // Find for a register sprite in this tilesheet
//                unsigned key = decalElem.GetInt("col") << 24 + decalElem.GetInt("row") << 16;
//                HashMap<unsigned , Sprite2D* >::ConstIterator it = spriteIndexes_.Find(key);
//                Sprite2D* sprite = it != spriteIndexes_.End() ? it->second_ : 0;
//                // new sprite
//                if (!sprite)
                {
                    int x = startx + (decalElem.GetInt("col") * (width + 2*spacing));
                    int y = starty + (decalElem.GetInt("row") * (height + 2*spacing));
                    sprite = DefineTileSprite(IntRect(x, y, x + width, y + height), hotspot, IntVector2::ZERO, edgeoffset, decalElem.HasAttribute("flipx") ? decalElem.GetBool("flipx") : false, decalElem.HasAttribute("flipy") ? decalElem.GetBool("flipy") : false);
                    //                spriteIndexes_[key] = sprite;
                }
                // register in atlas
                unsigned char connect = MapTilesConnectType::GetIndex(decalElem.GetAttribute("connection").CString());
                unsigned char propflag = decalElem.HasAttribute("property") ? TilePropertiesFlag::GetValue(decalElem.GetAttribute("property")) : TilePropertiesFlag::Blocked;
                sAtlas_->RegisterDecal(sprite, TileProperty(propflag, terrainId, connect).property_);
            }
        }
//        for (XMLElement furnitureElem = terrainElem.GetChild("Furniture"); furnitureElem; furnitureElem = furnitureElem.GetNext("Furniture"))
//        {
//            unsigned char furniture = MapFurnitureType::GetIndex(furnitureElem.GetAttribute("category").CString());
//            for (XMLElement compatibleElem = furnitureElem.GetChild("Compatible"); compatibleElem; compatibleElem = compatibleElem.GetNext("Compatible"))
//            {
//                unsigned char connect = MapTilesConnectType::GetIndex(compatibleElem.GetAttribute("connection").CString());
//                terrain.AddCompatibleFurniture(furniture, connect);
//            }
//        }

        // Register Scraps
        if (terrainElem.HasAttribute("scraps"))
            ScrapsEmitter::RegisterType(context_, terrainElem.GetAttribute("scraps"), terrainId);

    }
    for (XMLElement skinElem = rootElem.GetChild("Skin"); skinElem; skinElem = skinElem.GetNext("Skin"))
    {
        String sname = skinElem.GetAttribute("name");
        bool optionSkinExist = skinElem.GetAttribute("option") == "skinExist";
        signed char skinId = optionSkinExist ? sAtlas_->GetSkinId(sname) : sAtlas_->GetOrCreateSkin(sname);
        if (skinId == -1)
            continue;

        MapSkin& skin = sAtlas_->GetSkin(skinId);

        if (!optionSkinExist)
            skin.neighborMode_ = (NeighborMode)GetEnumValue(skinElem.GetAttribute("neighbors"), NeighborModeStr);

//        URHO3D_LOGINFOF("TileSheet2D() - EndLoadFromXMLFile : Set Skin=%s(ptr=%u) connectedTile=%s... OK !",
//                        sname.CString(), &skin, skin.neighborMode_ == Connected0 ? "0-connected" : skin.neighborMode_ == Connected4 ? "4-connected" : "8-connected");

        // Update Atlas SkinRef
        HashMap<unsigned char, unsigned char>& atlasSkinRef = sAtlas_->GetSkinRef(skinId);
        for (XMLElement elem = skinElem.GetChild("Associate"); elem; elem = elem.GetNext("Associate"))
        {
            String tname = elem.GetAttribute("terrain");
            StringHash terrain(tname);
            String fname = elem.GetAttribute("feature");
            unsigned char feature = MapFeatureType::GetIndex(fname.CString());

            atlasSkinRef[feature] = sAtlas_->GetTerrainId(terrain);

//            URHO3D_LOGINFOF("TileSheet2D() - EndLoadFromXMLFile : Associate To Skin=%s <= Terrain=%s, Feature=%s(%u) ... OK !",
//                            sname.CString(), tname.CString(), fname.CString(), feature);
        }
    }

    loadXMLFile_.Reset();

//    URHO3D_LOGINFOF("TileSheet2D() - EndLoadFromXMLFile : ...  %s ... OK !", GetName().CString());

    return true;
}

