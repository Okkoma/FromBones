#pragma once


struct CellularParams
{
    int r1_cutoff, r2_cutoff;
    int reps;
};

class World2DInfo;

class MapGeneratorCellular
{
public:
    MapGeneratorCellular(const World2DInfo * info);
    ~MapGeneratorCellular();

    int* Generate(int fill, int fillstart, int fillend, int r1, int r2, int reps);

private:
    int RandBrick();
    int RandPick();
    void FillMap();
    void MakePass();

    int* grid1_;
    int* grid2_;
    int size_x_, size_y_;

    int fillprob_, fillstart_, fillend_, fillrange_;
    int random_;

    CellularParams params_;
};

