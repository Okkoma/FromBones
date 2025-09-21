#include "GameOptions.h"

#ifdef USE_TILERENDERING

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text3D.h>

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>

#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameRand.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "Map.h"
#include "MapWorld.h"

#include "ViewManager.h"

#include "WaterLayer.h"

#include "ObjectTiled.h"

//#define ACTIVE_CORNERTILES
#define USE_MASKVIEWS
//#define OBJECTTILE_DEBUG

#define NOVIEW -1
#define TILEMAPSEED 1000
#define MAXDRAWVIEWS 10
#define MAX_MATERIALBYVIEW 2


extern const char* mapViewsNames[];


/// Custom Assert

#include <iostream>

#ifndef NDEBUG
#   define M_Assert(Expr, Msg) \
    __M_Assert(#Expr, Expr, __FILE__, __LINE__, Msg)
#else
#   define M_Assert(Expr, Msg) ;
#endif

String debugAssertString_;

void __M_Assert(const char* expr_str, bool expr, const char* file, int line, const char* msg)
{
    if (!expr)
    {
        std::cerr << "Assert failed:\t" << msg << "\n"
                  << "Expected:\t" << expr_str << "\n"
                  << "Source:\t\t" << file << ", line " << line << "\n";
        abort();
    }
}



#ifdef USE_CHUNKBATCH

/// ChunkBatch

void ChunkBatch::Allocate(Drawable2D* owner, unsigned numbatches)
{
    owner_ = owner;
    batches_.Resize(numbatches);
    localpositions_.Resize(numbatches);

    counterBatch_ = 0;
    batchMap_.Clear();
}

void ChunkBatch::Clear()
{
    for (unsigned i=0; i < batches_.Size(); i++)
    {
        SourceBatch2D& batch = batches_[i];
        batch.material_.Reset();
        batch.vertices_.Clear();
        batch.owner_ = owner_;
        localpositions_[i].Clear();
    }

    counterBatch_ = 0;
    batchMap_.Clear();
    dirty_ = false;
}

inline unsigned ChunkBatch::GetSourceBatchIndex(int indexV, Material* material, int drawOrder)
{
    M_Assert(indexV < 16, debugAssertString_.AppendWithFormat("ChunkBatch() - GetSourceBatchIndex : indexV(%d) >= 16 !", indexV).CString());

    // hash key : 32bits ( materialHash=28bits(), indexV=4bits(16indexV)
    unsigned key = (material->GetNameHash().Value() << 4) + indexV;

    HashMap<unsigned, unsigned >::Iterator it = batchMap_.Find(key);
    if (it == batchMap_.End())
    {
        M_Assert(indexV < 16, debugAssertString_.AppendWithFormat("ChunkBatch() - GetSourceBatchIndex : indexV(%d) >= 16 !", indexV).CString());

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGINFOF("ChunkBatch() - GetSourceBatchIndex ... new batch - material=%s(%u) draworder=%d hash=%u indexV=%d counter=%u key=%u ...",
                        material->GetName().CString(), material, drawOrder, material->GetNameHash().Value(), indexV, counterBatch_, key);
#endif

        M_Assert(counterBatch_ < batches_.Size(), debugAssertString_.AppendWithFormat("ChunkBatch() - GetSourceBatchIndex : counterBatch_(%u) >= batches_.Size(%u) !", counterBatch_, batches_.Size()).CString());

        SourceBatch2D& batch = batches_[counterBatch_];
        batch.drawOrder_ = drawOrder;
        batch.material_ = material;

        it = batchMap_.Insert(Pair<unsigned, unsigned>(key, counterBatch_));
        counterBatch_++;
    }

    return it->second_;
}

unsigned ChunkBatch::GetSourceBatchesToRender(int drawOrderMin, int drawOrderMax, Vector<SourceBatch2D*>& batchesToRender)
{
    unsigned numbatches = 0;

    for (unsigned i=0; i < batches_.Size(); ++i)
    {
        SourceBatch2D& batch = batches_[i];
        if (batch.vertices_.Size() > 0 && batch.drawOrder_ >= drawOrderMin && batch.drawOrder_ <= drawOrderMax)
        {
            batchesToRender.Push(&batch);
            numbatches++;
        }
    }
    /*
        for (HashMap<unsigned, SourceBatch2D* >::Iterator it=batchMap_.Begin(); it != batchMap_.End(); ++it)
        {
            batchesToRender.Push(it->second_);
            numbatches++;
        }
    */
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ChunkBatch() - GetSourceBatchesToRender ... batchesToRender.Size=%u (numbatches added=%u) ...", batchesToRender.Size(), numbatches);
#endif

    return numbatches;
}

void ChunkBatch::UpdateVerticePositions(Node* node)
{
    const Matrix2x3& worldTransform = node->GetWorldTransform2D();

    for (unsigned i=0; i < batches_.Size(); i++)
    {
        Vector<Vertex2D>& vertices = batches_[i].vertices_;
        Vector<Vector2>& localpositions = localpositions_[i];
        for (unsigned j=0; j < vertices.Size(); j++)
            vertices[j].position_ = worldTransform * localpositions[j];
    }
}

#else

void BatchInfo::UpdateVerticePositions(Node* node)
{
    Vector<Vertex2D>& vertices = batch_.vertices_;
    if (!vertices.Size() || vertices.Size() != localpositions_.Size())
        return;

    const Matrix2x3& worldTransform = node->GetWorldTransform2D();
    for (unsigned i=0; i < localpositions_.Size(); i++)
        vertices[i].position_ = worldTransform * localpositions_[i];
}

#endif

/// ObjectTiled Implementation

TerrainAtlas* ObjectTiled::atlas_ = 0;

int ObjectTiled::cuttingLevel_ = 0;
int ObjectTiled::maxDrawViews_ = MAXDRAWVIEWS;
bool ObjectTiled::tilesEnable_ = true;
bool ObjectTiled::decalsEnable_ = true;
//int ObjectTiled::viewRangeMode_ = ViewRange_Frustum;
int ObjectTiled::viewRangeMode_ = ViewRange_WorldVisibleRect;

const StringHash eventFluidUpdate = E_SCENEPOSTUPDATE;
//const StringHash eventUpdate = E_SCENEUPDATE;

void ObjectTiled::RegisterObject(Context* context)
{
    context->RegisterFactory<ObjectTiled>();
}

void ObjectTiled::RegisterWorldInfo(World2DInfo* winfo)
{
    atlas_ = winfo->atlas_;
}

void ObjectTiled::SetCuttingLevel(int level)
{
    if (cuttingLevel_ != level)
    {
        cuttingLevel_ = level;
        World2D::GetWorld()->SendEvent(WORLD_DIRTY);
    }
}

void ObjectTiled::SetMaxDrawView(int numViews)
{
    if (maxDrawViews_ != numViews)
    {
        maxDrawViews_ = !numViews ? MAXDRAWVIEWS : Max(1, numViews);
        World2D::GetWorld()->SendEvent(WORLD_DIRTY);
    }
}

void ObjectTiled::SetTilesEnable(bool state)
{
    if (tilesEnable_ != state)
    {
        tilesEnable_ = state;
        World2D::GetWorld()->SendEvent(WORLD_DIRTY);
    }
}

void ObjectTiled::SetDecalsEnable(bool state)
{
    if (decalsEnable_ != state)
    {
        decalsEnable_ = state;
        World2D::GetWorld()->SendEvent(WORLD_DIRTY);
    }
}

void ObjectTiled::SetViewRangeMode(int mode)
{
//    if (viewRangeMode_ != mode)
//    {
//        viewRangeMode_ = mode;
//        World2D::GetWorld()->SendEvent(WORLD_DIRTY);
//    }
}



ObjectTiled::ObjectTiled() :
    Drawable2D(0),
    chinfo_(0),
    isDynamic_(false),
    isChunked_(false),
    sharedChunkInfo_(false),
    enlargeBox_(0),
    numviews_(0)
{
    Init();
}

ObjectTiled::ObjectTiled(Context* context) :
    Drawable2D(context),
    chinfo_(0),
    isDynamic_(false),
    isChunked_(false),
    sharedChunkInfo_(false),
    enlargeBox_(0),
    numviews_(0)
{
    Init();
}

ObjectTiled::ObjectTiled(const ObjectTiled& obj) :
    Drawable2D(obj.GetContext()),
    chinfo_(0),
    isDynamic_(false),
    isChunked_(false),
    sharedChunkInfo_(false),
    enlargeBox_(obj.enlargeBox_),
    numviews_(0)
{
    Init();
}

ObjectTiled::~ObjectTiled()
{
//    URHO3D_LOGDEBUG("~ObjectTiled() ...");

    skinData_.Reset();

    /// Free none shared ChunkInfo
    if (chinfo_ && !sharedChunkInfo_)
    {
//        URHO3D_LOGDEBUGF("~ObjectTiled() - FreeChunks !");

        delete chinfo_;
        chinfo_ = 0;
    }

//    URHO3D_LOGDEBUG("~ObjectTiled() ... OK !");
}

void ObjectTiled::Init()
{
    layermodifier_ = 0;
    indexToSet_ = indexGrpToSet_ = indexVToSet_ = indexZToSet_ = indexChunks_ = 0;

    for (int i=0; i < MAX_VIEWPORTS; i++)
        viewportDatas_[i].Set(this, i);

    ClearChunksBatches();

    dirtyChunkGroups_.Clear();

    if (!skinData_)
        skinData_ = SharedPtr<ObjectSkinned>(new ObjectSkinned());
}

void ObjectTiled::Resize(int width, int height, unsigned numviews, ChunkInfo* chinfo)
{
    // offset indices in CW, start to the middle-left
    for (int i=0; i < 9; ++i)
        neighborInd[i] = width*MapInfo::neighborOffY[i] + MapInfo::neighborOffX[i];

    numviews_ = numviews;
    skinData_->Resize(width, height, numviews);

    float fwidth = (float)width * MapInfo::info.tileWidth_;
    float fheight = (float)height * MapInfo::info.tileHeight_;
    boundingBox_.Define(Vector3(-hotspot_.x_ * fwidth, -hotspot_.y_ * fheight, 0.f), Vector3((1.f-hotspot_.x_) * fwidth, (1.f-hotspot_.y_) * fheight, 0.f));

    SetChunked(chinfo, true);
}

void ObjectTiled::Clear()
{
    if (skinData_)
        skinData_->Clear();

    if (GetScene())
        UnsubscribeFromEvent(GetScene(), eventFluidUpdate);
    else
        UnsubscribeFromEvent(eventFluidUpdate);

    Init();
}

void ObjectTiled::AllocateChunkBatches()
{
    if (chinfo_->numx_*chinfo_->numy_ == 0)
        return;

#ifndef USE_CHUNKBATCH
    unsigned numchunks = chinfo_->numx_*chinfo_->numy_;
    unsigned numViewZ = ViewManager::Get()->GetNumViewZ();

    viewBatchesTable_.Reserve(numViewZ*numviews_*MAX_MATERIALBYVIEW*NUM_MAPOBJECTTYPE*numchunks);
//    URHO3D_LOGINFOF("ObjectTiled() - AllocateChunkBatches ... NumViewBatchesCapacity=%u (numViewZ(%u),numViews(%u),numChunks(%u),M=%d,O=%d) !",
//				viewBatchesTable_.Capacity(), numViewZ, numviews_, numchunks, MAX_MATERIALBYVIEW, NUM_MAPOBJECTTYPE);
#else
    unsigned numchunks = chinfo_->numx_*chinfo_->numy_;
    unsigned numViewZ = ViewManager::Get()->GetNumViewZ();
    chunkBatches_.Reserve(numViewZ);
    chunkBatches_.Resize(numViewZ);
    for (unsigned i=0; i < numViewZ; i++)
    {
        Vector<ChunkBatch>& viewbatches = chunkBatches_[i];
        viewbatches.Resize(numchunks);
        for (unsigned j=0; j < numchunks; j++)
            viewbatches[j].Allocate(this, numviews_*MAX_MATERIALBYVIEW);
    }
//    URHO3D_LOGINFOF("ObjectTiled() - AllocateChunkBatches ... chunkbatcheSize=%u (numViewZ(%u), numChunks(%u)) !", numViewZ * numchunks, numViewZ, numchunks);
#endif // USE_CHUNKBATCH
}

void ObjectTiled::ClearChunksBatches()
{
#ifdef USE_CHUNKBATCH
    unsigned numviewZ = chunkBatches_.Size();
    unsigned numchunks = numviewZ ? chunkBatches_[0].Size() : 0;
    for (unsigned i=0; i < numviewZ; i++)
    {
        for (unsigned j=0; j < numchunks; j++)
            chunkBatches_[i][j].Clear();
    }
#else
    viewBatchesTable_.Clear();
    viewBatchesIndexes_.Clear();
    viewMaterialsTable_.Clear();
#endif // USE_CHUNKBATCH
}

void ObjectTiled::SetChunked(ChunkInfo* chinfo, bool shared)
{
    unsigned oldchunks = chinfo_ ? chinfo_->numx_*chinfo_->numy_ : 0;
    unsigned newchunks = chinfo ? chinfo->numx_*chinfo->numy_ : 0;

    /// Free none shared ChunkInfo only if it's chunked
    if (chinfo_ && isChunked_)
    {
        if (!sharedChunkInfo_)
            delete chinfo_;

        chinfo_ = 0;
    }

    if (GetAtlas()->UseDimensionTiles())
        enlargeBox_ = 1;

    ApplyCuttingLevel();

    if (chinfo)
    {
        isChunked_ = true;
//        isChunked_ = (chinfo->numx_ > 0 && chinfo->numy_ > 0) && (chinfo->numx_ > 1 || chinfo->numy_ > 1);
        chinfo_ = chinfo;
        sharedChunkInfo_ = shared;
    }
    else if (!chinfo_)
    {
        // one big chunk by default
        isChunked_ = false;
        chinfo_ = new ChunkInfo(GetWidth(), GetHeight(), 1, 1, MapInfo::info.tileWidth_, MapInfo::info.tileHeight_);
        sharedChunkInfo_ = false;
    }

#ifdef RESERVE_VIEWBATCHES
    if (oldchunks != newchunks)
        AllocateChunkBatches();
#endif

    for (int i=0; i < MAX_VIEWPORTS; i++)
        viewportDatas_[i].chunkGroup_.Reset(chinfo_, i);

//    URHO3D_LOGINFOF("ObjectTiled() - SetChunked : shared=%s sx=%u sy=%u nx=%u ny=%u tx=%f ty=%f",
//                      shared ? "true" : "false", chinfo_->sizex_, chinfo_->sizey_, chinfo_->numx_, chinfo_->numy_, chinfo_->tileWidth_, chinfo_->tileHeight_);
//    URHO3D_LOGINFOF("ObjectTiled() - SetChunked : skinData_=%u featureData_=%u enlargeBox_=%d", skinData_.Get(), skinData_->GetFeatureData(), enlargeBox_);
}

void ObjectTiled::SetChunked(unsigned char numx, unsigned char numy)
{
    if (numx && numy)
    {
        ChunkInfo* chinfo = new ChunkInfo(GetWidth(), GetHeight(), numx, numy, MapInfo::info.tileWidth_, MapInfo::info.tileHeight_);

        if (chinfo->numx_ == 0)
            delete chinfo;
        else
            SetChunked(chinfo, false);
    }
}

void ObjectTiled::SetHotSpot(const Vector2& hotspot)
{
    if (hotspot != hotspot_)
    {
        hotspot_ = hotspot;

        float width = (float)GetWidth() * MapInfo::info.tileWidth_;
        float height = (float)GetHeight() * MapInfo::info.tileHeight_;
        boundingBox_.Define(Vector3(-hotspot_.x_ * width, -hotspot_.y_ * height, 0.f), Vector3((1.f-hotspot_.x_) * width, (1.f-hotspot_.y_) * height, 0.f));

        SetDirty();
    }
}

void ObjectTiled::SetCurrentViewZ(int viewport, int viewZindex, bool forced)
{
    if (viewZindex == -1)
        return;

    ViewportRenderData& viewportdata = viewportDatas_[viewport];

    int viewZ = ViewManager::Get()->GetViewZ(viewZindex);

//    URHO3D_LOGINFOF("ObjectTiled() - SetCurrentViewZ node=%s viewport=%d needed viewZ=%d (current=%d)... ", node_->GetName().CString(), viewport, viewZ, viewportdata.currentViewZindex_ != -1 ? ViewManager::Get()->GetViewZ(viewportdata.currentViewZindex_) : -1);

    if (viewZindex != viewportdata.currentViewZindex_ || forced)
    {
//        viewportdata.currentViewZindex_ = viewZindex;
        viewportdata.currentViewZindex_ = -1;
//        URHO3D_LOGWARNINGF("ObjectTiled() - SetCurrentViewZ node=%s viewport=%d viewZ=%d currentViewZindex_ = -1 Marked !", node_->GetName().CString(), viewport, viewZ);

        ObjectFeatured* feature = GetObjectFeatured();
        assert(feature);
        int id = feature->GetViewId(viewZ);

        if (viewportdata.currentViewID_ != id || forced)
        {
            viewportdata.currentViewID_ = id;

            if (viewportdata.currentViewID_ == NOVIEW)
            {
                URHO3D_LOGERRORF("ObjectTiled() - SetCurrentViewZ node=%s viewport=%d viewZ=%d viewID=%d NOVIEW DumpViewIds=%s !",
                                 node_ ? node_->GetName().CString() : "0", viewport, viewZ, id, feature->GetDumpViewIds().CString());
                return;
            }

//            URHO3D_LOGINFOF("ObjectTiled() - SetCurrentViewZ node=%s viewport=%d viewZ=%d viewID=%d ... OK !", node_->GetName().CString(), viewport, viewZ, id);

            SetDirty(viewport);
            if (WaterLayer::Get())
                WaterLayer::Get()->SetDirty(viewport);
        }
    }
}

void ObjectTiled::SetRenderPaused(int viewport, bool state)
{
    viewportDatas_[viewport].isPaused_ = state;
//    URHO3D_LOGINFOF("ObjectTiled() - SetRenderPaused %s(%u) : viewport=%d ispaused=%s !", node_ ? node_->GetName().CString() : "none", node_ ? node_->GetID() : 0, viewport, state ? "true":"false");
}

void ObjectTiled::SetDynamic(bool state)
{
    isDynamic_ = state;
}

void ObjectTiled::SetLayerModifier(int modifier)
{
    layermodifier_ = modifier;
}

void ObjectTiled::SetDirty(int viewport)
{
    if (IsEnabledEffective() && skinData_)
    {
        ApplyCuttingLevel(viewport);

        if (viewport == -1)
        {
            for (viewport=0; viewport < MAX_VIEWPORTS; viewport++)
            {
                ViewportRenderData& viewportdata = viewportDatas_[viewport];
                viewportdata.sourceBatchDirty_ = true;
//                viewportdata.sourceBatchFeatureDirty_ = true;
                viewportdata.isDirty_ = false;
                viewportdata.currentViewZindex_ = -1;
            }
            viewport = -1;
//            URHO3D_LOGINFOF("ObjectTiled() - SetDirty : %s viewport=-1 ! ", node_->GetName().CString());
        }
        else
        {
            ViewportRenderData& viewportdata = viewportDatas_[viewport];
            viewportdata.sourceBatchDirty_ = true;
//            viewportdata.sourceBatchFeatureDirty_ = true;
            viewportdata.isDirty_ = false;
            viewportdata.currentViewZindex_ = -1;
//            URHO3D_LOGINFOF("ObjectTiled() - SetDirty : %s viewport=%d ! ", node_->GetName().CString(), viewport);
        }
    }
    else
    {
        if (viewport == -1)
        {
            for (viewport=0; viewport < MAX_VIEWPORTS; viewport++)
                viewportDatas_[viewport].isDirty_ = true;
        }
        else
        {
            viewportDatas_[viewport].isDirty_ = true;
        }
    }
}

void ObjectTiled::ApplyCuttingLevel(int viewport)
{
    if (!skinData_->feature_->viewIds_.Size())
        return;

    int numviewportstoset = 1;
    if (viewport == -1)
    {
        viewport = 0;
        numviewportstoset = ViewManager::Get()->GetNumViewports();
    }

    while (numviewportstoset > 0)
    {
        ViewportRenderData& viewportdata = viewportDatas_[viewport];

        int viewZ = ViewManager::Get()->GetCurrentViewZ(viewport);
        if (!cuttingLevel_)
        {
            int numViews = (int)GetObjectFeatured()->GetViewIDs(viewZ).Size();
            viewportdata.indexMinView_ = numViews - Min(maxDrawViews_, numViews);
            viewportdata.indexMaxView_ = numViews - 1;
            // Keep plateform so indexMaxView_++
            viewportdata.indexMaxView_++;
        }
        else
        {
            int cutViewZ = viewZ - cuttingLevel_;
            if (cutViewZ < 0)
            {
                viewportdata.indexMinView_ = -1;
                numviewportstoset--;
                viewport++;
                continue;
            }

            int nearStdViewZ = ViewManager::Get()->GetNearViewZ(cutViewZ, 1);
            if (nearStdViewZ < 0)
            {
                viewportdata.indexMinView_ = -1;
                numviewportstoset--;
                viewport++;
                continue;
            }

            const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(nearStdViewZ);
            int numViews = (int)viewIds.Size();
            viewportdata.indexMinView_ = numViews - Min(maxDrawViews_, numViews);
            viewportdata.indexMaxView_ = numViews - 1;

            while (viewportdata.indexMaxView_ >= 0 && GetObjectFeatured()->GetViewZ(viewIds[viewportdata.indexMaxView_]) > cutViewZ)
                viewportdata.indexMaxView_--;

            if (viewportdata.indexMinView_ > viewportdata.indexMaxView_)
                Swap(viewportdata.indexMinView_, viewportdata.indexMaxView_);

//            URHO3D_LOGINFOF("ObjectTiled() - ApplyCuttingLevel : %s viewport=%d indexminview=%d indexmaxview=%d !", node_->GetName().CString(), viewport, viewportdata.indexMinView_, viewportdata.indexMaxView_);
        }

        numviewportstoset--;
        viewport++;
    }
}

const unsigned& ObjectTiled::GetWidth() const
{
    return GetObjectFeatured()->width_;
}

const unsigned& ObjectTiled::GetHeight() const
{
    return GetObjectFeatured()->height_;
}

BoundingBox ObjectTiled::GetWorldBoundingBox2D()
{
    if (IsEnabledEffective())
    {
        int viewport = ViewManager::Get()->GetViewport(renderer_->GetCurrentFrameInfo().camera_);

        ViewportRenderData& viewportdata = viewportDatas_[viewport];

        if (viewportdata.isPaused_)
            return worldBoundingBox_;

        if (viewportdata.isDirty_)
            SetDirty(viewport);

        bool toupdate = false;
        if (isChunked_)
        {
            if (viewRangeMode_ == ViewRange_WorldVisibleRect)
            {
                const Rect& viewrect = World2D::GetExtendedVisibleRect(viewport);
//                Rect viewrect = World2D::GetVisibleRect(viewport);

                if (viewportdata.lastViewBoundingBox_ != viewrect)
                {
                    viewportdata.lastViewBoundingBox_ = viewrect;
                    viewportdata.lastVisibleRect_ = viewportdata.visibleRect_;

                    if (!chinfo_->Convert2ChunkRect(viewportdata.lastViewBoundingBox_.Transformed(node_->GetWorldTransform().Inverse()), viewportdata.visibleRect_, 0))
                        URHO3D_LOGERRORF("ObjectTiled() - GetWorldBoundingBox2D : bbox not defined !");
                }
            }

            if (viewportdata.visibleRect_ != viewportdata.lastVisibleRect_)
            {
//                URHO3D_LOGERRORF("ObjectTiled() - GetWorldBoundingBox2D : viewport=%d lastVisibleRect_=%s visibleRect_=%s => update !",
//                                 viewport, viewportdata.lastVisibleRect_.ToString().CString(), viewportdata.visibleRect_.ToString().CString());

                viewportdata.lastVisibleRect_ = viewportdata.visibleRect_;
                toupdate = true;
            }
        }

        if (viewportdata.currentViewZindex_ == -1)
        {
//            URHO3D_LOGINFOF("ObjectTiled() - GetWorldBoundingBox2D : viewport=%d currentviewZ=%d != %d => update !",
//                                viewport, viewportdata.currentViewZindex_, ViewManager::Get()->GetCurrentViewZIndex(viewport));

            viewportdata.currentViewZindex_ = ViewManager::Get()->GetCurrentViewZIndex(viewport);
            if (viewportdata.currentViewZindex_ != -1)
            {
                viewportdata.currentViewID_ = GetObjectFeatured()->GetViewId(ViewManager::Get()->GetViewZ(viewportdata.currentViewZindex_));
                toupdate = true;
            }
        }

        if (toupdate)
        {
            if (viewportdata.visibleRect_ == IntRect::ZERO)
                URHO3D_LOGWARNINGF("ObjectTiled() - GetWorldBoundingBox2D : node=%s no rect=%s", node_->GetName().CString(), viewportdata.visibleRect_.ToString().CString());

            viewportdata.sourceBatchDirty_ = true;
            viewportdata.sourceBatchFeatureDirty_ = true;

            if (isChunked_)
                UpdateChunksVisiblity(viewportdata);
        }
    }

    if (worldBoundingBoxDirty_)
    {
        OnWorldBoundingBoxUpdate();
        worldBoundingBoxDirty_ = false;
    }

    return worldBoundingBox_;
}

void ObjectTiled::UpdateVerticesPositions(Node* node)
{
    if (isDynamic_)
    {
#ifndef USE_CHUNKBATCH
        for (unsigned i=0; i < viewBatchesTable_.Size(); i++)
            viewBatchesTable_[i].UpdateVerticePositions(node);
#else
        for (unsigned i=0; i < chunkBatches_.Size(); i++)
            for (unsigned j=0; j < chunkBatches_[i].Size(); j++)
                chunkBatches_[i][j].UpdateVerticePositions(node);
//        UpdateViews();
#endif

        for (int i=0; i < MAX_VIEWPORTS; i++)
            viewportDatas_[i].sourceBatchDirty_ = true;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGINFOF("ObjectTiled() - UpdateVerticesPositions : ref node=%s(%u) dynamic update vertices !", node->GetName().CString(), node->GetID());
#endif
    }
}

void ObjectTiled::OnWorldBoundingBoxUpdate()
{
    worldBoundingBox_ = boundingBox_.Transformed(node_->GetWorldTransform2D());
    UpdateVerticesPositions(node_);
}


void ObjectTiled::OnDrawOrderChanged()
{
    ;
}

void ObjectTiled::OnSetEnabled()
{
//    URHO3D_LOGINFOF("ObjectTiled() - OnSetEnabled : node=%s(%u) enabled=%s",
//             node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");

    Drawable2D::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene)
    {
        bool enable = IsEnabledEffective();
        if (enable)
        {
            SetDirty();
        }

        if (WaterLayer::Get())
        {
            for (int i=0; i < MAX_VIEWPORTS; i++)
                WaterLayer::Get()->SetViewportData(enable, &viewportDatas_[i]);
        }
    }
}

void ObjectTiled::OnNodeSet(Node* node)
{
    // Important : add listener for node markdirty (allow updateviews if isDynamic_)
    Drawable2D::OnNodeSet(node);
}


///
/// Update Batches
///

#ifndef USE_CHUNKBATCH
void ObjectTiled::ClearChunkBatches(const ChunkGroup& chunkGroup, MapObjectType type, int indexZ)
{
    URHO3D_PROFILE(ClearChunkBatches);

//    URHO3D_LOGINFOF("ObjectTiled() - ClearChunkBatches %s on indexZ=%d ...", node_->GetName().CString(), indexZ);

    if (type == DECAL && !GetObjectSkinned()->UseDimensionTiles())
        return;

    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const unsigned numchunks = chunkGroup.numChunks_;
    const int numViews = GetObjectFeatured()->GetViewIDs(ViewManager::Get()->GetViewZ(indexZ)).Size();

    unsigned indexC;
    for (int indexV = 0; indexV <= numViews; indexV++)
    {
        unsigned basekey = GetBaseBatchInfoKey(indexZ, indexV);
        // Clear TILE Vertices
        for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
            for (unsigned c = 0; c < numchunks; c++)
            {
                BatchInfo* binfo = GetChunkBatchInfoBased(TILE, basekey, chindex[c], m);
                if (!binfo)
                    continue;
                binfo->localpositions_.Clear();
                binfo->batch_.vertices_.Clear();
            }

        // Clear DECAL Vertices
        for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
            for (unsigned c = 0; c < numchunks; c++)
            {
                BatchInfo* binfo = GetChunkBatchInfoBased(DECAL, basekey, chindex[c], m);
                if (!binfo)
                    continue;
                binfo->localpositions_.Clear();
                binfo->batch_.vertices_.Clear();
            }
#ifdef RENDER_TERRAINS_SEWINGS
        // Clear SEWING Vertices
        for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
            for (unsigned c = 0; c < numchunks; c++)
            {
                BatchInfo* binfo = GetChunkBatchInfoBased(SEWING, basekey, chindex[c], m);
                if (!binfo)
                    continue;
                binfo->localpositions_.Clear();
                binfo->batch_.vertices_.Clear();
            }
#endif
    }
}

void ObjectTiled::SetChunkBatchesDirty(const ChunkGroup& chunkGroup)
{
    URHO3D_PROFILE(SetChunkBatchesDirty);

    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const unsigned numChunks = chunkGroup.numChunks_;
    const int numViewZ = ViewManager::Get()->GetNumViewZ();

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    if (chunkGroup.id_ == -1)
        URHO3D_LOGINFOF("ObjectTiled() - SetChunkBatchesDirty %s group=%u", node_->GetName().CString(), chunkGroup.chIndexes_[0]);
    else
        URHO3D_LOGINFOF("ObjectTiled() - SetChunkBatchesDirty %s group=%s", node_->GetName().CString(), MapDirection::GetName(chunkGroup.id_));
#endif

    bool useDecals = GetObjectSkinned()->UseDimensionTiles();
    for (int indexZ = 0; indexZ < numViewZ; indexZ++)
    {
        const int numViews = GetObjectFeatured()->GetViewIDs(ViewManager::Get()->GetViewZ(indexZ)).Size();
        for (int indexV = 0; indexV <= numViews; indexV++)
        {
            unsigned basekey = GetBaseBatchInfoKey(indexZ, indexV);
            // Set TILE Batches Dirty
            for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
                for (unsigned c = 0; c < numChunks; c++)
                {
                    BatchInfo* binfo = GetChunkBatchInfoBased(TILE, basekey, chindex[c], m);
                    if (!binfo)
                        continue;

                    binfo->dirty_ = true;
                    /// TEST
//                binfo->batch_.vertices_.Clear();
                }
            // Set DECAL Batches Dirty
            if (useDecals)
                for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
                    for (unsigned c = 0; c < numChunks; c++)
                    {
                        BatchInfo* binfo = GetChunkBatchInfoBased(DECAL, basekey, chindex[c], m);
                        if (!binfo)
                            continue;

                        binfo->dirty_ = true;
                        /// always clear Decals : need at borders after a Map::ConnectBorders
                        //binfo->batch_.vertices_.Clear();
                    }
#ifdef RENDER_TERRAINS_SEWINGS
            // Set SEWING Batches Dirty
            if (useDecals)
                for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
                    for (unsigned c = 0; c < numChunks; c++)
                    {
                        BatchInfo* binfo = GetChunkBatchInfoBased(SEWING, basekey, chindex[c], m);
                        if (!binfo)
                            continue;

                        binfo->dirty_ = true;
                    }
#endif
        }
    }
}
#else
void ObjectTiled::SetChunkBatchesDirty(const ChunkGroup& chunkGroup)
{
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const unsigned numChunks = chunkGroup.numChunks_;
    const int numViewZ = ViewManager::Get()->GetNumViewZ();

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    if (chunkGroup.id_ == -1)
        URHO3D_LOGINFOF("ObjectTiled() - SetChunkBatchesDirty %s group=%u", node_->GetName().CString(), chunkGroup.chIndexes_[0]);
    else
        URHO3D_LOGINFOF("ObjectTiled() - SetChunkBatchesDirty %s group=%s", node_->GetName().CString(), MapDirection::GetName(chunkGroup.id_));
#endif

    for (int indexZ = 0; indexZ < numViewZ; indexZ++)
    {
        for (unsigned c = 0; c < numChunks; c++)
        {
            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, chindex[c]);
            chunkBatch.dirty_ = true;
        }
    }
}
#endif

void ObjectTiled::MarkChunkGroupDirty(const ChunkGroup& chunkGroup)
{
    if (!dirtyChunkGroups_.Contains(&chunkGroup))
    {
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        if (chunkGroup.id_ == -1)
            URHO3D_LOGINFOF("ObjectTiled() - MarkChunkGroupDirty %s group=%u", node_->GetName().CString(), chunkGroup.chIndexes_[0]);
        else
            URHO3D_LOGINFOF("ObjectTiled() - MarkChunkGroupDirty %s group=%s", node_->GetName().CString(), MapDirection::GetName(chunkGroup.id_));
#endif
        dirtyChunkGroups_.Push(&chunkGroup);
    }
}

#ifndef USE_CHUNKBATCH

unsigned ObjectTiled::GetBaseBatchInfoKey(int indexZ, int indexV) const
{
    return (indexZ << 28) + (indexV << 24);
}

unsigned ObjectTiled::GetBatchInfoKey(unsigned baseKey, unsigned indexM, unsigned indexC, MapObjectType type) const
{
    return baseKey + (indexM << 20) + (type << 16) + indexC;
}

unsigned ObjectTiled::GetBatchInfoKey(int indexZ, int indexV, unsigned indexM, unsigned indexC, MapObjectType type) const
{
    unsigned key;
    // hash key : 32bits ( indexZ=4bits(16indexZsmax), indexV=4bits(16indexV), indexM=4bits(16materialmax), indexT=4bits(16typesmax), indexC=16bits(65536chunksmax)
    key = (indexZ << 28) + (indexV << 24) + (indexM << 20) + (type << 16) + indexC;

    return key;
}

BatchInfo* ObjectTiled::GetChunkBatchInfoBased(MapObjectType type, unsigned baseKey, unsigned indexC, unsigned indexM)
{
    URHO3D_PROFILE(GetChunkBatchInfoBased);

    M_Assert(indexM < 16 && indexC < 65536, debugAssertString_.AppendWithFormat("indexM(%u) >= 16 || indexC(%u) >= 65536 !", indexM, indexC).CString());

    unsigned key = GetBatchInfoKey(baseKey, indexM, indexC, type);

    HashMap<unsigned, unsigned >::ConstIterator it = viewBatchesIndexes_.Find(key);
    if (it == viewBatchesIndexes_.End())
        return 0;

    return &viewBatchesTable_[it->second_];
}

unsigned ObjectTiled::GetMaterialIndex(Material* material, int index)
{
    unsigned indexM = M_MAX_UNSIGNED;
    if (index != -1)
    {
        indexM = index;
        if (indexM+1 > viewMaterialsTable_.Size())
            viewMaterialsTable_.Resize(indexM+1);

        viewMaterialsTable_[indexM] = material;
        return indexM;
    }

    // Find indexM
    for (unsigned i=0; i < viewMaterialsTable_.Size(); ++i)
    {
        if (viewMaterialsTable_[i] == material)
        {
            indexM = i;
            break;
        }
    }

    // if not found, add material
    if (indexM == M_MAX_UNSIGNED)
    {
        indexM = viewMaterialsTable_.Size();
        viewMaterialsTable_.Resize(indexM+1);
        viewMaterialsTable_.Back() = material;
    }
    return indexM;
}

BatchInfo& ObjectTiled::GetChunkBatchInfo(MapObjectType type, int indexZ, int indexV, unsigned indexC, unsigned indexM, int drawOrder)
{
    M_Assert(indexZ < 16 && indexV < 16 && indexM < 16 && indexC < 65536, debugAssertString_.AppendWithFormat("indexZ(%d) >= 16 || indexV(%d) >= 16 || indexM(%u) >= 16 || indexC(%u) >= 65536 !", indexZ, indexV, indexM, indexC).CString());

    unsigned key = GetBatchInfoKey(indexZ, indexV, indexM, indexC, type);
//    URHO3D_LOGINFOF("ObjectTiled() - GetChunkBatchInfo ... %s batchKey=%u", node_->GetName().CString(), key);

    HashMap<unsigned, unsigned >::Iterator it = viewBatchesIndexes_.Find(key);
    if (it == viewBatchesIndexes_.End())
    {
        viewBatchesTable_.Resize(viewBatchesTable_.Size()+1);
        viewBatchesTable_.Back().dirty_ = true;
        it = viewBatchesIndexes_.Insert(Pair<unsigned, unsigned>(key, viewBatchesTable_.Size()-1));
//        URHO3D_LOGINFOF("ObjectTiled() - GetChunkBatchInfo ... %s newbatchKey=%u z=%u v=%u m=%u c=%u t=%d ", node_->GetName().CString(), key, indexZ, indexV, indexM, indexC, type);
    }

    BatchInfo& batchInfo = viewBatchesTable_[it->second_];
    if (batchInfo.dirty_)
    {
        batchInfo.batch_.material_ = viewMaterialsTable_[indexM];
        batchInfo.batch_.drawOrder_ = drawOrder;
//        batchInfo.batch_.vertices_.Clear();
        batchInfo.dirty_ = false;
    }
    return batchInfo;
}

void ObjectTiled::ClearBatchVertices(MapObjectType type, int indexZ, int indexV, unsigned indexC)
{
    for (unsigned indexM=0; indexM<viewMaterialsTable_.Size(); indexM++)
    {
        HashMap<unsigned, unsigned >::ConstIterator it = viewBatchesIndexes_.Find(GetBatchInfoKey(indexZ, indexV, indexM, indexC, type));
        if (it == viewBatchesIndexes_.End())
            continue;

        BatchInfo& batchInfo = viewBatchesTable_[it->second_];
        if (batchInfo.dirty_)
        {
//            URHO3D_LOGERRORF("... type=%d indexZ=%d indexV=%d indexC=%u indexM=%u clear batches ... batcheinfo=%u ...",
//                              type, indexZ, indexV, indexC, indexM, &batchInfo);
            batchInfo.localpositions_.Clear();
            batchInfo.batch_.vertices_.Clear();
        }
    }
}

bool ObjectTiled::UpdateTiledBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay)
{
    if (TimeOver(timer))
        return false;

    URHO3D_PROFILE(ObjectTile_TileBatches);

    const int currentViewZ = ViewManager::Get()->GetViewZ(indexZ);
    const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(currentViewZ);
    if (indexV >= (int)viewIds.Size())
    {
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGERRORF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d viewIndex=%d > viewIDs Size=%u ... return !",
                         currentViewZ, indexV+1, viewIds.Size());
#endif
        return true;
    }

    if (indexV >= (int)viewIds.Size())
    {
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGERRORF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d viewIndex=%d > viewIDs Size=%u ... return !",
                         currentViewZ, indexV+1, viewIds.Size());
#endif
        return true;
    }

    const int viewID = viewIds[indexV];

    const bool containPlateforms = (indexV == (int)viewIds.Size()-1);
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const ChunkInfo* chinfo = chunkGroup.chinfo_;
    const unsigned numchunks = chunkGroup.numChunks_;

    const int height = GetHeight();
    const int width = GetWidth();
    const float twidth = chinfo->tileWidth_;
    const float theight = chinfo->tileHeight_;
    const Vector2 center((float)width * hotspot_.x_, (float)height * hotspot_.y_);

    const FeaturedMap& mask = (indexV == (int)viewIds.Size()-1) ? GetObjectFeatured()->GetFeatureView(viewID) : GetObjectFeatured()->GetMaskedView(indexZ, indexV);
    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TiledMap& tiles = GetObjectSkinned()->GetTiledView(viewID);
    const ConnectedMap& connections = GetObjectSkinned()->GetConnectedView(viewID);

    int viewZ = GetObjectFeatured()->GetViewZ(viewID);
    int layermodifier = viewZ > INNERVIEW ? layermodifier_ : -layermodifier_;

    float viewcolor = (float)viewZ / (float)currentViewZ;
    float backmodifier = viewZ == BACKVIEW ? 0.5f : 1.f;
    Color viewZColor = Color(viewcolor * backmodifier, viewcolor * backmodifier, viewcolor * backmodifier, 1.f);
    Color viewPColor = Color(viewcolor * 0.6f, viewcolor * 0.6f, viewcolor * 0.6f, 1.f);
    unsigned newcolor, lastcolor = 0;

    const int drawOrderZ = ((viewZ + layermodifier) << 20) + (orderInLayer_ << 10);
    const int drawOrderP = ((viewZ + layermodifier + LAYER_PLATEFORMS) << 20) + (orderInLayer_ << 10);
#ifdef RENDER_ROOFS
    const int roofViewModifier = viewZ > INNERVIEW ? 1 : -1;
    const int drawOrderR = ((viewZ + layermodifier + (viewZ > INNERVIEW ? LAYER_FRONTROOFS : LAYER_BACKROOFS)) << 20) + (orderInLayer_ << 10);
    bool isRoof = false;
#endif

    const float zf = 1.f -(viewZ + layermodifier) * PIXEL_SIZE;
    const float zfplatform = 1.f - (viewZ + LAYER_PLATEFORMS) * PIXEL_SIZE;

    float xf, yf;
    unsigned addr;
    unsigned numbatches=0;
    Tile* tile;
    Sprite2D* sprite;
    unsigned char terrainid=255;
    Material *material, *lastmaterial = 0;
    unsigned indexM;
#ifdef ACTIVE_LAYERMATERIALS
    material = GameContext::Get().layerMaterials_[LAYERGROUNDS];
    indexM = GetMaterialIndex(material);
#endif
#ifdef RENDER_ROOFS
    const unsigned indexMRoof = GetMaterialIndex(material, 1);
#endif
    Texture2D *lasttexture=0;
    Vertex2D vertex0, vertex1, vertex2, vertex3;

    int drawOrder = drawOrderZ;
    int indexView = indexV;

#ifdef URHO3D_VULKAN
    unsigned texmode = 0;
#else
    Vector4 texmode;
#endif

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d(indexZ=%d) viewid=%d(%d/%u) chunk=%u/%u ... start at ystart=%d ...",
                    currentViewZ, indexZ, viewID, indexV+1, viewIds.Size(), indexChunks_+1, numchunks, indexStartY_);
#endif

    for (unsigned c = indexChunks_; c < numchunks; c++)
    {
        unsigned indexC = chindex[c];
//        URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... Update chunk=%u index=%u", c, indexC);

        const Chunk& chunk = chinfo->chunks_[indexC];

//        URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... w=%d h=%d Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                            width, height, c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

        // Be sure to check BatchInfo Dirty for each Material to clear batches for the case of "CREATEMODE : remove the last tile in a chunk"
        if (indexStartY_ == 0)
        {
#ifdef RENDER_ROOFS
            ClearBatchVertices(TILE, indexZ, indexV-1, indexC);
            if (!containPlateforms)
                ClearBatchVertices(TILE, indexZ, indexV+1, indexC);
#endif
            ClearBatchVertices(TILE, indexZ, indexV, indexC);
            if (containPlateforms)
                ClearBatchVertices(TILE, indexZ, indexV+1, indexC);

#ifdef DUMP_ERROR_ON_TIMEOVER
            if (timer)
                LogTimeOver(ToString("ObjectTiled() - UpdateTiledBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u Clearing Vertices",
                                     node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks), timer, delay);
#endif
        }

        for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
        {
            addr = y * width + chunk.startCol;
            yf = ((float)(height - y) - center.y_) * theight;

            for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
            {
                FeatureType feat = mask[addr];
                // Skip NoRender (Masked Tiles)
                if (feat == MapFeatureType::NoRender)
                    continue;

                const ConnectIndex& connectIndex = connections[addr];
                if (connectIndex == MapTilesConnectType::Void)
                    continue;

                tile = tiles[addr];
                if (tile->GetDimensions() < TILE_RENDER)
                    continue;

                if (containPlateforms)
                {
#ifdef RENDER_PLATEFORMS
                    if (feat == MapFeatureType::RoomPlateForm)
                    {
                        if (drawOrder != drawOrderP || tile->GetTerrain() != terrainid)
                            newcolor = (viewPColor*atlas_->GetTerrain(tile->GetTerrain()).GetColor()).ToUInt();
                        terrainid = tile->GetTerrain();
                        drawOrder = drawOrderP;
                        indexView = indexV + 1;
                        isRoof = false;
                    }
                    else
                    {
                        if (drawOrder != drawOrderZ || tile->GetTerrain() != terrainid)
                            newcolor = (viewZColor*atlas_->GetTerrain(tile->GetTerrain()).GetColor()).ToUInt();
                        terrainid = tile->GetTerrain();
                    #ifdef RENDER_ROOFS
                        isRoof = (feat == MapFeatureType::OuterRoof || feat == MapFeatureType::InnerRoof);
                    #endif
                        drawOrder = isRoof ? drawOrderR : drawOrderZ;
                        indexView = isRoof ? indexV + roofViewModifier : indexV;
                    }
#else
                    if (feat == MapFeatureType::RoomPlateForm)
                        continue;
#endif
                }
                else
                {
                    if (tile->GetTerrain() != terrainid)
                    {
                        terrainid = tile->GetTerrain();
                        newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                    }
#ifdef RENDER_ROOFS
                    isRoof = (feat == MapFeatureType::OuterRoof || feat == MapFeatureType::InnerRoof);
                #elif defined(ACTIVE_DUNGEONROOFS)
                    if (feat == MapFeatureType::OuterRoof || feat == MapFeatureType::InnerRoof)
                        continue;
#endif
                    drawOrder = isRoof ? drawOrderR : drawOrderZ;
                    indexView = isRoof ? indexV + roofViewModifier : indexV;
                }

                sprite = tile->GetSprite();
#ifndef ACTIVE_LAYERMATERIALS
                material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                if (lastmaterial != material)
                {
                    indexM = GetMaterialIndex(material);
                    lastmaterial = material;
                }
#endif
                if (lasttexture != sprite->GetTexture())
                {
                    lasttexture = sprite->GetTexture();
                    SetTextureMode(TXM_UNIT, material->GetTextureUnit(lasttexture), texmode);
                    vertex0.texmode_ = vertex1.texmode_ = vertex2.texmode_ = vertex3.texmode_ = texmode;
                }

                if (newcolor != lastcolor)
                {
                    vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = newcolor;
                    lastcolor = newcolor;
                }

                // get the good source batch considering chunk, material and plateforms
                BatchInfo& batchinfo = GetChunkBatchInfo(TILE, indexZ, indexView, indexC, isRoof ? indexMRoof : indexM, drawOrder);

//                if (indexZ == 1 && indexC == 23)
//                {
//                    URHO3D_LOGERRORF("... indexZ=%d indexV=%d indexC=%u indexM=%u viewID=%s(%d) addr=%u indexView=%d(%d) draworder=%d(%d) feat=%u batchinfo=%u",
//                                     indexZ, indexV, indexC, isRoof ? indexMRoof : indexM, ViewManager::GetViewName(viewID).CString(), viewID, addr, indexView, indexV, drawOrder, drawOrderZ, feat, &batchinfo);
//                }

                const Rect& drawRect = sprite->GetFixedDrawRectangle();
                const Rect& textureRect = sprite->GetFixedTextRectangle();

                xf = ((float)x - center.x_) * twidth;

                Vector<Vector2>& localpositions = batchinfo.localpositions_;
                localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.min_.y_));
                localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.max_.y_));
                localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.max_.y_));
                localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.min_.y_));

                vertex0.position_ = worldTransform * localpositions[localpositions.Size()-4];
                vertex1.position_ = worldTransform * localpositions[localpositions.Size()-3];
                vertex2.position_ = worldTransform * localpositions[localpositions.Size()-2];
                vertex3.position_ = worldTransform * localpositions[localpositions.Size()-1];
#ifdef URHO3D_VULKAN
                vertex0.z_ = vertex1.z_= vertex2.z_ = vertex3.z_ = containPlateforms ? zfplatform : zf;
#else
                vertex0.position_.z_ = vertex1.position_.z_= vertex2.position_.z_ = vertex3.position_.z_ = containPlateforms ? zfplatform : zf;
#endif
                vertex0.uv_ = textureRect.min_;
                vertex1.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
                vertex2.uv_ = textureRect.max_;
                vertex3.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

                Vector<Vertex2D>& vertices = batchinfo.batch_.vertices_;
                vertices.Push(vertex0);
                vertices.Push(vertex1);
                vertices.Push(vertex2);
                vertices.Push(vertex3);
            }

            if (TimeOver(timer))
            {
                indexStartY_ = y - chunk.startRow + 1;
                indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("ObjectTiled() - UpdateTiledBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u break at y=%d => ystart=%d",
                                     node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y, indexStartY_), timer, delay);
#endif
                return false;
            }
        }

        indexStartY_ = 0;
    }
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGERRORF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d view=%d/%u drawOrders=Tile(%u),Plaform(%u) zf=%f ... OK !",
                     currentViewZ, indexV+1, viewIds.Size(), drawOrderZ, drawOrderP, zf);
#endif
    indexChunks_ = numchunks-1;
    return true;
}

inline void ObjectTiled::PushDecalToVertices(int side, unsigned char terrainid, int rand, float xf, float yf, float zf, const Matrix2x3& worldTransform, Vertex2D* vertex, BatchInfo& batchinfo)
{
    Sprite2D* sprite = atlas_->GetDecalSprite(terrainid, side, rand+side);
    const Rect& drawRect = sprite->GetFixedDrawRectangle();
    const Rect& textureRect = sprite->GetFixedTextRectangle();

    Vector<Vector2>& localpositions = batchinfo.localpositions_;
    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.min_.y_));
    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.max_.y_));
    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.max_.y_));
    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.min_.y_));

    vertex[0].position_ = worldTransform * localpositions[localpositions.Size()-4];
    vertex[1].position_ = worldTransform * localpositions[localpositions.Size()-3];
    vertex[2].position_ = worldTransform * localpositions[localpositions.Size()-2];
    vertex[3].position_ = worldTransform * localpositions[localpositions.Size()-1];

#ifdef URHO3D_VULKAN
    for (int i =0; i < 4; i++)
        vertex[i].z_ = zf;
#else
    for (int i =0; i < 4; i++)
        vertex[i].position_.z_ = zf;
#endif

    vertex[0].uv_ = textureRect.min_;
    vertex[1].uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
    vertex[2].uv_ = textureRect.max_;
    vertex[3].uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

    Vector<Vertex2D>& vertices = batchinfo.batch_.vertices_;
    for (int i=0; i<4; i++)
        vertices.Push(vertex[i]);
}

// get the terrain id of a tile
inline unsigned char ObjectTiled::GetTerrainId(const TiledMap& tiles, unsigned addr)
{
    if (!tiles[addr] || tiles[addr] == Tile::EMPTYPTR)
        return 0;

    // first, get the offset depending on the tile dimension
    // second, apply the offset to get the tile with the terrainid
    return tiles[addr+neighborInd[Tile::DimensionNghIndex[tiles[addr]->GetDimensions()]]]->GetTerrain();
}

bool ObjectTiled::UpdateDecalBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay)
{
    if (TimeOver(timer))
        return false;

    URHO3D_PROFILE(ObjectTile_DecalBatches);

    const int currentViewZ = ViewManager::Get()->GetViewZ(indexZ);
    const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(currentViewZ);
    if ((int)viewIds.Size() <= indexV) return true;
    const int viewID = viewIds[indexV];
    if (viewID == NOVIEW) return true;

    const bool containPlateforms = (indexV == (int)viewIds.Size()-1);
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const ChunkInfo* chinfo = chunkGroup.chinfo_;
    const unsigned numchunks = chunkGroup.numChunks_;

    const int height = GetHeight();
    const int width = GetWidth();
    const int size = height*width;
    const float twidth = chinfo->tileWidth_;
    const float theight = chinfo->tileHeight_;
    const Vector2 center((float)width * hotspot_.x_, (float)height * hotspot_.y_);

    const FeaturedMap& mask = (indexV == (int)viewIds.Size()-1) ? GetObjectFeatured()->GetFeatureView(viewID) : GetObjectFeatured()->GetMaskedView(indexZ, indexV);

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TiledMap& tiles = GetObjectSkinned()->GetTiledView(viewID);
    const ConnectedMap& connections = GetObjectSkinned()->GetConnectedView(viewID);
//    const float adjustFactor = 0.8f;

    int viewZ = GetObjectFeatured()->GetViewZ(viewID);
    int layermodifier = viewZ > INNERVIEW ? layermodifier_ : -layermodifier_;

    float viewcolor = (float)viewZ / (float)currentViewZ;
    float backmodifier = viewZ == BACKVIEW ? 0.5f : 1.f;
    Color viewZColor = Color(viewcolor * backmodifier, viewcolor * backmodifier, viewcolor * backmodifier, 1.f);
    Color viewPColor = Color(viewcolor * 0.6f, viewcolor * 0.6f, viewcolor * 0.6f, 1.f);
    unsigned newcolor, lastcolor = 0;

    int drawOrderZ = ((viewZ + layermodifier) << 20) + ((orderInLayer_ + LAYER_DECALS) << 10);
    int drawOrderP = ((viewZ + layermodifier + LAYER_PLATEFORMS) << 20) + ((orderInLayer_ + LAYER_DECALS) << 10);

    const float zf = 1.f -(viewZ + layermodifier) * PIXEL_SIZE;
    const float zfplatform = 1.f - (viewZ + LAYER_PLATEFORMS) * PIXEL_SIZE;

//    if (currentViewZ == INNERVIEW)
//    {
//        drawOrderZ = (viewZ + LAYER_DECALS + INNERVIEWOVERFRONT) << 20;
//        drawOrderP = ((viewZ + INNERVIEWOVERFRONT + LAYER_PLATEFORMS) << 20);
//    }

    float xf, yf;
    unsigned addr;
    unsigned numbatches = 0;
    Tile* tile;
    unsigned char lastterrainid=255, terrainid=0;
    Sprite2D* sprite;
    Material *material, *lastmaterial = 0;
    unsigned indexM;
#ifdef ACTIVE_LAYERMATERIALS
//    material = GameContext::Get().layerMaterials_[viewIds.Back() == viewID ? LAYERFRONTSHAPES : LAYERGROUNDS];

    if (GameContext::Get().gameConfig_.fluidEnabled_)
        material = GameContext::Get().layerMaterials_[viewID == FrontView_ViewId ? LAYERFRONTSHAPES : LAYERGROUNDS];
    else
        material = GameContext::Get().layerMaterials_[LAYERGROUNDS];

    indexM = GetMaterialIndex(material);
#endif
    Texture2D *lasttexture=0;

#ifdef URHO3D_VULKAN
    unsigned texmode = 0;
#else
    Vector4 texmode;
#endif

    Vertex2D vertex[4];

    int drawOrder = drawOrderZ;
    int indexView = indexV;

//    if (chunkGroup.id_ != MapDirection::All && indexChunks_ == 0)
//    {
//        URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... currentViewZ=%d view=%d/%u indexChunks=%u/%u ...",
//                        currentViewZ, indexV+1, viewIds.Size(), indexChunks_+1, numchunks);
//        DumpData(&connections[0], -1, 2, width, height);
//    }

    for (unsigned c = indexChunks_; c < numchunks; c++)
    {
        unsigned indexC = chindex[c];
        const Chunk& chunk = chinfo->chunks_[indexC];

//        URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                            c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

        // Be sure to check BatchInfo Dirty for each Material to clear batches for the case of "CREATEMODE : remove the last tile in a chunk"
        if (indexStartY_ == 0)
        {
            ClearBatchVertices(DECAL, indexZ, indexV, indexC);
            if (containPlateforms)
                ClearBatchVertices(DECAL, indexZ, indexV+1, indexC);

#ifdef DUMP_ERROR_ON_TIMEOVER
            if (timer)
                LogTimeOver(ToString("ObjectTiled() - UpdateDecalBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u Clearing Vertices", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks), timer, delay);
#endif
        }

        for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
        {
            addr = y * width + chunk.startCol;

            yf = ((float)height - y - center.y_) * theight;

            for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
            {
                if (mask[addr] <= MapFeatureType::NoRender)
                    continue;

                const ConnectIndex& connectIndex = connections[addr];
                if (connectIndex == MapTilesConnectType::Void)
                    continue;

                terrainid = GetTerrainId(tiles, addr);

                if (!atlas_->GetTerrain(terrainid).UseDecals())
                    continue;

                if (containPlateforms)
                {
#ifdef RENDER_PLATEFORMS
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                    {
                        if (drawOrder != drawOrderP || terrainid != lastterrainid)
                            newcolor = (viewPColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                        lastterrainid = terrainid;
                        drawOrder = drawOrderP;
                        indexView = indexV + 1;
                    }
                    else
                    {
                        if (drawOrder != drawOrderZ || terrainid != lastterrainid)
                            newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                        lastterrainid = terrainid;
                        drawOrder = drawOrderZ;
                        indexView = indexV;
                    }
#else
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;
#endif
                }
                else
                {
                    if (terrainid != lastterrainid)
                    {
                        lastterrainid = terrainid;
                        newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                    }
                }

                sprite = atlas_->GetDecalSprite(terrainid, 1, addr);
#ifndef ACTIVE_LAYERMATERIALS
                material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                if (lastmaterial != material)
                {
                    indexM = GetMaterialIndex(material);
                    lastmaterial = material;
                }
#endif
                if (lasttexture != sprite->GetTexture())
                {
                    lasttexture = sprite->GetTexture();
                    SetTextureMode(TXM_UNIT, material->GetTextureUnit(lasttexture), texmode);
                    vertex[0].texmode_ = vertex[1].texmode_ = vertex[2].texmode_ = vertex[3].texmode_ = texmode;
                }

                if (newcolor != lastcolor)
                {
                    vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = newcolor;
                    lastcolor = newcolor;
                }

                xf = ((float)x - center.x_) * twidth;

                // get the good source batch considering chunk, material and plateforms
                BatchInfo& batchinfo = GetChunkBatchInfo(DECAL, indexZ, indexView, indexC, indexM, drawOrder);

                if ((connectIndex & LeftSide) == 0 || (x > 0 && mask[addr-1] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                    PushDecalToVertices(LeftSide, terrainid, addr, xf, yf-0.5f*theight, containPlateforms ? zfplatform : zf, worldTransform, vertex, batchinfo);

                if ((connectIndex & RightSide) == 0 || (x < width-1 && mask[addr+1] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                    PushDecalToVertices(RightSide, terrainid, addr, xf+twidth, yf-0.5f*theight, containPlateforms ? zfplatform : zf, worldTransform, vertex, batchinfo);

                if ((connectIndex & TopSide) == 0 || (y > 0 && mask[addr-width] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                    PushDecalToVertices(TopSide, terrainid, addr, xf+0.5f*twidth, yf, containPlateforms ? zfplatform : zf, worldTransform, vertex, batchinfo);

                if ((connectIndex & BottomSide) == 0 || (y < height-1 && mask[addr+width] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                    PushDecalToVertices(BottomSide, terrainid, addr, xf+0.5f*twidth, yf-1.f*theight, containPlateforms ? zfplatform : zf, worldTransform, vertex, batchinfo);
            }

            if (TimeOver(timer))
            {
                indexStartY_ = y - chunk.startRow + 1;
                indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("ObjectTiled() - UpdateDecalBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u y=%d", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y), timer, delay);
#endif
                return false;
            }
        }

        indexStartY_ = 0;
    }
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... currentViewZ=%d view=%d/%u drawOrders=%u,%u ... OK !", currentViewZ, indexV+1, viewIds.Size(), drawOrder, drawOrderP);
#endif
    indexChunks_ = numchunks-1;
    return true;
}

bool ObjectTiled::UpdateSewingBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay)
{
    if (TimeOver(timer))
        return false;

    URHO3D_PROFILE(ObjectTile_SewingBatches);

    const int currentViewZ = ViewManager::Get()->GetViewZ(indexZ);
    const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(currentViewZ);
    if ((int)viewIds.Size() <= indexV) return true;
    const int viewID = viewIds[indexV];
    if (viewID == NOVIEW) return true;

    const bool containPlateforms = (indexV == (int)viewIds.Size()-1);
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const ChunkInfo* chinfo = chunkGroup.chinfo_;
    const unsigned numchunks = chunkGroup.numChunks_;

    const int height = GetHeight();
    const int width = GetWidth();
    const int size = height*width;
    const float twidth = chinfo->tileWidth_;
    const float theight = chinfo->tileHeight_;
    const Vector2 center((float)width * hotspot_.x_, (float)height * hotspot_.y_);

    const FeaturedMap& mask = (indexV == (int)viewIds.Size()-1) ? GetObjectFeatured()->GetFeatureView(viewID) : GetObjectFeatured()->GetMaskedView(indexZ, indexV);

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TiledMap& tiles = GetObjectSkinned()->GetTiledView(viewID);
    const ConnectedMap& connections = GetObjectSkinned()->GetConnectedView(viewID);

    int viewZ = GetObjectFeatured()->GetViewZ(viewID);
    int layermodifier = viewZ > INNERVIEW ? layermodifier_ : -layermodifier_;

    float viewcolor = (float)viewZ / (float)currentViewZ;
    float backmodifier = viewZ == BACKVIEW ? 0.5f : 1.f;
    Color viewSColor = Color(viewcolor * backmodifier, viewcolor * backmodifier, viewcolor * backmodifier, 0.65f);
    unsigned newcolor, lastcolor = 0;

    int drawOrderP = ((viewZ + layermodifier + LAYER_PLATEFORMS) << 20) + ((orderInLayer_ + LAYER_SEWINGS) << 10);
//    if (currentViewZ == INNERVIEW)
//        drawOrderP = ((viewZ + LAYER_PLATEFORMS + INNERVIEWOVERFRONT) << 20) + 1;
    int drawOrderS = ((viewZ + layermodifier) << 20) + ((orderInLayer_ + LAYER_SEWINGS) << 10);
//    if (currentViewZ == INNERVIEW)
//        drawOrderS = ((viewZ + INNERVIEWOVERFRONT) << 20) + 1;
    int drawOrderSewing = drawOrderS;
    int indexView = indexV;
    unsigned indexM;

    float xf, yf;
    unsigned addr;
    Tile* tile;
    Sprite2D* sprite;
    unsigned char lastterrainid=255, terrainid1=0, terrainid2=0;
    Material *material, *lastmaterial = 0;
#ifdef ACTIVE_LAYERMATERIALS
    material = GameContext::Get().layerMaterials_[LAYERGROUNDS];
    indexM = GetMaterialIndex(material);
#endif
    Vertex2D vertex[4];
    BatchInfo* batchinfo = 0;
    Texture2D *lasttexture=0;

#ifdef URHO3D_VULKAN
    unsigned texmode = 0;
#else
    Vector4 texmode;
#endif

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... map=%s chgrp=%d currentViewZ=%d view=%s(z=%d) (%d/%u) ...", map_->GetMapPoint().ToString().CString(), chunkGroup.id_, currentViewZ, mapViewsNames[viewID], viewZ, indexV+1, viewIds.Size());
#endif

    for (unsigned c = indexChunks_; c < numchunks; c++)
    {
        unsigned indexC = chindex[c];
        const Chunk& chunk = chinfo->chunks_[indexC];
//        URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                            c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

        // Be sure to check BatchInfo Dirty for each Material to clear batches for the case of "CREATEMODE : remove the last tile in a chunk"
        if (indexStartY_ == 0)
        {
            ClearBatchVertices(SEWING, indexZ, indexV, indexC);
            if (containPlateforms)
                ClearBatchVertices(SEWING, indexZ, indexV+1, indexC);

#ifdef DUMP_ERROR_ON_TIMEOVER
            if (timer)
                LogTimeOver(ToString("ObjectTiled() - UpdateSewingBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u Clearing Vertices", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks), timer, delay);
#endif
        }

        for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
        {
            addr = y * width + chunk.startCol;

            yf = ((float)height - y - center.y_) * theight;

            for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
            {
                if (mask[addr] <= MapFeatureType::NoRender)
                    continue;

                const ConnectIndex& connectIndex = connections[addr];
                if (connectIndex == MapTilesConnectType::Void)
                    continue;

                terrainid1 = GetTerrainId(tiles, addr);

                if (!atlas_->GetTerrain(terrainid1).UseDecals())
                    continue;

                if (containPlateforms)
                {
#ifdef RENDER_PLATEFORMS
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                    {
                        if (drawOrderSewing != drawOrderP || terrainid1 != lastterrainid)
                            newcolor = (viewSColor*atlas_->GetTerrain(terrainid1).GetColor()).ToUInt();
                        lastterrainid = terrainid1;
                        drawOrderSewing = drawOrderP;
                        indexView = indexV + 1;
                    }
                    else
                    {
                        if (drawOrderSewing != drawOrderS || terrainid1 != lastterrainid)
                            newcolor = (viewSColor*atlas_->GetTerrain(terrainid1).GetColor()).ToUInt();
                        lastterrainid = terrainid1;
                        drawOrderSewing = drawOrderS;
                        indexView = indexV;
                    }
#else
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;
#endif
                }
                else
                {
                    if (drawOrderSewing != drawOrderS || terrainid1 != lastterrainid)
                        newcolor = (viewSColor*atlas_->GetTerrain(terrainid1).GetColor()).ToUInt();
                    if (terrainid1 != lastterrainid)
                    {
                        lastterrainid = terrainid1;
                        drawOrderSewing = drawOrderS;
                        indexView = indexV;
                    }
                }

                sprite = atlas_->GetDecalSprite(terrainid1, 1, addr);
#ifndef ACTIVE_LAYERMATERIALS
                material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                if (lastmaterial != material)
                {
                    indexM = GetMaterialIndex(material);
                    lastmaterial = material;
                }
#endif
                if (lasttexture != sprite->GetTexture())
                {
                    lasttexture = sprite->GetTexture();
                    SetTextureMode(TXM_UNIT, material->GetTextureUnit(lasttexture), texmode);
                    vertex[0].texmode_ = vertex[1].texmode_ = vertex[2].texmode_ = vertex[3].texmode_ = texmode;
                }

                if (newcolor != lastcolor)
                {
                    vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = newcolor;
                    lastcolor = newcolor;
                }

                xf = ((float)x - center.x_) * twidth;

                batchinfo = 0;

                if ((connectIndex & LeftSide) != 0)
                {
                    if (x == 0)
                    {
                        // check if viewid is also in connectmap (just check the num of views : cave(3)/dungeon(5)
                        // for example : if cmap is cavetype and map is dungeontype and if viewid=3-backview (there no backview in cave)
                        //               then in this case, terrainid2 equal 0 and sewing is made.
                        Map* cmap = map_->GetConnectedMap(MapDirection::West);
                        terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), addr + width - 1) : 0;
                    }
                    else
                    {
                        terrainid2 = GetTerrainId(tiles, addr-1);
                    }

                    if (terrainid1 > terrainid2)
                    {
                        if (!batchinfo)
                            batchinfo = &GetChunkBatchInfo(SEWING, indexZ, indexView, indexC, indexM, drawOrderSewing);
                        PushDecalToVertices(LeftSide, terrainid1, addr, xf, yf-0.5f*theight, 1.f, worldTransform, vertex, *batchinfo);
                    }
                }

                if ((connectIndex & RightSide) != 0)
                {
                    if (x == width-1)
                    {
                        Map* cmap = map_->GetConnectedMap(MapDirection::East);
                        terrainid2 = cmap && viewID < cmap->GetNumViews() ?  GetTerrainId(cmap->GetTiledView(viewID), addr - width + 1) : 0;
                    }
                    else
                    {
                        terrainid2 = GetTerrainId(tiles, addr+1);
                    }

                    if (terrainid1 > terrainid2)
                    {
                        if (!batchinfo)
                            batchinfo = &GetChunkBatchInfo(SEWING, indexZ, indexView, indexC, indexM, drawOrderSewing);
                        PushDecalToVertices(RightSide, terrainid1, addr, xf+twidth, yf-0.5f*theight, 1.f, worldTransform, vertex, *batchinfo);
                    }
                }

                if ((connectIndex & TopSide) != 0)
                {
                    if (y == 0)
                    {
                        Map* cmap = map_->GetConnectedMap(MapDirection::North);
                        terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), width * (height-1) + x) : 0;
                    }
                    else
                    {
                        terrainid2 = GetTerrainId(tiles, addr-width);
                    }

                    if (terrainid1 > terrainid2)
                    {
                        if (!batchinfo)
                            batchinfo = &GetChunkBatchInfo(SEWING, indexZ, indexView, indexC, indexM, drawOrderSewing);
                        PushDecalToVertices(TopSide, terrainid1, addr, xf+0.5f*twidth, yf, 1.f, worldTransform, vertex, *batchinfo);
                    }
                }

                if ((connectIndex & BottomSide) != 0)
                {
                    if (y == height-1)
                    {
                        Map* cmap = map_->GetConnectedMap(MapDirection::South);
                        terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), x) : 0;
                    }
                    else
                    {
                        terrainid2 = GetTerrainId(tiles, addr+width);
                    }

                    if (terrainid1 > terrainid2)
                    {
                        if (!batchinfo)
                            batchinfo = &GetChunkBatchInfo(SEWING, indexZ, indexView, indexC, indexM, drawOrderSewing);
                        PushDecalToVertices(BottomSide, terrainid1, addr, xf+0.5f*twidth, yf-theight, 1.f, worldTransform, vertex, *batchinfo);
                    }
                }
            }

            if (y+1 < chunk.endRow && TimeOver(timer))
            {
                indexStartY_ = y - chunk.startRow + 1;
                indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("ObjectTiled() - UpdateSewingBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u y=%d", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y), timer, delay);
#endif
                return false;
            }
        }

        indexStartY_ = 0;
    }
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... currentViewZ=%d view=%d/%u ... OK !", currentViewZ, indexV+1, viewIds.Size());
#endif
    indexChunks_ = numchunks-1;
    return true;
}

#else

bool ObjectTiled::UpdateTiledBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay)
{
    if (TimeOver(timer))
        return false;

    URHO3D_PROFILE(ObjectTile_TileBatches);

    const int currentViewZ = ViewManager::Get()->GetViewZ(indexZ);
    const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(currentViewZ);
    if (indexV >= (int)viewIds.Size())
    {
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGERRORF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d viewIndex=%d > viewIDs Size=%u ...",
                         currentViewZ, indexV+1, viewIds.Size());
#endif
        return true;
    }

    const int viewID = viewIds[indexV];
    if (viewID == NOVIEW)
    {
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGERRORF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d view=NOVIEW ...", currentViewZ);
#endif
        return true;
    }

    const bool containPlateforms = (indexV == (int)viewIds.Size()-1);
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const ChunkInfo* chinfo = chunkGroup.chinfo_;
    const unsigned numchunks = chunkGroup.numChunks_;

    const int height = GetHeight();
    const int width = GetWidth();
    const float twidth = chinfo->tileWidth_;
    const float theight = chinfo->tileHeight_;
    const Vector2 center((float)width * hotspot_.x_, (float)height * hotspot_.y_);

    const FeaturedMap& mask = (indexV == (int)viewIds.Size()-1) ? GetObjectFeatured()->GetFeatureView(viewID) : GetObjectFeatured()->GetMaskedView(indexZ, indexV);
    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TiledMap& tiles = GetObjectSkinned()->GetTiledView(viewID);
    const ConnectedMap& connections = GetObjectSkinned()->GetConnectedView(viewID);

    int viewZ = GetObjectFeatured()->GetViewZ(viewID);
    int layermodifier = viewZ > INNERVIEW ? layermodifier_ : -layermodifier_;

    float viewcolor = (float)viewZ / (float)currentViewZ;
    Color viewZColor = Color(viewcolor, viewcolor, viewcolor, 1.f);
    unsigned newcolor, lastcolor = 0;

    int drawOrderZ = ((viewZ + layermodifier) << 20) + (orderInLayer_ << 10);

    float xf, yf;
    unsigned addr;
    Tile* tile;
    Sprite2D* sprite;
    unsigned char terrainid=255;
    Material *material;
    Vertex2D vertex0, vertex1, vertex2, vertex3;

    int drawOrder = drawOrderZ;
    int indexView = indexV;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d(indexZ=%d) viewid=%d(%d/%u) chunk=%u/%u ... start at ystart=%d ...",
                    currentViewZ, indexZ, viewID, indexV+1, viewIds.Size(), indexChunks_+1, numchunks, indexStartY_);
#endif

    /// the view doesn't contain plateforms
    if (!containPlateforms)
    {
        for (unsigned c = indexChunks_; c < numchunks; c++)
        {
            unsigned indexC = chindex[c];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... Update chunk=%u index=%u", c, indexC);

            const Chunk& chunk = chinfo->chunks_[indexC];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                                c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, indexC);
//            chunkBatch.Clear();

            for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
            {
                addr = y * width + chunk.startCol;
                yf = ((float)height - y - center.y_) * theight;

                for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
                {
                    // Skip NoRender (Masked Tiles)
                    if (mask[addr] == MapFeatureType::NoRender)
                        continue;

                    const ConnectIndex& connectIndex = connections[addr];
                    if (connectIndex == MapTilesConnectType::Void)
                        continue;

                    tile = tiles[addr];
                    if (tile->GetDimensions() < TILE_RENDER)
                        continue;

                    sprite = tile->GetSprite();
                    material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                    if (!material)
                        continue;

                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;

                    if (tile->GetTerrain() != terrainid)
                    {
                        terrainid = tile->GetTerrain();
                        newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt() ;
                    }

                    if (newcolor != lastcolor)
                    {
                        vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = newcolor;
                        lastcolor = newcolor;
                    }

                    const Rect& drawRect = sprite->GetFixedDrawRectangle();
                    const Rect& textureRect = sprite->GetFixedTextRectangle();

                    xf = ((float)x - center.x_) * twidth;

                    unsigned index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrder);
                    Vector<Vector2>& localpositions = chunkBatch.localpositions_[index];
                    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.min_.y_));
                    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.max_.y_));
                    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.max_.y_));
                    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.min_.y_));

                    vertex0.position_ = worldTransform * localpositions[localpositions.Size()-4];
                    vertex1.position_ = worldTransform * localpositions[localpositions.Size()-3];
                    vertex2.position_ = worldTransform * localpositions[localpositions.Size()-2];
                    vertex3.position_ = worldTransform * localpositions[localpositions.Size()-1];

                    vertex0.uv_ = textureRect.min_;
                    vertex1.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
                    vertex2.uv_ = textureRect.max_;
                    vertex3.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

                    Vector<Vertex2D>& vertices = chunkBatch.batches_[index].vertices_;
                    vertices.Push(vertex0);
                    vertices.Push(vertex1);
                    vertices.Push(vertex2);
                    vertices.Push(vertex3);
                }

                if (TimeOver(timer))
                {
                    indexStartY_ = y - chunk.startRow + 1;
                    indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("ObjectTiled() - UpdateTiledBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u break at y=%d => ystart=%d",
                                         node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y, indexStartY_), timer, delay);
#endif
                    return false;
                }
            }

            indexStartY_ = 0;
        }
    }
#ifdef RENDER_PLATEFORMS
    /// the view contains plateforms
    else
    {
        Color viewPColor = Color(viewcolor * 0.8f, viewcolor * 0.7f, viewcolor * 0.6f, 1.f);
        int drawOrderP = ((viewZ + layermodifier + LAYER_PLATEFORMS) << 20) + (orderInLayer_ << 10);

        for (unsigned c = indexChunks_; c < numchunks; c++)
        {
            unsigned indexC = chindex[c];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... Update chunk=%u index=%u", c, indexC);

            const Chunk& chunk = chinfo->chunks_[indexC];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                                c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, indexC);
//            chunkBatch.Clear();

            for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
            {
                addr = y * width + chunk.startCol;
                yf = ((float)height - y - center.y_) * theight;

                for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
                {
                    // Skip NoRender (Masked Tiles)
                    if (mask[addr] == MapFeatureType::NoRender)
                        continue;

                    const ConnectIndex& connectIndex = connections[addr];
                    if (connectIndex == MapTilesConnectType::Void)
                        continue;

                    tile = tiles[addr];
                    if (tile->GetDimensions() < TILE_RENDER)
                        continue;

                    sprite = tile->GetSprite();
                    material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                    if (!material)
                        continue;

                    if (tile->GetTerrain() != terrainid)
                        terrainid = tile->GetTerrain();

                    /// plateforms
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                    {
                        newcolor = (viewPColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                        drawOrder = drawOrderP;
                        indexView = indexV + 1;
                    }
                    else
                    {
                        newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                        drawOrder = drawOrderZ;
                        indexView = indexV;
                    }

                    if (newcolor != lastcolor)
                    {
                        vertex0.color_ = vertex1.color_ = vertex2.color_ = vertex3.color_ = newcolor;
                        lastcolor = newcolor;
                    }

                    const Rect& drawRect = sprite->GetFixedDrawRectangle();
                    const Rect& textureRect = sprite->GetFixedTextRectangle();

                    xf = ((float)x - center.x_) * twidth;

                    unsigned index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrder);
                    Vector<Vector2>& localpositions = chunkBatch.localpositions_[index];
                    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.min_.y_));
                    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.max_.y_));
                    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.max_.y_));
                    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.min_.y_));

                    vertex0.position_ = worldTransform * localpositions[localpositions.Size()-4];
                    vertex1.position_ = worldTransform * localpositions[localpositions.Size()-3];
                    vertex2.position_ = worldTransform * localpositions[localpositions.Size()-2];
                    vertex3.position_ = worldTransform * localpositions[localpositions.Size()-1];

                    vertex0.uv_ = textureRect.min_;
                    vertex1.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
                    vertex2.uv_ = textureRect.max_;
                    vertex3.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

                    Vector<Vertex2D>& vertices = chunkBatch.batches_[index].vertices_;
                    vertices.Push(vertex0);
                    vertices.Push(vertex1);
                    vertices.Push(vertex2);
                    vertices.Push(vertex3);
                }

                if (TimeOver(timer))
                {
                    indexStartY_ = y - chunk.startRow + 1;
                    indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("ObjectTiled() - UpdateTiledBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u break at y=%d => ystart=%d",
                                         node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y, indexStartY_), timer, delay);
#endif
                    return false;
                }
            }

            indexStartY_ = 0;
        }
    }
#endif

//#ifdef DUMP_OBJECTTILED_UPDATEINFOS
//    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiledBatches ... currentViewZ=%d view=%d/%u drawOrders=%u ... OK !", currentViewZ, indexV+1, viewIds.Size(), drawOrder);
//#endif
    indexChunks_ = numchunks-1;
    return true;
}

// get the terrain id of a tile
inline unsigned char ObjectTiled::GetTerrainId(const TiledMap& tiles, unsigned addr)
{
    // first, get the offset depending on the tile dimension
    // second, apply the offset to get the tile with the terrainid
    return tiles[addr] ? tiles[addr+neighborInd[Tile::DimensionNghIndex[tiles[addr]->GetDimensions()]]]->GetTerrain() : 0;
}

inline void ObjectTiled::PushDecalToVertices(int side, unsigned char terrainid, int rand, float xf, float yf, const Matrix2x3& worldTransform, Vertex2D* vertex, ChunkBatch& chunkbatch, unsigned index)
{
    Sprite2D* sprite = atlas_->GetDecalSprite(terrainid, side, rand+side);
    const Rect& drawRect = sprite->GetFixedDrawRectangle();
    const Rect& textureRect = sprite->GetFixedTextRectangle();

    Vector<Vector2>& localpositions = chunkbatch.localpositions_[index];
    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.min_.y_));
    localpositions.Push(Vector2(xf + drawRect.min_.x_, yf + drawRect.max_.y_));
    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.max_.y_));
    localpositions.Push(Vector2(xf + drawRect.max_.x_, yf + drawRect.min_.y_));

    vertex[0].position_ = worldTransform * localpositions[localpositions.Size()-4];
    vertex[1].position_ = worldTransform * localpositions[localpositions.Size()-3];
    vertex[2].position_ = worldTransform * localpositions[localpositions.Size()-2];
    vertex[3].position_ = worldTransform * localpositions[localpositions.Size()-1];

    vertex[0].uv_ = textureRect.min_;
    vertex[1].uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
    vertex[2].uv_ = textureRect.max_;
    vertex[3].uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);

    Vector<Vertex2D>& vertices = chunkbatch.batches_[index].vertices_;
    for (int i=0; i<4; i++)
        vertices.Push(vertex[i]);
}

bool ObjectTiled::UpdateDecalBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay)
{
    if (TimeOver(timer))
        return false;

    URHO3D_PROFILE(ObjectTile_DecalBatches);

    const int currentViewZ = ViewManager::Get()->GetViewZ(indexZ);
    const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(currentViewZ);
    if ((int)viewIds.Size() <= indexV) return true;
    const int viewID = viewIds[indexV];
    if (viewID == NOVIEW) return true;

    const bool containPlateforms = (indexV == (int)viewIds.Size()-1);
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const ChunkInfo* chinfo = chunkGroup.chinfo_;
    const unsigned numchunks = chunkGroup.numChunks_;

    const int height = GetHeight();
    const int width = GetWidth();
    const int size = height*width;
    const float twidth = chinfo->tileWidth_;
    const float theight = chinfo->tileHeight_;
    const Vector2 center((float)width * hotspot_.x_, (float)height * hotspot_.y_);

    const FeaturedMap& mask = (indexV == (int)viewIds.Size()-1) ? GetObjectFeatured()->GetFeatureView(viewID) : GetObjectFeatured()->GetMaskedView(indexZ, indexV);

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TiledMap& tiles = GetObjectSkinned()->GetTiledView(viewID);
    const ConnectedMap& connections = GetObjectSkinned()->GetConnectedView(viewID);

    int viewZ = GetObjectFeatured()->GetViewZ(viewID);
    int layermodifier = viewZ > INNERVIEW ? layermodifier_ : -layermodifier_;

    float viewcolor = (float)viewZ / (float)currentViewZ;
    Color viewZColor = Color(viewcolor, viewcolor, viewcolor, 1.f);
    unsigned newcolor, lastcolor = 0;

    int drawOrderZ = ((viewZ + layermodifier) << 20) + ((orderInLayer_ + LAYER_DECALS) << 10);

    float xf, yf;
    unsigned addr;
    Tile* tile;
    unsigned char lastterrainid=255, terrainid=0;
    Sprite2D* sprite;
    Material *material;
    Vertex2D vertex[4];

    int drawOrder = drawOrderZ;
    int indexView = indexV;

//    URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... currentViewZ=%d(indexZ=%d) viewid=%d(%d/%u) chunk=%u/%u ...",
//                      currentViewZ, indexZ, viewID, indexV+1, viewIds.Size(), indexChunks_+1, numchunks);

    /// the view doesn't contain plateforms
    if (!containPlateforms)
    {
        for (unsigned c = indexChunks_; c < numchunks; c++)
        {
            unsigned indexC = chindex[c];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... Update chunk=%u index=%u", c, indexC);

            const Chunk& chunk = chinfo->chunks_[indexC];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                                c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, indexC);

            for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
            {
                addr = y * width + chunk.startCol;

                yf = ((float)height - y - center.y_) * theight;

                for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
                {
                    // Skip NoRender and NoDecal

                    if (mask[addr] <= MapFeatureType::NoRender || mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;

                    const ConnectIndex& connectIndex = connections[addr];
                    if (connectIndex == MapTilesConnectType::Void)
                        continue;

                    terrainid = GetTerrainId(tiles, addr);

                    if (!atlas_->GetTerrain(terrainid).UseDecals())
                        continue;

                    sprite = atlas_->GetDecalSprite(terrainid, 1, addr);
                    material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                    if (!material)
                        continue;

                    if (terrainid != lastterrainid)
                    {
                        lastterrainid = terrainid;
                        newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                    }

                    if (newcolor != lastcolor)
                    {
                        vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = newcolor;
                        lastcolor = newcolor;
                    }

                    xf = ((float)x - center.x_) * twidth;

                    // get the good source batch considering chunk, material and plateforms
                    unsigned index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrder);

                    if ((connectIndex & LeftSide) == 0 || (x > 0 && mask[addr-1] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(LeftSide, terrainid, addr, xf, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);

                    if ((connectIndex & RightSide) == 0 || (x < width-1 && mask[addr+1] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(RightSide, terrainid, addr, xf+twidth, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);

                    if ((connectIndex & TopSide) == 0 || (y > 0 && mask[addr-width] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(TopSide, terrainid, addr, xf+0.5f*twidth, yf, worldTransform, vertex, chunkBatch, index);

                    if ((connectIndex & BottomSide) == 0 || (y < height-1 && mask[addr+width] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(BottomSide, terrainid, addr, xf+0.5f*twidth, yf-1.f*theight, worldTransform, vertex, chunkBatch, index);
                }

                if (TimeOver(timer))
                {
                    indexStartY_ = y - chunk.startRow + 1;
                    indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("ObjectTiled() - UpdateDecalBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u y=%d", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y), timer, delay);
#endif
                    return false;
                }
            }

            indexStartY_ = 0;
        }
    }
#ifdef RENDER_PLATEFORMS
    /// the view contains plateforms
    else
    {
        Color viewPColor = Color(viewcolor * 0.8f, viewcolor * 0.7f, viewcolor * 0.6f, 1.f);
        int drawOrderP = ((viewZ + layermodifier + LAYER_PLATEFORMS) << 20) + ((orderInLayer_ + LAYER_DECALS) << 10);

        for (unsigned c = indexChunks_; c < numchunks; c++)
        {
            unsigned indexC = chindex[c];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... Update chunk=%u index=%u", c, indexC);

            const Chunk& chunk = chinfo->chunks_[indexC];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                                c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, indexC);

            for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
            {
                addr = y * width + chunk.startCol;

                yf = ((float)height - y - center.y_) * theight;

                for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
                {
                    // Skip NoRender and NoDecal

                    if (mask[addr] <= MapFeatureType::NoRender)
                        continue;

                    const ConnectIndex& connectIndex = connections[addr];
                    if (connectIndex == MapTilesConnectType::Void)
                        continue;

                    terrainid = GetTerrainId(tiles, addr);

                    if (!atlas_->GetTerrain(terrainid).UseDecals())
                        continue;

                    sprite = atlas_->GetDecalSprite(terrainid, 1, addr);
                    material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                    if (!material)
                        continue;

                    /// plateforms
#ifdef RENDER_PLATEFORMS
                    {
                        if (mask[addr] == MapFeatureType::RoomPlateForm)
                        {
                            if (drawOrder != drawOrderP || terrainid != lastterrainid)
                                newcolor = (viewPColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                            lastterrainid = terrainid;
                            drawOrder = drawOrderP;
                            indexView = indexV + 1;
                        }
                        else
                        {
                            if (drawOrder != drawOrderZ || terrainid != lastterrainid)
                                newcolor = (viewZColor*atlas_->GetTerrain(terrainid).GetColor()).ToUInt();
                            lastterrainid = terrainid;
                            drawOrder = drawOrderZ;
                            indexView = indexV;
                        }
                    }
#else
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;
#endif

                    if (newcolor != lastcolor)
                    {
                        vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = newcolor;
                        lastcolor = newcolor;
                    }

                    xf = ((float)x - center.x_) * twidth;

                    // get the good source batch considering chunk, material and plateforms
                    unsigned index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrder);

                    if ((connectIndex & LeftSide) == 0 || (x > 0 && mask[addr-1] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(LeftSide, terrainid, addr, xf, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);

                    if ((connectIndex & RightSide) == 0 || (x < width-1 && mask[addr+1] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(RightSide, terrainid, addr, xf+twidth, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);

                    if ((connectIndex & TopSide) == 0 || (y > 0 && mask[addr-width] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(TopSide, terrainid, addr, xf+0.5f*twidth, yf, worldTransform, vertex, chunkBatch, index);

                    if ((connectIndex & BottomSide) == 0 || (y < height-1 && mask[addr+width] == MapFeatureType::RoomPlateForm && mask[addr] != MapFeatureType::RoomPlateForm))
                        PushDecalToVertices(BottomSide, terrainid, addr, xf+0.5f*twidth, yf-1.f*theight, worldTransform, vertex, chunkBatch, index);
                }

                if (TimeOver(timer))
                {
                    indexStartY_ = y - chunk.startRow + 1;
                    indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("ObjectTiled() - UpdateDecalBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u y=%d", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y), timer, delay);
#endif
                    return false;
                }
            }

            indexStartY_ = 0;
        }
    }
#endif

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateDecalBatches ... currentViewZ=%d view=%d/%u drawOrders=%u ... OK !", currentViewZ, indexV+1, viewIds.Size(), drawOrder);
#endif
    indexChunks_ = numchunks-1;
    return true;
}

bool ObjectTiled::UpdateSewingBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay)
{
    URHO3D_PROFILE(ObjectTile_SewingBatches);

    const int currentViewZ = ViewManager::Get()->GetViewZ(indexZ);
    const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(currentViewZ);
    if ((int)viewIds.Size() <= indexV) return true;
    const int viewID = viewIds[indexV];
    if (viewID == NOVIEW) return true;

    const bool containPlateforms = (indexV == (int)viewIds.Size()-1);
    const PODVector<unsigned>& chindex = chunkGroup.chIndexes_;
    const ChunkInfo* chinfo = chunkGroup.chinfo_;
    const unsigned numchunks = chunkGroup.numChunks_;

    const int height = GetHeight();
    const int width = GetWidth();
    const int size = height*width;
    const float twidth = chinfo->tileWidth_;
    const float theight = chinfo->tileHeight_;
    const Vector2 center((float)width * hotspot_.x_, (float)height * hotspot_.y_);

    const FeaturedMap& mask = (indexV == (int)viewIds.Size()-1) ? GetObjectFeatured()->GetFeatureView(viewID) : GetObjectFeatured()->GetMaskedView(indexZ, indexV);

    const Matrix2x3& worldTransform = node_->GetWorldTransform2D();
    const TiledMap& tiles = GetObjectSkinned()->GetTiledView(viewID);
    const ConnectedMap& connections = GetObjectSkinned()->GetConnectedView(viewID);
    const float adjustFactor = 0.8f;

    int viewZ = GetObjectFeatured()->GetViewZ(viewID);
    int layermodifier = viewZ > INNERVIEW ? layermodifier_ : -layermodifier_;

    float viewcolor = (float)viewZ / (float)currentViewZ;
    Color viewSColor = Color(viewcolor, viewcolor, viewcolor, 0.65f);
    unsigned newcolor, lastcolor = 0;

    int drawOrderS = ((viewZ + layermodifier) << 20) + ((orderInLayer_ + LAYER_SEWINGS) << 10);
//    if (currentViewZ == INNERVIEW)
//        drawOrderS = ((viewZ + INNERVIEWOVERFRONT) << 20) + 1;

    int drawOrderSewing = drawOrderS;
    int indexView = indexV;

    float xf, yf;
    unsigned addr;
    Tile* tile;
    Sprite2D* sprite;
    unsigned char lastterrainid=255, terrainid1=0, terrainid2=0;
    Material *material;
    Vertex2D vertex[4];
    Vector<Vertex2D>* vertices = 0;

//    URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... currentViewZ=%d(indexZ=%d) viewid=%d(%d/%u) chunk=%u/%u ...",
//                      currentViewZ, indexZ, viewID, indexV+1, viewIds.Size(), indexChunks_+1, numchunks);

    /// the view doesn't contain plateforms
    if (!containPlateforms)
    {
        for (unsigned c = indexChunks_; c < numchunks; c++)
        {
            unsigned indexC = chindex[c];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... Update chunk=%u index=%u", c, indexC);

            const Chunk& chunk = chinfo->chunks_[indexC];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                                c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, indexC);

            for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
            {
                addr = y * width + chunk.startCol;

                yf = ((float)height - y - center.y_) * theight;

                for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
                {
                    // Skip NoRender and NoDecal

                    if (mask[addr] <= MapFeatureType::NoRender || mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;

                    const ConnectIndex& connectIndex = connections[addr];
                    if (connectIndex == MapTilesConnectType::Void)
                        continue;

                    terrainid1 = GetTerrainId(tiles, addr);

                    if (!atlas_->GetTerrain(terrainid1).UseDecals())
                        continue;

                    sprite = atlas_->GetDecalSprite(terrainid1, 1, addr);
                    material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                    if (!material)
                        continue;

                    if (drawOrderSewing != drawOrderS || terrainid1 != lastterrainid)
                        newcolor = (viewSColor*atlas_->GetTerrain(terrainid1).GetColor()).ToUInt();
                    if (terrainid1 != lastterrainid)
                    {
                        lastterrainid = terrainid1;
                        drawOrderSewing = drawOrderS;
                        indexView = indexV;
                    }

                    if (newcolor != lastcolor)
                    {
                        vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = newcolor;
                        lastcolor = newcolor;
                    }

                    xf = ((float)x - center.x_) * twidth;

                    int index = -1;

                    if ((connectIndex & LeftSide) != 0)
                    {
                        if (x == 0)
                        {
                            // check if viewid is also in connectmap (just check the num of views : cave(3)/dungeon(5)
                            // for example : if cmap is cavetype and map is dungeontype and if viewid=3-backview (there no backview in cave)
                            //               then in this case, terrainid2 equal 0 and sewing is made.
                            Map* cmap = map_->GetConnectedMap(MapDirection::West);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), addr + width - 1) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr-1);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(LeftSide, terrainid1, addr, xf, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);
                        }
                    }

                    if ((connectIndex & RightSide) != 0)
                    {
                        if (x == width-1)
                        {
                            Map* cmap = map_->GetConnectedMap(MapDirection::East);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ?  GetTerrainId(cmap->GetTiledView(viewID), addr - width + 1) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr+1);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(RightSide, terrainid1, addr, xf+twidth, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);
                        }
                    }

                    if ((connectIndex & TopSide) != 0)
                    {
                        if (y == 0)
                        {
                            Map* cmap = map_->GetConnectedMap(MapDirection::North);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), width * (height-1) + x) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr-width);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(TopSide, terrainid1, addr, xf+0.5f*twidth, yf, worldTransform, vertex, chunkBatch, index);
                        }
                    }

                    if ((connectIndex & BottomSide) != 0)
                    {
                        if (y == height-1)
                        {
                            Map* cmap = map_->GetConnectedMap(MapDirection::South);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), x) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr+width);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(BottomSide, terrainid1, addr, xf+0.5f*twidth, yf-theight, worldTransform, vertex, chunkBatch, index);
                        }
                    }
                }

                if (y+1 < chunk.endRow && TimeOver(timer))
                {
                    indexStartY_ = y - chunk.startRow + 1;
                    indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("ObjectTiled() - UpdateSewingBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u y=%d", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y), timer, delay);
#endif
                    return false;
                }
            }

            indexStartY_ = 0;
        }
    }
#ifdef RENDER_PLATEFORMS
    /// the view contains plateforms
    else
    {
        int drawOrderP = ((viewZ + layermodifier + LAYER_PLATEFORMS) << 20) + ((orderInLayer_ + LAYER_SEWINGS) << 10);
//	    if (currentViewZ == INNERVIEW)
//	        drawOrderP = ((viewZ + LAYER_PLATEFORMS + INNERVIEWOVERFRONT) << 20) + 1;
        for (unsigned c = indexChunks_; c < numchunks; c++)
        {
            unsigned indexC = chindex[c];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... Update chunk=%u index=%u", c, indexC);

            const Chunk& chunk = chinfo->chunks_[indexC];
//            URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... Update chunk=%u chunkrow=%d to %d chunkcol=%d to %d...",
//                                c, chunk.startRow, chunk.endRow, chunk.startCol, chunk.endCol);

            ChunkBatch& chunkBatch = GetChunkBatch(indexZ, indexC);

            for (int y=chunk.startRow + indexStartY_; y < chunk.endRow; y++)
            {
                addr = y * width + chunk.startCol;

                yf = ((float)height - y - center.y_) * theight;

                for (int x=chunk.startCol; x < chunk.endCol; x++,addr++)
                {
                    // Skip NoRender and NoDecal

                    if (mask[addr] <= MapFeatureType::NoRender)
                        continue;

                    const ConnectIndex& connectIndex = connections[addr];
                    if (connectIndex == MapTilesConnectType::Void)
                        continue;

                    terrainid1 = GetTerrainId(tiles, addr);

                    if (!atlas_->GetTerrain(terrainid1).UseDecals())
                        continue;

                    sprite = atlas_->GetDecalSprite(terrainid1, 1, addr);
                    material = renderer_->GetMaterial(sprite->GetTexture(), BLEND_ALPHA);
                    if (!material)
                        continue;

                    /// plateforms
#ifdef RENDER_PLATEFORMS
                    {
                        if (mask[addr] == MapFeatureType::RoomPlateForm)
                        {
                            if (drawOrderSewing != drawOrderP || terrainid1 != lastterrainid)
                                newcolor = (viewSColor*atlas_->GetTerrain(terrainid1).GetColor()).ToUInt();
                            lastterrainid = terrainid1;
                            drawOrderSewing = drawOrderP;
                            indexView = indexV + 1;
                        }
                        else
                        {
                            if (drawOrderSewing != drawOrderS || terrainid1 != lastterrainid)
                                newcolor = (viewSColor*atlas_->GetTerrain(terrainid1).GetColor()).ToUInt();
                            lastterrainid = terrainid1;
                            drawOrderSewing = drawOrderS;
                            indexView = indexV;
                        }
                    }
#else
                    if (mask[addr] == MapFeatureType::RoomPlateForm)
                        continue;
#endif

                    if (newcolor != lastcolor)
                    {
                        vertex[0].color_ = vertex[1].color_ = vertex[2].color_ = vertex[3].color_ = newcolor;
                        lastcolor = newcolor;
                    }

                    xf = ((float)x - center.x_) * twidth;

                    int index = -1;

                    if ((connectIndex & LeftSide) != 0)
                    {
                        if (x == 0)
                        {
                            // check if viewid is also in connectmap (just check the num of views : cave(3)/dungeon(5)
                            // for example : if cmap is cavetype and map is dungeontype and if viewid=3-backview (there no backview in cave)
                            //               then in this case, terrainid2 equal 0 and sewing is made.
                            Map* cmap = map_->GetConnectedMap(MapDirection::West);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), addr + width - 1) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr-1);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(LeftSide, terrainid1, addr, xf, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);
                        }
                    }

                    if ((connectIndex & RightSide) != 0)
                    {
                        if (x == width-1)
                        {
                            Map* cmap = map_->GetConnectedMap(MapDirection::East);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ?  GetTerrainId(cmap->GetTiledView(viewID), addr - width + 1) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr+1);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(RightSide, terrainid1, addr, xf+twidth, yf-0.5f*theight, worldTransform, vertex, chunkBatch, index);
                        }
                    }

                    if ((connectIndex & TopSide) != 0)
                    {
                        if (y == 0)
                        {
                            Map* cmap = map_->GetConnectedMap(MapDirection::North);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), width * (height-1) + x) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr-width);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(TopSide, terrainid1, addr, xf+0.5f*twidth, yf, worldTransform, vertex, chunkBatch, index);
                        }
                    }

                    if ((connectIndex & BottomSide) != 0)
                    {
                        if (y == height-1)
                        {
                            Map* cmap = map_->GetConnectedMap(MapDirection::South);
                            terrainid2 = cmap && viewID < cmap->GetNumViews() ? GetTerrainId(cmap->GetTiledView(viewID), x) : 0;
                        }
                        else
                        {
                            terrainid2 = GetTerrainId(tiles, addr+width);
                        }

                        if (terrainid1 > terrainid2)
                        {
                            if (index == -1)
                                index = chunkBatch.GetSourceBatchIndex(indexView, material, drawOrderSewing);
                            PushDecalToVertices(BottomSide, terrainid1, addr, xf+0.5f*twidth, yf-theight, worldTransform, vertex, chunkBatch, index);
                        }
                    }
                }

                if (y+1 < chunk.endRow && TimeOver(timer))
                {
                    indexStartY_ = y - chunk.startRow + 1;
                    indexChunks_ = c;
#ifdef DUMP_ERROR_ON_TIMEOVER
                    LogTimeOver(ToString("ObjectTiled() - UpdateSewingBatches : map=%s ... currentViewZ=%d view=%d/%u chunks=%u/%u y=%d", node_->GetName().CString(), currentViewZ, indexV+1, viewIds.Size(), indexChunks_, numchunks, y), timer, delay);
#endif
                    return false;
                }
            }

            indexStartY_ = 0;
        }
    }
#endif

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateSewingBatches ... currentViewZ=%d view=%d/%u drawOrders=%u ... OK !", currentViewZ, indexV+1, viewIds.Size(), drawOrderS);
#endif
    indexChunks_ = numchunks-1;
    return true;
}

#endif

bool ObjectTiled::UpdateTiles(const ChunkGroup& chunkGroup, int indexZ, HiresTimer* timer, const long long& delay)
{
    if (indexGrpToSet_ == 0)
    {
#ifndef USE_CHUNKBATCH
        // No TileDrawing => Clear Vertices
        if (!tilesEnable_)
            ClearChunkBatches(chunkGroup, TILE, indexZ);
        // No DecalDrawing => Clear Vertices
        if (!decalsEnable_)
        {
            ClearChunkBatches(chunkGroup, DECAL, indexZ);
            ClearChunkBatches(chunkGroup, SEWING, indexZ);
        }
#else
        for (unsigned i=0; i < chunkGroup.numChunks_; i++)
            GetChunkBatch(indexZ, chunkGroup.chIndexes_[i]).Clear();
#endif

        indexGrpToSet_++;
        indexVToSet_ = 0;
        indexChunks_ = 0;
        indexStartY_ = 0;
        if (timer)
            return false;
    }

    if (indexGrpToSet_ == 1)
    {
        if (tilesEnable_)
        {
            for (;;)
            {
                if (indexVToSet_ >= numviews_)
                    break;

                if (!UpdateTiledBatches(chunkGroup, indexZ, indexVToSet_, timer, delay))
                    return false;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
                if (chunkGroup.id_ == -1)
                    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiles ... %s group=%u UpdateTiledBatches indexZ=%d view=%u/%u numChunks=%u/%u ... timer=%d msec",
                                    node_->GetName().CString(), chunkGroup.chIndexes_[0], indexZ, indexVToSet_+1, numviews_, indexChunks_+1,
                                    chunkGroup.numChunks_, timer ? timer->GetUSec(false) / 1000 : 0);
                else
                    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiles ... %s group=%s UpdateTiledBatches indexZ=%d view=%u/%u numChunks=%u/%u ... timer=%d msec",
                                    node_->GetName().CString(), MapDirection::GetName(chunkGroup.id_), indexZ, indexVToSet_+1, numviews_, indexChunks_+1,
                                    chunkGroup.numChunks_, timer ? timer->GetUSec(false) / 1000 : 0);
#endif

                indexVToSet_++;
                indexChunks_ = 0;
                indexStartY_ = 0;
            }
        }

        indexGrpToSet_++;
        indexVToSet_ = 0;
        indexChunks_ = 0;
        indexStartY_ = 0;
        if (timer)
            return false;
    }

    if (indexGrpToSet_ == 2)
    {
#ifdef RENDER_TERRAINS_BORDERS
        if (GetObjectSkinned()->UseDimensionTiles() && decalsEnable_)
        {
            for (;;)
            {
                if (indexVToSet_ >= numviews_)
                    break;

                if (!UpdateDecalBatches(chunkGroup, indexZ, indexVToSet_, timer, delay))
                    return false;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
                if (chunkGroup.id_ == -1)
                    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiles ... %s group=%u UpdateDecalBatches indexZ=%d view=%u/%u numChunks=%u/%u ... timer=%d msec",
                                    node_->GetName().CString(), chunkGroup.chIndexes_[0], indexZ, indexVToSet_+1, numviews_, indexChunks_+1,
                                    chunkGroup.numChunks_, timer ? timer->GetUSec(false) / 1000 : 0);
                else
                    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiles ... %s group=%s UpdateDecalBatches indexZ=%d view=%u/%u numChunks=%u/%u ... timer=%d msec",
                                    node_->GetName().CString(), MapDirection::GetName(chunkGroup.id_), indexZ, indexVToSet_+1, numviews_, indexChunks_+1,
                                    chunkGroup.numChunks_, timer ? timer->GetUSec(false) / 1000 : 0);
#endif

                indexVToSet_++;
                indexChunks_ = 0;
                indexStartY_ = 0;
            }
        }
#endif

        indexGrpToSet_++;
        indexVToSet_ = 0;
        indexChunks_ = 0;
        indexStartY_ = 0;
        if (timer)
            return false;
    }

    if (indexGrpToSet_ == 3)
    {
#ifdef RENDER_TERRAINS_SEWINGS
        if (GetObjectSkinned()->UseDimensionTiles() && decalsEnable_)
        {
            for (;;)
            {
                if (indexVToSet_ >= numviews_)
                    break;

                if (!UpdateSewingBatches(chunkGroup, indexZ, indexVToSet_, timer, delay))
                    return false;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
                if (chunkGroup.id_ == -1)
                    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiles ... %s group=%u UpdateSewingBatches indexZ=%d view=%u/%u numChunks=%u/%u ... timer=%d msec",
                                    node_->GetName().CString(), chunkGroup.chIndexes_[0], indexZ, indexVToSet_+1, numviews_, indexChunks_+1,
                                    chunkGroup.numChunks_, timer ? timer->GetUSec(false) / 1000 : 0);
                else
                    URHO3D_LOGINFOF("ObjectTiled() - UpdateTiles ... %s group=%s UpdateSewingBatches indexZ=%d view=%u/%u numChunks=%u/%u ... timer=%d msec",
                                    node_->GetName().CString(), MapDirection::GetName(chunkGroup.id_), indexZ, indexVToSet_+1, numviews_, indexChunks_+1,
                                    chunkGroup.numChunks_, timer ? timer->GetUSec(false) / 1000 : 0);
#endif

                indexVToSet_++;
                indexChunks_ = 0;
            }
        }
#endif // RENDER_TERRAINS_SEWINGS

        indexGrpToSet_ = 0;
        indexVToSet_ = 0;
        indexChunks_ = 0;
        indexStartY_ = 0;
        return true;
    }

    return false;
}

bool ObjectTiled::UpdateChunkGroup(const ChunkGroup& chunkGroup, HiresTimer* timer, const long long& delay)
{
    if (indexToSet_ == 0)
    {
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGINFOF("ObjectTiled() - UpdateChunkGroup %s ...", node_->GetName().CString());
#endif
        GetObjectFeatured()->indexToSet_ = 0;
        indexZToSet_ = 0;
        indexToSet_++;
    }
    if (indexToSet_ == 1)
    {
        if (!GetObjectFeatured()->UpdateMaskViews(chunkGroup.GetTileGroup(), timer, delay, skinData_->GetSkin() ? skinData_->GetSkin()->neighborMode_ : Connected0))
            return false;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        if (chunkGroup.id_ == -1)
            URHO3D_LOGINFOF("ObjectTiled() - UpdateChunkGroup %s group=%u ... UpdateMaskViews indexZ=%d timer=%d msec ...", node_->GetName().CString(),
                            chunkGroup.chIndexes_[0], indexZToSet_, timer ? timer->GetUSec(false)/1000 : 0);
        else
            URHO3D_LOGINFOF("ObjectTiled() - UpdateChunkGroup %s group=%s ... UpdateMaskViews indexZ=%d timer=%d msec ...", node_->GetName().CString(),
                            MapDirection::GetName(chunkGroup.id_), indexZToSet_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
        indexToSet_++;
    }
    if (indexToSet_ == 2)
    {
        SetChunkBatchesDirty(chunkGroup);
        indexZToSet_ = 0;
        indexChunks_ = 0;
        indexStartY_ = 0;
        indexGrpToSet_ = 0;
        indexToSet_++;
    }
    if (indexToSet_ == 3)
    {
        for (;;)
        {
            if (indexZToSet_ >= ViewManager::Get()->GetNumViewZ())
                break;

            if (!UpdateTiles(chunkGroup, indexZToSet_, timer, delay))
                return false;

            indexChunks_ = 0;
            indexStartY_ = 0;
            indexZToSet_++;
        }
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        if (chunkGroup.id_ == -1)
            URHO3D_LOGINFOF("ObjectTiled() - UpdateChunkGroup %s group=%u ... UpdateMaskViews indexZ=%d timer=%d msec ... OK !", node_->GetName().CString(),
                            chunkGroup.chIndexes_[0], indexZToSet_, timer ? timer->GetUSec(false)/1000 : 0);
        else
            URHO3D_LOGINFOF("ObjectTiled() - UpdateChunkGroup %s group=%s ... UpdateMaskViews indexZ=%d timer=%d msec ... OK !", node_->GetName().CString(),
                            MapDirection::GetName(chunkGroup.id_), indexZToSet_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
    }
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateChunkGroup %s ... OK !", node_->GetName().CString());
#endif
    indexToSet_ = 0;
    return true;
}

bool ObjectTiled::UpdateDirtyChunks(HiresTimer* timer, const long long& delay)
{
    if (!dirtyChunkGroups_.Size())
    {
//        URHO3D_LOGINFOF("ObjectTiled() - UpdateDirtyChunks %s no diry chunks to update !", node_->GetName().CString());
        return true;
    }

    sourceBatchReady_ = false;

    if (!timer)
    {
        while (dirtyChunkGroups_.Size())
        {
            UpdateChunkGroup(*dirtyChunkGroups_.Back(), 0, 0);
            dirtyChunkGroups_.Pop();
            GetObjectSkinned()->indexToSet_ = 0;
        }

        SetDirty();
        sourceBatchReady_ = true;
//        URHO3D_LOGINFOF("ObjectTiled() - UpdateDirtyChunks %s ... OK !", node_->GetName().CString());
        return true;
    }
    else
    {
        const ChunkGroup& dirtychgrp = *dirtyChunkGroups_.Back();

        if (UpdateChunkGroup(dirtychgrp, timer, delay))
        {
            dirtyChunkGroups_.Pop();

            if (!dirtyChunkGroups_.Size())
            {
                GetObjectSkinned()->indexToSet_ = 0;

                SetDirty();

                sourceBatchReady_ = true;
//                URHO3D_LOGINFOF("ObjectTiled() - UpdateDirtyChunks %s ... OK !", node_->GetName().CString());
                return true;
            }
        }
    }

    return false;
}

void ObjectTiled::UpdateChunksVisiblity(ViewportRenderData& viewportdata)
{
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateChunksVisiblity ... %s viewport=%d viewrect=%s ...",
                    node_->GetName().CString(), viewportdata.viewport_, viewportdata.visibleRect_.ToString().CString());
#endif

    // update Visible Chunks group
    chinfo_->Convert2ChunkGroup(viewportdata.visibleRect_, viewportdata.chunkGroup_);

    // Check the Rect to Render is Enough
    if (!viewportdata.chunkGroup_.numChunks_ || viewportdata.chunkGroup_.rect_.Width() < 1 || viewportdata.chunkGroup_.rect_.Height() < 1)
    {
//        URHO3D_LOGWARNINGF("ObjectTiled() - UpdateChunksVisiblity  .. %s : No Chunks To Render !", node_->GetName().CString());
        return;
    }

    // Hide furnitures outside visible chunks

    // Show furnitures inside visible chunks
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
    URHO3D_LOGINFOF("ObjectTiled() - UpdateChunksVisiblity ... %s viewport=%d viewrect=%s chunkrect=%s numchunks=%u ... OK !",
                    node_->GetName().CString(), viewportdata.viewport_, viewportdata.visibleRect_.ToString().CString(), viewportdata.chunkGroup_.rect_.ToString().CString(), viewportdata.chunkGroup_.numChunks_);
#endif
}

bool ObjectTiled::UpdateViewBatches(int numViewZ, HiresTimer* timer, const long long& delay)
{
//    URHO3D_LOGINFOF("ObjectTiled() - UpdateViewBatches : ... startTimer=%d", timer ? timer->GetUSec(false)/1000 : 0);

    sourceBatchReady_ = false;

    for (;;)
    {
        URHO3D_LOGINFOF("ObjectTiled() - UpdateViewBatches ... indexZ=%d/%d ... timer=%d msec",
                        indexToSet_+1, numViewZ, timer ? timer->GetUSec(false)/1000 : 0);

        if (indexToSet_ >= numViewZ)
            break;

        if (!UpdateTiles(chinfo_->GetDefaultChunkGroup(MapDirection::All), indexToSet_, timer, delay))
            return false;

        indexToSet_++;

        if (timer)
            return false;
    }

    sourceBatchReady_ = true;
    indexToSet_ = 0;
    return true;
}

void ObjectTiled::UpdateViews()
{
    SetChunkBatchesDirty(chinfo_->GetDefaultChunkGroup(MapDirection::All));
    UpdateViewBatches(ViewManager::Get()->GetNumViewZ(), 0, 0);
}

const Vector<SourceBatch2D*>& ObjectTiled::GetSourceBatchesToRender(Camera* camera)
{
    ViewportRenderData& viewportdata = viewportDatas_[camera->GetViewport()];

    if (viewportdata.sourceBatchDirty_)
        UpdateSourceBatchesToRender(viewportdata);

    return viewportdata.sourceBatchesToRender_;
}

void ObjectTiled::UpdateSourceBatchesToRender(ViewportRenderData& viewportdata)
{
    URHO3D_PROFILE(ObjectTile_Update);

    if (!sourceBatchReady_)
    {
//        URHO3D_LOGWARNINGF("ObjectTiled() - UpdateSourceBatchesToRender %s(%u) ... not ready !", node_->GetName().CString(), node_->GetID());
        return;
    }

    if (viewportdata.indexMinView_ < 0)
    {
//        URHO3D_LOGERRORF("ObjectTiled() - UpdateSourceBatchesToRender : %s(%u) ... indexMinView_ < 0 !", node_->GetName().CString(), node_->GetID(), viewportdata.indexMinView_);
        return;
    }

    Vector<SourceBatch2D*>& batchesToRender = viewportdata.sourceBatchesToRender_;
    int indexViewZ = viewportdata.currentViewZindex_;
    if (indexViewZ == -1)
    {
        URHO3D_LOGERRORF("ObjectTiled() - UpdateSourceBatchesToRender : %s(%u) ... indexViewZ=-1 !", node_->GetName().CString(), node_->GetID(), indexViewZ);
        return;
    }
    viewportdata.sourceBatchDirty_ = false;

#ifdef RENDER_VIEWBATCHES
    if (viewportdata.lastNumTiledBatchesToRender_ == 0)
        viewportdata.sourceBatchFeatureDirty_ = true;

#ifndef USE_CHUNKBATCH
    /// No Render and no Fluid Update if no ViewBatches
    if (!viewBatchesTable_.Size())
    {
        batchesToRender.Clear();
        return;
    }
#endif

    if (viewportdata.sourceBatchFeatureDirty_)
    {
        URHO3D_PROFILE(ObjectTile_FeatureDirty);

        batchesToRender.Clear();

        viewportdata.lastNumTiledBatchesToRender_ = 0;
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGINFOF("ObjectTiled() - UpdateSourceBatchesToRender ... %s viewZ=%d(ind=%d) cuttingLevel_=%d rect=%s ...",
                        node_->GetName().CString(), ViewManager::Get()->GetViewZ(indexViewZ), indexViewZ, cuttingLevel_, viewportdata.visibleRect_.ToString().CString());
#endif
        unsigned numChunks = viewportdata.chunkGroup_.numChunks_;
        const PODVector<unsigned>& chindex = viewportdata.chunkGroup_.chIndexes_;

#ifdef USE_CHUNKBATCH
        /// TODO : replace indexMinView_ & indexMaxView_ directly by viewZMin, viewZMax
        const Vector<int>& viewIds = GetObjectFeatured()->GetViewIDs(ViewManager::Get()->GetViewZ(indexViewZ));
        int viewZMin = GetObjectFeatured()->GetViewZ(viewportdata.indexMinView_ < viewIds.Size() ? viewIds[viewportdata.indexMinView_] : viewIds.Back());
        int viewZMax = GetObjectFeatured()->GetViewZ(viewportdata.indexMaxView_ < viewIds.Size() ? viewIds[viewportdata.indexMaxView_] : viewIds.Back());
        int drawOrderMin = viewZMin << 20;
        int drawOrderMax = viewZMax << 20;
        for (unsigned c = 0; c < numChunks; c++)
            viewportdata.lastNumTiledBatchesToRender_ += GetChunkBatch(indexViewZ, chindex[c]).GetSourceBatchesToRender(drawOrderMin, drawOrderMax, batchesToRender);
#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGINFOF("ObjectTiled() - UpdateSourceBatchesToRender ... numChunkBatches=%u batchesToRender.Size=%u ivMin=%d ivMax=%d zMin=%d zMax=%d drawOrderMin=%d drawOrderMax=%d...",
                        numChunks, batchesToRender.Size(), viewportdata.indexMinView_, viewportdata.indexMaxView_, viewZMin, viewZMax, drawOrderMin, drawOrderMax);
#endif
#else

#ifdef DUMP_ERROR_ON_TIMEOVER
        HiresTimer timer;
#endif

        // All Views + Plateform View
//        for (int indexV = viewportdata.indexMinView_; indexV <= viewportdata.indexMaxView_; indexV++)
//        {
//            unsigned basekey = GetBaseBatchInfoKey(indexViewZ, indexV);
//            // Add TILE Batches
//            for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
//            for (unsigned c = 0; c < numChunks; c++)
//            {
//                BatchInfo* binfo = GetChunkBatchInfoBased(TILE, basekey, chindex[c], m);
//                if (!binfo || !binfo->batch_.vertices_.Size())
//                    continue;
//
//                batchesToRender.Push(&binfo->batch_);
////                viewportdata.lastNumTiledBatchesToRender_++;
//            }
//        #ifdef RENDER_TERRAINS_SEWINGS
//            // Add SEWING Batches
//            for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
//            for (unsigned c = 0; c < numChunks; c++)
//            {
//                BatchInfo* binfo = GetChunkBatchInfoBased(SEWING, basekey, chindex[c], m);
//                if (!binfo || !binfo->batch_.vertices_.Size())
//                    continue;
//
//                batchesToRender.Push(&binfo->batch_);
////                viewportdata.lastNumTiledBatchesToRender_++;
//            }
//        #endif
//            // Add DECAL Batches
//            for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
//            for (unsigned c = 0; c < numChunks; c++)
//            {
//                BatchInfo* binfo = GetChunkBatchInfoBased(DECAL, basekey, chindex[c], m);
//                if (!binfo || !binfo->batch_.vertices_.Size())
//                    continue;
//
//                batchesToRender.Push(&binfo->batch_);
////                viewportdata.lastNumTiledBatchesToRender_++;
//            }
//        }

        viewportdata.lastNumTiledBatchesToRender_ = batchesToRender.Size();

        BatchInfo* binfo;
        for (int indexV = viewportdata.indexMinView_; indexV <= viewportdata.indexMaxView_; indexV++)
        {
            unsigned basekey = GetBaseBatchInfoKey(indexViewZ, indexV);

            for (unsigned m = 0; m < viewMaterialsTable_.Size(); m++)
                for (unsigned c = 0; c < numChunks; c++)
                {
                    // Add TILE Batches
                    binfo = GetChunkBatchInfoBased(TILE, basekey, chindex[c], m);
                    if (binfo && binfo->batch_.vertices_.Size())
                        batchesToRender.Push(&binfo->batch_);
#ifdef RENDER_TERRAINS_SEWINGS
                    // Add SEWING Batches
                    binfo = GetChunkBatchInfoBased(SEWING, basekey, chindex[c], m);
                    if (binfo && binfo->batch_.vertices_.Size())
                        batchesToRender.Push(&binfo->batch_);
#endif
                    // Add DECAL Batches
                    binfo = GetChunkBatchInfoBased(DECAL, basekey, chindex[c], m);
                    if (binfo && binfo->batch_.vertices_.Size())
                        batchesToRender.Push(&binfo->batch_);
                }
            viewportdata.lastNumTiledBatchesToRender_ = batchesToRender.Size();
        }

#ifdef DUMP_ERROR_ON_TIMEOVER
        LogTimeOver(ToString("ObjectTiled() - UpdateSourceBatchesToRender : map=%s numBatchesToRender=%u", node_->GetName().CString(), batchesToRender.Size()), &timer);
#endif

#endif
        viewportdata.sourceBatchFeatureDirty_ = false;

#ifdef DUMP_OBJECTTILED_UPDATEINFOS
        URHO3D_LOGERRORF("ObjectTiled() - UpdateSourceBatchesToRender ... %s(%u) viewZ=%u numChunks=%u lastNumTiledBatchesToRender_=%u ... OK !",
                         node_->GetName().CString(), node_->GetID(), ViewManager::Get()->GetViewZ(indexViewZ), numChunks, viewportdata.lastNumTiledBatchesToRender_);
#endif
    }
#endif
}

void ObjectTiled::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!IsEnabledEffective())
    {
        return;
    }

    if (GameContext::Get().gameConfig_.debugObjectTiled_)
    {
        Vector2 scale = node_->GetWorldScale2D();

#ifdef DRAWDEBUG_CHUNKBORDER
        // Draw Chunks BoundingBoxes
        if (isChunked_)
        {
            const ChunkGroup& chunkgroup = viewportDatas_[0].chunkGroup_;
            unsigned numchunks = chunkgroup.numChunks_;
            const PODVector<unsigned>& chIndexes = chunkgroup.chIndexes_;
            for (unsigned c=0; c < numchunks; c++)
            {
                const Chunk& chunk = chinfo_->chunks_[chIndexes[c]];
                debug->AddBoundingBox(chunk.GetBoundingBox().Transformed(node_->GetWorldTransform()), Color::MAGENTA, depthTest);
            }
        }
#endif

#ifdef DRAWDEBUG_FEATURES
        if (isChunked_ && map_->IsEffectiveVisible())
        {
            int viewZ = ViewManager::Get()->GetCurrentViewZ();
            Map* map = map_->GetType() == Map::GetTypeStatic() ? static_cast<Map*>(map_) : 0;
            Node* node = map->GetTagNode();
            if (map && node)
            {
                Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");
                Text3D* text3D;
                const ChunkGroup& chunkgroup = viewportDatas_[0].chunkGroup_;
                unsigned numchunks = chunkgroup.numChunks_;
                const PODVector<unsigned>& chIndexes = chunkgroup.chIndexes_;
                for (unsigned c=0; c < numchunks; c++)
                {
                    const Chunk& chunk = chinfo_->chunks_[chIndexes[c]];

                    String chunkTag = "C_" + String(chIndexes[c]);
                    Node* chunkNode = node->GetChild(chunkTag);
                    if (!chunkNode)
                    {
                        chunkNode = node->CreateChild(chunkTag, LOCAL);
//                        text3D = chunkNode->CreateComponent<Text3D>(LOCAL);
//                        text3D->SetText(chunkTag);
//                        text3D->SetColor(Color::MAGENTA);
//                        text3D->SetFont(font, 50);
//                        text3D->SetOccludee(false);
//                        text3D->SetAlignment(HA_LEFT, VA_TOP);
//                        text3D->SetOnTop(true);
//                        chunkNode->SetWorldPosition(node->GetWorldPosition() + Vector3(chunk.startCol * World2D::GetWorldInfo()->mTileWidth_, (GetHeight() - chunk.startRow) * World2D::GetWorldInfo()->mTileHeight_, 0.f));
                    }

                    for (int y=chunk.startRow; y < chunk.endRow; y++)
                    {
                        for (int x=chunk.startCol; x < chunk.endCol; x++)
                        {
                            unsigned tileindex = map->GetTileIndex(x, y);
                            String tileTag = "T_" + String(tileindex);
                            Node* tagNode = chunkNode->GetChild(tileTag);

                            FeatureType feat;
                            int layerZ = viewZ;
                            if (map->FindSolidTile(tileindex, feat, layerZ))
                            {
                                String text;
                                text.AppendWithFormat("%u@%d", feat, layerZ);

                                if (tagNode)
                                {
                                    text3D = tagNode->GetComponent<Text3D>(LOCAL);
                                    text3D->SetText(text);
                                }
                                else
                                {
                                    tagNode = chunkNode->CreateChild(tileTag, LOCAL);
                                    text3D = tagNode->CreateComponent<Text3D>(LOCAL);
                                    text3D->SetText(text);
                                    text3D->SetColor(Color::GREEN);
                                    text3D->SetFont(font, 8);
                                    text3D->SetOccludee(false);
                                    text3D->SetAlignment(HA_CENTER, VA_CENTER);

                                    tagNode->SetWorldPosition(node->GetWorldPosition() + Vector3((x + 0.5f)* World2D::GetWorldInfo()->mTileWidth_, (GetHeight() - (y+0.5f)) * World2D::GetWorldInfo()->mTileHeight_, 0.f));
                                }
                                tagNode->SetEnabled(true);
                            }
                            else
                            {
                                if (tagNode)
                                    tagNode->SetEnabled(false);
                            }
                        }
                    }
                }
            }
        }
#endif

#ifdef DRAWDEBUG_MAPBORDER
        // Draw BoundingBox
        debug->AddBoundingBox(worldBoundingBox_, Color::YELLOW, depthTest);
#endif

        /// Add Node Position
        debug->AddNode(node_, scale.x_, depthTest);
    }
}

void ObjectTiled::DumpFluidCellAt(unsigned addr) const
{
    ObjectFeatured* featureData = skinData_->GetFeatureData();
    int viewZ = ViewManager::Get()->GetCurrentViewZ(0);
    const Vector<int>& fluidViewIds = featureData->GetFluidViewIDs(viewZ);
    if (fluidViewIds.Size() && fluidViewIds.Front() != NOVIEW)
    {
        int fluidviewid = fluidViewIds.Front();
        const FluidDatas& fluiddata = featureData->GetFluidView(fluidviewid);
        const FluidCell& cell = fluiddata.fluidmap_[addr];
        URHO3D_LOGINFOF("ObjectTiled() - DumpFluidCellAt : currentViewZ=%d fluidviewid=%d =>", viewZ, fluidviewid);
        cell.Dump();
    }
}

void ObjectTiled::Dump() const
{
    URHO3D_LOGINFOF("ObjectTiled() - Dump : node_=%s layer_=%d", node_->GetName().CString(), layer_.x_);
    URHO3D_LOGINFOF("ObjectTiled() - Dump : skinData_=%u featureData_=%u", skinData_.Get(), skinData_->GetFeatureData());
    for (unsigned i=0; i<skinData_->GetFeatureData()->viewZ_.Size(); i++)
    {
        URHO3D_LOGINFOF("ObjectTiled() - Dump : viewId=%u zValue=%d", i, skinData_->GetFeatureData()->viewZ_[i]);
    }
}


#endif
