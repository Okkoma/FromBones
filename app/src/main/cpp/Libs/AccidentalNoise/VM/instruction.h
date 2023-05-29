#ifndef INSTRUCTION_H
#define INSTRUCTION_H

#include "../vectortypes.h"

const int MaxSourceCount=10;

using namespace Urho3D;


namespace anl
{
#ifndef INSTRUCTION_OPCODE_H
enum EOpcodes
{
    OP_NOP,

    OP_Seed,
    OP_Constant,
    OP_Seeder,
    OP_CacheArray,

// Basis
    OP_ValueBasis,
    OP_GradientBasis,
    OP_SimplexBasis,
    OP_CellularBasis,

    OP_X,
    OP_Y,
    OP_Z,
    OP_Radial,
    OP_W,
    OP_U,
    OP_V,

// Coordinates Transformations
    OP_ScaleDomain,
    OP_ScaleX,
    OP_ScaleY,
    OP_ScaleZ,
    OP_ScaleW,
    OP_ScaleU,
    OP_ScaleV,

    OP_TranslateDomain,
    OP_TranslateX,
    OP_TranslateY,
    OP_TranslateZ,
    OP_TranslateW,
    OP_TranslateU,
    OP_TranslateV,

    OP_RotateDomain,

    OP_DX,
    OP_DY,
    OP_DZ,
    OP_DW,
    OP_DU,
    OP_DV,

    OP_Fractal,

// Value Operations
    // Single Sources
    OP_Abs,
    OP_Sin,
    OP_Cos,
    OP_Tan,
    OP_ASin,
    OP_ACos,
    OP_ATan,
    // Two Sources
    OP_Add,
    OP_Subtract,
    OP_Multiply,
    OP_Divide,
    OP_Pow,
    OP_Min,
    OP_Max,
    OP_Bias,
    OP_Gain,

    // Multi Sources
    OP_Sigmoid,
    OP_Randomize,
    OP_CurveSection,
    OP_HexTile,
    OP_HexBump,
    OP_Clamp,
    OP_Blend,
    OP_Select,

    // Filter
    OP_SmoothStep,
    OP_SmootherStep,
    OP_LinearStep,
    OP_Step,
    OP_Tiers,
    OP_SmoothTiers,

    // RGBA operations
    OP_Color,
    OP_ExtractRed,
    OP_ExtractGreen,
    OP_ExtractBlue,
    OP_ExtractAlpha,
    OP_Grayscale,
    OP_CombineRGBA,
    OP_CombineHSVA,

// BuiltIn Macros
    OP_BuiltInMacros,

    OP_SimpleFBM,
    OP_SimpleRidgedMF,
    OP_SimpleBillow,

    OP_NumFunctions,
};
#define INSTRUCTION_OPCODE_H
#endif
struct SInstruction
{
    SInstruction()
    {
        for(unsigned int c=0; c<MaxSourceCount; ++c)
            sources_[c] = 1000000u;
    }

    // Instruction opcode
    unsigned int opcode_;
    // Out fields
    SRGBA outrgba_;
    float outfloat_;
    // Source input indices
    unsigned int sources_[MaxSourceCount];
};

//typedef std::vector<SInstruction> InstructionListType;
typedef Vector<SInstruction > InstructionListType;

class CKernel;
class CNoiseExecutor;

//typedef unsigned int CInstructionIndex;

class CInstructionIndex
{
public:
    CInstructionIndex()
    {
        index_=0;
    }

    CInstructionIndex(unsigned int i)
    {
        index_=i;
    }

    CInstructionIndex(const CInstructionIndex &c)
    {
        index_=c.index_;
    }

    CInstructionIndex operator +(const CInstructionIndex &c)
    {
        return CInstructionIndex(index_+c.index_);
    }

    CInstructionIndex &operator +=(const CInstructionIndex &c)
    {
        index_+=c.index_;
        return *this;
    }

    CInstructionIndex operator -(const CInstructionIndex &c)
    {
        return CInstructionIndex(index_-c.index_);
    }

    CInstructionIndex &operator -=(const CInstructionIndex &c)
    {
        index_-=c.index_;
        return *this;
    }

    CInstructionIndex operator +(const unsigned int c)
    {
        return CInstructionIndex(index_+c);
    }

    CInstructionIndex &operator +=(const unsigned int c)
    {
        index_+=c;
        return *this;
    }

    CInstructionIndex operator -(const unsigned int c)
    {
        return CInstructionIndex(index_-c);
    }

    CInstructionIndex &operator -=(const unsigned int c)
    {
        index_-=c;
        return *this;
    }

    CInstructionIndex &operator++()
    {
        index_++;
        return *this;
    }

    CInstructionIndex operator++(int)
    {
        index_++;
        return *this;
    }

    CInstructionIndex &operator=(unsigned int c)
    {
        index_=c;
        return *this;
    }

    CInstructionIndex &operator=(const CInstructionIndex &c)
    {
        index_=c.index_;
        return *this;
    }

    unsigned int GetIndex() const
    {
        return index_;
    }

private:

    unsigned int index_;
    friend class CKernel;
    friend class CNoiseExecutor;
    friend class CExpressionBuilder;
};


};

#endif
