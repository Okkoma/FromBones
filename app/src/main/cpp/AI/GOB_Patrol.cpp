#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include "GameAttributes.h"
#include "GameEvents.h"

#include "CommonComponents.h"
#include "GOC_Abilities.h"

#include "GOB_Patrol.h"


// SIMPLE WALKER PATROLLER


void GOB_Patrol::Update(GOC_AIController& controller)
{
    Node* node = controller.GetNode();
    AInodeInfos& aiInfos = controller.GetaiInfos();
    const ObjectControlLocal& control = controller.control_;

    unsigned& order = aiInfos.order;
    unsigned& callBackOrder = aiInfos.callBackOrder;
    unsigned& buttons = aiInfos.buttons;
    const unsigned& state = control.animation_;
    const float& dirx = control.direction_;

    unsigned delay = Time::GetSystemTime() - aiInfos.lastUpdate;

#ifdef APPLY_RANDSKIP
    unsigned randomskip = Random(500);
    if (delay < randomskip)
    {
//        URHO3D_LOGINFOF("GOB_Patrol() - Update : apply random skip (%u)", randomskip);
        return;
    }
#endif // APPLY_RANDSKIP

//    URHO3D_LOGINFOF("GOB_Patrol() - Update : node=%s(%u) lastbuttons=%u ...", node->GetName().CString(), node->GetID(), buttons);

    // Check if order is released and reset
    if (callBackOrder && order == callBackOrder)
    {
        aiInfos.lastOrder = order;

//        URHO3D_LOGINFOF("GOB_Patrol() - Update : order=%u is applied ; reset()", order);

        if (order == STATE_ATTACK)
        {
            // reset after hit
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : Attack released => reset buttons");
            buttons &= ~CTRL_FIRE;
        }
        else if (state == STATE_SHOOT)
        {
            // reset after hit
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : Shoot released => reset buttons");
            buttons &= ~CTRL_FIRE2;
        }

        order = callBackOrder = 0;

        if (aiInfos.nextOrder)
        {
            order = aiInfos.nextOrder;
            aiInfos.nextOrder = 0;

//            URHO3D_LOGINFOF("GOB_Patrol() - Update : nextorder=%u is setted", order);
        }
    }

    if (!order)
    {
        // mode default = check anim state
        if (state == STATE_DEFAULT || state == STATE_DEFAULT_GROUND || state == STATE_DEFAULT_AIR || state == STATE_DEFAULT_FLUID)
        {
            if (delay > 2000)
            {
//                URHO3D_LOGINFOF("GOB_Patrol() - Update : delay idle released => go move");
                // change direction and move
                buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            }
//            else
//            {
//                URHO3D_LOGINFOF("GOB_Patrol() - Update ; wait delay idle");
//            }
        }
        else if (state == STATE_CLIMB || state == STATE_DEFAULT_CLIMB)
        {
            if (delay > 2000)
            {
                int movestate = node->GetVar(GOA::MOVESTATE).GetInt();

                if (movestate & MV_TOUCHWALL)
                {
                    if (movestate & MV_DIRECTION)
                    {
                        buttons = CTRL_RIGHT | CTRL_JUMP;
                    }
                    else
                    {
                        buttons = CTRL_LEFT | CTRL_JUMP;
                    }
                }
                else if (movestate & MV_TOUCHROOF)
                {
                    buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT) | CTRL_DOWN | CTRL_JUMP;
                }
                else
                {
                    buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
                }
            }
        }
        else if (state == STATE_WALK || state == STATE_FLYUP || state == STATE_FLYDOWN)
        {
            if (delay > 2500)
            {
//                URHO3D_LOGINFOF("GOB_Patrol() - Update : delay walk released => order stop");
                // stop
                buttons = 0;
            }
            else if ((buttons & (CTRL_LEFT | CTRL_RIGHT)) && node->GetComponent<GOC_Destroyer>()->HasWallInFront(dirx > 0.f))
            {
//                URHO3D_LOGINFOF("GOB_Patrol() - Update : %s(%u) is in front of a wall => JUMP !", node->GetName().CString(), node->GetID());
                buttons |= CTRL_JUMP;
            }
            else if (buttons & CTRL_JUMP)
            {
                buttons = (buttons & ~CTRL_JUMP);
            }
            else if (!buttons)
            {
                // the entity doesn't move so unstuck it.
                buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            }
//            else
//                URHO3D_LOGINFOF("GOB_Patrol() - Update : wait delay walk");
        }
        else if (state == STATE_SWIM)
        {
            if (delay > 2500)
            {
                // stop
                buttons = 0;
            }
            else if (!buttons)
            {
                // the entity doesn't move so unstuck it.
                buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            }
        }
        else if (state == STATE_FALL)
        {
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : is FALLING !");

            if (buttons & CTRL_JUMP)
                buttons = (buttons & ~CTRL_JUMP);

            if (delay > 2000)
            {
//                URHO3D_LOGINFOF("GOB_Patrol() - Update : delay idle released => go walk");
                // change direction and walk
                buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            }
            //        // try hit
            //        buttons = buttons & CTRL_FIRE;
        }
        else if (state == STATE_JUMP)
        {
            int movestate = node->GetVar(GOA::MOVESTATE).GetInt();
            if (movestate & MV_TOUCHOBJECT)
                buttons = (buttons & ~CTRL_JUMP);
        }
        else if (state == STATE_HURT)
        {
            // Find Ennemie and hit
            Node* attacker = node->GetComponent<GOC_Life>()->GetLastAttacker();

            if (attacker)
            {
                float inrange = node->GetWorldPosition2D().x_ - attacker->GetWorldPosition2D().x_;

//                if (Abs(inrange) < 1.f)
                {
//                    URHO3D_LOGINFOF("GOB_Patrol() - Update : STATE_HURT => try to attack %s(%u)", attacker->GetName().CString(), attacker->GetID());

                    if (dirx * inrange > 0.f)
                    {
                        aiInfos.nextOrder = STATE_ATTACK;
                        order = GO_CHANGEDIRECTION.Value();
//                        URHO3D_LOGINFOF("GOB_Patrol() - Update : Hurt ! attacker is behind => Face to attacker (%u) then Attack (%u)", order, aiInfos.nextOrder);
                    }
                    else
                    {
                        order = STATE_ATTACK;
//                        URHO3D_LOGINFOF("GOB_Patrol() - Update : Hurt ! is in front of attacker => Attack");
                    }
                }
//                else
//                {
//                    URHO3D_LOGINFOF("GOB_Patrol() - Update : STATE_HURT => attacker %s(%u) not in the range !", attacker->GetName().CString(), attacker->GetID());
//                }

                aiInfos.target = attacker;
            }
//            else
//            {
//                URHO3D_LOGINFOF("GOB_Patrol() - Update : Hurt ! but no attacker");
//            }

            GOC_Abilities* gocabilities = node->GetComponent<GOC_Abilities>();
            if (gocabilities)
            {
                Ability* ability = gocabilities->GetAbility(ABI_AnimShooter::GetTypeStatic());

                if (!ability)
                    URHO3D_LOGINFOF("GOB_Patrol() - Update : %s(%u) STATE_HURT => No Ability ABI_AnimShooter !", node->GetName().CString(), node->GetID());

                if (ability && gocabilities->SetActiveAbility(ability))
                {
                    order = STATE_SHOOT;
                    URHO3D_LOGINFOF("GOB_Patrol() - Update : %s(%u) STATE_HURT => Has Ability ABI_AnimShooter => SHOOT !", node->GetName().CString(), node->GetID());
                }
            }

        }
//        else if (state == STATE_ATTACK && (buttons & CTRL_FIRE))
//        {
//            // reset after hit
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : Attack => reset button");
//            buttons &= ~CTRL_FIRE;
//        }
//        else if (state == STATE_SHOOT && (buttons & CTRL_FIRE2))
//        {
//            // reset after hit
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : Shoot => reset button");
//            buttons &= ~CTRL_FIRE2;
//        }
    }

    if (order)
        // if new order => config buttons and put behavior's update in standby until action release
    {
        // config buttons
        if (order == GO_CHANGEDIRECTION.Value())
        {
            buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            aiInfos.waitCallBackOrderOfType = EventTypeForOrder;
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : order GO_CHANGEDIRECTION(%u) => button=%u => wait for this Event", GO_CHANGEDIRECTION.Value(), buttons);
        }
        else if (order == STATE_ATTACK || order == STATE_SHOOT)
        {
            buttons = order == STATE_SHOOT ? CTRL_FIRE2 : CTRL_FIRE;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_Patrol() - Update : order %s(%u) => button=%u => wait for state !", GOS::GetStateName(order).CString(), order, buttons);
        }
        else
            // cancel unknown order
        {
            URHO3D_LOGWARNINGF("GOB_Patrol() - Update : order unknown = %u => cancel it!", order);
            order = 0;
        }
    }

//    URHO3D_LOGINFOF("GOB_Patrol() - Update : buttons=%u order=%u state=%s(%u) ... OK !", buttons, order, GOS::GetStateName(state).CString(), state);
}


// WAIT AND REPLAY TO ATTACK


void GOB_WaitAndDefend::Update(GOC_AIController& controller)
{
    Node* node = controller.GetNode();
    AInodeInfos& aiInfos = controller.GetaiInfos();
    const ObjectControlLocal& control = controller.control_;

    unsigned& order = aiInfos.order;
    unsigned& callBackOrder = aiInfos.callBackOrder;
    unsigned& buttons = aiInfos.buttons;
    const unsigned& state = control.animation_;
    const float& dirx = control.direction_;

    unsigned delay = Time::GetSystemTime() - aiInfos.lastUpdate;

#ifdef APPLY_RANDSKIP
    unsigned randomskip = Random(500);
    if (delay < randomskip)
    {
//        URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : apply random skip (%u)", randomskip);
        return;
    }
#endif // APPLY_RANDSKIP

//    URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : lastbuttons=%u", buttons);

    if (callBackOrder)
    {
        //check if order is released and reset
        if (order == callBackOrder)
        {
            aiInfos.lastOrder = order;
//            URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : order=%u is applied ; reset()", order);
            callBackOrder = 0;
            order = 0;
            if (aiInfos.nextOrder)
            {
                order = aiInfos.nextOrder;
                aiInfos.nextOrder = 0;
            }
        }
    }

    if (!order)
    {
        // mode default = check anim state
        if (state == STATE_DEFAULT || state == STATE_DEFAULT_GROUND || state == STATE_DEFAULT_AIR)
        {
            if (delay > 5000)
            {
//                URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : delay idle released => go walk");
                // change direction and walk
                buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            }
            //        else
            //            URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update ; wait delay idle");
        }
        else if (state == STATE_WALK)
        {
            if (delay > 200)
            {
//                URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : delay walk released => order stop");
                // stop
                buttons = 0;
            }
            //        else
            //            URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : wait delay walk");
        }
        else if (state == STATE_TALK)
        {
            buttons = 0;
        }
        else if (state == STATE_FALL)
        {
            //        // try hit
            //        buttons = buttons & CTRL_FIRE;
        }
        else if (state == STATE_HURT)
        {
            // Find Ennemie and hit
            Node* attacker = node->GetComponent<GOC_Life>()->GetLastAttacker();
            if (attacker)
            {
                order = STATE_ATTACK;
                if (dirx * (node->GetWorldPosition2D().x_ - attacker->GetWorldPosition2D().x_) > 0.f)
                {
                    aiInfos.nextOrder = order;
                    order = GO_CHANGEDIRECTION.Value();
//                    URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : Hurt ! attacker is behind => Face to attacker (%u) then Attack (%u)",
//                             order, aiInfos.nextOrder);
                }
//                else
//                    URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : Hurt ! is in front of attacker => Attack");
            }
//            else
//                URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : Hurt ! but no attacker");
        }
        else if (state == STATE_ATTACK)
        {
            // reset after hit
//            URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : Attack => reset after hit");
            buttons = 0;
        }
    }

    if (order)
        // if new order => config buttons and put behavior's update in standby until action release
    {
        // config buttons
        if (order == GO_CHANGEDIRECTION.Value())
        {
            buttons = (dirx > 0.f ? CTRL_LEFT : CTRL_RIGHT);
            aiInfos.waitCallBackOrderOfType = EventTypeForOrder;
//            URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : order GO_CHANGEDIRECTION(%u) => button=%u => wait for this Event", GO_CHANGEDIRECTION.Value(), buttons);
        }
        else if (order == STATE_ATTACK)
        {
            buttons = CTRL_FIRE;
            aiInfos.waitCallBackOrderOfType = StateTypeForOrder;
//            URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : order Attack(%u) => button=%u => wait for State=State_Attack !", STATE_ATTACK, buttons);
        }
        else
            // cancel unknown order
        {
//            URHO3D_LOGWARNINGF("GOB_WaitAndDefend() - Update : order unknown = %u => cancel it!", order);
            order = 0;
        }
    }

//    URHO3D_LOGINFOF("GOB_WaitAndDefend() - Update : buttons=%u", buttons);
}
