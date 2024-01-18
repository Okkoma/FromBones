#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Light.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>

#include "DefsColliders.h"
#include "DefsFluids.h"
#include "DefsViews.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameNetwork.h"
#include "GameHelpers.h"

#include "GameOptionsTest.h"

#include "GOC_Animator2D.h"
#include "GOC_Life.h"
#include "GOC_Controller.h"
#include "GOC_ControllerAI.h"
#include "Actor.h"
#include "Player.h"

#include "ViewManager.h"
#include "Map.h"
#include "ObjectMaped.h"
#include "MapWorld.h"

#include "GOC_Destroyer.h"

//#define DUMP_UNSTUCKWARNING

extern const char* RemoveStateNames[];

const float TimeLockViewDelay = 0.5f;

bool GOC_Destroyer::useWorld2D_ = true;

static bool stilePositionUpdated_[2];
static bool sChangeMap_, sInsideBounds_, sWaitForMap_, moveEntityData_;
static int sViewZ_;
static WorldMapPosition sMPosition_;
static unsigned sNewHashPoint_, sOldHashPoint_;

GOC_Destroyer::GOC_Destroyer(Context* context) :
    Component(context),
    lifeTime_(0.f),
    elapsedTime_(0.f),
    buoyancy_(0.f),
    activeTimer_(false),
    viewZDirty_(false),
    destroyMode_(FREEMEMORY),
    viewZToCheck_(NOVIEW),
    body_(0),
    currentMap_(0),
    currentCell_(0),
    worldUpdatePosition_(true),
    lifeNotifier_(true),
    checkUnstuck_(true),
    allowWallSpawning_(false)
{
    enabled_ = false;

    body_ = GetComponent<RigidBody2D>();
    switchViewEnable_ = body_ ? body_->GetBodyType() != BT_STATIC && !node_->GetComponent<ObjectMaped>() && worldUpdatePosition_ : false;
    cellCheckFeature_[0] = cellCheckFeature_[1] = MapFeatureType::NoMapFeature;
    lastCellCheckFeature_[0] = lastCellCheckFeature_[1] = MapFeatureType::NoMapFeature;
}

GOC_Destroyer::~GOC_Destroyer()
{
//    URHO3D_LOGINFOF("~GOC_Destroyer() - Node=%s(%u)", node_->GetName().CString(), node_->GetID());
}

void GOC_Destroyer::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Destroyer>();

    URHO3D_ACCESSOR_ATTRIBUTE("Life Time", GetLifeTime, SetLifeTime, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Destroy Mode", GetDestroyMode, SetDestroyMode, int, 0, AM_FILE);
    URHO3D_ATTRIBUTE("World Position Update", bool, worldUpdatePosition_, true, AM_FILE);
    URHO3D_ATTRIBUTE("Unstuck", bool, checkUnstuck_, true, AM_FILE);
    URHO3D_ATTRIBUTE("Wall Spawning", bool, allowWallSpawning_, false, AM_FILE);
    URHO3D_ATTRIBUTE("Life Notifier", bool, lifeNotifier_, true, AM_FILE);
    URHO3D_ATTRIBUTE("Buoyancy", float, buoyancy_, 0.f, AM_FILE);

    URHO3D_LOGINFOF("GOC_Destroyer() - RegisterObject : hash=%u ... OK !", GOC_Destroyer::GetTypeStatic().Value());
}

void GOC_Destroyer::Reset(bool toActive)
{
//    URHO3D_LOGERRORF("GOC_Destroyer() - Reset : %s(%u) toActive=%s enabled=%s nodeEnabled=%s ... ",
//                                node_->GetName().CString(), node_->GetID(), toActive ? "true" : "false", enabled_ ? "true" : "false", node_->IsEnabled() ? "true" : "false");

    if (!body_)
        body_ = node_ ? node_->GetComponent<RigidBody2D>(true) : GetComponent<RigidBody2D>();

    elapsedTime_ = 0.f;
    activeTimer_ = false;
    switchViewEnable_ = body_ ? body_->GetBodyType() != BT_STATIC && !node_->GetComponent<ObjectMaped>() && worldUpdatePosition_ : false;
    viewZDirty_ = false;
    hasWallInFront_[0] = hasWallInFront_[1] = false;

    viewZToCheck_ = NOVIEW;
    currentMap_ = 0;
    currentCell_ = lastcurrentCell_[0] = lastcurrentCell_[1] = 0;
    lastObjectMaped_ = 0;

    cellCheckFeature_[0] = cellCheckFeature_[1] = MapFeatureType::NoMapFeature;
    lastCellCheckFeature_[0] = lastCellCheckFeature_[1] = MapFeatureType::NoMapFeature;

    // inactive position
    mapWorldPosition_.defined_ = false;

    if (toActive)
    {
        node_->RemoveVar(GOA::DESTROYING);
        if (!enabled_)
        {
            SubscribeToEvent(node_, WORLD_ENTITYCREATE, URHO3D_HANDLER(GOC_Destroyer, OnWorldEntityCreate));
            enabled_ = true;
            /// dont remove this : avoid entities to disappear when map reloading
            OnSetEnabled();
        }

        if (node_->IsEnabled())
        {
            // force recalculate batches (prevent clipping during reapparition)
            Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
            if (drawable)
            {
                drawable->SetEnabled(true);
                drawable->ForceUpdateBatches();
            }
        }
    }
    else
    {
        enabled_ = false;

        node_->SetEnabled(false);

        if (body_)
            body_->SetAwake(false);

        ResetViewZ();
        UnsubscribeFromAllEvents();

//        URHO3D_LOGERRORF("GOC_Destroyer() - Reset : %s(%u) toActive=%s enabled=%s nodeEnabled=%s ... ",
//                        node_->GetName().CString(), node_->GetID(), toActive ? "true" : "false", enabled_ ? "true" : "false", node_->IsEnabled() ? "true" : "false");
    }
}

void GOC_Destroyer::ResetViewZ()
{
    mapWorldPosition_.viewZ_ = 0;
    mapWorldPosition_.viewZIndex_ = NOVIEW;
    node_->SetVar(GOA::ONVIEWZ, FRONTVIEW);
}


void GOC_Destroyer::SetLifeTime(float delay)
{
    if (lifeTime_ == delay)
        return;

//    URHO3D_LOGINFOF("GOC_Destroyer() - SetLifeTime : %s(%u) time=%f", node_->GetName().CString(), node_->GetID(), delay);
    lifeTime_ = delay;
    elapsedTime_ = lifeTime_;
}

void GOC_Destroyer::SetEnableLifeTimer(bool enable)
{
    activeTimer_ = enable;

    if (activeTimer_)
    {
        elapsedTime_ = lifeTime_;
//        URHO3D_LOGINFOF("GOC_Destroyer() - SetEnableLifeTimer : %s(%u) enable_=%s activeTimer_=%s lifetime=%f ... OK !",
//                    node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", activeTimer_ ? "true" : "false", lifeTime_);
    }

    CheckLifeTime();
}

void GOC_Destroyer::SetEnableLifeNotifier(bool enable)
{
    lifeNotifier_ = enable;
}

void GOC_Destroyer::SetEnableUnstuck(bool enable)
{
    URHO3D_LOGINFOF("GOC_Destroyer() - SetEnableUnstuck : %s(%u) checkUnstuck_=%s !", node_->GetName().CString(), node_->GetID(), enable?"true":"false");
    checkUnstuck_ = enable;
}

void GOC_Destroyer::SetEnablePositionUpdate(bool enable)
{
    worldUpdatePosition_ = enable;
}

void GOC_Destroyer::CheckLifeTime()
{
    if (activeTimer_ && elapsedTime_ && enabled_)
    {
//        URHO3D_LOGINFOF("GOC_Destroyer() - CheckLifeTime : %s(%u) elapsedTime_=%f activeTimer !", node_->GetName().CString(), node_->GetID(), elapsedTime_);
        SubscribeToEvent(GetScene(), E_SCENEUPDATE, URHO3D_HANDLER(GOC_Destroyer, HandleUpdateTime));
    }
    else
        UnsubscribeFromEvent(GetScene(), E_SCENEUPDATE);
}

void GOC_Destroyer::SetDestroyMode(int state)
{
    if (destroyMode_ != (RemoveState)state)
    {
//        URHO3D_LOGINFOF("GOC_Destroyer() - SetDestroyMode : %s(%u) destroyMode=%s !", node_->GetName().CString(), node_->GetID(), RemoveStateNames[state]);
        destroyMode_ = (RemoveState)state;
    }
}

void GOC_Destroyer::SetWorldMapPosition(const WorldMapPosition& wmPosition)
{
    if (wmPosition.drawOrder_ != -1)
        mapWorldPosition_.drawOrder_ = wmPosition.drawOrder_;

    if (wmPosition.viewZIndex_ != -1)
        node_->SetVar(GOA::ONVIEWZ, wmPosition.viewZ_);

//    URHO3D_LOGINFOF("GOC_Destroyer() - SetWorldMapPosition : %s(%u) initialposition=%s to %s !",
//                    node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString(), wmPosition.ToString().CString());

    // wmPosition.position_ is the world mass center
    // body is setted in node position
    //body_->SetWorldTransform(wmPosition.position_ - body_->GetMassCenter() * node_->GetWorldScale2D(), node_->GetWorldRotation2D());
    Vector2 nodeposition = wmPosition.position_ - body_->GetMassCenter() * node_->GetWorldScale2D();
    float angle = node_->GetWorldRotation2D();
    node_->SetWorldPosition2D(nodeposition);
    // Apply position to physic too
    body_->SetBodyPosition(nodeposition, angle);

    SetViewZ(wmPosition.viewZ_);

    UpdatePositions(CHANGEMAP_FORCE);

    if (GameContext::Get().gameConfig_.enlightScene_)
        bool lightstate = GameHelpers::SetLightActivation(node_);

    // force recalculate batches (prevent clipping during reapparition)
    Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
    if (drawable)
        drawable->ForceUpdateBatches();
    else
        URHO3D_LOGERRORF("GOC_Destroyer() - SetWorldMapPosition : %s(%u) position=%s ERROR DRAWABLE !",
                         node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString());

//    URHO3D_LOGINFOF("GOC_Destroyer() - SetWorldMapPosition : %s(%u) position=%s !", node_->GetName().CString(), node_->GetID(), mapWorldPosition_.position_.ToString().CString());
}

void GOC_Destroyer::SetViewZ(int viewZ, unsigned viewMask, int drawOrder)
{
    if (!viewZ || viewZ == NOVIEW)
        viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();
    else
        mapWorldPosition_.viewZIndex_ = ViewManager::GetViewZIndex(viewZ);

    if (!viewZ || viewZ == NOVIEW)
    {
        URHO3D_LOGERRORF("GOC_Destroyer() - SetViewZ : node=%s(%u) NoViewZ Setted !", node_->GetName().CString(), node_->GetID());
        return;
    }

    int viewport = 0;
    GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
    bool isPlayer = (controller && controller->GetThinker() && controller->GetThinker()->IsInstanceOf<Player>());
//    bool isCurrentViewZ = viewZ == ViewManager::Get()->GetCurrentViewZ(viewport);

    // If Multiviewport, check the good viewport
    if (isPlayer && ViewManager::Get()->GetNumViewports() > 1)
    {
        viewport = Min(controller->GetThinker() ? controller->GetThinker()->GetControlID() : 0, (int)ViewManager::Get()->GetNumViewports()-1);
//        isCurrentViewZ = viewZ == ViewManager::Get()->GetCurrentViewZ(viewport);

        // Update the light viewmask
        Light* light = node_->GetComponent<Light>();
        if (light)
            light->SetViewMask((VIEWPORTSCROLLER_OUTSIDE_MASK << viewport) | (VIEWPORTSCROLLER_INSIDE_MASK << viewport));
    }

    unsigned numdrawables = 0;
    static PODVector<Drawable2D*> sDrawables;
    Drawable2D** drawables;
    GOC_Animator2D* animator = node_->GetComponent<GOC_Animator2D>();
    if (animator)
    {
        // Useful for Setting the Viewmask, layer for RenderTarget AnimatedSprite
        // So Keep it.
        numdrawables = animator->GetDrawables().Size();
        drawables = numdrawables ? (Drawable2D**)&animator->GetDrawables()[0] : 0;
    }
    else
    {
        node_->GetDerivedComponents<Drawable2D>(sDrawables, true);
        numdrawables = sDrawables.Size();
        drawables = &sDrawables[0];
    }

    if (numdrawables)
    {
        // Set ViewMask
        if (viewMask == 0)
            viewMask = ViewManager::GetLayerMask(viewZ);
        if (viewMask != 0)
            mapWorldPosition_.viewMask_ = viewMask;

        // Set Layer and Order
        IntVector2 layer(viewZ, -1);
        if (viewZ != THRESHOLDVIEW)
        {
            const int layermodifier = node_->GetVar(GOA::PLATEFORM).GetBool() ? LAYER_PLATEFORM : LAYER_ACTOR;
            // layer for INNERVIEW / FRONTVIEW
            layer.x_ += layermodifier + node_->GetVar(GOA::LAYERALIGNMENT).GetInt();
            // layer for BACKACTORVIEW ONLY
            layer.y_ = viewZ > BACKVIEW && viewZ < OUTERVIEW ? INNERVIEW : BACKACTORVIEW;
        }

        if (drawOrder != -1)
            mapWorldPosition_.drawOrder_ = drawOrder;
        else
            drawOrder = mapWorldPosition_.drawOrder_;

        for (unsigned i=0; i < numdrawables; i++)
        {
            Drawable2D* drawable = drawables[i];

            if (viewMask)
                drawable->SetViewMask(viewMask);
            drawable->SetLayer2(layer);
            //drawable->SetOrderInLayer(drawOrder+i);
        }
    }

    if (currentMap_ && mapWorldPosition_.viewZIndex_ == -1)
        mapWorldPosition_.viewZIndex_ = ViewManager::GetViewZIndex(ViewManager::GetNearViewZ(viewZ));

    UpdateFilterBits(viewZ);

    mapWorldPosition_.viewZ_ = viewZ;
    node_->SetVar(GOA::ONVIEWZ, viewZ);

    // Force setting the ViewZ to children entities (like mounted entity)
    PODVector<Node* > children;
    node_->GetChildrenWithComponent<GOC_Destroyer>(children, true);
    for (PODVector<Node* >::Iterator it=children.Begin(); it!=children.End(); ++it)
    {
        Node* node = *it;

//        URHO3D_LOGINFOF("GOC_Destroyer() - SetViewZ : children node=%s(%u) ...", node->GetName().CString(), node->GetID());

        int otherviewport = viewport;
        GOC_Controller* othercontroller = node->GetDerivedComponent<GOC_Controller>();
        if (othercontroller && othercontroller->GetThinker()->IsInstanceOf<Player>())
            otherviewport = Min(othercontroller->GetThinker()->GetControlID(), (int)ViewManager::Get()->GetNumViewports()-1);

        if (otherviewport != viewport)
            ViewManager::Get()->SwitchToViewZ(viewZ, node, otherviewport);
        else
            node->GetComponent<GOC_Destroyer>()->SetViewZ(viewZ);
    }

    if (isPlayer)
        URHO3D_LOGERRORF("GOC_Destroyer() - SetViewZ : node=%s(%u) viewport=%d viewZ=%d(index=%d) orderinlayer=%d viewMask=%u numdrawables=%u firstdrawable=%u layers=%s",
                         node_->GetName().CString(), node_->GetID(), viewport, mapWorldPosition_.viewZ_, mapWorldPosition_.viewZIndex_, drawOrder, viewMask,
                         numdrawables, numdrawables ? drawables[0]->GetID() : 0, drawables[0]->GetLayer2().ToString().CString());
}

void GOC_Destroyer::UpdateFilterBits(int viewZ)
{
    if (!viewZ || viewZ == NOVIEW)
        viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();

    if (!viewZ || viewZ == NOVIEW)
        URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) NoViewZ Setted !", node_->GetName().CString(), node_->GetID());

    /// Just for test for mobile castle
    if (GetComponent<ObjectMaped>())
    {
        URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) mobile castle !", node_->GetName().CString(), node_->GetID());
        return;
    }

    PODVector<CollisionShape2D*> shapes;
    node_->GetDerivedComponents(shapes, true);

    int type = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
    bool isdead = node_->GetVar(GOA::ISDEAD).GetBool();

    if (node_->GetVar(GOA::ISMOUNTEDON).GetUInt())
    {
        for (unsigned i = 0; i < shapes.Size(); i++)
        {
            if (!shapes[i]->IsTrigger())
                shapes[i]->SetFilterBits(CC_TRIGGER, CC_TRIGGER);
        }

        return;
    }

    bool isplateform = node_->GetVar(GOA::PLATEFORM).GetBool();
    bool isbullet = node_->GetVar(GOA::BULLET).GetBool();

    bool staticBody = (body_ && body_->GetBodyType() == BT_STATIC);

    for (unsigned i = 0; i < shapes.Size(); i++)
    {
        CollisionShape2D*& shape = shapes[i];

        if (!shape)
            continue;

        shape->SetViewZ(viewZ);

        if (shape->IsTrigger())
            continue;

        if (shape->GetCategoryBits() == CC_SCENEUI)
            continue;

        if (isbullet)
        {
            if (viewZ == INNERVIEW)
                shape->SetFilterBits(CC_INSIDEPROJECTILE, CM_INSIDEPROJECTILE);
            else if (viewZ == FRONTVIEW)
                shape->SetFilterBits(CC_OUTSIDEPROJECTILE, CM_OUTSIDEPROJECTILE);
            else
                shape->SetFilterBits(CC_ALLPROJECTILES, CM_ALLPROJECTILES);

//            URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) viewZ=%d shape=%u is bullet !", node_->GetName().CString(), node_->GetID(), viewZ, shape);

            return;
        }

        if (isplateform)
            shape->SetColliderInfo(PLATEFORMCOLLIDER);

        if (viewZ == INNERVIEW)
        {
            switch (type)
            {
            case GO_None :
                if (staticBody && isplateform)
                {
                    URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) Inside Static Object !", node_->GetName().CString(), node_->GetID());
                    shape->SetFilterBits(CC_INSIDESTATICFURNITURE, CM_INSIDESTATICFURNITURE);
//                    shape->SetFilterBits(CC_INSIDESTATICFURNITURE|CC_OUTSIDESTATICFURNITURE, CM_INSIDESTATICFURNITURE|CM_OUTSIDESTATICFURNITURE);
                }
                else
                    shape->SetFilterBits(CC_INSIDEOBJECT, CM_INSIDEOBJECT);
                break;
            case GO_AI_Ally :
            case GO_Player :
            case GO_NetPlayer :
                if (shape->GetMaskBits() == CM_DETECTOR || shape->GetMaskBits() == CM_OUTSIDEAVATAR_DETECTOR || shape->GetMaskBits() == CM_INSIDEAVATAR_DETECTOR)
                    shape->SetFilterBits(CC_INSIDEAVATAR, CM_INSIDEAVATAR_DETECTOR);
                else if (isdead)
                    shape->SetFilterBits(CC_INSIDEAVATAR, CM_DEADINSIDEENTITY);
                else if (shape->GetExtraContactBits() & CONTACT_BODYLESS)
                    shape->SetFilterBits(CC_INSIDEAVATAR, CM_INSIDEBODILESSPART);
                else
                    shape->SetFilterBits(CC_INSIDEAVATAR, CM_INSIDEAVATAR);
                break;
            case GO_AI :
            case GO_AI_None :
            case GO_AI_Enemy :
                if (isdead || (shape->GetExtraContactBits() & CONTACT_WALLONLY))
                    shape->SetFilterBits(CC_INSIDEMONSTER, CM_DEADINSIDEENTITY);
                else if (shape->GetExtraContactBits() & CONTACT_BODYLESS)
                    shape->SetFilterBits(CC_INSIDEMONSTER, CM_INSIDEBODILESSPART);
                else
                    shape->SetFilterBits(CC_INSIDEMONSTER, CM_INSIDEMONSTER);
                break;
            case GO_Furniture :
                URHO3D_LOGERRORF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) Inside Furniture !", node_->GetName().CString(), node_->GetID());
//                shape->SetFilterBits(CC_FURNITURE, CM_INSIDEFURNITURE);
                break;
            }
        }

        else if (viewZ == FRONTVIEW)
        {
            switch (type)
            {
            case GO_None :
                if (staticBody && isplateform)
                    shape->SetFilterBits(CC_OUTSIDESTATICFURNITURE, CM_OUTSIDESTATICFURNITURE);
                else
                    shape->SetFilterBits(CC_OUTSIDEOBJECT, CM_OUTSIDEOBJECT);
                break;
            case GO_AI_Ally :
            case GO_Player :
            case GO_NetPlayer :
                if (shape->GetMaskBits() == CM_DETECTOR || shape->GetMaskBits() == CM_OUTSIDEAVATAR_DETECTOR || shape->GetMaskBits() == CM_INSIDEAVATAR_DETECTOR)
                    shape->SetFilterBits(CC_OUTSIDEAVATAR, CM_OUTSIDEAVATAR_DETECTOR);
                else if (isdead)
                    shape->SetFilterBits(CC_OUTSIDEAVATAR, CM_DEADOUTSIDEENTITY);
                else if (shape->GetExtraContactBits() & CONTACT_BODYLESS)
                    shape->SetFilterBits(CC_OUTSIDEAVATAR, CM_OUTSIDEBODILESSPART);
                else
                    shape->SetFilterBits(CC_OUTSIDEAVATAR, CM_OUTSIDEAVATAR);
                break;
            case GO_AI :
            case GO_AI_None :
            case GO_AI_Enemy :
                if (isdead || (shape->GetExtraContactBits() & CONTACT_WALLONLY))
                    shape->SetFilterBits(CC_OUTSIDEMONSTER, CM_DEADOUTSIDEENTITY);
                else if (shape->GetExtraContactBits() & CONTACT_BODYLESS)
                    shape->SetFilterBits(CC_OUTSIDEMONSTER, CM_OUTSIDEBODILESSPART);
                else
                    shape->SetFilterBits(CC_OUTSIDEMONSTER, CM_OUTSIDEMONSTER);
                break;
            case GO_Furniture :
                URHO3D_LOGERRORF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) Outside Furniture !", node_->GetName().CString(), node_->GetID());
//                shape->SetFilterBits(CC_FURNITURE, CM_OUTSIDEFURNITURE);
                break;
            }
        }

        else if (viewZ < BACKVIEW)
        {
//            URHO3D_LOGERRORF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) viewZ=%d CC_NONE !", node_->GetName().CString(), node_->GetID(), viewZ);
            shape->SetFilterBits(CC_NONE, CM_NONE);
        }
        else
        {
//            URHO3D_LOGERRORF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) CC_ALLWALLS !", node_->GetName().CString(), node_->GetID());
            shape->SetFilterBits(CC_ALLWALLS, CM_ALLWALL);
        }
    }

//    URHO3D_LOGINFOF("GOC_Destroyer() - UpdateFilterBits : node=%s(%u) viewZ=%d controllertype=%d", node_->GetName().CString(), node_->GetID(), viewZ, type);
}


void GOC_Destroyer::ApplyAttributes()
{
//    URHO3D_LOGINFOF("GOC_Destroyer() - ApplyAttributes : %s(%u) componentEnable=%s nodeEnable=%s ...",
//                 node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", node_->IsEnabled() ? "true" : "false");

    if (!enabled_)
        Reset(true);

    // Client : NetPlayer Activation after Replication
//    if (GameContext::Get().ClientMode_ && IsEnabledEffective())
//    {
//        GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
////        URHO3D_LOGINFOF("GOC_Destroyer() - ApplyAttributes : %s(%u) controller=%u type=%d !", node_->GetName().CString(), node_->GetID(), controller, controller ? controller->GetControllerType() : -1);
//        if (controller)
//            if (!controller->IsMainController())// && controller->GetControllerType()  == GO_Player)
//            {
//                node_->SendEvent(WORLD_ENTITYCREATE);
//                URHO3D_LOGINFOF("GOC_Destroyer() - ApplyAttributes : %s(%u) ClientNet Replication !", node_->GetName().CString(), node_->GetID());
//            }
//    }

//    URHO3D_LOGINFOF("GOC_Destroyer() - ApplyAttributes : %s(%u) enable=%s nodeEnable=%s ... OK !",
//                 node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", node_->IsEnabled() ? "true" : "false");
}

void GOC_Destroyer::OnSetEnabled()
{
    if (!enabled_)
        return;

//    URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : Node=%s(%u) componentEnable=%s nodeEnable=%s world2D=%s ...",
//                    node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", node_->IsEnabled() ? "true" : "false",
//                    useWorld2D_ ? "true" : "false");

    if (!GetScene())
    {
        UnsubscribeFromEvent(E_SCENEUPDATE);
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
        UnsubscribeFromEvent(WORLD_ENTITYCREATE);
        URHO3D_LOGWARNINGF("GOC_Destroyer() - OnSetEnabled : Node=%s(%u) No Scene !", node_->GetName().CString(), node_->GetID());
        return;
    }

    Scene* scene = GetScene();

    CheckLifeTime();

//    if (activeTimer_ && elapsedTime_)
//    {
//        URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : Node=%s(%u) UpdateTime Started !", node_->GetName().CString(), node_->GetID());
//        SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(GOC_Destroyer, HandleUpdateTime));
//    }

    if (!useWorld2D_)
    {
        GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[Go_Appear::GO_ID] = node_->GetID();
        if (node_->GetVar(GOA::TYPECONTROLLER) != Variant::EMPTY)
            eventData[Go_Appear::GO_TYPE] = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
        else
            eventData[Go_Appear::GO_TYPE] = GO_None;
        eventData[Go_Appear::GO_MAINCONTROL] = controller ? controller->IsMainController() : false;
//        URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : Node=%s(%u) Appear (NO World2D!)", node_->GetName().CString(), node_->GetID());

        node_->SendEvent(GO_APPEAR, eventData);
    }
//    else
//        SubscribeToEvent(node_, WORLD_ENTITYCREATE, URHO3D_HANDLER(GOC_Destroyer, OnWorldEntityCreate));

    if (!World2D::GetWorld())
        return;

    if (!body_)
    {
        body_ = node_->GetComponent<RigidBody2D>(true);
        if (body_)
        {
            switchViewEnable_ = body_->GetBodyType() != BT_STATIC && !node_->GetComponent<ObjectMaped>() && worldUpdatePosition_;
//			URHO3D_LOGERRORF("GOC_Destroyer() - OnSetEnabled : node=%s(%u) body=%s(%u) switchViewEnable=%s ...",
//								node_->GetName().CString(), node_->GetID(), body_->GetNode()->GetName().CString(), body_->GetNode()->GetID(), switchViewEnable_ ? "true":"false");
        }
    }

    if (!body_)
        return;

    GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();

    // Active Colliders
    body_->SetEnabled(node_->IsEnabled());
    body_->SetAwake(node_->IsEnabled());

    if (node_->IsEnabled())
    {
        // Force Update CurrentMap
        // If no reset here, CurrentMap is not garantee to be at the good map that cause problems with goc_move2d, los, portal
        currentMap_ = 0;

        // Subscribe to UpdateWorld2D
        if (body_->GetBodyType() != BT_STATIC && worldUpdatePosition_)
        {
//            URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : node=%s(%u) start on World2D at mPoint=%s position=%s viewZ=%d subscribe to HandleUpdateWorld2D !",
//					                 node_->GetName().CString(), node_->GetID(), mapWorldPosition_.mPoint_.ToString().CString(), node_->GetWorldPosition2D().ToString().CString(), mapWorldPosition_.viewZ_);

//            body_->ApplyUserWorldTransform(node_->GetWorldPosition2D() + body_->GetMassCenter(), node_->GetWorldRotation2D());

            node_->RemoveVar(GOA::INFLUID);

            switchViewEnable_ = node_->GetComponent<ObjectMaped>() ? false : true;
            elapsedTimeLockView_ = TimeLockViewDelay;

            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_Destroyer, HandleUpdateWorld2D));

            UpdateShapesRect();
        }
    }
    else
    {
        /// TEST
        // if not main controller => replicate node, keep subscribe to update position
        if (controller && !controller->IsMainController())
        {
            URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : node=%s(%u) !maincontroller => keep update position !",
                            node_->GetName().CString(), node_->GetID(), mapWorldPosition_.mPoint_.ToString().CString());
            return;
        }

//        URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : node=%s(%u) stop on World2D at mPoint=%s",
//                                    node_->GetName().CString(), node_->GetID(), mapWorldPosition_.mPoint_.ToString().CString());

        UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
    }

//    URHO3D_LOGINFOF("GOC_Destroyer() - OnSetEnabled : Node=%s(%u) componentEnable=%s nodeEnable=%s world2D=%s bodyawake=%s ... OK !",
//                    node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", node_->IsEnabled() ? "true" : "false",
//                    useWorld2D_ ? "true" : "false", body_->IsAwake() ? "true" : "false");
}


void GOC_Destroyer::WorldAppearCallBack()
{
//    URHO3D_LOGINFOF("GOC_Destroyer() - WorldAppearCallBack : node=%s(%u) ... SubscribeToEvent WORLD_ENTITYDESTROY", node_->GetName().CString(), node_->GetID());
//    SubscribeToEvent(node_, WORLD_ENTITYDESTROY, URHO3D_HANDLER(GOC_Destroyer, OnWorldEntityDestroy));

    SetEnableLifeTimer(true);

//    if (lifeTime_)
//    {
//        URHO3D_LOGINFOF("GOC_Destroyer() - WorldAppearCallBack : node=%s(%u) Start Timer !", node_->GetName().CString(), node_->GetID());
//        if (!activeTimer_)
//        {
//            activeTimer_ = true;
//            elapsedTime_ = lifeTime_;
//        }
//        SubscribeToEvent(GetScene(), E_SCENEUPDATE, URHO3D_HANDLER(GOC_Destroyer, HandleUpdateTime));
//    }
}

void GOC_Destroyer::OnWorldEntityCreate(StringHash eventType, VariantMap& eventData)
{
    if (!World2D::GetWorld())
        return;

    sViewZ_ = node_->GetVar(GOA::ONVIEWZ) != Variant::EMPTY ? node_->GetVar(GOA::ONVIEWZ).GetInt() : NOVIEW;
    // Assure to have a viewz and not a layerz
//    if (sViewZ_ != NOVIEW)
//        sViewZ_ = ViewManager::Get()->GetNearViewZ(sViewZ_);

    if (body_)
    {
        // always reactive body (GOC_BodyExploder2D case)
        body_->SetEnabled(true);

        if (body_->GetBodyType() == BT_STATIC)
        {
            const Variant& viewZ = node_->GetVar(GOA::ONVIEWZ);
            if (viewZ != Variant::EMPTY)
                SetViewZ(viewZ.GetInt());
            else
                URHO3D_LOGWARNINGF("GOC_Destroyer() - OnWorldEntityCreate : %s(%u) ... No Var GOA::ONVIEWZ setted !", node_->GetName().CString(), node_->GetID());
        }
        else
        {
            SetViewZ(sViewZ_);
        }
    }

    // always reactive drawable (GOC_BodyExploder2D case)
    Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
    if (drawable && !drawable->IsEnabled())
        drawable->SetEnabled(true);

//    URHO3D_LOGERRORF("GOC_Destroyer() - OnWorldEntityCreate : %s(%u) PASS 1 test position=%s nodepos=%s viewZ=%d ... ",
//             node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString(), node_->GetWorldPosition2D().ToString().CString(), node_->GetVar(GOA::ONVIEWZ).GetInt());

    // Set Initial WorldMapPosition
    bool state = UpdatePositions(eventData, CHANGEMAP_NOCHANGE);
    if (!state)
        return;

    // validate position
    mapWorldPosition_.defined_ = true;

    // specific tileindex (for static furniture)
    if (eventData[Go_Appear::GO_TILE] != Variant::EMPTY)
        mapWorldPosition_.tileIndex_ = eventData[Go_Appear::GO_TILE].GetUInt();

    // already specified mappoint (for static furniture)
    unsigned mPoint = mapWorldPosition_.mPoint_.ToHash();
    if (eventData[Go_Appear::GO_MAP] != Variant::EMPTY)
    {
        mPoint = eventData[Go_Appear::GO_MAP].GetUInt();
    }
    else
    {
        mPoint = mapWorldPosition_.mPoint_.ToHash();
        eventData[Go_Appear::GO_MAP] = mPoint;
    }

//    URHO3D_LOGINFOF("GOC_Destroyer() - OnWorldEntityCreate : %s(%u) position=%s viewZ=%d ... ",
//             node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString(), node_->GetVar(GOA::ONVIEWZ).GetInt());

    // Subscriber to WorldDestroy
    UnsubscribeFromEvent(node_, WORLD_ENTITYCREATE);
    SubscribeToEvent(node_, WORLD_ENTITYDESTROY, URHO3D_HANDLER(GOC_Destroyer, OnWorldEntityDestroy));

    // Send Appear Event to world2D
    GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
    int controltype = controller ? controller->GetControllerType() : GO_None;

    node_->SetVar(GOA::ONMAP, mPoint);
    eventData[Go_Appear::GO_ID] = node_->GetID();
    eventData[Go_Appear::GO_TYPE] = controltype;
    eventData[Go_Appear::GO_MAINCONTROL] = controller ? controller->IsMainController() : false;
    node_->SendEvent(GO_APPEAR, eventData);

    if (controltype & GO_Entity)
    {
        eventData[World_EntityUpdate::GO_STATE] = 0;
        node_->SendEvent(WORLD_ENTITYUPDATE, eventData);
    }

//    URHO3D_LOGERRORF("GOC_Destroyer() - OnWorldEntityCreate : %s(%u) position=%s(%s) map=%s viewZ=%d ... OK !",
//             node_->GetName().CString(), node_->GetID(), mapWorldPosition_.position_.ToString().CString(),
//             node_->GetWorldPosition2D().ToString().CString(), mapWorldPosition_.mPoint_.ToString().CString(), mapWorldPosition_.viewZ_);
}

void GOC_Destroyer::OnWorldEntityDestroy(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Destroyer() - OnWorldEntityDestroy : %s(%u) ...", node_->GetName().CString(), node_->GetID());

//    if (currentMap_)
//        currentMap_->GetMapData()->RemoveEntityData(node_);

    UnsubscribeFromEvent(node_, WORLD_ENTITYDESTROY);

    /// don't set 0.f, it's critical time for Self-Destroying without Crash for Player Avatar
    Destroy(0.1f);

//    URHO3D_LOGINFOF("GOC_Destroyer() - OnWorldEntityDestroy : ... OK !");
}

void GOC_Destroyer::Destroy(float delay, bool reset)
{
    if (!node_)
        return;

    if (!enabled_)
        return;

    UnsubscribeFromAllEvents();

    if (reset && destroyMode_ != FREEMEMORY && delay != 0.f)
        Reset(destroyMode_ == DISABLE);

    unsigned nodeid = node_->GetID();

    URHO3D_LOGINFOF("GOC_Destroyer() - Destroy : node=%s(%u) ... enabled_=%s delay=%f mode=%s ...",
                                    node_->GetName().CString(), nodeid, enabled_ ? "true" : "false", delay, RemoveStateNames[destroyMode_]);

    if (!GameContext::Get().LocalMode_ && body_)
    {
        if (destroyMode_ == DISABLE)
            GameNetwork::Get()->SetEnableObject(false, nodeid);
        else if (GameContext::Get().ServerMode_)
            GameNetwork::Get()->Server_RemoveObject(nodeid);
        else if (GameContext::Get().ClientMode_)
            GameNetwork::Get()->Client_RemoveObject(nodeid);
    }

    VariantMap& eventData = context_->GetEventDataMap();

    if (!node_->GetVar(GOA::ISDEAD).GetBool())
    {
        node_->SetVar(GOA::DESTROYING, true);

        eventData[GOC_Life_Events::GO_ID] = node_->GetID();
        eventData[GOC_Life_Events::GO_KILLER] = 0;
        eventData[GOC_Life_Events::GO_TYPE] = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
        eventData[GOC_Life_Events::GO_WORLDCONTACTPOINT] = 0;
        node_->SetVar(GOA::ISDEAD, true);
        node_->SetVar(GOA::LIFE, 0);
        node_->SendEvent(GOC_LIFEDEAD, eventData);
    }

    GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
    int controltype = controller ? controller->GetControllerType() : GO_None;
    eventData[Go_Destroy::GO_ID] = nodeid;
    eventData[Go_Destroy::GO_TYPE] = controltype;
    eventData[Go_Destroy::GO_MAP] = mapWorldPosition_.mPoint_.ToHash();
    eventData[Go_Destroy::GO_PTR] = 0;

    if (GOT::GetTypeProperties(node_->GetVar(GOA::GOT).GetStringHash()) & GOT_Anchored)
        eventData[Go_Destroy::GO_TILE] = mapWorldPosition_.tileIndex_;

    node_->SendEvent(GO_DESTROY, eventData);

    if (controltype & GO_Entity)
    {
        eventData[World_EntityUpdate::GO_STATE] = 1;
        node_->SendEvent(WORLD_ENTITYUPDATE, eventData);
    }

    if (node_->GetComponent<ObjectMaped>())
    {
        node_->GetComponent<ObjectMaped>()->OnNodeRemoved();
    }

    if (destroyMode_ >= FASTPOOLRESTORE)
        delay = 0.f;

//    URHO3D_LOGERRORF("GOC_Destroyer() - Destroy : %s(%u) mode=%s delay=%f ... OK !", node_->GetName().CString(), nodeid, RemoveStateNames[destroyMode_], delay);

    node_->SetVar(GOA::DESTROYING, false);

    if (delay > 0.f)
    {
        TimerRemover* remover = TimerRemover::Get();
        if (remover)
        {
            remover->Start(node_, delay, (RemoveState)destroyMode_);
            enabled_ = false;
            return;
        }
    }
    TimerRemover::Remove(node_, (RemoveState)destroyMode_);
}

bool GOC_Destroyer::IsInWalls(MapBase* map, int viewZ)
{
    if (!mapWorldPosition_.defined_)
    {
        Vector2 position;

        if (!GetUpdatedWorldPosition2D(position))
            position = node_->GetWorldPosition2D();

        World2D::GetWorldInfo()->Convert2WorldMapPosition(position, mapWorldPosition_, mapWorldPosition_.positionInTile_);
    }

    if (!shapesRect_.Defined())
        UpdateShapesRect();

    if (GameHelpers::CheckFreeTilesAtViewZ(this, map, mapWorldPosition_.tileIndex_, viewZ))
        return false;

    return true;
}

bool GOC_Destroyer::IsOnFreeTiles(int viewZ) const
{
    if (!currentMap_)
    {
        URHO3D_LOGWARNINGF("GOC_Destroyer() - IsOnFreeTiles : %s(%u) !currentMap ... OK !", node_->GetName().CString(), node_->GetID());
        return true;
    }

    bool result = GameHelpers::CheckFreeTilesAtViewZ(this, currentMap_, mapWorldPosition_.tileIndex_, viewZ);

//    URHO3D_LOGINFOF("GOC_Destroyer() - IsOnFreeTiles : %s(%u) ... on currentmap viewZ=%d result=%s !", node_->GetName().CString(), node_->GetID(), viewZ, result ? "true":"false");

    if (result)
    {
        MapBase* objectmaped = 0;
        ObjectMaped::GetPhysicObjectAt(mapWorldPosition_.position_, objectmaped, false);
        if (objectmaped)
        {
            unsigned tileindex = objectmaped->GetTileIndexAt(mapWorldPosition_.position_);
            result = GameHelpers::CheckFreeTilesAtViewZ(this, objectmaped, tileindex, viewZ);
            URHO3D_LOGINFOF("GOC_Destroyer() - IsOnFreeTiles : %s(%u) ... on objectmaped=%d at tileindex=%u result=%s !", node_->GetName().CString(), node_->GetID(), ((ObjectMaped*)objectmaped)->GetPhysicObjectID(), tileindex, result ? "true":"false");
        }
    }

//    URHO3D_LOGINFOF("GOC_Destroyer() - IsOnFreeTiles : %s(%u) ... result=%s !", node_->GetName().CString(), node_->GetID(), result ? "true":"false");

    return result;
}

FeatureType GOC_Destroyer::GetFeatureOnViewId(int viewId) const
{
    return currentMap_ ? currentMap_->GetFeatureType(mapWorldPosition_.tileIndex_, viewId) : MapFeatureType::NoMapFeature;
}

FeatureType GOC_Destroyer::GetFeatureOnViewZ(int viewZ) const
{
    return currentMap_ ? currentMap_->GetFeatureAtZ(mapWorldPosition_.tileIndex_, viewZ) : MapFeatureType::NoMapFeature;
}

// Set shapesRect_      : the rect from collisionshapes centered on mass center and worldscaled
// Set shapeRectInTile_ : the same rect size centered on mass center in tilescaled based
void GOC_Destroyer::UpdateShapesRect()
{
    if (!node_->GetDerivedComponent<GOC_Controller>())
        return;

    if (!body_)
        return;

    shapesRect_.Clear();

    const Vector<WeakPtr<CollisionShape2D> >& shapes = body_->GetCollisionsShapes();

    for (unsigned i=0; i<shapes.Size(); i++)
    {
        if (!shapes[i] || shapes[i]->IsTrigger())
            continue;

        if (shapes[i]->IsInstanceOf<CollisionCircle2D>())
        {
            CollisionCircle2D* shape = static_cast<CollisionCircle2D*>(shapes[i].Get());

            Vector2 vmin = shape->GetCenter() - Vector2(shape->GetRadius(), shape->GetRadius());
            Vector2 vmax = shape->GetCenter() + Vector2(shape->GetRadius(), shape->GetRadius());

            if (shapesRect_.Defined())
                shapesRect_.Merge(Rect(vmin, vmax));
            else
                shapesRect_.Define(vmin, vmax);
        }
        else if (shapes[i]->IsInstanceOf<CollisionBox2D>())
        {
            CollisionBox2D* shape = static_cast<CollisionBox2D*>(shapes[i].Get());

            Vector2 vmin = shape->GetCenter() - 0.5f * shape->GetSize();
            Vector2 vmax = shape->GetCenter() + 0.5f * shape->GetSize();

            if (shapesRect_.Defined())
                shapesRect_.Merge(Rect(vmin, vmax));
            else
                shapesRect_.Define(vmin, vmax);
        }
        else
        {
            URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdateShapesRect : %s(%u) Shapes Size ... %s Not Implemented !",
                               node_->GetName().CString(), node_->GetID(), shapes[i]->GetTypeName().CString());
        }
    }

    shapesRect_.min_ = (shapesRect_.min_ - body_->GetMassCenter()) * node_->GetWorldScale2D();
    shapesRect_.max_ = (shapesRect_.max_ - body_->GetMassCenter()) * node_->GetWorldScale2D();

    Vector2 wTileSize(World2D::GetWorldInfo()->mTileWidth_, World2D::GetWorldInfo()->mTileHeight_);
    mapWorldPosition_.shapeRectInTile_.Define(shapesRect_.min_ / wTileSize, shapesRect_.max_/ wTileSize);

//    URHO3D_LOGINFOF("GOC_Destroyer() - UpdateShapesRect : %s(%u) shapesRect_=%s mcenter=%s shapeRectInTile_=%s ", node_->GetName().CString(), node_->GetID(),
//                     shapesRect_.ToString().CString(), body_->GetMassCenter().ToString().CString(), mapWorldPosition_.shapeRectInTile_.ToString().CString());
}

void GOC_Destroyer::AdjustPosition(const ShortIntVector2& mpoint, int viewZ, Vector2& position)
{
    if (!checkUnstuck_)
        return;

    World2D::GetWorld()->GetWorldInfo()->Convert2WorldMapPosition(mpoint, position, mapWorldPosition_.mPosition_);

    mapWorldPosition_.mPoint_ = mpoint;
    mapWorldPosition_.position_ = position;
    mapWorldPosition_.tileIndex_ = mapWorldPosition_.mPosition_.y_ * World2D::GetWorld()->GetWorldInfo()->mapWidth_ + mapWorldPosition_.mPosition_.x_;
    mapWorldPosition_.SetViewZ(viewZ);
    mapWorldPosition_.defined_ = true;

    if (!shapesRect_.Defined())
        UpdateShapesRect();

    if (!currentMap_)
        currentMap_ = World2D::GetMapAt(mapWorldPosition_.mPoint_, false);

    if (currentMap_)
        currentCell_ = currentMap_->GetFluidCellPtr(mapWorldPosition_.tileIndex_, mapWorldPosition_.viewZIndex_);

    bool check = Unstuck();

//    URHO3D_LOGINFOF("GOC_Destroyer() - AdjustNodePosition : %s(%u) initialposition=%s adjustedposition=%s !",
//                    node_->GetName().CString(), node_->GetID(), position.ToString().CString(), mapWorldPosition_.position_.ToString().CString());

    position = mapWorldPosition_.position_;
}

void GOC_Destroyer::AdjustPositionInTile(int viewZ)
{
    if (!checkUnstuck_)
        return;

    mapWorldPosition_.viewZ_ = viewZ;

    if (!shapesRect_.Defined())
        UpdateShapesRect();

    if (!currentMap_)
        currentMap_ = World2D::GetMapAt(mapWorldPosition_.mPoint_, false);

    if (currentMap_)
    {
        GameHelpers::AdjustPositionInTile(this, currentMap_, mapWorldPosition_);

//        URHO3D_LOGINFOF("GOC_Destroyer() - AdjustPositionInTile : %s(%u) adjustedposition_=%s !", node_->GetName().CString(), node_->GetID(),
//                        mapWorldPosition_.ToString().CString());
    }
}

void GOC_Destroyer::AdjustPositionInTile(const WorldMapPosition& initialposition)
{
//    URHO3D_LOGINFOF("GOC_Destroyer() - AdjustPositionInTile : %s(%u) initialposition=%s ...", node_->GetName().CString(), node_->GetID(),
//                     initialposition.ToString().CString());

    if (!checkUnstuck_)
        return;

    if (!shapesRect_.Defined())
        UpdateShapesRect();

    if (!currentMap_)
        currentMap_ = World2D::GetMapAt(initialposition.mPoint_, false);

    if (currentMap_)
    {
        GameHelpers::AdjustPositionInTile(this, currentMap_, initialposition);

//        URHO3D_LOGINFOF("GOC_Destroyer() - AdjustPositionInTile : %s(%u) adjustedposition_=%s !", node_->GetName().CString(), node_->GetID(),
//                        mapWorldPosition_.ToString().CString());
    }
}

bool GOC_Destroyer::Unstuck()
{
    if (!checkUnstuck_)
        return false;

    if (!currentCell_)
        return true;

    // Dimension in Tiles of the entity
    IntVector2 entitySizeInTiles;
    Vector2 rectsize = mapWorldPosition_.shapeRectInTile_.Size();
    entitySizeInTiles.x_ = CeilToInt(rectsize.x_);
    entitySizeInTiles.y_ = CeilToInt(rectsize.y_);
    IntVector2 tileOffset;

    FluidCell* currentcell = currentCell_;
    if (currentcell->type_ == BLOCK)
    {
        // Try to use the lastUnblockedTile
        if (lastUnblockedTile_ != -1)
        {
            currentcell = currentMap_->GetFluidCellPtr(lastUnblockedTile_, mapWorldPosition_.viewZIndex_);
            if (currentcell && currentcell->type_ != BLOCK)
            {
                tileOffset.x_ = currentMap_->GetTileCoordX(lastUnblockedTile_) - mapWorldPosition_.mPosition_.x_;
                tileOffset.y_ = currentMap_->GetTileCoordY(lastUnblockedTile_) - mapWorldPosition_.mPosition_.y_;

                URHO3D_LOGWARNINGF("GOC_Destroyer() - Unstuck : %s(%u) use lastUnblockedTile tileOffset=%d %d !", node_->GetName().CString(), node_->GetID(), tileOffset.x_, tileOffset.y_);
            }
            else
            {
                lastUnblockedTile_ = -1;
            }
        }

        // Try to Find a Neighbor Unblocked Tile
        if (lastUnblockedTile_ == -1)
        {
            currentcell = currentCell_;
            FluidCell* topcell = currentcell;
            int topexpand = 0;
            while (topexpand <= entitySizeInTiles.y_)
            {
                if (topcell->type_ == BLOCK)
                    topexpand++;

                if (!topcell->Top || topcell->Top->type_ != BLOCK)
                    break;

                topcell = topcell->Top;
            }

            FluidCell* bottomcell = currentcell;
            int bottomexpand = 0;
            while (bottomexpand <= entitySizeInTiles.y_)
            {
                if (bottomcell && bottomcell->type_ == BLOCK)
                    bottomexpand++;

                if (!bottomcell->Bottom || bottomcell->Bottom->type_ != BLOCK)
                    break;

                bottomcell = bottomcell->Bottom;
            }

            FluidCell* leftcell = currentcell;
            int leftexpand = 0;
            while (leftexpand <= entitySizeInTiles.x_)
            {
                if (leftcell && leftcell->type_ == BLOCK)
                    leftexpand++;

                if (!leftcell->Left || leftcell->Left->type_ != BLOCK)
                    break;

                leftcell = leftcell->Left;
            }

            FluidCell* rightcell = currentcell;
            int rightexpand = 0;
            while (rightexpand <= entitySizeInTiles.x_)
            {
                if (rightcell && rightcell->type_ == BLOCK)
                    rightexpand++;

                if (!rightcell->Right || rightcell->Right->type_ != BLOCK)
                    break;

                rightcell = rightcell->Right;
            }

            URHO3D_LOGWARNINGF("GOC_Destroyer() - Unstuck : %s(%u) t=%d b=%d l=%d r=%d !", node_->GetName().CString(), node_->GetID(), topexpand, bottomexpand, leftexpand, rightexpand);

            if (topexpand && topexpand < bottomexpand && topexpand < leftexpand && topexpand < rightexpand)
            {
                currentcell = topcell->Top;
                tileOffset.y_ = -topexpand-1;
            }
            else if (bottomexpand && bottomexpand < topexpand && bottomexpand < leftexpand && bottomexpand < rightexpand)
            {
                currentcell = bottomcell->Bottom;
                tileOffset.y_ = bottomexpand+1;
            }
            else if (leftexpand && leftexpand < topexpand && leftexpand < bottomexpand && leftexpand < rightexpand)
            {
                currentcell = leftcell->Left;
                tileOffset.x_ = -leftexpand-1;
            }
            else if (rightexpand && rightexpand <= topexpand && rightexpand <= bottomexpand && rightexpand <= leftexpand)
            {
                currentcell = rightcell->Right;
                tileOffset.x_ = rightexpand+1;
            }

            URHO3D_LOGWARNINGF("GOC_Destroyer() - Unstuck : %s(%u) initialOffset=%d %d !", node_->GetName().CString(), node_->GetID(), tileOffset.x_, tileOffset.y_);

            if (!currentcell || currentcell->type_ == BLOCK)
            {
                URHO3D_LOGWARNINGF("GOC_Destroyer() - Unstuck : %s(%u) is in block and can't find an unblocktile in the neighborhood !", node_->GetName().CString(), node_->GetID());
                return false;
            }
        }

        // Find the TileOffset to place the tiledRect of the avatar
        {
            // Calculate the tiles needed for the entity
            Rect numEntityInTiles;
            numEntityInTiles.min_.y_ = Abs(mapWorldPosition_.shapeRectInTile_.min_.y_) - 0.5f;
            numEntityInTiles.max_.y_ = Abs(mapWorldPosition_.shapeRectInTile_.max_.y_) - 0.5f;
            numEntityInTiles.min_.x_ = Abs(mapWorldPosition_.shapeRectInTile_.min_.x_) - 0.5f;
            numEntityInTiles.max_.x_ = Abs(mapWorldPosition_.shapeRectInTile_.max_.x_) - 0.5f;

            IntVector2 tileoffset;

            // Need to Check the tiles in Y
            if (numEntityInTiles.min_.y_ > 0.5f || numEntityInTiles.max_.y_ > 0.5f)
            {
                // Get the number of UNBLOCKED Tiles available in Y from currentCell
                int numBottomDirUnBlockTiles = 0;
                int numTopDirUnBlockTiles = 0;
                FluidCell* fcell = currentcell->Bottom;
                while (fcell && fcell->type_ != BLOCK && numBottomDirUnBlockTiles < entitySizeInTiles.y_)
                {
                    numBottomDirUnBlockTiles++;
                    fcell = fcell->Bottom;
                }
                fcell = currentcell->Top;
                while (fcell && fcell->type_ != BLOCK && numTopDirUnBlockTiles < entitySizeInTiles.y_)
                {
                    numTopDirUnBlockTiles++;
                    fcell = fcell->Top;
                }
                if (numTopDirUnBlockTiles + numBottomDirUnBlockTiles + 1 < entitySizeInTiles.y_)
                {
                    URHO3D_LOGWARNINGF("GOC_Destroyer() - Unstuck : %s(%u) Y Check entitySizeInTiles=%d available=%d(Top=%d + Bottom=%d) !",
                                       node_->GetName().CString(), node_->GetID(), entitySizeInTiles.y_, numTopDirUnBlockTiles+numBottomDirUnBlockTiles+1, numTopDirUnBlockTiles,numBottomDirUnBlockTiles);
                    return false;
                }

                URHO3D_LOGINFOF("GOC_Destroyer() - Unstuck : %s(%u) can be unstucked numEntityInTileY=%f %f freeTiles : DirTop=%d DirBottom=%d !",
                                node_->GetName().CString(), node_->GetID(), numEntityInTiles.max_.y_, numEntityInTiles.min_.y_, numTopDirUnBlockTiles, numBottomDirUnBlockTiles);

                // Set offset needed to Unstuck in y
                if (numEntityInTiles.max_.y_ - numTopDirUnBlockTiles > 0.5f)
                {
                    // Need to offset To Bottom
                    tileoffset.y_ += RoundToInt(numEntityInTiles.max_.y_ - numTopDirUnBlockTiles);
                }
                if (numEntityInTiles.min_.y_ > numBottomDirUnBlockTiles > 0.5f)
                {
                    // Need To offset To Top
                    tileoffset.y_ -= RoundToInt(numEntityInTiles.min_.y_ - numBottomDirUnBlockTiles);
                }
            }

            // Need to Check the tiles in X
            if (numEntityInTiles.min_.x_ > 0.5f || numEntityInTiles.max_.x_ > 0.5f)
            {
                // Get the number of UNBLOCKED Tiles available in X from currentCell
                int numLeftDirUnBlockTiles = 0;
                int numRightDirUnBlockTiles = 0;
                FluidCell* fcell = currentcell->Left;
                while (fcell && fcell->type_ != BLOCK && numLeftDirUnBlockTiles < entitySizeInTiles.x_)
                {
                    numLeftDirUnBlockTiles++;
                    fcell = fcell->Left;
                }
                fcell = currentcell->Right;
                while (fcell && fcell->type_ != BLOCK && numRightDirUnBlockTiles < entitySizeInTiles.x_)
                {
                    numRightDirUnBlockTiles++;
                    fcell = fcell->Right;
                }
                if (numLeftDirUnBlockTiles + numRightDirUnBlockTiles + 1 < entitySizeInTiles.y_)
                {
                    URHO3D_LOGWARNINGF("GOC_Destroyer() - Unstuck : %s(%u) X Check entitySizeInTiles=%d available=%d(Left=%d + Right=%d) !",
                                       node_->GetName().CString(), node_->GetID(), entitySizeInTiles.x_, numLeftDirUnBlockTiles+numRightDirUnBlockTiles+1, numLeftDirUnBlockTiles,numRightDirUnBlockTiles);
                    return false;
                }

                URHO3D_LOGINFOF("GOC_Destroyer() - Unstuck : %s(%u) can be unstucked numEntityInTiles=%f %f freeTiles : DirLeft=%d DirRight=%d !",
                                node_->GetName().CString(), node_->GetID(), numEntityInTiles.min_.x_, numEntityInTiles.max_.x_, numLeftDirUnBlockTiles, numRightDirUnBlockTiles);

                // Set offset needed to Unstuck in x
                if (numEntityInTiles.min_.x_ - numLeftDirUnBlockTiles > 0.5f)
                {
                    // Need to offset To Right
                    tileoffset.x_ += RoundToInt(numEntityInTiles.min_.x_ - numLeftDirUnBlockTiles);
                }
                if (numEntityInTiles.max_.x_ - numRightDirUnBlockTiles > 0.5f)
                {
                    // Need To offset To Left
                    tileoffset.x_ -= RoundToInt(numEntityInTiles.max_.x_ - numRightDirUnBlockTiles);
                }
            }

            // Set TileOffset needed to Unstuck
            if (tileoffset != IntVector2::ZERO)
            {
                // Reset fluicell for update
                currentcell = 0;
                tileOffset += tileoffset;
            }
        }
    }

    // Apply new position
    mapWorldPosition_.mPosition_ += tileOffset;
    mapWorldPosition_.positionInTile_.x_ = 0.5f;
    mapWorldPosition_.positionInTile_.y_ = 0.5f;

    World2D::GetWorldInfo()->UpdateWorldPosition(mapWorldPosition_);

    // Update currentCell
    if (currentcell)
        currentCell_ = currentcell;
    else
        currentCell_ = currentMap_->GetFluidCellPtr(mapWorldPosition_.tileIndex_, mapWorldPosition_.viewZIndex_);

    // wmPosition.position_ is the world mass center

    // Apply position to node
    Vector2 nodeposition = mapWorldPosition_.position_ - body_->GetMassCenter() * node_->GetWorldScale2D();
    node_->SetWorldPosition2D(nodeposition);
    // Apply position to physic too
    body_->SetBodyPosition(nodeposition, node_->GetWorldRotation2D());

#ifdef DUMP_UNSTUCKWARNING
    URHO3D_LOGINFOF("GOC_Destroyer() - Unstuck : %s(%u) update position=%s (tileOffset=%d %d) !",
                    node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString(), tileOffset.x_, tileOffset.y_);
#endif

    return true;
}

// direction=1=Right
bool GOC_Destroyer::HasCellInFront(bool direction)
{
    if (!currentCell_)
        return false;

    return direction ? currentCell_->Right != 0 : currentCell_->Left != 0;
}

// direction=1=Right
bool GOC_Destroyer::HasWallInFront(bool direction)
{
    if (!currentCell_)
        return false;

    bool& haswallinfront = hasWallInFront_[direction];

    if (lastcurrentCell_[direction] == currentCell_)
        return haswallinfront;

    lastcurrentCell_[direction] = currentCell_;
    FluidCell* curcell = currentCell_;

    if (!curcell->Top || curcell->Top->type_ != AIR)
    {
        haswallinfront = false;
        return false;
    }

    int distance = (int)floor((mapWorldPosition_.shapeRectInTile_.Size().x_ / World2D::GetWorldInfo()->mTileWidth_ + 1.f) * 0.5f);

    if (direction)
    {
        for (int i=0; i < distance; i++)
        {
            curcell = curcell->Right;
            if (!curcell || curcell->type_ != AIR)
            {
                haswallinfront = false;
                return false;
            }
        }

        haswallinfront = curcell->Right && curcell->Right->type_ == BLOCK;
    }
    else
    {
        for (int i=0; i < distance; i++)
        {
            curcell = curcell->Left;
            if (!curcell || curcell->type_ != AIR)
            {
                haswallinfront = false;
                return false;
            }
        }

        haswallinfront = curcell->Left && curcell->Left->type_ == BLOCK;
    }

//    URHO3D_LOGERRORF("GOC_Destroyer() - HasWallInFront : node=%s(%u) position=%s distance=%d wallinfront=%s!", node_->GetName().CString(), node_->GetID(),
//                     mapWorldPosition_.ToString().CString(), distance, haswallinfront ? "true":"false");

    return haswallinfront;
}

void GOC_Destroyer::UpdateAreaStates(bool sendfluidevent)
{
    if (!body_)
        return;

    unsigned movestates = node_->GetVar(GOA::MOVESTATE).GetUInt();

    // take care of climbing
    if ((movestates & MSK_MV_CLIMBWALL) == MSK_MV_CLIMBWALL || (movestates & MSK_MV_CLIMBROOF) == MSK_MV_CLIMBROOF)
    {
        if (body_->GetGravityScale() != NOGRAVITY)
        {
//            URHO3D_LOGINFOF("GOC_Destroyer() - UpdateAreaStates : Node=%s(%u) in CLIMBING ! (m=%u) ", node_->GetName().CString(), node_->GetID(), movestates);
            body_->SetGravityScale(NOGRAVITY);
            node_->SendEvent(EVENT_CHANGEGRAVITY);
        }
    }
    else
    {
        float liquidHeight = 0.f;

        // can check fluid
        // only if currentCell is a surface or a fullwater
        if (GameContext::Get().gameConfig_.fluidEnabled_ && currentCell_ && currentCell_->patternType_ >= FPT_Surface)
        {
            // only fullwater if Top is a Surface or a fullwater
//            liquidHeight = (currentCell_->Top && currentCell_->Top->pattern_ >= PTRN_CenterFallRightSurface) ? FLUID_MAXVALUE : Min(currentCell_->GetMass(), FLUID_MAXVALUE) * (MapInfo::info.mTileHalfSize_.y_ * 2.f / FLUID_MAXVALUE);
            liquidHeight = Min(currentCell_->GetMass(), FLUID_MAXVALUE) * (MapInfo::info.mTileHalfSize_.y_ * 2.f / FLUID_MAXVALUE);
        }

        bool inFluid = (liquidHeight > mapWorldPosition_.positionInTile_.y_);

        // in water
        if (inFluid)
        {
            if (body_->GetGravityScale() != WATERGRAVITY || sendfluidevent)
            {
                // Limit entrance in water : inverse velocity in reaction of the mass
                if (body_->GetLinearVelocity().y_ < -1.f && buoyancy_ > 0.f)
                {
//					URHO3D_LOGINFOF("GOC_Destroyer() - UpdateAreaStates : Node=%s(%u) Limit entrance in water !", node_->GetName().CString(), node_->GetID());
                    body_->GetBody()->SetLinearVelocity(b2Vec2(body_->GetBody()->GetLinearVelocity().x, body_->GetBody()->GetLinearVelocity().y * 0.25f));
                    body_->GetBody()->ApplyLinearImpulseToCenter(b2Vec2(0.f, 2.f * buoyancy_ * body_->GetMass()), true);
                }

//				URHO3D_LOGINFOF("GOC_Destroyer() - UpdateAreaStates : Node=%s(%u) in WATER ! currentCell_=%u mass=%F liquidHeight=%F > inTileY=%F",
//									node_->GetName().CString(), node_->GetID(), currentCell_, currentCell_->GetMass(), liquidHeight, mapWorldPosition_.positionInTile_.y_);

                body_->SetGravityScale(WATERGRAVITY);
                node_->SendEvent(EVENT_CHANGEGRAVITY);
                sendfluidevent = true;
            }

            // Apply this for Buoyancy
            if (buoyancy_ > 0.f && body_->GetLinearVelocity().y_ <= BUOYANCYVELMIN)
            {
                float fy = BUOYANCYVEL * buoyancy_ * body_->GetMass();
//				URHO3D_LOGINFOF("GOC_Destroyer() - UpdateAreaStates : Node=%s(%u) Buoyancy fy=%f applied ! (m=%u) ", node_->GetName().CString(), node_->GetID(), fy, movestates);
                body_->GetBody()->ApplyForceToCenter(b2Vec2(0.f, fy), true);
            }
        }
        // in air
        else
        {
            if (body_->GetGravityScale() != AIRGRAVITY || sendfluidevent)
            {
//				URHO3D_LOGINFOF("GOC_Destroyer() - UpdateAreaStates : Node=%s(%u) in AIR ! currentCell_=%u mass=%F liquidHeight=%F < inTileY=%F",
//									node_->GetName().CString(), node_->GetID(), currentCell_, currentCell_ ? currentCell_->GetMass() : 0.f, liquidHeight, mapWorldPosition_.positionInTile_.y_);

                body_->SetGravityScale(AIRGRAVITY);
                node_->SendEvent(EVENT_CHANGEGRAVITY);
                sendfluidevent = true;
            }
        }

        if (sendfluidevent)
        {
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Go_CollideFluid::GO_WETTED] = inFluid;
            node_->SendEvent(GO_COLLIDEFLUID, eventData);
        }
    }
}

/// Based on physic dont work with rotation
bool GOC_Destroyer::GetUpdatedWorldPosition2D(Vector2& position)
{
    position = node_->GetWorldPosition2D();

    // if position defined, adjust with mass center
    if (!IsNaN(position.x_))
    {
        if (body_ && body_->IsEnabled() && body_->GetBodyType() != BT_STATIC)
        {
            if (body_->IsFixedRotation())
            {
                Vector2 pos = body_->GetMassCenter();
                Vector2 scale = node_->GetWorldScale2D();
                position += pos*scale;
            }
            else
            {
                position = node_->GetWorldTransform2D() * body_->GetMassCenter();
            }
        }
    }

//    URHO3D_LOGERRORF("GOC_Destroyer() - UpdatePositions : node=%s(%u) (Enabled=%s) position=%s !",
//                    node_->GetName().CString(), node_->GetID(), node_->IsEnabled() ? "true":"false", position.ToString().CString());

    return !IsNaN(position.x_);
}

//bool GOC_Destroyer::GetUpdatedWorldMassCenter(Vector2& position)
//{
//    position = node_->GetWorldPosition2D();
//
//    // if position defined, adjust with mass center
//    if (!IsNaN(position.x_))
//    {
//        RigidBody2D* body = node_->GetComponent<RigidBody2D>();
//        if (body && body->IsEnabled())
//        {
//            Vector2 pos = body->GetMassCenter();
//            Vector2 scale = node_->GetWorldScale2D();
//            position += pos*scale;
//        }
//    }
//
//    return !IsNaN(position.x_);
//}

/// TEST
/// Based on physic works with rotation
//bool GOC_Destroyer::GetUpdatedWorldPosition2D(Vector2& position)
//{
//    RigidBody2D* body = node_->GetComponent<RigidBody2D>();
//    if (body && body->IsEnabled())
//        position = node_->LocalToWorld2D(body->GetMassCenter());
//    else
//        position = node_->GetWorldPosition2D();
//
//    return !IsNaN(position.x_);
//}

/// Based on DrawingBox, not handle physics
//bool GOC_Destroyer::GetUpdatedWorldPosition2D(Vector2& position)
//{
//    Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
//    bool update = node_->IsEnabled() && drawable && drawable->GetWorldBoundingBox().Defined();
//
//    if (update)
//    {
//        Vector3 pos = drawable->GetWorldBoundingBox().Center();
//        position.x_ = pos.x_;
//        position.y_ = pos.y_;
//    }
//
//    if (!update || IsNaN(position.x_))
//    {
//        if (drawable && node_->IsEnabled())
//            drawable->MarkDirty();
//
//        // instead use the worldmasscenter
//         return GetUpdatedWorldMassCenter(position);
//
////        URHO3D_LOGWARNINGF("GOC_Destroyer() - GetUpdatedWorldPosition : node=%s(%u) NO DRAWABLE2D or BBox NaN => Get Node position=%f %f parent=%s(%u)!",
////                            node_->GetName().CString(), node_->GetID(), position.x_, position.y_,
////                            node_->GetParent() ? node_->GetParent()->GetName().CString() : "", node_->GetParent() ? node_->GetParent()->GetID() : 0);
//    }
//
//    return !IsNaN(position.x_);
//}

#define ACTIVE_SWITCHVIEW_DOOR
//#define ACTIVE_SWITCHVIEW_WINDOW
//#define SWITCHVIEW_DELAY_TEST

void GOC_Destroyer::UpdatePositions(ChangeMapMode mode)
{
    bool state = UpdatePositions(context_->GetEventDataMap(false), mode);
}

//static Vector2 sTempPosition_;
static MapBase* sMaps_[2];
static WorldMapPosition sObjectWorldMapPosition_;

bool GOC_Destroyer::UpdatePositions(VariantMap& eventData, ChangeMapMode mode)
{
    if (!World2D::GetWorld())
        return false;

    // Get World Mass Center
    if (!GetUpdatedWorldPosition2D(sMPosition_.position_))
    {
        URHO3D_LOGERRORF("GOC_Destroyer() - UpdatePositions : node=%s(%u) NaN position Value (Enabled=%s) => Destroy !",
                         node_->GetName().CString(), node_->GetID(), node_->IsEnabled() ? "true":"false");

        OnWorldEntityDestroy(WORLD_ENTITYDESTROY, context_->GetEventDataMap(false));
//        node_->SendEvent(WORLD_ENTITYDESTROY);
        return false;
    }

    // Update map position, positionInTile and tileindex if need
    World2D::GetWorldInfo()->Convert2WorldMapPosition(sMPosition_.position_, sMPosition_, mapWorldPosition_.positionInTile_);
    mapWorldPosition_.position_ = sMPosition_.position_;

    stilePositionUpdated_[0] = sMPosition_.tileIndex_ != mapWorldPosition_.tileIndex_ || mode == CHANGEMAP_FORCE || !mapWorldPosition_.defined_;

    sChangeMap_ = (sMPosition_.mPoint_ != mapWorldPosition_.mPoint_ || mode == CHANGEMAP_FORCE);
    sInsideBounds_ = sChangeMap_ ? World2D::IsInsideWorldBounds(mapWorldPosition_.position_) : true;
    sChangeMap_ &= sInsideBounds_;
    sWaitForMap_ = moveEntityData_ = false;

    if (sChangeMap_ && !World2D::IsInsideWorldBounds(sMPosition_.mPoint_)) sChangeMap_ = false;

    sViewZ_ = mapWorldPosition_.viewZ_ != 0 ? mapWorldPosition_.viewZ_ : NOVIEW;

    // Not Inside World
    if (!GameContext::Get().ClientMode_ && !sInsideBounds_)
    {
        URHO3D_LOGERRORF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s not in WorldBounds=%s => Destroy !",
                         node_->GetName().CString(), node_->GetID(), sMPosition_.position_.ToString().CString(), World2D::GetWorldFloatBounds().ToString().CString());

        OnWorldEntityDestroy(WORLD_ENTITYDESTROY, context_->GetEventDataMap(false));

//        node_->SendEvent(WORLD_ENTITYDESTROY);
        return false;
    }

    if (sChangeMap_)
    {
//        if (currentMap_ && (node_->GetVar(GOA::TYPECONTROLLER).GetUInt() & (GO_AI_Ally | GO_Player)) == 0)
//        {
////            URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) Change MapFrom=%u => RemoveEntityData !", node_->GetName().CString(), node_->GetID(), currentMap_);
//            currentMap_->GetMapData()->RemoveEntityData(node_);
//            moveEntityData_ = true;
//        }
        currentMap_ = 0;
    }

//    URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s !", node_->GetName().CString(), node_->GetID(), sMPosition_.position_.ToString().CString());

    // Update position
    if (stilePositionUpdated_[0])
    {
        mapWorldPosition_.mPosition_ = sMPosition_.mPosition_;
        mapWorldPosition_.tileIndex_ = sMPosition_.tileIndex_;
        currentCell_ = 0;
        viewZToCheck_ = NOVIEW;

        if (currentMap_ && currentMap_->GetStatus() != Available)
        {
            sWaitForMap_ = true;

//            URHO3D_LOGERRORF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s not currentMap Available => WaitForMap !",
//                                   node_->GetName().CString(), node_->GetID(), sMPosition_.position_.ToString().CString());
            /*
            URHO3D_LOGERRORF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s not currentMap Available => DIE !",
                            node_->GetName().CString(), node_->GetID(), sMPosition_.position_.ToString().CString());
            Destroy();
            return;
            */
        }
    }
    /*
        /// just a temp checker for tileindex Error
        if (mapWorldPosition_.tileIndex_ > MapInfo::info.mapsize_ || sMPosition_.tileIndex_ > MapInfo::info.mapsize_)
        {
            URHO3D_LOGERRORF("GOC_Destroyer() - UpdatePositions : node=%s(%u) mPoint=%s tileposition=%s tileindex=%u,%u ERROR !",
                                node_->GetName().CString(), node_->GetID(), mapWorldPosition_.mPoint_.ToString().CString(),
                                mapWorldPosition_.mPosition_.ToString().CString(), mapWorldPosition_.tileIndex_, sMPosition_.tileIndex_);
        }
    */
    // Get current Map
    if (!currentMap_ || sChangeMap_)
        currentMap_ = World2D::GetMapAt(sMPosition_.mPoint_, false);

    // no map or insufficient map status
    if (!currentMap_ || currentMap_->GetStatus() <= Creating_Map_Layers)
    {
#ifdef DUMP_UNSTUCKWARNING
        URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s viewZ=%d sChangeMap_=true currentMap_=%s TODO Unstuck !",
                           node_->GetName().CString(), node_->GetID(), sMPosition_.position_.ToString().CString(),
                           mapWorldPosition_.viewZ_, currentMap_ ? currentMap_->GetMapPoint().ToString().CString() : "0");
#endif
        /// TODO : mark node to status waitformap
        /// => when map ready, check position and unstuck entity if necessary

        sWaitForMap_ = true;
    }

    if (!sWaitForMap_)
    {
        // Check for ViewZ Changing
        if (switchViewEnable_)
        {
            // sMaps_[0] for Map (i=0)
            // Check if the entity is over a Map with tileindex changed
            sMaps_[0] = currentMap_;
            stilePositionUpdated_[0] &= (currentMap_ != 0);
            if (stilePositionUpdated_[0])
            {
                tileIndexes_[0] = sMPosition_.tileIndex_;
            }

            // sMaps_[1] for ObjectMaped (i=1)
            // Check if the entity is over an ObjectMaped with tileindex changed
            ObjectMaped::GetPhysicObjectAt(sMPosition_.position_, sMaps_[1], true);
            stilePositionUpdated_[1] = false;
            if (sMaps_[1] && sMaps_[1]->CanSwitchViewZ())
                stilePositionUpdated_[1] = ObjectMaped::GetWorldMapPositionResult().tileIndex_ != tileIndexes_[1];

            if (stilePositionUpdated_[1])
            {
                //            URHO3D_LOGINFOF("GOC_Destroyer() - GetPhysicObjectAt : objectmapedid=%d ... mPosition=%s tileindex=%u ... OK !",
                //                            ((ObjectMaped*)sMaps_[1])->GetPhysicObjectID(), ObjectMaped::GetWorldMapPositionResult().mPosition_.ToString().CString(), ObjectMaped::GetWorldMapPositionResult().tileIndex_);

                tileIndexes_[1] = ObjectMaped::GetWorldMapPositionResult().tileIndex_;
            }

            // Check for go out of the lastObjectMaped_
            bool goOutObjectMaped = !sMaps_[1] && lastObjectMaped_ && lastObjectMaped_->CanSwitchViewZ();
            stilePositionUpdated_[1] |= goOutObjectMaped;

            // Update lastobjectMaped if has changed
            if (lastObjectMaped_ != sMaps_[1])
                lastObjectMaped_ = sMaps_[1];

            int imap = 1;

            if (sMaps_[imap])
            {
                // Check current cell feature in Object Map for changing viewZ
                if (stilePositionUpdated_[imap])
                {
                    // Go Out of Object Map
                    if (goOutObjectMaped)
                    {
                        viewZToCheck_ = sMaps_[0]->FindSolidTile(tileIndexes_[0], INNERVIEW) ? INNERVIEW : FRONTVIEW;
                        tileIndexes_[imap] = 0;
                        cellCheckFeature_[imap] = MapFeatureType::NoMapFeature;
                    }
                    else
                    {
#ifdef ACTIVE_SWITCHVIEW_DOOR
                        // Door Switch View
                        lastCellCheckFeature_[imap] = cellCheckFeature_[imap];
                        cellCheckFeature_[imap] = sMaps_[imap]->GetFeatureAtZ(tileIndexes_[imap], INNERVIEW);
                        if (cellCheckFeature_[imap] != lastCellCheckFeature_[imap])
                        {
                            if (sViewZ_ == INNERVIEW && cellCheckFeature_[imap] == MapFeatureType::NoMapFeature && lastCellCheckFeature_[imap] > MapFeatureType::NoMapFeature && lastCellCheckFeature_[imap] <= MapFeatureType::Door)
                            {
                                viewZToCheck_ = sMaps_[0]->FindSolidTile(tileIndexes_[0], INNERVIEW) ? INNERVIEW : FRONTVIEW;
                            }
                            else if (cellCheckFeature_[imap] == MapFeatureType::Door || lastCellCheckFeature_[imap] == MapFeatureType::Door)
                            {
                                int checkMapFrontView = sMaps_[0]->FindSolidTile(tileIndexes_[0], INNERVIEW) ? INNERVIEW : FRONTVIEW;

                                viewZToCheck_ = cellCheckFeature_[imap] == MapFeatureType::Door ? (lastCellCheckFeature_[imap] == MapFeatureType::NoMapFeature ? INNERVIEW : checkMapFrontView) :
                                                (cellCheckFeature_[imap] != MapFeatureType::NoMapFeature ? INNERVIEW : checkMapFrontView);
                            }
                        }
#endif
                    }
                }
            }

            else if (viewZToCheck_ == NOVIEW)
            {
                imap = 0;

                // Check current cell feature for changing viewZ
                if (stilePositionUpdated_[imap])
                {
#if defined (ACTIVE_SWITCHVIEW_DOOR) || defined (ACTIVE_SWITCHVIEW_WINDOW)
                    int outerviewid = sMaps_[imap]->GetViewId(OUTERVIEW);
                    FeatureType outerfeat = outerviewid != -1 ? sMaps_[imap]->GetFeature(tileIndexes_[imap], outerviewid) : MapFeatureType::NoMapFeature;

                    if (outerfeat != MapFeatureType::Window)
                    {
#ifdef ACTIVE_SWITCHVIEW_DOOR
                        int solidtilelayerZ = FRONTVIEW;
                        lastCellCheckFeature_[imap] = cellCheckFeature_[imap];
                        bool findsolidtile = sMaps_[imap]->FindSolidTile(tileIndexes_[imap], cellCheckFeature_[imap], solidtilelayerZ);
                        // From Innerview to Frontview
                        if (sViewZ_ == INNERVIEW)
                        {
                            if (!findsolidtile || (solidtilelayerZ < INNERVIEW && lastCellCheckFeature_[imap] != MapFeatureType::Door))
                            {
                                viewZToCheck_ = FRONTVIEW;
                            }
                        }
                        // From Frontview to Innerview
                        else if (sViewZ_ == FRONTVIEW)
                        {
//                            if (findsolidtile && (solidtilelayerZ <= INNERVIEW))// || sMaps_[i]->GetFeatureAtZ(tileIndexes_[i], INNERVIEW) == MapFeatureType::NoMapFeature))
                            if (findsolidtile && (solidtilelayerZ <= INNERVIEW && lastCellCheckFeature_[imap] == MapFeatureType::NoMapFeature))
                                viewZToCheck_ = INNERVIEW;
                        }
#endif
                    }
                    else
                    {
#ifdef ACTIVE_SWITCHVIEW_WINDOW
                        viewZToCheck_ = sViewZ_ == INNERVIEW ? FRONTVIEW : INNERVIEW;
#endif
                    }
#endif
                }
            }

            // Check tiles to not be blocked during the view's switching
            if (viewZToCheck_ != NOVIEW && viewZToCheck_ != sViewZ_)
            {
                // If go out ObjectMaped select the index corresponding of the Map (i=0)
                int j = goOutObjectMaped ? 0 : imap;

                if (sMaps_[j])
                {
                    if (GameHelpers::CheckFreeTilesAtViewZ(this, sMaps_[j], tileIndexes_[j], viewZToCheck_))
                    {
                        sViewZ_ = viewZToCheck_;
                        viewZToCheck_ = NOVIEW;
                        viewZDirty_ = true;
                    }
                }
            }
        }

        // Get viewZ if not defined or if map changed
        if ((viewZToCheck_ == NOVIEW && sViewZ_ == NOVIEW) || sChangeMap_)
        {
            if (sViewZ_ == NOVIEW)
                sViewZ_ = mapWorldPosition_.viewZ_;

            // Set the corresponding ViewZ with the map
            if (currentMap_)
            {
                int realViewZ = currentMap_->GetRealViewZ(sViewZ_);
                if (realViewZ != sViewZ_)
                {
                    if (realViewZ < sViewZ_)
                        realViewZ = FRONTVIEW;

                    if (realViewZ != sViewZ_)
                    {
                        //                    URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) enable=%s realViewZ=%d (initialViewZ=%d)",
                        //                                node_->GetName().CString(), node_->GetID(), node_->IsEnabled() ? "true":"false", realViewZ, sViewZ_);
                        sViewZ_ = realViewZ;
                    }
                }
                //            URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) Send GO_CHANGEMAP mapFrom=%s mapTo=%s !",
                //                            node_->GetName().CString(), node_->GetID(),
                //                            mapWorldPosition_.mPoint_.ToString().CString(), sMPosition_.mPoint_.ToString().CString());
            }
            // No Map
            /*
                    else
                    {
                        sViewZ_ = NOVIEW;
                        ResetViewZ();
                        node_->SetEnabled(false);
                        URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdatePositions : node=%s(%u) enable=%s no Available Map=%s => ViewZ=%d!",
                                                node_->GetName().CString(), node_->GetID(), node_->IsEnabled() ? "true":"false", sMPosition_.mPoint_.ToString().CString(), mapWorldPosition_.viewZ_);
                    }
            */
        }

        if (!viewZDirty_ && switchViewEnable_)
            viewZDirty_ = sViewZ_ != NOVIEW && (sViewZ_ != mapWorldPosition_.viewZ_ || mapWorldPosition_.viewZIndex_ == NOVIEW);

        // Change ViewZ and viewID
        if (viewZDirty_)
        {
            //        URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) change %s To viewZ=%d !",
            //                        node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString(), sViewZ_);

            ViewManager::Get()->SwitchToViewZ(sViewZ_, node_);

#ifdef SWITCHVIEW_DELAY_TEST
            switchViewEnable_ = false;
#endif
            viewZDirty_ = false;
        }

        // Change CurrentCell
        if (!currentCell_ && currentMap_ && mapWorldPosition_.viewZIndex_ != NOVIEW)
        {
            currentCell_ = currentMap_->GetFluidCellPtr(mapWorldPosition_.tileIndex_, mapWorldPosition_.viewZIndex_);

            // TODO : for ObjectMaped, CurrentCell can't do the job so skip check unstuck
            if (checkUnstuck_ && worldUpdatePosition_ && currentMap_->IsVisible() && currentCell_)
                // Check if stuck in a wall
                if (!IsOnFreeTiles(mapWorldPosition_.viewZ_))
                {
#ifdef DUMP_UNSTUCKWARNING
                    URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s stuck => try to unstuck !", node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString());
#endif
                    // can't unstuck => destroy it
                    if (!Unstuck())
                    {
                        URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s can't unstuck => destroy !",
                                           node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString());
                        OnWorldEntityDestroy(WORLD_ENTITYDESTROY, context_->GetEventDataMap(false));
                        return false;
                    }
                }

            lastUnblockedTile_ = mapWorldPosition_.tileIndex_;
        }
    }

    // if var InFluid not setted, it's an initial time
    if (!node_->GetVars().Contains(GOA::INFLUID))
    {
        UpdateAreaStates(true);
        // be sure to have setted GOA::INFLUID (in case that the node has no GOC_Animator2D)
        if (!node_->GetVars().Contains(GOA::INFLUID))
            node_->SetVar(GOA::INFLUID, 0);
    }
    else
    {
        UpdateAreaStates(false);
    }

    // Change Map
    if (sChangeMap_)
    {
//        URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s(%s) map=%s ChangeMapMode=%d !", node_->GetName().CString(), node_->GetID(),
//                                        mapWorldPosition_.position_.ToString().CString(), node_->GetWorldPosition().ToString().CString(), sMPosition_.mPoint_.ToString().CString(), (int)mode);

        sOldHashPoint_ = mapWorldPosition_.mPoint_.ToHash();
        sNewHashPoint_ = sMPosition_.mPoint_.ToHash();
        mapWorldPosition_.mPoint_ = sMPosition_.mPoint_;
        node_->SetVar(GOA::ONMAP, sNewHashPoint_);

        if (mode != CHANGEMAP_NOCHANGE)
        {
            eventData[Go_ChangeMap::GO_ID] = node_->GetID();
            eventData[Go_ChangeMap::GO_TYPE] = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
            eventData[Go_ChangeMap::GO_MAPFROM] = sOldHashPoint_;
            eventData[Go_ChangeMap::GO_MAPTO] = sNewHashPoint_;
            node_->SendEvent(GO_CHANGEMAP, eventData);
        }

//        if (currentMap_ && moveEntityData_)
//        {
////            URHO3D_LOGINFOF("GOC_Destroyer() - UpdatePositions : node=%s(%u) Change MapTo=%u => AddEntityData !", node_->GetName().CString(), node_->GetID(), currentMap_);
//            currentMap_->GetMapData()->AddEntityData(node_);
//        }
    }

    /// TODO : when map ready, check position and unstuck entity if necessary
    if (sWaitForMap_)
    {
        if (node_->IsEnabled() && (!currentMap_ || !currentMap_->IsVisible()))
        {
//            URHO3D_LOGWARNINGF("GOC_Destroyer() - UpdatePositions : node=%s(%u) position=%s(%s) map=%s ChangeMapMode=%d WaitForMapReady => DISABLE node !", node_->GetName().CString(), node_->GetID(),
//                               mapWorldPosition_.position_.ToString().CString(), node_->GetWorldPosition().ToString().CString(), sMPosition_.mPoint_.ToString().CString(), (int)mode);
            node_->SetEnabledRecursive(false);
        }
    }

    return true;
}


void GOC_Destroyer::DumpWorldMapPositions()
{
    URHO3D_LOGINFOF("GOC_Destroyer() - DumpWorldMapPositions : node=%s(%u) position=%s !",
                    node_->GetName().CString(), node_->GetID(), mapWorldPosition_.ToString().CString());
}

void GOC_Destroyer::HandleUpdateWorld2D(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_PROFILE(GOC_Destroyer);
//    URHO3D_LOGINFOF("GOC_Destroyer() - HandleUpdateWorld2D : %s(%u) !", node_->GetName().CString(), node_->GetID());
    if (!UpdatePositions(eventData))
        return;

#ifdef SWITCHVIEW_DELAY_TEST
    if (!switchViewEnable_)
    {
        elapsedTimeLockView_ -= eventData[ScenePostUpdate::P_TIMESTEP].GetFloat();
        if (elapsedTimeLockView_ <= 0.f)
        {
            switchViewEnable_ = true;
            elapsedTimeLockView_ = TimeLockViewDelay;
        }
    }
#endif
}

void GOC_Destroyer::HandleUpdateTime(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Destroyer() - HandleUpdateTime : %s(%u) !", node_->GetName().CString(), node_->GetID());

//    if (elapsedTime_ <= 0.f)
//    {
//        URHO3D_LOGERRORF("GOC_Destroyer() - HandleUpdateTime : %s(%u) NoTime available !!!!", node_->GetName().CString(), node_->GetID());
//        return;
//    }

//    URHO3D_PROFILE(GOC_Destroyer);

    elapsedTime_ -= eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    if (elapsedTime_ <= 0.f)
    {
        UnsubscribeFromEvent(GetScene(), E_SCENEUPDATE);

        if (IsEnabledEffective() && lifeNotifier_)
        {
            if (!node_->GetVar(GOA::ISDEAD).GetBool() && node_->GetComponent<GOC_Life>())
            {
//                URHO3D_LOGINFOF("GOC_Destroyer() - HandleUpdateTime : %s(%u) ... TimeOver => LifeDead !", node_->GetName().CString(), node_->GetID());
                node_->SetVar(GOA::ISDEAD, true);
                eventData[GOC_Life_Events::GO_ID] = node_->GetID();
                eventData[GOC_Life_Events::GO_KILLER] = 0;
                eventData[GOC_Life_Events::GO_TYPE] = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
                node_->SendEvent(GOC_LIFEDEAD, eventData);
            }
            else
            {
//                URHO3D_LOGINFOF("GOC_Destroyer() - HandleUpdateTime : %s(%u) ... TimeOver => Send GOC_LIFEDEAD !", node_->GetName().CString(), node_->GetID());
                node_->SendEvent(GOC_LIFEDEAD);
            }
        }
        else
        {
//            URHO3D_LOGINFOF("GOC_Destroyer() - HandleUpdateTime : %s(%u) ... TimeOver => Destroy !", node_->GetName().CString(), node_->GetID());
            node_->SendEvent(WORLD_ENTITYDESTROY);
        }
    }
}

Rect GOC_Destroyer::GetWorldShapesRect() const
{
    if (shapesRect_.Defined())
        return Rect(mapWorldPosition_.position_ + shapesRect_.min_, mapWorldPosition_.position_ + shapesRect_.max_);
    else
        return Rect(mapWorldPosition_.position_ - Vector2(0.2, 0.2f), mapWorldPosition_.position_ + Vector2(0.2, 0.2f));
}

void GOC_Destroyer::DrawDebugGeometry(DebugRenderer* debugRenderer, bool depthTest) const
{
    // Add World Mass Center
    debugRenderer->AddCross(Vector3(GetWorldMassCenter(), 0.f), 1.f, Color::WHITE, depthTest);
    // Add World Shapes Rectangle
    GameHelpers::DrawDebugRect(GetWorldShapesRect(), debugRenderer, depthTest, Color::WHITE);
}
