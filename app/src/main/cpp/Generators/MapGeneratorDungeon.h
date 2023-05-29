#pragma once


#include "MapGenerator.h"

using namespace Urho3D;

/*
SetParamsInt(5, maxFeatures, roomProb, corridorProb, roomMinSize, dungeontype)
    params_[0] = maxFeatures_
    params_[1] = roomProb_
    params_[2] = corridorProb_
    params_[3] = roomMinSize_
    params_[4] = roomMaxSize_
    params_[5] = dungeontype_
*/

enum DungeonType
{
    DUNG_UNDERGROUND = 0,
    DUNG_CENTEREDCASTLE,
    DUNG_CASTLE,
    DUNG_SKYCASTLE,
    NUMDUNGEONTYPES
};

enum BuildingType
{
    House1 = 0,
    House2,
    House3,
    NarrowTower,
    Tower,
    Dungeon,
    ChaoticDungeon,

    NumBuildingTypes
};

enum RoomType
{
    Corridor = 0,
    Chamber,
    Bedroom,
    Office,
    Kitchen,
    Library,
    Forge,
    Hall,
    Refectory,
    Sorcererroom,
    Priestroom,
    Pit,
    BossRoom,

    NumRoomTypes
};

enum FurnitureAlignement
{
    OnGround,
    OnRoof,
    OnBack,
    OnWall,

    NumFurnitureAlignements
};

struct FurnitureRequirement
{
    FurnitureRequirement(FurnitureAlignement align, bool supportIsSolid, bool rangeIsSolid, unsigned char numSupportCells, signed char supportOffsetFromRange, signed char finalCellAlign) :
        align_(align), supportIsSolid_(supportIsSolid), rangeIsSolid_(rangeIsSolid), numSupportCells_(numSupportCells), supportOffsetFromRange_(supportOffsetFromRange), finalCellAlign_(finalCellAlign) { }

    FurnitureAlignement align_;
    bool supportIsSolid_;
    bool rangeIsSolid_;
    unsigned char numSupportCells_;
    signed char supportOffsetFromRange_;
    signed char finalCellAlign_;
};

enum RoomGeometry
{
    ROOM_RECTANGLE = 0,
    ROOM_ROUNDED,
};

struct ConstRoomInfo
{
    int type_;
    const FurnitureRequirement* requirement_;

    Vector<StringHash> objects_;
    Vector<unsigned> maxQuantitiesByObjects_;
    Vector<IntVector2> objectsSize_;
};

struct RoomBoxInfo
{
    int id_;
    IntRect rect_;
    IntVector2 size_;
};

struct RoomInfo
{
    int id_;
    int type_;
    int geom_;
    IntRect rect_;
    IntVector2 size_;
    IntVector2 center_;
    IntVector2 entry_;
    IntVector2 exitx_;
    IntVector2 exity_;
};

struct DungeonInfo
{
    void Clear()
    {
        rooms_.Clear();
        doorIndexes_.Clear();
    }

    String name_;
    IntVector2 entry_;
    int dungeontype_;

    Vector<RoomInfo> rooms_;
    Vector<unsigned> doorIndexes_;
};

class MapGeneratorDungeon : public MapGenerator
{
public:
    MapGeneratorDungeon();
    virtual ~MapGeneratorDungeon();

    virtual void GenerateSpots(MapGeneratorStatus& genStatus);
    virtual void GenerateFurnitures(MapGeneratorStatus& genStatus);
    DungeonInfo& GetDungeonInfo()
    {
        return dinfo_;
    }

    static void SetConstRoomInfos();
    static MapGeneratorDungeon* Get()
    {
        return gen_;
    }

protected:
    virtual bool Init(MapGeneratorStatus& genStatus);
    virtual void Make(MapGeneratorStatus& genStatus);
    virtual bool Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);

private:
    bool MakeDungeon(MapGeneratorStatus& genStatus, int x, int y, int direction);
    bool MakeCity(MapGeneratorStatus& genStatus, Map* map, int x);
    bool MakeBuilding(MapGeneratorStatus& genStatus, Map* map, int type, int x, const IntVector2& size);

    bool ResolveEntrance(MapGeneratorStatus& genStatus);

    // Features
    bool PlaceFeature(MapGeneratorStatus& genStatus);
    bool MakeFeature(MapGeneratorStatus& genStatus, int fromRoom, int x, int y, int direction);
    bool MakeRoom(MapGeneratorStatus& genStatus, int fromRoom, int x, int y, int direction, bool addplateforms, bool addwindows, bool makeEntrance=true);

    void SetRoom(RoomInfo& room, int geom, const IntVector2& size, const IntRect& rect);
    void SetGeometry(RoomInfo& room, bool setfull=false);
    bool AddPlateforms(const RoomInfo& room);
    bool AddWindows(const RoomInfo& room);
    bool MakeCorridorPlateforms(int x, int y, int maxLength, int direction);
    bool MakeRandomWalk(RoomInfo& room);
    void RemoveUntidyWalls(const IntRect& rect);
    bool MakeStairs(FeatureType celltype);

    // Furnitures
    bool PlaceFurniture();

    // Helper Map Functions
    bool FindCell(FeatureType celltype, int* findx, int* findy, int direction, bool resetStartCoords=false);

    void SetCells(int xStart, int yStart, int xEnd, int yEnd, FeatureType cellType);
    void SetBorderCells(int xStart, int yStart, int xEnd, int yEnd, FeatureType cellType);
    void CopyShapeToCells(PixelShape* shape, const RoomInfo& room);

    bool IsXInBounds(int x) const;
    bool IsYInBounds(int y) const;
    bool IsXInsideBorder(int x) const;
    bool IsYInsideBorder(int y) const;
    bool IsAreaUnused(int xStart, int yStart, int xEnd, int yEnd, FeatureType skipType=0) const;
    bool IsAdjacent(int x, int y, FeatureType tile) const;

    bool GetCellCoordX(const FurnitureRequirement& requirement, const IntVector2& start, const IntVector2& range, int xEnd, const int incY, int& resultx) const;
    bool GetCellCoordY(const FurnitureRequirement& requirement, const IntVector2& start, const IntVector2& range, int yEnd, const int incX, int& resulty) const;
    bool GetPositionFor(const FurnitureRequirement& requirement, const IntVector2& start, const IntVector2& end, const IntVector2& range, IntVector2& result) const;

    void GetAdjacentDirection(int x, int y, FeatureType tile, IntVector2& direction) const;
    bool GetRandomSafePlaceInRoom(const IntRect& room, int *x, int *y, int shrink=0, bool checkInitialPosition = false);
//    int InvertDirection(int direction);

    DungeonInfo dinfo_;

    static MapGeneratorDungeon* gen_;
    static Vector<ConstRoomInfo> sConstRoomInfos_;
};
