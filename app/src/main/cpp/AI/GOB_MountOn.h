#pragma once

#include "Behavior.h"

using namespace Urho3D;


struct GOB_MountOn : public Behavior
{
    URHO3D_OBJECT(GOB_MountOn, Behavior);

public :
    GOB_MountOn(Context* context) : Behavior(context) { }
    virtual ~GOB_MountOn() { }

    void OnMount(GOC_AIController& controller);
    void OnUnmount(GOC_AIController& controller);

    void MountOn(GOC_AIController& controller, Node* target);

    virtual void Start(GOC_AIController& controller);
    virtual void Stop(GOC_AIController& controller);
    virtual void Update(GOC_AIController& controller);
};

