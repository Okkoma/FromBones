#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "DefsMove.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "ViewManager.h"

#include "GOC_Animator2D.h"
#include "GOC_Controller.h"

#include "GOC_Life.h"


/// LifeProps

LifeProps::LifeProps(float e, int l, float i) :
    energy(e),
    life(l),
    totalDpsReceived(0.f),
    invulnerability(i)
{ }


/// GOC_Life_Template

HashMap<StringHash, float> GOC_Life_Template::sImmunities_;

HashMap<StringHash, GOC_Life_Template> GOC_Life_Template::templates_;

const StringHash LIFE_TEMPLATE_DEFAULT			   = StringHash("LifeTemplate_Default");
const GOC_Life_Template GOC_Life_Template::DEFAULT = GOC_Life_Template("LifeTemplate_Default", 0, 3.f, 2);

GOC_Life_Template::GOC_Life_Template() :
    energyMax(1.f), lifeMax(1) { }

//GOC_Life_Template::GOC_Life_Template(const GOC_Life_Template& t) :
//    name(t.name),
//    hashName(t.hashName),
//    baseTemplate(t.baseTemplate),
//    energyMax(t.energyMax),
//    lifeMax(t.lifeMax)
//{
//    ;
//}

GOC_Life_Template::GOC_Life_Template(const String& n, GOC_Life_Template* base, float e, int l) :
    name(n),
    hashName(StringHash(n)),
    energyMax(e),
    lifeMax(l)
{
    if (base)
        baseTemplate = base->hashName;
    else
        baseTemplate = 0;
}

void GOC_Life_Template::RegisterTemplate(const String& s, const GOC_Life_Template& t)
{
    URHO3D_LOGINFOF("GOC_Life_Template() - RegisterTemplate %s ...", s.CString());

    StringHash key = StringHash(s);

    if (templates_.Contains(key))
        return;

    templates_[key] = t;
    //templates_ += Pair<StringHash, GOC_Life_Template>(key, t);
    templates_[key].name = s;
    templates_[key].hashName = StringHash(key);
    templates_[key].immunities_ = sImmunities_;

    sImmunities_.Clear();

    URHO3D_LOGINFOF("GOC_Life_Template() - RegisterTemplate templates_ size=%u", templates_.Size());
}

void GOC_Life_Template::DumpAll()
{
    unsigned index = 0;
    for (HashMap<StringHash, GOC_Life_Template>::ConstIterator it=templates_.Begin(); it!=templates_.End(); ++it)
    {
        URHO3D_LOGINFOF("GOC_Life_Template() - DumpAll - templates_[%u]",index);
        it->second_.Dump();
        ++index;
    }
}

void GOC_Life_Template::Dump() const
{
    URHO3D_LOGINFOF("GOC_Life_Template() - Dump - name=%s hash=%u base=%u - lifeMax=%d energyMax=%f",
                    name.CString(), hashName.Value(), baseTemplate.Value(), lifeMax, energyMax);
}




/// GOC_Life Implementation

GOC_Life::GOC_Life(Context* context) :
    Component(context),
//	customTemplate(false),
    currentTemplate(0),
    template_(0U),
    haloInvulnerability_(0),
    killer(0),
    props(0.f, 0, 0.f),
    updateProps_(false),
    waitServerReset_(false)
{
    ;
}

GOC_Life::~GOC_Life()
{
    SetLifeBars(false);
    UnsubscribeFromAllEvents();
//    ClearCustomTemplate();
}

void GOC_Life::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Life>();

    URHO3D_ACCESSOR_ATTRIBUTE("Register Template", GetEmptyAttr, RegisterTemplate, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Template", GetTemplateName, SetTemplate, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Energy Lost", GetTotalEnergyLost, SetTotalEnergyLost, float, 0.f, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Life", GetLife, SetLife, int, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Energy", GetEnergy, SetEnergy, float, 0.f, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Immunity", GetEmptyAttr, AddTemplateImmunity, String, String::EMPTY, AM_FILE);
//    URHO3D_ACCESSOR_ATTRIBUTE("Bonus Defense", GetBonusDefense, SetBonusDefense, float, 0.f, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Invulnerability", GetInvulnerability, SetInvulnerability, float, 2.f, AM_FILE);

    GOC_Life_Template::templates_.Clear();
    GOC_Life_Template::RegisterTemplate("LifeTemplate_Default", GOC_Life_Template::DEFAULT);
}

void GOC_Life::RegisterTemplate(const String& s)
{
    if (s.Empty())
        return;

    URHO3D_LOGINFOF("GOC_Life() - RegisterTemplate name=%s", s.CString());

//    GOC_Life_Template t = GOC_Life_Template(s, currentTemplate, props.energy, props.life);
    GOC_Life_Template::RegisterTemplate(s, GOC_Life_Template(s, currentTemplate, props.energy, props.life));
}

bool GOC_Life::CheckChangeTemplate(const String& newTemplateName)
{
    if (newTemplateName.Empty())
        return false;

    StringHash key(newTemplateName);
    GOC_Life_Template* newTemplate = GOC_Life_Template::templates_.Contains(key) ? &(GOC_Life_Template::templates_.Find(key)->second_) : 0;

    if (!newTemplate)
        return false;

    if (props.totalDpsReceived > newTemplate->energyMax * (float)newTemplate->lifeMax)
        return false;

    return true;
}

void GOC_Life::ValidateMarkNetworkUpdate()
{
    if (!GameContext::Get().ServerMode_)
        return;

    GOC_Controller* gocController = node_->GetDerivedComponent<GOC_Controller>();
    if (!gocController)
        return;

    if (!gocController->IsMainController())
        MarkNetworkUpdate();
}

void GOC_Life::SetTemplate(const String& s)
{
    if (s.Empty())
        return;

//    URHO3D_LOGINFOF("GOC_Life() - SetTemplate=%s ...", s.CString());

    if (ApplyTemplateProperties(StringHash(s)))
    {
        ValidateMarkNetworkUpdate();

//        URHO3D_LOGINFOF("GOC_Life() - SetTemplate=%s(%u) ... OK !", s.CString());
    }
}

void GOC_Life::SetTotalEnergyLost(float newtdps)
{
    if (GameContext::Get().ClientMode_ && waitServerReset_)
    {
        if (newtdps > GetTemplateMaxEnergy())
        {
//            URHO3D_LOGINFOF("GOC_Life() - SetTotalEnergyLost : %s(%u) newtdps=%f waiting ... for server reset !",
//                            node_->GetName().CString(), node_->GetID(), newtdps);
            return;
        }

        waitServerReset_ = false;
    }

    if (!newtdps || Abs(props.totalDpsReceived - newtdps) < 0.001f || GetLife() <= 0)
        return;

    URHO3D_LOGINFOF("GOC_Life() - SetTotalEnergyLost : %s(%u) totalDpsReceived=%f", node_->GetName().CString(), node_->GetID(), newtdps);

    if (GameContext::Get().ClientMode_)
    {
        // on client never play animation HURT here (when setting amount) because it was done before in HandleReceiveEffect
//        ApplyAmountEnergy(props.totalDpsReceived - newtdps, false);
        ApplyAmountEnergy(props.totalDpsReceived - newtdps);
    }
    else
    {
        SetEnergyLost(newtdps);
    }
}

void GOC_Life::SetEnergyLost(float e)
{
    props.totalDpsReceived = e;
    updateProps_ = true;

//    URHO3D_LOGINFOF("GOC_Life() - SetEnergyLost : %s(%u) total energy lost = %f !", node_->GetName().CString(), node_->GetID(), props.totalDpsReceived);

    ValidateMarkNetworkUpdate();
}

void GOC_Life::SetLife(int l)
{
    if (currentTemplate)
        l = Min(l, currentTemplate->lifeMax);

    if (props.life == l)
        return;

//    URHO3D_LOGINFOF("GOC_Life() - SetLife : %s(%u) life=%d", node_->GetName().CString(), node_->GetID(), l);

    props.life = l;
    node_->SetVar(GOA::LIFE, props.life);
    updateProps_ = true;
}

void GOC_Life::SetEnergy(float e)
{
    if (currentTemplate)
        e = Min(e, currentTemplate->energyMax);

    if (props.energy == e)
        return;

//    URHO3D_LOGINFOF("GOC_Life() - SetEnergy : %s(%u) energy=%f", node_->GetName().CString(), node_->GetID(), e);

    props.energy = e;
    updateProps_ = true;
}

void GOC_Life::AddTemplateImmunity(const String& name)
{
    if (name.Empty())
        return;

    Vector<String> args;
    args = name.Split('|');

    GOC_Life_Template::sImmunities_[StringHash("GOA_" + args[0])] = args.Size() > 1 ? ToFloat(args[1]) : 0.f;
}

void GOC_Life::SetInvulnerability(float i)
{
    if (props.invulnerability == i)
        return;

//    URHO3D_LOGINFOF("GOC_Life() - SetInvulnerability : %s(%u) inv=%f", node_->GetName().CString(), node_->GetID(), i);

    props.invulnerability = i;

    if (GetScene())
    {
        if (props.invulnerability > 0.f)
        {
            SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_Life, HandleInvulnerabilityUpdate));
        }
        else
        {
            haloInvulnerability_ = 0;
            UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
        }
    }
}

void GOC_Life::SetBonusDefense(const StringHash& type, float d)
{
//    URHO3D_LOGINFOF("GOC_Life() - SetBonusDefense : %s(%u) def=%f", node_->GetName().CString(), node_->GetID(), d);

    props.defBonuses_[type] = d;
}

float GOC_Life::GetBonusDefense(const StringHash& type) const
{
    HashMap<StringHash, float>::ConstIterator it = props.defBonuses_.Find(type);
    return it != props.defBonuses_.End() ? it->second_ : 0.f;
}

void GOC_Life::Set(float e, float inv)
{
//    URHO3D_LOGINFOF("GOC_Life() - Set ...");

    SetEnergyLost(e);
    SetInvulnerability(inv);

    ApplyAttributes();

//    URHO3D_LOGINFOF("GOC_Life() - Set ... OK !");
}

void GOC_Life::Reset()
{
    if (GameContext::Get().ClientMode_)
        waitServerReset_ = true;

    // Force Reset to Template Max Properties
    node_->SetVar(GOA::ISDEAD, false);
    props.life = 0;
    props.energy = 0.f;
    SetEnergyLost(0.f);
    SetInvulnerability(1.f);
    ApplyAttributes();

//    URHO3D_LOGINFOF("GOC_Life() - Reset : %s(%u) is Alive !", node_->GetName().CString(), node_->GetID());

    GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
    VariantMap& eventData = context_->GetEventDataMap();
    eventData[GOC_Life_Events::GO_ID] = node_->GetID();
    eventData[GOC_Life_Events::GO_TYPE] = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
    eventData[GOC_Life_Events::GO_MAINCONTROL] = controller ? controller->IsMainController() : false;
    node_->SendEvent(GOC_LIFERESTORE, eventData);
}

void GOC_Life::UpdateProperties()
{
//    URHO3D_LOGINFOF("GOC_Life() - UpdateProperties : %s(%u) totalDPS=%f currentTemplate=%u enabled=%s", node_->GetName().CString(), node_->GetID(),
//                    props.totalDpsReceived, currentTemplate, IsEnabledEffective() ? "true" : "false");

    if (!currentTemplate)
        return;

    if (props.totalDpsReceived > 0.f)
    {
        float newtotalenergy = (currentTemplate->energyMax * (float)currentTemplate->lifeMax) - props.totalDpsReceived;
        if (newtotalenergy > 0.1f)
        {
            int newlife = (int)(newtotalenergy / currentTemplate->energyMax);
            float newenergy = Max(newtotalenergy - (float)newlife * currentTemplate->energyMax, 0.f);
            newlife++;
            SetLife(newlife);
            SetEnergy(newenergy);
        }
        else
        {
            props.energy = 0.f;
        }
    }
    else
    {
        // Reset to Template Max Props if ZERO
        SetLife(props.life ? props.life : currentTemplate->lifeMax);
        SetEnergy(props.energy ? props.energy : currentTemplate->energyMax);

//        props.totalDpsReceived = currentTemplate->energyMax * ((float)currentTemplate->lifeMax - props.life + 1) - props.energy;
    }

//    URHO3D_LOGINFOF("GOC_Life() - UpdateProperties : %s(%u) ... Update Props(life=%d, energy=%f, dpsreceived=%f)",
//                    node_->GetName().CString(), node_->GetID(), props.life, props.energy, props.totalDpsReceived);
}

bool GOC_Life::ApplyTemplateProperties(const StringHash& key)
{
    if (key == template_)
        return false;

    if (GOC_Life_Template::templates_.Contains(key))
        currentTemplate = &(GOC_Life_Template::templates_.Find(key)->second_);
    else
        currentTemplate = &(GOC_Life_Template::templates_.Find(LIFE_TEMPLATE_DEFAULT)->second_);

    updateProps_ = true;
    template_ = key;

    return true;
}

void GOC_Life::ApplyAmountEnergy(float deltaAmount, bool hurtevent)
{
//    URHO3D_LOGINFOF("GOC_Life() - ApplyAmountEnergy : %s(%u) dps=%f ...", node_->GetName().CString(), node_->GetID(), deltaAmount);

    if (deltaAmount == 0.f)
    {
//        URHO3D_LOGINFOF("GOC_Life() - ApplyAmountEnergy : %s(%u) dps=0 ... skip !", node_->GetName().CString(), node_->GetID());
        return;
    }

    float newEnergy;

    // gain energy
    if (deltaAmount > 0.f)
    {
        if (props.energy == currentTemplate->energyMax)
            return;

        newEnergy = Min(props.energy+deltaAmount, currentTemplate->energyMax);
        deltaAmount = newEnergy - props.energy;

        SetEnergyLost(props.totalDpsReceived - deltaAmount);

//        URHO3D_LOGINFOF("GOC_Life() - ApplyAmountEnergy : %s(%u) Gain energy %f => %f !", node_->GetName().CString(), node_->GetID(), deltaAmount, newEnergy);
    }
    // decrease energy
    else
    {
        deltaAmount = -deltaAmount;
        newEnergy = Max(props.energy-deltaAmount, 0.f);
        deltaAmount = deltaAmount > currentTemplate->energyMax ? currentTemplate->energyMax : deltaAmount;

        SetEnergyLost(props.totalDpsReceived + deltaAmount);

//        URHO3D_LOGINFOF("GOC_Life() - ApplyAmountEnergy : %s(%u) Loose energy %f => %f !", node_->GetName().CString(), node_->GetID(), deltaAmount, newEnergy);

        if (newEnergy == 0.f && props.life > 0)
        {
            SetLife(props.life-1);
            SetInvulnerability(2.f);
            newEnergy = currentTemplate->energyMax;

            URHO3D_LOGINFOF("GOC_Life() - ApplyAmountEnergy : %s(%u) Loose a life !", node_->GetName().CString(), node_->GetID());
        }

//        if (hurtevent)
//            node_->SendEvent(GOC_LIFEDEC);
    }

    props.energy = newEnergy;
    updateProps_ = true;

    //node_->SendEvent(GOC_LIFEUPDATE);

    ApplyAttributes();
}

void GOC_Life::ApplyAttributes()
{
//    URHO3D_LOGINFOF("GOC_Life() - ApplyAttributes : %s(%u) enabled_=%s effective=%s", node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", IsEnabledEffective() ? "true" : "false");

//    if (node_->GetVar(GOA::ISDEAD) != Variant::EMPTY && node_->GetVar(GOA::ISDEAD).GetBool())
//    {
//        props.life = 0;
//        updateProps_ = true;
//        enabled_ = false;
////        URHO3D_LOGINFOF("GOC_Life() - ApplyAttributes : %s(%u) enabled=%s", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
//    }
//    if (node_->GetVar(GOA::ISDEAD) == Variant::EMPTY || node_->GetVar(GOA::ISDEAD).GetBool() == false)
//    {
//        enabled_ = true;
////        URHO3D_LOGINFOF("GOC_Life() - ApplyAttributes : %s(%u) enabled=%s", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
//
//        if (!currentTemplate && !props.life)
//        {
//            node_->SetVar(GOA::ISDEAD, false);
//            ApplyTemplateProperties(LIFE_TEMPLATE_DEFAULT);
//        }
//    }
//    else
//    {
//        props.life = 0;
//        updateProps_ = true;
//        enabled_ = false;
////        URHO3D_LOGINFOF("GOC_Life() - ApplyAttributes : %s(%u) enabled=%s", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
//    }

    if (node_->GetVar(GOA::ISDEAD) != Variant::EMPTY && node_->GetVar(GOA::ISDEAD).GetBool())
    {
        props.totalDpsReceived = GetTemplateMaxEnergy() * 2.f;
        props.life = 0;
        props.energy = 0.f;
        updateProps_ = true;
        enabled_ = false;
//        URHO3D_LOGINFOF("GOC_Life() - ApplyAttributes : %s(%u) enabled=%s is Dead ...", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
    }
    else
    {
        enabled_ = true;
//        URHO3D_LOGINFOF("GOC_Life() - ApplyAttributes : %s(%u) enabled=%s life=%d energy=%f",
//                        node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false", props.life, props.energy);

        if (!currentTemplate && !props.life)
        {
            node_->SetVar(GOA::ISDEAD, false);
            ApplyTemplateProperties(LIFE_TEMPLATE_DEFAULT);
        }
    }

    if (updateProps_)
    {
        UpdateProperties();
        updateProps_ = false;
        node_->SendEvent(GOC_LIFEUPDATE);
    }

    OnSetEnabled();
}

void GOC_Life::OnSetEnabled()
{
    if (IsEnabledEffective())
    {
        lastGainInvulnerabilityTime = 0;
        haloInvulnerability_ = 0;
        if (props.invulnerability > 0.f)
            SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_Life, HandleInvulnerabilityUpdate));

        // ServerMode don't need to use LifeUpdate ?
//        if (GameContext::Get().ServerMode_)
//            return;

//        URHO3D_LOGINFOF("GOC_Life() - OnSetEnabled : %s(%u) Subcribe to LifeUpdate !", node_->GetName().CString(), node_->GetID());
        SubscribeToEvent(node_, GOC_LIFEUPDATE, URHO3D_HANDLER(GOC_Life, HandleLifeUpdate));
    }
    else
    {
//        if (GameContext::Get().ServerMode_)
//            return;

//        URHO3D_LOGINFOF("GOC_Life() - OnSetEnabled : %s(%u) Unsubcribe from LifeUpdate !", node_->GetName().CString(), node_->GetID());
        UnsubscribeFromEvent(node_, GOC_LIFEUPDATE);

        SetLifeBars(false);
    }
}

void GOC_Life::HandleInvulnerabilityUpdate(StringHash eventType, VariantMap& eventData)
{
    props.invulnerability -= eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    if (props.invulnerability > 0.8f)
    {
        if (!haloInvulnerability_)
        {
            haloInvulnerability_ = (Node*) 1;
            Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
//            haloInvulnerability_ = GameHelpers::SpawnParticleEffectInNode(context_, node_, ParticuleEffect_[PE_LIFEFLAME], drawable->GetLayer(), drawable->GetViewMask(), Vector2::ZERO, 0.f, 0.5f, true, 0.5f, Color::RED, LOCAL);
        }
    }
    else if (props.invulnerability <= 0.f)
    {
        haloInvulnerability_ = 0;
        UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
    }
}

void GOC_Life::HandleLifeUpdate(StringHash eventType, VariantMap& eventData)
{
    if (props.life < 1)
    {
        SetInvulnerability(0.f);

//        UnsubscribeFromEvent(node_, GO_RECEIVEEFFECT);
        UnsubscribeFromEvent(node_, GOC_LIFEUPDATE);

        eventData[GOC_Life_Events::GO_ID] = node_->GetID();
        eventData[GOC_Life_Events::GO_KILLER] = killer;
        eventData[GOC_Life_Events::GO_TYPE] = node_->GetVar(GOA::TYPECONTROLLER).GetInt();
        eventData[GOC_Life_Events::GO_WORLDCONTACTPOINT] = lastDpsContactPoint_;

        node_->SetVar(GOA::ISDEAD, true);
        node_->SetVar(GOA::LIFE, 0);
        props.totalDpsReceived = 0.f;

        URHO3D_LOGINFOF("GOC_Life() - HandleLifeUpdate : %s(%u) is Dead !", node_->GetName().CString(), node_->GetID());

        node_->SendEvent(GOC_LIFEDEAD, eventData);

        const Variant& bosszone = node_->GetVar(GOA::BOSSZONEID);
        if (bosszone != Variant::EMPTY)
            BossZone::RemoveBoss(bosszone.GetIntVector3(), node_);

        if (killerPtr)
        {
            eventData.Clear();
            eventData[Go_Killer::GO_ID] = killerPtr->GetID();
            eventData[Go_Killer::GO_DEAD] = node_->GetID();
            killerPtr->SendEvent(GO_KILLER, eventData);
        }
    }

    UpdateLifeBars();
}

bool GOC_Life::IsImmune(const StringHash& effectelt, float value)
{
    if (!currentTemplate)
        return false;

    HashMap<StringHash, float >::ConstIterator it = currentTemplate->immunities_.Find(effectelt);
    if (it != currentTemplate->immunities_.End())
        return it->second_ >= Abs(value);

    return false;
}

bool GOC_Life::ReceiveEffectFrom(Node* sender, StringHash effectElt, float value, const Vector2& point, bool applyinvulnerability)
{
    unsigned senderFaction = sender ? sender->GetVar(GOA::FACTION).GetUInt() : 0;

//    URHO3D_LOGINFOF("GOC_Life() - ReceiveEffectFrom : %s(%u) faction=%u senderFaction=%u effect=%u value=%f ...",
//                    node_->GetName().CString(), node_->GetID(), node_->GetVar(GOA::FACTION).GetUInt(), senderFaction, effectElt.Value(), value);

    // check template immunity to effect
    if (IsImmune(effectElt, value))
    {
//        URHO3D_LOGWARNINGF("GOC_Life() - ReceiveEffectFrom : %s(%u) is immune to effect=%u value=%f ...",
//                        node_->GetName().CString(), node_->GetID(), effectElt.Value(), value);
        return false;
    }

    if (value < 0.f)
    {
        // never hit the owner
        if (sender && sender->GetVar(GOA::OWNERID).GetUInt() == node_->GetID())
        {
//            URHO3D_LOGWARNINGF("GOC_Life() - ReceiveEffectFrom : %s(%u) never must be hitted by his own object !", node_->GetName().CString(), node_->GetID());
            return false;
        }

        // faction=0 == allMonstersFactions => monsters can attack monster
        // NetPlayers can attack NetPlayers
        if (senderFaction && senderFaction != GO_AI_Enemy && senderFaction != GO_NetPlayer && senderFaction == node_->GetVar(GOA::FACTION).GetUInt())
        {
//            URHO3D_LOGWARNINGF("GOC_Life() - ReceiveEffectFrom : %s(%u) same faction=%u => no dmg !", node_->GetName().CString(), node_->GetID(), senderFaction);
            return false;
        }

        if (applyinvulnerability && props.invulnerability > 0.f)
        {
//            URHO3D_LOGINFOF("GOC_Life() - ReceiveEffectFrom : %s(%u) dps=%f apply Invulnerability no dmg inv=%f!", node_->GetName().CString(), node_->GetID(), value, props.invulnerability);
            return false;
        }

        value = Min(0.f, value + GetBonusDefense(effectElt) * (1.f + Random()));

        if (value >= 0.f)
        {
//            URHO3D_LOGINFOF("GOC_Life() - ReceiveEffectFrom : %s(%u) apply defBonus no dmg !", node_->GetName().CString(), node_->GetID());
            return false;
        }

        killerPtr = sender;
        if (killerPtr)
        {
            killer = killerPtr->GetVar(GOA::OWNERID).GetUInt();
            if (killer != 0)
                killerPtr = GetScene()->GetNode(killer);
            else
                killer = killerPtr->GetID();
        }
        else
        {
            killer = 0;
        }

        lastDpsContactPoint_ = point;

        node_->SendEvent(GOC_LIFEDEC);

//        URHO3D_LOGINFOF("GOC_Life() - ReceiveEffectFrom : %s(%u) Receive DPS=%f from %s(%u) !", node_->GetName().CString(), node_->GetID(), value, sender ? sender->GetName().CString() : "none", sender ? sender->GetID() : 0);

        if (applyinvulnerability)
            SetInvulnerability(0.8f);
    }
    else
    {
        if (senderFaction !=0 && senderFaction != node_->GetVar(GOA::FACTION).GetUInt())
            return false;

//        URHO3D_LOGINFOF("GOC_Life() - ReceiveEffectFrom : %s(%u) Receive Energy=%f from %s(%u)!", node_->GetName().CString(), node_->GetID(), value, sender ? sender->GetName().CString() : "none", sender ? sender->GetID() : 0);
    }

    if (GameContext::Get().ClientMode_)
    {
        if (value < 0.f)
        {
            URHO3D_LOGINFOF("GOC_Life() - ReceiveEffectFrom : %s(%u) Receive DPS in ClientMode", node_->GetName().CString(), node_->GetID());

            // on client simulation of loose a life => show invulnerability but not setting life
            if (props.energy - value <= 0.f && props.life > 0)
                SetInvulnerability(2.f);
        }
//        else
//        {
//            URHO3D_LOGWARNINGF("GOC_Life() - ReceiveEffectFrom : %s(%u) Receive LIFE in ClientMode TODO !", node_->GetName().CString(), node_->GetID());
//        }
    }
    else
    {
        ApplyAmountEnergy(value);
    }

    if (sender && value < 0.f && effectElt == GOA::LIFE)
    {
        ApplyForceEffect(sender->GetWorldPosition2D(), -value);
    }

    return true;
}

void GOC_Life::ApplyForceEffect(const Vector2& impactPoint, float strength)
{
    RigidBody2D* body = node_->GetComponent<RigidBody2D>();
    if (!body)
        return;

    Vector2 position = node_->GetWorldPosition2D();
    Vector2 delta(position - impactPoint);
    Vector2 force(strength / Clamp(Abs(delta.x_), 1.f, 20.f) * Sign(delta.x_),
                  strength / Clamp(Abs(delta.y_), 1.f, 20.f) * Sign(delta.y_));

    body->ApplyLinearImpulse(force, impactPoint, true);

//    URHO3D_LOGINFOF("GOC_Life() - ApplyForceEffect : %s(%u) position=%s impact=%s force=%s !",
//                    node_->GetName().CString(), node_->GetID(), position.ToString().CString(),
//                    impactPoint.ToString().CString(), force.ToString().CString());
}

void GOC_Life::UpdateLifeBars()
{
    if (!lifebarsui_.Size() && !lifebarnode_)
        return;

    float liferatio = currentTemplate ? 1.f - props.totalDpsReceived / (currentTemplate->energyMax * (float)currentTemplate->lifeMax) : 1.f;

    if (lifebarsui_.Size())
    {
        for (HashMap<int, WeakPtr<UIElement> >::Iterator it = lifebarsui_.Begin(); it != lifebarsui_.End(); ++it)
        {
            WeakPtr<UIElement>& lifebarui = it->second_;
            BorderImage* healthBar = lifebarui->GetChildStaticCast<BorderImage>(String("HealthBar"), true);
            if (healthBar)
                healthBar->SetSize(currentTemplate ? int(liferatio * healthBar->GetMaxSize().x_) : healthBar->GetMaxSize().x_, healthBar->GetMaxSize().y_);
        }
    }
    else if (lifebarnode_)
    {
        lifebarnode_->SetScale2D(Vector2(liferatio, 1.f));
    }
}

void GOC_Life::SetLifeBars(bool enable, bool follow, int viewport)
{
    if (enable)
    {
        // use UI
        if (!follow)
        {
            const unsigned numViewports = ViewManager::Get()->GetNumViewports();

            if (viewport == -1) // all viewport
            {
                for (unsigned i = 0; i < numViewports; i++)
                {
                    WeakPtr<UIElement>& lifebarui = lifebarsui_[i];
                    if (!lifebarui)
                    {
                        const IntRect& viewportRect = ViewManager::Get()->GetViewportRect(i);
                        lifebarui = GameHelpers::AddUIElement(String("UI/LifeBarUI.xml"), IntVector2::ZERO, HA_LEFT, VA_TOP);
                        lifebarui->SetPosition(IntVector2((viewportRect.left_ + (viewportRect.Width() - lifebarui->GetWidth())/2) / GameContext::Get().uiScale_, (viewportRect. top_ + 200) / GameContext::Get().uiScale_));
                        lifebarui->SetName("LifeBar");
                    }

                    lifebarui->SetVisible(true);
                }
            }
            else
            {
                WeakPtr<UIElement>& lifebarui = lifebarsui_[viewport];
                if (!lifebarui)
                {
                    const IntRect& viewportRect = ViewManager::Get()->GetViewportRect(viewport);
                    lifebarui = GameHelpers::AddUIElement(String("UI/LifeBarUI.xml"), IntVector2::ZERO, HA_LEFT, VA_TOP);
                    lifebarui->SetPosition(IntVector2((viewportRect.left_ + (viewportRect.Width() - lifebarui->GetWidth())/2) / GameContext::Get().uiScale_, (viewportRect. top_ + 200) / GameContext::Get().uiScale_));
                    lifebarui->SetName("LifeBar");
                    URHO3D_LOGINFOF("GOC_Life() - SetLifeBars : node=%s(%u) viewport=%d add ui lifebar !", node_->GetName().CString(), node_->GetID(), viewport);
                }

                lifebarui->SetVisible(true);
            }
        }
        // use node
        else
        {
            if (!lifebarnode_)
            {
                lifebarnode_ = node_->CreateChild("LifeBar", LOCAL);
                GameHelpers::LoadNodeXML(GetContext(), lifebarnode_, "UI/LifeBarNode.xml", LOCAL);

                lifebarnode_->SetPosition2D(Vector2(0.f, 2.f));
            }

            lifebarnode_->SetEnabled(true);
        }

        UpdateLifeBars();
    }
    else
    {
        if (lifebarsui_.Size())
        {
            if (viewport == -1)
            {
                for (HashMap<int, WeakPtr<UIElement> >::Iterator it = lifebarsui_.Begin(); it != lifebarsui_.End(); ++it)
                {
                    WeakPtr<UIElement>& lifebarui = it->second_;
                    if (lifebarui)
                        lifebarui->Remove();
                }
                lifebarsui_.Clear();
            }
            else
            {
                HashMap<int, WeakPtr<UIElement> >::Iterator it = lifebarsui_.Find(viewport);
                if (it != lifebarsui_.End())
                {
                    WeakPtr<UIElement>& lifebarui = it->second_;
                    if (lifebarui)
                        lifebarui->Remove();

                    lifebarsui_.Erase(it);
                }
            }
        }

        if (lifebarnode_)
            lifebarnode_->Remove();

        lifebarnode_.Reset();
    }
}

void GOC_Life::Dump() const
{
    URHO3D_LOGINFOF("GOC_Life() - Dump : life=%d/%d energy=%F/%F TotalEnergy=%F/%F", props.life, currentTemplate ? currentTemplate->lifeMax : 0,
                    props.energy, currentTemplate ? currentTemplate->energyMax : 0,
                    currentTemplate ? currentTemplate->energyMax * (float)currentTemplate->lifeMax - props.totalDpsReceived : props.energy * (float)props.life,
                    currentTemplate ? currentTemplate->energyMax * (float)currentTemplate->lifeMax : 0);
}
