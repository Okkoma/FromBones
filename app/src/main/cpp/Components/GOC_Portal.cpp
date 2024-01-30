#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameNetwork.h"
#include "GameContext.h"

#include "TimerRemover.h"
#include "Map.h"
#include "MapWorld.h"
#include "MapCreator.h"
#include "ViewManager.h"
#include "GEF_Scrolling.h"
#include "Actor.h"

#include "GOC_Controller.h"
#include "GOC_ControllerAI.h"
#include "GOC_Collide2D.h"
#include "GOC_Destroyer.h"
#include "GOC_Move2D.h"

#include "GOC_Portal.h"

extern const char* mapStatusNames[];

const float PORTALCAMERADELAY = 3.f;
const float PORTALDESACTIVEDELAY = 10.f;
#define MAX_NUMPORTALTRANSFERPERIODS 5

URHO3D_EVENT(PORTAL_REACTIVE, Portal_Reactive)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
}

URHO3D_EVENT(PORTAL_CAMERA, Portal_Camera)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
}


GOC_Portal::GOC_Portal(Context* context) :
    Component(context),
    dPosition_(UndefinedMapPosition),
    dViewZ_(NOVIEW)
{ }

GOC_Portal::~GOC_Portal()
{
//    URHO3D_LOGINFOF("~GOC_Portal");
}

void GOC_Portal::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Portal>();
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Destination Map", GetDestinationMap, SetDestinationMap, IntVector2, IntVector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Destination Position", GetDestinationPosition, SetDestinationPosition, IntVector2, UndefinedMapPosition, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Destination ViewZ", GetDestinationViewZ, SetDestinationViewZ, int, NOVIEW, AM_DEFAULT);
}

bool GOC_Portal::IsDestinationDefined() const
{
    return dPosition_ != UndefinedMapPosition;
}

void GOC_Portal::OnSetEnabled()
{
    if (GetScene())
    {
        if (IsEnabledEffective())
        {
//            URHO3D_LOGINFOF("GOC_Portal() - OnSetEnabled() : nodeID=%u enable=true", node_->GetID());
            GetComponent<AnimatedSprite2D>()->SetAnimation("no_move");
            UnsubscribeFromEvent(PORTAL_CAMERA);
            UnsubscribeFromEvent(GO_ENDTIMER);
            UnsubscribeFromEvent(PORTAL_REACTIVE);
            SubscribeToEvent(GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Portal, HandleBeginContact));
        }
        else
        {
//            URHO3D_LOGINFOF("GOC_Portal() - OnSetEnabled() : nodeID=%u enable=false", node_->GetID());
            UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
//            if (!node_->HasTag("InUse"))
            if (node_->isPoolNode_ && node_->isInPool_)
                UnsubscribeFromEvent(PORTAL_REACTIVE);
        }
    }
}

void GOC_Portal::OnNodeSet(Node* node)
{
    if (node)
    {
        enabled_ = true;
        SubscribeToEvent(node, GO_APPEAR, URHO3D_HANDLER(GOC_Portal, HandleAppear));

        OnSetEnabled();
    }
}

void GOC_Portal::SetDestinationMap(const ShortIntVector2& map)
{
    if (!World2D::GetWorld())
        return;

    // check if in world bounds
    if (World2D::IsInsideWorldBounds(map))
    {
        URHO3D_LOGINFOF("GOC_Portal() - SetDestinationMap : nodeID=%u map=%s", node_->GetID(), map.ToString().CString());
        dMap_.x_ = map.x_;
        dMap_.y_ = map.y_;
    }
    else
        URHO3D_LOGWARNINGF("GOC_Portal() - SetDestinationMap : nodeID=%u map=%s not inside world Bounds", node_->GetID(), map.ToString().CString());
}

void GOC_Portal::SetDestinationMap(const IntVector2& map)
{
    SetDestinationMap(ShortIntVector2(map.x_, map.y_));
}

void GOC_Portal::SetDestinationPosition(const IntVector2& position)
{
    if (position == UndefinedMapPosition)
    {
        dPosition_ = UndefinedMapPosition;
        URHO3D_LOGINFOF("GOC_Portal() - SetDestinationPosition : nodeID=%u position=undefined", node_->GetID());
        return;
    }

    if (!World2D::GetWorld())
        return;

    if (position.x_ >=0 && position.x_ < World2D::GetWorldInfo()->mapWidth_ && position.y_ >=0 && position.y_ < World2D::GetWorldInfo()->mapHeight_)
    {
//        URHO3D_LOGINFOF("GOC_Portal() - SetDestinationPosition : nodeID=%u position=%s", node_->GetID(), position.ToString().CString());
        dPosition_ = position;
    }
    else
    {
        dPosition_ = UndefinedMapPosition;
        URHO3D_LOGWARNINGF("GOC_Portal() - SetDestinationPosition : nodeID=%u position=%s out of map size=%d %d ", node_->GetID(),
                           position.ToString().CString(), World2D::GetWorldInfo()->mapWidth_, World2D::GetWorldInfo()->mapHeight_);
    }
}

void GOC_Portal::SetDestinationViewZ(int viewz)
{
    dViewZ_ = viewz;
}

void GOC_Portal::Teleport(Node* node)
{
    if (!node)
        return;

    URHO3D_LOGINFOF("GOC_Portal() - Teleport : node=%s(%u) to map=%s position=%s",
                    node->GetName().CString(), node->GetID(), dMap_.ToString().CString(), IsDestinationDefined() ? dPosition_.ToString().CString() : "undefined");

    // disable node after portal animation
    TimerRemover* disableTimer = TimerRemover::Get();
    disableTimer->Start(node, 0.4f, DISABLE, 0.f, node->GetID());

    teleportedInfos_.Resize(teleportedInfos_.Size()+1);
    teleportedInfos_.Back().nodeid_ = node->GetID();
}

void GOC_Portal::HandleAppear(StringHash eventType, VariantMap& eventData)
{
    // if not Destination, randomize map point destination here
    if (!IsDestinationDefined())
    {
        ShortIntVector2 mpoint(eventData[Go_Appear::GO_MAP].GetUInt());

        if (World2D::IsInfinite())
        {
//            dMap_.x_ = mpoint.x_ + GameRand::GetRand(OBJRAND, -5, 5);
//            dMap_.y_ = mpoint.y_ + GameRand::GetRand(OBJRAND, -2, 2);

            dMap_.x_ = mpoint.x_ + (GameRand::GetRand(OBJRAND, 0, 100) > 50 ? 1 : -1) * GameRand::GetRand(OBJRAND, 3, 5);
            dMap_.y_ = mpoint.y_ + (GameRand::GetRand(OBJRAND, 0, 100) > 50 ? 1 : -1) * GameRand::GetRand(OBJRAND, 3, 5);
        }
        else
        {
            const IntRect& wbounds = World2D::GetWorldBounds();
            dMap_.x_ = GameRand::GetRand(OBJRAND, wbounds.left_, wbounds.right_);
            dMap_.y_ = GameRand::GetRand(OBJRAND, wbounds.top_, wbounds.bottom_);
        }
//        URHO3D_LOGINFOF("GOC_Portal() - HandleAppear : nodeID=%u undefined destination => randomize dMap=%s", node_->GetID(), dMap_.ToString().CString());
    }
}

void GOC_Portal::HandleBeginContact(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsBeginContact2D;

    teleportedInfos_.Clear();
    dViewports_.Clear();
    Map* destinationMap = 0;

    Node* entity = 0;
    {
        const ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[P_CONTACTINFO].GetUInt());
        if (cinfo.shapeA_->GetViewZ() != cinfo.shapeB_->GetViewZ())
            return;

        RigidBody2D* b1 = cinfo.bodyA_;
        RigidBody2D* b2 = cinfo.bodyB_;
        entity = (b1 && b1 != node_->GetComponent<RigidBody2D>()) ? b1->GetNode() : (b2 ? b2->GetNode() : 0);
    }

    if (!entity)
        return;

    // Only player in the entrance direction can active the portal
    GOC_Controller* control = entity->GetDerivedComponent<GOC_Controller>();
    if (control && Sign(control->control_.direction_) != Sign(node_->GetVar(GOA::DIRECTION).GetVector2().x_))
    {
        URHO3D_LOGINFOF("GOC_Portal() - HandleBeginContact : nodeID=%u nodeInContact=%s(%u) controllertype=%d...",
                            node_->GetID(), entity->GetName().CString(), entity->GetID(), control->GetControllerType());

        if (control->GetControllerType() != GO_Player && control->GetControllerType() != GO_AI_Ally)
            return;

        int viewport = 0;

        if (control->GetControllerType() == GO_AI_Ally)
        {
            // Find a Player on this entity
            PODVector<Node* > children;
            entity->GetChildrenWithComponent<GOC_Destroyer>(children, true);
            bool hasAlocalMountedPlayer = false;
            for (PODVector<Node* >::Iterator it=children.Begin(); it!=children.End(); ++it)
            {
                GOC_Controller* othercontroller = (*it)->GetDerivedComponent<GOC_Controller>();
                if (othercontroller && othercontroller->IsMainController() && othercontroller->GetControllerType() == GO_Player)
                {
                    hasAlocalMountedPlayer = true;
                    viewport = othercontroller->GetThinker()->GetControlID();
                }
            }

            if (!hasAlocalMountedPlayer)
                return;
        }
        else
            viewport = control->GetThinker() ? control->GetThinker()->GetControlID() : 0;

        URHO3D_LOGINFOF("GOC_Portal() - HandleBeginContact : nodeID=%u nodeInContact=%s(%u) control=%u portaldir=%f controllerdir=%f portal at %s... Try to Get the Destination Map=%s ...",
                            node_->GetID(), entity->GetName().CString(), entity->GetID(), control, node_->GetVar(GOA::DIRECTION).GetVector2().x_, control ? control->control_.direction_ : 0.f,
                            GetComponent<GOC_Destroyer>()->GetWorldMapPosition().ToString().CString(), dMap_.ToString().CString());

        dViewports_.Push(viewport);

        // Require the destination map
        if (World2D::GetCurrentMap(viewport)->GetMapPoint() != ShortIntVector2(dMap_.x_, dMap_.y_))
        {
            destinationMap = World2D::GetMapAt(ShortIntVector2(dMap_.x_, dMap_.y_), true);
            if (!destinationMap)
            {
                URHO3D_LOGERRORF("GOC_Portal() - HandleBeginContact : nodeID=%u no map at destination %s", node_->GetID(), dMap_.ToString().CString());
                return;
            }
        }
        else
        {
            destinationMap = World2D::GetCurrentMap(viewport);
        }

        if (destinationMap)
        {
            URHO3D_LOGERRORF("GOC_Portal() - HandleBeginContact : Get Destination Map=%s status=%s ...", destinationMap->GetMapPoint().ToString().CString(), mapStatusNames[destinationMap->GetStatus()]);

            // Stop the entity
            control->ResetButtons();
            entity->GetComponent<GOC_Collide2D>()->ClearContacts();
            entity->GetComponent<GOC_Move2D>()->StopMove();
        }
        else
        {
            URHO3D_LOGERRORF("GOC_Portal() - HandleBeginContact : nodeID=%u no map at destination %s", node_->GetID(), dMap_.ToString().CString());
            return;
        }
    }
    else
        return;

    // Active Animation Use
    GetComponent<AnimatedSprite2D>()->SetAnimation("use");

    // Stop children (mounted ally)
    PODVector<GOC_Controller*> controllers;
    {
        PODVector<GOC_Controller*> tempcontrollers;
        entity->GetDerivedComponents<GOC_Controller>(tempcontrollers, true);
        for (PODVector<GOC_Controller*>::Iterator it=tempcontrollers.Begin(); it!=tempcontrollers.End(); ++it)
        {
            GOC_Controller* controller = *it;
            int clientid = GameContext::Get().ClientMode_ ? GameNetwork::Get()->GetClientID() : 0;
            if (controller->GetNode()->GetVar(GOA::CLIENTID).GetInt() == clientid)
            {
                // remove constraint by stopping behavior before Transfer
                controller->Stop();

                // store the viewport of this controller if different
                if (controller->GetThinker())
                {
                    int viewport = controller->GetThinker()->GetControlID();
                    if (!dViewports_.Contains(viewport))
                        dViewports_.Push(viewport);
                }

                controllers.Push(controller);
            }

            // unmount children
            if (controller != control)
                controller->Unmount();
        }
    }

    for (unsigned i=0; i < dViewports_.Size(); i++)
    {
        // Stop the focus in this viewport
        DrawableScroller::Pause(dViewports_[i], true);
//        ViewManager::Get()->SetFocusEnable(false, dViewports_[i]);
    }

    // Transfer entity & children
    for (PODVector<GOC_Controller*>::Iterator it=controllers.Begin(); it!=controllers.End(); ++it)
    {
        Teleport((*it)->GetNode());
    }

//    URHO3D_LOGINFOF("GOC_Portal() - HandleBeginContact : nodeID=%u desactived", node_->GetID());
    Desactive();

    // transfer bodies when ready
    transferOk_ = !destinationMap || destinationMap->GetStatus() == Available;
    if (!transferOk_)
    {
        numTransferPeriods_ = 0;
        TimerRemover* timer = TimerRemover::Get();
        timer->SetSendEvents(this, 0, GO_ENDTIMER);
        timer->Start(node_, 1.f, NOREMOVESTATE);
        SubscribeToEvent(this, GO_ENDTIMER, URHO3D_HANDLER(GOC_Portal, HandleApplyDestination));

        World2D::SetAllowClearMaps(false);
    }

    SubscribeToEvent(GetScene(), E_SCENEUPDATE, URHO3D_HANDLER(GOC_Portal, HandleApplyDestination));
}

void GOC_Portal::Desactive()
{
//    URHO3D_LOGINFOF("GOC_Portal() - Desactive : nodeID=%u desactived for %f seconds", node_->GetID(), PORTALDESACTIVEDELAY);

    // Disable GOC_Portal
    SetEnabled(false);

    // Subscribe to Reactive Portal Event
    TimerRemover* reactivePortalTimer = TimerRemover::Get();
    reactivePortalTimer->SetSendEvents(this, 0, PORTAL_REACTIVE);
    reactivePortalTimer->Start(node_, PORTALDESACTIVEDELAY, NOREMOVESTATE);

//    URHO3D_LOGINFOF("GOC_Portal() - Desactive : nodeID=%u desactived => subscribe to PORTAL_REACTIVE for reactivation in %f seconds !", node_->GetID(), PORTALDESACTIVEDELAY);

    SubscribeToEvent(this, PORTAL_REACTIVE, URHO3D_HANDLER(GOC_Portal, HandleReactivePortal));
}

void GOC_Portal::HandleReactivePortal(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, PORTAL_REACTIVE);

    // reactive only if not restore in objectpool
    if (node_->isPoolNode_ && node_->isInPool_)
//    if (!node_->HasTag("InUse"))
    {
//        URHO3D_LOGERRORF("GOC_Portal() - HandleReactivePortal : nodeID=%u is in pool", node_->GetID());
        return;
    }

//    URHO3D_LOGINFOF("GOC_Portal() - HandleReactivePortal : nodeID=%u", node_->GetID());

    // enable GOC_Portal
    SetEnabled(true);
}

void GOC_Portal::HandleApplyDestination(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GO_ENDTIMER)
    {
        UnsubscribeFromEvent(this, GO_ENDTIMER);

        ShortIntVector2 mpoint(dMap_.x_, dMap_.y_);

        if (teleportedInfos_.Size())
        {
            Map* map = World2D::GetMapAt(mpoint);
            transferOk_ = (map && map->GetStatus() == Available);
        }
        else
        {
            transferOk_ = true;
        }

        if (!transferOk_)
        {
            numTransferPeriods_++;
            if (numTransferPeriods_ < MAX_NUMPORTALTRANSFERPERIODS)
            {
                TimerRemover* timer = TimerRemover::Get();
                timer->SetSendEvents(this, 0, GO_ENDTIMER);
                timer->Start(node_, 1.f, NOREMOVESTATE);
                SubscribeToEvent(this, GO_ENDTIMER, URHO3D_HANDLER(GOC_Portal, HandleApplyDestination));
                URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : no available Map=%s ... wait again !", mpoint.ToString().CString());
            }
            else
            {
                URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : no available Map=%s ... transfer period finished => break and repop !", mpoint.ToString().CString());

                for (unsigned i=0; i < teleportedInfos_.Size(); ++i)
                {
                    unsigned nodeid = teleportedInfos_[i].nodeid_;
                    Node* node = nodeid ? GameContext::Get().rootScene_->GetNode(nodeid) : 0;

                    if (!node)
                    {
                        URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : can't repop nodeid=%u !", nodeid);
                        continue;
                    }

                    node->SetEnabled(true);
                    URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : repop node=%s(%u) enabled=%s !", node->GetName().CString(), node->GetID(), node->IsEnabled()?"true":"false");
                }

                teleportedInfos_.Clear();
                transferOk_ = true;
            }
        }
        else
        {
            URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : destination Map=%s OK !", mpoint.ToString().CString());
        }

        if (transferOk_)
        {
            eventType = E_SCENEUPDATE;
            World2D::SetAllowClearMaps(true);
        }
    }

    if (eventType == E_SCENEUPDATE && transferOk_)
    {
        UnsubscribeFromEvent(GetScene(), E_SCENEUPDATE);
        transferOk_ = false;

        if (teleportedInfos_.Size())
        {
            ShortIntVector2 mpoint(dMap_.x_, dMap_.y_);
            Map* map = World2D::GetMapAt(mpoint);
            if (!map)
            {
                URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : Error No destination Map=%s !", mpoint.ToString().CString());

                for (unsigned i=0; i < teleportedInfos_.Size(); ++i)
                {
                    unsigned nodeid = teleportedInfos_[i].nodeid_;
                    Node* node = nodeid ? GameContext::Get().rootScene_->GetNode(nodeid) : 0;

                    if (!node)
                    {
                        URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : can't repop nodeid=%u !", nodeid);
                        continue;
                    }

                    node->SetEnabled(true);

                    URHO3D_LOGERRORF("GOC_Portal() - HandleApplyDestination : repop node=%s(%u) enabled=%s !", node->GetName().CString(), node->GetID(), node->IsEnabled()?"true":"false");
                }

                teleportedInfos_.Clear();
                return;
            }

            // Set Destination
            {
                IntVector2 newposition;
                if (dViewZ_ == NOVIEW)
                    dViewZ_ = ViewManager::Get()->GetCurrentViewZ(dViewports_.Size() ? dViewports_[0] : 0);

                Node* destinationArea = GameContext::Get().FindMapPositionAt("GOT_Portal", mpoint, newposition, dViewZ_, node_);

                if (!destinationArea || newposition.x_ >= MapInfo::info.width_ || newposition.y_ >= MapInfo::info.height_)
                {
                    // Reset to Default Point
                    newposition.x_ = 1;
                    newposition.y_ = 2;
                    if (dViewZ_ != INNERVIEW)
                        dViewZ_ = FRONTVIEW;
                }
                else
                {
                    if (destinationArea->GetName() == "GOT_Portal")
                    {
                        // never pop on a portal
                        newposition.x_+= (int)destinationArea->GetVar(GOA::DIRECTION).GetVector2().x_;
                        // ajust position due to bottom alignement for portal (prevent to collide inside wall for avatar)
                        newposition.y_-= 1;
                        SetDestinationPosition(newposition);

                        // set returned destination for this new portal
                        const WorldMapPosition& mapposition = GetComponent<GOC_Destroyer>()->GetWorldMapPosition();
                        GOC_Portal* newportal = destinationArea->GetComponent<GOC_Portal>();

                        if (newportal != this)
                        {
                            newportal->SetDestinationMap(mapposition.mPoint_);
                            // never pop in portal and ajust y position due to bottom alignement for portal (prevent to collide inside wall for avatar)
                            newportal->SetDestinationPosition(mapposition.mPosition_ + IntVector2((int)node_->GetVar(GOA::DIRECTION).GetVector2().x_, -1));
                            newportal->SetDestinationViewZ(mapposition.viewZ_);
                            URHO3D_LOGINFOF("GOC_Portal() - HandleApplyDestination : current portal nodeID=%u => destination portal nodeID=%u desactived for 10sec",
                                            node_->GetID(), destinationArea->GetID());
                            newportal->Desactive();
                            newportal->GetComponent<AnimatedSprite2D>()->SetAnimation("inactive");
                        }
                    }
                }

                dPosition_ = newposition;
            }

            // Subscribe to Transfer Bodies
            TimerRemover* transferCameraTimer = TimerRemover::Get();
            transferCameraTimer->SetSendEvents(this, 0, PORTAL_CAMERA);
            transferCameraTimer->Start(node_, PORTALCAMERADELAY, NOREMOVESTATE);
            SubscribeToEvent(this, PORTAL_CAMERA, URHO3D_HANDLER(GOC_Portal, HandleTransferBodies));

            // Start Update destination zone
            // never switch view z here
            if (!IsDestinationDefined())
            {
                URHO3D_LOGINFOF("GOC_Portal() - HandleApplyDestination : to map=%s centeredposition", dMap_.ToString().CString());

                for (unsigned i=0; i < dViewports_.Size(); i++)
                    World2D::GetWorld()->GoToMap(mpoint, dViewports_[i]);
            }
            else
            {
                URHO3D_LOGINFOF("GOC_Portal() - HandleApplyDestination : to map=%s position=%s viewZ=%d",
                                dMap_.ToString().CString(), dPosition_.ToString().CString(), dViewZ_);

                for (unsigned i=0; i < dViewports_.Size(); i++)
                    World2D::GetWorld()->GoToMap(mpoint, dPosition_, -1, dViewports_[i]);
            }

            for (unsigned i=0; i < teleportedInfos_.Size(); i++)
            {
                teleportedInfos_[i].map_ = dMap_;
                teleportedInfos_[i].position_ = dPosition_;
                teleportedInfos_[i].viewZ_ = dViewZ_;
            }

            World2D::GetWorld()->UpdateStep();
        }
    }
}

void GOC_Portal::HandleTransferBodies(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(this, PORTAL_CAMERA);

    URHO3D_LOGINFOF("GOC_Portal() - HandleTransferBodies : portal at %s => to destination=%d %d position=%s viewZ=%d",
                    GetComponent<GOC_Destroyer>()->GetWorldMapPosition().ToString().CString(), dMap_.x_, dMap_.y_, dPosition_.ToString().CString(), dViewZ_);

    if (dViewports_.Size())
    {
        for (unsigned i=0; i < dViewports_.Size(); i++)
            ViewManager::Get()->SwitchToViewZ(dViewZ_, 0, dViewports_[i]);

        for (unsigned i=0; i < dViewports_.Size(); i++)
            World2D::GetWorld()->GoCameraToDestinationMap(dViewports_[i]);

        for (unsigned i=0; i < dViewports_.Size(); i++)
            DrawableScroller::Pause(dViewports_[i], false);
    }

    if (teleportedInfos_.Size())
    {
        for (unsigned i=0; i < teleportedInfos_.Size(); ++i)
        {
            const TeleportInfo& info = teleportedInfos_[i];
            Node* node = info.nodeid_ ? GameContext::Get().rootScene_->GetNode(info.nodeid_) : 0;
            if (!node)
                continue;

            WorldMapPosition position(World2D::GetWorldInfo(), info.map_, info.position_, info.viewZ_);

            URHO3D_LOGINFOF("GOC_Portal() - HandleTransferBodies : nodeID=%u nodeTeleported=%s(%u) ...",
                            node_->GetID(), node->GetName().CString(), node->GetID());

            // 13/09/2020 : must be enable before use SetWorlMapPosition, otherwise the entity disappears sometime.
            node->SetEnabled(true);
            node->GetComponent<GOC_Destroyer>()->SetWorldMapPosition(position);

            URHO3D_LOGINFOF("GOC_Portal() - HandleTransferBodies : nodeID=%u nodeTeleported=%s(%u) enabled=%s to %s map=%s tile=%s viewZ=%d !",
                            node_->GetID(), node->GetName().CString(), node->GetID(), node->IsEnabled()?"true":"false", node->GetWorldPosition2D().ToString().CString(),
                            info.map_.ToString().CString(), info.position_.ToString().CString(), info.viewZ_);

            // subscribe to enable timer (after teleported)
//            TimerRemover::Get()->Start(node, 0.2f + ((float)i)*0.2f, ENABLE, PORTALCAMERADELAY);
        }

        teleportedInfos_.Clear();
    }

    URHO3D_LOGINFOF("GOC_Portal() - HandleTransferBodies ... OK !");
}
