#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
class UIElement;
}

class Actor;
class Ability;

using namespace Urho3D;


class UIC_AbilityPanel : public UIPanel
{
    URHO3D_OBJECT(UIC_AbilityPanel, UIPanel);

public :
    UIC_AbilityPanel(Context* context);
    virtual ~UIC_AbilityPanel() { ; }

    static void RegisterObject(Context* context);

    virtual void Start(Object* user, Object* feeder);

    void Clear();
    virtual void Update();

protected :
    void SetActiveAbility(const StringHash& hash);
    void DesactiveAbility();

    void HandleClic(StringHash eventType, VariantMap& eventData);

    void OnAbilityUpdated(StringHash eventType, VariantMap& eventData);
    void OnNodesChange(StringHash eventType, VariantMap& eventData);

    WeakPtr<Actor> holder_;
    WeakPtr<Node> holderNode_;

    UIElement* slotZone_;
    UIElement* activeAbilityButton_;
    StringHash lastAbilityHash_;
};
