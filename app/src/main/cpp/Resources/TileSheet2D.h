#pragma once

#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{
class PListFile;
class Sprite2D;
class Texture2D;
class XMLFile;
}

using namespace Urho3D;


class TerrainAtlas;


/// TileSet sheet.
class TileSheet2D : public Resource
{
    URHO3D_OBJECT(TileSheet2D, Resource);

public:
    /// Construct.
    TileSheet2D(Context* context);
    /// Destruct.
    virtual ~TileSheet2D();
    /// Register object factory.
    static void RegisterObject(Context* context);
//    /// Register static atlas to keep ref
//    static void SetAtlas(TerrainAtlas* atlas) { atlas_ = atlas; }

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();

    Sprite2D* DefineTileSprite(const IntRect& rectangle, const Vector2& hotSpot, const IntVector2& offset, float edge, bool flipx=false, bool flipy=false);

//    static TerrainAtlas* GetAtlas() { return atlas_; }
    /// Return texture.
    Texture2D* GetTexture() const
    {
        return texture_;
    }
    /// Return sprite.
    Sprite2D* GetTileSprite(unsigned id) const;

    void UpdateTileScale(const Vector2& worldtilesize);

    const Vector2& GetTileSize() const
    {
        return tileSize_;
    }
    float GetTileScaleFactor() const
    {
        return scaleFactor_;
    }

    /// Return sprite mapping.
    const Vector<SharedPtr<Sprite2D> >& GetTileSprites() const
    {
        return sprites_;
    }

    /// static atlas to keep ref
    static TerrainAtlas* sAtlas_;

private:
    /// Begin load from PList file.
    bool BeginLoadFromPListFile(Deserializer& source);
    /// End load from PList file.
    bool EndLoadFromPListFile();
    /// Begin load from XML file.
    bool BeginLoadFromXMLFile(Deserializer& source);
    /// End load from XML file.
    bool EndLoadFromXMLFile();

    /// Texture.
    WeakPtr<Texture2D> texture_;
    /// Atlas
//    static TerrainAtlas* atlas_;

    String loadTextureName_;

    Vector2 tileSize_;
    float scaleFactor_;
    int tileNumNgh_;

    Vector<SharedPtr<Sprite2D> > sprites_;
    Vector<unsigned char > flippings_;

    /// PList file used while loading.
    SharedPtr<PListFile> loadPListFile_;
    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;
};

