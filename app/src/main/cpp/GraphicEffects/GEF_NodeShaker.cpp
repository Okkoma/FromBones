#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "GEF_NodeShaker.h"



/// GEF_NodeShaker


void GEF_NodeShaker::RegisterObject(Context* context)
{
    context->RegisterFactory<GEF_NodeShaker>();
}

GEF_NodeShaker::GEF_NodeShaker(Context* context) :
    Component(context)
{ }

GEF_NodeShaker::~GEF_NodeShaker()
{ }

void GEF_NodeShaker::SetNumShakes(int numshakes)
{
    numShakes_ = numshakes;
}

void GEF_NodeShaker::SetDuration(float duration)
{
    duration_ = duration;
}

void GEF_NodeShaker::SetAmplitude(const Vector2& amplitude)
{
    amplitude_ = amplitude;
}

void GEF_NodeShaker::Start()
{
    ishake_ = numShakes_;
    shakedelay_ = duration_ / numShakes_;
    delay_ = shakedelay_;

    SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(GEF_NodeShaker, HandleUpdate));
}
/*
void GEF_NodeShaker::OnSetEnabled()
{
    Component::OnSetEnabled();

    Scene* scene = GetScene();
    if (scene && IsEnabledEffective())
    {
        URHO3D_LOGINFOF("GEF_NodeShaker() - this=%u OnSetEnabled = true", this);
        Start();
    }
    else
    {
        URHO3D_LOGINFOF("GEF_NodeShaker() - this=%u OnSetEnabled = false", this);
        UnsubscribeFromAllEvents();
    }
}
*/
void GEF_NodeShaker::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    //URHO3D_LOGINFOF("GEF_NodeShaker() : HandleUpdate");

    float timestep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    delay_ -= timestep;

    if (delay_ <= 0.f)
    {
        delay_ = shakedelay_;
        ishake_--;
    }

    if (!ishake_)
    {
        UnsubscribeFromEvent(E_SCENEPOSTUPDATE);
        return;
    }

    if (ishake_%2)
        node_->Translate2D(amplitude_ * timestep);
    else
        node_->Translate2D(-amplitude_ * timestep);
}
