#ifndef ANLV2_INSTRUCTIONS_H
#define ANLV2_INSTRUCTIONS_H

#include <vector>
#include <list>
#include <map>

#include "../VCommon/coordinate.h"

#include <Urho3D/Container/Str.h>

namespace anl
{

const unsigned MAXPARAMS = 10U;

enum
{
    TRANS = 0,
    NOTRANS,
};

enum
{
    IC_NONE = 0,
    IC_COORDMODIFIER = 1,
    IC_VALUEMODIFIER = 2,
    IC_ROTATION      = 4,
    IC_CACHEARRAY    = 8,
    IC_ALLMODIFIER   = IC_COORDMODIFIER | IC_VALUEMODIFIER,
};

struct InstructionProperty
{
    int property_;
    int numparams_;
    int paramproperties_[MAXPARAMS];
};

struct InstructionParam
{
    InstructionParam(float f) : type_(ANL_FLOAT), float_(f), source_(false) { }
    InstructionParam(int i) : type_(ANL_INT), int_(i), source_(false) { }
    InstructionParam(unsigned u, bool source) : type_(ANL_UNSIGNED), unsigned_(u), source_(source) { }

    InstructionParam(const InstructionParam& i) :
        type_(i.type_),
        float_(i.float_),
        source_(i.source_)
    { }

    void Set(float v)
    {
        float_ = v;
    }
    void Set(int v)
    {
        int_ = v;
    }
    void Set(unsigned v)
    {
        unsigned_ = v;
    }

    enum {ANL_FLOAT, ANL_UNSIGNED, ANL_INT} type_;
    union
    {
        float float_;
        int int_;
        unsigned unsigned_;
    };
    bool source_;
};

typedef std::vector<InstructionParam> InstructionParams;

typedef float RotMatrix[9];

struct CRotMatrix
{
    CRotMatrix() : evaluated_(false), cstparams_(true) { }
    RotMatrix mat_;
    bool evaluated_;
    bool cstparams_;
};

struct InstructionValues
{
    void clear()
    {
        values_.clear();
    }

    std::vector<float> values_;
};

struct InstructionCoords
{
    InstructionCoords() : rotmat_(0), coordReady_(false) { }
    ~InstructionCoords()
    {
        if (rotmat_) delete rotmat_;
        rotmat_=0;
    }
    void AddRotMatrix()
    {
        if (!rotmat_) rotmat_ = new(CRotMatrix);
    }
    void clear()
    {
        coords_.clear();
        coordReady_ = false;
    }

    std::vector<CCoordinate> coords_;
    bool coordReady_;
    CRotMatrix* rotmat_;
};

struct InstructionData
{
    InstructionData() { }
    ~InstructionData() {  }

    void clear()
    {
        v_.clear();
        c_.clear();
    }

    InstructionValues v_;
    InstructionCoords c_;
};

struct Sequence
{
    Sequence(int root, unsigned base, bool trans) : id_(0), rootseq_(0), root_(root), base_(base), istrans_(trans), isparent_(false) { }
    Sequence(const Sequence& s) : id_(s.id_), rootseq_(s.rootseq_), root_(s.root_), base_(s.base_), istrans_(s.istrans_), isparent_(s.isparent_),
        childs_(s.childs_), childrids_(s.childrids_), instructions_(s.instructions_) { }

    bool HasInstruction(unsigned id) const
    {
        return std::find(instructions_.begin(), instructions_.end(), id) != instructions_.end();
    }
    bool HasRoot(unsigned id) const
    {
        return id == (unsigned)root_;
    }
    bool HasChild(unsigned id) const
    {
        return std::find(childs_.begin(), childs_.end(), id) != childs_.end();
    }
    int GetInstructionIndex(unsigned id) const
    {
        std::vector<unsigned>::const_iterator it = std::find(instructions_.begin(), instructions_.end(), id);
        return it != instructions_.end() ? it-instructions_.begin() : -1;
    }

    unsigned GetNumInstructions() const
    {
        return instructions_.size();
    }
    unsigned& operator[](unsigned index)
    {
        return instructions_[index];
    }
    const unsigned& operator[](unsigned index) const
    {
        return instructions_[index];
    }

    unsigned id_;
    unsigned rid_;

    Sequence* rootseq_;
    int root_;
    unsigned base_;
    bool istrans_;
    bool isparent_;

    std::vector<unsigned> childs_;
    std::vector<unsigned> childrids_;
    std::vector<unsigned> instructions_;
};

enum
{
    FUNCCOORD = 1,
    FUNCVALUE = 2,
    FUNCCOORDVALUE = 3,
};

struct SequenceCmd
{
    SequenceCmd(int cmd, Sequence* seqptr) : cmd_(cmd), seqptr_(seqptr) { }
    SequenceCmd(const SequenceCmd& s) : cmd_(s.cmd_), seqptr_(s.seqptr_) { }

    int cmd_; // COORD or VALUE or COORDVALUE
    Sequence* seqptr_; // sequence utilisée
};

struct InstructionTree
{
    InstructionTree() { }

    const Sequence* GetSequence(unsigned rid) const
    {
        for (std::vector<Sequence>::const_iterator it=sequences_.begin(); it!=sequences_.end(); ++it)
        {
            if (it->rid_ == rid)
                return &(*it);
        }
        return 0;
    }
    const Sequence* GetRootSequence(unsigned childrid) const
    {
        for (std::vector<Sequence>::const_iterator it=sequences_.begin(); it!=sequences_.end(); ++it)
        {
            std::vector<unsigned>::const_iterator ridt = std::find(it->childrids_.begin(), it->childrids_.end(), childrid);
            if (ridt != it->childrids_.end())
                return &(*it);
        }
        return 0;
    }

    unsigned index_;

    std::vector<SequenceCmd> commands_;
    std::vector<Sequence> sequences_;
};


class Evaluator;


struct Instruction
{
    friend class Evaluator;
public:
    Instruction(unsigned index, int type, const InstructionParams& params) :
        index_(index),
        type_(type),
        params_(params)
    {
        Set();
    }
    Instruction(const Instruction& i) :
        index_(i.index_),
        params_(i.params_),
        type_(i.type_),
        sources_(i.sources_),
        sourceTrans_(i.sourceTrans_),
        sourcesNoTrans_(i.sourcesNoTrans_),
        numTransSources_(i.numTransSources_)
    { }

    bool HasProperty(int property) const;
    bool IsSourceTransformable(unsigned index) const;
    bool HasAllSourcesTrans() const
    {
        return sources_.size() == numTransSources_;
    }
    void GetSourcesList(std::list<unsigned>& sources) const;
    const std::vector<unsigned>& GetSources() const
    {
        return sources_;
    }
    const std::vector<unsigned>& GetSourcesNoTrans() const
    {
        return sourcesNoTrans_;
    }

    const InstructionParam& GetParam(unsigned i) const;
    float GetParamFloat(Evaluator* evaluator, unsigned iparam, unsigned ilayer=0) const;
    int GetParamInt(Evaluator* evaluator, unsigned iparam, unsigned ilayer=0) const;
    unsigned GetParamUInt(Evaluator* evaluator, unsigned iparam, unsigned ilayer=0) const;
    unsigned GetParamSrcID(unsigned i) const;

    Urho3D::String GetParamStr() const;

    void Dump() const;

private:
    void Set();

    // index = id
    unsigned index_;
    // instruction type
    int type_;
    // instruction parameters
    InstructionParams params_;

    // sources indexes
    std::vector<unsigned> sources_;
    std::vector<unsigned> sourcesNoTrans_;
    std::vector<bool> sourceTrans_;
    unsigned numTransSources_;
};

struct InstructionList
{
    InstructionList(unsigned size)
    {
        instructions_.reserve(size);
        maintreeid_ = 0;
    }

    unsigned PushInstruction(int type, const InstructionParams& datas);
    unsigned PushBuiltInMacro(int type, const InstructionParams& datas);
    unsigned SimpleFractalLayer(int basistype, int interptype, float layerscale, float layerfreq, unsigned seed, bool rot=true, float angle=0.5, float ax=0, float ay=0, float az=1);
    unsigned SimpleRidgedLayer(int basistype, int interptype, float layerscale, float layerfreq, unsigned seed, bool rot=true, float angle=0.5, float ax=0, float ay=0, float az=1);
    unsigned SimpleBillowLayer(int basistype, int interptype, float layerscale, float layerfreq, unsigned int seed, bool rot=true, float angle=0.5, float ax=0, float ay=0, float az=1);
    unsigned SimplefBm(const InstructionParams& datas);
    unsigned SimpleRidgedMultifractal(const InstructionParams& datas);
    unsigned SimpleBillow(const InstructionParams& datas);

    Instruction& GetInstruction(unsigned index)
    {
        return instructions_[index];
    }
    unsigned GetNumInstructions() const
    {
        return (unsigned)instructions_.size();
    }
    const std::vector<unsigned>& GetCachedInstructions() const
    {
        return cachedInstructions_;
    }

    InstructionTree* BuildTree(unsigned index);
    InstructionTree* GetTree(unsigned index);
    InstructionTree* GetMainTree();

    void DumpTree(const InstructionTree& tree) const;
    void DumpInstructions() const;

    // instructions
    std::vector<Instruction> instructions_;
    // ids of the instructions to cache by cachemap
    std::vector<unsigned> cachedInstructions_;
    // sequence of instructions to evaluate (indexed by the instructionid of the tree)
    std::map<unsigned, InstructionTree> cachedtrees_;

    unsigned maintreeid_;
};

typedef void (*InstructionFunction)(Evaluator*, unsigned, const Instruction&);

};
#endif

