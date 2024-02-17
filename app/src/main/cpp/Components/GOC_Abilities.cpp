#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"

#include "GOC_Abilities.h"


#define MAX_ABILITIES 8

GOC_Abilities::GOC_Abilities(Context* context) :
    Component(context),
    holder_(0)
{
}

GOC_Abilities::~GOC_Abilities()
{
}

void GOC_Abilities::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Abilities>();

    URHO3D_ACCESSOR_ATTRIBUTE("Add Ability", GetEmptyString, AddAbilityAttr, String, String::EMPTY, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Abilities", GetAbilitiesAttr, SetAbilitiesAttr, VariantVector, Variant::emptyVariantVector, AM_DEFAULT);
}

void GOC_Abilities::AddAbilityAttr(const String& value)
{
    if (value.Empty())
        return;

    String nameStr, conditionStr, gotStr;
    StringHash condition, conditionvalue;

    Vector<String> vString = value.Split('|');

    if (vString.Size() == 0)
        return;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0) continue;

        String str = s[0].Trimmed();
        if (str == "name")
            nameStr = s[1].Trimmed();
        else if (str == "got")
            gotStr = s[1].Trimmed();
        else if (str == "activeif")
        {
            conditionStr = s[1].Trimmed();
            if (!conditionStr.Empty())
            {
                Vector<String> params = conditionStr.Split('=');
                if (params.Size() > 0)
                {
                    condition = StringHash(params[0]);
                    if (params.Size() > 1)
                        conditionvalue = StringHash(params[1]);
                }
            }
        }
    }

    if (nameStr.Empty())
    {
        URHO3D_LOGERROR("GOC_Abilities() : SetAbilityAttr NOK! name Empty");
        return;
    }

//    URHO3D_LOGINFOF("GOC_Abilities() : AddAbilityAttr = %s !", value.CString());

    AddAbility(false, StringHash(nameStr), StringHash(gotStr), condition, conditionvalue);
}

void GOC_Abilities::SetAbilitiesAttr(const VariantVector& values)
{
    abilities_.Clear();

    if (values.Size() < 2)
        return;

    for (unsigned i=0; i < values.Size()-1; ++i)
        AddAbility(values[i].GetInt(), values[i+1].GetIntRect().Data());
}

VariantVector GOC_Abilities::GetAbilitiesAttr() const
{
    VariantVector attributes;

    for (HashMap<StringHash, AbilityInfo >::ConstIterator it = abilities_.Begin(); it != abilities_.End(); ++it)
    {
        const StringHash& type = it->first_;
        const AbilityInfo& abilityinfo = it->second_;
        const StringHash& got = abilityinfo.ability_->GOT_;
        const Vector<ConditionalAbilityActivation >& conditions = abilityinfo.conditionalActivations_;

        attributes.Push((int)abilityinfo.native_);
        attributes.Push(IntRect(type.Value(), got.Value(),
                                conditions.Size() > 0 ? conditions[0].condition_.Value() : 0,
                                conditions.Size() > 0 ? conditions[0].conditionValue_.Value() : 0));

        if (conditions.Size() > 1)
        {
            for (unsigned i=1; i < conditions.Size(); i++)
            {
                attributes.Push((int)abilityinfo.native_);
                attributes.Push(IntRect(type.Value(), got.Value(), conditions[i].condition_.Value(), conditions[i].conditionValue_.Value()));
            }
        }
    }

    return attributes;
}

void GOC_Abilities::AddAbility(bool native, const int* data)
{
    AddAbility(native, StringHash(data[0]), StringHash(data[1]), StringHash(data[2]), StringHash(data[3]));
}

void GOC_Abilities::AddAbility(bool native, const StringHash& type, const StringHash& got, const StringHash& condition, const StringHash& conditionValue)
{
    if (!node_ || abilities_.Size() > MAX_ABILITIES)
        return;

    bool hasAlreadyAbility = HasAbility(type);

    AbilityInfo& abilityinfo = abilities_[type];
    if (!hasAlreadyAbility || native)
    {
        abilityinfo.native_ = native;
        abilityinfo.conditionalActivations_.Clear();
    }

    if (!hasAlreadyAbility)
    {
        SharedPtr<Ability>& ability = abilityinfo.ability_;
        if (!ability)
        {
            ability = static_cast<Ability*>(context_->CreateObject(type).Get());

            if (!ability)
            {
                URHO3D_LOGERRORF("GOC_Abilities() - AddAbility : new Ability = %u ... can't create this object !", type.Value());
                return;
            }
        }

        ability->SetHolder(node_);
        ability->SetObjetType(got != StringHash::ZERO ? got : type);

        if (native && !firstNativeAbility_)
            firstNativeAbility_ = ability;

        if (holder_)
        {
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[GOA::ABILITY] = type;
            holder_->SendEvent(GO_ABILITYADDED, eventData);
        }

//        URHO3D_LOGINFOF("GOC_Abilities() - AddAbility : new Ability=%u got=%u... OK !", type.Value(), got.Value());
    }

    /// Add Conditional Activations
    if (!abilityinfo.native_ && condition != StringHash::ZERO)
    {
        Vector<ConditionalAbilityActivation >& conditions = abilityinfo.conditionalActivations_;
        conditions.Resize(conditions.Size()+1);
        ConditionalAbilityActivation& c = conditions.Back();
        c.condition_ = condition;
        c.conditionValue_ = conditionValue;
    }
}

void GOC_Abilities::RemoveAbility(const StringHash& hash)
{
    if (!HasAbility(hash))
        return;

    AbilityInfo& abilityinfo = abilities_[hash];
    if (abilityinfo.native_)
        return;

    URHO3D_LOGINFOF("GOC_Abilities() - Remove Ability %d", hash.Value());

    if (HasActiveAbility(hash))
    {
        activeAbility_->OnActive(false);
        activeAbility_.Reset();
    }

    abilities_.Erase(hash);

    if (holder_)
    {
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[GOA::ABILITY] = hash;
        holder_->SendEvent(GO_ABILITYREMOVED, eventData);
    }
}

void GOC_Abilities::ClearAbilities()
{
    URHO3D_LOGINFOF("GOC_Abilities() - ClearAbilities ...");

    abilities_.Clear();

    if (activeAbility_)
        activeAbility_->OnActive(false);

    activeAbility_.Reset();
    firstNativeAbility_.Reset();

    if (holder_)
        holder_->SendEvent(GO_ABILITYREMOVED);

    URHO3D_LOGINFOF("GOC_Abilities() - ClearAbilities ... OK!");
}

void GOC_Abilities::AddNativeAbilities()
{
    if (!node_ || !node_->GetTags().Size())
        return;

    const StringVector& tags = node_->GetTags();
    for (StringVector::ConstIterator it = tags.Begin(); it != tags.End() ; ++it)
    {
        Vector<String> abilityStr = it->Split(':');
        if (abilityStr[0] != "native")
            continue;

        Vector<String> ability = abilityStr[1].Split('|');
        if (ability.Size() == 1)
            AddAbility(true, StringHash(ability[0]));
        else if (ability.Size() == 2)
            AddAbility(true, StringHash(ability[0]), StringHash(ability[1]));
    }
}

bool GOC_Abilities::SetActiveAbility(Ability* ability)
{
    if (!node_)
        return false;

    if (ability && HasAbility(ability->GetType()))
    {
        bool succeed = true;

        if (node_)
        {
            const Vector<ConditionalAbilityActivation >& conditions = abilities_[ability->GetType()].conditionalActivations_;
            if (conditions.Size() > 0)
                succeed = false;

            for (unsigned i=0; i < conditions.Size(); i++)
            {
                const ConditionalAbilityActivation& c = conditions[i];
                StringHash cond = c.condition_;

                if (cond == GOA::COND_APPLIEDMAPPING)
                    succeed = node_->GetComponent<AnimatedSprite2D>()->IsCharacterMapApplied(c.conditionValue_);
                /// TODO : see to change String by StringHash for entityname
                else if (cond == GOA::COND_ENTITY)
                    succeed = StringHash(node_->GetComponent<AnimatedSprite2D>()->GetEntityName()) == c.conditionValue_;
                if (succeed)
                    break;
            }
        }

//        URHO3D_LOGERRORF("GOC_Abilities() - SetActiveAbility : ability=%s(%u) ... succeed = %s ",
//                        ability->GetTypeName().CString(), ability->GetType().Value(), succeed ? "true":"false");

        if (succeed)
        {
            if (activeAbility_)
                activeAbility_->OnActive(false);

            activeAbility_ = ability;
            activeAbility_->OnActive(true);
        }

        return succeed;
    }
    else
    {
        if (activeAbility_)
        {
            activeAbility_->SetHolder(node_);
            activeAbility_->OnActive(false);
        }

        activeAbility_.Reset();
    }

    return false;
}

bool GOC_Abilities::HasAbility(const StringHash& hash) const
{
    return abilities_.Find(hash) != abilities_.End();
}

bool GOC_Abilities::HasActiveAbility(const StringHash& hash) const
{
    return activeAbility_ ? activeAbility_->GetType() == hash : false;
}

Ability* GOC_Abilities::GetAbility(const StringHash& hash) const
{
    HashMap<StringHash, AbilityInfo >::ConstIterator it = abilities_.Find(hash);
    return it != abilities_.End() ? it->second_.ability_.Get() : 0;
}

void GOC_Abilities::SetNode(Node* node)
{
    node_ = node;
    OnNodeSet(node);
}

void GOC_Abilities::SetHolder(Object* holder)
{
    holder_ = holder;
}

void GOC_Abilities::OnSetEnabled()
{
}

void GOC_Abilities::OnNodeSet(Node* node)
{
}


