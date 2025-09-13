#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

//#define USE_CACHESTAT

namespace anl
{

bool DebugVM = true;

static const char* ANLModuleTypeVMStr[] =
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
    "Mix", // OP_Blend
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


// VM reconstruction with no recursive evaluateInstruction function

const SVMOutput SVMOutput::ZERO = SVMOutput(0.f);


/// CacheMap : for CacheArray

void CacheMap::ResizeCaches(unsigned w, unsigned h, unsigned numaccessors, unsigned numinstructs, const Vector<unsigned >& cachedinstrucindexes)
{
    if (w == cachewidth_ && h == cacheheight_ && numinstructs == storagetable_.Size() && cacheavailable_.Size() == numaccessors)
        return;

    cachewidth_ = h;
    cacheheight_ = w;
    cachesize_ = w*h;

    // reset the pointer table to storage map
    if (storagetable_.Size() != numinstructs)
        storagetable_.Resize(numinstructs);

    for (unsigned i=0; i < numinstructs; i++)
        storagetable_[i] = 0;

    // resize the storage map
    for (unsigned i=0; i < cachedinstrucindexes.Size(); i++)
    {
        unsigned index = cachedinstrucindexes[i];
        CacheType& cachestorage = storagemap_[index];
        cachestorage.Resize(cachesize_);

        // update pointer table for this storage
        storagetable_[index] = &cachestorage;
    }

    // set the accessor table
    if (cacheavailable_.Size() != numaccessors)
        cacheavailable_.Resize(numaccessors);

    for (unsigned i=0; i < numaccessors; i++)
    {
        if (cacheavailable_[i].Size() != storagetable_.Size())
            cacheavailable_[i].Resize(storagetable_.Size());
    }

    URHO3D_LOGINFOF("CacheMap : ResizeCaches w=%u h=%u numaccessors=%u numinstructs=%u numcachedinstructs=%u",
                    w, h, numaccessors, storagetable_.Size(), cachedinstrucindexes.Size());
}

void CacheMap::ResetCacheAccess()
{
    for (unsigned i=0; i < cacheavailable_.Size(); i++)
        for (unsigned j=0; j < cacheavailable_[i].Size(); j++)
            cacheavailable_[i][j] = false;

    URHO3D_LOGWARNINGF("CacheMap : ResetCacheAccess numaccessors=%u numinstructs=%u", cacheavailable_.Size(), cacheavailable_[0].Size());
}


CacheMap CNoiseExecutor::cachemap_[2];

CNoiseExecutor::CNoiseExecutor(CKernel &kernel, int slot, unsigned accessorid) :
    kernel_(kernel),
    ilist_(kernel.getInstructions()),
    cacheavailable_(cachemap_[slot].cacheavailable_[accessorid]),
    accessorid_(accessorid) { }

CNoiseExecutor::~CNoiseExecutor() { }

/// TODO
void CNoiseExecutor::SeedSource(InstructionListType& kernel, EvaluatedType& evaluated, unsigned int index, unsigned int& seed)
{
    //std::cout << "Seed: " << seed << std::endl;
    SInstruction& instruct = kernel[index];
    evaluated[index] = false;

    if (instruct.opcode_==OP_Seed)
    {
        instruct.outfloat_=(float)seed++;
    }
    else
    {
        switch(instruct.opcode_)
        {
        case OP_NOP:
        case OP_Seed:
        case OP_Constant:
            break;
        case OP_Seeder:
            for(int c=0; c<2; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_ValueBasis:
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_GradientBasis:
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_SimplexBasis:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            break;
        case OP_CellularBasis:
            for(int c=0; c<10; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_Add:
        case OP_Subtract:
        case OP_Multiply:
        case OP_Divide:
        case OP_ScaleDomain:
        case OP_ScaleX:
        case OP_ScaleY:
        case OP_ScaleZ:
        case OP_ScaleW:
        case OP_ScaleU:
        case OP_ScaleV:
        case OP_TranslateDomain:
        case OP_TranslateX:
        case OP_TranslateY:
        case OP_TranslateZ:
        case OP_TranslateW:
        case OP_TranslateU:
        case OP_TranslateV:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_RotateDomain:
            for(int c=0; c<5; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_Blend:
            for(int c=0; c<3; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_Select:
            for(int c=0; c<5; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_Min:
        case OP_Max:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_Abs:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            break;
        case OP_Pow:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_Clamp:
            for(int c=0; c<3; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_Radial:
            break;
        case OP_Sin:
        case OP_Cos:
        case OP_Tan:
        case OP_ASin:
        case OP_ACos:
        case OP_ATan:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            break;
        case OP_Bias:
        case OP_Gain:
        case OP_Tiers:
        case OP_SmoothTiers:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_X:
        case OP_Y:
        case OP_Z:
        case OP_W:
        case OP_U:
        case OP_V:
            break;
        case OP_DX:
        case OP_DY:
        case OP_DZ:
        case OP_DW:
        case OP_DU:
        case OP_DV:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            SeedSource(kernel, evaluated, instruct.sources_[1], seed);
            break;
        case OP_Sigmoid:
            for(int c=0; c<3; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        case OP_Fractal:
            for(int c=0; c<6; ++c) SeedSource(kernel,evaluated,instruct.sources_[c],seed);
            break;//instruct.outfloat_=(float)seed++; return; break;
        case OP_Randomize:
        case OP_SmoothStep:
        case OP_SmootherStep:
        case OP_LinearStep:
            for(int c=0; c<3; ++c) SeedSource(kernel,evaluated,instruct.sources_[c],seed);
            break;
        case OP_Step:
            for(int c=0; c<2; ++c) SeedSource(kernel,evaluated,instruct.sources_[c],seed);
            break;
        case OP_CurveSection:
            for(int c=0; c<6; ++c) SeedSource(kernel,evaluated,instruct.sources_[c],seed);
            break;
        case OP_HexTile:
            SeedSource(kernel, evaluated, instruct.sources_[0], seed);
            break;
        case OP_HexBump:
            break;
        case OP_Color:
            break;
        case OP_ExtractRed:
        case OP_ExtractGreen:
        case OP_ExtractBlue:
        case OP_ExtractAlpha:
        case OP_Grayscale:
            break;
        case OP_CombineRGBA:
        case OP_CombineHSVA:
            for(int c=0; c<4; ++c) SeedSource(kernel, evaluated, instruct.sources_[c], seed);
            break;
        default:
            break;
        }
    }
}

void CNoiseExecutor::ResizeCache(CKernel& kernel, int slot, unsigned width, unsigned height, unsigned numaccessors)
{
    cachemap_[slot].ResizeCaches(width, height, numaccessors, kernel.getKernel()->Size(), kernel.getCachedIndexes());

//	URHO3D_LOGINFOF("CNoiseExecutor : ResizeCache : size=%d numaccessors=%u", (long)width*height, numaccessors);
}

void CNoiseExecutor::SetCacheAccessor(int slot, unsigned imin, unsigned imax)
{
    if (cacheaccessors_.Size() < ilist_.Size())
    {
        unsigned size = ilist_.Size();
        cacheaccessors_.Resize(size);
        for (unsigned i=0; i < size; i++)
            cacheaccessors_[i] = 0;
    }

    icacheaccessmin_ = imin;
    icacheaccessmax_ = imax;

    const Vector<unsigned >& cachedInstructs = kernel_.getCachedIndexes();
    unsigned numcachedinstructs = cachedInstructs.Size();
    for (unsigned i=0; i < numcachedinstructs; i++)
    {
        unsigned index = cachedInstructs[i];
        cacheaccessors_[index] = &(*cachemap_[slot].storagetable_[index])[0];
    }
}

void CNoiseExecutor::SetCacheAvailable(unsigned iid)
{
    cacheavailable_[iid] = true;

//    const std::vector<unsigned>& cachedInstructs = kernel_.getCachedIndexes();
//    for (unsigned i=0; i < cachedInstructs.size(); i++)
//    {
//        if (cachedInstructs[i] <= iid)
//            cacheavailable_[cachedInstructs[i]] = true;
//    }

#ifdef USE_CACHESTAT
    printf("CNoiseExecutor() - SetCacheAvailable : accessorid=%u available=true \n", accessorid_);
#endif
}

void CNoiseExecutor::ResetCacheAccess(bool resetcacheavailable)
{
    if (resetcacheavailable)
    {
        unsigned numinstructs = GetNumInstructions();
        for (unsigned i=0; i < numinstructs; i++)
            cacheavailable_[i] = false;
    }

    ResetCacheAccessIndex();
#ifdef USE_CACHESTAT
    printf("CNoiseExecutor() - ResetCacheAccess : accessorid=%u access=%u resetcacheavailable=%s \n", accessorid_, icacheaccess_, resetcacheavailable ? "true" : "false");
#endif
}

void CNoiseExecutor::ResetCacheAccessIndex()
{
    icacheaccess_ = icacheaccessmin_;
#ifdef USE_CACHESTAT
    printf("CNoiseExecutor() - ResetCacheAccesIndex : accessorid=%u access=%u \n", accessorid_, icacheaccess_);
#endif
}

void CNoiseExecutor::StartEvaluation()
{
    unsigned ksize = ilist_.Size();

    if (ksize != cache_.Size())
    {
        cache_.Resize(ksize);
        evaluated_.Resize(ksize);
        coordcache_.Resize(ksize);
    }

    // clear evaluated flags
    for(unsigned i=0; i < ksize; i++)
        evaluated_[i] = false;

#ifdef USE_CACHESTAT
    printf("CNoiseExecutor() - StartEvaluation : accessorid=%u ResetCacheAccessIndex previous index=%u/%u before reset \n", accessorid_, icacheaccess_, icacheaccessmax_);
#endif

    ResetCacheAccessIndex();
}

const SVMOutput& CNoiseExecutor::EvaluateAt(unsigned int index, const CCoordinate& coord)
{
#ifdef USE_CACHESTAT
    if (DebugVM) printf("CNoiseExecutor() - EvaluateAt : iid=%u coord(%f,%f) : ", index,  coord.x_, coord.y_);
#endif
    const SVMOutput& out = EvaluateInstruction(index, coord);

#ifdef USE_CACHESTAT
    if (DebugVM) printf(" => value=%f \n", out.outfloat_);
#endif

    DebugVM = false;
    return out;
}

float CNoiseExecutor::EvaluateParameter(unsigned int index, const CCoordinate& coord)
{
    return EvaluateInstruction(index, coord).outfloat_;
}

const SRGBA& CNoiseExecutor::EvaluateRGBA(unsigned int index, const CCoordinate& coord)
{
    return EvaluateInstruction(index, coord).outrgba_;
}

const SVMOutput& CNoiseExecutor::EvaluateBoth(unsigned int index, const CCoordinate& coord)
{
    return EvaluateInstruction(index, coord);
}

const SVMOutput& CNoiseExecutor::EvaluateInstruction(unsigned int index, const CCoordinate& coord)
{
    if (index >= ilist_.Size())
        return SVMOutput::ZERO;

    SInstruction& instruct = ilist_[index];
    SVMOutput& cache = cache_[index];

    SVMOutput* cacheaccessor;
    if (index >= cacheaccessors_.Size())
    {
        cacheaccessor = 0;
        URHO3D_LOGERRORF("CNoiseExecutor() - EvaluateInstruction : cacheaccessor Error !");
    }
    else
    {
        cacheaccessor = cacheaccessors_[index];
    }

    if (cacheaccessor) // => opcode == OP_CacheArray
    {
        if (cacheavailable_[index])
        {
#ifdef USE_CACHESTAT
            if (DebugVM) printf("%u(CacheAccGet)=%f > ", index, cache.outfloat_);
#endif
            return cacheaccessor[icacheaccess_];
        }

        if (!(evaluated_[index] && coordcache_[index] == coord))
        {
            cache = EvaluateBoth(instruct.sources_[0], coord);

            evaluated_[index] = true;
            cacheaccessor[icacheaccess_] = cache;
        }

#ifdef USE_CACHESTAT
        if (DebugVM) printf("%u(CacheAccSet)=%f > ", index, cache.outfloat_);
#endif
        return cache;
    }


    if (evaluated_[index] && coordcache_[index] == coord)
    {
#ifdef USE_CACHESTAT
        if (DebugVM) printf("%u(Cache)=%f > ", index, cache.outfloat_);
#endif
        return cache;
    }

    coordcache_[index] = coord;

    if (instruct.opcode_ <= OP_CacheArray)
    {
        switch(instruct.opcode_ )
        {
        case OP_NOP:
        case OP_Seed:
        case OP_Constant:
        {
            cache.set(instruct.outfloat_);
            evaluated_[index] = true;
        }
        break;
        case OP_Seeder:
        {
            // Need to iterate through source chain and set seeds based on current seed.
            unsigned seed = (unsigned int)EvaluateParameter(instruct.sources_[0], coord);
            SeedSource(ilist_, evaluated_, instruct.sources_[1], seed);
            cache.set(EvaluateBoth(instruct.sources_[1], coord));
            evaluated_[index] = true;
        }
        break;
        }
    }

    // Basis
    else if (instruct.opcode_ <= OP_V)
    {
        switch(instruct.opcode_)
        {
        case OP_ValueBasis:
            value_noise(coord, (unsigned int)EvaluateParameter(instruct.sources_[1], coord), InterPolationFunctions[Clamp((unsigned int)EvaluateParameter(instruct.sources_[0], coord), 0U, 3U)], &cache.outfloat_);
            cache.set(cache.outfloat_);
            evaluated_[index] = true;
            break;
        case OP_GradientBasis:
            gradient_noise(coord, (unsigned int)EvaluateParameter(instruct.sources_[1], coord), InterPolationFunctions[Clamp((unsigned int)EvaluateParameter(instruct.sources_[0], coord), 0U, 3U)], &cache.outfloat_);
            cache.set(cache.outfloat_);
            evaluated_[index] = true;
            break;
        case OP_SimplexBasis:
            // Simplex noise isn't interpolated, so interp does nothing
            simplex_noise(coord, (unsigned int)EvaluateParameter(instruct.sources_[0], coord), noInterp, &cache.outfloat_);
            cache.set(cache.outfloat_);
            evaluated_[index] = true;
            break;
        case OP_CellularBasis:
        {
            unsigned int dist = Clamp((unsigned int)EvaluateParameter(instruct.sources_[0], coord), 0U, 3U);
            unsigned int seed = (unsigned int)EvaluateParameter(instruct.sources_[9], coord);

            float f[4], d[4];
            if (coord.dimension_ == 2)
                cellular_function2D(coord.x_, coord.y_, seed, f, d, DistanceFunctions2[dist]);
            else if (coord.dimension_ == 3)
                cellular_function3D(coord.x_, coord.y_, coord.z_, seed, f, d, DistanceFunctions3[dist]);
            else if (coord.dimension_ == 4)
                cellular_function4D(coord.x_, coord.y_, coord.z_, coord.w_, seed, f, d, DistanceFunctions4[dist]);
            else
                cellular_function6D(coord.x_, coord.y_, coord.z_, coord.w_, coord.u_, coord.v_, seed, f, d, DistanceFunctions6[dist]);

            float value = 0.f;
            for (unsigned int i=0; i < 4; i++)
            {
                value += EvaluateParameter(instruct.sources_[i+1], coord) * f[i];
                value += EvaluateParameter(instruct.sources_[i+5], coord) * d[i];
            }

            cache.set(value);
            evaluated_[index] = true;
        }
        break;
        case OP_X:
        {
            cache.set(coord.x_);
            evaluated_[index] = true;
        }
        break;
        case OP_Y:
        {
            cache.set(coord.y_);
            evaluated_[index] = true;
        }
        break;
        case OP_Z:
        {
            cache.set(coord.z_);
            evaluated_[index] = true;
        }
        break;
        case OP_Radial:
        {
            if (coord.dimension_ == 2)
                cache.set(Sqrt(coord.x_*coord.x_ + coord.y_*coord.y_));
            else if (coord.dimension_ == 3)
                cache.set(Sqrt(coord.x_*coord.x_ + coord.y_*coord.y_ + coord.z_*coord.z_));
            else if (coord.dimension_ == 4)
                cache.set(Sqrt(coord.x_*coord.x_ + coord.y_*coord.y_ + coord.z_*coord.z_ + coord.w_*coord.w_));
            else
                cache.set(Sqrt(coord.x_*coord.x_ + coord.y_*coord.y_ + coord.z_*coord.z_ + coord.w_*coord.w_ + coord.u_*coord.u_ + coord.v_*coord.v_));

            evaluated_[index] = true;
        }
        break;
        case OP_W:
        {
            cache.set(coord.w_);
            evaluated_[index] = true;
        }
        break;
        case OP_U:
        {
            cache.set(coord.u_);
            evaluated_[index] = true;
        }
        break;
        case OP_V:
        {
            cache.set(coord.v_);
            evaluated_[index] = true;
        }
        break;
        }
    }

    // Coordinate Transformations
    else if (instruct.opcode_ <= OP_Fractal)
    {
        switch(instruct.opcode_)
        {
        case OP_ScaleDomain:
        {
            scale_ = EvaluateParameter(instruct.sources_[1], coord);

            if (coord.dimension_ == 2)
                cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(scale_, scale_, 1.f, 1.f, 1.f, 1.f)));
            else if (coord.dimension_ == 3)
                cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(scale_, scale_, scale_, 1.f, 1.f, 1.f)));
            else if (coord.dimension_ == 4)
                cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(scale_, scale_, scale_, scale_, 1.f, 1.f)));
            else
                cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(scale_, scale_, scale_, scale_, scale_, scale_)));

            evaluated_[index] = true;
        }
        break;
        case OP_ScaleX:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(EvaluateParameter(instruct.sources_[1], coord), 1.f, 1.f, 1.f, 1.f, 1.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_ScaleY:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(1.f, EvaluateParameter(instruct.sources_[1], coord), 1.f, 1.f, 1.f, 1.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_ScaleZ:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(1.f, 1.f, EvaluateParameter(instruct.sources_[1], coord), 1.f, 1.f, 1.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_ScaleW:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(1.f, 1.f, 1.f, EvaluateParameter(instruct.sources_[1], coord), 1.f, 1.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_ScaleU:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(1.f, 1.f, 1.f, 1.f, EvaluateParameter(instruct.sources_[1], coord), 1.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_ScaleV:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord * CCoordinate(1.f, 1.f, 1.f, 1.f, 1.f, EvaluateParameter(instruct.sources_[1], coord))));
            evaluated_[index] = true;
        }
        break;
        case OP_TranslateDomain:
        {
            scale_ = EvaluateParameter(instruct.sources_[1], coord);

            if (coord.dimension_ == 2)
                cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(scale_, scale_, 1.f, 1.f, 1.f, 1.f)));
            else if (coord.dimension_ == 3)
                cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(scale_, scale_, scale_, 1.f, 1.f, 1.f)));
            else if (coord.dimension_ == 4)
                cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(scale_, scale_, scale_, scale_, 1.f, 1.f)));
            else
                cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(scale_, scale_, scale_, scale_, scale_, scale_)));

            evaluated_[index] = true;
        }
        break;
        case OP_TranslateX:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(EvaluateParameter(instruct.sources_[1], coord), 0.f, 0.f, 0.f, 0.f, 0.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_TranslateY:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(0.f, EvaluateParameter(instruct.sources_[1], coord), 0.f, 0.f, 0.f, 0.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_TranslateZ:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(0.f, 0.f, EvaluateParameter(instruct.sources_[1], coord), 0.f, 0.f, 0.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_TranslateW:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(0.f, 0.f, 0.f, EvaluateParameter(instruct.sources_[1], coord), 0.f, 0.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_TranslateU:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(0.f, 0.f, 0.f, 0.f, EvaluateParameter(instruct.sources_[1], coord), 0.f)));
            evaluated_[index] = true;
        }
        break;
        case OP_TranslateV:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord + CCoordinate(0.f, 0.f, 0.f, 0.f, 0.f, EvaluateParameter(instruct.sources_[1], coord))));
            evaluated_[index] = true;
        }
        break;
        case OP_RotateDomain:
        {
            angle_ = EvaluateParameter(instruct.sources_[1], coord);
            ax_ = EvaluateParameter(instruct.sources_[2], coord);
            ay_ = EvaluateParameter(instruct.sources_[3], coord);
            az_ = EvaluateParameter(instruct.sources_[4], coord);

            cosanglem_ = 1.f - cos(angle_);
            sinangle_ = sin(angle_);

            len_ = std::sqrt(ax_*ax_ + ax_*ay_ + az_*az_);
            ax_ /= len_;
            ay_ /= len_;
            az_ /= len_;

            if (coord.dimension_ == 2)
            {
                cache.set(EvaluateBoth(instruct.sources_[0], CCoordinate(
                                           ((1.f + cosanglem_ * (ax_ * ax_ - 1.f)) * coord.x_) + ((-az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.y_),
                                           ((az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.x_) + ((1.f + cosanglem_ * (ay_ * ay_ - 1.f)) * coord.y_))));
            }
            else if (coord.dimension_ == 3)
            {
                cache.set(EvaluateBoth(instruct.sources_[0], CCoordinate(
                                           ((1.f + cosanglem_ * (ax_ * ax_ - 1.f)) * coord.x_) + ((-az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.y_) + ((ay_ * sinangle_ + cosanglem_ * ax_ * az_) * coord.z_),
                                           ((az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.x_) + ((1.f + cosanglem_ * (ay_ * ay_ - 1.f)) * coord.y_) + ((-ax_ * sinangle_ + cosanglem_ * ay_ * az_) * coord.z_),
                                           ((-ay_ * sinangle_ + cosanglem_ * ax_ * az_) * coord.x_) + ((ax_ * sinangle_ + cosanglem_ * ay_ * az_) * coord.y_) + ((1.f + cosanglem_ * (az_ * az_ - 1.f)) * coord.z_))));
            }
            else if (coord.dimension_ == 4)
            {
                cache.set(EvaluateBoth(instruct.sources_[0], CCoordinate(
                                           ((1.f + cosanglem_ * (ax_ * ax_ - 1.f)) * coord.x_) + ((-az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.y_) + ((ay_ * sinangle_ + cosanglem_ * ax_ * az_) * coord.z_),
                                           ((az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.x_) + ((1.f + cosanglem_ * (ay_ * ay_ - 1.f)) * coord.y_) + ((-ax_ * sinangle_ + cosanglem_ * ay_ * az_) * coord.z_),
                                           ((-ay_ * sinangle_ + cosanglem_ * ax_ * az_) * coord.x_) + ((ax_ * sinangle_ + cosanglem_ * ay_ * az_) * coord.y_) + ((1.f + cosanglem_ * (az_ * az_ - 1.f)) * coord.z_),
                                           coord.w_)));
            }
            else
            {
                cache.set(EvaluateBoth(instruct.sources_[0], CCoordinate(
                                           ((1.f + cosanglem_ * (ax_ * ax_ - 1.f)) * coord.x_) + ((-az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.y_) + ((ay_ * sinangle_ + cosanglem_ * ax_ * az_) * coord.z_),
                                           ((az_ * sinangle_ + cosanglem_ * ax_ * ay_) * coord.x_) + ((1.f + cosanglem_ * (ay_ * ay_ - 1.f)) * coord.y_) + ((-ax_ * sinangle_ + cosanglem_ * ay_ * az_) * coord.z_),
                                           ((-ay_ * sinangle_ + cosanglem_ * ax_ * az_) * coord.x_) + ((ax_ * sinangle_ + cosanglem_ * ay_ * az_) * coord.y_) + ((1.f + cosanglem_ * (az_ * az_ - 1.f)) * coord.z_),
                                           coord.w_, coord.u_, coord.v_)));
            }

            evaluated_[index] = true;
        }
        break;
        case OP_DX:
        {
            spacing_ = EvaluateParameter(instruct.sources_[1], coord);

            if (coord.dimension_ == 2)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_ + spacing_, coord.y_))) / spacing_);
            else if (coord.dimension_ == 3)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_ + spacing_, coord.y_, coord.z_))) / spacing_);
            else if (coord.dimension_ == 4)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_ + spacing_, coord.y_, coord.z_, coord.w_))) / spacing_);
            else
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_ + spacing_, coord.y_, coord.z_, coord.w_, coord.u_, coord.v_))) / spacing_);

            evaluated_[index] = true;
        }
        break;
        case OP_DY:
        {
            spacing_ = EvaluateParameter(instruct.sources_[1], coord);

            if (coord.dimension_ == 2)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_ + spacing_))) / spacing_);
            else if (coord.dimension_ == 3)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_ + spacing_, coord.z_))) / spacing_);
            else if (coord.dimension_ == 4)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_ + spacing_, coord.z_, coord.w_))) / spacing_);
            else
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_ + spacing_, coord.z_, coord.w_, coord.u_, coord.v_))) / spacing_);

            evaluated_[index] = true;
        }
        break;
        case OP_DZ:
        {
            spacing_ = EvaluateParameter(instruct.sources_[1], coord);

            if (coord.dimension_ <= 3)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_ + spacing_))) / spacing_);
            else if (coord.dimension_ == 4)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_ + spacing_, coord.w_))) / spacing_);
            else
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_ + spacing_, coord.w_, coord.u_, coord.v_))) / spacing_);

            evaluated_[index] = true;
        }
        break;
        case OP_DW:
        {
            spacing_ = EvaluateParameter(instruct.sources_[1], coord);

            if (coord.dimension_ <= 4)
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_, coord.w_ + spacing_))) / spacing_);
            else
                cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_, coord.w_ + spacing_, coord.u_, coord.v_))) / spacing_);

            evaluated_[index] = true;
        }
        break;
        case OP_DU:
        {
            spacing_ = EvaluateParameter(instruct.sources_[1], coord);

            cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_, coord.w_, coord.u_ + spacing_, coord.v_))) / spacing_);

            evaluated_[index] = true;
        }
        break;
        case OP_DV:
        {
            spacing_ = EvaluateParameter(instruct.sources_[1], coord);

            cache.set((EvaluateParameter(instruct.sources_[0], coord) - EvaluateParameter(instruct.sources_[0], CCoordinate(coord.x_, coord.y_, coord.z_, coord.w_, coord.u_, coord.v_ + spacing_))) / spacing_);

            evaluated_[index] = true;
        }
        break;
        case OP_Fractal:
        {
            //layer,pers,lac,octaves
            const unsigned int numoctaves = (unsigned int)EvaluateParameter(instruct.sources_[4], coord);
            unsigned int seed = (unsigned int)EvaluateParameter(instruct.sources_[0], coord);

            const float pers = EvaluateParameter(instruct.sources_[2], coord);
            const float lac = EvaluateParameter(instruct.sources_[3], coord);
            const float freq = EvaluateParameter(instruct.sources_[5], coord);
            //float amp = 1.f;
            //CCoordinate mycoord = coord*freq;

            cache.outfloat_ = 0.f;
            for(unsigned int c = 0; c < numoctaves; ++c)
            {
                SeedSource(ilist_, evaluated_, instruct.sources_[1], seed);
                //val += EvaluateParameter(instruct.sources_[1], mycoord) * amp;
                cache.outfloat_ += EvaluateParameter(instruct.sources_[1], coord * freq * std::pow(lac, c)) * std::pow(pers, c);
                //amp *= pers;
                //mycoord *= lac;
            }

            cache.set(cache.outfloat_);
            evaluated_[index] = true;
        }
        break;
        }
    }

    // Single Source Value Operations
    else if (instruct.opcode_ <= OP_ATan)
    {
        switch(instruct.opcode_)
        {
        case OP_Abs:
        {
            cache.set(std::abs(EvaluateParameter(instruct.sources_[0], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_Sin:
        {
            cache.set(std::sin(EvaluateParameter(instruct.sources_[0], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_Cos:
        {
            cache.set(std::cos(EvaluateParameter(instruct.sources_[0], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_Tan:
        {
            cache.set(std::tan(EvaluateParameter(instruct.sources_[0], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_ASin:
        {
            cache.set(EvaluateParameter(instruct.sources_[0], coord));
            evaluated_[index] = true;
        }
        break;
        case OP_ACos:
        {
            cache.set(std::acos(EvaluateParameter(instruct.sources_[0], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_ATan:
        {
            cache.set(std::atan(EvaluateParameter(instruct.sources_[0], coord)));
            evaluated_[index] = true;
        }
        break;
        }
    }

    // Single Source Value Operations
    else if (instruct.opcode_ <= OP_Gain)
    {
        switch(instruct.opcode_)
        {
        case OP_Add:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord) + EvaluateBoth(instruct.sources_[1], coord));
            evaluated_[index] = true;
        }
        break;
        case OP_Subtract:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord) - EvaluateBoth(instruct.sources_[1], coord));
            evaluated_[index] = true;
        }
        break;
        case OP_Multiply:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord) * EvaluateBoth(instruct.sources_[1], coord));
            evaluated_[index] = true;
        }
        break;
        case OP_Divide:
        {
            cache.set(EvaluateBoth(instruct.sources_[0], coord) / EvaluateBoth(instruct.sources_[1], coord));
            evaluated_[index] = true;
        }
        break;
        case OP_Pow:
        {
            evaluated_[index] = true;
            cache.set(std::pow(EvaluateParameter(instruct.sources_[0], coord), EvaluateParameter(instruct.sources_[1], coord)));
        }
        break;
        case OP_Min:
        {
            cache.set(std::min(EvaluateParameter(instruct.sources_[0], coord), EvaluateParameter(instruct.sources_[1], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_Max:
        {
            cache.set(std::max(EvaluateParameter(instruct.sources_[0], coord), EvaluateParameter(instruct.sources_[1], coord)));
            evaluated_[index] = true;
        }
        break;
        case OP_Bias:
        {
            cache.set(bias(std::max(0.f, std::min(1.f, EvaluateParameter(instruct.sources_[0], coord))), std::max(0.f, std::min(1.f, EvaluateParameter(instruct.sources_[1], coord)))));
            evaluated_[index] = true;
        }
        break;
        case OP_Gain:
        {
            cache.set(gain(std::max(0.f, std::min(1.f, EvaluateParameter(instruct.sources_[0], coord))), std::max(0.f, std::min(1.f, EvaluateParameter(instruct.sources_[1], coord)))));
            evaluated_[index] = true;
        }
        break;
        }
    }

    else if (instruct.opcode_ <= OP_Select)
    {
        switch(instruct.opcode_)
        {
        case OP_Sigmoid:
        {
            float s = EvaluateParameter(instruct.sources_[0], coord);
            float c = EvaluateParameter(instruct.sources_[1], coord);
            float r = EvaluateParameter(instruct.sources_[2], coord);
            cache.set(1.f / (1.f + std::exp(-r*(s-c))));
            evaluated_[index] = true;
        }
        break;
        case OP_Randomize:  // Randomize a value range based on seed
        {
            unsigned int seed= (unsigned int) EvaluateParameter(instruct.sources_[0], coord);
            KISS rnd;
            rnd.setSeed(seed);
            float low = EvaluateParameter(instruct.sources_[1], coord);
            float high = EvaluateParameter(instruct.sources_[2], coord);
            cache.set(low + rnd.get01() * (high - low));
            evaluated_[index] = true;
        }
        break;
        case OP_CurveSection:
        {
            const SVMOutput& lowv=EvaluateBoth(instruct.sources_[0], coord);
            float control=EvaluateParameter(instruct.sources_[5], coord);
            float t0=EvaluateParameter(instruct.sources_[1], coord);
            float t1=EvaluateParameter(instruct.sources_[2], coord);

            if(control<t0)
            {
                cache.set(lowv);
            }
            else
            {
                float interp=(control-t0)/(t1-t0);
                interp = interp*interp*interp*(interp*(interp*6.f-15.f)+10.f);
                interp = Min(1.f, Max(0.f, interp));
                const SVMOutput& v0=EvaluateBoth(instruct.sources_[3], coord);
                const SVMOutput& v1=EvaluateBoth(instruct.sources_[4], coord);
                cache.set(v0 + (v1-v0)*interp);
            }
            evaluated_[index] = true;
        }
        break;
        case OP_HexTile:
        {
            TileCoord tile=calcHexPointTile(coord.x_, coord.y_);
            unsigned int seed=(unsigned int)EvaluateParameter(instruct.sources_[0], coord);
            unsigned int hash=hash_coords_2(tile.x, tile.y, seed);
            cache.set((float)hash/255.f);
            evaluated_[index] = true;
        }
        break;
        case OP_HexBump:
        {
            TileCoord tile=calcHexPointTile(coord.x_, coord.y_);
            CoordPair center=calcHexTileCenter(tile.x, tile.y);
            float dx=coord.x_-center.x;
            float dy=coord.y_-center.y;
            cache.set(hex_function(dx,dy));
            evaluated_[index] = true;
        }
        break;
        case OP_Clamp:
        {
            float val = EvaluateParameter(instruct.sources_[0], coord);
            float low = EvaluateParameter(instruct.sources_[1], coord);
            float high = EvaluateParameter(instruct.sources_[2], coord);
            cache.set(Max(low, Min(high, val)));
            evaluated_[index] = true;
        }
        break;
        case OP_Blend:
        {
            const SVMOutput& low = EvaluateBoth(instruct.sources_[0], coord);
            const SVMOutput& high = EvaluateBoth(instruct.sources_[1], coord);
            float control = EvaluateParameter(instruct.sources_[2], coord);
            cache.set(low * (1.f-control) + high * control);
            evaluated_[index] = true;
        }
        break;
        case OP_Select:
        {
            float control = EvaluateParameter(instruct.sources_[2], coord);
            float threshold = EvaluateParameter(instruct.sources_[3], coord);
            float falloff = EvaluateParameter(instruct.sources_[4], coord);
            if(falloff>0.f)
            {
                if(control<(threshold-falloff))
                    cache.set(EvaluateBoth(instruct.sources_[0], coord));
                else if(control>(threshold+falloff))
                    cache.set(EvaluateBoth(instruct.sources_[1], coord));
                else
                {
                    SVMOutput low = EvaluateBoth(instruct.sources_[0], coord);
                    SVMOutput high = EvaluateBoth(instruct.sources_[1], coord);
                    float lower = threshold-falloff;
                    float upper = threshold+falloff;
                    float blend = quintic_blend((control-lower)/(upper-lower));
                    cache.set(low+(high-low)*blend);
                }
            }
            else
            {
                if (control<threshold)
                    cache.set(EvaluateBoth(instruct.sources_[0], coord));
                else
                    cache.set(EvaluateBoth(instruct.sources_[1], coord));
            }
            evaluated_[index] = true;
        }
        break;
        }
    }

    else if (instruct.opcode_ <= OP_SmoothTiers)
    {
        switch(instruct.opcode_)
        {
        case OP_SmoothStep:
        {
            float low = EvaluateParameter(instruct.sources_[0], coord);
            float high = EvaluateParameter(instruct.sources_[1], coord);
            float control = EvaluateParameter(instruct.sources_[2], coord);
            float t=Min(1.f, Max(0.f, (control-low)/(high-low)));
            t=t*t*(3.f-2.f*t);
            cache.set(t);
            evaluated_[index] = true;
        }
        break;
        case OP_SmootherStep:
        {
            float low = EvaluateParameter(instruct.sources_[0], coord);
            float high = EvaluateParameter(instruct.sources_[1], coord);
            float control = EvaluateParameter(instruct.sources_[2], coord);
            float t=Min(1.f, Max(0.f, (control-low)/(high-low)));
            t=t*t*t*(t*(t*6 - 15) + 10);
            cache.set(t);
            evaluated_[index] = true;
        }
        break;
        case OP_LinearStep:
        {
            float low = EvaluateParameter(instruct.sources_[0], coord);
            float high = EvaluateParameter(instruct.sources_[1], coord);
            float control = EvaluateParameter(instruct.sources_[2], coord);
            float t = (control-low)/(high-low);
            cache.set(Min(1.f, Max(0.f, t)));
            evaluated_[index] = true;
        }
        break;
        case OP_Step:
        {
            cache.set(EvaluateParameter(instruct.sources_[1], coord) < EvaluateParameter(instruct.sources_[0], coord) ? 0.f : 1.f);
            evaluated_[index] = true;
        }
        break;
        case OP_Tiers:
        {
            float numsteps = (int)EvaluateParameter(instruct.sources_[1], coord);
            float val = EvaluateParameter(instruct.sources_[0], coord);
            float Tb = Floor(val*numsteps);
            evaluated_[index] = true;
            cache.set(Tb/numsteps);
        }
        break;
        case OP_SmoothTiers:
        {
            float numsteps = (int)EvaluateParameter(instruct.sources_[1], coord)-1;
            float val = EvaluateParameter(instruct.sources_[0], coord);
            float Tb = Floor(val*numsteps);
            float Tt = Tb+1.f;
            float t = quintic_blend(val*numsteps-Tb);
            Tb /= numsteps;
            Tt /= numsteps;
            evaluated_[index] = true;
            cache.set(Tb+t*(Tt-Tb));
        }
        break;
        }
    }

    else
    {
        switch(instruct.opcode_)
        {
        case OP_Color:
        {
            evaluated_[index] = true;
            cache.set(instruct.outrgba_);
        }
        break;
        case OP_ExtractRed:
        {
            const SRGBA& c = EvaluateRGBA(instruct.sources_[0], coord);
            cache.set(c.r);
            evaluated_[index] = true;
        }
        break;
        case OP_ExtractGreen:
        {
            const SRGBA& c = EvaluateRGBA(instruct.sources_[0], coord);
            cache.set(c.g);
            evaluated_[index] = true;
        }
        break;
        case OP_ExtractBlue:
        {
            const SRGBA& c = EvaluateRGBA(instruct.sources_[0], coord);
            cache.set(c.b);
            evaluated_[index] = true;
        }
        break;
        case OP_ExtractAlpha:
        {
            const SRGBA& c = EvaluateRGBA(instruct.sources_[0], coord);
            cache.set(c.a);
            evaluated_[index] = true;
        }
        break;
        case OP_Grayscale:
        {
            float v = EvaluateParameter(instruct.sources_[0], coord);
            cache.set(v);
            evaluated_[index] = true;
        }
        break;
        case OP_CombineRGBA:
        {
            float r = EvaluateParameter(instruct.sources_[0], coord);
            float g = EvaluateParameter(instruct.sources_[1], coord);
            float b = EvaluateParameter(instruct.sources_[2], coord);
            float a = EvaluateParameter(instruct.sources_[3], coord);
            cache.set(SRGBA(r,g,b,a));
            evaluated_[index] = true;
        }
        break;
        case OP_CombineHSVA:
        {
            float h = EvaluateParameter(instruct.sources_[0], coord);
            float s = EvaluateParameter(instruct.sources_[1], coord);
            float v = EvaluateParameter(instruct.sources_[2], coord);
            float a = EvaluateParameter(instruct.sources_[3], coord);
            SRGBA col;

            float P,Q,T,fract;
            if (h>=360.f) h=0.f;
            else h=h/60.f;
            fract = h - Floor(h);

            P = v*(1.f-s);
            Q = v*(1.f-s*fract);
            T = v*(1.f-s*(1.f-fract));

            if (h>=0 && h<1) col=SRGBA(v,T,P,1);
            else if (h>=1 && h<2) col=SRGBA(Q,v,P,a);
            else if (h>=2 && h<3) col=SRGBA(P,v,T,a);
            else if (h>=3 && h<4) col=SRGBA(P,Q,v,a);
            else if (h>=4 && h<5) col=SRGBA(T,P,v,a);
            else if (h>=5 && h<6) col=SRGBA(v,P,Q,a);
            else col=SRGBA(0,0,0,a);

            cache.set(col);
            evaluated_[index] = true;
        }
        break;
        }
    }

#ifdef USE_CACHESTAT
    if (DebugVM)
    {
        printf("%u(%s)=%f > ", index, ANLModuleTypeVMStr[instruct.opcode_], cache.outfloat_);
        if (isnan(cache.outfloat_))
            URHO3D_LOGERRORF("CNoiseExecutor() - EvaluateInstruction : NaN Value at coord=%F,%F dim=%d instruction=%u(%s)", coord.x_, coord.y_, coord.dimension_, index, ANLModuleTypeVMStr[instruct.opcode_]);
    }
#endif

    return cache;
}

};
