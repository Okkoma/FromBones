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

    void SetPopup(bool enable) { popup_ = enable; }

    virtual void Start(Object* user, Object* feeder);
    virtual void GainFocus();
    virtual void LoseFocus();    

    bool IsPopup() const { return popup_; }
    virtual bool CanFocus() const;

    void Clear();

    virtual void OnSetVisible();

protected :
    void SetActiveAbility(const StringHash& hash);
    void DesactiveAbility();
    void UpdateSlotSelector();

    void HandleClic(StringHash eventType, VariantMap& eventData);

    void OnAbilityUpdated(StringHash eventType, VariantMap& eventData);
    void OnNodesChange(StringHash eventType, VariantMap& eventData);
    void OnKey(StringHash eventType, VariantMap& eventData);

    bool popup_;
    int slotselector_;

    WeakPtr<Actor> holder_;
    WeakPtr<Node> holderNode_;

    UIElement *slotZone_, *selectHalo_, *activeAbilityButton_;
    StringHash lastAbilityHash_;
};
