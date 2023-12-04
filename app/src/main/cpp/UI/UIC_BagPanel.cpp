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

#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Animator2D.h"
#include "GOC_Collectable.h"
#include "GOC_Inventory.h"
#include "GOC_ZoneEffect.h"

#include "Equipment.h"

#include "UISlotPanel.h"

#include "UIC_BagPanel.h"



UIC_BagPanel::UIC_BagPanel(Context* context) :
    UISlotPanel(context)
{
    SetInventorySection("Bag");
}

void UIC_BagPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_BagPanel>();
}

void UIC_BagPanel::SetSlotZone()
{
    UISlotPanel::SetSlotZone();
    slotSize_ = 64;
    miniSlotSize_ = 16;
}

void UIC_BagPanel::UpdateSlot(unsigned index, bool updateButtons, bool updateSubscribers, bool updateNet)
{
    if (!slotZone_)
        return;

//    URHO3D_LOGINFOF("UIC_BagPanel() - UpdateSlot : idSlot=%u updatebuttons=%s ...", index, updateButtons ? "true":"false");

    const Slot& slot = inventory_->GetSlot(index);

    UIElement* uiSlot = static_cast<UIElement*>(slotZone_->GetChild(String("Bag_" + String(index-startSlotIndex_+1)), true));
    if (!uiSlot)
        return;

    if (GameContext::Get().ClientMode_ && updateNet)
    {
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[Net_ObjectCommand::P_NODEIDFROM] = inventory_->GetNode()->GetID();
        eventData[Net_ObjectCommand::P_INVENTORYIDSLOT] = index;
        Slot::GetSlotData(slot, eventData);
        SendEvent(GO_INVENTORYSLOTSET, eventData);
    }

    if (!slot.quantity_)
    {
        uiSlot->RemoveAllChildren();
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
        if (!slot.uisprite_)
        {
            URHO3D_LOGERRORF("UIC_BagPanel() - UpdateSlot: idSlot=%u no Sprite in slot !", index);
            return;
        }
        button->SetTexture(slot.uisprite_->GetTexture());
//        button->SetEnableDpiScale(false);
        button->SetImageRect(slot.uisprite_->GetRectangle());
        IntVector2 size = slot.uisprite_->GetRectangle().Size();
        size.x_ = (int)((float)size.x_ * GameContext::Get().uiScale_);
        size.y_ = (int)((float)size.y_ * GameContext::Get().uiScale_);
        // don't need uiscale after that
        GameHelpers::ClampSizeTo(size, (int)(((float)slotSize_/* * GameContext::Get().uiScale_*/) + 0.5f));
        button->SetSize(size);
        button->SetDragDropMode(DD_SOURCE_AND_TARGET);
        button->SetAlignment(HA_CENTER, VA_CENTER);
        updateSubscribers = true;
    }
    if (updateSubscribers)
    {
        SubscribeToEvent(button, E_DRAGBEGIN, URHO3D_HANDLER(UIC_BagPanel, HandleSlotDragBegin));
        SubscribeToEvent(button, E_DRAGMOVE, URHO3D_HANDLER(UIC_BagPanel, HandleSlotDragMove));
        SubscribeToEvent(button, E_DRAGEND, URHO3D_HANDLER(UIC_BagPanel, HandleSlotDragEnd));
        SubscribeToEvent(button, E_DOUBLECLICK, URHO3D_HANDLER(UIC_BagPanel, HandleDoubleClic));
    }

    Text* quantity = static_cast<Text*>(button->GetChild(0));
    if (!quantity)
    {
        quantity = button->CreateChild<Text>();
        quantity->SetAlignment(HA_CENTER, VA_TOP);
        quantity->SetColor(Color(0.f, 1.f, 0.f));
        quantity->SetFont(GameContext::Get().uiFonts_[UIFONTS_ABY22], 22);
    }

    if (slot.quantity_ > 1)
        quantity->SetText(String(slot.quantity_));

    quantity->SetVisible(slot.quantity_ > 1);

    BorderImage* effect = static_cast<BorderImage*>(button->GetChild(1));
    if (!effect)
    {
        effect = button->CreateChild<BorderImage>();
        effect->SetAlignment(HA_RIGHT, VA_TOP);
        effect->SetOpacity(0.99f);
        effect->SetVisible(true);
    }

    if (slot.effect_ > -1)
        GameHelpers::SetUIElementFrom(effect, UIEQUIPMENT, String("effect") + String(slot.effect_), miniSlotSize_);

    effect->SetVisible(slot.effect_ > -1);
}

void UIC_BagPanel::OnResize()
{
//    if (!holderNode_ || !inventory_)
//        return;
//
//    URHO3D_LOGINFOF("UIC_BagPanel() - OnResize");
//
//    SetSlotZone();
//    UpdateSlots(true);
}

void UIC_BagPanel::HandleDoubleClic(StringHash eventType, VariantMap& eventData)
{
    UIElement* uielement = static_cast<UIElement*>(eventData["Element"].GetPtr());
    if (!uielement)
        return;

    uielement = uielement->GetParent();
    if (!uielement || uielement->GetName().Empty())
        return;

    int slotIndex = inventory_->GetSlotIndex(uielement->GetName());
    if (slotIndex == -1)
    {
        URHO3D_LOGERRORF("UIC_BagPanel() - HandleDoubleClic : on uielement=%s no slotindex !", uielement->GetName().CString());
        return;
    }

    Slot& slot = inventory_->GetSlot(slotIndex);

    URHO3D_LOGINFOF("UIC_BagPanel() - HandleDoubleClic : on index=%d slot=%s(%u)", slotIndex, GOT::GetType(slot.type_).CString(), slot.type_.Value());

    if (UseSlotItem(slot, slotIndex) != -1)
        UpdateSlot(slotIndex);
}
