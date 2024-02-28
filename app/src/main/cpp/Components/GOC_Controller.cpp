#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/SmoothedTransform.h>
#include <Urho3D/Network/Connection.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>

#include "GameNetwork.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameOptions.h"

#include "GOC_Animator2D.h"
#include "GOC_Life.h"
#include "GOC_Move2D.h"
#include "GOC_Destroyer.h"

#ifdef ACTIVE_PATHFINDER
#include "PathFinder2D.h"
#endif

#include "Player.h"
#include "ViewManager.h"

#include "GOC_Controller.h"


#define NOREVERSE


GOC_Controller::GOC_Controller(Context* context) :
    Component(context),
    mainController_(false),
    controlActionEnable_(true),
    interaction_(false),
    controlType_(GO_None),
    currentIdPath_(-1),
    thinker_(0),
    lastdesync_(true)
{ }

GOC_Controller::GOC_Controller(Context* context, int type) :
    Component(context),
    mainController_(false),
    controlActionEnable_(true),
    interaction_(false),
    controlType_(type),
    currentIdPath_(-1),
    thinker_(0),
    lastdesync_(true)
{ }

GOC_Controller::~GOC_Controller()
{
//    URHO3D_LOGINFOF("~GOC_Controller()");
    Stop();
}

void GOC_Controller::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Controller>();
}


void GOC_Controller::Start()
{
//    URHO3D_LOGINFOF("GOC_Controller() - Start : Type=%d", controlType_);
    ResetButtons();
    ResetDirection();

    // Pourquoi ça ?
//    node_->GetDerivedComponent<Drawable2D>()->SetViewMask(-1);

#ifdef ACTIVE_NETWORK_LOCALPHYSICSIMULATION_VELOCITY_NONMAIN
    if (!mainController_)
        SubscribeToEvent(E_UPDATESMOOTHING, URHO3D_HANDLER(GOC_Controller, HandleNetUpdate));
#endif
}

void GOC_Controller::Stop()
{
    // Always umount on stop
    Unmount();

    ResetButtons();
    ResetDirection();
    StopFollowPath();

#ifdef ACTIVE_NETWORK_LOCALPHYSICSIMULATION_VELOCITY_NONMAIN
    if (HasSubscribedToEvent(E_UPDATESMOOTHING))
        UnsubscribeFromEvent(E_UPDATESMOOTHING);
#endif
}


void GOC_Controller::CheckMountNode()
{
    const unsigned mountid = node_->GetVar(GOA::ISMOUNTEDON).GetUInt();
    if (mountid)
    {
        if (node_->GetParent() == GetScene() || node_->GetParent() == 0 || node_->GetParent()->GetVar(GOA::ISDEAD).GetBool() || node_->GetVar(GOA::ISDEAD).GetBool())
            bool ok = Unmount();
    }
}

bool GOC_Controller::MountOn(Node* target)
{
    if (!target)
        return false;

    // already mounted on a node
    const unsigned prevTargetID = node_->GetVar(GOA::ISMOUNTEDON).GetUInt();
    if (prevTargetID == target->GetID())
        return false;

    URHO3D_LOGINFOF("GOC_Controller() - MountOn : %s(%u) on node=%s(%u) ...",
                    node_->GetName().CString(), node_->GetID(), target->GetName().CString(), target->GetID());

    // unmount before mount on another target
    if (prevTargetID && !GOC_Controller::Unmount())
        return false;

    GOC_Abilities* gocability = node_->GetComponent<GOC_Abilities>();
    Ability* ability = gocability && gocability->GetActiveAbility() ? gocability->GetActiveAbility() : 0;
    if (ability)
        gocability->SetActiveAbility(0);

    node_->SetVar(GOA::ISMOUNTEDON, target->GetID());

    RigidBody2D* body = node_->GetComponent<RigidBody2D>();

    Node* mountNode = target->GetChild(MOUNTNODE, true);
    if (mountNode)
    {
        body->SetEnabled(false);

        // don't use setparent (hang program), prefer addchild
        Vector2 scale = node_->GetWorldScale2D();
        mountNode->AddChild(node_);
        node_->SetWorldScale2D(scale);

//        node_->SetParent(mountNode);

        node_->SetPosition2D(Vector2::ZERO);
    }
    else
    {
        Rect rect;
        target->GetComponent<GOC_Destroyer>()->GetWorldShapesRect(rect);

        node_->SetParent(target);
        node_->SetWorldPosition2D(rect.Center().x_, rect.max_.y_);

        // Divide Mass
        body->SetMass(body->GetMass() * 0.5f);

        // Normally, we don't have to disable body, but with network it's lagging ...
        body->SetEnabled(false);

        ConstraintWeld2D* constraintWeld = node_->CreateComponent<ConstraintWeld2D>();
        constraintWeld->SetOtherBody(target->GetComponent<RigidBody2D>());
        constraintWeld->SetAnchor(node_->GetWorldPosition2D());
        constraintWeld->SetFrequencyHz(4.0f);
        constraintWeld->SetDampingRatio(0.5f);
    }

    node_->GetComponent<GOC_Move2D>()->SetMoveType(MOVE2D_MOUNT);
    node_->GetComponent<GOC_Animator2D>()->SetState(STATE_IDLE);
    node_->GetDerivedComponent<Drawable2D>()->SetLayer(target->GetDerivedComponent<Drawable2D>()->GetLayer());
    if (target->GetComponent<GOC_Animator2D>())
        node_->GetComponent<GOC_Animator2D>()->SetDirection(target->GetComponent<GOC_Animator2D>()->GetDirection());

    GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
    if (destroyer)
    {
        // Always disable Unstuck on all client/client distant/server
        destroyer->SetEnableUnstuck(false);
        destroyer->SetDelegateNode(target);
        destroyer->SetViewZ();
    }

    if (thinker_)
        static_cast<Player*>(thinker_)->OnMount(target);

    SubscribeToEvent(target, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_Controller, OnMountNodeDead));

    if (ability)
        gocability->SetActiveAbility(ability);

    URHO3D_LOGINFOF("GOC_Controller() - MountOn : %s(%u) on node=%s(%u) ... OK !",
                    node_->GetName().CString(), node_->GetID(), target->GetName().CString(), target->GetID());

    return true;
}

bool GOC_Controller::Unmount()
{
    if (!node_)
        return false;

    const unsigned targetid = node_->GetVar(GOA::ISMOUNTEDON).GetUInt();

    if (!targetid || !node_->GetParent())
        return false;

    URHO3D_LOGINFOF("GOC_Controller() - Unmount : %s(%u) ... targetid=%u ...", node_->GetName().CString(), node_->GetID(), targetid);

    GOC_Abilities* gocability = node_->GetComponent<GOC_Abilities>();
    Ability* ability = gocability && gocability->GetActiveAbility() ? gocability->GetActiveAbility() : 0;
    if (ability)
        gocability->SetActiveAbility(0);

    Node* target = 0;

    if (node_->GetParent()->GetName().StartsWith(MOUNTNODE))
    {
        target = node_->GetParent();
        while (target && target->GetID() != targetid)
            target = target->GetParent();
        if (target && target->GetID() != targetid)
            target = 0;

        Vector2 wpos  = node_->GetWorldPosition2D();
        Vector2 scale = node_->GetWorldScale2D();
        node_->GetScene()->AddChild(node_);
        node_->SetWorldScale2D(scale);
//        node_->SetParent(node_->GetScene());
        node_->SetWorldPosition2D(wpos);

        node_->GetComponent<RigidBody2D>()->SetEnabled(true);
    }
    else
    {
        if (targetid)
        {
            // Get Contraints on the node and remove
            PODVector<ConstraintWeld2D* > contraints;
            node_->GetComponents<ConstraintWeld2D>(contraints);
            for (PODVector<ConstraintWeld2D* >::Iterator it=contraints.Begin(); it!=contraints.End(); ++it)
            {
                ConstraintWeld2D* constraint = *it;
                if (constraint && constraint->GetOtherBody()->GetNode()->GetID() == targetid)
                {
                    target = constraint->GetOtherBody()->GetNode();
                    constraint->ReleaseJoint();
                    constraint->Remove();
                }
            }
            // Get the Contraints on the parent and remove
            contraints.Clear();
            node_->GetParent()->GetComponents<ConstraintWeld2D>(contraints);
            for (PODVector<ConstraintWeld2D* >::Iterator it=contraints.Begin(); it!=contraints.End(); ++it)
            {
                ConstraintWeld2D* constraint = *it;
                if (constraint && constraint->GetOtherBody()->GetNode()->GetID() == node_->GetID())
                {
                    target = constraint->GetOtherBody()->GetNode();
                    constraint->ReleaseJoint();
                    constraint->Remove();
                }
            }
        }

        // Restore the mass
        RigidBody2D* body = node_->GetComponent<RigidBody2D>();
        if (body)
        {
            body->SetMass(body->GetMass() * 2.f);
            body->SetEnabled(true);
        }
        // Set Parent
        node_->SetParent(node_->GetScene());
    }

    if (target)
    {
        UnsubscribeFromEvent(target, GOC_LIFEDEAD);
        GOC_Move2D* move2d = target->GetComponent<GOC_Move2D>();
        if (move2d)
            move2d->ResetMoveType();
    }

    node_->SetVar(GOA::ISMOUNTEDON, 0U);

    node_->SetRotation2D(0.f);
    node_->SetWorldRotation2D(0.f);

    GOC_Move2D* move2d = node_->GetComponent<GOC_Move2D>();
    if (move2d)
        move2d->ResetMoveType();

    if (!node_->GetVar(GOA::ISDEAD).GetBool())
        node_->GetComponent<GOC_Animator2D>()->ResetState();

    GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
    if (destroyer)
    {
        destroyer->SetDelegateNode(0);

        if (!destroyer->IsInsideWorld())
        {
            Vector2 position;

            URHO3D_LOGINFOF("GOC_Controller() - Unmount : %s(%u) ... Outside World => adjust !", node_->GetName().CString(), node_->GetID());

            GOC_Destroyer* targetdestroyer = target ? target->GetComponent<GOC_Destroyer>() : 0;
            if (targetdestroyer && targetdestroyer->IsInsideWorld())
            {
                const WorldMapPosition& targetpos = targetdestroyer->GetWorldMapPosition();
                position = targetpos.position_;
                destroyer->AdjustPosition(targetpos.mPoint_, targetpos.viewZ_, position);
            }

            node_->SetWorldPosition2D(position);
        }

        destroyer->SetViewZ();
    }

    if (thinker_)
        static_cast<Player*>(thinker_)->OnUnmount(targetid);

    if (ability)
        gocability->SetActiveAbility(ability);

    URHO3D_LOGINFOF("GOC_Controller() - Unmount : %s(%u) ... OK !", node_->GetName().CString(), node_->GetID());

    return true;
}


void GOC_Controller::ResetButtons()
{
    control_.buttons_ = 0;
    buttonholdtime_ = 0;

    if (node_)
    {
        node_->SetVar(GOA::BUTTONS, 0);
        node_->SendEvent(GOC_CONTROLUPDATE);
    }
}

void GOC_Controller::ResetDirection()
{
    control_.direction_ = 0.f;

    if (node_)
        node_->SendEvent(GO_UPDATEDIRECTION);
}

void GOC_Controller::SetControllerType(int type, bool force)
{
    bool oldcontroltype = controlType_;
    controlType_ = type;

    if (node_)
    {
        node_->SetVar(GOA::TYPECONTROLLER, type);

        unsigned faction = 0;
        if (type & (GO_Player|GO_NetPlayer|GO_AI_Ally))
            faction = GameContext::Get().LocalMode_ ? type : (node_->GetVar(GOA::CLIENTID).GetUInt() << 8) + GO_Player;

        node_->SetVar(GOA::FACTION, faction);

        if (controlType_ != oldcontroltype || force)
        {
            if (!node_->isPoolNode_ || (node_->isPoolNode_ && node_->isInPool_))
            {
//                URHO3D_LOGERRORF("GOC_Controller() - SetControllerType : %s(%u) change from type=%d to type=%d", node_->GetName().CString(), node_->GetID(), oldcontroltype, controlType_);
                VariantMap& eventData = context_->GetEventDataMap();
                eventData[ControllerChange::NODE_ID] = node_->GetID();
                eventData[ControllerChange::GO_TYPE] = controlType_;
                eventData[ControllerChange::GO_MAINCONTROL] = mainController_;
                eventData[ControllerChange::NEWCTRL_PTR] = this;
                node_->SendEvent(GOC_CONTROLLERCHANGE, eventData);
            }
        }

//        URHO3D_LOGINFOF("GOC_Controller() - SetControllerType type=%d faction=%u", controlType_, node_->GetVar(GOA::FACTION).GetUInt());
    }
}

//void GOC_Controller::SetEnableObjectControl(bool enable)
//{
//    URHO3D_LOGINFOF("GOC_Controller() - SetEnableObjectControl : node=%u enableobjectcontrol=%s", node_->GetID(), enable ? "true":"false");
//    objectControlEnable_ = enable;
//}

void GOC_Controller::SetMainController(bool maincontroller)
{
    if (!node_)
        return;

//    URHO3D_LOGINFOF("GOC_Controller() - SetMainController node=%s(%u) maincontroller=%s", node_->GetName().CString(), node_->GetID(), maincontroller ? "true":"false");

    bool oldmaincontroller = mainController_;

    if (mainController_ != maincontroller)
        mainController_ = maincontroller;

    SetControllerType(controlType_, oldmaincontroller != mainController_);

    if (mainController_ != oldmaincontroller && IsEnabledEffective())
    {
        Stop();

        Start();
    }
}

bool GOC_Controller::ChangeAvatarOrEntity(unsigned type, unsigned char entityid)
{
    if (thinker_)
    {
        return static_cast<Player*>(thinker_)->ChangeAvatar(type, entityid, true);
    }
    else if (control_.type_ != type || control_.entityid_ != entityid)
    {
        bool ok = false;

        if (control_.type_ != type && type)
        {
            Node* templateNode = GOT::GetControllableTemplate(StringHash(type));
            if (templateNode && node_->GetNumComponents() >= templateNode->GetNumComponents())
            {
                URHO3D_LOGINFOF("GOC_Controller() - ChangeAvatarOrEntity : nodeid=%u from %s(%u) to type=%s(%u) entityid=%u ... CopyAttributes From Template",
                                node_->GetID(), GOT::GetType(StringHash(control_.type_)).CString(), control_.type_,
                                GOT::GetType(StringHash(type)).CString(), type, entityid);

                // unmount if mounted
                MountInfo mountinfo(node_);
                GameHelpers::UnmountNode(mountinfo);

                // Important in Networking : Backup Position/layering before changing avatar for the SpawnEffect
                Vector2 position = node_->GetWorldPosition2D();
                Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
                int clientid = node_->GetVar(GOA::CLIENTID).GetInt();
                int layer = drawable->GetLayer();
                unsigned viewmask = drawable->GetViewMask();

                GameHelpers::CopyAttributes(templateNode, node_, false, true);

                // Prevent apparition of Children like AileDark or SpiderThread (see TODO BEHAVIOR:26/04/2020)
    //            node_->RemoveAllChildren();

                SetControllerType(GO_NetPlayer, true);

                GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
                if (destroyer)
                    destroyer->SetDestroyMode(DISABLE);

                ResetDirection();

                int eid = entityid;
                GameHelpers::SetEntityVariation(node_->GetComponent<AnimatedSprite2D>(), LOCAL, eid);
                control_.type_ = type;
                control_.entityid_ = eid;

                node_->SetVar(GOA::CLIENTID, clientid);

                node_->SetEnabled(true);

                Light* light = node_->GetComponent<Light>();
                if (light)
                    light->SetEnabled(false);

                node_->ApplyAttributes();

                node_->AddTag(PLAYERTAG);

                // remount if was mounted
                GameHelpers::MountNode(mountinfo);

                // Spawn Effect
                GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_LIFEFLAME], layer, viewmask, position, 0.f, 1.f, true, 1.f, Color::BLUE, LOCAL);

                ok = true;
            }
        }

        if (!ok && control_.type_ && control_.entityid_ != entityid)
        {
//            URHO3D_LOGINFOF("GOC_Controller() - ChangeAvatarOrEntity : node=%s(%u) from entityid=%u to %u ...", node_->GetName().CString(), node_->GetID(), control_.entityid_, entityid);

            int eid = entityid;
            const GOTInfo& gotinfo = GOT::GetConstInfo(StringHash(control_.type_));
            GameHelpers::SetEntityVariation(node_->GetComponent<AnimatedSprite2D>(), LOCAL, eid, &gotinfo);
            control_.entityid_ = entityid;

            ok = true;
        }

        return ok;
    }

    return false;
}

// used by GOC_PlayerController::HandleLocalUpdate
// used by GOC_AIController::HandleUpdate
// used by GameNetwork::UpdateControl
bool GOC_Controller::Update(unsigned buttons, bool forceUpdate)
{
    if (!GameContext::Get().physicsWorld_->IsUpdateEnabled())
        return false;

    if (prevbuttons_ == buttons)
    {
        if (mainController_)
        {
            if (control_.IsButtonDown(CTRL_FIRE2))
            {
                if (buttonholdtime_ == ButtonHoldThreshold)
                    node_->SendEvent(GOC_CONTROLACTION2HOLD);
                else
                    buttonholdtime_++;
            }
        }

        if (!forceControlUpdate_ && !forceUpdate)
            return false;
    }
    else
    {
        if (buttonholdtime_)
        {
            buttonholdtime_ = 0U;
            node_->SendEvent(GOC_CONTROLACTIONSTOP);
        }
    }
    // Patch for GameNetwork to resolve "Ghosting" pb 11/11/2023.
    if (prevbuttons_ != buttons || forceUpdate)
    {
        prevbuttons_ = control_.buttons_;
        control_.buttons_ = buttons;
        node_->SetVar(GOA::BUTTONS, buttons);

//        URHO3D_LOGINFOF("GOC_Controller() - Update : buttons(prev)=%u(%u) mainController=%s forceUpdate=%s", control_.buttons_, prevbuttons_, mainController_ ? "true":"false", forceUpdate ? "true":"false");

        node_->SendEvent(GOC_CONTROLUPDATE);
    }

//    if (control_.IsButtonDown(CTRL_JUMP))
//        node_->SendEvent(GOC_CONTROLACTION0);

    if (mainController_)
    {
        if (controlActionEnable_)
        {
            if (control_.IsButtonDown(CTRL_FIRE1))
            {
    //            URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE1 buttons(prev)=%u(%u)", control_.buttons_, prevbuttons_);
                node_->SendEvent(GOC_CONTROLACTION1);
            }

            if (control_.IsButtonDown(CTRL_FIRE2))
            {
                GOC_Animator2D* gocanimator = node_->GetComponent<GOC_Animator2D>();
                bool canUseAction2 = gocanimator->HasAnimationForState(STATE_POWER1) || gocanimator->HasAnimationForState(STATE_POWER2);
                if (!canUseAction2)
                {
                    GOC_Abilities* gocabilities = node_->GetComponent<GOC_Abilities>();
                    if (gocabilities)
                    {
                        if (controlType_ & GO_Player)
                            canUseAction2 = gocabilities->GetActiveAbility() != 0;
                        else
                        {
                            Ability* ability = gocabilities->GetAbility(ABI_AnimShooter::GetTypeStatic());
                            canUseAction2 = ability && gocabilities->SetActiveAbility(ability);
                        }
                    }
                }

                if (canUseAction2)
                {
                    URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE2 buttons(prev)=%u(%u)", control_.buttons_, prevbuttons_);
                    node_->SendEvent(GOC_CONTROLACTION2);
                }
                else
                {
                    URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE2 can't use");
                }
            }

            if (control_.IsButtonDown(CTRL_FIRE3))
            {
    //            URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE3 buttons(prev)=%u(%u)", control_.buttons_, prevbuttons_);
                node_->SendEvent(GOC_CONTROLACTION3);
            }
        }
    }

    forceControlUpdate_ = false;

    return true;
}


void GOC_Controller::FindAndFollowPath(const Vector2& destination)
{
#ifdef ACTIVE_PATHFINDER
    currentIdPath_ = PathFinder2D::FindPath(GetNode()->GetWorldPosition2D(), ViewManager::FRONTVIEW_Index, destination, ViewManager::FRONTVIEW_Index, MV_WALK);
    if (currentIdPath_ != -1)
    {
        currentPath_ = PathFinder2D::GetPath(currentIdPath_);
        currentIdUserPath_ = PathFinder2D::SetNodeOnPath(currentIdPath_, GetNode());
        followPath_ = true;
        noreverse_ = false;
        lastimpulse_ = false;
        lastcount_ = 0;
    }
#endif
}

void GOC_Controller::StopFollowPath()
{
    currentIdPath_ = -1;
    currentPath_ = 0;
    followPath_ = false;
}

void GOC_Controller::UpdateFollowPath()
{
#ifdef ACTIVE_PATHFINDER
    Path2D* path = (Path2D*) currentPath_;

    Vector2 p = node_->GetWorldPosition2D();
    int index = PathFinder2D::GetPathIndex(path, currentIdUserPath_, p);

    // has reached destination, stabilize
    bool stabilize = (index == -1);

    if (stabilize)
        index = PathFinder2D::GetLastPathIndex(path);

    p = PathFinder2D::GetPathPosition(path, index) - p;

    if (stabilize)
    {
        stabilize = true;

        if (!lastimpulse_)
        {
            lastcount_++;
            if (lastcount_ > 2)
            {
                lastimpulse_ = true;
                lastcount_ = 0;
            }
        }

        const Vector2& segment = PathFinder2D::GetPathSegment(path, index);
        if (segment.x_ > 0.f && p.x_ > 0.f)
        {
            control_.SetButtons(CTRL_RIGHT, false);
            control_.SetButtons(CTRL_LEFT, lastimpulse_); // true
#ifdef NOREVERSE
            if (lastimpulse_)
            {
                node_->SetVar(GOA::KEEPDIRECTION, true);
                noreverse_ = true;
            }
#endif
//            URHO3D_LOGINFOF("updatefollowPath Has Reached Destination, stabilize on the last point lastcount=%u",lastcount_);
        }
        else if (segment.x_ < 0.f && p.x_ < 0.f)
        {
            control_.SetButtons(CTRL_RIGHT, lastimpulse_);  // true
            control_.SetButtons(CTRL_LEFT, false);
#ifdef NOREVERSE
            if (lastimpulse_)
            {
                node_->SetVar(GOA::KEEPDIRECTION, true);
                noreverse_ = true;
            }
#endif
//            URHO3D_LOGINFOF("updatefollowPath Has Reached Destination, stabilize on the last point lastcount=%u",lastcount_);
        }

        if (lastimpulse_)
        {
            control_.SetButtons(CTRL_RIGHT, false);
            control_.SetButtons(CTRL_LEFT, false);
            control_.SetButtons(CTRL_JUMP, false);
            currentIdPath_ = -1;
            followPath_ = false;
            lastcount_ = 0;
#ifdef NOREVERSE
            if (noreverse_)
            {
                node_->SetVar(GOA::KEEPDIRECTION, false);
                node_->SendEvent(GO_CHANGEDIRECTION);
            }
#endif
            forceControlUpdate_ = true;

            URHO3D_LOGINFOF("updatefollowPath - Stop Near End p.x_=%f p.y_=%f", p.x_, p.y_);
        }

        lastimpulse_ = false;
        return;
    }

    if (p.y_ > 1.2f * Abs(p.x_))
    {
//        URHO3D_LOGINFOF("updatefollowPath2 toJump p.x_=%f p.y_=%f", p.x_, p.y_);
        control_.SetButtons(CTRL_LEFT, false);
        control_.SetButtons(CTRL_RIGHT, false);
        control_.SetButtons(CTRL_JUMP, true);
        forceControlUpdate_ = true;
    }
    else
    {
//        URHO3D_LOGINFOF("updatefollowPath1 p.x_=%f p.y_=%f", p.x_, p.y_);
        control_.SetButtons(CTRL_LEFT, p.x_ < 0.f);
        control_.SetButtons(CTRL_RIGHT, p.x_ > 0.f);
        control_.SetButtons(CTRL_JUMP, p.y_ > 1.15f * Abs(p.x_));
    }

    // if near last point and velocity > limit, reverse to init stabilization on point
    if (PathFinder2D::IsOnLastPathSegment(path, index) && p.x_ > -1.f && p.x_ < 1.f)
    {
        Vector2 vel = node_->GetVar(GOA::VELOCITY).GetVector2();
        if (Abs(vel.x_) < 1.5f)
            return;

        // simulate an impulsion
        if (!lastimpulse_)
        {
            lastcount_++;
            if (lastcount_ > 1) // counter before activate impulse
            {
                lastimpulse_ = true;
                lastcount_ = 0;
            }
        }

        const Vector2& segment = PathFinder2D::GetPathSegment(path, index);
        if (segment.x_ > 0.f && p.x_ > 0.f)
        {
            control_.SetButtons(CTRL_RIGHT, false);
            control_.SetButtons(CTRL_LEFT, lastimpulse_); // true
#ifdef NOREVERSE
            if (lastimpulse_)
            {
                node_->SetVar(GOA::KEEPDIRECTION, true);
                noreverse_ = true;
            }
#endif

//            URHO3D_LOGINFOF("updatefollowPath - Reverse Before Stop p.x_=%f p.y_=%f index=%d velx=%f", p.x_, p.y_, index, vel.x_);
        }
        else if (segment.x_ < 0.f && p.x_ < 0.f)
        {
            control_.SetButtons(CTRL_RIGHT, lastimpulse_);  // true
            control_.SetButtons(CTRL_LEFT, false);
#ifdef NOREVERSE
            if (lastimpulse_)
            {
                node_->SetVar(GOA::KEEPDIRECTION, true);
                noreverse_ = true;
            }
#endif
//            URHO3D_LOGINFOF("updatefollowPath - Reverse Before Stop p.x_=%f p.y_=%f index=%d velx=%f", p.x_, p.y_, index, vel.x_);
        }
        lastimpulse_ = false;
    }
#endif
}


void GOC_Controller::OnSetEnabled()
{
    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
            Start();
        else
            Stop();
    }
}

void GOC_Controller::OnNodeSet(Node* node)
{
    if (node)
    {
//        URHO3D_LOGINFOF("GOC_Controller() - OnNodeSet : Type=(%d %d)", GetControllerType(), controlType_);

        GOC_Controller* prevGocControl = node->GetDerivedComponent<GOC_Controller>();
        if (prevGocControl && prevGocControl != this)
        {
//            URHO3D_LOGINFOF("GOC_Controller() - OnNodeSet : remove old Control Type=%d on node ID=%u", prevGocControl->GetControllerType(), node->GetID());
            node->RemoveComponent(prevGocControl);
        }

        SetControllerType(GetControllerType(), true);

        OnSetEnabled();
    }
}

void GOC_Controller::OnMountNodeDead(StringHash eventType, VariantMap& eventData)
{
    Unmount();
}

void GOC_Controller::HandleNetUpdate(StringHash eventType, VariantMap& eventData)
{
//    SmoothedTransform* smooth = node_->GetComponent<SmoothedTransform>();
//    if (smooth && smooth->IsInProgress())
//        return;
    using namespace UpdateSmoothing;

    float constant = eventData[P_CONSTANT].GetFloat();
    float squaredSnapThreshold = eventData[P_SQUAREDSNAPTHRESHOLD].GetFloat();

    RigidBody2D* body = node_->GetComponent<RigidBody2D>();
    GOC_Move2D* move2d = node_->GetComponent<GOC_Move2D>();
    if (body && move2d)
    {
        Vector2 vel = body->GetLinearVelocity();
        float dx = Abs(vel.x_);
        float dy = Abs(vel.y_);
        bool updatevx = (dx > 2.f * move2d->GetTemplate().vel_[vMINI] && dx < move2d->GetTemplate().vel_[vWALK]);
        bool updatevy = (dy > 2.f * move2d->GetTemplate().vel_[vMINI] && dy < move2d->GetTemplate().vel_[vWALK]);
        if (updatevx)
            body->GetBody()->ApplyForceToCenter(b2Vec2(vel.x_*0.66f*constant, 0.f), true);
        if (updatevy)
            body->GetBody()->ApplyForceToCenter(b2Vec2(0.f, vel.y_*0.66f*constant), true);

        if (node_->GetID() == 16777274 && (updatevx || updatevy))
            URHO3D_LOGINFOF("GOC_Controller() - HandleNetUpdate : node=%s(%u) velocity=%F,%F ...", node_->GetName().CString(), node_->GetID(), vel.x_, vel.y_);
    }
}
