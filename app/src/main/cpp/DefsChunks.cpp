#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "DefsMap.h"

#include "DefsChunks.h"


/// Chunks

void ChunkGroup::Reset(ChunkInfo* chinfo, int id)
{
    id_ = id;
    chinfo_ = chinfo;
    if (chinfo)
    {
        numChunkX_ = chinfo->numx_ > 0 ? chinfo->numx_ : 1;
        numChunkY_ = chinfo->numy_ > 0 ? chinfo->numy_ : 1;
    }
    else
    {
        numChunkX_ = 1;
        numChunkY_ = 1;
    }

    Resize(numChunkX_ * numChunkY_);

    rect_ = IntRect::ZERO;

//    URHO3D_LOGINFOF("ChunkGroup() - Reset : numChunksX=%u numChunksY=%u", numChunkX_, numChunkY_);
}

void ChunkGroup::Resize(unsigned size)
{
    if (numChunks_ != size || chIndexes_.Size() != size)
    {
        numChunks_ = size;
        chIndexes_.Resize(numChunks_);
        for (unsigned i=0; i < numChunks_; i++)
            chIndexes_[i] = i;

//        URHO3D_LOGINFOF("ChunkGroup() - Resize : numChunks_=%u chIndexes_.Size()=%u", size, chIndexes_.Size());
    }
}

void ChunkGroup::Resize(ChunkInfo* chinfo, unsigned size, int id)
{
    id_ = id;
    chinfo_ = chinfo;
    numChunkX_ = 0;
    numChunkY_ = 0;

    Resize(size);

//    URHO3D_LOGINFOF("ChunkGroup() - Resize : size=%u", numChunks_);
}



Chunk::Chunk(ChunkInfo* ch, int coordx, int coordy)
{
    startCol = coordx * ch->sizex_;
    endCol = startCol + ch->sizex_;
    startRow = coordy * ch->sizey_;
    endRow = startRow + ch->sizey_;

    float minx = startCol * ch->tileWidth_;
    float maxx = endCol * ch->tileWidth_;
    float miny = (ch->numy_ * ch->sizey_ - endRow) * ch->tileHeight_;
    float maxy = (ch->numy_ * ch->sizey_ - startRow) * ch->tileHeight_;

    boundingBox_.Define(Vector3(minx, miny, 0.f), Vector3(maxx, maxy, 0.f));
}

ChunkInfo::ChunkInfo(int gridwidth, int gridheight, int numchunkx, int numchunky, float tileWidth, float tileHeight)
{
    Set(gridwidth, gridheight, numchunkx, numchunky, tileWidth, tileHeight);
}

void ChunkInfo::Set(int gridwidth, int gridheight, int numchunkx, int numchunky, float tileWidth, float tileHeight)
{
    numx_ = numchunkx;
    numy_ = numchunky;

    if (numchunkx * numchunky == 0)
        return;

    if ((numx_ > 1 && gridwidth % numx_ > 0) || (numy_ > 1 && gridheight % numy_ > 0))
    {
        numx_ = 0;
        return;
    }

    sizex_ = gridwidth / numx_;
    sizey_ = gridheight / numy_;
    tileWidth_ = tileWidth;
    tileHeight_ = tileHeight;

    chunks_.Resize(numx_ * numy_);
    uniqueChunkGroups_.Resize(numx_ * numy_);
    uniqueTileGroups_.Resize(numx_ * numy_);

//    URHO3D_LOGINFOF("ChunkInfo() - Set : numChunksX=%u numChunksY=%u chunksize=(%u x %u tiles)",
//                     numx_, numy_, sizex_, sizey_);

    // Set All Chunks
    int i=0;
    for (int y=0; y < numy_; y++)
        for (int x=0; x < numx_ ; x++, i++)
        {
            // Set Chunk
            chunks_[i] = Chunk(this, x, y);

            // Set Unique TileGroup
            TileGroup& group = uniqueTileGroups_[i];
            group.Resize(sizex_*sizey_);
            unsigned offset=y*sizey_*gridwidth + x*sizex_;
            unsigned addr=0;
            for (unsigned j=0; j < sizey_; j++)
                for (unsigned i=0; i < sizex_; i++, addr++)
                    group[addr] = offset + j*gridwidth + i;

            // Set Unique ChunkGroup
            uniqueChunkGroups_[i].Resize(this, 1);
            uniqueChunkGroups_[i].chIndexes_[0] = i;
            uniqueChunkGroups_[i].tilegroup_ = &uniqueTileGroups_[i];
//            URHO3D_LOGINFOF("ChunkInfo() - Set : Chunks_[%u] %u %u => x1=%d, y1=%d, x2=%d, y2=%d bbox=%s",
//                     i, x, y, c.startCol, c.startRow, c.endCol, c.endRow, c.boundingBox_.ToString().CString());
        }

    // Set Default TileGroups with MapDirection Indexes
    {
        unsigned numx(numx_*sizex_);
        unsigned numy(numy_*sizey_);
        defaultTileGroups_.Resize(7);
        {
            // MapDirection::North => Get All Tiles At TopBorder
            TileGroup& group = defaultTileGroups_[MapDirection::North];
            group.Resize(numx);
            for (unsigned i=0; i < numx; i++)
                group[i] = i;
        }
        {
            // MapDirection::South => Get All Tiles At BottomBorder
            TileGroup& group = defaultTileGroups_[MapDirection::South];
            group.Resize(numx);
            unsigned offset = (numy-1) * numx;
            for (unsigned i=0; i < numx; i++)
                group[i] = i + offset;
        }
        {
            // MapDirection::East => Get All Tiles At RightBorder
            TileGroup& group = defaultTileGroups_[MapDirection::East];
            group.Resize(numy);
            for (unsigned j=0; j < numy; j++)
                group[j] = j * numx + numx - 1;
        }
        {
            // MapDirection::West => Get All Tiles At LeftBorder
            TileGroup& group = defaultTileGroups_[MapDirection::West];
            group.Resize(numy);
            for (unsigned j=0; j < numy; j++)
                group[j] = j * numx;
        }
        {
            // MapDirection::NoBorder => Get All NoBorder Tiles
            TileGroup& group = defaultTileGroups_[MapDirection::NoBorders];
            group.Resize((numy-1) * (numx-1));
            unsigned addr=0;
            for (unsigned j=1; j < numy-1; j++)
                for (unsigned i=1; i < numx-1; i++,addr++)
                    group[addr] = j*numx + i;
        }
        {
            // MapDirection::AllBorders => Get All Border Tiles
            TileGroup& group = defaultTileGroups_[MapDirection::AllBorders];
            group.Resize(2*(numx+numy)-4);
            unsigned addr=0;
            unsigned offset = (numy-1) * numx;
            for (unsigned i=0; i < numx; i++,addr++)
                group[addr] = i;
            for (unsigned i=0; i < numx; i++,addr++)
                group[addr] = i + offset;
            for (unsigned j=1; j < numy-1; j++,addr++)
                group[addr] = j * numx + numx - 1;
            for (unsigned j=1; j < numy-1; j++,addr++)
                group[addr] = j * numx;
        }
        {
            // MapDirection::Alls => Get All Tiles
            TileGroup& group = defaultTileGroups_[MapDirection::All];
            unsigned size = numx*numy;
            group.Resize(size);
            for (unsigned i = 0; i < size; i++)
                group[i] = i;
        }
    }

    defaultChunkGroups_.Resize(7);

    // if one big chunk
    if (numx_*numy_ == 1)
    {
        for (unsigned i=0; i < 7; ++i)
        {
            ChunkGroup& group = defaultChunkGroups_[i];
            group.Reset(this, i);
            group.tilegroup_ = &defaultTileGroups_[i];
        }
    }
    else
        // Set Default ChunkGroups with MapDirection Indexes
    {
        unsigned numx(numx_);
        unsigned numy(numy_);

        {
            // MapDirection::North => Get All Chunk At TopBorder
            ChunkGroup& group = defaultChunkGroups_[MapDirection::North];
            group.Resize(this, numx, MapDirection::North);
            group.tilegroup_ = &defaultTileGroups_[MapDirection::North];
            for (unsigned i=0; i < numx; i++)
                group.chIndexes_[i] = i;
        }
        {
            // MapDirection::South => Get All Chunk At BottomBorder
            ChunkGroup& group = defaultChunkGroups_[MapDirection::South];
            group.Resize(this, numx, MapDirection::South);
            group.tilegroup_ = &defaultTileGroups_[MapDirection::South];
            unsigned offset = (numy-1) * numx;
            for (unsigned i=0; i < numx; i++)
                group.chIndexes_[i] = i + offset;
        }
        {
            // MapDirection::East => Get All Chunk At RightBorder
            ChunkGroup& group = defaultChunkGroups_[MapDirection::East];
            group.tilegroup_ = &defaultTileGroups_[MapDirection::East];
            group.Resize(this, numy, MapDirection::East);
            for (unsigned j=0; j < numy; j++)
                group.chIndexes_[j] = j * numx + numx - 1;
        }
        {
            // MapDirection::West => Get All Chunk At LeftBorder
            ChunkGroup& group = defaultChunkGroups_[MapDirection::West];
            group.tilegroup_ = &defaultTileGroups_[MapDirection::West];
            group.Resize(this, numy, MapDirection::West);
            for (unsigned j=0; j < numy; j++)
                group.chIndexes_[j] = j * numx;
        }
        {
            // MapDirection::NoBorder => Get All Chunk But no BorderChunk
            ChunkGroup& group = defaultChunkGroups_[MapDirection::NoBorders];
            group.Resize(this, (numy-1) * (numx-1), MapDirection::NoBorders);
            group.tilegroup_ = &defaultTileGroups_[MapDirection::NoBorders];
            unsigned addr=0;
            for (unsigned j=1; j < numy-1; j++)
                for (unsigned i=1; i < numx-1; i++,addr++)
                    group.chIndexes_[addr] = j*numx + i;
        }
        {
            // MapDirection::AllBorders => Get All Chunk Borders
            ChunkGroup& group = defaultChunkGroups_[MapDirection::AllBorders];
            group.Resize(this, 2*numx + 2*numy - 4, MapDirection::AllBorders);
            group.tilegroup_ = &defaultTileGroups_[MapDirection::AllBorders];
            unsigned addr=0;
            unsigned offset = (numy-1) * numx;
            if (numx > 1)
            {
                for (unsigned i=0; i < numx; i++, addr++)
                    group.chIndexes_[addr] = i;

                if (numy > 1)
                    for (unsigned i=0; i < numx; i++, addr++)
                        group.chIndexes_[addr] = i + offset;
            }
            if (numy > 1)
            {
                for (unsigned j=1; j < numy-1; j++, addr++)
                    group.chIndexes_[addr] = j * numx;

                if (numx > 1)
                    for (unsigned j=1; j < numy-1; j++, addr++)
                        group.chIndexes_[addr] = j * numx + numx - 1;
            }
        }
        {
            // MapDirection::All => Get All Chunks
            ChunkGroup& group = defaultChunkGroups_[MapDirection::All];
            group.Reset(this, MapDirection::All);
            group.tilegroup_ = &defaultTileGroups_[MapDirection::All];
        }
    }

}

bool ChunkInfo::Convert2ChunkRect(const BoundingBox& box, IntRect& chunkedrect, int enlarge)
{
    if (!box.Defined())
        return false;

    // Enlarged version on border
    // find the chunk for each corner of the enlarged box
    chunkedrect.left_   = Max((int)floor(box.min_.x_ / tileWidth_ / sizex_) - enlarge, 0);
    chunkedrect.right_  = Min((int)floor(box.max_.x_ / tileWidth_ / sizex_) + enlarge, numx_-1);

    // Inversion IntRect(top is min Int, bottom is max Int) for array
    chunkedrect.top_    = Max(numy_ - (int)floor(box.max_.y_ / tileHeight_ / sizey_) - 1 - enlarge, 0);
    chunkedrect.bottom_ = Min(numy_ - (int)floor(box.min_.y_ / tileHeight_ / sizey_) - 1 + enlarge, numy_-1);

//    URHO3D_LOGINFOF("ChunkInfo() - Convert2ChunkRect : box=%s tileWidth_=%f sizex=%d => rect=%s", box.ToString().CString(), tileWidth_, sizex_, chunkedrect.ToString().CString());
    return true;
}

void ChunkInfo::Convert2ChunkGroup(const IntRect& chunkedrect, ChunkGroup& chunks)
{
    unsigned count = 0;
    for (int y= chunkedrect.top_; y <= chunkedrect.bottom_; y++)
        for (int x=chunkedrect.left_; x <= chunkedrect.right_; x++, count++)
            chunks.chIndexes_[count] = (unsigned)(y * numx_ + x);

    if (chunkedrect.right_ - chunkedrect.left_ >= 0)
        chunks.numChunkX_ = (unsigned)(chunkedrect.right_ - chunkedrect.left_ + 1);
    if (chunkedrect.bottom_ - chunkedrect.top_ >= 0)
        chunks.numChunkY_ = (unsigned)(chunkedrect.bottom_ - chunkedrect.top_ + 1);

    count = chunks.numChunkX_*chunks.numChunkY_;

//	URHO3D_LOGINFOF("ChunkInfo() - Convert2ChunkGroup : rect=%s ... numChunks=%u", chunkedrect.ToString().CString(), chunks.numChunks_);

    if (chunks_.Size() < count)
        URHO3D_LOGERRORF("ChunkInfo() - Convert2ChunkGroup : rect=%s ... numChunksInRect=%u > chunksSize=%u", chunkedrect.ToString().CString(), count, chunks_.Size());

    chunks.numChunks_ = Min(chunks_.Size(), count);

    // find tiles rect
    if (chunks.numChunks_)
    {
        unsigned cstart = chunks.chIndexes_[0];
        unsigned cend = chunks.chIndexes_[chunks.numChunks_ - 1];
        chunks.rect_.left_ = chunks_[cstart].startCol;
        chunks.rect_.top_ = chunks_[cstart].startRow;
        chunks.rect_.right_ = chunks_[cend].endCol;
        chunks.rect_.bottom_ = chunks_[cend].endRow;
    }
}

unsigned ChunkInfo::GetChunk(int x, int y) const
{
    assert(x < sizex_*numx_ && y < sizey_*numy_);

    return (unsigned)((y/sizey_) * numx_ + (x/sizex_));
}


void ChunkInfo::GetChunks(const IntRect& chunkedrect, PODVector<unsigned>& chindexes)
{
    unsigned count = 0;
    chindexes.Resize((chunkedrect.bottom_-chunkedrect.top_+1)*(chunkedrect.right_-chunkedrect.left_+1));

    for (int y=chunkedrect.top_; y <= chunkedrect.bottom_; y++)
        for (int x=chunkedrect.left_; x <= chunkedrect.right_; x++, count++)
            chindexes[count] = (unsigned)(y * numx_ + x);
}
