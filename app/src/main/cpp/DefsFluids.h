#pragma once

#include <Urho3D/Urho2D/Drawable2D.h>

#include "MapFeatureTypes.h"


using namespace Urho3D;

const float FLUID_MINDRAWF = PIXEL_SIZE * 2.f;
const float FLUID_MAXDRAWF = 1.0f;

// Max and min cell liquid values
const float FLUID_MAXVALUE = 1.0f;
const float FLUID_MINVALUE = 0.01f;

// Extra liquid a cell can store than the cell above it
const float FLUID_MAXCOMPRESSIONVALUE = 0.1f;

//const float FLUID_GROUNDIMPREGNATION = 0.001f;
//const float FLUID_MINGROUNDSATURATIONVALUE = 2.f*FLUID_MINVALUE;

// Lowest and highest amount of liquids allowed to flow per iteration
const float FLUID_MINflowValue_ = 0.01f;
const float FLUID_MAXflowValue_ = 1.2f;

// Adjusts flow speed (0.0f - 1.0f)
const float FLUID_FLOWSPEED = 1.0f;

enum MaterialType
{
    AIR         = 0,
    BLOCK       = 0x01,
    WATER       = 0x02,
    OIL         = 0x04,
    LAVA        = 0x08,
};

enum FluidDirection
{
    FD_None = 0,
    FD_Top = 1 << 0,
    FD_Bottom = 1 << 1,
    FD_Left = 1 << 2,
    FD_Right = 1 << 3
};

enum FluidAlignement
{
    FA_None = 0,
    FA_Center = 1,
    FA_Left = 2,
    FA_Right = 4,
    FA_All = 7
};

enum FluidPatternType
{
    FPT_Empty = 0,
    FPT_IsolatedWater,
    FPT_NoRender,
    FPT_WaterFall,
    FPT_Wave,
    FPT_Swirl,
    FPT_Surface,
    FPT_FullWater,
};

enum FluidPattern
{
    PTRN_Empty = 0,
    PTRN_Droplet,
// Swirl
    PTRN_Swirl,
// DepthZ
    PTRN_RightDepthZ,
    PTRN_LeftDepthZ,
    PTRN_CenteredDepthZ,
    PTRN_CenterFallDepthZ,
// Fall
    PTRN_RightFall,
    PTRN_LeftFall,
    PTRN_CenterLeftFall,
    PTRN_CenterRightFall,
    PTRN_LeftRightFall,
    PTRN_CenterFall1,
// Fall Head
    PTRN_RightFallHead,
    PTRN_LeftFallHead,
    PTRN_CenterLeftFallHead,
    PTRN_CenterRightFallHead,
    PTRN_LeftRightFallHead,
    PTRN_CenterFallHead,
    PTRN_CenterFallHead2,
    PTRN_SwirlHead,
// Fall Foot
    PTRN_RightFallFoot,
    PTRN_LeftFallFoot,
// Fall Part && Surface Part
    PTRN_CenterFallRightSurface,
    PTRN_CenterFallLeftSurface,
    PTRN_LeftFallAndRightSurface,
    PTRN_RightFallAndLeftSurface,
// Surface
    PTRN_CenterFallSurface,
    PTRN_LeftFallSurface,
    PTRN_RightFallSurface,
    PTRN_LeftRightFallSurface,
    PTRN_LeftSurface,
    PTRN_RightSurface,
    PTRN_TopLeftSurface,
    PTRN_TopRightSurface,
    PTRN_CenterSurface,
// Full
    PTRN_Full,
    PTRN_FullTest,
};

extern const char* FluidPatternTypeStr[];
extern const char* FluidPatternStr[];

const bool WaterLineAllowed_[] =
{
    false,
    false,
// Swirl
    false,
// DepthZ
    false,
    false,
    false,
    false,
// Fall
    false,
    false,
    false,
    false,
    false,
    false,
// Fall Head
    true,
    true,
    true,
    true,
    true,
    true,
    true,
    true,
// Fall Foot
    false,
    false,
// Fall Part && Surface Part
    true,
    true,
    true,
    true,
// Surface
    true,
    true,
    true,
    true,
    true,
    true,
    true,
    true,
    true,
// Full
    true,
    false,
};

struct FluidCell
{
    enum State
    {
        FS_Static         = 0x0001,
        SETTLED           = 0x0002,
        FS_SideTransfer   = 0x0004,
        FS_BelowResurge   = 0x0008,
        FS_AboveResurge   = 0x0010,
    };

    FluidCell() :
        type_(AIR),
        state_(0), stateCount_(0),
        align_(0), pattern_(0), patternType_(0),
        feat_(0), featCheck_(0)
    {
        ResetNeighBors();
        ResetMasses();
        ResetFlows();
    }

    FluidCell(int type, int patterntype)
    {
        ResetNeighBors();
        ResetMasses();
        ResetFlows();

        type_ = type;
        patternType_ = patterntype;
    }

    void Clear()
    {
        type_ = AIR;
        state_ = 0;
        stateCount_ = 0;
        align_ = 0;
        pattern_ = 0;
        patternType_ = 0;
        feat_ = 0;
        featCheck_ = 0;

        ResetMasses();
        ResetFlows();
    }

    void LinkBorderCells(FluidCell* bottom, FluidCell* left, FluidCell* right, FluidCell* top)
    {
        Bottom = bottom;
        Left = left;
        Right = right;
        Top = top;
    }

    void LinkDepthCell(FluidCell* depthZ)
    {
        DepthZ = depthZ;
    }

    // Force neighbors to simulate on next iteration
    void UnsettleNeighbors()
    {
        if (DepthZ)
            DepthZ->SetState(SETTLED, false);
        if (Bottom)
            Bottom->SetState(SETTLED, false);
        if (Left)
            Left->SetState(SETTLED, false);
        if (Right)
            Right->SetState(SETTLED, false);
        if (Top)
        {
            Top->SetState(SETTLED, false);
            if (Top->Right)
                Top->Right->SetState(SETTLED, false);
            if (Top->Left)
                Top->Left->SetState(SETTLED, false);
        }
    }

    void Set(FeatureType feat)
    {
        if (feat == MapFeatureType::Water)
        {
            type_ = WATER;
            state_ = 0;

            massL_ = massR_ = 0;
            mass_ = massC_ = FLUID_MAXVALUE;
            ResetFlows();
        }
        else
        {
            if (feat > MapFeatureType::NoRender && feat < MapFeatureType::CorridorPlateForm)
            {
                type_ = BLOCK;
                state_ = FS_Static;
            }
            else
            {
                type_ = AIR;
                state_ = 0;
            }

            ResetMasses();
            ResetFlows();
        }

        stateCount_ = 0;
        feat_ = feat;
    }

    void Set(FeatureType feat, FeatureType *check, float value=0.f)
    {
        if (feat > MapFeatureType::NoRender && feat < MapFeatureType::CorridorPlateForm)
        {
            type_ = BLOCK;
            state_ = FS_Static;

            ResetMasses();
        }
        else if (feat == MapFeatureType::Water || value > 0.f)
        {
            type_ = WATER;
            state_ = 0;

            massL_ = massR_ = 0;
            mass_ = massC_ = value;
        }
        else
        {
            type_ = AIR;
            state_ = 0;

            ResetMasses();
        }

        ResetFlows();

        stateCount_ = 0;
        feat_ = feat;
        featCheck_ = check;
    }

    void SetState(State state, bool enable)
    {
        if (type_ == BLOCK)
        {
            if (state_ != FS_Static)
                state_ = FS_Static;
            return;
        }

        if (enable)
            state_ |= state;
        else
            state_ &= ~state;

        if (!enable && !(state_ & SETTLED))
            stateCount_ = 0;
    }

    inline bool HasState(State state) const
    {
        return state_ && state;
    }

    // add fluid and return accepted flow
    void AddFluid(int type, float amount)
    {
        SetState(SETTLED, false);
        type_ |= type;

        massC_ += amount;

        UpdateMass();
    }

//    float AddFluid(int type, float value)
//	{
//	    if (value <= 0.f)
//            return 0.f;
//
//        SetState(SETTLED, false);
//
//        if (value + massC_ < FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE)
//        {
//            massC_ += value;
//        }
//        else
//        {
//            massC_ += FLUID_MAXCOMPRESSIONVALUE;
//        }
//
//        type_ |= type;
//        UpdateMass();
//
////        value = Min(value, FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE - mass_);
////
////        if (value > 0.f)
////        {
////            type_ |= type;
////            massC_ += value;
////            UpdateMass();
////        }
//
//		return value;
//	}

    bool Update()
    {
        if (type_ == BLOCK)
            return false;

        if (flowC_ != 0.f)
        {
            if (massC_+flowC_< FLUID_MINVALUE && (!Top || Top->type_ < WATER))
            {
                type_ = AIR;
                state_ = 0;

                mass_ = massC_ = 0.f;
                flowdir_ = FD_None;
                ResetDirections();
            }
            else
            {
                type_ = WATER;
                SetState(FluidCell::SETTLED, false);

                massC_ += flowC_;
                mass_ = massC_;
            }

            flowC_ = 0;
            return true;
        }

        return false;
    }

    bool Update3Flows()
    {
        if (type_ == BLOCK)
            return false;

        if (flowL_ != 0.f || flowC_ != 0.f || flowR_ != 0.f)
        {
            // AIR
            if ((massL_+massC_+massR_+flowL_+flowC_+flowR_)/3.f < FLUID_MINVALUE && (!Top || Top->type_ < WATER))
            {
                Reset();
            }
            // FLUID
            else
            {
                // central overFlow : Transfer to left and right
                if (massC_ + flowC_ > FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE)
                {
                    // remainder
                    flowC_ = (massC_+flowC_-FLUID_MAXVALUE-FLUID_MAXCOMPRESSIONVALUE)/2.f;
                    flowL_ += flowC_;
                    flowR_ += flowC_;
                    massC_ = FLUID_MAXVALUE+FLUID_MAXCOMPRESSIONVALUE;
                }
                else massC_ += flowC_;

                // left overFlow : Transfer and assure no overflow on central
                if (massL_ + flowL_ > FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE)
                {
                    massC_ = Min(massC_+ (FLUID_MAXVALUE+FLUID_MAXCOMPRESSIONVALUE) - (flowL_+massL_), FLUID_MAXVALUE+FLUID_MAXCOMPRESSIONVALUE);
                    massL_ = FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE;
                }
                else massL_ += flowL_;

                // right overFlow : Transfer and assure no overflow on central
                if (massR_ + flowR_ > FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE)
                {
                    massC_ = Min(massC_+ (FLUID_MAXVALUE+FLUID_MAXCOMPRESSIONVALUE) - (flowR_+massR_), FLUID_MAXVALUE+FLUID_MAXCOMPRESSIONVALUE);
                    massR_ = FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE;
                }
                else massR_ += flowR_;

                type_ = WATER;
                SetState(FluidCell::SETTLED, false);

//                massL_ += Min(flowL_, FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE - massL_);
//                massC_ += Min(flowC_, FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE - massC_);
//                massR_ += Min(flowR_, FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE - massR_);
                UpdateMass();
            }
            return true;
        }

        return false;
    }

    void ResetNeighBors()
    {
        DepthZ = Bottom = Left = Right = Top = 0;
    }
    void ResetMasses()
    {
        mass_ = massL_ = massC_ = massR_ = 0.f;
    }
    void ResetFlows()
    {
        flowL_ = flowC_ = flowR_ = 0.f;
        flowdir_ = FD_None;
    }
    void ResetDirections()
    {
        if (Top && Top->type_ >= WATER) Top->flowdir_ &= ~FD_Bottom;
        if (Bottom && Bottom->type_ >= WATER) Bottom->flowdir_ &= ~FD_Top;
        if (Left && Left->type_ >= WATER) Left->flowdir_ &= ~FD_Right;
        if (Right && Right->type_ >= WATER) Right->flowdir_ &= ~FD_Left;
    }
    void Reset()
    {
        type_ = AIR;
        state_ = 0;

        ResetMasses();
        ResetFlows();
        ResetDirections();
    }

    void UpdateMass()
    {
        mass_ = (massR_ > 0.f || massL_ > 0.f) ? (massL_+massC_+massR_)/3.f : massC_;
    }
    bool UpdateAlignement();
    bool UpdatePattern();

    float GetMass() const
    {
        return mass_;
    }

    void Dump() const;

    int type_;
    int state_;

    float flowL_, flowC_, flowR_;
    float massL_, massC_, massR_;
    float mass_;

    int flowdir_;
    int stateCount_;

    int align_;
    int pattern_;
    int patternType_;

    FeatureType feat_, *featCheck_;

    bool drawn_;

    // Neighboring cells
    FluidCell* DepthZ;
    FluidCell* Bottom;
    FluidCell* Left;
    FluidCell* Right;
    FluidCell* Top;

    static const FluidCell FullWaterCell;
};


typedef PODVector<FluidCell> FluidMap;

struct FluidSource
{
    FluidSource(MaterialType type, int x, int y, float qty, float flow) :
        fluid_(type), startx_(x), starty_(y), quantity_(qty), flow_(flow) { }

    MaterialType fluid_;
    int startx_;
    int starty_;
    float quantity_;
    float flow_;
};

struct ObjectFeatured;

struct FluidDatas
{
    FluidDatas() : lastFrameUpdate_(0U), viewZ_(0), indexFluidZ_(0), featureMap_(0), checkMap_(0), linkedCells_(false) { }
    void Resize(int index, unsigned width, unsigned height);

    void LinkFeatureMaps(ObjectFeatured* features, PODVector<FeatureType>* featureMap, PODVector<FeatureType>* checkMap)
    {
        features_ = features;
        featureMap_ = featureMap;
        checkMap_ = checkMap;
    }
    void LinkInnerCells(FluidDatas* depthZ);
    void LinkBorderCells(int direction, FluidDatas& fluidMap);
    void UnLinkBorderCells(int direction);

    void Clear();
    void Pull();
    void SetCells();
    bool UpdateMapData(HiresTimer* timer);

    ObjectFeatured* features_;

    unsigned lastFrameUpdate_;

    int viewZ_;
    int indexFluidZ_;

    PODVector<FeatureType>* featureMap_;
    PODVector<FeatureType>* checkMap_;

    FluidMap fluidmap_;
    PODVector<FluidSource> sources_;

    bool linkedCells_;

    static unsigned width_, height_;
};

