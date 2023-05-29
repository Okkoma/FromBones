#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Urho2D/Renderer2D.h>

#include "DefsMap.h"

#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameHelpers.h"
#include "ViewManager.h"
#include "MapWorld.h"
#include "Map.h"
#include "RenderShape.h"

#define EMBOSE_TRIANGLE

RenderShape::RenderShape(Context* context) :
    Drawable2D(context),
    collider_(0),
    materialDirty_(true),
    hasMapBorder_(false),
    isDynamic_(false),
    embose_(0.f)
{ }

RenderShape::~RenderShape()
{ }

void RenderShape::RegisterObject(Context* context)
{
    context->RegisterFactory<RenderShape>();
}

void RenderShape::AddBatch(const ShapeBatchInfo& batchinfo)
{
    unsigned ibatch = batchinfos_.Size();

    batchinfos_.Push(batchinfo);

    if (sourceBatches_[0].Size() != batchinfos_.Size())
        sourceBatches_[0].Resize(batchinfos_.Size());

    sourceBatches_[0][ibatch].owner_ = this;
#ifndef EMBOSE_TRIANGLE
    sourceBatches_[0][ibatch].quadvertices_ = batchinfo.type_ == BORDERBATCH || batchinfo.type_ == EMBOSEBATCH;
#else
    sourceBatches_[0][ibatch].quadvertices_ = batchinfo.type_ == BORDERBATCH;
#endif

    UpdateMaterials();

//    URHO3D_LOGERRORF("RenderShape() - AddBatch : ibatch=%d, material=%s !", ibatch, batchinfo.material_->GetName().CString());

    SetDirty(ibatch);
}

void RenderShape::AddShape(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> > *holesptr)
{
    if (hasMapBorder_ && !mapBorder_.Defined())
        mapBorder_ = Rect(0.01f, 0.01f, (float)MapInfo::info.width_ * MapInfo::info.tileWidth_ - 0.01f, (float)MapInfo::info.height_ * MapInfo::info.tileHeight_ - 0.01f);

    shapes_.Resize(shapes_.Size()+1);

    PolyShape& shape = shapes_.Back();
    shape.SetEmboseParameters(embose_, collider_, hasMapBorder_ ? &mapBorder_: 0);
    shape.AddContour(contour, holesptr);
    shape.Triangulate();

//    URHO3D_LOGERRORF("RenderShape() - AddShape : numVertices=%u numEmboseVertices=%u !", shape.triangles_.Size(), shape.emboses_.Size());

    for (unsigned i=0; i < batchinfos_.Size(); i++)
        SetDirty(i);

    worldBoundingBoxDirty_ = true;

    MarkNetworkUpdate();
}

void RenderShape::SetDirty(unsigned ibatch, bool state)
{
    sourceBatchesDirty_ = state;

    batchinfos_[ibatch].dirty_ = state;

    if (batchinfos_[ibatch].type_ == BORDERBATCH || batchinfos_[ibatch].type_ == EMBOSEBATCH)
        batchinfos_[ibatch].mapBorderDirty_ = state;

    if (state == false)
    {
        unsigned countdirty = 0;
        for (unsigned i=0; i < batchinfos_.Size(); i++)
        {
            if (batchinfos_[i].dirty_)
                countdirty++;
        }

        sourceBatchesDirty_ = (countdirty > 0);
    }

//    URHO3D_LOGINFOF("RenderShape() - SetDirty : ibatch=%u dirty=%s sourceBatchesDirty=%s !",
//                    ibatch, state ? "true" : "false", sourceBatchesDirty_ ? "true" : "false");

    MarkNetworkUpdate();
}

void RenderShape::SetMapBorderDirty(unsigned ibatch)
{
    if (ibatch >= batchinfos_.Size())
        return ;

    sourceBatchesDirty_ = true;
    batchinfos_[ibatch].mapBorderDirty_ = true;

    URHO3D_LOGINFOF("RenderShape() - SetMapBorderDirty : ibatch=%u !", ibatch);
}


void RenderShape::SetTextureRepeat(unsigned ibatch, const Vector2& repeat)
{
    if (repeat == batchinfos_[ibatch].textureRepeat_)
        return;

    batchinfos_[ibatch].textureRepeat_ = repeat;

    SetDirty(ibatch);
}

void RenderShape::SetColor(unsigned ibatch, const Color& color)
{
    if (color == batchinfos_[ibatch].color_)
        return;

    batchinfos_[ibatch].color_ = color;

    SetDirty(ibatch);
}

void RenderShape::SetAlpha(unsigned ibatch, float alpha)
{
    if (alpha == batchinfos_[ibatch].color_.a_)
        return;

    batchinfos_[ibatch].color_.a_ = alpha;

    SetDirty(ibatch);
}

void RenderShape::Clear()
{
    shapes_.Clear();

    ClearSourceBatches();
}

void RenderShape::SetCollider(MapCollider* collider)
{
    collider_ = collider;

    for (unsigned i = 0; i < collider_->contourVertices_.Size(); i++)
        AddShape(collider_->contourVertices_[i], &collider_->holeVertices_[i]);

    MarkDirty();

//    URHO3D_LOGINFOF("RenderShape() - SetCollider : collider=%u !", collider);
}

void RenderShape::SetEmboseOffset(float offset)
{
    embose_ = offset;
}

void RenderShape::SetMapBorder(bool state)
{
    hasMapBorder_ = state;

    if (state)
        mapBorder_ = Rect(0.01f, 0.01f, (float)MapInfo::info.width_ * MapInfo::info.tileWidth_ - 0.01f, (float)MapInfo::info.height_ * MapInfo::info.tileHeight_ - 0.01f);
}

/*
void RenderShape::OnWorldBoundingBoxUpdate()
{
    Camera* camera = ViewManager::Get()->GetCamera(0);
    const Vector<SourceBatch2D*>& batches = GetSourceBatchesToRender(camera);

    if (!batches.Size())
        return;

    const Vector<Vertex2D>& vertices = batches.Back()->vertices_;

    if (vertices.Empty())
        return;

    boundingBox_.Clear();
    worldBoundingBox_.Clear();

    for (unsigned i = 0; i < vertices.Size(); ++i)
        worldBoundingBox_.Merge(vertices[i].position_);
}
*/

void RenderShape::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_.Clear();

    const Vector2 embose = hasMapBorder_ && embose_ ? node_->GetWorldScale2D() * embose_ : Vector2::ZERO;

    for (unsigned i=0; i < shapes_.Size(); ++i)
    {
        PolyShape& shape = shapes_[i];
        shape.UpdateWorldBoundingRect(node_->GetWorldTransform2D());

        worldBoundingBox_.Merge(BoundingBox(shape.worldBoundingRect_.min_ - embose, shape.worldBoundingRect_.max_ + embose));
    }
}

void RenderShape::OnDrawOrderChanged()
{
    int drawOrder = GetDrawOrder();
    for (unsigned i=0; i < sourceBatches_[0].Size(); i++)
        sourceBatches_[0][i].drawOrder_ = drawOrder;// + i;

//    URHO3D_LOGINFOF("RenderShape() - OnDrawOrderChanged : ptr=%u draworder=%u layer=%d order=%d !", this, drawOrder, layer_.x_, orderInLayer_);

    sourceBatchesDirty_ = true;
}

void RenderShape::OnMarkedDirty(Node* node)
{
//    URHO3D_LOGINFOF("RenderShape() - OnMarkedDirty : ptr=%u ...", this);

    sourceBatchesDirty_ = worldBoundingBoxDirty_ = true;

    for (unsigned i=0; i < batchinfos_.Size(); i++)
        SetDirty(i);

//    URHO3D_LOGINFOF("RenderShape() - OnMarkedDirty : ptr=%u ... draworder=%d ! ", this, sourceBatches_[0].Size() ? sourceBatches_[0][0].drawOrder_ : 0);
}

void RenderShape::OnSetEnabled()
{
    bool enabled = IsEnabledEffective();

    if (enabled)
    {
        sourceBatchesDirty_ = visibility_ = true;

        if (renderer_)
        {
            renderer_->AddDrawable(this);
            if (isDynamic_)
                node_->AddListener(this);
        }

#ifdef ACTIVE_RENDERSHAPE_CLIPPING
        SubscribeToEvent(GetScene(), E_SCENEUPDATE, URHO3D_HANDLER(RenderShape, HandleUpdate));
        SubscribeToEvent(ViewManager::Get(), VIEWMANAGER_SWITCHVIEW, URHO3D_HANDLER(RenderShape, HandleUpdate));
#endif

//        if (collider_ && collider_->map_)
//            URHO3D_LOGINFOF("RenderShape() - OnSetEnabled : enabled=true Map=%s colliderZ=%d NumSourceBatches=%u!",
//                        collider_->map_->GetMapPoint().ToString().CString(), collider_->params_->colliderz_, GetSourceBatchesToRender().Size());
    }
    else
    {
        if (renderer_)
        {
            renderer_->RemoveDrawable(this);
            node_->RemoveListener(this);
        }

#ifdef ACTIVE_RENDERSHAPE_CLIPPING
        UnsubscribeFromAllEvents();
#endif

        sourceBatchesDirty_ = visibility_ = false;
    }
}

void RenderShape::UpdateMaterials()
{
    if (!renderer_)
        return;

    if (batchinfos_.Size())
    {
//        URHO3D_LOGINFOF("RenderShape() - UpdateMaterials : ptr=%u ... ", this);

        unsigned numbatches = batchinfos_.Size();

        if (sourceBatches_[0].Size() != numbatches)
            sourceBatches_[0].Resize(numbatches);

        for (unsigned i=0; i < numbatches; i++)
            sourceBatches_[0][i].material_ = batchinfos_[i].material_;

        OnDrawOrderChanged();

        for (unsigned i=0; i < numbatches; i++)
            SetDirty(i);
    }

    materialDirty_ = false;
}

void RenderShape::UpdateFrameVertices(unsigned ibatch)
{
    if (ibatch >= sourceBatches_[0].Size())
        return;

    ShapeBatchInfo& batchinfo = batchinfos_[ibatch];
    if (!batchinfo.dirty_)
        return;

    unsigned numtotalvertices = 0;

    Vector<PolyShape>& shapes = shapes_;

    for (unsigned i=0; i < shapes.Size(); ++i)
        numtotalvertices += shapes[i].triangles_.Size();

    if (!numtotalvertices)
    {
//        URHO3D_LOGWARNINGF("RenderShape() - UpdateFrameVertices : %s(%u) ptr=%u ... No Triangles !", GetNode()->GetName().CString(), GetNode()->GetID(), this);
        return;
    }

    const Vector2& textureSize = batchinfo.textureSize_;
    const Vector2& textureRepeat = batchinfo.textureRepeat_;

    Vector<Vertex2D>& vertices = sourceBatches_[0][ibatch].vertices_;
    vertices.Resize(numtotalvertices);

    unsigned startvertex = 0;
    unsigned color = batchinfo.color_.ToUInt();

#ifdef URHO3D_VULKAN
    unsigned texmode = 0;
#else
    Vector4 texmode;
#endif
    SetTextureMode(TXM_UNIT, batchinfo.textureId_, texmode);
    SetTextureMode(TXM_FX, textureFX_, texmode);

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    Vector2 baseUV;

    if (isDynamic_)
    {
        for (unsigned i=0; i < shapes.Size(); ++i)
        {
            const PODVector<Vector2>& triangles = shapes[i].triangles_;
            const unsigned numvertices = triangles.Size();
            if (!numvertices)
                continue;

            for (unsigned j = 0 ; j < numvertices; ++j)
            {
                Vertex2D& vertex = vertices[startvertex + j];
                vertex.position_ = worldTransform * triangles[j];
                // triangle barycentric value for "wireframe" shader - the value can be : 0 => vec2(0,0) or 1 => vec2(1,0) or 2 => vec2(0,1)
#ifdef URHO3D_VULKAN
                vertex.custom1_ = j % 3;
#else
                // TODO : Remove this barycentric in z an put in custom1
                //vertex.position_.z_ = j % 3;
                vertex.position_.z_ = 0.f;
#endif
            }

#ifdef ACTIVE_REDUCE_TEXTURE_UV
            // Get the base UV to reduce UV Coord (because problem with Same GL implementation like Android when big UV Coord)
            {
                float u = vertices[startvertex].position_.x_;
                float v = vertices[startvertex].position_.y_;

                for (unsigned j = 1; j < numvertices; ++j)
                {
                    const Vertex2D& vertex = vertices[startvertex + j];
                    if (vertex.position_.x_ < u)
                        u = vertex.position_.x_;
                    if (vertex.position_.y_ < v)
                        v = vertex.position_.y_;
                }

                baseUV.x_ = Floor(textureRepeat.x_ * u / textureSize.x_);
                baseUV.y_ = Floor(textureRepeat.y_ * v / textureSize.y_);
            }
#endif

            // Set the vertices UV and fx
            for (unsigned j = 0; j < numvertices; ++j)
            {
                Vertex2D& vertex = vertices[startvertex + j];
                vertex.uv_.x_ = textureRepeat.x_ * vertex.position_.x_ / textureSize.x_ - baseUV.x_;
                vertex.uv_.y_ = textureRepeat.y_ * vertex.position_.y_ / textureSize.y_ - baseUV.y_;
                vertex.color_ = color;
                vertex.texmode_ = texmode;
            }

            startvertex += numvertices;
        }
    }
    else
    {
        for (unsigned i=0; i < shapes.Size(); ++i)
        {
            const PODVector<Vector2>& triangles = shapes[i].triangles_;

            const unsigned numvertices = triangles.Size();
            if (!numvertices)
                continue;

            // Set the vertices positions
            for (unsigned j = 0; j < numvertices; ++j)
            {
                Vertex2D& vertex = vertices[startvertex + j];
                vertex.position_ = worldTransform * triangles[j];
#ifdef URHO3D_VULKAN
                vertex.custom1_ = j % 3;
#else
                // TODO : Remove this barycentric in z and put in custom1
                //vertex.position_.z_ = j % 3;
                vertex.position_.z_ = 0.f;
#endif
            }

#ifdef ACTIVE_REDUCE_TEXTURE_UV
            {
                float u = vertices[startvertex].position_.x_;
                float v = vertices[startvertex].position_.y_;

                for (unsigned j = 1; j < numvertices; ++j)
                {
                    const Vertex2D& vertex = vertices[startvertex + j];
                    if (vertex.position_.x_ < u)
                        u = vertex.position_.x_;
                    if (vertex.position_.y_ < v)
                        v = vertex.position_.y_;
                }

                baseUV.x_ = Floor(textureRepeat.x_ * u / textureSize.x_);
                baseUV.y_ = Floor(textureRepeat.y_ * v / textureSize.y_);
            }
#endif

            // Set the vertices UV and fx
            for (unsigned j = 0; j < numvertices; ++j)
            {
                Vertex2D& vertex = vertices[startvertex + j];
                vertex.uv_.x_ = textureRepeat.x_ * vertex.position_.x_ / textureSize.x_ - baseUV.x_;
                vertex.uv_.y_ = textureRepeat.y_ * vertex.position_.y_ / textureSize.y_ - baseUV.y_;
                vertex.color_ = color;
                vertex.texmode_ = texmode;
            }

#ifdef ACTIVE_BARYCENTRICEFFECTS_TEST
            //URHO3D_LOGERRORF("pass2 : start with %u vertices => check the %f triangles", numvertices, numvertices / 3.f);
            // TEST pass 2 : set barycentric value for each vertex of the triangles
            unsigned numtriangles = numvertices / 3;
            for (int j=0; j < numtriangles; ++j)
            {
                float& baryv0 = vertices[startvertex+j*3].position_.z_;
                float& baryv1 = vertices[startvertex+j*3+1].position_.z_;
                float& baryv2 = vertices[startvertex+j*3+2].position_.z_;

                // default barycentric value
                baryv0 = 1.f;
                baryv1 = 100.f;
                baryv2 = 10000.f;

#ifdef ACTIVE_BARYCENTRICEFFECTS_TEST_IDENTIFY_ADJACENTEDGES
                // for each other triangle, check if it has an adjacent edge (2 same vertices) with the current triangle

                static bool adjacentEdge[3];
                for (int v=0; v < 3; v++)
                    adjacentEdge[v] = false;

                for (int k=0; k < numtriangles; ++k)
                {
                    if (k == j)
                        continue;

                    int numCommonVertex = 0;
                    static bool sameVertex[3];
                    for (int v=0; v < 3; v++)
                        sameVertex[v] = false;
                    for (int v=0; v < 3; v++)
                    {
                        for (int w=0; w < 3; w++)
                            if (triangles[j*3+v] == triangles[k*3+w])
                            {
                                numCommonVertex++;
                                sameVertex[v] = true;
                                break;
                            }
                    }

                    if (numCommonVertex == 2)
                    {
                        if (sameVertex[0] && sameVertex[1])
                            adjacentEdge[0] = true;
                        else if (sameVertex[1] && sameVertex[2])
                            adjacentEdge[1] = true;
                        else if (sameVertex[2] && sameVertex[0])
                            adjacentEdge[2] = true;
                    }
                }

                // has edge 0<->1
                if (adjacentEdge[0])
                {
                    const float Q2 = 990000.f;
                    baryv0 += Q2;
                    baryv1 += Q2;
                }
                // has edge 1<->2
                if (adjacentEdge[1])
                {
                    const float Q0 = 99.f;
                    baryv1 += Q0;
                    baryv2 += Q0;
                }
                // has edge 2<->0
                if (adjacentEdge[2])
                {
                    const float Q1 = 9900.f;
                    baryv2 += Q1;
                    baryv0 += Q1;
                }
                //URHO3D_LOGERRORF(" .... triangle=%u/%u adj01=%u adj12=%u adj20=%u bary=%f,%f,%f", j+1, numtriangles, adjacentEdge[0], adjacentEdge[1], adjacentEdge[2], baryv0, baryv1, baryv2);

#endif
            }
#endif

            startvertex += numvertices;
        }
    }
}

inline void PushQuad4Vertices(const Vector2& position, Sprite2D* sprite, const Matrix2x3& worldTransform, Vector<Vertex2D>& vertices, Vertex2D* vertex)
{
    const Rect& textureRect = sprite->GetFixedTextRectangle();
    const Rect& drawRect = sprite->GetFixedDrawRectangle();

    vertex[0].position_ = worldTransform * (position + drawRect.min_);
    vertex[1].position_ = worldTransform * (position + Vector2(drawRect.min_.x_, drawRect.max_.y_));
    vertex[2].position_ = worldTransform * (position + drawRect.max_);
    vertex[3].position_ = worldTransform * (position + Vector2(drawRect.max_.x_, drawRect.min_.y_));

    vertex[0].uv_ = textureRect.min_;
    vertex[1].uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
    vertex[2].uv_ = textureRect.max_;
    vertex[3].uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

    vertices.Push(vertex[0]);
    vertices.Push(vertex[1]);
    vertices.Push(vertex[2]);
    vertices.Push(vertex[3]);
}

void RenderShape::AddBorderOnMapBorder(unsigned ibatch, int side)
{
    int colliderZ = collider_->params_->colliderz_;

    Map* map = static_cast<Map*>(collider_->map_);
    if (!map)
        return;

    Vector<Vertex2D>& vertices = sourceBatches_[0][ibatch].vertices_;
    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TerrainAtlas* atlas = World2D::GetWorldInfo()->atlas_;
    const MapTerrain& terrain = atlas->GetTerrain(batchinfos_[ibatch].terrainid_);

    unsigned color = batchinfos_[ibatch].color_.ToUInt();
//    unsigned color = Color::RED.ToUInt();

    Vertex2D vertex[4];
    vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = color;
    Vector2 tilesize(World2D::GetWorldInfo()->tileWidth_, World2D::GetWorldInfo()->tileHeight_);
    Vector2 position, stride;
    int numtiles, sideoffset1, sideoffset2, offset;
    Sprite2D* sprite;

    bool borderhorizontal = (side == MapDirection::North || side == MapDirection::South);

    const Vector<int>& gids = borderhorizontal
                              ? (side == MapDirection::North ? terrain.GetDecalGidTableForConnectIndex(TopSide) : terrain.GetDecalGidTableForConnectIndex(BottomSide))
                              : (side == MapDirection::East ? terrain.GetDecalGidTableForConnectIndex(RightSide) : terrain.GetDecalGidTableForConnectIndex(LeftSide));

    const FeaturedMap& features = collider_->map_->GetFeatureViewAtZ(colliderZ);

    if (borderhorizontal)
    {
        numtiles = MapInfo::info.width_;
        offset = 1;

        sideoffset1 = 0;
        sideoffset2 = (MapInfo::info.height_-1) * MapInfo::info.width_;
        if (side == MapDirection::South)
            Swap(sideoffset1, sideoffset2);

        stride.x_ = tilesize.x_;
        position.x_ = 0.5f * tilesize.x_;
        position.y_ = side == MapDirection::North ? (float)MapInfo::info.height_ * tilesize.y_ : 0.f;
    }
    else
    {
        numtiles = MapInfo::info.height_;
        offset = MapInfo::info.width_;
        sideoffset1 = 0;
        sideoffset2 = MapInfo::info.width_-1;
        if (side == MapDirection::East)
            Swap(sideoffset1, sideoffset2);

        stride.y_ = -tilesize.y_;
        position.x_ = side == MapDirection::West ? 0.f : (float)MapInfo::info.width_ * tilesize.x_;
        position.y_ = ((float)MapInfo::info.height_ - 0.5f) * tilesize.y_;
    }

    if (map->GetConnectedMap(side))
    {
        Map* sidemap = map->GetConnectedMap(side);
        /*
                unsigned layerZ = sidemap->GetObjectFeatured()->GetNearestViewZ(colliderZ);
                if (!layerZ)
                {
                    URHO3D_LOGERRORF("RenderShape() - AddBorderOnMapBorder : map=%s sidemap=%s GetNearestViewZ(colliderZ=%d) Error => 0 !",
                                     map->GetMapPoint().ToString().CString(), sidemap->GetMapPoint().ToString().CString(), colliderZ);
                    return;
                }

                const FeaturedMap& sidefeatures = sidemap->GetFeatureViewAtZ(layerZ);
        */
        const FeaturedMap& sidefeatures = sidemap->GetFeatureViewAtZ(colliderZ);

        for (int i=0; i < numtiles; i++)
        {
            if (features[sideoffset1+i*offset] > MapFeatureType::Threshold && sidefeatures[sideoffset2+i*offset] < MapFeatureType::Threshold)
                PushQuad4Vertices(position, atlas->GetDecalSprite(gids[i%gids.Size()]), worldTransform, vertices, vertex);
            position += stride;
        }
    }
    else
    {
        for (int i=0; i < numtiles; i++)
        {
            if (features[sideoffset1+i*offset] > MapFeatureType::Threshold)
                PushQuad4Vertices(position, atlas->GetDecalSprite(gids[i%gids.Size()]), worldTransform, vertices, vertex);
            position += stride;
        }
    }
}

inline void RenderShape::AddBorderOnContour(unsigned ibatch, const PODVector<Vector2>& contour, const Vector2& direction, bool checkborder)
{
    Vector<Vertex2D>& vertices = sourceBatches_[0][ibatch].vertices_;

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    TerrainAtlas* atlas = World2D::GetWorldInfo()->atlas_;

    const MapTerrain& terrain = atlas->GetTerrain(batchinfos_[ibatch].terrainid_);

    URHO3D_LOGWARNINGF("RenderShape() - AddBorderOnContour : use atlas=%s terrain=%u", atlas->GetName().CString(), &terrain);

    Vertex2D vertex[4];
    unsigned color = batchinfos_[ibatch].color_.ToUInt();
    vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = color;
    Sprite2D* sprite;

    Vector2 tilesize(World2D::GetWorldInfo()->tileWidth_, World2D::GetWorldInfo()->tileHeight_);
    Vector2 borderdirection, stride;
    unsigned numelements;

    checkborder = checkborder && hasMapBorder_;

    for (unsigned j=0; j < contour.Size(); j++)
    {
        Vector2 position = j != 0 ? contour[j-1] : contour[contour.Size()-1];

        // skip segments on mapborder
        if (checkborder)
            if (mapBorder_.IsInside(position) == OUTSIDE && mapBorder_.IsInside(contour[j]) == OUTSIDE)
                continue;

        Vector2 bordersegment = contour[j] - position;
        bool borderhorizontal = Abs(bordersegment.x_) > Abs(bordersegment.y_);

        if (borderhorizontal)
        {
            stride = Vector2(Sign(bordersegment.x_) * tilesize.x_, 0.f);
            borderdirection = direction * Sign(bordersegment.x_);
            numelements = (unsigned)round(Abs(bordersegment.x_) / tilesize.x_);
        }
        else
        {
            stride = Vector2(0.f, Sign(bordersegment.y_) * tilesize.y_);
            borderdirection = direction * Sign(bordersegment.y_);
            numelements = (unsigned)round(Abs(bordersegment.y_) / tilesize.y_);
        }

        const Vector<int>& gids = borderhorizontal
                                  ? (bordersegment.x_ > 0.f ? terrain.GetDecalGidTableForConnectIndex(TopSide) : terrain.GetDecalGidTableForConnectIndex(BottomSide))
                                  : (bordersegment.y_ > 0.f ? terrain.GetDecalGidTableForConnectIndex(RightSide) : terrain.GetDecalGidTableForConnectIndex(LeftSide));

        position += 0.5f * stride;

        for (unsigned k=0; k < numelements; k++)
        {
            PushQuad4Vertices(position, atlas->GetDecalSprite(gids[k%gids.Size()]), worldTransform, vertices, vertex);
            position += stride;
        }
    }
}

void RenderShape::UpdateBorderVertices(unsigned ibatch)
{
    if (ibatch >= sourceBatches_[0].Size())
    {
        URHO3D_LOGWARNING("RenderShape() - UpdateBorderVertices : ibatch >= sourceBatches_.Size() !");
        return;
    }

    ShapeBatchInfo& binfo = batchinfos_[ibatch];

    // Add Borders
    if (binfo.dirty_)
    {
        sourceBatches_[0][ibatch].vertices_.Clear();

        for (unsigned i=0; i < shapes_.Size(); i++)
        {
            const PolyShape& shape = shapes_[i];

            for (unsigned j=0; j < shape.contours_.Size(); j++)
                AddBorderOnContour(ibatch, shape.contours_[j]);

            for (unsigned j=0; j < shape.holes_.Size(); j++)
            {
                if (shape.holes_[j].Size())
                    for (unsigned k=0; k < shape.holes_[j].Size(); k++)
                        AddBorderOnContour(ibatch, shape.holes_[j][k], Vector2::NEGATIVE_ONE, false);
            }
        }

        if (hasMapBorder_)
            binfo.mapBorderVerticesOffset_ = sourceBatches_[0][ibatch].vertices_.Size();
    }

    // Add Borders on the MapBorder
    if (hasMapBorder_ && binfo.mapBorderDirty_)
    {
        sourceBatches_[0][ibatch].vertices_.Resize(binfo.mapBorderVerticesOffset_);
        for (int i=0; i < 4; i++)
            AddBorderOnMapBorder(ibatch, i);
    }
}

void RenderShape::UpdateEmboseVertices(unsigned ibatch)
{
    if (ibatch >= sourceBatches_[0].Size())
        return;

    ShapeBatchInfo& batchinfo = batchinfos_[ibatch];
    if (batchinfo.mapBorderDirty_)
    {
        batchinfo.mapBorderDirty_ = false;
        batchinfo.dirty_ = true;

        // Update the embose border for the contour with mapborder
        if (!mapBorder_.Defined())
            mapBorder_ = Rect(0.01f, 0.01f, (float)MapInfo::info.width_ * MapInfo::info.tileWidth_ - 0.01f, (float)MapInfo::info.height_ * MapInfo::info.tileHeight_ - 0.01f);

        for (unsigned i=0; i < shapes_.Size(); ++i)
            shapes_[i].CreateEmboseVertices();
    }

    if (!batchinfo.dirty_)
        return;

    unsigned numtotalvertices = 0;

    for (unsigned i=0; i < shapes_.Size(); ++i)
    {
        numtotalvertices += shapes_[i].emboses_.Size();
        numtotalvertices += shapes_[i].emboseholes_.Size();
        numtotalvertices += shapes_[i].mapbordervertices_.Size();
    }

    if (!numtotalvertices)
    {
//        URHO3D_LOGWARNING("RenderShape() - UpdateFrameVertices : No Triangles !");
        return;
    }

    const Vector2& textureSize = batchinfo.textureSize_;
    const Vector2& textureRepeat = batchinfo.textureRepeat_;

    Vector<Vertex2D>& vertices = sourceBatches_[0][ibatch].vertices_;
    vertices.Resize(numtotalvertices);

    unsigned startvertex = 0;
    unsigned color, colornoapha;
    {
        Color ccolor = batchinfo.color_;
        color = ccolor.ToUInt();
        ccolor.a_ = 0.f;
        colornoapha = ccolor.ToUInt();
    }

#ifdef URHO3D_VULKAN
    unsigned texmode = 0;
#else
    Vector4 texmode;
#endif
    SetTextureMode(TXM_UNIT, batchinfo.textureId_, texmode);
    SetTextureMode(TXM_FX, textureFX_, texmode);

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();

    if (isDynamic_)
    {
        for (unsigned i=0; i < shapes_.Size(); ++i)
        {
            // emboseholes
            {
                const PODVector<Vector2>& triangles = shapes_[i].emboseholes_;
                unsigned numvertices = triangles.Size();
#ifdef EMBOSE_TRIANGLE
                for (int j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    const Vector2& pos = triangles[j];
                    int externalvertex = j % 6;
                    vertex.position_ = worldTransform * pos;
                    // if dynamic, use the local triangle position for calculate the uv
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * pos.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * pos.y_;
                    vertex.color_ = externalvertex > 0 && externalvertex < 4 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#else
                // set vertices attributes : Quad Version
                for (unsigned j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    const Vector2& pos = triangles[j];
                    vertex.position_ = worldTransform * pos;
                    // if dynamic, use the local triangle position for calculate the uv
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * pos.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * pos.y_;
                    int externalvertex = j % 4;
                    vertex.color_ = externalvertex < 2 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#endif
                startvertex += numvertices;
            }

            // emboses
            {
                const PODVector<Vector2>& triangles = shapes_[i].emboses_;
                unsigned numvertices = triangles.Size();
#ifdef EMBOSE_TRIANGLE
                for (int j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    const Vector2& pos = triangles[j];
                    vertex.position_ = worldTransform * pos;
                    // if dynamic, use the local triangle position for calculate the uv
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * pos.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * pos.y_;
                    int externalvertex = j % 6;
                    vertex.color_ = externalvertex > 0 && externalvertex < 4 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#else
                // set vertices attributes : Quad Version
                for (unsigned j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    const Vector2& pos = triangles[j];
                    vertex.position_ = worldTransform * pos;
                    // if dynamic, use the local triangle position for calculate the uv
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * pos.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * pos.y_;
                    int externalvertex = j % 4;
                    vertex.color_ = externalvertex < 2 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#endif
                startvertex += numvertices;
            }
        }
    }
    else
    {
        for (unsigned i=0; i < shapes_.Size(); ++i)
        {
            // emboseholes_
            {
                const PODVector<Vector2>& triangles = shapes_[i].emboseholes_;
                unsigned numvertices = triangles.Size();
#ifdef EMBOSE_TRIANGLE
                // set vertices attributes
                for (unsigned j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    vertex.position_ = worldTransform * triangles[j];
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * vertex.position_.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * vertex.position_.y_;
                    int externalvertex = j % 6;
                    vertex.color_ = externalvertex > 0 && externalvertex < 4 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#else
                // set vertices attributes : Quad Version
                for (unsigned j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    vertex.position_ = worldTransform * triangles[j];
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * vertex.position_.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * vertex.position_.y_;
                    int externalvertex = j % 4;
                    vertex.color_ = externalvertex < 2 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#endif
                startvertex += numvertices;
            }

            // embose
            {
                const PODVector<Vector2>& triangles = shapes_[i].emboses_;
                unsigned numvertices = triangles.Size();
#ifdef EMBOSE_TRIANGLE
                // set vertices attributes
                for (unsigned j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    vertex.position_ = worldTransform * triangles[j];
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * vertex.position_.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * vertex.position_.y_;
                    int externalvertex = j % 6;
                    vertex.color_ = externalvertex > 0 && externalvertex < 4 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#else
                // set vertices attributes : Quad Version
                for (unsigned j=0; j < numvertices; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    vertex.position_ = worldTransform * triangles[j];
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * vertex.position_.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * vertex.position_.y_;
                    int externalvertex = j % 4;
                    vertex.color_ = externalvertex < 2 ? colornoapha : color;
                    vertex.texmode_ = texmode;
                }
#endif
                startvertex += numvertices;
            }

            // map border
            {
                const PODVector<Vector2>& trianglesonborder = shapes_[i].mapbordervertices_;
                unsigned numverticesonborder = trianglesonborder.Size();
                for (unsigned j=0; j < numverticesonborder; ++j)
                {
                    Vertex2D& vertex = vertices[startvertex+j];
                    vertex.position_ = worldTransform * trianglesonborder[j];
                    vertex.uv_.x_ = textureRepeat.x_ / textureSize.x_ * vertex.position_.x_;
                    vertex.uv_.y_ = textureRepeat.y_ / textureSize.y_ * vertex.position_.y_;
                    vertex.color_ = color;
                    vertex.texmode_ = texmode;
                }
                startvertex += numverticesonborder;
            }
        }
    }
}

void RenderShape::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    if (!IsEnabledEffective())
        return;

    if (materialDirty_)
        UpdateMaterials();

//    URHO3D_LOGINFOF("RenderShape() - UpdateSourceBatches : %s(%u) ptr=%u ...", GetNode()->GetName().CString(), GetNode()->GetID(), this);

    for (unsigned i=0; i < batchinfos_.Size(); i++)
    {
        if (batchinfos_[i].type_ == FRAMEBATCH)
            UpdateFrameVertices(i);

        if (batchinfos_[i].type_ == BORDERBATCH)
            UpdateBorderVertices(i);

        if (batchinfos_[i].type_ == EMBOSEBATCH)
            UpdateEmboseVertices(i);

        SetDirty(i, false);
    }
}

void RenderShape::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
#ifdef ACTIVE_RENDERSHAPE_CLIPPING
    // No Multi-viewport
    const Rect& clippingRect = World2D::GetTiledVisibleRect(0);

    if (lastClippingRect_ != clippingRect || eventType == VIEWMANAGER_SWITCHVIEW)
    {
        lastClippingRect_ = clippingRect;

        MarkDirty();

        URHO3D_LOGDEBUGF("RenderShape() - HandleUpdate : %s(%u) layer=%d clippingRect_=%s ...", GetNode()->GetName().CString(), GetNode()->GetID(), GetLayer2().x_, lastClippingRect_.ToString().CString());

        for (unsigned i=0; i < shapes_.Size(); ++i)
        {
            // Clip the shape
            shapes_[i].Clip(lastClippingRect_, true, node_->GetWorldTransform2D().Inverse());
        }

        URHO3D_LOGDEBUGF("RenderShape() - HandleUpdate : %s(%u) layer=%d clippingRect_=%s ... OK !", GetNode()->GetName().CString(), GetNode()->GetID(), GetLayer2().x_, lastClippingRect_.ToString().CString());
    }
#endif
}

void RenderShape::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!IsEnabledEffective())
        return;

    if (!collider_)
        return;

    if (ViewManager::Get()->GetCurrentViewZIndex() != collider_->params_->indz_)
        return;

    // Draw Resulting Meshes
//    for (unsigned i=0; i < sourceBatches_[0].Size(); i++)
//    {
//        const Color& color = batchinfos_[i].type_ == FRAMEBATCH ? Color::BLUE : Color::GRAY;
//        GameHelpers::DrawDebugMesh(debug, sourceBatches_[0][i].vertices_, color, sourceBatches_[0][i].quadvertices_);
//    }

    const Matrix3x4& worldTransform = node_->GetWorldTransform();

    // Draw Contours
    for (unsigned i=0; i < shapes_.Size(); ++i)
    {
        const PolyShape& shape = shapes_[i];

        for (unsigned j=0; j < shape.contours_.Size(); j++)
            GameHelpers::DrawDebugShape(debug, shape.contours_[j], Color::YELLOW, worldTransform);

        if (lastClippingRect_.Defined())
        {
            for (unsigned j=0; j < shape.clippedContours_.Size(); j++)
                GameHelpers::DrawDebugShape(debug, shape.clippedContours_[j], Color::MAGENTA, worldTransform);
        }

        for (unsigned j=0; j < shape.innerContours_.Size(); j++)
            GameHelpers::DrawDebugShape(debug, shape.innerContours_[j], Color::GREEN, worldTransform);

//        unsigned size = Min(contours.Size(), shape.boundingRects_.Size());
//        for (unsigned j=0; j < size; j++)
//            GameHelpers::DrawDebugRect(Rect((worldTransform * Vector3(shape.boundingRects_[j].min_)).ToVector2(), (worldTransform * Vector3(shape.boundingRects_[j].max_)).ToVector2()), debug, false, Color::CYAN);

    }

    // Draw Holes
//    for (unsigned i=0; i < shapes_.Size(); ++i)
//    {
//        const Vector<Vector<PODVector<Vector2> > >& holes = lastClippingRect_.Defined() ? shapes_[i].clippedHoles_ : shapes_[i].holes_;
//
//        for (unsigned j=0; j < holes.Size(); j++)
//            for (unsigned k=0; k < holes[j].Size(); k++)
//                GameHelpers::DrawDebugShape(debug, holes[j][k], Color::RED, worldTransform);
//    }

    // Draw segmentsOnMapBorder_
//    for (unsigned i=0; i < shapes_.Size(); ++i)
//        for (unsigned j=0; j < shapes_[i].segmentsOnMapBorder_.Size(); j++)
//        {
//            for (unsigned k=0; k < shapes_[i].segmentsOnMapBorder_[j].Size(); k++)
//                debug->AddCross(Vector3(worldTransform * shapes_[i].contours_[j][shapes_[i].segmentsOnMapBorder_[j][k]]), 0.5f, Color::GREEN, false);
//        }

    // Draw OnMapBorder Vertices
//    for (unsigned i=0; i < shapes_.Size(); ++i)
//        GameHelpers::DrawDebugVertices(debug, shapes_[i].mapbordervertices_, Color::CYAN, worldTransform);

//    GameHelpers::DrawDebugRect(Rect(worldBoundingBox_.min_.ToVector2(), worldBoundingBox_.max_.ToVector2()), debug, false, Color::MAGENTA);
}
