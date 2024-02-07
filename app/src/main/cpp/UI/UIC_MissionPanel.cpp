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

#include "GOC_Collectable.h"

#include "Actor.h"

#include "UISlotPanel.h"

#include "UIC_MissionPanel.h"


const String& ITEM = String("Item");
const String& QTY = String("Qty");
const String& ELAPSEDQTY = String("ElapsedQty");
const String& MARK = String("Mark");


UIC_MissionPanel::UIC_MissionPanel(Context* context) :
    UIPanel(context),
    mission_(0),
    slotSize_(48)
{ }

void UIC_MissionPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_MissionPanel>();
}

void UIC_MissionPanel::Start(Object* user, Object* feeder)
{
    user = user;
    feeder_ = feeder;

    Button* closeButton = dynamic_cast<Button*>(panel_->GetChild("OkButton", true));
    if (closeButton)
        SubscribeToEvent(closeButton, E_RELEASED, URHO3D_HANDLER(UIC_MissionPanel, OnCloseButton));

    Button* startButton = dynamic_cast<Button*>(panel_->GetChild("StateButton", true));
    if (startButton)
        SubscribeToEvent(startButton, E_RELEASED, URHO3D_HANDLER(UIC_MissionPanel, OnStateButton));

    objectiveZone_ = panel_->GetChild("ObjectiveZone", true);
    if (!objectiveZone_)
    {
        URHO3D_LOGERRORF("UIC_MissionPanel() - Start id=%s : Can't get objectiveZone_ Element !!!", name_.CString());
        return;
    }
    rewardZone_ = panel_->GetChild("RewardZone", true);
    if (!rewardZone_)
    {
        URHO3D_LOGERRORF("UIC_MissionPanel() - Start id=%s : Can't get rewardZone_ Element !!!", name_.CString());
        return;
    }

    manager_ = dynamic_cast<MissionManager*>(feeder_);
    mission_ = 0;

    Clear();

    SubscribeToEvent(manager_, GO_MISSIONADDED, URHO3D_HANDLER(UIC_MissionPanel, OnMissionChanged));
    SubscribeToEvent(manager_, GO_MISSIONUPDATED, URHO3D_HANDLER(UIC_MissionPanel, OnMissionUpdated));
    SubscribeToEvent(manager_, GO_OBJECTIVEUPDATED, URHO3D_HANDLER(UIC_MissionPanel, OnObjectiveUpdated));

}

void UIC_MissionPanel::SetImage(BorderImage* image, Sprite2D* sprite, StringHash got)
{
    if (image)
    {
        Sprite2D* uisprite=0;

        if (sprite)
            uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(sprite->GetName());

        if (!uisprite && got)
            uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(GOT::GetType(got));

        if (!uisprite)
            uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(String("or_pepite04"));

        /// Set Size by default for unknown item
        image->SetSize(IntVector2(slotSize_ * GameContext::Get().uiScale_, slotSize_ * GameContext::Get().uiScale_));

        if (uisprite)
        {
            image->SetTexture(uisprite->GetTexture());
            image->SetImageRect(uisprite->GetRectangle());
            IntVector2 size = uisprite->GetRectangle().Size();
            size.x_ = (int)((float)size.x_ * GameContext::Get().uiScale_);
            size.y_ = (int)((float)size.y_ * GameContext::Get().uiScale_);
            GameHelpers::ClampSizeTo(size, (int)(((float)slotSize_ * GameContext::Get().uiScale_) + 0.5f));
            image->SetSize(size);
        }
    }
}

void UIC_MissionPanel::UpdateObjective(unsigned index, bool dirty)
{
    URHO3D_LOGINFOF("UIC_MissionPanel() - UpdateObjective : id=%u dirty=%s", index, dirty ? "true":"false");

    const Objective& objective = mission_->GetObjectives()[index];

    String name = String("Objective_" + String(index));
    bool updateTextColor = false;
    Color textColor = Color::WHITE;

    UIElement* uiObj = static_cast<UIElement*>(objectiveZone_->GetChild(name));
    if (!uiObj)
    {
        uiObj = objectiveZone_->CreateChild<UIElement>(name);
        uiObj->SetStyle(GOA::GetAttributeName(StringHash(objective.type_)));
        uiObj->SetHorizontalAlignment(HA_CENTER);
        dirty = true;
    }

    if (dirty)
    {
        Text* text = static_cast<Text*>(uiObj->GetChild(QTY));
        text->SetText("x "+ String(objective.quantity_));

        if (objective.isMajor_)
        {
            textColor = Color::GREEN;
            updateTextColor = true;
        }

        BorderImage* image = static_cast<BorderImage*>(uiObj->GetChild(ITEM)->GetChild(0));
        Sprite2D* sprite = GameHelpers::GetSpriteForType(StringHash(objective.attribut_));
        SetImage(image, sprite, StringHash(objective.attribut_));
        uiObj->SetSize(image->GetSize());
    }
    /// Update State Mark
    if (objective.state_ > IsRunning)
    {
        Mark(uiObj->GetChild(ITEM)->GetChild(0), objective.state_);
        textColor = (objective.isMajor_ ? Color::GREEN : Color::WHITE) * 0.5f;
        updateTextColor = true;

        SetVisible(true);
    }
    /// Update Quantity
    if (objective.elapsedQty_ > 0)
    {
        Text* text = static_cast<Text*>(uiObj->GetChild(ITEM)->GetChild(ELAPSEDQTY,true));
        text->SetText(String(objective.elapsedQty_));
    }
    /// Update Text Color
    if (updateTextColor)
    {
        URHO3D_LOGINFOF("UIC_MissionPanel() - UpdateObjective : updateTextColor state=%d !", objective.state_);
        const Vector<SharedPtr<UIElement> >& children = uiObj->GetChildren();
        for (Vector<SharedPtr<UIElement> >::ConstIterator it=children.Begin(); it!=children.End(); ++it)
            it->Get()->SetColor(textColor);
    }
}

void UIC_MissionPanel::ShowRewards()
{
    const PODVector<Reward>& rewards = mission_->GetRewards();
    URHO3D_LOGINFOF("UIC_MissionPanel() - ShowRewards : numrewards = %u", rewards.Size());

    if (rewards.Size() > 1)
        rewardZone_->SetLayoutMode(LM_HORIZONTAL);
    else
        rewardZone_->SetLayoutMode(LM_FREE);

    for (PODVector<Reward>::ConstIterator it=rewards.Begin(); it!=rewards.End(); ++it)
    {
        UIElement* element = rewardZone_->GetChild(it-rewards.Begin());
        if (!element)
        {
            element = rewardZone_->CreateChild<UIElement>();
            element->SetStyle("Slot");
            element->SetAlignment(HA_CENTER, VA_CENTER);
        }

        BorderImage* image = static_cast<BorderImage*>(element->GetChild(0));
        SetImage(image, it->sprite_);

        Text* quantity = static_cast<Text*>(image->GetChild(0));
        quantity->SetText(String(it->quantity_));
    }
}

void UIC_MissionPanel::Mark(UIElement* element, int state)
{
    BorderImage* mark = static_cast<BorderImage*>(element->GetChild(MARK));
    if (!mark)
        mark = element->CreateChild<BorderImage>(MARK);

    GameHelpers::SetUIElementFrom(mark, UIEQUIPMENT, String("markcheck")+String(state - (int)IsCompleted), 40);
    mark->SetAlignment(HA_RIGHT, VA_CENTER);
}

void UIC_MissionPanel::Update()
{
    if (mission_)
    {
        const PODVector<Objective>& objectives = mission_->GetObjectives();

        int state = mission_->GetState();

        for (unsigned i=0; i <= mission_->GetObjIndex(); i++)
            UpdateObjective(i, missionDirty_);

        if (missionDirty_)
        {
            int zoneheight = objectiveZone_->GetChild(String("Objective_0"))->GetSize().y_ * objectives.Size();
            objectiveZone_->SetFixedHeight(zoneheight);
            objectiveZone_->GetParent()->SetFixedHeight(zoneheight);

            ShowRewards();
            missionDirty_ = false;
        }

        if (state == IsCompleted)
        {
            const PODVector<Reward>& rewards = mission_->GetRewards();
            tempSlotReward.Clear();
            Node* avatar = Actor::Get(manager_->GetOwner())->GetAvatar();
            if (!avatar)
                return;
            IntVector2 rPos;
            for (PODVector<Reward>::ConstIterator it=rewards.Begin(); it!=rewards.End(); ++it)
            {
                Slot slot(StringHash(it->type_), it->quantity_);
                if (it->sprite_)
                    slot.SetSprite(it->sprite_);

                tempSlotReward.Push(slot);
                Slot& slotToTransfer = tempSlotReward.Back();

                URHO3D_LOGINFOF("UIC_MissionPanel() - Transfer Reward %s(%u) qty=%u to avatar",
                                GOT::GetType(slotToTransfer.type_).CString(), slotToTransfer.type_.Value(), slotToTransfer.quantity_);

                rPos = rewardZone_->GetChild(it-rewards.Begin())->GetScreenPosition();
                GOC_Collectable::TransferSlotTo(slotToTransfer, 0, avatar, Variant(rPos));

                if (slotToTransfer.quantity_)
                {
                    URHO3D_LOGINFOF("UIC_MissionPanel() - No More Slot in Avatar => Drop Reward %s qty=%u from avatar to scene",
                                    GOT::GetType(slotToTransfer.type_).CString(), slotToTransfer.quantity_);

                    const int dropmode = !GameContext::Get().LocalMode_ ? SLOT_DROPREMAIN : SLOT_NONE;
                    Node* dropped = GOC_Collectable::DropSlotFrom(avatar, &slotToTransfer, dropmode);
                }
            }
            mission_->SetState(IsFinished);
        }

        if (state > IsRunning)
        {
            Mark(panel_->GetChild(String("ObjectiveTitle"))->GetChild(0), state);
            Text* startText = dynamic_cast<Text*>(panel_->GetChild("StateButton", true)->GetChild(0));
            startText->SetText("New");
        }

        //SetVisible(state != IsRunning);
    }
}

void UIC_MissionPanel::Clear()
{
    objectiveZone_->RemoveAllChildren();
    rewardZone_->RemoveAllChildren();
    /// Clear Marks && Rewards
    panel_->GetChild(String("ObjectiveTitle"))->GetChild(0)->RemoveAllChildren();

    if (mission_)
    {
        Text* startText = dynamic_cast<Text*>(panel_->GetChild("StateButton", true)->GetChild(0));
        if (mission_->GetState() > IsPaused)
            startText->SetText("Stop");
        else
            startText->SetText("Accept");
    }
    missionDirty_ = true;
}

void UIC_MissionPanel::OnMissionChanged(StringHash eventType, VariantMap& eventData)
{
    Mission* mission = manager_->GetMission(eventData[Go_MissionAdded::GO_MISSIONID].GetUInt());
    if (mission)
    {
        URHO3D_LOGINFOF("UIC_MissionPanel() - OnMissionChanged : mission state = %d!", mission->GetState());
        mission_ = mission;
        Clear();
        Update();
    }
}

void UIC_MissionPanel::OnMissionUpdated(StringHash eventType, VariantMap& eventData)
{
    Mission* mission = manager_->GetMission(eventData[Go_MissionUpdated::GO_MISSIONID].GetUInt());
    if (mission)
    {
        mission_ = mission;
        URHO3D_LOGINFOF("UIC_MissionPanel() - OnMissionUpdated : id=%u mission state = %d!", mission_->GetID(), mission_->GetState());
        Update();
    }
}

void UIC_MissionPanel::OnObjectiveUpdated(StringHash eventType, VariantMap& eventData)
{
    unsigned missionid = eventData[Go_ObjectiveUpdated::GO_MISSIONID].GetUInt();
    Mission* mission = manager_->GetMission(missionid);
    if (mission)
    {
        URHO3D_LOGINFOF("UIC_MissionPanel() - OnObjectiveUpdated !");
        if (mission_->GetID() != missionid)
            OnMissionChanged(eventType, eventData);
        else
            UpdateObjective(eventData[Go_ObjectiveUpdated::GO_OBJECTIVEID].GetUInt(), true);
    }
}

void UIC_MissionPanel::OnCloseButton(StringHash eventType, VariantMap& eventData)
{
//    SetVisible(false);

    UIPanel::ToggleVisible();

    if (!mission_)
        return;

    URHO3D_LOGINFOF("UIC_MissionPanel() - OnCloseButton : mission state = %d!", mission_->GetState());

    if (mission_->GetState() < IsPaused)
    {
        manager_->RemoveMission(mission_->GetID());
        manager_->NextMission();
    }
}

void UIC_MissionPanel::OnStateButton(StringHash eventType, VariantMap& eventData)
{
    SetVisible(false);

    if (!mission_ || !manager_)
        return;

    if (mission_->GetState() < IsPaused)
    {
        Text* startText = dynamic_cast<Text*>(panel_->GetChild("StateButton", true)->GetChild(0));
        startText->SetText("Stop");
    }

    if (mission_->GetState() < IsRunning)
        manager_->StartMission(mission_->GetID());

    else if (mission_->GetState() >= IsRunning)
    {
        manager_->RemoveMission(mission_->GetID());
        manager_->NextMission();
    }
}
