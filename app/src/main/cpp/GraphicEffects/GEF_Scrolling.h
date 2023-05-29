#pragma once


#include <Urho3D/Scene/Component.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

namespace Urho3D
{
class Node;
class Drawable2D;
class StaticSprite2D;
}

using namespace Urho3D;

#include "DefsShapes.h"
#include "DefsWorld.h"

/// ScrollingShape : défilement d'une texture dans une forme ou dans l'écran avec parallax (basé sur texture offset)

class ScrollingShape : public Drawable2D
{
    URHO3D_OBJECT(ScrollingShape, Drawable2D);

public:
    ScrollingShape(Context* context);
    ~ScrollingShape();

    static void RegisterObject(Context* context);

    void Clear();

    void SetTexture(Texture2D* texture);
    void SetTextureOffset(const Vector2& offset);
    void SetTextureRepeat(const Vector2& repeat);
    void SetTextureSize(const Vector2& texturesize, const Vector2& repeat = Vector2::ONE);
    void SetMaterial(Material* material, int textureunit = 0);

    void SetColor(const Color& color);
    void SetAlpha(float alpha);
    void SetNodeToFollow(Node* tofollow);
    void SetParallax(const Vector2& parallax);
    void SetMotion(const Vector2& motion);

    void SetClipping(bool enable);
    void SetScreenShape(bool enable);
    void SetQuadShape(float minx, float miny, float maxx, float maxy);
    void AddShape(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> > *holesptr);

    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    virtual void OnSetEnabled();

    static void AddQuadScrolling(Node* nodeRoot, int layer, int order, Material* material, int unit, int fx, const Vector2& offsetPosition=Vector2::ZERO,
                                 const Vector2& repeat=Vector2::ONE, const Vector2& speed=Vector2::ONE, const Vector2& parallax=Vector2::ZERO, const Color& color=Color::WHITE);

    static void SetIntensity(float intensity)
    {
        intensity_ = intensity;
    }
    static float GetIntensity()
    {
        return intensity_;
    }

protected:
    virtual void OnWorldBoundingBoxUpdate();
    virtual void OnDrawOrderChanged();
    virtual void UpdateSourceBatches();
    virtual void UpdateMaterial();

private :
    void SetDirty();

    void UpdateClipping();

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleSetCamera(StringHash eventType, VariantMap& eventData);

    Color color_;
    BlendMode blendMode_;
    SharedPtr<Texture2D> texture_;
    SharedPtr<Material> material_;
    WeakPtr<Node> nodeToFollow_;

    Vector<PolyShape > shapes_;

    Vector2 textureOffset_;
    Vector2 textureSize_;
    Vector2 textureRepeat_;

    Vector2 lastnodeposition_;
    Vector2 motion_;
    Vector2 parallax_;

    bool verticesDirty_;
    bool textureDirty_;
    bool screenshape_;

    bool clippingEnable_;
    Rect lastClippingRect_;

    static float intensity_;
};


/// GEF_Scrolling : défilement d'une image suivant l'écran avec parallax (basé sur un node suivant la camera)

struct ScrollingLayer
{
    int id_;
    int layer_, order_;
    int viewport_;
    unsigned viewmask_;
    String refname_;
    Material* material_;
    int textureFX_;
    Color color_;
    Vector2 parallaxspeed_, motionspeed_, offset_;
    Vector2 motion_, lastPosition_, halfspritesize_;
    float overlapping_;
    WeakPtr<Node> nodeToFollow_;
    WeakPtr<Node> nodeImages_;
    Vector<StaticSprite2D*> staticsprites_;
    float lastVisibleRectWidth_;
    bool resetScale_;

    void Set(int id, Node* root);
    void SetNodeToFollow(Node* nodeToFollow);
    void SetScale();

    void ForceUpdateBatches();

    void Update(float timeStep, bool recenter=false);
};

class GEF_Scrolling : public Component
{
    URHO3D_OBJECT(GEF_Scrolling, Component);

public :
    GEF_Scrolling(Context* context);
    virtual ~GEF_Scrolling();

    static void RegisterObject(Context* context);

    void Start(bool forced=false);
    void Stop(bool forced=false);
    void SetViewport(int viewport)
    {
        viewport_ = viewport;
    }
    void SetNodeToFollow(Node* node);
    void SetIntensity(float intensity)
    {
        intensity_ = intensity;
    }

    void AddMaterialLayer(Material* material, int textureFX, int layer, int order, unsigned viewmask, float overlap, const String& refname, const Vector2& offsetPosition,
                          const Vector2& speed, const Vector2& parallax=Vector2::ZERO, const Color& color=Color::WHITE);
    void AddSpritesLayer(int layer, int order, unsigned viewmask, float overlap, const String& spriteSheet, Material* material, const String& spriteRadix,
                         const Vector2& offsetPosition, const Vector2& speed, const Vector2& parallax=Vector2::ZERO, const Color& color=Color::WHITE);

    static void AddScrolling(Material* material, int textureFX, const String& refname, Node* nodeRoot, int layer=0, int order=0, float overlap=0.f, const Vector2& offsetPosition=Vector2::ZERO,
                             const Vector2& speed=Vector2::ONE, bool start=false, const Vector2& parallax=Vector2::ZERO, const Color& color=Color::WHITE);

    bool IsStarted() const
    {
        return started_;
    }

    float GetIntensity() const
    {
        return intensity_;
    }
    const Vector<ScrollingLayer>& GetLayers() const
    {
        return layers_;
    }

    virtual void OnSetEnabled();

private :
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleCameraPositionChanged(StringHash eventType, VariantMap& eventData);
    void Set();

    int viewport_;
    bool started_;
    float intensity_;
    WeakPtr<Node> nodeToFollow_;

    Vector<ScrollingLayer> layers_;
};

/// DrawableScroller : défilement de Sprite avec parallax. Un sprite a une position et est attaché à une zone (map point)

struct DrawableObjectInfo
{
    DrawableObjectInfo() { }
    DrawableObjectInfo(const ShortIntVector2& mpoint, Sprite2D* sprite, Material* material, bool flipX = false, bool flipY = false);
    DrawableObjectInfo(const DrawableObjectInfo& s) :
        zone_(s.zone_),
        sprite_(s.sprite_),
        material_(s.material_),
        drawRect_(s.drawRect_),
        textureRect_(s.textureRect_) { }

    void Set(const ShortIntVector2& mpoint, Sprite2D* sprite, Material* material, bool flipX = false, bool flipY = false);

    IntVector2 zone_;
    SharedPtr<Sprite2D> sprite_;
    Material* material_;
    Rect drawRect_, textureRect_;
};

struct DrawableObject
{
    Vector2 position_, angle_, size_;
    Vertex2D v0_, v1_, v2_, v3_;
    Rect rect_;
    DrawableObjectInfo* info_;
    bool enable_, flipX_;
};

struct DrawableScroller : public StaticSprite2D
{
    URHO3D_OBJECT(DrawableScroller, StaticSprite2D);

public:
    DrawableScroller(Context* context);
    ~DrawableScroller();

    static void RegisterObject(Context* context);

    static void Reset();
    static void Start();
    static void Stop();
    static void ToggleStartStop();

    static void Pause(int viewport=0, bool state=true);
    static void SetIntensity(int viewport=0, float intensity=0.f)
    {
        scrollerinfos_[viewport].intensity_ = intensity;
    }

    static DrawableScroller* AddScroller(int viewport, Material* material, int textureFX, int layer, int order, const Vector2& scale, const Vector2& motionBounds,
                                         const Vector2& parallax = Vector2::ONE, EllipseW* boundCurve=0, bool allowBottomCurve_=false, bool rotFollowCurve=false,
                                         const Vector2& offset = Vector2::ZERO, const Color& color = Color::WHITE, bool logTest=false);
    static void RemoveAllObjectsOnMap(const ShortIntVector2& mpoint);

    static unsigned GetNumScrollers(int viewport)
    {
        return scrollerinfos_[viewport].drawableScrollers_.Size();
    }
    static DrawableScroller* GetScroller(int viewport, int index)
    {
        return scrollerinfos_[viewport].drawableScrollers_[index];
    }

    void Clear();

    void SetViewport(int viewport)
    {
        viewport_ = viewport;
    }
    void SetId(unsigned id)
    {
        id_ = id;
    }

    void SetRotationFollowCurve(bool state)
    {
        rotationFollowCurve_ = state;
    }

    void SetParallax(const Vector2& parallax);

    void SetLogTest(bool enable);

    void SetOffset(const Vector2& offset)
    {
        scrolleroffset_ = offset;
    }
    void SetScale(const Vector2& scale)
    {
        scrollerscale_ = scale;
    }
    void SetBoundCurve(EllipseW* curve, bool allowBottomCurve)
    {
        boundcurve_ = curve;
        allowBottomCurve_ = allowBottomCurve;
    }
    void SetMotionBounds(const Vector2& motionBounds)
    {
        motionBounds_ = motionBounds;
    }

    void ResizeWorldScrollerObjectInfos(const ShortIntVector2& mpoint, unsigned numobjects, Sprite2D* sprite);
    void RemoveScrollerObjects(const ShortIntVector2& mpoint);

    const Vector2& GetParallax() const
    {
        return parallax_;
    }

    Vector<DrawableObjectInfo>* GetWorldScrollerObjectInfos(const ShortIntVector2& mpoint)
    {
        HashMap<ShortIntVector2, Vector<DrawableObjectInfo> >::Iterator it = worldScrollerObjectInfos_.Find(mpoint);
        return it != worldScrollerObjectInfos_.End() ? &it->second_ : 0;
    }
    DrawableObjectInfo* GetWorldScrollerObjectInfo(const ShortIntVector2& mpoint, unsigned index)
    {
        HashMap<ShortIntVector2, Vector<DrawableObjectInfo> >::Iterator it = worldScrollerObjectInfos_.Find(mpoint);
        if (it != worldScrollerObjectInfos_.End())
        {
            Vector<DrawableObjectInfo>& objects = it->second_;
            return objects.Size() > index ? &objects[index] : 0;
        }

        return 0;
    }

    virtual BoundingBox GetWorldBoundingBox2D();
    virtual void OnSetEnabled();
    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    void Dump() const;
    static void DumpDrawables(int viewport);

protected:
    virtual void OnWorldBoundingBoxUpdate();
    virtual void OnDrawOrderChanged();
    virtual void UpdateSourceBatches();

private:
    friend struct DrawableObject;

    void ClearDrawables();
    bool SetDrawableObjects();

    inline void SetDrawableVertices(DrawableObject& drawable);
    void UpdateDrawablesOnTopCurve(Vector2 bmin, Vector2 bmax);
    void UpdateDrawablesOnBottomCurve(Vector2 bmin, Vector2 bmax);

    void HandleUpdateEllipseMode(StringHash eventType, VariantMap& eventData);
    void HandleUpdateFlatMode(StringHash eventType, VariantMap& eventData);
    void HandleCameraPositionChanged(StringHash eventType, VariantMap& eventData);

    int viewport_;
    unsigned id_;
    bool rotationFollowCurve_;
    bool allowBottomCurve_;

    Vector2 scrollerposition_, scrolleroffset_, scrollerscale_;
    Vector2 parallax_, motionBounds_;
    ShortIntVector2 lastMapPoint_;
    EllipseW* boundcurve_;

    Vector2 drawableSize_;
    float drawableoverlap_;
    float lastDrawablePositionAtLeft_;
    Matrix2x3 matrix_;

    HashMap<ShortIntVector2, Vector<DrawableObjectInfo> > worldScrollerObjectInfos_;

    Vector<DrawableObject> drawablePool_;
    Vector<DrawableObject* > freeDrawables_;
    Vector<DrawableObject* > drawables_, drawablesTopCurve_, drawablesBottomCurve_;

    struct ScrollerViewInfo
    {
        ScrollerViewInfo() : camPositionCount_(0), intensity_(1.f) { }
        void Reset();

        WeakPtr<Node> nodeToFollow_;
        int camPositionCount_;
        Vector2 camPosition_, initialPosition_, lastCamPosition_;
        float intensity_;

        Vector<DrawableScroller*> drawableScrollers_;
    };

    bool logtest_;

    static ScrollerViewInfo scrollerinfos_[MAX_VIEWPORTS];

    static bool flatmode_;
};


