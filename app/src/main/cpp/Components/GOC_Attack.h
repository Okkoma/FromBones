#pragma once

#include <Urho3D/Scene/Component.h>

using namespace Urho3D;


// manage Attack GoComponent

struct AttackProps
{
    AttackProps() : elt_(0U), base_(0.f), equipment_(0.f), total_(0.f) { }

    StringHash elt_;
    float base_;
    float equipment_;
    float total_;

    static const AttackProps EMPTY;
};

class GOC_Attack : public Component
{
    URHO3D_OBJECT(GOC_Attack, Component);

public :
    GOC_Attack(Context* context) : Component(context), effects_(0)  { }
    virtual ~GOC_Attack() { }
    static void RegisterObject(Context* context);

    void SetBaseBonus(const StringHash& elt, float value);
    void SetEquipmentBonus(const StringHash& elt, float value);
    void SetLifeBaseBonus(float value);
    void SetDeathBaseBonus(float value);
    void SetFireBaseBonus(float value);
    void SetEquipmentEffects(Vector<int>* effects)
    {
        effects_ = effects;
    }

    const AttackProps& GetProperties(const StringHash& elt) const;
    float GetBaseBonus(const StringHash& elt) const;
    float GetEquipmentBonus(const StringHash& elt) const;
    float GetDamage(const StringHash& elt) const;
    float GetLifeBaseBonus() const;
    float GetDeathBaseBonus() const;
    float GetFireBaseBonus() const;
    Vector<int>* GetEquipmentEffects() const
    {
        return effects_;
    }
    const Vector<AttackProps>& GetAttackProps() const
    {
        return props_;
    }

private :
    Vector<AttackProps>::Iterator GetProperties(const StringHash& elt);

    Vector<AttackProps> props_;
    Vector<int>* effects_;
};



