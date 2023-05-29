#pragma once

namespace Urho3D
{
class UIElement;
class Text;
class Font;
}

using namespace Urho3D;

class GOC_Inventory;
class DelayInformer;

#include "UISlotPanel.h"
#include "CraftRecipes.h"

class UIC_CraftPanel : public UISlotPanel
{
    URHO3D_OBJECT(UIC_CraftPanel, UISlotPanel);

public :

    UIC_CraftPanel(Context* context);
    virtual ~UIC_CraftPanel();

    static void RegisterObject(Context* context);

    virtual void Start(Object* user, Object* feeder=0);

    virtual void UpdateSlot(unsigned index, bool updateButtons=false, bool updateSubscribers=false);
    void UpdateCraftedItem(bool clear=false);
    void SetTool(unsigned islot, const StringHash& type);

protected :

    virtual void SetSlotZone();
    virtual void UpdateSlots(bool updateButtons=false, bool updateSubscribers=false);
    virtual void OnRelease();

    void CheckAvailableTools();

    void HandleMake(StringHash eventType, VariantMap& eventData);
    void HandleCraftItemMade(StringHash eventType, VariantMap& eventData);

    GOC_Inventory* craft_;
    Slot craftedItem_;
    DelayInformer* craftTimer_;
    Vector<StringHash> availableTools_;
};
