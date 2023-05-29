#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
class Sprite2D;
class UIElement;
}

class Mission;

using namespace Urho3D;


class UIC_MissionPanel : public UIPanel
{
    URHO3D_OBJECT(UIC_MissionPanel, UIPanel);

public :

    UIC_MissionPanel(Context* context);
    virtual ~UIC_MissionPanel() { ; }

    static void RegisterObject(Context* context);

    void SetMission(Mission* mission)
    {
        mission_ = mission;
    }

    virtual void Start(Object* user, Object* feeder=0);

    virtual void Update();

protected :
    void UpdateObjective(unsigned index, bool dirty = false);
    void SetImage(BorderImage* image, Sprite2D* sprite, StringHash got=StringHash::ZERO);
    void ShowRewards();
    void Mark(UIElement* element, int state);
    void Clear();

    void OnMissionChanged(StringHash eventType, VariantMap& eventData);
    void OnMissionUpdated(StringHash eventType, VariantMap& eventData);
    void OnObjectiveUpdated(StringHash eventType, VariantMap& eventData);
    void OnCloseButton(StringHash eventType, VariantMap& eventData);
    void OnStateButton(StringHash eventType, VariantMap& eventData);

    bool missionDirty_;
    Mission* mission_;
    MissionManager* manager_;
    UIElement* objectiveZone_;
    UIElement* rewardZone_;
    int slotSize_;

    Vector<Slot> tempSlotReward;
};
