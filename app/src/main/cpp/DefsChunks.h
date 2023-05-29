#pragma once

#include <Urho3D/Urho2D/Drawable2D.h>

using namespace Urho3D;


typedef PODVector<unsigned> TileGroup;

struct ChunkInfo;
class ObjectTiled;

struct Chunk
{
    Chunk() {}
    Chunk(ChunkInfo* ch, int coordx, int coordy);
    const BoundingBox& GetBoundingBox() const
    {
        return boundingBox_;
    }
//    BoundingBox GetBoundingBox() const { return BoundingBox(Vector3(minx, miny, 0.f), Vector3(maxx,maxy,0.f)); }

    int startCol, endCol;
    int startRow, endRow;
//    float minx, miny, maxx, maxy;
    BoundingBox boundingBox_;
};

struct ChunkGroup
{
    void Reset(ChunkInfo* chinfo=0, int id=-1);
    void Resize(ChunkInfo* chinfo, unsigned size, int id=-1);
    void Resize(unsigned size);
    const IntRect& GetRect() const
    {
        return rect_;
    }
    const TileGroup& GetTileGroup() const
    {
        return *tilegroup_;
    }

    // infos for default chunkgroups
    int id_;
    unsigned numChunks_;
    unsigned numChunkX_, numChunkY_;

    // chunk indexes
    PODVector<unsigned> chIndexes_;

    // tilegroup
    TileGroup *tilegroup_;

    // corners in tile coords
    IntRect rect_;
    ChunkInfo* chinfo_;
};

struct ChunkInfo
{
    ChunkInfo() {}
    ChunkInfo(int gridwidth, int gridheight, int numchunkx, int numchunky, float tileWidth, float tileHeight);

    void Set(int gridwidth, int gridheight, int numchunkx, int numchunky, float tileWidth, float tileHeight);

    bool Convert2ChunkRect(const BoundingBox& box, IntRect& chunkedrect, int enlarge=0);
    void Convert2ChunkGroup(const IntRect& chunkedrect, ChunkGroup& chunks);

    unsigned GetChunk(int x, int y) const;
    void GetChunks(const IntRect& chunkedrect, PODVector<unsigned>& chindexes);

    /// GetDefaultChunkGroup : get a default ChunkGroup for borderChunks or AllChunks
    /// id = MapDirection Enum
    const ChunkGroup& GetDefaultChunkGroup(int id) const
    {
        return defaultChunkGroups_[id];
    }
    /// GetChunkGroup : get a default TileGroup for borderTiles
    /// id = MapDirection Enum
    const TileGroup& GetDefaultTileGroup(int id) const
    {
        return defaultTileGroups_[id];
    }
    /// GetUniqueChunkGroup : get a ChunkGroup from a chunk index
    const ChunkGroup& GetUniqueChunkGroup(unsigned id) const
    {
        return uniqueChunkGroups_[id];
    }
    /// GetUniqueTileGroup : get a TileGroup from a chunk index
    const TileGroup& GetUniqueTileGroup(unsigned id) const
    {
        return uniqueTileGroups_[id];
    }

    int sizex_, sizey_;
    int numx_, numy_;
    float tileWidth_, tileHeight_;

    PODVector<Chunk> chunks_;
    Vector<ChunkGroup> defaultChunkGroups_;
    Vector<TileGroup> defaultTileGroups_;
    Vector<ChunkGroup> uniqueChunkGroups_;
    Vector<TileGroup> uniqueTileGroups_;
};


struct ViewportRenderData
{
    void Clear()
    {
        currentViewZindex_ = currentViewID_ = -1;
        indexMinView_ = indexMaxView_ = -1;
        lastNumTiledBatchesToRender_ = 0;
        isDirty_ = sourceBatchDirty_ = sourceBatchFeatureDirty_ = sourceBatchFluidDirty_ = true;
        isPaused_ = false;
        sourceBatchesToRender_.Reserve(64);
    }

    void Set(ObjectTiled* objectTiled, int viewport)
    {
        objectTiled_ = objectTiled;
        viewport_ = viewport;
        Clear();
    }

    ObjectTiled* objectTiled_;

    int viewport_;
    int currentViewZindex_;
    int currentViewID_;
    int indexMinView_, indexMaxView_;

    BoundingBox lastViewBoundingBox_;
    unsigned lastNumTiledBatchesToRender_;
    IntRect visibleRect_, lastVisibleRect_;

    ChunkGroup chunkGroup_;
    bool isDirty_;
    bool sourceBatchDirty_;
    bool sourceBatchFeatureDirty_, sourceBatchFluidDirty_;
    bool isPaused_;

    Vector<SourceBatch2D* > sourceBatchesToRender_;
};
