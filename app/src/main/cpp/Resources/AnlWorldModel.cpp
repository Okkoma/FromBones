#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/Graphics/Graphics.h>

#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/XMLFile.h>

#include "Predefined.h"
#include "GameOptionsTest.h"

#define ANLVM_IMPLEMENTATION
//#define ANLVM_DUMPKERNEL
#include "anl.h"

#include "DefsMap.h"
#include "DefsWorld.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "Map.h"

#include "AnlWorldModel.h"

/// ANL options
#define ANL_ANLMAPSIZE 0.5f
#define ANL_SCREENSHOT_SIZE 64
#define ANL_SCREENSHOT_CENTEREDMAP_X 0
#define ANL_SCREENSHOT_CENTEREDMAP_Y 0
#define ANL_SCREENSHOT_MAPEXPANDED 35

typedef anl::CImplicitModuleBase *AnlModule;

extern const char* anl::ANLModuleTypeVMStr[];


static int terrainBound_;

enum
{
    MAPGENSLOT = 0,
    SNAPGENSLOT,
};

enum AnlModuleTypeV1
{
    ANLV1_Constant = 0,
    ANLV1_Gradient,
    ANLV1_Radial,
    ANLV1_Sphere,
    ANLV1_Fractal,
    ANLV1_AutoCorrect,
    ANLV1_Combiner,
    ANLV1_Select,
    ANLV1_Blend,
    ANLV1_Cache,
    ANLV1_CacheArray,
    ANLV1_Bias,
    ANLV1_Clamp,
    ANLV1_ScaleOffset,
    ANLV1_ScaleDomain,
    ANLV1_TranslateDomain,
};

static const char* ANLVersionNameStr[] =
{
    "V1",
    "VM",
    "V2",
    0
};

static const char* ANLModuleTypeV1Str[] =
{
    "Constant",
    "Gradient",
    "Radial",
    "Sphere",
    "Fractal",
    "AutoCorrect",
    "Combiner",
    "Select",
    "Blend",
    "Cache",
    "CacheArray",
    "Bias",
    "Clamp",
    "ScaleOffset",
    "ScaleDomain",
    "TranslateDomain",
    0
};

static const char* MapTypeStr[] =
{
    "GroundMap",
    "CaveMap",
    "TerrainMap",
    "BiomeMap",
    "PopulationMap",
    "TestMap",
    0
};

static const char* ANLCombinerTypesStr[] =
{
    "ADD",
    "SUB",
    "MULT",
    "MAX",
    "MIN",
    "AVG",
    0
};

static const char* ANLMappingModesStr[] =
{
    "SEAMLESS_NONE",
    "SEAMLESS_X",
    "SEAMLESS_Y",
    "SEAMLESS_Z",
    "SEAMLESS_XY",
    "SEAMLESS_XZ",
    "SEAMLESS_YZ",
    "SEAMLESS_XYZ",
    0
};

static const char* ANLBasisTypesStr[] =
{
    "VALUE",
    "GRADIENT",
    "GRADVAL",
    "SIMPLEX",
    "WHITE",
    0
};

static const char* ANLBasisTypesVMStr[] =
{
    "VALUE",
    "GRADIENT",
    "SIMPLEX",
    0
};

static const char* ANLInterpTypesStr[] =
{
    "NONE",
    "LINEAR",
    "CUBIC",
    "QUINTIC",
    0
};

static const char* ANLFractalTypesStr[] =
{
    "FBM",
    "RIDGEDMULTI",
    "BILLOW",
    "MULTI",
    "HYBRIDMULTI",
    "DECARPENTIERSWISS",
    0
};

enum ANLEnumCategoriesVM
{
    MappinMode = 0,
    BasisType,
    InterpType,

    NBCATVAL
};

static const char** ANLEnumCategoriesVMStr[3] =
{
    ANLMappingModesStr,
    ANLBasisTypesVMStr,
    ANLInterpTypesStr,
};



/// V1 Helpers

template< typename T >
void FindIndex(const T& entry, const Vector<T>& table, int* index)
{
    typename Vector<T>::ConstIterator it = table.Find(entry);
    *index = it != table.End() ? it-table.Begin() : -1;
}


/// VM Helpers

int GetEnumValue(const String& value)
{
    // If enums specified, do enum lookup and int assignment. Otherwise assign the variant directly
    bool enumFound = false;

    for (int i=0; i < (int)NBCATVAL; i++)
    {
        int enumValue = 0;
        const char **enumPtr = ANLEnumCategoriesVMStr[i];
        {
            while (*enumPtr)
            {
                if (!value.Compare(*enumPtr, false))
                {
                    enumFound = true;
                    break;
                }
                ++enumPtr;
                ++enumValue;
            }

            if (enumFound)
                return enumValue;
        }
    }
    return -1;
}


/// AnlWorldModel

AnlWorldModel::AnlWorldModel(Context* context) :
    Resource(context),
    seed_(0U),
    radius_(0.f),
    scale_(Vector2::ONE),
    offset_(Vector2::ZERO),
    radiusIndex_(-1),
    scaleIndex_(-1),
    offsetIndex_(-1),
    kernel_(0),
    evaluator_(0)
{ }

AnlWorldModel::~AnlWorldModel()
{
    Clear();
    URHO3D_LOGINFO("~AnlWorldModel() - " + GetName());
}

void AnlWorldModel::RegisterObject(Context* context)
{
    context->RegisterFactory<AnlWorldModel>();

    URHO3D_LOGINFO("AnlWorldModel() - RegisterObject() : ... OK !");
}

bool AnlWorldModel::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    Clear();

    String extension = GetExtension(source.GetName());

    if (extension == ".xml")
        return BeginLoadFromXMLFile(source);

    URHO3D_LOGERROR("Unsupported file type");
    return false;
}

bool AnlWorldModel::EndLoad()
{
    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    return false;
}

bool AnlWorldModel::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XMLFile(context_);
    if (!loadXMLFile_->Load(source))
    {
        URHO3D_LOGERROR("Could not load AnlWorldModel");
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    XMLElement rootElem = loadXMLFile_->GetRoot("AnlWorldModel");
    if (!rootElem)
    {
        URHO3D_LOGERROR("Invalid AnlWorldModel");
        loadXMLFile_.Reset();
        return false;
    }

    return true;
}

void DumpAnlKernelInstruction(anl::CKernel& kernel, unsigned index)
{
    anl::InstructionListType& instructions = kernel.getInstructions();

    if (index >= instructions.Size())
        return;

    unsigned srcindex = 0;
    anl::SInstruction& instruction = instructions[index];
    {
        String sources;
        while (instruction.sources_[srcindex] != 1000000 && srcindex < MaxSourceCount)
        {
            sources += String(instruction.sources_[srcindex]);
            sources += " ";
            srcindex++;
        }

        URHO3D_LOGINFOF("AnlWorldModel() - ANLVM : Instruction cindex=%u opcode=%s(%u) val=%f sources=%s", index, anl::ANLModuleTypeVMStr[instruction.opcode_], instruction.opcode_, instruction.outfloat_, sources.Empty() ? "None" : sources.CString());
    }

    srcindex = 0;
    while (instruction.sources_[srcindex] != 1000000 && srcindex < MaxSourceCount)
    {
        DumpAnlKernelInstruction(kernel, instruction.sources_[srcindex]);
        srcindex++;
    }
}

bool AnlWorldModel::EndLoadFromXMLFile()
{
    URHO3D_LOGINFOF("AnlWorldModel() - EndLoadFromXMLFile : %s ...", GetName().CString());

    if (!loadXMLFile_)
        return false;

    XMLElement rootElem = loadXMLFile_->GetRoot("AnlWorldModel");

    if (rootElem.IsNull())
    {
        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... Format Unknown !", GetName().CString());
        return false;
    }

    if (!rootElem.GetChild("ANL"))
    {
        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... ANL Version Unknown !", GetName().CString());
        return false;
    }

    HiresTimer timer;
    timer.Reset();

    Clear();

    anlVersion_ = GetEnumValue(rootElem.GetChild("ANL").GetAttribute("version"), ANLVersionNameStr);

    unsigned numModules = 0;
    unsigned numRenderables = 0;

    Vector<unsigned> screenshotmoduleindexes;
    Vector<String> screenshotmodulenames;

    renderableModules_.Clear();
    renderableModulesNames_.Clear();

    if (anlVersion_ == ANLV1)
    {
        Vector<int> moduletypes;
        Vector<String> modulenames;
        // Pass 1 : Allocate Modules / set name & type
        {
            for (XMLElement m = rootElem.GetChild("Module"); m; m = m.GetNext("Module"))
            {
                int type = GetEnumValue(m.GetAttribute("type"), ANLModuleTypeV1Str);
                AnlModule module = 0;

                switch ((AnlModuleTypeV1)type)
                {
                case ANLV1_Constant:
                {
                    anl::CImplicitConstant* constant = new anl::CImplicitConstant();
                    constant->setConstant(m.GetFloat("value"));
                    module = constant;
                }
                break;

                case ANLV1_Gradient:
                {
                    anl::CImplicitGradient* gradient = new anl::CImplicitGradient();
                    gradient->setGradient(m.GetFloat("x1"), m.GetFloat("x2"), m.GetFloat("y1"), m.GetFloat("y2"));
                    module = gradient;
                }
                break;

                case ANLV1_Radial:
                {
                    anl::CImplicitRadial* radial =  new anl::CImplicitRadial();
                    radial->setRadius(m.GetFloat("radius"));
                    module = radial;
                }
                break;

                case ANLV1_Sphere:
                {
                    anl::CImplicitSphere* sphere = new anl::CImplicitSphere();
                    module = sphere;
                }
                break;

                case ANLV1_Fractal:
                {
                    int fractype = GetEnumValue(m.GetAttribute("fractaltype"), ANLFractalTypesStr);
                    int basistype = GetEnumValue(m.GetAttribute("basistype"), ANLBasisTypesStr);
                    int interptype = GetEnumValue(m.GetAttribute("interptype"), ANLInterpTypesStr);
                    if (fractype < 0 || basistype < 0 || interptype < 0)
                    {
                        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... Fractal|Basis|Interpolate type undefined ! break", GetName().CString());
                        continue;
                    }
                    anl::CImplicitFractal* fractal = new anl::CImplicitFractal(fractype, basistype, interptype);
                    fractal->setNumOctaves(m.GetInt("octaves"));
                    fractal->setFrequency(m.GetFloat("frequency"));
                    module = fractal;
                }
                break;

                case ANLV1_AutoCorrect:
                {
                    anl::CImplicitAutoCorrect* autocorrect = new anl::CImplicitAutoCorrect();
                    autocorrect->setRange(m.GetFloat("low"), m.GetFloat("high"));
                    module = autocorrect;
                }
                break;

                case ANLV1_Combiner:
                {
                    int combinertype = GetEnumValue(m.GetAttribute("operation"), ANLCombinerTypesStr);
                    anl::CImplicitCombiner* combiner = new anl::CImplicitCombiner(combinertype);
                    module = combiner;
                }
                break;

                case ANLV1_Select:
                    module = new anl::CImplicitSelect();
                    break;

                case ANLV1_Blend:
                    module = new anl::CImplicitBlend();
                    break;

                case ANLV1_Cache:
                    module = new anl::CImplicitCache();
                    break;

                case ANLV1_CacheArray:
                    module = new anl::CImplicitCacheArray();
                    break;

                case ANLV1_Bias:
                    module = new anl::CImplicitBias(m.GetFloat("bias"));
                    break;

                case ANLV1_Clamp:
                    module = new anl::CImplicitClamp(m.GetFloat("low"), m.GetFloat("high"));
                    break;

                case ANLV1_ScaleOffset:
                    module = new anl::CImplicitScaleOffset(m.GetFloat("scale"), m.GetFloat("offset"));
                    break;

                case ANLV1_ScaleDomain:
                    module = new anl::CImplicitScaleDomain();
                    break;

                case ANLV1_TranslateDomain:
                    module = new anl::CImplicitTranslateDomain();
                    break;
                }

                if (module)
                {
                    modulenames.Push(m.GetAttribute("name"));
                    moduletypes.Push(type);
                    modules_.Push(module);

                    numModules++;

                    if (m.HasAttribute("renderable"))
                    {
                        int size;
                        int maptype = GetEnumValue(m.GetAttribute("renderable"), MapTypeStr);
                        size = maptype + 1;

                        if (size > renderableModules_.Size())
                        {
                            renderableModules_.Resize(size);
                            renderableModulesNames_.Resize(size);
                        }
                        renderableModules_[maptype] = modules_.Size()-1;
                        renderableModulesNames_[maptype] = modulenames.Back();
                    }
                }
            }
        }

        // Pass 2 : Resolve Sources/Attributes
        {
            int i=0;
            int index;
            String modulename;

            for (XMLElement m = rootElem.GetChild("Module"); m; m = m.GetNext("Module"))
            {
                modulename = m.GetAttribute("name");
                FindIndex(modulename, modulenames, &i);
                if (i == -1)
                {
                    URHO3D_LOGWARNINGF("AnlWorldModel() - EndLoadFromXMLFile : ... Not Allocated Module=%s !", modulename.CString());
                    continue;
                }

                if (m.HasAttribute("source"))
                {
                    if (IsDigital(m.GetAttribute("source")))
                        ((anl::CImplicitModuleBase*) modules_[i])->setSource(m.GetFloat("source"));
                    else
                    {
                        Vector<String> sources = m.GetAttribute("source").Split(';');
                        unsigned j=0;
                        for (Vector<String>::ConstIterator it=sources.Begin(); it != sources.End(); ++it)
                        {
                            FindIndex(*it, modulenames, &index);
                            if (index != -1)
                            {
                                ((anl::CImplicitModuleBase*)modules_[i])->setSource((anl::CImplicitModuleBase*)modules_[index], j);
                                j++;
                            }
                            else
                                URHO3D_LOGWARNINGF("AnlWorldModel() - EndLoadFromXMLFile : ... module=%s ...NO srcModule=%s !",
                                                   modulename.CString(), it->CString());
                        }
                    }
                }

                switch (moduletypes[i])
                {
                case ANLV1_Sphere:
                    if (m.HasAttribute("cx"))
                    {
                        String cx = m.GetAttribute("cx");
                        if (IsDigital(cx))
                            ((anl::CImplicitSphere*) modules_[i])->setCenterX(m.GetFloat("cx"));
                        else
                        {
                            FindIndex(cx, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSphere*) modules_[i])->setCenterX((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    if (m.HasAttribute("cx"))
                    {
                        String cy = m.GetAttribute("cy");
                        if (IsDigital(cy))
                            ((anl::CImplicitSphere*) modules_[i])->setCenterY(m.GetFloat("cy"));
                        else
                        {
                            FindIndex(cy, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSphere*) modules_[i])->setCenterY((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    if (m.HasAttribute("cz"))
                    {
                        String cz = m.GetAttribute("cz");
                        if (IsDigital(cz))
                            ((anl::CImplicitSphere*) modules_[i])->setCenterZ(m.GetFloat("cz"));
                        else
                        {
                            FindIndex(cz, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSphere*) modules_[i])->setCenterZ((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    if (m.HasAttribute("radius"))
                    {
                        String radius = m.GetAttribute("radius");
                        if (IsDigital(radius))
                            ((anl::CImplicitSphere*) modules_[i])->setRadius(m.GetFloat("radius"));
                        else
                        {
                            FindIndex(radius, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSphere*) modules_[i])->setRadius((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    break;

                case ANLV1_Select:
                    if (m.HasAttribute("low"))
                    {
                        String attr = m.GetAttribute("low");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSelect*) modules_[i])->setLowSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitSelect*) modules_[i])->setLowSource(m.GetFloat("low"));
                    }
                    if (m.HasAttribute("high"))
                    {
                        String attr = m.GetAttribute("high");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSelect*) modules_[i])->setHighSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitSelect*) modules_[i])->setHighSource(m.GetFloat("high"));
                    }
                    if (m.HasAttribute("control"))
                    {
                        String attr = m.GetAttribute("control");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSelect*) modules_[i])->setControlSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitSelect*) modules_[i])->setControlSource(m.GetFloat("control"));
                    }
                    if (m.HasAttribute("threshold"))
                    {
                        String attr = m.GetAttribute("threshold");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSelect*) modules_[i])->setThreshold((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitSelect*) modules_[i])->setThreshold(m.GetFloat("threshold"));
                    }
                    if (m.HasAttribute("falloff"))
                    {
                        String attr = m.GetAttribute("falloff");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitSelect*) modules_[i])->setFalloff((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitSelect*) modules_[i])->setFalloff(m.GetFloat("falloff"));
                    }
                    break;

                case ANLV1_Blend:
                    if (m.HasAttribute("low"))
                    {
                        String attr = m.GetAttribute("low");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitBlend*) modules_[i])->setLowSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitBlend*) modules_[i])->setLowSource(m.GetFloat("low"));
                    }
                    if (m.HasAttribute("high"))
                    {
                        String attr = m.GetAttribute("high");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitBlend*) modules_[i])->setHighSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitBlend*) modules_[i])->setHighSource(m.GetFloat("high"));
                    }
                    if (m.HasAttribute("control"))
                    {
                        String attr = m.GetAttribute("control");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitBlend*) modules_[i])->setControlSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitBlend*) modules_[i])->setControlSource(m.GetFloat("control"));
                    }
                    break;

                case ANLV1_Bias:
                    if (m.HasAttribute("bias"))
                    {
                        String attr = m.GetAttribute("bias");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitBias*) modules_[i])->setBias((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    break;

                case ANLV1_ScaleOffset:
                    if (m.HasAttribute("scale"))
                    {
                        String attr = m.GetAttribute("scale");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitScaleOffset*) modules_[i])->setScale((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    if (m.HasAttribute("offset"))
                    {
                        String attr = m.GetAttribute("offset");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitScaleOffset*) modules_[i])->setOffset((anl::CImplicitModuleBase*)modules_[index]);
                        }
                    }
                    break;

                case ANLV1_ScaleDomain:
                    if (m.HasAttribute("scalex"))
                    {
                        String attr = m.GetAttribute("scalex");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitScaleDomain*) modules_[i])->setXScale((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitScaleDomain*) modules_[i])->setXScale(m.GetFloat("scalex"));
                    }
                    if (m.HasAttribute("scaley"))
                    {
                        String attr = m.GetAttribute("scaley");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitScaleDomain*) modules_[i])->setYScale((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitScaleDomain*) modules_[i])->setYScale(m.GetFloat("scaley"));
                    }
                    break;

                case ANLV1_TranslateDomain:
                    if (m.HasAttribute("tx"))
                    {
                        String attr = m.GetAttribute("tx");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitTranslateDomain*) modules_[i])->setXAxisSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitTranslateDomain*) modules_[i])->setXAxisSource(m.GetFloat("tx"));
                    }
                    if (m.HasAttribute("ty"))
                    {
                        String attr = m.GetAttribute("ty");
                        if (!IsDigital(attr))
                        {
                            FindIndex(attr, modulenames, &index);
                            if (index != -1)
                                ((anl::CImplicitTranslateDomain*) modules_[i])->setYAxisSource((anl::CImplicitModuleBase*)modules_[index]);
                        }
                        else
                            ((anl::CImplicitTranslateDomain*) modules_[i])->setYAxisSource(m.GetFloat("ty"));
                    }
                    break;
                }

                // Get WorldEllipse Parameters
                if (modulename == "radial")
                {
                    anl::CImplicitRadial* module = (anl::CImplicitRadial*)modules_[i];
                    if (radius_ == 0.f)
                        radius_ = module->getRadius();
                    else
                        module->setRadius(radius_);
                    radiusIndex_ = i;
                }
                else if (modulename == "radialshape")
                {
                    anl::CImplicitScaleDomain* module = (anl::CImplicitScaleDomain*)modules_[i];
                    if (scale_ == Vector2::ONE)
                    {
                        scale_.x_ = module->getXScale();
                        scale_.y_ = module->getYScale();
                    }
                    else
                    {
                        module->setXScale(scale_.x_);
                        module->setYScale(scale_.y_);
                    }
                    scaleIndex_ = i;
                }
                else if (modulename == "ellipsoid")
                {
                    anl::CImplicitTranslateDomain* module = (anl::CImplicitTranslateDomain*)modules_[i];
                    offset_.x_ = module->getXTranslate();
                    offset_.y_ = module->getYTranslate();
                    offsetIndex_ = i;
                }

                if (m.HasAttribute("screenshot"))
                {
                    if (m.GetBool("screenshot") == true)
                    {
                        screenshotModules_.Push(i);
                        screenshotModuleNames_.Push(modulename);
                    }
                }
            }
        }

    }
    else if (anlVersion_ == ANLVM)
    {
        if (!kernel_)
            kernel_ = new anl::CKernel();

        if (!kernel_)
        {
            URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... ANLVM allocation kernel error !", GetName().CString());
            return false;
        }

        anl::CKernel& kernel = *(anl::CKernel*)kernel_;

        modulesources_.Clear();

        //Vector<unsigned int> caches;
        Vector<unsigned int> sources;
        Vector<float> constants;

        for (XMLElement m = rootElem.GetChild("Module"); m; m = m.GetNext("Module"))
        {
            int type = GetEnumValue(m.GetAttribute("type"), anl::ANLModuleTypeVMStr);
            String name = m.GetAttribute("name");

            unsigned int cindex;

            if (type == -1)
            {
                URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... ANLVM type=%s unknown type !", GetName().CString(), m.GetAttribute("type").CString());
                return false;
            }

            Vector<String> params = m.GetAttribute("params").Split(' ');

            if (type < anl::OP_BuiltInMacros)
            {
                sources.Clear();

                if (type == anl::OP_Constant)
                {
                    cindex = kernel.constant(Urho3D::ToFloat(params[0])).GetIndex();
                }
                else if (type == anl::OP_Seed)
                {
                    if (name == "seed")
                    {
                        if (seed_ == 0U)
                            seed_ = Urho3D::ToInt(params[0]);

                        cindex = kernel.seed(seed_).GetIndex();

                        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : set seed=%u !", seed_);
                    }
                    else
                    {
                        cindex = kernel.seed(Urho3D::ToInt(params[0])).GetIndex();
                    }
                }
                else
                {
                    for (Vector<String>::ConstIterator it=params.Begin(); it != params.End(); ++it)
                    {
                        const String& value = *it;

                        if (IsDigital(value)) // value is digital
                        {
                            if (type == anl::OP_Seed)
                                sources.Push(kernel.seed(Urho3D::ToInt(value)).GetIndex());
                            else
                                sources.Push(kernel.constant(Urho3D::ToFloat(value)).GetIndex());
                        }
                        else
                        {
                            int v = GetEnumValue(value);
                            if (v != -1) // value is enum
                            {
                                sources.Push(kernel.constant(v).GetIndex());
                            }
                            else // value is source name, get index
                            {
                                HashMap<String, unsigned int>::ConstIterator mt = modulesources_.Find(value);
                                if (mt != modulesources_.End())
                                    sources.Push(mt->second_);
                                else
                                {
                                    URHO3D_LOGWARNINGF("AnlWorldModel() - EndLoadFromXMLFile : value=%s is not a valid source !", value.CString());
                                }
                            }
                        }
                    }

                    cindex = kernel.pushInstruction(type, sources.Size(), sources.Size() ? &sources[0] : 0).GetIndex();
                }

                /*
                if (type == ANLVM_CacheArray)
                    caches.Push(cindex);
                */
#ifdef ANLVM_DUMPKERNEL
                String sourcesStr;
                for (int i=0; i < sources.Size(); i++)
                    sourcesStr += String(sources[i]) + " ";

                URHO3D_LOGINFOF("AnlWorldModel() - ANLVM : Instruction Name=%s Type=%s(%d) cindex=%u numSrc=%u srcInd=%s",
                                m.GetAttributeCString("name"), m.GetAttributeCString("type"), type, cindex, sources.Size(), sourcesStr.CString());
#endif
            }
            else
            {
                constants.Clear();
                for (Vector<String>::ConstIterator it=params.Begin(); it != params.End(); ++it)
                {
                    const String& value = *it;
                    if (IsDigital(value)) // value is digital
                    {
                        constants.Push(Urho3D::ToFloat(value));
                    }
                    else
                    {
                        int v = GetEnumValue(value);
                        if (v != -1) // value is enum
                        {
                            constants.Push(v);
                        }
                        else if (value == "seed")
                        {
                            constants.Push(seed_);
                        }
                    }
                }

                if (constants.Size() < 5)
                {
                    URHO3D_LOGWARNINGF("AnlWorldModel() - EndLoadFromXMLFile : type=%s need 5 constants (size=%u) !", m.GetAttribute("type").CString(), constants.Size());
                    continue;
                }

                if (type == anl::OP_SimpleFBM)
                    cindex = kernel.simplefBm((int)anl::OP_ValueBasis+constants[0], constants[1], constants[2], constants[3], constants[4]).GetIndex();
                else if (type == anl::OP_SimpleRidgedMF)
                    cindex = kernel.simpleRidgedMultifractal((int)anl::OP_ValueBasis+constants[0], constants[1], constants[2], constants[3], constants[4]).GetIndex();
                else if (type == anl::OP_SimpleBillow)
                    cindex = kernel.simpleBillow((int)anl::OP_ValueBasis+constants[0], constants[1], constants[2], constants[3], constants[4]).GetIndex();
#ifdef ANLVM_DUMPKERNEL
                String constantsStr;
                for (int i=0; i < constants.Size(); i++)
                    constantsStr += String(constants[i]) + " ";
                URHO3D_LOGINFOF("AnlWorldModel() - ANLVM : Instruction Name=%s Type=%s(%d) cindex=%u numconst=%u constants=%s",
                                m.GetAttributeCString("name"), m.GetAttributeCString("type"), type, cindex, constants.Size(), constantsStr.CString());
#endif
            }

            modulesources_[name] = cindex;
            numModules++;

            if (m.HasAttribute("renderable"))
            {
                int maptype = GetEnumValue(m.GetAttribute("renderable"), MapTypeStr);
                if (maptype !=-1)
                {
                    int size = maptype + 1;
                    if (size > renderableModules_.Size())
                    {
                        renderableModules_.Resize(size);
                        renderableModulesNames_.Resize(size);
                    }
                    renderableModules_[maptype] = cindex;
                    renderableModulesNames_[maptype] = name;
                }
            }

            // Get WorldEllipse Parameters
            {
                anl::InstructionListType& instructions = kernel.getInstructions();
                if (name == "radialshape")
                {
                    unsigned iparam = instructions[cindex].sources_[0];
                    if (radius_ == 0.f)
                    {
                        radius_ = instructions[iparam].outfloat_; // params="5 radial"
                        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : get radius=%F !", radius_);
                    }
                    else
                    {
                        instructions[iparam].outfloat_ = radius_;
                        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : set radius=%F !", radius_);
                    }

                    radiusIndex_ = iparam;
                }
                else if (name == "groundshape")
                {
                    unsigned iparam = instructions[cindex].sources_[1];
                    if (scale_ == Vector2::ONE)
                    {
                        scale_.x_ = instructions[iparam].outfloat_; // params="radialshape 0.3"
                        scale_.y_ = 1.f;
                    }
                    else
                    {
                        instructions[iparam].outfloat_ = scale_.x_;
                        URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : set scaleX=%F !", scale_.x_);
                    }

                    scaleIndex_ = iparam;
                }
            }


            if (m.HasAttribute("screenshot"))
            {
                if (m.GetBool("screenshot") == true)
                {
                    screenshotModules_.Push(cindex);
                    screenshotModuleNames_.Push(name);
                }
            }
        }

        // set radius to one if not exist
        if (radius_ == 0.f)
            radius_ = 1.f;

        for (int i=0; i < renderableModulesNames_.Size(); i++)
        {
            unsigned module = renderableModules_[i];
            if (!module)
                continue;

            numRenderables++;

#ifdef ANLVM_DUMPKERNEL
            URHO3D_LOGINFOF("AnlWorldModel() - ANLVM : Instruction Name=%s cindex=%u maptype=%s Renderable", renderableModulesNames_[i].CString(), module, MapTypeStr[i]);

            if (i == TESTMAP)
                DumpAnlKernelInstruction(kernel, module);
#endif
        }

        URHO3D_LOGINFOF("AnlWorldModel() - ANLVM : total num instructions=%u numcached=%u", kernel.getKernel()->Size(), kernel.getCachedIndexes().Size());
    }
    // new version
    else
    {
        if (!kernel_)
            kernel_ = new anl::InstructionList(500);

        if (!kernel_)
        {
            URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... ANLV2 allocation kernel error !", GetName().CString());
            return false;
        }

        anl::InstructionList& kernel = *(anl::InstructionList*)kernel_;

        HashMap<String, unsigned int> modulesources;
        for (XMLElement m = rootElem.GetChild("Module"); m; m = m.GetNext("Module"))
        {
            int type = GetEnumValue(m.GetAttribute("type"), anl::ANLModuleTypeV2Str);
            String name = m.GetAttribute("name");
            unsigned cindex;

            if (type == -1)
            {
                URHO3D_LOGERRORF("AnlWorldModel() - EndLoadFromXMLFile : %s ... ANLV2 type=%s unknown type !", GetName().CString(), m.GetAttribute("type").CString());
                return false;
            }

            Vector<String> params = m.GetAttribute("params").Split(' ');

            anl::InstructionParams datas;
            for (Vector<String>::ConstIterator it=params.Begin(); it != params.End(); ++it)
            {
                const String& value = *it;

                if (IsDigital(value)) // value is digital
                {
                    /// TODO : Separate Seed(unsigned) & Constant(float)
                    datas.push_back(anl::InstructionParam(Urho3D::ToFloat(value)));
                }
                else
                {
                    int v = GetEnumValue(value);
                    if (v != -1) // value is enum
                        datas.push_back(anl::InstructionParam(v));
                    else // value is source name, get index
                    {
                        HashMap<String, unsigned int>::ConstIterator mt = modulesources.Find(value);
                        if (mt != modulesources.End())
                            datas.push_back(anl::InstructionParam(mt->second_, true));
                        else
                            URHO3D_LOGWARNINGF("AnlWorldModel() - EndLoadFromXMLFile : ANLV2 value=%s is not a valid source !", value.CString());
                    }
                }
            }

            if (type > anl::OP_BuiltInMacros)
                cindex = kernel.PushBuiltInMacro(type, datas);
            else
                cindex = kernel.PushInstruction(type, datas);

#ifdef ANLVM_DUMPKERNEL
            URHO3D_LOGINFOF("AnlWorldModel() - ANLV2 : Instruction Name=%s Type=%s(%d) %s",
                            m.GetAttributeCString("name"), m.GetAttributeCString("type"), type, kernel.GetInstruction(cindex).GetParamStr().CString());
#endif

            modulesources[name] = cindex;
            numModules++;

            if (m.HasAttribute("renderable"))
            {
                renderableModules_.Push(cindex);
                renderableModulesNames_.Push(name);
            }

            if (m.HasAttribute("screenshot"))
            {
                if (m.GetBool("screenshot") == true)
                {
                    screenshotModules_.Push(cindex);
                    screenshotModuleNames_.Push(name);
                }
            }
        }

        kernel.DumpInstructions();

        for (int i=0; i < renderableModulesNames_.Size(); i++)
            kernel.BuildTree(renderableModules_[i]);

#ifdef ANLVM_DUMPKERNEL
        for (int i=0; i < renderableModulesNames_.Size(); i++)
        {
            URHO3D_LOGINFOF("AnlWorldModel() - ANLV2 : Instruction Name=%s cindex=%u Renderable", renderableModulesNames_[i].CString(), renderableModules_[i]);
            kernel.DumpTree(*kernel.GetTree(renderableModules_[i]));
        }
#endif

        URHO3D_LOGINFOF("AnlWorldModel() - ANLV2 : total num instructions=%u", kernel.GetNumInstructions());
    }

//    loadXMLFile_.Reset();

    URHO3D_LOGINFOF("AnlWorldModel() - EndLoadFromXMLFile : ...  %s ... NumModules=%u NumRenderables=%u... timer=%d msec ... OK !",
                    GetName().CString(), numModules, numRenderables, timer.GetUSec(false) / 1000);
    return true;
}

void AnlWorldModel::SetSeedAllModules(unsigned seed)
{
    URHO3D_LOGINFOF("AnlWorldModel() - SetSeedAllModules : seed=%u", seed);

    if (seed_ != seed)
    {
        seed_ = seed;

        if (GetAnlVersion() == ANLV1)
        {
            for (unsigned i=0; i < modules_.Size(); ++i)
                ((anl::CImplicitModuleBase*)modules_[i])->setSeed(seed);
        }
        else if (GetAnlVersion() == ANLVM)
        {
            bool state = EndLoadFromXMLFile();
        }
        else
        {
            /// TODO
        }
    }
}

void AnlWorldModel::SetRadius(float radius)
{
    if (radiusIndex_ == -1)
        return;

    if (anlVersion_ == ANLV1)
    {
        anl::CImplicitRadial* module = (anl::CImplicitRadial*)modules_[radiusIndex_];
        module->setRadius(radius);
        radius_ = radius;
    }
    else if (anlVersion_ == ANLVM)
    {
        anl::CKernel& kernel = *(anl::CKernel*)kernel_;
        kernel.getInstructions()[radiusIndex_].outfloat_ = radius;
        radius_ = radius;
        URHO3D_LOGINFOF("AnlWorldModel() - SetRadius : radius=%F", radius_);
    }
    else
    {

    }
}

void AnlWorldModel::SetScale(const Vector2& scale)
{
    if (scaleIndex_ == -1)
        return;

    if (anlVersion_ == ANLV1)
    {
        anl::CImplicitScaleDomain* module = (anl::CImplicitScaleDomain*)modules_[scaleIndex_];
        module->setXScale(scale.x_);
        module->setYScale(scale.y_);
        scale_ = scale;
    }
    else if (anlVersion_ == ANLVM)
    {
        anl::CKernel& kernel = *(anl::CKernel*)kernel_;
        kernel.getInstructions()[scaleIndex_].outfloat_ = scale.x_;
        scale_ = scale;
    }
    else
    {
        ;
    }
}

bool AnlWorldModel::AreThreadFinished() const
{
//	if (mapGenWorkInfos_.Size())
//		return false;

    if (!anl::isChunkWorkersFinished())
        return false;

    return true;
}

enum SnapShotGenerationState
{
    SnapShotGenerationNotStarted = 0,
    SnapShotGenerationStarted = 1,
    SnapShotGenerationFinished = 2,
    SnapShotGenerationSaved = 3
};

void AnlWorldModel::StopUnfinishedWorks()
{
    if (!anl::isChunkWorkersFinished())
        anl::StopChunkWorkers();

    snapShotWorkInfos_.Clear();
}

void AnlWorldModel::GenerateSnapShots(int size, unsigned color, const Vector2& center, float scale, unsigned nummodules, String* renderableModuleNames, Texture2D* texture)
{
    const float diameter = radiusIndex_ != -1 ? 2.f * radius_ : 1.f;
    scale *= diameter;

    AnlMappingRange mapping;
    mapping.mapx0 = ((float)center.x_) * AnlWorldModelGranularity * scale;
    mapping.mapx1 = ((float)center.x_ + 1.f) * AnlWorldModelGranularity * scale;
    mapping.mapy0 = ((float)-center.y_) * AnlWorldModelGranularity * scale;
    mapping.mapy1 = ((float)-center.y_ + 1.f) * AnlWorldModelGranularity * scale;

    URHO3D_LOGINFOF("AnlWorldModel() - GenerateSnapShots : %s ... size=%d center=%F,%F scale=%F ...",
                    GetName().CString(), size, center.x_, center.y_, scale);

    if (anlVersion_ == ANLV1)
    {
        Image *image = new Image(context_);
        image->SetSize(size, size, 4);

        for (int i=0; i < screenshotModules_.Size(); i++)
        {
            const String& modulename = screenshotModuleNames_[i];
            unsigned imodule = screenshotModules_[i];

            String fname = modulename + "_" + String(size) + ".png";

            anl::CArray2Dd data(size, size);
            anl::CImplicitModuleBase& module = *(anl::CImplicitModuleBase*)modules_[imodule];
            anl::map2DNoZ((int)anl::SEAMLESS_NONE, data, module, mapping);

            for(int x=0; x < size; ++x)
                for(int y=0; y < size; ++y)
                    image->SetPixelInt(x, y, data.get(x,y) > 0.1f ? color : 0U);

            image->SavePNG(fname);

            URHO3D_LOGINFOF("AnlWorldModel() - GenerateSnapShots : ANLV1 Generate SnapShot ...  %s ... %s(%u) .. OK !",
                            GetName().CString(), modulename.CString(), imodule);
        }

        delete image;
    }
    else
    {
        bool useRenderableModules = nummodules && renderableModuleNames;
        unsigned numModuleNames = useRenderableModules ? nummodules : screenshotModules_.Size();
        String* moduleNames = useRenderableModules ? renderableModuleNames : screenshotModuleNames_.Buffer();

        for (int i=0; i < numModuleNames; i++)
        {
            const String& modulename = moduleNames[i];

            int maptype = useRenderableModules ? GetEnumValue(modulename, MapTypeStr) : 0;
            if (maptype == -1)
            {
                URHO3D_LOGERRORF("AnlWorldModel() - GenerateSnapShots : ... can't find renderable module = %s !", modulename.CString());
                continue;
            }

            unsigned imodule = useRenderableModules && maptype < renderableModules_.Size() ? renderableModules_[maptype] : screenshotModules_[i];

            snapShotWorkInfos_.Resize(snapShotWorkInfos_.Size()+1);
            SnapShotWorkInfo& workinfo = snapShotWorkInfos_.Back();
            workinfo.state_ = SnapShotGenerationNotStarted;
            workinfo.anlversion_ = anlVersion_;
            workinfo.snapShotName_ = modulename + "_" + String(size);
            workinfo.snapShotTexture_ = texture;
            workinfo.snapShotColor_ = color;
            workinfo.imodule_ = imodule;
            workinfo.mapping_ = mapping;
            workinfo.pixelSize_ = size;

            URHO3D_LOGINFOF("AnlWorldModel() - GenerateSnapShots : %s Generate SnapShot ... %s ... %s(%u) .. launched !",
                            ANLVersionNameStr[anlVersion_], GetName().CString(), modulename.CString(), imodule);
        }
    }
}

void AnlWorldModel::GenerateWorldMapShots(const String& name, const Vector<String> modulenames)
{
    const float factor = 2.f / (scaleIndex_ != -1 && scale_.x_ != 0.f ? scale_.x_ : 1.f);
    const float diameter = radiusIndex_ != -1 ? 2.f * radius_ : 1.f;
    const float globalScale = diameter * factor;
    const int maprange = CeilToInt(globalScale / 2);
    const int pixsize = 64;

    for (int y=-maprange; y <= maprange; y++)
    {
        for (int x=-maprange; x <= maprange; x++)
        {
            for (int i=0; i < modulenames.Size(); i++)
            {
                const String& modulename = modulenames[i];

                int maptype = GetEnumValue(modulename, MapTypeStr);
                if (maptype == -1)
                {
                    URHO3D_LOGERRORF("AnlWorldModel() - GenerateWorldMapShots : ... can't find renderable module = %s !", modulename.CString());
                    continue;
                }

                unsigned imodule = renderableModules_[maptype];

                snapShotWorkInfos_.Resize(snapShotWorkInfos_.Size()+1);

                SnapShotWorkInfo& workinfo = snapShotWorkInfos_.Back();
                workinfo.state_ = SnapShotGenerationNotStarted;
                workinfo.anlversion_ = anlVersion_;
                workinfo.snapShotName_ = name + "_" + String(x) + "_" + String(y) + "_" + modulename + "_" + ".png";
                workinfo.imodule_ = imodule;
                workinfo.mapping_.mapx0 = ((float)x ) * AnlWorldModelGranularity;
                workinfo.mapping_.mapx1 = ((float)x + 1.f) * AnlWorldModelGranularity;
                workinfo.mapping_.mapy0 = ((float)-y) * AnlWorldModelGranularity;
                workinfo.mapping_.mapy1 = ((float)-y + 1.f) * AnlWorldModelGranularity;
                workinfo.pixelSize_ = pixsize;
            }
        }
    }
}

bool AnlWorldModel::UpdateGeneratingSnapShots()
{
//	URHO3D_LOGINFOF("AnlWorldModel() - UpdateGeneratingSnapShots : ANLVM Generate SnapShot ... %s ... snapShotWorkInfos_ Size = %u !", GetName().CString(), snapShotWorkInfos_.Size());

    if (!snapShotWorkInfos_.Size())
        return false;

    Vector<unsigned> notStartedSnaps, startedSnaps, finishedSnaps, savedSnaps;

    // Check States
    for (unsigned i=0; i < snapShotWorkInfos_.Size(); i++)
    {
        SnapShotWorkInfo& workinfo = snapShotWorkInfos_[i];

        if (workinfo.state_ == SnapShotGenerationNotStarted)
        {
            notStartedSnaps.Push(i);
        }
        else if (workinfo.state_ == SnapShotGenerationStarted)
        {
            if (anl::isChunkWorkersFinished())
            {
                workinfo.state_ = SnapShotGenerationFinished;
                finishedSnaps.Push(i);
            }
            else
            {
                startedSnaps.Push(i);
            }
        }
        else if (workinfo.state_ == SnapShotGenerationFinished)
        {
            finishedSnaps.Push(i);
        }
        else if (workinfo.state_ == SnapShotGenerationSaved)
        {
            savedSnaps.Push(i);
        }
    }

    // Start a work
    if (notStartedSnaps.Size() && !startedSnaps.Size() && AreThreadFinished())
    {
        SnapShotWorkInfo& workinfo = snapShotWorkInfos_[notStartedSnaps.Front()];
        workinfo.storage_.resize(workinfo.pixelSize_, workinfo.pixelSize_);
        workinfo.state_ = SnapShotGenerationStarted;

        const int numThreads = 2;

        URHO3D_LOGERRORF("AnlWorldModel() - UpdateGeneratingSnapShots : %s Generate SnapShot Started ... %s ... %s(%u)",
                         ANLVersionNameStr[anlVersion_], GetName().CString(), workinfo.snapShotName_.CString(), workinfo.imodule_);

        // with VM, anl::map2DNoZ or anl::savefloatArray seem to be bugged (=>repeated gradient, not infinite), so use Urho3D::Image

        if (workinfo.anlversion_ == ANLVM)
        {
            anl::CKernel& kernel = *(anl::CKernel*)kernel_;
            anl::CNoiseExecutor::ResizeCache(kernel, SNAPGENSLOT, workinfo.pixelSize_, workinfo.pixelSize_, numThreads);
            anl::CNoiseExecutor::GetCache(SNAPGENSLOT).ResetCacheAccess();
            anl::maptobuffer(numThreads, workinfo.storage_, kernel, anl::CInstructionIndex(workinfo.imodule_), (int)anl::SEAMLESS_NONE, workinfo.mapping_);
        }
        else if (workinfo.anlversion_ == ANLV2)
        {
            anl::InstructionList& kernel = *(anl::InstructionList*)kernel_;
            anl::Evaluator::ResizeCache(kernel, SNAPGENSLOT, workinfo.pixelSize_, workinfo.pixelSize_, numThreads);
            anl::Evaluator::GetCache(SNAPGENSLOT).ResetCacheAccess();
            anl::maptobufferV2(numThreads, workinfo.storage_, kernel, workinfo.imodule_, (int)anl::SEAMLESS_NONE, workinfo.mapping_);
        }
    }

    // Save finished Snaps
    if (finishedSnaps.Size())
    {
        Image* image = new Image(context_);

        for (unsigned i=0; i < finishedSnaps.Size(); i++)
        {
            SnapShotWorkInfo& workinfo = snapShotWorkInfos_[finishedSnaps[i]];

            image->SetSize(workinfo.pixelSize_, workinfo.pixelSize_, 4);

            for(int x=0; x < workinfo.pixelSize_; ++x)
                for(int y=0; y < workinfo.pixelSize_; ++y)
                    image->SetPixelInt(x, y, workinfo.storage_.get(x,y) > 0.1f ? workinfo.snapShotColor_ : 0U);

            if (workinfo.snapShotTexture_)
            {
                if (workinfo.snapShotTexture_->SetSize(image->GetWidth(), image->GetHeight(), Graphics::GetRGBAFormat(), TEXTURE_DYNAMIC, 0))
                    workinfo.snapShotTexture_->SetData(0, 0, 0, image->GetWidth(), image->GetHeight(), image->GetData());
            }
            else if (!workinfo.snapShotName_.Empty())
            {
                image->SavePNG(workinfo.snapShotName_ + ".png");
            }

            workinfo.state_ = SnapShotGenerationSaved;

            SendEvent(GAME_WORLDSNAPSHOTSAVED);

            URHO3D_LOGERRORF("AnlWorldModel() - UpdateGeneratingSnapShots : %s Generate SnapShot ... %s ... %s(%u) Saved ... OK !",
                             ANLVersionNameStr[anlVersion_], GetName().CString(), workinfo.snapShotName_.CString(), workinfo.imodule_);
        }

        delete image;

        savedSnaps.Clear();
        for (unsigned i=0; i < snapShotWorkInfos_.Size(); i++)
        {
            if (snapShotWorkInfos_[i].state_ == SnapShotGenerationSaved)
            {
                savedSnaps.Push(i);
            }
        }
    }

    // All Snaps are Saved : Clear the Work list
    if (savedSnaps.Size() == snapShotWorkInfos_.Size())
    {
        snapShotWorkInfos_.Clear();
        return false;
    }

    return true;
}


#ifdef ACTIVE_WORLD2D_THREADING

void MapGenThread(const WorkItem* item, unsigned threadIndex)
{
    MapGenWorkInfo& info = *reinterpret_cast<MapGenWorkInfo*>(item->aux_);

    HiresTimer timer;

//    URHO3D_LOGINFOF("MapGenThread : thread=%d ... %s ... start with ystart=%u ysize=%u ... ", info.ithread_, info.genStatus_->mappoint_.ToString().CString(), info.ystart_, info.ysize_);

    const int width = info.genStatus_->width_;
    const unsigned ystart = info.ystart_;
    const unsigned yend = ystart + info.ysize_;
    const AnlMappingRange& range = info.genStatus_->mappingRange_;
    const float tilingx2world = (range.mapx1 - range.mapx0) / (float)width;
    const float tilingy2world = (range.mapy1 - range.mapy0) / (float)info.genStatus_->height_;
    const Vector<unsigned int>& modules = *info.modules_;

    // create an executor for the thread
    anl::CNoiseExecutor nexec(*info.kernel_, MAPGENSLOT, info.ithread_);
    nexec.ResetCacheAccess(true);
    nexec.SetCacheAccessor(MAPGENSLOT, ystart * width, yend * width -1);

    anl::CCoordinate coord;
    coord.dimension_ = 2;

    for (unsigned imodule=0; imodule < info.modules_->Size(); ++imodule)
    {
        unsigned cindex = modules[imodule];

        nexec.StartEvaluation();

        FeatureType feat;

        if (!info.genStatus_->features_[imodule])
        {
            URHO3D_LOGWARNINGF("MapGenThread : thread=%d ... %s ... ERROR no featuremap for module=%u(iid=%u) ... skip!", info.ithread_, info.genStatus_->mappoint_.ToString().CString(), imodule, cindex);
            continue;
        }

        FeatureType* map = (FeatureType*)(info.genStatus_->features_[imodule]) + ystart * width;
        for (unsigned y=ystart; y<yend; ++y)
        {
            // check for breaking generation !
            if (info.genStatus_->status_ != Creating_Map_Layers)
            {
                URHO3D_LOGERRORF("MapGenThread : thread=%d ... %s ... BREAK !", info.ithread_, info.genStatus_->mappoint_.ToString().CString());
                return;
            }

            coord.y_ = range.mapy0 + (float)y * tilingy2world;

            if (imodule < TERRAINMAP)
            {
                for (unsigned x=0; x<width; ++x)
                {
                    coord.x_ = range.mapx0 + (float)x * tilingx2world;
                    feat = (FeatureType)MapFeatureType::OuterFloor * RoundToInt(nexec.EvaluateAt(cindex, coord).outfloat_);

                    *map = feat;
                    nexec.NextCacheAccessFastIndex();
                    map++;
                }
            }
            else if (imodule == TERRAINMAP)
            {
                for (unsigned x=0; x<width; ++x)
                {
                    coord.x_ = range.mapx0 + (float)x * tilingx2world;
                    feat = (FeatureType)Clamp(RoundToInt(nexec.EvaluateAt(cindex, coord).outfloat_ * (float)(terrainBound_)), 0, terrainBound_);

                    *map = feat;
                    nexec.NextCacheAccessFastIndex();
                    map++;
                }
            }
            else if (imodule != TESTMAP) // BIOMEMAP
            {
                for (unsigned x=0; x<width; ++x)
                {
                    coord.x_ = range.mapx0 + (float)x * tilingx2world;
                    feat = (FeatureType)RoundToInt(nexec.EvaluateAt(cindex, coord).outfloat_);

                    *map = feat;
                    nexec.NextCacheAccessFastIndex();
                    map++;
                }
            }
            else
            {
                float value;
                for (unsigned x=0; x<width; ++x)
                {
                    coord.x_ = range.mapx0 + (float)x * tilingx2world;
                    value = nexec.EvaluateAt(cindex, coord).outfloat_;

                    *map = value;
                    nexec.NextCacheAccessFastIndex();
                    map++;
                }
            }
        }

        // set cache available for cindex
        nexec.SetCacheAvailable(cindex);
    }

    info.time_ = (int)(timer.GetUSec(false) / 1000);

//    URHO3D_LOGINFOF("MapGenThread : thread=%d ... %s ... timer=%d msec ... OK !", info.ithread_, info.genStatus_->mappoint_.ToString().CString(), info.time_);
}

inline bool CreateMapGenWorkInfos(Context* context, MapGeneratorStatus& genStatus, List<MapGenWorkInfo>& moduleInfos, anl::CKernel& kernel, Vector<unsigned int>& renderableModules)
{
    WorkQueue* queue = GameContext::Get().gameWorkQueue_;

    queue->Pause();

    int numthreads = queue->GetNumThreads(); // Worker threads
    if (!numthreads) // if no Worker thread, Main Thread only
        numthreads = 1;

//    URHO3D_LOGINFOF("AnlWorldModel() - CreateMapGenWorkInfos : use numWorkItems=%u renderables=%u ... ", numthreads, renderableModules.Size());

    // create the datas for the threads
    if (moduleInfos.Size())
    {
        // check if the works are finished
        for (List<MapGenWorkInfo>::Iterator it = moduleInfos.Begin(); it != moduleInfos.End(); ++it)
        {
            if (!it->finished_)
            {
                URHO3D_LOGERRORF("AnlWorldModel() - CreateMapGenWorkInfos : Previous Works Unfinished ... Don't Add new Works  : TODO CORRECT THIS TO ALLOW ADDING WORKS !");
                return false;
            }
        }

//        URHO3D_LOGINFOF("AnlWorldModel() - CreateMapGenWorkInfos : mpoint=%s map=%s(%u) ... moduleInfos.Size=%u ... works finished !",
//                        genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none", genStatus.map_, moduleInfos.Size());
    }

    anl::CNoiseExecutor::ResizeCache(kernel, MAPGENSLOT, genStatus.width_, genStatus.height_, numthreads);
    int moduleysize = std::floor(genStatus.height_ / numthreads);

    if (moduleInfos.Size() < numthreads)
        moduleInfos.Resize(numthreads);

    int thread = 0;
    for (List<MapGenWorkInfo>::Iterator it = moduleInfos.Begin(); it != moduleInfos.End(); ++it, ++thread)
    {
        MapGenWorkInfo& info = *it;
        info.finished_ = false;
        info.time_ = 0;
        info.ithread_ = thread;
        info.ystart_ = thread * moduleysize;
        info.ysize_ = thread < numthreads-1 ? moduleysize : genStatus.height_-(numthreads-1)*moduleysize;
        info.kernel_ = &kernel;
        info.modules_ = &renderableModules;
        info.genStatus_ = &genStatus;
    }

    genStatus.Dump();

    // Add WorkerItems to WorkQueue
    for (List<MapGenWorkInfo>::Iterator it = moduleInfos.Begin(); it != moduleInfos.End(); ++it)
    {
        SharedPtr<WorkItem> item = queue->GetFreeItem();
        item->sendEvent_ = true;
        item->priority_ = ANL_WORKITEM_PRIORITY;
        item->workFunction_ = MapGenThread;
        item->aux_ = &(*it);
        queue->AddWorkItem(item);
    }

    queue->Resume();

    return true;
}
#endif

bool AnlWorldModel::GenerateModules(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay)
{
    int& imodule = genStatus.mapcount_[MAP_FUNC3];
    int& iprogress = genStatus.mapcount_[MAP_FUNC4];
    int& time = genStatus.time_;
    int starttime = timer ? timer->GetUSec(false) / 1000 : 0;

//    URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : ANL%s %s ... imodule=%d iprogress=%d  ...", ANLVersionNameStr[anlVersion_], GetName().CString(), imodule, iprogress);

    // initialization
    if (imodule == 0)
    {
        if (!renderableModules_.Size())
        {
            URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : %s ... No Renderable Modules !", GetName().CString());
            return true;
        }

        terrainBound_ = World2DInfo::currentAtlas_ ? World2DInfo::currentAtlas_->GetNumBiomeTerrains() - 1 : 1;

        Vector<void*>& maps = genStatus.features_;
        if (renderableModules_.Size() > maps.Size())
        {
            URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : %s ... Renderable Modules > Output Maps !", GetName().CString());
            return true;
        }

        if (anlVersion_ == ANLV1)
        {
            anl::CImplicitCacheArray::startStaticCache(genStatus.width_*genStatus.height_);
        }
        else if (anlVersion_ == ANLVM)
        {
            anl::CKernel& kernel = *((anl::CKernel*)kernel_);

#ifdef ACTIVE_WORLD2D_THREADING
//			if (!AreThreadFinished())
//            {
//                URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... wait for thread allocation ... ",
//                            genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none", genStatus.map_);
//                return false;
//            }
            if (!CreateMapGenWorkInfos(GetContext(), genStatus, mapGenWorkInfos_, kernel, renderableModules_))
            {
                URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... wait for thread allocation ... ",
                                 genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none", genStatus.map_);
                return false;
            }

            SubscribeToEvent(GameContext::Get().gameWorkQueue_, E_WORKITEMCOMPLETED, URHO3D_HANDLER(AnlWorldModel, HandleWorkItemComplete));

            URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... numInstructions=%u numRenderableModules=%u ... ",
                            genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none",
                            genStatus.map_, kernel.getInstructions().Size(), renderableModules_.Size());

//            printf("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... numInstructions=%u numRenderableModules=%u ... ",
//                            genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none",
//                            genStatus.map_, kernel.getInstructions().size(), renderableModules_.Size());
#else
            anl::CNoiseExecutor::ResizeCache(kernel, MAPGENSLOT, genStatus.width_, genStatus.height_, 1);

            if (!evaluator_) evaluator_ = new anl::CNoiseExecutor(kernel, MAPGENSLOT, 0);
            anl::CNoiseExecutor& evaluator = *(anl::CNoiseExecutor*)evaluator_;
            evaluator.ResetCacheAccess(true);
            evaluator.SetCacheAccessor(0, genStatus.width_*genStatus.height_-1);

            // Initialize Cache
            evaluator.StartEvaluation();

//            URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... numInstructions=%u ...",
//                            GetName().CString(), evaluator.GetNumInstructions(), time);
#endif
        }
        // New Version
        else
        {
#ifdef ACTIVE_WORLD2D_THREADING

#else
            anl::InstructionList& kernel = *((anl::InstructionList*)kernel_);
            anl::Evaluator::ResizeCache(kernel, genStatus.width_, genStatus.height_, 1);

            if (!evaluator_) evaluator_ = new anl::Evaluator(kernel, 0);
            anl::Evaluator& evaluator = *(anl::Evaluator*)evaluator_;
            evaluator.ResetCacheAccess(true);
            evaluator.SetCacheAccessor(0, genStatus.width_*genStatus.height_-1);
            evaluator.StartEvaluation();

            URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... numInstructions=%u ...",
                            GetName().CString(), evaluator.GetNumInstructions(), time);
#endif
        }

        time = 0;
        iprogress = 0;
        imodule++;
    }
    if (imodule > 0)
    {
        if (anlVersion_ == ANLV1)
        {
            for (;;)
            {
                if (imodule > renderableModules_.Size())
                {
                    imodule = 0;
                    time += (timer ? timer->GetUSec(false) / 1000 : 0) - starttime;
                    URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... renderableModules=%u ... in %d msec ... OK !",
                                    GetName().CString(), renderableModules_.Size(), time);
                    return true;
                }

                Vector<void*>& maps = genStatus.features_;
//                const AnlMappingRange& range = genStatus.mappingRange_;
                int width = genStatus.width_;
                int height = genStatus.height_;
                const AnlMappingRange& range = genStatus.mappingRange_;
                const float tilingx2world = (range.mapx1 - range.mapx0) / (float)width;
                const float tilingy2world = (range.mapy1 - range.mapy0) / (float)height;
                FeatureType* map = (FeatureType*)maps[imodule-1];

                float nx, ny;

                anl::CImplicitModuleBase& module = *(anl::CImplicitModuleBase*)modules_[renderableModules_[imodule-1]];
                module.setSeed(genStatus.wseed_);

                FeatureType featuretype = imodule-1 < TERRAINMAP ? (FeatureType)MapFeatureType::OuterFloor : 1;

                for (unsigned y=iprogress; y<height; ++y)
                {
//                    ny = range.mapy0 + ((float)y / (float)height) * (range.mapy1 - range.mapy0);
                    ny = range.mapy0 + (float)y * tilingy2world;

                    for (unsigned x=0; x<width; ++x)
                    {
//                        nx = range.mapx0 + ((float)x / (float)width) * (range.mapx1 - range.mapx0);
                        nx = range.mapx0 + (float)x * tilingx2world;
                        map[y*width+x] = featuretype * RoundToInt(module.get(nx, ny));
//                        map[y*width+x] = module.get(nx, ny) > 0.9f ? (FeatureType)MapFeatureType::OuterFloor : (FeatureType)MapFeatureType::NoMapFeature;
                    }

                    if (TimeOver(timer))
                    {
                        iprogress = y+1;
                        time += (timer ? timer->GetUSec(false) / 1000 : 0) - starttime;
                        URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... renderableModule=%u/%u ... timer=%d/%d msec ... breakatY=%u !",
                                        GetName().CString(), imodule, renderableModules_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000, y);

                        return false;
                    }
                }

                URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s module=%s ... renderableModule=%u/%u ... timer=%d/%d msec ... OK !",
                                GetName().CString(), renderableModulesNames_[imodule-1].CString(), imodule, renderableModules_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000);

                iprogress = 0;
                imodule++;
            }
        }
        else if (anlVersion_ == ANLVM)
        {
#ifdef ACTIVE_WORLD2D_THREADING
            if (!mapGenWorkInfos_.Size())
            {
                URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... No More Threads ... reset the generation !",
                                 genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none", genStatus.map_);
                imodule = iprogress = 0;
                return false;
            }

            // threads ended
            if (!HasSubscribedToEvent(GameContext::Get().gameWorkQueue_, E_WORKITEMCOMPLETED))
            {
                iprogress = 0;
                imodule = 0;
                for (List<MapGenWorkInfo>::Iterator it = mapGenWorkInfos_.Begin(); it != mapGenWorkInfos_.End(); ++it)
                    time = Max(time, it->time_);
                time -= starttime;

                URHO3D_LOGERRORF("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... Threads Finished ... in %d msec ... OK !",
                                 genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none", genStatus.map_, time);

//                printf("AnlWorldModel() - GenerateModules : mpoint=%s map=%s(%u) ... Threads Finished ... in %d msec ... OK !",
//                                genStatus.mappoint_.ToString().CString(), genStatus.map_ ? genStatus.map_->GetMapPoint().ToString().CString() : "none", genStatus.map_, time);
                return true;
            }
#else
            for (;;)
            {
                if (imodule > renderableModules_.Size())
                {
                    imodule = 0;
                    time += (timer ? timer->GetUSec(false) / 1000 : 0) - starttime;
                    URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... renderableModules=%u ... in %d msec ... OK !",
                                    GetName().CString(), renderableModules_.Size(), time);
                    return true;
                }

                unsigned cindex = renderableModules_[imodule-1];
                FeatureType* map = (FeatureType*)genStatus.features_[imodule-1];
                if (!map)
                {
                    iprogress = 0;
                    imodule++;
                    continue;
                }

                int width = genStatus.width_;
                int height = genStatus.height_;
                const AnlMappingRange& range = genStatus.mappingRange_;
                const float tilingx2world = (range.mapx1 - range.mapx0) / (float)width;
                const float tilingy2world = (range.mapy1 - range.mapy0) / (float)height;

                anl::CNoiseExecutor& evaluator = *(anl::CNoiseExecutor*)evaluator_;
                anl::CCoordinate coord;

                coord.dimension_ = 2;

                for (unsigned y=iprogress; y<height; ++y)
                {
                    if (TimeOver(timer))
                    {
                        iprogress = y;
                        time += (timer ? timer->GetUSec(false) / 1000 : 0) - starttime;

//                        URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... renderableModule=%u/%u ... timer=%d/%d msec ... breakatY=%u !",
//                            GetName().CString(), imodule, renderableModules_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000, y);

                        return false;
                    }

                    coord.y_ = range.mapy0 + (float)y * tilingy2world;

                    if (imodule < TERRAINMAP)
                    {
                        for (unsigned x=0; x<width; ++x)
                        {
                            coord.x_ = range.mapx0 + (float)x * tilingx2world;
                            float value = evaluator.EvaluateAt(cindex, coord).outfloat_;
                            map[y*width+x] = (FeatureType)MapFeatureType::OuterFloor * RoundToInt(value);
                            evaluator.NextCacheAccessFastIndex();
                        }
                    }
                    else if (imodule == TERRAINMAP)
                    {
                        for (unsigned x=0; x<width; ++x)
                        {
                            coord.x_ = range.mapx0 + (float)x * tilingx2world;
                            float value = evaluator.EvaluateAt(cindex, coord).outfloat_;
                            map[y*width+x] = (FeatureType)Clamp(RoundToInt(value * (float)(terrainBound_)), 0, terrainBound_);
                            evaluator.NextCacheAccessFastIndex();
                        }
                    }
                    else if (imodule != TESTMAP) // BIOMEMAP
                    {
                        float value;
                        for (unsigned x=0; x<width; ++x)
                        {
                            coord.x_ = range.mapx0 + (float)x * tilingx2world;
                            value = evaluator.EvaluateAt(cindex, coord).outfloat_;
                            map[y*width+x] = (FeatureType)RoundToInt(value);
                            evaluator.NextCacheAccessFastIndex();
                        }
                    }
                    else
                    {
                        for (unsigned x=0; x<width; ++x)
                        {
                            coord.x_ = range.mapx0 + (float)x * tilingx2world;
                            map[y*width+x] = evaluator.EvaluateAt(cindex, coord).outfloat_;
                            evaluator.NextCacheAccessFastIndex();
                        }
                    }
                }

                // Reset cache
                evaluator.StartEvaluation();
                evaluator.SetCacheAvailable(cindex);

                URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s module=%s ... renderableModule=%u/%u ... timer=%d/%d msec ... OK !",
                                GetName().CString(), renderableModulesNames_[imodule-1].CString(), imodule, renderableModules_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000);

                iprogress = 0;
                imodule++;
            }
#endif
        }
        // New Version
        else
        {
#ifdef ACTIVE_WORLD2D_THREADING

#else
            for (;;)
            {
                if (imodule > renderableModules_.Size())
                {
                    imodule = 0;
                    time += (timer ? timer->GetUSec(false) / 1000 : 0) - starttime;
                    URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... renderableModules=%u ... in %d msec ... OK !",
                                    GetName().CString(), renderableModules_.Size(), time);
                    return true;
                }

                int width = genStatus.width_;
                int height = genStatus.height_;
                const AnlMappingRange& range = genStatus.mappingRange_;
                const float tilingx2world = (range.mapx1 - range.mapx0) / (float)width;
                const float tilingy2world = (range.mapy1 - range.mapy0) / (float)height;
                FeatureType* map = (FeatureType*)genStatus.features_[imodule-1];

                anl::Evaluator& evaluator = *(anl::Evaluator*)evaluator_;
                anl::CCoordinate coord;
                unsigned cindex = renderableModules_[imodule-1];
                coord.dimension_ = 2;

                for (unsigned y=iprogress; y<height; ++y)
                {
                    if (TimeOver(timer))
                    {
                        iprogress = y;
                        time += (timer ? timer->GetUSec(false) / 1000 : 0) - starttime;

                        URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s ... renderableModule=%u/%u ... timer=%d/%d msec ... breakatY=%u !",
                                        GetName().CString(), imodule, renderableModules_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000, y);

                        return false;
                    }

//                    coord.y_ = range.mapy0 + ((float)y / (float)height) * (range.mapy1 - range.mapy0);
                    coord.y_ = range.mapy0 + (float)y * tilingy2world;

                    for (unsigned x=0; x<width; ++x)
                    {
//                        coord.x_ = range.mapx0 + ((float)x / (float)width) * (range.mapx1 - range.mapx0);
                        coord.x_ = range.mapx0 + (float)x * tilingx2world;

                        map[y*width+x] = evaluator.EvaluateAt(coord) > 0.5f ? (FeatureType)MapFeatureType::OuterFloor : (FeatureType)MapFeatureType::NoMapFeature;
                        evaluator.NextCacheAccessFastIndex();
                    }
                }

                evaluator.StartEvaluation();
                evaluator.SetCacheAvailable(cindex);

                URHO3D_LOGINFOF("AnlWorldModel() - GenerateModules : %s module=%s ... renderableModule=%u/%u ... timer=%d/%d msec ... OK !",
                                GetName().CString(), renderableModulesNames_[imodule-1].CString(), imodule, renderableModules_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delay/1000);

                iprogress = 0;
                imodule++;
            }
#endif
        }
    }

    return false;
}

void AnlWorldModel::HandleWorkItemComplete(StringHash eventType, VariantMap& eventData)
{
    WorkItem* item = static_cast<WorkItem*>(eventData[WorkItemCompleted::P_ITEM].GetPtr());

    if (!static_cast<Object*>(item->aux_)->IsInstanceOf<MapGenWorkInfo>())
        return;

    MapGenWorkInfo* info = static_cast<MapGenWorkInfo*>(item->aux_);

//    URHO3D_LOGINFOF("AnlWorldModel() - HandleWorkItemComplete ... %s ... thread=%d ysize=%d finished in %d msec !", info->genStatus_->mappoint_.ToString().CString(), info->ithread_, info->ysize_, info->time_);

    // Reset workinfo
    info->finished_ = true;

    // the WorkItems are not all finished => continue
    for (List<MapGenWorkInfo>::ConstIterator it = mapGenWorkInfos_.Begin(); it != mapGenWorkInfos_.End(); ++it)
    {
        if (!it->finished_)
            return;
    }

    //  All WorkItems are finished => stop
//    URHO3D_LOGINFOF("AnlWorldModel() - HandleWorkItemComplete ... All WorkItems Finished !");

    UnsubscribeFromEvent(GameContext::Get().gameWorkQueue_, E_WORKITEMCOMPLETED);
}

void AnlWorldModel::Clear()
{
    for (Vector<void*>::Iterator module=modules_.Begin(); module != modules_.End(); ++module)
        delete (anl::CImplicitModuleBase*)(*module);

    modules_.Clear();
    modulesources_.Clear();
    renderableModules_.Clear();

    if (kernel_)
    {
        if (anlVersion_ == ANLVM)
            delete (anl::CKernel*)kernel_;
        else
            delete (anl::InstructionList*)kernel_;
        kernel_ = 0;
    }
    if (evaluator_)
    {
        if (anlVersion_ == ANLVM)
            delete (anl::CNoiseExecutor*)evaluator_;
        else
            delete (anl::Evaluator*)evaluator_;
        evaluator_ = 0;
    }
//    URHO3D_LOGINFOF("AnlWorldModel() - Clear : ... %s ... OK !", GetName().CString());
}

