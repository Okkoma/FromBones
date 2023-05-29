#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Timer.h>
#include <Urho3D/IO/Log.h>

#include "../VCommon/random_gen.h"

#include "Evaluator.h"
#include "Functions.h"

#include "Instructions.h"


namespace anl
{

static InstructionProperty instructionProperties_[] =
{
    { IC_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
    { IC_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
    { IC_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
    { IC_NONE, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },
    { IC_VALUEMODIFIER | IC_CACHEARRAY, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // CacheArray

    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ValueBasis
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // GradientBasis
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // SimplexBasis
    { IC_VALUEMODIFIER, 10, TRANS, TRANS, TRANS, TRANS, TRANS, TRANS, TRANS, TRANS, TRANS, TRANS  },          // CellularBasis
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // X
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // Y
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // Z
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // Radial
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // W
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // U
    { IC_VALUEMODIFIER, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  },    // V

    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleDomain
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleX
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleY
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleZ
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleW
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleU
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // ScaleV
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateDomain
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateX
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateY
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateZ
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateW
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateU
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // TranslateV
    { IC_COORDMODIFIER | IC_ROTATION, 5, TRANS, NOTRANS, NOTRANS, NOTRANS, NOTRANS, 0, 0, 0, 0, 0  },    // RotateDomain
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // DX
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // DY
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // DZ
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // DW
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // DU
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // DV
    { IC_COORDMODIFIER, 2, TRANS, NOTRANS, 0, 0, 0, 0, 0, 0, 0, 0  },    // Fractal /// TODO

    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // Abs
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // Sin
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // Cos
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // Tan
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // ASin
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // ACos
    { IC_VALUEMODIFIER, 1, TRANS, 0, 0, 0, 0, 0, 0, 0, 0, 0  },          // ATan

    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Add
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Substract
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Multiply
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Divide
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Pow
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Min
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Max
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Bias
    { IC_VALUEMODIFIER, 2, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0, 0  },      // Gain

    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Sigmoid /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Randomize /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // CurveSection /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // HexTile /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // HexBump  /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Clamp
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Mix
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Select   /// TODO

    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // SmoothStep /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // SmootherStep /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // LinearStep  /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Step      /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Tiers     /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // SmoothTiers   /// TODO

    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Color /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // ExtractRed /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // ExtractGreen  /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // ExtractBlue   /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // ExtractAlpha      /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // Grayscale   /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // CombineRGBA      /// TODO
    { IC_VALUEMODIFIER, 3, TRANS, TRANS, TRANS, 0, 0, 0, 0, 0, 0, 0  },  // CombineHSVA   /// TODO
};


/// Instruction

bool Instruction::HasProperty(int property) const
{
    return (instructionProperties_[type_].property_ & property);
}

bool Instruction::IsSourceTransformable(unsigned index) const
{
    return sourceTrans_[index];
}

void Instruction::Set()
{
    numTransSources_ = 0;
    // Set Sources & numTransSources_
    for (unsigned i=0; i < params_.size(); ++i)
    {
        unsigned isrc = params_[i].unsigned_;
        if (params_[i].source_ && isrc != index_)
        {
            sources_.push_back(isrc);
            sourceTrans_.push_back(instructionProperties_[type_].paramproperties_[i] == TRANS);
            if (sourceTrans_.back())
                numTransSources_++;
            else
                sourcesNoTrans_.push_back(isrc);
        }
    }
}

void Instruction::GetSourcesList(std::list<unsigned>& sources) const
{
    for (std::vector<unsigned>::const_iterator s=sources_.begin(); s!=sources_.end(); ++s)
        sources.push_back(*s);
}

const InstructionParam& Instruction::GetParam(unsigned iparam) const
{
    return params_[iparam];
}

float Instruction::GetParamFloat(Evaluator* evaluator, unsigned iparam, unsigned ilayer) const
{
    if (!params_[iparam].source_)
        return params_[iparam].float_;

    return evaluator->GetCachedValue(params_[iparam].unsigned_, ilayer);
}

int Instruction::GetParamInt(Evaluator* evaluator, unsigned iparam, unsigned ilayer) const
{
    if (!params_[iparam].source_)
        return params_[iparam].int_;

    return (int)evaluator->GetCachedValue(params_[iparam].unsigned_, ilayer);
}

unsigned Instruction::GetParamUInt(Evaluator* evaluator, unsigned iparam, unsigned ilayer) const
{
    if (!params_[iparam].source_)
        return params_[iparam].unsigned_;

    return (unsigned)evaluator->GetCachedValue(params_[iparam].unsigned_, ilayer);
}

unsigned Instruction::GetParamSrcID(unsigned iparam) const
{
    return params_[iparam].unsigned_;
}

Urho3D::String GetParamStr(const InstructionParams& datas)
{
    Urho3D::String str;
    for (unsigned i=0; i< datas.size(); ++i)
    {
        Urho3D::String str2;
        if (datas[i].type_ == InstructionParam::ANL_FLOAT)
            str2.AppendWithFormat(" Float[%d]=%f", i, datas[i].float_);
        else if (datas[i].type_ == InstructionParam::ANL_UNSIGNED)
        {
            if (datas[i].source_)
                str2.AppendWithFormat(" Src[%d]=%u", i, datas[i].unsigned_);
            else
                str2.AppendWithFormat(" UInt[%d]=%u", i, datas[i].unsigned_);
        }
        else if (datas[i].type_ == InstructionParam::ANL_INT)
            str2.AppendWithFormat(" Int[%d]=%d", i, datas[i].int_);
        str += str2;
    }
    return str;
}

Urho3D::String Instruction::GetParamStr() const
{
    Urho3D::String str;
    str.AppendWithFormat(" id=%u", index_);
    str += anl::GetParamStr(params_);
    return str;
}

void Instruction::Dump() const
{
    URHO3D_LOGINFOF("Instruction() - Dump : index=%u type=%s params:%s numsources=%u numsourcesTRANS=%u", index_, ANLModuleTypeV2Str[type_], GetParamStr().CString(), sources_.size(), numTransSources_);
}


/// InstructionList

Urho3D::String GetListStr(const std::list<unsigned>& data)
{
    Urho3D::String str;
    for (std::list<unsigned>::const_iterator i=data.begin(); i != data.end(); ++i)
    {
        str += ' ';
        str += *i;
    }
    return str;
}

unsigned InstructionList::PushInstruction(int type, const InstructionParams& datas)
{
    unsigned index = instructions_.size();
    instructions_.push_back(Instruction(index, type, datas));
    if (type == OP_CacheArray)
        cachedInstructions_.push_back(index);

    return index;
}

unsigned InstructionList::PushBuiltInMacro(int type, const InstructionParams& datas)
{
    if (type == OP_SimpleFBM)
        SimplefBm(datas);

    else if (type == OP_SimpleRidgedMF)
        SimpleRidgedMultifractal(datas);

    else if (type == OP_SimpleBillow)
        SimpleBillow(datas);

    return instructions_.size()-1;
}

unsigned InstructionList::SimpleFractalLayer(int basistype, int interptype, float layerscale, float layerfreq, unsigned mseed, bool rot,
        float angle, float ax, float ay, float az)
{
    InstructionParams datas;

    if (basistype != OP_SimplexBasis)
        datas.push_back(InstructionParam(interptype));
    datas.push_back(InstructionParam(mseed, false));
    unsigned base = PushInstruction(basistype, datas);

    datas.clear();
    datas.push_back(InstructionParam(base, true));
    datas.push_back(InstructionParam(layerscale));
    unsigned multiply = PushInstruction(OP_Multiply, datas);

    datas.clear();
    datas.push_back(InstructionParam(multiply, true));
    datas.push_back(InstructionParam(layerfreq));
    unsigned scale = PushInstruction(OP_ScaleDomain, datas);

    if(rot)
    {
        float len=std::sqrt(ax*ax+ay*ay+az*az);
        datas.clear();
        datas.push_back(InstructionParam(scale, true));
        datas.push_back(InstructionParam(angle));
        datas.push_back(InstructionParam(ax/len));
        datas.push_back(InstructionParam(ay/len));
        datas.push_back(InstructionParam(az/len));
        unsigned scale = PushInstruction(OP_RotateDomain, datas);
    }

    return instructions_.size()-1;
}

unsigned InstructionList::SimpleRidgedLayer(int basistype, int interptype, float layerscale, float layerfreq, unsigned mseed, bool rot,
        float angle, float ax, float ay, float az)
{
    InstructionParams datas;

    if (basistype != OP_SimplexBasis)
        datas.push_back(InstructionParam(interptype));
    datas.push_back(InstructionParam(mseed, false));
    unsigned base = PushInstruction(basistype, datas);

    datas.clear();
    datas.push_back(InstructionParam(base, true));
    unsigned abso = PushInstruction(OP_Abs, datas);

    datas.clear();
    datas.push_back(InstructionParam(1.f));
    datas.push_back(InstructionParam(abso, true));
    unsigned substract = PushInstruction(OP_Subtract, datas);

    datas.clear();
    datas.push_back(InstructionParam(substract, true));
    datas.push_back(InstructionParam(layerscale));
    unsigned multiply = PushInstruction(OP_Multiply, datas);

    datas.clear();
    datas.push_back(InstructionParam(multiply, true));
    datas.push_back(InstructionParam(layerfreq));
    unsigned scale = PushInstruction(OP_ScaleDomain, datas);

    if(rot)
    {
        float len=std::sqrt(ax*ax+ay*ay+az*az);
        datas.clear();
        datas.push_back(InstructionParam(scale, true));
        datas.push_back(InstructionParam(angle));
        datas.push_back(InstructionParam(ax/len));
        datas.push_back(InstructionParam(ay/len));
        datas.push_back(InstructionParam(az/len));
        unsigned scale = PushInstruction(OP_RotateDomain, datas);
    }

    return instructions_.size()-1;
}

unsigned InstructionList::SimpleBillowLayer(int basistype, int interptype, float layerscale, float layerfreq, unsigned mseed, bool rot,
        float angle, float ax, float ay, float az)
{
    InstructionParams datas;

    URHO3D_LOGINFOF("billowlayer scale=%f freq=%f", layerscale, layerfreq);

    if (basistype != OP_SimplexBasis)
        datas.push_back(InstructionParam(interptype));
    datas.push_back(InstructionParam(mseed, false));
    unsigned base = PushInstruction(basistype, datas);

    datas.clear();
    datas.push_back(InstructionParam(base, true));
    unsigned abso = PushInstruction(OP_Abs, datas);

    datas.clear();
    datas.push_back(InstructionParam(abso, true));
    datas.push_back(InstructionParam(2.f));
    unsigned multiply = PushInstruction(OP_Multiply, datas);

    datas.clear();
    datas.push_back(InstructionParam(multiply, true));
    datas.push_back(InstructionParam(1.f));
    unsigned subtract = PushInstruction(OP_Subtract, datas);

    datas.clear();
    datas.push_back(InstructionParam(subtract, true));
    datas.push_back(InstructionParam(layerscale));
    unsigned multiplysc = PushInstruction(OP_Multiply, datas);

    datas.clear();
    datas.push_back(InstructionParam(multiplysc, true));
    datas.push_back(InstructionParam(layerfreq));
    unsigned scale = PushInstruction(OP_ScaleDomain, datas);

    if(rot)
    {
        float len=std::sqrt(ax*ax+ay*ay+az*az);
        datas.clear();
        datas.push_back(InstructionParam(scale, true));
        datas.push_back(InstructionParam(angle));
        datas.push_back(InstructionParam(ax/len));
        datas.push_back(InstructionParam(ay/len));
        datas.push_back(InstructionParam(az/len));
        unsigned scale = PushInstruction(OP_RotateDomain, datas);
    }

    return instructions_.size()-1;
}

unsigned InstructionList::SimplefBm(const InstructionParams& datas)
{
    // NOTE : NumInstructions generated = 4+5*(numoctaves-1)
    // NOTE : 1 octave  = 4 instructions
    // NOTE : 6 octaves = 29 instructions

    unsigned numoctaves = (unsigned)datas[2].float_;

    if (numoctaves < 1)
        return 0;

    URHO3D_LOGINFOF("InstructionList() - SimplefBm : ... %s ...", GetParamStr(datas).CString());

    int basistype = OP_ValueBasis + datas[0].int_;
    int interptype = datas[1].int_;
    float frequency = datas[3].float_;
    unsigned seed = (unsigned)datas[4].float_;
    bool rot = true;

    URHO3D_LOGINFOF("InstructionList() - SimplefBm : ... basis=%d inter=%d octaves=%u freq=%f seed=%u ...", basistype, interptype, numoctaves, frequency, seed);

    KISS rnd;
    rnd.setSeed(seed);

    InstructionParams layerdatas;

    unsigned lastlayer = SimpleFractalLayer(basistype, interptype, 1.f, 1.f*frequency, seed+10, rot,
                                            rnd.get01()*3.14159265f, rnd.get01(), rnd.get01(), rnd.get01());

    for(unsigned octave=0; octave < numoctaves-1; ++octave)
    {
        float fo = std::pow(2.f, (float)octave);
        unsigned nextlayer = SimpleFractalLayer(basistype, interptype, 1.f/fo, fo*frequency, seed+10+octave*1000, rot,
                                                rnd.get01()*3.14159265f, rnd.get01(), rnd.get01(), rnd.get01());
        layerdatas.clear();
        layerdatas.push_back(InstructionParam(lastlayer, true));
        layerdatas.push_back(InstructionParam(nextlayer, true));
        lastlayer = PushInstruction(OP_Add, layerdatas);
    }

    return instructions_.size()-1;
}

unsigned InstructionList::SimpleRidgedMultifractal(const InstructionParams& datas)
{
    // NOTE : NumInstructions generated = 6+7*(numoctaves-1)
    // NOTE : 1 octave  = 6 instructions
    // NOTE : 6 octaves = 41 instructions

    unsigned numoctaves = (unsigned)datas[2].float_;

    if (numoctaves < 1)
        return 0;

    URHO3D_LOGINFOF("InstructionList() - SimpleRidgedMultifractal : ... %s ...", GetParamStr(datas).CString());

    int basistype = OP_ValueBasis + datas[0].int_;
    int interptype = datas[1].int_;
    float frequency = datas[3].float_;
    unsigned seed = (unsigned)datas[4].float_;
    bool rot = true;

    URHO3D_LOGINFOF("InstructionList() - SimpleRidgedMultifractal : ... basis=%d inter=%d octaves=%u freq=%f seed=%u ...", basistype, interptype, numoctaves, frequency, seed);

    KISS rnd;
    rnd.setSeed(seed);

    InstructionParams layerdatas;

    unsigned lastlayer = SimpleRidgedLayer(basistype, interptype, 1.f, 1.f*frequency, seed+10, rot,
                                           rnd.get01()*3.14159265f, rnd.get01(), rnd.get01(), rnd.get01());

    for(unsigned octave=0; octave<numoctaves-1; ++octave)
    {
        float fo = std::pow(2.f, (float)octave);
        unsigned nextlayer=SimpleRidgedLayer(basistype, interptype, 1.f/fo, fo*frequency, seed+10+octave*1000, rot,
                                             rnd.get01()*3.14159265f, rnd.get01(), rnd.get01(), rnd.get01());
        layerdatas.clear();
        layerdatas.push_back(InstructionParam(lastlayer, true));
        layerdatas.push_back(InstructionParam(nextlayer, true));
        lastlayer = PushInstruction(OP_Add, layerdatas);
    }

    return instructions_.size()-1;
}

unsigned InstructionList::SimpleBillow(const InstructionParams& datas)
{
    unsigned numoctaves = (unsigned)datas[2].float_;

    if (numoctaves < 1)
        return 0;

    URHO3D_LOGINFOF("InstructionList() - SimpleBillow : ... %s ...", GetParamStr(datas).CString());

    int basistype = OP_ValueBasis + datas[0].int_;
    int interptype = datas[1].int_;
    float frequency = datas[3].float_;
    unsigned seed = (unsigned)datas[4].float_;
    bool rot = true;

    URHO3D_LOGINFOF("InstructionList() - SimpleBillow : ... basis=%d inter=%d octaves=%u freq=%f seed=%u ...", basistype, interptype, numoctaves, frequency, seed);

    KISS rnd;
    rnd.setSeed(seed);

    InstructionParams layerdatas;

    unsigned lastlayer = SimpleBillowLayer(basistype, interptype, 1.f, frequency, seed+10, rot,
                                           rnd.get01()*3.14159265f, rnd.get01(), rnd.get01(), rnd.get01());

    for(unsigned octave=0; octave<numoctaves-1; ++octave)
    {
        float fo = std::pow(2.f, (float)octave);
        unsigned nextlayer=SimpleBillowLayer(basistype, interptype, 1.f/fo, fo*frequency, seed+10+octave*1000, rot,
                                             rnd.get01()*3.14159265f, rnd.get01(), rnd.get01(), rnd.get01());

        layerdatas.clear();
        layerdatas.push_back(InstructionParam(lastlayer, true));
        layerdatas.push_back(InstructionParam(nextlayer, true));
        lastlayer = PushInstruction(OP_Add, layerdatas);
    }

    return instructions_.size()-1;
}


bool CompareCommands(const SequenceCmd& first, const SequenceCmd& second)
{
    /*
    NOTE : l'établissement de la règle de comparaison doit suivre cet exemple:

    seq(startat=144 endat= 141 140 trans=false parent=true root=-1) =>  99 100 101 142 143 144
    seq(startat=141 endat= 3 80 trans=true parent=true root=142) =>  36 46 81 95 97 98 99 100 102 103 105 106 107 108 109 110 141
    seq(startat=140 endat= trans=false parent=false root=142) =>  111 112 113 114 115 116 117 118 119 120 121 122 123 124 125 126 127 128 129 130 131 132 133 134 135 136 137 138 139 140
    seq(startat=3 endat= trans=true parent=false root=81) =>  0 1 2 3
    seq(startat=80 endat= trans=false parent=false root=81) =>  47 48 49 50 51 52 53 54 55 56 57 58 59 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80
    => les commandes doivent etre trier comme suit:
    cmd(FUNCCOORD, seqid=144)       NOTRANS     ROOT=-1     144 est ROOT                            => evalue en COORD de 144 à 99      => 142 initialisé en COORD
    cmd(FUNCCOORDVALUE, seqid=140)  NOTRANS     ROOT=142    142 est initialisé en COORD             => evalue de 140 à 111              => évalue son root(142) en COORD
    cmd(FUNCCOORD, seqid=141)       TRANS       ROOT=142    142 est évalué en COORD                 => evalue en COORD de 141 à 36      => 81 initialisé en COORD
    cmd(FUNCCOORDVALUE, seqid=80)   NOTRANS     ROOT=81     81 est initialisé en COORD              => evalue de 80 à 47                => évalue son root(81) en COORD
    cmd(FUNCCOORD, seqid=3)         TRANS       ROOT=81     81 est evalué en COORD                  => evalue en COORD de 3 à 0
    cmd(FUNCVALUE, seqid=3)         TRANS       ROOT=81     est évalué en COORD et noparent         => evalue en VALUE de 0 à 3
    cmd(FUNCVALUE, seqid=141)       TRANS       ROOT=142    est évalué en COORD et 3,80 évalués     => evalue en VALUE de 36 à 141
    cmd(FUNCVALUE, seqid=144)       NOTRANS     ROOT=-1     est évalué en COORD et 141,140 évalués  => evalue en VALUE de 99 à 144
    */

    if (first.cmd_ != FUNCVALUE)
    {
        if (first.seqptr_->root_ == -1)
            return true;
        if (second.cmd_ == FUNCVALUE)
            return true;
        if (!first.seqptr_->istrans_ && second.seqptr_->istrans_ && first.seqptr_->root_ >= second.seqptr_->root_)
            return true;
    }
    else
    {
        if (second.cmd_ != FUNCVALUE)
            return false;
        if (second.seqptr_->base_ > first.seqptr_->base_)
            return true;
    }

    return false;
}

InstructionTree* InstructionList::BuildTree(unsigned index)
{
    URHO3D_LOGINFOF("InstructionList() - BuildTree : id=%u ...", index);

    Urho3D::HiresTimer timer;

    InstructionTree& tree = cachedtrees_[index];
    tree.index_ = index;

    if (maintreeid_ < index)
        maintreeid_ = index;

    std::list<Sequence> sequences;

    // obtenir les sequences minimales des instructions TRANS et NOTRANS.
    // => chacune de ces séquences est évaluée dans un premier temps pour les fonctions "TRANS" dans le sens PARENT=>SOURCES puis pour les fonctions "VALUE" dans le sens inverse
    // les séquences sont évaluées dans un ordre dépendant du type de leur première instruction par rapport à leur instruction "PARENT"
    // => une séquence peut être évaluée en TRANS, si l'instruction "PARENT" a été évaluée par une fonction TRANS ou bien si aucun PARENT (il s'agit d'une root)
    // => une séquence peut être évaluée en VALUE, si elle a déjà été evaluée en TRANS et (si ses sources sont évaluées en VALUE ou bien si aucune source).
    // => si le type est NOTRANS, la séquence est une root. Elle prend ces Coords du PARENT sans que celle-ci ait été évaluée.
    // ==> Elle peut donc etre evaluée en TRANS. De plus, si la séquence n'est pas marquée PARENT d'autre séquence, elle peut être évaluée en VALUE
    {
        std::vector<unsigned> newsequencerids;
        std::list<Sequence> newsequences;
        newsequences.push_front(Sequence(-1, index, false));
        std::list<unsigned> transSources;

        while (newsequences.size())
        {
            Sequence& sequence = newsequences.back();
            sequence.instructions_.push_back(sequence.base_);

            // parcourir les sources
            unsigned noTransCounter = 0;
            transSources.clear();
            transSources.push_back(sequence.base_);

            while (transSources.size() > 0)
            {
                int root = transSources.back();
                transSources.pop_back();

                const Instruction& instruction = instructions_[root];
                const std::vector<unsigned>& sources = instruction.GetSources();

                if (sources.size() > 0)
                {
                    for (unsigned i=0; i < sources.size(); ++i)
                    {
                        if (!instruction.IsSourceTransformable(i) || instructions_[sources[i]].HasProperty(IC_CACHEARRAY))
                            noTransCounter++;
                    }

                    if (noTransCounter == 0)
                    {
                        // si aucune des sources n'est NOTRANS, insérer les sources dans la séquence
                        // et continuer
                        for (unsigned i=0; i < sources.size(); ++i)
                        {
                            sequence.instructions_.push_back(sources[i]);
                            transSources.push_front(sources[i]);
                        }
                    }
                    else
                    {
                        // il y a des sources NOTRANS, la séquence est interrompue et devient parent d'autres séquences
                        sequence.isparent_ = true;
                        // toutes les sources deviennent de nouvelles séquences à explorer
                        for (unsigned i=0; i < sources.size(); ++i)
                        {
                            /// TODO : plusieurs roots sont possible =>
                            /// dans le tree 96(outer_lowhigh=>Mix), 3(groundshape) a comme roots 36(lowland_terrain) et 46(highland_terrain)
                            /// la sequence 3 doit donc être évalué 2fois, l'une pour 36 et l'autre pour 46
                            /// il faut stocker les coords provenant de ces differents roots (stockage dans le cache avec ilayer differents ?)
                            /// ou bien stocker uniquement dans le cache des instructions de type TRANSFORMATION, les coords et stocker les values dans les ROOTS
                            /// ce qui evite la copie descendante des coords vers chaque instruction.
                            unsigned rid = GetInstruction(sources[i]).HasProperty(IC_CACHEARRAY) ? sources[i] : root + sources[i]*1000;
                            if (std::find(newsequencerids.begin(), newsequencerids.end(), rid) == newsequencerids.end())
                            {
                                URHO3D_LOGINFOF("=> push newsequence rid=%u ...", rid);
                                newsequences.push_front(Sequence(root, sources[i], instruction.IsSourceTransformable(i)));
                                newsequencerids.push_back(rid);
                            }
                            sequence.childs_.push_back(sources[i]);
                        }
                    }
                }
            }

            sequences.push_back(sequence);
            newsequences.pop_back();
        }
    }

    {
        std::vector<Sequence>& seqs = tree.sequences_;
        std::vector<SequenceCmd>& cmds = tree.commands_;
        seqs.clear();
        cmds.clear();

        unsigned id = 0;
        for (std::list<Sequence>::iterator st=sequences.begin(); st!=sequences.end(); ++st)
        {
            // supprimer les instructions redondantes
            std::vector<unsigned>& datas = st->instructions_;
            std::sort(datas.begin(), datas.end());
            std::vector<unsigned>::iterator it = std::unique(datas.begin(), datas.end());
            datas.resize(std::distance(datas.begin(), it));

            // transférer dans la structure
            seqs.push_back(*st);
            seqs.back().id_ = id;
            id++;
        }

        {
            // attribuer les sequences rid = (root + 1000*base)
            for (std::vector<Sequence>::iterator st=seqs.begin(); st!=seqs.end(); ++st)
            {
                if (st->root_ == -1)
                    st->rid_ = 1000*st->base_;
                else if (GetInstruction(st->base_).HasProperty(IC_CACHEARRAY))
                    st->rid_ = st->base_;
                else
                    st->rid_ = st->root_ + 1000*st->base_;
            }
        }

        {
            // creer les chidrids
            for (std::vector<Sequence>::iterator childt=seqs.begin(); childt!=seqs.end(); ++childt)
            {
                unsigned root = childt->root_;
                if (root == -1)
                    continue;

                unsigned child = childt->base_;

                for (std::vector<Sequence>::iterator roott=seqs.begin(); roott!=seqs.end(); ++roott)
                {
                    std::vector<unsigned>::iterator it = std::find(roott->childs_.begin(), roott->childs_.end(), child);
                    if (it != roott->childs_.end())
                    {
                        if (GetInstruction(child).HasProperty(IC_CACHEARRAY))
                            roott->childrids_.push_back(child);
                        else
                            roott->childrids_.push_back(root + 1000*child);
                    }
                }
            }
        }

        {
            // créer les root sequences ptr
            for (std::vector<Sequence>::iterator childt=seqs.begin(); childt!=seqs.end(); ++childt)
            {
                unsigned root = childt->root_;
                if (root == -1)
                {
                    childt->rootseq_ = 0;
                    continue;
                }

                unsigned child = childt->base_;
                for (std::vector<Sequence>::iterator roott=seqs.begin(); roott!=seqs.end(); ++roott)
                {
                    std::vector<unsigned>::iterator it = std::find(roott->childs_.begin(), roott->childs_.end(), child);
                    if (it != roott->childs_.end())
                    {
                        childt->rootseq_ = &(*roott);
                        break;
                    }
                }
            }
        }

        {
            // initialiser les commandes de sequences
            std::list<SequenceCmd> commands;
            for (unsigned i=0; i < seqs.size(); ++i)
            {
                if (!seqs[i].istrans_ && !seqs[i].isparent_)
                    commands.push_back(SequenceCmd(FUNCCOORDVALUE, &seqs[i]));
                else
                {
                    commands.push_back(SequenceCmd(FUNCCOORD, &seqs[i]));
                    commands.push_back(SequenceCmd(FUNCVALUE, &seqs[i]));
                }
            }

            // tri des commandes par ordre
            commands.sort(CompareCommands);

            // transférer dans la structure
            for (std::list<SequenceCmd>::iterator ct=commands.begin(); ct!=commands.end(); ++ct)
                cmds.push_back(*ct);
        }
    }

    URHO3D_LOGINFOF("InstructionList() - BuildTree : id=%u ... timer=%d msec ... OK !", index, timer.GetUSec(false) / 1000);

    return &tree;
}

InstructionTree* InstructionList::GetTree(unsigned index)
{
    std::map<unsigned, InstructionTree>::iterator tt = cachedtrees_.find(index);
    if (tt == cachedtrees_.end())
    {
        InstructionTree* tree = BuildTree(index);
        DumpTree(*tree);
        return tree;
    }

    URHO3D_LOGINFOF("InstructionList() - GetTree : index=%u ... OK !", index);

    return &(tt->second);
}

InstructionTree* InstructionList::GetMainTree()
{
    std::map<unsigned, InstructionTree>::iterator tt = cachedtrees_.find(maintreeid_);
    return tt != cachedtrees_.end() ? &(tt->second) : 0;
}

void InstructionList::DumpTree(const InstructionTree& tree) const
{
    URHO3D_LOGINFOF("InstructionList() - DumpTree : %u ...", tree.index_);

    URHO3D_LOGINFOF("=> Sequences ...");
    for (std::vector<Sequence>::const_iterator tt=tree.sequences_.begin(); tt!=tree.sequences_.end(); ++tt)
    {
        const std::vector<unsigned>& data = tt->instructions_;
        Urho3D::String seqstr;
        for (std::vector<unsigned>::const_iterator it=data.begin(); it!=data.end(); ++it)
        {
            seqstr += ' ';
            seqstr += *it;
        }
        const std::vector<unsigned>& childs = tt->childs_;
        Urho3D::String childstr;
        for (std::vector<unsigned>::const_iterator it=childs.begin(); it!=childs.end(); ++it)
        {
            childstr += ' ';
            childstr += *it;
        }
        const std::vector<unsigned>& rids = tt->childrids_;
        Urho3D::String childridstr;
        for (std::vector<unsigned>::const_iterator it=rids.begin(); it!=rids.end(); ++it)
        {
            childridstr += ' ';
            childridstr += *it;
        }
        URHO3D_LOGINFOF("seq(rid=%u root=%d rootseq=%u base=%u childs=%s childrids=%s trans=%s parent=%s) => %s",
                        tt->rid_, tt->root_, tt->rootseq_, tt->base_, childstr.CString(), childridstr.CString(), tt->istrans_ ? "true":"false", tt->isparent_ ? "true":"false",
                        seqstr.CString());
    }

    URHO3D_LOGINFOF("=> Commands ...");
    for (std::vector<SequenceCmd>::const_iterator tt=tree.commands_.begin(); tt!=tree.commands_.end(); ++tt)
        URHO3D_LOGINFOF("cmd(%s, seqid=%u)", tt->cmd_ == FUNCCOORDVALUE ? "FUNCCOORDVALUE" : tt->cmd_ == FUNCCOORD ? "FUNCCOORD" : "FUNCVALUE", tt->seqptr_->base_);

    URHO3D_LOGINFOF("InstructionList() - DumpTree : %u ... OK !", tree.index_);
}

void InstructionList::DumpInstructions() const
{
    URHO3D_LOGINFOF("InstructionList() - DumpInstructions : ...");
    for (std::vector<Instruction>::const_iterator it=instructions_.begin(); it!=instructions_.end(); ++it)
        it->Dump();
    URHO3D_LOGINFOF("InstructionList() - DumpInstructions : ... OK !");
}


};
