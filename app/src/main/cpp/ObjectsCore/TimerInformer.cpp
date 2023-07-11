#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Node.h>

#include "GameEvents.h"

#include "TimerInformer.h"

TimerInformer::TimerInformer(Node* node, float delay, unsigned userdata)
    : Object(node->GetContext())
{
    node_ = node;
    timer_ = 0.0f;
    expirationTime_ = delay;
    userData_ = userdata;

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TimerInformer, handleUpdateNode));
    SubscribeToEvent(GAME_EXIT, URHO3D_HANDLER(TimerInformer, handleDestroy));
}

TimerInformer::~TimerInformer()
{
    URHO3D_LOGINFOF("Auto ~TimerInformer() - userData_ = %u", userData_);

    VariantMap eventData;
    eventData[Go_TimerInformer::GO_SENDER] = this;
    eventData[Go_TimerInformer::GO_DATA] = userData_;
    SendEvent(GO_TIMERINFORMER, eventData);

    UnsubscribeFromEvent(E_UPDATE);
}

void TimerInformer::handleUpdateNode(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (timer_ > expirationTime_)
        this->~TimerInformer();
}

void TimerInformer::handleDestroy(StringHash eventType, VariantMap& eventData)
{
    this->~TimerInformer();
}


DelayInformer::DelayInformer(Context* context)
    : Object(context), autodestroy_(false)
{ }

DelayInformer::DelayInformer(Object* object, float delay, const StringHash& eventType)
    : Object(object->GetContext()), autodestroy_(true)
{
    URHO3D_LOGERRORF("DelayInformer() - %u", this);
    Start(object, delay, eventType);
}

DelayInformer::~DelayInformer()
{
    URHO3D_LOGINFO("Auto ~DelayInformer()");

    if (autodestroy_ && object_)
    {
        URHO3D_LOGINFOF("Auto ~DelayInformer() : sendEvent Type=%u", eventType_.Value());
        object_->SendEvent(eventType_);
    }

    UnsubscribeFromEvent(E_UPDATE);
}

void DelayInformer::Start(Object* object, float delay, const StringHash& eventType)
{
    object_ = object;
    timer_ = 0.0f;
    expirationTime_ = delay;
    eventType_ = eventType;

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(DelayInformer, handleUpdate));
    SubscribeToEvent(GAME_EXIT, URHO3D_HANDLER(DelayInformer, handleDestroy));
}

void DelayInformer::handleUpdate(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (timer_ > expirationTime_)
    {
        if (autodestroy_)
        {
            this->~DelayInformer();
        }
        else
        {
            object_->SendEvent(eventType_);
            UnsubscribeFromEvent(E_UPDATE);
        }
    }
}

void DelayInformer::handleDestroy(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGERRORF("~DelayInformer() - %u", this);
    this->~DelayInformer();
}


TimerSendEvent::TimerSendEvent(Object* object, float delay, StringHash eventType, const VariantMap& eventData)
    : Object(object->GetContext())
{
    object_ = object;
    timer_ = 0.0f;
    expirationTime_ = delay;
    eventType_ = eventType;
    eventData_ = eventData;

    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(TimerSendEvent, handleUpdate));
    SubscribeToEvent(GAME_EXIT, URHO3D_HANDLER(TimerSendEvent, handleDestroy));
}

TimerSendEvent::~TimerSendEvent()
{
    URHO3D_LOGINFOF("Auto ~TimerSendEvent()");

    if (object_)
        object_->SendEvent(eventType_, eventData_);

    UnsubscribeFromEvent(E_UPDATE);
}

void TimerSendEvent::handleUpdate(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (timer_ > expirationTime_)
        this->~TimerSendEvent();
}

void TimerSendEvent::handleDestroy(StringHash eventType, VariantMap& eventData)
{
    this->~TimerSendEvent();
}

