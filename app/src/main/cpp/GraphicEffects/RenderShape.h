#pragma once

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Urho2D/Drawable2D.h>

#include "DefsShapes.h"


using namespace Urho3D;

struct MapCollider;

enum ShapeBatchType
{
    FRAMEBATCH = 0,
    BORDERBATCH,
    EMBOSEBATCH
};

struct ShapeBatchInfo
{
    ShapeBatchInfo() : dirty_(true) { }

    ShapeBatchInfo(int type, Material* material, int textureunit, int borderterrainid, const Color& color, const Vector2& textureRepeat = Vector2::ONE) :
        type_(type),
        material_(material),
        textureId_(textureunit),
        terrainid_(borderterrainid),
        color_(color),
        textureRepeat_(textureRepeat),
        mapBorderVerticesOffset_(0U),
        dirty_(true),
        mapBorderDirty_(true)
    {
        if (material)
        {
            Texture* texture = material->GetTexture((TextureUnit)textureunit);

            if (!texture)
            {
                URHO3D_LOGERRORF("ShapeBatchInfo() - type=%d material=%s textureunit=%d No Texture !", type, material->GetName().CString(), textureunit);
                return;
            }

            textureSize_ = PIXEL_SIZE * Vector2((float)texture->GetWidth(), (float)texture->GetHeight());

//                Vector2 numframes = material->GetShaderParameter("NumFrames").GetVector2();
//                if (numframes.x_ != 0.f)
//                {
////                    textureRepeat_ = Vector2::ONE;
////                    textureSize_ /= numframes;
//                    URHO3D_LOGERRORF("ShapeBatchInfo() - type=%d material=%s numFrames=%s don't apply texturerepeat factor !", type, material->GetName().CString(), numframes.ToString().CString());
//                }
        }
    }

    ShapeBatchInfo(const ShapeBatchInfo& b) :
        type_(b.type_),
        material_(b.material_),
        textureId_(b.textureId_),
        terrainid_(b.terrainid_),
        color_(b.color_),
        textureSize_(b.textureSize_),
        textureRepeat_(b.textureRepeat_),
        mapBorderVerticesOffset_(0U),
        dirty_(true),
        mapBorderDirty_(true)
    { }

    int type_;
    SharedPtr<Material> material_;
    int textureId_;
    int terrainid_;

    Color color_;
    Vector2 textureSize_;
    Vector2 textureRepeat_;

    unsigned mapBorderVerticesOffset_;

    bool dirty_;
    bool mapBorderDirty_;
};

class FROMBONES_API RenderShape : public Drawable2D
{
    URHO3D_OBJECT(RenderShape, Drawable2D);

public:
    RenderShape(Context* context);
    ~RenderShape();

    static void RegisterObject(Context* context);

    void Clear();

    void AddBatch(const ShapeBatchInfo& batchinfo);
    void AddShape(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> > *holesptr);

    void SetDynamic(bool state)
    {
        isDynamic_ = state;
    }
    void SetViewportClipping(bool enable);
    void SetDirty(unsigned ibatch, bool state=true);
    void SetMapBorderDirty(unsigned ibatch);

    void SetTextureRepeat(unsigned ibatch, const Vector2& repeat);
    void SetColor(unsigned ibatch, const Color& color);
    void SetAlpha(unsigned ibatch, float alpha);
    void SetCollider(MapCollider* collider);
    void SetEmboseOffset(float offset);
    void SetMapBorder(bool state);

    const Vector<PolyShape >& GetShapes() const
    {
        return shapes_;
    }
    Vector<PolyShape >& GetShapes()
    {
        return shapes_;
    }

    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    virtual void OnSetEnabled();

protected:
    virtual void OnWorldBoundingBoxUpdate();
    virtual void OnDrawOrderChanged();
    virtual void OnMarkedDirty(Node* node);

    virtual void UpdateSourceBatches();

private :
    void UpdateMaterials();
    void UpdateFrameVertices(unsigned ibatch);
    void UpdateBorderVertices(unsigned ibatch);
    void UpdateEmboseVertices(unsigned ibatch);

    void AddBorderOnMapBorder(unsigned ibatch, int side);
    void AddBorderOnContour(unsigned ibatch, const PODVector<Vector2>& contour, const Vector2& direction = Vector2::ONE, bool checkborder=true);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    MapCollider* collider_;

    Vector<PolyShape > shapes_;
    Rect lastClippingRect_;

    Vector<ShapeBatchInfo > batchinfos_;

    bool materialDirty_;
    bool hasMapBorder_;
    bool isDynamic_;
    Rect mapBorder_;
    float embose_;
};
