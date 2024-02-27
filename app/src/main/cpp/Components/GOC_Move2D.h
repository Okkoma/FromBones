#pragma once

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "DefsMove.h"

namespace Urho3D
{
class RigidBody2D;
}

using namespace Urho3D;


class GOC_Destroyer;
class GOC_Controller;

enum Velocities
{
    vMINI,
    vWALK,
    vMAXI,
    vRUN,
    vCLIMB,
    vCLIMBMIN,
    vCLIMBMAX,
    vJUMP,
    vJUMPMIN,
    vJUMPMAX,
    vMINDOUBLEJUMP,
    vFALLMAX,
    vFALLMIN,
    vFLY,
    vFLYMAX,
    vSWIM,
    vSWIMMAX,

    NUM_VELOCITIES
};

struct GOC_Move2D_Template
{
    static void RegisterTemplate(const String& s);
    static const GOC_Move2D_Template& GetTemplate(const String& s);

    static void DumpAll();

    void Dump() const;

    String name_;
    StringHash hashName_;

    float vel_[17];

    static const GOC_Move2D_Template DEFAULT;

    static HashMap<StringHash, GOC_Move2D_Template> templates_;
};

class GOC_Move2D : public Component
{
    URHO3D_OBJECT(GOC_Move2D, Component);

public :
    GOC_Move2D(Context* context);
    virtual ~GOC_Move2D();

    static void RegisterObject(Context* context);

    void RegisterTemplate(const String& s);
    void SetTemplateName(const String& s);
    const String& GetTemplateName() const
    {
        return template_ ? template_->name_ : String::EMPTY;
    }
    const String& GetEmptyAttr() const
    {
        return String::EMPTY;
    }
    const GOC_Move2D_Template& GetTemplate() const
    {
        return *template_;
    }

    void SetMoveType(MoveTypeMode type);
    void ResetMoveType();
    void SetActiveLOS(int active);
    void SetNumJumps(int numJumps);
    void SetVehicleWheels();

    void AddFlagToMoveState(int flag)
    {
        moveStates_ = (moveStates_ | flag);
    }
    void RemoveFlagToMoveState(int flag)
    {
        moveStates_ = (moveStates_ & ~flag);
    }

    void SetPhysicEnable(bool enable, const void* control=0);
    void SetPhysicReduce(bool enable)
    {
        physicsReduced_ = enable;
    }
    bool IsPhysicEnable() const
    {
        return physicsEnable_;
    }

    MoveTypeMode GetMoveType() const
    {
        return moveType_;
    }
    MoveTypeMode GetLastMoveType() const
    {
        return lastMoveType_;
    }
    int GetActiveLOS() const
    {
        return activeLOS_;
    }
    unsigned GetActiveMaskLOS() const
    {
        return 0x0001 << activeLOS_;
    }
    int GetNumJumps() const
    {
        return numJumps_;
    }

    void GetLinearVelocity(Vector2& vel);
    const Vector2& GetLastVelocity() const
    {
        return lastvel_;
    }
    short int GetLastDirectionX() const
    {
        return lastDirectionX_;
    }
    short int GetLastDirectionY() const
    {
        return lastDirectionY_;
    }

    void Dump() const;
    void DumpPhysics() const;
    void DumpLOS() const;
    void DumpFeatures() const;

    void ReduceMove(bool reducex, bool reducey);
    void StopMove();

    void ApplyForce(const Vector2& force, const Vector2& impact, bool rotate);

    virtual void CleanDependences();
    virtual void OnSetEnabled();

    inline void SetLinearVelocity(const Vector2& vel);
    inline void SetLinearVelocityX(float velX);
    inline void SetLinearVelocityY(float velY);
    inline void ApplyForce(const Vector3& f);
    inline void ApplyForceX(float fx);
    inline void ApplyForceY(float fy);
    inline void ApplyImpulseX(float ix);
    inline void ApplyImpulseY(float iy);

    static const float heightMax;
    static const float velJumpMax;

    RigidBody2D* body;

    void OnWallContactBegin(int walltype, int wallside);
    void OnWallContactEnd(int walltype, int numgroundcontacts, int numcontacts);

protected :
    void UpdateAttributes();

    virtual void OnNodeSet(Node* node);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleControlUpdate(StringHash eventType, VariantMap& eventData);
    void HandleControlUpdate_Mount(StringHash eventType, VariantMap& eventData);
    void HandleGravityChanged(StringHash eventType, VariantMap& eventData);

private :
    void UpdateLineOfSight();

    void Update_Climb();
    bool Update_Walk();
    bool Update_Fly();
    bool Update_Swim();
    bool ControlUpdate_Ground();
    bool ControlUpdate_Air();
    bool ControlUpdate_Liquid();

    bool GoDownPlateform();

    const GOC_Move2D_Template* template_;

    GOC_Destroyer* destroyer_;
    GOC_Controller* controller_;
    WeakPtr<Node> vehicleWheels_;

    MoveTypeMode moveType_, lastMoveType_;
    unsigned buttons_;
    unsigned moveStates_, defaultStates_;
    int activeLOS_;
    short int numJumps_;
    short int numRemainJumps_;
    short int lastDirectionX_, lastDirectionY_;
    bool physicsEnable_, physicsReduced_;

    Vector2 vel_, lastvel_;
    float startjumpy_;
};

inline void GOC_Move2D::SetLinearVelocity(const Vector2& vel)
{
    body->GetBody()->SetLinearVelocity(ToB2Vec2(vel));
}

inline void GOC_Move2D::SetLinearVelocityX(float velX)
{
    b2Body* b = body->GetBody();
    b->SetLinearVelocity(b2Vec2(velX, b->GetLinearVelocity().y));
}

inline void GOC_Move2D::SetLinearVelocityY(float velY)
{
    b2Body* b = body->GetBody();
    b->SetLinearVelocity(b2Vec2(b->GetLinearVelocity().x, velY));
}

inline void GOC_Move2D::ApplyForce(const Vector3& f)
{
    if (!physicsEnable_)
        return;

    body->GetBody()->ApplyForceToCenter(ToB2Vec2(f), true);
}

inline void GOC_Move2D::ApplyForceX(float fx)
{
    if (!physicsEnable_)
        return;

//    if (node_->GetID() == 16777274)
//        URHO3D_LOGINFOF("GOC_Move2D() - ApplyForceX : %s(%u) force=%F !", node_->GetName().CString(), node_->GetID(), fx);

    body->GetBody()->ApplyForceToCenter(b2Vec2(fx, 0.f), true);
}

inline void GOC_Move2D::ApplyForceY(float fy)
{
    if (!physicsEnable_)
        return;

//    if (node_->GetID() == 16777274)
//        URHO3D_LOGINFOF("GOC_Move2D() - ApplyForceY : %s(%u) force=%F !", node_->GetName().CString(), node_->GetID(), fy);

    body->GetBody()->ApplyForceToCenter(b2Vec2(0.f, fy), true);
}

inline void GOC_Move2D::ApplyImpulseX(float ix)
{
    if (!physicsEnable_)
        return;

    body->GetBody()->ApplyLinearImpulseToCenter(b2Vec2(ix*0.5f, 0.f), true);
//    body->GetBody()->ApplyLinearImpulse(b2Vec2(ix*0.5f, 0.f), ToB2Vec2(GetNode()->GetWorldPosition2D()), true);
}

inline void GOC_Move2D::ApplyImpulseY(float iy)
{
    if (!physicsEnable_)
        return;

    body->GetBody()->ApplyLinearImpulseToCenter(b2Vec2(0.f, iy*0.5f), true);
//    body->GetBody()->ApplyLinearImpulse(b2Vec2(0.f, iy*0.5f), ToB2Vec2(GetNode()->GetWorldPosition2D()), true);
}
