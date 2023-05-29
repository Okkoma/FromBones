#pragma once

#include <cstdarg>

#include "DefsMap.h"

#include "GameRand.h"


using namespace Urho3D;

class World2DInfo;


enum BiomeLayer
{
    BiomeExternalBack = 0,
    BiomeCave,
    BiomeDungeon,
    BiomeExternal,
};

class MapGenerator : RefCounted
{
public :
    MapGenerator(const String& name);
    virtual ~MapGenerator();

    static void InitTable();
    static void SetWorldInfo(const World2DInfo * info);
    static void SetSize(int width, int height);

    void SetGeneratorMap(MapGeneratorStatus& genStatus, int mapslot, int viewZ=0, FeatureType* data=0, bool genspot=false, bool clean=false);
    void SetParamsInt(MapGeneratorStatus& genStatus, const PODVector<int>& params);
    void SetParamsInt(MapGeneratorStatus& genStatus, int n, ...);

    void SaveStateToMapStatus(MapGeneratorStatus& genStatus);
    void RestoreStateFromMapStatus(MapGeneratorStatus& genStatus);

    FeatureType* const Generate(MapGeneratorStatus& genStatus);
    bool Generate(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);

    void ClearSpots()
    {
        spots_.Clear();
    }
    void ClearFurnitures()
    {
        furnitures_.Clear();
    }

    virtual void GenerateSpots(MapGeneratorStatus& genStatus);
    virtual void GenerateFurnitures(MapGeneratorStatus& genStatus);
    virtual bool GenerateSpots(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);
    virtual bool GenerateFurnitures(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);

    static void GenerateDefaultTerrainMap(MapGeneratorStatus& genStatus);
    bool GenerateRandomSpots(MapGeneratorStatus& genStatus, const MapSpotType& spotType, const int& minSpots=0, const int& maxSpots=1, int maxTries=1000, const int& spotProb=100);
    void GenerateBiomeFurnitures(MapGeneratorStatus& genStatus, int biomelayer);

    FeatureType* const GetBuffer() const
    {
        return map_;
    }
    const FeatureType* GetMap() const
    {
        return map_;
    }

    const PODVector<MapSpot>& GetSpots() const
    {
        return spots_;
    }
    const PODVector<EntityData>& GetFurnitures() const
    {
        return furnitures_;
    }

protected :

    virtual bool Init(MapGeneratorStatus& genStatus);
    virtual void Make(MapGeneratorStatus& genStatus);
    virtual bool Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);

    bool IsAreaForSpot(MapGeneratorStatus& genStatus, MapSpotType spotType, int x, int y);

    inline void SetCell(int addr, FeatureType celltype)
    {
        assert(addr < xSize_*ySize_);
        map_[addr] = celltype;
    }

    inline void SetCell(int x, int y, FeatureType celltype)
    {
        assert(x + xSize_ * y < xSize_*ySize_);
        map_[x + xSize_ * y] = celltype;
    }

    inline void SetCellBoundCheck(int x, int y, FeatureType celltype)
    {
        if (x < 0 || x >= xSize_ || y < 0 || y >= ySize_)
            return;

        map_[x + xSize_ * y] = celltype;
    }

    inline unsigned char GetCell(int addr) const
    {
        assert(addr < xSize_*ySize_);
        return map_[addr];
    }

    inline unsigned char GetCell(int x, int y) const
    {
        assert(x + xSize_ * y < xSize_*ySize_);
        return map_[x + xSize_ * y];
    }

    inline unsigned char GetCell(const IntVector2& coord) const
    {
        assert(coord.x_ + xSize_ * coord.y_ < xSize_*ySize_);
        return map_[coord.x_ + xSize_ * coord.y_];
    }

    inline unsigned char GetCellBoundCheck(int x, int y) const
    {
        return x < 0 || x >= xSize_ || y < 0 || y >= ySize_ ? 0 : map_[x + xSize_ * y];
    }

    String name_;

    FeatureType* map_;
    PODVector<MapSpot> spots_;
    PODVector<EntityData> furnitures_;

    GameRand& Random;

    static const World2DInfo * info_;
    static int xSize_, ySize_;

    static short int nghTable4_[4];
    static short int nghTable8_[8];
    static short int nghTable4xy_[4][2];
    static short int nghTable8xy_[8][2];
};


