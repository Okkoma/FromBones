#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "GameAttributes.h"


#include "GOC_Attack.h"

const AttackProps AttackProps::EMPTY;

void GOC_Attack::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Attack>();
    URHO3D_ACCESSOR_ATTRIBUTE("Life base bonus", GetLifeBaseBonus, SetLifeBaseBonus, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Death base bonus", GetDeathBaseBonus, SetDeathBaseBonus, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Fire base bonus", GetFireBaseBonus, SetFireBaseBonus, float, 0.f, AM_DEFAULT);
}

void GOC_Attack::SetBaseBonus(const StringHash& elt, float value)
{
    if (!elt)
        return;

    Vector<AttackProps>::Iterator props = GetProperties(elt);
    props->base_ = value;
    props->total_ = props->base_ + props->equipment_;
}

void GOC_Attack::SetEquipmentBonus(const StringHash& elt, float value)
{
    if (!elt)
        return;

    Vector<AttackProps>::Iterator props = GetProperties(elt);
    props->equipment_ = value;
    props->total_ = props->base_ + props->equipment_;

    if (!props->total_)
        props_.Erase(props);
}

void GOC_Attack::SetLifeBaseBonus(float value)
{
    SetBaseBonus(GOA::LIFE, value);
}

void GOC_Attack::SetDeathBaseBonus(float value)
{
    SetBaseBonus(GOA::DEATH, value);
}

void GOC_Attack::SetFireBaseBonus(float value)
{
    SetBaseBonus(GOA::FIRE, value);
}

const AttackProps& GOC_Attack::GetProperties(const StringHash& elt) const
{
    for (Vector<AttackProps>::ConstIterator it = props_.Begin(); it != props_.End(); ++it)
    {
        if (it->elt_ == elt)
            return *it;
    }

    return AttackProps::EMPTY;
}

Vector<AttackProps>::Iterator GOC_Attack::GetProperties(const StringHash& elt)
{
    for (Vector<AttackProps>::Iterator it = props_.Begin(); it != props_.End(); ++it)
    {
        if (it->elt_ == elt)
            return it;
    }

    // create an entry
    props_.Resize(props_.Size()+1);
    props_.Back().elt_ = elt;
    return props_.End()-1;
}

float GOC_Attack::GetBaseBonus(const StringHash& elt) const
{
    return GetProperties(elt).base_;
}

float GOC_Attack::GetEquipmentBonus(const StringHash& elt) const
{
    return GetProperties(elt).equipment_;
}

float GOC_Attack::GetDamage(const StringHash& elt) const
{
    return GetProperties(elt).total_;
}

float GOC_Attack::GetLifeBaseBonus() const
{
    return GetBaseBonus(GOA::LIFE);
}

float GOC_Attack::GetDeathBaseBonus() const
{
    return GetBaseBonus(GOA::DEATH);
}

float GOC_Attack::GetFireBaseBonus() const
{
    return GetBaseBonus(GOA::FIRE);
}


