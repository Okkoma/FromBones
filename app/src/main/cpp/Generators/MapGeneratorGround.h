#pragma once


#include "MapGenerator.h"

/*
SetParams(5, maxHeight1, maxHeight2, octave1, octave2, step)
    params_[0] = maxHeight1
    params_[1] = maxHeight2
    params_[2] = octave1
    params_[3] = octave2
    params_[4] = step

SetParamsFloat(4, startYFactor1, startYFactor2, freq1, freq2)
    paramsf_[0] = startYFactor1
    paramsf_[1] = startYFactor2
    paramsf_[2] = freq1
    paramsf_[3] = freq2
*/

class MapGeneratorGround : public MapGenerator
{
public:
    MapGeneratorGround();
    virtual ~MapGeneratorGround() { }

    virtual void GenerateSpots(MapGeneratorStatus& genStatus);
    virtual void GenerateFurnitures(MapGeneratorStatus& genStatus);

protected:
    virtual void Make(MapGeneratorStatus& genStatus);
};

