#pragma once

#include <Urho3D/Scene/Component.h>

#include "DefsGame.h"
#include "Ability.h"

namespace Urho3D
{

}

using namespace Urho3D;


struct ConditionalAbilityActivation
{
    StringHash condition_;
    StringHash conditionValue_;
};

struct AbilityInfo
{
    SharedPtr<Ability> ability_;
    bool native_;
    Vector<ConditionalAbilityActivation > conditionalActivations_;
};

class GOC_Abilities : public Component
{
    URHO3D_OBJECT(GOC_Abilities, Component);

public:

    GOC_Abilities(Context* context);
    virtual ~GOC_Abilities();

    static void RegisterObject(Context* context);

    /// Attributes
    void AddAbilityAttr(const String& abilityname);
    const String& GetEmptyString() const
    {
        return String::EMPTY;
    }
    void SetAbilitiesAttr(const VariantVector& values);
    VariantVector GetAbilitiesAttr() const;
    void SetAbilityAttr(const VariantVector& attributes);
    VariantVector GetAbilityAttr() const;

    /// Setters
    void ClearAbilities();
    void AddAbility(bool native, const int* data);
    void AddAbility(bool native, const StringHash& type, const StringHash& got=StringHash::ZERO, const StringHash& condition=StringHash::ZERO, const StringHash& conditionValue=StringHash::ZERO);
    void RemoveAbility(const StringHash& type);
    void AddNativeAbilities();

    bool SetActiveAbility(Ability* ability);

    /// Getters
    bool HasAbility(const StringHash& type) const;
    bool HasActiveAbility(const StringHash& type) const;
    Ability* GetAbility(const StringHash& type) const;
    Ability* GetActiveAbility()
    {
        return activeAbility_.Get();
    }
    unsigned GetNumAbilities() const
    {
        return abilities_.Size();
    }
    Ability* GetFirstNativeAbility() const
    {
        return firstNativeAbility_;
    }
    void SetNode(Node* node);
    void SetHolder(Object* holder);

    virtual void OnSetEnabled();

protected:

    virtual void OnNodeSet(Node* node);

private:
    HashMap<StringHash, AbilityInfo > abilities_;

    WeakPtr<Ability> activeAbility_;
    WeakPtr<Ability> firstNativeAbility_;

    Object* holder_;
};
