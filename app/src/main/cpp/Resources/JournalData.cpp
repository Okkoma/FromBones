#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include "GameOptions.h"
#include "GameHelpers.h"

#include <PugiXml/pugixml.hpp>

#include "JournalData.h"

//const char* MessageCategoryStr[] =
//{
//    "",
//    0
//};


JournalData::JournalData(Context* context) :
    Resource(context)
{
    URHO3D_LOGINFO("JournalData()");
}

JournalData::~JournalData()
{
    URHO3D_LOGINFO("~JournalData()");
}

void JournalData::RegisterObject(Context* context)
{
    context->RegisterFactory<JournalData>();
}

bool JournalData::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    String extension = GetExtension(source.GetName());
    if (extension == ".xml")
        return BeginLoadFromXMLFile(source);

    URHO3D_LOGERROR("JournalData() - BeginLoad : Unsupported file type !");
    return false;
}

bool JournalData::EndLoad()
{
    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    return false;
}

bool JournalData::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XMLFile(context_);
    if (!loadXMLFile_->Load(source))
    {
        URHO3D_LOGERROR("JournalData() - BeginLoad : could not load file !");
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    if (!loadXMLFile_->GetRoot("journaldata"))
    {
        URHO3D_LOGERROR("JournalData() - BeginLoadFromXMLFile : Invalid Dialogue File !");
        loadXMLFile_.Reset();
        return false;
    }

    return true;
}

extern const char* GoalConditionStr[];
extern const char* GoalActionStr[];

bool JournalData::EndLoadFromXMLFile()
{
    XMLElement rootElem = loadXMLFile_->GetRoot("journaldata");

    for (XMLElement missionElem = rootElem.GetChild("mission"); missionElem; missionElem = missionElem.GetNext("mission"))
    {
        String name = missionElem.GetAttribute("name");

        MissionData& missiondata = missionData_[StringHash(name)];
        missiondata.name_        = name;
        missiondata.title_       = missionElem.GetAttribute("title");
        missiondata.sequenced_   = missionElem.GetBool("sequenced");

        for (XMLElement objectiveElem = missionElem.GetChild("objective"); objectiveElem; objectiveElem = objectiveElem.GetNext("objective"))
        {
            missiondata.objectives_.Resize(missiondata.objectives_.Size()+1);
            ObjectiveData& objectivedata = missiondata.objectives_.Back();
            objectivedata.type_          = StringHash(objectiveElem.GetAttribute("type")).Value();
            objectivedata.isMajor_       = objectiveElem.GetBool("major");
            objectivedata.delay_         = objectiveElem.GetInt("delay");

            XMLElement objectElem     = objectiveElem.GetChild("object");
            objectivedata.objectType_ = StringHash(objectElem.GetAttribute("type")).Value();
            objectivedata.objectQty_  = objectElem.GetInt("qty");

            for (XMLElement conditionElem = objectiveElem.GetChild("condition"); conditionElem; conditionElem = conditionElem.GetNext("condition"))
            {
                objectivedata.conditionDatas_.Resize(objectivedata.conditionDatas_.Size()+1);
                ObjectiveCommandData& conditiondata = objectivedata.conditionDatas_.Back();
                conditiondata.cmd_ = GetEnumValue(conditionElem.GetAttribute("type"), GoalConditionStr);
                conditiondata.actor1Type_ = StringHash(conditionElem.GetAttribute("actor1")).Value();
                conditiondata.actor2Type_ = StringHash(conditionElem.GetAttribute("actor2")).Value();
                conditiondata.argType_ = StringHash(conditionElem.GetAttribute("arg")).Value();
            }
            for (XMLElement actionElem = objectiveElem.GetChild("action"); actionElem; actionElem = actionElem.GetNext("action"))
            {
                objectivedata.actionDatas_.Resize(objectivedata.actionDatas_.Size()+1);
                ObjectiveCommandData& actiondata = objectivedata.actionDatas_.Back();
                actiondata.cmd_ = GetEnumValue(actionElem.GetAttribute("type"), GoalActionStr);
                actiondata.actor1Type_ = StringHash(actionElem.GetAttribute("actor1")).Value();
                actiondata.actor2Type_ = StringHash(actionElem.GetAttribute("actor2")).Value();
                actiondata.argType_ = StringHash(actionElem.GetAttribute("arg")).Value();
            }
            for (XMLElement textElem = objectiveElem.GetChild("text"); textElem; textElem = textElem.GetNext("text"))
                objectivedata.texts_[StringHash(textElem.GetAttribute("name"))] = textElem.GetValue();
        }
        for (XMLElement rwdElem = missionElem.GetChild("reward"); rwdElem; rwdElem = rwdElem.GetNext("reward"))
        {
            missiondata.rewards_.Resize(missiondata.rewards_.Size()+1);
            Reward& reward         = missiondata.rewards_.Back();
            reward.rewardCategory_ = StringHash(rwdElem.GetAttribute("category")).Value();
            reward.type_           = StringHash(rwdElem.GetAttribute("type")).Value();
            reward.fromtype_       = StringHash(rwdElem.GetAttribute("fromtype")).Value();
            reward.partindex_      = rwdElem.GetInt("partindex");
            reward.quantity_       = rwdElem.GetInt("qty");
        }
    }

    loadXMLFile_.Reset();
    return true;
}

MissionData* JournalData::GetMissionData(const StringHash& hashname) const
{
    HashMap<StringHash, MissionData>::ConstIterator it = missionData_.Find(hashname);
    return it != missionData_.End() ? (MissionData*)&it->second_ : 0;
}

void JournalData::Dump() const
{
    URHO3D_LOGINFO("JournalData() - Dump() ...");

    unsigned i=0;
    for (HashMap<StringHash, MissionData>::ConstIterator mt=missionData_.Begin(); mt!=missionData_.End(); ++mt,++i)
    {
        const MissionData& md = mt->second_;

        URHO3D_LOGINFOF("missiondata[%u] name=%s title=%s sequenced=%s :", i, md.name_.CString(), md.title_.CString(), md.sequenced_?"true":"false");

        for (HashMap<StringHash, String>::ConstIterator tt = md.texts_.Begin(); tt != md.texts_.End(); ++tt)
        {
            URHO3D_LOGINFOF("  text[%u]=%s", tt->first_.Value(), tt->second_.CString());
        }
        for (Vector<ObjectiveData>::ConstIterator ot = md.objectives_.Begin(); ot != md.objectives_.End(); ++ot)
        {
            const ObjectiveData& od = *ot;
            URHO3D_LOGINFOF("  objective type=%u object=%u qty=%d :", od.type_, od.objectType_, od.objectQty_);
            for (HashMap<StringHash, String>::ConstIterator ttt = od.texts_.Begin(); ttt != od.texts_.End(); ++ttt)
            {
                URHO3D_LOGINFOF("     text[%u]=%s", ttt->first_.Value(), ttt->second_.CString());
            }
        }
    }

    URHO3D_LOGINFO("JournalData() - Dump() ... OK !");
}
