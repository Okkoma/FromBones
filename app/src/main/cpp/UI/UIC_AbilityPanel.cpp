#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Input/Input.h>

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
#include "Ability.h"
#include "Equipment.h"

#include "UISlotPanel.h"

#include "UIC_AbilityPanel.h"



UIC_AbilityPanel::UIC_AbilityPanel(Context* context) :
    UIPanel(context),
    popup_(false),
    selectHalo_(0),
    activeAbilityButton_(0) { }

void UIC_AbilityPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_AbilityPanel>();
}

void UIC_AbilityPanel::Start(Object* user, Object* feeder)
{
    UIPanel::Start(user, feeder);

    holder_ = dynamic_cast<Actor*>(feeder_);

    if (holder_)
    {
        SubscribeToEvent(holder_, GO_NODESCHANGED, URHO3D_HANDLER(UIC_AbilityPanel, OnNodesChange));
        SubscribeToEvent(holder_, GO_ABILITYADDED, URHO3D_HANDLER(UIC_AbilityPanel, OnAbilityUpdated));
        SubscribeToEvent(holder_, GO_ABILITYREMOVED, URHO3D_HANDLER(UIC_AbilityPanel, OnAbilityUpdated));
    }
    else
        URHO3D_LOGWARNINGF("UIC_AbilityPanel() - Start id=%d : no holder !");

    slotZone_ = panel_->GetChild("SlotZone", true);
    slotselector_ = 0;
    selectHalo_ = panel_->GetChild(String("I_Select"), true);

    Clear();
}

void UIC_AbilityPanel::GainFocus()
{
    if (selectHalo_)
        selectHalo_->SetVisible(true);
    if (popup_)
    {
        SetVisible(true);
        GetElement()->SetPriority(GetElement()->GetParent()->GetPriority()+11);
    }
    UIPanel::GainFocus();
}

void UIC_AbilityPanel::LoseFocus()
{
    if (selectHalo_)
        selectHalo_->SetVisible(false);
    if (popup_)
        SetVisible(false);
    UIPanel::LoseFocus();
}

bool UIC_AbilityPanel::CanFocus() const
{
    return holder_->GetAbilities()->GetNumAbilities() && UIPanel::CanFocus();
}

void UIC_AbilityPanel::OnSetVisible()
{
    UIPanel::OnSetVisible();

    if (panel_->IsVisible())
    {
        int controltype = GameContext::Get().playerState_[static_cast<Actor*>(user_)->GetControlID()].controltype;
        if (controltype == CT_KEYBOARD || controltype == CT_JOYSTICK)
        {
            if (panel_->HasFocus())
            {
                URHO3D_LOGINFOF("UIC_AbilityPanel() - OnSetVisible : is visible and has the focus ...");

                // Allow Selection Access to other UIC with the keyboard Arrows
                if (controltype == CT_KEYBOARD)
                {
                    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(UIC_AbilityPanel, OnKey));
                }
                else
                {
                    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(UIC_AbilityPanel, OnKey));
                    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(UIC_AbilityPanel, OnKey));
                    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(UIC_AbilityPanel, OnKey));
                }

                UpdateSlotSelector();
            }
            else
            {
                URHO3D_LOGINFOF("UIC_AbilityPanel() - OnSetVisible : is visible and has not the focus ...");

                if (HasSubscribedToEvent(E_KEYDOWN))
                {
                    UnsubscribeFromEvent(E_KEYDOWN);
                }
                else
                {
                    UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
                    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
                    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
                }
            }
        }
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
    }
}

void UIC_AbilityPanel::Clear()
{
    if (!slotZone_)
        return;

    URHO3D_LOGINFOF("UIC_AbilityPanel() - Clear() !");

    const Vector<SharedPtr<UIElement> >& children = slotZone_->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator it=children.Begin(); it!=children.End(); ++it)
    {
        (*it)->SetVisible(false);
        (*it)->SetVar(GOA::ABILITY, StringHash::ZERO);
    }

    SetVisible(false);
}

void UIC_AbilityPanel::SetActiveAbility(const StringHash& hash)
{
    if (!slotZone_)
        return;

    // GetChild with variable attribute GOA::ABILITY
    UIElement* elementToActive = slotZone_->GetChild(GOA::ABILITY, hash);

    if (!elementToActive)
        return;

    if (!elementToActive->IsEnabled())
        return;

    // DeActive the current actived Ability
    if (activeAbilityButton_)
    {
        activeAbilityButton_->SetColor(Color::GRAY);
        activeAbilityButton_->GetChild(0)->SetColor(Color::GRAY);
    }

    activeAbilityButton_ = elementToActive;
    activeAbilityButton_->SetColor(Color::WHITE);
    activeAbilityButton_->GetChild(0)->SetColor(Color::WHITE);

    // Active new selected Ability
    GOC_Abilities* abilities = holder_->GetAbilities();
    if (abilities)
    {
        Ability* ability = abilities->GetAbility(hash);
        abilities->SetActiveAbility(ability);
        if (ability)
            ability->Update();
    }
}

void UIC_AbilityPanel::DesactiveAbility()
{
    // DeActive the current actived Ability
    if (activeAbilityButton_)
    {
        activeAbilityButton_->SetColor(Color::GRAY);
        activeAbilityButton_->GetChild(0)->SetColor(Color::GRAY);
        activeAbilityButton_ = 0;
        lastAbilityHash_ = 0;
        if (holder_->GetAbilities())
            holder_->GetAbilities()->SetActiveAbility(0);
    }
}

void UIC_AbilityPanel::UpdateSlotSelector()
{
    if (!selectHalo_)
        return;

    UIElement* slotselected = slotselector_ != -1 ? slotZone_->GetChild(slotselector_) : 0;
    if (slotselected)
    {
        selectHalo_->SetMinSize(slotselected->GetSize());
        selectHalo_->SetMaxSize(slotselected->GetSize());
        selectHalo_->SetPosition(slotselected->GetScreenPosition() - panel_->GetScreenPosition());
    }

    selectHalo_->SetVisible(slotselected != 0);

    URHO3D_LOGINFOF("UIC_AbilityPanel() - UpdateSlotSelector : %s ... slotselector_=%d size=%s !",
                    panel_->GetName().CString(), slotselector_, selectHalo_->GetSize().ToString().CString());
}

void UIC_AbilityPanel::HandleClic(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_AbilityPanel() - HandleClic !");
    UIElement* element = static_cast<UIElement*>(eventData["Element"].GetPtr());

    if (!element)
        return;

    if (activeAbilityButton_ != element)
    {
        lastAbilityHash_ = element->GetVar(GOA::ABILITY).GetStringHash();
        URHO3D_LOGINFOF("UIC_AbilityPanel() - HandleClic : Active Ability hash = %u", lastAbilityHash_.Value());
        SetActiveAbility(lastAbilityHash_);
    }
    else
    {
        DesactiveAbility();
    }

    GetSubsystem<UI>()->SetFocusElement(0);
}

void UIC_AbilityPanel::OnAbilityUpdated(StringHash eventType, VariantMap& eventData)
{
    if (!slotZone_)
        return;

    StringHash hash = eventData[GOA::ABILITY].GetStringHash();

    if (eventType == GO_ABILITYADDED)
    {
        if (hash != StringHash::ZERO)
        {
            Button* abilitySlot = static_cast<Button*>(slotZone_->GetChild(GOA::ABILITY, StringHash::ZERO));
            // update texture rect
            BorderImage* image = static_cast<BorderImage*>(abilitySlot->GetChild(0));
            Ability* ability = holder_->GetAbilities()->GetAbility(hash);
            String gotname = GOT::GetType(ability->GOT_);

            URHO3D_LOGINFOF("UIC_AbilityPanel() - Add Ability : got=%u gotname=%s", ability->GOT_.Value(), gotname.CString());

            GameHelpers::SetUIElementFrom(image, UIEQUIPMENT, gotname.Empty() ? ability->GetTypeName() : gotname, abilitySlot->GetSize().x_);
//            GameHelpers::SetUIElementFrom(image, UIEQUIPMENT, got, (int)((float)abilitySlot->GetSize().x_*0.75f));
            image->SetOpacity(0.95f);

//            abilitySlot->SetImageRect(holder_->GetAbilities()->GetAbility(hash)->GetIconRect());
            // mark ability in slot
            abilitySlot->SetVar(GOA::ABILITY, hash);

            SubscribeToEvent(abilitySlot, E_RELEASED, URHO3D_HANDLER(UIC_AbilityPanel, HandleClic));

            abilitySlot->SetVisible(true);
            // active it by default
            SetActiveAbility(hash);
        }
        else if (activeAbilityButton_)
        {
            if (holder_->GetAbilities()->GetActiveAbility())
                SetActiveAbility(holder_->GetAbilities()->GetActiveAbility()->GetType());
            else
                DesactiveAbility();
//            if (holder_->GetAbilities()->GetAbility(lastAbilityHash_))
//            {
//                URHO3D_LOGERRORF("UIC_AbilityPanel() - Refresh activeAbility hash=%u !", lastAbilityHash_.Value());
//                SetActiveAbility(lastAbilityHash_);
//            }
        }

        if (!popup_)
            SetVisible(true);
    }
    else if (eventType == GO_ABILITYREMOVED)
    {
        /// Remove a ability button
        if (hash != StringHash::ZERO)
        {
            URHO3D_LOGINFOF("UIC_AbilityPanel() - Remove Ability !");

            UIElement* abilitySlot = slotZone_->GetChild(GOA::ABILITY, hash);
            if (abilitySlot)
            {
                UnsubscribeFromEvent(abilitySlot, E_RELEASED);
                abilitySlot->SetVisible(false);
                abilitySlot->SetVar(GOA::ABILITY, StringHash::ZERO);
            }
        }
        /// Remove all abilities buttons
        else
        {
            URHO3D_LOGINFOF("UIC_AbilityPanel() - Remove All Abilities !");
            Clear();
        }
    }
}

void UIC_AbilityPanel::OnNodesChange(StringHash eventType, VariantMap& eventData)
{
    if (!holder_)
        return;

    if (holderNode_ && holder_->GetAvatar() == holderNode_)
        return;

    URHO3D_LOGINFOF("UIC_AbilityPanel() - OnHolderNodeChange : panel=%s ...", name_.CString());

    holderNode_ = holder_->GetAvatar();

    slotZone_ = panel_->GetChild("SlotZone", true);

    if (slotZone_)
    {
        panel_->SetVar(GOA::OWNERID, holderNode_->GetID());
        slotZone_->SetVar(GOA::OWNERID, holderNode_->GetID());
        panel_->SetVar(GOA::PANELID, name_);
        slotZone_->SetVar(GOA::PANELID, name_);

        if (!popup_ && holder_ && holder_->GetAbilities() && holder_->GetAbilities()->GetNumAbilities())
            SetVisible(true);
    }
}

void UIC_AbilityPanel::OnKey(StringHash eventType, VariantMap& eventData)
{
    Player* player = static_cast<Player*>(holder_.Get());
    if (player->GetFocusPanel() != this)
        return;

    int scancode = GetKeyFromEvent(player->GetControlID(), eventType, eventData);
    if (!scancode)
        return;

    const PODVector<int>& keymap = GameContext::Get().keysMap_[player->GetControlID()];

    URHO3D_LOGINFOF("UIC_AbilityPanel() - OnKey : panel=%s ...", name_.CString());

    // TODO add an ability selector
    if (scancode == keymap[ACTION_DOWN])
    {
        LoseFocus();
        player->SetFocusPanel(-1);
    }
    else if (scancode == keymap[ACTION_UP])
    {
        LoseFocus();
        if (popup_)
        {
            player->GetPanel(STATUSPANEL)->GainFocus();
            player->SetFocusPanel(STATUSPANEL);
        }
        else
            player->SetFocusPanel(-1);
    }
    else if (scancode == keymap[ACTION_LEFT]) // Move Selector to left
    {
        slotselector_ = Clamp(slotselector_-1, 0, (int)holder_->GetAbilities()->GetNumAbilities()-1);
        UpdateSlotSelector();
    }
    else if (scancode == keymap[ACTION_RIGHT]) // Move Selector to right
    {
        slotselector_ = Clamp(slotselector_+1, 0, (int)holder_->GetAbilities()->GetNumAbilities()-1);
        UpdateSlotSelector();
    }
    else if (scancode == keymap[ACTION_INTERACT]) // Active/Desactive Ability under the Selector
    {
        eventData["Element"] = slotZone_->GetChild(slotselector_);
        HandleClic(StringHash::ZERO, eventData);
    }
}
