#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "Actor.h"
#include "GOC_Inventory.h"

#include "UISlotPanel.h"

#include "UIC_ShopPanel.h"


const String SHOPITEM("Shop_");
const String BASKETITEM("Basket_");

UIC_ShopPanel::UIC_ShopPanel(Context* context) :
    UISlotPanel(context),
    shop_(0),
    basket_(0)
{
    SetInventorySection("Shop");
    basket_ = new GOC_Inventory(context);
    basket_->SetTemplate("InventoryTemplate_Basket_5SlotsQ500");
}

UIC_ShopPanel::~UIC_ShopPanel()
{
    if (basket_)
        delete basket_;
}

void UIC_ShopPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_ShopPanel>();
}

void UIC_ShopPanel::Start(Object* user, Object* feeder)
{
    shopZone_ = panel_->GetChild("ShopSlotZone", true);
    basketZone_ = panel_->GetChild("BasketSlotZone", true);

    priceText_ = static_cast<Text*>(panel_->GetChild("PriceText", true));

    UISlotPanel::Start(user, feeder);

    equipment_ = 0;
    inventory_ = shop_ = 0;
    basket_->ResetSlots();

    URHO3D_LOGINFOF("UIC_ShopPanel() - Start panel=%s : feeder=%u", panel_->GetName().CString(), feeder);

    if (feeder && user != feeder)
    {
        Node* holdernode = ((Actor*)feeder)->GetAvatar();

        if (holdernode)
        {
            // Overide OnSwitchVisible
            Button* closeButton = dynamic_cast<Button*>(panel_->GetChild("OkButton", true));
            if (closeButton)
                SubscribeToEvent(closeButton, E_RELEASED, URHO3D_HANDLER(UIC_ShopPanel, OnShopClosing));

            shop_ = holdernode->GetComponent<GOC_Inventory>();

            UpdateNodes();

//            URHO3D_LOGINFOF("UIC_ShopPanel() - Start panel=%s : holderNode_=%s(%u)", panel_->GetName().CString(),
//                            holdernode->GetName().CString(), holdernode->GetID());
        }
    }
}

void UIC_ShopPanel::ResetShop()
{
    URHO3D_LOGINFOF("UIC_ShopPanel() - ResetShop %s", panel_->GetName().CString());

    if (IsVisible())
        ToggleVisible();

    CancelBasket();

    equipment_ = 0;
    inventory_ = shop_ = 0;
    feeder_ = 0;
    updatePrice_ = false;

    UnsubscribeFromAllEvents();

    UISlotPanel::Reset();
}

void UIC_ShopPanel::SetSlotZone()
{
    slotSize_ = 64;

    Actor* actor = ((Actor*)user_);

//    URHO3D_LOGINFOF("UIC_ShopPanel() - SetSlotZone : panel=%s user=%u", name_.CString(), actor);

    if (actor)
    {
        shopZone_->SetVar(GOA::OWNERID, actor->GetID());
        shopZone_->SetVar(GOA::PANELID, name_);
        basketZone_->SetVar(GOA::OWNERID, actor->GetID());
        basketZone_->SetVar(GOA::PANELID, name_);
    }

    if (slotZone_ != shopZone_)
        slotZone_ = shopZone_;

    SwitchInventoryTo(shop_);
}

void UIC_ShopPanel::SelectInventoryFrom(const String& name)
{
    if (name.StartsWith(BASKETITEM))
        SwitchInventoryTo(basket_);
    else
        SwitchInventoryTo(shop_);
}

void UIC_ShopPanel::SwitchInventoryTo(GOC_Inventory* inventory)
{
    if (!inventory)
        return;

//    URHO3D_LOGINFOF("UIC_ShopPanel() - SwitchInventoryTo : panel=%s inventory=%u", name_.CString(), inventory->GetID());

    if (inventory_ != inventory)
    {
        if (inventory == basket_)
        {
            inventory_ = basket_;
            slotZone_ = basketZone_;
            itemTag_ = BASKETITEM;
            // no moneyslot in basket
            startSlotIndex_ = 0;
            endSlotIndex1_ = endSlotIndex2_ = 4;
            numSlots_ = endSlotIndex2_ - startSlotIndex_ + 1;
            slotselector_ = startSlotIndex_;
//            URHO3D_LOGINFOF("UIC_ShopPanel() - SwitchInventoryTo : panel=%s BASKET", name_.CString());
        }
        else
        {
            inventory_ = shop_;
            slotZone_ = shopZone_;
            itemTag_ = SHOPITEM;
            if (inventorySectionStart_ != String::EMPTY)
            {
                startSlotIndex_ = inventory_->GetSectionStartIndex(inventorySectionStart_);
                endSlotIndex1_ = endSlotIndex2_ = inventory_->GetSectionEndIndex(inventorySectionEnd_);
                numSlots_ = endSlotIndex2_ - startSlotIndex_ + 1;
                slotselector_ = startSlotIndex_;
            }
//            URHO3D_LOGINFOF("UIC_ShopPanel() - SwitchInventoryTo : panel=%s SHOP slotzone=%u startSlotIndex=%u endSlotIndex=%u",
//                             name_.CString(), slotZone_, startSlotIndex_, endSlotIndex2_);
        }
    }
}


void UIC_ShopPanel::CancelBasket()
{
    if (!basket_ || basket_->Empty())
        return;

    Node* userNode = ((Actor*)user_)->GetAvatar();
    GOC_Inventory* userinv = userNode->GetComponent<GOC_Inventory>();

    for (unsigned i=0; i<5; i++)
    {
        Slot slot = basket_->GetSlot(i);
        if (!slot.quantity_)
            continue;

        if (basketSlotMarks_[i] == SHOPBASKET_TOBUY) // 0 = Mark "ToBuy"
        {
            // restore to shop
            int slotIndex = shop_->AddCollectableFromSlot(slot, slot.quantity_, 0);
            if (slotIndex == -1)
            {
                // log error : can't restore to shop
            }
        }
        else if (basketSlotMarks_[i] == SHOPBASKET_TOSELL) // 1 = Mark "ToSell"
        {
            // restore to user
            basket_->TransferSlotTo(i, userinv);
        }
    }

    basket_->ResetSlots();
    UpdateBasketSlots();
    UpdatePrice();
}

bool UIC_ShopPanel::CheckValidateBasket()
{
    return true;
}

void UIC_ShopPanel::ValidateBasket()
{
    if (!basket_ || basket_->Empty())
        return;

    Node* userNode = ((Actor*)user_)->GetAvatar();
    if (!userNode || userNode->GetVar(GOA::ISDEAD).GetBool())
        return;

    GOC_Inventory* userinv = userNode->GetComponent<GOC_Inventory>();

    /// Transfer MONEY (slot=0)
    {
        if (price_ > 0)
            userinv->TransferSlotTo(0, holderNode_, price_);
        else if (price_ < 0)
            shop_->TransferSlotTo(0, userNode, -price_);

        userNode->SendEvent(UI_MONEYUPDATED);
    }

    /// Transfer Basket Items
    bool updateshop = false;
    for (unsigned i=0; i<5; i++)
    {
        Slot slot = basket_->GetSlot(i);
        if (!slot.quantity_)
            continue;

        if (basketSlotMarks_[i] == SHOPBASKET_TOBUY) // 0 = Mark "ToBuy"
        {
            basket_->TransferSlotTo(i, userinv);
        }
        else if (basketSlotMarks_[i] == SHOPBASKET_TOSELL) // 1 = Mark "ToSell"
        {
            // transfer to shop
            int slotIndex = shop_->AddCollectableFromSlot(slot, slot.quantity_, 0);
            if (slotIndex != -1)
                updateshop = true;
        }
    }

    if (updateshop)
        UpdateShopSlots();

    basket_->ResetSlots();
    UpdateBasketSlots();
    UpdatePrice();
}

void UIC_ShopPanel::UpdateSlot(unsigned index, bool updateButtons, bool updateSubscribers)
{
//    URHO3D_LOGINFOF("UIC_ShopPanel() - UpdateSlot : idSlot=%u inventory=%u", index, inventory_);
    if (!inventory_)
    {
        URHO3D_LOGERRORF("UIC_ShopPanel() - UpdateSlot : idSlot=%u no inventory !", index);
        return;
    }

    const Slot& slot = inventory_->GetSlot(index);

    String slotname(itemTag_);
    slotname += String(index-startSlotIndex_+1);
    UIElement* uiSlot = static_cast<UIElement*>(slotZone_->GetChild(slotname, true));

    if (!uiSlot)
        return;

    if (!slot.quantity_)
    {
        uiSlot->RemoveAllChildren();
        if (inventory_ == basket_)
            MarkBasketSlot(index, SHOPBASKET_EMPTY);
        return;
    }

    Button* button = static_cast<Button*>(uiSlot->GetChild(0));
    if (!button)
    {
        button = uiSlot->CreateChild<Button>();
        updateButtons = true;
    }
    if (updateButtons)
    {
        if (!slot.sprite_)
        {
            URHO3D_LOGERRORF("UIC_ShopPanel() - UpdateSlot: idSlot=%u no Sprite in slot !", index);
            return;
        }
        button->SetTexture(slot.sprite_->GetTexture());
//        button->SetEnableDpiScale(false);
        button->SetImageRect(slot.sprite_->GetRectangle());
        IntVector2 size = slot.sprite_->GetRectangle().Size();
        size.x_ = (int)((float)size.x_ * GameContext::Get().uiScale_);
        size.y_ = (int)((float)size.y_ * GameContext::Get().uiScale_);
        GameHelpers::ClampSizeTo(size, slotSize_ /* * GameContext::Get().uiScale_ */);
        button->SetSize(size);
        button->SetDragDropMode(DD_SOURCE_AND_TARGET);
        button->SetHoverOffset(0, 0);
        button->SetAlignment(HA_CENTER, VA_CENTER);
        if (inventory_ == basket_)
            button->SetColor(Color(basketSlotMarks_[index] == SHOPBASKET_TOBUY, basketSlotMarks_[index] == SHOPBASKET_TOSELL, basketSlotMarks_[index] == SHOPBASKET_EMPTY));
        SubscribeToEvent(button, E_DRAGBEGIN, URHO3D_HANDLER(UIC_ShopPanel, HandleSlotDragBegin));
        SubscribeToEvent(button, E_DRAGMOVE, URHO3D_HANDLER(UIC_ShopPanel, HandleSlotDragMove));
        SubscribeToEvent(button, E_DRAGEND, URHO3D_HANDLER(UIC_ShopPanel, HandleSlotDragEnd));
    }
    else if (updateSubscribers)
    {
        SubscribeToEvent(button, E_DRAGBEGIN, URHO3D_HANDLER(UIC_ShopPanel, HandleSlotDragBegin));
        SubscribeToEvent(button, E_DRAGMOVE, URHO3D_HANDLER(UIC_ShopPanel, HandleSlotDragMove));
        SubscribeToEvent(button, E_DRAGEND, URHO3D_HANDLER(UIC_ShopPanel, HandleSlotDragEnd));
    }

    // Add Quantity Text
    if (slot.quantity_ > 1)
    {
        Text* quantity = static_cast<Text*>(button->GetChild(0));
        if (!quantity)
        {
            quantity = button->CreateChild<Text>();
            updateButtons = true;
        }
        if (updateButtons)
        {
            quantity->SetAlignment(HA_RIGHT, VA_TOP);
            quantity->SetColor(Color(0.f, 1.f, 0.f));
            quantity->SetFont(GameContext::Get().uiFonts_[UIFONTS_ABY22], 22);
            quantity->SetText(String(slot.quantity_));
            quantity->SetVisible(true);
        }
    }
    // Remove Quantity Text if quantity == 0
    else
    {
        Text* quantity = static_cast<Text*>(button->GetChild(0));
        if (quantity)
            quantity->SetVisible(false);
    }

    // Add Price Text
    {
        Text* slotprice = static_cast<Text*>(button->GetChild(1));
        if (!slotprice)
        {
            slotprice = button->CreateChild<Text>();
            updateButtons = true;
        }
        if (updateButtons)
        {
            slotprice->SetAlignment(HA_CENTER, VA_BOTTOM);
            slotprice->SetPosition(0, 20);
            slotprice->SetFont(GameContext::Get().uiFonts_[UIFONTS_ABY22], 22);
        }

        String value(slot.quantity_*GOT::GetDefaultValue(slot.type_));

        // Add BasketMark for "Sell" or "Buy"
        if (inventory_ == basket_ && GetBasketSlotMark(index) == SHOPBASKET_TOSELL)
        {
            // in basket with mark ToSell
            slotprice->SetColor(Color(0.f, 0.f, 1.f));
            value = String("+") + value;
        }
        else
        {
            // in shop or in basket with no mark ToSell => yellow color
            slotprice->SetColor(Color(1.f, 1.f, 0.f));
        }

        slotprice->SetText(value);
    }
}

void UIC_ShopPanel::UpdatePrice()
{
    if (updatePrice_)
    {
//        URHO3D_LOGINFOF("UIC_ShopPanel() - UpdatePrice !");

        price_ = 0;
        int filledbasket = 5;
        for (unsigned i=0; i < 5; ++i)
        {
            if (GetBasketSlotMark(i) == SHOPBASKET_EMPTY)
            {
                filledbasket--;
                continue;
            }

            const Slot& slot = basket_->GetSlot(i);
            price_ += GetBasketSlotMark(i) * (long int)slot.quantity_ * GOT::GetDefaultValue(slot.type_);
        }

        if (!filledbasket)
        {
            priceText_->SetText(String::EMPTY);
            priceText_->SetColor(Color(0.f, 0.f, 1.f));
            priceText_->GetParent()->SetColor(Color(0.5f, 0.5f, 0.5f, 0.5f));
            UnsubscribeFromEvent(priceText_->GetParent(), E_RELEASED);
//            URHO3D_LOGINFOF("UIC_ShopPanel() - UpdatePrice : not filled");
        }
        else
        {
            /// Set Price Button
            {
                UIElement* pricebutton = priceText_->GetParent();
                unsigned int money = ((Actor*)user_)->GetAvatar()->GetComponent<GOC_Inventory>()->GetQuantityfor(COT::MONEY);

                if (money >= price_ || price_ <= 0)
                {
                    /// user can buy the basket
                    pricebutton->SetColor(Color(0.f, 1.f, 0.f, 0.8f));
                    SubscribeToEvent(pricebutton, E_RELEASED, URHO3D_HANDLER(UIC_ShopPanel, OnReleasePriceButton));
//                    URHO3D_LOGINFOF("UIC_ShopPanel() - UpdatePrice : subscribe !");
                }
                else
                {
                    /// not enough money to buy the basket
                    pricebutton->SetColor(Color(1.f, 0.f, 0.f, 0.8f));
                    UnsubscribeFromEvent(pricebutton, E_RELEASED);
//                    URHO3D_LOGINFOF("UIC_ShopPanel() - UpdatePrice : unsubscribe !");
                }
            }

            /// Set Price Text
            {
                priceText_->SetText(String(Abs(price_)));
                if (price_ >= 0)
                    priceText_->SetColor(Color(1.f, 1.f, 0.f));
                else
                    priceText_->SetColor(Color(0.f, 0.f, 1.f));
            }
        }

        updatePrice_ = false;
    }
}

void UIC_ShopPanel::UpdateShopSlots(bool updateButtons, bool updateSubscribers)
{
    // Update Shop slots
//    URHO3D_LOGINFOF("UIC_ShopPanel() - UpdateShopSlots : start=%u to end=%u !", startSlotIndex_, endSlotIndex2_);

    SwitchInventoryTo(shop_);
    for (unsigned i = startSlotIndex_; i <= endSlotIndex2_; ++i)
        UpdateSlot(i, updateButtons, updateSubscribers);
}

void UIC_ShopPanel::UpdateBasketSlots(bool updateButtons, bool updateSubscribers)
{
    // Update Basket slots & Price
//    URHO3D_LOGINFOF("UIC_ShopPanel() - UpdateBasketSlots !");

    SwitchInventoryTo(basket_);
    for (unsigned i = 0; i < 5; ++i)
        UpdateSlot(i, updateButtons, updateSubscribers);

    updatePrice_ = true;
}

void UIC_ShopPanel::UpdateSlots(bool updateButtons, bool updateSubscribers)
{
    if (!inventory_)
        SwitchInventoryTo(shop_);

    if (!inventory_)
    {
        URHO3D_LOGERRORF("UIC_ShopPanel() - UpdateSlots : inventory_=0 ! ");
        return;
    }

    URHO3D_LOGINFOF("UIC_ShopPanel() - UpdateSlots !");

    UpdateShopSlots(updateButtons, updateSubscribers);
    UpdateBasketSlots(updateButtons, updateSubscribers);
    UpdatePrice();
}


void UIC_ShopPanel::OnReleasePriceButton(StringHash eventType, VariantMap& eventData)
{
    ValidateBasket();
}

void UIC_ShopPanel::OnShopClosing(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("UIC_ShopPanel() - OnShopClosing : panel=%s Player=%u Actor=%u", panel_->GetName().CString(),
//                   user_ ?  ((Actor*)user_)->GetID() : 0, feeder_ ? ((Actor*)feeder_)->GetID() : 0);

    if (feeder_) eventData[Dialog_Close::ACTOR_ID] = ((Actor*)feeder_)->GetID();
    if (user_) eventData[Dialog_Close::PLAYER_ID] = ((Actor*)user_)->GetID();

    // first send message to actor
    if (feeder_)
        feeder_->SendEvent(DIALOG_CLOSE, eventData);
    // second to player who reset shop (clear feeder_)
    if (user_)
        user_->SendEvent(DIALOG_CLOSE, eventData);
}

int UIC_ShopPanel::GetFocusSlotId(UIElement* focus, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel)
{
    /// Shop has 2 inventories (shop & basket)

    /// Check Here all allowed cases

    int slotId = -1;
    savedBasketMark_ = 0;

    dragFromInventory_ = fromPanel == this ? GetInventory() : 0;

    if (!focus->GetName().Empty())
        SelectInventoryFrom(focus->GetName());

    if (fromPanel == this)
    {
        URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : from ShopPanel ...");

        if (GetInventory() == GetShop())
        {
            /// Inside the shop inventory, Swap Cases Not Allowed
            if (dragFromInventory_ == GetShop())
            {
                URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : Swap Inside Same Inventory not allowed !");
                SwitchInventoryTo(dragFromInventory_);
                return -1;
            }
            /// From Basket To Shop : Not Allowed if fromslot is not marked to Buy
            else if (GetBasketSlotMark(fromSlotId) != SHOPBASKET_TOBUY)
            {
                URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : From Basket To Shop : Not Allowed if fromslot is not marked to Buy !");
                SwitchInventoryTo(dragFromInventory_);
                return -1;
            }
        }

        slotId = GetInventory()->GetSlotIndexFor(fromSlot.type_, true);

        /// From Shop to Basket
        if (dragFromInventory_ == GetShop() && GetInventory() == GetBasket())
        {
            /// Not Allowed if focus is marked to Sell
            if (GetBasketSlotMark(slotId) == SHOPBASKET_TOSELL)
            {
                URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : From Shop to Basket : Not Allowed if focus is marked to Sell !");
                SwitchInventoryTo(dragFromInventory_);
                return -1;
            }
        }
    }
    else
    {
        /// From other Panel, only Basket Inventory is allowed
        if (GetInventory() != GetBasket())
        {
            URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : From other Panel, only Basket Inventory is allowed !");
            SwitchInventoryTo(dragFromInventory_);
            return -1;
        }

        /// Get the focused slot id
        slotId = GetInventory()->GetSlotIndex(focus->GetName());

        /// If not slotid, find a free Slot
        if (slotId == -1)
        {
            slotId = GetInventory()->GetSlotIndexFor(fromSlot.type_, true);
            if (slotId == -1)
            {
                URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : can't find a free slot !");
                SwitchInventoryTo(dragFromInventory_);
                return -1;
            }
        }

        /// Focus marked to BUY is not Allowed
        if (GetBasketSlotMark(slotId) == SHOPBASKET_TOBUY)
        {
            URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : Focus marked to BUY is not Allowed !");
            SwitchInventoryTo(dragFromInventory_);
            return -1;
        }

        /// From other Panel to Basket => Mark ToSell
        URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : fromPanel=%s to Basket slotid=%d : mark SHOPBASKET_TOSELL !", fromPanel->GetName().CString(), slotId);
        savedBasketMark_ = SHOPBASKET_TOSELL;
    }

    URHO3D_LOGINFOF("UIC_ShopPanel() - GetFocusSlotId : ... slotId=%d !", slotId);

    return slotId;
}

bool UIC_ShopPanel::AcceptSlotType(int slotId, const StringHash& type, UISlotPanel* otherPanel)
{
    /// Check allowed cases (ShopPanel -> UserPanel)

    if (otherPanel != this)
    {
        /// Not Allowed between Shop and other Panel
        if (GetInventory() == GetShop())
        {
            URHO3D_LOGINFOF("UIC_ShopPanel() - AcceptSlotType : Not Allowed between Shop and other Panel !");
            return false;
        }

        /// In Basket : Save the mark for OnDragSlotFinish (when restore the remain)
        if (!savedBasketMark_)
            savedBasketMark_ = GetBasketSlotMark(slotId);

        /// In Basket : Not Allowed to drag a slot marked to BUY
        if (GetBasketSlotMark(slotId) == SHOPBASKET_TOBUY)
        {
            URHO3D_LOGINFOF("UIC_ShopPanel() - AcceptSlotType : In Basket : Not Allowed to drag a slot marked to BUY !");
            return false;
        }
    }

    return true;
}

void UIC_ShopPanel::OnDragSlotIn(int slotId, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel)
{
    if (fromPanel == this)
    {
        URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : slotid=%d", slotId);

        /// Copy the slot
        Slot toSlot = GetInventory()->GetSlot(slotId);

        unsigned int quantity = fromSlot.quantity_;
        unsigned int remainquantity = quantity;

        /// toSlot is empty => move slot
        if (toSlot.Empty())
        {
            /// From Basket
            if (dragFromInventory_ == GetBasket())
            {
                /// To Basket
                if (GetInventory() == GetBasket())
                {
                    MarkBasketSlot(slotId, GetBasketSlotMark(fromSlotId));
                    URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : From Basket to Basket : move slot and move mark !");
                }

                MarkBasketSlot(fromSlotId, SHOPBASKET_EMPTY);
            }
            /// From Shop To Basket
            else if (GetInventory() == GetBasket())
            {
                MarkBasketSlot(slotId, SHOPBASKET_TOBUY);
                URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : From Shop to Basket : move slot and mark SHOPBASKET_TOBUY !");
            }

            GetInventory()->AddCollectableFromSlot(fromSlot, slotId, remainquantity, false, false);
        }
        /// Swap Items
        else
        {
            /// From Basket
            if (dragFromInventory_ == GetBasket())
            {
                /// To Basket
                if (GetInventory() == GetBasket())
                {
                    /// Swap the basketmarks
                    int tempmark = GetBasketSlotMark(slotId);
                    MarkBasketSlot(slotId, GetBasketSlotMark(fromSlotId));
                    MarkBasketSlot(fromSlotId, tempmark);
                    URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : From Basket to Basket : swap slots and swap marks !");
                }
                else
                    /// To Shop
                {
                    URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : From Basket to Shop : swap slots already marked SHOPBASKET_TOBUY !");
                }
            }
            /// From Shop
            else
            {
                /// To Shop
                if (GetInventory() == GetShop())
                {
                    URHO3D_LOGERRORF("UIC_ShopPanel() - OnDragSlotIn : From Shop to Shop : not allowed !");
                    return;
                }
                /// To Basket
                else
                {
                    URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : From Shop to Basket : swap slots already marked SHOPBASKET_TOBUY !");
                }
            }

            dragFromInventory_->AddCollectableFromSlot(toSlot, fromSlotId, toSlot.quantity_, true);
            GetInventory()->RemoveCollectableFromSlot(slotId);
            GetInventory()->AddCollectableFromSlot(fromSlot, slotId, remainquantity, false, false);

            /// Remain quantity on toPanel
            if (toSlot.quantity_ > 0)
                OnSlotRemain(toSlot, slotId);
        }

        if (quantity != remainquantity)
        {
            long int deltaqty = (long int)quantity - (long int)remainquantity;

            // update quantity in the frompanel
            fromSlot.quantity_ = fromSlot.quantity_ <= deltaqty ? 0 : fromSlot.quantity_ - deltaqty;

            /// update slot
            OnDragSlotFinish(slotId);
        }
    }
    else
    {
        /// From User Panel to Basket
        URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotIn : fromPanel=%s to Basket slotid=%d !", fromPanel->GetName().CString(), slotId);
        UISlotPanel::OnDragSlotIn(slotId, fromSlot, fromSlotId, fromPanel);
    }

    /// for Remain quantity on fromPanel
    if (dragFromInventory_)
        SwitchInventoryTo(dragFromInventory_);
}

void UIC_ShopPanel::OnSlotRemain(Slot& slot, int slotid)
{
    URHO3D_LOGINFOF("UIC_ShopPanel() - OnSlotRemain ...");

    /// Restore the remain quantity in the source
    if (slot.quantity_ > 0)
    {
        URHO3D_LOGINFOF("UIC_ShopPanel() - OnSlotRemain : restore remain item=%s qty=%d to inventory startid=%d !",
                        GOT::GetType(slot.type_).CString(), slot.quantity_, slotid);

        GetInventory()->AddCollectableFromSlot(slot, slotid, slot.quantity_, false, false);
    }

    OnDragSlotFinish(slotid);
}

void UIC_ShopPanel::OnDragSlotFinish(int slotId)
{
    URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotFinish ...");

    if (slotId != -1 && GetInventory() == GetBasket() && savedBasketMark_ != 0)
    {
        URHO3D_LOGINFOF("UIC_ShopPanel() - OnDragSlotFinish : In Basket slotid=%d basketmark=%d!", slotId, savedBasketMark_);
        MarkBasketSlot(slotId, savedBasketMark_);
    }

    UpdateSlots(true, true);
}

