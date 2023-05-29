#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include <cstdio>
#include <cassert>

#include "Functions.h"

#include "Evaluator.h"

namespace anl
{

int DebugV2 = 0;
const int DebugMaxV2 = 1;

/// CacheMap : for CacheArray

void CacheMapV2::ResizeCaches(unsigned w, unsigned h, unsigned numaccessors, unsigned numinstructs, const std::vector<unsigned>& cachedinstrucindexes)
{
    cachewidth_ = h;
    cacheheight_ = w;
    cachesize_ = w*h;

    if (storagetable_.size() != numinstructs)
        storagetable_.resize(numinstructs);

    for (unsigned i=0; i < numinstructs; i++)
        storagetable_[i] = 0;

    for (unsigned i=0; i < cachedinstrucindexes.size(); i++)
    {
        unsigned index = cachedinstrucindexes[i];
        storagetable_[index] = &(storagemap_[index]);
    }

    for (std::map<unsigned, CacheTypeV2>::iterator cache=storagemap_.begin(); cache!=storagemap_.end(); ++cache)
        cache->second.resize(cachesize_);

    if (cacheavailable_.size() != numaccessors)
        cacheavailable_.resize(numaccessors);

    for (unsigned i=0; i < numaccessors; i++)
    {
        if (cacheavailable_[i].size() != storagetable_.size())
            cacheavailable_[i].resize(storagetable_.size());
    }

    URHO3D_LOGINFOF("CacheMapV2 : ResizeCaches w=%u h=%u numaccessors=%u numinstructs=%u numcachedinstructs=%u",
                    w, h, numaccessors, storagetable_.size(), cachedinstrucindexes.size());
}

void CacheMapV2::ResetCacheAccess()
{
    for (unsigned i=0; i < cacheavailable_.size(); i++)
        for (unsigned j=0; j < cacheavailable_[i].size(); j++)
            cacheavailable_[i][j] = false;

    URHO3D_LOGINFOF("CacheMapV2 : ResetCacheAccess numaccessors=%u numinstructs=%u", cacheavailable_.size(), cacheavailable_[0].size());
}


/// Evaluator


enum
{
    ValueCalculation = 0,
    CoordModifier
};

CacheMapV2 Evaluator::cachemap_[2];

Evaluator::Evaluator(InstructionList& instructions, int slot, unsigned accessorid) :
    instructionList_(instructions),
    currentTree_(0),
    currSequenceId_(0),
    accessorid_(accessorid),
    cacheavailable_(cachemap_[slot].cacheavailable_[accessorid])
{ }

Evaluator::~Evaluator()
{ }

void Evaluator::ResizeCache(InstructionList& kernel, int slot,unsigned width, unsigned height, unsigned numaccessors)
{
    cachemap_[slot].ResizeCaches(width, height, numaccessors, kernel.GetNumInstructions(), kernel.GetCachedInstructions());

    URHO3D_LOGINFOF("Evaluator : ResizeCache : size=%d numaccessors=%u", (long)width*height, numaccessors);
}

void Evaluator::SetCacheAccessor(int slot, long imin, long imax)
{
    if (!cacheaccessors_.size())
    {
        unsigned size = instructionList_.GetNumInstructions();
        cacheaccessors_.resize(size);
        for (unsigned i=0; i < size; i++)
            cacheaccessors_[i] = 0;
    }

    icacheaccessmin_ = imin;
    icacheaccessmax_ = imax;

    const std::vector<unsigned>& cachedInstructs = instructionList_.GetCachedInstructions();
    for (unsigned i=0; i < cachedInstructs.size(); i++)
    {
        unsigned index = cachedInstructs[i];
        cacheaccessors_[index] = &(*cachemap_[slot].storagetable_[index])[0];
    }
}

void Evaluator::SetCacheAvailable(unsigned iid)
{
    const std::vector<unsigned>& cachedInstructs = instructionList_.GetCachedInstructions();
    for (unsigned i=0; i < cachedInstructs.size(); i++)
    {
        if (cachedInstructs[i] <= iid)
            cacheavailable_[cachedInstructs[i]] = true;
    }

#ifdef USE_CACHESTAT
    URHO3D_LOGINFOF("Evaluator() - SetCacheAvailable : accessorid=%u available=true", accessorid_);
#endif
}

void Evaluator::ResetCacheAccess(bool resetcacheavailable)
{
    if (resetcacheavailable)
        for (unsigned i=0; i < GetNumInstructions(); i++)
            cacheavailable_[i] = false;

    ResetCacheAccessIndex();
#ifdef USE_CACHESTAT
    URHO3D_LOGINFOF("Evaluator() - ResetCacheAccess : accessorid=%u access=%d resetcacheavailable=%s", accessorid_, icacheaccess_, resetcacheavailable ? "true" : "false");
#endif
}

void Evaluator::ResetCacheAccessIndex()
{
    icacheaccess_ = icacheaccessmin_;
#ifdef USE_CACHESTAT
    URHO3D_LOGINFOF("Evaluator() - ResetCacheAccesIndex : accessorid=%u access=%d", accessorid_, icacheaccess_);
#endif
}

InstructionData& Evaluator::GetCachedDatas(unsigned seqrid, unsigned index)
{
    return cacheseqdata_[seqrid][index];
}

std::vector<float>& Evaluator::GetCachedValues(int iseq, unsigned iins)
{
    return cachevalueaccess_[iseq][iins]->values_;
}

std::vector<float>& Evaluator::GetCachedValues(unsigned iins)
{
    return cachevalueaccess_[currSequenceId_][iins]->values_;
}

float Evaluator::GetCachedValue(unsigned iins, unsigned ilayer)
{
    std::vector<float>& values = cachevalueaccess_[currSequenceId_][iins]->values_;
    return (ilayer >= values.size()) ? values[0] : values[ilayer];
}

std::vector<CCoordinate>& Evaluator::GetCachedCoords(int iseq, unsigned iins)
{
    return cachecoordaccess_[iseq][iins]->coords_;
}

std::vector<CCoordinate>& Evaluator::GetCachedCoords(unsigned iins)
{
    return cachecoordaccess_[currSequenceId_][iins]->coords_;
}

CRotMatrix& Evaluator::GetCachedRotMatrix(unsigned iins)
{
    return *(cachecoordaccess_[currSequenceId_][iins]->rotmat_);
}

void Evaluator::SetCoordReady(unsigned iins, bool state)
{
    cachecoordaccess_[currSequenceId_][iins]->coordReady_ = state;
}

bool Evaluator::HasCoordReady(unsigned iins)
{
    return cachecoordaccess_[currSequenceId_][iins]->coordReady_;
}


void Evaluator::ClearCachedDataValues(unsigned iins)
{
    GetCachedValues(iins).clear();
}

void Evaluator::ClearCachedDataCoords(unsigned iins)
{
    GetCachedCoords(iins).clear();
}

void Evaluator::PushCachedDataValue(unsigned iins, float value)
{
    GetCachedValues(iins).push_back(value);
}

void Evaluator::PushCachedDataCoord(unsigned iins, const CCoordinate &coord)
{
    GetCachedCoords(iins).push_back(coord);
}


void Evaluator::StartEvaluation(InstructionTree* tree)
{
    if (!tree)
    {
        InstructionTree* maintree = instructionList_.GetMainTree();
        if (currentTree_ != maintree)
            tree = maintree;
    }

    if (currentTree_ != tree)
    {
#ifdef USE_CACHESTAT
        URHO3D_LOGINFOF("Evaluator() - StartEvaluation : setting cachesequences ...");
#endif

        currentTree_ = tree;

        unsigned numsequences = GetNumSequences();

        // allocate cache datas
        for (unsigned iseq=0; iseq < numsequences; ++iseq)
        {
            const Sequence& sequence = currentTree_->sequences_[iseq];

            unsigned numseqinstructs = sequence.GetNumInstructions();
            std::vector<InstructionData>& instructdata = cacheseqdata_[sequence.rid_];
            instructdata.resize(numseqinstructs);
            for (unsigned i=0; i<numseqinstructs; ++i)
            {
                if (instructionList_.GetInstruction(sequence[i]).HasProperty(IC_ROTATION))
                    instructdata[i].c_.AddRotMatrix();

#ifdef USE_CACHESTAT
                URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d instruct=%u coorddata*=%u valuedata*=%u", iseq, sequence.base_, sequence.rid_, sequence.root_, sequence[i],
                                &(instructdata[i].c_), &(instructdata[i].v_));
#endif
            }
        }

        // allocate cache accessors
        cachevalueaccess_.resize(numsequences);
        cachecoordaccess_.resize(numsequences);
        for (unsigned iseq=0; iseq < numsequences; ++iseq)
        {
            unsigned numinstructs = GetNumInstructions();
            std::vector<InstructionValues*>& valueaccessor = cachevalueaccess_[iseq];
            std::vector<InstructionCoords*>& coordaccessor = cachecoordaccess_[iseq];

            valueaccessor.resize(numinstructs);
            coordaccessor.resize(numinstructs);
            for (unsigned i=0; i<numinstructs; i++)
            {
                valueaccessor[i] = 0;
                coordaccessor[i] = 0;
            }
        }

        // set accessors to instruction data following sequence dependency (root, child)
        for (std::vector<SequenceCmd>::const_iterator ct=currentTree_->commands_.begin(); ct!=currentTree_->commands_.end(); ++ct)
        {
            const Sequence& sequence = *ct->seqptr_;
            unsigned iseq = sequence.id_;
            std::vector<InstructionValues*>& valueaccessor = cachevalueaccess_[iseq];
            std::vector<InstructionCoords*>& coordaccessor = cachecoordaccess_[iseq];
            int cmd = ct->cmd_;
            int iend = sequence.GetNumInstructions()-1;

            // set coord accessors
            if (cmd & FUNCCOORD)
            {
                if (sequence.root_ != -1)
                {
                    unsigned iins = sequence.base_;
                    Instruction& instruction = instructionList_.GetInstruction(iins);

                    // Get the root sequence
                    const Sequence* rootseq = currentTree_->GetRootSequence(sequence.rid_);
                    if (!rootseq) URHO3D_LOGERRORF("=> iseq=%u base=%u root=%d error rid=%u!", iseq, iins, sequence.root_, sequence.rid_);

                    // Set coordaccessor for root in this sequence
                    InstructionCoords* ptr = cachecoordaccess_[rootseq->id_][sequence.root_];
                    coordaccessor[sequence.root_] = ptr;

                    // if base has no IC_COORDMODIFIER properties
                    // => set base coordaccessor to root sequence
                    if (!instruction.HasProperty(IC_COORDMODIFIER))
                    {
                        coordaccessor[iins] = ptr;
#ifdef USE_CACHESTAT
                        URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d !IC_COORDMODIFIER => instruction=%u coordaccessor*=%u from root", iseq, sequence.base_, sequence.rid_, sequence.root_, iins, coordaccessor[iins]);
#endif
                    }
                }
                // set coord accessors for the instructions in the sequence
                for (int i=iend; i >= 0; --i)
                {
                    unsigned iins = sequence[i];
                    Instruction& instruction = instructionList_.GetInstruction(iins);
                    const std::vector<unsigned>& sources = instruction.GetSources();

                    if (instruction.HasProperty(IC_COORDMODIFIER) || sequence.root_ == -1)
                    {
                        coordaccessor[iins] = &(cacheseqdata_[sequence.rid_][i].c_);
#ifdef USE_CACHESTAT
                        URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d IC_COORDMODIFIER => instruction=%u coordaccessor*=%u", iseq, sequence.base_, sequence.rid_, sequence.root_, iins, coordaccessor[iins]);
#endif
                    }
                    for (int j=0; j < sources.size(); ++j)
                    {
                        unsigned child = sources[j];
                        // if the child is in the sequence and has no IC_COORDMODIFIER properties
                        // => its coordaccessor is the cordaccessor of the current instruction
                        if (sequence.HasInstruction(child) && !instructionList_.GetInstruction(child).HasProperty(IC_COORDMODIFIER))
                        {
                            coordaccessor[child] = coordaccessor[iins];
#ifdef USE_CACHESTAT
                            URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d !IC_COORDMODIFIER => coordaccessor*=%u on instruction=%u", iseq, sequence.base_, sequence.rid_, sequence.root_, coordaccessor[iins], child);
#endif
                        }
                    }
                }
                // set coord accessors for the child instructions of the sequence
                for (unsigned i=0; i < sequence.childrids_.size(); ++i)
                {
                    unsigned child = sequence.childs_[i];
                    Instruction& childinstruction = instructionList_.GetInstruction(child);

                    if (!childinstruction.HasProperty(IC_COORDMODIFIER))
                    {
                        unsigned childrid = sequence.childrids_[i];
                        const Sequence* childseq = currentTree_->GetSequence(childrid);
                        if (!childseq) URHO3D_LOGERRORF("=> iseq=%u base=%u root=%d  child error rid=%u !", iseq, sequence.base_, sequence.root_, childrid);
                        InstructionCoords* ptr = cachecoordaccess_[sequence.id_][childseq->root_];
                        cachecoordaccess_[childseq->id_][child] = ptr;
                        coordaccessor[child] = ptr;
#ifdef USE_CACHESTAT
                        URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d !IC_COORDMODIFIER => coordaccessor*=%u on child=%u(seqchildid=%u)",
                                        iseq, sequence.base_, sequence.rid_, sequence.root_, cachecoordaccess_[childseq->id_][child], child, childseq->id_);
#endif
                    }
                }
            }
            // set value accessors
            if (cmd & FUNCVALUE)
            {
                // set value accessors for the instructions in the sequence
                for (int i=0; i <= iend; ++i)
                {
                    unsigned iins = sequence[i];
                    Instruction& instruction = instructionList_.GetInstruction(iins);
                    const std::vector<unsigned>& sources = instruction.GetSources();

                    if (!sources.size() || instruction.HasProperty(IC_VALUEMODIFIER))
                    {
                        valueaccessor[iins] = &(cacheseqdata_[sequence.rid_][i].v_);
#ifdef USE_CACHESTAT
                        URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d IC_VALUEMODIFIER => instruction=%u valueaccessor*=%u", iseq, sequence.base_, sequence.rid_, sequence.root_, iins, valueaccessor[iins]);
#endif
                        continue;
                    }

                    // if instruction has no IC_VALUEMODIFIER properties
                    // => set valueaccessor to first source data value
                    unsigned child = sources[0];
                    // source in the sequence
                    if (sequence.HasInstruction(child))
                    {
                        valueaccessor[iins] = valueaccessor[child];
#ifdef USE_CACHESTAT
                        URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d !IC_VALUEMODIFIER => instruction=%u valueaccessor*=%u from child=%u",
                                        iseq, sequence.base_, sequence.rid_, sequence.root_, iins, valueaccessor[iins], child);
#endif
                    }
                    // source in child sequence
                    else
                    {
                        unsigned childrid = instructionList_.GetInstruction(child).HasProperty(IC_CACHEARRAY) ? child : iins+child*1000;
                        const Sequence* childseq = currentTree_->GetSequence(childrid);
                        if (!childseq) URHO3D_LOGERRORF("=> iseq=%u base=%u root=%d  child error rid=%u !", iseq, sequence.base_, sequence.root_, childrid);
                        valueaccessor[iins] = cachevalueaccess_[childseq->id_][child];
#ifdef USE_CACHESTAT
                        URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d !IC_VALUEMODIFIER => instruction=%u valueaccessor*=%u from child=%u(seqchildid=%u)",
                                        iseq, sequence.base_, sequence.rid_, sequence.root_, iins, valueaccessor[iins], child, childseq->id_);
#endif
                    }
                }
                for (unsigned i=0; i < sequence.childrids_.size(); ++i)
                {
                    unsigned child = sequence.childs_[i];
                    Instruction& childinstruction = instructionList_.GetInstruction(child);

                    unsigned childrid = sequence.childrids_[i];
                    const Sequence* childseq = currentTree_->GetSequence(childrid);
                    if (!childseq) URHO3D_LOGERRORF("=> iseq=%u base=%u root=%d  child error rid=%u !", iseq, sequence.base_, sequence.root_, childrid);
                    valueaccessor[child] = cachevalueaccess_[childseq->id_][child];
#ifdef USE_CACHESTAT
                    URHO3D_LOGINFOF("=> iseq=%u base=%u rid=%u root=%d => valueaccessor*=%u from child=%u(seqchildid=%u)",
                                    iseq, sequence.base_, sequence.rid_, sequence.root_, valueaccessor[child], child, childseq->id_);
#endif
                }
            }
        }
    }

    {
        // resize sequence status
        unsigned numsequences = GetNumSequences();
        currentSequencesEnabled_.resize(numsequences);

        // reset all sequences to enable
        for (unsigned iseq=0; iseq < numsequences; ++iseq)
            currentSequencesEnabled_[iseq] = true;

        // if cache is available for a sequence,
        // don't need to recalculate child sequences => disable its
        for (unsigned iseq=0; iseq < numsequences; ++iseq)
        {
            const Sequence& sequence = currentTree_->sequences_[iseq];

            if (instructionList_.GetInstruction(sequence.base_).HasProperty(IC_CACHEARRAY) && cacheavailable_[sequence.base_])
            {
                for (unsigned i=0; i < sequence.childrids_.size(); ++i)
                {
                    const Sequence* childseq = currentTree_->GetSequence(sequence.childrids_[i]);
                    if (childseq)
                        currentSequencesEnabled_[childseq->id_] = false;
                }
            }
        }
    }

#ifdef USE_CACHESTAT
    URHO3D_LOGINFOF("Evaluator() - StartEvaluation : accessorid=%u ResetCacheAccessIndex previous index=%d/%d before reset", accessorid_, icacheaccess_, icacheaccessmax_);
#endif

    ResetCacheAccessIndex();
}

void Evaluator::ResetCacheSequence(const Sequence& sequence)
{
    GetCachedDatas(sequence.rid_, sequence.GetNumInstructions()-1).clear();
}

float Evaluator::EvaluateAt(const CCoordinate& coord)
{
    const std::vector<SequenceCmd>& cmds = currentTree_->commands_;
    const std::vector<Sequence>& seqs = currentTree_->sequences_;

    for (std::vector<Sequence>::const_iterator ct=seqs.begin(); ct!=seqs.end(); ++ct)
        ResetCacheSequence(*ct);

    for (std::vector<SequenceCmd>::const_iterator ct=cmds.begin(); ct!=cmds.end(); ++ct)
        if (currentSequencesEnabled_[ct->seqptr_->id_])
            EvaluateSequence(ct->cmd_, *ct->seqptr_, coord);

    float val = GetCachedValues(currentTree_->index_)[0];

#ifdef USE_CACHESTAT
    if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("Evaluator : EvaluateAt=(%f,%f) val=%f", coord.x_, coord.y_, val);
    DebugV2++;
#endif

    return val;
}

void Evaluator::EvaluateSequence(int cmd, Sequence& sequence, const CCoordinate& origincoord)
{
    if (sequence.instructions_.back() >= GetNumInstructions())
        return;

    currSequenceId_ = sequence.id_;

    unsigned iins;
    int iend = sequence.instructions_.size()-1;
    int ibegin = 0;

    {
        // when cacheavailable, get the nearest cache index in the sequence to start with
        for (int i=0; i < sequence.instructions_.size(); ++i)
        {
            if (cacheavailable_[sequence.instructions_[i]])
                ibegin = i;
        }
    }

    iins = sequence.base_;

    // Evaluate Transformations
    if (cmd & FUNCCOORD)
    {
        // Get the Root Coords for the first instruction in the sequence
        {
            std::vector<CCoordinate>& coords = GetCachedCoords(iins);
            // if no root get the coord from origincoord
            if (!sequence.rootseq_)
            {
                if (!coords.size())
                    PushCachedDataCoord(iins, origincoord);
            }
            // or if sequence is notrans from root coords
            else if (!sequence.istrans_)
            {
                coords = GetCachedCoords(sequence.root_);
            }
            // or if root sequence coords are ready
            else// if (HasCoordReady(sequence.root_))
            {
                coords = GetCachedCoords(sequence.root_);
            }
            /*
            else
            {
                #ifdef USE_CACHESTAT
                if (DebugV2<DebugMaxV2) URHO3D_LOGERRORF("Evaluator() - EvaluateSequence rid=%u : COORDS => can't get coords at root=%u rootcoorddata*=%u !",
                                                             sequence.rid_, sequence.root_, cachecoordaccess_[currSequenceId_][sequence.root_]);
                #endif
                return;
            }
            */
#ifdef USE_CACHESTAT
            if (DebugV2<DebugMaxV2)
            {
                URHO3D_LOGINFOF("Evaluator() - EvaluateSequence rid=%u : COORDS Functions ... coorddata*=%u", sequence.rid_, cachecoordaccess_[currSequenceId_][sequence.base_]);
                for (unsigned j=0; j < coords.size(); ++j)
                    URHO3D_LOGINFOF(" => COORDS[%u] = %f,%f", j, coords[j].x_, coords[j].y_);
            }
#endif
        }

        // Evaluate all the coords in the sequence
        {
            int coordcount = 0;
            for (int iseq=iend; iseq >= ibegin; --iseq)
            {
                iins = sequence[iseq];

                const Instruction& instruction = instructionList_.GetInstruction(iins);
                const std::vector<unsigned>& sourcesNoTrans = instruction.GetSourcesNoTrans();
                // Check if all SourcesNoTrans are setted Values
                {
                    bool coordready = true;
                    for (unsigned i=0; i < sourcesNoTrans.size(); ++i)
                    {
                        if (GetCachedValues(sourcesNoTrans[i]).size() == 0)
                        {
                            coordready = false;
                            break;
                        }
                    }
                    if (!coordready)
                        continue;
                }

                coordcount++;

                std::vector<CCoordinate>& coords = GetCachedCoords(iins);

#ifdef USE_CACHESTAT
                if (DebugV2<DebugMaxV2)
                    URHO3D_LOGINFOF("=> instructtype=%s(%u) ... ", ANLModuleTypeV2Str[instruction.type_], iins);
#endif

                // Evaluate the coords
                EvaluateCoords(iins);
                SetCoordReady(iins);

#ifdef USE_CACHESTAT
                if (DebugV2<DebugMaxV2)
                {
                    for (unsigned j=0; j < coords.size(); ++j)
                        URHO3D_LOGINFOF(" => COORDS[%u] = %f,%f", j, coords[j].x_, coords[j].y_);
                }
#endif

                // Copy coords to sources with COORDMODIFIER property
                {
                    // Copy coords to sources in the sequence
                    const std::vector<unsigned>& sources = instruction.GetSources();
                    for (unsigned i=0; i < sources.size(); ++i)
                    {
                        unsigned isrc = sources[i];
                        if (sequence.HasInstruction(isrc) && instructionList_.GetInstruction(isrc).HasProperty(IC_COORDMODIFIER))
                        {
                            GetCachedCoords(isrc) = coords;
#ifdef USE_CACHESTAT
                            if (DebugV2<DebugMaxV2)
                                for (unsigned j=0; j < coords.size(); ++j)
                                    URHO3D_LOGINFOF("=> instructtype=%s(%u) COORDS[%u] = %f,%f copy idata*=%u to source=%u srcdata*=%u",
                                                    ANLModuleTypeV2Str[instruction.type_], iins, j, coords[j].x_, coords[j].y_, &GetCachedCoords(iins), isrc, &GetCachedCoords(isrc));
#endif
                        }
                    }
                }
            }

            SetCoordReady(sequence.base_, coordcount == (iend-ibegin+1));

#ifdef USE_CACHESTAT
            if (HasCoordReady(sequence.base_) && (DebugV2<DebugMaxV2)) URHO3D_LOGINFOF("Evaluator() - EvaluateSequence rid=%u : COORDS Functions ... Coord Ready !", sequence.rid_);
#endif
        }
    }

    // Evaluate Values
    if (cmd & FUNCVALUE)
    {
#ifdef USE_CACHESTAT
        if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("Evaluator() - EvaluateSequence rid=%u : VALUES Functions ...", sequence.rid_);
#endif

        unsigned iins;

        for (int iseq=ibegin; iseq <= iend; ++iseq)
        {
            iins = sequence[iseq];
            const Instruction& instruction = instructionList_.GetInstruction(iins);

            if (EvaluateValues(iins))
            {
#ifdef USE_CACHESTAT
                if (DebugV2<DebugMaxV2)
                {
                    std::vector<float>& values = GetCachedValues(iins);
                    std::vector<CCoordinate>& coords = GetCachedCoords(iins);
                    for (unsigned j=0; j < values.size(); ++j)
                        URHO3D_LOGINFOF("=> instructtype=%s(%u) VALUES[%u] coords[0]=(%f,%f) => val=%f idata*=%u",
                                        ANLModuleTypeV2Str[instruction.type_], iins, j, coords[0].x_, coords[0].y_, values[j],
                                        cachevalueaccess_[currSequenceId_][iins]);
                }
#endif
            }
            /*
            else
            {
                /// TEST : NOT A GOOD CODE => FIND ANOTHER SOLUTION
                // if the instruction is a transformation, get the value from the sequence of the source[0]
                unsigned isrc = instruction.GetParamSrcID(0);
                if (sequence.HasInstruction(isrc))
                {
                    #ifdef USE_CACHESTAT
                    if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("=> instructtype=%s(%u) is transformation => get value %f from src=%u isrcvaluedata*=%u",
                                                     ANLModuleTypeV2Str[instruction.type_], iins, GetCachedValues(isrc)[0], isrc,
                                                     cachevalueaccess_[currSequenceId_][isrc]);
                    #endif
                    GetCachedValues(iins) = GetCachedValues(isrc);
                }
                else
                {
                    unsigned rid = instruction.index_ + 1000*isrc;
                    const Sequence* seq = currentTree_->GetSequence(rid);
                    if (seq)
                    {
                        GetCachedValues(iins) = GetCachedData(seq->id_, isrc).values_;
                        #ifdef USE_CACHESTAT
                        if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("=> instructtype=%s(%u) is transformation => get value from src=%u seq rid=%u value=%f isrcvaluedata*=%u",
                                                         ANLModuleTypeV2Str[instruction.type_], iins, isrc, rid, GetCachedValues(iins)[0],
                                                         cachevalueaccess_[seq->id_][isrc]);
                        #endif
                    }
                    else
                    {
                        #ifdef USE_CACHESTAT
                        if (DebugV2<DebugMaxV2) URHO3D_LOGERRORF("=> instructtype=%s(%u) is transformation => get value from src=%u error rid=%u !",
                                                         ANLModuleTypeV2Str[instruction.type_], iins, isrc, rid);
                        #endif
                        GetCachedValues(iins) = GetCachedValues(isrc);
                    }
                }
            }
            */
        }

        // if is notrans evaluate root
        if (!sequence.istrans_ && sequence.rootseq_)
        {
#ifdef USE_CACHESTAT
            if (DebugV2<DebugMaxV2) URHO3D_LOGINFOF("==> UPDATE COORDS at instruction=%u ... at idata*=%u",
                                                        sequence.root_, cachevalueaccess_[currSequenceId_][sequence.root_]);
#endif
            EvaluateCoords(sequence.root_);
            SetCoordReady(sequence.root_);
        }
    }
}


bool Evaluator::EvaluateCoords(unsigned instructionindex)
{
    const Instruction& instruction = instructionList_.GetInstruction(instructionindex);
    InstructionFunction func = instructionFunctions_[instruction.type_][CoordModifier];

    if (func != FunctionEmpty)
        (*func)(this, instructionindex, instruction);

    return true;
}

bool Evaluator::EvaluateValues(unsigned instructionindex)
{
    const Instruction& instruction = instructionList_.GetInstruction(instructionindex);
    InstructionFunction func = instructionFunctions_[instruction.type_][ValueCalculation];

    if (func == FunctionEmpty)
        return false;

    (*func)(this, instructionindex, instruction);
    return true;
}

};
