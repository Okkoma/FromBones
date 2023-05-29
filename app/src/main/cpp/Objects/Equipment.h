#pragma once

#include <Urho3D/Urho2D/SpriterData2D.h>

namespace Urho3D
{
class AnimatedSprite2D;
}

using namespace Urho3D;

class GOC_Inventory;
class GOC_Life;
class GOC_Attack;
class GOC_Abilities;
class Actor;
class Ability;

struct AnimationEquipment
{
    AnimationEquipment() : lastchange_(0) { }

    const StringHash& GetSlotHashName(const String& word);
    const Vector2& GetPositionForSlot(const StringHash& slotName) const
    {
        HashMap<StringHash, Vector2 >::ConstIterator it=slotPositions_.Find(slotName);
        return it!=slotPositions_.End() ? it->second_ : Vector2::ZERO;
    }

    bool HasSlot(const StringHash& slotName) const
    {
        HashMap<StringHash, Vector2 >::ConstIterator it=slotPositions_.Find(slotName);
        return it != slotPositions_.End();
    }
    bool HasSlots() const
    {
        return slotPositions_.Size() != 0;
    }

    bool Update(AnimatedSprite2D* animatedSprite);

    WeakPtr<AnimatedSprite2D> animatedSprite_;
    String lastentity_;
    int lastchange_;

    Vector<Spriter::BoneTimelineKey > bones_;
    HashMap<StringHash, Vector2 > slotPositions_;
};

class Equipment : public Object
{
    URHO3D_OBJECT(Equipment, Object);

public :
    Equipment(Context* context);
    virtual ~Equipment();

    static void RegisterObject(Context* context);

    void SetHolder(Actor* holder, bool forceUpdate);

    void SaveActiveAbility();
    void SetActiveAbility();

    Actor* GetHolder() const
    {
        return holder_;
    }
    Node* GetHolderNode() const;
    Variant GetEquipmentValue(unsigned index, const StringHash& attribut) const;
    AnimationEquipment& GetAnimationEquipment()
    {
        return animationEquipment_;
    }

    bool IsDirty() const
    {
        return dirty_;
    }
    void SetDirty(bool dirty)
    {
        dirty_ = dirty;
    }

    void Clear();
    void Update(bool sendNetMessage);

    void UpdateAttributes(unsigned index);
    void UpdateCharacter(unsigned i, bool sendNetMessage);

    const StringHash& GetWeapon(int weaponid) const
    {
        return weapons_[weaponid-1];
    }
    const StringHash& GetWeaponAbility(int weaponid) const
    {
        return weaponabilities_[weaponid-1];
    }

    void Dump() const;

private :
    WeakPtr<Actor> holder_;
    WeakPtr<AnimatedSprite2D> animatedSprite_;
    WeakPtr<GOC_Inventory> inventory_;
    WeakPtr<GOC_Life> life_;
    WeakPtr<GOC_Attack> attack_;
    WeakPtr<GOC_Abilities> abilities_;

    unsigned startSlotIndex_;
    unsigned endSlotIndex_;

    bool dirty_;

    /// Equipment GOT type by Slot
    Vector<StringHash> equipmentObjects_;
    /// Defense Bonus
    HashMap<StringHash, float> defenseEquipmentBonus_;
    /// Attack Bonus
    HashMap<StringHash, float> attackEquipmentBonus_;
    /// Num wearable Abilities by ability type
    HashMap<StringHash, unsigned> numEquipWithAbilities_;
    /// Num Effects by effectid (effectid=index)
    Vector<int> equipmentEffects_;

    StringHash weapons_[2];
    StringHash weaponabilities_[2];

    AnimationEquipment animationEquipment_;

    StringHash savedActiveAbilityType_;
};
