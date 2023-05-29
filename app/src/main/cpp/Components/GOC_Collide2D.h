#pragma once

#include <Urho3D/Scene/Component.h>

#include "ShortIntVector2.h"
#include "DefsMap.h"

namespace Urho3D
{
class CollisionShape2D;
class RigidBody2D;
}

class b2Contact;

using namespace Urho3D;

struct WallContact
{
    WallContact() : type_(Wall_Ground), count_(1) { }
    WallContact(WallType type, unsigned char count) : type_(type), count_(count) { }
    WallContact(const WallContact& wc) : type_(wc.type_), count_(wc.count_) { }

    WallType type_;
    unsigned char count_;
};

#define WALLCONTACTMODE 1


class GOC_Collide2D : public Component
{
    URHO3D_OBJECT(GOC_Collide2D, Component);

public :
    static void RegisterObject(Context* context);

    GOC_Collide2D(Context* context);
    virtual ~GOC_Collide2D();

    virtual void OnSetEnabled();

    void SetCollidersEnable(bool state, unsigned contactBits, bool inverse=false);
    void ClearContacts();

#if (WALLCONTACTMODE == 0)
    const HashMap<CollisionShape2D*, unsigned>& GetContacts() const
    {
        return groundContacts;
    }
#else
    const HashMap<unsigned, WallContact>& GetContacts() const
    {
        return wallContacts;
    }
    void GetNumContacts(int (&numcontacts)[3]) const; // pass the array by reference
#endif

    bool HasAttacked(float delay=1.f) const;
    float GetLastAttackTime() const
    {
        return lastAttackTime_;
    }
    Node* GetLastAttackedNode() const
    {
        return lastAttackedNode_.Get();
    }
    void ResetLastAttack();

    void DumpContacts();

    RigidBody2D* body;

protected :
    virtual void OnNodeSet(Node* node);
#if (WALLCONTACTMODE == 0)
    void AddGroundContact2D(CollisionShape2D* cs);
    void RemoveGroundContact2D(CollisionShape2D* cs);
#elif (WALLCONTACTMODE == 1)
    void AddWallContact2D(CollisionShape2D* cs, unsigned ishape, const Vector2& normal);
    void RemoveWallContact2D(CollisionShape2D* cs, unsigned ishape);
#else
    void AddWallContact2D(b2Contact* contact, const Vector2& normal);
    void RemoveWallContact2D(b2Contact* contact);
#endif
    void HandleBeginContact(StringHash eventType, VariantMap& eventData);
    void HandleEndContact(StringHash eventType, VariantMap& eventData);
    void HandleBreakContact(StringHash eventType, VariantMap& eventData);
    void HandleDead(StringHash eventType, VariantMap& eventData);

    void OnBreakGroundContacts(StringHash eventType, VariantMap& eventData);

    bool DisableContacts();
    void ApplyForceEffect(const Vector2& impactPoint, float strength);

private :
#if (WALLCONTACTMODE == 0)
    HashMap<CollisionShape2D*, unsigned> groundContacts;
#else
    HashMap<unsigned, WallContact> wallContacts;
#endif
    int numGroundContacts_;

    float lastAttackTime_;
    WeakPtr<Node> lastAttackedNode_;
};
