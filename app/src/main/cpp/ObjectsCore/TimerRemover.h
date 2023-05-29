#pragma once

#include <Urho3D/Core/Object.h>
#include <Urho3D/UI/UIElement.h>

namespace Urho3D
{
class Node;
class Component;
class UIElement;
}

using namespace Urho3D;

URHO3D_EVENT(GO_STARTTIMER, Go_StartTimer)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
    URHO3D_PARAM(GO_DATA1, GoData1);
    URHO3D_PARAM(GO_DATA2, GoData2);
}

URHO3D_EVENT(GO_ENDTIMER, Go_EndTimer)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
    URHO3D_PARAM(GO_DATA1, GoData1);
    URHO3D_PARAM(GO_DATA2, GoData2);
}

//URHO3D_EVENT(GO_REMOVEFINISH, Go_RemoveFinish)
//{
//    URHO3D_PARAM(GO_SENDER, GoSender);
//    URHO3D_PARAM(GO_DATA1, GoData1);
//    URHO3D_PARAM(GO_DATA2, GoData2);
//}

enum RemoveState
{
    FREEMEMORY,
    POOLRESTORE,
    FASTPOOLRESTORE,
    DISABLE,
    DISABLERECURSIVE,
    ENABLE,
    ENABLERECURSIVE,
    NOREMOVESTATE,
};

enum RemoveObjectType
{
    NODE,
    COMPONENT,
    UIELEMENT,
};

class TimerRemover : public Object
{
    URHO3D_OBJECT(TimerRemover, Object);

public :
    TimerRemover(Context* context) : Object(context), startok_(0) { ; }
    TimerRemover() : Object(0) { ; }
    TimerRemover(const TimerRemover& timer) : Object(timer.GetContext()) { ; }
    ~TimerRemover();

    static void Reset(Context* context=0, int size=0);
    static TimerRemover* Get();
    static void Remove(Node* object, RemoveState state=FREEMEMORY);

    void SetSendEvents(Object* sender=0, const StringHash& eventType1=GO_STARTTIMER, const StringHash& eventType2=StringHash::ZERO);

    void Start(Node* object, float delay, RemoveState state=FREEMEMORY, float delayBeforeStart = 0.f, unsigned userdata1=0, unsigned userdata2=0);
    void Start(Component* object, float delay, RemoveState state=FREEMEMORY, float delayBeforeStart = 0.f, unsigned userdata1=0, unsigned userdata2=0);
    void Start(UIElement* object, float delay, RemoveState state=FREEMEMORY, float delayBeforeStart = 0.f, unsigned userdata1=0, unsigned userdata2=0);

    void Stop(VariantMap& eventData);

    static void Dump();

private :
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    static void Free(TimerRemover* timer);

    RemoveObjectType objectType_;
    RemoveState removeState_;
    WeakPtr<Animatable> object_;
    WeakPtr<Object> sender_;

    float timer_;
    float startTime_;
    float expirationTime_;
    unsigned userData_[2];
    StringHash eventType_[2];

    bool startok_;

    static Vector<TimerRemover> timers_;
    static PODVector<TimerRemover* > freetimers_;
};
