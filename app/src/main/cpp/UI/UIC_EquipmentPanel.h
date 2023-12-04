#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
class UIElement;
class Font;
}

using namespace Urho3D;


class UIC_EquipmentPanel : public UISlotPanel
{
    URHO3D_OBJECT(UIC_EquipmentPanel, UISlotPanel);

public :

    UIC_EquipmentPanel(Context* context);
    virtual ~UIC_EquipmentPanel();

    static void RegisterObject(Context* context);

    virtual void Start(Object* user, Object* feeder=0);
    virtual void Reset();

    virtual void Update();
    virtual void UpdateSlot(unsigned index, bool updateButtons=false, bool updateSubscribers=false, bool updateNet=false);
    void UpdateEquipment(unsigned index);

    virtual void OnResize();

protected :
    virtual void SetSlotZone();

    bool UpdateSlotPositions(bool force=false);
    void HideUISlots();

    virtual void OnSlotUpdate(StringHash eventType, VariantMap& eventData);
    virtual void OnDragSlotIn(int slotId, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel);
    virtual void OnDragSlotFinish(int slotId);
    virtual void OnEquipmentUpdate(StringHash eventType, VariantMap& eventData);

    UIElement* wearablesZone_;
    UIElement* nowearablesZone_;

    int lastchange_;
};
