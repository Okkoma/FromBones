#ifndef ANLV2_EVALUATOR_H
#define ANLV2_EVALUATOR_H

#include <vector>
#include <map>

#include "Instructions.h"

//#define USE_CACHESTAT


namespace anl
{

extern int DebugV2;
extern const int DebugMaxV2;

typedef std::map<unsigned, std::vector<InstructionData> > InstructionCache;
typedef std::vector<std::vector<InstructionData*> > InstructionCachePtr;
typedef std::vector<std::vector<InstructionValues*> > InstructionValuePtr;
typedef std::vector<std::vector<InstructionCoords*> > InstructionCoordPtr;
typedef std::vector<float> CacheTypeV2;

struct CacheMapV2
{
    CacheMapV2() : cachesize_(0), cacheheight_(0), cachewidth_(0) {}

    void ResizeCaches(unsigned w, unsigned h, unsigned numaccessors, unsigned numinstructs, const std::vector<unsigned>& cachedinstructindexes);

    void ResetCacheAccess();

    unsigned cacheheight_, cachewidth_;
    long cachesize_;
    std::map<unsigned, CacheTypeV2 > storagemap_;
    // indexed by instructindex
    std::vector<CacheTypeV2* > storagetable_;
    // indexed accessorid and by instructindex
    std::vector<std::vector<bool> > cacheavailable_;
};

// Threadable
// Need that all Instructions in the sources be evaluated first, so we can GetCacheValue() from Evaluator
class Evaluator
{
public:
    Evaluator(InstructionList& instructions, int slot, unsigned accessorid);
    ~Evaluator();

    static void ResizeCache(InstructionList& instructions, int slot, unsigned width, unsigned height, unsigned numaccessors);
    static CacheMapV2& GetCache(int slot)
    {
        return cachemap_[slot];
    }

    void SetCacheAccessor(int slot, long imin, long imax);
    void SetCacheAvailable(unsigned iid);

    void ResetCacheAccess(bool resetcacheavailable = false);
    void ResetCacheAccessIndex();

    void NextCacheAccessFastIndex()
    {
        icacheaccess_++;
    }
    long GetCacheAccessIndex() const
    {
        return icacheaccess_;
    }
    unsigned GetAccessorId() const
    {
        return accessorid_;
    }
    bool GetCacheAvailable(unsigned index) const
    {
        return cacheavailable_[index];
    }
    float* GetCacheAccessor(unsigned index) const
    {
        return cacheaccessors_[index];
    }

    InstructionData& GetCachedDatas(unsigned seqrid, unsigned index);
    std::vector<float>& GetCachedValues(int iseq, unsigned iins);
    std::vector<float>& GetCachedValues(unsigned iins);
    float GetCachedValue(unsigned iins, unsigned ilayer);
    std::vector<CCoordinate>& GetCachedCoords(int iseq, unsigned iins);
    std::vector<CCoordinate>& GetCachedCoords(unsigned iins);
    CRotMatrix& GetCachedRotMatrix(unsigned iins);
    void SetCoordReady(unsigned iins, bool state=true);
    bool HasCoordReady(unsigned iins);

    void ClearCachedDataValues(unsigned iins);
    void ClearCachedDataCoords(unsigned iins);
    void PushCachedDataValue(unsigned iins, float value);
    void PushCachedDataCoord(unsigned iins, const CCoordinate &coord);

    unsigned GetNumInstructions() const
    {
        return instructionList_.GetNumInstructions();
    }
    unsigned GetNumSequences() const
    {
        return currentTree_->sequences_.size();
    }

    // Evaluate all values for all instructions included in the currenttree
    void StartEvaluation(InstructionTree* tree=0);
    float EvaluateAt(const CCoordinate& coord);

private :
    void ResetCacheSequence(const Sequence& sequence);
    void EvaluateSequence(int cmd, Sequence& sequence, const CCoordinate& origincoord=CCoordinate::EMPTY);
    bool EvaluateCoords(unsigned instructionindex);
    bool EvaluateValues(unsigned instructionindex);

    // all instructions of the evaluator
    InstructionList& instructionList_;
    InstructionTree* currentTree_;
    unsigned currSequenceId_;

    // cached value/coord for the instructions of a sequence
    InstructionCache cacheseqdata_;
    InstructionValuePtr cachevalueaccess_;
    InstructionCoordPtr cachecoordaccess_;

    // sequences status (used for skip sequence if its root sequence has a cachearray available)
    // evaluated at each StartEvaluation
    std::vector<bool> currentSequencesEnabled_;

    // cacheaccessor indexes
    unsigned accessorid_;
    long icacheaccess_;
    long icacheaccessmin_, icacheaccessmax_;
    // cacheaccessor by instructions
    std::vector<float*> cacheaccessors_;
    std::vector<bool>& cacheavailable_;

    // cachearray for cached instructions
    static CacheMapV2 cachemap_[2];
};


};
#endif

