#pragma once

namespace Urho3D
{
class Node;
}

using namespace Urho3D;

URHO3D_EVENT(GO_TIMERINFORMER, Go_TimerInformer)
{
    URHO3D_PARAM(GO_SENDER, GoSender);
    URHO3D_PARAM(GO_DATA, GoData);
}

class TimerInformer : public Object
{
    URHO3D_OBJECT(TimerInformer, Object);

public :
//    TimerInformer(Context* context) : Object(context) { ; }
    TimerInformer(Node* node, float delay, unsigned userdata=0);
    ~TimerInformer();

private :
    void handleUpdateNode(StringHash eventType, VariantMap& eventData);

    SharedPtr<Node> node_;

    float timer_;
    float expirationTime_;
    unsigned userData_;
};

class DelayInformer : public Object
{
    URHO3D_OBJECT(DelayInformer, Object);

public :
    DelayInformer(Context* context);
    DelayInformer(Object* object, float delay, const StringHash& eventType);
    ~DelayInformer();

    void Start(Object* object, float delay, const StringHash& eventType);

private :
    void handleUpdate(StringHash eventType, VariantMap& eventData);

    WeakPtr<Object> object_;
    bool autodestroy_;
    float timer_;
    float expirationTime_;
    StringHash eventType_;
};

class TimerSendEvent : public Object
{
    URHO3D_OBJECT(TimerSendEvent, Object);

public :
    TimerSendEvent(Object* object, float delay, StringHash eventType, const VariantMap& eventData);
    ~TimerSendEvent();

private :
    void handleUpdate(StringHash eventType, VariantMap& eventData);

    WeakPtr<Object> object_;
    float timer_;
    float expirationTime_;
    StringHash eventType_;
    VariantMap eventData_;
};
