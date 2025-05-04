#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include "GameOptionsTest.h"

#include "GameData.h"


/// GameData

const char* DIALOGUEPATH = "Texts/Dialogues_";
const char* JOURNALPATH = "Texts/Journal_";

GameData* GameData::data_=0;

GameData::GameData(Context* context)  :
    Object(context)
{
    data_ = this;
    SubscribeToEvent(E_CHANGELANGUAGE, URHO3D_HANDLER(GameData, HandleChangeLanguage));
}

GameData::~GameData()
{ }

void GameData::SetLanguage(const String& lang)
{
    if (lang_ != lang)
    {
        lang_ = lang;

        dialogueData_ = GetSubsystem<ResourceCache>()->GetResource<DialogueData>(String(DIALOGUEPATH)+lang_+String(".xml"));
        journalData_  = GetSubsystem<ResourceCache>()->GetResource<JournalData>(String(JOURNALPATH)+lang_+String(".xml"));

        URHO3D_LOGINFOF("GameData() - SetLanguage : lang = %s", lang_.CString());
    }
}

void GameData::HandleChangeLanguage(StringHash eventType, VariantMap& eventData)
{
    Localization* l10n = GetSubsystem<Localization>();
    if (l10n)
        GameData::Get()->SetLanguage(l10n->GetLanguage());
}