#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Input/Controls.h>

#include <Urho3D/Urho2D/CollisionCircle2D.h>

#include "DefsGame.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"

#include "Behavior.h"

#include "GOC_Destroyer.h"
#include "GOC_Detector.h"
#include "GOC_Move2D.h"

#include "GOC_ControllerAI.h"


GOC_AIController::GOC_AIController(Context* context) :
    GOC_Controller(context, GO_AI_Enemy),
    behavior_(0),
    behaviorDefault_(BEHAVIOR_DEFAULT),
    behaviorOnTarget_(BEHAVIOR_DEFAULT),
    lastBehavior_(0),
    targetID_(0),
    started_(0),
    targetDetection_(false),
    alwaysUpdate_(false),
    externalController_(0)
{
    SetMainController(true);
    aiInfos_.buttons = 0;
}

GOC_AIController::GOC_AIController(Context* context, int type) :
    GOC_Controller(context, type),
    behavior_(0),
    behaviorDefault_(BEHAVIOR_DEFAULT),
    behaviorOnTarget_(BEHAVIOR_DEFAULT),
    lastBehavior_(0),
    targetID_(0),
    started_(0),
    targetDetection_(false),
    alwaysUpdate_(false),
    externalController_(0)
{
    SetMainController(true);
    aiInfos_.buttons = 0;
}

GOC_AIController::~GOC_AIController()
{ }

void GOC_AIController::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_AIController>();
    URHO3D_ACCESSOR_ATTRIBUTE("Controller Type", GetControllerType, SetControllerTypeAttr, int, GO_AI_Enemy, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Behavior", GetBehaviorAttr, SetBehaviorAttr, String, "Patrol", AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Behavior Target", GetBehaviorTargetAttr, SetBehaviorTargetAttr, String, "Patrol", AM_FILE);
    URHO3D_ATTRIBUTE("Detects Target", bool, targetDetection_, false, AM_DEFAULT);
}

void GOC_AIController::SetControllerTypeAttr(int type)
{
//    URHO3D_LOGINFOF("GOC_AIController() - SetControllerTypeAttr %s(%u) type=%d", node_->GetName().CString(), node_->GetID(), type);
    SetControllerType(type);
}

void GOC_AIController::SetDetectTarget(bool enable, float radius)
{
    if (enable)
    {
        GOC_Detector* detector = node_->GetComponent<GOC_Detector>();
        if (!detector)
        {
            detector = node_->CreateComponent<GOC_Detector>();

            CollisionCircle2D* cshape = node_->CreateComponent<CollisionCircle2D>();
            cshape->SetRadius(radius);
            cshape->SetTrigger(true);
            cshape->SetViewZ(node_->GetVar(GOA::ONVIEWZ).GetInt());
        }

        detector->SetDetectedCategoriesAttr("Characters");
        detector->SetSameViewOnly(true);
    }

    targetDetection_ = enable;
}

void GOC_AIController::SetTarget(unsigned targetID, bool reset)
{
    if (!targetID && aiInfos_.targetID && !reset)
        targetID = aiInfos_.targetID;

    if (targetID == node_->GetID() && !reset)
        return;

    targetID_ = targetID;

    aiInfos_.Reset();
    if (targetID_)
    {
        aiInfos_.target = GameContext::Get().Get().rootScene_->GetNode(targetID_);
        aiInfos_.targetID = aiInfos_.target ? targetID_ : 0;
    }

    URHO3D_LOGINFOF("GOC_AIController() - SetTarget %s(%u) : set target node=%s(id=%u ptr=%u)",
                    node_->GetName().CString(), node_->GetID(), aiInfos_.target ? aiInfos_.target->GetName().CString() : "",
                    aiInfos_.target ? aiInfos_.target->GetID() : 0, aiInfos_.target.Get());
}

void GOC_AIController::SetBehavior(unsigned b)
{
    if (b && b != behavior_)
    {
        behavior_ = b;

        Behavior* behavior = Behaviors::Get(behavior_);
        URHO3D_LOGINFOF("GOC_AIController() - SetBehavior : %s(%u) behavior=%s", node_->GetName().CString(), node_->GetID(),
                         behavior->GetTypeName().Substring(4).CString());
    }
}

void GOC_AIController::SetBehaviorAttr(const String& n)
{
    if (!n.Empty())
    {
//        URHO3D_LOGERRORF("GOC_AIController() - SetBehaviorAttr : %s(%u) behavior=%s", node_->GetName().CString(), node_->GetID(), n.CString());
        behaviorDefault_ = StringHash(String("GOB_") + n).Value();
        SetBehavior(behaviorDefault_);
    }
}

String GOC_AIController::GetBehaviorAttr() const
{
    return Behaviors::Get(behaviorDefault_)->GetTypeName().Substring(4);
}

void GOC_AIController::SetBehaviorTargetAttr(const String& n)
{
    if (!n.Empty())
    {
        behaviorOnTarget_ = StringHash(String("GOB_") + n).Value();
    }
}

String GOC_AIController::GetBehaviorTargetAttr() const
{
    return Behaviors::Get(behaviorOnTarget_)->GetTypeName().Substring(4);
}

Behavior* GOC_AIController::StartBehavior(unsigned b, bool forced)
{
    if (!b)
        b = lastBehavior_;

    if (behavior_ && behavior_ != b)
    {
//        URHO3D_LOGINFOF("GOC_AIController() - StartBehavior : node=%s(%u) ... stop previous behavior",
//                        node_->GetName().CString(), node_->GetID());
        StopBehavior();
    }

    Behavior* behavior = Behaviors::Get(b);

    if (behavior)
    {
        if (behavior_ != b || forced)
        {
            SetBehavior(b);

            behavior->Start(*this);
//            URHO3D_LOGINFOF("GOC_AIController() - StartBehavior : %s(%u) behavior=%s ... OK !", node_->GetName().CString(), node_->GetID(),
//                                    behavior ? behavior->GetTypeName().Substring(4).CString() : "unknow");
        }
    }

    return behavior;
}

void GOC_AIController::StopBehavior()
{
    if (!behavior_)
        return;

    Behavior* behavior = Behaviors::Get(behavior_);
    if (behavior)
    {
        lastBehavior_ = behavior_;
//        URHO3D_LOGINFOF("GOC_AIController() - StopBehavior : node=%s(%u) behavior=%s",
//                        node_->GetName().CString(), node_->GetID(), behavior->GetTypeName().Substring(4).CString());
        behavior->Stop(*this);
    }

    behavior_ = 0;
}

void GOC_AIController::Start()
{
    if (!mainController_)
        return;

    if (started_)
        return;

//    URHO3D_LOGINFOF("GOC_AIController() - Start %s(%u) ... ", node_->GetName().CString(), node_->GetID());

    started_ = true;

    if (GetControllerType() == GO_AI_Ally)
    {
        behaviorOnTarget_ = GOB_FOLLOWATTACK;
        behaviorOnTargetDead_ = GOB_PATROL;
        behaviorDefault_ = GOB_FOLLOW;
    }
    else if (GetControllerType() == GO_AI)
    {
        behaviorOnTarget_ = GOB_PLAYERCPU;
        behaviorOnTargetDead_ = GOB_PATROL;
        behaviorDefault_ = GOB_PATROL;
    }

    GOC_Controller::Start();

    Behavior* b = StartBehavior(behaviorDefault_, true);

    aiInfos_.Reset();
    aiInfos_.buttons = control_.direction_ > 0.f ? CTRL_RIGHT : CTRL_LEFT;
    aiInfos_.lastMoveType = node_->GetComponent<GOC_Move2D>()->GetMoveType();
    lastTime_ = aiInfos_.lastUpdate = Time::GetSystemTime();

    if (targetID_)
        SetTarget(targetID_);

    SubscribeToEvent(node_, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_AIController, OnDead));

    if (GetControllerType() == GO_AI_Ally)
    {
        if (aiInfos_.target)
            SubscribeToEvent(aiInfos_.target, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_AIController, OnFollowedNodeDead));

        SubscribeToEvent(GO_SELECTED, URHO3D_HANDLER(GOC_AIController, OnEntitySelection));
//        URHO3D_LOGINFOF("GOC_AIController() - Start %s : SubscribeTo GO_SELECTED", node_->GetName().CString());
    }

//    URHO3D_LOGINFOF("GOC_AIController() - Start %s(%u) ... OK !", node_->GetName().CString(), node_->GetID());
}

void GOC_AIController::Stop()
{
    if (!started_)
        return;

//    URHO3D_LOGINFOF("GOC_AIController() - Stop %s(%u)", node_->GetName().CString(), node_->GetID());

    UnsubscribeFromEvent(node_, GOC_LIFEDEAD);

    StopBehavior();

    GOC_Controller::Stop();

    if (this->GetControllerType() == GO_AI_Ally)
    {
        UnsubscribeFromEvent(GO_SELECTED);
//        URHO3D_LOGINFOF("GOC_AIController() - Stop %s : UnsubscribeFrom GO_SELECTED", node_->GetName().CString());
    }

    started_ = false;
}

bool GOC_AIController::MountOn(Node* node)
{
    SetTarget(node->GetID(), true);
    StartBehavior(GOB_MOUNTON, true);

    return true;
}

bool GOC_AIController::Unmount()
{
    SetTarget(0, true);
    StopBehavior();

    return true;
}

void GOC_AIController::ResetOrder()
{
    if (!aiInfos_.order && aiInfos_.lastOrder)
        aiInfos_.order = aiInfos_.lastOrder;

    if (aiInfos_.order)
    {
//        URHO3D_LOGINFOF("GOC_AIController() - ResetOrder() : node=%u reset order !", node_->GetID());

        if (aiInfos_.waitCallBackOrderOfType)
        {
            if (aiInfos_.waitCallBackOrderOfType == StateTypeForOrder)
            {
                UnsubscribeFromEvent(node_, GO_CHANGESTATE);
            }
            else
            {
                UnsubscribeFromEvent(node_, E_SCENEPOSTUPDATE);
                UnsubscribeFromEvent(node_, StringHash(aiInfos_.order));
            }

            aiInfos_.waitCallBackOrderOfType = 0;
        }

        aiInfos_.order = aiInfos_.callBackOrder = 0;
        aiInfos_.updateNeed = true;
    }
}

void GOC_AIController::ChangeOrder(unsigned order)
{
//    URHO3D_LOGINFOF("GOC_AIController() - ChangeOrder : ID=%u neworder=%u !", node_->GetID(), order);

    ResetOrder();

    aiInfos_.order = order;
    aiInfos_.callBackOrder = order;
    aiInfos_.buttons = 0;

    VariantMap& eventdata = context_->GetEventDataMap();
    eventdata[Ai_ChangeOrder::AI_ORDER] = order;
    node_->SendEvent(AI_CHANGEORDER, eventdata);
}

void GOC_AIController::UpdateRangeValues(MoveTypeMode movetype)
{
//    URHO3D_LOGINFOF("GOC_AIController() - UpdateRangeValues : nodeID=%u movetype=%d !", node_->GetID(), (int)movetype);

    if (aiInfos_.lastMoveType <= MOVE2D_WALKANDJUMPFIRSTBOXANDSWIM)
    {
        if (this->GetControllerType() & (GO_Player|GO_AI_Ally))
        {
            aiInfos_.minRangeTarget = defaultMinAllyRangeTarget;
            aiInfos_.maxRangeTarget = defaultMaxAllyRangeTarget;
        }
        else
        {
            aiInfos_.minRangeTarget = defaultMinWlkRangeTarget;
            aiInfos_.maxRangeTarget = defaultMaxWlkRangeTarget;
        }
    }
    else if (aiInfos_.lastMoveType >= MOVE2D_WALKANDFLY && aiInfos_.lastMoveType<= MOVE2D_FLYCLIMB)
    {
        aiInfos_.minRangeTarget = defaultMinFlyRangeTarget;
        aiInfos_.maxRangeTarget = defaultMaxFlyRangeTarget;
    }
}


void GOC_AIController::UpdateTargetDetection()
{
    // detect a target
    if (!aiInfos_.target)
    {
        // try to get the node with the targetID
        if (aiInfos_.targetID)
        {
            aiInfos_.target = node_->GetScene()->GetNode(aiInfos_.targetID);
            aiInfos_.aggressiveDelay = 0U;
        }

        // no targetID, take one in the detector
        if (!aiInfos_.targetID)
        {
            GOC_Detector* detector = node_->GetComponent<GOC_Detector>();
            if (detector)
            {
                aiInfos_.targetID = detector->GetFirstNodeDetect();
                if (aiInfos_.targetID)
                {
                    aiInfos_.target = node_->GetScene()->GetNode(aiInfos_.targetID);
                    aiInfos_.aggressiveDelay = 0U;
//                    URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) detect a prey => set targetID=%u", node_->GetName().CString(), node_->GetID(), aiInfos_.targetID);
                }
            }
        }
    }

    // reset target if disabled
    if (aiInfos_.target && !aiInfos_.target->IsEnabled())
    {
        aiInfos_.target.Reset();
        aiInfos_.buttons = 0;
        aiInfos_.aggressiveDelay = 0;
        GOC_Detector* detector = node_->GetComponent<GOC_Detector>();
        if (detector)
            detector->RemoveDetectedNode(aiInfos_.targetID);
        aiInfos_.targetID = 0;

//        URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) target=%u is disabled => reset target !", node_->GetName().CString(), node_->GetID(), aiInfos_.targetID);
    }
}

void GOC_AIController::UpdateAggressivity(unsigned time)
{
    if (aiInfos_.target)
    {
        // check target inside/outside detector
        GOC_Detector* detector = node_->GetComponent<GOC_Detector>();
        if (detector)
        {
            if (!detector->Contains(aiInfos_.targetID))
            {
                aiInfos_.aggressiveDelay += time - lastTime_;
//                URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) outside detector -> update aggressive delay=%u !", node_->GetName().CString(), node_->GetID(), aiInfos_.aggressiveDelay);
            }
            else// if (aiInfos_.aggressiveDelay)
            {
                aiInfos_.aggressiveDelay = 0;
//                URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) inside detector -> reset aggressive delay=%u !", node_->GetName().CString(), node_->GetID(), aiInfos_.aggressiveDelay);
            }
        }
        // no detector : update also the aggro delay
        else
        {
            aiInfos_.aggressiveDelay += time - lastTime_;
        }

        // check aggro delay expired
        if (aiInfos_.aggressiveDelay > MAX_AIAGGRESSIVEDELAY)
        {
            // switch target
            unsigned newid = detector ? detector->GetFirstNodeDetect() : 0;
            if (newid && aiInfos_.targetID != newid)
            {
//                URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) aggressive delay expired => switch to detector targetid=%u !", node_->GetName().CString(), node_->GetID(), newid);
                aiInfos_.targetID = newid;
                aiInfos_.target = node_->GetScene()->GetNode(aiInfos_.targetID);
            }
            // reset the target : patrol
            else
            {
//                URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) aggressive delay expired => reset target !", node_->GetName().CString(), node_->GetID());
                aiInfos_.target.Reset();
                aiInfos_.targetID = 0;
                aiInfos_.buttons = 0;
            }

            aiInfos_.aggressiveDelay = 0;
        }
    }
}

Behavior* GOC_AIController::ChooseBehavior()
{
    // set the good behavior between Target or NoTarget or Mounted
    // TODO : replace this by a State Machine ?

    Behavior* behavior = 0;

//    URHO3D_LOGINFOF("GOC_AIController() - ChooseBehavior() : choose behavior ... ally=%s target=%u mountedon=%u",
//                    GetControllerType() & GO_AI_Ally ? "true":"false", aiInfos_.target?aiInfos_.target->GetID():0, node_->GetVar(GOA::ISMOUNTEDON).GetUInt());

    if (aiInfos_.target)
    {
        if (GetControllerType() & GO_AI_Ally)
        {
            if (node_->GetVar(GOA::ISMOUNTEDON).GetUInt() == aiInfos_.target->GetID())
                behavior = Behaviors::Get(behavior_);
            else if (aiInfos_.target->GetVar(GOA::TYPECONTROLLER).GetUInt() & (GO_AI_Ally | GO_Player))
                behavior = StartBehavior(GOB_FOLLOW);
            else
                behavior = StartBehavior(behaviorOnTarget_);
        }
        else
        {
            behavior = StartBehavior(behaviorOnTarget_);
        }
    }
    else
    {
        bool targetdeadbehavior = ((GetControllerType() & GO_AI_Ally) && behavior_ == behaviorOnTargetDead_);
        behavior = Behaviors::Get(targetdeadbehavior ? behaviorOnTargetDead_ : behaviorDefault_);
    }

    return behavior;
}

bool GOC_AIController::Update(unsigned time)
{
    if (!IsStarted())
        return true;

    if (externalController_)
    {
        bool update = GOC_Controller::Update(externalController_->control_.buttons_, true);
        return true;
    }

//    URHO3D_LOGINFOF("GOC_AIController() - Update : node=%s(%u) targetdetection=%d updateaggro=%d ...", node_->GetName().CString(), node_->GetID(), targetDetection_, aiInfos_.updateAggressivity);

    if (targetDetection_)
    {
        UpdateTargetDetection();
    }

    if (aiInfos_.updateAggressivity)
    {
        UpdateAggressivity(time);
    }

    Behavior* behavior = ChooseBehavior();
    if (!behavior)
    {
        URHO3D_LOGINFOF("GOC_AIController() - Update() : can't find behavior for node %u ! Waiting for new behavior ", node_->GetID());
        return true;
    }
//    else
//    {
//        URHO3D_LOGINFOF("GOC_AIController() - Update() : choose behavior %s", behavior->GetTypeName().CString());
//    }

    lastTime_ = time;

    // skip if no update need
    if (!alwaysUpdate_ && !aiInfos_.updateNeed)
    {
        // unblock if waiting too long for callback
        if (time - aiInfos_.lastUpdate > MAX_AIWAITCALLBACKDELAY)
        {
            aiInfos_.Reset();
            URHO3D_LOGERRORF("GOC_AIController() - Update() : node %u waits too long ! cancel order", node_->GetID());
        }
//        else
//        {
//            URHO3D_LOGERRORF("AIManager() - Update() : node %u no need update !", node_->GetID());
//        }

        return true;
    }

    // skip if updatedelay expired
    {
        unsigned updatedelay = time - aiInfos_.lastUpdate;
        // reset order if STATE_JUMP and MaxJumpDelay expired
        if (aiInfos_.order == STATE_JUMP && updatedelay > MAX_AIJUMPDELAY)
        {
            aiInfos_.callBackOrder = aiInfos_.order;
            UnsubscribeFromEvent(node_, GO_CHANGESTATE);
        }
        // skip if lastupdate finished a short time ago
        else if (updatedelay < MIN_AIUPDATEDELAY)
        {
            return true;
        }
    }

//    URHO3D_LOGINFOF("GOC_AIController() - Update() : %s(%u) time=%u buttons=%u ...",
//                    node_->GetName().CString(), node_->GetID(), time, aiInfos_.buttons);

    // update logic behavior
    behavior->Update(*this);

    if (aiInfos_.waitCallBackOrderOfType || aiInfos_.buttons != node_->GetVar(GOA::BUTTONS).GetUInt())
    {
        // to achieve an order, aicontrol handle alone and callback
        if (aiInfos_.waitCallBackOrderOfType && aiInfos_.order)
        {
//            URHO3D_LOGINFOF("GOC_AIController() - Update() : node=%u wait for callback !", node_->GetID());

            aiInfos_.updateNeed = false;

            if (aiInfos_.waitCallBackOrderOfType == StateTypeForOrder)
            {
                SubscribeToEvent(node_, GO_CHANGESTATE, URHO3D_HANDLER(GOC_AIController, OnEvent));
//                URHO3D_LOGINFOF("GOC_AIController() - Update() : node=%u subscribe to GO_CHANGESTATE = %u !", node_->GetID(), GO_CHANGESTATE.Value());
            }
            else
            {
                SubscribeToEvent(node_, StringHash(aiInfos_.order), URHO3D_HANDLER(GOC_AIController, OnEvent));
                SubscribeToEvent(node_, E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_AIController, OnEvent));
//                URHO3D_LOGINFOF("GOC_AIController() - Update() : node=%u subscribe to event=%s(%u) !",
//                             node_->GetID(), GOE::GetEventName(StringHash(aiInfos_.order)).CString(), aiInfos_.order);
            }
        }

        // update control
        prevbuttons_ = control_.buttons_;
        bool update = GOC_Controller::Update(aiInfos_.buttons, true);
        aiInfos_.lastUpdate = time;
    }

    if (aiInfos_.waitCallBackOrderOfType && !aiInfos_.order)
    {
//        URHO3D_LOGERRORF("GOC_AIController() - Update() : node=%u wait for callback but no order ! => Reset callback", node_->GetID());

        if (aiInfos_.waitCallBackOrderOfType == StateTypeForOrder)
        {
            UnsubscribeFromEvent(node_, GO_CHANGESTATE);
        }
        else
        {
            UnsubscribeFromEvent(node_, E_SCENEPOSTUPDATE);
            if (aiInfos_.lastOrder)
            {
                UnsubscribeFromEvent(node_, StringHash(aiInfos_.lastOrder));
                // allow to change to nextorder (if exist) in the Behavior
                aiInfos_.order = aiInfos_.callBackOrder = aiInfos_.lastOrder;
                aiInfos_.lastOrder = 0;
            }
        }

        aiInfos_.waitCallBackOrderOfType = 0;
    }

//    URHO3D_LOGINFOF("GOC_AIController() - Update() : %s(%u) time=%u buttons=%u ... OK !",
//                    node_->GetName().CString(), node_->GetID(), time, aiInfos_.buttons);
    return true;
}

void GOC_AIController::OnDead(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("GOC_AIController() - OnDead : GO Dead %s(%u)", node_->GetName().CString(), node_->GetID());

    Stop();
}

void GOC_AIController::OnFollowedNodeDead(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("GOC_AIController() - OnFollowedNodeDead : ID=%u !", node_->GetID());

    if (aiInfos_.target)
        UnsubscribeFromEvent(aiInfos_.target, GOC_LIFEDEAD);

    unsigned backuptargetid = aiInfos_.targetID;
    aiInfos_.Reset();
    aiInfos_.target.Reset();
    aiInfos_.targetID = backuptargetid;

    StartBehavior(behaviorOnTargetDead_, true);
}

void GOC_AIController::OnEntitySelection(StringHash eventType, VariantMap& eventData)
{
    unsigned type = eventData[Go_Selected::GO_TYPE].GetUInt();
    unsigned selectedID = eventData[Go_Selected::GO_ID].GetUInt();

//    URHO3D_LOGERRORF("GOC_AIController() - OnEntitySelection : ID=%u selected=%u type=%u ...",
//                        node_->GetID(), selectedID, type);

    if (selectedID < 2)
        return;

    if (selectedID == node_->GetID())
    {
        URHO3D_LOGERRORF("GOC_AIController() - OnEntitySelection : ID=%u <= selected target %u ... stop behavior !",
                         node_->GetID(), selectedID);
        return;
//        SetTarget(0, true);
//        StopBehavior();
//        return;
    }

    if (type == GO_AI_Enemy)
    {
        Node* selectedNode = GetScene()->GetNode(selectedID);

        if (selectedNode && selectedNode != node_)
        {
            URHO3D_LOGINFOF("GOC_AIController() - OnEntitySelection : ID=%u on Enemy node=%s(%u) => Follow and Attack !",
                            node_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());

            SetTarget(selectedID);
            StartBehavior(GOB_FOLLOWATTACK);
        }
    }
    else if (type & (GO_Player|GO_NetPlayer|GO_AI_Ally))
    {
        Node* selectedNode = GetScene()->GetNode(selectedID);
        if (!selectedNode)
            return;

        unsigned mountedOnID = node_->GetVar(GOA::ISMOUNTEDON).GetUInt();
        Node* mountedOnNode  = mountedOnID ? GetScene()->GetNode(mountedOnID) : 0;

        URHO3D_LOGERRORF("GOC_AIController() - OnEntitySelection : ID=%u selected target %u node=%u mountedID=%u ...",
                         node_->GetID(), selectedID, selectedNode, mountedOnID);

        bool mountok = false;

        // if selectnode is the node
        if (selectedID == mountedOnID)
        {
            SetTarget(0);

            // if mounted on, unmount and follow target
            if (mountedOnNode)
            {
                URHO3D_LOGINFOF("GOC_AIController() - OnEntitySelection : ID=%u on Mounted Node=%s(%u) => Unmount and Follow !",
                                node_->GetID(), mountedOnNode->GetName().CString(), mountedOnNode->GetID());
                StartBehavior(GOB_FOLLOW);
                mountok = true;
                mountedOnID = 0;
            }
            else
            {
                URHO3D_LOGINFOF("GOC_AIController() - OnEntitySelection : ID=%u on same node=%s(%u) => Stop !",
                                node_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());
                StopBehavior();
            }
        }
        // if not mounted, try to mount the selected node
        else if (!mountedOnNode)
        {
            SetTarget(selectedNode->GetID());

            if ((type & GO_Player) && aiInfos_.target)
                SubscribeToEvent(aiInfos_.target, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_AIController, OnFollowedNodeDead));

            Vector2 delta = selectedNode->GetPosition2D() - node_->GetPosition2D();
            // Follow if not near
            if (delta.LengthSquared() > DISTANCEFORMOUNT)
            {
                URHO3D_LOGINFOF("GOC_AIController() - OnEntitySelection : ID=%u on friend node=%s(%u) => Follow !",
                                node_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());

                StartBehavior(GOB_FOLLOW);
            }
            // Mount if near
            else if (!selectedNode->GetVar(GOA::ISMOUNTEDON).GetUInt())
            {
                URHO3D_LOGINFOF("GOC_AIController() - OnEntitySelection : ID=%u near friend node=%s(%u) => Mount on It !",
                                node_->GetID(), selectedNode->GetName().CString(), selectedNode->GetID());

                StartBehavior(GOB_MOUNTON);
                mountok = true;
                mountedOnID = selectedNode->GetID();
            }
        }

        if (GameContext::Get().ServerMode_ && mountok)
        {
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Net_ObjectCommand::P_NODEID] = node_->GetID();
            eventData[Net_ObjectCommand::P_NODEPTRFROM] = mountedOnID;
            eventData[Net_ObjectCommand::P_CLIENTID] = 0;
            node_->SendEvent(GO_MOUNTEDON, eventData);
        }
    }
}

void GOC_AIController::OnEvent(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_SCENEPOSTUPDATE)
    {
        bool update = GOC_Controller::Update(aiInfos_.buttons, false);
        return;
    }
    else if (eventType == GO_CHANGESTATE)
    {
//        unsigned state = eventData[Go_ChangeState::GO_STATE].GetUInt();

        if (control_.animation_ != aiInfos_.order)
        {
            if (aiInfos_.order == STATE_ATTACK && (control_.animation_ == STATE_ATTACK_GROUND || control_.animation_ == STATE_ATTACK_AIR))
            {
                ;
            }
            else if (aiInfos_.order == STATE_JUMP && control_.animation_ == STATE_FLYUP)
            {
                ; //RHO3D_LOGINFOF("GOC_AIController() - OnEvent node=%u : STATE_JUMP OK !", node_->GetID());;
            }
            else
            {
//                URHO3D_LOGINFOF("GOC_AIController() - OnEvent : %s(%u) animstate=%s(%u) received != order=%s(%u) - listen again GO_CHANGESTATE ! Repost button",
//                                 node_->GetName().CString(), node_->GetID(), GOS::GetStateName(control_.animation_).CString(), control_.animation_,
//                                GOS::GetStateName(aiInfos_.order).CString(), aiInfos_.order);

                bool update = GOC_Controller::Update(aiInfos_.buttons, true);
                return;
            }
        }

        aiInfos_.callBackOrder = aiInfos_.order;

//        URHO3D_LOGINFOF("GOC_AIController() - OnEvent node=%u : ChangeState OK !", node_->GetID());
    }
    else
    {
        aiInfos_.callBackOrder = eventType.Value();

//        URHO3D_LOGINFOF("GOC_AIController() - OnEvent node=%u : Event %s(%u) received",
//                        node_->GetID(), GOE::GetEventName(eventType).CString(), eventType.Value());
    }

    aiInfos_.updateNeed = true;
    aiInfos_.waitCallBackOrderOfType = 0;

    UnsubscribeFromEvent(node_, eventType);
}

//void GOC_AIController::PushCommandStack(int value)
//{
//    assert(aiInfos_.stackCommandSize_ < MAX_STACKCOMMAND);
//    aiInfos_.stackCommand_[aiInfos_.stackCommandSize_++] = value;
//}
//
//int GOC_AIController::PopCommandStack()
//{
//    assert(aiInfos_.stackCommandSize_ > 0);
//    return aiInfos_.stackCommand_[--aiInfos_.stackCommandSize_];
//}
//
//void GOC_AIController::ClearCommandStack()
//{
//    aiInfos_.stackCommandSize_ = 0;
//}

extern const char* moveTypeModes[];

void GOC_AIController::Dump() const
{
    URHO3D_LOGINFOF("GOC_AIController() - Dump : %s(%u) : started=%s ...", node_->GetName().CString(), node_->GetID(), started_ ? "true":"false");
    URHO3D_LOGINFOF(" => behavior=%s(%u)", behavior_ ? Behaviors::Get(behavior_)->GetTypeName().CString() : "", behavior_);
    URHO3D_LOGINFOF(" => movetype=%s(%d)", moveTypeModes[(int)aiInfos_.lastMoveType], (int)aiInfos_.lastMoveType);
    URHO3D_LOGINFOF(" => target=%s(%u) targetID=%u", aiInfos_.target ? aiInfos_.target->GetName().CString() : "none", aiInfos_.target ? aiInfos_.target->GetID() : 0, aiInfos_.targetID);
    URHO3D_LOGINFOF(" => state=%s(%u) laststate=%s(%u)", GOS::GetStateName(aiInfos_.state).CString(), aiInfos_.state, GOS::GetStateName(aiInfos_.lastState).CString(), aiInfos_.lastState);
    URHO3D_LOGINFOF(" => buttons=%u", aiInfos_.buttons);
    URHO3D_LOGINFOF(" => order=%s(%u) lastorder=%s(%u) callbackorder=%s(%u)", GOS::GetStateName(StringHash(aiInfos_.order)).CString(), aiInfos_.order,
                    GOS::GetStateName(StringHash(aiInfos_.lastOrder)).CString(), aiInfos_.lastOrder, GOS::GetStateName(StringHash(aiInfos_.callBackOrder)).CString(), aiInfos_.callBackOrder);
    URHO3D_LOGINFOF(" => updateneed=%u updateaggro=%u", aiInfos_.updateNeed, aiInfos_.updateAggressivity);
}
