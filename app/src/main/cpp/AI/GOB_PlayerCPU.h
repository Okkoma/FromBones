#pragma once

#include "Behavior.h"

using namespace Urho3D;

struct GOB_PlayerCPU : public Behavior
{
    URHO3D_OBJECT(GOB_PlayerCPU, Behavior);

public :
    GOB_PlayerCPU(Context* context) : Behavior(context) { }
    virtual ~GOB_PlayerCPU() { }

    virtual void Start(GOC_AIController& controller);
    virtual void Stop(GOC_AIController& controller);
    virtual void Update(GOC_AIController& controller);
};
