#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "GameOptions.h"
#include "GameEvents.h"
#include "GameAttributes.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Move2D.h"
#include "GOC_Life.h"
#include "GOC_Attack.h"
#include "GOC_ZoneEffect.h"

#include "MAN_Effects.h"


Vector<EffectInstance> EffectsManager::instances_;
Vector<EffectType> EffectsManager::bufferEffects_;
unsigned EffectsManager::ibufferEffects_ = 0;
EffectsManager* EffectsManager::effectsManager_ = 0;


EffectsManager::EffectsManager(Context* context)
    : Object(context)
{
    Init();

    effectsManager_ = this;
}

EffectsManager::~EffectsManager()
{
    effectsManager_ = 0;
}

void EffectsManager::RegisterObject(Context* context)
{
    context->RegisterFactory<EffectsManager>();
}

void EffectsManager::Init()
{
    instances_.Reserve(500);
    bufferEffects_.Resize(100);
    ibufferEffects_ = 0;
}

void EffectsManager::Start()
{
    SubscribeToEvent(GameContext::Get().rootScene_, E_SCENEUPDATE, URHO3D_HANDLER(EffectsManager, HandleSceneUpdate));
}

void EffectsManager::Stop()
{
    UnsubscribeFromAllEvents();
}

Vector<EffectInstance>::Iterator EffectsManager::GetEffectInstanceFor(GOC_ZoneEffect* zone, Node* node, EffectType* effect)
{
    Vector<EffectInstance>::Iterator it = instances_.Begin();

    while (it != instances_.End())
    {
        if (zone == it->zone_ && node == it->node_.Get() && effect == it->effect_)
            return it;

        it++;
    }

    return instances_.End();
}

static String CAC("CaC");

void EffectsManager::SetEffectsOn(Node* sender, Node* receiver, const String& triggerName, const Vector2& point)
{
    static Vector<EffectType*> effects;
    effects.Clear();

    if (sender)
    {
        GOC_Attack* attack = sender->GetComponent<GOC_Attack>();
        // for equiped weapons like swords, axes or Magic ...
        if (attack)
        {
            if (ibufferEffects_ < bufferEffects_.Size())
            {
                // Only a Base Effect applied for this attack (without equipment)
                if (triggerName.Length() >= 3)
                {
                    char elttype = triggerName.At(2);

//                    URHO3D_LOGINFOF("EffectsManager() - SetEffectsOn : %s(%u) Receive EffectElt=%c from %s(%u)!", receiver->GetName().CString(), receiver->GetID(),
//                                    elttype, sender->GetName().CString(), sender->GetID());

                    EffectType& effecttype = bufferEffects_[ibufferEffects_];
                    effecttype.effectRessource.Clear();

                    if (elttype == 'D')
                    {
                        effecttype.Set(CAC, GOA::DEATH, attack->GetDeathBaseBonus());
                        effecttype.effectRessource = "Particules/bluemagic.pex";
                        effects.Push(&effecttype);
                        ibufferEffects_ = (ibufferEffects_+1) % bufferEffects_.Size();
                    }
                    else if (elttype == 'F')
                    {
                        effecttype.Set(CAC, GOA::FIRE, attack->GetFireBaseBonus());
                        effecttype.effectRessource = "Particules/fire3.pex";
                        effects.Push(&effecttype);
                        ibufferEffects_ = (ibufferEffects_+1) % bufferEffects_.Size();
                    }
                    else if (elttype == 'L')
                    {
                        effecttype.Set(CAC, GOA::LIFE, attack->GetLifeBaseBonus());
                        effects.Push(&effecttype);
                        ibufferEffects_ = (ibufferEffects_+1) % bufferEffects_.Size();
                    }
                }

//                if (!effects.Size())
                {
                    // Add Body Attack Effects
                    const Vector<AttackProps>& attackprops = attack->GetAttackProps();
                    for (unsigned i = 0; i < attackprops.Size(); i++)
                    {
                        bufferEffects_[ibufferEffects_].Set(CAC, attackprops[i].elt_, attackprops[i].total_);
                        effects.Push(&bufferEffects_[ibufferEffects_]);
                        ibufferEffects_ = (ibufferEffects_+1) % bufferEffects_.Size();
                        //                    URHO3D_LOGINFOF("EffectsManager() - SetEffectsOn : %s(%u) Receive EffectElt=%u value=%f from %s(%u)!", receiver->GetName().CString(), receiver->GetID(),
                        //                                     attackprops[i].elt_.Value(), attackprops[i].total_, sender->GetName().CString(), sender->GetID());
                    }

                    // Add equipment effects like poison
                    PODVector<unsigned char>* equipmenteffects = attack->GetEquipmentEffects();
                    if (equipmenteffects)
                    {
                        for (unsigned i = 0; i < equipmenteffects->Size(); i++)
                        {
                            if (equipmenteffects->At(i) > 0)
                                effects.Push(EffectType::GetPtr(i));
                        }
                    }
                }
            }
        }
        else
            // for ranged weapons like bomb, lame, bullet ...
        {
            const Variant& varEffect = sender->GetVar(GOA::EFFECTID1);
            if (varEffect != Variant::EMPTY)
            {
                effects.Push(EffectType::GetPtr(varEffect.GetInt()));
//                URHO3D_LOGINFOF("EffectsManager() - SetEffectsOn : %s(%u) Receive EffectElt=%u value=%f from %s(%u)!", receiver->GetName().CString(), receiver->GetID(), effects.Back()->effectElt.Value(), effects.Back()->qtyByTicks, sender->GetName().CString(), sender->GetID());
            }
        }
    }

    if (effects.Size())
    {
//        URHO3D_LOGINFOF("EffectsManager() - SetEffectsOn : %s(%u) Receive Effects ...", receiver->GetName().CString(), receiver->GetID());

        unsigned i=0;
        bool receiveEffects = false;
        for (Vector<EffectType*>::ConstIterator it = effects.Begin(); it != effects.End(); ++it,++i)
        {
            receiveEffects |= AddEffectOn(sender, receiver, *it, point, false, false);
        }

//        URHO3D_LOGINFOF("EffectsManager() - SetEffectsOn : %s(%u) Receive Effects ... OK !", receiver->GetName().CString(), receiver->GetID());

        if (receiveEffects)
        {
            receiver->SendEvent(GO_RECEIVEEFFECT);

            // Set Invulnerability here for skip more effects in this pass !
            GOC_Life* life = receiver->GetComponent<GOC_Life>();
            if (life)
                life->SetInvulnerability(0.5f);
        }
    }
}

bool EffectsManager::AddEffectOn(Node* sender, Node* receiver, EffectType* effect, const Vector2& point, bool applyinvulnerability, bool sendevent)
{
    if (!effect)
        return false;

    bool applied = true;

    GOC_ZoneEffect* zone = sender ? sender->GetComponent<GOC_ZoneEffect>() : 0;

    if (zone && receiver->GetID() == zone->GetHolder() && !zone->GetApplyToHolder())
    {
        URHO3D_LOGINFO("EffectsManager() - AddEffectOn : Can't AddNode - Don't Apply To Holder");
        applied = false;
    }

//    if (!receiver || receiver->GetVar(GOA::ISDEAD).GetBool() || (receiver->isPoolNode_ && !receiver->HasTag("InUse")))
    if (applied && (!receiver || receiver->GetVar(GOA::ISDEAD).GetBool() || (receiver->isPoolNode_ && receiver->isInPool_)))
    {
        URHO3D_LOGERRORF("EffectsManager() - AddEffectOn : !receiver %s(%u) or in pool !", receiver ? receiver->GetName().CString() : "none", receiver ? receiver->GetID() : 0);
        applied = false;
    }

    // Apply effect immediately
    if (applied)
        applied = ApplyEffectOn(sender, receiver, effect, point, applyinvulnerability);

    if (applied)
    {
//        URHO3D_LOGINFOF("EffectsManager() - AddEffectOn : %s(%u) receive effect=%s(%u) value=%f numticks=%u", receiver->GetName().CString(), receiver->GetID(), effect->name.CString(), effect->id, effect->qtyByTicks, effect->numTicks);

        // Add an instance of the effect for the other ticks
        if (effect->numTicks > 1)
        {
            Vector<EffectInstance>::Iterator it = GetEffectInstanceFor(zone, receiver, effect);

            // reset effect tick count
            if (it != instances_.End())
            {
                it->firstCount = false;
                it->tickCount = 0;
                it->chrono = 0.f;
                it->localImpact_ = point - receiver->GetWorldPosition2D();
            }
            // add a new instance
            else
            {
                instances_.Push(EffectInstance(zone, receiver, effect, point - receiver->GetWorldPosition2D()));
                EffectInstance& instance = instances_.Back();
                instance.firstCount = false;
                if (!zone)
                    instance.outZone = true;
            }
        }
    }
//    else
//    {
//        URHO3D_LOGWARNINGF("EffectsManager() - AddEffectOn : %s(%u) => can't add effect=%s !", receiver->GetName().CString(), receiver->GetID(), effect ? effect->name.CString() : "none");
//    }

    // if zone and holder => clear slot => node to destroy
    if (zone && zone->GetApplyToHolder() && zone->GetHolder() == receiver->GetID())
    {
        zone->GetNode()->SendEvent(GO_INVENTORYEMPTY);
    }

    if (applied && sendevent)
    {
        receiver->SendEvent(GO_RECEIVEEFFECT);
    }

    return applied;
}

bool EffectsManager::ApplyEffectOn(Node* sender, Node* receiver, EffectType* effect, const Vector2& point, bool applyinvulnerability)
{
    bool applied = true;

    if (!effect)
    {
//        URHO3D_LOGINFOF("EffectsManager() - ApplyEffectOn : %s(%u) => empty effect !", receiver->GetName().CString(), receiver->GetID());
        applied = false;
    }

    if (applied && GameHelpers::IsNodeImmuneToEffect(receiver, *effect))
    {
//        URHO3D_LOGINFOF("EffectsManager() - ApplyEffectOn : %s(%u) => can't add effect, this receiver is immune !", receiver->GetName().CString(), receiver->GetID());
        applied = false;
    }

    // spawn effect
    if (applied)
    {
        // receive effect in GOC_LIFE
        GOC_Life* life = receiver->GetComponent<GOC_Life>();
        if (life && life->ReceiveEffectFrom(sender, effect->effectElt, effect->qtyByTicks * effect->qtyFactorOutZone, point, applyinvulnerability))
        {
            if (!effect->effectRessource.Empty())
            {
                // spawn effect
                float angle;
                Vector2 medianpoint = (receiver->GetWorldPosition2D() + point) / 2.f;

                GOC_Move2D* mover = receiver->GetComponent<GOC_Move2D>();
                if (mover)
                {
                    angle = -ToDegrees(atan(mover->GetLastVelocity().y_ / mover->GetLastVelocity().x_));
                }
                else
                {
                    Vector2 deltapos = receiver->GetWorldPosition2D() - point;
                    if (Abs(deltapos.x_) > Abs(deltapos.y_))
                        angle = deltapos.x_ > 0.f ? 0.f : 180.f;
                    else
                        angle = deltapos.y_ > 0.f ? 90.f : -90.f;
                }

                if (IsNaN(angle))
                    angle = 0.f;

//				URHO3D_LOGINFOF("EffectsManager() - ApplyEffectOn : node=%s(%u) effect=%s angle=%f", receiver->GetName().CString(), receiver->GetID(), effect->effectRessource.CString(), angle);

                Drawable2D* drawable = receiver->GetDerivedComponent<Drawable2D>();
                GameHelpers::SpawnParticleEffectInNode(receiver->GetContext(), receiver, effect->effectRessource, drawable->GetLayer(), drawable->GetViewMask(), medianpoint,
                                                       angle, 1.f/receiver->GetWorldScale2D().x_, true, effect->delayBetweenTicks, Color::WHITE, LOCAL);
            }
        }
//        else
//        {
//			URHO3D_LOGINFOF("EffectsManager() - ApplyEffectOn : node=%s(%u) effect=%s can't apply more life !", receiver->GetName().CString(), receiver->GetID(), effect->effectRessource.CString());
//        }
    }

    return applied;
}

Vector<EffectInstance>::Iterator EffectsManager::RemoveInstance(Vector<EffectInstance>::Iterator instanceit)
{
    if (instanceit != instances_.End())
    {
        // can switch to outzone ?
        if (!instanceit->outZone && instanceit->effect_->persistentOutZone && instanceit->tickCount < instanceit->effect_->numTicks)
        {
            // outzone effect => reset zone
            instanceit->zone_.Reset();
            instanceit->outZone = true;

//			URHO3D_LOGINFOF("EffectsManager() - RemoveInstance : node=%s(%u) has persistent outZone Effect Type=%s(%u), continue counter in outZone !",
//							instanceit->node_->GetName().CString(), instanceit->node_->GetID(), instanceit->effect_->name.CString(), instanceit->effect_->effectElt.Value());

            ++instanceit;
        }
        // erase instance
        else
        {
            if (instanceit->node_)
            {
                // if holder => item used => to destroy
                if (instanceit->zone_ && instanceit->zone_->GetApplyToHolder() && instanceit->node_->GetID() == instanceit->zone_->GetHolder())
                    instanceit->zone_->GetNode()->SendEvent(GO_INVENTORYEMPTY);

//				URHO3D_LOGINFOF("EffectsManager() - RemoveInstance : node=%s(%u) erase !", instanceit->node_->GetName().CString(), instanceit->node_->GetID());
            }

            instanceit = instances_.Erase(instanceit);
        }
    }

    return instanceit;
}

void EffectsManager::RemoveEffectOn(GOC_ZoneEffect* zone, Node* node, EffectType* effect)
{
    Vector<EffectInstance>::Iterator it = GetEffectInstanceFor(zone, node, effect);

    if (it != instances_.End())
    {
//        URHO3D_LOGINFOF("EffectsManager() - RemoveEffectOn : zone=%u node=%s(%u) ...", zone, node->GetName().CString(), node->GetID());
        it = RemoveInstance(it);
    }
}

void EffectsManager::RemoveInstancesIn(GOC_ZoneEffect* zone)
{
    Vector<EffectInstance>::Iterator it = instances_.Begin();

    while (it != instances_.End())
    {
        if (zone == it->zone_)
        {
            it = RemoveInstance(it);
        }
        else
        {
            it++;
        }
    }
}

void EffectsManager::Update(float timestep)
{
//    URHO3D_LOGINFOF("EffectsManager() : Update ...");

    // update affectedNodes list

    Vector<EffectInstance>::Iterator it = instances_.Begin();
    while (it != instances_.End())
    {
        Node* node = it->node_;
        EffectType* effect = it->effect_;

        if (!node || !effect || node->GetVar(GOA::ISDEAD).GetBool() || (node->isPoolNode_ && node->isInPool_))
        {
//        	URHO3D_LOGINFOF("EffectsManager() - Update : no more node to apply on => remove effect !");
            it = instances_.Erase(it);
            continue;
        }

        it->chrono += timestep;
        if (it->firstCount || it->chrono > effect->delayBetweenTicks)
        {
            it->firstCount = false;
            if (it->tickCount < effect->numTicks)
            {
                ++(it->tickCount);
                it->chrono = 0.f;

//                URHO3D_LOGINFOF("EffectsManager() - Update : %s(%u) tick=%d/%d - apply Effect %s (dps=%f) on %s(%u)", it->zone_ ? it->zone_->GetNode()->GetName().CString() : "MAN_Effects",
//                                it->zone_ ? it->zone_->GetNode()->GetID() : 0, it->tickCount, effect->numTicks, effect->name.CString(), effect->qtyByTicks*effect->qtyFactorOutZone ,node->GetName().CString(), node->GetID());

                if (ApplyEffectOn(it->zone_ ? it->zone_->GetNode() : 0, node, effect, node->GetWorldPosition2D() + it->localImpact_, false))
                {
                    node->SendEvent(GO_RECEIVEEFFECT);

                    // if holder => clear slot => node to destroy
                    if (it->activeForHolder)
                    {
                        if (it->tickCount == effect->numTicks)
                        {
//                            URHO3D_LOGINFO("EffectsManager() - Update : tickCount Reached for effect on holder");
                            it = RemoveInstance(it);
                            continue;
                        }
                    }
                }
                else
                {
//					URHO3D_LOGINFO("EffectsManager() - Update : can't apply effect => remove it");
                    // force the remove : no outzone allowed
                    it->outZone = true;
                    it = RemoveInstance(it);
                    continue;
                }
            }
            else
            {
//                URHO3D_LOGINFOF("EffectsManager() - Update : tickCount Reached");
                // tickcount reached : desactive effect
                it = RemoveInstance(it);
                continue;
            }
        }

        it++;
    }

//    if (instances_.Size())
//        URHO3D_LOGINFOF("EffectsManager() : Update ... numinstances=%u OK !", instances_.Size());
}

void EffectsManager::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    Update(eventData[SceneUpdate::P_TIMESTEP].GetFloat());

    EffectAction::PurgeFinishedActions();
}


void EffectsManager::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{

}
