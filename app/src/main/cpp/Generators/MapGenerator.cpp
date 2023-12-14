#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/IO/Log.h>

#include <stdio.h>

#include "Noise.h"

#include "DefsWorld.h"
#include "ViewManager.h"
#include "GameHelpers.h"
#include "GameAttributes.h"

#include "Map.h"
#include "MapWorld.h"

#include "MapGenerator.h"

extern const char* MapSpotTypeString[];
const World2DInfo * MapGenerator::info_ = 0;
int MapGenerator::xSize_ = 0;
int MapGenerator::ySize_ = 0;
short int MapGenerator::nghTable4_[4];
short int MapGenerator::nghTable8_[8];
short int MapGenerator::nghTable4xy_[4][2];
short int MapGenerator::nghTable8xy_[8][2];

#define MAX_SPOTS 200
#define MAX_FURNITURES 1000

void MapGenerator::InitTable()
{
    nghTable4xy_[0][0] = 0;        // TOP (N) x
    nghTable4xy_[0][1] = -1;       // TOP (N) y
    nghTable4xy_[1][0] = 1;        // RIGHT (E) x
    nghTable4xy_[1][1] = 0;        // RIGHT (E) y
    nghTable4xy_[2][0] = 0;        // BOTTOM (S) x
    nghTable4xy_[2][1] = 1;        // BOTTOM (S) y
    nghTable4xy_[3][0] = -1;       // LEFT (W) x
    nghTable4xy_[3][1] = 0;        // LEFT (W) y

    nghTable8xy_[0][0] = 0;        // TOP x
    nghTable8xy_[1][0] = 1;        // TOP-RIGHT x
    nghTable8xy_[2][0] = 1;        // RIGHT x
    nghTable8xy_[3][0] = 1;        // BOTTOM-RIGHT x
    nghTable8xy_[4][0] = 0;        // BOTTOM x
    nghTable8xy_[5][0] = -1;       // BOTTOM-LEFT x
    nghTable8xy_[6][0] = -1;       // LEFT x
    nghTable8xy_[7][0] = -1;       // TOP-LEFT x
    nghTable8xy_[0][1] = -1;       // TOP y
    nghTable8xy_[1][1] = -1;       // TOP-RIGHT y
    nghTable8xy_[2][1] = 0;        // RIGHT y
    nghTable8xy_[3][1] = 1;        // BOTTOM-RIGHT y
    nghTable8xy_[4][1] = 1;        // BOTTOM y
    nghTable8xy_[5][1] = 1;        // BOTTOM-LEFT y
    nghTable8xy_[6][1] = 0;        // LEFT y
    nghTable8xy_[7][1] = -1;       // TOP-LEFT y
}

void MapGenerator::SetWorldInfo(const World2DInfo * info)
{
    if (info_ != info)
    {
        info_ = info;
    }
}

void MapGenerator::SetSize(int width, int height)
{
    if (ySize_ != height || xSize_ != width)
    {
        xSize_ = width;
        ySize_ = height;

        nghTable4_[0] = -xSize_;       // TOP (N)
        nghTable4_[1] = 1;             // RIGHT (E)
        nghTable4_[2] = xSize_;        // BOTTOM (S)
        nghTable4_[3] = -1;            // LEFT (W)

        nghTable8_[0] = -xSize_;        // TOP
        nghTable8_[1] = -xSize_ + 1;    // TOP-RIGHT
        nghTable8_[2] = 1;              // RIGHT
        nghTable8_[3] = xSize_ + 1;     // BOTTOM-RIGHT
        nghTable8_[4] = xSize_;         // BOTTOM
        nghTable8_[5] = xSize_ - 1;     // BOTTOM-LEFT
        nghTable8_[6] = -1;             // LEFT
        nghTable8_[7] = -xSize_ - 1;    // TOP-LEFT
    }
}

MapGenerator::MapGenerator(const String& name) :
    name_(name),
    map_(0),
    Random(GameRand::GetRandomizer(MAPRAND))
{
    spots_.Reserve(MAX_SPOTS);
    furnitures_.Reserve(MAX_FURNITURES);
}

MapGenerator::~MapGenerator()
{
//    String str;
//    GameHelpers::AppendBufferToString(str, "PODVector=%p mapPtr=%p(%p)", &selfAllocatedMap_, selfAllocatedMap_.Size() ? &selfAllocatedMap_[0] : map_, map_);
//    URHO3D_LOGINFOF("~%s() - ... %s", name_.CString(), str.CString());

    spots_.Clear();
    furnitures_.Clear();
    map_ = 0;

    URHO3D_LOGINFOF("~%s() - OK !", name_.CString());
}



void MapGenerator::SaveStateToMapStatus(MapGeneratorStatus& genStatus)
{
    genStatus.rseed_ = GameRand::GetSeedRand(MAPRAND);
    genStatus.features_[genStatus.activeslot_] = map_;
}

void MapGenerator::RestoreStateFromMapStatus(MapGeneratorStatus& genStatus)
{
    GameRand::SetSeedRand(MAPRAND, genStatus.rseed_);
    map_ = genStatus.GetMap(genStatus.activeslot_);
}

void MapGenerator::SetGeneratorMap(MapGeneratorStatus& genStatus, int mapslot, int viewZ, FeatureType* data, bool genspot, bool clean)
{
    URHO3D_LOGINFOF("%s() - SetGeneratorMap : mapslot=%d viewz=%d", name_.CString(), mapslot, viewZ);

    genStatus.genSpots_ = genspot;
    genStatus.activeslot_ = mapslot;
    genStatus.viewZindexes_[mapslot] = ViewManager::GetViewZIndex(ViewManager::GetNearViewZ(viewZ));
    genStatus.features_[mapslot] = data;
    genStatus.rseed_ = genStatus.wseed_ + genStatus.cseed_;

    GameRand::SetSeedRand(MAPRAND, genStatus.rseed_);
    map_ = genStatus.GetMap(genStatus.activeslot_);

    if (clean)
        memset(map_, 0, xSize_ * ySize_ * sizeof(FeatureType));

    SaveStateToMapStatus(genStatus);

    genStatus.Dump();
}

void MapGenerator::SetParamsInt(MapGeneratorStatus& genStatus, const PODVector<int>& params)
{
    genStatus.genparams_ = params;
    /*
        String paramStr;
        for (int i=0; i < params.Size() ; ++i)
           paramStr += String(params[i]) + "|";

        URHO3D_LOGINFOF("%s() - SetParamsInt : %s", name_.CString(), paramStr.CString());
    */
}

void MapGenerator::SetParamsInt(MapGeneratorStatus& genStatus, int n, ...)
{
    if (n<1)
        return ;

//    String paramStr;

    genStatus.genparams_.Resize(n);

    va_list args;
    va_start(args, n);

    for (int i=0; i < n; ++i)
    {
        genStatus.genparams_[i] = va_arg(args, int);
//        paramStr += String(genStatus.genparams_[i]) + "|";
    }

//    URHO3D_LOGINFOF("%s() - SetParamsInt : %s", name_.CString(), paramStr.CString());

    va_end(args);
}

bool MapGenerator::Init(MapGeneratorStatus& genStatus)
{
    spots_.Clear();
    furnitures_.Clear();

    if (!genStatus.genparams_.Size())
        return false;

    SaveStateToMapStatus(genStatus);

    return true;
}

FeatureType* const MapGenerator::Generate(MapGeneratorStatus& genStatus)
{
    RestoreStateFromMapStatus(genStatus);

    if (!Init(genStatus))
        return 0;

    Make(genStatus);

    SaveStateToMapStatus(genStatus);

    return map_;
}

bool MapGenerator::Generate(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    int& mcount2 = genStatus.mapcount_[MAP_FUNC2];
    int& mcount3 = genStatus.mapcount_[MAP_FUNC3];

//    URHO3D_LOGINFOF("%s() - Generate ... indexToSet_=%u", name_.CString(), mcount2);

    if (mcount2 == 0)
    {
        RestoreStateFromMapStatus(genStatus);

        if (!Init(genStatus))
            return false;

        mcount3 = 0;
        mcount2++;
    }
    if (mcount2 == 1)
    {
        if (!Make(genStatus, timer, delay))
            return false;

        mcount3 = 0;
        //mcount2++;

        mcount2 = 0;
        return true;
    }

    mcount2 = 0;
    return false;
}

void MapGenerator::Make(MapGeneratorStatus& genStatus) { }

bool MapGenerator::Make(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    return true;
}

void MapGenerator::GenerateSpots(MapGeneratorStatus& genStatus)
{
    URHO3D_LOGERROR("MapGenerator() - GenerateSpots : EMPTY HERE !");
}


void MapGenerator::GenerateFurnitures(MapGeneratorStatus& genStatus)
{
    URHO3D_LOGERROR("MapGenerator() - GenerateFurnitures : EMPTY HERE !");
}


bool MapGenerator::GenerateSpots(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    RestoreStateFromMapStatus(genStatus);

    if (genStatus.GetActiveViewZIndex() < 0)
    {
        URHO3D_LOGERRORF("%s() - GenerateSpots : activeslot=%d with viewzindex < 0 !", name_.CString(), genStatus.activeslot_);
        genStatus.Dump();
        return true;
    }

    GenerateSpots(genStatus);

    SaveStateToMapStatus(genStatus);

    return true;
}

bool MapGenerator::GenerateFurnitures(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    RestoreStateFromMapStatus(genStatus);

    GenerateFurnitures(genStatus);

    SaveStateToMapStatus(genStatus);

    return true;
}

void MapGenerator::GenerateDefaultTerrainMap(MapGeneratorStatus& genStatus)
{
    FeatureType* terrainmap = genStatus.GetMap(TERRAINMAP);

    if (!terrainmap && genStatus.map_)
    {
        URHO3D_LOGINFOF("MapGenerator() - GenerateDefaultTerrainMap ... Set TerrainMap Ptr ...");
        terrainmap = genStatus.map_->GetTerrainMap();
        genStatus.features_[TERRAINMAP] = terrainmap;
    }

    Noise noise;
    noise.SetValueSeed(genStatus.rseed_);
    int octave = 1;
    float freq = 0.01f;

    unsigned addr = 0;
    int height = genStatus.height_;
    int width = genStatus.width_;
    for (int j=0; j < height; j++)
        for (int i=0; i < width; i++, addr++)
            terrainmap[addr] = RoundToInt(3.f * fabs(noise.Noise2D(Noise::SIMPLEX, float(i)/width, float(j)/height, freq, octave)));

    URHO3D_LOGINFOF("MapGenerator() - GenerateDefaultTerrainMap ... OK !");
}

#if 0
bool MapGenerator::IsAreaForSpot(MapGeneratorStatus& genStatus, MapSpotType spotType, int x, int y)
{
    switch (spotType)
    {
    case SPOT_START:
    case SPOT_ROOM:
    case SPOT_LAND:
        return (GetCell(x, y) < MapFeatureType::NoRender &&
                GetCell(x+1, y) < MapFeatureType::NoRender &&
                GetCell(x, y+1) > MapFeatureType::NoRender &&
                GetCell(x+1, y+1) > MapFeatureType::NoRender);
        break;

    case SPOT_LIQUID:
        return (GetCell(x, y) < MapFeatureType::NoRender &&
                GetCell(x+1, y) < MapFeatureType::NoRender &&
                GetCell(x, y+1) < MapFeatureType::NoRender &&
                GetCell(x+1, y+1) < MapFeatureType::NoRender);
        break;
    default :
        return false;
    }
}
#else
bool MapGenerator::IsAreaForSpot(MapGeneratorStatus& genStatus, MapSpotType spotType, int x, int y)
{
    if (spotType == SPOT_LIQUID)
    {
        return (GetCell(x, y) < MapFeatureType::NoRender && GetCell(x+1, y) < MapFeatureType::NoRender &&
                GetCell(x, y+1) < MapFeatureType::NoRender && GetCell(x+1, y+1) < MapFeatureType::NoRender);
    }
    else
    {
        if (GetCell(x, y) < MapFeatureType::NoRender && GetCell(x+1, y) < MapFeatureType::NoRender &&
                GetCell(x, y+1) > MapFeatureType::NoRender)
        {
            Vector2 position = World2D::GetWorldInfo()->GetPosition(x,y);

            // INNERVIEW
            // ==> si le mapspot vérifie MapCollider(FRONTVIEW)->IsInside(mapspot.point, checkholes=false) == true
            // ===> et si le mapspot vérifie MapCollider(INNERVIEW)->IsInside(mapspot.point, checkholes=false) == false, placer le mapspot en INNERVIEW sinon ne pas enregistrer ce POINT

            if (genStatus.GetActiveViewZIndex() == ViewManager::INNERVIEW_Index)
            {
                PODVector<MapCollider*> colliders;

                int indv = genStatus.map_->GetColliderIndex(FRONTVIEW);
                if (indv != -1)
                {
                    bool outsidefront = false;

                    genStatus.map_->GetColliders(PHYSICCOLLIDERTYPE, indv, colliders);
                    for (unsigned i=0; i < colliders.Size(); i++)
                        outsidefront |= colliders[i]->IsInside(position, false);

                    if (!outsidefront)
                        return false;
                }

                indv = genStatus.map_->GetColliderIndex(INNERVIEW);
                if (indv != -1)
                {
                    bool insideinner = false;

                    genStatus.map_->GetColliders(PHYSICCOLLIDERTYPE, indv, colliders);
                    for (unsigned i=0; i < colliders.Size(); i++)
                        insideinner |= colliders[i]->IsInside(position, false);

                    return insideinner == false;
                }

                return false;
            }

            // FRONTVIEW
            // => FRONTVIEW seulement si map->GetTopography()->IsFullGround() == false
            // ==> si le mapspot verifie MapCollider(FRONTVIEW)->IsInside(mapspot.point, checkholes=false) == false, placer le mapspot en FRONTVIEW sinon ne pas enregistrer ce POINT

            else
            {
                if (genStatus.map_->GetType() != Map::GetTypeStatic())
                    return false;

                Map* map = static_cast<Map*>(genStatus.map_);
                if (map && map->GetTopography().IsFullGround())
                    return false;

                bool insidefront = false;

                int indv = genStatus.map_->GetColliderIndex(FRONTVIEW);
                if (indv != -1)
                {
                    PODVector<MapCollider*> colliders;
                    genStatus.map_->GetColliders(PHYSICCOLLIDERTYPE, indv, colliders);
                    for (unsigned i=0; i < colliders.Size(); i++)
                        insidefront |= colliders[i]->IsInside(position, false);
                }

                return insidefront == false;
            }
        }
        else
            return false;
    }

    return false;
}
#endif

bool MapGenerator::GenerateRandomSpots(MapGeneratorStatus& genStatus, const MapSpotType& spotType, const int& minSpots, const int& maxSpots, int maxTries, const int& spotProb)
{
    int viewZIndex = genStatus.GetActiveViewZIndex();
    const String& viewZ = ViewManager::Get()->GetViewZName(viewZIndex);

    URHO3D_LOGINFOF("%s() - GenerateRandomSpots : Type=%s(%d) on ViewZ=%s minSpots=%d maxSpots=%d maxTries=%d prob=%d - Spots InitSize=%u ...",
                    name_.CString(), MapSpotTypeString[spotType], spotType, viewZ.CString(), minSpots, maxSpots, maxTries, spotProb, spots_.Size());

    // SPOT_START with Empty Map : 1 spot START mini
    if (spotType == SPOT_START && minSpots > 0)
    {
        if (genStatus.map_->GetType() != Map::GetTypeStatic())
            return false;

        Map* map = static_cast<Map*>(genStatus.map_);
        if (map && map->GetTopography().IsFullSky())
        {
            spots_.Push(MapSpot(spotType, xSize_ / 2, ySize_ - 2, viewZIndex, 1, 1));
            return true;
        }
    }

    int x, y, count = 0;

    if (spotProb >= 100)
    {
        while (maxTries--)
        {
            x = Random.Get(1, xSize_ - 2);
            y = Random.Get(1, ySize_ - 2);

            // Find an area for spotType
            if (IsAreaForSpot(genStatus, spotType, x, y))
            {
                spots_.Push(MapSpot(spotType, x, y, viewZIndex, 1, 1));
                count++;

//                URHO3D_LOGINFOF("%s() - GenerateRandomSpots : Add Spot at %d %d viewZ=%s", name_.CString(), x, y, viewZ.CString());

                if (spots_.Size() >= MAX_SPOTS || count >= maxSpots)
                    break;
            }
        }
    }
    else
    {
        while (maxTries--)
        {
            if (Random.Get(0, 100) < spotProb)
            {
                x = Random.Get(1, xSize_ - 2);
                y = Random.Get(1, ySize_ - 2);

                // Find an area for spotType
                if (IsAreaForSpot(genStatus, spotType, x, y))
                {
                    spots_.Push(MapSpot(spotType, x, y, viewZIndex, 1, 1));
                    count++;

//                    URHO3D_LOGINFOF("%s() - GenerateRandomSpots : Add Spot at %d %d viewZ=%s", name_.CString(), x, y, viewZ.CString());

                    if (spots_.Size() >= MAX_SPOTS || count >= maxSpots)
                        break;
                }
            }
        }
    }
//    URHO3D_LOGINFOF("%s() - GenerateRandomSpots : ... Type=%s(%d) on ViewZ=%s Size=%u ... OK !", name_.CString(), MapSpotTypeString[spotType], spotType, viewZ.CString(), spots_.Size());

    return count >= minSpots;
}

//static Vector<unsigned> sConnectionIndex_;

const int BiomeLayerZ[] =
{
    BACKBIOME,      // BiomeExternalBack
    BACKINNERBIOME, // BiomeCave
    BACKINNERBIOME,  // BiomeDungeon
    FRONTBIOME,     // BiomeExternal
};

const int BiomeLinkedViewZs[] =
{
    BACKGROUND, // BiomeExternalBack
    INNERVIEW,  // BiomeCave
    INNERVIEW,  // BiomeDungeon
    FRONTVIEW,  // BiomeExternal
};

const int BiomeSpawnBase[] =
{
    3,  // BiomeExternalBack
    0,  // BiomeCave
    0,  // BiomeDungeon
    1,  // BiomeExternal
};

const int BiomeSpawnRatio[4][4] = // Top, Bottom, Right, Left
{
    { 10, 0, 0, 0 }, // BiomeExternalBack
    { 10, 10, 10, 10 },  // BiomeCave
    { 2, 2, 1, 1 },  // BiomeDungeon
    { 2, 0, 0, 0 },  // BiomeExternal
};

const unsigned BiomeDirectionalCOTs[4][4] = // Top, Bottom, Right, Left
{
    { BIOMEBACK.Value(), BIOMEBACKBOTTOM.Value(), BIOMEBACKSIDES.Value(), BIOMEBACKSIDES.Value() },                 // BiomeExternalBack
    { BIOMECAVE.Value(), BIOMECAVEBOTTOM.Value(), BIOMECAVESIDES.Value(), BIOMECAVESIDES.Value() },                 // BiomeCave
    { DUNGEON.Value(), DUNGEONBOTTOM.Value(), DUNGEONSIDES.Value(), DUNGEONSIDES.Value() },                         // BiomeDungeon
    { BIOMEEXTERNAL.Value(), BIOMEEXTERNALBOTTOM.Value(), BIOMEEXTERNALSIDES.Value(), BIOMEEXTERNALSIDES.Value() }, // BiomeExternal
};

const int DirectionalInfos[4][4] = // tileoffsetx, tileoffsety, rotate, flipY
{
    { 0, 127, false, false }, // Top
    { 0, -127, false, true }, // Bottom
    { 127, 0, true, true },   // Right
    { -127, 0, true, false }, // Left
};

void MapGenerator::GenerateBiomeFurnitures(MapGeneratorStatus& genStatus, int biome)
{
    URHO3D_LOGINFOF("MapGenerator() - GenerateBiomeFurnitures ... biome=%d num furnitures=%u ...", biome, furnitures_.Size());

    if (genStatus.map_->GetType() != Map::GetTypeStatic())
        return;

    Map* map = static_cast<Map*>(genStatus.map_);
    if (!map)
        return;

    const int viewZ = BiomeLinkedViewZs[biome];

    MapTopography& topo = map->GetTopography();
    topo.GenerateFreeSideTiles(genStatus, viewZ);

    if (topo.HasNoFreeSides())
        return;

    /*
        Biomemap =>
        valeur(3) zone très dense : beaucoup de végétation, grands arbres
        valeur(2) zone moyenne : quelques arbres, arbustres et plantes basses
        valeur(1) zone pauvre : quelques herbages
        valeur(0) zone aride : quelques cactés
    */

    // Obtenir la terrainmap => utiliser pour les variétés de vegetation selon (le type de sol)
    const FeatureType* terrainmap = genStatus.GetMap(TERRAINMAP);

    // Obtenir la biomemap => utiliser pour la densité en vegetation
    const FeatureType* biomemap = genStatus.GetMap(BIOMEMAP);

    GameRand& ORand = GameRand::GetRandomizer(OBJRAND);
    ORand.SetSeed(1000);

    const int layerZ = BiomeLayerZ[biome];

    unsigned tileindex, gotprops;
    int rand;
    bool spawnsuccess;

    URHO3D_LOGINFOF("MapGenerator() - GenerateBiomeFurnitures ... OK !");

    // spawn on directions : top, bottom, right, left
    for (int i = 0 ; i < 4; i++)
    {
        const PODVector<unsigned>& sidetiles = topo.freeSideTiles_[i];

        StringHash cot(BiomeDirectionalCOTs[biome][i]);

//        URHO3D_LOGINFOF("MapGenerator() - GenerateBiomeFurnitures ... dir=%d topofreesides=%u cot=%u!", i, sidetiles.Size(), cot.Value());

        for (PODVector<unsigned>::ConstIterator it = sidetiles.Begin(); it != sidetiles.End(); ++it)
        {
            tileindex = *it;
            rand = ORand.Get(100);
            spawnsuccess = rand < (BiomeSpawnBase[biome] + BiomeSpawnRatio[biome][i] * (int)biomemap[tileindex]);

            if (!spawnsuccess)
                continue;

            const StringHash& got = COT::GetRandomTypeFrom(cot, tileindex+1);
            if (got.Value() != 0)
            {
                gotprops = GOT::GetTypeProperties(got);

                int layer = GOT::GetObject(got)->GetVar(GOA::PLATEFORM).GetBool() ? viewZ : layerZ;

                rand = ORand.Get(-127, 127);
                furnitures_.Resize(furnitures_.Size()+1);
                EntityData& entitydata = furnitures_.Back();
                //EntityData::Set(short unsigned gotindex=0, short unsigned tileindex=USHRT_MAX, signed char tilepositionx=0, signed char tilepositiony_=0, unsigned char sstype=-1, int layerZ=0, bool rotate=false, bool flipX=false, bool flipY=false);
                entitydata.Set(GOT::GetIndex(got), tileindex, DirectionalInfos[i][0] != 0 ? DirectionalInfos[i][0] : rand, DirectionalInfos[i][1] != 0 ? DirectionalInfos[i][1] : rand, 0,
                               layer, DirectionalInfos[i][2], (gotprops & GOT_Flippable) ? rand > 0 : false, DirectionalInfos[i][3]);
//                URHO3D_LOGERRORF("MapGenerator() - GenerateBiomeFurnitures dir=%d ... got=%s(%u) flippable=%s layer=%d layerZIndex=%d !",
//                                   i, GOT::GetType(got).CString(), got.Value(), (gotprops & GOT_Flippable) ? "true":"false", layerZ, entitydata.GetLayerZIndex());
            }
        }
    }

    URHO3D_LOGINFOF("MapGenerator() - GenerateBiomeFurnitures ... num furnitures=%u OK !", furnitures_.Size());
}


