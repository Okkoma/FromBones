#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/CollisionBox2D.h>

#include "GameOptions.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "MapWorld.h"
#include "ViewManager.h"

#include "RenderShape.h"

#include "DefsColliders.h"


/// Colliders

const ColliderParams dungeonPhysicColliderParams[] =
{
    ///           { indz_, indv_, colliderz_, layerz_, mode_, filtercategory, filtermask }
    /*BackGround*/{ 1, 0, BACKGROUND, BACKGROUND, BackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
    /*BackView*/  { 0, 1, BACKVIEW, BACKVIEW, BackMode, SHT_CHAIN, CC_INSIDEWALL, CM_INSIDEWALL },
    /*InnerView*/ { 0, 2, INNERVIEW, INNERVIEW, InnerMode, SHT_CHAIN, CC_INSIDEWALL, CM_INSIDEWALL },
    /*OuterView*/ { 1, 3, OUTERVIEW, OUTERVIEW, BackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
    /*FrontView*/ { 1, 4, FRONTVIEW, FRONTVIEW, FrontMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },

    /*BackTop*/   { 1, 0, BACKGROUND, BACKGROUND, TopBorderBackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
    /*OuterTop*/  { 1, 3, OUTERVIEW, OUTERVIEW, TopBorderBackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
};

const ColliderParams dungeonRenderColliderParams[] =
{
    ///           { indz_, indv_, colliderz_, layerz_, mode_, viewmask, 0 }
    /*BackGround*/{ 1, 0, BACKGROUND, BACKGROUND, BackRenderMode, SHT_CHAIN, BACKGROUND_MASK, 0 },
    //*BackGround*/{ 1, 0, BACKGROUND, BACKGROUND, FrontMode, SHT_CHAIN, BACKGROUND_MASK, 0 },
    /*BackView*/  { 0, 1, BACKVIEW, BACKVIEW, BackRenderMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
//    /*BackView*/  { 0, 1, BACKVIEW, BACKVIEW, FrontMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*InnerView*/ { 0, 2, INNERVIEW, INNERVIEW, WallMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*Plateform*/ { 0, 2, INNERVIEW, INNERVIEW + LAYER_PLATEFORMS, PlateformMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*OuterView*/ { 1, 3, OUTERVIEW, OUTERVIEW, FrontMode, SHT_CHAIN, OUTERVIEW_MASK, 0 },
    /*FrontView*/ { 1, 4, FRONTVIEW, FRONTVIEW, FrontMode, SHT_CHAIN, FRONTVIEW_MASK, 0 },
};

const ColliderParams cavePhysicColliderParams[] =
{
    /*BackGround*/{ 1, 0, BACKGROUND, BACKGROUND, BackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
    /*InnerView*/ { 0, 1, INNERVIEW, INNERVIEW, InnerMode, SHT_CHAIN, CC_INSIDEWALL, CM_INSIDEWALL },
    /*FrontView*/ { 1, 2, FRONTVIEW, FRONTVIEW, FrontMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },

    /*BackTop*/   { 1, 0, BACKGROUND, BACKGROUND, TopBorderBackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
};

const ColliderParams caveRenderColliderParams[] =
{
    /*BackGround*/{ 1, 0, BACKGROUND, BACKGROUND, BackRenderMode, SHT_CHAIN, BACKGROUND_MASK|BACKVIEW_MASK, 0 },
    /*InnerView*/ { 0, 1, INNERVIEW, INNERVIEW, WallMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*Plateform*/ { 0, 1, INNERVIEW, INNERVIEW + LAYER_PLATEFORMS, PlateformMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*FrontView*/ { 1, 2, FRONTVIEW, FRONTVIEW, FrontMode, SHT_CHAIN, FRONTVIEW_MASK, 0 },
};

const ColliderParams asteroidPhysicColliderParams[] =
{
    /*InnerView*/ { 0, 0, INNERVIEW, INNERVIEW, BorderMode, SHT_BOX, CC_INSIDEOBJECT, CM_INSIDEOBJECT },
    /*Outside  */ { 0, 0, INNERVIEW, INNERVIEW, FrontMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
};

const ColliderParams asteroidRenderColliderParams[] =
{
    /*InnerView*/ { 0, 0, INNERVIEW, INNERVIEW, WallMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
};

const ColliderParams castlePhysicColliderParams[] =
{
    ///           { indz_, indv_, colliderz_, layerz_, mode_, filtercategory, filtermask }
    /*BackView*/  { 0, 0, BACKVIEW, BACKVIEW, BackMode, SHT_CHAIN, CC_INSIDEWALL, CM_INSIDEWALL },
    /*InnerView*/ { 0, 1, INNERVIEW, INNERVIEW, InnerMode, SHT_CHAIN, CC_INSIDEWALL, CM_INSIDEWALL },
    /*InnerView*/ { 0, 1, INNERVIEW, INNERVIEW, InnerMode, SHT_BOX, CC_INSIDEOBJECT, CM_INSIDEOBJECT },
    /*OuterView*/ { 1, 2, OUTERVIEW, OUTERVIEW, BackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },

    /*OuterTop*/  { 1, 2, OUTERVIEW, OUTERVIEW, TopBorderBackMode, SHT_CHAIN, CC_OUTSIDEWALL, CM_OUTSIDEWALL },
};

const ColliderParams castleRenderColliderParams[] =
{
    ///           { indz_, indv_, colliderz_, layerz_, mode_, filtercategory, filtermask }
    /*BackView*/  { 0, 0, BACKVIEW, BACKVIEW, BackRenderMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*InnerView*/ { 0, 1, INNERVIEW, INNERVIEW, WallMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*Plateform*/ { 0, 1, INNERVIEW, INNERVIEW + LAYER_PLATEFORMS, PlateformMode, SHT_CHAIN, BACKVIEW_MASK, 0 },
    /*OuterView*/ { 1, 2, OUTERVIEW, OUTERVIEW, FrontMode, SHT_CHAIN, OUTERVIEW_MASK, 0 },
};


/// MapCollider Class

void MapCollider::Clear()
{
    for (unsigned i=0; i < NUM_COLLIDERSHAPETYPEMODES; ++i)
        numShapes_[i] = 0;

    contourIds_.Clear();
    contourVertices_.Clear();
    holeVertices_.Clear();
}

bool MapCollider::IsInside(const Vector2& point, bool checkhole) const
{
    if (!checkhole)
    {
        for (unsigned i=0; i < contourVertices_.Size(); i++)
        {
            if (GameHelpers::IsInsidePolygon(point, contourVertices_[i]))
                return true;
        }
    }
    else
    {
        for (unsigned i=0; i < contourVertices_.Size(); i++)
        {
            if (GameHelpers::IsInsidePolygon(point, contourVertices_[i]))
            {
                const Vector<PODVector<Vector2> >& holeVertices = holeVertices_[i];
                bool insidehole = false;
                for (Vector<PODVector<Vector2> >::ConstIterator it=holeVertices.Begin(); it != holeVertices.End(); ++it)
                {
                    if (GameHelpers::IsInsidePolygon(point, *it))
                    {
                        insidehole = true;
                        break;
                    }
                }

                if (!insidehole)
                    return true;
            }
        }
    }

    return false;
}

void MapCollider::DumpContour() const
{
    String str;

    str.AppendWithFormat("MapCollider() - DumpContour : numChains=%u\n", contourVertices_.Size());

    for (unsigned i = 0; i < contourVertices_.Size(); i++)
    {
        str.AppendWithFormat("Contour[%u] : numVertices=%u\n", i, contourVertices_[i].Size());
        for (unsigned j = 0; j < contourVertices_[i].Size(); j++)
            str.AppendWithFormat("   Vertex[%u] coords(%f,%f) \n", j, contourVertices_[i][j].x_, contourVertices_[i][j].y_);

        if (holeVertices_.Size())
        {
            for (unsigned j = 0; j < holeVertices_[i].Size(); j++)
            {
                str.AppendWithFormat("Hole[%u][%u] : numVertices=%u\n", i, j, holeVertices_[i][j].Size());
                for (unsigned k = 0; k < holeVertices_[i][j].Size(); k++)
                    str.AppendWithFormat("   Vertex[%u] coords(%f,%f) \n", k, holeVertices_[i][j][k].x_, holeVertices_[i][j][k].y_);
            }
        }
    }

    URHO3D_LOGINFOF("%s", str.CString());
}


/// PhysicCollider Class

PhysicCollider::PhysicCollider() { }

void PhysicCollider::Clear()
{
    MapCollider::Clear();
    infoVertices_.Clear();

//    URHO3D_LOGINFOF("PhysicCollider() - Clear : ... OK !");
}

void PhysicCollider::ClearBlocks()
{
    blocks_.Clear();

//    URHO3D_LOGINFOF("PhysicCollider() - ClearBlocks : ... OK !");
}

void PhysicCollider::ClearPlateforms()
{
    for (HashMap<unsigned, Plateform* >::Iterator it=plateforms_.Begin(); it!=plateforms_.End(); ++it)
    {
        if (it->second_ && it->second_->tileleft_ == it->first_)
            delete it->second_;
    }
    plateforms_.Clear();
}

void PhysicCollider::ReservePhysic(unsigned maxNumBlocks)
{
    if (maxNumBlocks)
    {
//        blocks_.Reserve(maxNumBlocks / 2);
//        plateforms_.Reserve(maxNumBlocks / 20);
    }
}

unsigned PhysicCollider::GetInfoVertex(unsigned index_shape, unsigned index_vertex) const
{
    assert(index_shape < infoVertices_.Size());
    assert(index_vertex < infoVertices_[index_shape].Size());
    return infoVertices_[index_shape][index_vertex].side;
}


/// RenderCollider Class

RenderCollider::RenderCollider()
{
    MapCollider::Clear();
}

void RenderCollider::Clear()
{
    if (renderShape_)
        renderShape_->Clear();
}

void RenderCollider::CreateRenderShape(Node* rootnode, bool mapborder, bool dynamic)
{
    String collidername = String("RenderShape_") + String(GameHelpers::ToUIntAddr(this));
    node_ = rootnode->GetChild(collidername);
    if (!node_)
        node_ = rootnode->CreateChild(collidername, LOCAL);

    node_->SetEnabled(false);

    renderShape_ = node_->GetComponent<RenderShape>();

    if (!renderShape_)
    {
        int viewZ = ViewManager::Get()->GetViewZ(params_->indz_);

        float color = (float)params_->colliderz_ / (float)viewZ;

        ResourceCache* cache = node_->GetSubsystem<ResourceCache>();
        renderShape_ = node_->CreateComponent<RenderShape>();
        renderShape_->SetDynamic(dynamic);
        renderShape_->SetViewMask(params_->bits1_);
        renderShape_->SetLayer2(IntVector2(params_->layerz_ + LAYER_RENDERSHAPE, -1));
        renderShape_->SetOrderInLayer(LAYER_RENDERSHAPE);
        renderShape_->SetMapBorder(mapborder);

        if (GameContext::Get().gameConfig_.renderShapes_)
        {
#ifdef ACTIVE_RENDERSHAPE_EMBOSE
            const float emboseoffset = World2D::GetWorldInfo()->tileWidth_ / 2.5f;
            URHO3D_LOGDEBUGF("tilewidth=%f emboseoffset=%f", World2D::GetWorldInfo()->tileWidth_, emboseoffset);
#endif
            // These are Back RenderShapes
            if (params_->colliderz_ == BACKVIEW)
            {
#ifndef ACTIVE_LAYERMATERIALS
                Material* material = cache->GetResource<Material>("Materials/backviewlayer.xml");
#else
                Material* material = GameContext::Get().layerMaterials_[LAYERRENDERSHAPES];
#endif
                renderShape_->AddBatch(ShapeBatchInfo(FRAMEBATCH, material, 0, -1, Color(1.f, 0.75f, 1.f, RS_ALPHA_BACKVIEW), Vector2(0.3f, 0.3f)));
                renderShape_->SetTextureFX(BACKVIEWLAYER);
            }
            else if (params_->colliderz_ == BACKGROUND)
            {
#ifndef ACTIVE_LAYERMATERIALS
                Material* material = cache->GetResource<Material>("Materials/backgroundlayer.xml");
#else
                Material* material = GameContext::Get().layerMaterials_[LAYERRENDERSHAPES];
#endif
                renderShape_->AddBatch(ShapeBatchInfo(FRAMEBATCH, material, 0, -1, Color(1.f, 1.f, 1.f, RS_ALPHA_BACKGROUND), Vector2(0.3f, 0.3f)));
                renderShape_->SetTextureFX(BACKGROUNDLAYER);
            }
            else if (params_->colliderz_ == OUTERVIEW)
            {
#ifndef ACTIVE_LAYERMATERIALS
                Material* material = cache->GetResource<Material>("Materials/backgroundlayer.xml");
#else
                Material* material = GameContext::Get().layerMaterials_[LAYERRENDERSHAPES];
#endif
                renderShape_->AddBatch(ShapeBatchInfo(FRAMEBATCH, material, 0, -1, Color(0.4f, 0.8f, 0.4f, RS_ALPHA_COMMON), Vector2(0.5f, 0.5f)));
                renderShape_->SetTextureFX(BACKINNERLAYER);
#ifdef ACTIVE_RENDERSHAPE_EMBOSE
                renderShape_->SetEmboseOffset(emboseoffset);
                renderShape_->AddBatch(ShapeBatchInfo(EMBOSEBATCH, material, 0, -1, Color(0.4f, 0.8f, 0.4f, RS_ALPHA_COMMON), Vector2(0.5f, 0.5f)));
#endif
            }
            else if (params_->colliderz_ == INNERVIEW)
            {
#ifndef ACTIVE_LAYERMATERIALS
                Material* material = cache->GetResource<Material>("Materials/backinnerlayer.xml");
#else
                Material* material = GameContext::Get().layerMaterials_[LAYERRENDERSHAPES];
#endif
                renderShape_->AddBatch(ShapeBatchInfo(FRAMEBATCH, material, 0, -1, Color(1.f, 1.f, 1.f, RS_ALPHA_INNERVIEW), Vector2(1.f, 1.f)));
                renderShape_->SetTextureFX(BACKINNERLAYER);
#ifdef ACTIVE_RENDERSHAPE_EMBOSE
                renderShape_->SetEmboseOffset(emboseoffset);
                renderShape_->AddBatch(ShapeBatchInfo(EMBOSEBATCH, material, 0, -1, Color(1.f, 1.f, 1.f, RS_ALPHA_INNERVIEW), Vector2(1.f, 1.f)));
#endif
            }
            // These are Front RenderShapes
            else if (params_->colliderz_ == FRONTVIEW)
            {
#ifndef ACTIVE_LAYERMATERIALS
                Material* material = cache->GetResource<Material>("Materials/backinnerlayer.xml");
#else
                Material* material = GameContext::Get().gameConfig_.fluidEnabled_ ? GameContext::Get().layerMaterials_[LAYERFRONTSHAPES] : GameContext::Get().layerMaterials_[LAYERRENDERSHAPES];
//                Material* material = GameContext::Get().layerMaterials_[LAYERRENDERSHAPES];
#endif
                renderShape_->AddBatch(ShapeBatchInfo(FRAMEBATCH, material, 0, -1, Color(0.4f, 0.8f, 0.4f, RS_ALPHA_COMMON), Vector2(0.5f, 0.5f)));
                renderShape_->SetTextureFX(BACKINNERLAYER);
#ifdef ACTIVE_RENDERSHAPE_EMBOSE
                renderShape_->SetEmboseOffset(emboseoffset);
                renderShape_->AddBatch(ShapeBatchInfo(EMBOSEBATCH, material, 0, -1, Color(0.4f, 0.8f, 0.4f, RS_ALPHA_COMMON), Vector2(0.5f, 0.5f)));
#endif
            }
        }
    }
}

void RenderCollider::UpdateRenderShape()
{
    if (renderShape_)
    {
        renderShape_->Clear();
        renderShape_->SetCollider(this);
    }
}



