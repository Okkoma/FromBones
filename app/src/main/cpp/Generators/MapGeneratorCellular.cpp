#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include "DefsWorld.h"

#include "MapGeneratorCellular.h"

#define TILE_EMPTY 0

MapGeneratorCellular::MapGeneratorCellular(const World2DInfo * info)
{
    URHO3D_LOGINFOF("MapGeneratorCellular()");
    size_x_ = info->mapWidth_;
    size_y_ = info->mapHeight_;

    grid1_ = (int*) malloc(size_x_ * size_y_ * sizeof(int));
    grid2_ = (int*) malloc(size_x_ * size_y_ * sizeof(int));
}

MapGeneratorCellular::~MapGeneratorCellular()
{
    URHO3D_LOGINFOF("~MapGeneratorCellular()");
    if (grid1_) free(grid1_);
    if (grid2_) free(grid2_);
}

int MapGeneratorCellular::RandBrick()
{
    return fillstart_ + rand()%fillrange_;
}

int MapGeneratorCellular::RandPick()
{
    random_ = rand();
    if(random_%100 < fillprob_)
    {
        return fillstart_ + random_%fillrange_;
    }
    else
        return TILE_EMPTY;
}

void MapGeneratorCellular::FillMap()
{
    int xi, yi;

    for(yi=0; yi<size_y_; yi++)
        for(xi=0; xi<size_x_; xi++)
            grid1_[yi*size_x_ + xi] = RandPick();

    for(yi=0; yi<size_y_; yi++)
        for(xi=0; xi<size_x_; xi++)
            grid2_[yi*size_x_ + xi] = RandBrick();
}

void MapGeneratorCellular::MakePass()
{
    int xi, yi, ii, jj;

    for(yi=1; yi<size_y_-1; yi++)
        for(xi=1; xi<size_x_-1; xi++)
        {
            int adjcount_r1 = 0,
                adjcount_r2 = 0;

            for(ii=-1; ii<=1; ii++)
                for(jj=-1; jj<=1; jj++)
                {
                    if(grid1_[(yi+ii)*size_x_ + xi+jj] != TILE_EMPTY)
                        adjcount_r1++;
                }

            for(ii=yi-2; ii<=yi+2; ii++)
                for(jj=xi-2; jj<=xi+2; jj++)
                {
                    if(abs(ii-yi)==2 && abs(jj-xi)==2)
                        continue;
                    if(ii<0 || jj<0 || ii>=size_y_ || jj>=size_x_)
                        continue;
                    if(grid1_[ii*size_x_ + jj] != TILE_EMPTY)
                        adjcount_r2++;
                }

            if(adjcount_r1 >= params_.r1_cutoff || adjcount_r2 <= params_.r2_cutoff)
                grid2_[yi*size_x_ + xi] = RandBrick();
            else
                grid2_[yi*size_x_ + xi] = TILE_EMPTY;
        }

    for(yi=1; yi<size_y_-1; yi++)
        for(xi=1; xi<size_x_-1; xi++)
            grid1_[yi*size_x_ + xi] = grid2_[yi*size_x_ + xi];
}

int* MapGeneratorCellular::Generate(int fill, int fillstart, int fillend, int r1, int r2, int reps)
{
    fillprob_ = fill;
    fillstart_ = fillstart;
    fillend_ = fillend;
    fillrange_ = fillend - fillstart + 1;

    params_.r1_cutoff = r1;
    params_.r2_cutoff = r2;
    params_.reps = reps;

    FillMap();
    for(int i=0; i < params_.reps; i++)
        MakePass();

    return grid1_;
}
