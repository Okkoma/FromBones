#pragma once

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/XMLFile.h>

#include "GameContext.h"

namespace Urho3D
{
class XMLFile;
class Texture2D;
}

namespace anl
{
class CKernel;
}

using namespace Urho3D;

const float AnlWorldModelGranularity = 0.5f;

enum ANLVersionName
{
    ANLV1 = 0,
    ANLVM,
    ANLV2,
};

const unsigned ANL_WORKITEM_PRIORITY = 1000U;

/// MapGen Work Info
class MapGenWorkInfo : public Object
{
    URHO3D_OBJECT(MapGenWorkInfo, Object);

public:
    MapGenWorkInfo() :
        Object(GameContext::Get().context_),
        finished_(true),
        time_(0),
        ithread_(0),
        ystart_(0),
        ysize_(0),
        kernel_(0),
        modules_(0),
        genStatus_(0) { }

    MapGenWorkInfo(const MapGenWorkInfo& e) :
        Object(GameContext::Get().context_),
        finished_(e.finished_),
        time_(e.time_),
        ithread_(e.ithread_),
        ystart_(e.ystart_),
        ysize_(e.ysize_),
        kernel_(e.kernel_),
        modules_(e.modules_),
        genStatus_(e.genStatus_) { }

    ~MapGenWorkInfo() { }

    bool finished_;
    int time_;
    int ithread_;
    unsigned ystart_, ysize_;

    anl::CKernel* kernel_;
    Vector<unsigned int>* modules_;
    MapGeneratorStatus* genStatus_;
};

/// WorldMap Work Info
struct SnapShotWorkInfo
{
    Texture2D* snapShotTexture_;
    unsigned snapShotColor_;
    String snapShotName_;

    int state_;
    int anlversion_;
    unsigned imodule_;
    AnlMappingRange mapping_;

    int pixelSize_;
    anl::CArray2Dd storage_;
};

class FROMBONES_API AnlWorldModel : public Resource
{
    URHO3D_OBJECT(AnlWorldModel, Resource);

public:
    /// Construct.
    AnlWorldModel(Context* context);
    /// Destruct.
    virtual ~AnlWorldModel();
    /// Register object factory.
    static void RegisterObject(Context* context);
    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();

    void SetSeedAllModules(unsigned seed);
    void SetRadius(float radius);
    void SetScale(const Vector2& scale);

    bool GenerateModules(MapGeneratorStatus& genStatus, HiresTimer* timer, const long long& delay);
    bool HasRenderableModule(int maptype)
    {
        return (maptype < renderableModules_.Size() && renderableModules_[maptype] > 0);
    }

    void StopUnfinishedWorks();

    void GenerateSnapShots(int pixelSize, unsigned color, const Vector2& center=Vector2::ZERO, float scale=1.f, unsigned numModules=0, String* renderableModuleNames=0, Texture2D* texture=0);
    void GenerateWorldMapShots(const String& name, const Vector<String> renderableModuleNames);
    bool UpdateGeneratingSnapShots();

    bool AreThreadFinished() const;

    float GetSeed() const
    {
        return seed_;
    }
    float GetRadius() const
    {
        return radiusIndex_ != -1 ? radius_ : 0.f;
    }
    const Vector2& GetScale() const
    {
        return scaleIndex_ != -1 && scale_.x_ != 0.f ? scale_ : Vector2::ONE;
    }
    const Vector2& GetOffset() const
    {
        return offsetIndex_ != -1 ? offset_ : Vector2::ZERO;
    }
    Vector2 GetSize() const
    {
        return (radiusIndex_ != -1 ? radius_ : 1.f) * 2.f * Vector2::ONE / GetScale();
    }

    int GetAnlVersion() const
    {
        return anlVersion_;
    }

private:
    void Clear();

    bool BeginLoadFromXMLFile(Deserializer& source);
    bool EndLoadFromXMLFile();

    void HandleWorkItemComplete(StringHash eventType, VariantMap& eventData);

    /// XML file used while loading.
    SharedPtr<XMLFile> loadXMLFile_;

    /// Parameters
    unsigned seed_;
    float radius_;
    Vector2 scale_;
    Vector2 offset_;
    int radiusIndex_;
    int scaleIndex_;
    int offsetIndex_;

    /// ANL Kernel
    int anlVersion_;

    void* kernel_;              // ANLVM kernel || ANLV2 InstructionList
    void* evaluator_;           // ANLVM noiseexecutor || ANLV2 Evaluator

    Vector<void* > modules_;    // ANLV1
    HashMap<String, unsigned int> modulesources_; // ANLVM kernel
    Vector<unsigned int> renderableModules_;
    Vector<String> renderableModulesNames_;
    Vector<unsigned int> screenshotModules_;
    Vector<String> screenshotModuleNames_;

    List<MapGenWorkInfo> mapGenWorkInfos_;

    Vector<SnapShotWorkInfo> snapShotWorkInfos_;
};
