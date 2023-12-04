#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/SpriterData2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>

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

#include "GOC_Inventory.h"
#include "Player.h"
#include "Equipment.h"

#include "UISlotPanel.h"

#include "UIC_EquipmentPanel.h"



UIC_EquipmentPanel::UIC_EquipmentPanel(Context* context) :
    UISlotPanel(context),
    lastchange_(0)
{
    SetInventorySection("Equipment");
}

UIC_EquipmentPanel::~UIC_EquipmentPanel()
{ }

void UIC_EquipmentPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_EquipmentPanel>();
}

void UIC_EquipmentPanel::Start(Object* user, Object* feeder)
{
    UISlotPanel::Start(user, feeder);

    SubscribeToEvent((Object*) equipment_, GO_EQUIPMENTUPDATED, URHO3D_HANDLER(UIC_EquipmentPanel, OnEquipmentUpdate));
}

void UIC_EquipmentPanel::Reset()
{
    UISlotPanel::Reset();
}

void UIC_EquipmentPanel::SetSlotZone()
{
    UISlotPanel::SetSlotZone();

    if (!slotZone_)
        return;

    wearablesZone_ = slotZone_->GetChild("Wearables", true);
    nowearablesZone_ = slotZone_->GetChild("NonWearables", true);

    Sprite2D* slotSprite = Sprite2D::LoadFromResourceRef(GetContext(), ResourceRef(SpriteSheet2D::GetTypeStatic(), "UI/game_equipment.xml@slot"));
    float slotscale = 272.f / ((float)(slotSprite->GetRectangle().Size().x_) * sqrt(numSlots_ + 1.f));
    slotSize_ = (int)((float)slotSprite->GetRectangle().Size().x_ * slotscale);
    miniSlotSize_ = slotSize_ / 2;

    URHO3D_LOGINFOF("UIC_EquipmentPanel() - SetSlotZone : numslots=%u size=%d slotscale=%f", numSlots_, slotSize_, slotscale);
}

void UIC_EquipmentPanel::UpdateSlot(unsigned index, bool updateButtons, bool updateSubscribers, bool updateNet)
{
    const Slot& slot = inventory_->GetSlot(index);
    String slotName = inventory_->GetSlotName(index);

    UIElement* rootZone = slotName.StartsWith("Special") ? nowearablesZone_ : wearablesZone_;

    UIElement* uiSlot = static_cast<UIElement*>(rootZone->GetChild(slotName, true));

    if (!uiSlot)
        return;

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
            URHO3D_LOGERRORF("UIC_EquipmentPanel() - UpdateSlots : idSlot=%u no Sprite in slot !", index);
            return;
        }

        IntVector2 ssize = slot.uisprite_->GetRectangle().Size();
        ssize.x_ = (int)((float)ssize.x_/* * GameContext::Get().uiScale_*/);
        ssize.y_ = (int)((float)ssize.y_/* * GameContext::Get().uiScale_*/);
        GameHelpers::ClampSizeTo(ssize, uiSlot->GetSize().x_);
        button->SetAlignment(HA_CENTER, VA_CENTER);
        button->SetTexture(slot.uisprite_->GetTexture());
        button->SetImageRect(slot.uisprite_->GetRectangle());
        button->SetSize(ssize);
        button->SetDragDropMode(DD_SOURCE_AND_TARGET);
        SubscribeToEvent(button, E_DRAGBEGIN, URHO3D_HANDLER(UIC_EquipmentPanel, HandleSlotDragBegin));
        SubscribeToEvent(button, E_DRAGMOVE, URHO3D_HANDLER(UIC_EquipmentPanel, HandleSlotDragMove));
        SubscribeToEvent(button, E_DRAGEND, URHO3D_HANDLER(UIC_EquipmentPanel, HandleSlotDragEnd));
    }

    int equipvalue = (int)floor(equipment_->GetEquipmentValue(index, GOA::LIFE).GetFloat() * 10.f);
    Text* stateText = static_cast<Text*>(button->GetChild(0));
    if (!stateText)
    {
        stateText = button->CreateChild<Text>();
        stateText->SetAlignment(HA_CENTER, VA_TOP);
        stateText->SetFont(GameContext::Get().uiFonts_[UIFONTS_ABY22], 22);
        stateText->SetColor(equipvalue > 0 ? Color(0.f, 1.f, 0.f) : Color(1.f, 0.f, 0.f));
    }
    if (equipvalue && equipvalue != ToInt(stateText->GetText()))
    {
        stateText->SetText(String(equipvalue));
    }
    stateText->SetVisible(equipvalue);

    BorderImage* effect = static_cast<BorderImage*>(button->GetChild(1));
    if (!effect)
    {
        effect = button->CreateChild<BorderImage>();
        effect->SetAlignment(HA_RIGHT, VA_TOP);
        effect->SetOpacity(0.99f);
    }
    if (slot.effect_ > -1)
    {
        GameHelpers::SetUIElementFrom(effect, UIEQUIPMENT, String("effect") + String(slot.effect_), miniSlotSize_);
    }
    effect->SetVisible(slot.effect_ > -1);
}

void UIC_EquipmentPanel::OnResize()
{
    URHO3D_LOGINFOF("UIC_EquipmentPanel() - OnResize");

    SetSlotZone();
    bool ok = UpdateSlotPositions(true);
}

void UIC_EquipmentPanel::OnSlotUpdate(StringHash eventType, VariantMap& eventData)
{
    unsigned index = eventData[Go_InventoryGet::GO_IDSLOT].GetUInt();

    if (index < startSlotIndex_ || index > endSlotIndex2_)
    {
        URHO3D_LOGWARNINGF("UIC_EquipmentPanel() - OnSlotUpdate : panel=%s index %u out of section (%u to %u) !", name_.CString(), index, startSlotIndex_, endSlotIndex2_);
        return;
    }

    URHO3D_LOGINFOF("UIC_EquipmentPanel() - OnSlotUpdate : panel=%s index=%u...", name_.CString(), index);

    UpdateEquipment(index);
}

void UIC_EquipmentPanel::OnDragSlotIn(int slotId, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel)
{
    /// Transfer the slot
    Slot& slot = GetInventory()->GetSlot(slotId);

    /// Limit the transfered quantity to 1 for the Equipment Panel
    unsigned int quantity = 1U;

    /// toSlot is empty => move slot
    if (slot.Empty())
    {
        GetInventory()->AddCollectableFromSlot(fromSlot, slotId, quantity, false, false, GetStartSlotIndex());

        URHO3D_LOGINFOF("UIC_EquipmentPanel() - OnDragSlot : fill empty slotid=%d!", slotId);
    }
    /// Acceptance of the source and destination Inventories => Swap Items
    else if (GetInventory()->GetTemplate()->CanSlotCollectCategory(slotId, fromSlot.type_) &&
             fromPanel->GetInventory()->GetTemplate()->CanSlotCollectCategory(fromSlotId, slot.type_))
    {
        fromPanel->GetInventory()->AddCollectableFromSlot(slot, fromSlotId, slot.quantity_, false, false, fromPanel->GetStartSlotIndex());
        GetInventory()->RemoveCollectableFromSlot(slotId);
        GetInventory()->AddCollectableFromSlot(fromSlot, slotId, quantity, false, false, GetStartSlotIndex());
        URHO3D_LOGINFOF("UIC_EquipmentPanel() - OnDragSlot : swap items !");
    }
    else
    {
        URHO3D_LOGINFOF("UIC_EquipmentPanel() - OnDragSlot : slotid=%d can't accept fromtype !", slotId);
        return;
    }

    /// update quantity
    fromSlot.quantity_ = fromSlot.quantity_ > 0 ? fromSlot.quantity_-1 : 0U;

    /// update slot
    OnDragSlotFinish(slotId);
}

void UIC_EquipmentPanel::OnDragSlotFinish(int index)
{
    UpdateEquipment(index);
}

void UIC_EquipmentPanel::UpdateEquipment(unsigned index)
{
    URHO3D_LOGINFOF("UIC_EquipmentPanel() - UpdateEquipment : panel=%s index=%u...", name_.CString(), index);

    if (index < startSlotIndex_ || index > endSlotIndex2_)
    {
        URHO3D_LOGWARNINGF("UIC_EquipmentPanel() - UpdateEquipment : panel=%s index %u out of section (%u to %u) !", name_.CString(), index, startSlotIndex_, endSlotIndex2_);
        return;
    }

    Player* player = static_cast<Player*>(user_);
    equipment_->UpdateCharacter(index, player && player->IsMainController());
    equipment_->UpdateAttributes(index);

    UpdateSlot(index, true);
}

/// use the character Maps of the animatedSprite
/// to get the animation Equipment Slots and set the layout

bool UIC_EquipmentPanel::UpdateSlotPositions(bool force)
{
    if (!slotZone_)
        return false;

    AnimationEquipment& animationEquipment = equipment_->GetAnimationEquipment();

    if (!animationEquipment.HasSlots())
    {
        HideUISlots();
        return false;
    }

    if (!force && lastchange_ == animationEquipment.lastchange_)
        return true;

//    URHO3D_LOGINFOF("UIC_EquipmentPanel() - UpdateSlotPositions ... node=%s", holderNode_->GetName().CString());

    if (inventory_)
    {
        Sprite2D* sprite = Sprite2D::LoadFromResourceRef(GetContext(), ResourceRef(SpriteSheet2D::GetTypeStatic(), "UI/game_equipment.xml@slot"));

        for (unsigned i = startSlotIndex_; i <= endSlotIndex2_; i++)
        {
            const StringHash& slotHashName = inventory_->GetSlotHashName(i);

            if (!slotHashName)
                continue;

            const String& slotName = inventory_->GetSlotName(i);

            if (slotName.StartsWith("Special"))
                continue;

            UIElement* uiSlot = static_cast<UIElement*>(wearablesZone_->GetChild(slotName, true));

            if (!animationEquipment.HasSlot(slotHashName))
            {
                if (uiSlot)
                    uiSlot->SetVisible(false);

                continue;
            }

            if (!uiSlot)
            {
                BorderImage* slotsprite = wearablesZone_->CreateChild<BorderImage>(slotName);
                uiSlot = slotsprite;

                if (!uiSlot)
                    continue;

                slotsprite->SetTexture(GameContext::Get().textures_[UIEQUIPMENT]);
                slotsprite->SetImageRect(sprite->GetRectangle());
                slotsprite->SetSize(IntVector2(slotSize_, slotSize_));
                slotsprite->SetOpacity(0.99f);
                slotsprite->SetAlignment(HA_CENTER, VA_CENTER);
            }

            int x = (int)(animationEquipment.GetPositionForSlot(slotHashName).x_ * ((float)wearablesZone_->GetSize().x_ - (float)slotSize_/* *GameContext::Get().uiScale_*/) * 0.5f);
            int y = (int)(animationEquipment.GetPositionForSlot(slotHashName).y_ * ((float)wearablesZone_->GetSize().y_ - (float)slotSize_/* *GameContext::Get().uiScale_*/) * 0.5f);

            uiSlot->SetPosition(x, y);
            uiSlot->SetVisible(true);
        }
    }

    lastchange_ = animationEquipment.lastchange_;

    URHO3D_LOGINFOF("UIC_EquipmentPanel() - UpdateSlotPositions ... OK !");

    return true;
}

void UIC_EquipmentPanel::HideUISlots()
{
    PODVector<UIElement*> uislots;
    wearablesZone_->GetChildren(uislots);

    for (unsigned i=0; i < uislots.Size(); i++)
        uislots[i]->SetVisible(false);

    URHO3D_LOGINFOF("UIC_EquipmentPanel() - HideUISlots ... OK !");
}

void UIC_EquipmentPanel::Update()
{
    if (!holderNode_)
        return;

    URHO3D_LOGINFOF("UIC_EquipmentPanel() - Update ... ");

    if (UpdateSlotPositions())
        UpdateSlots(true);

    URHO3D_LOGINFOF("UIC_EquipmentPanel() - Update ... OK !");
}

void UIC_EquipmentPanel::OnEquipmentUpdate(StringHash eventType, VariantMap& eventData)
{
    Update();
}
