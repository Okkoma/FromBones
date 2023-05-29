#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include "DefsWorld.h"

#include "GameHelpers.h"

#include "MapGeneratorMaze.h"


///    SetParams(5, fillprob, neighbours, spawnfactor, probExceeded, fillstart)
///    SetParams(6, fillprob, neighbours, spawnfactor, probExceeded, fillstart, fillend)
///
///         params_[0] = fillprob_
///         params_[1] = neighbours_
///         params_[2] = spawnfactor_
///         params_[3] = probExceeded_
///         params_[4] = fillstart_
///         params_[5] = fillend_

MapGeneratorMaze::MapGeneratorMaze()
    : MapGenerator("MapGeneratorMaze")
{ }

const int MaxIterationsInAPass = 3000;

#ifdef GenTestversion1
inline int MapGeneratorMaze::ExamineNeighbours(int xVal, int yVal)
{
    int count = 0;
    for (int x = -1; x < 2; x++)
    {
        for (int y = -1; y < 2; y++)
        {
            if (GetCell(xVal + x, yVal + y) != MapFeatureType::NoMapFeature)
                count += 1;
        }
    }
    return count;
}

void MapGeneratorMaze::Make(MapGeneratorStatus& genStatus)
{
    if (xSize_ < 3 || ySize_ < 3)
        return;

    Timer timer;

//    URHO3D_LOGINFOF("MapGeneratorMaze() - Make ...");

    //go through each cell and use the specified probability to determine if it's open
    if (genStatus.genparams_.Size() < 5)
    {
        for (int x = 1; x < xSize_-1; x++)
            for (int y = 1; y < ySize_-1; y++)
                if (Random.Get(0, 100) < genStatus.genparams_[0])
                    SetCell(y*xSize_ + x, MapFeatureType::OuterFloor);
    }
    else
    {
        for (int x = 1; x < xSize_-1; x++)
            for (int y = 1; y < ySize_-1; y++)
                if (Random.Get(0, 100) < genStatus.genparams_[0])
                    SetCell(y*xSize_ + x, Random.Get(genStatus.genparams_[4], genStatus.genparams_[5]));
    }


    int iterations = (10-genStatus.genparams_[2]%10) * (xSize_ * ySize_);
//    URHO3D_LOGINFOF("MapGeneratorMaze() - Make ... version1 spawnfactor=%d iterations=%d", genStatus.genparams_[2], iterations);

    int rX, rY;

    //pick some cells at random
    if (genStatus.genparams_[3])
    {
        if (genStatus.genparams_.Size() < 5)
            for (int x = 0; x <= iterations; x++)
            {
                rX = Random.Get(1, xSize_-2);
                rY = Random.Get(1, ySize_-2);

                SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? MapFeatureType::OuterFloor : MapFeatureType::NoMapFeature);
            }
        else
            for (int x = 0; x <= iterations; x++)
            {
                rX = Random.Get(1, xSize_-2);
                rY = Random.Get(1, ySize_-2);

                SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? Random.Get(genStatus.genparams_[4], genStatus.genparams_[5]) : MapFeatureType::NoMapFeature);
            }
    }
    else
    {
        if (genStatus.genparams_.Size() < 5)
            for (int x = 0; x <= iterations; x++)
            {
                rX = Random.Get(1, xSize_-2);
                rY = Random.Get(1, ySize_-2);

                SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? MapFeatureType::NoMapFeature : MapFeatureType::OuterFloor);
            }
        else
            for (int x = 0; x <= iterations; x++)
            {
                rX = Random.Get(1, xSize_-2);
                rY = Random.Get(1, ySize_-2);

                SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? MapFeatureType::NoMapFeature : Random.Get(genStatus.genparams_[4], genStatus.genparams_[5]));
            }
    }

//    URHO3D_LOGINFOF("MapGeneratorMaze() - Make ... OK  => timer = %u msec", timer.GetMSec(true));
}

bool MapGeneratorMaze::Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    if (xSize_ < 3 || ySize_ < 3)
        return true;

    int& mcount3 = genStatus.mapcount_[MAP_FUNC3];

    if (mcount3 == 0)
    {
        if (genStatus.genparams_.Size() < 4)
            return true;

        if (genStatus.genparams_.Size() < 5)
        {
            for (int x = 1; x < xSize_-1; x++)
                for (int y = 1; y < ySize_-1; y++)
                    if (Random.Get(0, 100) < genStatus.genparams_[0])
                        SetCell(y*xSize_ + x, MapFeatureType::OuterFloor);
        }
        else
        {
            for (int x = 1; x < xSize_-1; x++)
                for (int y = 1; y < ySize_-1; y++)
                    if (Random.Get(0, 100) < genStatus.genparams_[0])
                        SetCell(y*xSize_ + x, Random.Get(genStatus.genparams_[4], genStatus.genparams_[5]));
        }

        // num of iterations;
        mcount3 = (10-genStatus.genparams_[2]%10) * (xSize_ * ySize_);

        return false;
    }

    if (mcount3 > 0)
    {
        int rX, rY;
        int startcount = mcount3;

        //pick some cells at random
        if (genStatus.genparams_[3])
        {
            if (genStatus.genparams_.Size() < 5)
                while (mcount3--)
                {
                    rX = Random.Get(1, xSize_-2);
                    rY = Random.Get(1, ySize_-2);

                    SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? MapFeatureType::OuterFloor : MapFeatureType::NoMapFeature);

                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
            else
                while (mcount3--)
                {
                    rX = Random.Get(1, xSize_-2);
                    rY = Random.Get(1, ySize_-2);

                    SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? Random.Get(genStatus.genparams_[4], genStatus.genparams_[5]) : MapFeatureType::NoMapFeature);

                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
        }
        else
        {
            if (genStatus.genparams_.Size() < 5)
                while (mcount3--)
                {
                    rX = Random.Get(1, xSize_-2);
                    rY = Random.Get(1, ySize_-2);

                    SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? MapFeatureType::NoMapFeature : MapFeatureType::OuterFloor);

                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
            else
                while (mcount3--)
                {
                    rX = Random.Get(1, xSize_-2);
                    rY = Random.Get(1, ySize_-2);

                    SetCell(rY*xSize_ + rX, ExamineNeighbours(rX, rY) > genStatus.genparams_[1] ? MapFeatureType::NoMapFeature : Random.Get(genStatus.genparams_[4], genStatus.genparams_[5]));

                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
        }

        return (mcount3 == 0);
    }

    return true;
}
#else

void MapGeneratorMaze::Make(MapGeneratorStatus& genStatus)
{
//    URHO3D_LOGINFOF("MapGeneratorMaze() - Make ...");
    if (genStatus.genparams_.Size() < 4)
        return;

    int prob = genStatus.genparams_[0] % 100;
    probMax_ = 1000;

    const int& neighbors = genStatus.genparams_[1];
    const int& probExceeded = genStatus.genparams_[3];

    FeatureType cellType = genStatus.genparams_.Size() < 5 ? MapFeatureType::OuterFloor : genStatus.genparams_[4];

    int i;
    int iterations = (10-genStatus.genparams_[2]%10) * (xSize_ * ySize_);
//    URHO3D_LOGINFOF("MapGeneratorMaze() - Make ... version2 spawnfactor=%d iterations=%d", genStatus.genparams_[2], iterations);

    //pick some cells at random
    if (probExceeded)
    {
        if (genStatus.genparams_.Size() == 6)
            for (i = 0; i <= iterations; i++)
                AddNoiseEx(prob, neighbors, Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), genStatus.genparams_[4], genStatus.genparams_[5]);
        else
            for (i = 0; i <= iterations; i++)
                AddNoiseEx(prob, neighbors, Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), cellType);
    }
    else
    {
        if (genStatus.genparams_.Size() == 6)
            for (i = 0; i <= iterations; i++)
                AddNoise(prob, neighbors, Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), genStatus.genparams_[4], genStatus.genparams_[5]);
        else
            for (i = 0; i <= iterations; i++)
                AddNoise(prob, neighbors, Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), cellType);
    }
//    URHO3D_LOGINFOF("MapGeneratorMaze() - Make ... OK !");
}

bool MapGeneratorMaze::Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    int& mcount3 = genStatus.mapcount_[MAP_FUNC3];

    if (mcount3 == 0)
    {
        if (genStatus.genparams_.Size() < 4)
            return true;

        // num of iterations;
        mcount3 = (10-genStatus.genparams_[2]%10) * (xSize_ * ySize_);
    }

    if (mcount3 > 0)
    {
        int startcount = mcount3;
        int prob = genStatus.genparams_[0] % 100;
        probMax_ = 1000;

        FeatureType cellType = genStatus.genparams_.Size() < 5 ? MapFeatureType::OuterFloor : genStatus.genparams_[4];

        //pick some cells at random
        if (genStatus.genparams_[3])
        {
            if (genStatus.genparams_.Size() == 6)
                while (mcount3--)
                {
                    AddNoiseEx(prob, genStatus.genparams_[1], Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), genStatus.genparams_[4], genStatus.genparams_[5]);
                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
            else
                while (mcount3--)
                {
                    AddNoiseEx(prob, genStatus.genparams_[1], Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), cellType);
                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
        }
        else
        {
            if (genStatus.genparams_.Size() == 6)
                while (mcount3--)
                {
                    AddNoise(prob, genStatus.genparams_[1], Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), genStatus.genparams_[4], genStatus.genparams_[5]);
                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
            else
                while (mcount3--)
                {
                    AddNoise(prob, genStatus.genparams_[1], Random.Get(0, ySize_-1), Random.Get(0, xSize_-1), cellType);
                    if ((startcount-mcount3)%MaxIterationsInAPass == 0)
                        if (TimeOver(timer))
                            break;
                }
        }

        return (mcount3 == 0);
    }

    return true;
}

inline int MapGeneratorMaze::GetNumNeighbor8(const int& x, const int& y)
{
    int count = 0, nx, ny;

    for (nx = -1; ny < 2; nx++)
        for (ny = -1; ny < 2; ny++)
            if (GetCellBoundCheck(x + nx, y + ny) != MapFeatureType::NoMapFeature)
                count += 1;

    return count;
}

inline void MapGeneratorMaze::AddRandomSeed(const FeatureType& cellType, int span)
{
    int x = Random.Get(0, xSize_-1);
    int y = Random.Get(0, ySize_-1);

    map_[y * xSize_ + x] = cellType;

    int n = Random.Get(0, 7);
    while (span-- != -1)
    {
        SetCellBoundCheck(x + nghTable8xy_[n][0], y + nghTable8xy_[n][1], cellType);
        n = (n+1) %7;
    }
}

inline void MapGeneratorMaze::AddNoiseEx(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType)
{
    if (prob)
        if (Random.Get(0, probMax_) < prob)
            AddRandomSeed(cellType, neighbors);

    if (GetNumNeighbor8(x, y) > neighbors)
        map_[y * xSize_ + x] = cellType;
//    map_[y * xSize_ + x] = GetNumNeighbor8(x, y) > neighbors ? cellType : MapFeatureType::NoMapFeature;
}

inline void MapGeneratorMaze::AddNoiseEx(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType1, const FeatureType& cellType2)
{
    if (prob)
        if (Random.Get(0, probMax_) < prob)
            AddRandomSeed(Random.Get(cellType1, cellType2), neighbors);

    map_[y * xSize_ + x] = GetNumNeighbor8(x, y) > neighbors ? Random.Get(cellType1, cellType2) : MapFeatureType::NoMapFeature;
}

inline void MapGeneratorMaze::AddNoise(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType)
{
    if (prob)
        if (Random.Get(0, probMax_) < prob)
            AddRandomSeed(cellType, neighbors);

    map_[y * xSize_ + x] = GetNumNeighbor8(x, y) > neighbors ? MapFeatureType::NoMapFeature : cellType;
}

inline void MapGeneratorMaze::AddNoise(const int& prob, const int& neighbors, const int& x, const int& y, const FeatureType& cellType1, const FeatureType& cellType2)
{
    if (prob)
        if (Random.Get(0, probMax_) < prob)
            AddRandomSeed(Random.Get(cellType1, cellType2), neighbors);

    map_[y * xSize_ + x] = GetNumNeighbor8(x, y) > neighbors ? MapFeatureType::NoMapFeature : Random.Get(cellType1, cellType2);
}
#endif
