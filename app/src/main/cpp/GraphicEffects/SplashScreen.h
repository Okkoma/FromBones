#pragma once

namespace Urho3D
{
class Context;
class Sprite;
class Texture2D;
}

using namespace Urho3D;

/// SPLASHSCREEN_FINISH Event

URHO3D_EVENT(SPLASHSCREEN_DELAYEDSTART, SplashScreen_DelayedStart) { }
URHO3D_EVENT(SPLASHSCREEN_STOP, SplashScreen_Stop) { }
URHO3D_EVENT(SPLASHSCREEN_FINISH, SplashScreen_Finish) { }

class SplashScreen: public Object
{
    URHO3D_OBJECT(SplashScreen, Object);

public:
    SplashScreen(Context* context, Object* eventSender, const StringHash& stopTrigger, const char * fileName) ;
    ~SplashScreen();

    void Stop();

private :
    void ResizeScreen();

    void HandleDelayedStart(StringHash eventType, VariantMap& eventData);
    void HandleFinishSplash(StringHash eventType, VariantMap& eventData);
    void HandleStop(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    Sprite* splashUI;
};
