#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>

#include "DefsColliders.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"

#include "CommonComponents.h"

#include "GOB_MountOn.h"


void GOB_MountOn::Start(GOC_AIController& controller)
{
    if (!controller.GetaiInfos().target)
        return;

    AInodeInfos& aiInfos = controller.GetaiInfos();
    aiInfos.Reset();
    aiInfos.updateAggressivity = 0;
    aiInfos.aggressiveDelay = 0;

    MountOn(controller, aiInfos.target);
}

// UNMOUNT From a target
void GOB_MountOn::Stop(GOC_AIController& controller)
{
    Node* node = controller.GetNode();

    if (!node->GetVar(GOA::ISMOUNTEDON).GetUInt())
        return;

    URHO3D_LOGINFO("GOB_MountOn() - Stop : unmount ...");

    AInodeInfos& aiInfos = controller.GetaiInfos();

    unsigned parentid = node->GetVar(GOA::ISMOUNTEDON).GetUInt();

    node->SetVar(GOA::ISMOUNTEDON, unsigned(0));

    PODVector<ConstraintWeld2D* > contraints;

    // Get Contraints on the node and remove
    node->GetComponents<ConstraintWeld2D>(contraints);
    for (PODVector<ConstraintWeld2D* >::Iterator it=contraints.Begin(); it!=contraints.End(); ++it)
    {
        ConstraintWeld2D* constraint = *it;
        if (constraint && constraint->GetOtherBody()->GetNode()->GetID() == parentid)
        {
            constraint->ReleaseJoint();
            constraint->Remove();
        }
    }
    // Get the Contraints on the parent and remove
    contraints.Clear();
    node->GetParent()->GetComponents<ConstraintWeld2D>(contraints);
    for (PODVector<ConstraintWeld2D* >::Iterator it=contraints.Begin(); it!=contraints.End(); ++it)
    {
        ConstraintWeld2D* constraint = *it;
        if (constraint && constraint->GetOtherBody()->GetNode()->GetID() == node->GetID())
        {
            constraint->ReleaseJoint();
            constraint->Remove();
        }
    }

    if (aiInfos.lastParent)
    {
        node->SetParent(aiInfos.lastParent);
        aiInfos.lastParent.Reset();
    }
    else
    {
        node->SetParent(GameContext::Get().rootScene_);
    }

//    URHO3D_LOGINFOF("GOB_MountOn() - Stop : ... restore lastmovetype=%d", aiInfos.lastMoveType);
    node->GetComponent<GOC_Move2D>()->SetMoveType(aiInfos.lastMoveType);
    node->GetComponent<AnimatedSprite2D>()->SetLayer2(IntVector2(aiInfos.lastLayer,-1));

    // Restore Mass
    RigidBody2D* body = node->GetComponent<RigidBody2D>();
    if (body)
        body->SetMass(body->GetMass() * 2.f);

    GOC_Inventory* inventory = node->GetComponent<GOC_Inventory>();
    if (inventory)
        inventory->SetEnabled(true);

    node->GetComponent<GOC_Animator2D>()->ResetState();
    node->GetComponent<GOC_Destroyer>()->SetViewZ();

    URHO3D_LOGINFOF("GOB_MountOn() - Stop : unmount ... OK !");
}

// MOUNT ON A TARGET AND DESACTIVE CONTROLLER
void GOB_MountOn::MountOn(GOC_AIController& controller, Node* target)
{
    Node* node = controller.GetNode();

    // Entity can not mount on itself or on void
    if (target == node || !target)
        return;

    Stop(controller);

    URHO3D_LOGINFOF("GOB_MountOn() - MountOn : %s Mount on %s ...", node->GetName().CString(), target->GetName().CString());

    AInodeInfos& aiInfos = controller.GetaiInfos();

    node->SetVar(GOA::ISMOUNTEDON, target->GetID());

    aiInfos.lastParent = node->GetParent();
    node->SetParent(target);

    aiInfos.lastMoveType = node->GetComponent<GOC_Move2D>()->GetMoveType();
    aiInfos.lastLayer = node->GetComponent<AnimatedSprite2D>()->GetLayer();

    node->GetComponent<GOC_Move2D>()->SetMoveType(MOVE2D_MOUNT);
    node->GetComponent<GOC_Animator2D>()->SetState(STATE_IDLE);
    node->GetComponent<GOC_Animator2D>()->SetDirection(target->GetComponent<GOC_Animator2D>()->GetDirection());

    // Divide Mass
    RigidBody2D* body = node->GetComponent<RigidBody2D>();
    if (body)
        body->SetMass(body->GetMass() * 0.5f);

    GOC_Inventory* inventory = node->GetComponent<GOC_Inventory>();
    if (inventory)
        inventory->SetEnabled(false);

    // Set the position of the node on the top of the target
    // TODO : Add a node Mounted in all animations (adapt the method used for the swords or wing)
    GOC_Destroyer* targetdestroyer = target->GetComponent<GOC_Destroyer>();
    if (targetdestroyer)
        node->SetWorldPosition2D(Vector2(0.f, targetdestroyer->GetWorldShapesRect().max_.y_));
    else
        node->SetWorldPosition2D(target->GetWorldPosition2D() + Vector2(0.f, 0.5f));

    GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
    if (destroyer)
        destroyer->SetViewZ();

    ConstraintWeld2D* constraintWeld = node->CreateComponent<ConstraintWeld2D>();
    constraintWeld->SetOtherBody(target->GetComponent<RigidBody2D>());
    constraintWeld->SetAnchor(node->GetWorldPosition2D());
    constraintWeld->SetFrequencyHz(4.0f);
    constraintWeld->SetDampingRatio(0.5f);

    URHO3D_LOGINFOF("GOB_MountOn() - MountOn : %s Mount on %s ... !", node->GetName().CString(), target->GetName().CString());
}

void GOB_MountOn::Update(GOC_AIController& controller)
{
    Node* node = controller.GetNode();
    AInodeInfos& aiInfos = controller.GetaiInfos();
    const ObjectControlLocal& control = controller.control_;

    unsigned& targetID = aiInfos.targetID;
    WeakPtr<Node>& target = aiInfos.target;
    unsigned& order = aiInfos.order;
    unsigned& callBackOrder = aiInfos.callBackOrder;
    unsigned& buttons = aiInfos.buttons;
    const unsigned& state = control.animation_;
    const float& dirx = control.direction_;

    unsigned delay = Time::GetSystemTime() - aiInfos.lastUpdate;

    if (callBackOrder)
    {
        //check if order is released and reset
        if (order == callBackOrder)
        {
            aiInfos.lastOrder = order;
//            URHO3D_LOGINFOF("GOB_MountOn() - Update : order=%u is applied ; reset()", order);
            callBackOrder = 0;
            order = 0;
            buttons = 0;
            if (aiInfos.nextOrder)
            {
                order = aiInfos.nextOrder;
                aiInfos.nextOrder = 0;
            }
        }
    }

    if (!order)
    {
        if (state == STATE_HURT)
        {
            // if Mounted, fall
            if (node->GetVar(GOA::ISMOUNTEDON).GetUInt())
            {
                URHO3D_LOGINFOF("GOB_MountOn() - Update : Hurt -> UnMount !");
                Stop(controller);
            }
            // Find Enemy and hit
            Node* attacker = node->GetComponent<GOC_Life>()->GetLastAttacker();
            if (attacker)
            {
                order = STATE_ATTACK;
                if (dirx * (node->GetWorldPosition2D().x_ - attacker->GetWorldPosition2D().x_) > 0.f)
                {
                    aiInfos.nextOrder = order;
                    order = GO_CHANGEDIRECTION.Value();
//                    URHO3D_LOGINFOF("GOB_MountOn() - Update : Hurt ! attacker is behind => Face to attacker (%u) then Attack (%u)",
//                             order, aiInfos.nextOrder);
                }
//                else
//                    URHO3D_LOGINFOF("GOB_MountOn() - Update : Hurt ! is in front of attacker => Attack");
            }
//            else
//                URHO3D_LOGINFOF("GOB_MountOn() - Update : Hurt ! but no attacker");
        }
        else if (state == STATE_ATTACK)
        {
            // reset after hit
//            URHO3D_LOGINFOF("GOB_MountOn() - Update : Attack => reset after hit");
            buttons = 0;
        }
        else if (!node->GetVar(GOA::ISMOUNTEDON).GetUInt())
        {
            if (!targetID)
            {
                URHO3D_LOGWARNINGF("GOB_MountOn() - Update : noMountOn && targetID=0 !");
                return;
            }

            if (target.Get() == 0)
            {
                target = node->GetScene()->GetNode(targetID);
                if (target.Get() == 0)
                {
                    URHO3D_LOGWARNINGF("GOB_MountOn() - Update : noMountOn && !target !");
                    return;
                }
            }

            // find target position and update
            Vector2 targetPosition = target->GetWorldPosition2D();
            Vector2 deltaPosition = targetPosition - node->GetWorldPosition2D();

            // no big gap, try to mount
            if (deltaPosition.LengthSquared() < 2.f)
            {
                buttons = 0;
                GOC_Controller* tcontroller = target->GetDerivedComponent<GOC_Controller>();
                float targetdirx = tcontroller ? tcontroller->control_.direction_ : dirx;
                // not in the same direction, back away
                if (targetdirx != dirx)
                {
                    buttons = (dirx > targetdirx ? CTRL_LEFT : CTRL_RIGHT);
                }
                // Mount On target
                else
                {
                    MountOn(controller, target);
                }
            }
            // continue to follow
            else
            {
                buttons = (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);
            }
        }
    }

    if (order)
        // if new order => config buttons and put behavior's update in standby until action release
    {
        // config buttons
        if (order == GO_CHANGEDIRECTION.Value())
        {
            buttons = (dirx > 0 ? CTRL_LEFT : CTRL_RIGHT);
            aiInfos.waitCallBackOrderOfType = EventTypeForOrder;
//            URHO3D_LOGINFOF("GOB_MountOn() - Update : order GO_CHANGEDIRECTION(%u) => button=%u => wait for this Event", GO_CHANGEDIRECTION.Value(), buttons);
        }
        else if (order == STATE_ATTACK)
        {
            buttons = CTRL_FIRE;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_MountOn() - Update : order Attack(%u) => button=%u => wait for State=State_Attack !", STATE_ATTACK, buttons);
        }
        else if (order == STATE_JUMP)
        {
            buttons = buttons | CTRL_JUMP;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_MountOn() - Update : order Jump(%u) => button=%u => wait for State=State_Jump !", STATE_JUMP, buttons);
        }
        else
            // cancel unknown order
        {
            URHO3D_LOGWARNINGF("GOB_MountOn() - Update : order unknown = %u => cancel it!", order);
            order = 0;
            buttons = 0;
        }
    }
//    URHO3D_LOGINFOF("GOB_MountOn() - Update : buttons=%u", buttons);
}
