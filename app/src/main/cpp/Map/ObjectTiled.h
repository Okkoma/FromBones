#pragma once

#include "GameOptions.h"

#ifdef USE_TILERENDERING

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Urho2D/Drawable2D.h>

#include "DefsMap.h"
#include "DefsChunks.h"

#include "ObjectFeatured.h"
#include "ObjectSkinned.h"

#include "GameOptions.h"


class World2DInfo;
class Map;
class ObjectMaped;

const int NUM_MAPOBJECTTYPE = 3;
enum MapObjectType
{
    TILE = 0,
    DECAL,
    SEWING
};

enum
{
    ViewRange_WorldVisibleRect = 0,
    ViewRange_Frustum = 1,
};

#ifdef USE_CHUNKBATCH
/// ChunkBatch can't filter MapObjectType but has less batches
struct ChunkBatch
{
    ChunkBatch()
    {
        Clear();
    }

    void Allocate(Drawable2D* owner, unsigned numbatches);

    void Clear();

    unsigned GetSourceBatchIndex(int indexV, Material* material, int drawOrder);
    SourceBatch2D& GetSourceBatch(int indexV, Material* material, int drawOrder);
    unsigned GetSourceBatchesToRender(int drawOrderMin, int drawOrderMax, Vector<SourceBatch2D*>& batchesToRender);

    void UpdateVerticePositions(Node* node);

    bool dirty_;
    Drawable2D* owner_;
    unsigned counterBatch_;

    Vector<SourceBatch2D> batches_;
    Vector<Vector<Vector2> > localpositions_;
    HashMap<unsigned, unsigned > batchMap_;
};
#else
/// BatchInfo can filter MapObjectType but has more batches
struct BatchInfo
{
    BatchInfo() : dirty_(true) { }

    void Clear()
    {
        dirty_ = true;
        localpositions_.Clear();
        batch_.vertices_.Clear();
        batch_.material_.Reset();
    }

    void UpdateVerticePositions(Node* node);

    bool dirty_;
    Vector<Vector2> localpositions_;
    SourceBatch2D batch_;
    int drawOrder_;
};
#endif


class ObjectTiled : public Drawable2D
{
    URHO3D_OBJECT(ObjectTiled, Drawable2D);

    friend struct ObjectSkinned;
    friend class Map;
    friend class ObjectMaped;

public:
    /// Register object factory. Drawable must be registered first.
    static void RegisterObject(Context* context);
    static void RegisterWorldInfo(World2DInfo* winfo);
    static void SetTilesEnable(bool state);
    static int GetTilesEnable()
    {
        return tilesEnable_;
    }
    static void SetDecalsEnable(bool state);
    static int GetDecalsEnable()
    {
        return decalsEnable_;
    }
    static void SetMaxDrawView(int numViews);
    static int GetMaxDrawView()
    {
        return maxDrawViews_;
    }
    static void SetCuttingLevel(int level);
    static int GetCuttingLevel()
    {
        return cuttingLevel_;
    }
    static TerrainAtlas* GetAtlas()
    {
        return atlas_;
    }
    static void SetViewRangeMode(int mode);
    static int GetViewRangeMode()
    {
        return viewRangeMode_;
    }

private :
    static TerrainAtlas* atlas_;
    static int cuttingLevel_;
    static int maxDrawViews_;
    static bool tilesEnable_, decalsEnable_;
    static int viewRangeMode_;

public :
    /// Construct.
    ObjectTiled();
    ObjectTiled(Context* context);
    ObjectTiled(const ObjectTiled& obj);
    /// Destruct.
    ~ObjectTiled();

    void Resize(int width, int height, unsigned numviews, ChunkInfo* chinfo=0);
    void Clear();

    void SetChunked(unsigned char numx, unsigned char numy);
    void SetChunked(ChunkInfo* chinfo, bool shared=true);
    void SetHotSpot(const Vector2& hotspot);
    void SetDynamic(bool state);
    void SetLayerModifier(int modifier);
    void SetDirty(int viewport=-1);

    void SetCurrentViewZ(int viewport, int viewZindex=0, bool forced=false);
    void SetRenderPaused(int viewport, bool state);

    void MarkChunkGroupDirty(const ChunkGroup& group);

    bool UpdateViewBatches(int numViewZ, HiresTimer* timer, const long long& delay);
    bool UpdateDirtyChunks(HiresTimer* timer, const long long& delay);
    void UpdateViews();
    void UpdateVerticesPositions(Node* node);

    /// NOTE : if problem in fluid check if urho3D drawable2d has this method as virtual
    virtual BoundingBox GetWorldBoundingBox2D();
    /// Return all source batches To Renderer (called by Renderer2D).
    virtual const Vector<SourceBatch2D* >& GetSourceBatchesToRender(Camera* camera);

    const Vector2& GetHotSpot() const
    {
        return hotspot_;
    }
    const unsigned& GetWidth() const;
    const unsigned& GetHeight() const;

    ObjectSkinned* GetObjectSkinned() const
    {
        return skinData_;
    }
    ObjectFeatured* GetObjectFeatured() const
    {
        assert(skinData_);
        return skinData_->GetFeatureData();
    }

    inline unsigned char GetTerrainId(const TiledMap& tiles, unsigned addr);

    bool IsChunked() const
    {
        return isChunked_;
    }
    const ChunkGroup& GetChunkGroup(int viewport=0) const
    {
        return viewportDatas_[viewport].chunkGroup_;
    }

    ChunkInfo* GetChunkInfo() const
    {
        return chinfo_;
    }

    void DumpFluidCellAt(unsigned addr) const;
    void Dump() const;

    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    /// Handle enabled/disabled state change.
    virtual void OnSetEnabled();

    MapBase* map_;

protected:
    /// Handle node set.
    virtual void OnNodeSet(Node* node);
    /// Recalculate the world-space bounding box.
    virtual void OnWorldBoundingBoxUpdate();
    /// Handle draw order changed.
    virtual void OnDrawOrderChanged();
    /// Update source batches.
    virtual void UpdateSourceBatches() { }

private :
    void Init();
    void AllocateChunkBatches();
    void ClearChunksBatches();

    void ApplyCuttingLevel(int viewport=-1);

    void SetChunkBatchesDirty(const ChunkGroup& group);

    /// Solid Batches
#ifdef USE_CHUNKBATCH
    ChunkBatch& GetChunkBatch(int indexZ, unsigned indexC)
    {
        return chunkBatches_[indexZ][indexC];
    }
    inline void PushDecalToVertices(int side, unsigned char terrainid, int rand, float xf, float yf, const Matrix2x3& worldTransform, Vertex2D* vertex, ChunkBatch& chunkbatch, unsigned index);
#else
    void ClearChunkBatches(const ChunkGroup& group, MapObjectType type, int indexZ);
    unsigned GetBaseBatchInfoKey(int indexZ, int indexV) const;
    unsigned GetBatchInfoKey(unsigned baseKey, unsigned indexM, unsigned indexC, MapObjectType type) const;
    unsigned GetBatchInfoKey(int indexZ, int indexV, unsigned indexM, unsigned indexC, MapObjectType type) const;
    unsigned GetMaterialIndex(Material* material, int indexM=-1);
    BatchInfo* GetChunkBatchInfoBased(MapObjectType type, unsigned baseKey, unsigned indexC, unsigned indexM);
    BatchInfo& GetChunkBatchInfo(MapObjectType type, int indexZ, int indexV, unsigned indexC, unsigned indexM, int drawOrder);
    void ClearBatchVertices(MapObjectType type, int indexZ, int indexV, unsigned indexC);
    inline void PushDecalToVertices(int side, unsigned char terrainid, int rand, float xf, float yf, float zf, const Matrix2x3& worldTransform, Vertex2D* vertex, BatchInfo& batchinfo);
#endif
    bool UpdateTiledBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay);
    bool UpdateDecalBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay);
    bool UpdateSewingBatches(const ChunkGroup& chunkGroup, int indexZ, int indexV, HiresTimer* timer, const long long& delay);
    bool UpdateTiles(const ChunkGroup& chunkGroup, int indexZ, HiresTimer* timer, const long long& delay);
    bool UpdateChunkGroup(const ChunkGroup& chunkGroup, HiresTimer* timer, const long long& delay);

    void UpdateSourceBatchesToRender(ViewportRenderData& data);
    void UpdateChunksVisiblity(ViewportRenderData& viewportdata);

    SharedPtr<ObjectSkinned> skinData_;

    short int neighborInd[9];

    ViewportRenderData viewportDatas_[MAX_VIEWPORTS];

    ChunkInfo* chinfo_;
    Vector2 hotspot_;
    bool isDynamic_;
    bool isChunked_;
    bool sharedChunkInfo_;

    List<const ChunkGroup* > dirtyChunkGroups_;

    bool sourceBatchReady_;

    int layermodifier_;
    int enlargeBox_;
    unsigned numviews_;
    unsigned indexGrpToSet_, indexToSet_, indexZToSet_, indexVToSet_, indexChunks_, indexStartY_;

#ifdef USE_CHUNKBATCH
    // indexed by indexZ and indexChunk
    Vector<Vector<ChunkBatch> > chunkBatches_;
#else
    Vector<BatchInfo > viewBatchesTable_;
    Vector<WeakPtr<Material> > viewMaterialsTable_;
    HashMap<unsigned, unsigned > viewBatchesIndexes_;
#endif
};

#endif
