#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include "DefsWorld.h"
#include "DefsViews.h"

#include "AnlWorldModel.h"

#include "MapGeneratorWorld.h"


/// MapGeneratorWorld



MapGeneratorWorld::MapGeneratorWorld()
    : MapGenerator("MapGeneratorWorld")
{
//    URHO3D_LOGINFOF("MapGeneratorWorld()");
}

MapGeneratorWorld::~MapGeneratorWorld()
{
    URHO3D_LOGINFOF("~MapGeneratorWorld()");
    worldModel_.Reset();
}

bool MapGeneratorWorld::SetAnlWorldModel(AnlWorldModel* model)
{
    if (!model && !worldModel_)
    {
        URHO3D_LOGWARNINGF("MapGeneratorWorld() - SetAnlWorldModel ... model=%u => worldModel_ Not Set !", model);
        return false;
    }

    worldModel_ = model;

    return true;
}

bool MapGeneratorWorld::Init(MapGeneratorStatus& gS)
{
    URHO3D_LOGINFOF("MapGeneratorWorld() - Init ...");

    if (!gS.model_)
    {
        if (!SetAnlWorldModel(info_->worldModel_))
            return false;

        AnlMappingRange& mr = gS.mappingRange_;
        mr.mapx0 = ((float)gS.x_) * AnlWorldModelGranularity;
        mr.mapx1 = ((float)gS.x_ + 1.f) * AnlWorldModelGranularity;
        mr.mapy0 = ((float)-gS.y_) * AnlWorldModelGranularity;
        mr.mapy1 = ((float)-gS.y_ + 1.f) * AnlWorldModelGranularity;
    }
    else
    {
        if (!SetAnlWorldModel(gS.model_))
            return false;
    }

    worldModel_->SetSeedAllModules(gS.wseed_);

    AnlMappingRange& mr = gS.mappingRange_;

    URHO3D_LOGINFOF("MapGeneratorWorld() - Init ... mpoint=%s(%d,%d) wseed=%u rseed=%u cseed=%u mrange=(%f,%f,%f,%f) activeslot=%d ... OK !",
                    gS.mappoint_.ToString().CString(), gS.x_, gS.y_, gS.wseed_, gS.rseed_, gS.cseed_, mr.mapx0, mr.mapx1, mr.mapy0, mr.mapy1, gS.activeslot_);

    return MapGenerator::Init(gS);
}

bool MapGeneratorWorld::Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    if (!worldModel_)
    {
        URHO3D_LOGERROR("MapGeneratorWorld() - Make ... worldModel_=0 !");
        return false;
    }

    RestoreStateFromMapStatus(genStatus);

    bool generateok = worldModel_->GenerateModules(genStatus, timer, delay);// && MapGenerator::Make(genStatus, timer, delay);

    if (generateok && !worldModel_->HasRenderableModule(TERRAINMAP))
        GenerateDefaultTerrainMap(genStatus);

    SaveStateToMapStatus(genStatus);

    return generateok;
}

//#include "GameHelpers.h"

void MapGeneratorWorld::GenerateSpots(MapGeneratorStatus& genStatus)
{
    int viewZindex = genStatus.GetActiveViewZIndex();

    URHO3D_LOGINFOF("MapGeneratorWorld() - GenerateSpots ... FRONTVIEW viewZindex=%d ...", viewZindex);

//    GameHelpers::DumpData(map_, 1, 2, xSize_, ySize_);

    // Generate spots for frontview (viewindex=1)
    GenerateRandomSpots(genStatus, SPOT_START, 1);
    GenerateRandomSpots(genStatus, SPOT_LIQUID, 10, 50, 500, 100);
    GenerateRandomSpots(genStatus, SPOT_LAND, 2, 20, 500, 45);

    URHO3D_LOGINFOF("MapGeneratorWorld() - GenerateSpots ... OK !");
}

void MapGeneratorWorld::GenerateFurnitures(MapGeneratorStatus& genStatus)
{
    URHO3D_LOGINFOF("MapGeneratorWorld() - GenerateFurnitures ... ");

    GenerateBiomeFurnitures(genStatus, BiomeExternalBack);
    GenerateBiomeFurnitures(genStatus, BiomeExternal);
    GenerateBiomeFurnitures(genStatus, BiomeCave);

    URHO3D_LOGINFOF("MapGeneratorWorld() - GenerateFurnitures ... OK !");
}
