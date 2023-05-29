#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "MAN_Effects.h"

#include "GOC_ZoneEffect.h"



GOC_ZoneEffect::GOC_ZoneEffect(Context* context) :
    Component(context),
    body_(0),
    zoneArea_(0),
    effectID_(-1),
    particuleOnHolderID_(-1),
    radius_(0),
    holder_(0),
    actived_(false),
    appliedToHolder_(false)
{ }

GOC_ZoneEffect::~GOC_ZoneEffect()
{
//    LOGINFO("~GOC_ZoneEffect()");

}

void GOC_ZoneEffect::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_ZoneEffect>();
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Register Effect", GetEffectAttr, RegisterEffectAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Center", GetCenter, SetCenter, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Apply To Holder", GetApplyToHolder, SetApplyToHolder, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Holder", GetHolder, SetHolder, unsigned, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Effect", GetEffectName, SetEffect, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Is Actived", GetActived, SetActived, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Particules On Holder", GetParticulesOnHolder, SetParticulesOnHolder, String, String::EMPTY, AM_DEFAULT);
}

void GOC_ZoneEffect::OnSetEnabled()
{
//    URHO3D_LOGINFOF("GOC_ZoneEffect() : node(%u) - OnSetEnabled ...", node_ ? node_->GetID() : 0);

    UpdateAttributes();
}

void GOC_ZoneEffect::OnNodeSet(Node* node)
{
//    LOGINFOF("GOC_ZoneEffect() - OnNodeSet : %u", node);

    if (node)
    {
        body_ = GetComponent<RigidBody2D>();
    }
    else
    {
        if (node_)
        {
            if (zoneArea_)
            {
                zoneArea_->Remove();
                zoneArea_ = 0;
            }

            Node* nodePE = node_->GetChild("PE");
            if (nodePE)
                nodePE->Remove();
        }

        UnsubscribeFromAllEvents();
        EffectsManager::RemoveInstancesIn(this);
    }
}

void GOC_ZoneEffect::SetActived(bool actived)
{
    if (actived_ == actived)
        return;

//    URHO3D_LOGINFOF("GOC_ZoneEffect() - SetActived : %u", actived);
    actived_ = actived;

    UpdateAttributes();
}

void GOC_ZoneEffect::SetApplyToHolder(bool state)
{
    if (appliedToHolder_ == state)
        return;

//    URHO3D_LOGINFOF("GOC_ZoneEffect() - SetApplyToHolder : %u", state);
    appliedToHolder_ = state;

    UpdateAttributes();
}

void GOC_ZoneEffect::SetHolder(unsigned id)
{
    if (holder_ == id)
        return;

//    URHO3D_LOGINFOF("GOC_ZoneEffect() - SetHolder : %u", id);
    holder_ = id;

    UpdateAttributes();
}

void GOC_ZoneEffect::SetRadius(float radius)
{
    radius_ = radius;
}

void GOC_ZoneEffect::SetCenter(const Vector2& center)
{
    center_ = center;
}

void GOC_ZoneEffect::SetParticulesOnHolder(const String& effectName)
{
    if (!effectName.Empty())
        particuleOnHolderID_ = EffectType::GetID(effectName);
    else
        particuleOnHolderID_ = -1;
}

void GOC_ZoneEffect::UseEffectOn(unsigned id)
{
    actived_ = true;
    holder_ = id;
    UpdateAttributes();
}

void GOC_ZoneEffect::SetArea(bool enable)
{
    if (!node_)
        return;

    if (radius_ > 0.f && !zoneArea_ && enable)
    {
        zoneArea_ = node_->CreateComponent<CollisionCircle2D>();
        zoneArea_->SetTrigger(true);
        zoneArea_->SetCenter(center_);
        zoneArea_->SetRadius(radius_);
    }

    if (zoneArea_)
        zoneArea_->SetEnabled(radius_ > 0.f && enable);

    if (particuleOnHolderID_ != -1)
    {
        Node* nodePE = node_->GetChild("PE");
        if (!nodePE && enable)
        {
            Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
            nodePE = GameHelpers::SpawnParticleEffectInNode(context_, node_, EffectType::Get(particuleOnHolderID_).effectRessource, drawable->GetLayer(), drawable->GetViewMask(), GameHelpers::GetWorldMassCenter(node_, zoneArea_), 0.f, 1.f, false, -1.f, Color::WHITE, LOCAL);
        }

        if (nodePE)
            nodePE->SetEnabled(enable);
    }
}

void GOC_ZoneEffect::UpdateAttributes()
{
//    URHO3D_LOGINFOF("GOC_ZoneEffect() - UpdateAttributes : actived_(%u), enabled_(%u), appliedToOwner_(%u), radius=%f, zoneArea(%u)",
//            actived_, IsEnabledEffective(), appliedToHolder_, radius_, zoneArea_);

    if (IsEnabledEffective())
    {
        if (!HasSubscribedToEvent(node_, GO_COLLIDEFLUID))
            SubscribeToEvent(node_, GO_COLLIDEFLUID, URHO3D_HANDLER(GOC_ZoneEffect, HandleTouchFluid));

        SetArea(actived_);

        if (actived_)
        {
//            URHO3D_LOGINFOF("GOC_ZoneEffect() - Node %s(%u) - UpdateAttributes - actived(%u) && zoneArea(%u) !",
//                     GetNode()->GetName().CString(), GetNode()->GetID(), actived_, zoneArea_);
            SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_ZoneEffect, HandleBeginContact));
//            SubscribeToEvent(node_, E_PHYSICSENDCONTACT2D, URHO3D_HANDLER(GOC_ZoneEffect, HandleBeginContact));
        }
        else
        {
//            URHO3D_LOGINFOF("GOC_ZoneEffect() - UpdateAttributes : on Node %s(%u) actived(%u) zoneArea(%u) => UnsubscribeEvents !",
//                     GetNode()->GetName().CString(), GetNode()->GetID(), actived_, zoneArea_);
            UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
//			UnsubscribeFromEvent(E_PHYSICSENDCONTACT2D);

            SubscribeToEvent(node_, GO_USEITEMEND, URHO3D_HANDLER(GOC_ZoneEffect, HandleActive));
        }

        if (actived_ && appliedToHolder_ && holder_)
        {
//            URHO3D_LOGINFOF("GOC_ZoneEffect() - UpdateAttributes : on Node %s(%u) actived && appliedToHolder => Add Holder to ListNodeEffect !",
//                     GetNode()->GetName().CString(), GetNode()->GetID());
            Node* receiver = GetScene()->GetNode(holder_);
            if (receiver)
                EffectsManager::AddEffectOn(node_, receiver, EffectType::GetPtr(effectID_), receiver->GetWorldPosition2D());
        }
    }
    else
    {
//        URHO3D_LOGINFOF("GOC_ZoneEffect() - UpdateAttributes - not enabled => UnsubscribeEvents !");

        SetArea(false);

        EffectsManager::RemoveInstancesIn(this);

        UnsubscribeFromAllEvents();
    }

//    URHO3D_LOGINFOF("GOC_ZoneEffect() - UpdateAttributes : OK !");
}

void GOC_ZoneEffect::SetEffect(int effectid)
{
    effectID_ = effectid;
}

void GOC_ZoneEffect::SetEffect(const String& effectName)
{
    effectID_ = EffectType::GetID(effectName);
}

void GOC_ZoneEffect::RegisterEffectAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_ZoneEffect() - RegisterEffectAttr : %s", value.CString());

    if (value.Empty())
        return;

    String name = String::EMPTY;
    String varStr = String::EMPTY;
    String ressource = String::EMPTY;
    String numTickStr = String::EMPTY;
    String quantityStr = String::EMPTY;
    String delayStr = String::EMPTY;
    String persistentStr = String::EMPTY;
    String factorStr = String::EMPTY;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() < 2) continue;

        String str = s[0].Trimmed();
        if (str == "name")
            name = s[1].Trimmed();
        else if (str == "var")
            varStr = s[1].Trimmed();
        else if (str == "numTick")
            numTickStr = s[1].Trimmed();
        else if (str == "qty")
            quantityStr = s[1].Trimmed();
        else if (str == "delay")
            delayStr = s[1].Trimmed();
        else if (str == "persistent")
            persistentStr = s[1].Trimmed();
        else if (str == "factor")
            factorStr = s[1].Trimmed();
        else if (str == "ressource")
            ressource = s[1].Trimmed();
    }

    if (name.Empty())
    {
        URHO3D_LOGERRORF("GOC_ZoneEffect() - SetEffectAttr : value=%s NAME Empty !", value.CString());
        return;
    }
    if (varStr.Empty())
    {
        URHO3D_LOGERRORF("GOC_ZoneEffect() - SetEffectAttr : value=%s VAR Empty !", value.CString());
        return;
    }
    if (quantityStr.Empty())
    {
        URHO3D_LOGERRORF("GOC_ZoneEffect() - SetEffectAttr : value=%s QTY Empty !", value.CString());
        return;
    }

    // Register Effect
    float quantity = ToFloat(quantityStr);
    unsigned numticks = (numTickStr == String::EMPTY ? 1 : ToUInt(numTickStr));
    float delay = (delayStr == String::EMPTY ? 0.f : ToFloat(delayStr));
    bool persistent = (persistentStr == String::EMPTY ? false : ToBool(persistentStr));
    float factor  = (factorStr == String::EMPTY ? 1.f : ToFloat(factorStr));

    EffectType effectType(StringHash("GOA_" + varStr), quantity, name, ressource, numticks, delay, persistent, factor);
    effectType.Register();

    // Set Effect
    effectID_ = EffectType::GetID(name);

    if (effectID_ == -1)
        URHO3D_LOGERRORF("GOC_ZoneEffect() - SetEffectAttr : effectID = -1 !");
}

String GOC_ZoneEffect::GetEffectAttr() const
{
    String value;

    if (effectID_ == -1)
        return String::EMPTY;

    const EffectType& e = GetEffect();

//    value = "name:" + e.name;
    value = "name:" + e.name + "|var:" + GOA::GetAttributeName(e.effectElt).Substring(4) +
            "|numTick:" + String(e.numTicks) + "|qty:" + String(e.qtyByTicks) + "|delay:" + String(e.delayBetweenTicks) +
            "|persistent:" + String(e.persistentOutZone) + "|factor:" + String(e.qtyFactorOutZone) +
            "|ressource:" + e.effectRessource;

    return value;
}

bool GOC_ZoneEffect::GetActived() const
{
    return actived_;
}

const String& GOC_ZoneEffect::GetParticulesOnHolder() const
{
    return particuleOnHolderID_ != -1 ? EffectType::Get(particuleOnHolderID_).name : String::EMPTY;
}

bool GOC_ZoneEffect::GetApplyToHolder() const
{
    return appliedToHolder_;
}

unsigned GOC_ZoneEffect::GetHolder() const
{
    return holder_;
}

const Vector2& GOC_ZoneEffect::GetCenter() const
{
    return center_;
}

float GOC_ZoneEffect::GetRadius() const
{
    return radius_;
}

int GOC_ZoneEffect::GetEffectID() const
{
    return effectID_;
}

const String& GOC_ZoneEffect::GetEffectName() const
{
    return EffectType::Get(effectID_).name;
}

const StringHash& GOC_ZoneEffect::GetEffectElt() const
{
    return EffectType::Get(effectID_).effectElt;
}

const EffectType& GOC_ZoneEffect::GetEffect() const
{
    return EffectType::Get(effectID_);
}

void GOC_ZoneEffect::HandleActive(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFO("GOC_ZoneEffect() - HandleActive !");
    SetActived(true);
}

void GOC_ZoneEffect::HandleBeginContact(StringHash eventType, VariantMap& eventData)
{
    assert(body_);

    const ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[PhysicsBeginContact2D::P_CONTACTINFO].GetUInt());

    RigidBody2D* b1 = cinfo.bodyA_;
    RigidBody2D* b2 = cinfo.bodyB_;
    RigidBody2D* other = 0;
    CollisionShape2D* csOther = 0;

    if (b1 == body_)
    {
        if (cinfo.shapeB_ && cinfo.shapeB_->IsTrigger())
            return;
        if (cinfo.shapeA_ != zoneArea_)
            return;

        other = b2;
        csOther = cinfo.shapeB_;
    }
    else if (b2 == body_)
    {
        if (cinfo.shapeA_ && cinfo.shapeA_->IsTrigger())
            return;
        if (cinfo.shapeB_ != zoneArea_)
            return;

        other = b1;
        csOther = cinfo.shapeA_;
    }
    else
        return;

    if (!csOther)
        return;

    /// Skip Attack if not on the same ViewZ
    if (body_->GetNode()->GetVar(GOA::ONVIEWZ).GetInt() != other->GetNode()->GetVar(GOA::ONVIEWZ).GetInt())
        return;

//	URHO3D_LOGINFOF("GOC_ZoneEffect() - HandleContact : %s Begin Contact with node %s", body_->GetNode()->GetName().CString(), other->GetNode()->GetName().CString());

    EffectsManager::AddEffectOn(node_, other->GetNode(), EffectType::GetPtr(effectID_), csOther->GetMassCenter() + csOther->GetNode()->GetWorldPosition2D());
}

void GOC_ZoneEffect::HandleTouchFluid(StringHash eventType, VariantMap& eventData)
{
    const StringHash& effectElt = GetEffectElt();

    if (effectElt == GOA::FIRE)
    {
        bool isWetted = eventData[Go_CollideFluid::GO_WETTED].GetBool();
        URHO3D_LOGINFOF("GOC_ZoneEffect() - HandleTouchFluid : %s(%u) touches fluid : is wetted = %s !", node_->GetName().CString(), node_->GetID(), isWetted?"true":"false");

        SetActived(!isWetted);
    }
}
