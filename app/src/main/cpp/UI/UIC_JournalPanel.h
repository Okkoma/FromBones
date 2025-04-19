#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
    class Sprite2D;
    class UIElement;
}

using namespace Urho3D;

class Mission;  

class UIC_JournalPanel : public UIPanel
{
    URHO3D_OBJECT(UIC_JournalPanel, UIPanel);

public :

    UIC_JournalPanel(Context* context);
    virtual ~UIC_JournalPanel() { }

    static void RegisterObject(Context* context);

    virtual void Start(Object* user, Object* feeder=0);

    virtual void Update();

protected :

    void UpdateSelectedMission();
    void UpdateSelector();
    void MarkAsNew(UIElement* element);

    void OnNamedMissionAdded(StringHash eventType, VariantMap& eventData);
    void OnNamedMissionUpdated(StringHash eventType, VariantMap& eventData);
    void OnNamedObjectiveUpdated(StringHash eventType, VariantMap& eventData);
    void OnCloseButton(StringHash eventType, VariantMap& eventData);

    Mission* selectedMission_;
    MissionManager* manager_;
    UIElement* questSelectorZone_;
    UIElement* currentQuestZone_;

    int slotSize_;

    PODVector<Mission*> delayedMissionsToUpdate_;
};
