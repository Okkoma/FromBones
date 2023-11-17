#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/UI/UIElement.h>

#include "DefsEffects.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"

#include "GOC_Inventory.h"
#include "GOC_Attack.h"
#include "GOC_Life.h"
#include "GOC_Abilities.h"

#include "Actor.h"

#include "Equipment.h"


/// AnimationEquipment

const char* ProfilAnimationNames[] =
{
    "profil",
};

const int ProfilAnimationNameSize = 1;

const StringHash& AnimationEquipment::GetSlotHashName(const String& word)
{
    URHO3D_LOGINFOF("AnimationEquipment() - GetSlotHashName with word=%s ...", word.CString());

    GOC_Inventory_Template::DumpBoneKeywords();

    const HashMap<StringHash, Vector<String> >& boneKeywordsBySlot = GOC_Inventory_Template::GetBoneKeywordsBySlot();
    for (HashMap<StringHash, Vector<String> >::ConstIterator it=boneKeywordsBySlot.Begin(); it!=boneKeywordsBySlot.End(); ++it)
    {
//        URHO3D_LOGINFOF("=> Check word %s with slot %u ...", word.CString(), it->first_.Value());
        const Vector<String>& keyWords = it->second_;
        for (Vector<String>::ConstIterator it2=keyWords.Begin(); it2!=keyWords.End(); ++it2)
        {
//            URHO3D_LOGINFOF("   => Check keyword %s in word %s ...", (*it2).CString(), word.CString());
            if (word.Contains(*it2))
                return it->first_;
        }
    }
    return StringHash::ZERO;
}

bool AnimationEquipment::Update(AnimatedSprite2D* animatedSprite)
{
    if (!animatedSprite)
        return false;

    if (animatedSprite == animatedSprite_ && animatedSprite->GetEntity() == lastentity_)
    {
//        URHO3D_LOGINFOF("AnimationEquipment() - Update : same animatedsprite & entity=%s for node=%s(%u)!",
//                         animatedSprite->GetEntity().CString(), animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID());
        return true;
    }

    bones_.Clear();
    slotPositions_.Clear();

    animatedSprite_ = animatedSprite;
    lastentity_ = animatedSprite_->GetEntity();

    Spriter::Animation* profil = 0;
    int i = 0;
    while (i < ProfilAnimationNameSize)
    {
        profil = animatedSprite_->GetSpriterAnimation(String(ProfilAnimationNames[i]));
        if (profil)
            break;
        i++;
    }

    if (!profil)
    {
        URHO3D_LOGERRORF("AnimationEquipment() - Update : No Animation Profil for node=%s(%u)!",
                         animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID());
        return false;
    }

    // use only the first mainkeyframe
    const PODVector<Spriter::Ref*>& objectsref = profil->mainlineKeys_[0]->objectRefs_;
    const PODVector<Spriter::Timeline*>& timelines = profil->timelines_;

    unsigned numSlots = objectsref.Size();

    if (!numSlots)
    {
        URHO3D_LOGERRORF("AnimationEquipment() - Update : No Slots for node=%s(%u)!",
                         animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID());
        return false;
    }

    /// extract the bone tree : the animation must have a bones structure
    URHO3D_LOGINFOF("AnimationEquipment() - Update : Extract bone Tree ... numSlots=%u ...", numSlots);
    for (unsigned i = 0; i < numSlots; ++i)
    {
        Spriter::Ref* objref = objectsref[i];
        Spriter::Timeline* timeline = timelines[objref->timeline_];
        const String& name = timeline->name_;
        Spriter::BoneTimelineKey* objkey = (Spriter::BoneTimelineKey*) timeline->keys_[objref->key_];

        URHO3D_LOGINFOF("  => index=%u Bone=%s parent=%d", i, name.CString(), objref->parent_);

        bones_.Push(Spriter::BoneTimelineKey(timeline));
        Spriter::BoneTimelineKey& bone = bones_.Back();
        bone = *objkey;
        if (objref->parent_ != -1)
        {
            Spriter::Ref* boneparentref = objectsref[objref->parent_];
            Spriter::BoneTimelineKey* boneparentkey = (Spriter::BoneTimelineKey*) timelines[boneparentref->timeline_]->keys_[boneparentref->key_];
            bone.info_.UnmapFromParent(boneparentkey->info_);
        }
    }
    URHO3D_LOGINFOF("AnimationEquipment() - Update : Extract bone Tree ... OK !");

    /// get the boundingbox for this animation
    BoundingBox box;
    for (Vector<Spriter::BoneTimelineKey >::Iterator it=bones_.Begin(); it!=bones_.End(); ++it)
        box.Merge(Vector2(it->info_.x_, it->info_.y_));
    Vector3 boxCenter(box.Center());
    Vector3 boxHalfSize(box.HalfSize());

    URHO3D_LOGINFOF("AnimationEquipment() - Update : Create Positions ... ");
    /// create position layout for each wearable identified equipment slot
    for (Vector<Spriter::BoneTimelineKey >::Iterator it=bones_.Begin(); it!=bones_.End(); ++it)
    {
        const String& boneName = it->timeline_->name_;
        const StringHash& hash = GetSlotHashName(boneName);

        if (hash != StringHash::ZERO)
        {
            Vector2& position = slotPositions_[hash];
            position.x_ = boxHalfSize.x_ ? (it->info_.x_ - boxCenter.x_) / boxHalfSize.x_ : (it->info_.x_ - boxCenter.x_);
            position.y_ = boxHalfSize.y_ ? (boxCenter.y_ - it->info_.y_) / boxHalfSize.y_ : (boxCenter.y_ - it->info_.y_);
            URHO3D_LOGINFOF(" => Bone=%s - Set Position = %s", boneName.CString(), position.ToString().CString());
        }
    }

    URHO3D_LOGINFOF("AnimationEquipment() - Update : Create Positions ... OK !");

    lastchange_++;
    return true;
}


/// Equipment

static Vector<StringHash> sRegisteredEffectTypes_;

Equipment::Equipment(Context* context) :
    Object(context),
    startSlotIndex_(0),
    endSlotIndex_(0),
    dirty_(false),
    animationEquipment_() { }

Equipment::~Equipment() { }

void Equipment::RegisterObject(Context* context)
{
    context->RegisterFactory<Equipment>();

    sRegisteredEffectTypes_.Push(GOA::LIFE);
    sRegisteredEffectTypes_.Push(GOA::DEATH);
    sRegisteredEffectTypes_.Push(GOA::FIRE);
}

void Equipment::SetHolder(Actor* actor, bool forceUpdate)
{
    holder_ = actor;

    if (holder_)
        abilities_ = holder_->GetAbilities();

    Node* node = GetHolderNode();
    if (node)
    {
        animatedSprite_ = node->GetComponent<AnimatedSprite2D>();
        inventory_ = node->GetComponent<GOC_Inventory>();
        life_ = node->GetComponent<GOC_Life>();
        attack_ = node->GetComponent<GOC_Attack>();
        if (attack_)
            attack_->SetEquipmentEffects(&equipmentEffects_);

        startSlotIndex_ = inventory_->GetSectionStartIndex("Equipment");
        endSlotIndex_ = inventory_->GetSectionEndIndex("Equipment");

        URHO3D_LOGINFOF("Equipment() - SetHolder : InventorySection=Equipment start=%u end=%u", startSlotIndex_, endSlotIndex_);

        equipmentObjects_.Resize(endSlotIndex_-startSlotIndex_+1);

        if (!abilities_)
            abilities_ = node->GetComponent<GOC_Abilities>();

        animationEquipment_.Update(animatedSprite_);

        if (forceUpdate)
        {
            Update(!GameContext::Get().ClientMode_);
        }

        if (holder_)
            holder_->SendEvent(GO_NODESCHANGED);
    }
}

void Equipment::SaveActiveAbility()
{
    savedActiveAbilityType_ = abilities_ && abilities_->GetActiveAbility() ? abilities_->GetActiveAbility()->GetType() : StringHash::ZERO;
    URHO3D_LOGINFOF("Equipment() - SaveActiveAbility : type=%s(%u)", savedActiveAbilityType_ ? abilities_->GetActiveAbility()->GetTypeName().CString() : "",
                    savedActiveAbilityType_ ? abilities_->GetActiveAbility()->GetType().Value() : 0);
}

void Equipment::SetActiveAbility()
{
    if (abilities_)
    {
        abilities_->SetActiveAbility(0);

        if (savedActiveAbilityType_)
        {
            if (abilities_->HasAbility(savedActiveAbilityType_))
                abilities_->SetActiveAbility(abilities_->GetAbility(savedActiveAbilityType_));
        }

        if (!abilities_->GetActiveAbility() && abilities_->GetFirstNativeAbility())
        {
            abilities_->SetActiveAbility(abilities_->GetFirstNativeAbility());
        }

        URHO3D_LOGINFOF("Equipment() - SetActiveAbility : activeAbility_=%u type=%s(%u)", abilities_->GetActiveAbility(), abilities_->GetActiveAbility() ? abilities_->GetActiveAbility()->GetTypeName().CString() : "",
                        abilities_->GetActiveAbility() ? abilities_->GetActiveAbility()->GetType().Value() : 0);

        holder_->SendEvent(GO_ABILITYADDED);
    }
}

Node* Equipment::GetHolderNode() const
{
    return holder_ ? holder_->GetAvatar() : 0;
}

//inline bool Equipment::CheckController()
//{
//    Node* node = GetHolderNode();
//
//    if (!node)
//    {
//        sendUpdatedEquip_ = false;
//        return false;
//    }
//
//    GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
//    sendUpdatedEquip_ = controller ? controller->IsMainController() : false;
//    return (controller != 0);
//}

void Equipment::UpdateAttributes(unsigned index)
{
    URHO3D_LOGINFOF("Equipment() - UpdateAttributes index=%u ...", index);

    const String& slotName = inventory_->GetSlotName(index);
    const StringHash& slothashname = inventory_->GetSlotHashName(index);

    // Search if it's a weapon slot
    int weaponindex = -1;
    if (slothashname == CMAP_WEAPON1 || slothashname == CMAP_NOWEAPON1)
        weaponindex = 0;
    else if (slothashname == CMAP_WEAPON2 || slothashname == CMAP_NOWEAPON2)
        weaponindex = 1;

    if (index > 0 && index <= equipmentObjects_.Size())
    {
        StringHash& currentEqpObject = equipmentObjects_[index-1];

        // remove old equiped Object
        if (currentEqpObject != StringHash::ZERO)
        {
            const VariantMap& currAttr = GOT::GetObject(currentEqpObject)->GetVars();
            for (VariantMap::ConstIterator ita=currAttr.Begin(); ita!=currAttr.End(); ++ita)
            {
                if (ita->second_.GetType() == VAR_FLOAT)
                {
                    // Remove Defense / Attack Bonus
                    float value = ita->second_.GetFloat();
                    if (value != 0.f)
                    {
                        float& var = value > 0.f ? defenseEquipmentBonus_[ita->first_] : attackEquipmentBonus_[ita->first_];
                        if (var != 0.f)
                            var -= value;
                    }
                }
                else if (ita->first_.Value() >= GOA::EFFECTID1.Value() && ita->first_.Value() <= GOA::EFFECTID3.Value())
                {
                    int effectid = ita->second_.GetInt();

                    if (effectid >= equipmentEffects_.Size())
                        equipmentEffects_.Resize(EffectType::GetNumRegisteredEffects());

                    if (effectid < equipmentEffects_.Size())
                    {
                        int& numeffects = equipmentEffects_[effectid];
                        if (numeffects > 0)
                            numeffects--;
                    }
                    else
                    {
                        URHO3D_LOGERRORF("Equipment() - UpdateAttributes index=%u ... error on remove effect id=%d", index, effectid);
                    }
                }
            }

            // remove Abilities
            if (abilities_)
            {
                VariantMap::ConstIterator it = currAttr.Find(GOA::ABILITY);
                if (it != currAttr.End())
                {
                    const String& name = it->second_.GetString();
                    StringHash type(name);

//                    URHO3D_LOGINFOF("Equipment() - UpdateAttributes : index=%u remove=%s(%u) ... !", index, name.CString(), type.Value());

                    if (abilities_->GetAbility(type))
                    {
                        unsigned& numAbilities = numEquipWithAbilities_[type];

//                        URHO3D_LOGINFOF("Equipment() - UpdateAttributes : index=%u remove=%s(%u) numAbilities=%u ... !", index, name.CString(), type.Value(), numAbilities);

                        if (numAbilities)
                            numAbilities--;

                        if (!numAbilities)
                            abilities_->RemoveAbility(type);

                        URHO3D_LOGINFOF("Equipment() - UpdateAttributes : index=%u remove Ability %s(%u) numEquip=%u !", index, name.CString(), type.Value(), numAbilities);
                    }
                }
            }

            // Remove Weapon Slot
            if (weaponindex != -1)
            {
                weapons_[weaponindex] = 0;
                weaponabilities_[weaponindex] = 0;
            }
        }

        currentEqpObject = StringHash::ZERO;
    }

    if (slotName.StartsWith("Special") || animationEquipment_.HasSlot(slothashname))
    {
        StringHash newEqpObject(inventory_->Get()[index].type_);
        StringHash abilityType;

        // add new equiped Object
        if (newEqpObject != StringHash::ZERO)
        {
            // Update Attribute values
            Node* node = GOT::GetObject(newEqpObject);
            if (!node)
            {
                URHO3D_LOGERRORF("Equipment() - UpdateAttributes index=%u newEqpObject=%u ... no template node !", index, newEqpObject.Value());
                return;
            }

            const VariantMap& newAttr = node->GetVars();

            for (VariantMap::ConstIterator ita = newAttr.Begin(); ita != newAttr.End(); ++ita)
            {
                if (ita->second_.GetType() == VAR_FLOAT)
                {
                    // Modify Defense / Attack Bonus
                    float value = ita->second_.GetFloat();
                    if (value != 0.f)
                    {
                        float& var = value > 0.f ? defenseEquipmentBonus_[ita->first_] : attackEquipmentBonus_[ita->first_];
                        var += ita->second_.GetFloat();
                    }
                }
                else if (ita->first_.Value() >= GOA::EFFECTID1.Value() && ita->first_.Value() <= GOA::EFFECTID3.Value())
                {
                    int effectid = ita->second_.GetInt();

                    if (effectid >= equipmentEffects_.Size())
                        equipmentEffects_.Resize(EffectType::GetNumRegisteredEffects());

                    if (effectid < equipmentEffects_.Size())
                    {
                        int& numeffects = equipmentEffects_[effectid];
                        numeffects++;
                    }
                    else
                    {
                        URHO3D_LOGERRORF("Equipment() - UpdateAttributes index=%u ... error on equip effect id=%d equipmentEffects_.Size()=%u", index, effectid, equipmentEffects_.Size());
                    }
                }
            }

            VariantMap::ConstIterator it = newAttr.Find(GOA::ABILITY);
            if (it != newAttr.End())
            {
                const String& abilityShortName = it->second_.GetString();
                abilityType = StringHash(abilityShortName);
                if (!Ability::Get(abilityType))
                    abilityType = StringHash::ZERO;
            }

            // Add Abilities
            if (abilityType && abilities_)
            {
                Ability* ability = abilities_->GetAbility(abilityType);
//                URHO3D_LOGINFOF("Equipment() - UpdateAttributes : index=%u ... object has ability=%s(%u) !", index, abilityName.CString(), abilityType.Value());

                unsigned& numAbilities = numEquipWithAbilities_[abilityType];

                if (!ability)
                {
                    const String& abilityName = Ability::Get(abilityType)->GetTypeName();
                    if (holder_->HasTag("wearable:" + abilityName))
                    {
                        if (!numAbilities)
                        {
                            abilities_->AddAbility(false, abilityType);
                            abilities_->SetActiveAbility(abilities_->GetAbility(abilityType));
                            numAbilities = 1;
                        }

                        URHO3D_LOGINFOF("Equipment() - UpdateAttributes : index=%u Add Ability %s(%u) numEquip=%u", index, abilityName.CString(), abilityType.Value(), numAbilities);
                    }
                }
                else
                {
                    numAbilities++;
                    ability->Update();
                    URHO3D_LOGINFOF("Equipment() - UpdateAttributes : index=%u Update Ability %s(%u) numEquip=%u", index, ability->GetTypeName().CString(), abilityType.Value(), numAbilities);
                }
            }

        }

        if (index > 0 && index <= equipmentObjects_.Size())
        {
            equipmentObjects_[index-1] = newEqpObject;

            // Update Weapon Slot
            if (weaponindex != -1)
            {
                weapons_[weaponindex] = newEqpObject;
                weaponabilities_[weaponindex] = abilityType;
            }
        }
    }

    // Update Bonus Attack & Defense

    for (unsigned i = 0; i < sRegisteredEffectTypes_.Size(); i++)
    {
        const StringHash& type = sRegisteredEffectTypes_[i];

        if (life_)
        {
            HashMap<StringHash, float>::ConstIterator it = defenseEquipmentBonus_.Find(type);
            life_->SetBonusDefense(type, it != defenseEquipmentBonus_.End() ? it->second_ : 0.f);
        }

        if (attack_)
        {
            HashMap<StringHash, float>::ConstIterator it = attackEquipmentBonus_.Find(type);
            attack_->SetEquipmentBonus(type, it != attackEquipmentBonus_.End() ? it->second_ : 0.f);
        }
    }

    URHO3D_LOGINFOF("Equipment() - UpdateAttributes index=%u ... OK !", index);
}

void Equipment::UpdateCharacter(unsigned slotIndex, bool sendNetMessage)
{
    Node* node = GetHolderNode();
    if (!node || !animatedSprite_)
        return;

    URHO3D_LOGINFOF("Equipment() - UpdateCharacter slotIndex=%u ... sendnetmess=%s maincontroller=%s",
                    slotIndex, sendNetMessage ? "true":"false", holder_->IsMainController() ? "true":"false");

    if (inventory_->Get().Size() <= slotIndex)
    {
        URHO3D_LOGERRORF("Equipment() - UpdateCharacter slotIndex=%u inventorySize=%u ! ", slotIndex, inventory_->Get().Size());
        return;
    }

    URHO3D_LOGINFOF("Equipment() - UpdateCharacter slotIndex=%u ... OK !", slotIndex);

    GOC_Inventory::LocalEquipSlotOn(inventory_, slotIndex, animatedSprite_, holder_->IsMainController() && sendNetMessage);
}

void Equipment::Clear()
{
    URHO3D_LOGINFO("Equipment() - Clear ...");

    Node* node = GetHolderNode();

    if (abilities_)
        abilities_->ClearAbilities();

    if (node && animatedSprite_)
    {
        inventory_ = node->GetComponent<GOC_Inventory>();
        for (unsigned i = startSlotIndex_; i <= endSlotIndex_; i++)
            UpdateCharacter(i, false);
    }

    numEquipWithAbilities_.Clear();
    equipmentObjects_.Clear();
    equipmentObjects_.Resize(endSlotIndex_-startSlotIndex_+1);
    defenseEquipmentBonus_.Clear();
    attackEquipmentBonus_.Clear();

    for (int i=0; i < 2; i++)
    {
        weapons_[i] = 0;
        weaponabilities_[i] = 0;
    }

    URHO3D_LOGERRORF("Equipment() - Clear ... OK !");
}

void Equipment::Update(bool sendNetMessage)
{
    URHO3D_LOGINFO("Equipment() - Update ...");

    Node* node = GetHolderNode();
    if (!node)
        return;

    numEquipWithAbilities_.Clear();

    if (abilities_)
    {
        abilities_->ClearAbilities();
        abilities_->AddNativeAbilities();
    }

    for (unsigned i = startSlotIndex_; i <= endSlotIndex_; i++)
    {
        UpdateCharacter(i, sendNetMessage);
        UpdateAttributes(i);
    }

    dirty_ = false;

    SendEvent(GO_EQUIPMENTUPDATED);
    holder_->SendEvent(GO_ABILITYADDED);

    URHO3D_LOGINFO("Equipment() - Update ... OK !");
}

void Equipment::Dump() const
{
    String attackStr, defenseStr;

    const Vector<AttackProps>& attackprops = attack_->GetAttackProps();
    for (unsigned i = 0; i < attackprops.Size(); i++)
    {
        const AttackProps& props = attackprops[i];
        float value = props.total_;
        if (value)
        {
            attackStr += String(props.elt_.Value());
            attackStr += "=";
            attackStr += String(value) + "(base=" + String(props.base_) + ")";
            attackStr += " ";
        }
    }

    for (unsigned i = 0; i < sRegisteredEffectTypes_.Size(); i++)
    {
        float value = life_->GetBonusDefense(sRegisteredEffectTypes_[i]);
        if (value)
        {
            defenseStr += String(sRegisteredEffectTypes_[i].Value());
            defenseStr += "=";
            defenseStr += String(value);
            defenseStr += " ";
        }
    }

    URHO3D_LOGINFOF("Equipment() - Dump : TOTAL def=(%s) att=(%s)", defenseStr.CString(), attackStr.CString());

    for (unsigned i = startSlotIndex_; i <= endSlotIndex_; i++)
    {
        const StringHash& type = equipmentObjects_[i-1];

        if (!type)
            continue;

        String valueStr;

        const VariantMap& attributes = GOT::GetObject(type)->GetVars();
        for (unsigned j = 0; j < sRegisteredEffectTypes_.Size(); j++)
        {
            VariantMap::ConstIterator it = attributes.Find(sRegisteredEffectTypes_[j]);
            if (it != attributes.End())
            {
                valueStr += String(sRegisteredEffectTypes_[j].Value());
                valueStr += "=";
                valueStr += String(it->second_.GetFloat());
                valueStr += " ";
            }
        }

        URHO3D_LOGINFOF("Equipment() - slot[%u] - Object=%s(%u) %s", i, GOT::GetType(type).CString(), type.Value(), valueStr.CString());
    }

    for (int i=0; i < 2; i++)
        URHO3D_LOGINFOF("Equipment() - weapon[%d]=%u ability=%u", i, weapons_[i].Value(), weaponabilities_[i].Value());
}

Variant Equipment::GetEquipmentValue(unsigned index, const StringHash& attribut) const
{
    StringHash type = equipmentObjects_[index-1];
    if (type == StringHash::ZERO)
    {
        URHO3D_LOGWARNING("Equipment() - GetEquipmentValue : return Empty Variant !");
        return Variant::EMPTY;
    }

    const VariantMap& attr = GOT::GetObject(type)->GetVars();
    VariantMap::ConstIterator it=attr.Find(attribut);

    if (it != attr.End())
    {
        URHO3D_LOGINFOF("Equipment() - GetEquipmentValue : index=%u attribut=%s(%u) item=%s => return Variant type=%u",
                        index, GOA::GetAttributeName(attribut).CString(), attribut.Value(), GOT::GetType(type).CString(), it->second_.GetType());
        return it->second_;
    }

    URHO3D_LOGWARNINGF("Equipment() - GetEquipmentValue : index=%u attribut=%s(%u) item=%s => return Empty Variant !",
                       index, GOA::GetAttributeName(attribut).CString(), attribut.Value(), GOT::GetType(type).CString());
    return Variant::EMPTY;
}
