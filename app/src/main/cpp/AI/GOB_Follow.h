#pragma once

#include "Behavior.h"

using namespace Urho3D;


struct GOB_Follow : public Behavior
{
    URHO3D_OBJECT(GOB_Follow, Behavior);

public :
    GOB_Follow(Context* context) : Behavior(context) { }
    virtual ~GOB_Follow() {  }

    virtual void Stop(GOC_AIController& controller);
    virtual void Update(GOC_AIController& controller);
};


struct GOB_FollowAttack : public Behavior
{
    URHO3D_OBJECT(GOB_FollowAttack, Behavior);

public :
    GOB_FollowAttack(Context* context) : Behavior(context) { }
    virtual ~GOB_FollowAttack() { }

    virtual void Start(GOC_AIController& controller);
    virtual void Stop(GOC_AIController& controller);
    virtual void Update(GOC_AIController& controller);
};
