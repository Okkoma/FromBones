#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Compression.h>
#include <Urho3D/IO/VectorBuffer.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>

#include "DefsEntityInfo.h"
#include "DefsWorld.h"
#include "DefsMap.h"

#include "GOC_Animator2D.h"
#include "GOC_Destroyer.h"
#include "MapWorld.h"

#include "GameRand.h"
#include "GameAttributes.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "ViewManager.h"

#include "ObjectSkinned.h"
#include "RenderShape.h"

#include "Map.h"


const char* wallTypeNames[] =
{
    "Wall_Ground",
    "Wall_Border",
    "Wall_Roof",
};

const char* mapStatusNames[] =
{
    "Uninitialized=0",
    "Initializing=1",
    "Loading_Map=2",
    "Generating_Map=3",
    "Creating_Map_Layers=4",
    "Creating_Map_Colliders=5",
    "Creating_Map_Entities=6",
    "Setting_Map=7",
    "Setting_Layers=8",
    "Setting_Colliders=9",
    "Updating_ViewBatches=10",
    "Updating_RenderShapes=11",
    "Setting_Areas=12",
    "Setting_BackGround=13",
    "Setting_MiniMap=14",
    "Setting_Furnitures=15",
    "Setting_Entities=16",
    "Setting_Borders=17",
    "Available=18",
    "Unloading_Map=19",
    "Saving_Map=20",
};

const char* mapViewsNames[] =
{
    "FrontView=0",
    "BackGround=1",
    "InnerView=2",
    "BackView=3",
    "OuterView=4"
};

const char* mapGenModeNames[] =
{
    "GENERATED=0",
    "FROMMEMORY=1",
    "FROMBINFILE=2",
    "FROMXMLFILE=3"
};

const char* mapDataSectionNames[] =
{
    "MAPDATASECTION_LAYER=0",
    "MAPDATASECTION_TILEMODIFIER=1",
    "MAPDATASECTION_FLUIDVALUE=2",
    "MAPDATASECTION_SPOT=3",
    "MAPDATASECTION_FURNITURE=4",
    "MAPDATASECTION_ENTITY=5",
    "MAPDATASECTION_ENTITYATTR=6"
};

const char* mapAsynStateNames[] =
{
    "MAPASYNC_NONE=0",
    "MAPASYNC_LOADQUEUED=1",
    "MAPASYNC_SAVEQUEUED=2",
    "MAPASYNC_LOADING=3",
    "MAPASYNC_SAVING=4",
    "MAPASYNC_LOADSUCCESS=5",
    "MAPASYNC_LOADFAIL=6",
    "MAPASYNC_SAVESUCCESS=7",
    "MAPASYNC_SAVEFAIL=8"
};


/// MapInfo
MapInfo MapInfo::info;

const short int MapInfo::neighborOffX[9] = { -1, 0,  1,  1,  1,  0, -1, -1, 0 }; // NW, N, NE, E, SE, S, SW, W, ZERO
const short int MapInfo::neighborOffY[9] = { -1, -1, -1,  0,  1,  1,  1, 0, 0 };

const int MapDirection::sDirectionDx_[4] = { 0, 0, 1, -1 };
const int MapDirection::sDirectionDy_[4] = { -1, 1, 0, 0 };

//const short int MapInfo::neighborCardX[4] = { 0, 1, 0, -1 }; // N, E, S, W
//const short int MapInfo::neighborCardY[4] = { -1, 0, 1, 0 };

void MapInfo::Reset()
{
    if (info.chinfo_)
    {
        delete info.chinfo_;
        info.chinfo_ = 0;
    }
}

void MapInfo::Initialize(int mapwidth, int mapheight, int chunknumx, int chunknumy, float tilewidth, float tileheight, float mwidth, float mheight)
{
    info.chinfo_ = 0;
    if (chunknumx * chunknumy > 0)
        info.chinfo_ = new ChunkInfo(mapwidth, mapheight, chunknumx, chunknumy, tilewidth, tileheight);

    info.width_ = mapwidth;
    info.height_ = mapheight;
    info.mapsize_ = mapwidth * mapheight;

    info.tileWidth_ = tilewidth;
    info.tileHeight_ = tileheight;
    info.tileHalfSize_ = Vector2(tilewidth/2, tileheight/2);

    info.mWidth_ = mwidth;   // scale pris en compte via mapworld::setworldbounds
    info.mHeight_ = mheight; // scale pris en compte via mapworld::setworldbounds
    info.mTileHalfSize_ = Vector2(mwidth/(float)mapwidth * 0.5f, mheight/(float)mapheight * 0.5f);

    // offset indices in CW, start to the middle-left
    for (int i=0; i < 9; ++i)
        info.neighborInd[i] = mapwidth*MapInfo::neighborOffY[i] + MapInfo::neighborOffX[i];

    info.neighbor4Ind[0] = mapwidth*neighborOffY[1]; // Top (N)
    info.neighbor4Ind[1] = neighborOffX[3];        // Right (E)
    info.neighbor4Ind[2] = mapwidth*neighborOffY[5]; // Bottom (S)
    info.neighbor4Ind[3] = neighborOffX[7];        // Left (W)

    info.cornerInd[TopLeftBit]     = mapwidth*neighborOffY[1] + neighborOffX[7];
    info.cornerInd[TopRightBit]    = mapwidth*neighborOffY[1] + neighborOffX[3];
    info.cornerInd[BottomRightBit] = mapwidth*neighborOffY[5] + neighborOffX[3];
    info.cornerInd[BottomLeftBit]  = mapwidth*neighborOffY[5] + neighborOffX[7];
}

const char* NeighborModeStr[] =
{
    "Connected4",
    "Connected8",
    "Connected0"
};

/// MapTerrain Class
//const MapTerrain MapTerrain::EMPTY = MapTerrain(String::EMPTY, Connected4);

MapTerrain::MapTerrain() :
    Random(GameRand::GetRandomizer(TILRAND))
{ ; }

MapTerrain::MapTerrain(const String& name, unsigned property, const Color& color) :
    name_(name),
    property_(property),
    color_(color),
    useDimension_(false),
    Random(GameRand::GetRandomizer(TILRAND))
{ ; }

MapTerrain::MapTerrain(const MapTerrain& t) :
    name_(t.name_), property_(t.property_), color_(t.color_), useDimension_(t.useDimension_), compatibleFeatures_(t.compatibleFeatures_),
    tileGidConnectivity_(t.tileGidConnectivity_), decalGidConnectivity_(t.decalGidConnectivity_),
    tileGidByDimension_(t.tileGidByDimension_), compatibleFurnitures_(t.compatibleFurnitures_),
    Random(GameRand::GetRandomizer(TILRAND))
{ ; }


void MapTerrain::AddConnectIndexToTileGid(int gid, ConnectIndex connectIndex)
{
    while (tileGidConnectivity_.Size() <= connectIndex)
        tileGidConnectivity_.Push(Vector<int>());

    if (!tileGidConnectivity_[connectIndex].Contains(gid))
        tileGidConnectivity_[connectIndex].Push(gid);
}

void MapTerrain::AddConnectIndexToDecalGid(int gid, ConnectIndex connectIndex)
{
    while (decalGidConnectivity_.Size() <= connectIndex)
        decalGidConnectivity_.Push(Vector<int>());

    if (!decalGidConnectivity_[connectIndex].Contains(gid))
        decalGidConnectivity_[connectIndex].Push(gid);
}

void MapTerrain::AddGidToDimension(int gid, int dimension)
{
    dimension -= TILE_RENDER;

    while (tileGidByDimension_.Size() <= dimension)
        tileGidByDimension_.Push(Vector<int>());

    if (!tileGidByDimension_[dimension-1].Contains(gid))
    {
        tileGidByDimension_[dimension-1].Push(gid);
        if (dimension > 1)
            useDimension_ = true;
    }
}

void MapTerrain::AddCompatibleFurniture(int furniture, ConnectIndex connectIndex)
{
    while (compatibleFurnitures_.Size() <= connectIndex)
        compatibleFurnitures_.Push(Vector<int>());

    if (!compatibleFurnitures_[connectIndex].Contains(furniture))
        compatibleFurnitures_[connectIndex].Push(furniture);
}

int MapTerrain::GetRandomTileGidForConnectValue(int connectValue) const
{
    int connectIndex = MapTilesConnectType::GetIndex(connectValue);
#ifdef DUMP_MAPTERRAIN_LOGS
    if (tileGidConnectivity_.Size() <= connectIndex)
    {
        URHO3D_LOGERRORF("MapTerrain() - GetRandomTileGidForConnectValue : tileGidConnectivity_.Size()=%u <= connectIndex=%d !",
                         tileGidConnectivity_.Size(), connectIndex);
        return 0;
    }
#endif
    assert(tileGidConnectivity_.Size() > connectIndex);
    return tileGidConnectivity_[connectIndex][Random.Get(tileGidConnectivity_[connectIndex].Size())];
}

int MapTerrain::GetRandomTileGidForConnectIndex(ConnectIndex connectIndex) const
{
    if (connectIndex == MapTilesConnectType::Void)
        return 0;

#ifdef DUMP_MAPTERRAIN_LOGS
    if (tileGidConnectivity_.Size() <= connectIndex)
    {
        URHO3D_LOGERRORF("MapTerrain() - GetRandomTileGidForConnectIndex : this=%s tileGidConnectivity_.Size()=%u <= connectIndex=%d !",
                         name_.CString(), tileGidConnectivity_.Size(), connectIndex);
        return 0;
    }
    if (tileGidConnectivity_[connectIndex].Size() == 0)
    {
        URHO3D_LOGERRORF("MapTerrain() - GetRandomTileGidForConnectIndex : this=%s tileGidConnectivity_[%d].Size() = 0 !", name_.CString(), connectIndex);
        return 0;
    }
#endif
    assert(tileGidConnectivity_.Size() > connectIndex && tileGidConnectivity_[connectIndex].Size());
    return tileGidConnectivity_[connectIndex][Random.Get(tileGidConnectivity_[connectIndex].Size())];
}

int MapTerrain::GetRandomDecalGidForConnectIndex(ConnectIndex connectIndex) const
{
    if (connectIndex == MapTilesConnectType::Void)
        return 0;

#ifdef DUMP_MAPTERRAIN_LOGS
    if (decalGidConnectivity_.Size() <= connectIndex)
    {
        URHO3D_LOGERRORF("MapTerrain() - GetRandomDecalGidForConnectIndex : decalGidConnectivity_.Size()=%u <= connectIndex=%d !",
                         decalGidConnectivity_.Size(), connectIndex);
        return 0;
    }
#endif
    assert(decalGidConnectivity_.Size() > connectIndex);
    return decalGidConnectivity_[connectIndex][Random.Get(decalGidConnectivity_[connectIndex].Size())];
}

int MapTerrain::GetRandomTileGidForDimension(int dimension) const
{
    if (dimension < TILE_RENDER)
        return 0;

    if (!UseDimensionTiles())
        dimension = TILE_1X1;

    if (dimension > TILE_1X1)
    {
        int r = Random.Get(100);
        if (r < 70)
            dimension = TILE_1X1;
        else if (dimension == TILE_2X2)
        {
            if (r < 83)
                dimension = TILE_1X2;
            else if (r < 95)
                dimension = TILE_2X1;
        }
    }

    dimension -= TILE_1X1;

//    if (dimension < 0) return 0;

#ifdef DUMP_MAPTERRAIN_LOGS
    if (tileGidByDimension_.Size() <= dimension)
    {
        URHO3D_LOGERRORF("MapTerrain() - GetRandomTileGidForDimension : name=%s property=%u ptr=%u tileGidByDimension_.Size()=%u <= dimension=%d !",
                         name_.CString(), property_, this, tileGidByDimension_.Size(), dimension);
        return 0;
    }

    if (!tileGidByDimension_[dimension].Size())
    {
        URHO3D_LOGERRORF("MapTerrain() - GetRandomTileGidForDimension : name=%s property=%u ptr=%u tileGidByDimension_[%d].Size()=%u !",
                         name_.CString(), property_, this, dimension, tileGidByDimension_[dimension].Size());
        return 0;
    }
#endif

    return tileGidByDimension_[dimension][Random.Get(tileGidByDimension_[dimension].Size())];
}

int MapTerrain::GetRandomFurniture(ConnectIndex connectIndex) const
{
    return compatibleFurnitures_[connectIndex].Size() ? compatibleFurnitures_[connectIndex][Random.Get(compatibleFurnitures_[connectIndex].Size())] : -1;
}

void MapTerrain::Dump() const
{
    URHO3D_LOGINFOF("Dump Terrain : name=%s property=%u ptr=%u tileGidConnectivity_=%u decalGidConnectivity_=%u dimension=%u compatibleFeatures_=%u",
                    name_.CString(), property_, this, tileGidConnectivity_.Size(), decalGidConnectivity_.Size(), tileGidByDimension_.Size(), compatibleFeatures_.Size());

#ifdef DUMP_MAPTERRAIN_LOGS
    for (unsigned i=0; i<compatibleFeatures_.Size(); i++)
        URHO3D_LOGINFOF("=> Features[%d] = %s(%d)", i, MapFeatureType::GetName(compatibleFeatures_[i]), compatibleFeatures_[i]);

    for (unsigned i=0; i<tileGidConnectivity_.Size(); i++)
        for (unsigned j=0; j<tileGidConnectivity_[i].Size(); j++)
            URHO3D_LOGINFOF("=> tileconnect[%u][%u]=%s => gid=%d", i, j, MapTilesConnectType::GetName(i), tileGidConnectivity_[i][j]);

    for (unsigned i=0; i<decalGidConnectivity_.Size(); i++)
        for (unsigned j=0; j<decalGidConnectivity_[i].Size(); j++)
            URHO3D_LOGINFOF("=> decalconnect[%u][%u]=%s => gid=%d", i, j, MapTilesConnectType::GetName(i), decalGidConnectivity_[i][j]);

    for (unsigned i=0; i<tileGidByDimension_.Size(); i++)
        for (unsigned j=0; j<tileGidByDimension_[i].Size(); j++)
            URHO3D_LOGINFOF("=> dimension[%u][%u] => gid=%d", i, j, tileGidByDimension_[i][j]);

    for (unsigned i=0; i<compatibleFurnitures_.Size(); i++)
        for (unsigned j=0; j<compatibleFurnitures_[i].Size(); j++)
            URHO3D_LOGINFOF("=> Furnitures[%u][%u] = %s(%d)", i, j, MapFurnitureType::GetName(compatibleFurnitures_[i][j]), compatibleFurnitures_[i][j]);
#endif
}


/// MapGeneratorStatus

void MapGeneratorStatus::ResetCounters(int startindex)
{
//    URHO3D_LOGINFOF("MapGeneratorStatus() - ResetCounters : map=%s startindex=%d ...", map_ ? map_->GetMapGeneratorStatus().mappoint_.CString() : 0, startindex);
    for (int i=startindex; i < 8; i++)
        mapcount_[i] = 0;
}

void MapGeneratorStatus::Clear()
{
//    URHO3D_LOGERRORF("MapGeneratorStatus() - Clear : map=%s ...", map_ ? map_->GetMapGeneratorStatus().mappoint_.CString() : 0);

    status_ = Uninitialized;

    ResetCounters();

    genparams_.Clear();
    genparamsf_.Clear();
    genFront_ = genInner_ = 0;
    genFrontFunc_ = genBackFunc_ = genInnerFunc_ = 0;
    model_ = 0;

    viewZindexes_.Resize(MAXMAPTYPES);
    features_.Resize(MAXMAPTYPES);
    for (unsigned i=0; i < MAXMAPTYPES; i++)
    {
        viewZindexes_[i] = -1;
        features_[i] = 0;
    }

    activeslot_ = 0;
    currentMap_ = 0;
    time_ = 0;
}

void MapGeneratorStatus::Dump() const
{
    String vString, fString;

    for (unsigned i=0; i < MAXMAPTYPES; i++)
    {
        vString += String(viewZindexes_[i]) + " ";
        fString += String((void*)features_[i]) + " ";
    }

    URHO3D_LOGINFOF("MapGeneratorStatus() - Dump : map=%s viewZindexes_=%s features_=%s ...",
                    mappoint_.ToString().CString(), vString.CString(), fString.CString());
}

String MapGeneratorStatus::DumpMapCounters() const
{
    String s;
    for (int i=0; i < 8; i++)
        s += String(mapcount_[i]) + " ";
    return s;
}


/// MapTopography

MapTopography::MapTopography() :
    generatedBackCurves_(false),
    worldEllipseVisible_(false),
    worldEllipseOver_(false)
{ }

void MapTopography::Clear()
{
//    URHO3D_LOGINFOF("MapTopography() - Clear ...");
    generatedView_ = -1;

    numGroundTiles_ = 0;
    groundDensity_ = 0.f;
    for (unsigned i=0; i < 4; i++)
        freeSideTiles_[i].Clear();

    fullSky_ = fullGround_ = false;
    generatedBackCurves_ = false;
    worldEllipseVisible_ = worldEllipseOver_ = false;

    floorTileY_.Clear();
    ellipseY_.Clear();
    ellipseTileY_.Clear();
}

#define NUMTILESUNDERFLOOR 1

void MapTopography::Generate(MapGeneratorStatus& genStatus)
{
    const int viewZ = BACKGROUND;

    const int viewid = genStatus.map_->GetViewId(viewZ);
    if (viewid < 0)
    {
        URHO3D_LOGERRORF("MapTopography() - Generate : viewid < 0 !");
        return;
    }

    URHO3D_LOGINFOF("MapTopography() - Generate :  ... map=%s viewZ=%d viewid=%d", genStatus.mappoint_.ToString().CString(), viewZ, viewid);

    numGroundTiles_ = 0;
    unsigned addr = 0;

    const FeaturedMap& featuredMap = genStatus.map_->GetFeatureView(viewid);

    /// Set Ground Informations

    for (unsigned y=0; y < MapInfo::info.height_; ++y)
    {
        for (unsigned x=0; x < MapInfo::info.width_; ++x, ++addr)
        {
            if (featuredMap[addr] < MapFeatureType::Threshold)
                continue;

            numGroundTiles_++;
        }
    }

    groundDensity_ = (float)numGroundTiles_ / (float)(MapInfo::info.height_ * MapInfo::info.width_);
    fullSky_ = !numGroundTiles_;
    fullGround_ = (numGroundTiles_ == MapInfo::info.width_ * MapInfo::info.height_);

    URHO3D_LOGINFOF("MapTopography() - Generate : map=%s numGroundTiles_=%u fullSky_=%s fullGround_=%s groundDensity_=%f",
                    genStatus.mappoint_.ToString().CString(), numGroundTiles_, fullSky_ ? "true":"false", fullGround_ ? "true":"false", groundDensity_);

    if (generatedBackCurves_)
        return;

    /// Generate Background Curves

    generatedBackCurves_ = true;

    URHO3D_LOGINFOF("MapTopography() - Generate : generate back curves ... width=%d", MapInfo::info.width_);

    // generate floor Y
    floorTileY_.Resize(MapInfo::info.width_);
    if (fullGround_)
    {
        memset(&floorTileY_.Front(), MapInfo::info.height_, sizeof(int) * MapInfo::info.width_);
    }
    else
    {
        memset(&floorTileY_.Front(), -1, sizeof(int) * MapInfo::info.width_);

        int y;
        const FeaturedMap& featuremap = genStatus.map_->GetFeatureView(viewid);
        for (unsigned x=0; x < MapInfo::info.width_; ++x)
        {
            y = MapInfo::info.height_-1;

            while (y >= 0 && featuremap[y*MapInfo::info.width_+x] == MapFeatureType::NoMapFeature)
                y--;

            if (y == -1)
                continue;

            while (y >= 0 && featuremap[y*MapInfo::info.width_+x] != MapFeatureType::NoMapFeature)
                y--;

            if (y == -1)
            {
                floorTileY_[x] = MapInfo::info.height_;
                continue;
            }

            floorTileY_[x] = MapInfo::info.height_ - 1 - y;
        }
    }

#ifdef USE_TOPOGRAPHY_BACKCURVEFLOOR
    // generate back curve

    // get minimal and maximal y points (y=-1.f & y=mHeight)
    Vector<int> xforminy;
    Vector<int> xformaxy;
    int xi = 0, yi = 0;
    Vector<Vector2> splineknots;
    Vector2 knot;

    while (xi < MapInfo::info.width_)
    {
        yi = floorTileY_[xi];

        if (yi == -1)
        {
            xforminy.Push(xi);
            while (++xi < MapInfo::info.width_ && floorTileY_[xi] == -1) ;
            xforminy.Push(xi-1);
            continue;
        }

        if (yi == MapInfo::info.height_-1)
        {
            xformaxy.Push(xi);
            while (++xi < MapInfo::info.width_ && floorTileY_[xi] == MapInfo::info.height_-1) ;
            xformaxy.Push(xi-1);
            continue;
        }

        xi++;
    }

    // fill splineknots
    int ixmin = 0, ixmax = 0, numknots, nextxi, distance;
    float startx, distancebetweenknots;
    float tilewidth = MapInfo::info.mWidth_ / MapInfo::info.width_;
    xi = 0;

    while (xi < MapInfo::info.width_)
    {
        // bottom sequence : 1 or 2 knots
        if (xforminy.Size() >= ixmin+2 && xforminy[ixmin] == xi)
        {
            knot.x_ = MapInfo::info.mWidth_ * ((float)xforminy[ixmin]/(float)(MapInfo::info.width_));
            knot.y_ = -1.f;
            splineknots.Push(knot);

            if (xforminy[ixmin] != xforminy[ixmin+1])
            {
                knot.x_ = MapInfo::info.mWidth_ * ((float)xforminy[ixmin+1]/(float)(MapInfo::info.width_));
                knot.y_ = -1.f;
                splineknots.Push(knot);
            }

            URHO3D_LOGINFOF("MapTopography() - Generate : backcurve bottom sequence push xi=%d to %d ...", xforminy[ixmin], xforminy[ixmin+1]);

            xi = xforminy[ixmin+1]+1;
            ixmin += 2;

            continue;
        }

        // top sequence : 1 or 2 knots
        if (xformaxy.Size() >= ixmax+2 && xformaxy[ixmax] == xi)
        {
            knot.x_ = MapInfo::info.mWidth_ * ((float)xformaxy[ixmax]/(float)(MapInfo::info.width_));
            knot.y_ = MapInfo::info.mHeight_;
            splineknots.Push(knot);

            if (xformaxy[ixmax] != xformaxy[ixmax+1])
            {
                knot.x_ = MapInfo::info.mWidth_ * ((float)xformaxy[ixmax+1]/(float)(MapInfo::info.width_));
                knot.y_ = MapInfo::info.mHeight_;
                splineknots.Push(knot);
            }

            URHO3D_LOGINFOF("MapTopography() - Generate : backcurve top sequence push xi=%d to %d ...", xformaxy[ixmax], xformaxy[ixmax+1]);

            xi = xformaxy[ixmax+1]+1;
            ixmax += 2;

            continue;
        }

        // medium sequence : numknots knots

        // get distance between xi and the next sequence
        nextxi = Min(ixmin < xforminy.Size() ? xforminy[ixmin]-1 : MapInfo::info.width_-1, ixmax < xformaxy.Size() ? xformaxy[ixmax]-1 : MapInfo::info.width_-1);
        distance = nextxi - xi;
        // ratio 25% of knots, minimal 2
        numknots = Max(distance / 4, 1) + 1;
        distancebetweenknots = (numknots > 2 ? (float)distance / (numknots-1) : distance) * tilewidth;
        startx = (xi + 0.5f) * tilewidth;

        URHO3D_LOGINFOF("MapTopography() - Generate : backcurve medium sequence push numknots=%d from xi=%d to %d ...", numknots, xi, nextxi);

        for (int i=0; i < numknots; i++)
        {
            knot.x_ = startx + i * distancebetweenknots;
            knot.y_ = GetFloorY(knot.x_);
            splineknots.Push(knot);
        }

        xi = nextxi+1;
    }

    URHO3D_LOGINFOF("MapTopography() - Generate : backcurve numknots=%u ...", splineknots.Size());

    backFloorCurve_.SetInterpolationMode(CATMULL_ROM_FULL_CURVE);
    //backFloorCurve_.SetInterpolationMode(LINEAR_CURVE);
    backFloorCurve_.SetKnots(splineknots);
#endif
}

void MapTopography::GenerateFreeSideTiles(MapGeneratorStatus& genStatus, int viewZ)
{
    if (generatedView_ == viewZ)
        return;

    // clear free side tiles
    for (unsigned i=0; i < 4; i++)
        freeSideTiles_[i].Clear();

    int viewid = genStatus.map_->GetViewId(viewZ);
    if (viewid < 0)
    {
        URHO3D_LOGERRORF("MapTopography() - GenerateFreeSideTiles : viewid < 0 !");
        generatedView_ = -1;
        return;
    }

    generatedView_ = viewZ;

    URHO3D_LOGINFOF("MapTopography() - GenerateFreeSideTiles :  ... map=%s viewZ=%d viewid=%d", genStatus.mappoint_.ToString().CString(), viewZ, viewid);

    unsigned addr = 0;
    ConnectIndex connectindex;

    const ConnectedMap& connectedmap = genStatus.map_->GetConnectedView(viewid);

    if (viewZ == INNERVIEW)
    {
        for (unsigned y=0; y < MapInfo::info.height_; ++y)
        {
            for (unsigned x=0; x < MapInfo::info.width_; ++x, ++addr)
            {
                // only allow y under floor
                if (!fullGround_ && y-NUMTILESUNDERFLOOR <= MapInfo::info.height_ - floorTileY_[x])
                    continue;

                connectindex = connectedmap[addr];
                if (connectindex == MapTilesConnectType::Void)
                    continue;

                if (connectindex < MapTilesConnectType::AllConnect)
                {
                    // skip borders (because not connected at this moment with other maps)
                    if (y > 0 && (connectindex & TopSide) == 0)
                        freeSideTiles_[0].Push(addr);
                    if (y < MapInfo::info.height_-1 && (connectindex & BottomSide) == 0)
                        freeSideTiles_[1].Push(addr);
                    if (x < MapInfo::info.width_-1 && (connectindex & RightSide) == 0)
                        freeSideTiles_[2].Push(addr);
                    if (x > 0 && (connectindex & LeftSide) == 0)
                        freeSideTiles_[3].Push(addr);
                }
            }
        }
    }
    else
    {
        // Get the number of GroundTiles and Set freeSideTiles
        for (unsigned y=0; y < MapInfo::info.height_; ++y)
        {
            for (unsigned x=0; x < MapInfo::info.width_; ++x, ++addr)
            {
                connectindex = connectedmap[addr];
                if (connectindex == MapTilesConnectType::Void)
                    continue;

                if (connectindex < MapTilesConnectType::AllConnect)
                {
                    // skip borders (because not connected at this moment with other maps)
                    if (y > 0 && (connectindex & TopSide) == 0)
                        freeSideTiles_[0].Push(addr);
                    if (y < MapInfo::info.height_-1 && (connectindex & BottomSide) == 0)
                        freeSideTiles_[1].Push(addr);
                    if (x < MapInfo::info.width_-1 && (connectindex & RightSide) == 0)
                        freeSideTiles_[2].Push(addr);
                    if (x > 0 && (connectindex & LeftSide) == 0)
                        freeSideTiles_[3].Push(addr);
                }
            }
        }
    }
}

void MapTopography::GenerateWorldEllipseInfos(MapGeneratorStatus& genStatus)
{
    // generate world ellipse tileposition for the map
    if (!genStatus.map_->GetRootNode())
        return;

    maporigin_ = genStatus.map_->GetRootNode()->GetWorldPosition2D();
    flatmode_ = Map::info_->worldGround_.radius_.x_ == 0.f;

    // flat world, no ellipse
    if (flatmode_)
        return;

//    URHO3D_LOGERRORF("MapTopography() - GenerateWorldEllipseInfos");

    int width = MapInfo::info.width_;
    int height = MapInfo::info.height_;
    IntVector2 mPosition;
    float x = maporigin_.x_ + 0.5f * Map::info_->mTileWidth_;
    float y;

    const EllipseW& ellipse = Map::info_->worldGround_;
    ellipseY_.Resize(width);
    ellipseTileY_.Resize(width);

    for (unsigned xi = 0; xi < width; xi++)
    {
        y = ellipse.GetY(x);
        ellipseY_[xi] = y;

        Map::info_->Convert2WorldMapPosition(genStatus.mappoint_, x, y, mPosition);
        ellipseTileY_[xi] = mPosition.y_;

        x += Map::info_->mTileWidth_;
    }

    worldEllipseVisible_ = (ellipse.center_.y_ < maporigin_.y_) && (ellipseTileY_[0] < height || ellipseTileY_[width/2] < height || ellipseTileY_[width-1] < height);
    worldEllipseOver_ = (ellipseTileY_[0] < 0 && ellipseTileY_[width/2] < 0 && ellipseTileY_[width-1] < 0);
}

int MapTopography::GetFloorTileY(int tilex) const
{
    return floorTileY_[tilex];
}

int MapTopography::GetEllipseTileY(int tilex) const
{
    return ellipseTileY_[tilex];
}

float MapTopography::GetFloorY(float x) const
{
    if (!floorTileY_.Size())
        return -1.f;

    x = floor(x * (float)MapInfo::info.width_ / MapInfo::info.mWidth_);
    int xtile = Urho3D::Clamp((int)x, 0, MapInfo::info.width_-1);

    assert(xtile < floorTileY_.Size());

    int floortiley = floorTileY_[xtile];
    if (floortiley == -1)
        return -1.f;
    if (floortiley == MapInfo::info.height_-1)
        return MapInfo::info.mHeight_;

    return (float)(floortiley) / (float)(MapInfo::info.height_) * MapInfo::info.mHeight_;
}

#ifdef USE_TOPOGRAPHY_BACKCURVEFLOOR
Vector2 MapTopography::GetBackFloorPoint(float t) const
{
    return backFloorCurve_.GetPoint(t);
}

float MapTopography::GetBackFloorNearestYAt(float x) const
{
    float t = x / MapInfo::info.mWidth_;
    Vector2 point = backFloorCurve_.GetPoint(t);

//    URHO3D_LOGINFOF("MapTopography() - GetBackFloorNearestYAt() : entry x=%F => result x=%F y=%F !", x, point.x_, point.y_);

    return point.y_;
}
#endif

float MapTopography::GetEllipseY(int tilex) const
{
    return ellipseY_[tilex];
}

float MapTopography::GetEllipseY(float x) const
{
    x = floor(x * (float)MapInfo::info.width_ / MapInfo::info.mWidth_);
    return ellipseY_[Urho3D::Clamp((int)x, 0, MapInfo::info.width_-1)];
}

float MapTopography::GetY(float x) const
{
#ifdef USE_TOPOGRAPHY_BACKCURVEFLOOR
    return flatmode_ ? GetFloorY(x-maporigin_.x_) : GetBackFloorNearestYAt(x-maporigin_.x_);
#else
    return GetFloorY(x-maporigin_.x_);
#endif
}


/// MapDirection

const char* MapDirection::names_[7] = { "North", "South", "East", "West", "NoBorders", "AllBorders", "All" };


/// MapSpot

const char* MapSpotTypeString[] =
{
    "NONE",
    "ROOM",
    "TUNN",
    "CAVE",
    "LAND",
    "LIQU",
    "STAR",
    "EXIT",
    "BOSS"
};

String MapSpot::GetName(int index) const
{
    return String(MapSpotTypeString[type_]) + "_" + String(index) + "_" + String(position_.x_) + ":" + String(position_.y_) + ":" + String(ViewManager::Get()->GetViewZName(viewZIndex_).At(0));
}

void MapSpot::GetSpotsOfType(MapSpotType type, const PODVector<MapSpot>& spots, PODVector<const MapSpot*>& typedspots)
{
    typedspots.Clear();

    if (spots.Size())
    {
        unsigned size = spots.Size();
        for (unsigned i=0; i < size; i++)
            if (spots[i].type_ == type)
                typedspots.Push(&spots[i]);
    }
}

int MapSpot::GetNumSpotsOfType(const PODVector<MapSpot>& spots, MapSpotType type)
{
    int num=0;
    unsigned size = spots.Size();

    if (size)
    {
        for (unsigned i=0; i < size; i++)
            if (spots[i].type_ == type)
                num++;
    }

    return num;
}

bool MapSpot::HasSpotOfType(const PODVector<MapSpot>& spots, MapSpotType type)
{
    unsigned i=0;
    unsigned size = spots.Size();

    if (size)
    {
        while (i < size && spots[i].type_ != type)
            i++;
        return (i < size);
    }

    return false;
}

unsigned MapSpot::GetRandomEntityForSpotType(const MapSpot& spot, const Vector<StringHash>& authorizedCategories)
{
#ifdef HANDLE_ENTITIES
    StringHash got;

    GameRand& ORand = GameRand::GetRandomizer(OBJRAND);

    int rand1 = ORand.Get(100);
    int rand2 = ORand.Get(100);
//    URHO3D_LOGINFOF("MapSpot() - GetRandomEntityForSpotType() : spottype=%d ... rand=%d ...", (int)spot.type_, rand);

    switch (spot.type_)
    {
    case SPOT_ROOM :

    case SPOT_LAND :
        if (!authorizedCategories.Size())
            got = rand2 < 20 ? (5 * rand2 < 30) ? COT::GetRandomTypeFrom(COT::CONTAINERS, rand1)
                  : COT::GetRandomTypeFrom(COT::ITEMS, rand1)
                  : COT::GetRandomTypeFrom(COT::MONSTERS, rand1);
        else
            got = COT::GetRandomTypeFrom(authorizedCategories[ORand.Get(authorizedCategories.Size())], rand1);
        break;

    case SPOT_LIQUID :
        if (!authorizedCategories.Size())
            got = rand2 < 65 ? rand2 < 5 ? COT::GetRandomTypeFrom(COT::CONTAINERS, rand1)
                  : COT::GetRandomTypeFrom(COT::LIQUIDS, rand1)
                  : COT::GetRandomTypeFrom(COT::MONSTERS, rand1);
        else
            got = COT::GetRandomTypeFrom(authorizedCategories[ORand.Get(authorizedCategories.Size())], rand1);
        break;
    default :
        break;
    }

//    URHO3D_LOGINFOF("MapSpot() - GetRandomEntityForSpotType() : spottype=%d got=%s(%u) rand=%d,%d ... OK !", spot.type_, GOT::GetType(got).CString(), got.Value(), rand1, rand2);

    return got.Value();
#else
    return 0;
#endif // HANDLE_ENTITIES
}



/// MapData


EntityData::EntityData(const StringHash& got, const Vector2& position, int layerZ)
{
    World2DInfo* info = World2D::GetWorldInfo();
    ShortIntVector2 mpoint;
    IntVector2 coords;
    Vector2 positionInTile;

    info->Convert2WorldMapPoint(position, mpoint);
    info->Convert2WorldMapPosition(mpoint, position, coords, positionInTile);
    positionInTile -= Vector2(World2D::GetWorldInfo()->mTileWidth_ * 0.5f, World2D::GetWorldInfo()->mTileHeight_ * 0.5f);

    tileindex_ = (short unsigned)(coords.y_ * info->mapWidth_ + coords.x_);
    gotindex_ = (short unsigned)GOT::GetIndex(got);
    sstype_ = 0;
    drawableprops_ = (unsigned char)(ViewManager::GetLayerZIndex(layerZ));
    SetPositionProps(positionInTile);
}

void EntityData::Set(unsigned short gotindex, short unsigned tileindex, signed char tilepositionx, signed char tilepositiony, unsigned char sstype, int layerZ, bool rotate, bool flipX, bool flipY)
{
    gotindex_      = gotindex;
    tileindex_     = tileindex;
    tilepositionx_ = tilepositionx;
    tilepositiony_ = tilepositiony;
    sstype_        = sstype;
    drawableprops_ = (unsigned char)(ViewManager::GetLayerZIndex(layerZ) + ((unsigned char)rotate << 5) + ((unsigned char)flipX << 6) + ((unsigned char)flipY << 7));
}

void EntityData::SetDrawableProps(int layerZ, bool rotate, bool flipX, bool flipY)
{
    drawableprops_ = (unsigned char)(ViewManager::GetLayerZIndex(layerZ) + ((unsigned char)rotate << 5) + ((unsigned char)flipX << 6) + ((unsigned char)flipY << 7));
}

void EntityData::SetLayerZ(int layerZ)
{
    drawableprops_ = (drawableprops_ & ~FlagLayerZIndex_);
    drawableprops_ += (ViewManager::GetLayerZIndex(layerZ) & FlagLayerZIndex_);
}

void EntityData::SetPositionProps(const Vector2& positionInTile, ContactSide anchor)
{
    const float halftilewidth = World2D::GetWorldInfo()->mTileWidth_ * 0.5f;
    const float halftileheight = World2D::GetWorldInfo()->mTileHeight_ * 0.5f;

    switch (anchor)
    {
    case NoneSide :
        tilepositionx_ = (signed char)RoundToInt(Clamp(positionInTile.x_, -halftilewidth, halftilewidth) / halftilewidth * 127.f);
        tilepositiony_ = (signed char)RoundToInt(Clamp(positionInTile.y_, -halftileheight, halftileheight) / halftileheight * 127.f);
        break;
    case LeftSide :
        tilepositionx_ = -127;
        tilepositiony_ = (signed char)RoundToInt(Clamp(positionInTile.y_, -halftileheight, halftileheight) / halftileheight * 127.f);
        drawableprops_ = (unsigned char)(drawableprops_|FlagRotate_); // rotate
        break;
    case RightSide :
        tilepositionx_ = 127;
        tilepositiony_ = (signed char)RoundToInt(Clamp(positionInTile.y_, -halftileheight, halftileheight) / halftileheight * 127.f);
        drawableprops_ = (unsigned char)(drawableprops_|FlagRotate_|FlagFlipY_); // rotate + flipy
        break;
    case TopSide :
        tilepositionx_ = (signed char)RoundToInt(Clamp(positionInTile.x_, -halftilewidth, halftilewidth) / halftilewidth * 127.f);
        tilepositiony_ = 127;
        break;
    case BottomSide :
        tilepositionx_ = (signed char)RoundToInt(Clamp(positionInTile.x_, -halftilewidth, halftilewidth) / halftilewidth * 127.f);
        tilepositiony_ = -127;
        drawableprops_ = (unsigned char)(drawableprops_|FlagFlipY_); // flipy
        break;
    default :
        break;
    }

//    URHO3D_LOGINFOF("tilepositionx_=%d tilepositiony_=%d !", tilepositionx_, tilepositiony_);
}

// Return Range -0.5f...0.5f
Vector2 EntityData::GetNormalizedPositionInTile() const
{
    return Vector2((float)(tilepositionx_) / 127.f * 0.5f, (float)(tilepositiony_) / 127.f * 0.5f);
}

String EntityData::Dump() const
{
    String s;
    s = s.AppendWithFormat("ptr=%u tindex=%u tpos=%d %d sstype=%u(etype=%u,isboss=%s)", this, tileindex_, tilepositionx_, tilepositiony_, sstype_, GetEntityValue(), IsBoss() ? "true":"false");
    return s;
}


bool ZoneData::IsInside(const IntVector2& position, int zindex) const
{
    if (zindex != zindex_)
        return false;

    int w    = GetWidth();
    int h    = GetHeight();
    int left = centerx_ - w/2;
    int top  = centery_ - h/2;

    if (position.x_ < left || position.x_ > left + w || position.y_ <  top || position.y_ > top + h)
        return false;

    return true;
}

int ZoneData::GetNumPlayersInside() const
{
    int num = 0;

    for (int i=0; i < GameContext::Get().MAX_NUMPLAYERS; i++)
    {
        Node* node = GameContext::Get().playerAvatars_[i];
        if (node && node->IsEnabled())
        {
            const WorldMapPosition& position = node->GetComponent<GOC_Destroyer>()->GetWorldMapPosition();
            if (IsInside(position.mPosition_, position.viewZIndex_))
                num++;
        }
    }

    return num;
}


#include <LZ4/lz4.h>
#include <LZ4/lz4hc.h>

MapData::MapData()
{
//    URHO3D_LOGINFO("MapData()");

    Clear();

    furnitures_.Reserve(1000);
    entities_.Reserve(500);
}

MapData::~MapData()
{
//    URHO3D_LOGINFO("~MapData()");
}

void MapData::Clear()
{
    map_ = 0;

    state_ = MAPASYNC_NONE;
    for (int i=0; i < MAPDATASECTION_MAX; i++)
        sectionSet_[i] = false;

    dataSize_ = 0U;
    compresseddataSize_ = 0U;

    seed_ = 0U;
    collidertype_ = 0;
    gentype_ = 0;
    prefab_ = 0;

    maps_.Clear();
    tilesModifiers_.Clear();
    fluidValues_.Clear();
    spots_.Clear();
    furnitures_.Clear();
    entities_.Clear();
    entitiesAttributes_.Clear();

    entityInfos_.Clear();
    freeFurnitureDatas_.Clear();
    freeEntityDatas_.Clear();
}

void MapData::SetMap(MapBase* map)
{
    map_ = map;

    if (map_)
        maps_.Resize(MAP_NUMMAXVIEWS+2);

    SetMapsLinks();

    // when mapData unlinked to a map.
    if (!map_)
        PurgeEntityDatas();

//    URHO3D_LOGINFOF("MapData() - SetMap : ptr=%u mpoint=%s map=%u ... OK !", this, mpoint_.ToString().CString(), map_);
}

void MapData::SetMapsLinks()
{
    if (map_)
    {
        numviews_ = map_->GetObjectSkinned() ? map_->GetObjectSkinned()->GetNumViews() : 0;
        if (!numviews_)
            numviews_ = MAP_NUMMAXVIEWS;

        for (int i=0; i < numviews_; i++)
            maps_[i] = &map_->GetFeaturedMap(i);
        for (int i=numviews_; i < MAP_NUMMAXVIEWS; i++)
            maps_[i] = 0;

        maps_[MAP_NUMMAXVIEWS] = &map_->GetObjectFeatured()->GetTerrainMap();
        maps_[MAP_NUMMAXVIEWS+1] = &map_->GetObjectFeatured()->GetBiomeMap();

//        URHO3D_LOGERRORF("MapData() - SetMapsLinks : ptr=%u mpoint=%s map=%u numviews=%u : ", this, mpoint_.ToString().CString(), map_, numviews_);
//        for (int i=0; i < MAP_NUMMAXVIEWS+2; i++)
//            URHO3D_LOGERRORF(" => maplink[%d]=%u", i, maps_[i]);
    }
    else
    {
        state_ = MAPASYNC_NONE;
        if (!prefab_)
            sectionSet_[MAPDATASECTION_LAYER] = false;
        maps_.Clear();
    }
}

void MapData::SetSection(int sid, bool ok)
{
    sectionSet_[sid] = ok;
//    URHO3D_LOGINFOF("MapData() - SetSection : ptr=%u mpoint=%s map=%u section[%d]=%s... OK !", this, mpoint_.ToString().CString(), map_, sid, ok ? "true":"false");
}


void MapData::CopyPrefabLayer(int i)
{
    if (maps_[i])
    {
        if (i < prefabMaps_.Size() && prefabMaps_[i].Buffer())
        {
            maps_[i]->Resize(width_ * height_);
            memcpy(maps_[i]->Buffer(), prefabMaps_[i].Buffer(), width_ * height_ * sizeof(FeatureType));

//            URHO3D_LOGINFOF("MapData() - CopyPrefabLayer : %d ... OK !", i);
        }
    }
}

void MapData::CopyPrefabLayers()
{
    for (int i=0; i < MAP_NUMMAXVIEWS+2; i++)
        CopyPrefabLayer(i);
}

void MapData::UpdatePrefabLayer(int i)
{
    if (maps_[i])
    {
        if (i >= prefabMaps_.Size())
            prefabMaps_.Resize(i+1);

        if (maps_[i]->Buffer())
        {
            prefabMaps_[i].Resize(width_ * height_);
            memcpy(prefabMaps_[i].Buffer(), maps_[i]->Buffer(), width_ * height_ * sizeof(FeatureType));

//            URHO3D_LOGINFOF("MapData() - UpdatePrefabLayer : %d ... OK !", i);
        }
    }
}


void MapData::PurgeEntityDatas()
{
    if (map_)
        return;

    entityInfos_.Clear();
    freeFurnitureDatas_.Clear();
    freeEntityDatas_.Clear();

    if (furnitures_.Size())
    {
        PODVector<EntityData>::Iterator it = furnitures_.Begin();
        while (it != furnitures_.End())
        {
            if (it->gotindex_ == 0 || it->gotindex_ >= GOT::GetSize())
            {
//                URHO3D_LOGERRORF("MapData() - PurgeEntityDatas : furnitures %u/%u", it-furnitures_.Begin(), furnitures_.Size());
                furnitures_.EraseSwap(it-furnitures_.Begin());
            }
            else
                it++;
        }
    }
    if (entities_.Size())
    {
        PODVector<EntityData>::Iterator it = entities_.Begin();
        while (it != entities_.End())
        {
            if (it->gotindex_ == 0 || it->gotindex_ >= GOT::GetSize())
            {
//                URHO3D_LOGERRORF("MapData() - PurgeEntityDatas : entities %u/%u", it-entities_.Begin(), furnitures_.Size());
                unsigned index = it-entities_.Begin();
                entities_.EraseSwap(index);
            }
            else
                it++;
        }
    }
}

void MapData::AddEntityData(Node* node, EntityData* entitydata, bool forceToSet, bool priorizeEntityData)
{
//    URHO3D_LOGINFOF("MapData() - AddEntityData : node=%s(%u) entitydataptr=%u ...", node->GetName().CString(), node->GetID(), entitydata);
    int type = GOT::GetTypeProperties(entitydata && entitydata->gotindex_ && entitydata->gotindex_ < GOT::GetSize() ? GOT::Get(entitydata->gotindex_) : node->GetVar(GOA::GOT).GetStringHash());
    bool furnituretype = (type & GOT_Furniture);
    PODVector<EntityData>& datas = furnituretype ? furnitures_ : entities_;
    PODVector<EntityData* >& freedatas = furnituretype ? freeFurnitureDatas_ : freeEntityDatas_;

    if (entitydata)
    {
        unsigned index = PODVector<EntityData>::Iterator(entitydata) - datas.Begin();
        // entitydata is not inside storage => Push entitydata in the storage
        if (index >= datas.Size())
        {
            if (freedatas.Size())
            {
//                URHO3D_LOGINFOF("MapData() - AddEntityData : node=%s(%u) copy entitydata inside free entitydata !", node->GetName().CString(), node->GetID());
                *freedatas.Back() = *entitydata;
                entitydata = freedatas.Back();
                freedatas.Pop();
            }
            else
            {
//                URHO3D_LOGINFOF("MapData() - AddEntityData : node=%s(%u) copy entitydata inside a new entitydata !", node->GetName().CString(), node->GetID());
                // add new entitydata and set it
                datas.Resize(datas.Size() + 1);
                datas.Back() = *entitydata;
                entitydata = &datas.Back();
            }
            forceToSet = false;
        }
        else
        {
//            URHO3D_LOGINFOF("MapData() - AddEntityData : node=%s(%u) entitydata already inside storage !", node->GetName().CString(), node->GetID());
        }
    }
    else
    {
        if (freedatas.Size())
        {
//            URHO3D_LOGINFOF("MapData() - AddEntityData : node=%s(%u) free entitydata setted !", node->GetName().CString(), node->GetID());
            entitydata = freedatas.Back();
            freedatas.Pop();
        }
        else
        {
//            URHO3D_LOGINFOF("MapData() - AddEntityData : node=%s(%u) new entitydata setted !", node->GetName().CString(), node->GetID());
            // add new entitydata and set it
            datas.Resize(datas.Size() + 1);
            entitydata = &datas.Back();
        }
        forceToSet = true;
    }

    if (forceToSet)
        SetEntityData(node, entitydata, priorizeEntityData);

    if (node)
        entityInfos_.Insert(Pair<unsigned, EntityData*>(node->GetID(), entitydata));
}

EntityData* MapData::GetEntityData(Node* node)
{
    HashMap<unsigned, EntityData* >::Iterator jt = entityInfos_.Find(node->GetID());
    return jt != entityInfos_.End() ? jt->second_ : 0;
}

int MapData::GetEntityDataID(EntityData* entitydata, bool furnituretype)
{
    PODVector<EntityData>& datas = furnituretype ? furnitures_ : entities_;
    int index = PODVector<EntityData>::Iterator(entitydata) - datas.Begin();
    return index >= 0 && index < datas.Size() ? index : -1;
}

void MapData::RemoveEntityData(EntityData* entitydata, bool furnituretype)
{
    PODVector<EntityData>& datas = furnituretype ? furnitures_ : entities_;
    PODVector<EntityData* >& freedatas = furnituretype ? freeFurnitureDatas_ : freeEntityDatas_;
    unsigned index = PODVector<EntityData>::Iterator(entitydata) - datas.Begin();
    if (index < datas.Size())
    {
//        URHO3D_LOGINFOF("MapData() - RemoveEntityData : remove entitydata !");
        freedatas.Push(entitydata);
        entitydata->Clear();
    }
}

void MapData::RemoveEntityData(Node* node)
{
    HashMap<unsigned, EntityData* >::Iterator jt = entityInfos_.Find(node->GetID());
    if (jt == entityInfos_.End())
        return;

//    URHO3D_LOGERRORF("MapData() - RemoveEntityData : remove link entitydata and node=%s(%u) !", node->GetName().CString(), node->GetID());
    RemoveEntityData(jt->second_, (GOT::GetTypeProperties(node->GetVar(GOA::GOT).GetStringHash()) & GOT_Furniture));

    jt = entityInfos_.Erase(jt);
}

void MapData::RemoveEffectActions()
{
    for (unsigned i=0; i < zones_.Size(); i++)
        EffectAction::Clear(IntVector3(mpoint_.x_, mpoint_.y_, i));
}

void MapData::SetEntityData(Node* node, EntityData* entitydata, bool priorizeEntityData)
{
    // Update Game Object Types

    StringHash got(node->GetVar(GOA::GOT).GetUInt());

    entitydata->gotindex_ = (short unsigned)GOT::GetIndex(got);
    entitydata->sstype_ = (unsigned char)(node->HasComponent<AnimatedSprite2D>() ? node->GetComponent<AnimatedSprite2D>()->GetSpriterEntityIndex()|RandomMappingFlag : RandomEntityFlag|RandomMappingFlag);

    bool setted = false;
    bool onTile = node->GetVars().Contains(GOA::ONTILE);
    if (onTile)
        onTile = node->GetVar(GOA::ONTILE).GetInt() != -1;

//    URHO3D_LOGERRORF("MapData() - SetEntityData : map=%s node=%s(%u) ... ontile=%s", map_ ? map_->GetMapPoint().ToString().CString() : "none", node->GetName().CString(), node->GetID(), onTile ? "true":"false");

    // if ONTILE not defined (new furniture or furniture position edited)
    if (!onTile && (GOT::GetTypeProperties(got) & GOT_Anchored))
    {
        unsigned lasttileindex = USHRT_MAX;
        if (entitydata->tileindex_ != USHRT_MAX)
            lasttileindex = entitydata->tileindex_;
        else
            lasttileindex = map_->GetTileIndexAt(node->GetWorldPosition2D());

        setted = map_->AnchorEntityOnTileAt(*entitydata, priorizeEntityData ? 0 : node);
        if (setted)
            node->SetVar(GOA::ONTILE, entitydata->tileindex_);
        else if (lasttileindex != USHRT_MAX)
            node->SetVar(GOA::ONTILE, lasttileindex);
    }

    if (!priorizeEntityData && !setted)
    {
//        URHO3D_LOGERRORF("MapData() - SetEntityData : map=%s node=%s(%u) ... setted ...", map_ ? map_->GetMapPoint().ToString().CString() : "none", node->GetName().CString(), node->GetID());

        entitydata->tileindex_ = (short unsigned)(onTile ? node->GetVar(GOA::ONTILE).GetUInt() : map_->GetTileIndexAt(node->GetWorldPosition2D()));
        entitydata->SetPositionProps(node->GetWorldPosition2D() - map_->GetWorldTilePosition(map_->GetTileCoords(entitydata->tileindex_)));

        // Update Drawable properties bit0-4=viewz; bit5=rotate; bit6=flipx; bit7=flipy;
        StaticSprite2D* staticsprite = node->GetComponent<StaticSprite2D>();
        GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
        unsigned char rotate = node->GetWorldRotation2D() == 90.f;
        unsigned char flipX = animator ? animator->GetDirection().x_ < 0.f : (staticsprite ? staticsprite->GetFlipX() : 0);
        unsigned char flipY = staticsprite ? staticsprite->GetFlipY() : 0;
        entitydata->drawableprops_ = (unsigned char)(ViewManager::GetLayerZIndex(node->GetVar(GOA::ONVIEWZ).GetInt()) + (rotate << 5) + (flipX << 6) + (flipY << 7));
    }
}

void SetPhysicFlip(CollisionBox2D* shape, bool flipX, bool flipY=false)
{
    if (flipX && flipY)
        shape->SetCenter(-shape->GetCenter());
    else if (flipX)
    {
        shape->SetCenter(-shape->GetCenter().x_, shape->GetCenter().y_);
        if (shape->GetAngle())
            shape->SetAngle(-shape->GetAngle());
    }
    else
        shape->SetCenter(shape->GetCenter().x_, -shape->GetCenter().y_);
}

void SetPhysicFlip(CollisionCircle2D* shape, bool flipX, bool flipY=false)
{
    if (flipX && flipY)
        shape->SetCenter(-shape->GetCenter());
    else if (flipX)
        shape->SetCenter(-shape->GetCenter().x_, shape->GetCenter().y_);
    else
        shape->SetCenter(shape->GetCenter().x_, -shape->GetCenter().y_);
}

void MapData::UpdateEntityNode(Node* node, EntityData* entitydata)
{
    if (!entitydata)
    {
        HashMap<unsigned, EntityData* >::ConstIterator jt = entityInfos_.Find(node->GetID());
        if (jt == entityInfos_.End())
        {
            URHO3D_LOGERRORF("MapData() - UpdateEntityNode : nodeid=%u can't find an EntityData !", node->GetID());
            return;
        }

        entitydata = jt->second_;
    }

    // Update the Position and Orientation
    Vector2 positionintile = entitydata->GetNormalizedPositionInTile();
    node->SetWorldPosition2D(map_->GetWorldTilePosition(map_->GetTileCoords(entitydata->tileindex_), positionintile));
//    URHO3D_LOGINFOF("MapData() - UpdateEntityNode : tileindex=%u position=%s positionintile=%s ...", entitydata->tileindex_, node->GetWorldPosition2D().ToString().CString(), positionintile.ToString().CString());

    if (entitydata->drawableprops_ & FlagRotate_)
        node->SetWorldRotation2D(90.f);

    bool flipX = entitydata->drawableprops_ & FlagFlipX_;
    bool flipY = entitydata->drawableprops_ & FlagFlipY_;

    StaticSprite2D* drawable = node->GetDerivedComponent<StaticSprite2D>();
    if (drawable)
        drawable->SetFlip(flipX, flipY);

    GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
    if (animator)
        animator->SetDirection(Vector2(flipX ? -1.f : 1.f, 0.f));

    // Flip physic center if drawable is flip
    if (flipX || flipY)
    {
        PODVector<CollisionShape2D*> shapes;
        node->GetDerivedComponents<CollisionShape2D>(shapes);

        for (unsigned i=0; i < shapes.Size(); i++)
        {
            if (shapes[i]->IsInstanceOf<CollisionBox2D>())
                SetPhysicFlip(static_cast<CollisionBox2D*>(shapes[i]), flipX, flipY);
            else if (shapes[i]->IsInstanceOf<CollisionCircle2D>())
                SetPhysicFlip(static_cast<CollisionCircle2D*>(shapes[i]), flipX, flipY);
        }
    }

    PurgeEntityDatas();

    // TODO ViewZ
//    int viewZ = ViewManager::GetLayerZ(entitydata->drawableprops_ & FlagLayerZIndex_);
//    node->SetVar(GOA::ONVIEWZ, viewZ);
}


bool MapData::Load(Deserializer& source)
{
    compresseddataSize_ = source.GetSize();

    // Load Map Infos
    mpoint_.x_ = source.ReadInt();
    mpoint_.y_ = source.ReadInt();

    width_ = source.ReadInt();
    height_ = source.ReadInt();
    numviews_ = source.ReadUInt();

    seed_ = source.ReadUInt();
    collidertype_ = source.ReadInt();
    gentype_ = source.ReadInt();
    skinid_ = source.ReadInt();

    prefab_ = source.ReadInt();

    dataSize_ = 10 * sizeof(unsigned);

    if (prefab_)
        prefabMaps_.Resize(MAP_NUMMAXVIEWS+2);

//    URHO3D_LOGINFOF("MapData() - Load : this=%u map=%u ... read mpoint=%s width=%d height=%d seed=%u numviews=%u OK !",
//                     this, map_, mpoint_.ToString().CString(), width_, height_, seed_, numviews_);

    unsigned layersize = width_ * height_ * sizeof(FeatureType);

    // Load Sections : each section is compressed
    while (!source.IsEof())
    {
        int sid = source.ReadInt();
        unsigned num = source.ReadUInt();

        // Get Decompression source/destination sizes

        unsigned decompSize = source.ReadUInt();
        unsigned compSize = source.ReadUInt();

        dataSize_ += 4 * sizeof(unsigned) + decompSize;

        if (!decompSize || !compSize)
            continue;

        URHO3D_LOGINFOF("MapData() - Load : this=%u mpoint=%s map=%u ... read sid=%s(%d) num=%u compSize=%u decompSize=%u  ...",
                        this, mpoint_.ToString().CString(), map_, mapDataSectionNames[sid], sid, num, compSize, decompSize);

        // Illegal source (packed data) size reported, possibly not valid data
        if (compSize > compresseddataSize_)
            return false;

        // Skip Section if over bufferSize or unknown section id
        // Skip if Section is already set
        if (sid >= MAPDATASECTION_MAX || IsSectionSet(sid))
        {
            source.Seek(source.GetPosition() + compSize);
            continue;
        }

        // Check for no map link
        if (sid == MAPDATASECTION_LAYER && !prefab_ && !maps_[num])
        {
            URHO3D_LOGERRORF("MapData() - Load : this=%u mpoint=%s map=%u ... read sid=%s(%d) num=%u no map link set !",
                             this, mpoint_.ToString().CString(), map_, mapDataSectionNames[sid], sid, num);
            return false;
        }

        SharedArrayPtr<unsigned char> srcBuffer(new unsigned char[compSize]);
        SharedArrayPtr<unsigned char> destBuffer(new unsigned char[decompSize]);
        MemoryBuffer buffer(destBuffer.Get(), decompSize);

        // Read File in SrcBuffer
        if (source.Read(srcBuffer, compSize) != compSize)
            return false;

        // Decompress in buffer (buffer is a memorybuffer that use destBuffer)
        LZ4_decompress_fast((const char*)srcBuffer.Get(), (char*)destBuffer.Get(), decompSize);

        // Copy the section in MapData
        buffer.Seek(0);

        if (sid == MAPDATASECTION_LAYER)
        {
            if (num > MAP_NUMMAXVIEWS+1)
                return false;

            FeaturedMap* map = prefab_ ? &prefabMaps_[num] : maps_[num];

            // copy to layer view id
            map->Resize(width_ * height_);
            buffer.Read(map->Buffer(), layersize);

            // copy prefab layer in MapBase
            if (prefab_)
                CopyPrefabLayer(num);

            if (num == MAP_NUMMAXVIEWS+1)
                SetSection(MAPDATASECTION_LAYER, true);
        }
        else
        {
            if (sid == MAPDATASECTION_TILEMODIFIER)
            {
                if (!num)
                    return false;

                // copy to tilemodifier
                tilesModifiers_.Resize(num);
                buffer.Read(&(tilesModifiers_.Front()), num * sizeof(TileModifier));
            }
            else if (sid == MAPDATASECTION_FLUIDVALUE)
            {
                if (!num)
                    return false;

//				URHO3D_LOGERRORF("MapData() - Load : this=%u mpoint=%s map=%u ... read sid=%s(%d) num=%u set MAPDATASECTION_FLUIDVALUE !",
//                            this, mpoint_.ToString().CString(), map_, mapDataSectionNames[sid], sid, num, compSize, decompSize);

                fluidValues_.Resize(num);
                for (unsigned i=0; i < num; i++)
                {
                    PODVector<FeatureType>& fluidValues = fluidValues_[i];
                    fluidValues.Resize(width_ * height_);
                    buffer.Read(&(fluidValues.Front()), layersize);
                }
            }
            else if (sid == MAPDATASECTION_SPOT)
            {
                if (!num)
                    return false;

                // copy to spots
                spots_.Resize(num);
                buffer.Read(&(spots_.Front()), num * sizeof(MapSpot));
            }
            else if (sid == MAPDATASECTION_ZONE)
            {
                if (!num)
                    return false;

                // copy to zones
                zones_.Resize(num);
                buffer.Read(&(zones_.Front()), num * sizeof(ZoneData));
            }
            else if (sid == MAPDATASECTION_FURNITURE)
            {
                if (!num)
                    return false;

                // copy to furnitures
                furnitures_.Resize(num);
                buffer.Read(&(furnitures_.Front()), num * sizeof(EntityData));
            }
            else if (sid == MAPDATASECTION_ENTITY)
            {
                if (!num)
                    return false;

                // copy to entities
                entities_.Resize(num);
                buffer.Read(&(entities_.Front()), num * sizeof(EntityData));
            }
            else if (sid == MAPDATASECTION_ENTITYATTR)
            {
                if (!num)
                    return false;

                // copy to entities attributes
                entitiesAttributes_.Resize(num);
                for (unsigned i=0; i < num; i++)
                {
                    unsigned numcomponents = buffer.ReadVLE();
                    NodeAttributes& entitycomponents = entitiesAttributes_[i];
                    entitycomponents.Resize(numcomponents);
                    for (unsigned j=0; j < numcomponents; j++)
                    {
                        entitycomponents[j] = buffer.ReadVariantMap();
                    }
                }
            }

            SetSection(sid, true);
        }
    }
//#ifdef MAPDATA_SAVEFURNITURELIKEENTITIES
//    // Test : always set Section Furniture (furniture are saved in section EntityAttr)
//    SetSection(MAPDATASECTION_FURNITURE, true);
//#endif
//    Dump();

    return true;
}

bool MapData::Save(Serializer& dest)
{
    // Save Map Infos
    dest.WriteInt(mpoint_.x_);
    dest.WriteInt(mpoint_.y_);
    dest.WriteInt(width_);
    dest.WriteInt(height_);
    dest.WriteUInt(numviews_);
    dest.WriteUInt(seed_);
    dest.WriteInt(collidertype_);
    dest.WriteInt(gentype_);
    dest.WriteInt(skinid_);
    dest.WriteInt(prefab_);

    dataSize_ = 10 * sizeof(unsigned);
    compresseddataSize_ = dataSize_;

    URHO3D_LOGINFOF("MapData() - Save : this=%u map=%u ... write mpoint=%s width=%d height=%d numviews=%u seed=%u ...",
                    this, map_, mpoint_.ToString().CString(), width_, height_, numviews_, seed_);

    unsigned srcSize, destSize, maxDestSize;
    const char* srcBuffer;
    PODVector<char> destBuffer;
    VectorBuffer srcVBuffer;

    // if map is not a prefab skip serialize Layers
    int sid = GameContext::Get().allMapsPrefab_ || prefab_ ? MAPDATASECTION_LAYER : MAPDATASECTION_TILEMODIFIER;
    unsigned num = 0;

    // Save Sections : each section is compressed
    while (sid < MAPDATASECTION_MAX)
    {
        if (sid == MAPDATASECTION_LAYER)
        {
            // When all views(+terrainmap+biomemap) are saved changed to next sid
            // Or When map is not a prefab
            if (num > MAP_NUMMAXVIEWS+1)
            {
                sid++;
                continue;
            }

            srcSize = width_ * height_ * sizeof(FeatureType);
            srcBuffer = maps_[num] && maps_[num]->Size() ? (const char*)maps_[num]->Buffer() : 0;
        }
        else if (sid == MAPDATASECTION_TILEMODIFIER && tilesModifiers_.Size())
        {
            num = tilesModifiers_.Size();
            srcBuffer = (const char*)(&tilesModifiers_.Front());
            srcSize = num * sizeof(TileModifier);
        }
        else if (sid == MAPDATASECTION_FLUIDVALUE && fluidValues_.Size())
        {
            srcVBuffer.Clear();
            num = fluidValues_.Size();

//			URHO3D_LOGERRORF("MapData() - Save : this=%u mpoint=%s map=%u ... write sid=%s(%d) num=%u set !",
//                            this, mpoint_.ToString().CString(), map_, mapDataSectionNames[sid], sid, num);

            for (unsigned i=0; i < num; i++)
            {
                const PODVector<FeatureType>& fluidValues = fluidValues_[i];
                srcVBuffer.Write(fluidValues.Buffer(), fluidValues.Size() * sizeof(FeatureType));
            }
            srcBuffer = (const char*)(srcVBuffer.GetData());
            srcSize = srcVBuffer.GetBuffer().Size();
        }
        else if (sid == MAPDATASECTION_SPOT && spots_.Size())
        {
            num = spots_.Size();
            srcBuffer = (const char*)(&spots_.Front());
            srcSize = num * sizeof(MapSpot);
        }
        else if (sid == MAPDATASECTION_ZONE && zones_.Size())
        {
            num = zones_.Size();
            srcBuffer = (const char*)(&zones_.Front());
            srcSize = num * sizeof(ZoneData);
        }
        else if (sid == MAPDATASECTION_FURNITURE && furnitures_.Size())
        {
#ifdef MAPDATA_SAVEFURNITURELIKEENTITIES
            sid++;
            continue;
#else
            num = 0;
            srcVBuffer.Clear();
            for (unsigned i=0; i < furnitures_.Size(); i++)
            {
                EntityData& furniture = furnitures_[i];
                if (furniture.gotindex_ == 0 || furniture.gotindex_ >= GOT::GetSize())
                    continue;

                srcVBuffer.Write(&furniture, sizeof(EntityData));
                num++;
            }
            srcBuffer = (const char*)(srcVBuffer.GetData());
            srcSize = srcVBuffer.GetBuffer().Size();
#endif
        }
        else if (sid == MAPDATASECTION_ENTITY && entities_.Size())
        {
#ifdef MAPDATA_SAVEFURNITURELIKEENTITIES
            sid++;
            continue;
#else
            num = 0;
            srcVBuffer.Clear();
            for (unsigned i=0; i < entities_.Size(); i++)
            {
                EntityData& entity = entities_[i];
                if (entity.gotindex_ == 0 || entity.gotindex_ >= GOT::GetSize())
                    continue;

                srcVBuffer.Write(&entity, sizeof(EntityData));
                num++;
            }
            srcBuffer = (const char*)(srcVBuffer.GetData());
            srcSize = srcVBuffer.GetBuffer().Size();
#endif
        }
//        else if (sid == MAPDATASECTION_FURNITURE && furnitures_.Size())
//        {
//            num = furnitures_.Size();
//            srcBuffer = (const char*)(&furnitures_.Front());
//            srcSize = num * sizeof(EntityData);
//        }
//        else if (sid == MAPDATASECTION_ENTITY && entities_.Size())
//        {
//            num = entities_.Size();
//            srcBuffer = (const char*)(&entities_.Front());
//            srcSize = num * sizeof(EntityData);
//        }
        else if (sid == MAPDATASECTION_ENTITYATTR && entitiesAttributes_.Size())// && !GameContext::Get().arenaZoneOn_)
        {
            num = entitiesAttributes_.Size();
            srcVBuffer.Clear();
            for (unsigned i=0; i < num; i++)
            {
                NodeAttributes& entitycomponents = entitiesAttributes_[i];
                unsigned numcomponents = entitycomponents.Size();
                srcVBuffer.WriteVLE(numcomponents);
                for (unsigned j=0; j < numcomponents; j++)
                    srcVBuffer.WriteVariantMap(entitycomponents[j]);
            }
            srcBuffer = (const char*)(srcVBuffer.GetData());
            srcSize = srcVBuffer.GetBuffer().Size();
        }
        else
        {
            sid++;
            continue;
        }

        if (srcBuffer)
        {
            // Compression
            maxDestSize = (unsigned)LZ4_compressBound(srcSize);
            destBuffer.Resize(maxDestSize);
            destSize = (unsigned)LZ4_compress_HC(srcBuffer, destBuffer.Buffer(), srcSize, maxDestSize, 0);

            // Save
            bool success = true;
            success &= dest.WriteInt(sid);
            success &= dest.WriteUInt(num);
            success &= dest.WriteUInt(srcSize);
            success &= dest.WriteUInt(destSize);
            success &= dest.Write(destBuffer.Buffer(), destSize) == destSize;

            dataSize_ += 4 * sizeof(unsigned) + srcSize;
            compresseddataSize_ += 4 * sizeof(unsigned) + destSize;

            URHO3D_LOGINFOF("MapData() - Save : this=%u mpoint=%s map=%u ... write sid=%s(%d) num=%u srcsize=%u destsize=%u ...",
                            this, mpoint_.ToString().CString(), map_, mapDataSectionNames[sid], sid, num, srcSize, destSize);

            if (!success)
                return false;
        }

        // Next sid (skip for layer section that incrementes sid when all views are saved)
        if (sid == MAPDATASECTION_LAYER)
        {
            num++;

            // go to terrainmap
            if (num == numviews_)
                num = MAP_NUMMAXVIEWS;
        }
        else
        {
            sid++;
        }
    }

//    Dump();

    return true;
}

void MapData::Dump() const
{
    URHO3D_LOGINFOF("MapData() - Dump : this=%u mpoint=%s map=%u numviews=%u size=%u compressedsize=%u ...", this, mpoint_.ToString().CString(), map_, numviews_, dataSize_, compresseddataSize_);

    for (unsigned i=0; i < numviews_; i++)
        GameHelpers::DumpData(maps_[i]->Buffer(), 1, 2, width_, height_);

    for (unsigned i=MAP_NUMMAXVIEWS; i < MAP_NUMMAXVIEWS+2; i++)
        GameHelpers::DumpData(maps_[i]->Buffer(), 1, 2, width_, height_);

    unsigned i=0;
    URHO3D_LOGINFOF("MapData() - Dump : this=%u mpoint=%s entityInfosSize=%u ...", this, mpoint_.ToString().CString(), entityInfos_.Size());
    for (HashMap<unsigned, EntityData* >::ConstIterator it = entityInfos_.Begin(); it != entityInfos_.End(); ++it, i++)
        URHO3D_LOGINFOF(" entityInfos_[%u] = nodeid=%u entitydata=%s", i, it->first_, it->second_ ? it->second_->Dump().CString() : "null");
}



