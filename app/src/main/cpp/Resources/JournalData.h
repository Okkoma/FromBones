#pragma once

#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{
class XMLFile;
}

using namespace Urho3D;

#include "GameGoal.h"


struct JournalData : public Resource
{
    URHO3D_OBJECT(JournalData, Resource);

public:
    JournalData(Context* context);
    virtual ~JournalData();

    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();

    /// Return the first message for the speaker
    MissionData* GetMissionData(const StringHash& hashname) const;

    void Dump() const;

private :
    /// Begin load from XML file.
    bool BeginLoadFromXMLFile(Deserializer& source);
    /// End load from XML file.
    bool EndLoadFromXMLFile();

    HashMap<StringHash, MissionData> missionData_;

    SharedPtr<XMLFile> loadXMLFile_;
};


