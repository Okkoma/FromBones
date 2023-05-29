#pragma once

#include "Behavior.h"

using namespace Urho3D;

struct GOB_Patrol : public Behavior
{
    URHO3D_OBJECT(GOB_Patrol, Behavior);

public :
    GOB_Patrol(Context* context) : Behavior(context) { }
    virtual ~GOB_Patrol() { }

    virtual void Update(GOC_AIController& controller);
};

struct GOB_WaitAndDefend : public Behavior
{
    URHO3D_OBJECT(GOB_WaitAndDefend, Behavior);

public :
    GOB_WaitAndDefend(Context* context) : Behavior(context) { }
    virtual ~GOB_WaitAndDefend() { }

    virtual void Update(GOC_AIController& controller);
};
