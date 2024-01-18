#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Light.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>
#include <Urho3D/Scene/SmoothedTransform.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>

#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Window.h>

#include <Urho3D/Input/InputEvents.h>
#include <Urho3D/Input/Input.h>

#include "DefsGame.h"
#include "DefsEffects.h"

#include "GameOptionsTest.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameNetwork.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameGoal.h"
#include "GameData.h"

#include "GOC_Animator2D.h"
#include "GOC_Controller.h"
#include "GOC_ControllerPlayer.h"
#include "GOC_ControllerAI.h"
#include "GOC_Move2D.h"
#include "GOC_Collectable.h"
#include "GOC_Inventory.h"
#include "GOC_Attack.h"
#include "GOC_Life.h"
#include "GOC_ZoneEffect.h"
#include "GOC_Destroyer.h"
#include "GOC_PhysicRope.h"

#include "TimerRemover.h"
#include "TimerInformer.h"

#include "Ability.h"
#include "Equipment.h"

#include "MapWorld.h"
#include "Map.h"
#include "ViewManager.h"
#include "MAN_Go.h"

#include "UISlotPanel.h"
#include "UIC_StatusPanel.h"
#include "UIC_BagPanel.h"
#include "UIC_EquipmentPanel.h"
#include "UIC_CraftPanel.h"
#include "UIC_ShopPanel.h"
#include "UIC_AbilityPanel.h"
#include "UIC_MissionPanel.h"
#include "UIC_JournalPanel.h"
#include "UIC_MiniMap.h"

#include "Player.h"

#define UI_COLLECTABLE_SEMITRANSFERTIME 0.3f
#define SPRITEUISIZE 80

#define XMLMODE

const char* PanelNames_[] =
{
    "StatusPanel",
    "BagPanel",
    "EquipmentPanel",
    "CraftPanel",
    "AbilityPanel",
    "DialoguePanel",
    "ShopPanel",
    "JournalPanel",
    "MissionPanel",
    "MinimapPanel"
};


Player::Player(unsigned id) :
    Actor(0, id),
    equipment_(0),
    missionManager_(0),
    faction_((unsigned)GO_Player),
    avatarIndex_(0),
    avatarAccumulatorIndex_(0)
{ }

Player::Player(Context* context, unsigned id) :
    Actor(context, id),
    equipment_(0),
    missionManager_(0),
    faction_((unsigned)GO_Player),
    avatarIndex_(0),
    avatarAccumulatorIndex_(0)
{ }

void Player::Reset(Context* context, unsigned id)
{
    URHO3D_LOGINFOF("Player() - Reset : context=%u id=%u", context, id);

    gocLife = 0;
    gocInventory = 0;
    gocController = 0;
    gocDestroyer_ = 0;
    scene_ = 0;

    if (context)
    {
        if (!equipment_)
        {
            equipment_ = new Equipment(context);
            equipment_->SetHolder(this, false);
        }
    }

    avatars_.Clear();

#ifdef ACTIVE_ALLAVATARS
    unsigned numavatars = GOT::GetControllables().Size();
    for (int i=0; i < numavatars; i++)
        AddAvatar(i);
#endif
}

Player::~Player()
{
    URHO3D_LOGINFOF("~Player() - ID=%u ...", GetID());

    //Stop();
    if (avatar_)
    {
        MountInfo mountinfo(avatar_);
        GameHelpers::UnmountNode(mountinfo);
    }

    if (equipment_)
    {
        delete equipment_;
        equipment_ = 0;
    }

    if (missionManager_)
    {
        delete missionManager_;
        missionManager_ = 0;
    }

    URHO3D_LOGINFOF("~Player() - ID=%u ... OK !", GetID());
}


/// Setters

void Player::SetPlayerName(const char *n)
{
    SetName( !n ? "Player_" + String(GetID()) : n );
}

void Player::SetMissionEnable(bool enable)
{
    URHO3D_LOGINFOF("Player() - SetMissionEnable : id=%u enable=%s", GetID(), enable ? "true":"false");

    missionEnable_ = enable;

    if (missionEnable_)
    {
        if (!missionManager_)
            missionManager_ = new MissionManager(context_);

        missionManager_->Clear();
        missionManager_->SetOwner(this->GetID());
    }
    else
    {
        if (missionManager_)
        {
            missionManager_->Stop();
            delete missionManager_;
            missionManager_ = 0;
        }
    }

    URHO3D_LOGINFOF("Player() - SetMissionEnable : id=%u enable=%s ... OK !", GetID(), missionEnable_ ? "true":"false");
}

void Player::Set(UIElement* elem, bool missionEnable, bool multiLocalPlayerMode, unsigned index, const char *name)//, const GOCProfile& profile)
{
    URHO3D_LOGINFOF("Player() - Set : ... index=%u name=%s ...", index, name);

    score = 0;

    Reset(context_, index+1);

    SetControlID(index);
    SetPlayerName(name);
    SetMissionEnable(missionEnable);

    ResetAvatar();

    SetEnableStats(true);

    CreateUI(elem, multiLocalPlayerMode);

    URHO3D_LOGINFOF("Player() - Set : missionEnable = %s ... OK ! ", missionEnable_ ? "true" : "false");
}

void Player::SetScene(Scene* scene, const Vector2& position, int viewZ, bool loadmode, bool initmode, bool restartmode, bool forceposition)
{
    if (restartmode && mainController_)
    {
        GameContext::Get().playerState_[controlID_].active = true;
        active = true;
    }

    if (!active)
    {
        URHO3D_LOGINFOF("Player() - SetScene : actorID=%u nodeID=%u controlID=%d restart=%s active=false ... ", info_.actorID_, nodeID_, controlID_, restartmode ? "true":"false");
        return;
    }

    URHO3D_LOGINFOF("---------------------------------------------------");
    URHO3D_LOGINFOF("- Player() - SetScene : player ID=%u ... ", GetID());
    URHO3D_LOGINFOF("---------------------------------------------------");

    if (!loadmode)
    {
        if (!forceposition)
        {
            // No restartmode allowed if unknown position
            if (GameContext::Get().playerState_[controlID_].position == Vector2::ZERO && GameContext::Get().playerState_[controlID_].viewZ == 0)
                restartmode = false;

            // Set Start Position if initmode or unknown state position
            if (initmode || !restartmode)
            {
                GameContext::Get().playerState_[controlID_].position.x_ = position.x_ + (controlID_-1) * 0.5f;// + (float)GameContext::Get().ClientMode_ * 1.5f;
                GameContext::Get().playerState_[controlID_].position.y_ = position.y_;// + 1.f;
                GameContext::Get().playerState_[controlID_].viewZ = viewZ;
            }

            URHO3D_LOGINFOF("Player() - SetScene : actorID=%u nodeID=%u controlID=%d restart=%s position=%s viewZ=%d ... ", info_.actorID_, nodeID_, controlID_, restartmode ? "true" : "false",
                            GameContext::Get().playerState_[controlID_].position.ToString().CString(), GameContext::Get().playerState_[controlID_].viewZ);
        }

        if (!forceposition)
            InitAvatar(scene, GameContext::Get().playerState_[controlID_].position, GameContext::Get().playerState_[controlID_].viewZ);
        else
            InitAvatar(scene, position, viewZ);

        if (!initmode || restartmode)
        {
            LoadState();
        }
    }
    else
    {
        LoadAvatar(scene);
    }

    if (connection_ && !GameContext::Get().ClientMode_)
        connection_->SetPosition(forceposition ? position : GameContext::Get().playerState_[controlID_].position);

    // it's a restart so reset Life
    if (restartmode && gocLife)
        gocLife->Reset();

    if (GameContext::Get().ServerMode_)
    {
        URHO3D_LOGINFOF("- Player() - SetScene : player ID=%u ... Set Net Equipment ...", GetID());
        GOC_Inventory::LoadInventory(avatar_, false);
        UpdateEquipment();
    }
    else
    {
        URHO3D_LOGINFOF("- Player() - SetScene : player ID=%u ... Set Local Equipment ...", GetID());
        equipment_->SetHolder(this, true);
    }

    UpdateUI();

    SaveState();

    URHO3D_LOGINFOF("---------------------------------------------------");
    URHO3D_LOGINFOF("- Player() - SetScene : player ID=%u ... OK !", GetID());
    URHO3D_LOGINFOF("---------------------------------------------------");
}

void Player::SetFaction(unsigned faction)
{
    faction_ = faction ? faction : (unsigned)GO_Player;

    if (avatar_)
        avatar_->SetVar(GOA::FACTION, faction_);

    URHO3D_LOGINFOF("---------------------------------------------------");
    URHO3D_LOGINFOF("- Player() - SetFaction : player ID=%u faction=%u", GetID(), faction_);
    URHO3D_LOGINFOF("---------------------------------------------------");
}

void Player::SetMissionWin()
{
    if (missionManager_)
        missionManager_->SetMissionToState(missionManager_->GetFirstMissionToComplete(), IsCompleted);
}

void Player::SetWorldPosition(const WorldMapPosition& position)
{
    if (avatar_)
        avatar_->GetComponent<GOC_Destroyer>()->SetWorldMapPosition(position);

    GameHelpers::SetLightActivation(this);
}

bool Player::AddAvatar(const StringHash& got)
{
    int index = GOT::GetControllableIndex(got);
    if (index == -1)
        return false;

    return AddAvatar(index);
}

bool Player::AddAvatar(int avatarIndex)
{
    if (avatarIndex >= GOT::GetControllables().Size())
        return false;

    if (!avatars_.Contains(avatarIndex))
    {
        avatars_.Push(avatarIndex);
        URHO3D_LOGINFOF("Player() - AddAvatar : numavatars=%u !", avatars_.Size());
        return true;
    }

    return false;
}

void Player::InitAvatar(Scene* root, const Vector2& position, int viewZ)
{
    if (!root)
        return;

    URHO3D_LOGINFOF("---------------------------------------");
    URHO3D_LOGINFOF("- Player() - InitAvatar : player ID=%u ...", GetID());
    URHO3D_LOGINFOF("---------------------------------------");

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, false);

    // Create avatar
    avatarIndex_ = -1;

    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU)
        GameContext::Get().playerState_[controlID_].avatar = GameRand::GetRand(OBJRAND, GOT::GetControllables().Size());

    if (!avatars_.Contains(GameContext::Get().playerState_[controlID_].avatar))
        avatars_.Push(GameContext::Get().playerState_[controlID_].avatar);

    ChangeAvatar(GameContext::Get().playerState_[controlID_].avatar, !avatar_);

    Actor::SetScene(root, position, viewZ);

    if (!avatar_)
        active = false;

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);

#ifdef PLAYER_ACTIVEINITIALSTUFF
    GOC_Inventory::LoadInventory(avatar_, false);
#endif

    URHO3D_LOGINFOF("---------------------------------------------------------");
    URHO3D_LOGINFOF("- Player() - InitAvatar : player ID=%u nodeID=%u active=%s avatarindex=%d position=%s viewZ=%d ... OK!",
                    GetID(), nodeID_, active ? "true":"false", avatarIndex_, avatar_ ? avatar_->GetWorldPosition2D().ToString().CString() : "", GetViewZ());

    URHO3D_LOGINFOF("---------------------------------------------------------");
}

bool Player::ChangeAvatar(unsigned type, unsigned char entityid, bool instant)
{
    if (avatar_ && avatar_->GetVar(GOA::ISDEAD).GetBool())
        return false;

    int avatarIndex = GOT::GetControllableIndex(StringHash(type));

    if (avatarIndex_ != avatarIndex)
    {
        if (avatarIndex == -1)
            return false;

        avatarIndex_ = avatarAccumulatorIndex_ = avatarIndex;
        GameContext::Get().playerState_[controlID_].avatar = avatarIndex_;
        dirtyPlayer_ = true;
        info_.entityid_ = entityid;

        if (instant)
            UpdateAvatar();

        return true;
    }

    return false;
}

void Player::ChangeAvatar(int avatarIndex, bool instant)
{
    if (avatar_ && avatar_->GetVar(GOA::ISDEAD).GetBool())
        return;

    if (avatarIndex == avatarIndex_)
    {
        URHO3D_LOGERRORF("Player() - ChangeAvatar : player ID=%u avatarindex=%d same avatar than previous !", GetID(), avatarIndex_);
        return;
    }

    if (!avatars_.Contains(avatarIndex))
    {
        URHO3D_LOGERRORF("Player() - ChangeAvatar : player ID=%u avatarindex=%d not owned !", GetID(), avatarIndex_);
        return;
    }

    if (avatarIndex != avatarIndex_)
    {
        avatarIndex_ = avatarAccumulatorIndex_ = avatarIndex;
        GameContext::Get().playerState_[controlID_].avatar = avatarIndex_;
        dirtyPlayer_ = true;
        info_.entityid_ = 0;

        if (instant)
            UpdateAvatar();
    }
}


void Player::UpdateAvatar(bool forced)
{
    if (!forced && !dirtyPlayer_)
        return;

    if (gocController)
    {
        gocController->ResetDirection();
        gocController->control_.SetButtons(CTRL_FIRE3, false);
    }

    const StringHash& type = GOT::GetControllableType(avatarIndex_);

    if (type == StringHash::ZERO)
        return;

    URHO3D_LOGERRORF("Player() - UpdateAvatar : avatarIndex_=%d => change to avatar type=%s(%u) ...", avatarIndex_, GOT::GetType(type).CString(), type.Value());

//    URHO3D_LOGINFOF("Player() - UpdateAvatar : ... Skip Logs !");
//    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, false);

    WorldMapPosition initialposition;
    if (avatar_ && gocDestroyer_)
    {
        initialposition = gocDestroyer_->GetWorldMapPosition();
    }

    // Unmount the avatar if mounted
    MountInfo mountinfo(avatar_);
    GameHelpers::UnmountNode(mountinfo);

    SaveState();

//    if (avatar_)
//        GameHelpers::DumpNode(avatar_->GetID(), true);

    if (equipment_)
    {
        equipment_->SaveActiveAbility();
        equipment_->SetDirty(true);
    }

    Actor::SetViewZ(initialposition.viewZ_);
    Actor::SetObjectType(type, info_.entityid_);

//    if (avatar_)
//        GameHelpers::DumpNode(avatar_->GetID(), true);

    Actor::Start(false, initialposition);

    // Remount the avatar on the mount ad other entities mounted on avatar_
    GameHelpers::MountNode(mountinfo);

    dirtyPlayer_ = false;

    ResetUI();

    UIPanel* panel = GetPanel(STATUSPANEL);
    if (panel)
        panel->Update();

    if (avatar_)
    {
        bool lightstate = GameHelpers::SetLightActivation(this);

        // Spawn Effect
        Drawable2D* drawable = avatar_->GetDerivedComponent<Drawable2D>();
        GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_LIFEFLAME], drawable->GetLayer(), drawable->GetViewMask(), avatar_->GetWorldPosition2D(), 0.f, 1.f, true, 2.f, Color::BLUE, LOCAL);

//        drawable->enableDebugLog_ = true;

        avatar_->AddTag("Player");

        if (equipment_)
            equipment_->SetActiveAbility();
    }

//    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);

    URHO3D_LOGINFOF("Player() - UpdateAvatar : avatarIndex_=%d => change to avatar type=%s(%u) avatar=%s(%u) viewmask=%u ... OK !",
                    avatarIndex_, GOT::GetType(type).CString(), type.Value(), avatar_ ? avatar_->GetName().CString() : "none", avatar_ ? avatar_->GetID() : 0, avatar_ ? avatar_->GetDerivedComponent<AnimatedSprite2D>()->GetViewMask() : 0);
}

void Player::UpdateEquipment()
{
    if (equipment_)
        equipment_->Update(false);
}

void Player::UpdateControls(bool restartcontroller)
{
    if (!avatar_)
        return;

    int controltype = GameContext::Get().playerState_[controlID_].controltype;

    if (gocController && restartcontroller)
    {
        gocController->Stop();
        avatar_->SetEnabled(false);
    }

    if (controltype != CT_CPU)
    {
        GOC_PlayerController* gocPlayer = avatar_->GetComponent<GOC_PlayerController>(LOCAL);
        if (!gocPlayer)
        {
            gocPlayer = new GOC_PlayerController(context_);
            avatar_->AddComponent(gocPlayer, 0, LOCAL);
        }

        gocPlayer->playerID = controlID_;
        gocController = gocPlayer;

        if (controltype == CT_KEYBOARD)
        {
            gocPlayer->SetKeyControls(controlID_);
            URHO3D_LOGERRORF("Player() - UpdateControls : ID=%d ControlID=%d => keyboard=%d !", GetID(), controlID_, controlID_);
        }

        if (controltype == CT_JOYSTICK)
        {
            // select the nearest index != -1, by beginning at controlID_
            int controlid = controlID_;
            while (controlid > -1)
            {
                if (GameContext::Get().joystickIndexes_[controlid] != -1)
                {
                    if (controlid == controlID_)
                        break;

                    if (GameContext::Get().playerState_[controlid].controltype != CT_JOYSTICK)
                        break;
                }
                controlid--;
            }

            if (controlid == -1)
            {
                URHO3D_LOGERRORF("Player() - UpdateControls : ID=%d ControlID=%d No Joystick => use Keyboard !");
                gocPlayer->SetKeyControls(controlID_);
            }
            else
            {
                gocPlayer->SetJoystickControls(controlid);
                URHO3D_LOGERRORF("Player() - UpdateControls : ID=%d ControlID=%d => joystickid=%d !", GetID(), controlid, GameContext::Get().joystickIndexes_[controlid]);
            }
        }

        if (GameContext::Get().gameConfig_.touchEnabled_ || GameContext::Get().gameConfig_.forceTouch_)
        {
            if (controltype == CT_SCREENJOYSTICK)
            {
                if (GameContext::Get().gameConfig_.screenJoystickID_ != -1)
                {
                    gocPlayer->SetTouchControls(GameContext::Get().gameConfig_.screenJoystickID_);
                    UIElement* uiscreenjoy = GameContext::Get().input_->GetJoystick(GameContext::Get().gameConfig_.screenJoystickID_)->screenJoystick_;
                    uiscreenjoy->SetVisible(true);
                    URHO3D_LOGERRORF("Player() - UpdateControls : ID=%d ControlID=%d => screenjoystick=%d !", GetID(), controlID_, GameContext::Get().gameConfig_.screenJoystickID_);
                }
            }
            else if (controltype == CT_TOUCH)
            {
                gocPlayer->SetTouchControls(-1);
                if (GameContext::Get().gameConfig_.screenJoystickID_ != -1)
                {
                    UIElement* uiscreenjoy = GameContext::Get().input_->GetJoystick(GameContext::Get().gameConfig_.screenJoystickID_)->screenJoystick_;
                    uiscreenjoy->SetVisible(false);
                }
                URHO3D_LOGERRORF("Player() - UpdateControls : ID=%d ControlID=%d => touchscreen !", GetID(), controlID_);
            }
        }
    }
    else
    {
        GOC_AIController* aicontroller = avatar_->GetComponent<GOC_AIController>(LOCAL);
        if (!aicontroller)
        {
            aicontroller = new GOC_AIController(context_);
            avatar_->AddComponent(aicontroller, 0, LOCAL);
        }

        gocController = aicontroller;

        if (GameContext::Get().arenaZoneOn_)
        {
            aicontroller->SetControllerType(GO_AI);
            aicontroller->SetBehaviorAttr("PlayerCPU");
//            aicontroller->SetTarget(GameContext::Get().playerAvatars_[0] ? GameContext::Get().playerNodes_[0] : 0);
        }
        else
        {
            aicontroller->SetControllerType(GO_AI_Ally);
            aicontroller->SetTarget(GameContext::Get().playerAvatars_[0] ? GameContext::Get().playerNodes_[0] : 0);
            aicontroller->SetBehavior(GOB_FOLLOW);
        }

        aicontroller->SetAlwaysUpdate(true);

        URHO3D_LOGINFOF("Player() - UpdateControls :  ID=%d ControlID=%d mainController => set ai controller", GetID(), controlID_);
    }

    gocController->SetMainController(true);
    gocController->SetThinker(this);

    if (restartcontroller)
    {
        // needed when change of type between cpu and other type
        // because of use of GOC_Controller derived component in other components like GOC_Animator2D
        avatar_->ApplyAttributes();
        avatar_->SetEnabled(true);
    }
}

void Player::UpdateComponents()
{
    URHO3D_LOGINFOF("Player() - UpdateComponents ...");

    Actor::UpdateComponents();

//    AnimatedSprite2D* drawable = avatar_->GetDerivedComponent<AnimatedSprite2D>();
//    drawable->SetColor(GameContext::Get().playerColor_[ID]);

    gocDestroyer_ = avatar_->GetComponent<GOC_Destroyer>();
    if (gocDestroyer_)
    {
        gocDestroyer_->SetDestroyMode(GameContext::Get().LocalMode_ ? FREEMEMORY : DISABLE);
        gocDestroyer_->UpdatePositions();
    }
    else
    {
        URHO3D_LOGERROR("Player() - UpdateComponents : !gocDestroyer_");
    }

    if (!avatar_)
    {
        URHO3D_LOGERRORF("Player() - UpdateComponents : avatar has been destroyed !");
        return;
    }

    gocLife = avatar_->GetComponent<GOC_Life>();
    if (!gocLife)
        URHO3D_LOGERROR("Player() - UpdateComponents : !gocLife");

    // local player
    if (mainController_)
    {
        GameContext::Get().players_[controlID_] = this;
        GameContext::Get().playerNodes_[controlID_] = nodeID_;
        GameContext::Get().playerAvatars_[controlID_] = avatar_;

        UpdateControls();

        UpdateTriggerAttacks();
    }
    else
    {
        URHO3D_LOGINFOF("Player() - UpdateComponents : NO mainController => net player ! ");
        gocController = avatar_->GetOrCreateComponent<GOC_Controller>(LOCAL);
        gocController->SetControllerType(GO_NetPlayer, true);
        gocController->SetThinker(this);
//        if (gocInventory)
//            gocInventory->SetReceiveTriggerEvent(GOE::GetEventName(GO_INVENTORYRECEIVE));
    }

    avatarIndex_ = avatarAccumulatorIndex_ = GameContext::Get().playerState_[controlID_].avatar;
    gocController->control_.type_ = GOT::GetControllableType(avatarIndex_).Value();

    gocInventory = avatar_->GetOrCreateComponent<GOC_Inventory>(LOCAL);
    if (gocInventory)
    {
        if (!gocInventory->HasTemplate())
            gocInventory->SetTemplate("InventoryTemplate_MoneySlot_15SlotsQ20_Equip1");
        else
            gocInventory->ResetTemplate(gocInventory->GetTemplate());

        URHO3D_LOGINFOF("Player() - UpdateComponents : gocInventory^=%u use template=%s(%u)", gocInventory, gocInventory->GetTemplateName().CString(), gocInventory->GetTemplate());
    }
    else
    {
        URHO3D_LOGERROR("Player() - UpdateComponents : !gocInventory");
    }

    SmoothedTransform* smooth = avatar_->GetOrCreateComponent<SmoothedTransform>(LOCAL);

    avatar_->SetVar(GOA::FACTION, faction_);

    active = true;

    if (equipment_)
    {
        equipment_->SetHolder(this, equipment_->IsDirty());
    }

    avatar_->GetComponents<Light>(lights_, true);

    URHO3D_LOGINFOF("Player() - UpdateComponents ... OK !");
}

void Player::UpdateTriggerAttacks()
{
    // create or update the trigger Attack.
    AnimatedSprite2D* animatedSprite = avatar_->GetComponent<AnimatedSprite2D>();
    if (!animatedSprite)
        return;

    // work only for one trigger TRIGATTACK for the moment
    // TODO : RockGolem has 2 triggers attack.

    CollisionCircle2D* triggerAttack = 0;
    Node* triggerAttackNode = avatar_->GetChild(TRIGATTACK);
    if (!triggerAttackNode)
    {
        triggerAttackNode = avatar_->CreateChild(TRIGATTACK, LOCAL);
        triggerAttackNode->isPoolNode_ = avatar_->isPoolNode_;
        triggerAttackNode->SetChangeModeEnable(false);
        triggerAttackNode->SetTemporary(true);

        triggerAttack = triggerAttackNode->CreateComponent<CollisionCircle2D>(LOCAL);
        triggerAttack->SetChangeModeEnable(false);

        animatedSprite->AddPhysicalNode(triggerAttackNode);
    }

    if (!triggerAttack)
        triggerAttack = triggerAttackNode->GetComponent<CollisionCircle2D>(LOCAL);

    if (triggerAttack)
    {
        triggerAttack->SetFilterBits(CC_OUTSIDEAVATAR, CM_OUTSIDEAVATAR_DETECTOR);
        triggerAttack->SetTrigger(false);
    }

    triggerAttackNode->SetEnabled(false);
}

void Player::UpdateZones()
{
    if (gocDestroyer_->GetCurrentMap())
        EffectAction::Update(GameContext::Get().gameConfig_.multiviews_ ? controlID_ : 0, gocDestroyer_->GetCurrentMap(), gocDestroyer_->GetWorldMapPosition(), zone_);
}

void Player::UseWeaponAbilityAt(const StringHash& abi, const Vector2& position)
{
    if (!equipment_)
        return;

    float time = GameContext::Get().time_->GetElapsedTime();

    if (time - lastTime_ > 0.6f)
    {
        bool success = false;

        for (int i=1; i <= 2; i++)
        {
            const StringHash& ability = equipment_->GetWeaponAbility(i);
            if (ability != abi)
                continue;

            URHO3D_LOGINFOF("Player() - UseWeaponAbilityAt : position=%s ... weapon%dability=%u", position.ToString().CString(), i, ability.Value());

            if (ability && Ability::Get(ability) && Ability::Get(ability)->UseAtPoint(position, avatar_))
                success = true;
        }

        if (success)
        {
//            URHO3D_LOGINFOF("Player() - UseWeaponAbilityAt : position=%s !", position.ToString().CString());
            lastTime_ = time;
        }
    }
}


void Player::SwitchViewZ(int newViewZ)
{
    if (!avatar_)
        return;

    if (!gocDestroyer_)
    {
        URHO3D_LOGERROR("Player() - SwitchViewZ : no gocDestroyer !");
        return;
    }

    const WorldMapPosition& wmPosition = gocDestroyer_->GetWorldMapPosition();
    if (!wmPosition.viewZ_)
        return;

    if (!newViewZ)
        newViewZ = (wmPosition.viewZ_ == FRONTVIEW ? INNERVIEW : FRONTVIEW);

    if (newViewZ != wmPosition.viewZ_)
    {
        // Check if not blocked
        if (gocDestroyer_->IsOnFreeTiles(newViewZ))
        {
            // Send Event to ViewManager
            URHO3D_LOGINFOF("Player() - SwitchViewZ : ID=%u from=%d to=%d !", GetID(), wmPosition.viewZ_, newViewZ);
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Go_Detector::GO_GETTER] = avatar_->GetID();
            avatar_->SendEvent(newViewZ == INNERVIEW ? GO_DETECTORSWITCHVIEWIN : GO_DETECTORSWITCHVIEWOUT, eventData);
        }
    }
}


void Player::OnMount(Node* target)
{
    if (!avatar_ || !target)
        return;

    // if Player only (not netplayer who doesn't need to set these attributes)
    if (gocController->GetControllerType() == GO_Player)
    {
        gocDestroyer_->SetEnableUnstuck(false);
        gocDestroyer_->SetEnablePositionUpdate(false);
    }

    GOC_Inventory* inventory = avatar_->GetComponent<GOC_Inventory>();
    if (inventory)
        inventory->SetEnabled(false);

    avatar_->GetDerivedComponent<GOC_Controller>()->SetControlActionEnable(false);

    // for the target set the External Controller.
    GOC_AIController* targetcontroller = target->GetComponent<GOC_AIController>();
    if (targetcontroller)
        targetcontroller->SetExternalController(avatar_->GetDerivedComponent<GOC_Controller>());

    URHO3D_LOGINFOF("Player() - OnMount : Player=%s(%u) Mount on Target=%s(%u) position=%s worldposition=%s... OK !",
                     avatar_->GetName().CString(), avatar_->GetID(), target->GetName().CString(), target->GetID(),
                     avatar_->GetPosition().ToString().CString(), avatar_->GetWorldPosition().ToString().CString());
}

void Player::OnUnmount(unsigned targetid)
{
    if (!avatar_)
        return;

    URHO3D_LOGINFOF("Player() - OnUnmount : ...");

    GOC_Inventory* inventory = avatar_->GetComponent<GOC_Inventory>();
    if (inventory)
        inventory->SetEnabled(true);

    // if Player only (not netplayer who need to keep these attributes desactivated)
    if (gocController->GetControllerType() == GO_Player)
    {
        gocDestroyer_->SetEnablePositionUpdate(true);
        gocDestroyer_->SetEnableUnstuck(true);
    }

    avatar_->GetDerivedComponent<GOC_Controller>()->SetControlActionEnable(true);

    // for the target reset the External Controller.
    if (targetid)
    {
        Node* target = GetScene()->GetNode(targetid);
        GOC_AIController* targetcontroller = target ? target->GetComponent<GOC_AIController>() : 0;
        if (targetcontroller)
            targetcontroller->SetExternalController(0);
    }

    URHO3D_LOGINFOF("Player() - OnUnmount : ... OK !");
}


/// Getters

int Player::GetViewZ() const
{
    if (!avatar_)
        return 0;

    if (!gocDestroyer_)
    {
        URHO3D_LOGERROR("Player() - GetViewZ : no gocDestroyer !");
        return 0;
    }

    return gocDestroyer_->GetWorldMapPosition().viewZ_;
}

const WorldMapPosition& Player::GetWorldMapPosition() const
{
    assert(avatar_);
    return gocDestroyer_->GetWorldMapPosition();
}

const PODVector<Light*>& Player::GetLights() const
{
    return lights_;
}


void Player::Dump() const
{
    if (avatar_)
    {
        avatar_->GetComponent<GOC_Move2D>()->Dump();
        gocInventory->Dump();
    }

    if (equipment_)
        equipment_->Dump();

    GetStats()->DumpAllStats();

    if (avatar_ && !GameContext::Get().LocalMode_)
    {
        const ObjectControlInfo* cinfo = GameNetwork::Get()->GetObjectControl(avatar_->GetID());
        if (cinfo)
            URHO3D_LOGINFOF("Player() - Dump() : NetState active=%s isEnable=%s", cinfo->active_ ? "true":"false", cinfo->IsEnable() ? "true":"false");
    }
}


/// Serialize

void Player::LoadAvatar(Scene* root)
{
    if (!root)
        return;

    URHO3D_LOGINFOF("---------------------------------------------------------");
    URHO3D_LOGINFOF("Player() - LoadAvatar : player ID=%u ... ", GetID());
    URHO3D_LOGINFOF("---------------------------------------------------------");

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, false);

    Node* nodeTemp = root->CreateChild(String::EMPTY, LOCAL);
    GameHelpers::LoadNodeXML(context_, nodeTemp, String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedPlayerFile_[controlID_]).CString(), LOCAL);

    // Don't Allow the update of the equipment in UpdateComponents
    equipment_->SetDirty(false);

    // Create avatar
    avatars_.Clear();
    LoadState();
    ChangeAvatar(GameContext::Get().playerState_[controlID_].avatar, false);

    Actor::SetObjectType(GOT::GetControllableType(avatarIndex_));
    Actor::SetScene(root, nodeTemp->GetWorldPosition2D(), nodeTemp->GetVar(GOA::ONVIEWZ).GetInt());

    if (avatar_)
    {
        avatar_->SetEnabled(false);

        // Load States
        LoadStateFrom(nodeTemp);

        // Load Missions
        URHO3D_LOGERRORF("Player() - LoadAvatar : player ID=%u ... missionEnable=%s (manager=%u)", GetID(), missionEnable_? "true":"false", missionManager_);
        if (missionManager_ && missionEnable_)
        {
#ifdef XMLMODE
            String ext = ".xml";
#else
            String ext = ".dat";
#endif

            File f(context_, String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedPlayerMissionFile_[controlID_] + ext).CString(), FILE_READ);

#ifdef XMLMODE
            missionManager_->LoadXML(f);
#else
            missionManager_->Load(f);
#endif

            f.Close();

            missionManager_->Start();
        }

//        if (World2D::GetWorldInfo())
//            avatar_->SendEvent(WORLD_ENTITYCREATE);

        GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);

        UpdateUI();

        URHO3D_LOGINFOF("---------------------------------------------------------");
        URHO3D_LOGINFOF("Player() - LoadAvatar : player ID=%u avatarindex=%d ... OK !", GetID(), avatarIndex_);
        URHO3D_LOGINFOF("---------------------------------------------------------");
    }
    else
    {
        GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);

        URHO3D_LOGINFOF("---------------------------------------------------------");
        URHO3D_LOGINFOF("Player() - LoadAvatar : player ID=%u avatarindex=%d ... FAILED !", GetID(), avatarIndex_);
        URHO3D_LOGINFOF("---------------------------------------------------------");
        active = false;
    }

    nodeTemp->Remove();
}

void Player::LoadState()
{
    const GameContext::PlayerState& playerstate = GameContext::Get().playerState_[controlID_];

    // Set Unblocked Avatars & current avatar index
    if (!avatars_.Size())
    {
        for (unsigned avatarindex=0; avatarindex < 32; avatarindex++)
        {
            if ((playerstate.unblockedAvatars_ & (1 << avatarindex)) != 0)
                avatars_.Push(avatarindex);
        }

        if (!avatars_.Contains(playerstate.avatar))
            avatars_.Push(playerstate.avatar);
    }

    if (active && avatar_ && mainController_)
    {
        URHO3D_LOGINFOF("Player() - LoadState : player ID=%u active=%s", GetID(), active ? "true" : "false");

        gocLife = avatar_->GetComponent<GOC_Life>();
        gocInventory = avatar_->GetComponent<GOC_Inventory>();

        if (gocLife)
            gocLife->Set(playerstate.energyLost, playerstate.invulnerability);

        if (GameContext::Get().ClientMode_ && GOC_Inventory::IsNetworkInventoryAvailable(avatar_))
        {
            GOC_Inventory::LoadInventory(avatar_, false);
        }
        else
        {
            if (gocInventory && GameContext::Get().playerInventory_[controlID_].Size() > 0)
                gocInventory->Set(GameContext::Get().playerInventory_[controlID_]);

            if (equipment_)
            {
                URHO3D_LOGINFOF("Player() - LoadState : player ID=%u update equipment ...", GetID());
                equipment_->Update(!GameContext::Get().ClientMode_);
                URHO3D_LOGINFOF("Player() - LoadState : player ID=%u update equipment ... OK !", GetID());
            }
        }
    }
}

void Player::LoadStateFrom(Node* node)
{
    URHO3D_LOGINFOF("Player() - LoadStateFrom : player %u ... ", GetID());

    GameContext::PlayerState& playerstate = GameContext::Get().playerState_[controlID_];

    const LifeProps& props = node->GetComponent<GOC_Life>()->Get();
    playerstate.energyLost = props.totalDpsReceived;
    playerstate.invulnerability  = props.invulnerability;
    GameContext::Get().playerInventory_[controlID_] = node->GetComponent<GOC_Inventory>()->Get();

    if (node->GetComponent<RigidBody2D>() && World2D::IsInsideWorldBounds(node->GetWorldPosition2D()))
    {
        playerstate.position = node->GetWorldPosition2D();
        playerstate.viewZ = node->GetVar(GOA::ONVIEWZ).GetInt();
    }
    else
    {
        playerstate.position = Vector2::ZERO;
        playerstate.viewZ = 0;
    }

    LoadState();

    URHO3D_LOGINFOF("Player() - LoadStateFrom : player %u ... OK ! ", GetID());
}

void Player::SaveState()
{
    if (active && avatar_ && mainController_)
    {
        if (!avatar_->GetComponent<GOC_Life>() || !avatar_->GetComponent<GOC_Inventory>())
            return;

        GameContext::PlayerState& playerstate = GameContext::Get().playerState_[controlID_];

        URHO3D_LOGINFOF("Player() - SaveState : player %u ...", GetID());

        const LifeProps& props = avatar_->GetComponent<GOC_Life>()->Get();
        playerstate.energyLost = props.totalDpsReceived;
        playerstate.invulnerability  = props.invulnerability;

        GameContext::Get().playerInventory_[controlID_] = avatar_->GetComponent<GOC_Inventory>()->Get();

        if (avatar_->GetComponent<RigidBody2D>() && World2D::IsInsideWorldBounds(avatar_->GetWorldPosition2D()))
        {
            playerstate.position = avatar_->GetWorldPosition2D();
            playerstate.viewZ = avatar_->GetVar(GOA::ONVIEWZ).GetInt();
        }
        else
        {
            playerstate.position = Vector2::ZERO;
            playerstate.viewZ = 0;
        }

        playerstate.unblockedAvatars_ = 0;
        for (unsigned i=0; i < avatars_.Size(); i++)
            playerstate.unblockedAvatars_ += (1 << avatars_[i]);

//        URHO3D_LOGINFOF("Player() - SaveState : player %u ... OK !", GetID());
    }
}

void Player::SaveAll()
{
    URHO3D_LOGINFOF("Player() - SaveAll : player %u active=%u", GetID(), active);

    SaveState();

    // Save node in xml file, include the inventory
    avatar_->SetTemporary(false);
    GameHelpers::SaveNodeXML(context_, avatar_, String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedPlayerFile_[controlID_]).CString());
    avatar_->SetTemporary(true);

    // Save Missions
    if (missionManager_ && missionEnable_)
    {
        missionManager_->Stop();

        String ext = ".dat";
#ifdef XMLMODE
        ext = ".xml";
#endif

        File f(context_, String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedPlayerMissionFile_[controlID_] + ext).CString(), FILE_WRITE);

#ifdef XMLMODE
        if (!missionManager_->SaveXML(f))
#else
        if (!missionManager_->Save(f))
#endif
            URHO3D_LOGERRORF("Player() - SaveAll() : player %u active=%u - Error saving !", GetID(), active);

        missionManager_->Start();
    }

//    URHO3D_LOGINFOF("Player() - SaveAll : player %u active=%u ... OK !", ID, active);
}


void Player::CreateUI(UIElement* root, bool multiLocalPlayerMode)
{
    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    uiStatusBar = root;
    if (!uiStatusBar)
        return;

    URHO3D_LOGINFOF("Player() - CreateUI : id=%u ... uiStatusBar=%u ...", GetID(), uiStatusBar.Get());
    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();

    // Main Panel
    int barpositiony = 0;

#ifdef HANDLE_SCORES
    scoreText = static_cast<Text*>(uiStatusBar->GetChild(String("playerscore")));
    if (!scoreText)
    {
        scoreText = uiStatusBar->CreateChild<Text>();
        scoreText->SetName("playerscore");
        scoreText->SetTextAlignment(HA_CENTER);
        scoreText->SetFont(cache->GetResource<Font>(GameContext::Get().txtMsgFont_), 26);
        scoreText->SetColor(C_BOTTOMLEFT, Color(1, 0.25, 0.25));
        scoreText->SetColor(C_BOTTOMRIGHT, Color(1, 0.25, 0.25));
        scoreText->SetText("00");

        barpositiony = 30;
    }
#endif

    // Create Panels if not exist

    UIPanel* statusPanel = GetPanel(STATUSPANEL);
    if (!statusPanel)
    {
        statusPanel = UIPanel::GetPanel(String("PlayerStatus")+String(GetID()));
        bool panelsetted = statusPanel != 0;
        if (!statusPanel)
        {
            statusPanel = new UIC_StatusPanel(context_);
            panelsetted = statusPanel->Set(STATUSPANEL, String("PlayerStatus")+String(GetID()), "UI/PlayerStatus.xml", uiStatusBar->GetPosition(), uiStatusBar->GetHorizontalAlignment(), VA_TOP, 0.99f);
        }
        if (panelsetted)
        {
            panels_[STATUSPANEL] = statusPanel;
            GameHelpers::SetEnableScissor(statusPanel->GetElement(), false);
        }
    }

    UIPanel* bagPanel = GetPanel(BAGPANEL);
    if (!bagPanel)
    {
        bagPanel = UIPanel::GetPanel(String("PlayerBag")+String(GetID()));
        bool panelsetted = bagPanel != 0;
        if (!bagPanel)
        {
            bagPanel = new UIC_BagPanel(context_);
            IntRect rect = statusPanel->GetElement()->GetCombinedScreenRect();
            panelsetted = bagPanel->Set(BAGPANEL, String("PlayerBag")+String(GetID()), "UI/PlayerInventory.xml", IntVector2(rect.left_+130, rect.top_+10), uiStatusBar->GetHorizontalAlignment(), VA_TOP, 0.8f);
        }
        if (panelsetted)
        {
            panels_[BAGPANEL] = WeakPtr<UIPanel>(bagPanel);
            GameHelpers::SetEnableScissor(bagPanel->GetElement(), false);
        }
    }

    UIPanel* panel = GetPanel(EQUIPMENTPANEL);
    if (!panel)
    {
        panel = UIPanel::GetPanel(String("PlayerEqp")+String(GetID()));
        bool panelsetted = panel != 0;
        if (!panel)
        {
            panel = new UIC_EquipmentPanel(context_);
            IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
            panelsetted = panel->Set(EQUIPMENTPANEL, String("PlayerEqp")+String(GetID()), "UI/PlayerEquipment.xml", IntVector2(rect.right_+5, rect.top_), HA_LEFT, VA_TOP, 0.8f);
        }
        if (panelsetted)
        {
            panels_[EQUIPMENTPANEL] = WeakPtr<UIPanel>(panel);
            GameHelpers::SetEnableScissor(panel->GetElement(), false);
        }
    }

    panel = GetPanel(CRAFTPANEL);
    if (!panel)
    {
        panel = UIPanel::GetPanel(String("PlayerCraft")+String(GetID()));
        bool panelsetted = panel != 0;
        if (!panel)
        {
            panel = new UIC_CraftPanel(context_);
            IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
            panelsetted = panel->Set(CRAFTPANEL, String("PlayerCraft")+String(GetID()), "UI/PlayerCraft.xml", IntVector2(rect.left_, rect.bottom_+5), HA_LEFT, VA_TOP, 0.8f);
        }
        if (panelsetted)
        {
            panels_[CRAFTPANEL] = WeakPtr<UIPanel>(panel);
            GameHelpers::SetEnableScissor(panel->GetElement(), false);
        }
    }

    panel = GetPanel(ABILITYPANEL);
    if (!panel)
    {
        String panelname(String("PlayerAbi")+String(GetID()));
        UIC_AbilityPanel* abilityPanel = static_cast<UIC_AbilityPanel*>(UIPanel::GetPanel(panelname));
        if (abilityPanel && abilityPanel->IsPopup() != multiLocalPlayerMode)
        {
            // Recreate the panel if no same mode
            UIPanel::RemovePanel(abilityPanel);
            abilityPanel = 0;
        }
        bool panelsetted = abilityPanel != 0;
        if (!abilityPanel)
        {
            abilityPanel = new UIC_AbilityPanel(context_);
            if (multiLocalPlayerMode)
            {
                const IntVector2& parentsize = statusPanel->GetElement()->GetSize();
                panelsetted = abilityPanel->Set(ABILITYPANEL, panelname, "UI/PlayerAbilityPopup.xml", IntVector2(parentsize.x_, parentsize.y_/2), HA_LEFT, VA_TOP, 0.8f,
                                        statusPanel->GetElement()->GetChild(String("AbilitySlot")));
            }
            else
                panelsetted = abilityPanel->Set(ABILITYPANEL, panelname, "UI/PlayerAbility.xml", IntVector2(-GetSubsystem<Graphics>()->GetWidth()/6, 0), HA_CENTER, VA_TOP, 0.8f);
        }
        if (panelsetted)
        {
            abilityPanel->SetPopup(multiLocalPlayerMode);
            panels_[ABILITYPANEL] = WeakPtr<UIPanel>(abilityPanel);
        }
    }

    panel = GetPanel(DIALOGUEPANEL);
    if (!panel)
    {
        panel = UIPanel::GetPanel(String("PlayerDiag")+String(GetID()));
        bool panelsetted = panel != 0;
        if (!panel)
        {
            panel = new UIPanel(context_);
            panelsetted = panel->Set(DIALOGUEPANEL, String("PlayerDiag")+String(GetID()), "UI/PlayerDialogue.xml", IntVector2::ZERO, HA_CENTER, VA_BOTTOM, 0.8f);
        }
        if (panelsetted)
            panels_[DIALOGUEPANEL] = WeakPtr<UIPanel>(panel);
    }

    panel = GetPanel(SHOPPANEL);
    if (!panel)
    {
        panel = UIPanel::GetPanel(String("PlayerShop")+String(GetID()));
        bool panelsetted = panel != 0;
        if (!panel)
        {
            panel = new UIC_ShopPanel(context_);
            IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
            panelsetted = panel->Set(SHOPPANEL, String("PlayerShop")+String(GetID()), "UI/PlayerShop.xml", IntVector2(rect.right_+10, rect.top_), HA_LEFT, VA_TOP, 0.8f);
        }
        if (panelsetted)
        {
            panels_[SHOPPANEL] = WeakPtr<UIPanel>(panel);
            GameHelpers::SetEnableScissor(panel->GetElement(), false);
        }
    }

    panel = GetPanel(JOURNALPANEL);
    if (!panel)
    {
        panel = UIPanel::GetPanel(String("PlayerJournal")+String(GetID()));
        bool panelsetted = panel != 0;
        if (!panel)
        {
            panel = new UIC_JournalPanel(context_);
            IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
            panelsetted = panel->Set(JOURNALPANEL, String("PlayerJournal")+String(GetID()), "UI/PlayerJournal.xml", IntVector2(rect.right_+10, rect.top_), HA_LEFT, VA_TOP, 0.8f);
        }
        if (panelsetted)
            panels_[JOURNALPANEL] = WeakPtr<UIPanel>(panel);
    }

    panel = GetPanel(MISSIONPANEL);
    if (missionEnable_ && !panel)
    {
        // Mission Panel only for Player Human control
        panel = UIPanel::GetPanel(String("PlayerMis")+String(GetID()));
        bool panelsetted = panel != 0;
        if (!panel)
        {
            panel = new UIC_MissionPanel(context_);
            IntRect rect = statusPanel->GetElement()->GetCombinedScreenRect();
            panelsetted = panel->Set(MISSIONPANEL, String("PlayerMis")+String(GetID()), "UI/PlayerMission.xml", IntVector2(rect.right_+10, rect.top_), HA_LEFT, VA_TOP, 0.8f);
        }
        if (panelsetted)
        {
            panels_[MISSIONPANEL] = WeakPtr<UIPanel>(panel);
            GameHelpers::SetEnableScissor(panel->GetElement(), false);
        }
    }
    else if (!missionEnable_ && panel)
    {
        panel->UnsubscribeFromEvent(statusPanel->GetElement()->GetChild("QuestButton", true), E_RELEASED);
        panels_.Erase(MISSIONPANEL);
    }

    // Connect Panels/Events
    if (statusPanel)
        statusPanel->Start(this);

    if (bagPanel)
    {
        bagPanel->Start(this, equipment_);
        bagPanel->SubscribeToEvent(statusPanel->GetElement()->GetChild("BagButton", true), E_RELEASED, new Urho3D::EventHandlerImpl<UIPanel>(bagPanel, &UIPanel::OnSwitchVisible));
    }

    panel = GetPanel(EQUIPMENTPANEL);
    if (panel)
    {
        panel->Start(this, equipment_);
        panel->SubscribeToEvent(statusPanel->GetElement()->GetChild("EquipButton", true), E_RELEASED, new Urho3D::EventHandlerImpl<UIPanel>(panel, &UIPanel::OnSwitchVisible));
    }

    panel = GetPanel(CRAFTPANEL);
    if (panel)
    {
        panel->Start(this, this);
        panel->SubscribeToEvent(statusPanel->GetElement()->GetChild("CraftButton", true), E_RELEASED, new Urho3D::EventHandlerImpl<UIPanel>(panel, &UIPanel::OnSwitchVisible));
    }

    panel = GetPanel(ABILITYPANEL);
    if (panel)
        panel->Start(this, this);

    panel = GetPanel(DIALOGUEPANEL);
    if (panel)
        panel->Start(this);

    panel = GetPanel(SHOPPANEL);
    if (panel)
        panel->Start(this, this);

    panel = GetPanel(JOURNALPANEL);
    if (panel)
    {
        panel->Start(this, missionManager_);
        panel->SubscribeToEvent(statusPanel->GetElement()->GetChild("QuestButton", true), E_RELEASED, new Urho3D::EventHandlerImpl<UIPanel>(panel, &UIPanel::OnSwitchVisible));
    }

    if (missionEnable_)
    {
        panel = GetPanel(MISSIONPANEL);
        if (panel)
        {
            panel->Start(this, missionManager_);
            panel->SubscribeToEvent(statusPanel->GetElement()->GetChild("QuestButton", true), E_RELEASED, new Urho3D::EventHandlerImpl<UIPanel>(panel, &UIPanel::OnSwitchVisible));
        }
    }

#ifdef HANDLE_MINIMAP
    Button* mapButton = (Button*) dynamic_cast<Button*>(statusPanel->GetElement()->GetChild("MapButton", true));
    if (mapButton && controlID_ == 0)
    {
        UIC_MiniMap* minimap = GameContext::Get().cameraNode_->GetOrCreateComponent<UIC_MiniMap>();
        if (minimap)
            minimap->AddActivatorButton(mapButton);
    }
#endif

    focusPanel_.Reset();

    URHO3D_LOGINFOF("Player() - CreateUI : id=%u ... OK !", GetID());
}

void Player::RemoveUI()
{
    URHO3D_LOGINFOF("Player() - ID=%d : RemoveUI ...", GetID());
    panels_.Clear();
    URHO3D_LOGINFOF("Player() - ID=%d : RemoveUI ... OK !", GetID());
}

void Player::ResizeUI()
{
    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ...", GetID());

    // set the position of the root
    int x = 0;
    int y = 0;

    IntRect viewportRect(0, 0, GetSubsystem<Graphics>()->GetWidth() / GameContext::Get().uiScale_, GetSubsystem<Graphics>()->GetHeight() / GameContext::Get().uiScale_);

    if (GameContext::Get().gameConfig_.multiviews_)
    {
        viewportRect = ViewManager::viewportRects_[GameContext::Get().numPlayers_-1][controlID_];
        x = viewportRect.left_ + 30 * GameContext::Get().uiScale_;
        y = viewportRect.top_ + 80 * GameContext::Get().uiScale_;
    }
    else
    {
        x = 30;
        y = (80 + 140 * controlID_) * GameContext::Get().uiScale_;
    }

    IntVector2 viewportSize = viewportRect.Size();
    IntVector2 viewportOrigin(viewportRect.left_, viewportRect.top_);

    uiStatusBar->SetAlignment(HA_LEFT, VA_TOP);
    uiStatusBar->SetPosition(x, y);

    // move ui panels considering the viewport rect

    UIPanel* statusPanel = GetPanel(STATUSPANEL);
    if (statusPanel)
    {
        float adjustfactor;
        if (!GameContext::Get().gameConfig_.touchEnabled_ && !GameContext::Get().gameConfig_.forceTouch_ && GameContext::Get().screenInches_ < 9.f)
            adjustfactor = 0.866666667f;
        else
            adjustfactor = GameContext::Get().GetAdjustUIFactor();

        static_cast<UIC_StatusPanel*>(statusPanel)->SetUIFactor(adjustfactor);

        statusPanel->SetPosition(uiStatusBar->GetPosition(), HA_LEFT, VA_TOP);

        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... status panel pos=%s size=%s adjustfactor=%F",
                        GetID(), statusPanel->GetElement()->GetPosition().ToString().CString(), statusPanel->GetElement()->GetSize().ToString().CString(), GameContext::Get().uiScale_ * adjustfactor);
    }

    UIPanel* bagPanel = GetPanel(BAGPANEL);
    if (bagPanel)
    {
        GameHelpers::SetUIScale(bagPanel->GetElement(), IntVector2(328, 294));

        IntRect rect = statusPanel->GetElement()->GetCombinedScreenRect();
        bagPanel->SetPosition(rect.right_+5, rect.top_+10, HA_LEFT, VA_TOP);
        bagPanel->OnResize();
        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... bag panel pos=%s size=%s", GetID(), bagPanel->GetElement()->GetPosition().ToString().CString(), bagPanel->GetElement()->GetSize().ToString().CString());
    }

    UIPanel* panel = GetPanel(EQUIPMENTPANEL);
    if (panel)
    {
        GameHelpers::SetUIScale(panel->GetElement(), IntVector2(272, 394));

        IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
        panel->SetPosition(rect.right_+5, rect.top_, HA_LEFT, VA_TOP);
        panel->OnResize();

        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... eqp panel pos=%s size=%s", GetID(), panel->GetElement()->GetPosition().ToString().CString(), panel->GetElement()->GetSize().ToString().CString());
    }

    panel = GetPanel(CRAFTPANEL);
    if (panel)
    {
        GameHelpers::SetUIScale(panel->GetElement(), IntVector2(331, 353));

        IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
        panel->SetPosition(rect.left_, rect.bottom_+5, HA_LEFT, VA_TOP);

        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... craft panel pos=%s size=%s", GetID(), panel->GetElement()->GetPosition().ToString().CString(), panel->GetElement()->GetSize().ToString().CString());
    }

    panel = GetPanel(ABILITYPANEL);
    if (panel)
    {
        if (static_cast<UIC_AbilityPanel*>(panel)->IsPopup())
        {
            const IntVector2& parentsize = panel->GetElement()->GetParent()->GetSize();
            panel->SetPosition(IntVector2(parentsize.x_, parentsize.y_/2), HA_LEFT, VA_TOP);
        }
        else
            panel->SetPosition(IntVector2(viewportOrigin.x_ + viewportSize.x_ / 3, viewportOrigin.y_ + 5), HA_LEFT, VA_TOP);
    }

    panel = GetPanel(DIALOGUEPANEL);
    if (panel)
    {
        GameHelpers::SetUIScale(panel->GetElement(), IntVector2(125, 244));
        //panel->SetPosition(viewportOrigin.x_ + viewportSize.x_ / 2, viewportOrigin.y_ + viewportSize.y_, HA_LEFT, VA_TOP);
        panel->SetPosition(0, 0, HA_CENTER, VA_BOTTOM);
    }

    panel = GetPanel(SHOPPANEL);
    if (panel)
    {
        GameHelpers::SetUIScale(panel->GetElement(), IntVector2(362, 552));

        IntRect rect = bagPanel->GetElement()->GetCombinedScreenRect();
        panel->SetPosition(rect.right_+10, rect.top_, HA_LEFT, VA_TOP);

        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... shop panel pos=%s size=%s", GetID(), panel->GetElement()->GetPosition().ToString().CString(), panel->GetElement()->GetSize().ToString().CString());
    }

    panel = GetPanel(MISSIONPANEL);
    if (panel)
    {
        GameHelpers::SetUIScale(panel->GetElement(), IntVector2(420, 310));

        IntVector2 size = panel->GetElement()->GetSize();
        panel->SetPosition(viewportOrigin.x_ + viewportSize.x_ / 2 - size.x_/2, viewportOrigin.y_ + viewportSize.y_ / 2 - size.y_/2, HA_LEFT, VA_TOP);

        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... mission panel pos=%s size=%s", GetID(), panel->GetElement()->GetPosition().ToString().CString(), panel->GetElement()->GetSize().ToString().CString());
    }

    panel = GetPanel(JOURNALPANEL);
    if (panel)
    {
        GameHelpers::SetUIScale(panel->GetElement(), IntVector2(612, 268));

        IntVector2 size = panel->GetElement()->GetSize();
        panel->SetPosition(viewportOrigin.x_ + viewportSize.x_ / 2 - size.x_/2, y, HA_LEFT, VA_TOP);

        URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... journal panel pos=%s size=%s", GetID(), panel->GetElement()->GetPosition().ToString().CString(), panel->GetElement()->GetSize().ToString().CString());
    }

    // clamp the panels
    for (HashMap<int, WeakPtr<UIPanel> >::Iterator it = panels_.Begin(); it != panels_.End(); ++it)
        GameHelpers::ClampPositionUIElementToScreen(it->second_->GetElement());

#ifdef HANDLE_MINIMAP
    if (controlID_ == 0)
        GameHelpers::ClampPositionUIElementToScreen(GetSubsystem<UI>()->GetRoot()->GetChild(String("MiniMap")));
#endif

    URHO3D_LOGINFOF("Player() - ID=%d : ResizeUI ... OK !", GetID());
}

void Player::SetVisibleUI(bool state, bool all)
{
    int controltype = GameContext::Get().playerState_[controlID_].controltype;

    if (controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    if (!uiStatusBar)
        return;

    URHO3D_LOGINFOF("Player() - SetVisibleUI :  ID=%d %s ...", GetID(), state ? "show":"hide");

    uiStatusBar->SetVisible(state);

    if (all)
    {
        for (HashMap<int, WeakPtr<UIPanel> >::Iterator it = panels_.Begin(); it != panels_.End(); ++it)
        {
            UIPanel* panel = it->second_.Get();
            panel->GetElement()->SetEnabled(state);
            panel->SetVisible(state);
        }
    }

    if (state && !all)
    {
        // status panel
        UIPanel* statusPanel = GetPanel(STATUSPANEL);
        if (statusPanel && !statusPanel->IsVisible())
            statusPanel->ToggleVisible();

        // ability panel
        UIC_AbilityPanel* abilitypanel = static_cast<UIC_AbilityPanel*>(GetPanel(ABILITYPANEL));
        if (abilitypanel && !abilitypanel->IsPopup())
        {
            if (state)
                abilitypanel->GetElement()->SetEnabled(true);
            if (!abilitypanel->IsVisible())
                abilitypanel->ToggleVisible();
        }
    }

    if (GameContext::Get().gameConfig_.touchEnabled_ && GameContext::Get().gameConfig_.screenJoystick_)
    {
        GameContext::Get().input_->SetScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoystickID_, state && controltype == CT_SCREENJOYSTICK);
        GameContext::Get().input_->SetScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoysticksettingsID_, false);
    }

    URHO3D_LOGINFOF("Player() - SetVisibleUI :  ID=%d %s ... OK!", GetID(), state ? "show":"hide");
}

void Player::ResetUI()
{
    if (!uiStatusBar)
        return;

    for (HashMap<int, WeakPtr<UIPanel> >::Iterator it = panels_.Begin(); it != panels_.End(); ++it)
        it->second_->Reset();
}

void Player::UpdateUI()
{
    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    if (!uiStatusBar)
        return;

    for (HashMap<int, WeakPtr<UIPanel> >::Iterator it = panels_.Begin(); it != panels_.End(); ++it)
        it->second_->Update();
}

void Player::UpdatePoints(const unsigned& points)
{
    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    score += points;

    if (!uiStatusBar)
        return;

    scoreText->SetText(String(score));
//    LOGINFOF("Player() - Update Score = %u", score);
}

void Player::NextPanelFocus()
{
    if (!panels_.Size())
        return;

    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    // Get the current panel
    UIPanel* focuspanel = GetFocusPanel();
    int focuspanelid = focuspanel ? focuspanel->GetPanelId() : -1;
    URHO3D_LOGINFOF("Player() - NextPanelFocus :  ID=%d currentpanelfocus=%s(%d) ...", GetID(), focuspanelid!=-1 ? PanelNames_[focuspanelid] : "none", focuspanelid);

    if (!focuspanel)
    {
        focuspanelid = STATUSPANEL;
        focuspanel = GetPanel(focuspanelid);
        if (!focuspanel->IsVisible())
            focuspanel->ToggleVisible();
        focuspanel->GainFocus();
    }
    else
    {
        // Find the next panel to focus
        bool hasfocus = false;
        int newfocus = focuspanelid;
        do
        {
            UIPanel* panel = GetPanel(++newfocus);
            if (panel && panel != focuspanel && panel->IsVisible() && panel->CanFocus())
            {
                if (focuspanel)
                    focuspanel->LoseFocus();

                focuspanelid = newfocus;
                focuspanel = panel;
                focuspanel->GainFocus();
                hasfocus = true;
            }
        }
        while (newfocus != focuspanelid && newfocus < panels_.Size());

        // no find next focus => losefocus
        if (!hasfocus)
        {
            if (focuspanel)
                focuspanel->LoseFocus();
            focuspanel = 0;
            focuspanelid = -1;
        }
    }

    SetFocusPanel(focuspanelid);

    URHO3D_LOGINFOF("Player() - NextPanelFocus :  ID=%d currentpanelfocus=%s(%d) hasfocus=%s... OK !", GetID(),
                    focuspanelid != -1 ? PanelNames_[focuspanelid] : "none", focuspanelid, !focuspanel || !focuspanel->GetElement()->HasFocus() ? "false":"true");
}

void Player::DebugDrawUI()
{
    if (GameContext::Get().playerState_[controlID_].controltype == CT_CPU && GameContext::Get().arenaZoneOn_)
        return;

    if (!uiStatusBar)
        return;

    for (HashMap<int, WeakPtr<UIPanel> >::ConstIterator it = panels_.Begin(); it != panels_.End(); ++it)
    {
        if (it->second_->IsVisible())
            it->second_->DebugDraw();
    }
}

void Player::TransferCollectableToUI(const Variant& varpos, UIElement* dest, unsigned idslot, unsigned qty, const ResourceRef& collectable)
{
    if (!qty)
    {
        URHO3D_LOGERRORF("Player() - TransferDrawable2DToUI : idslot=%u qty=%u", idslot, qty);
        return;
    }

//    URHO3D_LOGINFOF("Player() - TransferDrawable2DToUI : this=%u idslot=%u qty=%u ... ", this, idslot, qty);

    // Set delayed Time
    unsigned time = Time::GetSystemTime();
    if (time - lastInventoryUpdate_ < 1000 * UI_COLLECTABLE_SEMITRANSFERTIME)
    {
        lastTransferDelay_ += UI_COLLECTABLE_SEMITRANSFERTIME * 0.5f;
    }
    else
    {
        lastTransferDelay_ = 0.f;
    }

    Sprite* spriteui = 0;

    // Create object animation.
    if (dest)
    {
        spriteui = GetSubsystem<UI>()->GetRoot()->CreateChild<Sprite>();
        if (!idslot && !collectable.type_)
        {
            ResourceRef ref;
            ref.type_ = SpriteSheet2D::GetTypeStatic();
            ref.name_ = "UI/game_equipment.xml@or_pepite04";
            GameHelpers::SetUIElementFrom(spriteui, UIEQUIPMENT, ref, SPRITEUISIZE);
        }
        else
        {
            GameHelpers::SetUIElementFrom(spriteui, UIEQUIPMENT, collectable, SPRITEUISIZE);
        }

        spriteui->SetOpacity(0.0f);

        IntVector2 position;
        if (varpos.GetType() == VAR_VECTOR3 || varpos.GetType() == VAR_VECTOR2)
            GameHelpers::OrthoWorldToScreen(position, GetAvatar(), controlID_);
        else if (varpos.GetType() == VAR_INTVECTOR2)
            position = varpos.GetIntVector2();

        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));

        SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(context_));
        positionAnimation->SetKeyFrame(lastTransferDelay_, Vector2(float(position.x_), float(position.y_)));
        positionAnimation->SetKeyFrame(lastTransferDelay_ + UI_COLLECTABLE_SEMITRANSFERTIME * 2, Vector2(float(dest->GetScreenPosition().x_),float(dest->GetScreenPosition().y_)));
        objectAnimation->AddAttributeAnimation("Position", positionAnimation, WM_ONCE);

        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
        scaleAnimation->SetKeyFrame(lastTransferDelay_, spriteui->GetSize());
        scaleAnimation->SetKeyFrame(lastTransferDelay_ + UI_COLLECTABLE_SEMITRANSFERTIME, spriteui->GetSize() * 3);
        scaleAnimation->SetKeyFrame(lastTransferDelay_ + UI_COLLECTABLE_SEMITRANSFERTIME * 2, spriteui->GetSize());
        objectAnimation->AddAttributeAnimation("Size", scaleAnimation, WM_ONCE);

        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(lastTransferDelay_, 0.f);
        alphaAnimation->SetKeyFrame(lastTransferDelay_ + UI_COLLECTABLE_SEMITRANSFERTIME, 0.95f);
        objectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);

        spriteui->SetObjectAnimation(objectAnimation);
    }

    // Remove Timer on SpriteUI
    TimerRemover* timer = TimerRemover::Get();
    if (timer)
    {
        timer->SetSendEvents(this, GO_PLAYERTRANSFERCOLLECTABLESTART, GO_PLAYERTRANSFERCOLLECTABLEFINISH);
        timer->Start(spriteui, UI_COLLECTABLE_SEMITRANSFERTIME * 2 + 0.05f, FREEMEMORY, lastTransferDelay_, idslot, qty);
        lastInventoryUpdate_ = time;

        URHO3D_LOGINFOF("Player() - TransferDrawable2DToUI : this=%u spriteui=%u idslot=%u qty=%u ...OK !", this, spriteui, idslot, qty);
    }
    else
    {
        URHO3D_LOGERRORF("Player() - TransferDrawable2DToUI : this=%u idslot=%u qty=%u ... NO Remove Timer !", this, spriteui, idslot, qty);
    }
}


/// Handlers

void Player::Start()
{
    zone_.z_ = -1;

    if (!avatar_)
    {
        URHO3D_LOGWARNINGF("Player() - Start : player ID=%u NoAvatar => ResetAvatar at position=%s", GetID(), GameContext::Get().playerState_[controlID_].position.ToString().CString());

        Actor::SetViewZ(GameContext::Get().playerState_[controlID_].viewZ);
        ResetAvatar(GameContext::Get().playerState_[controlID_].position);

        if (!avatar_)
        {
            URHO3D_LOGERRORF("Player() - Start : player ID=%u NoAvatar !", GetID());
            return;
        }
    }

    URHO3D_LOGINFOF("---------------------------------------------");
    URHO3D_LOGINFOF("- Player() - Start : player ID=%u scene_=%u mission=%s    -", GetID(), scene_, missionEnable_ ? "true":"false");
    URHO3D_LOGINFOF("---------------------------------------------");

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, false);

    dirtyPlayer_ = false;

    active = true;
    Actor::Start();

    GetStats()->DumpAllStats();

    // Reset Life
    if (gocLife)
        gocLife->Reset();

    // Reset Animation
    GOC_Animator2D* animator = avatar_->GetComponent<GOC_Animator2D>();
    if (animator)
        animator->ResetState();

    // Spawn Effect when Players Appear
    Drawable2D* drawable = avatar_->GetDerivedComponent<Drawable2D>();
    GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_LIFEFLAME], drawable->GetLayer(), drawable->GetViewMask(), avatar_->GetWorldPosition2D(), 0.f, 2.f, true, 3.f, Color::BLACK, LOCAL);

    avatar_->AddTag("Player");

    ViewManager::Get()->SetFocusEnable(true, GameContext::Get().gameConfig_.multiviews_ ? controlID_ : 0);

    // Start UIC_MiniMap
//	#ifdef HANDLE_MINIMAP
//	if (GetID() == 0)
//	{
//		UIC_MiniMap* minimap = avatar_->GetComponent<UIC_MiniMap>();
//		if (minimap)
//			minimap->Start();
//	}
//    #endif // HANDLE_MINIMAP

//    if (GameHelpers::SetLightActivation(this))
//    {
//        SubscribeToEvent(WEATHER_DAWN, URHO3D_HANDLER(Player, HandleUpdateTimePeriod));
//        SubscribeToEvent(WEATHER_TWILIGHT, URHO3D_HANDLER(Player, HandleUpdateTimePeriod));
//    }

    UpdateUI();

    SaveState();

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);

    if (gocDestroyer_)
        gocDestroyer_->UpdateFilterBits();

    URHO3D_LOGINFOF("----------------------------------------");
    URHO3D_LOGINFOF("- Player() - Start : player ID=%u viewZ=%d ... OK   -", GetID(), GetViewZ());
    URHO3D_LOGINFOF("----------------------------------------");
}

void Player::Stop()
{
    URHO3D_LOGINFOF("------------------------------------");
    URHO3D_LOGINFOF("- Player() - Stop : player ID=%u  -", GetID());
    URHO3D_LOGINFOF("------------------------------------");

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, false);

    UnsubscribeFromAllEvents();

    if (equipment_)
        equipment_->Clear();

    if (missionManager_)
        missionManager_->Stop();

//    GetStats()->DumpAllStats();

    Actor::Stop();
    active = false;

    if (GameNetwork::Get() && avatar_)
        GameNetwork::Get()->SetEnableObject(false, avatar_->GetID(), true);

//    if (GameContext::Get().ServerMode_ && avatar_)
//        World2D::RemoveTraveler(avatar_);

    GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);

    URHO3D_LOGINFOF("----------------------------------------");
    URHO3D_LOGINFOF("- Player() - Stop : player ID=%u OK   -", GetID());
    URHO3D_LOGINFOF("----------------------------------------");
}

void Player::StartSubscribers()
{
//    URHO3D_LOGERRORF("Player() - StartSubscribers : player ID=%u avatarID=%u(%u)", GetID(), nodeID_, avatar_->GetID());

    if (mainController_)
    {
        SubscribeToEvent(avatar_, GOC_CONTROLACTION1, URHO3D_HANDLER(Player, OnFire1));
        SubscribeToEvent(avatar_, GOC_CONTROLACTION2, URHO3D_HANDLER(Player, OnFire2));
        SubscribeToEvent(avatar_, GOC_CONTROLACTION3, URHO3D_HANDLER(Player, OnFire3));
        SubscribeToEvent(avatar_, GOC_CONTROLACTIONSTATUS, URHO3D_HANDLER(Player, OnStatus));
    }

    SubscribeToEvent(avatar_, COLLIDEWALLBEGIN, URHO3D_HANDLER(Player, OnCollideWall));

    SubscribeToEvent(avatar_, GOC_LIFEDEAD, URHO3D_HANDLER(Player, OnDead));
    SubscribeToEvent(avatar_, GO_KILLER, URHO3D_HANDLER(Player, OnDead));
    SubscribeToEvent(avatar_, GO_DESTROY, URHO3D_HANDLER(Player, OnAvatarDestroy));

    SubscribeToEvent(avatar_, GO_INVENTORYEMPTY, URHO3D_HANDLER(Player, OnInventoryEmpty));
    SubscribeToEvent(avatar_, GO_COLLECTABLEDROP, URHO3D_HANDLER(Player, OnDropCollectable));

    SubscribeToEvent(this, DIALOG_OPEN, URHO3D_HANDLER(Player, OnTalkBegin));
    SubscribeToEvent(this, DIALOG_QUESTION, URHO3D_HANDLER(Player, OnTalkQuestion));
    SubscribeToEvent(this, DIALOG_RESPONSE, URHO3D_HANDLER(Player, OnTalkShowResponses));
    SubscribeToEvent(this, DIALOG_BARGAIN, URHO3D_HANDLER(Player, OnBargain));
    SubscribeToEvent(this, DIALOG_CLOSE, URHO3D_HANDLER(Player, OnTalkEnd));

    if (GameHelpers::SetLightActivation(this))
    {
        SubscribeToEvent(WEATHER_DAWN, URHO3D_HANDLER(Player, HandleUpdateTimePeriod));
        SubscribeToEvent(WEATHER_TWILIGHT, URHO3D_HANDLER(Player, HandleUpdateTimePeriod));
    }

    gocController->ResetButtons();

    if (mainController_)
    {
        if (controlID_ == 0)
        {
            SubscribeToEvent(E_RELEASED, URHO3D_HANDLER(Player, HandleClic));
            SubscribeToEvent(E_MOUSEBUTTONUP, URHO3D_HANDLER(Player, HandleClic));
            SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(Player, HandleClic));

            URHO3D_LOGINFOF("Player() - Start : player ID=%u subscribe HandleClic !", GetID());

            if (missionEnable_)
            {
                if (missionManager_ && !missionManager_->IsStarted())
                {
                    SubscribeToEvent(this, GO_PLAYERMISSIONMANAGERSTART, URHO3D_HANDLER(Player, HandleDelayedActions));
                    DelayInformer* delayedMissionStart = new DelayInformer(this, 7.f, GO_PLAYERMISSIONMANAGERSTART);
                }
            }
            else
            {
                UnsubscribeFromEvent(this, GO_PLAYERMISSIONMANAGERSTART);
            }
        }

        SubscribeToEvent(avatar_, GO_INVENTORYGET, URHO3D_HANDLER(Player, OnGetCollectable));
        SubscribeToEvent(this, GO_PLAYERTRANSFERCOLLECTABLESTART, URHO3D_HANDLER(Player, OnStartTransferCollectable));
        SubscribeToEvent(this, GO_PLAYERTRANSFERCOLLECTABLEFINISH, URHO3D_HANDLER(Player, OnReceiveCollectable));
    }

    if ((GameContext::Get().ServerMode_ && (gocController->GetControllerType() & (GO_Player | GO_NetPlayer))) ||
        (!GameContext::Get().ServerMode_ && (gocController->GetControllerType() & GO_Player)))
        SubscribeToEvent(avatar_, GO_SELECTED, URHO3D_HANDLER(Player, OnEntitySelection));

    SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(Player, OnPostUpdate));
}

void Player::StopSubscribers()
{
    URHO3D_LOGINFOF("Player() - StopSubscribers : player ID=%u nodeID=%u avatar=%u ...", GetID(), nodeID_, avatar_.Get());

//    UnsubscribeFromEvent(healthBorder, E_RELEASED);

    if (avatar_)
    {
        if (mainController_)
        {
            UnsubscribeFromEvent(avatar_, GOC_CONTROLACTION1);
            UnsubscribeFromEvent(avatar_, GOC_CONTROLACTION2);
            UnsubscribeFromEvent(avatar_, GOC_CONTROLACTION3);
            UnsubscribeFromEvent(avatar_, GOC_CONTROLACTIONSTATUS);

            UnsubscribeFromEvent(avatar_, GO_INVENTORYGET);
        }

        UnsubscribeFromEvent(avatar_, COLLIDEWALLBEGIN);

        UnsubscribeFromEvent(avatar_, GOC_LIFEDEAD);
        UnsubscribeFromEvent(avatar_, GOC_LIFEUPDATE);
        UnsubscribeFromEvent(avatar_, GO_KILLER);

        UnsubscribeFromEvent(avatar_, GO_INVENTORYEMPTY);
        UnsubscribeFromEvent(avatar_, GO_COLLECTABLEDROP);
    }

    UnsubscribeFromEvent(this, DIALOG_OPEN);
    UnsubscribeFromEvent(this, DIALOG_QUESTION);
    UnsubscribeFromEvent(this, DIALOG_RESPONSE);
    UnsubscribeFromEvent(this, DIALOG_BARGAIN);
    UnsubscribeFromEvent(this, DIALOG_CLOSE);

    if (HasSubscribedToEvent(WEATHER_DAWN))
    {
        UnsubscribeFromEvent(WEATHER_DAWN);
        UnsubscribeFromEvent(WEATHER_TWILIGHT);
    }

    if (mainController_)
    {
        if (controlID_ == 0)
        {
            UnsubscribeFromEvent(E_RELEASED);
            UnsubscribeFromEvent(E_MOUSEBUTTONUP);
            UnsubscribeFromEvent(E_TOUCHEND);

            if (missionEnable_ && missionManager_ && missionManager_->IsStarted())
                UnsubscribeFromEvent(this, GO_PLAYERMISSIONMANAGERSTART);
        }

        UnsubscribeFromEvent(this, GO_PLAYERTRANSFERCOLLECTABLESTART);
        UnsubscribeFromEvent(this, GO_PLAYERTRANSFERCOLLECTABLEFINISH);
    }

    if (avatar_ && gocController)
        UnsubscribeFromEvent(avatar_, GO_SELECTED);

    UnsubscribeFromEvent(E_SCENEPOSTUPDATE);

    URHO3D_LOGINFOF("Player() - StopSubscribers : player ID=%u nodeID=%u avatar=%u ... OK !", GetID(), nodeID_, avatar_.Get());
}


void Player::OnFire1(StringHash eventType, VariantMap& eventData)
{
//    if (!abilities_)
//        return;
//
//    Ability* ability = abilities_ ? abilities_->GetActiveAbility() : 0;

//    URHO3D_LOGINFOF("Player() - OnFire1 : nodeid=%u ... ability=%s", avatar_->GetID(), ability ? ability->GetTypeName().CString() : "none");
//
//    if (ability)
//    {
//        if (ability->GetType() == ABIBomb::GetTypeStatic())
//            ability->Use(avatar_->GetVar(GOA::DIRECTION).GetVector2());
//    }
}

void Player::OnFire2(StringHash eventType, VariantMap& eventData)
{
    if (!abilities_)
    {
        URHO3D_LOGERRORF("Player() - OnFire2 : nodeid=%u ... no abilities", avatar_->GetID());
        return;
    }

    Ability* ability = abilities_->GetActiveAbility();

//    URHO3D_LOGINFOF("Player() - OnFire2 : nodeid=%u ... ability=%s", avatar_->GetID(), ability ? ability->GetTypeName().CString() : "none");

    if (ability)
    {
        StringHash abi(ability->GetType());

        if (abi == ABIBomb::GetTypeStatic())
        {
//            URHO3D_LOGINFOF("Player() - OnFire2 : nodeid=%u ABIBomb skipped !", avatar_->GetID());
            return;
        }

        Vector2 wpoint = abi == ABI_WallBreaker::GetTypeStatic() || abi == ABI_AnimShooter::GetTypeStatic() ? Vector2::ZERO :
                         avatar_->GetWorldPosition2D() + 1.1f * avatar_->GetVar(GOA::DIRECTION).GetVector2();

        if (abi == ABI_WallBreaker::GetTypeStatic())
        {
            ability->isInUse_ = false;
        }
        else
        {
            if (abi == ABI_Grapin::GetTypeStatic())
                wpoint += Vector2::UP;

            ObjectControlInfo* oinfo = 0;
            ability->Use(wpoint, &oinfo);
        }
    }
}

void Player::OnFire3(StringHash eventType, VariantMap& eventData)
{
    if (dirtyPlayer_)
        return;

    if (!avatar_)
        return;

    gocLife = avatar_->GetComponent<GOC_Life>();
    if (!gocLife)
        return;

    int newavatarindex = avatars_[(avatarAccumulatorIndex_+1) % avatars_.Size()];
    GOC_Life* newgoclife = GOT::GetControllableTemplate(newavatarindex)->GetComponent<GOC_Life>();
    if (!newgoclife || gocLife->CheckChangeTemplate(newgoclife->GetTemplateName()))
    {
        URHO3D_LOGINFOF("Player() - OnFire3 : change avatar ... index=%d numavailableavatars=%u", newavatarindex, avatars_.Size());
        ChangeAvatar(newavatarindex);
    }
    else
    {
        URHO3D_LOGINFOF("Player() - OnFire3 : can't change to avatar : not enough energy !");
        avatarAccumulatorIndex_ = newavatarindex;
    }
}

void Player::OnStatus(StringHash eventType, VariantMap& eventData)
{
    if (dirtyPlayer_)
        return;

    NextPanelFocus();
}

void Player::OnPostUpdate(StringHash eventType, VariantMap& eventData)
{
    // Update the Change of Avatar if dirtyPlayer_
    UpdateAvatar();

    UpdateZones();
}


extern const char* wallTypeNames[];

void Player::OnCollideWall(StringHash eventType, VariantMap& eventData)
{
    if (!abilities_)
        return;

    Ability* ability = abilities_->GetActiveAbility();

    if (ability && ability->GetType() == ABI_WallBreaker::GetTypeStatic() && !ability->isInUse_)
    {
        int walltype = eventData[CollideWallBegin::WALLCONTACTTYPE].GetInt();

        URHO3D_LOGINFOF("Player() - OnCollideWall - ABI_WallBreaker walltype=%s !", wallTypeNames[walltype]);

        if (walltype == Wall_Ground)
            ability->Use(avatar_->GetWorldPosition2D() + Vector2::DOWN);
        else if (walltype == Wall_Roof)
            ability->Use(avatar_->GetWorldPosition2D() + Vector2::UP);
        else
            ability->Use(avatar_->GetWorldPosition2D() + avatar_->GetVar(GOA::DIRECTION).GetVector2());
    }
}

void Player::OnDead(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GOC_LIFEDEAD)
    {
        URHO3D_LOGINFOF("Player() - OnDead : Avatar Node=%u died ... ", nodeID_);

        UIPanel* statusPanel = GetPanel(STATUSPANEL);
        if (statusPanel)
            statusPanel->Update();

        StopSubscribers();

        Vector2 position = gocDestroyer_ ? gocDestroyer_->GetWorldMassCenter() : avatar_->GetWorldPosition2D();

        if (World2D::IsInsideWorldBounds(position))
        {
            GameContext::Get().playerState_[controlID_].position = position;
            GameContext::Get().playerState_[controlID_].viewZ = GetViewZ();
            URHO3D_LOGINFOF(" ... Save Position %s viewZ=%d!", position.ToString().CString(), GameContext::Get().playerState_[controlID_].viewZ);
        }

        if (!GameContext::Get().arenaZoneOn_)
        {
            // Add a Stele
            Node* stele = World2D::SpawnFurniture(StringHash("GOT_SteleRIP"), position, GetViewZ(), false, false, true);
            if (stele)
            {
                GOC_Animator2D* animator = stele->GetComponent<GOC_Animator2D>();
                animator->SetState(StringHash(STATE_APPEAR));
            }
        }

        SaveState();

        active = false;

        SendEvent(GAME_PLAYERDIED);
    }
    else if (eventType == GO_KILLER)
    {
        unsigned deadID = eventData[Go_Killer::GO_DEAD].GetUInt();
        URHO3D_LOGINFOF("Player() - OnDead : Avatar Node=%u kills node=%u", nodeID_, deadID);

        Node* deadNode = scene_->GetNode(deadID);
        if (deadNode)
        {
            StringHash type(deadNode->GetName());
//            unsigned& stat = GetStat(GOA::ACS_ENTITIES_KILLED, type);
//            stat = stat + 1;
            GetStat(GOA::ACS_ENTITIES_KILLED, type)++;
            GetStats()->DumpStat(GOA::ACS_ENTITIES_KILLED, type);
        }
    }
}

void Player::OnAvatarDestroy(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("Player() - OnAvatarDestroy  : Avatar Node=%u ... ", nodeID_);

    if (mainController_)
    {
        SetVisibleUI(false, true);

        Actor::Stop();

        nodeID_ = 0;
    }
    else
    {
        if (GameContext::Get().ServerMode_)
            GameNetwork::Get()->Server_SetActivePlayer(this, false);
    }

    active = false;

    URHO3D_LOGINFOF("Player() - OnAvatarDestroy  : ... OK !");
}

void Player::OnInventoryEmpty(StringHash eventType, VariantMap& eventData)
{
    equipment_->Clear();
    equipment_->Update(!GameContext::Get().ClientMode_);

    UpdateUI();
}

void Player::OnGetCollectable(StringHash eventType, VariantMap& eventData)
{
    unsigned ID_Slot = eventData[Go_InventoryGet::GO_IDSLOT].GetUInt();
    unsigned qty = eventData[Go_InventoryGet::GO_QUANTITY].GetUInt();
    const ResourceRef& resourceRef = eventData[Go_InventoryGet::GO_RESOURCEREF].GetResourceRef();

    URHO3D_LOGINFOF("Player() - OnGetCollectable ... playerID=%u avatar=%s(%u) idslot=%u name=%s qty=%u ... ",
                    GetID(), GetAvatar()->GetName().CString(), GetAvatar()->GetID(), ID_Slot, resourceRef.name_.CString(), qty);

    TransferCollectableToUI(eventData[Go_InventoryGet::GO_POSITION], panels_.Size() ? panels_[STATUSPANEL]->GetElement() : 0, ID_Slot, qty, resourceRef);

    URHO3D_LOGINFOF("Player() - OnGetCollectable ... OK ! ");
}

void Player::OnStartTransferCollectable(StringHash eventType, VariantMap& eventData)
{
    GameHelpers::SpawnSound(GameContext::Get().cameraNode_.Get(), "Sounds/042-Knock03.ogg", 0.3f);
}

void Player::OnEntitySelection(StringHash eventType, VariantMap& eventData)
{
    unsigned type = eventData[Go_Selected::GO_TYPE].GetUInt();
    unsigned selectedID = eventData[Go_Selected::GO_ID].GetUInt();

    URHO3D_LOGINFOF("Player() - OnEntitySelection : %s(%u) selectedid=%u type=%u...", avatar_->GetName().CString(), avatar_->GetID(), selectedID, type);

    if (selectedID < 2)
        return;

    Node* selectedNode = avatar_->GetScene()->GetNode(selectedID);
    if (!selectedNode)
    {
        URHO3D_LOGERRORF("Player() - OnEntitySelection : %s(%u) Select node=0 !", avatar_->GetName().CString(), avatar_->GetID());
        return;
    }

    if (selectedNode == avatar_)
    {
        URHO3D_LOGERRORF("Player() - OnEntitySelection : %s(%u) Select node=avatar ! ", avatar_->GetName().CString(), avatar_->GetID());
        return;
    }

    if (type == GO_AI_Enemy)
    {
        URHO3D_LOGINFOF("Player() - OnEntitySelection : %s(%u) Select Enemy node=%s(%u)",
                        avatar_->GetName().CString(), avatar_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());

        // TODO if need

    }
    else if (type & (GO_Player|GO_AI_Ally))
    {
        bool mountok = false;
        unsigned mountid = 0U;

        const unsigned mountedon = avatar_->GetVar(GOA::ISMOUNTEDON).GetUInt();
        const unsigned selectionmountedon = selectedNode->GetVar(GOA::ISMOUNTEDON).GetUInt();

        URHO3D_LOGINFOF("Player() - OnEntitySelection : %s(%u) mountedon=%u selectionmountedon=%u ...",
                         avatar_->GetName().CString(), avatar_->GetID(), mountedon, selectionmountedon);

        // if not mounted
        if (!mountedon && selectionmountedon != avatar_->GetID())
        {
            Vector2 delta = selectedNode->GetWorldPosition2D() - avatar_->GetWorldPosition2D();

            URHO3D_LOGINFOF("Player() - OnEntitySelection : %s(%u) mount on target %s(%u) pos=%s spos=%s...",
                            avatar_->GetName().CString(), avatar_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID(),
                            avatar_->GetWorldPosition2D().ToString().CString(), selectedNode->GetWorldPosition2D().ToString().CString());
            // Mount if near
            if (delta.LengthSquared() <= DISTANCEFORMOUNT)
            {
                URHO3D_LOGINFOF("Player() - OnEntitySelection : %s(%u) mount on target %s(%u) ...",
                                avatar_->GetName().CString(), avatar_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());

                mountok = gocController->MountOn(selectedNode);
                mountid = selectedNode->GetID();
            }
        }
        else if (mountedon == selectedNode->GetID())
        {
            URHO3D_LOGINFOF("Player() - OnEntitySelection : %s(%u) unmount from target %s(%u) ...",
                                avatar_->GetName().CString(), avatar_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());

            mountok = gocController->Unmount();
        }

        if (GameContext::Get().ServerMode_ && mountok)
        {
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Net_ObjectCommand::P_NODEID] = avatar_->GetID();
            eventData[Net_ObjectCommand::P_NODEPTRFROM] = mountid;
            eventData[Net_ObjectCommand::P_CLIENTID] = clientID_;
            avatar_->SendEvent(GO_MOUNTEDON, eventData);
        }
    }
}

void Player::OnMountControlUpdate(StringHash eventType, VariantMap& eventData)
{
    // On Fire => Unmount
//    if (avatar_->GetDerivedComponent<GOC_Controller>()->control_.buttons_ & CTRL_FIRE3)
//    {
//        Unmount();
//        return;
//    }
}

void Player::OnReceiveCollectable(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("Player() - OnReceiveCollectable ...");

    if (!avatar_ || !gocInventory)
    {
        URHO3D_LOGERRORF("Player() - OnReceiveCollectable : avatar_=%u gocInventory=%u !", avatar_.Get(), gocInventory);
        return;
    }

    unsigned slotIndex = eventData[Go_EndTimer::GO_DATA1].GetUInt();
    unsigned qty = eventData[Go_EndTimer::GO_DATA2].GetUInt();

    if (!qty)
    {
        URHO3D_LOGERRORF("Player() - OnReceiveCollectable : slotIndex=%u qty=%u !", slotIndex, qty);
        return;
    }

    URHO3D_LOGINFOF("Player() - OnReceiveCollectable : slotIndex=%u qty=%u", slotIndex, qty);

    if (slotIndex == 0)
    {
        avatar_->SendEvent(UI_MONEYUPDATED);
    }
    else
    {
        eventData[Go_InventoryGet::GO_IDSLOT] = slotIndex;
        SendEvent(PANEL_SLOTUPDATED, eventData);
    }

    /// update states
    const Slot& slot = gocInventory->GetSlot(slotIndex);

//    LOGINFO("Player() - OnReceiveCollectable : Dump Before Update State");
//    GetStats()->DumpStat(GOA::ACS_ITEMS_COLLECTED, slot.type_);

//    LOGINFOF("Player() - OnReceiveCollectable : Update State type=%s(%u) qty=%d", GOT::GetType(slot.type_).CString(), slot.type_.Value(), qty);
//    unsigned& stat = GetStat(GOA::ACS_ITEMS_COLLECTED, slot.type_);
//    LOGINFOF("Player() - OnReceiveCollectable : stat value before add=%u", stat);
//    stat = stat + qty;
    GetStat(GOA::ACS_ITEMS_COLLECTED, slot.type_) += qty;
//    LOGINFO("Player() - OnReceiveCollectable : Dump After Update State");
    GetStats()->DumpStat(GOA::ACS_ITEMS_COLLECTED, slot.type_);

//    URHO3D_LOGINFOF("Player() - OnReceiveCollectable ... OK !");
}

void Player::OnDropCollectable(StringHash eventType, VariantMap& eventData)
{
    StringHash type(eventData[Go_CollectableDrop::GO_TYPE].GetUInt());
    unsigned qty = eventData[Go_CollectableDrop::GO_QUANTITY].GetUInt();

    URHO3D_LOGINFOF("Player() - OnDropCollectable : type=%s(%u) qty=%u", GOT::GetType(type).CString(), type.Value(), qty);

//    URHO3D_LOGINFO("Player() - OnDropCollectable : Dump Before Update State");
//    GetStats()->DumpStat(GOA::ACS_ITEMS_COLLECTED, type);

//    URHO3D_LOGINFOF("Player() - OnDropCollectable : Update State type=%s(%u) qty=%d", GOT::GetType(type).CString(), type.Value(), qty);
    unsigned& stat = GetStat(GOA::ACS_ITEMS_COLLECTED, type);
//    URHO3D_LOGINFOF("Player() - OnDropCollectable : stat value before add=%u", stat);
    stat = ((int)stat - (int)qty < 0 ? 0 : stat - qty);

//    URHO3D_LOGINFO("Player() - OnDropCollectable : Dump After Update State");
    GetStats()->DumpStat(GOA::ACS_ITEMS_COLLECTED, type);
}


void Player::OnTalkBegin(StringHash eventType, VariantMap& eventData)
{
    unsigned actorid = eventData[Dialog_Open::ACTOR_ID].GetUInt();

    if (!dialogInteractor_ && actorid)
        dialogInteractor_ = Actor::Get(actorid);

    URHO3D_LOGINFOF("Player() - OnTalkBegin ... player=%s(%u) interactor=%s(%u)", avatar_->GetName().CString(), avatar_->GetID(),
                    dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetName().CString() : "NONE",
                    dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetID() : 0);
}

void Player::OnTalkQuestion(StringHash eventType, VariantMap& eventData)
{
    // Open UI Question Panel
    /// TODO
}

void Player::OnTalkShowResponses(StringHash eventType, VariantMap& eventData)
{
    if (!dialogInteractor_)
        return;

    unsigned actorid = eventData[Dialog_Response::ACTOR_ID].GetUInt();

    URHO3D_LOGINFOF("Player() - OnTalkShowResponses ... actorid=%u", actorid);

    if (dialogInteractor_->GetID() == actorid)
    {
        // Get Dialogue Responses
        StringHash dialogueKey = eventData[Dialog_Response::DIALOGUENAME].GetStringHash();
        int dialogueMsgIndex   = eventData[Dialog_Response::DIALOGUEMSGID].GetInt();
        Vector<const DialogueResponse*> responses = GameData::GetDialogueData()->GetReponses(dialogueKey, dialogueMsgIndex, this);

        if (!responses.Size())
        {
            // if no responses, close dialogue
            eventData[Dialog_Close::ACTOR_ID] = actorid;
            eventData[Dialog_Close::PLAYER_ID] = GetID();
            dialogInteractor_->SendEvent(DIALOG_CLOSE, eventData);
            dialogInteractor_.Reset();
            return;
        }

        // Open UI Response Panel
        UIPanel* panel = UIPanel::GetPanel(String("PlayerDiag")+String(GetID()));
        if (panel)
        {
            UIElement* uiroot = panel->GetElement();
            unsigned numresponses = Min(responses.Size(), DIALOGUE_MAXRESPONSES);

            URHO3D_LOGINFOF("Player() - OnTalkShowResponses ... numresponses=%u ", numresponses);

            // Set and Show the responses
            for (unsigned i=0; i < numresponses; i++)
            {
                const DialogueMessage* responsemessage = GameData::GetDialogueData()->GetMessage(responses[i]->name_, this);
                UIElement* choice = uiroot->GetChild(i);
                choice->GetChildStaticCast<Text>(0)->SetText(responsemessage->text_);
                choice->SetName(String(responses[i]->next_.Value()));
                choice->SetVisible(true);
                SubscribeToEvent(choice, E_RELEASED, URHO3D_HANDLER(Player, OnTalkResponseSelected));
            }

            // Hide other buttons
            if (numresponses < DIALOGUE_MAXRESPONSES)
                for (unsigned i=numresponses; i < DIALOGUE_MAXRESPONSES; i++)
                    uiroot->GetChild(i)->SetVisible(false);

            panel->SetVisible(true);
        }

        URHO3D_LOGINFOF("Player() - OnTalkShowResponses ... OK !");
    }
}

void Player::OnTalkResponseSelected(StringHash eventType, VariantMap& eventData)
{
    if (!dialogInteractor_)
        return;

    UIElement* button = (UIElement*)eventData[Released::P_ELEMENT].GetPtr();
    if (button)
    {
        StringHash next(ToUInt(button->GetName()));

        URHO3D_LOGINFOF("Player() - OnTalkResponseSelected : next = %u", next.Value());

        // Close Dialog UI
        UIPanel* panel = UIPanel::GetPanel(String("PlayerDiag")+String(GetID()));
        if (panel)
            panel->SetVisible(false);

        // Send Choice to Interactor
        eventData[Dialog_Next::ACTOR_ID] = dialogInteractor_->GetID();
        eventData[Dialog_Next::PLAYER_ID] = GetID();
        eventData[Dialog_Next::DIALOGUENAME] = next;
        dialogInteractor_->SendEvent(DIALOG_NEXT, eventData);
    }
}

void Player::OnTalkEnd(StringHash eventType, VariantMap& eventData)
{
    if (!dialogInteractor_ || (GetID() == eventData[Dialog_Close::PLAYER_ID].GetUInt() &&
                               dialogInteractor_->GetID() == eventData[Dialog_Close::ACTOR_ID].GetUInt()))
    {
        URHO3D_LOGINFOF("Player() - OnTalkEnd ... player=%s(%u) interactor=%s(%u)", avatar_->GetName().CString(), avatar_->GetID(),
                        dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetName().CString() : "NONE",
                        dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetID() : 0);

        // Close Dialog UI
        UIPanel* panel = UIPanel::GetPanel(String("PlayerDiag")+String(GetID()));
        if (panel)
            panel->SetVisible(false);

        // Close Bargain UI
        UIC_ShopPanel* shoppanel = static_cast<UIC_ShopPanel*>(UIPanel::GetPanel(String("PlayerShop")+String(GetID())));
        if (shoppanel)
            shoppanel->ResetShop();

        dialogInteractor_.Reset();

//        SetAllDialogMarkersEnable(true);

        URHO3D_LOGINFOF("Player() - OnTalkEnd ... OK !");
    }
}

void Player::OnBargain(StringHash eventType, VariantMap& eventData)
{
    if (dialogInteractor_)
    {
        URHO3D_LOGINFOF("Player() - OnBargain ...");
        // Open Bargain UI
        UIPanel* panel = UIPanel::GetPanel(String("PlayerShop")+String(GetID()));
        if (panel)
        {
            panel->Start(this, dialogInteractor_);

            if (!panel->IsVisible())
                panel->ToggleVisible();

            URHO3D_LOGINFOF("Player() - OnBargain ... panel visible OK !");
        }
    }
}


void Player::HandleDelayedActions(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GO_PLAYERMISSIONMANAGERSTART && active)
    {
        if (missionManager_)
            missionManager_->Start();
        UnsubscribeFromEvent(this, GO_PLAYERMISSIONMANAGERSTART);
    }
}

void Player::HandleClic(StringHash eventType, VariantMap& eventData)
{
    if (UISlotPanel::draggedElement_ && eventType != E_RELEASED)
    {
        UISlotPanel::draggedElement_.Reset();
        URHO3D_LOGINFOF("Player() - HandleClic : ID=%u skip draggedElement_ => return !", GetID());
        return;
    }

    int x, y;

    if (eventType == E_TOUCHEND)
    {
        x = eventData[TouchEnd::P_X].GetInt();
        y = eventData[TouchEnd::P_Y].GetInt();
        UIElement* uiElement = GameContext::Get().ui_->GetElementAt(x / GameContext::Get().uiScale_, y / GameContext::Get().uiScale_);
        if (uiElement)
        {
//            URHO3D_LOGINFOF("Player() - HandleClic : ID=%u uiElement at %d %d => return !", GetID(), x, y);
            return;
        }
        URHO3D_LOGINFOF("Player() - HandleClic : ID=%u E_TOUCHEND  at %d %d", GetID(), x, y);
    }
    else if (eventType == E_MOUSEBUTTONUP)
    {
        if (eventData[MouseButtonUp::P_BUTTON].GetInt() == MOUSEB_LEFT)
        {
            UIElement* uielt;
            GameHelpers::GetInputPosition(x, y, &uielt);
            if (uielt)
            {
//				URHO3D_LOGINFOF("Player() - HandleClic : ID=%u uiElement(%s) at %d %d => return !", GetID(), uielt->GetName().CString(), x, y);
                return;
            }
            URHO3D_LOGINFOF("Player() - HandleClic : ID=%u E_MOUSEBUTTONUP at %d %d", GetID(), x, y);
        }
        else
        {
            return;
        }

    }
    else if (eventType == E_RELEASED)
    {
        // for skip E_MOUSEBUTTONUP on after a ui button release
        GameHelpers::GetInputPosition(lastxonui_, lastyonui_);
//        URHO3D_LOGINFOF("Player() - HandleClic : ID=%u skip E_MOUSEBUTTONUP on after a ui button release at %d %d => return !", GetID(), x, y);
        return;
    }

    // for skip E_MOUSEBUTTONUP on after a ui button release
    if (lastxonui_ == x && lastyonui_ == y)
    {
//        URHO3D_LOGINFOF("Player() - HandleClic : ID=%u on the last prev UI element at %d %d", GetID(), x, y);
        return;
    }

    int viewportid = ViewManager::Get()->GetViewportAt(x, y);
    Viewport* viewport = GetSubsystem<Renderer>()->GetViewport(viewportid);
    Ray ray = viewport->GetScreenRay(x, y);

    Vector3 wpoint3 = viewport->ScreenToWorldPoint(x, y, ray.HitDistance(GameContext::Get().GroundPlane_));
    Vector2 wpoint = Vector2(wpoint3.x_, wpoint3.y_);

    RigidBody2D* body = 0;
    CollisionShape2D* shape = 0;
    scene_->GetComponent<PhysicsWorld2D>()->GetPhysicElements(wpoint, body, shape);

    if (avatar_ && body && shape)
    {
        URHO3D_LOGINFOF("Player() - HandleClic : ID=%u at %d %d", GetID(), x, y);

        Node* node = body->GetNode();

        if (shape->IsTrigger())
        {
            VariantMap goEventData;
            body->GetNode()->SendEvent(GO_TRIGCLICKED, goEventData);
            URHO3D_LOGINFOF("Player() - HandleClic : ID=%u - Clic on the Trigger of %s(%u) shapeviewz=%d nodepos=%s", GetID(), node->GetName().CString(), node->GetID(), shape->GetViewZ(), node->GetWorldPosition2D().ToString().CString());
        }
        else
        {
            if (body->GetBodyType() == BT_DYNAMIC)
            {
                URHO3D_LOGINFOF("Player() - HandleClic : ID=%u Select entity %s(%u) enabled=%s shapeviewz=%d nodepos=%s",
                                GetID(), node->GetName().CString(), node->GetID(), node->IsEnabled() ? "true":"false", shape->GetViewZ(), node->GetWorldPosition2D().ToString().CString());

                GOC_Destroyer* gocdestroyer = node->GetComponent<GOC_Destroyer>();
                if (gocdestroyer)
                    gocdestroyer->DumpWorldMapPositions();

//                VariantMap goEventData;
                VariantMap& goEventData = context_->GetEventDataMap();
                goEventData[Net_ObjectCommand::P_NODEID] = avatar_->GetID();
                goEventData[Go_Selected::GO_ID] = node->GetID();
                goEventData[Go_Selected::GO_TYPE] = node->GetVar(GOA::TYPECONTROLLER).GetInt();
                goEventData[Go_Selected::GO_ACTION] = 0;
                avatar_->SendEvent(GO_SELECTED, goEventData);
                return;
            }
            else if (!dialogInteractor_)
            {
                if (shape->GetMaskBits() == CM_SCENEUI)
                    //            if (node->GetName() == "DialogMarker")
                {
                    if (shape->GetCategoryBits() == CC_SCENEUI)
                    {
                        Actor* actor = Actor::GetWithNode(node->GetParent());
                        Actor* player = Actor::Get(viewportid+1);

                        if (actor && player && player->SetDialogueInterator(actor))
                        {
                            // Player Start a dialog
                            URHO3D_LOGINFOF("Player() - HandleClic : Clic on DialogMarker in Viewport=%d => open dialog player=%s(%u-actorid=%u) with node=%s(%u-actorid=%u) ",
                                            viewportid, node->GetName().CString(), node->GetID(), actor->GetID(), player->GetAvatar()->GetName().CString(), player->GetAvatar()->GetID(), player->GetID());

                            player->SetFaceTo(actor->GetAvatar());

//                            VariantMap goEventData;
                            VariantMap& goEventData = context_->GetEventDataMap();
                            goEventData[Dialog_Open::ACTOR_ID] = actor->GetID();
                            goEventData[Dialog_Open::PLAYER_ID] = player->GetID();
                            actor->SendEvent(DIALOG_OPEN, goEventData);
                            player->SendEvent(DIALOG_OPEN, goEventData);
                            return;
                        }
                    }
                }
                else
                {
                    URHO3D_LOGINFOF("Player() - HandleClic : ID=%u - Clic On Point on %s(%u) enabled=%s shapeviewz=%d nodepos=%s",
                                    GetID(), node->GetName().CString(), node->GetID(), node->IsEnabled()?"true":"false", shape->GetViewZ(), node->GetWorldPosition2D().ToString().CString());
                }
            }
        }
    }

    if (avatar_ && !dialogInteractor_ && abilities_ && !avatar_->GetVar(GOA::ISDEAD).GetBool())
    {
        Ability* ability = abilities_->GetActiveAbility();
        if (ability)
        {
            URHO3D_LOGINFOF("Player() - HandleClic : ID=%u at %d %d", GetID(), x, y);

            StringHash abi(ability->GetType());
            if (abi == ABI_Grapin::GetTypeStatic() || abi == ABI_Shooter::GetTypeStatic() || abi == ABIBomb::GetTypeStatic() || abi == ABI_AnimShooter::GetTypeStatic())
                ability->UseAtPoint(wpoint);
        }
    }

//    gocController->FindAndFollowPath(wpoint);
}

void Player::HandleUpdateTimePeriod(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGERRORF("Player() - HandleUpdateTimePeriod !");

    bool uselit = GameHelpers::SetLightActivation(this);
}

void Player::DrawDebugGeometry(DebugRenderer* debugRenderer) const
{
    if (gocDestroyer_)
        gocDestroyer_->DrawDebugGeometry(debugRenderer, false);
}
