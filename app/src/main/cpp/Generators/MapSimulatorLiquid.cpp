#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Urho2D/Renderer2D.h>

#include "GameOptionsTest.h"
#include "GameContext.h"

#include "MemoryObjects.h"

#include "MapSimulatorLiquid.h"


#define FLUID_EQUALIZE

MapSimulatorLiquid* MapSimulatorLiquid::simulator_ = 0;

int MapSimulatorLiquid::mode_ = FLUID_SIMULATION;
int MapSimulatorLiquid::numiterations_ = FLUID_ITERATIONS;


MapSimulatorLiquid::MapSimulatorLiquid() :
    MapGenerator("MapSimulatorLiquid"),
    fluidDatas_(0)
{
    simulator_ = this;
}

MapSimulatorLiquid::~MapSimulatorLiquid()
{
    if (simulator_ == this)
        simulator_ = 0;
}

int MapSimulatorLiquid::Update(FluidDatas& fluiddatas)
{
    if (fluiddatas.lastFrameUpdate_ == GameContext::Get().renderer2d_->GetCurrentFrameInfo().frameNumber_)
    {
//        URHO3D_LOGWARNINGF("MapSimulatorLiquid() - Update : fluidDatas_=%u ... already done this frame ...", &fluiddatas);
        return 1;
    }

    fluidDatas_ = &fluiddatas;

//    URHO3D_LOGINFOF("MapSimulatorLiquid() - Update : fluidDatas_=%u ... xSize=%d ySize=%d ...", fluidDatas_, xSize_, ySize_);

    // Update FluidSources
    for (PODVector<FluidSource>::Iterator it=fluiddatas.sources_.Begin(); it!=fluiddatas.sources_.End(); ++it)
    {
        if (it->quantity_ < 0)
            continue;

        FluidSource& source = *it;

        FluidCell& cell = fluiddatas.fluidmap_[xSize_ * source.starty_ + source.startx_];
        cell.AddFluid(source.fluid_, source.flow_);
        source.quantity_ -= source.flow_;
    }

    // Update Fluids
    int numiterations = numiterations_;
    if (mode_ == 5)
        while (numiterations--)
            Simulate5();

    else if (mode_ == 6)
        while (numiterations--)
            Simulate6();
//        URHO3D_LOGINFOF("MapSimulatorLiquid() - Update : Simulation=%d Iterations=%d ... amountUpdated=%u OK !", params_[0], params_.Size() > 1 ? params_[1] : 1, amountUpdated_);

    fluidDatas_->lastFrameUpdate_ = GameContext::Get().renderer2d_->GetCurrentFrameInfo().frameNumber_;

    return amountUpdated_;
}

void MapSimulatorLiquid::Make()
{
//    URHO3D_LOGINFOF("MapSimulatorLiquid() - Make ...");

//    URHO3D_LOGINFOF("MapSimulatorLiquid() - Make ... OK !");
}


/// Simulation Section


// Calculate how much liquid should flow to Vertical destination with pressure
inline float CalculateFlow(float totalmass)
{
    if (totalmass <= FLUID_MAXVALUE)
        return FLUID_MAXVALUE;
    else if (totalmass < 2.f * FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE)
        return (FLUID_MAXVALUE * FLUID_MAXVALUE + totalmass * FLUID_MAXCOMPRESSIONVALUE) / (FLUID_MAXVALUE + FLUID_MAXCOMPRESSIONVALUE);
    else
        return (totalmass + FLUID_MAXCOMPRESSIONVALUE) * 0.5f;
}

#ifdef FLUID_EQUALIZE
//bool Equalize1SideF(FluidMap& fluidmap, int numPasses)
//{
//    bool updated=false;
//    int pass = 0;
//    int dirInc = -1;
//    int addr;
//
//    while (pass++ < numPasses)
//    {
//        dirInc = -dirInc;
//        const int start = dirInc > 0 ? 0 : fluidmap.Size()-1;
//        const int end = dirInc > 0 ? fluidmap.Size() : -1;
//
//        for (addr = start; addr != end; addr+=dirInc)
//        {
//            FluidCell& cell = fluidmap[addr];
//
//            // skip block
//            if (cell.type_ < WATER)
//                continue;
//
//            // No support => Skip
//            if (!cell.Bottom || (cell.Bottom->type_ != BLOCK && cell.Bottom->massC_ < FLUID_MAXVALUE))
//                continue;
//
//            FluidCell* side = dirInc > 0 ? cell.Right : cell.Left;
//            if (!side || side->massC_ < FLUID_MINVALUE)
//                continue;
//
//            cell.mass_ = cell.massC_ = side->mass_ = side->massC_ = Max((cell.massC_ + side->massC_) * 0.5f, FLUID_MINVALUE);
//            updated = true;
//        }
//    }
//
//	return (updated);
//}

bool Equalize1SideF(FluidMap& fluidmap, int numPasses)
{
    bool updated=false;
    int pass = 0;
    int dirInc = -1;
    int addr;

    while (pass++ < numPasses)
    {
        dirInc = -dirInc;

        const int start = dirInc > 0 ? 0 : fluidmap.Size()-1;
        const int end = dirInc > 0 ? fluidmap.Size() : -1;

        for (addr = start; addr != end; addr+=dirInc)
        {
            FluidCell& cell = fluidmap[addr];

            if (cell.massC_ < FLUID_MINVALUE || !cell.Bottom || (cell.Bottom->type_!= BLOCK && cell.Bottom->massC_ < FLUID_MINVALUE))
                continue;

            FluidCell* side = dirInc > 0 ? cell.Right : cell.Left;
            if (!side || side->massC_ < FLUID_MINVALUE || !side->Bottom || (side->Bottom->type_!= BLOCK && side->Bottom->massC_ < FLUID_MINVALUE))
                continue;

            cell.mass_ = cell.massC_ = side->mass_ = side->massC_ = Max((cell.massC_ + side->massC_) * 0.5f, FLUID_MINVALUE);
            updated = true;
        }
    }

    return (updated);
}

bool Equalize3SideF(FluidMap& fluidmap, int numPasses)
{
    bool updated=false;
    int pass = 0;
    int dirInc = -1;
    int addr;

    while (pass++ < numPasses)
    {
        dirInc = -dirInc;

        if (dirInc > 0)
        {
            for (addr = 0; addr < fluidmap.Size(); addr++)
            {
                FluidCell& cell = fluidmap[addr];
                if (cell.GetMass() < FLUID_MINVALUE || !cell.Bottom || (cell.Bottom->type_!= BLOCK && cell.Bottom->GetMass() < FLUID_MINVALUE))
                    continue;

                if (!cell.Right || cell.Right->GetMass() < FLUID_MINVALUE || !cell.Right->Bottom || (cell.Right->Bottom->type_!= BLOCK && cell.Right->Bottom->GetMass() < FLUID_MINVALUE))
                    continue;

                FluidCell& right = *cell.Right;
                cell.massR_ = right.massL_ = Max((cell.massR_ + right.massL_) * 0.5f, FLUID_MINVALUE);
                cell.massC_ = (cell.massL_ + cell.massR_) * 0.5f;
                cell.UpdateMass();
                right.massC_ = (right.massL_ + right.massR_) * 0.5f;
                right.UpdateMass();
                updated = true;
            }
        }
        else
        {
            for (addr = fluidmap.Size()-1; addr >= 0; addr--)
            {
                FluidCell& cell = fluidmap[addr];
                if (cell.GetMass() < FLUID_MINVALUE || !cell.Bottom || (cell.Bottom->type_!= BLOCK && cell.Bottom->GetMass() < FLUID_MINVALUE))
                    continue;

                if (!cell.Left || cell.Left->GetMass() < FLUID_MINVALUE || !cell.Left->Bottom || (cell.Left->Bottom->type_!= BLOCK && cell.Left->Bottom->GetMass() < FLUID_MINVALUE))
                    continue;

                FluidCell& left =  *cell.Left;
                cell.massL_ = left.massR_ = (cell.massL_ + left.massR_) * 0.5f;
                cell.massC_ = (cell.massL_ + cell.massR_) * 0.5f;
                cell.UpdateMass();
                left.massC_ = (left.massL_ + left.massR_) * 0.5f;
                left.UpdateMass();
                updated = true;
            }
        }
    }

    return (updated);
}

#endif // FLUID_EQUALIZE

/// Simulation 5 : Simple Compression Simulation (based on src:http://www.jgallant.com/2d-liquid-simulator-with-cellular-automaton-in-unity/)

void MapSimulatorLiquid::Simulate5()
{
    if (!fluidDatas_)
        return;

    amountUpdated_ = 0;

    FluidMap& fluidmap = fluidDatas_->fluidmap_;
    const unsigned size = fluidmap.Size();

    // 0 - Update Border Cells Only (for the changes made by linked Maps)
    {
        int x,y;

        // Top Border Cells
        for (x = 0; x < xSize_; x++)
            fluidmap[x].Update();
        // Bottom Border Cells
        for (x = 0; x < xSize_; x++)
            fluidmap[(ySize_-1)*xSize_ + x].Update();
        // Left Border Cells
        for (y = 1; y < ySize_-1; y++)
            fluidmap[y*xSize_].Update();
        // Right Border Cells
        for (y = 1; y < ySize_-1; y++)
            fluidmap[(y+1)*xSize_-1].Update();
    }

    // 1 - Transfer Mass
    {
        float startMass, rMassC;
        float flow, sideFlow;
        bool flowToLeft, flowToRight;

        unsigned addr;

        // Reset flow direction
        for (addr = 0; addr < size; addr++)
        {
            FluidCell& cell = fluidmap[addr];

            if (cell.type_ >= WATER && !cell.HasState(FluidCell::SETTLED))
                cell.flowdir_ = FD_None;
        }

        // Main loop
        for (addr = 0; addr < size; addr++)
        {
            FluidCell& cell = fluidmap[addr];

            // Validate cell
            if (cell.type_ == BLOCK)
                continue;

            if (cell.mass_ > 0.f && cell.mass_ < 0.75f*FLUID_MINVALUE)
            {
                cell.Reset();
                continue;
            }

            if (cell.massC_ < FLUID_MINVALUE)
                continue;

            if (cell.HasState(FluidCell::SETTLED))
                continue;

            // Keep track of how much liquid this cell started off with
            startMass = rMassC = cell.massC_;

            // Flow to bottom cell
            if (cell.Bottom)
            {
                if (cell.Bottom->type_ != BLOCK)
                {
                    // Determine flow
                    flow = Clamp((CalculateFlow(rMassC + cell.Bottom->massC_) - cell.Bottom->massC_) * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassC));

                    // Update flows
                    if (flow != 0.f)
                    {
                        rMassC -= flow;
                        cell.flowC_ -= flow;
                        cell.Bottom->flowC_ += flow;
                        cell.flowdir_ |= FD_Bottom;
                        cell.Bottom->flowdir_ |= FD_Top;
                        cell.Bottom->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }
                // If Bottom Block, Flow to DepthZ cell
                else
                {
                    // Flow only if No Block in DepthZ and Check
                    if (cell.DepthZ && cell.DepthZ->type_ != BLOCK && (!cell.featCheck_ || (*cell.featCheck_) <= MapFeatureType::Threshold))
                    {
                        // Calculate flow rate
                        flow = (rMassC - cell.DepthZ->massC_) / 4.f;

                        if (flow > FLUID_MINflowValue_)
                            flow *= FLUID_FLOWSPEED;

                        flow *= FLUID_FLOWSPEED;

                        // Constrain flow
                        flow = Max(0.f, flow);
                        if (flow > Min(FLUID_MAXflowValue_, rMassC))
                            flow = Min(FLUID_MAXflowValue_, rMassC);

                        // Update flows
                        if (flow != 0.f)
                        {
                            rMassC -= flow;
                            cell.flowC_ -= flow;
                            cell.DepthZ->flowC_ += flow;
                            cell.DepthZ->SetState(FluidCell::SETTLED, false);
                            cell.DepthZ->Update();
                            amountUpdated_++;
                        }
                    }
                }
            }
#ifdef FLUID_SIMULATION_REMOVEFLOWATEMPTYBORDER
            else
            {
                flow = CalculateFlow(rMassC);
                rMassC -= flow;
                cell.flowC_ -= flow;
                amountUpdated_++;
                continue;
            }

            sideFlow = rMassC;

            if (!cell.Left)
            {
                rMassC -= sideFlow/2.f;
                cell.flowC_ -= sideFlow/2.f;
                amountUpdated_++;
            }

            if (!cell.Right)
            {
                rMassC -= sideFlow/2.f;
                cell.flowC_ -= sideFlow/2.f;
                amountUpdated_++;
            }
#endif

            // Check to ensure we still have liquid in this cell
            if (rMassC < FLUID_MINVALUE)
            {
                cell.flowC_ -= rMassC;
                continue;
            }

            // Flow to side cells
            flowToLeft = cell.Left && cell.Left->type_ != BLOCK && rMassC > cell.Left->massC_;
            flowToRight = cell.Right && cell.Right->type_ != BLOCK && rMassC > cell.Right->massC_;
            sideFlow = rMassC;

            if (flowToLeft || flowToRight)
            {
                // Flow to left cell
                if (flowToLeft)
                {
                    // Determine flow
                    flow = Max(0.f, Min((sideFlow - cell.Left->massC_)/4.f*FLUID_FLOWSPEED, FLUID_MAXflowValue_));

                    // Update flows
                    if (flow != 0.f)
                    {
                        rMassC -= flow;
                        cell.flowC_ -= flow;
                        cell.Left->flowC_ += flow;
                        cell.flowdir_ |= FD_Left;
                        cell.Left->flowdir_ |= FD_Right;
                        cell.Left->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }

                // Check to ensure we still have liquid in this cell
                if (rMassC < FLUID_MINVALUE)
                {
                    cell.flowC_ -= rMassC;
                    continue;
                }

                // Flow to right cell
                if (flowToRight)
                {
                    // Determine flow
                    flow = Max(0.f, Min((sideFlow - cell.Right->massC_)/4.f*FLUID_FLOWSPEED, FLUID_MAXflowValue_));

                    // Update flows
                    if (flow != 0.f)
                    {
                        rMassC -= flow;
                        cell.flowC_ -= flow;
                        cell.Right->flowC_ += flow;
                        cell.flowdir_ |= FD_Right;
                        cell.Right->flowdir_ |= FD_Left;
                        cell.Right->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }
            }

//            // Flow to left cell
//            if (cell.Left)
//            {
//                if (cell.Left->type_ != BLOCK)
//                {
//                    // Calculate flow rate
//                    flow = (rMassC - cell.Left->massC_) / 4.f;
//                    if (flow > FLUID_MINflowValue_)
//                        flow *= FLUID_FLOWSPEED;
//
//                    // Constrain flow
//                    flow = Max(0.f, flow);
//                    if (flow > Min(FLUID_MAXflowValue_, rMassC))
//                        flow = Min(FLUID_MAXflowValue_, rMassC);
//
//                    // Adjust temp values
//                    if (flow != 0.f)
//                    {
//                        rMassC -= flow;
//                        cell.flowC_ -= flow;
//                        cell.Left->flowC_ += flow;
//                        cell.Left->SetState(FluidCell::SETTLED, false);
//                        amountUpdated_++;
//                    }
//                }
//            }
//            #ifdef FLUID_SIMULATION_REMOVEFLOWATEMPTYBORDER
//            else
//            {
//                flow = CalculateFlow(rMassC);
//                rMassC -= flow;
//                cell.flowC_ -= flow;
//                amountUpdated_++;
//            }
//            #endif
//
//            // Check to ensure we still have liquid in this cell
//            if (rMassC < FLUID_MINVALUE)
//            {
//                cell.flowC_ -= rMassC;
//                continue;
//            }
//
//            // Flow to right cell
//            if (cell.Right)
//            {
//                if (cell.Right->type_ != BLOCK)
//                {
//                    // calc flow rate
//                    flow = (rMassC - cell.Right->massC_) / 3.f;
//                    if (flow > FLUID_MINflowValue_)
//                        flow *= FLUID_FLOWSPEED;
//
//                    // Constrain flow
//                    flow = Max(0.f, flow);
//                    if (flow > Min(FLUID_MAXflowValue_, rMassC))
//                        flow = Min(FLUID_MAXflowValue_, rMassC);
//
//                    // Adjust temp values
//                    if (flow != 0.f)
//                    {
//                        rMassC -= flow;
//                        cell.flowC_ -= flow;
//                        cell.Right->flowC_ += flow;
//                        cell.Right->SetState(FluidCell::SETTLED, false);
//                        amountUpdated_++;
//                    }
//                }
//            }
//            #ifdef FLUID_SIMULATION_REMOVEFLOWATEMPTYBORDER
//            else
//            {
//                flow = CalculateFlow(rMassC);
//                rMassC -= flow;
//                cell.flowC_ -= flow;
//                amountUpdated_++;
//            }
//            #endif

            // Check to ensure we still have liquid in this cell
            if (rMassC < FLUID_MINVALUE)
            {
                cell.flowC_ -= rMassC;
                continue;
            }

            // Flow to Top cell
            if (cell.Top)
            {
                if (cell.Top->type_ != BLOCK)
                {
                    flow = rMassC - CalculateFlow(rMassC + cell.Top->massC_);
                    if (flow > FLUID_MINflowValue_)
                        flow *= FLUID_FLOWSPEED;

                    // Constrain flow
                    flow = Max(0.f, flow);
                    if (flow > Min(FLUID_MAXflowValue_, rMassC))
                        flow = Min(FLUID_MAXflowValue_, rMassC);

                    // Update flows
                    if (flow != 0.f)
                    {
                        rMassC -= flow;
                        cell.flowC_ -= flow;
                        cell.Top->flowC_ += flow;
                        cell.flowdir_ |= FD_Top;
                        cell.Top->flowdir_ |= FD_Bottom;
                        cell.Top->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }
            }
#ifdef FLUID_SIMULATION_REMOVEFLOWATEMPTYBORDER
            else
            {
                flow = CalculateFlow(rMassC);
                rMassC -= flow;
                cell.flowC_ -= flow;
                amountUpdated_++;
            }
#endif

            // Check to ensure we still have liquid in this cell
            if (rMassC < FLUID_MINVALUE)
            {
                cell.flowC_ -= rMassC;
                continue;
            }

            // Check if cell is settled
            if (startMass == rMassC)
            {
                cell.stateCount_++;
                if (cell.stateCount_ >= 10)
                    cell.SetState(FluidCell::SETTLED, true);
            }
            else
            {
                cell.UnsettleNeighbors();
            }
        }
    }

    // 2 - Update Cells
    if (amountUpdated_)
    {
        for (unsigned addr = 0; addr < size; addr++)
            if (fluidmap[addr].Update())
                amountUpdated_++;
    }

#ifdef FLUID_EQUALIZE
    if (amountUpdated_)
        if (Equalize1SideF(fluidmap, 2))
            amountUpdated_++;
#endif
}



void MapSimulatorLiquid::Simulate6()
{
    if (!fluidDatas_)
        return;

    amountUpdated_ = 0;

    FluidMap& fluidmap = fluidDatas_->fluidmap_;
    const unsigned size = fluidmap.Size();

    // 0 - Update Border Cells Only (for the changes made by linked Maps)
    {
        int x,y;

        // Top Border Cells
        for (x = 0; x < xSize_; x++)
            fluidmap[x].Update3Flows();
        // Bottom Border Cells
        for (x = 0; x < xSize_; x++)
            fluidmap[(ySize_-1)*xSize_ + x].Update3Flows();
        // Left Border Cells
        for (y = 1; y < ySize_-1; y++)
            fluidmap[y*xSize_].Update3Flows();
        // Right Border Cells
        for (y = 1; y < ySize_-1; y++)
            fluidmap[(y+1)*xSize_-1].Update3Flows();
    }

    // 1 - Transfer Mass
    {
        float rMassL, rMassC, rMassR;
        float flowL, flowC, flowR;
        bool flowToLeft, flowToRight;

        unsigned addr;

        // Reset flows
        for (addr = 0; addr < size; addr++)
        {
            FluidCell& cell = fluidmap[addr];

            if (cell.type_ >= WATER && !cell.HasState(FluidCell::SETTLED))
            {
                cell.flowdir_ = FD_None;

                cell.ResetFlows();
                cell.ResetDirections();
            }
        }

        // Main loop
        for (addr = 0; addr < size; addr++)
        {
            FluidCell& cell = fluidmap[addr];

            // Validate cell
            if (cell.type_ == BLOCK)
                continue;

            if (cell.HasState(FluidCell::SETTLED))
                continue;

            rMassL = cell.massL_;
            rMassC = cell.massC_;
            rMassR = cell.massR_;

            if (rMassL < FLUID_MINVALUE && rMassC < FLUID_MINVALUE && rMassR < FLUID_MINVALUE)
                continue;

            // Flow to bottom cell
            if (cell.Bottom)
            {
                if (cell.Bottom->type_ != BLOCK)
                {
                    // Determine flows
                    flowL = Clamp((CalculateFlow(rMassL + cell.Bottom->massL_) - cell.Bottom->massL_) * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassL));
                    flowC = Clamp((CalculateFlow(rMassC + cell.Bottom->massC_) - cell.Bottom->massC_) * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassC));
                    flowR = Clamp((CalculateFlow(rMassR + cell.Bottom->massR_) - cell.Bottom->massR_) * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassR));
//                    flowL = Max(0.f, CalculateFlow(rMassL + cell.Bottom->massL_) - cell.Bottom->massL_);
//                    flowC = Max(0.f, CalculateFlow(rMassC + cell.Bottom->massC_) - cell.Bottom->massC_);
//                    flowR = Max(0.f, CalculateFlow(rMassR + cell.Bottom->massR_) - cell.Bottom->massR_);
                    // Update values
                    if (flowL != 0.f || flowC != 0.f || flowR != 0.f)
                    {
                        if (flowL != 0.f)
                        {
                            rMassL -= flowL;
                            cell.flowL_ -= flowL;
                            cell.Bottom->flowL_ += flowL;
                        }
                        if (flowC != 0.f)
                        {
                            rMassC -= flowC;
                            cell.flowC_ -= flowC;
                            cell.Bottom->flowC_ += flowC;
                        }
                        if (flowR != 0.f)
                        {
                            rMassR -= flowR;
                            cell.flowR_ -= flowR;
                            cell.Bottom->flowR_ += flowR;
                        }
                        cell.flowdir_ |= FD_Bottom;
                        cell.Bottom->flowdir_ |= FD_Top;
                        cell.Bottom->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }
                // If Bottom Block, Flow to DepthZ cell
                else
                {
//                    // Flow only if No Block in DepthZ and Check
//                    if (cell.DepthZ && cell.DepthZ->type_ != BLOCK && cell.featCheck_ < MapFeatureType::InnerFloor)
//                    {
//                        // Determine flow Central
//                        flowC = Clamp((rMassC - cell.DepthZ->massC_) / 4.f * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassC));
//
//                        // Update values
//                        if (flowC != 0.f)
//                        {
//                            rMassC -= flowC;
//                            cell.flowC_ -= flowC;
//                            cell.DepthZ->flowC_ += flowC;
//                            cell.DepthZ->SetState(FluidCell::SETTLED, false);
//                            cell.DepthZ->Update3Flows();
//                            amountUpdated_++;
//                        }
//                    }
                    flowL = flowC = flowR = 0.f;
                }

                // no equalized masses and no flow, equalize Inner masses
                if (rMassC != rMassL || rMassC != rMassR)
                    if (flowL == 0.f && flowC == 0.f && flowR == 0.f)
                    {
                        rMassL = rMassC = rMassR = (rMassL+rMassC+rMassR)/3.f;
                        // Inner Mass & Flow Update
                        cell.flowL_ = cell.massL_ - rMassL;
                        cell.flowC_ = cell.massC_ - rMassC;
                        cell.flowR_ = cell.massR_ - rMassR;
                    }
            }
#ifdef FLUID_SIMULATION_REMOVEFLOWATEMPTYBORDER
            else
            {
//                cell.flowL_ -= rMassL;
//                cell.flowC_ -= rMassC;
//                cell.flowR_ -= rMassR;
                cell.flowL_ -= CalculateFlow(rMassL);
                cell.flowC_ -= CalculateFlow(rMassC);
                cell.flowR_ -= CalculateFlow(rMassR);
                amountUpdated_++;
                continue;
            }

            if (!cell.Left)
            {
                cell.flowL_ -= rMassL;
                rMassL = 0.f;
                amountUpdated_++;
            }

            if (!cell.Right)
            {
                cell.flowR_ -= rMassR;
                rMassR = 0.f;
                amountUpdated_++;
            }
#endif

            // Check to ensure we still have liquid in this cell
            if (rMassL+rMassC+rMassR < FLUID_MINVALUE)
            {
                cell.flowL_ -= rMassL;
                cell.flowC_ -= rMassC;
                cell.flowR_ -= rMassR;
                continue;
            }

            // Flow to side cells
            flowToLeft = cell.Left && cell.Left->type_ != BLOCK && cell.mass_ > cell.Left->mass_;
            flowToRight = cell.Right && cell.Right->type_ != BLOCK && cell.mass_ > cell.Right->mass_;

            if (flowToLeft || flowToRight)
            {
                // Flow to left cell
                if (flowToLeft)
                {
                    // Determine flow
//                    flowL = Max(0.f, (rMassL - cell.Left->massR_)/4.f*FLUID_FLOWSPEED);
                    flowL = Clamp((rMassL - cell.Left->massR_) / 4.f * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassL));

                    // Update flows
                    if (flowL != 0.f)
                    {
                        rMassL -= flowL;
                        cell.flowL_ -= flowL;
                        cell.Left->flowR_ += flowL;
                        cell.flowdir_ |= FD_Left;
                        cell.Left->flowdir_ |= FD_Right;
                        cell.Left->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }

                // Check to ensure we still have liquid in this cell
                if (rMassL+rMassC+rMassR < FLUID_MINVALUE)
                {
                    cell.flowL_ -= rMassL;
                    cell.flowC_ -= rMassC;
                    cell.flowR_ -= rMassR;
                    continue;
                }

                // Flow to right cell
                if (flowToRight)
                {
                    // Determine flow
//                    flowR = Max(0.f, (rMassR - cell.Right->massL_)/4.f*FLUID_FLOWSPEED);
                    flowR = Clamp((rMassR - cell.Right->massL_) / 4.f * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassR));

                    // Update flows
                    if (flowR != 0.f)
                    {
                        rMassR -= flowR;
                        cell.flowR_ -= flowR;
                        cell.Right->flowL_ += flowR;
                        cell.flowdir_ |= FD_Right;
                        cell.Right->flowdir_ |= FD_Left;
                        cell.Right->SetState(FluidCell::SETTLED, false);
                        amountUpdated_++;
                    }
                }
            }

            // Check to ensure we still have liquid in this cell
            if (rMassL+rMassC+rMassR < FLUID_MINVALUE)
            {
                cell.flowL_ -= rMassL;
                cell.flowC_ -= rMassC;
                cell.flowR_ -= rMassR;
                continue;
            }

//            // Flow to Top cell
//            if (cell.Top)
//            {
//                if (cell.Top->type_ != BLOCK && rMassC > cell.Top->massC_)
//                {
//                    // Determine flows
//                    flowC = Clamp(rMassC - (CalculateFlow(rMassC + cell.Top->massC_)) * FLUID_FLOWSPEED, 0.f, Min(FLUID_MAXflowValue_, rMassC));
//
//                    // Adjust values
//                    if (flowC != 0.f)
//                    {
//                        rMassC -= flowC;
//                        cell.flowC_ -= flowC;
//                        cell.Top->flowC_ += flowC;
//                        cell.flowdir_ |= FD_Top;
//                        cell.Top->flowdir_ |= FD_Bottom;
//                        cell.Top->SetState(FluidCell::SETTLED, false);
//                        amountUpdated_++;
//                    }
//                }
//            }
//            #ifdef FLUID_SIMULATION_REMOVEFLOWATEMPTYBORDER
//            else
//            {
//                flowL = CalculateFlow(rMassL);
//                flowC = CalculateFlow(rMassC);
//                flowR = CalculateFlow(rMassR);
//                rMassL -= flowL;
//                rMassC -= flowC;
//                rMassR -= flowR;
//                cell.flowL_ -= flowL;
//                cell.flowC_ -= flowC;
//                cell.flowR_ -= flowR;
//                amountUpdated_++;
//            }
//            #endif
//
//            // Check to ensure we still have liquid in this cell
//            if (rMassL+rMassC+rMassR < FLUID_MINVALUE)
//            {
//                cell.flowL_ -= rMassL;
//                cell.flowC_ -= rMassC;
//                cell.flowR_ -= rMassR;
//                continue;
//            }

            // if no mass change then Check if cell is settled
            if (cell.massL_ == rMassL && cell.massC_ == rMassC && cell.massR_ == rMassR)
            {
                cell.stateCount_++;
                if (cell.stateCount_ >= 10)
                    cell.SetState(FluidCell::SETTLED, true);
            }
            else
            {
                cell.UnsettleNeighbors();
            }
        }
    }

    // 2 - Update Cells
    if (amountUpdated_)
    {
        unsigned addr;

        for (addr = 0; addr < size; addr++)
            if (fluidmap[addr].Update3Flows())
                amountUpdated_++;
    }

#ifdef FLUID_EQUALIZE
    if (amountUpdated_)
        if (Equalize3SideF(fluidmap, 4))
            amountUpdated_++;
#endif
}


