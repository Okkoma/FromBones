#include <iostream>
#include <stdlib.h>

#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Container/List.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

#include "DefsWorld.h"

#include "GameOptionsTest.h"
#include "GameHelpers.h"

#include "Map.h"
#include "ObjectMaped.h"
#include "ViewManager.h"

#include "MapColliderGenerator.h"

const unsigned MapColliderGenerator::blockMapBlockedTileID = 0x0001;
const unsigned MapColliderGenerator::BlockBoxID   = 0x0001;
const unsigned MapColliderGenerator::BlockChainID = 0x0002;
const unsigned MapColliderGenerator::BlockEdgeID  = 0x0003;
const unsigned MapColliderGenerator::BlockHoleID  = 0x0004;
const unsigned MapColliderGenerator::BlockCheckID = 0x0005;
const unsigned MapColliderGenerator::VectorCapacity = 8;

const unsigned BlockIDMask = 0x7FFF;
const int BlockShapeBit = 16;

enum BlockShapeType
{
    Empty,
    FullSquare = 1,
    HalfHeight,
    HalfWidth_Left,
    HalfWidth_Right,
    NUM_BLOCKSHAPETYPES
};

const unsigned BlockShapeID[NUM_BLOCKSHAPETYPES] =
{
    0x00000,
    0x10000,
    0x20000,
    0x30000,
    0x40000,
};

const Rect BlockShape[NUM_BLOCKSHAPETYPES] =
{
    Rect(0.f, 0.f, 0.f, 0.f),
    Rect(0.f, 0.f, 1.f, 1.f),
    Rect(0.f, 0.f, 1.f, 0.45f),
    Rect(0.f, 0.f, 0.5f, 1.f),
    Rect(0.5f, 0.f, 1.f, 1.f),
};

const char* ColliderModeStr[] =
{
    "FrontMode=0",
    "BackMode=1",
    "BackRenderMode=2",
    "TopBorderBackMode=3",
    "InnerMode=4",
    "WallMode=5",
    "WallAndPlateformMode=6",
    "PlateformMode=7",
    "BorderMode=8",
    "TopBackMode=9",
    "EmptyMode=10"
};

const char* ShapeTypeStr[] =
{
    "SHT_ALL=0",
    "SHT_BOX=1",
    "SHT_CHAIN=2",
    "SHT_EDGE=3",
};

#define START_PATTERNMARK 6U
#define NUM_BORDERTILES 2

#define MAPCOLLIDER_PHYSIC_DEFAULTDEBUGLEVEL 0
#define MAPCOLLIDER_RENDER_DEFAULTDEBUGLEVEL 0
#define MAPCOLLIDER_PHYSIC_TRACEDEBUGLEVEL 3

//#define DUMPMAP_DEBUG
//#define DUMPMAP_DEBUG_RENDER
//#define DUMPMAP_DEBUG_CONTOUR
//#define DUMPMAP_BLOCKMAP_DEBUG_ENTRY
//#define DUMPMAP_BLOCKMAP_DEBUG
//#define DUMPMAP_BLOCKINFO_DEBUG
//#define DUMPMAP_INTERNAL1_DEBUG
//#define DUMPMAP_INTERNAL2_DEBUG
//#define DUMPMAP_INTERNAL2_DEBUGDELAY 1

//#define DEBUGLOGSTR

MapColliderGenerator* MapColliderGenerator::generator_ = 0;


MapColliderGenerator::MapColliderGenerator(World2DInfo* info) :
    forcedShapeType_(info->forcedShapeType_),
    shapeType_(info->shapeType_),
    delayUpdateUsec_(World2DInfo::delayUpdateUsec_),
    debugTraceOn_(false)
{
    URHO3D_LOGDEBUGF("MapColliderGenerator()");

    info_ = info;
    atlas_ = info->atlas_;
    tileWidth_ = info->tileWidth_;
    tileHeight_ = info->tileHeight_;

    indexToSet_ = 0;
    findHoles_ = false;
    holesfounded_ = false;
    shrinkContours_ = false;
    debuglevel_ = 0;
    debuglevelphysic_ = 0;
    debuglevelrender_ = 0;

    map_ = 0;

    unsigned size = (info->mapWidth_ + 2 * NUM_BORDERTILES) * (info->mapHeight_ + 2 * NUM_BORDERTILES);
    blockMap_.Reserve(size);
    holeMap_.Reserve(size);
    collStack_.Reserve(size);

    contourBorder_.Reserve(100);
    contour_.Reserve(1000);

    generator_ = this;
}

void MapColliderGenerator::SetParameters(bool findholes, bool shrink, int debugphysic, int debugrender)
{
    findHoles_ = findholes;
    shrinkContours_ = shrink;

    debuglevelphysic_ = debugphysic == -1 ? MAPCOLLIDER_PHYSIC_DEFAULTDEBUGLEVEL : debugphysic;
    debuglevelrender_ = debugrender == -1 ? MAPCOLLIDER_RENDER_DEFAULTDEBUGLEVEL : debugrender;

    if (debugTraceOn_)
    {
        debuglevelphysic_ = MAPCOLLIDER_PHYSIC_TRACEDEBUGLEVEL;
        debugTraceOn_ = false;
    }

    holesfounded_ = false;
}

void MapColliderGenerator::SetSize(int width, int height, int maxheight)
{
    width_ = width;
    height_ = height;
    maxheight_ = maxheight;

    blockMapWidth_ = width + 2 * NUM_BORDERTILES;
    blockMapHeight_ = height + 2 * NUM_BORDERTILES;

    // for GenerateContours : offset indices (relative to a certain tile) in clockwise order, beginning with the tile to the LEFT
    unsigned i = 0;
    neighborOffsets[(i++)%8] = blockMapWidth_ - 1;          // BOTTOM-LEFT
    neighborOffsets[(i++)%8] = -1;                          // LEFT
    neighborOffsets[(i++)%8] = -(int)blockMapWidth_ - 1;    // TOP-LEFT
    neighborOffsets[(i++)%8] = -(int)blockMapWidth_;        // TOP
    neighborOffsets[(i++)%8] = -(int)blockMapWidth_ + 1;    // TOP-RIGHT
    neighborOffsets[(i++)%8] = 1;                           // RIGHT
    neighborOffsets[(i++)%8] = blockMapWidth_ + 1;          // BOTTOM-RIGHT
    neighborOffsets[(i++)%8] = blockMapWidth_;              // BOTTOM

    blockMap_.Resize(blockMapWidth_, blockMapHeight_);
    holeMap_.Resize(blockMapWidth_, blockMapHeight_);
    collStack_.Resize(blockMapWidth_*blockMapHeight_);
}

bool MapColliderGenerator::GeneratePhysicCollider(MapBase* map, HiresTimer* timer, PhysicCollider& collider, int specificwidth, int specificheight, const Vector2& center, bool createPlateform)
{
    if (!map)
        return true;

    if (!map_)
        map_ = map;

    if (map_ && map != map_)
    {
        if (map_->GetType() != ObjectMaped::GetTypeStatic())
        {
            URHO3D_LOGDEBUGF("MapColliderGenerator() - GeneratePhysicCollider ... map=%s indz=%d indv=%d ... Generator blocked by map=%s ... unblock !",
                             map->GetMapPoint().ToString().CString(), collider.params_->indz_, collider.params_->indv_, map_->GetMapPoint().ToString().CString());

            // reinitialize MapCounter for the current map that blocks the generator
            if (map_->GetStatus() == Creating_Map_Colliders && map_->GetMapCounter(MAP_GENERAL) == 1 && map_->GetMapCounter(MAP_FUNC1) == 1)
            {
                map_->GetMapCounter(MAP_FUNC2) = 0;
                map_->GetMapCounter(MAP_FUNC3) = 0;
            }

            // unblock
            map_ = map;

            // reinitialize MapCounter for the current map that blocks the generator
            if (map_->GetStatus() == Creating_Map_Colliders && map_->GetMapCounter(MAP_GENERAL) == 1 && map_->GetMapCounter(MAP_FUNC1) == 1)
            {
                map_->GetMapCounter(MAP_FUNC2) = 0;
                map_->GetMapCounter(MAP_FUNC3) = 0;
            }
        }
        else
        {
            return false;
        }
    }

    int& mcount3 = map->GetMapCounter(MAP_FUNC3);

    if (!timer || mcount3 > 5)
        mcount3 = 0;

    if (mcount3 == 0)
    {
        debuglevel_ = debuglevelphysic_;

        viewManager_ = ViewManager::Get();

        center_ = center;

        if (specificwidth)
            SetSize(specificwidth, specificheight, map->GetObjectFeatured()->height_);
        else
            SetSize(map->GetObjectFeatured()->width_, map->GetObjectFeatured()->height_, map->GetObjectFeatured()->height_);

        collider.Clear();

#if defined(DUMPMAP_DEBUG)
        if (debuglevel_)
        {
            int viewZ = viewManager_->viewZ_[collider.params_->indz_];
            int colliderZ = collider.params_->colliderz_;
            int viewId = map->GetViewIDs(colliderZ)[collider.params_->indv_];

            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);

            URHO3D_LOGDEBUGF("MapColliderGenerator() - GeneratePhysicCollider ... viewZ=%d viewid=%s(%d) colliderZ=%d(%d) colliderIndZV=%d,%d colliderMode=%s shapeType=%s",
                            viewZ, ViewManager::GetViewName(viewId).CString(), viewId, colliderZ, collider.params_->colliderz_,
                            collider.params_->indz_, collider.params_->indv_, ColliderModeStr[collider.params_->mode_], ShapeTypeStr[collider.params_->shapetype_]);

            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
#endif

        mcount3++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount3 == 1)
    {
        if (!GenerateWorkMatrix(&collider, timer, map->GetMaskedView(collider.params_->indz_, collider.params_->indv_), (MapColliderMode)collider.params_->mode_, createPlateform))
            return false;

        mcount3++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount3 > 1 && mcount3 < 5)
    {
        if (collider.params_->shapetype_ == SHT_ALL)
        {
            if (mcount3 == 2)
            {
                if (!GenerateBlockInfos(collider, timer, BlockBoxID))
                    return false;

                mcount3++;

                if (TimeOverMaximized(timer))
                    return false;
            }
            if (mcount3 == 3)
            {
                if (!GenerateContours(&collider, timer, BlockChainID))
                    return false;


                mcount3++;

                if (TimeOverMaximized(timer))
                    return false;
            }
            if (mcount3 == 4)
            {
                if (!GenerateBlockInfos(collider, timer, BlockChainID))
                    return false;

                mcount3 = 5;

                if (TimeOverMaximized(timer))
                    return false;
            }
        }
        else if (collider.params_->shapetype_ == SHT_BOX)
        {
            if (mcount3 == 2)
            {
                if (!GenerateBlockInfos(collider, timer, BlockBoxID))
                    return false;
            }

            mcount3 = 5;

            if (TimeOverMaximized(timer))
                return false;
        }
        else if (collider.params_->shapetype_ == SHT_CHAIN)
        {
            if (mcount3 == 2)
            {
                if (!GenerateContours(&collider, timer, BlockChainID))
                    return false;

                mcount3++;

                if (TimeOverMaximized(timer))
                    return false;
            }
            if (mcount3 == 3)
            {
                if (!GenerateBlockInfos(collider, timer, BlockChainID))
                    return false;

                mcount3 = 5;

                if (TimeOverMaximized(timer))
                    return false;
            }
        }
        else
        {
            mcount3 = 5;

            if (TimeOverMaximized(timer))
                return false;
        }
    }

    if (mcount3 == 5)
    {
//#if defined(DUMPMAP_DEBUG)
//        if (debuglevel_)
//        {
//            if (collider.contourIds_.Size())
//            {
//                GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
//                GameHelpers::DumpData(&collider.contourIds_[0], 1, 2, width_, height_);
//                GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
//            }
//        }
//#endif
        mcount3 = 0;
        map_ = 0;
    }

    return true;
}

bool MapColliderGenerator::GenerateRenderCollider(MapBase* map, HiresTimer* timer, RenderCollider& collider, const Vector2& center)
{
    if (!map)
        return true;

    if (!map_)
        map_ = map;

    if (map_ && map != map_)
    {
        if (map_->GetType() != ObjectMaped::GetTypeStatic())
        {
            URHO3D_LOGDEBUGF("MapColliderGenerator() - GenerateRenderCollider ... map=%s indz=%d indv=%d ... Generator blocked by map=%s ... unblock !", map->GetMapPoint().ToString().CString(), collider.params_->indz_, collider.params_->indv_, map_->GetMapPoint().ToString().CString());

            // reinitialize MapCounter for the current map that blocks the generator
            if (map_->GetStatus() == Creating_Map_Colliders && map_->GetMapCounter(MAP_GENERAL) == 1 && map_->GetMapCounter(MAP_FUNC1) == 2)
            {
                map_->GetMapCounter(MAP_FUNC2) = 0;
                map_->GetMapCounter(MAP_FUNC3) = 0;
            }

            // unblock
            map_ = map;

            // reinitialize MapCounter for the current map that was blocked
            if (map_->GetStatus() == Creating_Map_Colliders && map_->GetMapCounter(MAP_GENERAL) == 1 && map_->GetMapCounter(MAP_FUNC1) == 2)
            {
                map_->GetMapCounter(MAP_FUNC2) = 0;
                map_->GetMapCounter(MAP_FUNC3) = 0;
            }
        }
        else
        {
            return false;
        }
    }

    int& mcount3 = map->GetMapCounter(MAP_FUNC3);

    if (!timer || mcount3 > 4)
        mcount3 = 0;

//    URHO3D_LOGDEBUGF("MapColliderGenerator() - GenerateRenderCollider ... map=%s mcount3=%d", map->GetMapPoint().ToString().CString(), mcount3);

    if (mcount3 == 0)
    {
        debuglevel_ = debuglevelrender_;

        viewManager_ = ViewManager::Get();

        center_ = center;

        SetSize(map->GetObjectFeatured()->width_, map->GetObjectFeatured()->height_, map->GetObjectFeatured()->height_);

        collider.Clear();
        collider.map_ = map;

#if defined(DUMPMAP_DEBUG_RENDER)
        if (debuglevel_)
        {
            int viewZ = viewManager_->viewZ_[collider.params_->indz_];
            //        int viewId = map->GetViewIDs(viewZ)[collider.params_->indv_];
            //        int colliderZ = map->GetViewZ(viewId);
            int colliderZ = collider.params_->colliderz_;
            int viewId = map->GetViewIDs(colliderZ)[collider.params_->indv_];

            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
            URHO3D_LOGDEBUGF("MapColliderGenerator() - GenerateRenderCollider ... viewZ=%d viewid=%s(%d) colliderZ=%d(%d) colliderIndZV=%d,%d colliderMode=%s",
                            viewZ, ViewManager::GetViewName(viewId).CString(), viewId, colliderZ, collider.params_->colliderz_,
                            collider.params_->indz_, collider.params_->indv_, ColliderModeStr[collider.params_->mode_]);
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
#endif

        map->GetMapCounter(MAP_FUNC4) = 0;
        mcount3++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount3 == 1)
    {
        if (!GenerateWorkMatrix(&collider, timer, map->GetMaskedView(collider.params_->indz_, collider.params_->indv_), (MapColliderMode)collider.params_->mode_, false))
            return false;

        mcount3++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount3 == 2)
    {
        if (!GenerateContours(&collider, timer, BlockChainID))
            return false;

        mcount3++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount3 == 3)
    {
        if (!GenerateBlockInfos(collider, timer))
            return false;

        mcount3++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount3 == 4)
    {
#if defined(DUMPMAP_DEBUG_RENDER) && !defined(DUMPMAP_DEBUG_CONTOUR)
        if (debuglevel_)
        {
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
            GameHelpers::DumpData(&collider.contourIds_[0], 1, 2, width_, height_);
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
#endif
        mcount3 = 0;
        map_ = 0;
    }

//    URHO3D_LOGDEBUGF("MapColliderGenerator() - GenerateRenderCollider ... map=%s indz=%d indv=%d numblocks=%u... OK !", map->GetMapPoint().ToString().CString(), collider.params_->indz_, collider.params_->indv_, numBlocks_);

    return true;
}

bool MapColliderGenerator::Has4NeighBors(const FeaturedMap& featureMap, int x, int y) const
{
    if (x - 1 < 0 || featureMap[y*width_ + x-1] < MapFeatureType::Threshold)
        return false;
    if (x + 1 >= width_ || featureMap[y*width_ + x+1] < MapFeatureType::Threshold)
        return false;
    if (y - 1 < 0 || featureMap[(y-1)*width_ + x] < MapFeatureType::Threshold)
        return false;
    if (y + 1 >= height_ || featureMap[(y+1)*width_ + x] < MapFeatureType::Threshold)
        return false;

    return true;
}

bool MapColliderGenerator::GenerateWorkMatrix(MapCollider* collider, HiresTimer* timer, const FeaturedMap& view, MapColliderMode mode, bool createPlateform)
{
    int& mcount4 = collider->map_->GetMapCounter(MAP_FUNC4);
    unsigned x, y, addr;

    if (!timer)
        mcount4 = 0;

    if (mcount4 == 0)
    {
        blockMap_.SetBufferValue(0);

        numBlocks_ = 0;

#ifdef DUMPMAP_BLOCKMAP_DEBUG_ENTRY
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
        if (debuglevel_ >= 1)
        {
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
            URHO3D_LOGINFOF("MapColliderGenerator() - GenerateWorkMatrix ... colliderMode=%s shapetype=%s ...",
                            ColliderModeStr[mode], ShapeTypeStr[collider->params_->shapetype_]);
            URHO3D_LOGINFOF("ENTRY VIEW : ");
            GameHelpers::DumpData(&view[0], -1, 2, width_, height_);
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
#endif
#endif
        mcount4++;
        collider->map_->GetMapCounter(MAP_FUNC5) = 0;

        if (collider->GetType() == PHYSICCOLLIDERTYPE && createPlateform)
            ((PhysicCollider*)collider)->ClearPlateforms();

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount4 == 1)
    {
        int& mcount5 = collider->map_->GetMapCounter(MAP_FUNC5);
        int forcedBlockedTileID = collider->params_->shapetype_ != SHT_ALL ? collider->params_->shapetype_ : blockMapBlockedTileID;

        URHO3D_LOGDEBUGF("MapColliderGenerator() - GenerateWorkMatrix ... colliderMode=%s mcount4=1 mcount5=%d/%d starttimer=%d/%d ...",
                           ColliderModeStr[mode], mcount5, height_, timer ? (int)(timer->GetUSec(false) / 1000) : -1, (int)(World2DInfo::delayUpdateUsec_/1000));

        if (mode == FrontMode)
        {
            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x <width_; x++, addr++)
                {
                    if (view[addr] <= MapFeatureType::NoRender)
                        continue;

                    blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                    numBlocks_++;
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == BackMode)
        {
            if (mcount5 == 0)
                mcount5 = 1;

            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x <width_; x++, addr++)
                {
                    FeatureType feat = view[addr];
                    // check if roof, add a halftile over the cell (Outerview)
                #ifdef ACTIVE_DUNGEONROOFS
                    if (feat == MapFeatureType::OuterRoof && y > 0)
                    {
                        blockMap_[(y + NUM_BORDERTILES - 1) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID + BlockShapeID[HalfHeight];
                        numBlocks_++;
                        continue;
                    }
                #endif
                    // if it's not a block, skip !
                    if (feat <= MapFeatureType::Threshold)
                        continue;

                    FeatureType topfeat = view[addr-width_];

                    // if has no block over it, add the block !
                    if (topfeat < MapFeatureType::Door || topfeat == MapFeatureType::Window)
                    {
                        blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                        numBlocks_++;
                    }
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == TopBackMode && collider->GetType() == PHYSICCOLLIDERTYPE)
        {
            // first row, never check row over it.
            if (mcount5 == 0)
            {
                addr = 0;
                for (x = 0; x <width_; x++, addr++)
                {
                    if (view[addr] <= MapFeatureType::Threshold)
                        continue;

                    blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                    numBlocks_++;
                }

                mcount5 ++;

                if (HalfTimeOver(timer))
                    return false;
            }

            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x <width_; x++, addr++)
                {
                    // if it's a block and no block over it, make collider
                    if (view[addr] <= MapFeatureType::Threshold)
                        continue;

                    const FeatureType& topfeat = view[addr-width_];
                    if (topfeat < MapFeatureType::Door || topfeat == MapFeatureType::Window)
                    {
                        blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                        numBlocks_++;
                    }
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == BackRenderMode)
        {
            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x <width_; x++, addr++)
                {
                    if (view[addr] <= MapFeatureType::Threshold)
                        continue;
                #ifdef ACTIVE_DUNGEONROOFS
                    if (view[addr] == MapFeatureType::InnerRoof)
                        continue;
                #endif
                    blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                    numBlocks_++;
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == BorderMode)
        {
            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x < width_; x++, addr++)
                {
                    if (view[addr] == 0 || Has4NeighBors(view, x, y))
                        continue;

                    blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                    numBlocks_++;
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == TopBorderBackMode && collider->GetType() == PHYSICCOLLIDERTYPE)
        {
            // top border, use fluidcell
            // only use this mode after setting fluidcell at top border
            {
                FluidMap& fluidMap = collider->map_->GetFluidDatas(ViewManager::FLUIDINNERVIEW_Index).fluidmap_;

                unsigned baddrtop = blockMapWidth_ * NUM_BORDERTILES + NUM_BORDERTILES;
                unsigned baddrbottom = blockMapWidth_ * (blockMapHeight_ - NUM_BORDERTILES -1) + NUM_BORDERTILES;
                FluidCell* cell;

                for (x = 0; x < width_; x++)
                {
                    if (view[x] > MapFeatureType::Threshold)
                    {
                        cell = fluidMap[x].Top;

                        // if no block on top it's a border => make collider
                        if (!cell || cell->type_ != BLOCK)
                        {
                            blockMap_[baddrtop + x] = forcedBlockedTileID;
                            numBlocks_++;
                        }
                    }
                }

                URHO3D_LOGDEBUGF("MapColliderGenerator() - GenerateWorkMatrix map=%s ... TopBorderBackMode numBorderBlocks_=%u",
                                collider->map_->GetMapGeneratorStatus().mappoint_.ToString().CString(), numBlocks_);
            }
        }
        else if (mode == InnerMode && collider->GetType() == PHYSICCOLLIDERTYPE)
        {
            PhysicCollider* physicCollider = (PhysicCollider*)collider;

            for (y = mcount5; y < height_; y++)
            {
                int plateform_lastx = 0, plateform_firstx = -1;
                addr = y * width_;
                Plateform* plateform = 0;

                for (x = 0; x < width_; x++, addr++)
                {
                    FeatureType feat = view[addr];

                #ifdef ACTIVE_DUNGEONROOFS
                    // if roof, add collider block on the top cell if empty
                    if (feat == MapFeatureType::RoofInnerSpace && y > 0)
                    {
                        FeatureType topfeat = view[addr-width_];
                        if (topfeat < MapFeatureType::Door)
                        {
                            blockMap_[(y + NUM_BORDERTILES - 1) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID + BlockShapeID[HalfHeight];
                            numBlocks_++;
                        }
                        // add block to close roof inner space
                        if (x == 0 || view[addr-1] == MapFeatureType::NoMapFeature)
                        {
                            blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID + BlockShapeID[HalfWidth_Left];
                            numBlocks_++;
                        }
                        else if (x == width_-1 || view[addr+1] == MapFeatureType::NoMapFeature)
                        {
                            blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID + BlockShapeID[HalfWidth_Right];
                            numBlocks_++;
                        }
                    }
                #endif
                    if (feat <= MapFeatureType::Threshold)
                        continue;

                    // if plateform feature, find plateform
                    if (feat == MapFeatureType::RoomPlateForm)
                    {
                        if (!createPlateform)
                            continue;

#ifdef GENERATE_PLATEFORMCOLLIDERS
                        // only generate plateform for SHT_CHAIN : prevent to have multiple collisionBoxes for a same plateform
                        // (in mobilecastles that have 2 PhysicColliders in INNERVIEW (SHT_CHAIN & SHT_BOX))
                        if (collider->params_->shapetype_ != SHT_CHAIN)
                            continue;

                        // end of plateform : set the plateform size
                        if (plateform && x > plateform_lastx)
                        {
                            plateform->size_ = plateform_lastx - plateform_firstx;
                            // next plateform
                            plateform = 0;
                            plateform_firstx = -1;
                        }
                        // begin of plateform : allocate a plateform at the tileindex addr
                        if (plateform_firstx == -1)
                        {
                            HashMap<unsigned, Plateform*>::Iterator it = physicCollider->plateforms_.Find(addr);
                            if (it == physicCollider->plateforms_.End())
                            {
                                // new allocation
                                plateform = new Plateform(addr);
                                physicCollider->plateforms_[addr] = plateform;
                            }
                            else
                            {
                                // get the existing plateform
                                plateform = it->second_;
                            }

                            plateform_firstx = x;
                            plateform_lastx = x + 1;
                        }
                        else if (plateform)
                            // continue plateform : continue to assign the same plateform at the tileindex addr
                        {
                            physicCollider->plateforms_[addr] = plateform;
                            plateform_lastx++;
                        }
#endif

                        continue;
                    }

                    blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                    numBlocks_++;
                }
#ifdef GENERATE_PLATEFORMCOLLIDERS
                // finish to get the last plateform
                if (plateform && plateform_firstx < plateform_lastx)
                {
                    plateform->size_ = plateform_lastx - plateform_firstx;
                }
#endif

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == WallMode)
        {
            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x < width_; x++, addr++)
                {
                    // No Plateform
                    if (view[addr] >= MapFeatureType::InnerFloor && view[addr] <= MapFeatureType::TunnelWall)
                    {
                        blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                        numBlocks_++;
                    }
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == WallAndPlateformMode)
        {
            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x < width_; x++, addr++)
                {
                    // With Plateform
                    if (view[addr] >= MapFeatureType::InnerFloor)
                    {
                        blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                        numBlocks_++;
                    }
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }
        else if (mode == PlateformMode)
        {
            for (y = mcount5; y < height_; y++)
            {
                addr = y * width_;
                for (x = 0; x < width_; x++, addr++)
                {
                    // With Plateform
                    if (view[addr] == MapFeatureType::RoomPlateForm)
                    {
                        blockMap_[(y + NUM_BORDERTILES) * blockMapWidth_ + x + NUM_BORDERTILES] = forcedBlockedTileID;
                        numBlocks_++;
                    }
                }

                mcount5++;

                if (HalfTimeOver(timer))
                    return false;
            }
        }

        mcount4++;
        collider->map_->GetMapCounter(MAP_FUNC5) = 0;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount4 == 2)
    {
#ifdef DUMPMAP_BLOCKMAP_DEBUG
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
        if (debuglevel_ >= 1)
        {
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
            URHO3D_LOGINFOF("GENERATE BLOCKMAP : colliderMode=%s (A=BOXorALL B=CHAIN C=EDGE)", ColliderModeStr[mode]);
            GameHelpers::DumpData(&blockMap_[0], 1, 2, blockMapWidth_, blockMapHeight_);
            GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
        }
#endif
#endif
        //	URHO3D_LOGINFOF("MapColliderGenerator() - GenerateWorkMatrix ... OK !");

        mcount4 = 0;
    }

    return true;
}

// for debug
#ifdef DEBUGLOGSTR
static MapCollider* sCollider_;
static String sdebugTag_;
static bool sDebuglog_;
#endif

bool MapColliderGenerator::GenerateContours(MapCollider* collider, HiresTimer* timer, unsigned blockID)
{
    int& mcount4 = collider->map_->GetMapCounter(MAP_FUNC4);

    if (!timer)
        mcount4 = 0;

    if (mcount4 == 0)
    {
#if defined(DUMPMAP_DEBUG_CONTOUR)
        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, true);
        URHO3D_LOGINFO("MapColliderGenerator() : GenerateContours ...");
#endif

        collider->contourVertices_.Clear();

        if (collider->GetType() == PHYSICCOLLIDERTYPE)
        {
            ((PhysicCollider*)collider)->infoVertices_.Clear();
        }

        mcount4++;
        collider->map_->GetMapCounter(MAP_FUNC5) = 0;

        if (TimeOverMaximized(timer))
            return false;

    }
    // Generate Contour
    if (mcount4 == 1)
    {
#ifdef DEBUGLOGSTR
        sCollider_ = collider;
#endif
        TraceContours_MooreNeighbor(blockMap_, blockID, START_PATTERNMARK, collider->contourVertices_, collider->GetType() == PHYSICCOLLIDERTYPE ? &((PhysicCollider*)collider)->infoVertices_ : 0);

        collider->contourBorders_ = contourBorder_;
        collider->holeVertices_.Resize(collider->contourVertices_.Size());

        mcount4++;
        collider->map_->GetMapCounter(MAP_FUNC5) = 0;

#if defined(DUMPMAP_DEBUG_CONTOUR)
        if (debuglevel_ >= 2)
            GameHelpers::DumpData(blockMap_.Buffer(), START_PATTERNMARK, 2, blockMapWidth_, blockMapHeight_);
#endif

        if (TimeOverMaximized(timer))
            return false;
    }
    // Generate Holes
    if (mcount4 == 2)
    {
        if (findHoles_)
        {
            int& mcount5 = collider->map_->GetMapCounter(MAP_FUNC5);

            Vector<IntRect >& contourBorder = collider->contourBorders_;
            Vector<Vector<PODVector<Vector2> > >& holeVertices = collider->holeVertices_;

            unsigned holeMark = START_PATTERNMARK + holeVertices.Size();

            for (int i=mcount5; i < holeVertices.Size(); i++)
            {
#if defined(DUMPMAP_DEBUG_CONTOUR)
                if (debuglevel_ >= 3)
                    URHO3D_LOGINFOF("MapColliderGenerator() : GenerateContours ... finding holes for contour[%d]=%c in border=%s... ",
                                    i, (char)(65 + i), contourBorder[i].ToString().CString());
#endif

                holeMap_.SetBufferFrom(blockMap_.Buffer());
                holesfounded_ = MarkHolesInArea(holeMap_, contourBorder[i], START_PATTERNMARK + i, BlockHoleID);
                holeVertices[i].Clear();

                if (holesfounded_)
                {
                    TraceContours_MooreNeighbor(holeMap_, BlockHoleID, holeMark, holeVertices[i], 0);

                    for (int j=0; j < holeVertices[i].Size(); j++)
                        GameHelpers::OffsetEqualVertices(collider->contourVertices_[i], holeVertices[i][j], 0.0001f, true);

#if defined(DUMPMAP_DEBUG_CONTOUR)
                    if (debuglevel_ >= 3 && holeVertices[i].Size())
                    {
                        URHO3D_LOGINFOF("MapColliderGenerator() : GenerateContours ... %u holes for contour[%d]=%c ... Dump Hole Map :",
                                        holeVertices[i].Size(), i, (char)(65 + i));
                        GameHelpers::DumpData(holeMap_.Buffer(), START_PATTERNMARK, 3, blockMapWidth_, blockMapHeight_, holeMark);
                    }
#endif
                }

                mcount5++;

                if (mcount5 < holeVertices.Size() && HalfTimeOver(timer))
                    return false;
            }
        }

        mcount4++;
        collider->map_->GetMapCounter(MAP_FUNC5) = 0;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount4 == 3)
    {
#if defined(DUMPMAP_DEBUG_CONTOUR)
        if (debuglevel_ >= 6)
            collider->DumpContour();

        URHO3D_LOGINFO("MapColliderGenerator() : GenerateContours ... OK !");
        GAME_SETGAMELOGENABLE(GAMELOG_MAPCREATE, false);
#endif

        mcount4 = 0;
    }

    return true;
}

bool MapColliderGenerator::GenerateBlockInfos(PhysicCollider& collider, HiresTimer* timer, unsigned blockID)
{
    if (blockID == BlockBoxID)
    {
        // Allocate MapCollider BlockBoxes entries to NULL
        // => each entry is created after when creatint collisionshapes
        if (numBlocks_)
        {
            unsigned addr=0;

            for (unsigned y=0; y < height_; y++)
                for (unsigned x=0; x < width_; x++, addr++)
                {
                    if (blockMap_[(y+NUM_BORDERTILES)*blockMapWidth_+x+NUM_BORDERTILES] != BlockBoxID)
                        continue;

                    collider.blocks_[addr] = 0;
                }
        }
        collider.numShapes_[SHT_BOX] = numBlocks_;
        URHO3D_LOGDEBUGF("MapColliderGenerator() : GenerateBlockInfos ... SHT_BOX numBoxes=%u OK !", numBlocks_);
    }

    if (blockID == BlockChainID)
    {
        // update num contours
        collider.numShapes_[SHT_CHAIN] = collider.contourVertices_.Size();

        // update contourids
        PODVector<unsigned char>& contourIds = collider.contourIds_;
        contourIds.Resize(width_ * height_);
        unsigned addr=0;
        int id;
        for (unsigned y=NUM_BORDERTILES; y < blockMapHeight_-NUM_BORDERTILES; ++y)
            for (unsigned x=NUM_BORDERTILES; x < blockMapWidth_-NUM_BORDERTILES; ++x, ++addr)
            {
                id = blockMap_[y*blockMapWidth_+x] - START_PATTERNMARK + 1;
                contourIds[addr] = id > 0 ? id : 0;
            }

#if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_BLOCKINFO_DEBUG)
        if (debuglevel_ >= 1)
        {
            URHO3D_LOGINFOF("MapColliderGenerator() : GenerateBlockInfos indz=%u indv=%u colliderZ=%d SHT_CHAIN ContourIds : ",
                            collider.params_->indz_, collider.params_->indv_, collider.params_->colliderz_);

            GameHelpers::DumpData(&contourIds[0], 1, 2, width_, height_);

            URHO3D_LOGINFOF("MapColliderGenerator() : GenerateBlockInfos ... SHT_CHAIN numChains=%u OK !",
                            collider.numShapes_[SHT_CHAIN]);
        }
#endif
    }

    return true;
}

bool MapColliderGenerator::GenerateBlockInfos(RenderCollider& collider, HiresTimer* timer)
{
    // update numchains
    collider.numShapes_[SHT_CHAIN] = collider.contourVertices_.Size();

    // update contourids
    PODVector<unsigned char>& contourIds = collider.contourIds_;

    contourIds.Resize(width_ * height_);
    unsigned addr=0;
    int id;

    for (unsigned y=NUM_BORDERTILES; y < blockMapHeight_-NUM_BORDERTILES; ++y)
        for (unsigned x=NUM_BORDERTILES; x < blockMapWidth_-NUM_BORDERTILES; ++x, ++addr)
        {
            id = blockMap_[y*blockMapWidth_+x] - START_PATTERNMARK + 1;
            contourIds[addr] = id > 0 ? id : 0;
        }

    return true;
}

void MapColliderGenerator::GetUpdatedBlockBoxes(const PhysicCollider& collider, const int x, const int y, PODVector<unsigned>& addedBlocks, PODVector<unsigned>& removedBlocks)
{
    const FeaturedMap& view = collider.map_->GetFeatureView(collider.params_->indv_);
    MapColliderMode mode = (MapColliderMode)collider.params_->mode_;
    unsigned addr;

    if (mode == InnerMode)
    {
        addr = collider.map_->GetTileIndex(x, y);
        const FeatureType& feat = view[addr];

        if (feat < MapFeatureType::Threshold)
            removedBlocks.Push(addr);
        // skip plateform cases
        else if (feat != MapFeatureType::RoomPlateForm)
            addedBlocks.Push(addr);
    }
    else if (mode == BorderMode)
    {
        addr = collider.map_->GetTileIndex(x, y);
        if (view[addr] < MapFeatureType::Threshold || Has4NeighBors(view, x, y))
            removedBlocks.Push(addr);
        else
            addedBlocks.Push(addr);

        if (y-1 >= 0)
        {
            addr = collider.map_->GetTileIndex(x, y-1);
            if (view[addr] < MapFeatureType::Threshold || Has4NeighBors(view, x, y-1))
                removedBlocks.Push(addr);
            else
                addedBlocks.Push(addr);
        }
        if (y+1 < height_)
        {
            addr = collider.map_->GetTileIndex(x, y+1);
            if (view[addr] < MapFeatureType::Threshold || Has4NeighBors(view, x, y+1))
                removedBlocks.Push(addr);
            else
                addedBlocks.Push(addr);
        }
        if (x-1 >= 0)
        {
            addr = collider.map_->GetTileIndex(x-1, y);
            if (view[addr] < MapFeatureType::Threshold || Has4NeighBors(view, x-1, y))
                removedBlocks.Push(addr);
            else
                addedBlocks.Push(addr);
        }
        if (x+1 < width_)
        {
            addr = collider.map_->GetTileIndex(x+1, y);
            if (view[addr] < MapFeatureType::Threshold || Has4NeighBors(view, x+1, y))
                removedBlocks.Push(addr);
            else
                addedBlocks.Push(addr);
        }
    }

//	URHO3D_LOGDEBUGF("MapColliderGenerator() - GetUpdatedBlockBoxes : update colliderid=%d collidermode=%d x=%d y=%d Boxes ... boxesAdded=%u boxesRemoved=%u ...", collider.id_, mode, x, y, addedBlocks.Size(), removedBlocks.Size());
}

void MapColliderGenerator::GetStartBlockPoint(unsigned* refmap, unsigned* mapblock, const unsigned& refMark, const unsigned& patternMark, const unsigned& index, unsigned& startPoint, unsigned& startBackTrackPoint)
{
    const unsigned Size = blockMapWidth_ * blockMapHeight_;
    unsigned i = index;
    // search first blocking point
    while (i < Size && (refmap[i] & BlockIDMask) != refMark)
    {
        i++;
    }
    if (i < Size && (refmap[i] & BlockIDMask) == refMark)
    {
        startPoint = i;
        startBackTrackPoint = startPoint - 1;
        mapblock[startPoint] = patternMark;
    }
    else
    {
        startPoint = 0;
        startBackTrackPoint = 0;
    }
}

static Matrix2D<unsigned> sMapblock_;
static PODVector<InfoVertex> sInfoVertex_;

unsigned MapColliderGenerator::GetTileIndex(unsigned BoundaryTileIndex) const
{
    return (BoundaryTileIndex / blockMapWidth_ - NUM_BORDERTILES) * (blockMapWidth_ - 2 * NUM_BORDERTILES) + (BoundaryTileIndex % blockMapWidth_) - NUM_BORDERTILES;
}

inline int GetNextMooreIndex(int mooreIndex)
{
    mooreIndex++;
    if (mooreIndex > 7)
        mooreIndex = 0;
    return mooreIndex;
}

inline int GetPrevMooreIndex(int mooreIndex)
{
    mooreIndex--;
    if (mooreIndex < 0)
        mooreIndex = 7;
    return mooreIndex;
}

void MapColliderGenerator::TraceContours_MooreNeighbor(Matrix2D<unsigned>& refmap, const unsigned& refMark, const unsigned& patternMark, Vector<PODVector<Vector2> >& contourVertices, Vector<PODVector<InfoVertex> >* infoVertices)
{
    /// Uses the Jacob's Criterion to stop tracing a contour.
    /// https://www.imageprocessingplace.com/downloads_V3/root_downloads/tutorials/contour_tracing_Abeer_George_Ghuneim/moore.html

    const unsigned Size = blockMapWidth_ * blockMapHeight_;
    unsigned startPoint, startBackTrackPoint, currentPoint, bMin, bMax;
    unsigned boundaryPoint = 0U;
    unsigned backTrackPoint = 0U;
    unsigned currentMark = patternMark;
    int startMooreIndex, mooreIndex;
    bool stop;

    sMapblock_.Resize(blockMapWidth_, blockMapHeight_);
    memcpy(sMapblock_.Buffer(), refmap.Buffer(), Size * sizeof(unsigned));

    sInfoVertex_.Reserve(VectorCapacity);
    PODVector<InfoVertex>* pinfo = infoVertices ? &sInfoVertex_ : 0;

#if defined(DUMPMAP_DEBUG) && (defined(DUMPMAP_INTERNAL1_DEBUG) || defined(DUMPMAP_INTERNAL2_DEBUG))
    Matrix2D<unsigned> mapblockview(blockMapWidth_, blockMapHeight_);
    mapblockview.SetBufferValue(0);
#endif // DUMPMAP_DEBUG

    GetStartBlockPoint(refmap.Buffer(), sMapblock_.Buffer(), refMark, currentMark, 0, startPoint, startBackTrackPoint);

//    URHO3D_LOGINFOF("MapColliderGenerator() : TraceContours_MooreNeighbor ... startPoint=%u startBackTrackPoint=%u ...", startPoint, startBackTrackPoint);
#ifdef DEBUGLOGSTR
//    sDebuglog_ = (sCollider_->GetType() == RENDERCOLLIDERTYPE && sCollider_->params_->mode_ == BackRenderMode);
//    sDebuglog_ = (sCollider_->GetType() == PHYSICCOLLIDERTYPE && sCollider_->params_->mode_ == BackMode);
    sDebuglog_ = false;
#endif

    contourBorder_.Clear();

    while (!(startPoint == 0 && startBackTrackPoint == 0))
    {
    #ifdef DEBUGLOGSTR
        const unsigned contourmark = 65+currentMark-START_PATTERNMARK;
        /// Temp debug
        sDebuglog_ = (sCollider_->GetType() == PHYSICCOLLIDERTYPE && sCollider_->params_->mode_ == BackMode && contourmark >= 103 && contourmark <= 106);

//        if (sDebuglog_)
            URHO3D_LOGINFOF("MapColliderGenerator() : TraceContours_MooreNeighbor ... start new contour mark=%u(%c) at tileindex=%u ...", contourmark, contourmark, GetTileIndex(startPoint));
    #endif

        sInfoVertex_.Clear();
        contour_.Clear();

#if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_INTERNAL2_DEBUG)
        bool addpoints = false;
#endif

        stop = false;

        // bounding box indexes
        bMin = Size-1;
        bMax = startPoint;

        contourBorder_.Resize(contourBorder_.Size()+1);
        IntRect& border = contourBorder_.Back();
        border = IntRect::ZERO;

        const int startBackTrackMoore = 1;
        startMooreIndex = mooreIndex = GetNextMooreIndex(startBackTrackMoore);
        boundaryPoint = startPoint;
        backTrackPoint = startBackTrackPoint;
        currentPoint = boundaryPoint + neighborOffsets[mooreIndex];

#ifdef DEBUGLOGSTR
        if (sDebuglog_)
        {
            sdebugTag_.Clear();
            sdebugTag_.AppendWithFormat("S(sP=%u,sbk=%u,bP=%u,bk=%u,mr=%d,cP=%u);", GetTileIndex(startPoint), GetTileIndex(startBackTrackPoint), GetTileIndex(boundaryPoint), GetTileIndex(backTrackPoint), mooreIndex, GetTileIndex(currentPoint));
        }
#endif
        while (!stop)
        {
            if ((refmap[currentPoint] & BlockIDMask) == refMark)
            {
                // finish adding points
#ifdef DEBUGLOGSTR
                if (sDebuglog_)
                    sdebugTag_.AppendWithFormat("F(bP=%u,smr=%d,mr=%d)!", GetTileIndex(boundaryPoint), startMooreIndex, mooreIndex);
#endif
                AddPointsToSegments(contour_, pinfo, boundaryPoint, startMooreIndex, /*GetNextMooreIndex(*/mooreIndex/*)*/, refmap[boundaryPoint]);

                // change to the new boundary point
                boundaryPoint = currentPoint;
                sMapblock_[boundaryPoint] = currentMark;

                // find the new start moore Index
                startMooreIndex = 0;
                for (int i=0; i < 8; i++)
                {
                    if (boundaryPoint + neighborOffsets[i] == backTrackPoint)
                    {
                        startMooreIndex = i;
                        break;
                    }
                }
                mooreIndex = startMooreIndex;

                // next Current Point = BackTrack
                currentPoint = backTrackPoint;

                // start adding points
#ifdef DEBUGLOGSTR
                if (sDebuglog_)
                {
                    sdebugTag_.Clear();
                    sdebugTag_.AppendWithFormat("A(bP=%u,bk=%u,smr=%d,mr=%d,NcP=%u);", GetTileIndex(boundaryPoint), GetTileIndex(backTrackPoint), startMooreIndex, mooreIndex, GetTileIndex(currentPoint));
                }
#endif
                // set the startindex allowing to add the necessary BackShape points
                AddPointsToSegments(contour_, pinfo, boundaryPoint, startMooreIndex %2 ? GetPrevMooreIndex(startMooreIndex) : startMooreIndex, GetNextMooreIndex(mooreIndex), refmap[boundaryPoint]);

                // resize border left and right
                {
                    int x = boundaryPoint % blockMapWidth_;
                    if (x > border.right_)
                        border.right_ = x;
                    else if (x < border.left_)
                        border.left_ = x;
                    if (boundaryPoint < bMin)
                        bMin = boundaryPoint;
                    if (boundaryPoint > bMax)
                        bMax = boundaryPoint;
                }

#if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_INTERNAL2_DEBUG)
                addpoints = true;
#endif

#if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_INTERNAL1_DEBUG)
                mapblockview[boundaryPoint] = currentMark;
#endif
            }
            else
            {
                // Turn around the boundary point on the Moore Neighborhood
                mooreIndex = GetNextMooreIndex(mooreIndex);
                if (mooreIndex == startMooreIndex)
                    mooreIndex = GetPrevMooreIndex(mooreIndex);

                backTrackPoint = currentPoint;
                currentPoint = boundaryPoint + neighborOffsets[mooreIndex];
#ifdef DEBUGLOGSTR
                if (sDebuglog_)
                    sdebugTag_.AppendWithFormat("T(bP=%u,bk=%u,smr=%d,mr=%d,cP=%u);", GetTileIndex(boundaryPoint), GetTileIndex(backTrackPoint), startMooreIndex, mooreIndex, GetTileIndex(currentPoint));
#endif
            }

            // Using the Stopping Jacob's Criterion
            stop = (boundaryPoint == startPoint) && (backTrackPoint == startBackTrackPoint);

#if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_INTERNAL2_DEBUG)
            if (debuglevel_ >= 3 && sDebuglog_)
            {
                if (addpoints)
                {
                    addpoints = false;
                    mapblockview[startPoint] = '!';
                    mapblockview[startBackTrackPoint] = '#';
                    mapblockview[boundaryPoint] = '+';
                    mapblockview[backTrackPoint] = '&';
                    mapblockview[currentPoint] = '*';
                    GameHelpers::DumpData(mapblockview.Buffer(), START_PATTERNMARK, 8, blockMapWidth_, blockMapHeight_, currentMark,
                                          currentPoint, startPoint, startBackTrackPoint, boundaryPoint, backTrackPoint);
#ifdef DUMPMAP_INTERNAL2_DEBUGDELAY
                    Time::Sleep(DUMPMAP_INTERNAL2_DEBUGDELAY);
#endif
                }
            }
#endif
        }

        // finish adding points
#ifdef DEBUGLOGSTR
        if (sDebuglog_)
            sdebugTag_.AppendWithFormat("F(bP=%u,smr=%d,mr=%d)! Stop Contour!", GetTileIndex(boundaryPoint), startMooreIndex, mooreIndex);
#endif

        AddPointsToSegments(contour_, pinfo, boundaryPoint, startMooreIndex, mooreIndex, refmap[boundaryPoint]);

        CloseContour(contour_, pinfo);

        contourVertices.Push(contour_);
        if (infoVertices)
            infoVertices->Push(sInfoVertex_);

        /// Fill closed contour
        if (bMin < bMax)
        {
            /// Resize border Top and Bottom
            border.top_ = bMin / blockMapWidth_;
            border.bottom_ = bMax / blockMapWidth_;

            if (FillClosedContourInArea(sMapblock_, border, refMark, currentMark, currentMark))
            {
#if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_INTERNAL1_DEBUG)
                if (debuglevel_ >= 2)
                {
                    URHO3D_LOGINFOF("FILLED MAPBLOCK for contour[%d] = %c Border=%s ... ", currentMark-patternMark, (char)(65 + currentMark-patternMark), border.ToString().CString());
                    GameHelpers::DumpData(sMapblock_.Buffer(), currentMark, 2, blockMapWidth_, blockMapHeight_);
                }
#endif
            }
        }
//    #if defined(DUMPMAP_DEBUG) && defined(DUMPMAP_INTERNAL1_DEBUG)
//        else if (debuglevel_ >= 2)
//        {
//            URHO3D_LOGERRORF("Contour[%d]=%c ... => Border=%s !", currentMark-patternMark, (char)(65 + currentMark-patternMark), border.ToString().CString());
//            URHO3D_LOGINFOF("MAPBLOCK");
//            GameHelpers::DumpData(sMapblock_.Buffer(), -1, 2, blockMapWidth_, blockMapHeight_);
//        }
//    #endif

        /// Next pattern
        currentMark++;
        memcpy(refmap.Buffer(), sMapblock_.Buffer(), Size * sizeof(unsigned));

        GetStartBlockPoint(refmap.Buffer(), sMapblock_.Buffer(), refMark, currentMark, 0, startPoint, startBackTrackPoint);
    }

#if !defined(DUMPMAP_BLOCKINFO_DEBUG) && !defined(DUMPMAP_DEBUG_CONTOUR)
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
    if (debuglevel_ > 1)
    {
        URHO3D_LOGINFOF("RESULT TRACE CONTOURS :");
        GameHelpers::DumpData(refmap.Buffer(), START_PATTERNMARK, 2, blockMapWidth_, blockMapHeight_);
    }
#endif
#endif
}

bool isPointOnSameLine(const Vector2& point, const Vector2& segmentStart, const Vector2& segmentEnd)
{
    return (segmentStart.x_ == segmentEnd.x_ && segmentStart.x_ == point.x_) || (segmentStart.y_ == segmentEnd.y_ && segmentStart.y_ == point.y_);
}

void MapColliderGenerator::AddPointsToSegments(PODVector<Vector2>& segments, PODVector<InfoVertex>* infoVertices, unsigned boundaryTile, int fromNeighbor, int toNeighbor, unsigned blockID)
{
//	unsigned contactSide = contactMap_[boundaryTile];
//    const int blockShapeType = 1;
    const unsigned blockShapeType = Clamp((blockID >> BlockShapeBit) & BlockIDMask, 1U, (unsigned)NUM_BLOCKSHAPETYPES);
//    const unsigned blockShapeType = (blockID >> BlockShapeBit) & BlockIDMask;

    // Flip y for Urho convention
    Vector2 boundaryCoords;
    boundaryCoords.x_ = (float)((int)(boundaryTile % blockMapWidth_)-1) - 1.f;
    boundaryCoords.y_ = (float)maxheight_ - (float)((int)(boundaryTile / blockMapWidth_)-1);

#ifdef DEBUGLOGSTR
    String debuglogstr;
#endif

    for (int neighbor = fromNeighbor; neighbor != toNeighbor; neighbor = (neighbor + 1) % 8)
    {
        bool isPointValid = true;
        Vector2 newPoint;
        InfoVertex newInfo = { NoneSide, boundaryTile };

        switch (neighbor)
        {
        // Moore NeighborHood of X
        // |2|3|4|
        // |1|X|5|
        // |0|7|6|
        //
        case 0:
            // Edge Left Bottom
            newPoint.x_ = (boundaryCoords.x_ + BlockShape[blockShapeType].min_.x_) * tileWidth_;
            newPoint.y_ = (boundaryCoords.y_ + BlockShape[blockShapeType].min_.y_) * tileHeight_;
            newInfo.side = (LeftSide | BottomSide);
//				URHO3D_LOGDEBUGF(" new point : neighbor=%d contactInfo=%d", neighbor, newInfo);
            break;
        case 2:
            // Edge Left Top
            newPoint.x_ = (boundaryCoords.x_ + BlockShape[blockShapeType].min_.x_) * tileWidth_;
            newPoint.y_ = (boundaryCoords.y_ + BlockShape[blockShapeType].max_.y_) * tileHeight_;
            newInfo.side = (LeftSide | TopSide);
//				URHO3D_LOGDEBUGF(" new point : neighbor=%d contactInfo=%d", neighbor, newInfo);
            break;
        case 4:
            // Edge Right Top
            newPoint.x_ = (boundaryCoords.x_ + BlockShape[blockShapeType].max_.x_) * tileWidth_;
            newPoint.y_ = (boundaryCoords.y_ + BlockShape[blockShapeType].max_.y_) * tileHeight_;
            newInfo.side = (RightSide | TopSide);
//				URHO3D_LOGDEBUGF(" new point : neighbor=%d contactInfo=%d", neighbor, newInfo);
            break;
        case 6:
            // Edge Right Bottom
            newPoint.x_ = (boundaryCoords.x_ + BlockShape[blockShapeType].max_.x_) * tileWidth_;
            newPoint.y_ = (boundaryCoords.y_ + BlockShape[blockShapeType].min_.y_) * tileHeight_;
            newInfo.side = (RightSide | BottomSide);
//				URHO3D_LOGDEBUGF(" new point : neighbor=%d contactInfo=%d", neighbor, newInfo);
            break;

        default:
            // cardinal neighborhood don't add any points
            isPointValid = false;
//				URHO3D_LOGDEBUGF(" new point : neighbor=%d contactInfo=%d", neighbor, newInfo);
            break;
        }
        if (isPointValid)
        {
            // apply the center offset
            newPoint -= center_;

            if (segments.Size() >= 1 && AreEquals(newPoint, segments.Back(), 0.001f))
            {
#ifdef DEBUGLOGSTR
                if (sDebuglog_)
                    debuglogstr.AppendWithFormat("moore[%d]:skip1(%F,%F) ", neighbor, newPoint.x_, newPoint.y_);
#endif
                continue;
            }

            if (segments.Size() >= 2)
            {
                const Vector2& segmentStart = segments[segments.Size() - 2];

                if (AreEquals(newPoint, segmentStart, 0.001f))
                {
#ifdef DEBUGLOGSTR
                    if (sDebuglog_)
                        debuglogstr.AppendWithFormat("moore[%d]:skip2(%F,%F) ", neighbor, newPoint.x_, newPoint.y_);
#endif
                    continue;
                }

                // check if the new point is on the same segment (axis) as the last two points
                if (isPointOnSameLine(newPoint, segmentStart, segments.Back()))
                {
                    // replace previous segment's end point
                    segments.Back() = newPoint;
                    if (infoVertices)
                    {
                        infoVertices->At(infoVertices->Size()-1).side |= newInfo.side;
                        infoVertices->At(infoVertices->Size()-1).indexTile = newInfo.indexTile;
#ifdef DEBUGLOGSTR
                        if (sDebuglog_)
                            debuglogstr.AppendWithFormat("moore[%d]:replacePt[%d](%F,%F) ", neighbor, segments.Size()-1, newPoint.x_, newPoint.y_);
#endif
                    }
                }
                else
                {
                    // add as new point
                    segments.Push(newPoint);
                    if (infoVertices)
                        infoVertices->Push(newInfo);
#ifdef DEBUGLOGSTR
                    if (sDebuglog_)
                        debuglogstr.AppendWithFormat("moore[%d]:addPt[%d](%F,%F) ", neighbor, segments.Size()-1, newPoint.x_, newPoint.y_);
#endif
                }
            }
            else
            {
                // add as new point
                segments.Push(newPoint);
                if (infoVertices)
                    infoVertices->Push(newInfo);
#ifdef DEBUGLOGSTR
                if (sDebuglog_)
                    debuglogstr.AppendWithFormat("moore[%d]:addPt[%d](%F,%F) ", neighbor, segments.Size()-1, newPoint.x_, newPoint.y_);
#endif
            }
        }
    }

#ifdef DEBUGLOGSTR
    if (sDebuglog_)
    {
        if (!debuglogstr.Empty())
            URHO3D_LOGINFOF("MapColliderGenerator() - AddPoints tileindex=%u tag=%s moore=(%d to %d) blockid=%u blockshape=%u : %s", GetTileIndex(boundaryTile), sdebugTag_.CString(), fromNeighbor, toNeighbor, blockID, blockShapeType, debuglogstr.CString());
        else
            URHO3D_LOGINFOF("MapColliderGenerator() - AddPoints tileindex=%u tag=%s moore=(%d to %d) blockid=%u blockshape=%u : Empty !",
                            GetTileIndex(boundaryTile), sdebugTag_.CString(), fromNeighbor, toNeighbor, blockID, blockShapeType);

    }
#endif
}

void MapColliderGenerator::CloseContour(PODVector<Vector2>& segments, PODVector<InfoVertex>* infoVertices)
{
#ifdef DEBUGLOGSTR
    String debuglogstr;
#endif
/*
    // some contours may be traced more than once, find and remove repeating patterns
    unsigned sequenceLength = 0;
    unsigned compareCount = 0;
    for (unsigned i = 1; i < segments.Size(); i++)
    {
        Vector2 point = segments[i];

        if (sequenceLength)
        {
            Vector2 comparePoint = segments[i - sequenceLength];
            if (point == comparePoint)
            {
                // the pattern is repeating so far
                compareCount++;
            }
            else
            {
                // false alarm, sometimes the first point may be visited again without repeat
                sequenceLength = 0;
                compareCount = 0;
            }

            // did we match an entire repeating sequence once?
            if (sequenceLength > 0 && compareCount == sequenceLength)
            {
                // remove all the repeating points
                segments.Resize(sequenceLength);
                if (infoVertices)
                    infoVertices->Resize(sequenceLength);
                break;
            }
        }

        // try to find the first point again
        if (sequenceLength == 0 && (point == segments.Front()))
        {
            // now all following points must match sequenceLength times, then it is a repeating pattern
            sequenceLength = i;
        }
    }
*/

    // check if the two first points and the last point are on the same line
    if (segments.Size() >= 3)
    {
        if (isPointOnSameLine(segments.Back(), segments[0], segments[1]))
        {
            // first point becomes the last point, and the last point deleted
#ifdef DEBUGLOGSTR
            if (sDebuglog_)
                debuglogstr.AppendWithFormat("v[0]--v[%d]:remove ", segments.Size()-1);
#endif
            segments.Front() = segments.Back();
            segments.Resize(segments.Size()-1);
            if (infoVertices)
            {
                infoVertices->At(0).side |= infoVertices->At(infoVertices->Size() - 1).side;
                infoVertices->Resize(infoVertices->Size()-1);
            }
        }
    }

    // check if the two last points and the first point are on the same line
    if (segments.Size() >= 3)
    {
        if (isPointOnSameLine(segments.Front(), segments[segments.Size() - 1], segments[segments.Size() - 2]))
        {
#ifdef DEBUGLOGSTR
            if (sDebuglog_)
                debuglogstr.AppendWithFormat("v[0]--v[%d]:remove ", segments.Size()-1);
#endif
            segments.Resize(segments.Size()-1);
            if (infoVertices)
            {
                infoVertices->At(segments.Size() - 2).side |= infoVertices->At(infoVertices->Size() - 1).side;
                infoVertices->Resize(infoVertices->Size()-1);
            }
        }
    }

    // check if the two last points and the first point are on the same line
    if (segments.Size() >= 3)
    {
        if (isPointOnSameLine(segments.Front(), segments[1], segments[segments.Size() - 1]))
        {
#ifdef DEBUGLOGSTR
            if (sDebuglog_)
                debuglogstr.AppendWithFormat("v[0]--v[%d]:remove ", segments.Size()-1);
#endif
            segments.Front() = segments.Back();
            segments.Resize(segments.Size()-1);
        }
    }

    // ensure that first and last point are not equal
    while (segments.Size() >= 2 && AreEquals(segments.Front(), segments.Back(), 0.01f))
    {
#ifdef DEBUGLOGSTR
        if (sDebuglog_)
            debuglogstr.AppendWithFormat("v[0]=v[%d]:remove ", segments.Size()-1);
#endif
        // remove the last point
        segments.Resize(segments.Size()-1);

        if (infoVertices)
            infoVertices->Resize(infoVertices->Size()-1);
    }

    while (segments.Size() >= 2 && AreEquals(segments.Front(), segments[1], 0.01f))
    {
#ifdef DEBUGLOGSTR
        if (sDebuglog_)
            debuglogstr.Append("v[0]=v[1]:remove ");
#endif
        Swap(segments.Front(), segments.Back());
        segments.Resize(segments.Size()-1);
    }
#ifdef DEBUGLOGSTR
    if (sDebuglog_ && !debuglogstr.Empty())
        URHO3D_LOGINFOF("MapColliderGenerator() - CloseContour numVertices=%u : %s", segments.Size(), debuglogstr.CString());
#endif
    // Shrink with epsilon to prevent crash in triangulation (FastPoly2Tri)
    // shrink only the holes (not the contours) or if option
    if (!infoVertices || shrinkContours_)
        GameHelpers::OffsetEqualVertices(segments, 0.0001f, true);
#ifdef DEBUGLOGSTR
    if (sDebuglog_)// && !debuglogstr.Empty())
        GameHelpers::DumpVertices(segments);
#endif
}

bool MapColliderGenerator::FindPatternInClosedContour(Matrix2D<unsigned>& matrix, const IntRect& border, const unsigned& searchpattern, const unsigned& borderPattern, int& x, int& y)
{
//#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
//    if (debuglevel_ >= 2)
//        URHO3D_LOGINFOF("FindPatternInClosedContour : border=%s ... searchpattern=%u borderpattern=%u start x=%d y=%d",
//                         border.ToString().CString(), searchpattern, borderPattern, x, y);
//#endif
    int k, ymax = border.bottom_;
    bool findBorder;

    if (y <= border.top_ || y > ymax)
        y = border.top_+1;

    if (x > border.right_ || x < border.left_)
    {
        x = border.left_;
        y++;
    }

    for (;;)
    {
        /// find a first border marked borderPattern at (i,j), with a pattern at (i,j+1)
        if (matrix(x, y-1) == borderPattern && matrix(x, y) == searchpattern)
        {
            /// find a second border marked borderPattern
            k = y+1;
            findBorder = false;

            while (!findBorder && k <= ymax)
                findBorder = (matrix(x, k++) == borderPattern);

            if (findBorder)
            {
//            #if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
//                if (debuglevel_ >= 3)
//                    URHO3D_LOGINFOF("=> find internal point x=%d, y=%d", x, y);
//            #endif
                return true;
            }
        }

        x++;
        if (x > border.right_)
        {
            y++;
            if (y > ymax)
                break;

            x = border.left_;
        }
    }
//#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
//    if (debuglevel_ >= 2)
//        URHO3D_LOGINFOF("FindPatternInClosedContour : border=%s ... NOK !", border.ToString().CString());
//#endif
    return false;
}

bool MapColliderGenerator::FillClosedContourInArea(Matrix2D<unsigned>& matrix, const IntRect& border, const unsigned& patternToFill, const unsigned& borderPattern, const unsigned& fillPattern)
{
    int x=0, y=0;
    bool filled = false;
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
    if (debuglevel_ >= 6)
        URHO3D_LOGINFOF("FillClosedContourInArea : border=%s ...", border.ToString().CString());
#endif
    while (FindPatternInClosedContour(matrix, border, patternToFill, borderPattern, x, y))
    {
        if (GameHelpers::FloodFill(matrix, collStack_, patternToFill, fillPattern, x, y))
            filled = true;
        else
            break;
    }
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
    if (debuglevel_ >= 6)
        URHO3D_LOGINFOF("FillClosedContourInArea : border=%s ... filled=%s", border.ToString().CString(), filled ? "OK":"NOK");
#endif
    return filled;
}


bool MapColliderGenerator::MarkHolesInArea(Matrix2D<unsigned>& matrix, const IntRect& border, const unsigned& borderPattern, const unsigned& markpattern)
{
    int x=0, y=0;
    bool filled = false;

#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
    if (debuglevel_ >= 4)
        URHO3D_LOGINFOF("MarkHolesInArea : border=%s ... borderPattern=%u markpattern=%u ...", border.ToString().CString(), borderPattern, markpattern);
#endif
    checkBuffer_.Resize(matrix.Width(), matrix.Height());

    while (FindPatternInClosedContour(matrix, border, 0U, borderPattern, x, y))
    {
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
        if (debuglevel_ >= 5)
            URHO3D_LOGINFOF("MarkHolesInArea : ... start Fill at x=%d y=%d with pattern=%u ...", x, y, markpattern);
#endif
        if (GameHelpers::FloodFillInBorder(matrix, checkBuffer_, collStack_, borderPattern, markpattern, x, y, border))
        {
            matrix.CopyValueFrom(markpattern, checkBuffer_, false);
            filled = true;
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
            if (debuglevel_ >= 5)
                URHO3D_LOGINFOF("MarkHolesInArea : ... FloodFill in ClosedContour OK !");
#endif
        }
        else
        {
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
            if (debuglevel_ >= 5)
            {
                URHO3D_LOGINFOF("MarkHolesInArea : ... FloodFill breaks !");
                GameHelpers::DumpData(&checkBuffer_[0], markpattern, 3, checkBuffer_.Width(), checkBuffer_.Height(), markpattern);
            }
#endif
            while (checkBuffer_(x,y) == markpattern) x++;
        }
    }
#if defined(DUMPMAP_DEBUG) || defined(DUMPMAP_DEBUG_RENDER)
    if (debuglevel_ >= 4)
        URHO3D_LOGINFOF("MarkHolesInArea : border=%s ... OK !", border.ToString().CString());
#endif
    return filled;
}
