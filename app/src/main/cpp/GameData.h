#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>

using namespace Urho3D;

#include "DefsCore.h"
#include "DialogueData.h"
#include "JournalData.h"

class FROMBONES_API GameData : public Object
{
    URHO3D_OBJECT(GameData, Object);

public:
    GameData(Context* context);
    ~GameData();

    static GameData* Get()
    {
        return data_;
    }
    static DialogueData* GetDialogueData()
    {
        return data_->dialogueData_;
    }
    static JournalData* GetJournalData()
    {
        return data_->journalData_;
    }

    /// Setters
    void SetLanguage(const String& lang);

    /// Getters
    const String& GetLanguage() const
    {
        return lang_;
    }

protected:
    void HandleChangeLanguage(StringHash eventType, VariantMap& eventData);

private:
    String lang_;
    SharedPtr<DialogueData> dialogueData_;
    SharedPtr<JournalData> journalData_;

    static GameData* data_;
};
