#ifndef VM_H
#define VM_H


#include <Urho3D/Container/Str.h>

#include "instruction.h"

#include "../VCommon/noise_gen.h"
#include "../VCommon/utility.h"


using namespace Urho3D;


namespace anl
{

extern bool DebugVM;

struct SVMOutput
{
    SVMOutput()
    {
    }

    SVMOutput(float v) : outfloat_(v), outrgba_(v, v, v, 1.f)
    {
    }

    SVMOutput(float v, const SRGBA& rgba)
    {
        outfloat_ = v;
        outrgba_ = rgba;
    }

    SVMOutput(const SVMOutput &rhs) : outfloat_(rhs.outfloat_), outrgba_(rhs.outrgba_)
    {
    }

    void set(float v)
    {
        outfloat_ = v;
        outrgba_.r = outrgba_.g = outrgba_.b = v;
        outrgba_.a = 1.f;
    }

    void set(const SRGBA& v)
    {
        outrgba_ = v;
        outfloat_ = 0.2126f*v.r + 0.7152f*v.g + 0.0722f*v.b;
    }

    SVMOutput operator-(const SVMOutput &rhs) const
    {
        return SVMOutput(outfloat_-rhs.outfloat_, outrgba_-rhs.outrgba_);
    }

    SVMOutput operator+(const SVMOutput &rhs) const
    {
        return SVMOutput(outfloat_+rhs.outfloat_, outrgba_+rhs.outrgba_);
    }

    SVMOutput operator*(const SVMOutput &rhs) const
    {
        return SVMOutput(outfloat_*rhs.outfloat_, outrgba_*rhs.outrgba_);
    }

    SVMOutput operator/(const SVMOutput &rhs) const
    {
        return SVMOutput(outfloat_/rhs.outfloat_, outrgba_/rhs.outrgba_);
    }

    SVMOutput operator*(float rhs) const
    {
        return SVMOutput(outfloat_*rhs, outrgba_*rhs);
    }

    void set(const SVMOutput &rhs)
    {
        outfloat_ = rhs.outfloat_;
        outrgba_ = rhs.outrgba_;
    }

    static const SVMOutput ZERO;

    float outfloat_;
    SRGBA outrgba_;
};

typedef Vector<bool > EvaluatedType;
typedef Vector<CCoordinate > CoordCacheType;
typedef Vector<SVMOutput > CacheType;


struct CacheMap
{
    CacheMap() : cachesize_(0), cacheheight_(0), cachewidth_(0) {}

    void ResizeCaches(unsigned w, unsigned h, unsigned numaccessors, unsigned numinstructs, const Vector<unsigned >& cachedinstructindexes);
    void ResetCacheAccess();

    unsigned cacheheight_, cachewidth_;
    long cachesize_;
    HashMap<unsigned, CacheType > storagemap_;
    // indexed by instructindex
    Vector<CacheType* > storagetable_;
    // indexed accessorid and by instructindex
    Vector<Vector<bool > > cacheavailable_;
};

class CNoiseExecutor
{
public:
    CNoiseExecutor(CKernel &kernel, int slot, unsigned accessorid);
    ~CNoiseExecutor();

    static void ResizeCache(CKernel& kernel, int slot, unsigned width, unsigned height, unsigned numaccessors);
    static CacheMap& GetCache(int slot)
    {
        return cachemap_[slot];
    }

    void SetCacheAccessor(int slot, unsigned imin, unsigned imax);
    void SetCacheAvailable(unsigned iid);

    void ResetCacheAccess(bool resetcacheavailable = false);
    void ResetCacheAccessIndex();

    void NextCacheAccessFastIndex()
    {
        icacheaccess_++;
    }
    unsigned GetCacheAccessIndex() const
    {
        return icacheaccess_;
    }

    unsigned GetNumInstructions() const
    {
        return ilist_.Size();
    }

    void StartEvaluation();

    const SVMOutput& EvaluateAt(unsigned instructindex, const CCoordinate& coord);

private:
    void SeedSource(InstructionListType& kernel, EvaluatedType& evaluated, unsigned index, unsigned &seed);
    float EvaluateParameter(unsigned index, const CCoordinate& coord);
    const SRGBA& EvaluateRGBA(unsigned index, const CCoordinate& coord);
    const SVMOutput& EvaluateBoth(unsigned index, const CCoordinate& coord);
    const SVMOutput& EvaluateInstruction(unsigned index, const CCoordinate& coord);

    CKernel& kernel_;
    InstructionListType& ilist_;

    // one cached value by instruction
    CacheType cache_;
    // cached value is evaluated
    EvaluatedType evaluated_;
    // coordinate of the cached value
    CoordCacheType coordcache_;

    // cacheaccessor indexes
    unsigned accessorid_;
    unsigned icacheaccess_, icacheaccessmin_, icacheaccessmax_;

    // cacheaccessor by instructions
    Vector<SVMOutput* > cacheaccessors_;
    Vector<bool >& cacheavailable_;

    // cachearray by cached instruction
    static CacheMap cachemap_[2];

    // variables used in Evaluation
    float scale_;
    float angle_;
    float ax_;
    float ay_;
    float az_;
    float len_;
    float cosanglem_;
    float sinangle_;
    float spacing_;
};


};
#endif

