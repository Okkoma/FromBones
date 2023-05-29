#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include "../VCommon/noise_gen.h"
#include "../VCommon/random_gen.h"
#include "../VCommon/hashing.h"
#include "../VCommon/utility.h"

#include "Evaluator.h"

#include "Functions.h"


namespace anl
{

const char* ANLModuleTypeV2Str[] =
{
    "None",

    "Seed",
    "Constant",
    "Seeder",
    "CacheArray",

    "ValueBasis",
    "GradientBasis",
    "SimplexBasis",
    "CellularBasis",
    "X",
    "Y",
    "Z",
    "Radial",
    "W",
    "U",
    "V",

    "ScaleDomain",
    "ScaleX",
    "ScaleY",
    "ScaleZ",
    "ScaleW",
    "ScaleU",
    "ScaleV",

    "TranslateDomain",
    "TranslateX",
    "TranslateY",
    "TranslateZ",
    "TranslateW",
    "TranslateU",
    "TranslateV",

    "RotateDomain",

    "DX",
    "DY",
    "DZ",
    "DW",
    "DU",
    "DV",

    "Fractal",

    "Abs",
    "Sin",
    "Cos",
    "Tan",
    "ASin",
    "ACos",
    "ATan",

    "Add",
    "Substract",
    "Multiply",
    "Divide",
    "Pow",
    "Min",
    "Max",
    "Bias",
    "Gain",

    "Sigmoid",
    "Randomize",
    "CurveSection",
    "HexTile",
    "HexBump",
    "Clamp",
    "Mix",
    "Select",

    "SmoothStep",
    "SmootherStep",
    "LinearStep",
    "Step",
    "Tiers",
    "SmoothTiers",

    "Color",
    "ExtractRed",
    "ExtractGreen",
    "ExtractBlue",
    "ExtractAlpha",
    "Grayscale",
    "CombineRGBA",
    "CombineHSVA",

    "1",

    "SimpleFBM",
    "SimpleRidgedMF",
    "SimpleBillow",

    0
};


// Instruction Functions

void FunctionSeed(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{

}

void FunctionEmpty(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{

}

void FunctionSeeder(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{

}

void FunctionCacheArray(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);

    float* cacheaccessor = evaluator->GetCacheAccessor(index);

    if (cacheaccessor)
    {
        // get cachearray value
        if (evaluator->GetCacheAvailable(index))
        {
#ifdef USE_CACHESTAT
            // keep track
            if (evaluator->GetCacheAccessIndex() % (evaluator->GetCache().cacheheight_* evaluator->GetNumInstructions()/32) == 0)
            {
                URHO3D_LOGINFOF("Getter Access accessorid=%u iget=%d (iid=%u)", evaluator->GetAccessorId(), evaluator->GetCacheAccessIndex(), index);
            }
#endif
            evaluator->PushCachedDataValue(index, cacheaccessor[evaluator->GetCacheAccessIndex()]);
            return;
        }

        float val = instruction.GetParamFloat(evaluator, 0);

        // copy the value of the source in the cachedata of the current instruction
        // the source value for the coord is already evaluated by sequence
        evaluator->PushCachedDataValue(index, val);

#ifdef USE_CACHESTAT
        // keep track
        if (evaluator->GetCacheAccessIndex() % (evaluator->GetCache().cacheheight_* evaluator->GetNumInstructions()/32) == 0)
        {
            URHO3D_LOGINFOF("Setter Access accessorid=%u iset=%d (iid=%u)", evaluator->GetAccessorId(), evaluator->GetCacheAccessIndex(), index);
        }
#endif
        // set cachearray value
        cacheaccessor[evaluator->GetCacheAccessIndex()] = val;
    }
}

// Basis

void FunctionValueBasis(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    const std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val;
        value_noise(coords[ilayer], instruction.GetParamUInt(evaluator, 1, ilayer), Interpolation(instruction.GetParamInt(evaluator, 0, ilayer)), &val);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionGradientBasis(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    const std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    unsigned seed = instruction.GetParamUInt(evaluator, 1);
    interp_func func = Interpolation(instruction.GetParamInt(evaluator, 0));
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val;
        gradient_noise(coords[ilayer], seed, func, &val);
        evaluator->PushCachedDataValue(index, val);
//        #ifdef USE_CACHESTAT
//        if (DebugV2) URHO3D_LOGINFOF("FunctionGradientBasis: ilayer=%u coords=(%f,%f) val=%f", ilayer, coords[ilayer].x_, coords[ilayer].y_, val);
//        #endif
    }
}

void FunctionSimplexBasis(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    const std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val;
        simplex_noise(coords[ilayer], instruction.GetParamUInt(evaluator, 0, ilayer), noInterp, &val);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionCellularBasis(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    float f[4], d[4];

    evaluator->ClearCachedDataValues(index);
    const std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();

    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        const CCoordinate& coord = coords[ilayer];
        int dimension = coord.dimension_;
        float val = 0.f;
        if (dimension == 2)
            cellular_function2D(coord.x_, coord.y_, instruction.GetParamUInt(evaluator, 9, ilayer), f, d, Distance2(instruction.GetParamInt(evaluator, 0, ilayer)));
        else if (dimension == 3)
            cellular_function3D(coord.x_, coord.y_, coord.z_, instruction.GetParamUInt(evaluator, 9, ilayer), f, d, Distance3(instruction.GetParamInt(evaluator, 0, ilayer)));
        else if (dimension == 4)
            cellular_function4D(coord.x_, coord.y_, coord.z_, coord.w_, instruction.GetParamUInt(evaluator, 9, ilayer), f, d, Distance4(instruction.GetParamInt(evaluator, 0, ilayer)));
        else
            cellular_function6D(coord.x_, coord.y_, coord.z_, coord.w_, coord.u_, coord.v_, instruction.GetParamUInt(evaluator, 9, ilayer), f, d, Distance6(instruction.GetParamInt(evaluator, 0, ilayer)));

        for (int j=0; j < 4; j++)
            val += instruction.GetParamFloat(evaluator, 1+j, ilayer)*f[j] + instruction.GetParamFloat(evaluator, 5+j, ilayer)*d[j];
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionX(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        evaluator->PushCachedDataValue(index, evaluator->GetCachedCoords(index)[ilayer].x_);
}

void FunctionY(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        evaluator->PushCachedDataValue(index, evaluator->GetCachedCoords(index)[ilayer].y_);
}

void FunctionZ(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        evaluator->PushCachedDataValue(index, evaluator->GetCachedCoords(index)[ilayer].z_);
}

void FunctionRadial(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    const std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val;
        const CCoordinate& coord = coords[ilayer];
        if (coord.dimension_ == 2) val = std::sqrt(coord.x_*coord.x_+coord.y_*coord.y_);
        else if (coord.dimension_ == 3) val = std::sqrt(coord.x_*coord.x_+coord.y_*coord.y_+coord.z_*coord.z_);
        else if (coord.dimension_ == 4) val = std::sqrt(coord.x_*coord.x_+coord.y_*coord.y_+coord.z_*coord.z_+coord.w_*coord.w_);
        else val = std::sqrt(coord.x_*coord.x_+coord.y_*coord.y_+coord.z_*coord.z_+coord.w_*coord.w_+coord.u_*coord.u_+coord.v_*coord.v_);
//        #ifdef USE_CACHESTAT
//        if (DebugV2) URHO3D_LOGINFOF("FunctionRadial: ilayer=%u coords=(%f,%f) => val=%f", ilayer, coords[ilayer].x_, coords[ilayer].y_, val);
//        #endif
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionW(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        evaluator->PushCachedDataValue(index, evaluator->GetCachedCoords(index)[ilayer].w_);
}

void FunctionU(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        evaluator->PushCachedDataValue(index, evaluator->GetCachedCoords(index)[ilayer].u_);
}

void FunctionV(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        evaluator->PushCachedDataValue(index, evaluator->GetCachedCoords(index)[ilayer].v_);
}

// Coordinates Transformations

void FunctionScaleDomain(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();

    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float scalefactor = instruction.GetParamFloat(evaluator, 1, ilayer);
        CCoordinate scale(coords[ilayer].dimension_, scalefactor);
        coords[ilayer] *= scale;
#ifdef USE_CACHESTAT
        if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionScaleDomain: ilayer=%u scalefactor=%f coords=(%f,%f)", ilayer, scalefactor, coords[ilayer].x_, coords[ilayer].y_);
#endif
    }
}

void FunctionScaleX(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate scale(coords[ilayer].dimension_, 1.f);
        scale.x_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] *= scale;
    }
}

void FunctionScaleY(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate scale(coords[ilayer].dimension_, 1.f);
        scale.y_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] *= scale;
    }
}

void FunctionScaleZ(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate scale(coords[ilayer].dimension_, 1.f);
        scale.z_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] *= scale;
    }
}

void FunctionScaleW(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate scale(coords[ilayer].dimension_, 1.f);
        scale.w_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] *= scale;
    }
}

void FunctionScaleU(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate scale(coords[ilayer].dimension_, 1.f);
        scale.u_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] *= scale;
    }
}

void FunctionScaleV(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate scale(coords[ilayer].dimension_, 1.f);
        scale.v_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] *= scale;
    }
}

void FunctionTranslateDomain(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float translation = instruction.GetParamFloat(evaluator, 1, ilayer);
        CCoordinate trans(coords[ilayer].dimension_, translation);
        coords[ilayer] += trans;
#ifdef USE_CACHESTAT
        if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionTranslateDomain: ilayer=%u translation=%f coords=(%f,%f)", ilayer, translation, coords[ilayer].x_, coords[ilayer].y_);
#endif
    }
}

void FunctionTranslateX(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate trans(coords[ilayer].dimension_, 0.f);
        trans.x_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] += trans;
    }
}

void FunctionTranslateY(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate trans(coords[ilayer].dimension_, 0.f);
        trans.y_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] += trans;
    }
}

void FunctionTranslateZ(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate trans(coords[ilayer].dimension_, 0.f);
        trans.z_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] += trans;
    }
}

void FunctionTranslateW(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate trans(coords[ilayer].dimension_, 0.f);
        trans.w_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] += trans;
    }
}

void FunctionTranslateU(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate trans(coords[ilayer].dimension_, 0.f);
        trans.u_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] += trans;
    }
}

void FunctionTranslateV(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate trans(coords[ilayer].dimension_, 0.f);
        trans.v_ = instruction.GetParamFloat(evaluator, 1, ilayer);
        coords[ilayer] += trans;
    }
}

void FunctionRotateDomain(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();
    int dimension = coords[0].dimension_;

    CRotMatrix& rmatrix = evaluator->GetCachedRotMatrix(index);
    RotMatrix& rmat = rmatrix.mat_;

    if (rmatrix.cstparams_)
    {
        if (!rmatrix.evaluated_)
        {
            float angle = instruction.GetParamFloat(evaluator, 1);
            float cosangle=cos(angle);
            float sinangle=sin(angle);

            float ax = instruction.GetParamFloat(evaluator, 2);
            float ay = instruction.GetParamFloat(evaluator, 3);
            float az = instruction.GetParamFloat(evaluator, 4);
            float len = std::sqrt(ax*ax+ay*ay+az*az);
//            float len = std::sqrt(ax*ax+ax*ay+az*az);
            ax/=len;
            ay/=len;
            az/=len;


            rmat[0] = 1.f + (1.f-cosangle)*(ax*ax-1.f);
            rmat[3] = -az*sinangle+(1.f-cosangle)*ax*ay;
            rmat[1] = az*sinangle+(1.f-cosangle)*ax*ay;
            rmat[4] = 1.f + (1.f-cosangle)*(ay*ay-1.f);

            if (dimension > 2)
            {
                rmat[6] = ay*sinangle+(1.f-cosangle)*ax*az;
                rmat[7] = -ax*sinangle+(1.f-cosangle)*ay*az;
                rmat[2] = -ay*sinangle+(1.f-cosangle)*ax*az;
                rmat[5] = ax*sinangle+(1.f-cosangle)*ay*az;
                rmat[8] = 1.f + (1.f-cosangle)*(az*az-1.f);
            }
#ifdef USE_CACHESTAT
            URHO3D_LOGINFOF("FunctionRotateDomain: index=%u rotation matrix evaluated !", index);
#endif
            rmatrix.evaluated_ = true;
        }

        if (dimension <= 2)
        {
            for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
            {
                CCoordinate& coord = coords[ilayer];

                float x = (rmat[0]*coord.x_) + (rmat[3]*coord.y_);
                float y = (rmat[1]*coord.x_) + (rmat[4]*coord.y_);

#ifdef USE_CACHESTAT
                if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionRotateDomain: ilayer=%u coords=(%f,%f) newcoords=(%f,%f)", ilayer, coord.x_, coord.y_, x, y);
#endif

                coord.x_ = x;
                coord.y_ = y;
            }
        }
        else
        {
            for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
            {
                CCoordinate& coord = coords[ilayer];

                float x = (rmat[0]*coord.x_) + (rmat[3]*coord.y_) + (rmat[6]*coord.z_);
                float y = (rmat[1]*coord.x_) + (rmat[4]*coord.y_) + (rmat[7]*coord.z_);
                float z = (rmat[2]*coord.x_) + (rmat[5]*coord.y_) + (rmat[8]*coord.z_);

#ifdef USE_CACHESTAT
                if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionRotateDomain: ilayer=%u coords=(%f,%f,%f) newcoords=(%f,%f,%f)", ilayer, coord.x_, coord.y_, coord.z_, x, y, z);
#endif

                coord.x_ = x;
                coord.y_ = y;
                coord.z_ = z;
            }
        }
    }
    else
    {
        for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
        {
            CCoordinate& coord = coords[ilayer];

            float x, y, z;

            float angle = instruction.GetParamFloat(evaluator, 1, ilayer);
            float cosangle=cos(angle);
            float sinangle=sin(angle);

            float ax = instruction.GetParamFloat(evaluator, 2, ilayer);
            float ay = instruction.GetParamFloat(evaluator, 3, ilayer);
            float az = instruction.GetParamFloat(evaluator, 4, ilayer);
            float len = std::sqrt(ax*ax+ay*ay+az*az);
//            float len = std::sqrt(ax*ax+ax*ay+az*az);
            ax/=len;
            ay/=len;
            az/=len;

            rmat[0] = 1.f + (1.f-cosangle)*(ax*ax-1.f);
            rmat[3] = -az*sinangle+(1.f-cosangle)*ax*ay;
            rmat[1] = az*sinangle+(1.f-cosangle)*ax*ay;
            rmat[4] = 1.f + (1.f-cosangle)*(ay*ay-1.f);

            if (dimension > 2)
            {
                rmat[6] = ay*sinangle+(1.f-cosangle)*ax*az;
                rmat[7] = -ax*sinangle+(1.f-cosangle)*ay*az;
                rmat[2] = -ay*sinangle+(1.f-cosangle)*ax*az;
                rmat[5] = ax*sinangle+(1.f-cosangle)*ay*az;
                rmat[8] = 1.f + (1.f-cosangle)*(az*az-1.f);
            }

            if (dimension <= 2)
            {
                x = (rmat[0]*coord.x_) + (rmat[3]*coord.y_);
                y = (rmat[1]*coord.x_) + (rmat[4]*coord.y_);
            }
            else
            {
                x = (rmat[0]*coord.x_) + (rmat[3]*coord.y_) + (rmat[6]*coord.z_);
                y = (rmat[1]*coord.x_) + (rmat[4]*coord.y_) + (rmat[7]*coord.z_);
                z = (rmat[2]*coord.x_) + (rmat[5]*coord.y_) + (rmat[8]*coord.z_);
            }

#ifdef USE_CACHESTAT
            if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionRotateDomain: ilayer=%u a=%f ax=%f ay=%f az=%f rot(%f %f %f %f %f %f) coords=(%f,%f) newcoords=(%f,%f)",
                                                        ilayer, angle, ax, ay, az, rmat[0], rmat[3], rmat[6], rmat[1], rmat[4], rmat[7], coord.x_, coord.y_, x, y);
#endif

            coord.x_ = x;
            coord.y_ = y;

            if (dimension > 2)
                coord.z_ = z;
        }
    }
}

void FunctionDXCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size();

    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        CCoordinate newcoord(coords[ilayer]);
        newcoord.x_ +=  instruction.GetParamFloat(evaluator, 1, ilayer);
        evaluator->PushCachedDataCoord(index, newcoord);
#ifdef USE_CACHESTAT
        if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionDXCoord: coord(%f,%f) decalx=%f => pushNewCoordx=(%f,%f) => numlayers=%u",
                                                    coords[ilayer].x_, coords[ilayer].y_, instruction.GetParamFloat(evaluator, 1, ilayer), newcoord.x_, newcoord.y_, coords.size());
#endif
    }
}

void FunctionDXValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    std::vector<CCoordinate>& coords = evaluator->GetCachedCoords(index);
    unsigned numlayers = coords.size()/2;
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float v0 = instruction.GetParamFloat(evaluator, 0, ilayer);
        float v1 = instruction.GetParamFloat(evaluator, 0, ilayer + numlayers);
        float decal = instruction.GetParamFloat(evaluator, 1, ilayer);
        float val = (v0-v1) / decal;
        evaluator->PushCachedDataValue(index, val);
#ifdef USE_CACHESTAT
        if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("FunctionDXValue: v[%u]=%f v[%u]=%f decal=%f => val=%f",
                                                    v0, ilayer, v1, ilayer + numlayers, decal, val);
#endif
    }
}

void FunctionDYCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//            else if (type == OP_DY)
//            {
//                float v0 = i.GetFloat(this, 0);
//                float decal = i.GetFloat(this, 1);
//                CCoordinate newcoord(coord.x_, coord.y_+decal, coord.z_, coord.w_, coord.u_, coord.v_);
//                newcoord.dimension_ = coord.dimension_;
//                float v1 = EvaluateInstruction(i.GetSrcID(0),  newcoord);
//                val = (v0-v1) / decal;
//            }
}

void FunctionDYValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{

}

void FunctionDZCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//            else if (type == OP_DZ)
//            {
//                float v0 = i.GetFloat(this, 0);
//                float decal = i.GetFloat(this, 1);
//                CCoordinate newcoord(coord.x_, coord.y_, coord.z_+decal, coord.w_, coord.u_, coord.v_);
//                newcoord.dimension_ = coord.dimension_;
//                float v1 = EvaluateInstruction(i.GetSrcID(0),  newcoord);
//                val = (v0-v1) / decal;
//            }
}

void FunctionDZValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
}

void FunctionDWCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//            else if (type == OP_DW)
//            {
//                float v0 = i.GetFloat(this, 0);
//                float decal = i.GetFloat(this, 1);
//                CCoordinate newcoord(coord.x_, coord.y_, coord.z_, coord.w_+decal, coord.u_, coord.v_);
//                newcoord.dimension_ = coord.dimension_;
//                float v1 = EvaluateInstruction(i.GetSrcID(0),  newcoord);
//                val = (v0-v1) / decal;
//            }
}

void FunctionDWValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
}

void FunctionDUCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//            else if (type == OP_DU)
//            {
//                float v0 = i.GetFloat(this, 0);
//                float decal = i.GetFloat(this, 1);
//                CCoordinate newcoord(coord.x_, coord.y_, coord.z_, coord.w_, coord.u_+decal, coord.v_);
//                newcoord.dimension_ = coord.dimension_;
//                float v1 = EvaluateInstruction(i.GetSrcID(0),  newcoord);
//                val = (v0-v1) / decal;
//            }
}

void FunctionDUValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
}

void FunctionDVCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//            else if (type == OP_DV)
//            {
//                float v0 = i.GetFloat(this, 0);
//                float decal = i.GetFloat(this, 1);
//                CCoordinate newcoord(coord.x_, coord.y_, coord.z_, coord.w_, coord.u_, coord.v_+decal);
//                newcoord.dimension_ = coord.dimension_;
//                float v1 = EvaluateInstruction(i.GetSrcID(0),  newcoord);
//                val = (v0-v1) / decal;
//            }
}

void FunctionDVValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
}


void FunctionFractalCoord(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//            else // OP_Fractal
//            {
//                // parameters : seed, layer, pers, lac, octaves, freq
//                unsigned seed = i.GetUInt(this, 0);
//                unsigned layer = i.GetSrcID(1);
//                float pers = i.GetFloat(this, 2);
//                float lac = i.GetFloat(this, 3);
//                unsigned octaves = i.GetUInt(this, 4);
//                float freq = i.GetFloat(this, 5);
//
//                float amp = 1.f;
//                CCoordinate mycoord(coord);
//                mycoord *= freq;
//                for (unsigned o=0; o < octaves; ++o)
//                {
//                    //SeedSource(layer, seed);
//                    float v = EvaluateInstruction(layer, mycoord);
//                    val += v * amp;
//
//                    amp *= pers;
//                    mycoord *= lac;
//                }
//            }
}

// Value Operations

void FunctionFractalValue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{

}

// Single Sources

void FunctionAbs(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::abs(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionSin(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::sin(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionCos(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::cos(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionTan(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::tan(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionASin(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::asin(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionACos(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::acos(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionATan(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::atan(instruction.GetParamFloat(evaluator, 0, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

// Two Sources

void FunctionAdd(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = instruction.GetParamFloat(evaluator, 0, ilayer) + instruction.GetParamFloat(evaluator, 1, ilayer);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionSubstract(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float v0 = instruction.GetParamFloat(evaluator, 0, ilayer);
        float v1 = instruction.GetParamFloat(evaluator, 1, ilayer);
        float val =  v0 - v1;
//        #ifdef USE_CACHESTAT
//        if (DebugV2) URHO3D_LOGINFOF("FunctionSubstract: ilayer=%u v0=%f v1=%f v0-v1=%f", ilayer, v0, v1, val);
//        #endif
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionMultiply(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float term0 = instruction.GetParamFloat(evaluator, 0, ilayer);
        float term1 = instruction.GetParamFloat(evaluator, 1, ilayer);
        float val = term0 * term1;
        evaluator->PushCachedDataValue(index, val);
//        #ifdef USE_CACHESTAT
//        if (DebugV2) URHO3D_LOGINFOF("FunctionMultiply: %f x %f = %f", term0, term1, val);
//        #endif
    }
}

void FunctionDivide(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = instruction.GetParamFloat(evaluator, 0, ilayer) / instruction.GetParamFloat(evaluator, 1, ilayer);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionPow(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::pow(instruction.GetParamFloat(evaluator, 0, ilayer), instruction.GetParamFloat(evaluator, 1, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionMin(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::min(instruction.GetParamFloat(evaluator, 0, ilayer), instruction.GetParamFloat(evaluator, 1, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionMax(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = std::max(instruction.GetParamFloat(evaluator, 0, ilayer), instruction.GetParamFloat(evaluator, 1, ilayer));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionBias(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float v1 = std::max(0.f, std::min(1.f, instruction.GetParamFloat(evaluator, 0, ilayer)));
        float v2 = std::max(0.f, std::min(1.f, instruction.GetParamFloat(evaluator, 1, ilayer)));
        float val = bias(v1, v2);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionGain(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float v1 = std::max(0.f, std::min(1.f, instruction.GetParamFloat(evaluator, 0, ilayer)));
        float v2 = std::max(0.f, std::min(1.f, instruction.GetParamFloat(evaluator, 1, ilayer)));
        float val = gain(v1, v2);
        evaluator->PushCachedDataValue(index, val);
    }
}

// Multi Sources

void FunctionSigmoid(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float s0 = instruction.GetParamFloat(evaluator, 0, ilayer);
        float s1 = instruction.GetParamFloat(evaluator, 1, ilayer);
        float s2 = -instruction.GetParamFloat(evaluator, 2, ilayer);
        float val = 1.f / (1.f + std::exp(s2*s0 - s1));
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionRandomize(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    KISS rnd;
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        rnd.setSeed(instruction.GetParamUInt(evaluator, 0, ilayer));
        float low = instruction.GetParamFloat(evaluator, 1, ilayer);
        float high = instruction.GetParamFloat(evaluator, 2, ilayer);
        float val = low + rnd.get01()*(high-low);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionCurveSection(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float control = instruction.GetParamFloat(evaluator, 5, ilayer);
        float t0 = instruction.GetParamFloat(evaluator, 1, ilayer);
        float val;

        if (control < t0)
            val = instruction.GetParamFloat(evaluator, 0, ilayer);
        else
        {
            float interp = (control-t0) / (instruction.GetParamFloat(evaluator, 2, ilayer)-t0);
            interp = std::min(1.f, std::max(0.f, quinticInterp(interp)));
            t0 = instruction.GetParamFloat(evaluator, 3, ilayer);
            control = instruction.GetParamFloat(evaluator, 4, ilayer);
            val = t0 + (control-t0)*interp;
        }
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionHexTile(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        const CCoordinate& coord = evaluator->GetCachedCoords(index)[ilayer];
        TileCoord tile = calcHexPointTile(coord.x_, coord.y_);
        unsigned hash = hash_coords_2(tile.x, tile.y, instruction.GetParamUInt(evaluator, 0, ilayer));
        float val = (float)hash/255.f;
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionHexBump(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        const CCoordinate& coord = evaluator->GetCachedCoords(index)[ilayer];
        TileCoord tile = calcHexPointTile(coord.x_, coord.y_);
        CoordPair center = calcHexTileCenter(tile.x, tile.y);
        float dx = coord.x_-center.x;
        float dy = coord.y_-center.y;
        evaluator->PushCachedDataValue(index, hex_function(dx,dy));
    }
}

void FunctionClamp(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float s0 = instruction.GetParamFloat(evaluator, 0, ilayer);
        float s1 = instruction.GetParamFloat(evaluator, 1, ilayer);
        float s2 = instruction.GetParamFloat(evaluator, 2, ilayer);
        evaluator->PushCachedDataValue(index, std::max(s0, std::min(s1, s2)));
    }
}

void FunctionBlend(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float src0 = instruction.GetParamFloat(evaluator, 0, ilayer);
        float src1 = instruction.GetParamFloat(evaluator, 1, ilayer);
        float control = instruction.GetParamFloat(evaluator, 2, ilayer);
        float val = src0*(1.f-control) + src1*control;
        evaluator->PushCachedDataValue(index, val);
//        #ifdef USE_CACHESTAT
//        if (DebugV2) URHO3D_LOGINFOF("FunctionBlend: blend(%f,%f,%f) = %f", src0, src1, control, val);
//        #endif
    }
}

void FunctionSelect(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float control = instruction.GetParamFloat(evaluator, 2, ilayer);
        float threshold = instruction.GetParamFloat(evaluator, 3, ilayer);
        float falloff = instruction.GetParamFloat(evaluator, 4, ilayer);
        float val;

        if (falloff > 0.f)
        {
            if (control < (threshold-falloff))
                val = instruction.GetParamFloat(evaluator, 0, ilayer);
            else if(control > (threshold+falloff))
                val = instruction.GetParamFloat(evaluator, 1, ilayer);
            else
            {
                float low = instruction.GetParamFloat(evaluator, 0, ilayer);
                float high = instruction.GetParamFloat(evaluator, 1, ilayer);
                float blend = quinticInterp((control-threshold+falloff)/(2.f*falloff));
                val = low + (high-low)*blend;
            }
        }
        else
            val = control < threshold ? instruction.GetParamFloat(evaluator, 0, ilayer) : instruction.GetParamFloat(evaluator, 1, ilayer);

        evaluator->PushCachedDataValue(index, val);
    }
}

// Filter

void FunctionSmoothStep(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float low = instruction.GetParamFloat(evaluator, 0, ilayer);
        float high = instruction.GetParamFloat(evaluator, 1, ilayer);
        float control = instruction.GetParamFloat(evaluator, 2, ilayer);
        float t = std::min(1.f, std::max(0.f, (control - low)/(high - low)));
        evaluator->PushCachedDataValue(index, hermiteInterp(t));
    }
}

void FunctionSmootherStep(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float low = instruction.GetParamFloat(evaluator, 0, ilayer);
        float high = instruction.GetParamFloat(evaluator, 1, ilayer);
        float control = instruction.GetParamFloat(evaluator, 2, ilayer);
        float t = std::min(1.f, std::max(0.f, (control - low)/(high - low)));
        evaluator->PushCachedDataValue(index, quinticInterp(t));
    }
}

void FunctionLinearStep(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float low = instruction.GetParamFloat(evaluator, 0, ilayer);
        float t = (instruction.GetParamFloat(evaluator, 2, ilayer) - low)/(instruction.GetParamFloat(evaluator, 1, ilayer) - low);
        evaluator->PushCachedDataValue(index, std::min(1.f, std::max(0.f, t)));
    }
}

void FunctionStep(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    /// TODO : Inversion ?
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float val = instruction.GetParamFloat(evaluator, 1, ilayer) < instruction.GetParamFloat(evaluator, 0, ilayer) ? 0.f : 1.f;
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionTiers(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float numsteps = (int)instruction.GetParamFloat(evaluator, 1, ilayer);
        float val = (std::floor(instruction.GetParamFloat(evaluator, 0, ilayer) * numsteps) / numsteps);
        evaluator->PushCachedDataValue(index, val);
    }
}

void FunctionSmoothTiers(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
    evaluator->ClearCachedDataValues(index);
    unsigned numlayers = evaluator->GetCachedCoords(index).size();
    for (unsigned ilayer=0; ilayer < numlayers; ++ilayer)
    {
        float numsteps = (int)instruction.GetParamFloat(evaluator, 1, ilayer) - 1;
        float v = instruction.GetParamFloat(evaluator, 0, ilayer);
        float Tb = std::floor(v*numsteps);
        float Tt = Tb+1.f;
        float t = quinticInterp(v*numsteps-Tb);
        Tb /= numsteps;
        Tt /= numsteps;
        evaluator->PushCachedDataValue(index, Tb+t*(Tt-Tb));
    }
}

// RGBA operations

void FunctionColor(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    evaluated_[index] = true;
//    cache.set(i.outrgba_);
}

void FunctionExtractRed(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    const SRGBA& c = EvaluateRGBA(i.sources_[0], coord);
//    cache.set(c.r);
//    evaluated_[index] = true;
}

void FunctionExtractGreen(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    const SRGBA& c = EvaluateRGBA(i.sources_[0], coord);
//    cache.set(c.g);
//    evaluated_[index] = true;
}

void FunctionExtractBlue(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    const SRGBA& c = EvaluateRGBA(i.sources_[0], coord);
//    cache.set(c.b);
//    evaluated_[index] = true;
}

void FunctionExtractAlpha(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    const SRGBA& c = EvaluateRGBA(i.sources_[0], coord);
//    cache.set(c.a);
//    evaluated_[index] = true;
}

void FunctionGrayscale(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    float v = EvaluateParameter(i.sources_[0], coord);
//    cache.set(v);
//    evaluated_[index] = true;
}

void FunctionCombineRGBA(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    case OP_CombineRGBA:
//    {
//        float r = EvaluateParameter(i.sources_[0], coord);
//        float g = EvaluateParameter(i.sources_[1], coord);
//        float b = EvaluateParameter(i.sources_[2], coord);
//        float a = EvaluateParameter(i.sources_[3], coord);
//        cache.set(SRGBA(r,g,b,a));
//        evaluated_[index] = true;
//    }
}

void FunctionCombineHSVA(Evaluator* evaluator, unsigned index, const Instruction& instruction)
{
//    case OP_CombineHSVA:
//    {
//        float h = EvaluateParameter(i.sources_[0], coord);
//        float s = EvaluateParameter(i.sources_[1], coord);
//        float v = EvaluateParameter(i.sources_[2], coord);
//        float a = EvaluateParameter(i.sources_[3], coord);
//        SRGBA col;
//
//        float P,Q,T,fract;
//        if (h>=360.f) h=0.f;
//        else h=h/60.f;
//        fract = h - std::floor(h);
//
//        P = v*(1.f-s);
//        Q = v*(1.f-s*fract);
//        T = v*(1.f-s*(1.f-fract));
//
//        if (h>=0 and h<1) col=SRGBA(v,T,P,1);
//        else if (h>=1 and h<2) col=SRGBA(Q,v,P,a);
//        else if (h>=2 and h<3) col=SRGBA(P,v,T,a);
//        else if (h>=3 and h<4) col=SRGBA(P,Q,v,a);
//        else if (h>=4 and h<5) col=SRGBA(T,P,v,a);
//        else if (h>=5 and h<6) col=SRGBA(v,P,Q,a);
//        else col=SRGBA(0,0,0,a);
//
//        cache.set(col);
//        evaluated_[index] = true;
//    }
}

InstructionFunction instructionFunctions_[OP_NumFunctions][2] =
{
    {&FunctionEmpty, &FunctionEmpty},
    {&FunctionSeed, &FunctionEmpty},
    {&FunctionEmpty, &FunctionEmpty},
    {&FunctionSeeder, &FunctionEmpty},
    {&FunctionCacheArray, &FunctionEmpty},

// Basis : index 5 to 15
    {&FunctionValueBasis, &FunctionEmpty},
    {&FunctionGradientBasis, &FunctionEmpty},
    {&FunctionSimplexBasis, &FunctionEmpty},
    {&FunctionCellularBasis, &FunctionEmpty},
    {&FunctionX, &FunctionEmpty},
    {&FunctionY, &FunctionEmpty},
    {&FunctionZ, &FunctionEmpty},
    {&FunctionRadial, &FunctionEmpty},
    {&FunctionW, &FunctionEmpty},
    {&FunctionU, &FunctionEmpty},
    {&FunctionV, &FunctionEmpty},

// Coordinates Transformations : index 16 to 37
    {&FunctionEmpty, &FunctionScaleDomain},
    {&FunctionEmpty, &FunctionScaleX},
    {&FunctionEmpty, &FunctionScaleY},
    {&FunctionEmpty, &FunctionScaleZ},
    {&FunctionEmpty, &FunctionScaleW},
    {&FunctionEmpty, &FunctionScaleU},
    {&FunctionEmpty, &FunctionScaleV},

    {&FunctionEmpty, &FunctionTranslateDomain},
    {&FunctionEmpty, &FunctionTranslateX},
    {&FunctionEmpty, &FunctionTranslateY},
    {&FunctionEmpty, &FunctionTranslateZ},
    {&FunctionEmpty, &FunctionTranslateW},
    {&FunctionEmpty, &FunctionTranslateU},
    {&FunctionEmpty, &FunctionTranslateV},

    {&FunctionEmpty, &FunctionRotateDomain},
    {&FunctionDXValue, &FunctionDXCoord},
    {&FunctionDYValue, &FunctionDYCoord},
    {&FunctionDZValue, &FunctionDZCoord},
    {&FunctionDWValue, &FunctionDWCoord},
    {&FunctionDUValue, &FunctionDUCoord},
    {&FunctionDVValue, &FunctionDVCoord},
    {&FunctionFractalValue, &FunctionFractalCoord},

// Value Operations
    // Single Sources : index 38 to 44
    {&FunctionAbs, &FunctionEmpty},
    {&FunctionSin, &FunctionEmpty},
    {&FunctionCos, &FunctionEmpty},
    {&FunctionTan, &FunctionEmpty},
    {&FunctionASin, &FunctionEmpty},
    {&FunctionACos, &FunctionEmpty},
    {&FunctionATan, &FunctionEmpty},
    // Two Sources : index 45 to 53
    {&FunctionAdd, &FunctionEmpty},
    {&FunctionSubstract, &FunctionEmpty},
    {&FunctionMultiply, &FunctionEmpty},
    {&FunctionDivide, &FunctionEmpty},
    {&FunctionPow, &FunctionEmpty},
    {&FunctionMin, &FunctionEmpty},
    {&FunctionMax, &FunctionEmpty},
    {&FunctionBias, &FunctionEmpty},
    {&FunctionGain, &FunctionEmpty},
    // Multi Sources : index 54 to 61
    {&FunctionSigmoid, &FunctionEmpty},
    {&FunctionRandomize, &FunctionEmpty},
    {&FunctionCurveSection, &FunctionEmpty},
    {&FunctionHexTile, &FunctionEmpty},
    {&FunctionHexBump, &FunctionEmpty},
    {&FunctionClamp, &FunctionEmpty},
    {&FunctionBlend, &FunctionEmpty},
    {&FunctionSelect, &FunctionEmpty},
    // Filter : index 62 to 67
    {&FunctionSmoothStep, &FunctionEmpty},
    {&FunctionSmootherStep, &FunctionEmpty},
    {&FunctionLinearStep, &FunctionEmpty},
    {&FunctionStep, &FunctionEmpty},
    {&FunctionTiers, &FunctionEmpty},
    {&FunctionSmoothTiers, &FunctionEmpty},
    // RGBA operations : index 68 to 75
    {&FunctionColor, &FunctionEmpty},
    {&FunctionExtractRed, &FunctionEmpty},
    {&FunctionExtractGreen, &FunctionEmpty},
    {&FunctionExtractBlue, &FunctionEmpty},
    {&FunctionExtractAlpha, &FunctionEmpty},
    {&FunctionGrayscale, &FunctionEmpty},
    {&FunctionCombineRGBA, &FunctionEmpty},
    {&FunctionCombineHSVA, &FunctionEmpty},
// BuiltIn Macros : index 76 to 79
    {&FunctionEmpty, &FunctionEmpty},
    {&FunctionEmpty, &FunctionEmpty},
    {&FunctionEmpty, &FunctionEmpty},
    {&FunctionEmpty, &FunctionEmpty},
};


};
