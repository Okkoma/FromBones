#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Container/Vector.h>

#include "DefsViews.h"
#include "DefsWorld.h"

#include "Map.h"

#include "GameAttributes.h"
#include "GameHelpers.h"
#include "GameOptionsTest.h"
#include "ViewManager.h"

#include "MemoryObjects.h"

#include "MapGeneratorDungeon.h"


#define RESOLVE_ENTRANCE

Vector<unsigned> doorIndexes_;
Matrix2D<FeatureType> checkBuffer_;
Stack<unsigned> dungStack_;

const char* dungeonTypeNames_[] =
{
    "Underground Dungeon",
    "Centered Castle on GroundLevel",
    "Castle on Ground",
    "Castle in sky",
};

const char* BuildingTypeNames_[] =
{
    "House1",
    "House2",
    "House3",
    "NarrowTower",
    "Tower",
    "Dungeon",
    "ChaoticDungeon",
    0
};

const char* dungeonRoomTypeNames_[] =
{
    "Corridor",
    "Chamber",
    "Bedroom",
    "Office",
    "Kitchen",
    "Library",
    "Forge",
    "Hall",
    "Refectory",
    "Sorcererroom",
    "Priestroom",
    "Pit",
    "BossRoom"
};

const StringHash dungeonRoomAlignements_[] =
{
    StringHash("Corridor"),
    StringHash("Chamber"),
    StringHash("Bedroom"),
    StringHash("Office"),
    StringHash("Kitchen"),
    StringHash("Library"),
    StringHash("Forge"),
    StringHash("Hall"),
    StringHash("Refectory"),
    StringHash("Sorcererroom"),
    StringHash("Priestroom"),
    StringHash("Pit"),
    StringHash("BossRoom"),

    StringHash("CorridorRoof"),
    StringHash("ChamberRoof"),
    StringHash("BedroomRoof"),
    StringHash("OfficeRoof"),
    StringHash("KitchenRoof"),
    StringHash("LibraryRoof"),
    StringHash("ForgeRoof"),
    StringHash("HallRoof"),
    StringHash("RefectoryRoof"),
    StringHash("SorcererroomRoof"),
    StringHash("PriestroomRoof"),
    StringHash("PitRoof"),
    StringHash("BossRoomRoof"),

    StringHash("CorridorBack"),
    StringHash("ChamberBack"),
    StringHash("BedroomBack"),
    StringHash("OfficeBack"),
    StringHash("KitchenBack"),
    StringHash("LibraryBack"),
    StringHash("ForgeBack"),
    StringHash("HallBack"),
    StringHash("RefectoryBack"),
    StringHash("SorcererroomBack"),
    StringHash("PriestroomBack"),
    StringHash("PitBack"),
    StringHash("BossRoomBack"),

    StringHash("CorridorWall"),
    StringHash("ChamberWall"),
    StringHash("BedroomWall"),
    StringHash("OfficeWall"),
    StringHash("KitchenWall"),
    StringHash("LibraryWall"),
    StringHash("ForgeWall"),
    StringHash("HallWall"),
    StringHash("RefectoryWall"),
    StringHash("SorcererroomWall"),
    StringHash("PriestroomWall"),
    StringHash("PitWall"),
    StringHash("BossRoomWall"),
};

const FurnitureRequirement dungeonRoomRequirements_[NumFurnitureAlignements] =
{
//	FurnitureRequirement(FurnitureAlignement align, bool supportIsSolid, bool rangeIsSolid, unsigned char numSupportCells, signed char supportOffsetOnRange, char finalCellAlign)

    FurnitureRequirement(OnGround, true, false, 255, 0, -127),
    FurnitureRequirement(OnRoof, true, false, 255, 0, 100),
    FurnitureRequirement(OnBack, false, false, 0, 0, 0),
    FurnitureRequirement(OnWall, false, false, 1, -1, 80)
};

const IntVector2 BossRoomMinSize(13, 13);


Vector<ConstRoomInfo> MapGeneratorDungeon::sConstRoomInfos_;
MapGeneratorDungeon* MapGeneratorDungeon::gen_ = 0;

MapGeneratorDungeon::MapGeneratorDungeon() :
    MapGenerator("MapGeneratorDungeon")
{
    if (!gen_)
        gen_ = this;

    dinfo_.rooms_.Reserve(500);
    dinfo_.doorIndexes_.Reserve(1000);
}

MapGeneratorDungeon::~MapGeneratorDungeon()
{
    if (gen_ == this)
        gen_ = 0;
}

void MapGeneratorDungeon::SetConstRoomInfos()
{
    if (sConstRoomInfos_.Size())
        return;

    sConstRoomInfos_.Resize(NumFurnitureAlignements * NumRoomTypes);

    unsigned addr = 0;
    for (unsigned j=0; j < NumFurnitureAlignements; j++)
    {
        for (unsigned i=0; i < NumRoomTypes; i++, addr++)
        {
            ConstRoomInfo& info = sConstRoomInfos_[addr];
            info.type_ = i;
            info.requirement_ = &dungeonRoomRequirements_[j];

            const Vector<StringHash>& roomobjects = COT::GetTypesInCategory(dungeonRoomAlignements_[addr]);
            if (!roomobjects.Size())
                continue;

            Vector<StringHash>& objects = info.objects_;
            objects.Resize(roomobjects.Size());

            Vector<unsigned>& qties = info.maxQuantitiesByObjects_;
            qties.Resize(roomobjects.Size());

            Vector<IntVector2>& sizes = info.objectsSize_;
            sizes.Resize(roomobjects.Size());

            for (unsigned j=0; j < roomobjects.Size(); j++)
            {
                const StringHash& got = roomobjects[j];
                const GOTInfo& gotinfo = GOT::GetConstInfo(got);
                objects[j] = got;
                qties[j] = gotinfo.maxdropqty_;
                sizes[j] = gotinfo.entitySize_;
            }
        }
    }
}

//MapGeneratorDungeon::~MapGeneratorDungeon()
//{
//
//}

/*
SetParamsInt(5, maxFeatures, roomProb, corridorProb, roomMinSize, dungeontype)
    params_[0] = maxFeatures_
    params_[1] = roomProb_
    params_[2] = corridorProb_
    params_[3] = roomMinSize_
    params_[4] = roomMaxSize_
    params_[5] = dungeontype_
*/

bool MapGeneratorDungeon::Init(MapGeneratorStatus& genStatus)
{
    checkBuffer_.Resize(xSize_, ySize_);
    dungStack_.Resize(xSize_ * ySize_);

    dinfo_.Clear();

    return MapGenerator::Init(genStatus);
}

void MapGeneratorDungeon::Make(MapGeneratorStatus& genStatus)
{
    Make(genStatus, 0, 0);
}

/// TODO Async

bool MapGeneratorDungeon::Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    URHO3D_LOGINFOF("MapGeneratorDungeon() - Make ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, delay/1000);

//    SetSeed(ALLRAND, seed_);

    int type = dinfo_.dungeontype_ = genStatus.genparams_[5];

    if (dinfo_.dungeontype_ == -1)
    {
        // take a Random Dungeon Type (don't consider Castle In Sky)
        type = Random.Get(NUMDUNGEONTYPES-1);
    }

    int direction;
    int x=0, y=0;
    bool done = false;
    int essay = 0;

    Map* map = 0;
    if (genStatus.map_->GetType() == Map::GetTypeStatic())
        map = static_cast<Map*>(genStatus.map_);

    if (type == DUNG_SKYCASTLE)
    {
        // centered castle in sky
        x = xSize_/2;
        y = ySize_;
        direction = MapDirection::North;

        URHO3D_LOGINFOF("MapGeneratorDungeon() - Make seed=%u p=%d|%d|%d|%d|%d ... Creating a %s ... at %d %d",
                        genStatus.rseed_, genStatus.genparams_[0], genStatus.genparams_[1], genStatus.genparams_[2],
                        genStatus.genparams_[3], genStatus.genparams_[4], dungeonTypeNames_[type], x, y);

        done = MakeDungeon(genStatus, x, y, direction);
    }
    else
    {
        if (map)
        {
            if (map->GetTopography().IsFullGround())
            {
                type = DUNG_UNDERGROUND;
                direction = MapDirection::North;
                x = 0;
                y = ySize_ - 1;
                done = FindCell(MapFeatureType::OuterFloor, &x, &y, direction, true);
                done = MakeDungeon(genStatus, x, y, direction);
            }
            else if (!map->GetTopography().IsFullSky())
            {
                type = DUNG_CENTEREDCASTLE;

                done = MakeCity(genStatus, map, xSize_ / 2);
            }
        }
        else
        {
            while (!done && essay < NUMDUNGEONTYPES)
            {
                direction = type != DUNG_CASTLE ? MapDirection::North : MapDirection::South;

                if (type == DUNG_CENTEREDCASTLE)
                {
                    // centered dungeon on ground level
                    x = xSize_ / 2;
                    y = (int)floor((float)info_->simpleGroundLevel_ * (float)ySize_ / 100.f);
                    done = FindCell(MapFeatureType::OuterFloor, &x, &y, direction, true);
                }
                else
                {
                    x = 0;
                    y = 0;
                    done = FindCell(MapFeatureType::OuterFloor, &x, &y, direction, true);
                }

                URHO3D_LOGINFOF("MapGeneratorDungeon() - Make seed=%u p=%d|%d|%d|%d|%d ... Try Creating a %s ... at %d %d",
                                genStatus.rseed_, genStatus.genparams_[0], genStatus.genparams_[1], genStatus.genparams_[2],
                                genStatus.genparams_[3], genStatus.genparams_[4], dungeonTypeNames_[type], x, y);

                if (done)
                    done = MakeDungeon(genStatus, x, y, direction);

                if (!done)
                    type = (type+1)%(NUMDUNGEONTYPES-1);

                essay++;
            }
        }
    }

    if (!done)
    {
        URHO3D_LOGERRORF("MapGeneratorDungeon() - Make : map=%s seed=%u p=%d|%d|%d|%d|%d ... Can't Create Dungeon !",
                         genStatus.mappoint_.ToString().CString(), genStatus.rseed_, genStatus.genparams_[0], genStatus.genparams_[1],
                         genStatus.genparams_[2], genStatus.genparams_[3], genStatus.genparams_[4]);
    }
    else
    {
        dinfo_.dungeontype_ = type;
        URHO3D_LOGINFOF("MapGeneratorDungeon() - Make Create a %s with %d rooms ... timer=%d/%d msec", dungeonTypeNames_[type], dinfo_.rooms_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000);
    }

    return done;
}

void MapGeneratorDungeon::GenerateSpots(MapGeneratorStatus& genStatus)
{
    int viewZindex = genStatus.GetActiveViewZIndex();

    URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateSpots ... INNERVIEW viewZindex=%d numRooms=%u ...", viewZindex, dinfo_.rooms_.Size());

//    GameHelpers::DumpData(map_ , 1, 2, xSize_, ySize_);

    // Add the SPOT_ROOMS
    if (dinfo_.rooms_.Size() > 0)
    {
        int x, y;
        for (PODVector<RoomInfo>::Iterator it=dinfo_.rooms_.Begin(); it != dinfo_.rooms_.End(); ++it)
        {
            RoomInfo& room = *it;
            x = (room.rect_.right_ + room.rect_.left_)/2;
            y = (room.rect_.bottom_ + room.rect_.top_)/2;

            if (room.type_ != BossRoom)
            {
                if (GetRandomSafePlaceInRoom(room.rect_, &x, &y, 1, true))
                {
                    spots_.Push(MapSpot(SPOT_ROOM, x, y, viewZindex, room.rect_.Width()-1, room.rect_.Height()-1, room.geom_));
//                    URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateSpots ... has placed spot=%s in the room=%s !", spots_.Back().GetName(0).CString(), room.rect_.ToString().CString());
                    room.center_.x_ = x;
                    room.center_.y_ = y;
                }
            }
            else
            {
                spots_.Push(MapSpot(SPOT_BOSS, x, y, viewZindex, room.rect_.Width(), room.rect_.Height(), room.geom_));
//                URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateSpots ... has placed spot=%s in the room=%s !", spots_.Back().GetName(0).CString(), room.rect_.ToString().CString());
                room.center_.x_ = x;
                room.center_.y_ = y;
            }

            // Place Water
            if (Random.Get(0, 100) < 10)
            {
                if (GetRandomSafePlaceInRoom(room.rect_, &x, &y))
                {
                    spots_.Push(MapSpot(SPOT_LIQUID, x, y, viewZindex, 1, 1));
//                    URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateSpots ... has placed spot=%s in the room=%s !", spots_.Back().GetName(0).CString(), room.rect_.ToString().CString());
                }
            }
        }

        if (spots_.Size())
        {
            // ADD a SPOT_START in the last room
//            URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateSpots ... has placed a SPOT_START (initial=%s) at %s !", spots_.Back().GetName(0).CString(), spots_.Back().position_.ToString().CString());
            spots_.Back().type_ = SPOT_START;
        }
    }
    else
    {
        GenerateRandomSpots(genStatus, SPOT_START, 1);
    }

    // ADD a default SPOT_START if no spot
    if (!spots_.Size())
    {
        viewZindex = ViewManager::FRONTVIEW_Index;

        URHO3D_LOGERRORF("MapGeneratorDungeon() - GenerateSpots : map=%s ... No Safe Place for SPOT_START viewZindex=%d : make default at %d,%d!",
                         genStatus.mappoint_.ToString().CString(), viewZindex, dinfo_.entry_.x_, dinfo_.entry_.y_-1);

        spots_.Push(MapSpot(SPOT_START, dinfo_.entry_.x_, dinfo_.entry_.y_-1, viewZindex, 1, 1));
    }

    if (viewZindex == ViewManager::FRONTVIEW_Index)
        GenerateRandomSpots(genStatus, SPOT_LIQUID, 10, 50, 500, 100);
    //GenerateRandomSpots(genStatus, SPOT_LAND, 2, 20, 500, 45);

    URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateSpots ... OK !");
}

//#define DOOR_WITHFLOORAFTER_ONLY

void MapGeneratorDungeon::GenerateFurnitures(MapGeneratorStatus& genStatus)
{
#ifdef HANDLE_FURNITURES
    int furnitures = 0;
    Vector<int> checkPositions;

    // Add Doors
    if (dinfo_.doorIndexes_.Size())
    {
        const StringHash& got = COT::GetTypeFrom(DUNGEONTHRESHOLD, 0);
        for (unsigned i=0; i < dinfo_.doorIndexes_.Size(); ++i)
        {
            unsigned tileindex = dinfo_.doorIndexes_[i];

            if (tileindex == 0 || tileindex+1 >= xSize_*ySize_)
                continue;

            // check if bottom is solid
            if (tileindex+xSize_ < xSize_*ySize_ && GetCell(tileindex+xSize_) < MapFeatureType::InnerFloor)
                continue;

            // skip if already a furniture at this place
            if (checkPositions.Contains(tileindex))
                continue;

            bool outsideAtLeft = (GetCell(tileindex-1) == MapFeatureType::NoMapFeature &&
                                  GetCell(tileindex+1) == MapFeatureType::RoomInnerSpace);

            bool outsideAtRight = (GetCell(tileindex+1) == MapFeatureType::NoMapFeature &&
                                   GetCell(tileindex-1) == MapFeatureType::RoomInnerSpace);

            if (!outsideAtLeft && !outsideAtRight)
                continue;

#ifdef DOOR_WITHFLOORAFTER_ONLY
            if (outsideAtLeft && tileindex+xSize_+1 < xSize_*ySize_ && GetCell(tileindex+xSize_+1) < MapFeatureType::InnerFloor)
                continue;

            if (outsideAtRight && GetCell(tileindex+xSize_-1) < MapFeatureType::InnerFloor)
                continue;
#endif
            checkPositions.Push(tileindex);

            // random door entity
            furnitures_.Resize(furnitures_.Size()+1);
            EntityData& entitydata = furnitures_.Back();
            //EntityData::Set(short unsigned gotindex=0, short unsigned tileindex=USHRT_MAX, signed char tilepositionx=0, signed char tilepositiony_=0, unsigned char sstype=0, int layerZ=0, bool rotate=false, bool flipX=false, bool flipY=false);
            entitydata.Set(GOT::GetIndex(got), tileindex, outsideAtLeft ? 127 : -127, -127, 0, THRESHOLDVIEW, false, !outsideAtLeft, false);

//            URHO3D_LOGERRORF("-> place a %s : entitydata=%s !", GOT::GetType(got).CString(), entitydata.Dump().CString());

            furnitures++;
        }
    }

    URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateFurnitures : %d/%u doors created !", furnitures, dinfo_.doorIndexes_.Size());

    // Add Innerview Furnitures
    for (Vector<RoomInfo>::ConstIterator it = dinfo_.rooms_.Begin(); it != dinfo_.rooms_.End(); ++it)
    {
        const RoomInfo& room = *it;
        const IntRect& roomrect = room.rect_;

//        URHO3D_LOGERRORF("MapGeneratorDungeon() - GenerateFurnitures : room (type=%s rect=%s) : ...", dungeonRoomTypeNames_[room.type_], roomrect.ToString().CString());

        IntVector2 coord, newcoord;
        unsigned tileindex, gotprops;
        unsigned roomfurnitures = 0;

        static Vector<unsigned> objectqtys;

        for (int align=0; align < NumFurnitureAlignements; align++)
        {
            const ConstRoomInfo& roomobjectinfos = sConstRoomInfos_[align * NumRoomTypes + room.type_];
            if (!roomobjectinfos.objects_.Size())
                continue;

            objectqtys.Resize(roomobjectinfos.objects_.Size());
            for (Vector<unsigned>::Iterator it=objectqtys.Begin(); it!=objectqtys.End(); ++it)
                *it = 0;

            if (align < OnWall)
            {
                const IntVector2 endcoord(roomrect.right_-1, align == OnRoof ? roomrect.top_+1 : roomrect.bottom_-1);

                coord.x_ = roomrect.left_+1;
                coord.y_ = newcoord.y_ = endcoord.y_;

                const int maxtries = roomrect.Size().x_;
                for (int essay = 0; essay != maxtries; ++essay)
                {
                    if (coord.x_ > endcoord.x_)
                        break;

                    unsigned iobject = (essay + Random.Get(100)) % roomobjectinfos.objects_.Size();
                    if (roomobjectinfos.maxQuantitiesByObjects_[iobject] && objectqtys[iobject] >= roomobjectinfos.maxQuantitiesByObjects_[iobject])
                        continue;

                    const StringHash& got = roomobjectinfos.objects_[iobject];
                    if (!got)
                    {
                        URHO3D_LOGERRORF("MapGeneratorDungeon() - GenerateFurnitures : room (type=%s rect=%s) : empty furnitures types !", dungeonRoomTypeNames_[room.type_], roomrect.ToString().CString());
                        break;
                    }

                    // reset newcoord y (in case GetPositionFor made changes)
                    newcoord.y_ = coord.y_;

                    const IntVector2& objectrange = roomobjectinfos.objectsSize_[iobject];

//					URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateFurnitures : room (type=%s rect=%s) : try to put a %s objectRange=%s at startcoord=%s endcoord.y=%d ...",
//										dungeonRoomTypeNames_[room.type_], roomrect.ToString().CString(), GOT::GetType(got).CString(), objectrange.ToString().CString(), coord.ToString().CString(), endcoord.y_);

                    if (!GetPositionFor(*roomobjectinfos.requirement_, coord, endcoord, objectrange, newcoord))
                        break;

                    tileindex = newcoord.y_ * xSize_ + newcoord.x_;

                    // put furniture if position on ground and not already a furniture at this location
                    if (!objectrange.x_ || (!checkPositions.Contains(tileindex) && !checkPositions.Contains(tileindex+1)))
                    {
                        gotprops = GOT::GetTypeProperties(got);
                        checkPositions.Push(tileindex);

                        furnitures_.Resize(furnitures_.Size()+1);
                        EntityData& entitydata = furnitures_.Back();
                        //EntityData::Set(short unsigned gotindex=0, short unsigned tileindex=USHRT_MAX, signed char tilepositionx=0, signed char tilepositiony_=0, unsigned char sstype=0, int layerZ=0, bool rotate=false, bool flipX=false, bool flipY=false);
                        entitydata.Set(GOT::GetIndex(got), tileindex, Random.Get(-127, 127), roomobjectinfos.requirement_->finalCellAlign_, GOT::GetConstInfo(got).entityVariation_ ? 255 : 0,
                                       INNERVIEW, false, (gotprops & GOT_Flippable) ? newcoord.x_ > room.center_.x_ : false, false);

                        roomfurnitures++;
                        objectqtys[iobject]++;
                        URHO3D_LOGINFOF("-> place a %s in room=%s at start=%s coord=%s tileindex=%u entitydump=%s !",
                                         GOT::GetType(got).CString(), roomrect.ToString().CString(), coord.ToString().CString(), newcoord.ToString().CString(), tileindex, entitydata.Dump().CString());
                    }

                    coord.x_ = newcoord.x_ + Random.Get(1, objectrange.x_);
                }
            }
            else
            {
                // leftalign - search from bottom to top
                //		------------- top = 52
                // ^	|
                // |	|
                // |	------------- bottom = 56

                const int dirY = 1;
                const IntVector2 endcoord(roomrect.left_+1, dirY > 0 ? roomrect.bottom_-1 : roomrect.top_+1);

                coord.x_ = roomrect.left_+1;
                coord.y_ = dirY > 0 ? roomrect.top_+1 : roomrect.bottom_-1;

                const int maxtries = roomrect.Size().y_;
                for (int essay = 0; essay != maxtries; ++essay)
                {
                    if (dirY > 0)
                    {
                        if (coord.y_ > endcoord.y_)
                            break;
                    }
                    else if (coord.y_ < endcoord.y_)
                    {
                        break;
                    }

                    unsigned iobject = (essay + Random.Get(100)) % roomobjectinfos.objects_.Size();
                    if (roomobjectinfos.maxQuantitiesByObjects_[iobject] && objectqtys[iobject] >= roomobjectinfos.maxQuantitiesByObjects_[iobject])
                        continue;

                    const StringHash& got = roomobjectinfos.objects_[iobject];
                    if (!got)
                    {
                        URHO3D_LOGERRORF("MapGeneratorDungeon() - GenerateFurnitures : room (type=%s rect=%s) : empty furnitures types !", dungeonRoomTypeNames_[room.type_], roomrect.ToString().CString());
                        break;
                    }

                    // reset newcoord x (in case GetPositionFor made changes)
                    newcoord.x_ = coord.x_;

                    const IntVector2& objectrange = roomobjectinfos.objectsSize_[iobject];

                    URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateFurnitures : room (type=%s rect=%s) : try to put a %s objectRange=%s at startcoord=%s endcoord.y=%d ...",
                                    dungeonRoomTypeNames_[room.type_], roomrect.ToString().CString(), GOT::GetType(got).CString(), objectrange.ToString().CString(), coord.ToString().CString(), endcoord.y_);

                    // here the requirement for "pont" is to have no block on the wall and in the room.
                    if (!GetPositionFor(*roomobjectinfos.requirement_, coord, endcoord, objectrange, newcoord))
                        break;

                    // put furniture if the position is found and there is not already a furniture at this location
                    tileindex = newcoord.y_ * xSize_ + newcoord.x_;

                    if (!objectrange.y_ || !checkPositions.Contains(tileindex))
                    {
                        gotprops = GOT::GetTypeProperties(got);
                        checkPositions.Push(tileindex);

                        furnitures_.Resize(furnitures_.Size()+1);
                        EntityData& entitydata = furnitures_.Back();
                        //EntityData::Set(short unsigned gotindex=0, short unsigned tileindex=USHRT_MAX, signed char tilepositionx=0, signed char tilepositiony_=0, unsigned char sstype=0, int layerZ=0, bool rotate=false, bool flipX=false, bool flipY=false);
                        entitydata.Set(GOT::GetIndex(got), tileindex, 110, roomobjectinfos.requirement_->finalCellAlign_, GOT::GetConstInfo(got).entityVariation_ ? 255 : 0, INNERVIEW, false, false, false);

                        roomfurnitures++;
                        objectqtys[iobject]++;
                        URHO3D_LOGINFOF("-> place a %s in room=%s at start=%s coord=%s entitydump=%s !",
                                         GOT::GetType(got).CString(), roomrect.ToString().CString(), coord.ToString().CString(), newcoord.ToString().CString(), entitydata.Dump().CString());
                    }

                    coord.y_ = newcoord.y_ + dirY * Random.Get(1, objectrange.y_);
                }
            }
        }

        furnitures += roomfurnitures;
    }

    if (!furnitures)
        URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateFurnitures : no furnitures created !!!");
    else
        URHO3D_LOGINFOF("MapGeneratorDungeon() - GenerateFurnitures : has placed %d furnitures !", furnitures);
#endif
}

bool MapGeneratorDungeon::PlaceFurniture()
{
    // Pick a Random room
    const RoomInfo& roominfo = dinfo_.rooms_[Random.Get(dinfo_.rooms_.Size())];
    const IntRect& placeinfo = roominfo.rect_;
    IntVector2 place;
    unsigned hashplace;

    // Pick a direction in the room
    switch (Random.GetAllDir())
    {
    case MapDirection::North:

        break;
    case MapDirection::South:

        break;
    case MapDirection::West:

        break;
    case MapDirection::East:

        break;
    case MapDirection::NoBorders:

        break;
    case MapDirection::AllBorders:

        break;
    case MapDirection::All:

        break;
    }

    return true;
}

bool MapGeneratorDungeon::MakeDungeon(MapGeneratorStatus& genStatus, int x, int y, int direction)
{
    Timer timer;

    SetConstRoomInfos();

    dinfo_.entry_.x_ = x = Clamp(x, 0, xSize_-1);
    dinfo_.entry_.y_ = y = Clamp(y, 0, ySize_-1);

    // Make one room at x y to start
    if (!MakeRoom(genStatus, -1, x, y, direction, false, false, true))
    {
        URHO3D_LOGERRORF("MapGeneratorDungeon() - MakeDungeon : map=%s ... No Room Created !", genStatus.mappoint_.ToString().CString());
        return false;
    }

    // Creates Features/Rooms
    {
        int features;
        int maxFeatures = genStatus.genparams_[0];

        for (features = 0; features != maxFeatures; ++features)
        {
            if (!PlaceFeature(genStatus))
                break;
        }

        if (!features)
            URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeDungeon : just initial room created / no other features created (max=%d) !!!", maxFeatures);
        else
            URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeDungeon : has placed %d/%d features  => timer = %u msec", features, maxFeatures, timer.GetMSec(false));
    }

    bool ok = ResolveEntrance(genStatus);

    return true;
}

bool MapGeneratorDungeon::MakeCity(MapGeneratorStatus& genStatus, Map* map, int x)
{
    SetConstRoomInfos();

    const IntVector2 minSize(genStatus.genparams_[3], genStatus.genparams_[3]);
    const IntVector2 maxSize(genStatus.genparams_[4], genStatus.genparams_[4]);
    const IntVector2 medSize((maxSize + minSize) / 2);
    const int height = ySize_ - map->GetTopography().GetFloorTileY(x);
    if (height < medSize.y_)
        return false;

    const int forcedtype = -1;
    const int direction[2] = { 1, -1 };
    const int initialSpace[2] = { xSize_-x, x };

    IntVector2 buildingSizes[NumBuildingTypes-1][2] =
    {
        // Houses
        { IntVector2(minSize.x_, Min(minSize.y_, height)), IntVector2(medSize.x_, Min(medSize.y_, height)) },
        { IntVector2(medSize.x_ * 2 / 3, Min(medSize.x_ * 3 / 2, height)), IntVector2(medSize.x_ + minSize.x_ / 2, Min(medSize.y_ + minSize.y_, height)) },
        { IntVector2(medSize.x_, Min(medSize.y_ + 2 * minSize.y_, height)), IntVector2(maxSize.x_, Min(medSize.y_ + 2 * medSize.y_, height)) },
        // Towers
        { IntVector2(minSize.x_, Max(minSize.y_, height)), IntVector2(medSize.x_, Max(minSize.y_, height)) },
        { IntVector2(medSize.x_, Max(minSize.y_, height)), IntVector2(maxSize.x_, Max(minSize.y_, height)) },
        // Dungeon
        { IntVector2(xSize_ * 2 / 3 / 2, Max(minSize.y_, height)), IntVector2(xSize_ * 2 / 3, Max(minSize.y_, height)) }
    };

    IntVector2 buildingSize;
    int usedSpace[2] = { 0, 0 };
    int buildingCount = 0;
    int dirIndex = 0;
    int spacing;
    int heightAdjust = 0;
    bool ok = false;

    // first centered building
    int buildingType = forcedtype == -1 ? Random.Get(NumBuildingTypes) : forcedtype;

    if (buildingType == ChaoticDungeon)
    {
        ok = MakeDungeon(genStatus, x, height+1, MapDirection::South);
        if (ok)
            buildingCount++;
    }

    if (!ok)
    {
        buildingType = forcedtype == -1 ? Random.Get(NumBuildingTypes-1) : forcedtype;

        URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeCity : x=%d initial building ...", x);

        buildingSize.x_ = Random.Get(buildingSizes[buildingType][0].x_, buildingSizes[buildingType][1].x_);
        buildingSize.y_ = Random.Get(buildingSizes[buildingType][0].y_, buildingSizes[buildingType][1].y_);

        if (buildingSize.x_ / 2 < initialSpace[0] && buildingSize.x_ / 2 < initialSpace[1])
        {
            ok = MakeBuilding(genStatus, map, buildingType, x, buildingSize);
            if (ok)
            {
                usedSpace[0] += (buildingSize.x_/2 + Random.Get(3));
                usedSpace[1] += (buildingSize.x_/2 + Random.Get(3));
                buildingCount++;

                // Add Cave Dungeon
                if (buildingType == Dungeon || height < minSize.y_)
                {
                    URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeCity : add dungeon at x=%d y=%d  ...", x, height+1);

                    ok = MakeDungeon(genStatus, x, height+1, MapDirection::South);
                }
            }
        }

        // next buildings with repartition on alternative horizontal spreading from the center
        while (usedSpace[0] < initialSpace[0] || usedSpace[1] < initialSpace[1])
        {
            ok = false;

            if (usedSpace[dirIndex] < initialSpace[dirIndex])
            {
                spacing = Random.Get(4);

                URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeCity : x=%d direction=%d type=%d usedSpace=%d/%d spacing=%d ...", x, direction[dirIndex], buildingType, usedSpace[dirIndex], initialSpace[dirIndex], spacing);

                if (buildingType < Tower)
                {
                    buildingSize.x_ = Random.Get(buildingSizes[buildingType][0].x_, buildingSizes[buildingType][1].x_);
                    buildingSize.y_ = Random.Get(buildingSizes[buildingType][0].y_, buildingSizes[buildingType][1].y_);
                }
                else if (buildingType >= Tower)
                {
                    // update decrease the height of the tower
                    // on center, the towers will be higher
                    heightAdjust += Random.Get(1, medSize.x_);

                    buildingSize.x_ = Random.Get(buildingSizes[buildingType][0].x_, buildingSizes[buildingType][1].x_);
                    buildingSize.y_ = Random.Get(Max(1, buildingSizes[buildingType][0].y_ - 2 * heightAdjust), Max(2, buildingSizes[buildingType][0].y_ - heightAdjust));
                }

                // place the building if enough space
                if (usedSpace[dirIndex] + buildingSize.x_ < initialSpace[dirIndex])
                    ok = MakeBuilding(genStatus, map, buildingType, x + direction[dirIndex] * (usedSpace[dirIndex] + buildingSize.x_/2 + spacing), buildingSize);

                // update the free space for the next building in the same direction
                usedSpace[dirIndex] += (buildingSize.x_ + spacing);
            }

            if (ok)
                buildingCount++;

            // next iteration

            // set next building type if not forcedTypes
            buildingType = forcedtype == -1 ? Random.Get(NumBuildingTypes-1) : forcedtype;

            dirIndex = (dirIndex+1) % 2;
        }
    }

#ifdef ACTIVE_DUNGEONROOFS
    // Add Roof Tiles
    if (buildingCount > 0)
    {
        for (int i = 0; i < xSize_; i++)
        {
            const int y = ySize_ - map->GetTopography().GetFloorTileY(i);
            if (y >= ySize_-1)
                continue;

            for (int j = 1; j < y-1; j++)
            {
                if ((GetCell(i, j+1) == MapFeatureType::RoomWall || GetCell(i, j+1) == MapFeatureType::InnerSpace)  && GetCell(i, j) != MapFeatureType::NoMapFeature && GetCell(i, j-1) == MapFeatureType::NoMapFeature)
                    SetCell(i, j, MapFeatureType::InnerRoof);
            }
        }
    }
#endif

//    bool ok = ResolveEntrance(genStatus);

    return buildingCount > 0;
}

bool MapGeneratorDungeon::MakeBuilding(MapGeneratorStatus& genStatus, Map* map, int type, int x, const IntVector2& size)
{
    // Create a building of type at x on the floor
    const IntVector2 align(size.x_/2, 0);

    // set the room minimal maximal sizes
    const IntVector2 maxSize(genStatus.genparams_[4], genStatus.genparams_[4]);
    const IntVector2 minSize(genStatus.genparams_[3], genStatus.genparams_[3]);

    int roomCounter = 0;

    // set the first big box with size
    List<RoomBoxInfo> rooms;
    rooms.Push(RoomBoxInfo());
    {
        RoomBoxInfo& room = rooms.Front();

        room.id_ = roomCounter++;

        room.rect_.left_ = Clamp(x - align.x_, 0, xSize_-1);
        room.rect_.right_ = Min(room.rect_.left_ + size.x_, xSize_-1);

        // try to anchor the building on the floor
        // need 3 anchor tiles
        int numAnchors = 0;
        int miny = ySize_ - 1;
        for (int floorx=room.rect_.left_; floorx <=room.rect_.right_; floorx++)
        {
            int floory = map->GetTopography().GetFloorTileY(floorx);
            if (floory < miny)
                miny = floory;
            else if (floory > 0)
                numAnchors++;
        }

        if (numAnchors < 3)
        {
            URHO3D_LOGERRORF("MapGeneratorDungeon() - MakeBuilding : Can't Push First Room(%d) : not enough Anchors  ...", room.id_);
            return false;
        }

        room.rect_.bottom_ = Min(ySize_ - miny, ySize_-1);
        room.rect_.top_    = Clamp(room.rect_.bottom_ - size.y_, 1, room.rect_.bottom_);
        room.size_ = room.rect_.Size();
        if (room.size_.x_ < minSize.x_ || room.size_.y_ < minSize.y_)
        {
            URHO3D_LOGERRORF("MapGeneratorDungeon() - MakeBuilding : Can't Push First Room(%d) rect=%s : too small size  ...", room.id_, room.rect_.ToString().CString());
            return false;
        }
    }

    IntRect towerRect = rooms.Front().rect_;

    URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeBuilding : Start %s centered at x=%d Rect=%s size=%s...", BuildingTypeNames_[type], x, towerRect.ToString().CString(), size.ToString().CString());

    // Divide the rooms if its size is bigger than the maximum room size
    List<RoomBoxInfo>::Iterator it = rooms.Begin();
    while (it != rooms.End())
    {
        RoomBoxInfo& room = *it;

        IntVector2 deltaSize = maxSize - room.size_;

        if (deltaSize.x_ >= 0 && deltaSize.y_ >= 0)
        {
            it++;
            continue;
        }

        // divide along x
        if (deltaSize.x_ < 0)
        {
            // set the width for the new room
            int newwidth = Random.Get(minSize.x_, room.size_.x_/2);

//            URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeBuilding : divide in X newwidth=%d Room(%d) rect=%s size=%s ... ", newwidth, room.id_, room.rect_.ToString().CString(), room.size_.ToString().CString());

            /*

            Left     NewWidth    Right
            ------------------------ Top
            |           |          |
            |   Room    | NewRoom  |
            |           |          |
            ------------------------ Bottom
            */

            // insert the new room
            rooms.Insert(it, RoomBoxInfo());

            RoomBoxInfo& newroom = *(--it);
            newroom.id_ = roomCounter++;
            newroom.size_ = IntVector2(newwidth, room.size_.y_);
            newroom.rect_ = IntRect(room.rect_.right_ - newwidth, room.rect_.top_, room.rect_.right_, room.rect_.bottom_);

            room.size_.x_ -= newwidth;
            room.rect_.right_ -= newwidth;

//            URHO3D_LOGINFOF("... Room(%d) rect=%s ... New Room(%d) rect=%s ...", room.id_, room.rect_.ToString().CString(), newroom.id_, newroom.rect_.ToString().CString());
        }

        // divide along y
        if (deltaSize.y_ < 0)
        {
            // set the height for the new room
            int newheight = Random.Get(minSize.y_, room.size_.y_/2);

//            URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeBuilding : divide in Y newheight=%d => Room(%d) rect=%s size=%s ...", newheight, room.id_, room.rect_.ToString().CString(), room.size_.ToString().CString());

            /*

            Left    Right
            ------------- Top
            |           |
            |   NewRoom |
            |           |
            ------------- NewHeight
            |    Room   |
            ------------- Bottom
            */

            // insert the new room
            rooms.Insert(it, RoomBoxInfo());

            RoomBoxInfo& newroom = *(--it);
            newroom.id_ = roomCounter++;
            newroom.size_ = IntVector2(room.size_.x_, newheight);
            newroom.rect_ = IntRect(room.rect_.left_, room.rect_.top_ , room.rect_.right_, room.rect_.top_ + newheight);

            room.size_.y_ -= newheight;
            room.rect_.top_ = newroom.rect_.bottom_;

//            URHO3D_LOGINFOF("... Room(%d) rect=%s ... New Room(%d) rect=%s ...", room.id_, room.rect_.ToString().CString(), newroom.id_, newroom.rect_.ToString().CString());
        }
    }

    // Registering and Setting the rooms
    unsigned startIndex = dinfo_.rooms_.Size();
    {
        dinfo_.rooms_.Resize(startIndex + rooms.Size());
        int i = startIndex;
        for (List<RoomBoxInfo>::Iterator it = rooms.Begin(); it != rooms.End(); it++, i++)
        {
            RoomBoxInfo& roombox = *it;

            // Add Room Structure to Dungeon Info
            RoomInfo& room = dinfo_.rooms_[i];
            room.id_ = roombox.id_;

            SetRoom(room, ROOM_RECTANGLE, roombox.size_, roombox.rect_);

    #ifdef ACTIVE_DUNGEONRANDOMWALK
            bool randomWalk = type == Dungeon && room.type_ != BossRoom && roombox.size_.x_ > maxSize.x_-5 && roombox.size_.y_ > maxSize.y_-5;
    #else
            bool randomWalk = false;
    #endif
            room.entry_.x_ = roombox.rect_.right_;
            room.entry_.y_ = roombox.rect_.bottom_-1;

            SetGeometry(room, randomWalk);

            if (!randomWalk)
            {
                AddPlateforms(room);

                AddWindows(room);
            }

            URHO3D_LOGINFOF("MapGeneratorDungeon() - MakeBuilding : Setting Room(%d) rect=%s size=%s type=%d ...", room.id_, room.rect_.ToString().CString(), room.size_.ToString().CString(), room.type_);
        }
    }

    // Add Main Entrance
    {
        RoomInfo& room = dinfo_.rooms_[startIndex];
        SetCell(room.rect_.left_, room.rect_.bottom_-1, MapFeatureType::InnerSpace);
        if (GetCell(room.rect_.left_-1, room.rect_.bottom_-1) > MapFeatureType::Threshold)
            SetCell(room.rect_.left_-1, room.rect_.bottom_-1, MapFeatureType::InnerSpace);
    }

    // Add entrances for each rooms
    for (unsigned i = startIndex; i < dinfo_.rooms_.Size(); i++)
    {
        RoomInfo& room = dinfo_.rooms_[i];

        // Add Entry from right
        if (GetCell(room.entry_.x_, room.entry_.y_) > MapFeatureType::Threshold)
            SetCell(room.entry_.x_, room.entry_.y_, MapFeatureType::InnerSpace);
        if (GetCell(room.entry_.x_+1, room.entry_.y_) > MapFeatureType::Threshold)
            SetCell(room.entry_.x_+1, room.entry_.y_, MapFeatureType::InnerSpace);

        // Add Exitx if defined
        if (room.exitx_.x_ != -1)
        {
            if (GetCell(room.exitx_.x_, room.exitx_.y_) > MapFeatureType::Threshold)
                SetCell(room.exitx_.x_, room.exitx_.y_, MapFeatureType::InnerSpace);
            if (GetCell(room.exitx_.x_+1, room.exitx_.y_) > MapFeatureType::Threshold)
                SetCell(room.exitx_.x_+1, room.exitx_.y_, MapFeatureType::InnerSpace);
            if (GetCell(room.exitx_.x_-1, room.exitx_.y_) > MapFeatureType::Threshold)
                SetCell(room.exitx_.x_-1, room.exitx_.y_, MapFeatureType::InnerSpace);
        }

        // Add Exity if defined
        if (room.exity_.x_ != -1)
        {
            if (GetCell(room.exity_.x_, room.exity_.y_) > MapFeatureType::Threshold)
                SetCell(room.exity_.x_, room.exity_.y_, MapFeatureType::InnerSpace);
            if (GetCell(room.exity_.x_, room.exity_.y_-1) > MapFeatureType::Threshold)
                SetCell(room.exity_.x_, room.exity_.y_-1, MapFeatureType::InnerSpace);
        }
        else
        {
            // Add Random Roof Entry
            int r = Random.Get(room.size_.x_) -1;
            if (r >= 0 && r < room.size_.x_/2)
            {
                int holex = room.center_.x_ + (Random.Get(100) < 50 ? -1 : 1) * r;
                SetCell(holex, room.rect_.top_, MapFeatureType::InnerSpace);
                if (GetCell(holex, room.rect_.top_+1) > MapFeatureType::Threshold)
                    SetCell(holex, room.rect_.top_+1, MapFeatureType::InnerSpace);
            }
        }
    }

    return true;
}


bool MapGeneratorDungeon::ResolveEntrance(MapGeneratorStatus& genStatus)
{
#ifdef RESOLVE_ENTRANCE
    Timer timer;

    // Resolve Entrance : Check Exit starting at SPOT_START
    if (dinfo_.rooms_.Size())
    {
        FeatureType fillPattern = 42; // char '*'
        int roomInfoSize = dinfo_.rooms_.Size();
        int rindex = 0;

        for (;;)
        {
            if (rindex >= roomInfoSize)
                break;

            checkBuffer_.SetBufferFrom(GetBuffer());

            if (GameHelpers::FloodFill(checkBuffer_, dungStack_, (FeatureType) MapFeatureType::NoMapFeature, (FeatureType) MapFeatureType::Threshold,
                                       fillPattern, dinfo_.rooms_[rindex].center_.x_, dinfo_.rooms_[rindex].center_.y_))
            {
                IntVector2 point;
                IntVector2 direction = GameHelpers::BorderContains(checkBuffer_, fillPattern, point);

                // if the borders don't contain a fillPattern Value, find an exit to dungeon !
                if (direction == IntVector2::ZERO)
                {
                    URHO3D_LOGWARNINGF("MapGeneratorDungeon() - ResolveEntrance : No Exit => Try To Find a Room Wall close to Outside ...");
//                    URHO3D_LOGINFOF("MapGeneratorDungeon() - ResolveEntrance : Check Exit => FEATURE MAP with Flooding (* = OK, ? = NOK) : ");
//                    GameHelpers::DumpData(checkBuffer_.Buffer(), (FeatureType) MapFeatureType::InnerFloor, 2, xSize_, ySize_);

                    // Try to Find a wall which has a contact with outside in the rooms not checked yet
                    if (rindex >= roomInfoSize)
                    {
//                        URHO3D_LOGINFOF("MapGeneratorDungeon() - ResolveEntrance : ... No Exit for this dungeon !");
                        break;
                    }
                    int i;
                    for (i = rindex; i < roomInfoSize; i++)
                    {
                        const IntRect& roomBorder = dinfo_.rooms_[i].rect_;

                        // if finding an outside wall, add a break in it
                        if ((direction = GameHelpers::BorderContains(checkBuffer_, (FeatureType) MapFeatureType::NoMapFeature, point, roomBorder, 1)) != IntVector2::ZERO)
                        {
//                            SetCell(point.x_-direction.x_, point.y_, MapFeatureType::InnerSpace);
//                            SetCell(point.x_+direction.x_, point.y_, MapFeatureType::InnerSpace);
//                            SetCell(point.x_, point.y_-direction.y_, MapFeatureType::InnerSpace);
//                            SetCell(point.x_, point.y_+direction.y_, MapFeatureType::InnerSpace);
                            SetCell(point.x_-direction.x_, point.y_-direction.y_, MapFeatureType::InnerSpace);
//                            SetCell(point.x_-direction.x_, point.y_-direction.y_, MapFeatureType::Threshold);
                            rindex = i++;
                            URHO3D_LOGWARNINGF("MapGeneratorDungeon() - ResolveEntrance : ... Find a Wall At %s (dir=%s)! add an Exit ! ...", point.ToString().CString(), direction.ToString().CString());
//                            GameHelpers::DumpData(GetMap(), 1U, 2, xSize_, ySize_);
                            break;
                        }
                    }
                    rindex = i;
                }
                else
                {
//                    if (rindex > 0)
//                    {
//                        URHO3D_LOGINFOF("MapGeneratorDungeon() - ResolveEntrance : Check Again Exit => FEATURE MAP with Flooding (* = OK, ? = NOK) : ");
//                        DumpData(checkBuffer_.Buffer(), (FeatureType) MapFeatureType::InnerFloor, 2, xSize_, ySize_);
//                    }
//                    URHO3D_LOGINFOF("MapGeneratorDungeon() - ResolveEntrance : the dungeon has at least an Exit => timer = %u msec", timer.GetMSec(false));
                    break;
                }
            }
            else
            {
                URHO3D_LOGERRORF("MapGeneratorDungeon() - ResolveEntrance : map=%s timer=%u msec ... FloodFill Error : No Exit for this dungeon from room=id=%d,x=%d,y=%d!",
                                 genStatus.mappoint_.ToString().CString(), timer.GetMSec(false),
                                 dinfo_.rooms_[rindex].id_, dinfo_.rooms_[rindex].center_.x_, dinfo_.rooms_[rindex].center_.y_);
                GameHelpers::DumpData(checkBuffer_.Buffer(), (FeatureType) MapFeatureType::InnerFloor, 2, xSize_, ySize_);
                break;
            }
        }
    }
#endif

    return true;
}

bool MapGeneratorDungeon::PlaceFeature(MapGeneratorStatus& genStatus)
{
    int tries = 0;
    int maxTries = xSize_ * ySize_ / 10;
    int x, y, fromRoom;
    bool pickRandomPlace;
    for( ; tries != maxTries; ++tries)
    {
        pickRandomPlace = false;
        if (dinfo_.rooms_.Size())
            pickRandomPlace = Random.Get(0, 100) < 70;

        // Pick a Created Random Place
        if (pickRandomPlace)
        {
            fromRoom = dinfo_.rooms_.Size() > 1 ? Random.Get(dinfo_.rooms_.Size()) : 0;
            const IntRect& placeinfo = dinfo_.rooms_[fromRoom].rect_;

            // Pick a wall direction
            switch (Random.GetDir())
            {
            case MapDirection::North:
                x = Random.Get(placeinfo.left_, placeinfo.right_+1);
                y = placeinfo.bottom_;
                break;
            case MapDirection::South:
                x = Random.Get(placeinfo.left_, placeinfo.right_+1);
                y = placeinfo.top_;
                break;
            case MapDirection::West:
                x = placeinfo.left_;
                y = Random.Get(placeinfo.top_, placeinfo.bottom_+1);
                break;
            case MapDirection::East:
                x = placeinfo.right_;
                y = Random.Get(placeinfo.top_, placeinfo.bottom_+1);
                break;
            }
            pickRandomPlace = IsXInsideBorder(x) && IsYInsideBorder(y);
        }

        // Pick a random wall or corridor tile in the map
        if (!pickRandomPlace)
        {
            fromRoom = -1;
            x = Random.Get(1, xSize_ - 2);
            y = Random.Get(1, ySize_ - 2);
        }

        if (GetCell(x, y) != MapFeatureType::RoomWall && GetCell(x, y) != MapFeatureType::CorridorWall)
            continue;

        // Make sure it has no adjacent doors (looks weird to have doors next to each other).
//        if (IsAdjacent(x, y, MapFeatureType::Threshold))
//            continue;

        // Find a direction from which it's reachable.
        // Attempt to make a feature (room or corridor) starting at this point.

        if (GetCell(x, y + 1) == MapFeatureType::InnerSpace || GetCell(x, y + 1) == MapFeatureType::CorridorPlateForm)
        {
            if (MakeFeature(genStatus, fromRoom, x, y, MapDirection::North))
                return true;
        }

        else if (GetCell(x - 1, y) == MapFeatureType::InnerSpace || GetCell(x - 1, y) == MapFeatureType::CorridorPlateForm)
        {
            if (MakeFeature(genStatus, fromRoom, x, y, MapDirection::East))
                return true;
        }

        else if (GetCell(x, y - 1) == MapFeatureType::InnerSpace || GetCell(x, y - 1) == MapFeatureType::CorridorPlateForm)
        {
            if (MakeFeature(genStatus, fromRoom, x, y, MapDirection::South))
                return true;
        }

        else if (GetCell(x + 1, y) == MapFeatureType::InnerSpace || GetCell(x + 1, y) == MapFeatureType::CorridorPlateForm)
        {
            if (MakeFeature(genStatus, fromRoom, x, y, MapDirection::West))
                return true;
        }
    }

    return false;
}

bool MapGeneratorDungeon::MakeFeature(MapGeneratorStatus& genStatus, int fromRoom, int x, int y, int direction)
{
    // Choose what feature to build
    int chance = Random.Get(0, 100);

    if (chance <= genStatus.genparams_[1])
    {
        if (MakeRoom(genStatus, fromRoom, x, y, direction, chance > genStatus.genparams_[1] / 2, chance > genStatus.genparams_[1] / 3))
        {
            //URHO3D_LOGINFOF("Room: from (%d %d) to (%d %d)", xStart, yStart, xEnd, yEnd);
            return true;
        }

        return false;
    }
    else if (direction == MapDirection::East || direction == MapDirection::West)
    {
        if (MakeCorridorPlateforms(x, y, 3, direction))
//        if (MakeCorridorPlateforms(x, y, 6, direction))
        {
            if (IsYInBounds(y-1))
                SetCell(x, y-1, MapFeatureType::InnerSpace);
//                SetCell(x, y-1, MapFeatureType::Threshold);

            return true;
        }

        return false;
    }

    return false;
}

bool MapGeneratorDungeon::MakeRoom(MapGeneratorStatus& genStatus, int fromRoom, int x, int y, int direction, bool addplateforms, bool addwindows, bool makeEntrance)
{
    // Minimum room size of 4x4 tiles (2x2 for walking on, the rest is walls)

    int xLength = Random.Get(genStatus.genparams_[3], genStatus.genparams_[4]);
    int yLength = Random.Get(genStatus.genparams_[3], genStatus.genparams_[4]);
    int xStart, yStart;
    int xEnd, yEnd;
    int xmod, ymod;

    switch (direction)
    {
    case MapDirection::North :
        xmod = 0;
        ymod = -1;
        y += ymod;
        yStart = y - yLength;
        yEnd = y;
        xStart = x - xLength / 2;
        xEnd = x + (xLength + 1) / 2;
        break;
    case MapDirection::East :
        xmod = 1;
        ymod = 0;
        x += xmod;
        yStart = y - yLength / 2;
        yEnd = y + (yLength + 1) / 2;
        xStart = x;
        xEnd = x + xLength;
        break;
    case MapDirection::South :
        xmod = 0;
        ymod = 1;
        y += xmod;
        yStart = y;
        yEnd = y + yLength;
        xStart = x - xLength / 2;
        xEnd = x + (xLength + 1) / 2;
        break;
    case MapDirection::West :
        xmod = -1;
        ymod = 0;
        x += xmod;
        yStart = y - yLength / 2;
        yEnd = y + (yLength + 1) / 2;
        xStart = x - xLength;
        xEnd = x;
        break;
    }

    if (!IsXInBounds(xStart) || !IsXInBounds(xEnd) || !IsYInBounds(yStart) || !IsYInBounds(yEnd))
    {
//        URHO3D_LOGWARNINGF("Room: from (%d %d) to (%d %d) Out of Bounds !", xStart, yStart, xEnd, yEnd);
        return false;
    }

    if (!IsAreaUnused(xStart, yStart, xEnd, yEnd, MapFeatureType::OuterFloor))
    {
//        URHO3D_LOGWARNINGF("Room: from (%d %d) to (%d %d) Area Used !", xStart, yStart, xEnd, yEnd);
        return false;
    }

    // Add Room Structure to Dungeon Info
    dinfo_.rooms_.Resize(dinfo_.rooms_.Size()+1);

    RoomInfo& room = dinfo_.rooms_.Back();

    SetRoom(room, fromRoom < 0 ? ROOM_RECTANGLE : ROOM_ROUNDED, IntVector2(xLength, yLength), IntRect(xStart, yStart, xEnd, yEnd));

#ifdef ACTIVE_DUNGEONRANDOMWALK
    bool randomWalk = room.type_ != BossRoom && fromRoom < 0 && xLength > 5 && yLength > 5;
#else
    bool randomWalk = false;
#endif

    room.entry_ = IntVector2(-1,-1);

    SetGeometry(room, randomWalk);

    if (addplateforms && room.geom_ == ROOM_RECTANGLE && !randomWalk)
        AddPlateforms(room);

    if (addwindows)
        AddWindows(room);

    // Remove wall next to the door.
    if (makeEntrance)
    {
        // entrance on ground
        if (room.geom_ == ROOM_RECTANGLE && fromRoom != -1 && direction >= MapDirection::East)
        {
            const IntRect& fromRomRect = dinfo_.rooms_[fromRoom].rect_;
            yEnd = fromRomRect.bottom_ < yEnd ? fromRomRect.bottom_ : yEnd;
            yEnd--;
            SetCell(x + 2*xmod, yEnd, MapFeatureType::InnerSpace);
            SetCell(x + xmod, yEnd, MapFeatureType::InnerSpace);
            SetCell(x, yEnd, MapFeatureType::InnerSpace);
            SetCell(x - xmod, yEnd, MapFeatureType::InnerSpace);
            SetCell(x - 2*xmod, yEnd, MapFeatureType::InnerSpace);
            room.entry_.x_ = x;
            room.entry_.y_ = yEnd;
        }
        // centered entrance
        else
        {
            SetCell(x + xmod, y + ymod, MapFeatureType::InnerSpace);
            SetCell(x, y, MapFeatureType::InnerSpace);
            SetCell(x - xmod, y - ymod, MapFeatureType::InnerSpace);
            room.entry_.x_ = x;
            room.entry_.y_ = y;
        }
    }

    return true;
}

void MapGeneratorDungeon::SetRoom(RoomInfo& room, int geom, const IntVector2& size, const IntRect& rect)
{
    int& type = room.type_;

    room.geom_ = geom;

    // if too small room => Make Rectangle
    if (geom == ROOM_ROUNDED && (size.x_ < 7 || size.y_ < 7))
        room.geom_ = ROOM_RECTANGLE;

    // if even dimension => Make Rectangle
    if (geom == ROOM_ROUNDED && (size.x_%2 == 0 || size.y_%2 == 0))
        room.geom_ = ROOM_RECTANGLE;

    room.rect_ = rect;
    room.size_ = size;
    room.center_.x_ = (rect.left_ + rect.right_) / 2;
    room.center_.y_ = (rect.top_ + rect.bottom_) / 2;
    room.entry_.x_ = room.exitx_.x_ = room.exity_.x_ = -1;

    type = Chamber;

    if (size.y_ < 3)
    {
        if (size.x_ < 7)
        {
            type = Random.Get(Chamber, Office);
        }
        else
        {
            type = Corridor;
        }
    }
    else
    {
        if (size.x_ < 3)
        {
            type = Corridor;
        }
        else if (size.x_ < 7)
        {
            if (size.x_ < 5 && size.y_ > 8)
                type = Pit;
            else
                type = Random.Get(Chamber, Office);
        }
        else if (size.x_ < BossRoomMinSize.x_ || size.y_ < BossRoomMinSize.y_)
        {
            if (size.y_ < 6 && Random.Get(100) < 10)
                type = Random.Get(Sorcererroom, Priestroom);
            else
                type = Random.Get(Kitchen, Refectory);
        }
        else
        {
            type = BossRoom;
        }
    }

//    URHO3D_LOGERRORF("MapGeneratorDungeon() - SetRoom : type=%s rect=%s", dungeonRoomTypeNames_[type], rect.ToString().CString());
}

void MapGeneratorDungeon::SetGeometry(RoomInfo& room, bool randomWalk)
{
    if (room.geom_ == ROOM_RECTANGLE)
    {
        if (randomWalk)
        {
            SetCells(room.rect_.left_, room.rect_.top_, room.rect_.right_, room.rect_.bottom_, MapFeatureType::RoomWall);
            MakeRandomWalk(room);
            RemoveUntidyWalls(room.rect_);
        }
        else
        {
            SetBorderCells(room.rect_.left_, room.rect_.top_, room.rect_.right_, room.rect_.bottom_, MapFeatureType::RoomWall);
            SetCells(room.rect_.left_ + 1, room.rect_.top_ + 1, room.rect_.right_ - 1, room.rect_.bottom_ - 1, MapFeatureType::InnerSpace);
        }
    }
    else if (room.geom_ == ROOM_ROUNDED)
    {
        PixelShape* shape = GameHelpers::GetPixelShape(PIXSHAPE_SUPERELLIPSE, room.size_.x_, room.size_.y_, MapFeatureType::RoomWall, MapFeatureType::InnerSpace, room.id_*room.size_.x_*room.size_.y_);
        if (shape)
            CopyShapeToCells(shape, room);
        else
            URHO3D_LOGERRORF("MapGeneratorDungeon() - SetGeometry : can't Get PixelShape !");

        RemoveUntidyWalls(room.rect_);
    }
}

bool MapGeneratorDungeon::AddPlateforms(const RoomInfo& room)
{
#ifdef ACTIVE_DUNGEONPLATEFORMS
    const IntRect& roominfo = room.rect_;

    if (roominfo.Width() < 4 || roominfo.Height() < 4)
        return false;

    const int PLATEFORM_AVERAGE_LENGTH = Random.Get(1, roominfo.Width()-3);
    const int PLATEFORM_YSPACE = 3;
    const int PLATEFORM_XSPACE = 5;

    int numPlateformsY = (roominfo.Height()-2) / PLATEFORM_YSPACE;
    int numPlateformsX = (roominfo.Width()-2) / PLATEFORM_AVERAGE_LENGTH;

    if (!numPlateformsX || !numPlateformsY) return false;

    int xStart = roominfo.left_;
    int xEnd = roominfo.right_;
    int yStart = roominfo.top_;
    int yEnd = roominfo.bottom_;

    int y = yStart + PLATEFORM_YSPACE;
    int x1, x2;
    while (y < yEnd-1)
    {
        x1 = xStart + Random.Get(2, PLATEFORM_XSPACE);
        x2 = x1 + Random.Get(1, PLATEFORM_AVERAGE_LENGTH);

        while (x1 < x2 && x2 < xEnd-1)
        {
            SetCells(x1, y, x2, y, MapFeatureType::RoomPlateForm);

            x1 = x2 + Random.Get(2, PLATEFORM_XSPACE);
            x2 = x1 + Random.Get(1, PLATEFORM_AVERAGE_LENGTH);

//            URHO3D_LOGINFOF("MapGeneratorDungeon() - AddPlateforms : at (x:%d < %d, y:%d) inRoom(x:%d < %d, y:%d < %d)", x1, x2, y, xStart, xEnd, yStart, yEnd);
        }

        y += PLATEFORM_YSPACE;
    }
#endif
    return true;
//    SetShapes(type, x, y, MapFeatureType::RoomPlateForm);
}

bool MapGeneratorDungeon::AddWindows(const RoomInfo& room)
{
#ifdef ACTIVE_DUNGEONWINDOWS
    const IntRect& roominfo = room.rect_;

    const int WINDOW_SPACE = 3;

    int numWindowsX = (roominfo.Width()-2) / WINDOW_SPACE;
    int numWindowsY = (roominfo.Height()-2) / WINDOW_SPACE;

    if (numWindowsX <= 0 || numWindowsY <= 0) return false;

    int xStart = roominfo.left_;
    int xEnd = roominfo.right_;
    int yStart = roominfo.top_;
    int yEnd = roominfo.bottom_;

    int y = yStart;
    int x;
    for (int i=0; i < numWindowsY; ++i)
    {
        y += WINDOW_SPACE;
        if (y > yEnd) break;

        x = xStart;
        for (int j=0; j < numWindowsX; ++j)
        {
            x += Random.Get(2, WINDOW_SPACE);
            if (x >= xEnd)
                break;

            if (GetCell(x, y) == MapFeatureType::InnerSpace)
                SetCells(x, y, x, y, MapFeatureType::Window);
        }
    }
#endif
    return true;
}

bool MapGeneratorDungeon::MakeCorridorPlateforms(int x, int y, int maxLength, int direction)
{
    int length = Random.Get(0, maxLength);

    int xStart, yStart;
    int xEnd, yEnd;
    int xmod, ymod;

    // Adjust the position for a new feature in function of the direction
    switch (direction)
    {
    case MapDirection::North :
        xmod = 0;
        ymod = -1;
        y += ymod;
        xStart = xEnd = x;
        yStart = y - length;
        yEnd = y;
        break;
    case MapDirection::East :
        xmod = 1;
        ymod = 0;
        x += xmod;
        xStart = x;
        xEnd = x + length;
        yStart = yEnd = y;
        break;
    case MapDirection::South :
        xmod = 0;
        ymod = 1;
        y += ymod;
        xStart = xEnd = x;
        yStart = y;
        yEnd = y + length;
        break;
    case MapDirection::West :
        xmod = -1;
        ymod = 0;
        x += xmod;
        xStart = x - length;
        xEnd = x;
        yStart = yEnd = y;
        break;
    }

    if (!IsXInBounds(xStart) || !IsXInBounds(xEnd) || !IsYInBounds(yStart) || !IsYInBounds(yEnd))
        return false;

    if (!IsAreaUnused(xStart, yStart, xEnd, yEnd, MapFeatureType::OuterFloor))
        return false;

    SetCells(xStart, yStart, xEnd, yEnd, MapFeatureType::CorridorPlateForm);

//    URHO3D_LOGINFOF("Corridor: from (%d %d) to (%d %d)", xStart, yStart, xEnd, yEnd);

    return true;
}

bool MapGeneratorDungeon::MakeRandomWalk(RoomInfo& room)
{
	int x, y, dx, dy, straight, n, r;

	float coverage = (20 + Random.Get() % 16) * 0.01f;
	n = (room.size_.x_ * room.size_.y_) * coverage;
	dx = dy = straight = 0;

    if (room.entry_.x_ == -1)
    {
        x = room.rect_.left_ + 1 + Random.Get() % (room.size_.x_ - 2);
        y = room.rect_.top_ +  1 + Random.Get() % (room.size_.y_ - 2);
    }
    else
    {
        x = room.entry_.x_;
        y = room.entry_.y_;
    }

    URHO3D_LOGERRORF("MapGeneratorDungeon() - MakeRandomWalk : room=%d rect=%s entry=%d %d ...", room.id_, room.rect_.ToString().CString(), x, y);

	SetCell(x, y, MapFeatureType::InnerSpace);

	room.exitx_.x_ = room.exity_.x_ = -1;

	while (n > 0)
	{
		straight = Max(straight - 1, 0);

		if (straight == 0)
		{
		    r = Random.Get(5);
            if (r > 3)
            {
                straight = 4 + Random.Get(8);
            }
            else
            {
                dx = MapDirection::GetDx(r);
                dy = MapDirection::GetDy(r);
            }
		}

		x = Min(Max(x + dx, room.rect_.left_ + 1), room.rect_.right_ -  1);
		y = Min(Max(y + dy, room.rect_.top_ +  1), room.rect_.bottom_ - 1);

		if (GetCell(x, y) == MapFeatureType::RoomWall)
		{
		    SetCell(x, y, MapFeatureType::InnerSpace);

		    if (room.exitx_.x_ == -1)
            {
                // add an exit in x
                if (x+1 == room.rect_.right_ && GetCell(x+1, y) == MapFeatureType::RoomWall)
                    room.exitx_ = IntVector2(x+1, y);
                else if (x-1 == room.rect_.left_ && GetCell(x-1, y) == MapFeatureType::RoomWall)
                    room.exitx_ = IntVector2(x-1, y);
            }

		    if (room.exity_.x_ == -1)
            {
                // add an roof access
                if (y-1 == room.rect_.top_ && GetCell(x, y-1) == MapFeatureType::RoomWall)
                    room.exity_ = IntVector2(x, y-1);
            }

			n--;
		}
	}

//	RemoveUntidyWalls(room.rect_);

	return true;
}

void MapGeneratorDungeon::RemoveUntidyWalls(const IntRect& rect)
{
    int x, y, i, n;
    unsigned addr;
//    bool wallsRemoved;
    FeatureType cell;

    dungStack_.Clear();

//    do
	{
//		wallsRemoved = false;

        for (x = rect.left_+1; x < rect.right_; x++)
        {
            for (y = rect.top_+1; y < rect.bottom_; y++)
            {
                addr = y * xSize_ + x;

                FeatureType cell = GetCell(addr);

                if (cell >= MapFeatureType::RoomWall && cell <= MapFeatureType::TunnelWall)
                {
                    n = 0;
                    i = 0;
                    while (i < 8 && n < 2)
                    {
                        cell = GetCell(addr + nghTable8_[i]);
                        if (cell >= MapFeatureType::RoomWall && cell <= MapFeatureType::TunnelWall)
                            n++;
                        i++;
                    }

                    if (n < 2)
                        dungStack_.Push(addr);
                }
            }
        }

        // Do the removing
        if (!dungStack_.IsEmpty())
        {
            while (dungStack_.Pop(addr))
                SetCell(addr, MapFeatureType::RoomInnerSpace);

//            wallsRemoved = true;
        }
	}
//	while (wallsRemoved);
}

bool MapGeneratorDungeon::MakeStairs(FeatureType celltype)
{
    int tries = 0;
    int maxTries = 10000;

    for ( ; tries != maxTries; ++tries)
    {
        int x = Random.Get(1, xSize_ - 2);
        int y = Random.Get(1, ySize_ - 2);

        if (!IsAdjacent(x, y, MapFeatureType::RoomFloor) && !IsAdjacent(x, y, MapFeatureType::CorridorPlateForm))
            continue;

        if (IsAdjacent(x, y, MapFeatureType::InnerSpace))
//        if (IsAdjacent(x, y, MapFeatureType::Threshold))
            continue;

        SetCell(x, y, celltype);

        return true;
    }

    return false;
}



// Find a cell of type in the direction
// start from bottom of map with North
// start from top with South
// East and West not implemented
bool MapGeneratorDungeon::FindCell(FeatureType celltype, int* findx, int* findy, int direction, bool resetStartCoords)
{
    int yStart, yEnd, yInc;

    if (direction == MapDirection::South)
    {
        if (resetStartCoords)
            yStart = 0;
        yEnd = ySize_;
        yInc = +1;
    }
    else if (direction == MapDirection::North)
    {
        if (resetStartCoords)
            yStart = ySize_-1;
        yEnd = -1;
        yInc = -1;
    }
    else
    {
        URHO3D_LOGERRORF("MapGeneratorDungeon() - FindCell : direction=%d NOT IMPLEMENTED !", direction);
        return false;
    }

    if (!resetStartCoords)
        yStart = *findy;

    for (int y = yStart; y != yEnd; y+=yInc)
    {
        for (int x = xSize_/2; x > 0; --x)
            if (GetCell(x,y) == celltype)
            {
                *findx = x;
                *findy = y;
                return true;
            }
        for (int x = (xSize_/2)+1; x < xSize_; ++x)
            if (GetCell(x,y) == celltype)
            {
                *findx = x;
                *findy = y;
                return true;
            }
    }

    return false;
}

void MapGeneratorDungeon::SetCells(int xStart, int yStart, int xEnd, int yEnd, FeatureType cellType)
{
    assert(yStart >= 0 && yEnd < ySize_ && yStart <= yEnd && xStart >= 0 && xEnd < xSize_ && xStart <= xEnd);

    for (int y = yStart; y != yEnd + 1; ++y)
        for (int x = xStart; x != xEnd + 1; ++x)
            SetCell(x, y, cellType);
}

void MapGeneratorDungeon::SetBorderCells(int xStart, int yStart, int xEnd, int yEnd, FeatureType cellType)
{
    // horizontal border
    for (int x = xStart; x <= xEnd; ++x)
    {
        SetCell(x, yStart, cellType);
        SetCell(x, yEnd, cellType);
    }
    // vertical border
    for (int y = yStart+1; y < yEnd; ++y)
    {
        SetCell(xStart, y, cellType);
        SetCell(xEnd, y, cellType);
    }
}

void MapGeneratorDungeon::CopyShapeToCells(PixelShape* shape, const RoomInfo& room)
{
//    URHO3D_LOGERRORF("MapGeneratorDungeon() - CopyShapeToCells : shape=%u !", shape);
    unsigned src_dw = shape->width_ * sizeof(FeatureType);
    unsigned src_skip = src_dw;
    unsigned dst_start = (room.rect_.top_ * xSize_ + room.rect_.left_)  * sizeof(FeatureType);
    unsigned dst_dw = xSize_ * sizeof(FeatureType);
    CopyFast((char*)map_ + dst_start, (char*)(&shape->data_[0]), dst_dw, src_dw, src_skip, shape->height_);
}

bool MapGeneratorDungeon::IsXInBounds(int x) const
{
    return x >= 0 && x < xSize_;
}

bool MapGeneratorDungeon::IsYInBounds(int y) const
{
    return y >= 0 && y < ySize_;
}

bool MapGeneratorDungeon::IsXInsideBorder(int x) const
{
    return x > 0 && x < xSize_-1;
}

bool MapGeneratorDungeon::IsYInsideBorder(int y) const
{
    return y > 0 && y < ySize_-1;
}

bool MapGeneratorDungeon::IsAreaUnused(int xStart, int yStart, int xEnd, int yEnd, FeatureType skipType) const
{
    if (skipType)
    {
        unsigned char cell;
        for (int y = yStart; y != yEnd + 1; ++y)
            for (int x = xStart; x != xEnd + 1; ++x)
            {
                cell = GetCell(x, y);
                if (cell != MapFeatureType::NoMapFeature && cell != skipType)
                    return false;
            }
    }
    else
    {
        for (int y = yStart; y != yEnd + 1; ++y)
            for (int x = xStart; x != xEnd + 1; ++x)
                if (GetCell(x, y) != MapFeatureType::NoMapFeature)
                    return false;
    }

    return true;
}

bool MapGeneratorDungeon::IsAdjacent(int x, int y, FeatureType cellType) const
{
    return
        GetCell(x - 1, y) == cellType || GetCell(x + 1, y) == cellType ||
        GetCell(x, y - 1) == cellType || GetCell(x, y + 1) == cellType;
}


// Find a Position X for the "size" (eq. range here) of an entity
bool MapGeneratorDungeon::GetCellCoordX(const FurnitureRequirement& requirement, const IntVector2& start, const IntVector2& range, int xEnd, const int incY, int& resultx) const
{
    bool check = false;

    // check only at start.x
    if (!range.x_)
    {
        check = requirement.supportIsSolid_ ? GetCell(start.x_, start.y_ - incY) > MapFeatureType::NoRender : GetCell(start.x_, start.y_ - incY) < MapFeatureType::NoRender;

        if (check)
        {
            const int rangestart = incY >= 0 ? 0 : range.y_;
            const int rangeend = incY >= 0 ? range.y_ : 0;

            int yr = rangestart;
            if (requirement.rangeIsSolid_)
            {
                while (check && yr != rangeend)
                {
                    check = GetCell(start.x_, start.y_ + incY * yr) > MapFeatureType::NoRender;
                    yr += incY;
                }
            }
            else
            {
                while (check && yr != rangeend)
                {
                    check = GetCell(start.x_, start.y_ + incY * yr) < MapFeatureType::NoRender;
                    yr += incY;
                }
            }
        }

        if (check)
            resultx = start.x_;
    }
    // check from start.x to xEnd
    else
    {
        const int rangestart = incY >= 0 ? 0 : range.y_;
        const int rangeend = incY >= 0 ? range.y_ : 0;
        const int incX = xEnd - start.x_ >=0 ? 1 : -1;
        const int minx = incX > 0 ? start.x_ : xEnd;
        const int maxx= incX > 0 ? xEnd - range.x_ - 1 : start.x_ - range.x_ - 1;

        int xr, yr;
        int x = start.x_;

        while (x >= minx && x <= maxx)
        {
            check = true;

            // Check if the horizontal carrier matches the requirement (block or hole)
            xr = 0;
            yr = start.y_ - incY;
            if (requirement.supportIsSolid_)
            {
                while (check && xr < range.x_)
                {
                    check = GetCell(x + incX * xr, yr) > MapFeatureType::NoRender;
                    xr++;
                }
            }
            else
            {
                while (check && xr < range.x_)
                {
                    check = GetCell(x + incX * xr, yr) < MapFeatureType::NoRender;
                    xr++;
                }
            }
            if (!check)
            {
                x += incX * xr;
                continue;
            }

            // Check if the cells in the specified range match the requirement (block or hole)
            xr = 0;
            if (requirement.rangeIsSolid_)
            {
                while (check && xr < range.x_)
                {
                    yr = rangestart;
                    while (check && yr != rangeend)
                    {
                        check = GetCell(x + incX * xr, start.y_ + incY * yr) > MapFeatureType::NoRender;
                        yr += incY;
                    }

                    xr++;

                    if (!check)
                        break;
                }
            }
            else
            {
                while (check && xr < range.x_)
                {
                    yr = rangestart;
                    while (check && yr != rangeend)
                    {
                        check = GetCell(x + incX * xr, start.y_ + incY * yr) < MapFeatureType::NoRender;
                        yr += incY;
                    }

                    xr++;

                    if (!check)
                        break;
                }
            }
            if (check)
                break;

            x += incX * xr;
        }

        // centered in the range
        if (check)
            resultx = x + incX * range.x_/2;
    }

    return check;
}

bool MapGeneratorDungeon::GetCellCoordY(const FurnitureRequirement& requirement, const IntVector2& start, const IntVector2& range, int yEnd, const int incX, int& resulty) const
{
    bool check = false;

    // check only at start.y
    if (!range.y_)
    {
        check = requirement.supportIsSolid_ ? GetCell(start.x_ - incX, start.y_) > MapFeatureType::NoRender : GetCell(start.x_ - incX, start.y_) < MapFeatureType::NoRender;

        if (check)
        {
            const int rangestart = incX >= 0 ? 0 : range.x_;
            const int rangeend = incX >= 0 ? range.x_ : 0;

            int xr = rangestart;
            if (requirement.rangeIsSolid_)
            {
                while (check && xr != rangeend)
                {
                    check = GetCell(start.x_ + incX * xr, start.y_) > MapFeatureType::NoRender;
                    xr += incX;
                }
            }
            else
            {
                while (check && xr != rangeend)
                {
                    check = GetCell(start.x_ + incX * xr, start.y_) < MapFeatureType::NoRender;
                    xr += incX;
                }
            }
        }

        if (check)
            resulty = start.y_;
    }
    // check from start.y to yEnd
    else
    {
        const int numsupportcells = Min(requirement.numSupportCells_, range.y_);

        const int rangestart = incX >= 0 ? 0 : range.x_;
        const int rangeend = incX >= 0 ? range.x_ : 0;

        const int incY = yEnd - start.y_ >= 0 ? 1 : -1;
        const int miny = incY > 0 ? start.y_ : yEnd - incY * (range.y_ - 1);
        const int maxy = incY > 0 ? yEnd - incY * (range.y_ - 1) : start.y_;

        int xr, yr;
        int y = start.y_;

//        URHO3D_LOGERRORF("start=%d %d yEnd=%d range=%d %d miny=%d maxy=%d incX=%d incY=%d numsupportcells=%d !",
//						start.x_, start.y_, yEnd, range.x_, range.y_, miny, maxy, incX, incY, numsupportcells);

        while (y >= miny && y <= maxy)
        {
            check = true;

            // Check if the vertical carrier matches the requirement (block or hole)
            yr = 0;
            xr = start.x_ - incX;
            if (requirement.supportIsSolid_)
            {
                while (check && yr < numsupportcells)
                {
                    check = GetCell(xr, y + incY * yr) > MapFeatureType::NoRender;
                    yr++;
                }
            }
            else
            {
                while (check && yr < numsupportcells)
                {
                    check = GetCell(xr, y + incY * yr) < MapFeatureType::NoRender;
                    yr++;
                }
                // need to find a solid support under the empty support (entrance)
                if (check)
                    check = GetCell(xr, y + incY * yr) > MapFeatureType::NoRender;
            }

            if (!check)
            {
//				URHO3D_LOGERRORF("Can't find a support at y=%d incY=%d!", y, incY);
                y += incY * yr;
                continue;
            }

            y -= incY * requirement.supportOffsetFromRange_;

            // Check if the cells in the specified range match the requirement (block or hole)
            yr = 0;
            if (requirement.rangeIsSolid_)
            {
                while (check && yr < range.y_)
                {
                    xr = rangestart;
                    while (check && xr != rangeend)
                    {
                        check = GetCell(start.x_ + incX * xr, y + incY * yr) > MapFeatureType::NoRender;
//						if (!check)
//							URHO3D_LOGERRORF("range solid breaks at %d %d incX=%d incY=%d !", start.x_ + incX * xr, y + incY * yr, incX, incY);
                        xr += incX;
                    }

                    yr++;

                    if (!check)
                    {
//						URHO3D_LOGERRORF("range solid breaks !");
                        break;
                    }
                }
            }
            else
            {
                while (check && yr < range.y_)
                {
                    xr = rangestart;
                    while (check && xr != rangeend)
                    {
                        check = GetCell(start.x_ + incX * xr, y + incY * yr) < MapFeatureType::NoRender;
//						if (!check)
//							URHO3D_LOGERRORF("range empty breaks at %d %d incX=%d incY=%d !", start.x_ + incX * xr, y + incY * yr, incX, incY);
                        xr += incX;
                    }

                    yr++;

                    if (!check)
                    {
//						URHO3D_LOGERRORF("range empty breaks !");
                        break;
                    }
                }

                if (check)
                {
//					URHO3D_LOGERRORF("range empty breaks OK !");
                    break;
                }
            }

            if (check)
                break;

            y += incY * (yr + requirement.supportOffsetFromRange_);

//            URHO3D_LOGERRORF("Change y=%d incY=%d!", y, incY);
        }

//        URHO3D_LOGERRORF("y=%d check=%s !", y, check ? "true":"false");

        if (check)
            resulty = y + incY * (range.y_/2 - 1);
    }

    return check;
}

bool MapGeneratorDungeon::GetPositionFor(const FurnitureRequirement& requirement, const IntVector2& start, const IntVector2& end, const IntVector2& range, IntVector2& result) const
{
    // Horizontal Alignements
    if (requirement.align_ < OnWall)
    {
        // Test if enough space
        if (range.x_ && Abs(start.x_-end.x_) < range.x_)
            return false;

        if (start.x_ <= 0 || end.x_ >= xSize_-1 || start.y_ - range.y_ < 0 || start.y_ >= ySize_-1)
            return false;

        int x;
        const int incY = requirement.align_ == OnRoof ? 1 : -1;
        if (GetCellCoordX(requirement, start, range, end.x_, incY, x))
        {
            result.x_ = x;

            // Correct the starty if is required empty cell.
            if (!requirement.rangeIsSolid_)
                if (GetCell(x, start.y_) > MapFeatureType::NoRender)
                    result.y_ = start.y_ + incY;

            return true;
        }
    }
    // Vertical Alignements
    else
    {
        if (range.y_ && Abs(start.y_-end.y_) < range.y_)
            return false;

        int y;
        if (GetCellCoordY(requirement, start, range, end.y_, 1, y))
        {
            result.y_ = y;

            if (!requirement.supportIsSolid_)
                result.x_--;

            return true;
        }
    }

    return false;
}


void MapGeneratorDungeon::GetAdjacentDirection(int x, int y, FeatureType cellType, IntVector2& direction) const
{
    direction.x_ = 0;
    direction.y_ = 0;

    if (GetCell(x - 1, y) == cellType)
        direction.x_ = -1;
    else if (GetCell(x + 1, y) == cellType)
        direction.x_ = 1;
    if (GetCell(x, y + 1) == cellType)
        direction.y_ = -1;
    else if (GetCell(x, y - 1) == cellType)
        direction.y_ = 1;
}

bool MapGeneratorDungeon::GetRandomSafePlaceInRoom(const IntRect& room, int *x, int *y, int roomshrinkvalue, bool checkInitialPosition)
{
    if (checkInitialPosition)
        if (GetCell(*x, *y) <= MapFeatureType::Threshold && GetCell(*x, Max(*y - 1, 0)) <= MapFeatureType::Threshold)
            return true;

    int shrinkx = roomshrinkvalue;
    int shrinky = roomshrinkvalue;

    if (roomshrinkvalue)
    {
        // ajust the shrink value to the size of the room
        if (room.Width() < 2*roomshrinkvalue)
            shrinkx = (room.Width() - 1)/2;
        if (room.Height() < 2*roomshrinkvalue)
            shrinky = (room.Height() - 1)/2;
    }

    int i,j;

    int tries = 0;
    int maxTries = 500;

    for ( ; tries != maxTries; ++tries)
    {
        i = Random.Get(room.left_+shrinkx, room.right_-shrinkx);
        j = Random.Get(room.top_+1+shrinky, room.bottom_-shrinky);
        if (GetCell(i,j) <= MapFeatureType::Threshold && GetCell(i,j-1) <= MapFeatureType::Threshold)
        {
            *x = i;
            *y = j;
            return true;
        }
    }

    return false;
}

//int MapGeneratorDungeon::InvertDirection(int direction)
//{
//    if (direction == MapDirection::North)
//        return MapDirection::South;
//    else if (direction == MapDirection::South)
//        return MapDirection::North;
//    else if (direction == MapDirection::East)
//        return MapDirection::West;
//    else if (direction == MapDirection::West)
//        return MapDirection::East;
//}

