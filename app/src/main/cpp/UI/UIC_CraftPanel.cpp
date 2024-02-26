#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

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

#include "TimerInformer.h"

#include "Actor.h"
#include "GOC_Inventory.h"
#include "GOC_Collectable.h"

#include "UISlotPanel.h"

#include "UIC_CraftPanel.h"



UIC_CraftPanel::UIC_CraftPanel(Context* context) :
    UISlotPanel(context)
{
    craft_ = new GOC_Inventory(context);
    craft_->SetTemplate("InventoryTemplate_Craft");
    SetInventorySection("Material", "Tool");

    craftTimer_ = new DelayInformer(context);
}

UIC_CraftPanel::~UIC_CraftPanel()
{
    if (craft_)
        delete craft_;

    if (craftTimer_)
        delete craftTimer_;
}

void UIC_CraftPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_CraftPanel>();
}

void UIC_CraftPanel::Start(Object* user, Object* feeder)
{
    UISlotPanel::Start(user, feeder);

    UpdateNodes();
    UpdateCraftedItem(true);

    SubscribeToEvent(GetElement()->GetChild("MakeButton", true), E_RELEASED, URHO3D_HANDLER(UIC_CraftPanel, HandleMake));
}

void UIC_CraftPanel::SetSlotZone()
{
    slotSize_ = 64;

    incselector_[ACTION_LEFT] = -1;
    incselector_[ACTION_RIGHT] = 1;
    incselector_[ACTION_UP] = -1;
    incselector_[ACTION_DOWN] = 1;

    slotZone_ = panel_->GetChild("SlotZone", true);
    if (!slotZone_)
    {
        URHO3D_LOGERRORF("UIC_CraftPanel() - SetSlotZone : name=%s : Can't get SlotZone Element !!!", name_.CString());
        return;
    }

    Actor* actor = ((Actor*)user_);
    if (actor)
    {
        slotZone_->SetVar(GOA::OWNERID, actor->GetID());
        slotZone_->SetVar(GOA::PANELID, name_);
    }

    inventory_ = craft_;
    if (inventorySectionStart_ != String::EMPTY)
    {
        startSlotIndex_ = inventory_->GetSectionStartIndex(inventorySectionStart_);
        endSlotIndex1_ = inventory_->GetSectionEndIndex(inventorySectionStart_);
        endSlotIndex2_ = inventory_->GetSectionEndIndex(inventorySectionEnd_);
        numSlots_ = endSlotIndex2_ - startSlotIndex_ + 1;
        slotselector_ = startSlotIndex_;

        URHO3D_LOGERRORF("UIC_CraftPanel() - SetSlotZone : startSlotIndex=%u endSlotIndex=%u", startSlotIndex_, endSlotIndex2_);
    }
}

void UIC_CraftPanel::UpdateSlot(unsigned index, bool updateButtons, bool updateSubscribers, bool updateNet)
{
//    URHO3D_LOGINFOF("UIC_CraftPanel() - UpdateSlot : idSlot=%u inventory=%u", index, inventory_);

    if (!inventory_)
    {
        URHO3D_LOGERRORF("UIC_CraftPanel() - UpdateSlot : idSlot=%u no inventory !", index);
        return;
    }

    const Slot& slot = inventory_->GetSlot(index);
    String slotname(index < 4 ? String("Material_") + String(index+1) : String("Tool_") + String(index-4+1));

    UIElement* uiSlot = static_cast<UIElement*>(slotZone_->GetChild(slotname, true));

    if (!uiSlot)
    {
        URHO3D_LOGWARNINGF("UIC_CraftPanel() - UpdateSlot: idSlot=%u can't find uielement=%s !", index, slotname.CString());
        return;
    }

//    if (!slot.quantity_)
//    {
//        URHO3D_LOGWARNINGF("UIC_CraftPanel() - UpdateSlot: idSlot=%u qty=0 clear uislot !", index);
//        uiSlot->RemoveAllChildren();
//        return;
//    }

    Button* button = static_cast<Button*>(uiSlot->GetChild(0));
    if (!button)
    {
        button = uiSlot->CreateChild<Button>();
        updateButtons = true;
    }

    if (updateButtons)
    {
        URHO3D_LOGWARNINGF("UIC_CraftPanel() - UpdateSlot: idSlot=%u slotname=%s uispritename=%s(%u) !", index, slotname.CString(), slot.uisprite_ ? slot.uisprite_->GetName().CString() : "",  slot.uisprite_.Get());

        IntVector2 size(slotSize_*GameContext::Get().uiScale_, slotSize_*GameContext::Get().uiScale_);
        if (slot.uisprite_)
        {
            button->SetTexture(slot.uisprite_->GetTexture());
//            button->SetEnableDpiScale(false);
            button->SetImageRect(slot.uisprite_->GetRectangle());
            size = slot.uisprite_->GetRectangle().Size();
            size.x_ = (int)((float)size.x_ * GameContext::Get().uiScale_);
            size.y_ = (int)((float)size.y_ * GameContext::Get().uiScale_);
            GameHelpers::ClampSizeTo(size, slotSize_*GameContext::Get().uiScale_);
        }
        else
        {
            // Default transparent
            button->SetTexture(GameContext::Get().textures_[UIEQUIPMENT]);
//            button->SetEnableDpiScale(false);
            button->SetImageRect(IntRect(0, 0, 1, 1));
            URHO3D_LOGWARNINGF("UIC_CraftPanel() - UpdateSlot: idSlot=%u no Sprite in slot !", index);
        }

        button->SetSize(size);
        button->SetDragDropMode(DD_SOURCE_AND_TARGET);
        button->SetHoverOffset(0, 0);
        button->SetAlignment(HA_CENTER, VA_CENTER);
    }

    if (updateSubscribers)
    {
        SubscribeToEvent(button, E_DRAGBEGIN, URHO3D_HANDLER(UIC_CraftPanel, HandleSlotDragBegin));
        SubscribeToEvent(button, E_DRAGMOVE, URHO3D_HANDLER(UIC_CraftPanel, HandleSlotDragMove));
        SubscribeToEvent(button, E_DRAGEND, URHO3D_HANDLER(UIC_CraftPanel, HandleSlotDragEnd));
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
}

void UIC_CraftPanel::UpdateSlots(bool updateButtons, bool updateSubscribers)
{
    if (!inventory_)
    {
        URHO3D_LOGERRORF("UIC_CraftPanel() - UpdateSlots : inventory_=0 ! ");
        return;
    }

    for (unsigned i=0; i < 4; ++i)
        UpdateSlot(i, updateButtons, updateSubscribers);
    for (unsigned i=4; i < 6; ++i)
        UpdateSlot(i, updateButtons, false);
}

void UIC_CraftPanel::UpdateCraftedItem(bool clear)
{
    UnsubscribeFromEvent(this, GO_TIMERINFORMER);

    // If not empty, transfer the crafted Item to actor Inventory
    if (craftedItem_.quantity_)
    {
        Node* avatar = ((Actor*)user_)->GetAvatar();
        GOC_Collectable::TransferSlotTo(craftedItem_, 0, avatar, Variant(avatar->GetWorldPosition()));

        if (craftedItem_.quantity_)
        {
            URHO3D_LOGINFOF("UIC_CraftPanel() - No More Slot in Avatar => Drop %s qty=%u to scene",
                            GOT::GetType(craftedItem_.type_).CString(), craftedItem_.quantity_);

            const int dropmode = !GameContext::Get().LocalMode_ ? SLOT_DROPREMAIN : SLOT_NONE;
            Node* node = GOC_Collectable::DropSlotFrom(avatar, &craftedItem_, dropmode);
        }
    }

    // Clear Crafted Item
    if (clear)
        craftedItem_.Clear();

    // Update ui slot
    UIElement* uiCraftedItem = GetElement()->GetChild(String("CraftedItem"), true);
    if (uiCraftedItem)
    {
        IntVector2 size;

        Button* button = static_cast<Button*>(uiCraftedItem->GetChild(0));

        if (!button)
            button = uiCraftedItem->CreateChild<Button>();

        if (!button)
            return;

        if (craftedItem_.sprite_)
        {
            button->SetTexture(craftedItem_.sprite_->GetTexture());
//            button->SetEnableDpiScale(false);
            button->SetImageRect(craftedItem_.sprite_->GetRectangle());
            size = craftedItem_.sprite_->GetRectangle().Size();
            size.x_ = (int)((float)size.x_ * GameContext::Get().uiScale_);
            size.y_ = (int)((float)size.y_ * GameContext::Get().uiScale_);
            GameHelpers::ClampSizeTo(size, uiCraftedItem->GetSize().x_ * GameContext::Get().uiScale_);
        }
        else
        {
            // Default transparent
            button->SetTexture(GameContext::Get().textures_[UIEQUIPMENT]);
//            button->SetEnableDpiScale(false);
            button->SetImageRect(IntRect(0, 0, 1, 1));
        }

        if (craftedItem_.effect_)
        {
            BorderImage* effect = static_cast<BorderImage*>(button->GetChild(0));
            if (!effect)
            {
                effect = button->CreateChild<BorderImage>();
                effect->SetAlignment(HA_RIGHT, VA_TOP);
                effect->SetOpacity(0.99f);
                effect->SetVisible(true);
            }

            if (craftedItem_.effect_ > -1)
                GameHelpers::SetUIElementFrom(effect, UIEQUIPMENT, String("effect") + String(craftedItem_.effect_), miniSlotSize_);

            effect->SetVisible(craftedItem_.effect_ > -1);
        }
        button->SetSize(size);
        button->SetAlignment(HA_CENTER, VA_CENTER);

        uiCraftedItem->SetVisible(craftedItem_.type_);
    }

    UIElement* uiTextEqual = static_cast<UIElement*>(GetElement()->GetChild(String("Equal"), true));
    if (uiTextEqual)
    {
        uiTextEqual->SetVisible(uiCraftedItem && craftedItem_.type_);
    }

    // Reset the Progress Bar
    UIElement* uiProgressBar = static_cast<UIElement*>(GetElement()->GetChild(String("ProgressBar"), true));
    if (uiProgressBar)
    {
        uiProgressBar->RemoveObjectAnimation();
        uiProgressBar->SetSize(uiProgressBar->GetMinSize());
    }
}


void UIC_CraftPanel::CheckAvailableTools()
{
    URHO3D_LOGINFOF("UIC_CraftPanel() - CheckAvailableTools : ... ");

    availableTools_.Clear();

    Node* avatar = ((Actor*)user_)->GetAvatar();

    // Check for the external tools
    Vector2 pos = avatar->GetWorldPosition2D();
    Rect aabb(pos - Vector2(2.f, 1.f), pos + Vector2(2.f, 1.f));
    PODVector<RigidBody2D*> results;
    GameContext::Get().physicsWorld_->GetRigidBodies(results, aabb);

    for (unsigned i=0; i < results.Size(); i++)
    {
        Node* node = results[i]->GetNode();

        StringHash got(node->GetVar(GOA::GOT).GetUInt());
        if (got && (GOT::GetTypeProperties(got) & GOT_Tool))
        {
            if (!availableTools_.Contains(got))
            {
                URHO3D_LOGINFOF("UIC_CraftPanel() - CheckAvailableTools : node=%s(%u) is an external tool ...", node->GetName().CString(), node->GetID());
                availableTools_.Push(got);
            }
        }
    }

    // Check for the portable tools
    GOC_Inventory* userinv = avatar->GetComponent<GOC_Inventory>();
    if (userinv)
    {
        const Vector<Slot>& slots = userinv->Get();
        for (unsigned i=0; i < slots.Size(); i++)
        {
            StringHash got(slots[i].type_);
            if (got && (GOT::GetTypeProperties(got) & GOT_Tool))
            {
                if (!availableTools_.Contains(got))
                {
                    URHO3D_LOGINFOF("UIC_CraftPanel() - CheckAvailableTools : node=%s is a portable tool ...", GOT::GetType(got).CString());
                    availableTools_.Push(got);
                }
            }
        }
    }
}

void UIC_CraftPanel::SetTool(unsigned islot, const StringHash& type)
{
    Slot& slot = inventory_->GetSlot(islot+3);
    slot.Clear();

    if (type)
    {
        slot.Set(type, 1);
        slot.Dump();
    }

    UpdateSlot(islot+3, true);
}

void UIC_CraftPanel::HandleMake(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_CraftPanel() - HandleMake : panel=%s Player=%u Actor=%u ...", panel_->GetName().CString(),
                    user_ ?  ((Actor*)user_)->GetID() : 0, feeder_ ? ((Actor*)feeder_)->GetID() : 0);

    // TODO : remove this test and replace the button by a dropdownlist for each tool slot
    CheckAvailableTools();
    SetTool(1, availableTools_.Size() ? availableTools_[0] : StringHash::ZERO);

    // Compose the recipe with the name of available materials and tools in inventory

    Vector<String> materials;
    for (unsigned i=0; i < 4; i++)
    {
        const Slot& slot = inventory_->GetSlot(i);
        if (slot.quantity_)
        {
            materials.Push(GOT::GetType(slot.type_));
            URHO3D_LOGINFOF("UIC_CraftPanel() - HandleMake : recipe material[%u]=%s ...", i, materials.Back().CString());
        }
    }

    Vector<String> tools;
    for (unsigned i=4; i < 6; i++)
    {
        const Slot& slot = inventory_->GetSlot(i);
        if (slot.quantity_)
        {
            tools.Push(GOT::GetType(slot.type_));
            URHO3D_LOGINFOF("UIC_CraftPanel() - HandleMake : recipe tool[%u]=%s ...", i, tools.Back().CString());
        }
    }

    // Find a recipe for the combined materials and tools (elements)

    const String& sortedelements = CraftRecipes::GetSortedElementsName(materials, tools);
//    URHO3D_LOGINFOF("UIC_CraftPanel() - HandleMake : check for sortedelements=%s(%u) ...", sortedelements.CString(), StringHash(sortedelements).Value());

    const String& recipename = CraftRecipes::GetRecipeForElements(sortedelements);
    if (recipename.Empty())
    {
        URHO3D_LOGINFOF("UIC_CraftPanel() - HandleMake : recipe not found !", recipename.CString());
        return;
    }

    URHO3D_LOGINFOF("UIC_CraftPanel() - HandleMake : recipe found = %s", recipename.CString());

    // add to the item slot (set sprite and effect if template node has it)
    craftedItem_.Set(StringHash(recipename));

    // update the item slot button
    UpdateCraftedItem();

    // add craft timer
    const float craftdelay = 5.f;
    craftTimer_->Start(this, craftdelay+0.25f, GO_TIMERINFORMER);
    SubscribeToEvent(this, GO_TIMERINFORMER, URHO3D_HANDLER(UIC_CraftPanel, HandleCraftItemMade));

    // set progress bar
    UIElement* uiProgressBar = static_cast<UIElement*>(GetElement()->GetChild(String("ProgressBar"), true));
    if (uiProgressBar)
    {
        uiProgressBar->SetSize(uiProgressBar->GetMinSize());
        uiProgressBar->RemoveObjectAnimation();

        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> sizeAnimation(new ValueAnimation(context_));
        sizeAnimation->SetKeyFrame(0.f, uiProgressBar->GetMinSize());
        sizeAnimation->SetKeyFrame(craftdelay, uiProgressBar->GetMaxSize());
        objectAnimation->AddAttributeAnimation("Size", sizeAnimation, WM_ONCE);
        uiProgressBar->SetObjectAnimation(objectAnimation);
    }
}

void UIC_CraftPanel::HandleCraftItemMade(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_CraftPanel() - HandleCraftItemMade !");

    // Put the crafed item in the user inventory
    craftedItem_.quantity_ = 1;
    UpdateCraftedItem();

    // Remove the used materials
    for (unsigned i=0; i < 4; i++)
        inventory_->RemoveCollectableFromSlot(i, 1);

    UpdateSlots(true, true);
}


void UIC_CraftPanel::OnRelease()
{
    URHO3D_LOGINFOF("UIC_CraftPanel() - OnRelease : panel=%s Player=%u Actor=%u", panel_->GetName().CString(),
                    user_ ?  ((Actor*)user_)->GetID() : 0, feeder_ ? ((Actor*)feeder_)->GetID() : 0);

    // Reset the crafted item slot
    UpdateCraftedItem(true);

    Node* avatar = ((Actor*)user_)->GetAvatar();
    GOC_Inventory* userinv = avatar->GetComponent<GOC_Inventory>();

    // Restore remaining materials to Actor
    for (unsigned i=0; i < 4; i++)
    {
        if (!inventory_->GetSlot(i).quantity_)
            continue;

        inventory_->TransferSlotTo(i, userinv);
    }

    // Reset inventory
    inventory_->ResetSlots();
    UpdateSlots(true, true);
}


