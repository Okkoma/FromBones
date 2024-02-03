#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
//#include <Urho3D/Urho2D/AnimationSet2D.h>
//#include <Urho3D/Urho2D/AnimatedSprite2D.h>

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
#include "GameHelpers.h"
#include "GameGoal.h"
#include "GameContext.h"

#include "TimerInformer.h"

#include "GOC_Collectable.h"

#include "Actor.h"
#include "Player.h"

#include "UISlotPanel.h"

#include "UIC_JournalPanel.h"



UIC_JournalPanel::UIC_JournalPanel(Context* context) :
    UIPanel(context),
    selectedMission_(0),
    slotSize_(48)
{ }

void UIC_JournalPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_JournalPanel>();
}

void UIC_JournalPanel::Start(Object* user, Object* feeder)
{
    user = user;
    feeder_ = feeder;

    Button* closeButton = dynamic_cast<Button*>(panel_->GetChild("OkButton", true));
    if (closeButton)
        SubscribeToEvent(closeButton, E_RELEASED, URHO3D_HANDLER(UIC_JournalPanel, OnCloseButton));

    questSelectorZone_ = panel_->GetChild("QuestSelector", true);
    if (!questSelectorZone_)
    {
        URHO3D_LOGERRORF("UIC_JournalPanel() - Start id=%s : Can't get questSelectorZone_ Element !!!", name_.CString());
        return;
    }

    currentQuestZone_ = panel_->GetChild("CurrentQuest", true);
    if (!currentQuestZone_)
    {
        URHO3D_LOGERRORF("UIC_JournalPanel() - Start id=%s : Can't get currentQuestZone_ Element !!!", name_.CString());
        return;
    }

    manager_ = dynamic_cast<MissionManager*>(feeder_);

    if (manager_)
    {
        manager_->Clear();

        SubscribeToEvent(manager_, GO_NAMEDMISSIONADDED, URHO3D_HANDLER(UIC_JournalPanel, OnNamedMissionAdded));
        SubscribeToEvent(manager_, GO_NAMEDMISSIONUPDATED, URHO3D_HANDLER(UIC_JournalPanel, OnNamedMissionUpdated));
        SubscribeToEvent(manager_, GO_NAMEDOBJECTIVEUPDATED, URHO3D_HANDLER(UIC_JournalPanel, OnNamedObjectiveUpdated));
    }

    selectedMission_ = 0;
    Update();
}

void UIC_JournalPanel::Update()
{
    UpdateSelector();
    UpdateSelectedMission();
}

void UIC_JournalPanel::UpdateSelectedMission()
{
    if (!currentQuestZone_)
        return;

    URHO3D_LOGERRORF("UIC_JournalPanel() - UpdateSelectedMission !");

    Text* title = static_cast<Text*>(panel_->GetChild("QuestTitle", true));
    title->SetText(selectedMission_ ? selectedMission_->GetMissionData()->title_ : "empty");

    UIElement* details = currentQuestZone_->GetChild("QuestDetails", false);
    details->RemoveAllChildren();

    UIElement* content = panel_->GetChild("QuestContent", true);
    content->SetMinSize(600, selectedMission_ ? 220 : 20);
    content->SetMaxSize(600, selectedMission_ ? 220 : 20);
    content->UpdateLayout();

    if (!selectedMission_)
        return;

    const PODVector<Objective>& objectives = selectedMission_->GetObjectives();
    for (unsigned i=0; i < objectives.Size(); i++)
    {
        const Objective& objective = objectives[i];

        if (!objective.data_)
        {
            if (selectedMission_->GetSequenced())
                break;
            else
                continue;
        }

        bool success = (objective.state_ >= IsCompleted && objective.state_ != IsFailed);

        Text* text = details->CreateChild<Text>();
        text->SetStyle("QuestObjective");
        text->SetWordwrap(true);
        text->SetText(objective.data_->texts_[StringHash(success ? "success" : "intro")]);

        if (selectedMission_->GetSequenced() && !success)
            break;
    }
}

void UIC_JournalPanel::UpdateSelector()
{
    if (!questSelectorZone_ || !manager_)
        return;

    const HashMap<StringHash, Mission>& namedmissions = manager_->GetNamedMissions();

    PODVector<const Mission*> newmissions;
    PODVector<UIElement*> missionstoremove;

    {
        const Vector<SharedPtr<UIElement> >& questbuttons = questSelectorZone_->GetChildren();
        for (Vector<SharedPtr<UIElement> >::ConstIterator it = questbuttons.Begin(); it != questbuttons.End(); ++it)
        {
            const String& questname = it->Get()->GetName();

            HashMap<StringHash, Mission>::ConstIterator jt = namedmissions.Find(StringHash(questname));
            if (jt == namedmissions.End())
            {
                missionstoremove.Push(it->Get());
            }
//            else
//            {
//                missionstoupdate.Push(it->Get());
//            }
        }
    }

    // Remove Old Quest Buttons
    for (PODVector<UIElement*>::ConstIterator it = missionstoremove.Begin(); it != missionstoremove.End(); ++it)
    {
        questSelectorZone_->RemoveChild(*it);
    }

    {
        const Vector<SharedPtr<UIElement> >& questbuttons = questSelectorZone_->GetChildren();

        for (HashMap<StringHash, Mission>::ConstIterator it = namedmissions.Begin(); it != namedmissions.End(); ++it)
        {
            const Mission& mission = it->second_;

            bool isnew = true;
            for (Vector<SharedPtr<UIElement> >::ConstIterator jt = questbuttons.Begin(); jt != questbuttons.End(); ++jt)
                isnew &= (StringHash(jt->Get()->GetName()) != it->first_);

            if (!isnew)
                continue;

            // Add New Mission
            newmissions.Push(&mission);
        }
    }

    // Add New Quest Button
    for (unsigned i=0; i < newmissions.Size(); i++)
    {
        const Mission* mission = newmissions[i];
        const String& name = mission->GetMissionData()->name_;
        Button* questbutton = questSelectorZone_->CreateChild<Button>(name);
        questbutton->SetStyle("QuestButton");
        questbutton->SetName(name);
        Text* text = static_cast<Text*>(questbutton->GetChild(0));
        text->SetText(name);

        MarkAsNew(questbutton);

        if (!selectedMission_)
        {
            selectedMission_ = (Mission*)mission;
            UpdateSelectedMission();
        }
    }
}

void UIC_JournalPanel::MarkAsNew(UIElement* element)
{

}

void UIC_JournalPanel::OnNamedMissionAdded(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_JournalPanel() - OnNamedMissionAdded !");

    UpdateSelector();

    if (!IsVisible())
        ToggleVisible();
}

void UIC_JournalPanel::OnNamedMissionUpdated(StringHash eventType, VariantMap& eventData)
{
    Mission* mission = manager_->GetNamedMission(eventData[Go_MissionUpdated::GO_MISSIONID].GetUInt());
    if (mission)
        delayedMissionsToUpdate_.Push(mission);

    if (!IsVisible())
    {
        ToggleVisible();
        DelayInformer* delay = new DelayInformer(manager_, SWITCHSCREENTIME + 0.1f, GO_NAMEDMISSIONUPDATED);
        return;
    }

    if (!panel_->IsEnabled())
        return;

    for (PODVector<Mission*>::Iterator it=delayedMissionsToUpdate_.Begin(); it != delayedMissionsToUpdate_.End(); ++it)
    {
        mission = *it;

        if (selectedMission_ == mission)
            UpdateSelectedMission();

        if (mission->GetState() == IsCompleted)
        {
            // Quest Interactions / Animations

            // Get Rewards
            const PODVector<Reward>& rewards = mission->GetRewards();
            Actor* actor = Actor::Get(manager_->GetOwner());

            Node* avatar = actor ? actor->GetAvatar() : 0;

            URHO3D_LOGINFOF("UIC_JournalPanel() - OnNamedMissionUpdated : mission state = IsCompleted ! get rewards number=%u for avatar=%s",
                            rewards.Size(), avatar ? avatar->GetName().CString() : "NA");

            if (avatar)
            {
                for (PODVector<Reward>::ConstIterator it=rewards.Begin(); it!=rewards.End(); ++it)
                {
                    const Reward& reward = *it;

                    if (reward.rewardCategory_ == COT::AVATAR.Value())
                    {
                        Player* player = static_cast<Player*>(actor);
                        if (player)
                        {
                            URHO3D_LOGWARNINGF("UIC_JournalPanel() - Add Avatar Reward %s(%u) to avatar ...", GOT::GetType(StringHash(reward.type_)).CString(), reward.type_);
                            if (player->AddAvatar(StringHash(reward.type_)))
                                URHO3D_LOGINFOF(" ... Added !");
                        }
                    }
                    else if (reward.rewardCategory_ == COT::ABILITY.Value())
                    {
                        URHO3D_LOGWARNINGF("UIC_JournalPanel() - Add Ability Reward %u to avatar TODO ...", reward.type_);
                    }
                    else if (reward.rewardCategory_ == COT::QUEST.Value())
                    {
                        URHO3D_LOGWARNINGF("UIC_JournalPanel() - Add Quest Reward %u to avatar TODO ...", reward.type_);
                    }
                    else if (reward.rewardCategory_ == COT::MONEY.Value() || reward.rewardCategory_ == COT::ITEMS.Value())
                    {
                        Slot slot;
                        slot.Set(StringHash(reward.type_), reward.quantity_);

                        URHO3D_LOGINFOF("UIC_JournalPanel() - Add Items Reward %s(%u) qty=%u to avatar ...",
                                        GOT::GetType(slot.type_).CString(), slot.type_.Value(), slot.quantity_);

                        GOC_Collectable::TransferSlotTo(slot, 0, avatar, Variant(currentQuestZone_->GetScreenPosition()));

                        if (slot.quantity_)
                        {
                            URHO3D_LOGINFOF("UIC_JournalPanel() - No More Slot in Avatar => Drop Collectable Reward %s qty=%u from avatar to scene",
                                            GOT::GetType(slot.type_).CString(), slot.quantity_);

                            const int dropmode = !GameContext::Get().LocalMode_ ? SLOT_DROPREMAIN : SLOT_NONE;
                            Node* dropped = GOC_Collectable::DropSlotFrom(avatar, slot, dropmode);
                        }
                    }
                    else
                    {
                        URHO3D_LOGWARNINGF("UIC_JournalPanel() - Reward category=%u NOT IMPLEMENTED ...", reward.rewardCategory_);
                    }
                }
            }

            mission->SetState(IsFinished);
        }
    }

    delayedMissionsToUpdate_.Clear();
}

void UIC_JournalPanel::OnNamedObjectiveUpdated(StringHash eventType, VariantMap& eventData)
{
    Mission* mission = manager_->GetNamedMission(eventData[Go_ObjectiveUpdated::GO_MISSIONID].GetUInt());

    if (mission)
    {
        URHO3D_LOGINFOF("UIC_JournalPanel() - OnNamedObjectiveUpdated : mission id=%u (selected=%u) !", mission->GetID(), selectedMission_->GetID());

        if (mission == selectedMission_)
        {
            URHO3D_LOGINFOF("UIC_JournalPanel() - OnNamedObjectiveUpdated !");
            UpdateSelectedMission();
        }
    }
}

void UIC_JournalPanel::OnCloseButton(StringHash eventType, VariantMap& eventData)
{
    UIPanel::ToggleVisible();
}


