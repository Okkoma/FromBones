#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include "GameAttributes.h"
#include "GameEvents.h"

#include "CommonComponents.h"
#include "GOC_Abilities.h"

#include "GOB_Follow.h"



// WALKER FOLLOWING A TARGET


void GOB_Follow::Stop(GOC_AIController& controller)
{
    // Reset Target
    controller.GetaiInfos().target.Reset();

    // Reset Ability
    GOC_Abilities* gocabilities = controller.GetNode()->GetComponent<GOC_Abilities>();
    if (gocabilities)
        gocabilities->SetActiveAbility(0);
}

void GOB_Follow::Update(GOC_AIController& controller)
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

    unsigned delay = controller.GetLastTimeUpdate() - aiInfos.lastUpdate;

#ifdef APPLY_RANDSKIP
    unsigned randomskip = Random(250);
    if (delay < randomskip)
    {
//        URHO3D_LOGINFOF("GOB_Follow() - Update : apply random skip (%u)", randomskip);
        return;
    }
#endif // APPLY_RANDSKIP

    if (callBackOrder)
    {
        //check if order is released and reset
        if (order == callBackOrder)
        {
            aiInfos.lastOrder = order;
//            URHO3D_LOGINFOF("GOB_Follow() - Update : order=%u is applied ; reset()", order);
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

    if (!order && state == STATE_HURT)
    {
        // Find Enemy and hit
        Node* attacker = node->GetComponent<GOC_Life>()->GetLastAttacker();
        if (attacker)
        {
            if (!target)
            {
                targetID = attacker->GetID();
                target = attacker;
            }

            order = STATE_ATTACK;
            if (dirx * (node->GetWorldPosition2D().x_ - attacker->GetWorldPosition2D().x_) > 0.f)
            {
                aiInfos.nextOrder = order;
                order = GO_CHANGEDIRECTION.Value();
//                URHO3D_LOGINFOF("GOB_Follow() - Update : Hurt ! attacker is behind => Face to attacker (%u) then Attack (%u)",
//                                order, aiInfos.nextOrder);
            }
//            else
//                URHO3D_LOGINFOF("GOB_Follow() - Update : Hurt ! is in front of attacker => Attack");
        }
//        else
//            URHO3D_LOGINFOF("GOB_Follow() - Update : Hurt ! but no attacker");
    }
    else if (!order && (state == STATE_ATTACK || state == STATE_SHOOT))
    {
        // reset after hit
//        URHO3D_LOGINFOF("GOB_Follow() - Update : Attack => reset after hit");
        buttons = 0;
    }
    else if (!order && (state == STATE_FALL))
    {
        if (buttons & CTRL_JUMP)
            buttons = (buttons & ~CTRL_JUMP);

        if (delay > 2000)
        {
            buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
        }
    }

    if (target)
    {
        // find target position and update
        Vector2 targetPosition = target->GetWorldPosition2D();
        Vector2 deltaPosition = targetPosition - node->GetWorldPosition2D();

        if (state == STATE_DEFAULT_CLIMB || state == STATE_CLIMB)
        {
            int movestate = node->GetVar(GOA::MOVESTATE).GetInt();

            if (movestate & MV_TOUCHWALL)
            {
                if (Abs(deltaPosition.x_) > Abs(deltaPosition.y_))
                {
                    if (deltaPosition.x_ > 0.f && movestate & MV_DIRECTION)
                    {
                        buttons = CTRL_RIGHT | CTRL_JUMP;
                        order = STATE_JUMP;
//                        URHO3D_LOGINFOF("GOB_Follow() - Update : Climb On a Left Wall and target on the right (dx>dy) => JUMP and Go to the right");
                    }
                    else if (deltaPosition.x_ < 0.f && !(movestate & MV_DIRECTION))
                    {
                        buttons = CTRL_LEFT | CTRL_JUMP;
                        order = STATE_JUMP;
//                        URHO3D_LOGINFOF("GOB_Follow() - Update : Climb On a Right Wall and target on the left (dx>dy) => JUMP and Go to the Left");
                    }
                    else if (deltaPosition.y_ > 0.f && !(movestate & MV_TOUCHROOF))
                    {
                        buttons = CTRL_UP;
//                        URHO3D_LOGINFOF("GOB_Follow() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                    else if (deltaPosition.y_ < 0.f && !(movestate & MV_TOUCHGROUND))
                    {
                        buttons = CTRL_DOWN;
//                        URHO3D_LOGINFOF("GOB_Follow() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                }
                else
                {
                    if (deltaPosition.y_ > 0.f && !(movestate & MV_TOUCHROOF))
                    {
                        buttons = CTRL_UP;
//                        URHO3D_LOGINFOF("GOB_Follow() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                    else if (deltaPosition.y_ < 0.f)// && !(movestate & MV_TOUCHGROUND))
                    {
                        buttons = CTRL_DOWN;
//                        URHO3D_LOGINFOF("GOB_Follow() - Update : Go Down ! (dx>dy)");
                    }
                }
            }
            if (movestate & MV_TOUCHROOF)
            {
                buttons = (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);

                if (deltaPosition.y_ < 0.f && Abs(deltaPosition.x_) < 0.5f)
                {
                    buttons = buttons | CTRL_DOWN | CTRL_JUMP;
                    order = STATE_JUMP;
//                    URHO3D_LOGINFOF("GOB_Follow() - Update : %s(%u) Climb Horizontal on a Roof and near target (dx<dy) => Jump", node->GetName().CString(), node->GetID());
                }
                else if (order == STATE_JUMP)
                {
//                    URHO3D_LOGINFOF("GOB_Follow() - Update : Cancel Jump if on Roof !");
                    aiInfos.nextOrder = aiInfos.nextOrder == STATE_JUMP ? 0 : order;
                    order = 0;
                    aiInfos.waitCallBackOrderOfType = 0;
                }
            }
        }
        else
        {
            // examin what to do when a big gap in y_
            if (deltaPosition.y_ < -0.2f || Abs(deltaPosition.y_) > aiInfos.minRangeTarget.y_)
            {
                buttons = buttons & ~(CTRL_RIGHT | CTRL_LEFT | CTRL_JUMP);

                // target is above : Jump
                const Vector2& vel = node->GetVar(GOA::VELOCITY).GetVector2();
                int movestate = node->GetVar(GOA::MOVESTATE).GetInt();

                if (deltaPosition.y_ > 0.f &&
                        ((vel.y_ < GOC_Move2D::velJumpMax && !(movestate & MV_TOUCHOBJECT)) ||
                         ((vel.y_ < -0.15f || vel.y_ > 0.15f) && movestate & MV_TOUCHOBJECT)) ) // && vel.y_ < GOC_Move2D::velJumpMax)
                {
                    if (!order || !aiInfos.nextOrder)
                    {
                        if (order)
                            aiInfos.nextOrder = order;
                        buttons = buttons | CTRL_JUMP;
                        order = STATE_JUMP;
                    }
                }
                // target is below : cancel Jump order
                else if (order == STATE_JUMP)
                {
//                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : under target => Remove Jumping buttons=%u !", buttons);
                    aiInfos.nextOrder = aiInfos.nextOrder == STATE_JUMP ? 0 : order;
                    order = 0;
                    aiInfos.waitCallBackOrderOfType = 0;
                }

                if (Abs(deltaPosition.x_) > aiInfos.minRangeTarget.x_)
                    buttons = buttons | (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);
            }
            // no big gap in y_
            else if (!order)
            {
                // target in the good range
                if (Abs(deltaPosition.x_) < aiInfos.maxRangeTarget.x_)
                {
                    buttons = 0;

                    // target too close
                    if (Abs(deltaPosition.x_) < aiInfos.minRangeTarget.x_)
                    {
//                        float targetdirx = target->GetVar(GOA::DIRECTION).GetVector2().x_;
                        GOC_Controller* tcontrol = target->GetDerivedComponent<GOC_Controller>();
                        if (tcontrol)
                            buttons = tcontrol->control_.buttons_ & (CTRL_LEFT|CTRL_RIGHT);

//                        float targetdirx = target->GetDerivedComponent<GOC_Controller>() ? target->GetDerivedComponent<GOC_Controller>()->control_.direction_ : dirx;
                        // not in the same direction, back away
//                        if (dirx != targetdirx)
//                        {
////                            buttons = (dirx > targetdirx ? CTRL_LEFT : CTRL_RIGHT);
////                            URHO3D_LOGINFOF("GOB_Follow() - Update : too close change direction (buttons=%u)", buttons);
//                        }

//                        buttons = (targetdirx < 0.f ? CTRL_LEFT : CTRL_RIGHT);
                    }
                }
                // continue to follow
                else
                {
                    buttons = (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);
//                    URHO3D_LOGINFOF("GOB_Follow() - Update : continue to follow (buttons=%u)", buttons);
                }
            }

        }

        if (delay > 300)
            aiInfos.lastTargetPosition = targetPosition;
    }

    if (order)
        // if new order => config buttons and put behavior's update in standby until action release
    {
        // config buttons
        if (order == GO_CHANGEDIRECTION.Value())
        {
            buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            aiInfos.waitCallBackOrderOfType = EventTypeForOrder;
//            URHO3D_LOGINFOF("GOB_Follow() - Update : order GO_CHANGEDIRECTION(%u) => button=%u => wait for this Event", GO_CHANGEDIRECTION.Value(), buttons);
        }
        else if (order == STATE_ATTACK)
        {
            buttons = CTRL_FIRE1;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_Follow() - Update : order Attack(%u) => button=%u => wait for State=State_Attack !", STATE_ATTACK, buttons);
        }
        else if (order == STATE_JUMP)
        {
            buttons = buttons | CTRL_JUMP;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_Follow() - Update : order Jump(%u) => button=%u => wait for State=State_Jump !", STATE_JUMP, buttons);
        }
        else
            // cancel unknown order
        {
            URHO3D_LOGWARNINGF("GOB_Follow() - Update : order unknown = %u => cancel it!", order);
            order = 0;
            buttons = 0;
        }
    }
//    URHO3D_LOGINFOF("GOB_Follow() - Update : buttons=%u", buttons);
}



// FOLLOWING A TARGET AND ATTACK


void GOB_FollowAttack::Start(GOC_AIController& controller)
{
//    URHO3D_LOGINFOF("GOB_FollowAttack() - Start : %s(%u)", controller.GetNode()->GetName().CString(), controller.GetNode()->GetID());

    AInodeInfos& aiInfos = controller.GetaiInfos();

    aiInfos.Reset();

//    aiInfos.target.Reset();
    aiInfos.updateAggressivity = 1;
    aiInfos.aggressiveDelay = 0;
}

void GOB_FollowAttack::Stop(GOC_AIController& controller)
{
//    URHO3D_LOGINFOF("GOB_FollowAttack() - Stop : %s(%u)", controller.GetNode()->GetName().CString(), controller.GetNode()->GetID());

    AInodeInfos& aiInfos = controller.GetaiInfos();

    aiInfos.Reset();

    aiInfos.target.Reset();
    aiInfos.updateAggressivity = 0;
    aiInfos.aggressiveDelay = 0;
}

void GOB_FollowAttack::Update(GOC_AIController& controller)
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

    unsigned delay = controller.GetLastTimeUpdate() - aiInfos.lastUpdate;

#ifdef APPLY_RANDSKIP
    unsigned randomskip = Random(250);
    if (delay < randomskip)
    {
//        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : apply random skip (%u)", randomskip);
        return;
    }
#endif // APPLY_RANDSKIP

    if (callBackOrder)
    {
        // check if order is released and reset
        if (order == callBackOrder)
        {
            aiInfos.lastOrder = order;
//            URHO3D_LOGINFOF("GOB_FollowAttack() - Update : order=%u is applied ; reset()", order);
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

    if (!order && state == STATE_HURT)
    {
        // Find Enemy and hit
        Node* attacker = node->GetComponent<GOC_Life>()->GetLastAttacker();
        if (attacker)
        {
            if (!target)
            {
                targetID = attacker->GetID();
                target = attacker;
                aiInfos.aggressiveDelay = 0;
            }

            order = STATE_ATTACK;
            if (dirx * (node->GetWorldPosition2D().x_ - attacker->GetWorldPosition2D().x_) > 0.f)
            {
                aiInfos.nextOrder = order;
                order = GO_CHANGEDIRECTION.Value();
//                URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Hurt ! attacker is behind => Face to attacker (%u) then Attack (%u)",
//                             order, aiInfos.nextOrder);
            }
//            else
//                URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Hurt ! is in front of attacker => Attack");
        }
//        else
//            LOGINFOF("GOB_FollowAttack() - Update : Hurt ! but no attacker");

        {
            GOC_Abilities* gocabilities = node->GetComponent<GOC_Abilities>();
            if (gocabilities)
            {
                Ability* ability = gocabilities->GetAbility(ABI_AnimShooter::GetTypeStatic());
                if (ability && gocabilities->SetActiveAbility(ability))
                {
                    order = STATE_SHOOT;
//                    URHO3D_LOGINFOF("GOB_Patrol() - Update : STATE_HURT => SHOOT on %s(%u)", attacker->GetName().CString(), attacker->GetID());
                }
            }
        }
    }
    else if (!order && (state == STATE_ATTACK || state == STATE_SHOOT || order == STATE_POWER1 || order == STATE_POWER2))
    {
        // reset after hit
//        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Attack => reset after hit");
        buttons = 0;
    }
    else if (!order)// && (state == STATE_FALL))
    {
        if ((buttons & CTRL_JUMP) && (state == STATE_FALL))
            buttons = (buttons & ~CTRL_JUMP);

        if (delay > 2000)
        {
            buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
        }
    }

    if (target)
    {
        // find target position and update
        Vector2 targetPosition = target->GetWorldPosition2D();
        Vector2 deltaPosition = targetPosition - node->GetWorldPosition2D();

        if (state == STATE_DEFAULT_CLIMB || state == STATE_CLIMB)
        {
            int movestate = node->GetVar(GOA::MOVESTATE).GetInt();

            if (movestate & MV_TOUCHWALL)
            {
                if (Abs(deltaPosition.x_) > Abs(deltaPosition.y_))
                {
                    if (deltaPosition.x_ > 0.f && (movestate & MV_DIRECTION))
                    {
                        buttons = CTRL_RIGHT | CTRL_JUMP;
                        order = STATE_JUMP;
//                        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Climb On a Left Wall and target on the right (dx>dy) => JUMP and Go to the right");
                    }
                    else if (deltaPosition.x_ < 0.f && !(movestate & MV_DIRECTION))
                    {
                        buttons = CTRL_LEFT | CTRL_JUMP;
                        order = STATE_JUMP;
//                        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Climb On a Right Wall and target on the left (dx>dy) => JUMP and Go to the Left");
                    }
                    else if (deltaPosition.y_ > 0.f && !(movestate & MV_TOUCHROOF))
                    {
                        buttons = CTRL_UP;
//                        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                    else if (deltaPosition.y_ < 0.f && !(movestate & MV_TOUCHGROUND))
                    {
                        buttons = CTRL_DOWN;
//                        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                }
                else
                {
                    if (deltaPosition.y_ > 0.f && !(movestate & MV_TOUCHROOF))
                    {
                        buttons = CTRL_UP;
//                        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                    else if (deltaPosition.y_ < 0.f)// && !(movestate & MV_TOUCHGROUND))
                    {
                        buttons = CTRL_DOWN;
//                        URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Climb Vertical on a Wall (dx>dy)");
                    }
                }
            }
            if (movestate & MV_TOUCHROOF)
            {
                buttons = (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);

                if (deltaPosition.y_ < 0.f && Abs(deltaPosition.x_) < 0.5f)
                {
                    buttons = buttons | CTRL_DOWN | CTRL_JUMP;
                    order = STATE_JUMP;
//                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : %s(%u) Climb Horizontal on a Roof and near target (dx<dy) => Jump", node->GetName().CString(), node->GetID());
                }
                else if (order == STATE_JUMP)
                {
//                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Cancel Jump if on Roof !");
                    aiInfos.nextOrder = aiInfos.nextOrder == STATE_JUMP ? 0 : order;
                    order = 0;
                    aiInfos.waitCallBackOrderOfType = 0;
                }
            }
        }
        else
        {
            // no big gap in y_
            if (!order && Abs(deltaPosition.y_) < aiInfos.minRangeTarget.y_)
            {
                // TODO : check for Wall between entity and the target
                int movestate = node->GetVar(GOA::MOVESTATE).GetInt();
                if ((movestate & MV_TOUCHWALL) && node->GetComponent<GOC_Destroyer>()->HasWallInFront(deltaPosition.x_ > 0.f))
                {
//            		URHO3D_LOGINFOF("GOB_FollowAttack() - Update : node=%s(%u) can't Attack wall in front !", node->GetName().CString(), node->GetID());
                }
                else
                {
                    float absposx = Abs(deltaPosition.x_);
                    // melee attack
                    if (absposx <= defaultMaxMeleeAttRange.x_)
                    {
                        order = STATE_ATTACK;
                    }
                    // target in range for ranged attack
                    else if (absposx > defaultMinRangedAttRange.x_ && absposx < defaultMaxRangedAttRange.x_)
                    {
                        static unsigned powers[3];
                        int numpowers = 0;

                        GOC_Animator2D* gocanimator = node->GetComponent<GOC_Animator2D>();
                        GOC_Abilities* gocabilities = node->GetComponent<GOC_Abilities>();
                        if (gocanimator->HasAnimationForState(STATE_POWER1))
                            powers[numpowers++] = STATE_POWER1;
                        if (gocanimator->HasAnimationForState(STATE_POWER2))
                            powers[numpowers++] = STATE_POWER2;
                        if (gocabilities)
                        {
                            Ability* ability = gocabilities->GetAbility(ABI_AnimShooter::GetTypeStatic());
                            if (ability && gocabilities->SetActiveAbility(ability))
                                powers[numpowers++] = STATE_SHOOT;
                        }
                        if (numpowers > 0)
                        {
                            int power = numpowers > 1 ? delay % numpowers : 0;
                            order = powers[power];
                        }
                        //                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : continue to follow (buttons=%u)", buttons);
                    }

                    if (order != STATE_SHOOT)
                        buttons = (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);

                    // if not in front of target, change direction before attack
                    if (deltaPosition.x_ * dirx < 0.f)
                    {
                        aiInfos.nextOrder = order;
                        order = GO_CHANGEDIRECTION.Value();
                        //                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : closed to enemy => Face to attacker (%u) then Attack (%u)",
                        //                                    order, aiInfos.nextOrder);
                    }
                    //                else
                    //                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : closed to enemy => Attack");
                }
            }
            // examin what to do when a big gap in y_
            else
            {
                buttons = buttons & ~(CTRL_RIGHT | CTRL_LEFT | CTRL_JUMP);

                // target is above : Jump
                const Vector2& vel = node->GetVar(GOA::VELOCITY).GetVector2();
                int movestate = node->GetVar(GOA::MOVESTATE).GetInt();

                if (deltaPosition.y_ > 0.f &&
                        ((vel.y_ < GOC_Move2D::velJumpMax && !(movestate & MV_TOUCHOBJECT)) ||
                         ((vel.y_ < -0.15f || vel.y_ > 0.15f) && movestate & MV_TOUCHOBJECT))
                   )// && vel.y_ < GOC_Move2D::velJumpMax)
                {
                    if (!order || !aiInfos.nextOrder)
                    {
                        if (order)
                            aiInfos.nextOrder = order;
                        buttons = buttons | CTRL_JUMP;
                        order = STATE_JUMP;
                    }
                }
                // target is below : cancel Jump order
                else if (order == STATE_JUMP)
                {
//                    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : under target => Remove Jumping buttons=%u !", buttons);
                    aiInfos.nextOrder = aiInfos.nextOrder == STATE_JUMP ? 0 : order;
                    order = 0;
                    aiInfos.waitCallBackOrderOfType = 0;
                }

                if (Abs(deltaPosition.x_) > aiInfos.minRangeTarget.x_)
                    buttons = buttons | (deltaPosition.x_ > 0.f ? CTRL_RIGHT : CTRL_LEFT);

                if (movestate & MV_TOUCHGROUND)
                {
                    if (deltaPosition.y_ < 0.f)
                    {
                        buttons = buttons | (CTRL_DOWN | CTRL_JUMP);
                        //order = STATE_JUMP;
                        order = 0;
//						URHO3D_LOGINFOF("GOB_FollowAttack() - Update : Go Down ! (dx>dy)");
                    }
                }
            }
        }

        if (delay > 300)
            aiInfos.lastTargetPosition = targetPosition;
    }

    if (order)
        // if new order => config buttons and put behavior's update in standby until action release
    {
        // config buttons
        if (order == GO_CHANGEDIRECTION.Value())
        {
            buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            aiInfos.waitCallBackOrderOfType = EventTypeForOrder;
//            URHO3D_LOGINFOF("GOB_FollowAttack() - Update : order GO_CHANGEDIRECTION(%u) => button=%u => wait for this Event", GO_CHANGEDIRECTION.Value(), buttons);
        }
        else if (order == STATE_ATTACK)
        {
            buttons = CTRL_FIRE1;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_FollowAttack() - Update : order Attack(%u) => button=%u => wait for State=State_Attack !", STATE_ATTACK, buttons);
        }
        else if (order == STATE_SHOOT || order == STATE_POWER1 || order == STATE_POWER2)
        {
            buttons = CTRL_FIRE2;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_FollowAttack() - Update : order Shoot(%u) => button=%u => wait for State=State_Shoot !", STATE_SHOOT, buttons);
        }
        else if (order == STATE_JUMP)
        {
            buttons = buttons | CTRL_JUMP;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_FollowAttack() - Update : order Jump(%u) => button=%u => wait for State=State_Jump !", STATE_JUMP, buttons);
        }
        else
            // cancel unknown order
        {
            URHO3D_LOGWARNINGF("GOB_FollowAttack() - Update : node=%s(%u) order unknown = %u => cancel it!", node->GetName().CString(), node->GetID(), order);
            order = 0;
            buttons = 0;
        }
    }
//    URHO3D_LOGINFOF("GOB_FollowAttack() - Update : buttons=%u", buttons);
}

