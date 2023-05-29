#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Component.h>
#include <Urho3D/UI/UIElement.h>

#include "GO_Pool.h"
#include "ObjectPool.h"

#include "GameContext.h"

#include "TimerRemover.h"


StringHash HandleUpdateEvent = E_POSTUPDATE;
//StringHash HandleUpdateEvent = E_SCENEUPDATE;

Vector<TimerRemover> TimerRemover::timers_;
PODVector<TimerRemover* > TimerRemover::freetimers_;

const char* RemoveStateNames[] =
{
    "FREEMEMORY",
    "POOLRESTORE",
    "FASTPOOLRESTORE",
    "DISABLE",
    "ENABLE",
    "NOREMOVESTATE",
};

void TimerRemover::Reset(Context* context, int size)
{
    timers_.Clear();
    freetimers_.Clear();

    if (size > 0)
    {
        for (int i=0; i < size; i++)
            timers_.Push(TimerRemover(context));

        freetimers_.Reserve(size);

        for (int i=0; i < size; i++)
            freetimers_.Push(&timers_[i]);
    }
}

TimerRemover* TimerRemover::Get()
{
    if (!freetimers_.Size())
    {
        URHO3D_LOGWARNINGF("TimerRemover() - Get : not enough free timer => Create new !!!");
        timers_.Push(TimerRemover(GameContext::Get().context_));
        return &timers_.Back();
    }

    TimerRemover* timer = freetimers_.Back();
    freetimers_.Pop();

//	URHO3D_LOGINFOF("TimerRemover() - Get : free=%u/%u", freetimers_.Size(), timers_.Size());

    return timer;
}

void TimerRemover::Remove(Node* node, RemoveState removeState)
{
    if (removeState == FREEMEMORY)
        node->Remove();
    else if (removeState == POOLRESTORE)
        ObjectPool::Free(node, true);
    else if (removeState == FASTPOOLRESTORE)
        GO_Pools::Free(node);
    else if (removeState == ENABLE)
        node->SetEnabled(true);
    else if (removeState == DISABLE)
        node->SetEnabled(false);
}

void TimerRemover::Free(TimerRemover* timer)
{
    freetimers_.Push(timer);

//    URHO3D_LOGINFOF("TimerRemover() - Free : free=%u/%u", freetimers_.Size(), timers_.Size());
}

TimerRemover::~TimerRemover()
{

}


void TimerRemover::Start(Node* object, float delay, RemoveState state, float delayBeforeStart, unsigned userdata1, unsigned userdata2)
{
    if (object)
    {
        objectType_ = NODE;
        removeState_ = state;
        object_ = WeakPtr<Animatable>(static_cast<Animatable*>(object));
    }

    userData_[0] = userdata1;
    userData_[1] = userdata2;

    startok_ = (delayBeforeStart + delay == 0.f);

    if (startok_)
    {
        timer_ = 0.1f;
        Stop(context_->GetEventDataMap());
    }
    else
    {
        timer_ = 0.0f;
        startTime_ = delayBeforeStart;
        expirationTime_ = delay + startTime_;
        SubscribeToEvent(HandleUpdateEvent, URHO3D_HANDLER(TimerRemover, HandleUpdate));
    }
}

void TimerRemover::Start(Component* object, float delay, RemoveState state, float delayBeforeStart, unsigned userdata1, unsigned userdata2)
{
    if (object)
    {
        objectType_ = COMPONENT;
        removeState_ = state;
        object_ = WeakPtr<Animatable>(static_cast<Animatable*>(object));
    }

    timer_ = 0.f;
    startTime_ = delayBeforeStart;
    expirationTime_ = delay + startTime_;
    userData_[0] = userdata1;
    userData_[1] = userdata2;

    startok_ = false;

    SubscribeToEvent(HandleUpdateEvent, URHO3D_HANDLER(TimerRemover, HandleUpdate));
}

void TimerRemover::Start(UIElement* object, float delay, RemoveState state, float delayBeforeStart, unsigned userdata1, unsigned userdata2)
{
    if (object)
    {
        objectType_ = UIELEMENT;
        removeState_ = state;
        object_ = WeakPtr<Animatable>(static_cast<Animatable*>(object));
    }

    timer_ = 0.f;
    startTime_ = delayBeforeStart;
    expirationTime_ = delay + startTime_;
    userData_[0] = userdata1;
    userData_[1] = userdata2;

    startok_ = false;

    SubscribeToEvent(HandleUpdateEvent, URHO3D_HANDLER(TimerRemover, HandleUpdate));
}


void TimerRemover::SetSendEvents(Object* sender, const StringHash& eventType1, const StringHash& eventType2)
{
    if (sender)
    {
        sender_ = sender;
        eventType_[0] = eventType1;
        eventType_[1] = eventType2;
    }
}

void TimerRemover::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[Update::P_TIMESTEP].GetFloat();

    if (!startok_)
    {
        if (timer_ > startTime_)
        {
            if (sender_ && eventType_[0])
            {
                eventData[Go_StartTimer::GO_SENDER] = (void*)object_.Get(); // use (void ptr) for variant assignation to skip weakptr creation, we just need a number to keep track
                eventData[Go_StartTimer::GO_DATA1] = userData_[0];
                eventData[Go_StartTimer::GO_DATA2] = userData_[1];
//                URHO3D_LOGINFOF("TimerRemover() - HandleUpdate : this=%u SendEvent1=%u ...", this, eventType_[0].Value());
                sender_->SendEvent(eventType_[0], eventData);
                eventType_[0] = 0;
            }

            startok_ = true;
        }
    }

    if (timer_ > expirationTime_)
    {
//        URHO3D_LOGINFOF("TimerRemover() - HandleUpdate : Stop this=%u ...", this);
        Stop(eventData);
    }
}

void TimerRemover::Stop(VariantMap& eventData)
{
    UnsubscribeFromEvent(HandleUpdateEvent);

    if (sender_ && eventType_[1])
    {
        eventData[Go_EndTimer::GO_SENDER] = (void*)sender_.Get(); // use (void ptr) for variant assignation to skip weakptr creation, we just need a number to keep track
        eventData[Go_EndTimer::GO_DATA1] = userData_[0];
        eventData[Go_EndTimer::GO_DATA2] = userData_[1];

//        URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u SendEvent2=%u ...", this, eventType_[1].Value());
        sender_->SendEvent(eventType_[1], eventData);
        eventType_[1] = 0;
        sender_.Reset();
    }

    if (object_)
    {
        if (objectType_ == NODE)
        {
            Node* node = static_cast<Node*>(object_.Get());

            //        URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u sender=%u nodeid=%u removestate=%s(%d)",
            //                        this, sender_.Get(), node->GetID(), RemoveStateNames[removeState_], (int)removeState_);
            if (removeState_ == FREEMEMORY)
                node->Remove();
            else if (removeState_ == POOLRESTORE)
                ObjectPool::Free(node, true);
            else if (removeState_ == FASTPOOLRESTORE)
                GO_Pools::Free(node);
            else if (removeState_ == ENABLE)
                node->SetEnabled(true);
            else if (removeState_ == ENABLERECURSIVE)
                node->SetEnabledRecursive(true);
            else if (removeState_ == DISABLE)
                node->SetEnabled(false);
            else if (removeState_ == DISABLERECURSIVE)
                node->SetEnabledRecursive(false);
        }
        else if (objectType_ == COMPONENT)
        {
            Component* component = static_cast<Component*>(object_.Get());
            //        URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u sender=%u componentptr=%u removestate=%s(%d)", this, sender_.Get(), node->GetID(), RemoveStateNames[removeState_], removeState_);
            if (removeState_ == ENABLE)
                component->SetEnabled(true);
            else if (removeState_ == DISABLE)
                component->SetEnabled(false);
        }
        else
        {
            UIElement* uielt = static_cast<UIElement*>(object_.Get());
            //        URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u sender=%u uielt=%u removestate=%s(%d)...", this, sender_.Get(), uielt, RemoveStateNames[removeState_], removeState_);
            if (removeState_ == FREEMEMORY)
                uielt->Remove();
            else if (removeState_ == ENABLE)
            {
                uielt->SetEnabled(true);
                uielt->SetVisible(true);
            }
            else if (removeState_ == DISABLE)
            {
                uielt->SetEnabled(false);
                uielt->SetVisible(false);
            }
        }
    }

//    URHO3D_LOGINFOF("TimerRemover() - Stop() : this=%u ... OK !", this);
    object_.Reset();
    TimerRemover::Free(this);
}


void TimerRemover::Dump()
{
    URHO3D_LOGINFOF("TimerRemover() - Dump : free=%u/%u", freetimers_.Size(), timers_.Size());
}

