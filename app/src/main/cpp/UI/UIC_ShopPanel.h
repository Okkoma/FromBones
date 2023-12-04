#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
class UIElement;
class Text;
class Font;
}

using namespace Urho3D;


class GOC_Inventory;

enum
{
    SHOPBASKET_TOSELL = -1,
    SHOPBASKET_EMPTY = 0,
    SHOPBASKET_TOBUY = 1,
};

class UIC_ShopPanel : public UISlotPanel
{
    URHO3D_OBJECT(UIC_ShopPanel, UISlotPanel);

public :

    UIC_ShopPanel(Context* context);
    virtual ~UIC_ShopPanel();

    void SetShopInventory(GOC_Inventory* inventory);

    static void RegisterObject(Context* context);

    virtual void Start(Object* user, Object* feeder=0);
    void ResetShop();

    void MarkBasketSlot(unsigned idslot, int mark)
    {
        basketSlotMarks_[idslot] = mark;
    }

    GOC_Inventory* GetShop() const
    {
        return shop_;
    }
    GOC_Inventory* GetBasket() const
    {
        return basket_;
    }
    int GetBasketSlotMark(unsigned idslot) const
    {
        return basketSlotMarks_[idslot];
    }

    virtual void UpdateSlot(unsigned index, bool updateButtons=false, bool updateSubscribers=false, bool updateNet=false);
    void UpdatePrice();
    void UpdateShopSlots(bool updateButtons=false, bool updateSubscribers=false);
    void UpdateBasketSlots(bool updateButtons=false, bool updateSubscribers=false);

    virtual bool AcceptSlotType(int slotId, const StringHash& type, UISlotPanel* otherPanel);

protected :

    virtual void SetSlotZone();
    virtual void SelectInventoryFrom(const String& name);
    void SwitchInventoryTo(GOC_Inventory* inventory);
    void CancelBasket();
    bool CheckValidateBasket();
    void ValidateBasket();

    virtual void UpdateSlots(bool updateButtons=false, bool updateSubscribers=false);

    virtual int GetFocusSlotId(UIElement* focus, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel);
    virtual void OnDragSlotIn(int slotId, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel);
    virtual void OnSlotRemain(Slot& slot, int slotid);
    virtual void OnDragSlotFinish(int slotId);

    void OnReleasePriceButton(StringHash eventType, VariantMap& eventData);
    void OnShopClosing(StringHash eventType, VariantMap& eventData);

    UIElement* shopZone_;
    UIElement* basketZone_;
    Text* priceText_;
    GOC_Inventory* shop_;
    GOC_Inventory* basket_;
    int basketSlotMarks_[5];
    int savedBasketMark_;
    String itemTag_;
    long long price_;
    bool updatePrice_;
};
