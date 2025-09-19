#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "Map.h"

#ifdef DUMP_MAPDEBUG_FLUIDVIEW
#include "GameHelpers.h"
#endif

#include "DefsFluids.h"

// FluidCell

const char* MaterialTypeStr[] =
{
    "AIR",
    "BLOCK",
    "WATER",
    "OIL",
    "LAVA",
    0
};

const char* FluidPatternTypeStr[] =
{
    "FPT_Empty",
    "FPT_IsolatedWater",
    "FPT_NoRender",
    "FPT_WaterFall",
    "FPT_Wave",
    "FPT_Swirl",
    "FPT_Surface",
    "FPT_FullWater",
    0
};

const char* FluidPatternStr[] =
{
    "PTRN_Empty",
    "PTRN_Droplet",
    // Swirl
    "PTRN_Swirl",
    // DepthZ
    "PTRN_RightDepthZ",
    "PTRN_LeftDepthZ",
    "PTRN_CenteredDepthZ",
    "PTRN_CenterFallDepthZ",
    // Fall
    "PTRN_RightFall",
    "PTRN_LeftFall",
    "PTRN_CenterLeftFall",
    "PTRN_CenterRightFall",
    "PTRN_LeftRightFall",
    "PTRN_CenterFall1",
    // Fall Head
    "PTRN_RightFallHead",
    "PTRN_LeftFallHead",
    "PTRN_CenterLeftFallHead",
    "PTRN_CenterRightFallHead",
    "PTRN_LeftRightFallHead",
    "PTRN_CenterFallHead",
    "PTRN_CenterFallHead2",
    "PTRN_SwirlHead",
    // Fall Foot
    "PTRN_RightFallFoot",
    "PTRN_LeftFallFoot",
    // Fall Part && Surface Part
    "PTRN_CenterFallRightSurface",
    "PTRN_CenterFallLeftSurface",
    "PTRN_LeftFallAndRightSurface",
    "PTRN_RightFallAndLeftSurface",
    //Surface
    "PTRN_CenterFallSurface",
    "PTRN_LeftFallSurface",
    "PTRN_RightFallSurface",
    "PTRN_LeftRightFallSurface",
    "PTRN_LeftSurface",
    "PTRN_RightSurface",
    "PTRN_TopLeftSurface",
    "PTRN_TopRightSurface",
    "PTRN_CenterSurface",
    // Full
    "PTRN_Full",
    "PTRN_FullTest",
    0
};

const FluidCell FluidCell::FullWaterCell((int)WATER, (int)FPT_FullWater);

#if (FLUID_RENDER_USEMESH == 4)

bool FluidCell::UpdateAlignement()
{
    if (type_ == BLOCK)
        return false;

    if (type_ == AIR)
    {
        patternType_ = FPT_Empty;
        return false;
    }

    if (patternType_ <= FPT_NoRender)
        return false;

    switch (patternType_)
    {
    case FPT_WaterFall:

        if (Top)
        {
            if (Top->align_ > FA_None)
                align_ = Top->align_;
        }
        else
            align_= FA_None;

        if (Bottom)
        {
            if (Left && Bottom->Left && Bottom->Left->type_ == BLOCK && Left->type_ >= WATER && Left->patternType_ > FPT_WaterFall)
                align_ |= FA_Left;

            if (Right && Bottom->Right && Bottom->Right->type_ == BLOCK && Right->type_ >= WATER && Right->patternType_ > FPT_WaterFall)
                align_ |= FA_Right;
        }

        if (Top && Top->type_ != BLOCK)
        {
            if (Left  && Top->Left && Left->type_ == BLOCK && Top->Left->type_ >= WATER && Top->Left->patternType_ > FPT_WaterFall)
                align_ |= FA_Left;

            if (Right && Top->Right && Right->type_ == BLOCK && Top->Right->type_ >= WATER && Top->Right->patternType_ > FPT_WaterFall)
                align_ |= FA_Right;
        }

        if (align_ == FA_None)
            align_ = FA_Center;

        break;

    case FPT_Wave:
        if (Top && Top->patternType_ > FPT_NoRender)
        {
            align_ = Top->align_;
        }
        else
        {
            if (Top) Top->align_ = FA_None;
            align_ = FA_None;

            if (Left && Left->patternType_ > FPT_Wave)
                align_ |= FA_Left;

            if (Right && Right->patternType_ > FPT_Wave)
                align_ |= FA_Right;
        }
        break;

    case FPT_Swirl:
        align_ = (FA_Right|FA_Left);
        break;

    case FPT_Surface:
        if (Top && Top->type_ >= WATER && Top->patternType_ > FPT_NoRender &&
                (Top->patternType_ != FPT_Wave || Top->align_ != FA_None))
        {
            align_ = Top->align_;

            if (align_ == FA_None && Top->patternType_ != FPT_Wave)
            {
                if (Left && (Left->type_ == BLOCK || (Left->type_ >= WATER && Left->patternType_ >= FPT_WaterFall)))
                    align_ |= FA_Left;

                if (Right && (Right->type_ == BLOCK || (Right->type_ >= WATER && Right->patternType_ >= FPT_WaterFall)))
                    align_ |= FA_Right;
            }
        }
        else
        {
            if (Top) Top->align_ = FA_None;
            align_ = FA_Center;

            if (Left && (Left->type_ == BLOCK || (Left->type_ >= WATER && Left->patternType_ >= FPT_WaterFall)))
                align_ |= FA_Left;

            if (Right && (Right->type_ == BLOCK || (Right->type_ >= WATER && Right->patternType_ >= FPT_WaterFall)))
                align_ |= FA_Right;
        }
        break;

    case FPT_FullWater:
        align_ = FA_Center;
        break;
    }

    // Reset Here Top pattern Alignement if not render
    if (Top && Top->patternType_ <= FPT_NoRender)
        Top->align_ = FA_None;

    return true;
}

bool FluidCell::UpdatePattern()
{
    if (!UpdateAlignement())
        return false;

    switch (patternType_)
    {
    case FPT_Swirl:
        if (Top && Top->type_ >= WATER && Top->align_ > FA_None)
            pattern_ = PTRN_Swirl;
        else
            pattern_ = PTRN_SwirlHead;
        break;

    case FPT_WaterFall:
        //if (Top && Top->type_ >= WATER)
        if (Top && Top->type_ >= WATER && Top->patternType_ > FPT_NoRender)
        {
            if (align_ != Top->align_)
            {
                if (Bottom && Bottom->patternType_ == FPT_Surface && (Top->align_ == FA_Center || Top->align_ == (FA_Center|FA_Left|FA_Right)))
                {
                    if (align_ == (FA_Center|FA_Right))
                        pattern_ = PTRN_CenterFallRightSurface;
                    else if (align_ == (FA_Center|FA_Left))
                        pattern_ = PTRN_CenterFallLeftSurface;
                    else
                        pattern_ = PTRN_CenterFallSurface;
                }
                else if (Top->align_ == FA_Left && align_ == (FA_Left|FA_Right))
                    pattern_ = PTRN_LeftFallAndRightSurface;
                else if (Top->align_ == FA_Right && align_ == (FA_Left|FA_Right))
                    pattern_ = PTRN_RightFallAndLeftSurface;
                else if (Top->align_ == FA_None && align_ == FA_Center)
                    pattern_ = PTRN_CenterFall1;
                else if (align_ == (FA_Center|FA_Left))
                    pattern_ = PTRN_CenterLeftFall;
                else if (align_ == (FA_Center|FA_Right))
                    pattern_ = PTRN_CenterRightFall;
            }
            else
            {
                if (align_ == FA_Center || align_ == FA_All)
                    pattern_ = PTRN_CenterFall1;
                else if (align_ == FA_Left)
                    pattern_ = PTRN_LeftFall;
                else if (align_ == FA_Right)
                    pattern_ = PTRN_RightFall;
                else if (align_ == (FA_Center|FA_Left))
                    pattern_ = PTRN_CenterLeftFall;
                else if (align_ == (FA_Center|FA_Right))
                    pattern_ = PTRN_CenterRightFall;
                else if (align_ == (FA_Left|FA_Right))
                    pattern_ = PTRN_LeftRightFall;
            }
        }
        else
        {
            if (Top && Top->type_ == AIR && Top->DepthZ && Top->DepthZ->type_ >= WATER &&
                    (!Top->featCheck_ || (*Top->featCheck_) <= MapFeatureType::Threshold))
                pattern_ = PTRN_CenterFallDepthZ;
            else if (align_ == FA_Center || align_ == FA_All)
                pattern_ = PTRN_CenterFallHead;
            else if (align_ == FA_Left)
                pattern_ = PTRN_LeftFallHead;
            else if (align_ == FA_Right)
                pattern_ = PTRN_RightFallHead;
            else if (align_ == (FA_Center|FA_Left))
                pattern_ = PTRN_CenterLeftFallHead;
            else if (align_ == (FA_Center|FA_Right))
                pattern_ = PTRN_CenterRightFallHead;
            else if (align_ == (FA_Left|FA_Right))
                pattern_ = PTRN_LeftRightFallHead;
        }
        break;

    case FPT_Wave:
        if (Top && Top->type_ >= WATER && Top->patternType_ > FPT_NoRender)
        {
            if (align_ == FA_Center)
                pattern_ = PTRN_CenterFall1;
            else if (align_ == FA_Left || align_ == (FA_Center|FA_Left))
                pattern_ = PTRN_LeftFall;
            else if (align_ == FA_Right || align_ == (FA_Center|FA_Right))
                pattern_ = PTRN_RightFall;
            else
                pattern_ = PTRN_LeftRightFall;
        }
        else
        {
            if (align_ == FA_Left)
                pattern_ = PTRN_LeftFallHead;
            else if (align_ == FA_Right)
                pattern_ = PTRN_RightFallHead;
            else if (align_ == (FA_Left|FA_Right))
                pattern_ = PTRN_LeftRightFallHead;
            else
                pattern_ = PTRN_Empty;
        }
        break;

    case FPT_Surface:
        if (Top && Top->type_ >= WATER && Top->patternType_ > FPT_NoRender &&
                (Top->patternType_ != FPT_Wave || Top->align_ != FA_None))
        {
            if (align_ == FA_Center || align_ == (FA_Center|FA_Left|FA_Right))
                pattern_ = PTRN_CenterFallSurface;
            else if (align_ == FA_Left || align_ == (FA_Center|FA_Left))
                pattern_ = PTRN_RightFallSurface;
            else if (align_ == FA_Right || align_ == (FA_Center|FA_Right))
                pattern_ = PTRN_LeftFallSurface;
            else if (align_ == FA_None)
                pattern_ = PTRN_CenterSurface;
            else
                pattern_ = PTRN_LeftRightFallSurface;
        }
        else
        {
            if (align_ == FA_Left || align_ == (FA_Center|FA_Left))
                pattern_ = PTRN_LeftSurface;
            else if (align_ == FA_Right || align_ == (FA_Center|FA_Right))
                pattern_ = PTRN_RightSurface;
            else if (Left && Left->patternType_ == FPT_WaterFall && Top->patternType_ == FPT_WaterFall)
                pattern_ = PTRN_RightSurface;
            else if (Right && Right->patternType_ == FPT_WaterFall && Top->patternType_ == FPT_WaterFall)
                pattern_ = PTRN_LeftSurface;
            else
                pattern_ = PTRN_CenterSurface;
        }
        break;

    case FPT_FullWater:
        pattern_ = PTRN_Full;
        break;
    }

    return true;
}
#endif

void FluidCell::Dump() const
{
    URHO3D_LOGINFOF("FluidCell : this=%u mass=%F liquidHeightInTile=%F materialtype=%s ptrntype=%s ptrn=%s",
                    this, mass_, mass_ / FLUID_MAXVALUE * MapInfo::info.mTileHalfSize_.y_ * 2.f, MaterialTypeStr[type_], FluidPatternTypeStr[patternType_], FluidPatternStr[pattern_]);
}

// FluidDatas

unsigned FluidDatas::width_ = 0;
unsigned FluidDatas::height_ = 0;

void FluidDatas::Resize(int index, unsigned width, unsigned height)
{
    indexFluidZ_ = index;

    if (width_ != width || height_ != height || !fluidmap_.Size())
    {
        width_ = width;
        height_ = height;
        fluidmap_.Resize(width*height);
        linkedCells_ = false;

        for (FluidMap::Iterator it=fluidmap_.Begin(); it != fluidmap_.End(); ++it)
            it->Clear();
    }
}

void FluidDatas::LinkInnerCells(FluidDatas* depthZ)
{
    // Link All Inner Cells
    if (!linkedCells_)
    {
        FluidCell* cell;
        // don't connect LinkedMap Cells here
        // Inner Cells
        for (unsigned y = 1; y < height_-1; y++)
            for (unsigned x = 1; x < width_-1; x++)
            {
                cell = &(fluidmap_[y*width_+x]);
                cell->LinkBorderCells(cell + width_, cell - 1, cell + 1, cell - width_);
            }
        // Top & Bottom Border Cells
        for (unsigned x = 1; x < width_-1; x++)
        {
            cell = &(fluidmap_[x]);
            cell->LinkBorderCells(cell + width_, cell - 1, cell + 1, 0);
            cell = &(fluidmap_[(height_-1)*width_ + x]);
            cell->LinkBorderCells(0, cell - 1, cell + 1, cell - width_);
        }
        // Left & Right Border Cells
        for (unsigned y = 1; y < height_-1; y++)
        {
            cell = &(fluidmap_[y*width_]);
            cell->LinkBorderCells(cell + width_, 0, cell + 1, cell - width_);
            cell = &(fluidmap_[(y+1)*width_-1]);
            cell->LinkBorderCells(cell + width_, cell - 1, 0, cell - width_);
        }
        // TopLeft
        cell = &(fluidmap_[0]);
        cell->LinkBorderCells(cell + width_, 0, cell + 1, 0);
        // TopRight
        cell = &(fluidmap_[width_-1]);
        cell->LinkBorderCells(cell + width_, cell - 1, 0, 0);
        // BottomLeft
        cell = &(fluidmap_[(height_-1)*width_]);
        cell->LinkBorderCells(0, 0, cell + 1, cell-width_);
        // BottomRight
        cell = &(fluidmap_[height_*width_-1]);
        cell->LinkBorderCells(0, cell - 1, 0, cell-width_);
    }

    if (depthZ)
    {
        FluidMap& fluidIn = depthZ->fluidmap_;

        if (fluidmap_[0].DepthZ != &(fluidIn[0]) || !linkedCells_)
        {
            unsigned size = height_ * width_;
            // Inner Cells
            for (unsigned addr = 0; addr < size; addr++)
                fluidmap_[addr].LinkDepthCell(&(fluidIn[addr]));
        }
    }
    else if (fluidmap_[0].DepthZ != 0 || !linkedCells_)
    {
        unsigned size = height_ * width_;
        for (unsigned addr = 0; addr < size; addr++)
            fluidmap_[addr].LinkDepthCell(0);
    }

    linkedCells_ = true;
}

void FluidDatas::LinkBorderCells(int direction, FluidDatas& fluidData)
{
    switch (direction)
    {
    case MapDirection::North:
        // Top Border Cells
        for (unsigned x = 0; x < width_; x++)
            fluidmap_[x].Top = &(fluidData.fluidmap_[(height_-1)*width_+x]);
        break;

    case MapDirection::South:
        // Bottom Border Cells
        for (unsigned x = 0; x < width_; x++)
            fluidmap_[(height_-1)*width_+x].Bottom = &(fluidData.fluidmap_[x]);
        break;

    case MapDirection::East:
        // Right Border Cells
        for (unsigned y = 0; y < height_; y++)
            fluidmap_[y*width_+width_-1].Right = &(fluidData.fluidmap_[y*width_]);
        break;

    case MapDirection::West:
        // Left Border Cells
        for (unsigned y = 0; y < height_; y++)
            fluidmap_[y*width_].Left = &(fluidData.fluidmap_[y*width_+width_-1]);
        break;
    }
}

void FluidDatas::UnLinkBorderCells(int direction)
{
    if (!fluidmap_.Size())
        return;

    switch (direction)
    {
    case MapDirection::AllBorders:

    case MapDirection::North:
        // Top Border Cells
        for (unsigned x = 0; x < width_; x++)
            fluidmap_[x].Top = 0;
        if (direction == MapDirection::North)
            break;

    case MapDirection::South:
        // Bottom Border Cells
        for (unsigned x = 0; x < width_; x++)
            fluidmap_[(height_-1)*width_+x].Bottom = 0;
        if (direction == MapDirection::South)
            break;

    case MapDirection::East:
        // Right Border Cells
        for (unsigned y = 0; y < height_; y++)
            fluidmap_[y*width_+width_-1].Right = 0;
        if (direction == MapDirection::East)
            break;

    case MapDirection::West:
        // Left Border Cells
        for (unsigned y = 0; y < height_; y++)
            fluidmap_[y*width_].Left = 0;
        if (direction == MapDirection::West)
            break;
    }
}

void FluidDatas::Clear()
{
    Pull();

    UnLinkBorderCells(MapDirection::AllBorders);

    sources_.Clear();
}

void FluidDatas::Pull()
{
    if (!fluidmap_.Size())
        return;

    for (FluidMap::Iterator it=fluidmap_.Begin(); it != fluidmap_.End(); ++it)
    {
        if (it->patternType_ >= WATER)
            it->Clear();
    }

    lastFrameUpdate_ = 0U;
}

const float fluidFromIntToFloat = (FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE) / 255.f;

void FluidDatas::SetCells()
{
    if (!featureMap_)
    {
        URHO3D_LOGWARNING("FluidDatas() - SetCells : No Feature Map !");
    }
    else
    {
        const PODVector<FeatureType>& features = *featureMap_;
        unsigned size = features.Size();

        MapData* mapdata = features_->map_->GetMapData();
        if (indexFluidZ_ < mapdata->fluidValues_.Size())
        {
            const PODVector<FeatureType>& fluidCellValues = mapdata->fluidValues_[indexFluidZ_];
            if (fluidCellValues.Size() != size)
            {
                // TODO : Fix it
                URHO3D_LOGERRORF("FluidDatas() - SetCells : fluidvalues not mapsized !");
                return;
            }
            
            URHO3D_LOGINFOF("FluidDatas() - SetCells : index=%d with fluidvalues !", indexFluidZ_);

            if (checkMap_)
            {
                PODVector<FeatureType>& checkCells = *checkMap_;
                for (unsigned addr = 0; addr < size; addr++)
                {
                    fluidmap_[addr].Set(features[addr], &checkCells[addr], (float)fluidCellValues[addr] * fluidFromIntToFloat);
                }
            }
            else
            {
                for (unsigned addr = 0; addr < size; addr++)
                    fluidmap_[addr].Set(features[addr], 0, (float)fluidCellValues[addr] * fluidFromIntToFloat);
            }

#ifdef DUMP_MAPDEBUG_FLUIDVIEW
            GameHelpers::DumpData(&fluidCellValues[0], 1, 2, width_, height_);
#endif
        }
        else
        {
            if (checkMap_)
            {
                PODVector<FeatureType>& checkCells = *checkMap_;
                for (unsigned addr = 0; addr < size; addr++)
                    fluidmap_[addr].Set(features[addr], &checkCells[addr]);
            }
            else
            {
                for (unsigned addr = 0; addr < size; addr++)
                    fluidmap_[addr].Set(features[addr], 0);
            }
        }
    }

#ifdef DUMP_MAPDEBUG_FLUIDVIEW
    GameHelpers::DumpData(&fluidmap_[0], 1, 2, width_, height_);
#endif
}

// TODO ASYNC
bool FluidDatas::UpdateMapData(HiresTimer* timer)
{
    MapData* mapdata = features_->map_->GetMapData();
    if (indexFluidZ_ >= mapdata->fluidValues_.Size())
        mapdata->fluidValues_.Resize(indexFluidZ_+1);

    PODVector<FeatureType>& fluidCellValues = mapdata->fluidValues_[indexFluidZ_];
    fluidCellValues.Resize(width_ * height_);

    URHO3D_LOGINFOF("FluidDatas() - UpdateMapData : index=%d !", indexFluidZ_);

    unsigned addr = 0;
    for (unsigned i = 0; i < height_; i++)
    {
        for (unsigned j = 0; j < width_; j++, addr++)
        {
            fluidCellValues[addr] = (unsigned char)(fluidmap_[addr].mass_ / fluidFromIntToFloat);
        }
    }

#ifdef DUMP_MAPDEBUG_FLUIDVIEW
    GameHelpers::DumpData(&fluidCellValues[0], 1, 2, width_, height_);
#endif

    return true;
}

