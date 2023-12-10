#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Graphics.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>

#include "DefsMap.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Animator2D.h"
#include "GOC_Collectable.h"
#include "GOC_Inventory.h"
#include "GOC_ZoneEffect.h"

#include "Equipment.h"
#include "Actor.h"
#include "TimerRemover.h"
#include "ViewManager.h"

#include "UIC_BagPanel.h"
#include "UIC_EquipmentPanel.h"
#include "UIC_CraftPanel.h"
#include "UIC_ShopPanel.h"

#include "UISlotPanel.h"


/// UIPanel

HashMap<String, SharedPtr<UIPanel> > UIPanel::panels_;
SharedPtr<UIElement> UIPanel::draggedElement_;
bool UIPanel::onKeyDrag_ = false;
bool UIPanel::allowSceneInteraction_ = false;

UIPanel::UIPanel(Context* context) :
    Object(context),
    feeder_(0)
{ }

UIPanel::~UIPanel()
{
    Stop();

    URHO3D_LOGINFOF("~UIPanel() - panel=%s ...", name_.CString());

    if (panel_)
    {
        panel_->GetParent()->RemoveChild(panel_);
        panel_.Reset();
    }

    URHO3D_LOGINFOF("~UIPanel() - ... OK !");
}

void UIPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIPanel>();

    panels_.Clear();
}

UIPanel* UIPanel::GetPanel(const String& name)
{
    HashMap<String, SharedPtr<UIPanel> >::ConstIterator it = panels_.Find(name);
    return it != panels_.End() ? it->second_.Get() : 0;
}

void UIPanel::RegisterPanel(UIPanel* panel)
{
    if (panels_.Contains(panel->name_))
    {
        URHO3D_LOGERRORF("UIPanel() - RegisterPanel id=%s : already registered !!!", panel->name_.CString());
        return;
    }

    URHO3D_LOGINFOF("UIPanel() - RegisterPanel id=%s !", panel->name_.CString());
    panels_[panel->name_] = panel;
}

void UIPanel::RemovePanel(UIPanel* panel)
{
    URHO3D_LOGINFOF("UIPanel() - RemovePanel id=%s !", panel->name_.CString());
    if (panels_.Contains(panel->name_))
        panels_.Erase(panel->name_);
}

void UIPanel::ClearRegisteredPanels()
{
    draggedElement_.Reset();
    panels_.Clear();
}

void UIPanel::SetPosition(int x, int y, HorizontalAlignment hAlign, VerticalAlignment vAlign)
{
    SetPosition(IntVector2(x, y), hAlign, vAlign);
}

void UIPanel::SetPosition(const IntVector2& position, HorizontalAlignment hAlign, VerticalAlignment vAlign)
{
    lastPosition_ = position;
    GetElement()->SetHorizontalAlignment(hAlign);
    GetElement()->SetVerticalAlignment(vAlign);
    GetElement()->SetPosition(lastPosition_);
}

bool UIPanel::Set(int id, const String& name, const String& layout, const IntVector2& position, HorizontalAlignment hAlign, VerticalAlignment vAlign, float alpha, UIElement* parent)
{
    URHO3D_LOGINFOF("UIPanel() - Set panel=%s ...", name.CString());

    id_ = id;
    name_ = name;
    panel_ = GameHelpers::AddUIElement(layout, position, hAlign, vAlign, alpha, parent);

    lastPosition_ = position;

    if (panel_)
        RegisterPanel(this);

    return panel_ != 0;
}

void UIPanel::Start(Object* user, Object* feeder)
{
    user_ = user;
    feeder_ = feeder;

    Button* closeButton = dynamic_cast<Button*>(panel_->GetChild("OkButton", true));
    if (closeButton)
        SubscribeToEvent(closeButton, E_RELEASED, URHO3D_HANDLER(UIPanel, OnSwitchVisible));

//    URHO3D_LOGINFOF("UIPanel() - Start panel=%s : user_=%u feeder=%u", panel_->GetName().CString(), user_, feeder);
}

void UIPanel::Reset()
{
    ;
}

void UIPanel::Stop()
{
    UnsubscribeFromAllEvents();
}

void UIPanel::GainFocus()
{
    if (GetElement())
        GetElement()->SetFocus(true);

    OnSetVisible();

    GameContext::Get().ui_->SetHandleJoystickEnable(false);
}

void UIPanel::LoseFocus()
{
    if (GetElement())
        GetElement()->SetFocus(false);

    OnSetVisible();

    GameContext::Get().ui_->SetHandleJoystickEnable(true);
}

void UIPanel::SetVisible(bool state, bool saveposition)
{
    URHO3D_LOGINFOF("UIPanel() - SetVisible : panel=%s ... %s !", name_.CString(), state ? "show":"hide");

    panel_->SetVisible(state);

    OnSetVisible();
}

void UIPanel::ToggleVisible()
{
    if (!panel_->IsVisible())
    {
        SetVisible(true);

        UIElement* frontelement = GetSubsystem<UI>()->GetFrontElement();
        if (frontelement && frontelement != panel_)
            panel_->SetPriority(frontelement->GetPriority()+1);

        GameHelpers::ClampPositionToScreen(lastPosition_);
        IntVector2 exitpoint = GameHelpers::GetUIExitPoint(panel_, lastPosition_);
        GameHelpers::SetMoveAnimationUI(panel_, exitpoint, lastPosition_, 0.f, SWITCHSCREENTIME);
        TimerRemover::Get()->Start(panel_, SWITCHSCREENTIME + 0.05f, ENABLE);

        panel_->SetBringToFront(true);
        panel_->BringToFront();

//        URHO3D_LOGINFOF("UIPanel() - ToggleVisible : panel=%s ... Visible !", name_.CString());
    }
    else if (panel_->IsEnabled())
    {
        lastPosition_ = panel_->GetPosition();
        panel_->SetEnabled(false);

        IntVector2 exitpoint = GameHelpers::GetUIExitPoint(panel_, lastPosition_);
        GameHelpers::SetMoveAnimationUI(panel_, lastPosition_, exitpoint, 0.f, SWITCHSCREENTIME);
        TimerRemover::Get()->Start(panel_, SWITCHSCREENTIME + 0.05f, DISABLE);

        OnRelease();

//        URHO3D_LOGINFOF("UIPanel() - ToggleVisible : panel=%s ... Not Visible !", name_.CString());
    }
}

bool UIPanel::IsVisible() const
{
    return panel_->IsVisible();
}

bool UIPanel::CanFocus() const
{
    return GetElement()->GetFocusMode() == FM_FOCUSABLE && !GetElement()->HasFocus();
}

const float PanelJoystickSensivity_ = 0.98f;
int UIPanel::GetKeyFromEvent(int controlid, StringHash eventType, VariantMap& eventData) const
{
    int scancode = 0;
    const PODVector<int>& keymap = GameContext::Get().keysMap_[controlid];

    if (eventType == E_JOYSTICKBUTTONDOWN || eventType == E_JOYSTICKAXISMOVE || eventType == E_JOYSTICKHATMOVE)
    {
        const PODVector<int>& buttonmap = GameContext::Get().buttonsMap_[controlid];
        JoystickState* joystick = GameContext::Get().input_->GetJoystickByIndex(GameContext::Get().joystickIndexes_[controlid]);
        int joyid = joystick->joystickID_;

        if (eventType == E_JOYSTICKBUTTONDOWN && eventData[JoystickButtonDown::P_JOYSTICKID].GetInt() == joyid)
        {
            if (eventData[JoystickButtonDown::P_BUTTON].GetInt() == buttonmap[ACTION_INTERACT])
            {
                scancode = keymap[ACTION_INTERACT];

                if (!allowSceneInteraction_ || buttonmap[ACTION_INTERACT] == buttonmap[ACTION_JUMP])
                    joystick->Reset();
            }
        }
        else if (eventType == E_JOYSTICKHATMOVE && eventData[JoystickHatMove::P_JOYSTICKID].GetInt() == joyid)
        {
            int axis = eventData[JoystickHatMove::P_HAT].GetInt();
            int position = eventData[JoystickHatMove::P_POSITION].GetInt();
            {
                if (position == HAT_UP)
                    scancode = keymap[ACTION_UP];
                else if (position == HAT_DOWN)
                    scancode = keymap[ACTION_DOWN];
                else if (position == HAT_LEFT)
                    scancode = keymap[ACTION_LEFT];
                else if (position == HAT_RIGHT)
                    scancode = keymap[ACTION_RIGHT];
            }
            if (!allowSceneInteraction_)
                joystick->Reset();
        }
        else if (!allowSceneInteraction_ && eventType == E_JOYSTICKAXISMOVE && eventData[JoystickAxisMove::P_JOYSTICKID].GetInt() == joyid)
        {
            int axis = eventData[JoystickAxisMove::P_AXIS].GetInt();
            float position = eventData[JoystickAxisMove::P_POSITION].GetFloat();
            if (position > PanelJoystickSensivity_)
                scancode = keymap[axis == 0 ? ACTION_RIGHT : ACTION_DOWN];
            else if (position < -PanelJoystickSensivity_)
                scancode = keymap[axis == 0 ? ACTION_LEFT : ACTION_UP];
            joystick->Reset();
        }
        if (scancode)
            URHO3D_LOGINFOF("UIPanel() - GetKeyFromEvent : controlid=%d joystickid=%d scancode=%d ...", controlid, joyid, scancode);
    }
    else if (eventType == E_KEYDOWN)
    {
        scancode = eventData[KeyDown::P_SCANCODE].GetInt();
    }

    return scancode;
}

void UIPanel::OnSetVisible()
{
    if (panel_->IsVisible())
    {
        SubscribeToEvent(panel_, E_DRAGBEGIN, URHO3D_HANDLER(UIPanel, OnDrag));
        SubscribeToEvent(panel_, E_DRAGEND, URHO3D_HANDLER(UIPanel, OnDrag));
        SubscribeToEvent(panel_, E_FOCUSED, URHO3D_HANDLER(UIPanel, OnFocusChange));
        SubscribeToEvent(panel_, E_DEFOCUSED, URHO3D_HANDLER(UIPanel, OnFocusChange));

//        URHO3D_LOGINFOF("UIPanel() - OnSetVisible : %s pos=%s size=%s", name_.CString(), GetElement()->GetPosition().ToString().CString(), GetElement()->GetSize().ToString().CString());
    }
    else
    {
        UnsubscribeFromEvent(panel_, E_DRAGBEGIN);
        UnsubscribeFromEvent(panel_, E_DRAGEND);
        UnsubscribeFromEvent(panel_, E_FOCUSED);
        UnsubscribeFromEvent(panel_, E_DEFOCUSED);

//        URHO3D_LOGINFOF("UIPanel() - OnSetVisible : %s novisible", name_.CString());

//        panel_->SetFocus(false);
    }
}

void UIPanel::OnSwitchVisible(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("UIPanel() - OnSwitchVisible : %s become %s", name_.CString(), panel_->IsVisible() ? "hide":"show");
    ToggleVisible();
}

void UIPanel::OnFocusChange(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("UIPanel() - OnFocusChange : %s hasfocus=%s ... !", panel_->GetName().CString(), panel_->HasFocus() ? "true":"false");
    Text* titletext = panel_->GetChildStaticCast<Text>(String("TitleText"), true);
    if (titletext)
    {
        titletext->SetColor(panel_->HasFocus() ? Color::YELLOW : Color::WHITE);
    }
}

void UIPanel::OnDrag(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_DRAGEND)
    {
        lastPosition_ = panel_->GetPosition();
    }
}

void UIPanel::DebugDraw()
{
    UI* ui = GetSubsystem<UI>();
    ui->DebugDraw(panel_);

    PODVector<UIElement*> children;
    panel_->GetChildren(children, true);
    for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
        ui->DebugDraw(*it);
}



/// UISlotPanel

WeakPtr<UIElement> UISlotPanel::selectSlotByKey_;
WeakPtr<UIElement> UISlotPanel::beginDragElementByKey_;
UISlotPanel* UISlotPanel::fromPanelDrag_;

UISlotPanel::UISlotPanel(Context* context) :
    UIPanel(context),
    slotZone_(0),
    selectHalo_(0),
    startSlotIndex_(0),
    endSlotIndex1_(0),
    endSlotIndex2_(0),
    slotselector_(0),
    numSlots_(0),
    inventory_(0)
{ }

UISlotPanel::~UISlotPanel()
{ }

void UISlotPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UISlotPanel>();
}

void UISlotPanel::Start(Object* user, Object* feeder)
{
    URHO3D_LOGINFOF("%s() - Start ...", GetTypeName().CString());

    UIPanel::Start(user, feeder);

    if (feeder_ && feeder_->IsInstanceOf<Equipment>())
    {
        equipment_ = dynamic_cast<Equipment*>(feeder_);
    }
    else
        equipment_ = 0;

    if (user_)
    {
        SubscribeToEvent(user_, GO_NODESCHANGED, URHO3D_HANDLER(UISlotPanel, OnNodesChange));
        SubscribeToEvent(user_, PANEL_SLOTUPDATED, URHO3D_HANDLER(UISlotPanel, OnSlotUpdate));
    }
    else
        URHO3D_LOGWARNINGF("UISlotPanel() - Start panel=%s : no user !", name_.CString());
}

void UISlotPanel::SetInventorySection(const String& sectionstart, const String& sectionend)
{
    inventorySectionStart_ = sectionstart;
    inventorySectionEnd_ = sectionend.Empty() ? inventorySectionStart_ : sectionend;
}

void UISlotPanel::Reset()
{
    UIPanel::Reset();
}

void UISlotPanel::SetSlotZone()
{
//    font_ = GetSubsystem<ResourceCache>()->GetResource<Font>(GameContext::Get().txtMsgFont_);
    slotSize_ = 80;
    miniSlotSize_ = 20;

    incselector_[ACTION_LEFT] = -1;
    incselector_[ACTION_RIGHT] = 1;
    incselector_[ACTION_UP] = -5;
    incselector_[ACTION_DOWN] = 5;

    slotZone_ = panel_->GetChild("SlotZone", true);
    if (!slotZone_)
    {
        URHO3D_LOGERRORF("UISlotPanel() - Start name=%s : Can't get SlotZone Element !!!", name_.CString());
        return;
    }

    Actor* actor = static_cast<Actor*>(user_);
    if (actor)
    {
        slotZone_->SetVar(GOA::OWNERID, actor->GetID());
        slotZone_->SetVar(GOA::PANELID, name_);

        inventory_ = actor->GetAvatar() ? actor->GetAvatar()->GetComponent<GOC_Inventory>() : 0;

        if (inventory_)
        {
            if (inventorySectionStart_ != String::EMPTY)
            {
                startSlotIndex_ = inventory_->GetSectionStartIndex(inventorySectionStart_);
                endSlotIndex1_ = inventory_->GetSectionEndIndex(inventorySectionStart_);
                endSlotIndex2_ = inventory_->GetSectionEndIndex(inventorySectionEnd_);
                numSlots_ = endSlotIndex2_ - startSlotIndex_ + 1;
                slotselector_ = startSlotIndex_;

                URHO3D_LOGERRORF("%s() - SetSlotZone : startSlotIndex=%u endSlotIndex=%u", GetTypeName().CString(), startSlotIndex_, endSlotIndex2_);
            }
        }
    }
    else
    {
        inventory_ = 0;
    }
}

void UISlotPanel::UpdateNodes()
{
    Node* holdernode = 0;

    if (equipment_)
        holdernode = equipment_->GetHolderNode();
    else if (feeder_)
        holdernode = static_cast<Actor*>(feeder_)->GetAvatar();

    if (!holdernode)
        return;

    holderNode_ = holdernode;
    Actor* actor = static_cast<Actor*>(user_);
    Node* actorNode = actor ? actor->GetAvatar() : 0;

//    URHO3D_LOGINFOF("UISlotPanel() - UpdateNodes panel=%s ... actorNode=%s(%u) holderNode=%s(%u) ...", name_.CString(),
//                    actorNode ? actorNode->GetName().CString() : "", actorNode ? actorNode->GetID() : 0,
//                    holderNode_ ? holderNode_->GetName().CString(): "", holderNode_ ? holderNode_->GetID() : 0);

    if (!actorNode)
        return;

    panel_->SetVar(GOA::OWNERID, actor->GetID());
    panel_->SetVar(GOA::PANELID, name_);

    SetSlotZone();

    if (!slotZone_)
        return;

    Update();

//    URHO3D_LOGINFOF("UISlotPanel() - UpdateNodes panel=%s ... actorNode=%s(%u) holderNode=%s(%u) inventory_=%u OK !", name_.CString(),
//                    actorNode->GetName().CString(), actorNode->GetID(), holderNode_ ? holderNode_->GetName().CString(): "", holderNode_ ? holderNode_->GetID() : 0,
//                    inventory_);
}

void UISlotPanel::UpdateSlot(unsigned index, bool updateButtons, bool updateSubscribers, bool updateNet)
{
    ;
}

void UISlotPanel::UpdateSlots(bool updateButtons, bool updateSubscribers)
{
    if (!numSlots_)
        return;

//    URHO3D_LOGINFOF("UISlotPanel() - UpdateSlots panel=%s ... ", name_.CString());

    for (unsigned i=startSlotIndex_; i<=endSlotIndex2_; ++i)
        UpdateSlot(i, updateButtons, updateSubscribers);
}

void UISlotPanel::Update()
{
    if (!holderNode_ || !inventory_)
        return;

    UpdateSlots(true, true);
}

void UISlotPanel::OnSetVisible()
{
    UIPanel::OnSetVisible();

    if (panel_->IsVisible() && panel_->HasFocus())
    {
        int controltype = GameContext::Get().playerState_[static_cast<Actor*>(user_)->GetControlID()].controltype;
        if (controltype == CT_KEYBOARD)
        {
            SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(UISlotPanel, OnKey));
        }
        else if (controltype == CT_JOYSTICK)
        {
            SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(UISlotPanel, OnKey));
            SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(UISlotPanel, OnKey));
            SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(UISlotPanel, OnKey));
        }

        selectHalo_ = panel_->GetChild(String("I_Select"), true);
        UpdateSlotSelector();

//        URHO3D_LOGINFOF("UISlotPanel() - OnSetVisible : panel=%s visible and focused !", name_.CString());
    }
    else
    {
        if (selectHalo_)
            selectHalo_->SetVisible(false);

        if (HasSubscribedToEvent(E_KEYDOWN))
            UnsubscribeFromEvent(E_KEYDOWN);

        if (HasSubscribedToEvent(E_JOYSTICKBUTTONDOWN))
        {
            UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
            UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
            UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
        }

//        URHO3D_LOGINFOF("UISlotPanel() - OnSetVisible : panel=%s hide selecthalo !", name_.CString());
    }
}

void UISlotPanel::OnSlotUpdate(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UISlotPanel() - OnSlotUpdate : panel=%s ...", name_.CString());

    unsigned index = eventData[Go_InventoryGet::GO_IDSLOT].GetUInt();

    if (index < startSlotIndex_ || index > endSlotIndex2_)
    {
        URHO3D_LOGWARNINGF("UISlotPanel() - OnSlotUpdate : panel=%s index %u out of section (%u to %u) !", name_.CString(), index, startSlotIndex_, endSlotIndex2_);
        return;
    }

    UpdateSlot(index, true);
}

void UISlotPanel::OnNodesChange(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("UISlotPanel() - OnNodesChange : panel=%s ...", name_.CString());

    UpdateNodes();
}

void UISlotPanel::HandleSlotDragBegin(StringHash eventType, VariantMap& eventData)
{
    if (onKeyDrag_ && beginDragElementByKey_)
        EndKeyDrag();

//    URHO3D_LOGINFOF("UISlotPanel() - HandleSlotDragBegin");
    // Get draggedElement
    draggedElement_.Reset();

    if (onKeyDrag_ && !selectSlotByKey_)
    {
        onKeyDrag_ = false;
        return;
    }

    draggedElement_ = onKeyDrag_ ? selectSlotByKey_ : static_cast<UIElement*>(eventData["Element"].GetPtr());
    beginDragElementByKey_ = draggedElement_;

    IntVector2 dragCurrentPosition = IntVector2(eventData["X"].GetInt(), eventData["Y"].GetInt());
    dragParent_ = draggedElement_->GetParent();
    if (!dragParent_ || dragParent_->GetName().Empty())
    {
        URHO3D_LOGERRORF("UISlotPanel() - HandleSlotDragBegin : panel=%s ... no parent dragParent_=%u !", name_.CString(), dragParent_.Get());
        draggedElement_.Reset();
        return;
    }

    dragElementIndexFrom_ = dragParent_->FindChild(draggedElement_);

    /// Switch to the inventory if more than one inventory in a panel (ShopPanel case)
    SelectInventoryFrom(dragParent_->GetName());
    dragFromInventory_ = GetInventory();

    slotIndex_ = dragFromInventory_->GetSlotIndex(dragParent_->GetName());

//    URHO3D_LOGINFOF("UISlotPanel() - HandleSlotDragBegin : panel=%s uielement=%s => slotindex=%d !", name_.CString(), dragParent_->GetName().CString(), slotIndex_);

    if (slotIndex_ == -1)
    {
        URHO3D_LOGERRORF("UISlotPanel() - HandleSlotDragBegin : panel=%s ... not find slot name %s in inventory template !", name_.CString(), dragParent_->GetName().CString());
        draggedElement_.Reset();
        return;
    }

    if (!dragFromInventory_->GetSlot(slotIndex_).quantity_)
    {
        URHO3D_LOGERRORF("UISlotPanel() - HandleSlotDragBegin : panel=%s ... try to get an empty slot !", name_.CString());
        draggedElement_.Reset();
        return;
    }

    GameContext::Get().gameConfig_.autoHideCursorEnable_ = false;

    // remove draggedElement_
    dragParent_->RemoveChildAtIndex(dragElementIndexFrom_);

    GetSubsystem<UI>()->GetRoot()->AddChild(draggedElement_);
    draggedElement_->SetAlignment(HA_LEFT, VA_TOP);
    if (onKeyDrag_)
        draggedElement_->SetPosition(dragCurrentPosition-IntVector2(12, 12));
    else
        draggedElement_->SetPivot(Vector2(0.5f, 0.5f));

    // Related BUG : the inventory panel was over draggedelement_ in a nextlevel while the draggedelement was brought to front with BringToFront
    // resolve bug by using priority
    draggedElement_->SetPriority(GetSubsystem<UI>()->GetFrontElement()->GetPriority()+1);
    draggedElement_->SetOpacity(0.95f);

//    URHO3D_LOGINFOF("UISlotPanel() - HandleSlotDragBegin : name=%s slotIndex=%u at position=%d,%d", dragParent_->GetName().CString(), slotIndex_, dragCurrentPosition.x_, dragCurrentPosition.y_);
}

void UISlotPanel::HandleSlotDragMove(StringHash eventType, VariantMap& eventData)
{
    if (!draggedElement_)
        return;

//    LOGINFOF("UISlotPanel() - HandleSlotDragMove");
    IntVector2 dragCurrentPosition = IntVector2(eventData["X"].GetInt(), eventData["Y"].GetInt());

    draggedElement_->SetPosition(dragCurrentPosition);
//    LOGINFOF("UISlotPanel() - HandleSlotDragMove : position=%d,%d", dragCurrentPosition.x_, dragCurrentPosition.y_);
}

int UISlotPanel::GetFocusSlotId(UIElement* focus, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel)
{
    /// Get the Slot Destination
    int slotId = GetInventory()->GetSlotIndex(focus->GetName());
    URHO3D_LOGINFOF("UISlotPanel() - GetFocusSlotId : panel=%s focus=%s slotId=%d !", GetName().CString(), focus->GetName().CString(), slotId);

    if (slotId == -1)
    {
        slotId = GetInventory()->GetSlotIndexFor(fromSlot.type_, true, GetStartSlotIndex());
        URHO3D_LOGINFOF("UISlotPanel() - GetFocusSlotId : panel=%s newslotId=%d (startindex=%d) !", GetName().CString(), slotId, GetStartSlotIndex());
    }

    if (slotId != -1)
    {
        /// The destination slot is not compatible with the type of the source => Find a new slot compatible
        if (!GetInventory()->GetTemplate()->CanSlotCollectCategory(slotId, fromSlot.type_))
        {
            slotId = GetInventory()->GetSlotIndexFor(fromSlot.type_, true, GetStartSlotIndex());
            URHO3D_LOGINFOF("UISlotPanel() - GetFocusSlotId : panel=%s lastslot incompatible => newslotId=%d (startindex=%d) !", GetName().CString(), slotId, GetStartSlotIndex());
        }
    }

    return slotId;
}

bool UISlotPanel::AcceptSlotType(int slotId, const StringHash& type, UISlotPanel* otherPanel)
{
    return type ? GetInventory()->GetTemplate()->CanSlotCollectCategory(slotId, type) : true;
}

void UISlotPanel::OnDragSlotIn(int slotId, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel)
{
    /// Acceptance of the source and destination Inventories
    if (!fromPanel->AcceptSlotType(fromSlotId, GetInventory()->GetSlot(slotId).type_, this) || !AcceptSlotType(slotId, fromSlot.type_, fromPanel))
    {
        URHO3D_LOGINFOF("UISlotPanel() - OnDragSlotIn : panels don't accept the drag !");
        return;
    }

    /// Copy the slot
    Slot toSlot = GetInventory()->GetSlot(slotId);

    unsigned int quantity = fromSlot.quantity_;
    unsigned int remainquantity = quantity;

    /// toSlot is empty => move slot
    if (toSlot.Empty())
    {
        GetInventory()->AddCollectableFromSlot(fromSlot, slotId, remainquantity, false, false, GetStartSlotIndex());
    }
    /// Same type => transfer quantity
    else if (Slot::HaveSameTypes(fromSlot, toSlot) && fromSlot.sprite_ == toSlot.sprite_)
    {
        GetInventory()->AddCollectableFromSlot(fromSlot, slotId, remainquantity, false, false, GetStartSlotIndex());
    }
    /// Different types and Acceptance of the source and destination Inventories => Swap Items
    else
    {
        fromPanel->GetInventory()->AddCollectableFromSlot(toSlot, fromSlotId, toSlot.quantity_, false, false, fromPanel->GetStartSlotIndex());

        GetInventory()->RemoveCollectableFromSlot(slotId);
        GetInventory()->AddCollectableFromSlot(fromSlot, slotId, remainquantity, false, false, GetStartSlotIndex());

        // remain quantity in the topanel
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

void UISlotPanel::OnDragSlotFinish(int slotId)
{
    UpdateSlot(slotId, true, true, true);
}

void UISlotPanel::HandleSlotDragEnd(StringHash eventType, VariantMap& eventData)
{
    if (beginDragElementByKey_)
    {
        draggedElement_ = beginDragElementByKey_;
        beginDragElementByKey_.Reset();
    }

    if (!draggedElement_)
        return;

    draggedElement_->SetEnabled(false);

    if (slotIndex_ == -1 || (onKeyDrag_ && !selectSlotByKey_))
    {
        URHO3D_LOGERRORF("UISlotPanel() - HandleDragEnd : drag error => canceled !");

        GetSubsystem<UI>()->GetRoot()->RemoveChild(draggedElement_);
        if (slotIndex_ != -1)
        {
            Slot slot = inventory_->GetSlot(slotIndex_);
            OnSlotRemain(slot, slotIndex_);
        }
        return;
    }

    IntVector2 dragCurrentPosition = IntVector2(eventData["X"].GetInt(), eventData["Y"].GetInt());
    UIElement* focusElement = onKeyDrag_ ? selectSlotByKey_ : GetSubsystem<UI>()->GetElementAt(dragCurrentPosition);
    const int fromSlotId = slotIndex_;
    int toSlotId = -1;

//    URHO3D_LOGINFOF("UISlotPanel() - HandleSlotDragEnd : panel=%s position=%d,%d ... focusElt=%s fromSlotId=%d",
//                    name_.CString(), dragCurrentPosition.x_, dragCurrentPosition.y_, focusElement ? focusElement->GetName().CString() : "none", fromSlotId);

    /// Backup Slot and remove
    Slot slot = inventory_->GetSlot(fromSlotId);
    inventory_->RemoveCollectableFromSlot(fromSlotId);

    /// Get the toPanel
    UISlotPanel* toPanel = 0;
    Node* toAvatar = 0;
    if (focusElement)
    {
        UIElement* parent;
        Variant controllerVar;
        GameHelpers::FindParentWithAttribute(GOA::OWNERID, focusElement, parent, controllerVar);
        if (parent)
            toPanel = dynamic_cast<UISlotPanel*>(GetPanel(parent->GetVar(GOA::PANELID).GetString()));
    }

    /// Drop in a panel
    if (toPanel)
    {
        if (focusElement->GetName().Empty())
            focusElement = focusElement->GetParent();

        /// Get the slot id
        toSlotId = toPanel->GetFocusSlotId(focusElement, slot, fromSlotId, this);

        /// Skip Case : same panel, same inventory and same slotids
        if (toPanel == this && dragFromInventory_ == GetInventory() && toSlotId == fromSlotId)
        {
            URHO3D_LOGINFOF("UISlotPanel() - HandleSlotDragEnd() : Skip Case : same panel same slotids=%d !", fromSlotId);
            toSlotId = -1;
        }

        if (toSlotId != -1)
            toPanel->OnDragSlotIn(toSlotId, slot, fromSlotId, this);
    }
    /// Drop out of a panel
    else
    {
        bool allowDropInScene = !IsInstanceOf<UIC_ShopPanel>();

        if (allowDropInScene)
        {
            int x = dragCurrentPosition.x_ * GameContext::Get().uiScale_;
            int y = dragCurrentPosition.y_ * GameContext::Get().uiScale_;

            int dropmode = UseSlotItem(slot, fromSlotId, true, IntVector2(x, y));
        }
    }

    OnDragSlotFinish(fromSlotId);
    OnSlotRemain(slot, fromSlotId);

    GetSubsystem<UI>()->GetRoot()->RemoveChild(draggedElement_);
    GameContext::Get().gameConfig_.autoHideCursorEnable_ = true;

    draggedElement_.Reset();

//    URHO3D_LOGINFO("UISlotPanel() - HandleSlotDragEnd() : ... OK !");
}

int UISlotPanel::UseSlotItem(Slot& slot, int fromSlotId, bool allowdrop, const IntVector2& droppedPosition)
{
    unsigned int slotqty = slot.quantity_;

    /// Check for usability of the type in the slot
    StringHash type = slot.type_;
    unsigned typeProps = GOT::GetTypeProperties(type);
    bool useon  = (typeProps & GOT_Usable);
    bool useout = (typeProps & GOT_Usable_DropOut);

    if (!useon && !useout && !allowdrop)
        return -1;

    int dropmode = -1;

    // Check dropon mode
    if (useon && droppedPosition != IntVector2::NEGATIVE_ONE)
    {
        Viewport* viewport = GetSubsystem<Renderer>()->GetViewport(ViewManager::GetViewportAt(droppedPosition.x_, droppedPosition.y_));
        Vector3 itemWorldPosition = viewport->ScreenToWorldPoint(droppedPosition.x_, droppedPosition.y_, 12.f);
        itemWorldPosition.z_ = 0.f;

        Drawable* drawable = holderNode_->GetDerivedComponent<Drawable>();
        useon = drawable && drawable->GetWorldBoundingBox().IsInside(itemWorldPosition) != OUTSIDE;
    }

    Node* node = 0;

    // Note : Never Use context_->GetEventDataMap() Here
    // Because if we get it, this EventData will be cleared in next step (inside Map::AddEntity) and we need to keep
    // the content of the evendata for use in GO_DROPITEM with the slotdata inside it.

    VariantMap eventData;

    /// Use the item on holder (DropOn Mode)
    if (useon)
    {
        /// Drop an item
        slotqty = 1U;
        node = GOC_Collectable::DropSlotFrom(holderNode_, slot, true, slotqty, &eventData);
        if (node)
        {
            // Apply Effect
            GOC_ZoneEffect* itemEffect = node->GetComponent<GOC_ZoneEffect>();
            if (itemEffect)
                itemEffect->UseEffectOn(holderNode_->GetID());

            // Item don't fall/move
            RigidBody2D* itemBody = node->GetComponent<RigidBody2D>();
            if (itemBody)
                itemBody->SetEnabled(false);

            dropmode = 1;
            URHO3D_LOGINFOF("UISlotPanel() - UseSlotItem : drop and use item=%s on holder !", GOT::GetType(type).CString());
        }
    }
    /// Use item (DropOut Mode)
    else if (useout)
    {
        /// Drop an item
        slotqty = 1U;
        node = GOC_Collectable::DropSlotFrom(holderNode_, slot, true, slotqty, &eventData);
        if (node)
        {
            GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
            if (animator)
            {
                if (!animator->GetTemplateName().StartsWith("AnimatorTemplate_Usable"))
                    animator->SetTemplateName("AnimatorTemplate_Usable");
            }
            //node->GetComponent<GOC_Collectable>()->SetEnabled(false);
            node->SendEvent(GO_USEITEM);
            dropmode = 2;
            URHO3D_LOGINFOF("UISlotPanel() - UseSlotItem : drop and use item=%s !", GOT::GetType(type).CString());
        }
    }
    /// Just drop
    else if (allowdrop)
    {
        /// Drop items
        node = GOC_Collectable::DropSlotFrom(holderNode_, slot, true, slotqty, &eventData);
        if (node)
        {
            dropmode = 0;
            URHO3D_LOGINFOF("UISlotPanel() - UseSlotItem : drop item=%s!", GOT::GetType(type).CString());
        }
    }

    /// Send Network event
    if (!GameContext::Get().LocalMode_ && dropmode != -1 && node)
    {
        eventData[Net_ObjectCommand::P_NODEID] = node->GetID();
        eventData[Net_ObjectCommand::P_NODEIDFROM] = holderNode_->GetID();
        eventData[Net_ObjectCommand::P_SLOTQUANTITY] = slotqty;
        eventData[Net_ObjectCommand::P_INVENTORYIDSLOT] = fromSlotId;
        eventData[Net_ObjectCommand::P_INVENTORYDROPMODE] = dropmode;
        SendEvent(GO_DROPITEM, eventData);
    }

    return dropmode;
}

void UISlotPanel::OnSlotRemain(Slot& slot, int slotid)
{
    /// Restore the remain quantity in the source
    if (slot.quantity_ > 0)
    {
        URHO3D_LOGINFOF("UISlotPanel() - OnSlotRemain : restore remain item=%s qty=%u to inventory !",
                        GOT::GetType(slot.type_).CString(), slot.quantity_);

        GetInventory()->AddCollectableFromSlot(slot, slot.quantity_, GetStartSlotIndex());
        UpdateSlots(true, true);
    }

    /// Drop the remain to the scene
    if (slot.quantity_ > 0)
    {
        URHO3D_LOGINFOF("UISlotPanel() - OnSlotRemain : drop remain item=%s qty=%u !",
                        GOT::GetType(slot.type_).CString(), slot.quantity_);

        Node* node = GOC_Collectable::DropSlotFrom(holderNode_, slot, false);
        GameHelpers::SpawnSound(holderNode_, "Sounds/024-Door01.ogg");
    }

    OnDragSlotFinish(slotid);
}

void UISlotPanel::BeginKeyDrag()
{
    VariantMap& eventData = context_->GetEventDataMap();

    if (selectHalo_)
    {
        selectHalo_->SetEnabled(false);
        IntVector2 position = selectHalo_->GetScreenPosition();
        eventData["X"] = position.x_;
        eventData["Y"] = position.y_;
    }

    onKeyDrag_ = true;
    HandleSlotDragBegin(StringHash::ZERO, eventData);

    fromPanelDrag_ = this;
    selectSlotByKey_.Reset();

    if (selectHalo_)
        selectHalo_->SetEnabled(true);
}

void UISlotPanel::EndKeyDrag()
{
    VariantMap& eventData = context_->GetEventDataMap();

    if (selectHalo_)
    {
        selectHalo_->SetEnabled(false);
        IntVector2 position = selectHalo_->GetScreenPosition();
        eventData["X"] = position.x_;
        eventData["Y"] = position.y_;
    }

    fromPanelDrag_->HandleSlotDragEnd(StringHash::ZERO, eventData);
    onKeyDrag_ = false;

    fromPanelDrag_ = 0;
    selectSlotByKey_.Reset();

    if (selectHalo_)
        selectHalo_->SetEnabled(true);
}

void UISlotPanel::UpdateSlotSelector()
{
    if (!selectHalo_)
        return;

    slotselector_ = Clamp(slotselector_, slotselector_, endSlotIndex2_);

    String slotname = inventory_->GetSlotName(slotselector_);
    if (slotname.Empty())
    {
        if (slotselector_ > endSlotIndex1_)
            slotname = inventorySectionEnd_ + String("_") + String(slotselector_-endSlotIndex1_+1);
        else
            slotname = inventorySectionStart_ + String("_") + String(slotselector_-startSlotIndex_+1);
    }

    selectSlotByKey_ = panel_->GetChild(slotname, true);

    bool setHalo = selectSlotByKey_ && selectSlotByKey_->IsVisible();
    if (setHalo)
    {
        selectHalo_->SetMinSize(selectSlotByKey_->GetSize());
        selectHalo_->SetMaxSize(selectSlotByKey_->GetSize());
        selectHalo_->SetPosition(selectSlotByKey_->GetScreenPosition() - panel_->GetScreenPosition());
        if (selectSlotByKey_->GetChild(0))
            selectSlotByKey_ = selectSlotByKey_->GetChild(0);
    }

    URHO3D_LOGINFOF("UISlotPanel() - UpdateSlotSelector : %s ... slotselector_=%u slotname=%s size=%s !", panel_->GetName().CString(), slotselector_, slotname.CString(), selectHalo_->GetSize().ToString().CString());

    selectHalo_->SetVisible(setHalo);
}

void UISlotPanel::OnKey(StringHash eventType, VariantMap& eventData)
{
    Actor* holder = static_cast<Actor*>(user_);
    if (holder->GetFocusPanel() != this)
        return;

    int scancode = GetKeyFromEvent(holder->GetControlID(), eventType, eventData);
    if (!scancode)
        return;

    const PODVector<int>& keymap = GameContext::Get().keysMap_[holder->GetControlID()];

    if (scancode == keymap[ACTION_INTERACT])
    {
        URHO3D_LOGINFOF("UISlotPanel() - OnKeyDown : %s key=%s action ... !", panel_->GetName().CString(), GameContext::Get().input_->GetScancodeName(scancode).CString());

        UpdateSlotSelector();

        if (!beginDragElementByKey_)
            BeginKeyDrag();
        else
            EndKeyDrag();
    }
    else
    {
        int inc = 0;
        if (scancode == keymap[ACTION_LEFT])
            inc = incselector_[ACTION_LEFT];
        else if (scancode == keymap[ACTION_DOWN])
            inc = incselector_[ACTION_DOWN];
        else if (scancode == keymap[ACTION_RIGHT])
            inc = incselector_[ACTION_RIGHT];
        else if (scancode == keymap[ACTION_UP])
            inc = incselector_[ACTION_UP];

        if (inc != 0)
        {
            if (inc < 0 && slotselector_ == startSlotIndex_)
                slotselector_ = endSlotIndex2_;
            else if (inc > 0 && slotselector_ == endSlotIndex2_)
                slotselector_ = startSlotIndex_;
            else
                slotselector_ += inc;
            UpdateSlotSelector();
        }
    }
}
