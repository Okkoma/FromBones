#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"

// Object's Components
#include "GOC_Animator2D.h"
#include "GOC_Controller.h"
#include "GOC_ControllerPlayer.h"
#include "GOC_Move2D.h"
#include "GOC_Destroyer.h"

#include "MapWorld.h"
#include "ViewManager.h"

#include "SimpleActor.h"


/// Simple Actor Class Implementation

SimpleActor::~SimpleActor()
{
    Stop();
}

void SimpleActor::RegisterObject(Context* context)
{
    context->RegisterFactory<SimpleActor>();
}

void SimpleActor::SetNameFile(const String& name)
{
    if (name_ != name)
    {
        name_ = name;

        Vector2 position = avatar_ ? avatar_->GetWorldPosition2D() : Vector2::ZERO;
        Vector2 direction = avatar_ ? avatar_->GetComponent<GOC_Animator2D>()->GetDirection() : Vector2::ZERO;

        Stop();

        ResetAvatar(position, direction);
    }
}

void SimpleActor::Start()
{
    if (!avatar_)
        return;

    avatar_->SetEnabled(true);

    avatar_->SendEvent(WORLD_ENTITYCREATE);

    URHO3D_LOGINFOF("SimpleActor() - Start !");
}

void SimpleActor::Stop()
{
    if (!avatar_)
        return;

    avatar_->SetEnabled(false);

    UnsubscribeFromAllEvents();

    avatar_->Remove();

//    scene_ = 0;

    URHO3D_LOGINFOF("SimpleActor() - Stop !");
}

void SimpleActor::SetScene(Scene* scene)
{
    scene_ = scene;

    ResetAvatar();
}

void SimpleActor::ResetMapping()
{
    AnimatedSprite2D* animatedSprite = avatar_->GetComponent<AnimatedSprite2D>();
    if (animatedSprite)
    {
        animatedSprite->ResetCharacterMapping();
        animatedSprite->ApplyCharacterMap(CMAP_NOARMOR);
        animatedSprite->ApplyCharacterMap(CMAP_NOWEAPON1);
        animatedSprite->ApplyCharacterMap(CMAP_NOWEAPON2);
    }
}

void SimpleActor::SetAnimationSet(const String& nameSet)
{
    AnimatedSprite2D* animatedSprite = avatar_->GetComponent<AnimatedSprite2D>();
    if (animatedSprite)
    {
        if (!nameSet.Empty())
        {
            URHO3D_LOGINFOF("SimpleActor() - SetAnimationSet : apply %s ...", nameSet.CString());

            AnimationSet2D* animationSet = GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>("2D/" + nameSet + ".scml");

            if (animationSet)
                animatedSprite->SetAnimationSet(animationSet);

            GOC_Animator2D* gocAnimator = avatar_->GetComponent<GOC_Animator2D>();
            if (gocAnimator)
                gocAnimator->ApplyAttributes();

            URHO3D_LOGINFOF("SimpleActor() - SetAnimationSet : apply %s ... OK !", nameSet.CString());
        }
    }

    ResetMapping();
}

void SimpleActor::ResetAvatar(const Vector2& position, const Vector2& dir)
{
    avatar_.Reset();

    if (scene_ == 0 || name_ == String::EMPTY)
        return;

    URHO3D_LOGINFOF("SimpleActor() - ResetAvatar ... ");

    avatar_ = scene_->CreateChild(name_.CString());
    nodeID_ = avatar_->GetID();

    if (GameHelpers::LoadNodeXML(context_, avatar_, name_))
    {
        UpdateComponents();

        ResetMapping();

        avatar_->SetWorldPosition2D(position);

        if (dir != Vector2::ZERO)
            avatar_->GetComponent<GOC_Animator2D>()->SetDirection(dir);

//        avatar_->SendEvent(WORLD_ENTITYCREATE);
    }

    URHO3D_LOGINFOF("SimpleActor() - ResetAvatar ... OK !");
}

void SimpleActor::UpdateComponents()
{
    URHO3D_LOGINFOF("SimpleActor() - UpdateComponents ...");

    GOC_Controller* gocController = avatar_->GetDerivedComponent<GOC_Controller>();

    if (!gocController)
        URHO3D_LOGERROR("SimpleActor() - UpdateComponents : !gocController");

    if (gocController->GetControllerType() == GO_Player)
    {
        GOC_PlayerController* gocPlayer = static_cast<GOC_PlayerController*>(gocController);
        gocPlayer->playerID = controlID_;
        gocPlayer->SetKeyControls(controlID_);
    }

    SetAnimationSet();

    GOC_Move2D* gocMove = avatar_->GetComponent<GOC_Move2D>();
    if (gocMove)
    {
        gocMove->SetActiveLOS(controlID_+1);
    }

    RigidBody2D* body = avatar_->GetComponent<RigidBody2D>();
    if (body)
    {
        body->SetAllowSleep(false);
    }

    GOC_Destroyer* gocDestroyer = avatar_->GetComponent<GOC_Destroyer>();
    if (gocDestroyer)
    {
        gocDestroyer->SetViewZ(viewManager_->GetCurrentViewZ(), 0, 1);
    }

    avatar_->SetTemporary(true);

    avatar_->SetEnabled(false);

    URHO3D_LOGINFOF("SimpleActor() - UpdateComponents ... OK !");
}
