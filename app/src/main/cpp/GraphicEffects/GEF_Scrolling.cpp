#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>

#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"

#include "MapWorld.h"
#include "ViewManager.h"
#include "Map.h"

#include "GEF_Scrolling.h"



/// ScrollingShape


float ScrollingShape::intensity_ = 1.f;

ScrollingShape::ScrollingShape(Context* context) :
    Drawable2D(context),
    color_(Color::WHITE),
    blendMode_(BLEND_ALPHA),
    verticesDirty_(true),
    textureDirty_(true),
    screenshape_(false),
    clippingEnable_(false)
{
    sourceBatches_[0].Resize(1);
    sourceBatches_[0][0].owner_ = this;
    sourceBatches_[0][0].quadvertices_ = false;
    textureOffset_ = Vector2::ZERO;
    textureRepeat_ = Vector2::ONE;
}

ScrollingShape::~ScrollingShape()
{
}

void ScrollingShape::RegisterObject(Context* context)
{
    context->RegisterFactory<ScrollingShape>();
}

void ScrollingShape::SetDirty()
{
    sourceBatchesDirty_ = true;
    verticesDirty_ = true;
    textureDirty_ = true;
    MarkNetworkUpdate();
}

void ScrollingShape::SetTexture(Texture2D* texture)
{
    texture_ = texture;
    material_.Reset();
    UpdateMaterial();

    SetDirty();
}

void ScrollingShape::SetTextureOffset(const Vector2& offset)
{
    if (offset == textureOffset_)
        return;

    sourceBatchesDirty_ = true;
    textureDirty_ = true;
    textureOffset_ = offset;
    MarkNetworkUpdate();
}

void ScrollingShape::SetTextureRepeat(const Vector2& repeat)
{
    if (repeat == textureRepeat_)
        return;

    textureRepeat_ = repeat;

    SetDirty();
}

void ScrollingShape::SetTextureSize(const Vector2& texturesize, const Vector2& repeat)
{
    if (repeat == textureRepeat_ && textureSize_ == texturesize)
        return;

    textureSize_ = texturesize;
    textureRepeat_ = repeat;

    SetDirty();
}

void ScrollingShape::SetMaterial(Material* material, int textureunit)
{
    material_ = material;
    texture_ = (Texture2D*)material_->GetTexture((TextureUnit)textureunit);

    UpdateMaterial();

    textureSize_ = PIXEL_SIZE * Vector2((float)texture_->GetWidth(), (float)texture_->GetHeight());
    textureRepeat_ = Vector2::ONE;

    SetDirty();
}

void ScrollingShape::SetColor(const Color& color)
{
    if (color == color_)
        return;

    color_ = color;

    SetDirty();
}

void ScrollingShape::SetAlpha(float alpha)
{
    if (alpha == color_.a_)
        return;

    color_.a_ = alpha;

    SetDirty();
}

void ScrollingShape::Clear()
{
    shapes_.Clear();
    sourceBatches_[0][0].vertices_.Clear();

    SetDirty();
}

void ScrollingShape::SetClipping(bool enable)
{
    if (enable == clippingEnable_)
        return;

#ifdef ACTIVE_SCROLLINGSHAPE_CLIPPING
    clippingEnable_ = enable;
#endif

    if (clippingEnable_)
    {
        UpdateClipping();

        OnSetEnabled();
    }
}

void ScrollingShape::SetScreenShape(bool enable)
{
    if (screenshape_ != enable)
    {
        screenshape_ = enable;
        if (screenshape_)
        {
            // set worldboundingbox infinite else the texture disappear at borders
            boundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
            worldBoundingBox_ = boundingBox_;
            sourceBatches_[0][0].quadvertices_ = true;
            Clear();
        }
    }
}

void ScrollingShape::SetQuadShape(float minx, float miny, float maxx, float maxy)
{
    sourceBatches_[0][0].quadvertices_ = true;
    /*
    V1---------V2
    |         / |
    |       /   |
    |     /     |
    |   /       |
    | /         |
    V0---------V3
    */
    shapes_.Resize(1);
    PolyShape& shape = shapes_.Back();

    if (shape.contours_.Size() == 0)
        shape.contours_.Resize(1);

    PODVector<Vector2> contour = shape.contours_[0];
    contour.Push(Vector2(minx, miny));
    contour.Push(Vector2(minx, maxy));
    contour.Push(Vector2(maxx, maxy));
    contour.Push(Vector2(maxx, miny));

    shape.triangles_ = contour;

    SetDirty();

    MarkNetworkUpdate();
}

void ScrollingShape::AddShape(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> > *holesptr)
{
    shapes_.Resize(shapes_.Size()+1);

    PolyShape& shape = shapes_.Back();

    if (!shape.contours_.Size())
        shape.contours_.Resize(1);
    shape.contours_[0] = contour;

    if (!shape.holes_.Size())
        shape.holes_.Resize(1);
    if (holesptr)
        shape.holes_[0] = *holesptr;

    shape.Triangulate();

    URHO3D_LOGINFOF("ScrollingShape() - AddShape : this=%u numTriangles=%u !", this, shape.triangles_.Size());

    SetDirty();

    worldBoundingBoxDirty_ = true;

    MarkNetworkUpdate();
}

void ScrollingShape::SetNodeToFollow(Node* tofollow)
{
    nodeToFollow_ = tofollow;
    OnSetEnabled();
}

void ScrollingShape::SetParallax(const Vector2& parallax)
{
    if (parallax_ != parallax)
    {
        parallax_ = parallax;
        OnSetEnabled();
    }
}

void ScrollingShape::SetMotion(const Vector2& motion)
{
    if (motion_ != motion)
    {
        motion_ = motion;
        OnSetEnabled();
    }
}

void ScrollingShape::OnWorldBoundingBoxUpdate()
{
    if (screenshape_)
        return;

    worldBoundingBox_.Clear();

    for (unsigned i=0; i < shapes_.Size(); ++i)
    {
        PolyShape& shape = shapes_[i];
        shape.UpdateWorldBoundingRect(node_->GetWorldTransform2D());
        worldBoundingBox_.Merge(BoundingBox(shape.worldBoundingRect_.min_, shape.worldBoundingRect_.max_));
    }
}

void ScrollingShape::OnDrawOrderChanged()
{
    sourceBatches_[0][0].drawOrder_ = GetDrawOrder();
}

void ScrollingShape::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    Vector<Vertex2D>& vertices = sourceBatches_[0][0].vertices_;

    if (!IsEnabledEffective())
    {
        sourceBatchesDirty_ = false;
        vertices.Clear();
        return;
    }

    if (!sourceBatches_[0][0].material_)
        UpdateMaterial();

    unsigned color;
    {
        float alpha = color_.a_;
        Color lumcolor = color_ * GameContext::Get().luminosity_;
        lumcolor.a_ = alpha;
        color = lumcolor.ToUInt();
    }

    if (screenshape_)
    {
        const Frustum& frustum = GameContext::Get().camera_->GetFrustum();
        Rect vrect;
        vrect.min_.x_ = frustum.vertices_[2].x_;
        vrect.min_.y_ = frustum.vertices_[2].y_;
        vrect.max_.x_ = frustum.vertices_[0].x_;
        vrect.max_.y_ = frustum.vertices_[0].y_;

        vertices.Resize(4);

        vertices[0].position_ = vrect.min_;
        vertices[1].position_ = Vector2(vrect.min_.x_, vrect.max_.y_);
        vertices[2].position_ = vrect.max_;
        vertices[3].position_ = Vector2(vrect.max_.x_, vrect.min_.y_);

//        float textureSizeX = vrect.max_.x_ - vrect.min_.x_;
//        float textureSizeY = vrect.max_.y_ - vrect.min_.y_;
#ifdef URHO3D_VULKAN
        unsigned texmode = 0;
#else
        Vector4 texmode;
#endif
        SetTextureMode(TXM_UNIT, material_ && texture_ ? material_->GetTextureUnit(texture_) : 0, texmode);
        SetTextureMode(TXM_FX, textureFX_, texmode);

        for (int i=0; i<4; i++)
        {
            Vertex2D& vertex = vertices[i];
//            vertex.uv_.x_ = textureOffset_.x_ + textureRepeat_.x_ * vertex.position_.x_ / textureSizeX;
//            vertex.uv_.y_ = textureOffset_.y_ + textureRepeat_.y_ * vertex.position_.y_ / textureSizeY;

            vertex.uv_.x_ = textureOffset_.x_ + textureRepeat_.x_ * vertex.position_.x_ / textureSize_.x_;
            vertex.uv_.y_ = textureOffset_.y_ + textureRepeat_.y_ * vertex.position_.y_ / textureSize_.y_;

            vertex.color_ = color;
            vertex.texmode_ = texmode;
        }

        sourceBatchesDirty_ = false;
        return;
    }

    Vector<PolyShape>& shapes = shapes_;

    unsigned trianglesize = 0;
    for (unsigned i=0; i < shapes.Size(); ++i)
        trianglesize += shapes[i].triangles_.Size();

    if (!trianglesize)
    {
//        URHO3D_LOGWARNINGF("ScrollingShape() - UpdateSourceBatches : this=%u clippingRect_=%u (Rect=%s) ... No Triangles !", this, clippingRect_, clippingRect_ ? clippingRect_->ToString().CString() : "0");
        verticesDirty_ = textureDirty_ = sourceBatchesDirty_ = false;
        return;
    }

    if (verticesDirty_)
    {
#ifdef URHO3D_VULKAN
        unsigned texmode = 0;
#else
        Vector4 texmode;
#endif
        SetTextureMode(TXM_UNIT, material_ && texture_ ? material_->GetTextureUnit(texture_) : 0, texmode);
        SetTextureMode(TXM_FX, textureFX_, texmode);

        vertices.Resize(trianglesize);
        unsigned ivertex=0;
        for (unsigned i=0; i < shapes.Size(); ++i)
        {
            const PODVector<Vector2>& triangles = shapes[i].triangles_;
            const Matrix3x4& worldTransform = node_->GetWorldTransform();

            for (int i=0; i < triangles.Size(); ++i, ++ivertex)
            {
                Vertex2D& vertex = vertices[ivertex];
                vertex.color_ = color;
                vertex.position_ = (worldTransform * Vector3(triangles[i].x_, triangles[i].y_, 0.0f)).ToVector2();
                vertex.texmode_ = texmode;
            }
        }
        verticesDirty_ = false;
    }

    if (textureDirty_ && vertices.Size() == trianglesize)
    {
        unsigned ivertex=0;
        Vector2 baseUV;

        for (unsigned i=0; i < shapes.Size(); ++i)
        {
            const PODVector<Vector2>& triangles = shapes[i].triangles_;

            // First Vertex of the first Triangle : Get the base UV to reduce UV Coord (if not problem with Android when big UV Coord)
            {
                Vertex2D& vertex = vertices[ivertex];
                vertex.uv_.x_ = (textureRepeat_.x_ * (vertex.position_.x_ - textureOffset_.x_) / textureSize_.x_);
                vertex.uv_.y_ = (textureRepeat_.y_ * (vertex.position_.y_ - textureOffset_.y_) / textureSize_.y_);

                baseUV.x_ = Floor(vertex.uv_.x_);
                baseUV.y_ = Floor(vertex.uv_.y_);
                vertex.uv_.x_ -= baseUV.x_;
                vertex.uv_.y_ -= baseUV.y_;

                ivertex++;
            }

            for (int j=1; j < triangles.Size(); ++j, ++ivertex)
            {
                Vertex2D& vertex = vertices[ivertex];
                vertex.uv_.x_ = (textureRepeat_.x_ * (vertex.position_.x_ - textureOffset_.x_) / textureSize_.x_) - baseUV.x_;
                vertex.uv_.y_ = (textureRepeat_.y_ * (vertex.position_.y_ - textureOffset_.y_) / textureSize_.y_) - baseUV.y_;
            }
        }

        textureDirty_ = false;
    }

    sourceBatchesDirty_ = verticesDirty_ || textureDirty_;
}

void ScrollingShape::UpdateMaterial()
{
    if ((!material_ && !texture_) || !renderer_)
        return;

    if (material_)
        sourceBatches_[0][0].material_ = material_;

    else if (texture_)
        sourceBatches_[0][0].material_ = renderer_->GetMaterial(texture_, blendMode_);
}

void ScrollingShape::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!IsEnabledEffective())
        return;

    // Draw Resulting Meshes
    if (sourceBatches_[0].Size() && sourceBatches_[0][0].quadvertices_ == false)
        GameHelpers::DrawDebugMesh(debug, sourceBatches_[0][0].vertices_, Color::BLUE);

    const Matrix3x4& worldTransform = node_->GetWorldTransform();

    // Draw original shape
    for (unsigned i=0; i < shapes_.Size(); ++i)
    {
        for (unsigned j=0; j < shapes_[i].contours_.Size(); j++)
            GameHelpers::DrawDebugShape(debug, shapes_[i].contours_[j], Color::GREEN, worldTransform);
        for (unsigned j=0; j < shapes_[i].clippedContours_.Size(); j++)
            GameHelpers::DrawDebugShape(debug, shapes_[i].clippedContours_[j], Color::RED, worldTransform);
    }

    // Draw original holes
    for (unsigned i=0; i < shapes_.Size(); ++i)
    {
        for (unsigned j=0; j < shapes_[i].holes_.Size(); j++)
        {
            if (shapes_[i].holes_[j].Size())
            {
                for (unsigned k=0; k < shapes_[i].holes_[j].Size(); k++)
                    GameHelpers::DrawDebugShape(debug, shapes_[i].holes_[j][k], Color::RED, worldTransform);
            }
        }
    }
}

void ScrollingShape::OnSetEnabled()
{
    Drawable2D::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene && IsEnabledEffective())
    {
        verticesDirty_ = true;
        textureDirty_ = true;
//        URHO3D_LOGINFOF("ScrollingShape() - this=%u OnSetEnabled = true", this);
        if (nodeToFollow_ || motion_ != Vector2::ZERO || clippingEnable_)
        {
            SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(ScrollingShape, HandleUpdate));

#ifdef ACTIVE_SCROLLINGSHAPE_FRUSTUMCAMERA
            if (clippingEnable_)
                SubscribeToEvent(WORLD_CAMERACHANGED, URHO3D_HANDLER(ScrollingShape, HandleSetCamera));
#endif
        }
    }
    else
    {
//        URHO3D_LOGINFOF("ScrollingShape() - this=%u OnSetEnabled = false", this);
        UnsubscribeFromAllEvents();
    }

    sourceBatchesDirty_ = true;
}

void ScrollingShape::UpdateClipping()
{
    if (clippingEnable_ && shapes_.Size())
    {
        for (unsigned i=0; i < shapes_.Size(); ++i)
        {
            // Clip the shape
            shapes_[i].Clip(lastClippingRect_);
        }

//        URHO3D_LOGINFOF("ScrollingShape() - UpdateClipping : %s(%u) clippingRect_=%s", GetNode()->GetName().CString(), GetNode()->GetID(), lastClippingRect_.ToString().CString());

        SetDirty();
    }
}

void ScrollingShape::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat() * intensity_;
    Vector2 motion;

    // Clip the shape if change
    // TODO Multiviewport
    const int viewport = 0;
    if (clippingEnable_ && lastClippingRect_ != World2D::GetTiledVisibleRect(viewport))
    {
        lastClippingRect_ = World2D::GetTiledVisibleRect(viewport);
        UpdateClipping();
    }

    if (nodeToFollow_)
    {
        Vector2 position = nodeToFollow_->GetWorldPosition2D();

        if (position != lastnodeposition_)
        {
            SetDirty();

            motion += parallax_ * (position - lastnodeposition_);
            lastnodeposition_ = position;
        }
    }

    if (motion_ != Vector2::ZERO)
        motion += timeStep*motion_;

    if (motion != Vector2::ZERO)
        SetTextureOffset(motion + textureOffset_);
}

void ScrollingShape::HandleSetCamera(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("ScrollingShape() - this=%u HandleSetCamera !", this);

    // force recalculate batches (prevent clipping during reapparition)
    ForceUpdateBatches();
}

void ScrollingShape::AddQuadScrolling(Node* nodeRoot, int layer, int order, Material* material, int textureunit, int texturefx, const Vector2& offsetPosition, const Vector2& repeat, const Vector2& speed, const Vector2& parallax, const Color& color)
{
    Node* scrollnode = nodeRoot->CreateChild("scrollingshape");
    scrollnode->SetPosition2D(offsetPosition);

    ScrollingShape* scrollshape = scrollnode->CreateComponent<ScrollingShape>();
    scrollshape->SetLayer2(IntVector2(layer,-1));
    scrollshape->SetOrderInLayer(order);
    scrollshape->SetViewMask(layer <= INNERVIEW ? ViewManager::effectMask_[INNERVIEW] : ViewManager::effectMask_[FRONTVIEW]);
    scrollshape->SetMaterial(material, textureunit);
    scrollshape->SetTextureFX(texturefx);
    scrollshape->SetScreenShape(true);
    scrollshape->SetTextureRepeat(repeat);
    scrollshape->SetColor(color);
//    scrollshape->SetAlpha(SCROLLERBACKGROUND_ALPHA);
    scrollshape->SetNodeToFollow(GameContext::Get().cameraNode_);
    scrollshape->SetParallax(parallax);
    scrollshape->SetMotion(speed);
}



/// GEF_Scrolling

void ScrollingLayer::SetNodeToFollow(Node* nodeToFollow)
{
    nodeToFollow_ = nodeToFollow;
    lastPosition_ = nodeToFollow ? nodeToFollow->GetWorldPosition2D() : Vector2::ZERO;

    if (nodeToFollow && nodeImages_)
        nodeImages_->SetWorldPosition2D(nodeToFollow->GetWorldPosition2D());
}

void ScrollingLayer::Set(int id, Node* root)
{
    id_ = id;

    String layername("ScrollingLayer");
    layername += String(id_);

    nodeImages_ = root->GetChild(layername, LOCAL);

    // create Draw Component Nodes
    if (!nodeImages_)
    {
        // create sprite
        Vector<String> vars = refname_.Split(';');
        Sprite2D* sprite = Sprite2D::LoadFromResourceRef(GameContext::Get().context_, ResourceRef(vars[0], vars[1]));
        if (!sprite)
            return;

        Rect spriterect;
        if (!sprite->GetDrawRectangle(spriterect))
            return;

        // create node images
        nodeImages_ = root->CreateChild(layername, LOCAL);

        // use edge offset to prevent edge bleeding with texture atlas
        sprite->SetTextureEdgeOffset(Min(1.f, 1.f/GameContext::Get().dpiScale_));

        // create 9 nodes with the same texture
        staticsprites_.Clear();
        StaticSprite2D* staticsprite;

        for (int y=-1; y < 2; y++)
        {
            for (int x=-1; x < 2; x++)
            {
                staticsprite = nodeImages_->CreateChild("", LOCAL)->CreateComponent<StaticSprite2D>(LOCAL);
                staticsprites_.Push(staticsprite);
                staticsprite->SetSprite(sprite);
                staticsprite->SetCustomMaterial(material_);
                staticsprite->SetTextureFX(textureFX_);
                staticsprite->SetColor(color_);
                staticsprite->SetLayer2(IntVector2(layer_,-1));
                staticsprite->SetOrderInLayer(order_);
                staticsprite->SetViewMask(viewmask_);//layer_ <= INNERVIEW ? ViewManager::effectMask_[INNERVIEW] : ViewManager::effectMask_[FRONTVIEW]);
            }
        }

        resetScale_ = true;

//        URHO3D_LOGINFOF("ScrollingLayer() - Set : id=%d layer=%d parallax=%f !", id_, layer_, parallaxspeed_.ToString().CString());
    }
}

void ScrollingLayer::ForceUpdateBatches()
{
    resetScale_ = true;

    Update(0.f, true);

    for (unsigned i=0; i < staticsprites_.Size(); i++)
        staticsprites_[i]->ForceUpdateBatches();

    URHO3D_LOGERRORF("ScrollingLayer() - ForceUpdateBatches : ... OK !");
}

void ScrollingLayer::SetScale()
{
    // adjust scale to screensize
    if (nodeImages_)
    {
        Sprite2D* sprite = staticsprites_[0]->GetSprite();
        Rect spriterect;
        if (!sprite->GetDrawRectangle(spriterect))
            return;

        Vector2 screensize(World2D::GetExtendedVisibleRect(viewport_).Size());
        Vector2 spritesize = spriterect.Size();
        Vector2 sizescale = screensize / spritesize;

        float scale = Max(sizescale.x_, sizescale.y_) + 0.1f;
        nodeImages_->SetWorldScale2D(scale, scale);
        halfspritesize_ = 0.5f * scale * spritesize;

        unsigned i=0;
        Vector2 overlap(Vector2::ZERO);
        for (int y=-1; y < 2; y++)
        {
            overlap.y_ = y < 0 ? overlapping_ : (y > 0 ? -overlapping_ : 0.f);
            for (int x=-1; x < 2; x++, i++)
            {
                StaticSprite2D* staticsprite = staticsprites_[i];
                overlap.x_ = x < 0 ? overlapping_ : (x > 0 ? -overlapping_ : 0.f);
                staticsprite->GetNode()->SetPosition2D(Vector2(x*spritesize.x_+offset_.x_/scale, y*spritesize.y_+offset_.y_/scale) + overlap);
            }
        }

        resetScale_ = false;
    }
}

void ScrollingLayer::Update(float timeStep, bool recenter)
{
    motion_ = timeStep * motionspeed_;

    // apply following node with parallax effect
    if (nodeToFollow_)
    {
        if (lastPosition_ != nodeToFollow_->GetWorldPosition2D() || recenter)
        {
            motion_ += parallaxspeed_ * (nodeToFollow_->GetWorldPosition2D() - lastPosition_);
            lastPosition_ = nodeToFollow_->GetWorldPosition2D();
        }
    }

    if (recenter)
    {
        nodeImages_->SetWorldPosition2D(nodeToFollow_ ? nodeToFollow_->GetWorldPosition2D() : Vector2::ZERO);
        URHO3D_LOGINFOF("ScrollingLayer() - Update : recenter nodeimagesPos=%s nodeFollowPos=%s !", nodeImages_->GetWorldPosition2D().ToString().CString(), nodeToFollow_ ? nodeToFollow_->GetWorldPosition2D().ToString().CString() : "ZERO");
    }

    // dynamic scale
    static const float SizeThresHold = 0.1f;
    const float VisibleRectWidth = World2D::GetExtendedVisibleRect(viewport_).Size().x_;
    if (lastVisibleRectWidth_ > VisibleRectWidth + SizeThresHold || lastVisibleRectWidth_ < VisibleRectWidth - SizeThresHold)
    {
        SetScale();

        if (nodeToFollow_)
        {
            // TODO : try to set the position to keep the motion
            nodeImages_->SetWorldPosition2D(nodeToFollow_->GetWorldPosition2D() + ((nodeToFollow_->GetWorldPosition2D() - nodeImages_->GetWorldPosition2D()) + motion_));
        }

        lastVisibleRectWidth_ = VisibleRectWidth;
//        URHO3D_LOGINFOF("ScrollingLayer() - Update : id=%d layer=%d position=%s dynamic scale on !", id_, layer_, nodeImages_->GetWorldPosition2D().ToString().CString());
    }
    else
    {
        // apply motion speed
        nodeImages_->Translate2D(motion_);
    }

    Vector2 position = nodeImages_->GetWorldPosition2D();

    // apply scrolling recenter if overstep the limits
    const Rect& visibleRect = World2D::GetExtendedVisibleRect(viewport_);
    if (motion_.x_ < 0.f && position.x_ - halfspritesize_.x_ + offset_.x_ < visibleRect.min_.x_)
    {
        position.x_ += 2.f*halfspritesize_.x_;
        recenter = true;
    }
    else if (motion_.x_ > 0.f && position.x_ + halfspritesize_.x_ + offset_.x_ > visibleRect.max_.x_)
    {
        position.x_ -= 2.f*halfspritesize_.x_;
        recenter = true;
    }
    if (motion_.y_ < 0.f && position.y_ - halfspritesize_.y_ + offset_.y_ < visibleRect.min_.y_)
    {
        position.y_ += 2.f*halfspritesize_.y_;
        recenter = true;
    }
    else if (motion_.y_ > 0.f && position.y_ + halfspritesize_.y_ + offset_.y_ > visibleRect.max_.y_)
    {
        position.y_ -= 2.f*halfspritesize_.y_;
        recenter = true;
    }

    if (recenter)
    {
        nodeImages_->SetWorldPosition2D(position);
    }

    // update luminosity
    if (GameContext::Get().luminosity_ != GameContext::Get().lastluminosity_)
    {
        float alpha = color_.a_;
        Color color = color_ * GameContext::Get().luminosity_;
        color.a_ = alpha;

        for (unsigned i=0; i < staticsprites_.Size(); i++)
            staticsprites_[i]->SetColor(color);
    }

    // update scale
    if (resetScale_)
        SetScale();

//    URHO3D_LOGINFOF("ScrollingLayer() - Update : id=%d layer=%d position=%s !", id_, layer_, nodeImages_->GetWorldPosition2D().ToString().CString());
}


GEF_Scrolling::GEF_Scrolling(Context* context) :
    Component(context),
    viewport_(0),
    started_(false)
{
    URHO3D_LOGINFOF("GEF_Scrolling() - this=%u", this);
}

GEF_Scrolling::~GEF_Scrolling()
{
//    URHO3D_LOGINFOF("~GEF_Scrolling() - this=%u ...", this);

    Stop();

    URHO3D_LOGINFOF("~GEF_Scrolling() - this=%u ... OK !", this);
}

void GEF_Scrolling::RegisterObject(Context* context)
{
    context->RegisterFactory<GEF_Scrolling>();
}

void GEF_Scrolling::Start(bool forced)
{
    if (started_ && !forced)
        return;

//    URHO3D_LOGINFOF("GEF_Scrolling() - Start : this=%u node=%s(%u) nodeToFollow=%s(%u)", this, node_->GetName().CString(), node_->GetID(), nodeToFollow_ ? nodeToFollow_->GetName().CString() : "None", nodeToFollow_ ? nodeToFollow_->GetID() : 0);

    started_ = true;
    enabled_ = true;
    intensity_ = 1.f;

    for (unsigned i=0; i<layers_.Size(); i++)
    {
        layers_[i].SetNodeToFollow(nodeToFollow_);
    }

    OnSetEnabled();
}

void GEF_Scrolling::Stop(bool forced)
{
    if (!started_ && !forced)
        return;

//    URHO3D_LOGINFOF("GEF_Scrolling() - Stop : this=%u ", this);

    started_ = false;
    enabled_ = false;
    //nodeToFollow_ = 0;

    OnSetEnabled();
}

void GEF_Scrolling::OnSetEnabled()
{
    Scene* scene = GetScene();
    if (scene && IsEnabledEffective())
    {
//        URHO3D_LOGINFOF("GEF_Scrolling() - this=%u OnSetEnabled = true", this);

        if (!nodeToFollow_)
            SetNodeToFollow(ViewManager::Get()->GetCameraNode(viewport_));

        node_->SetWorldPosition2D(nodeToFollow_->GetWorldPosition2D());

        SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(GEF_Scrolling, HandleUpdate));
//        SubscribeToEvent(WORLD_CAMERACHANGED, URHO3D_HANDLER(GEF_Scrolling, HandleCameraPositionChanged));

        for (unsigned i = 0; i < layers_.Size(); i++)
            layers_[i].ForceUpdateBatches();
    }
    else
    {
//        URHO3D_LOGINFOF("GEF_Scrolling() - this=%u OnSetEnabled = false", this);
        UnsubscribeFromAllEvents();
    }

    if (node_)
        node_->SetEnabledRecursive(enabled_);
}

void GEF_Scrolling::AddMaterialLayer(Material* material, int textureFX, int l, int order, unsigned viewmask, float overlap, const String& refname, const Vector2& offsetPosition, const Vector2& speed, const Vector2& parallax, const Color& color)
{
    layers_.Push(ScrollingLayer());

    ScrollingLayer& layer = layers_.Back();
    layer.viewport_ = viewport_;
    layer.layer_ = l;
    layer.order_ = order;
    layer.viewmask_ = viewmask;
    layer.overlapping_ = overlap;
    layer.refname_ = refname;
    layer.material_ = material;
    layer.textureFX_ = textureFX;
    layer.offset_ = offsetPosition;
    layer.motionspeed_ = speed;
    layer.color_ = color;
    layer.parallaxspeed_ = parallax;

    layer.Set(layers_.Size()-1, node_);
}

void GEF_Scrolling::AddSpritesLayer(int layer, int order, unsigned viewmask, float overlap, const String& spriteSheet, Material* material, const String& spriteRadix, const Vector2& offsetPosition, const Vector2& speed, const Vector2& parallax, const Color& color)
{

}

void GEF_Scrolling::AddScrolling(Material* material, int textureFX, const String& refname, Node* nodeRoot, int layer, int order, float overlap, const Vector2& offsetPosition, const Vector2& speed, bool start, const Vector2& parallax, const Color& color)
{
    GEF_Scrolling* gefscrolling = nodeRoot->GetOrCreateComponent<GEF_Scrolling>();
    gefscrolling->SetViewport(0);
    gefscrolling->AddMaterialLayer(material, textureFX, layer, order, Urho3D::DRAWABLE_ANY, overlap, refname, offsetPosition, speed, parallax, color);

    if (start)
    {
        gefscrolling->SetNodeToFollow(ViewManager::Get()->GetCameraNode(0));
        gefscrolling->Start();
    }
}

void GEF_Scrolling::SetNodeToFollow(Node* node)
{
    URHO3D_LOGINFOF("GEF_Scrolling() - SetNodeToFollow : this=%u following=%s(%u) ", this, node ? node->GetName().CString() : "", node ? node->GetID() : 0);

    nodeToFollow_ = node;
    node_->SetWorldPosition2D(nodeToFollow_->GetWorldPosition2D());

    for (unsigned i=0; i<layers_.Size(); i++)
        layers_[i].SetNodeToFollow(node);
}

void GEF_Scrolling::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat() * intensity_;

    for (unsigned i = 0; i < layers_.Size(); i++)
        layers_[i].Update(timeStep);
}





/// DrawableScroller

DrawableObjectInfo::DrawableObjectInfo(const ShortIntVector2& mpoint, Sprite2D* sprite, Material* material, bool flipX, bool flipY) :
    sprite_(sprite),
    material_(material)
{
    if (!sprite_)
        return;

    zone_.x_ = mpoint.x_;
    zone_.y_ = mpoint.y_;

    sprite_->GetDrawRectangle(drawRect_, flipX, flipY);
    sprite_->GetTextureRectangle(textureRect_, flipX, flipY);
}

void DrawableObjectInfo::Set(const ShortIntVector2& mpoint, Sprite2D* sprite, Material* material, bool flipX, bool flipY)
{
    zone_.x_ = mpoint.x_;
    zone_.y_ = mpoint.y_;

    sprite_ = sprite;
    sprite_->GetDrawRectangle(drawRect_, flipX, flipY);
    sprite_->GetTextureRectangle(textureRect_, flipX, flipY);

    material_ = material;
}


DrawableScroller::ScrollerViewInfo DrawableScroller::scrollerinfos_[MAX_VIEWPORTS];
bool DrawableScroller::flatmode_ = 0;

DrawableScroller::DrawableScroller(Context* context) :
    StaticSprite2D(context),
    rotationFollowCurve_(false),
    allowBottomCurve_(false)
{
    enabled_ = false;
}

DrawableScroller::~DrawableScroller() { }

void DrawableScroller::RegisterObject(Context* context)
{
    context->RegisterFactory<DrawableScroller>();
}

void DrawableScroller::Start()
{
    flatmode_ = !World2D::GetWorldInfo()->worldModel_;

    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (int viewport=0; viewport < numviewports; viewport++)
    {
        ScrollerViewInfo& info = scrollerinfos_[viewport];
        info.camPositionCount_ = 0;
        info.camPosition_ = info.lastCamPosition_ = Vector2::ZERO;
        info.nodeToFollow_ = ViewManager::Get()->GetCameraNode(viewport);
        info.initialPosition_ = info.nodeToFollow_->GetWorldPosition2D();

        unsigned numscrollers = info.drawableScrollers_.Size();
        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->SetEnabled(true);

        info.camPositionCount_ = 0;

//        URHO3D_LOGINFOF("DrawableScroller() - Start() : viewport=%d numscrollers=%u !", viewport, numscrollers);
    }

    if (World2D::GetWorld())
        World2D::GetWorld()->SendEvent(WORLD_CAMERACHANGED);
}

void DrawableScroller::Stop()
{
    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (int viewport=0; viewport < numviewports; viewport++)
    {
        ScrollerViewInfo& info = scrollerinfos_[viewport];
        info.camPositionCount_ = 0;
        info.camPosition_ = info.initialPosition_ = info.lastCamPosition_ = Vector2::ZERO;
        info.nodeToFollow_.Reset();

        unsigned numscrollers = info.drawableScrollers_.Size();
        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->SetEnabled(false);

//        URHO3D_LOGINFOF("DrawableScroller() - Stop() : viewport=%d numscrollers=%u !", viewport, numscrollers);
    }
}

void DrawableScroller::ToggleStartStop()
{
    ScrollerViewInfo& info = scrollerinfos_[0];

    bool enable = info.drawableScrollers_.Size() > 0 ? info.drawableScrollers_[0]->IsEnabled() : true;

    if (enable)
        Stop();
    else
        Start();
}

void DrawableScroller::ScrollerViewInfo::Reset()
{
    camPositionCount_ = 0;
    camPosition_ = initialPosition_ = lastCamPosition_ = Vector2::ZERO;
    nodeToFollow_.Reset();

    unsigned numscrollers = drawableScrollers_.Size();
    for (unsigned i = 0; i < numscrollers; ++i)
    {
        drawableScrollers_[i]->Clear();
        drawableScrollers_[i]->Remove();
    }

    drawableScrollers_.Clear();
}

void DrawableScroller::Reset()
{
    URHO3D_LOGINFOF("DrawableScroller() - Reset !");

    for (int viewport=0; viewport < MAX_VIEWPORTS; viewport++)
        scrollerinfos_[viewport].Reset();
}

void DrawableScroller::Pause(int viewport, bool state)
{
    ScrollerViewInfo& info = scrollerinfos_[viewport];
    unsigned numscrollers = info.drawableScrollers_.Size();

    if (state)
    {
        URHO3D_LOGINFOF("DrawableScroller() - Pause : true");

        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->UnsubscribeFromEvent(GameContext::Get().rootScene_, E_SCENEUPDATE);
    }
    else
    {
        URHO3D_LOGINFOF("DrawableScroller() - Pause : false");

        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->OnSetEnabled();

        VariantMap& eventdata = GameContext::Get().context_->GetEventDataMap();
        info.drawableScrollers_[0]->HandleCameraPositionChanged(StringHash(), eventdata);
    }
}

void DrawableScroller::Clear()
{
    URHO3D_LOGINFOF("DrawableScroller() - Clear !");

    drawablePool_.Clear();
    freeDrawables_.Clear();
    drawables_.Clear();

    worldScrollerObjectInfos_.Clear();
}

void DrawableScroller::RemoveAllObjectsOnMap(const ShortIntVector2& mpoint)
{
    unsigned numviewports = ViewManager::Get()->GetNumViewports();

    for (int viewport=0; viewport < numviewports; viewport++)
    {
        ScrollerViewInfo& info = scrollerinfos_[viewport];

        unsigned numscrollers = info.drawableScrollers_.Size();
        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->RemoveScrollerObjects(mpoint);
    }

//    URHO3D_LOGINFOF("DrawableScroller() - RemoveAllObjectsOnMap : mpoint=%s !", mpoint.ToString().CString());
}

void DrawableScroller::SetParallax(const Vector2& parallax)
{
    parallax_ = parallax;
}

void DrawableScroller::SetLogTest(bool enable)
{
    logtest_ = enable;
}

void DrawableScroller::ResizeWorldScrollerObjectInfos(const ShortIntVector2& mpoint, unsigned numobjects, Sprite2D* sprite)
{
    Vector<DrawableObjectInfo>& drawableobjectinfos = worldScrollerObjectInfos_[mpoint];

    if (drawableobjectinfos.Size() == numobjects)
        return;

    drawableobjectinfos.Resize(numobjects);

    Material* material = customMaterial_ ? customMaterial_ : renderer_->GetMaterial(sprite->GetTexture(), GetBlendMode());

    // add infos to scroller
    for (Vector<DrawableObjectInfo>::Iterator it=drawableobjectinfos.Begin(); it!=drawableobjectinfos.End(); ++it)
    {
        it->Set(mpoint, sprite, material, Random(100) > 49, false);
//        it->drawRect_.min_ *= scrollerscale_;
//        it->drawRect_.max_ *= scrollerscale_;
    }
}

void DrawableScroller::RemoveScrollerObjects(const ShortIntVector2& mpoint)
{
    worldScrollerObjectInfos_[mpoint].Clear();
    sourceBatchesDirty_ = true;
}

DrawableScroller* DrawableScroller::AddScroller(int viewport, Material* material, int textureFX, int layer, int order, const Vector2& scale, const Vector2& motionBounds, const Vector2& parallax,
        EllipseW* boundCurve, bool allowBottomCurve, bool rotFollowCurve, const Vector2& offset, const Color& color, bool logTest)
{
    ScrollerViewInfo& info = scrollerinfos_[viewport];

    DrawableScroller* scroller = GameContext::Get().rootScene_->CreateComponent<DrawableScroller>();

    scroller->SetViewport(viewport);
    scroller->SetId(info.drawableScrollers_.Size());
    scroller->SetCustomMaterial(material);
    scroller->SetTextureFX(textureFX);
    scroller->SetLayer2(IntVector2(layer,-1));
    scroller->SetOrderInLayer(order);
    scroller->SetViewMask(layer <= INNERVIEW ? (VIEWPORTSCROLLER_OUTSIDE_MASK << viewport) | (VIEWPORTSCROLLER_INSIDE_MASK << viewport) : VIEWPORTSCROLLER_INSIDE_MASK << viewport);
    scroller->SetScale(scale);
    scroller->SetBoundCurve(boundCurve, allowBottomCurve);
    scroller->SetRotationFollowCurve(rotFollowCurve);
    scroller->SetMotionBounds(motionBounds);
    scroller->SetParallax(parallax);
    scroller->SetOffset(offset);
    scroller->SetColor(color);
    scroller->SetLogTest(logTest);

    info.drawableScrollers_.Push(scroller);

//    URHO3D_LOGINFOF("DrawableScroller() - AddScroller : viewport=%d id=%u scrollerptr=%u parallax=%s", scroller->viewport_, scroller->id_, scroller, parallax.ToString().CString());

    return scroller;
}

BoundingBox DrawableScroller::GetWorldBoundingBox2D()
{
    return Drawable2D::GetWorldBoundingBox2D();
}

void DrawableScroller::OnWorldBoundingBoxUpdate()
{
    boundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
    worldBoundingBox_ = boundingBox_;
}

void DrawableScroller::OnDrawOrderChanged()
{
    for (unsigned i = 0; i < sourceBatches_[0].Size(); ++i)
        sourceBatches_[0][i].drawOrder_ = GetDrawOrder();
}

void DrawableScroller::OnSetEnabled()
{
    StaticSprite2D::OnSetEnabled();

    ScrollerViewInfo& info = scrollerinfos_[viewport_];

    Scene* scene = GetScene();
    if (scene && IsEnabledEffective())
    {
        URHO3D_LOGINFOF("DrawableScroller() - this=%u viewport=%d flatmode=%s OnSetEnabled = true nodeToFollow=%s(%u) numdrawableObjects=%u",
                        this, viewport_, flatmode_?"true":"false", info.nodeToFollow_ ? info.nodeToFollow_->GetName().CString() :
                        "NONE", info.nodeToFollow_ ? info.nodeToFollow_->GetID() : 0, drawables_.Size());

        info.camPosition_ = info.lastCamPosition_ = info.nodeToFollow_ ? info.nodeToFollow_->GetWorldPosition2D() : Vector2::ZERO;

        if (flatmode_)
        {
            boundcurve_ = 0;
            SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(DrawableScroller, HandleUpdateFlatMode));
        }
        else
        {
            SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(DrawableScroller, HandleUpdateEllipseMode));
        }

        if (!info.camPositionCount_)
        {
            SubscribeToEvent(WORLD_CAMERACHANGED, URHO3D_HANDLER(DrawableScroller, HandleCameraPositionChanged));
        }

        SetDrawableObjects();
    }
    else
    {
        URHO3D_LOGINFOF("DrawableScroller() - this=%u viewport=%d OnSetEnabled = false", this, viewport_);
        UnsubscribeFromAllEvents();
    }

    info.camPositionCount_++;

    sourceBatchesDirty_ = true;
}

//#define DRAWABLESCROLLER_COLORZONETEST

void DrawableScroller::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    if (!drawables_.Size())
    {
#ifdef DUMP_DRAWABLESCROLLER_WARNINGS
        URHO3D_LOGWARNINGF("DrawableScroller() - UpdateSourceBatches : viewport=%d map=%s => no drawableobjects !", viewport_, World2D::GetCurrentMapPoint(viewport_).ToString().CString());
#endif
        sourceBatches_[0].Clear();
        sourceBatchesDirty_ = false;
        return;
    }

    sourceBatches_[0].Clear();

    if (!IsEnabledEffective() || !visibility_)
    {
        sourceBatchesDirty_ = false;
        URHO3D_LOGWARNINGF("DrawableScroller() - UpdateSourceBatches : viewport=%d this=%u disabled !", viewport_, this);
        return;
    }

    const Material* lastmaterial = 0;
    int ibatch = -1;
    int draworder = GetDrawOrder();

    unsigned color;
    {
        float alpha = color_.a_;
        Color lumcolor = color_ * GameContext::Get().luminosity_;
        lumcolor.a_ = alpha;
        color = lumcolor.ToUInt();
    }

#ifdef DRAWABLESCROLLER_COLORZONETEST
    Color colorzone;
    colorzone.a_ = 1.f;
#endif

    if (!boundcurve_ || !rotationFollowCurve_)
    {
        for (Vector<DrawableObject*>::Iterator it = drawables_.Begin(); it != drawables_.End(); ++it)
        {
            DrawableObject& drawable = **it;

            if (!drawable.enable_)
                continue;

            const Rect& drawRect = drawable.info_->drawRect_;
            matrix_.Set(scrollerposition_ + drawable.position_, 0.f, scrollerscale_);
            drawable.v0_.position_ = matrix_ * drawRect.min_;
            drawable.v1_.position_ = matrix_ * Vector2(drawRect.min_.x_, drawRect.max_.y_);
            drawable.v2_.position_ = matrix_ * drawRect.max_;
            drawable.v3_.position_ = matrix_ * Vector2(drawRect.max_.x_, drawRect.min_.y_);
        }
    }

    for (Vector<DrawableObject*>::Iterator it = drawables_.Begin(); it != drawables_.End(); ++it)
    {
        DrawableObject& drawable = **it;

        if (!drawable.enable_)
            continue;

        const DrawableObjectInfo& dinfo = *drawable.info_;
        const Rect& textureRect = dinfo.textureRect_;

    #ifdef URHO3D_VULKAN
        drawable.v0_.z_ = drawable.v1_.z_ = drawable.v2_.z_ = drawable.v3_.z_ = 0.f;
    #else
        drawable.v0_.position_.z_ = drawable.v1_.position_.z_ = drawable.v2_.position_.z_ = drawable.v3_.position_.z_ = 0.f;
    #endif

        if (drawable.flipX_)
        {
            // Flip X
            drawable.v0_.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);
            drawable.v1_.uv_ = textureRect.max_;
            drawable.v2_.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
            drawable.v3_.uv_ = textureRect.min_;
        }
        else
        {
            // No Flip
            drawable.v0_.uv_ = textureRect.min_;
            drawable.v1_.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
            drawable.v2_.uv_ = textureRect.max_;
            drawable.v3_.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);
        }

#ifdef DRAWABLESCROLLER_COLORZONETEST
        colorzone.r_ = colorzone.g_ = colorzone.b_ = 0.25f*(float)(Abs(dinfo.zone_.x_)) + 0.25f;
        drawable.v0_.color_ = drawable.v1_.color_ = drawable.v2_.color_ = drawable.v3_.color_ = colorzone.ToUInt();
#else
        drawable.v0_.color_ = drawable.v1_.color_ = drawable.v2_.color_ = drawable.v3_.color_ = color;
#endif
        Material* const& material = dinfo.material_;
        if (lastmaterial != material)
        {
            ibatch++;
            sourceBatches_[0].Resize(ibatch+1);
            sourceBatches_[0].Back().material_ = material;
            sourceBatches_[0].Back().drawOrder_ = draworder;
            lastmaterial = material;
        }

#ifdef URHO3D_VULKAN
        unsigned texmode = 0;
#else
        Vector4 texmode;
#endif
        SetTextureMode(TXM_UNIT, material ? material->GetTextureUnit(dinfo.sprite_->GetTexture()) : 0, texmode);
        SetTextureMode(TXM_FX, textureFX_, texmode);
        drawable.v0_.texmode_ = drawable.v1_.texmode_ = drawable.v2_.texmode_ = drawable.v3_.texmode_ = texmode;

        Vector<Vertex2D>& vertices = sourceBatches_[0].Back().vertices_;
        vertices.Push(drawable.v0_);
        vertices.Push(drawable.v1_);
        vertices.Push(drawable.v2_);
        vertices.Push(drawable.v3_);
    }

    sourceBatchesDirty_ = false;
}

void DrawableScroller::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
//    if (parallax_.x_ != 0.015f)
//        return;

    if (!IsEnabledEffective())
        return;

    if (!logtest_)
        return;

    if (boundcurve_ && rotationFollowCurve_)
    {
        const Vector<DrawableObject*>* vdrawables[2] = { &drawablesTopCurve_, &drawablesBottomCurve_ };
        const unsigned uintColors[2] = { Color::BLUE.ToUInt(), Color::GREEN.ToUInt() };
        for (int i = 0; i < 2; i++)
        {
            const unsigned uintColor = uintColors[i];
            const Vector<DrawableObject*>& drawables = *vdrawables[i];
            for (Vector<DrawableObject*>::ConstIterator it = drawables.Begin(); it != drawables.End(); ++it)
            {
                const DrawableObject& drawable = **it;

                if (drawable.rect_.Defined())
                {
                    debug->AddLine(Vector3(drawable.rect_.min_.x_, drawable.rect_.min_.y_), Vector3(drawable.rect_.min_.x_, drawable.rect_.max_.y_), uintColor, false);
                    debug->AddLine(Vector3(drawable.rect_.min_.x_, drawable.rect_.max_.y_), Vector3(drawable.rect_.max_.x_, drawable.rect_.max_.y_), uintColor, false);
                    debug->AddLine(Vector3(drawable.rect_.max_.x_, drawable.rect_.max_.y_), Vector3(drawable.rect_.max_.x_, drawable.rect_.min_.y_), uintColor, false);
                    debug->AddLine(Vector3(drawable.rect_.max_.x_, drawable.rect_.min_.y_), Vector3(drawable.rect_.min_.x_, drawable.rect_.min_.y_), uintColor, false);
                }
                else
                {
                    debug->AddLine(drawable.v0_.position_, drawable.v1_.position_, uintColor, false);
                    debug->AddLine(drawable.v1_.position_, drawable.v2_.position_, uintColor, false);
                    debug->AddLine(drawable.v2_.position_, drawable.v3_.position_, uintColor, false);
                    debug->AddLine(drawable.v3_.position_, drawable.v0_.position_, uintColor, false);
                }
            }

            if (drawables.Size())
            {
                debug->AddCross(Vector3(scrollerposition_ + scrolleroffset_ + drawables.Front()->position_), 2.f, Color::RED, false);
                debug->AddCross(Vector3(scrollerposition_ + scrolleroffset_ + drawables.Back()->position_), 2.f, Color::GREEN, false);
            }
        }
    }
    else
    {
        const Vector<DrawableObject*>& drawables = drawables_;
        for (Vector<DrawableObject*>::ConstIterator it = drawables.Begin(); it != drawables.End(); ++it)
        {
            const DrawableObject& drawable = **it;
            debug->AddCross(Vector3(drawable.position_),2.f, Color::BLUE, false);
        }

//        URHO3D_LOGWARNINGF("DrawableScroller() - DrawDebugGeometry : this=%u drawablesSize=%u !", this, drawables.Size());
    }
}

void DrawableScroller::Dump() const
{
    if (!IsEnabledEffective())
        return;

    if (!logtest_)
        return;

    URHO3D_LOGINFOF("DrawableScroller() - Dump : this=%u ...", this);

    if (boundcurve_ && rotationFollowCurve_)
    {
        const Vector<DrawableObject*>* vdrawables[2] = { &drawablesTopCurve_, &drawablesBottomCurve_ };
        const unsigned uintColors[2] = { Color::BLUE.ToUInt(), Color::GREEN.ToUInt() };
        for (int i = 0; i < 2; i++)
        {
            const unsigned uintColor = uintColors[i];
            const Vector<DrawableObject*>& drawables = *vdrawables[i];
            int j = 0;
            for (Vector<DrawableObject*>::ConstIterator it = drawables.Begin(); it != drawables.End(); ++it, ++j)
            {
                const DrawableObject& drawable = **it;
                URHO3D_LOGINFOF(" this=%u drawables[%d][%d] : position=(%F,%F)", this, i, j, drawable.position_.x_, drawable.position_.y_);
            }
        }
    }
    else
    {
        const unsigned uintColor = Color::BLUE.ToUInt();
        const Vector<DrawableObject*>& drawables = drawables_;
        int j = 0;
        for (Vector<DrawableObject*>::ConstIterator it = drawables.Begin(); it != drawables.End(); ++it, ++j)
        {
            const DrawableObject& drawable = **it;
            URHO3D_LOGINFOF(" this=%u drawables[%d] : position=(%F,%F) enable=%s", this, j, drawable.position_.x_, drawable.position_.y_, drawable.enable_ ? "true":"false");
        }
    }
}

void DrawableScroller::DumpDrawables(int viewport)
{
    ScrollerViewInfo& info = scrollerinfos_[viewport];
    const Vector<DrawableScroller*>& scrollers = info.drawableScrollers_;
    for (unsigned i=0; i < scrollers.Size(); i++)
        scrollers[i]->Dump();
}

// Get the new y and map for x at the mpoint map
float GetNewYAtX(int viewport, const float& x, ShortIntVector2& mpoint, Map*& map)
{
    mpoint.x_ = World2D::GetWorldInfo()->GetMapPointCoordX(x);

    // Check the good map for x
    if (!map || mpoint != map->GetMapPoint())
    {
        if (mpoint == World2D::GetCurrentMapPoint(viewport))
        {
            map = World2D::GetCurrentMap(viewport);
        }
        else
        {
            mpoint.y_ = World2D::GetCurrentMapPoint(viewport).y_;
            map = World2D::GetMapAt(mpoint);
        }

        if (!map)
            return -1.f;
    }

    // Check the good map for y
    float ytemp = map->GetTopography().GetFloorY(x-map->GetTopography().maporigin_.x_);
    if (ytemp < 0.f)
        mpoint.y_ = map->GetMapPoint().y_ - 1;
    else if (ytemp >= MapInfo::info.mHeight_)
        mpoint.y_ = map->GetMapPoint().y_ + 1;

    if (mpoint != map->GetMapPoint())
    {
        if (mpoint != World2D::GetCurrentMapPoint(viewport))
        {
            map = World2D::GetMapAt(mpoint);
            if (!map)
                return -1.f;
        }
        else
            map = World2D::GetCurrentMap(viewport);
    }

    // Finally get the y on the good map
    return map->GetTopography().GetY(x);
}

bool DrawableScroller::SetDrawableObjects()
{
    if (drawablePool_.Size() > 0)
    {
        if (logtest_)
            URHO3D_LOGERRORF("DrawableScroller() - SetDrawableObjects : this=%u viewport=%d drawablePool_.Size()=%u drawables_.Size()=%u ... skip !", this, viewport_, drawablePool_.Size(), drawables_.Size());
        return true;
    }

    ScrollerViewInfo& info = scrollerinfos_[viewport_];

    Map* map = World2D::GetCurrentMap(viewport_);
    if (!map)
    {
#ifdef DUMP_DRAWABLESCROLLER_WARNINGS
        URHO3D_LOGERRORF("DrawableScroller() - SetDrawableObjects : viewport=%d no current map !", viewport_);
#endif
        return false;
    }

    DrawableObjectInfo* dinfo = GetWorldScrollerObjectInfo(World2D::GetCurrentMapPoint(viewport_), 0);
    if (!dinfo)
    {
        if (logtest_)
            URHO3D_LOGWARNINGF("DrawableScroller() - SetDrawableObjects : viewport=%d map=%s => no dinfo ... skip !", viewport_, World2D::GetCurrentMapPoint(viewport_).ToString().CString());
        return false;
    }

    if (dinfo->drawRect_.Size().x_ <= 0.f)
    {
        URHO3D_LOGERRORF("DrawableScroller() - SetDrawableObjects : dinfo->drawRect_.Size().x_ <= 0.f !");
        return false;
    }

    info.camPosition_ = info.lastCamPosition_ = info.nodeToFollow_ ? info.nodeToFollow_->GetWorldPosition2D() : Vector2::ZERO;

    // Reset the initial camera position for the scroller
    if (!flatmode_)
    {
        // 26/06/2023 : add PIXEL_SIZE to prevent "no initial move"
        info.initialPosition_ = info.camPosition_;
        info.initialPosition_.x_ += PIXEL_SIZE;
    }

    info.lastCamPosition_.x_ -= 0.1f;

    // Scaling
    drawableSize_    = dinfo->drawRect_.Size() * scrollerscale_;
    drawableoverlap_ = drawableSize_.x_ * 0.25f;

    const Rect& visibleRect = World2D::GetExtendedVisibleRect(viewport_);
    const Vector2 camInitialOffset = info.camPosition_ - info.initialPosition_;

    if (logtest_)
        URHO3D_LOGINFOF("DrawableScroller() - SetDrawableObjects : this=%u viewport=%d currentmap=%s visibleRectMin=%F,%F nodeToFollow=%s(%u) campPos=%F,%F lastPos=%F,%F initpos=%F,%F ...",
                        this, viewport_, map ? map->GetMapPoint().ToString().CString() : "", visibleRect.min_.x_, visibleRect.min_.y_,
                        info.nodeToFollow_ ? info.nodeToFollow_->GetName().CString() : "", info.nodeToFollow_ ? info.nodeToFollow_->GetID() : 0,
                        info.camPosition_.x_, info.camPosition_.y_, info.lastCamPosition_.x_, info.lastCamPosition_.y_, info.initialPosition_.x_, info.initialPosition_.y_);

    drawables_.Clear();
    freeDrawables_.Clear();
    drawablesBottomCurve_.Clear();
    drawablesTopCurve_.Clear();

    if (boundcurve_)
    {
        // Initialize the pool, set the maximal number of drawables in the pool.
        const unsigned MaxDrawablesInPool = 15U;
        drawablePool_.Resize(MaxDrawablesInPool);
        drawables_.Reserve(MaxDrawablesInPool);
        freeDrawables_.Reserve(MaxDrawablesInPool);
        for (unsigned i = 0; i < MaxDrawablesInPool; i++)
        {
            drawablePool_[i].info_ = dinfo;
            freeDrawables_.Push(&drawablePool_[i]);
        }

        // Initialize the drawable to draw for the visible Rect.
        const float diry = boundcurve_ ? (info.camPosition_.y_ > boundcurve_->center_.y_ ? 1.f : -1.f) : 1.f;

        // Extends to the right
        Vector2 drawablePosition;
        float hw = 0.f;
        const float borderLeft  = visibleRect.min_.x_ - scrollerposition_.x_ - scrolleroffset_.x_;
        const float borderRight = visibleRect.max_.x_ - scrollerposition_.x_ - scrolleroffset_.x_;
        lastDrawablePositionAtLeft_ = drawablePosition.x_ = borderLeft;

        if (!rotationFollowCurve_)
        {
            while (freeDrawables_.Size() && drawablePosition.x_ - hw < borderRight)
            {
                drawables_.Push(freeDrawables_.Back());
                freeDrawables_.Pop();

                DrawableObject& drawable = *drawables_.Back();
                drawable.position_.x_ = drawablePosition.x_;
                drawable.flipX_ = drawables_.Size() % 3;

                boundcurve_->GetPositionOn(0.f, diry, drawable.position_);
                drawable.size_ = drawable.info_->drawRect_.Size() * scrollerscale_;
                drawable.size_ *= drawable.angle_;

                // next drawable position
                hw = drawable.size_.x_ * 0.375f; // overlap of 0.25 => 1.0-0.25 = 0.75 => half => 0.375
                drawablePosition.x_ = drawable.position_.x_ + 2.f * hw;

                if (logtest_)
                    URHO3D_LOGINFOF(" this=%u ... generate to right drawables[%u] position=%s (visibleRect=%s)...", this, drawables_.Size()-1, drawable.position_.ToString().CString(), visibleRect.ToString().CString());
            }
        }
    }
    else
    {
        // Set scroller offset in X
        scrolleroffset_.x_ = drawableSize_.x_ * 0.5f;

        // Set the number of drawables to show for the visible rect.
        const float visiblewidth = visibleRect.Size().x_ + drawableSize_.x_;
        unsigned numdrawables = (unsigned)floor(visiblewidth / (drawableSize_.x_ - drawableoverlap_)) + 2;
        drawablePool_.Resize(numdrawables);

        ShortIntVector2 mpoint = World2D::GetCurrentMapPoint(viewport_);
        for (unsigned idrawable = 0; idrawable < numdrawables; idrawable++)
        {
            drawables_.Push(&drawablePool_[idrawable]);

            DrawableObject& drawable = drawablePool_[idrawable];
            drawable.info_ = dinfo;
            drawable.position_.x_ = visibleRect.min_.x_ - scrolleroffset_.x_ + idrawable * (drawableSize_.x_ - drawableoverlap_);

            if (!map)
                map = World2D::GetCurrentMap(viewport_);

            float y = GetNewYAtX(viewport_, drawable.position_.x_, mpoint, map);
            drawable.enable_ = parallax_.x_ < 0.1f ? map != 0 : map && y > -1.f;
            if (drawable.enable_)
                drawable.position_.y_ = scrolleroffset_.y_ + map->GetTopography().maporigin_.y_ + y;

            if (logtest_)
                URHO3D_LOGINFOF(" ... drawableObjects[%u] position=%s ...", idrawable, drawable.position_.ToString().CString());
        }
    }

    if (logtest_)
        URHO3D_LOGINFOF("DrawableScroller() - SetDrawableObjects : this=%u ... drawables_.Size()=%u OK !", this, drawables_.Size());

    return true;
}

void DrawableScroller::ClearDrawables()
{
    while (drawables_.Size())
    {
        freeDrawables_.Push(drawables_.Back());
        drawables_.Pop();
    }

    drawables_.Clear();
    drawablesBottomCurve_.Clear();
    drawablesTopCurve_.Clear();
}

void DrawableScroller::SetDrawableVertices(DrawableObject& drawable)
{
    matrix_.Set(scrollerposition_ + Vector2(0.f, drawable.angle_.x_ != 0.f ? (drawable.angle_.x_ > 0.f ? scrolleroffset_.y_ : -scrolleroffset_.y_) : 0.f) + drawable.position_, drawable.angle_, scrollerscale_);
    drawable.v0_.position_ = matrix_ * drawable.info_->drawRect_.min_;
    drawable.v1_.position_ = matrix_ * Vector2(drawable.info_->drawRect_.min_.x_, drawable.info_->drawRect_.max_.y_);
    drawable.v2_.position_ = matrix_ * drawable.info_->drawRect_.max_;
    drawable.v3_.position_ = matrix_ * Vector2(drawable.info_->drawRect_.max_.x_, drawable.info_->drawRect_.min_.y_);

#ifdef URHO3D_VULKAN
    drawable.rect_.Define(drawable.v0_.position_);
    drawable.rect_.Merge(drawable.v1_.position_);
    drawable.rect_.Merge(drawable.v2_.position_);
    drawable.rect_.Merge(drawable.v3_.position_);
#else
    drawable.rect_.Define(drawable.v0_.position_.ToVector2());
    drawable.rect_.Merge(drawable.v1_.position_.ToVector2());
    drawable.rect_.Merge(drawable.v2_.position_.ToVector2());
    drawable.rect_.Merge(drawable.v3_.position_.ToVector2());
#endif
    drawable.enable_ = true;
}

void DrawableScroller::UpdateDrawablesOnTopCurve(Vector2 first, Vector2 last)
{
    Rect visibleRect = World2D::GetExtendedVisibleRect(viewport_);

    const float overlap = 0.75f;
    const float maximizedOverlap = 0.5f;

    Vector<DrawableObject*>& drawables = drawablesTopCurve_;

    Vector2 bmin(Min(first.x_, last.x_), Min(first.y_, last.y_));
    Vector2 bmax(Max(first.x_, last.x_), Max(first.y_, last.y_));

    if (bmin.x_ >= visibleRect.min_.x_)
        bmin.x_ = visibleRect.min_.x_;
    else if (bmax.x_ > visibleRect.max_.x_)
        bmax.x_ = visibleRect.max_.x_;

    // Y Reduce to Top Curve
    if (bmin.y_ <= boundcurve_->center_.y_)
        bmin.y_ = boundcurve_->center_.y_;
    if (bmin.y_ < visibleRect.min_.y_)
        bmin.y_ = visibleRect.min_.y_;

    if (bmax.y_ <= visibleRect.max_.y_)
        bmax.y_ = visibleRect.max_.y_;
    else
        visibleRect.max_.y_ = bmax.y_;

    // Remove Drawables Outside Bounds
    if (drawables.Size())
    {
        Vector<DrawableObject*>::Iterator it = drawables.Begin();
        while (it != drawables.End())
        {
            if (visibleRect.IsInside((*it)->rect_) == OUTSIDE)
            {
//                if (logtest_)
//                    URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... visibleRect(%F,%F %F,%F) ... Outside => remove drawable at (%F,%F) Rect(%F,%F %F,%F)",
//                                this, visibleRect.min_.x_, visibleRect.min_.y_, visibleRect.max_.x_, visibleRect.max_.y_, (*it)->position_.x_, (*it)->position_.y_, (*it)->rect_.min_.x_, (*it)->rect_.min_.y_, (*it)->rect_.max_.x_, (*it)->rect_.max_.y_);

                freeDrawables_.Push(*it);
                it = drawables.Erase(it);
                continue;
            }

            ++it;
        }
    }

    if (!freeDrawables_.Size())
        return;

    if (visibleRect.max_.y_ < boundcurve_->center_.y_)
        return;

    if (logtest_)
        URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... entry(%F,%F %F,%F) bounds(%F,%F %F,%F) ... scroll=%F %F ", this, first.x_, first.y_, last.x_, last.y_, bmin.x_, bmin.y_, bmax.x_, bmax.y_, scrollerposition_.x_, scrollerposition_.y_);

    // Update Positions for existing drawables
    if (scrollerposition_.x_ * scrollerposition_.y_ != 0.f)
    {
        for (Vector<DrawableObject*>::Iterator it = drawables.Begin(); it != drawables.End(); ++it)
        {
            DrawableObject& drawable = **it;
            boundcurve_->GetPositionOn(scrollerposition_.x_, 1.f, drawable.position_, drawable.angle_);
            SetDrawableVertices(drawable);
        }
    }

    const float quadrantx = bmin.x_ < boundcurve_->center_.x_ ? -1.f : 1.f;
    Vector2 pos;

    // On X Ellipse border Add Drawable
    if (freeDrawables_.Size() && bmin.y_ == boundcurve_->center_.y_)
    {
        if (quadrantx < 0.f && (!drawables.Size() || drawables.Front()->position_.x_ != boundcurve_->GetMinX()))
        {
            DrawableObject& drawable = *freeDrawables_.Back();
            drawable.flipX_ = drawables.Size() % 3;
            drawable.position_.y_ = boundcurve_->center_.y_;
            drawable.angle_.x_ = 0.f;
            drawable.position_.x_ = boundcurve_->GetMinX();
            drawable.angle_.y_ = 1.f;
            SetDrawableVertices(drawable);
            drawables.Insert(0, &drawable);
            freeDrawables_.Pop();
            pos = drawable.position_;
            if (logtest_)
                URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... bounds(%F,%F %F,%F) ... add Xborder- drawable at (%F,%F) ... drawablesSize=%u !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, pos.x_, pos.y_, drawables.Size());
        }
        else if (quadrantx > 0.f && (!drawables.Size() || drawables.Back()->position_.x_ != boundcurve_->GetMaxX()))
        {

            DrawableObject& drawable = *freeDrawables_.Back();
            drawable.flipX_ = drawables.Size() % 3;
            drawable.position_.y_ = boundcurve_->center_.y_;
            drawable.angle_.x_ = 0.f;
            drawable.position_.x_ = boundcurve_->GetMaxX();
            drawable.angle_.y_ = -1.f;
            SetDrawableVertices(drawable);
            drawables.Insert(drawables.Size(), &drawable);
            freeDrawables_.Pop();
            pos = drawable.position_;
            if (logtest_)
                URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... bounds(%F,%F %F,%F) ... add Xborder+ drawable at (%F,%F) ... drawablesSize=%u !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, pos.x_, pos.y_, drawables.Size());
        }
    }

    // Put the first drawable if not exist.
    if (!drawables.Size())
    {
        DrawableObject& drawable = *freeDrawables_.Back();
        drawable.flipX_ = false;
        drawable.position_.x_ = (first.x_ + last.x_) / 2.f - scrollerposition_.x_;

        boundcurve_->GetPositionOn(scrollerposition_.x_, 1.f, drawable.position_, drawable.angle_);

        SetDrawableVertices(drawable);
        if (visibleRect.IsInside(drawable.rect_) == OUTSIDE)
        {
//            URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... bounds(%F,%F %F,%F) ... can't add first drawable at (%F,%F) scrollY=%F Rect(%F,%F %F,%F) Outside visiblerect(%F,%F %F,%F) !",
//                            this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, drawable.position_.x_, drawable.position_.y_, scrollerposition_.y_,
//                            drawable.rect_.min_.x_, drawable.rect_.min_.y_, drawable.rect_.max_.x_, drawable.rect_.max_.y_,
//                            visibleRect.min_.x_, visibleRect.min_.y_, visibleRect.max_.x_, visibleRect.max_.y_);
            return;
        }

        drawables.Insert(0, &drawable);
        freeDrawables_.Pop();

        pos = drawable.position_;

        if (logtest_)
            URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... bounds(%F,%F %F,%F) ... add first drawable at (%F,%F) !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, pos.x_, pos.y_);
    }
    else
    {
        pos = drawables.Front()->position_;
    }

    // Expand to bmin bound
    float w = drawables.Front()->info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawables.Front()->angle_.x_);
    while (freeDrawables_.Size() && pos.x_ > boundcurve_->GetMinX())
    {
        // be sure to have a width to add
        if (w < 0.01f)
            w = 0.01f;
        // maximize the overlapping near X border
        else if (w < 1.f)
            w *= maximizedOverlap;

        DrawableObject& drawable = *freeDrawables_.Back();
        drawable.flipX_ = drawables.Size() % 3;
        drawable.position_.x_ = pos.x_ - w * overlap;

        boundcurve_->GetPositionOn(scrollerposition_.x_, 1.f, drawable.position_, drawable.angle_);
        if (drawable.position_.x_ >= pos.x_)
            break;

        SetDrawableVertices(drawable);
        if (visibleRect.IsInside(drawable.rect_) == OUTSIDE)
            break;

        drawables.Insert(0, &drawable);
        freeDrawables_.Pop();

        if (logtest_)
            URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... bounds(%F,%F %F,%F) ... DirX- add drawable at (%F,%F) w=%F ... drawablesSize=%u !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, drawable.position_.x_, drawable.position_.y_, w, drawables.Size());

        // next drawable position
        pos = drawable.position_;
        w = drawable.info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawable.angle_.x_);
    }

    // Expand to bmax bound
    pos = drawables.Back()->position_;
    w = drawables.Back()->info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawables.Back()->angle_.x_);
    while (freeDrawables_.Size() && pos.x_ < boundcurve_->GetMaxX())
    {
        // be sure to have a width to add
        if (w < 0.01f)
            w = 0.01f;
        // maximize the overlapping near X border
        else if (w < 1.f)
            w *= maximizedOverlap;

        DrawableObject& drawable = *freeDrawables_.Back();
        drawable.flipX_ = drawables.Size() % 3;
        drawable.position_.x_ = pos.x_ + w * overlap;

        boundcurve_->GetPositionOn(scrollerposition_.x_, 1.f, drawable.position_, drawable.angle_);
        if (drawable.position_.x_ <= pos.x_)
            break;

        SetDrawableVertices(drawable);
        if (visibleRect.IsInside(drawable.rect_) == OUTSIDE)
            break;

        drawables.Insert(drawables.Size(), &drawable);
        freeDrawables_.Pop();

        if (logtest_)
            URHO3D_LOGINFOF("UpdateDrawablesOnTopCurve : this=%u ... bounds(%F,%F %F,%F) ... DirX+ add drawable at (%F,%F) w=%F ... drawablesSize=%u", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, drawable.position_.x_, drawable.position_.y_, w, drawables.Size());

        // next drawable position
        pos = drawable.position_;
        w = drawable.info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawable.angle_.x_);
    }
}

void DrawableScroller::UpdateDrawablesOnBottomCurve(Vector2 first, Vector2 last)
{
    if (!allowBottomCurve_)
        return;

    Rect visibleRect = World2D::GetExtendedVisibleRect(viewport_);

    const float overlap = 0.75f;
    const float maximizedOverlap = 0.5f;

    Vector<DrawableObject*>& drawables = drawablesBottomCurve_;

    Vector2 bmin(Min(first.x_, last.x_), Min(first.y_, last.y_));
    Vector2 bmax(Max(first.x_, last.x_), Max(first.y_, last.y_));

    if (bmin.x_ >= visibleRect.min_.x_)
        bmin.x_ = visibleRect.min_.x_;
    else if (bmax.x_ > visibleRect.max_.x_)
        bmax.x_ = visibleRect.max_.x_;

    if (bmax.y_ > visibleRect.max_.y_)
        bmax.y_ = visibleRect.max_.y_;

    // Y Reduce to Bottom Curve
    if (bmax.y_ >= boundcurve_->center_.y_)
        bmax.y_ = boundcurve_->center_.y_;

    if (bmin.y_ >= visibleRect.min_.y_)
        bmin.y_ = visibleRect.min_.y_;
    else
        visibleRect.min_.y_ = bmin.y_;

    // Remove Drawables Outside Bounds
    if (drawables.Size())
    {
        Vector<DrawableObject*>::Iterator it = drawables.Begin();
        while (it != drawables.End())
        {
            if (visibleRect.IsInside((*it)->rect_) == OUTSIDE)
            {
//                if (logtest_)
//                    URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... visibleRect(%F,%F %F,%F) ... Outside => remove drawable at (%F,%F) Rect(%F,%F %F,%F)",
//                                this, visibleRect.min_.x_, visibleRect.min_.y_, visibleRect.max_.x_, visibleRect.max_.y_, (*it)->position_.x_, (*it)->position_.y_, (*it)->rect_.min_.x_, (*it)->rect_.min_.y_, (*it)->rect_.max_.x_, (*it)->rect_.max_.y_);

                freeDrawables_.Push(*it);
                it = drawables.Erase(it);
                continue;
            }

            ++it;
        }
    }

    if (!freeDrawables_.Size())
        return;

    if (visibleRect.min_.y_ > boundcurve_->center_.y_)
        return;

//    if (logtest_)
//        URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... entry(%F,%F %F,%F) bounds(%F,%F %F,%F) ...", this, first.x_, first.y_, last.x_, last.y_, bmin.x_, bmin.y_, bmax.x_, bmax.y_);

    const float quadrantx = bmin.x_ < boundcurve_->center_.x_ ? -1.f : 1.f;
    Vector2 pos;

    // On X Ellipse border Add Drawable
    if (freeDrawables_.Size() && bmax.y_ == boundcurve_->center_.y_)
    {
        if (quadrantx < 0.f && (!drawables.Size() || drawables.Front()->position_.x_ != boundcurve_->GetMinX()))
        {
            DrawableObject& drawable = *freeDrawables_.Back();
            drawable.flipX_ = drawables.Size() % 3;
            drawable.position_.y_ = boundcurve_->center_.y_;
            drawable.angle_.x_ = 0.f;
            drawable.position_.x_ = boundcurve_->GetMinX();
            drawable.angle_.y_ = 1.f;
            SetDrawableVertices(drawable);
            drawables.Insert(0, &drawable);
            freeDrawables_.Pop();
            pos = drawable.position_;
            if (logtest_)
                URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... bounds(%F,%F %F,%F) ... add Xborder- drawable at (%F,%F) ... drawablesSize=%u !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, pos.x_, pos.y_, drawables.Size());
        }
        else if (quadrantx > 0.f && (!drawables.Size() || drawables.Back()->position_.x_ != boundcurve_->GetMaxX()))
        {

            DrawableObject& drawable = *freeDrawables_.Back();
            drawable.flipX_ = drawables.Size() % 3;
            drawable.position_.y_ = boundcurve_->center_.y_;
            drawable.angle_.x_ = 0.f;
            drawable.position_.x_ = boundcurve_->GetMaxX();
            drawable.angle_.y_ = -1.f;
            SetDrawableVertices(drawable);
            drawables.Insert(drawables.Size(), &drawable);
            freeDrawables_.Pop();
            pos = drawable.position_;
            if (logtest_)
                URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... bounds(%F,%F %F,%F) ... add Xborder+ drawable at (%F,%F) ... drawablesSize=%u !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, pos.x_, pos.y_, drawables.Size());
        }
    }

    // Put a first drawable if not exist.
    if (!drawables.Size())
    {
        DrawableObject& drawable = *freeDrawables_.Back();
        drawable.flipX_ = false;
        drawable.position_.x_ = (first.x_ + last.x_) / 2.f - scrollerposition_.x_;

        boundcurve_->GetPositionOn(scrollerposition_.x_, -1.f, drawable.position_, drawable.angle_);

        SetDrawableVertices(drawable);
        if (visibleRect.IsInside(drawable.rect_) == OUTSIDE)
        {
//            URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... bounds(%F,%F %F,%F) ... can't add first drawable at (%F,%F) Outside visiblerect(%F,%F %F,%F) !",
//                            this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, drawable.position_.x_, drawable.position_.y_, visibleRect.min_.x_, visibleRect.min_.y_, visibleRect.max_.x_, visibleRect.max_.y_);
            return;
        }

        drawables.Insert(0, &drawable);
        freeDrawables_.Pop();

        pos = drawable.position_;

        if (logtest_)
            URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... bounds(%F,%F %F,%F) ... add first drawable at (%F,%F) !", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, pos.x_, pos.y_);
    }
    else
    {
        pos = drawables.Front()->position_;
    }

    // Expand to bmin bound
    float w = drawables.Front()->info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawables.Front()->angle_.x_);
    while (freeDrawables_.Size() && pos.x_ > boundcurve_->GetMinX())
    {
        // be sure to have a width to add
        if (w < 0.01f)
            w = 0.01f;
        // maximize the overlapping near X border
        else if (w < 1.f)
            w *= maximizedOverlap;

        DrawableObject& drawable = *freeDrawables_.Back();
        drawable.flipX_ = drawables.Size() % 3;
        drawable.position_.x_ = pos.x_ - w * overlap;

        boundcurve_->GetPositionOn(scrollerposition_.x_, -1.f, drawable.position_, drawable.angle_);
        if (drawable.position_.x_ >= pos.x_)
            break;

        SetDrawableVertices(drawable);
        if (visibleRect.IsInside(drawable.rect_) == OUTSIDE)
            break;

        drawables.Insert(0, &drawable);
        freeDrawables_.Pop();

        if (logtest_)
            URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... bounds(%F,%F %F,%F) ... DirX- add drawable at (%F,%F) w=%F ... drawablesSize=%u", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, drawable.position_.x_, drawable.position_.y_, w, drawables.Size());

        // next drawable position
        pos = drawable.position_;
        w = drawable.info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawable.angle_.x_);
    }

    // Expand to bmax bound
    pos = drawables.Back()->position_;
    w = drawables.Back()->info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawables.Back()->angle_.x_);
    while (freeDrawables_.Size() && pos.x_ < boundcurve_->GetMaxX())
    {
        // be sure to have a width to add
        if (w < 0.01f)
            w = 0.01f;
        // maximize the overlapping near X border
        else if (w < 1.f)
            w *= maximizedOverlap;

        DrawableObject& drawable = *freeDrawables_.Back();
        drawable.flipX_ = drawables.Size() % 3;
        drawable.position_.x_ = pos.x_ + w * overlap;

        boundcurve_->GetPositionOn(scrollerposition_.x_, -1.f, drawable.position_, drawable.angle_);
        if (drawable.position_.x_ <= pos.x_)
            break;

        SetDrawableVertices(drawable);
        if (visibleRect.IsInside(drawable.rect_) == OUTSIDE)
            break;

        drawables.Insert(drawables.Size(), &drawable);
        freeDrawables_.Pop();

        if (logtest_)
            URHO3D_LOGINFOF("UpdateDrawablesOnBottomCurve : this=%u ... bounds(%F,%F %F,%F) ... DirX+ add drawable at (%F,%F) w=%F ... drawablesSize=%u", this, bmin.x_, bmin.y_, bmax.x_, bmax.y_, drawable.position_.x_, drawable.position_.y_, w, drawables.Size());

        // next drawable position
        pos = drawable.position_;
        w = drawable.info_->drawRect_.Size().x_ * scrollerscale_.x_ * Abs(drawable.angle_.x_);
    }
}

void DrawableScroller::HandleUpdateEllipseMode(StringHash eventType, VariantMap& eventData)
{
    if (!SetDrawableObjects())
    {
//        URHO3D_LOGERRORF("DrawableScroller() - HandleUpdateEllipseMode : SetDrawableObjects NOK currentmap=%s !", World2D::GetCurrentMap(viewport_) ? World2D::GetCurrentMap(viewport_)->GetMapPoint().ToString().CString() : "");
        return;
    }

    ScrollerViewInfo& info = scrollerinfos_[viewport_];

    // update camPosition (update just once for all DrawableScrollers)
    if (!info.camPositionCount_ && info.nodeToFollow_)
        info.camPosition_ = info.nodeToFollow_->GetWorldPosition2D();

    if (info.camPosition_ == info.lastCamPosition_ && GameContext::Get().luminosity_ == GameContext::Get().lastluminosity_)
        return;

    const Vector2 camInitialOffset = info.camPosition_ - info.initialPosition_;
    const Vector2 camMotion = info.camPosition_ - info.lastCamPosition_;

    // update lastCamPosition (update just once for all DrawableScrollers)
    info.camPositionCount_++;
    if (info.camPositionCount_ == info.drawableScrollers_.Size())
    {
//        URHO3D_LOGERRORF("DrawableScroller() - HandleUpdateEllipseMode : this=%u Update Camera camPositionCount_=%d", this, info.camPositionCount_);
        info.lastCamPosition_ = info.camPosition_;
        info.camPositionCount_ = 0;
    }

    scrollerposition_.x_ = camInitialOffset.x_ * parallax_.x_;
    if (boundcurve_ && parallax_.y_)
    {
        scrollerposition_.x_ = boundcurve_->ClampX(scrollerposition_.x_);
        scrollerposition_.y_ = (info.camPosition_.y_ - boundcurve_->GetY(boundcurve_->ClampX(info.camPosition_.x_))) * parallax_.y_;

        // Reduce the scroller on the x borders (0.5% of the radius) of the world Ellipse
        const float reducescroller = Clamp(Pow((info.camPosition_.x_ < boundcurve_->center_.x_ ? info.camPosition_.x_ - boundcurve_->GetMinX() : boundcurve_->GetMaxX() - info.camPosition_.x_) / (boundcurve_->radius_.x_ * 0.005f), 3.f) - 0.01f, 0.f, 1.f);
        scrollerposition_.x_ *= reducescroller;
        scrollerposition_.y_ *= reducescroller;
    }

    ShortIntVector2 mpoint = World2D::GetCurrentMapPoint(viewport_);
    Map* map = World2D::GetCurrentMap(viewport_);

    if (!boundcurve_ && parallax_.x_ < 0.1f)
    {
        if (drawables_.Size())
        {
            Rect visibleRect = World2D::GetExtendedVisibleRect(viewport_);

            DrawableObject* leftdrawable = drawables_.Front();
            DrawableObject* rightdrawable = drawables_.Back();

            // camera move to right : check right drawable go inside visiblerect => get left drawable and put it at right, change the sprite considering the current map
            if (camMotion.x_ > 0.f)
            {
                if (rightdrawable->info_->drawRect_.max_.x_ + rightdrawable->position_.x_ + scrollerposition_.x_ - drawableoverlap_ <= visibleRect.max_.x_)
                {
                    leftdrawable->position_.x_ = rightdrawable->position_.x_ + drawableSize_.x_ - drawableoverlap_;
                    float y = GetNewYAtX(viewport_, scrollerposition_.x_ + leftdrawable->position_.x_, mpoint, map);
                    DrawableObjectInfo* dinfo = map ? GetWorldScrollerObjectInfo(map->GetMapPoint(), 0) : 0;
                    leftdrawable->enable_ = dinfo && y > -1.f;
                    if (leftdrawable->enable_)
                    {
                        leftdrawable->info_ = dinfo;
                        leftdrawable->position_.y_ = scrolleroffset_.y_ + map->GetTopography().maporigin_.y_ + y;
                    }

                    // "Left" Drawable becomes the "Right"
                    drawables_.Erase(0U);
                    drawables_.Push(leftdrawable);
                }
            }

            // camera move to left : check left drawable go inside visiblerect => get right drawable and put it at left, change the sprite considering the current map
            else if (camMotion.x_ < 0.f)
            {
                if (leftdrawable->info_->drawRect_.min_.x_ + leftdrawable->position_.x_ + scrollerposition_.x_ + drawableoverlap_ >= visibleRect.min_.x_)
                {
                    rightdrawable->position_.x_ = leftdrawable->position_.x_ - drawableSize_.x_ + drawableoverlap_;
                    float y = GetNewYAtX(viewport_, scrollerposition_.x_ + rightdrawable->position_.x_, mpoint, map);
                    DrawableObjectInfo* dinfo = map ? GetWorldScrollerObjectInfo(map->GetMapPoint(), 0) : 0;
                    rightdrawable->enable_ = dinfo && (y > -1.f);
                    if (rightdrawable->enable_)
                    {
                        rightdrawable->info_ = dinfo;
                        rightdrawable->position_.y_ = scrolleroffset_.y_ + map->GetTopography().maporigin_.y_ + y;
                    }

                    // "Right" Drawable becomes the "Left"
                    drawables_.Pop();
                    drawables_.Insert(0, rightdrawable);
                }
            }
        }

        // if motion in y, check for changing map
        if (camMotion.y_)
        {
            map = World2D::GetCurrentMap(viewport_);
            mpoint = World2D::GetCurrentMapPoint(viewport_);

            float y;
            for (Vector<DrawableObject*>::Iterator it=drawables_.Begin(); it!=drawables_.End(); ++it)
            {
                if (!map)
                {
                    map = World2D::GetCurrentMap(viewport_);
                    mpoint = World2D::GetCurrentMapPoint(viewport_);
                }

                DrawableObject& drawable = **it;

                // skip if already enable
                if (drawable.enable_)
                    continue;

                y = GetNewYAtX(viewport_, scrollerposition_.x_ + drawable.position_.x_, mpoint, map);

                // skip if in same map coord y
                if (drawable.info_ && drawable.info_->zone_.y_ == mpoint.y_)
                    continue;

                DrawableObjectInfo* dinfo = map ? GetWorldScrollerObjectInfo(map->GetMapPoint(), 0) : 0;
                drawable.enable_ = dinfo && y > -1.f && y < MapInfo::info.mHeight_;
                if (drawable.enable_)
                {
                    drawable.info_ = dinfo;
                    drawable.position_.y_ = scrolleroffset_.y_ + map->GetTopography().maporigin_.y_ + y;
                }
            }
        }
    }
    else if (boundcurve_ && rotationFollowCurve_)
    {
        if (!map || map->GetTopography().IsFullGround())
        {
            ClearDrawables();
        }
        else
        {
            Rect visibleRect = World2D::GetExtendedVisibleRect(viewport_);

            // enlarge the visibleRect in Y before doing the intersection with the boundcurve.
            if (visibleRect.max_.y_ > boundcurve_->center_.y_)
                visibleRect.max_.y_ += scrolleroffset_.y_;
            if (visibleRect.min_.y_ < boundcurve_->center_.y_)
                visibleRect.min_.y_ -= scrolleroffset_.y_;
            visibleRect.max_.y_ += (-scrollerposition_.y_ + Max(drawableSize_.x_, drawableSize_.y_));
            visibleRect.min_.y_ += (-scrollerposition_.y_ - Max(drawableSize_.x_, drawableSize_.y_));

            Vector<Vector2> intersections;
            boundcurve_->GetIntersections(visibleRect, intersections, true);

            if (intersections.Size() > 1)
            {
                // no drawables : regenerate drawables list at bounds.min_
                if (drawablesTopCurve_.Size() + drawablesBottomCurve_.Size() == 0 && lastDrawablePositionAtLeft_ > intersections.Front().x_ - scrollerposition_.x_ - scrolleroffset_.x_)
                {
                    lastDrawablePositionAtLeft_ = intersections.Front().x_ - scrollerposition_.x_ - scrolleroffset_.x_;
//                    if (logtest_)
//                        URHO3D_LOGINFOF("RemoveDrawablesOutsideBounds this=%u ... new lastDrawablePositionAtLeft_=%F ...", this, lastDrawablePositionAtLeft_);
                }

                if (lastDrawablePositionAtLeft_ + scrollerposition_.x_ > intersections.Back().x_)
                {
                    // the drawables are all outside on the right
                    ClearDrawables();
                    if (logtest_)
                        URHO3D_LOGINFOF("RemoveDrawablesOutsideBounds this=%u ... lastDrawablePositionAtLeft_=%F + ScrollerX=%F > boundRight=%F => Clear All Drawables ...", this, lastDrawablePositionAtLeft_, scrollerposition_.x_, intersections.Back().x_);
                }

                UpdateDrawablesOnTopCurve(intersections.Front(), intersections.Back());

                UpdateDrawablesOnBottomCurve(intersections.Front(), intersections.Back());

                drawables_ = drawablesTopCurve_;
                drawables_ += drawablesBottomCurve_;

                // Update the new lastDrawablePositionAtLeft_
                if (drawablesTopCurve_.Size() && drawablesTopCurve_.Front()->position_.x_ < lastDrawablePositionAtLeft_)
                {
                    lastDrawablePositionAtLeft_ = drawablesTopCurve_.Front()->position_.x_;
//                    if (logtest_)
//                        URHO3D_LOGINFOF(" this=%u ... HandleUpdateEllipseMode : new1 lastDrawablePositionAtLeft_=%F ...", this, lastDrawablePositionAtLeft_);
                }
                if (drawablesBottomCurve_.Size() && (!drawablesTopCurve_.Size() || drawablesBottomCurve_.Front()->position_.x_ < lastDrawablePositionAtLeft_))
                {
                    lastDrawablePositionAtLeft_ = drawablesBottomCurve_.Front()->position_.x_;
//                    if (logtest_)
//                        URHO3D_LOGINFOF(" this=%u ... HandleUpdateEllipseMode : new2 lastDrawablePositionAtLeft_=%F ...", this, lastDrawablePositionAtLeft_);
                }
            }
            else if (drawables_.Size())
            {
                ClearDrawables();
                if (logtest_)
                    URHO3D_LOGINFOF(" this=%u ... Outside => Clear Drawables ...", this);
            }
//            else
//            {
//                if (logtest_)
//                    URHO3D_LOGINFOF(" this=%u ... Outside => no intersection and no Drawables ...", this);
//            }
        }
    }
    else
    {
        // Wrap Drawables on visible border.

        if (drawables_.Size() > 2)
        {
            Rect visibleRect = World2D::GetExtendedVisibleRect(viewport_);

            DrawableObject* leftdrawable = drawables_.Front();
            DrawableObject* rightdrawable = drawables_.Back();

            // camera move to right : check if right drawable is all inside visiblerect => get left drawable and put it at right, change the sprite considering the current map
            if (camMotion.x_ > 0.f)
            {
                const float drawablewidth = rightdrawable->info_->drawRect_.Size().x_ * scrollerscale_.x_;
                if (scrollerposition_.x_ + rightdrawable->position_.x_ + drawablewidth * 0.5f <= visibleRect.max_.x_)
                {
                    leftdrawable->position_.x_ = rightdrawable->position_.x_ + drawablewidth * 0.75f;
                    float y = GetNewYAtX(viewport_, scrollerposition_.x_ + leftdrawable->position_.x_, mpoint, map);
                    DrawableObjectInfo* dinfo = map ? GetWorldScrollerObjectInfo(map->GetMapPoint(), 0) : 0;
                    leftdrawable->enable_ = dinfo && y > -1.f;
                    if (leftdrawable->enable_)
                    {
                        leftdrawable->info_ = dinfo;
                        leftdrawable->position_.y_ = scrolleroffset_.y_ + map->GetTopography().maporigin_.y_ + y;
                    }

                    // "Left" Drawable becomes the "Right"
                    drawables_.Erase(0U);
                    drawables_.Push(leftdrawable);
                }
            }
            // camera move to left : check if left drawable is all inside the visiblerect => get right drawable and put it at left, change the sprite considering the current map
            else if (camMotion.x_ < 0.f)
            {
                const float drawablewidth = leftdrawable->info_->drawRect_.Size().x_ * scrollerscale_.x_;
                if (scrollerposition_.x_ + leftdrawable->position_.x_ - drawablewidth * 0.5f >= visibleRect.min_.x_)
                {
                    rightdrawable->position_.x_ = leftdrawable->position_.x_ - drawablewidth * 0.75f;
                    float y = GetNewYAtX(viewport_, scrollerposition_.x_ + rightdrawable->position_.x_, mpoint, map);
                    DrawableObjectInfo* dinfo = map ? GetWorldScrollerObjectInfo(map->GetMapPoint(), 0) : 0;
                    rightdrawable->enable_ = dinfo && (y > -1.f);
                    if (rightdrawable->enable_)
                    {
                        rightdrawable->info_ = dinfo;
                        rightdrawable->position_.y_ = scrolleroffset_.y_ + map->GetTopography().maporigin_.y_ + y;
                    }

                    // "Right" Drawable becomes the "Left"
                    drawables_.Pop();
                    drawables_.Insert(0U, rightdrawable);
                }
            }
        }

        // update drawables positions and visibility

        // if boundcurve is setted
        if (boundcurve_)
        {
            bool visibility = map ? !map->GetTopography().IsFullGround() : false;

            if (!visibility)
            {
                for (Vector<DrawableObject*>::Iterator it=drawables_.Begin(); it!=drawables_.End(); ++it)
                    (*it)->enable_ = false;
            }
            else
            {
                unsigned i = 0;
                for (Vector<DrawableObject*>::Iterator it=drawables_.Begin(); it!=drawables_.End(); ++it, i++)
                {
                    DrawableObject& drawable = **it;
                    drawable.enable_ = boundcurve_->IsInBounds(scrollerposition_.x_ + drawable.position_.x_);
                    if (drawable.enable_)
                        drawable.position_.y_ = scrolleroffset_.y_ + boundcurve_->GetY(scrollerposition_.x_ + drawable.position_.x_);

                    if (logtest_)
                        URHO3D_LOGINFOF(" ... drawableObjects[%u] position=%s visible=%s...", i, drawable.position_.ToString().CString(), drawable.enable_ ? "true":"false");
                }
            }
        }
        // else if have parallax in x
        else
        {
            for (Vector<DrawableObject*>::Iterator it=drawables_.Begin(); it!=drawables_.End(); ++it)
            {
                if (!map)
                {
                    map = World2D::GetCurrentMap(viewport_);
                    mpoint = World2D::GetCurrentMapPoint(viewport_);
                }

                DrawableObject& drawable = **it;
                drawable.position_.y_ = scrolleroffset_.y_ + GetNewYAtX(viewport_, scrollerposition_.x_ + drawable.position_.x_, mpoint, map);
                if (map)
                {
                    drawable.position_.y_ += map->GetTopography().maporigin_.y_;
                    drawable.enable_ = map->IsEffectiveVisible();
                }
                else
                    drawable.enable_ = false;
            }
        }
    }

    sourceBatchesDirty_ = true;
}

void DrawableScroller::HandleUpdateFlatMode(StringHash eventType, VariantMap& eventData)
{
    ScrollerViewInfo& info = scrollerinfos_[viewport_];

    if (!SetDrawableObjects())
    {
//        if (logtest_)
//            URHO3D_LOGERRORF("DrawableScroller() - HandleUpdateFlatMode : SetDrawableObjects NOK currentmap=%s !", World2D::GetCurrentMap(viewport_) ? World2D::GetCurrentMap(viewport_)->GetMapPoint().ToString().CString() : "none");
        return;
    }

    // update camPosition (update just once for all DrawableScrollers)
    if (!info.camPositionCount_ && info.nodeToFollow_)
        info.camPosition_ = info.nodeToFollow_->GetWorldPosition2D();

    if (info.camPosition_ == info.lastCamPosition_ && GameContext::Get().luminosity_ == GameContext::Get().lastluminosity_)
    {
//        if (logtest_)
//            URHO3D_LOGERRORF("DrawableScroller() - HandleUpdateFlatMode : SetDrawableObjects same positions and luminosity => skip !");
        return;
    }

    Vector2 deltamotion = info.camPosition_ - info.lastCamPosition_;

    // update lastCamPosition (update just once for all DrawableScrollers)
    info.camPositionCount_++;
    if (info.camPositionCount_ == info.drawableScrollers_.Size())
    {
        info.lastCamPosition_ = info.camPosition_;
        info.camPositionCount_ = 0;
    }

    scrollerposition_ = (info.camPosition_- info.initialPosition_) * parallax_;
    if (motionBounds_.x_)
        scrollerposition_.x_ = Clamp(scrollerposition_.x_, -motionBounds_.x_, motionBounds_.x_);

    Map* map = World2D::GetCurrentMap(viewport_);

//    if (boundcurve_)
//        scrollerposition_.y_ += (float)(100 - World2D::GetWorldInfo()->simpleGroundLevel_ + 8) * World2D::GetWorldInfo()->mHeight_ / 100.f;
//    else
//        scrollerposition_.y_ = map->GetTopography().maporigin_.y_ + Clamp(scrollerposition_.y_, 0.f, motionBounds_.y_);

    if (boundcurve_)
    {
        scrollerposition_.y_ = (info.camPosition_.y_ - boundcurve_->GetY(boundcurve_->ClampX(info.camPosition_.x_))) * parallax_.y_;
    }

//    if (logtest_)
//        URHO3D_LOGINFOF("DrawableScroller() - HandleUpdateFlatMode : this=%u scrollerposition_=%s", this, scrollerposition_.ToString().CString());

    const Rect& visibleRect = World2D::GetExtendedVisibleRect(viewport_);

    DrawableObject* leftdrawable = drawables_.Front();
    DrawableObject* rightdrawable = drawables_.Back();

    // camera move to right : check right drawable go inside visiblerect => get left drawable and put it at right, change the sprite considering the current map
    if (deltamotion.x_ > 0.f)
    {
        if (rightdrawable->info_->drawRect_.max_.x_ + rightdrawable->position_.x_ + scrollerposition_.x_ - drawableoverlap_ <= visibleRect.max_.x_)
        {
            leftdrawable->position_.x_ = rightdrawable->position_.x_ + drawableSize_.x_ - drawableoverlap_;
            DrawableObjectInfo* dinfo = GetWorldScrollerObjectInfo(World2D::GetCurrentMapPoint(viewport_), 0);
            leftdrawable->enable_ = dinfo != 0;
            if (leftdrawable->enable_)
            {
                leftdrawable->info_ = dinfo;
                leftdrawable->position_.y_ = scrolleroffset_.y_;
                if (!boundcurve_)
                    leftdrawable->position_.y_ += map->GetTopography().GetY(scrollerposition_.x_+leftdrawable->position_.x_);
                leftdrawable->enable_ = map->IsInYBound(leftdrawable->position_.y_);
            }

            // "Left" Drawable becomes the "Right"
            drawables_.Erase(0U);
            drawables_.Push(leftdrawable);
        }
    }
    // camera move to left : check left drawable go inside visiblerect => get right drawable and put it at left, change the sprite considering the current map
    else if (deltamotion.x_ < 0.f)
    {
        if (leftdrawable->info_->drawRect_.min_.x_ + leftdrawable->position_.x_ + scrollerposition_.x_ + drawableoverlap_ >= visibleRect.min_.x_)
        {
            rightdrawable->position_.x_ = leftdrawable->position_.x_ - drawableSize_.x_ + drawableoverlap_;
            DrawableObjectInfo* dinfo = GetWorldScrollerObjectInfo(World2D::GetCurrentMapPoint(viewport_), 0);
            rightdrawable->enable_ = (dinfo != 0);
            if (rightdrawable->enable_)
            {
                rightdrawable->info_ = dinfo;
                rightdrawable->position_.y_ = scrolleroffset_.y_;
                if (!boundcurve_)
                    rightdrawable->position_.y_ += map->GetTopography().GetY(scrollerposition_.x_+rightdrawable->position_.x_);
                rightdrawable->enable_ = map->IsInYBound(rightdrawable->position_.y_);
            }

            // "Right" Drawable becomes the "Left"
            drawables_.Pop();
            drawables_.Insert(0U, rightdrawable);
        }
    }

    sourceBatchesDirty_ = true;
}

void DrawableScroller::HandleCameraPositionChanged(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGERRORF("DrawableScroller() - HandleCameraPositionChanged : on viewport ...", viewport_);

    ScrollerViewInfo& info = scrollerinfos_[viewport_];

    info.camPositionCount_ = 0;

    unsigned numscrollers = info.drawableScrollers_.Size();

    for (unsigned i = 0; i < numscrollers; ++i)
        info.drawableScrollers_[i]->drawablePool_.Clear();

//    logtest_ = false;

    if (flatmode_)
    {
        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->HandleUpdateFlatMode(eventType, eventData);
    }
    else
    {
        for (unsigned i = 0; i < numscrollers; ++i)
            info.drawableScrollers_[i]->HandleUpdateEllipseMode(eventType, eventData);
    }

//    logtest_ = false;

    URHO3D_LOGERRORF("DrawableScroller() - HandleCameraPositionChanged : viewport=%d ... this=%u camerapos=%s!", viewport_, this, info.camPosition_.ToString().CString());
}
