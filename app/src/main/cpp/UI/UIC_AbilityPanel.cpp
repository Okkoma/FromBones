#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

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

#include "Actor.h"
#include "Ability.h"
#include "Equipment.h"

#include "UISlotPanel.h"

#include "UIC_AbilityPanel.h"



UIC_AbilityPanel::UIC_AbilityPanel(Context* context) :
    UIPanel(context),
    activeAbilityButton_(0)
{
    ;
}

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

    Clear();
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

void UIC_AbilityPanel::Update()
{
    if (holder_ && holder_->GetAbilities() && holder_->GetAbilities()->GetNumAbilities())
        SetVisible(true);
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

        Update();
    }
}
