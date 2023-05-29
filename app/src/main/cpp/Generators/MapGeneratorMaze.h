#pragma once


#include "MapGenerator.h"

/*
SetParams(6, fillprob, neighbours, iterations, probExceeded, fillstart, fillend)
    params_[0] = fillprob_
    params_[1] = neighbours_
    params_[2] = iterations_
    params_[3] = probExceeded_
    params_[4] = fillstart_
    params_[5] = fillend_
*/

#define GenTestversion1


class MapGeneratorMaze : public MapGenerator
{
public:
    MapGeneratorMaze();
    virtual ~MapGeneratorMaze() { }

protected:
    virtual void Make(MapGeneratorStatus& genStatus);
    virtual bool Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);

private:
#ifdef GenTestversion1
    inline int ExamineNeighbours(int xVal, int yVal);
#else
    int probMax_;
    inline int GetNumNeighbor8(const int& x, const int& y);
    inline void AddRandomSeed(const FeatureType& cellType, int span);
    inline void AddNoiseEx(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType);
    inline void AddNoiseEx(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType1, const FeatureType& cellType2);
    inline void AddNoise(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType);
    inline void AddNoise(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType1, const FeatureType& cellType2);
#endif
};

