#include <Urho3D/Urho3D.h>

#include <Urho3D/Engine/Engine.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/UI.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "TimerInformer.h"

#include "SplashScreen.h"

const float splashMinScale = 1.5f;
const float splashMaxScale = 3.f;

#define COLOREDBACKGROUND

#ifdef COLOREDBACKGROUND
static Sprite* BackUI = 0;
#endif


SplashScreen::SplashScreen(Context* context, Object* eventSender, const StringHash& finishTrigger, const char * fileName)
    : Object(context)
{
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
    URHO3D_LOGINFOF("SplashScreen() - Start : finishTrigger = %u", finishTrigger.Value());
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");

    float width = GameContext::Get().ui_->GetRoot()->GetSize().x_;
    float height = GameContext::Get().ui_->GetRoot()->GetSize().y_;
    IntVector2 size(width, height);

//    SharedPtr<Texture2D> texture = GetSubsystem<ResourceCache>()->GetTempResource<Texture2D>(fileName);
    Texture2D* texture = GetSubsystem<ResourceCache>()->GetResource<Texture2D>(fileName);

    // adjust ui size to screen keeping the aspect ratio of the texture
    GameHelpers::ApplySizeRatio(texture->GetWidth(), texture->GetHeight(), size);

    // readjust size for the initial scale
    size.x_ = ceil(size.x_ / splashMinScale) + 2;
    size.y_ = ceil(size.y_ / splashMinScale) + 2;

    UnsubscribeFromAllEvents();

#ifdef COLOREDBACKGROUND
    {
//    	SharedPtr<Texture2D> backtexture = GetSubsystem<ResourceCache>()->GetTempResource<Texture2D>("Textures/UI/white512.png");
        Texture2D* backtexture = GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/UI/white512.png");

        BackUI = GameContext::Get().ui_->GetRoot()->CreateChild<Sprite>();
        BackUI->SetTexture(backtexture);
        BackUI->SetScale(splashMinScale);
        BackUI->SetSize(size);
        BackUI->SetHotSpot(size/2);
        BackUI->SetColor(Color(0.f, 0.f, 0.f, 0.5f));
        BackUI->SetOpacity(1.0f);
        BackUI->SetPosition(width / 2.f, height / 2.f);
        BackUI->SetBlendMode(BLEND_ALPHA);
        BackUI->SetPriority(0);

        SubscribeToEvent(this, SPLASHSCREEN_DELAYEDSTART, URHO3D_HANDLER(SplashScreen, HandleDelayedStart));
        DelayInformer* delayedStart = new DelayInformer(this, 0.1f, SPLASHSCREEN_DELAYEDSTART);
    }
#endif

    {
        splashUI = GameContext::Get().ui_->GetRoot()->CreateChild<Sprite>();
        splashUI->SetTexture(texture);
        splashUI->SetScale(splashMinScale);
        splashUI->SetSize(size);
        splashUI->SetHotSpot(size/2);
        splashUI->SetOpacity(1.0f);
        splashUI->SetPosition(width / 2.f, height / 2.f);
        splashUI->SetBlendMode(BLEND_ALPHA);
        splashUI->SetPriority(1);

        SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
        scaleAnimation->SetKeyFrame(0.f, splashMinScale * Vector2::ONE);
        scaleAnimation->SetKeyFrame(1.5f, splashMaxScale * Vector2::ONE);
        scaleAnimation->SetKeyFrame(2.2f, splashMinScale * Vector2::ONE);
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_LOOP);
        splashUI->SetObjectAnimation(objectAnimation);
    }

    SubscribeToEvent(eventSender, finishTrigger, URHO3D_HANDLER(SplashScreen, HandleFinishSplash));
    SubscribeToEvent(eventSender, SPLASHSCREEN_STOP, URHO3D_HANDLER(SplashScreen, HandleStop));
    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(SplashScreen, HandleScreenResized));
}

SplashScreen::~SplashScreen()
{
    URHO3D_LOGDEBUG("~SplashScreen() - ---------------------------------------");
    URHO3D_LOGDEBUG("~SplashScreen() - End");
    URHO3D_LOGDEBUG("~SplashScreen() - ---------------------------------------");
}

void SplashScreen::ResizeScreen()
{
    if (!splashUI)
        return;

    float width = GameContext::Get().ui_->GetRoot()->GetSize().x_;
    float height = GameContext::Get().ui_->GetRoot()->GetSize().y_;
    IntVector2 size(width, height);

    Texture2D* texture = static_cast<Texture2D*>(splashUI->GetTexture());
    GameHelpers::ApplySizeRatio(texture->GetWidth(), texture->GetHeight(), size);

    // readjust size for the initial scale
    size.x_ = ceil(size.x_ / splashMinScale) + 2;
    size.y_ = ceil(size.y_ / splashMinScale) + 2;

#ifdef COLOREDBACKGROUND
    if (BackUI->GetSize().x_ != width || BackUI->GetSize().y_ != height)
    {
        BackUI->SetSize(size);
        BackUI->SetHotSpot(size/2);
        BackUI->SetPosition(width / 2.f, height / 2.f);
    }
#endif

    if (splashUI->GetSize().x_ != width || splashUI->GetSize().y_ != height)
    {
        splashUI->SetSize(size);
        splashUI->SetHotSpot(size/2);
        splashUI->SetPosition(width / 2.f, height / 2.f);
    }
}

void SplashScreen::HandleDelayedStart(StringHash eventType, VariantMap& eventData)
{
#ifdef COLOREDBACKGROUND
    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
    SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(context_));
    colorAnimation->SetKeyFrame(0.f, Color(1.0f, 0.0f, 0.f, 0.3f));
    colorAnimation->SetKeyFrame(1.1f, Color(1.0f, 0.0f, 0.f, 0.8f));
    colorAnimation->SetKeyFrame(2.2f, Color(1.0f, 0.0f, 0.f, 0.3f));
    objectAnimation->AddAttributeAnimation("Color", colorAnimation, WM_LOOP);
    BackUI->SetObjectAnimation(objectAnimation);
#endif
}

void SplashScreen::HandleFinishSplash(StringHash eventType, VariantMap& eventData)
{
#ifdef COLOREDBACKGROUND
    {
        SharedPtr<ObjectAnimation> objectAnimation(!BackUI->GetObjectAnimation() ? new ObjectAnimation(BackUI->GetContext()) : BackUI->GetObjectAnimation());
        objectAnimation->RemoveAttributeAnimation("Color");
        SharedPtr<ValueAnimation> colorAnimation(new ValueAnimation(context_));
        colorAnimation->SetKeyFrame(0.f, Color(0.f, 0.f, 0.f, 0.5f));
        colorAnimation->SetKeyFrame(0.25f, Color(0.f, 0.f, 0.f, 0.f));
        objectAnimation->AddAttributeAnimation("Color", colorAnimation, WM_ONCE);
        BackUI->SetObjectAnimation(objectAnimation);
    }
#endif

    // Create object animation.
    {
        SharedPtr<ObjectAnimation> objectAnimation(!splashUI->GetObjectAnimation() ? new ObjectAnimation(splashUI->GetContext()) : splashUI->GetObjectAnimation());
        objectAnimation->RemoveAttributeAnimation("Scale");
        objectAnimation->RemoveAttributeAnimation("Opacity");
        SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
        scaleAnimation->SetKeyFrame(0.f, splashUI->GetScale());
        scaleAnimation->SetKeyFrame(1.5f, Vector2(15.f, 15.f));
        objectAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.99f);
        alphaAnimation->SetKeyFrame(1.5f, 0.f);
        objectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        splashUI->SetObjectAnimation(objectAnimation);
    }

    UnsubscribeFromAllEvents();

    // Remove Timer on SpriteUI
    SubscribeToEvent(this, SPLASHSCREEN_STOP, URHO3D_HANDLER(SplashScreen, HandleStop));
    DelayInformer* delayedMissionStart = new DelayInformer(this, 2.5f + 0.05f, SPLASHSCREEN_STOP);

    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
    URHO3D_LOGINFO("SplashScreen() - HandleFinishSplash ! ");
    URHO3D_LOGINFO("SplashScreen() - ---------------------------------------");
}

void SplashScreen::Stop()
{
    URHO3D_LOGINFO("SplashScreen() - Stop !");

    if (splashUI)
        splashUI->Remove();

#ifdef COLOREDBACKGROUND
    if (BackUI)
        BackUI->Remove();
#endif

    UnsubscribeFromAllEvents();

    SendEvent(SPLASHSCREEN_FINISH);

    this->~SplashScreen();
}

void SplashScreen::HandleStop(StringHash eventType, VariantMap& eventData)
{
    Stop();
}

void SplashScreen::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("SplashScreen() - HandleScreenResized ...");

    ResizeScreen();

    URHO3D_LOGINFO("SplashScreen() - HandleScreenResized ... OK !");
}
