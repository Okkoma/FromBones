#pragma once

#include "MapGenerator.h"
#include "DefsFluids.h"


using namespace Urho3D;


class MapSimulatorLiquid : public MapGenerator
{
public:
    MapSimulatorLiquid();
    virtual ~MapSimulatorLiquid();

    static MapSimulatorLiquid* Get()
    {
        return simulator_;
    }
    static void SetSimulationMode(int mode, int numiterations)
    {
        mode_=mode;
        numiterations_ = numiterations;
    }

    int Update(FluidDatas& fluiddatas);

protected:
    virtual void Make();

private:
    void Simulate5();
    void Simulate6();

    FluidDatas* fluidDatas_;

    int amountUpdated_;

    static int mode_;
    static int numiterations_;
    static MapSimulatorLiquid* simulator_;
};
