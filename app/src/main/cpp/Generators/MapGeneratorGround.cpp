#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include "Noise.h"

#include "ViewManager.h"

#include "MapGeneratorGround.h"

/*
SetParams(6, maxHeight1, maxHeight2, octave1, octave2, step, noiseType)
    params_[0] = maxHeight1
    params_[1] = maxHeight2
    params_[2] = octave1
    params_[3] = octave2
    params_[4] = step
    params_[5] = noiseType
    params_[6] = startYFactor1
    params_[7] = startYFactor2
    params_[8] = freq1
    params_[9] = freq2
*/

MapGeneratorGround::MapGeneratorGround()
    : MapGenerator("MapGeneratorGround")
{ }

void MapGeneratorGround::Make(MapGeneratorStatus& genStatus)
{
    Noise *tnoise, *bnoise;
    Noise noise1, noise2;
    const Noise::NoiseType noisetype = genStatus.genparams_.Size() == 6 ? (Noise::NoiseType)genStatus.genparams_[5] : Noise::VALUE;

    String paramStr;
    for (unsigned i=0; i<genStatus.genparams_.Size(); ++i)
        paramStr += String(genStatus.genparams_[i]) + "|";

    URHO3D_LOGINFOF("MapGeneratorGround() - Make ... s=%u p=%s", genStatus.rseed_, paramStr.CString());

    const float beginX1 = noisetype == Noise::VALUE ? 0.f : (float)(genStatus.x_+genStatus.y_*genStatus.y_);
    const float beginX2 = noisetype == Noise::VALUE ? 0.f : beginX1 + 0.5f;

    if (noisetype == Noise::VALUE)
    {
        // noisevalue for Top Curve
        noise1.SetValueSeed(Random.Get());
        tnoise = &noise1;
        // noisevalue for Bottom Curve
        noise2.SetValueSeed(Random.Get());
        bnoise = &noise2;
    }
    else
        // same noiseobject for noisetype > VALUE
        tnoise = bnoise = &noise1;

    // params for Top Curve
    const int height1 = genStatus.genparams_[0];
    const int octave1 = genStatus.genparams_[2];
    const float freq1 = (float)genStatus.genparams_[8]/100.f;
    const int beginY1 = ySize_*genStatus.genparams_[6]/100;

    // params for Bottom Curve
    const int height2 = genStatus.genparams_[1];
    const int octave2 = genStatus.genparams_[3];
    const float freq2 = (float)genStatus.genparams_[9]/100.f;
    const int beginY2 = ySize_*genStatus.genparams_[7]/100;

    const int step = genStatus.genparams_[4];

    if (genStatus.GetMap(GROUNDMAP))
    {
        map_ = genStatus.GetMap(GROUNDMAP);
        int i, k;
        int ytopcurve, ybottomcurve;
        for (i=0; i<xSize_; i++)
        {
            if (!i || !(i%step))
            {
                ytopcurve    = Min(beginY1 + (int) std::floor(tnoise->Noise1D(noisetype, beginX1 + float(i)/xSize_, freq1, octave1) * height1), ySize_-1);
                ybottomcurve = Min(beginY2 + (int) std::floor(bnoise->Noise1D(noisetype, beginX2 + float(i)/xSize_, freq2, octave2) * height2), ySize_-1);
            }

            k = ytopcurve-1;
            // under ytopcurve, fill with blocks = ground
            while (++k < ybottomcurve)
                map_[xSize_*k+i] = MapFeatureType::OuterFloor;

            // under ybottomcurve, remove blocks
            k--;
            while (++k < ySize_)
                map_[xSize_*k+i] = MapFeatureType::NoMapFeature;
        }
    }

    GenerateDefaultTerrainMap(genStatus);

    URHO3D_LOGINFOF("MapGeneratorGround() - Make ... OK !");
}

void MapGeneratorGround::GenerateSpots(MapGeneratorStatus& genStatus)
{
    int viewZindex = genStatus.GetActiveViewZIndex();

    URHO3D_LOGINFOF("MapGeneratorGround() - GenerateSpots ... %s ...", ViewManager::Get()->GetViewZName(viewZindex).CString());

//    GameHelpers::DumpData(map_ , 1, 2, xSize_, ySize_);
    GenerateRandomSpots(genStatus, SPOT_START, 1);
    if (viewZindex == ViewManager::FRONTVIEW_Index)
        GenerateRandomSpots(genStatus, SPOT_LIQUID, 10, 50, 500, 100);
    GenerateRandomSpots(genStatus, SPOT_LAND, 2, 20, 500, 45);

    URHO3D_LOGINFOF("MapGeneratorGround() - GenerateSpots ... OK !");
}

void MapGeneratorGround::GenerateFurnitures(MapGeneratorStatus& genStatus)
{
    URHO3D_LOGINFOF("MapGeneratorGround() - GenerateFurnitures ... ");

    GenerateBiomeFurnitures(genStatus, BiomeExternalBack);
    GenerateBiomeFurnitures(genStatus, BiomeExternal);

    URHO3D_LOGINFOF("MapGeneratorGround() - GenerateFurnitures ... OK !");
}
