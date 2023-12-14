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
    controlType_(GO_None),
    currentIdPath_(-1),
    thinker_(0),
    lastdesync_(true)
{ }

GOC_Controller::GOC_Controller(Context* context, int type) :
    Component(context),
    mainController_(false),
    controlActionEnable_(true),
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
    URHO3D_LOGINFOF("GOC_Controller() - Start : Type=%d", controlType_);
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
        if (node_->GetParent() == GetScene() || node_->GetParent() == 0 || node_->GetParent()->GetVar(GOA::ISDEAD).GetBool())
            Unmount();
    }
}

void GOC_Controller::MountOn(Node* target)
{
    URHO3D_LOGINFOF("GOC_Controller() - MountOn : %s(%u) on node=%s(%u) ...",
                    node_->GetName().CString(), node_->GetID(), target->GetName().CString(), target->GetID());

    if (thinker_)
    {
        static_cast<Player*>(thinker_)->MountOn(target);
    }
    else
    {
        node_->SetVar(GOA::ISMOUNTEDON, target->GetID());

        Node* mountNode = target->GetChild(MOUNTNODE, true);
        if (mountNode)
        {
            node_->GetComponent<RigidBody2D>()->SetEnabled(false);
            node_->SetParent(mountNode);
            node_->SetPosition2D(Vector2::ZERO);
            node_->GetComponent<GOC_Move2D>()->SetMoveType(MOVE2D_MOUNT);
            node_->GetComponent<GOC_Animator2D>()->SetState(STATE_IDLE);
            node_->GetDerivedComponent<Drawable2D>()->SetLayer(target->GetDerivedComponent<Drawable2D>()->GetLayer());
        }
        else
        {
            // Divide Mass
            RigidBody2D* body = node_->GetComponent<RigidBody2D>();
            if (body)
            {
//                body->SetMass(body->GetMass() * 0.5f);
                body->SetEnabled(false);
            }

            node_->SetParent(target);

            GOC_Destroyer* targetdestroyer = target->GetComponent<GOC_Destroyer>();
            if (targetdestroyer && targetdestroyer->GetShapesRect().Defined())
                node_->SetWorldPosition2D(Vector2(targetdestroyer->GetWorldShapesRect().Center().x_, targetdestroyer->GetWorldShapesRect().max_.y_));
            else
                node_->SetWorldPosition2D(target->GetWorldPosition2D() + Vector2(0.f, 0.5f));

            ConstraintWeld2D* constraintWeld = node_->CreateComponent<ConstraintWeld2D>();
            constraintWeld->SetOtherBody(target->GetComponent<RigidBody2D>());
            constraintWeld->SetAnchor(node_->GetWorldPosition2D());
            constraintWeld->SetFrequencyHz(4.0f);
            constraintWeld->SetDampingRatio(0.5f);
        }
    }

    URHO3D_LOGINFOF("GOC_Controller() - MountOn : %s(%u) on node=%s(%u) ... OK !",
                    node_->GetName().CString(), node_->GetID(), target->GetName().CString(), target->GetID());
}

void GOC_Controller::Unmount()
{
    URHO3D_LOGINFOF("GOC_Controller() - Unmount : %s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (thinker_)
    {
        static_cast<Player*>(thinker_)->Unmount();
    }
    else
    {
        if (node_->GetParent()->GetName().StartsWith(MOUNTNODE))
        {
            node_->SetParent(node_->GetScene());
            node_->GetComponent<RigidBody2D>()->SetEnabled(true);
        }
        else
        {
            const unsigned targetid = node_->GetVar(GOA::ISMOUNTEDON).GetUInt();
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
                        constraint->ReleaseJoint();
                        constraint->Remove();
                    }
                }
            }
            // Restore the mass
            RigidBody2D* body = node_->GetComponent<RigidBody2D>();
            if (body)
            {
//                body->SetMass(body->GetMass() * 2.f);
                body->SetEnabled(true);
            }
            // Set Parent
            node_->SetParent(node_->GetScene());
        }

        node_->SetVar(GOA::ISMOUNTEDON, 0U);

        GOC_Move2D* move2d = node_->GetComponent<GOC_Move2D>();
        if (move2d)
            move2d->SetMoveType(move2d->GetLastMoveType());

        node_->GetComponent<GOC_Animator2D>()->ResetState();

        node_->GetComponent<GOC_Destroyer>()->SetViewZ();
    }

    URHO3D_LOGINFOF("GOC_Controller() - Unmount : %s(%u) ... OK !", node_->GetName().CString(), node_->GetID());
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
        node_->SetVar(GOA::FACTION, GameContext::Get().LocalMode_ ? type : (node_->GetVar(GOA::CLIENTID).GetUInt() << 8) + GO_Player);

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

bool GOC_Controller::ChangeAvatar(unsigned type, unsigned char entityid)
{
    if (thinker_)
    {
        return static_cast<Player*>(thinker_)->ChangeAvatar(type, entityid, true);
    }
    else
    {
        if (control_.type_ == type)
            return false;

        Node* templateNode = GOT::GetControllableTemplate(StringHash(type));
        if (templateNode && node_->GetNumComponents() >= templateNode->GetNumComponents())
        {
            URHO3D_LOGINFOF("GOC_Controller() - ChangeAvatar : nodeid=%u from %s(%u) to type=%s(%u) ... CopyAttributes From Template", node_->GetID(),
                            GOT::GetType(StringHash(control_.type_)).CString(), control_.type_,
                            GOT::GetType(StringHash(type)).CString(), type);

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
            GameHelpers::SetEntityVariation(node_->GetComponent<AnimatedSprite2D>(), eid);

            control_.type_ = type;
            control_.entityid_ = eid;

            node_->SetVar(GOA::CLIENTID, clientid);

            node_->SetEnabled(true);

            Light* light = node_->GetComponent<Light>();
            if (light)
                light->SetEnabled(false);

            node_->ApplyAttributes();

            node_->AddTag("Player");

            // remount if was mounted
            GameHelpers::MountNode(mountinfo);

            // Spawn Effect
            GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_LIFEFLAME], layer, viewmask, position, 0.f, 1.f, true, 1.f, Color::BLUE, LOCAL);

            return true;
        }
        else
            URHO3D_LOGERRORF("GOC_Controller() - ChangeAvatar : nodeid=%u from %s(%u) to type=%s(%u) ... templateNode=%u NOK !",
                             node_->GetID(), GOT::GetType(StringHash(control_.type_)).CString(), control_.type_,
                             GOT::GetType(StringHash(type)).CString(), type, templateNode);
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
            if (control_.IsButtonDown(CTRL_FIRE))
            {
    //            URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE buttons(prev)=%u(%u)", control_.buttons_, prevbuttons_);
                node_->SendEvent(GOC_CONTROLACTION1);
            }

            if (control_.IsButtonDown(CTRL_FIRE2))
            {
    //            URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE2 buttons(prev)=%u(%u)", control_.buttons_, prevbuttons_);
                node_->SendEvent(GOC_CONTROLACTION2);
            }

            if (control_.IsButtonDown(CTRL_FIRE3))
            {
    //            URHO3D_LOGINFOF("GOC_Controller() - Update : CTRL_FIRE3 buttons(prev)=%u(%u)", control_.buttons_, prevbuttons_);
                node_->SendEvent(GOC_CONTROLACTION3);
            }
        }

        if (control_.IsButtonDown(CTRL_STATUS))
        {
            node_->SendEvent(GOC_CONTROLACTIONSTATUS);
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
