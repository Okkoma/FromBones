#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"

#include "GOC_Destroyer.h"

#include "DefsViews.h"
#include "DefsColliders.h"

#include "GOC_Detector.h"



GOC_Detector::GOC_Detector(Context* context) :
    Component(context),
    body_(0),
    eventIn_(GO_DETECTORPLAYERIN),
    eventOut_(GO_DETECTORPLAYEROFF),
    directionOut_(IntVector2::ZERO),
    wallDetector_(false),
    viewDetector_(false),
    attackDetector_(false),
    stickTarget_(false),
    target_(0)
{
//    URHO3D_LOGINFOF("GOC_Detector()");
}

GOC_Detector::~GOC_Detector()
{
//    URHO3D_LOGINFOF("~GOC_Detector()");
}

void GOC_Detector::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Detector>();

    URHO3D_ACCESSOR_ATTRIBUTE("Trigger Event In", GetTriggerEventInAttr, SetTriggerEventInAttr, String, GOE::GetEventName(GO_DETECTORPLAYERIN), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Trigger Event Out", GetTriggerEventOutAttr, SetTriggerEventOutAttr, String, GOE::GetEventName(GO_DETECTORPLAYEROFF), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Direction Out", GetDirectionOut, SetDirectionOut, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ATTRIBUTE("Wall Detector Only", bool, wallDetector_, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Same View Only", GetSameViewOnly, SetSameViewOnly, bool, false, AM_FILE);
    URHO3D_ATTRIBUTE("Attack Only", bool, attackDetector_, false, AM_FILE);
    URHO3D_ATTRIBUTE("Stick Target", bool, stickTarget_, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Detected Categories", GetDetectedCategoriesAttr, SetDetectedCategoriesAttr, String, String::EMPTY, AM_FILE);

}

void GOC_Detector::SetTriggerEventInAttr(const String& eventname)
{
    eventIn_ = StringHash(eventname);
//    URHO3D_LOGINFOF("GOC_Detector() - SetTriggerEventInAttr : eventname=%s", eventname.CString());
}

const String& GOC_Detector::GetTriggerEventInAttr() const
{
    return GOE::GetEventName(eventIn_);
}

void GOC_Detector::SetTriggerEventOutAttr(const String& eventname)
{
    eventOut_ = StringHash(eventname);
}

const String& GOC_Detector::GetTriggerEventOutAttr() const
{
    return GOE::GetEventName(eventOut_);
}

void GOC_Detector::SetDirectionOut(const IntVector2& dirOut)
{
    directionOut_ = dirOut;
}

void GOC_Detector::SetSameViewOnly(bool enable)
{
    viewDetector_ = enable;
}

void GOC_Detector::SetDetectedCategoriesAttr(const String& value)
{
    if (value != detectedCategoriesStr_)
    {
        detectedCategories_.Clear();
        detectedCategoriesStr_ = value;
        Vector<String> categoriesNames = value.Split('|');
        for (unsigned i=0; i < categoriesNames.Size(); i++)
        {
            StringHash category(categoriesNames[i]);
            if (category && !detectedCategories_.Contains(category))
                detectedCategories_.Push(category);
        }
    }
}

void GOC_Detector::OnSetEnabled()
{
//    if (GameContext::Get().ClientMode_)
//        return;

//    URHO3D_LOGINFOF("GOC_Detector() - OnSetEnabled : enabled=%s", IsEnabledEffective() ? "true" : "false");

    if (IsEnabledEffective())
    {
        body_ = node_->GetComponent<RigidBody2D>();

        if (!body_ && node_->GetParent())
            body_ = node_->GetParent()->GetComponent<RigidBody2D>();

        if (body_)
        {
            detectedNodeIDs_.Clear();

            SubscribeToEvent(body_->GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Detector, HandleContact));
            SubscribeToEvent(body_->GetNode(), E_PHYSICSENDCONTACT2D, URHO3D_HANDLER(GOC_Detector, HandleContact));
        }
    }
    else
        UnsubscribeFromAllEvents();
}

void GOC_Detector::OnNodeSet(Node* node)
{
//    URHO3D_LOGINFOF("GOC_Detector() - OnNodeSet !");
    if (node)
        OnSetEnabled();
    else
        UnsubscribeFromAllEvents();
}

void GOC_Detector::CleanDependences()
{
    if (target_)
    {
        PODVector<Constraint2D* > contraints;
        // Get Contraints on the node and remove
        node_->GetDerivedComponents<Constraint2D>(contraints);
        for (PODVector<Constraint2D* >::Iterator it=contraints.Begin(); it!=contraints.End(); ++it)
        {
            Constraint2D* constraint = *it;
            if (constraint)
            {
                constraint->ReleaseJoint();
                constraint->Remove();
            }
        }

        // need to reset the shape extra properties
        if (node_->GetVar(GOA::PLATEFORM).GetBool())
        {
            CollisionBox2D* shape = node_->GetComponent<CollisionBox2D>();
            if (shape)
            {
                shape->SetExtraContactBits(0);
                shape->SetColliderInfo(NOCOLLIDER);
            }
        }
    }

    target_ = 0;
}

void GOC_Detector::HandleContact(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Detector() - HandleContact() : this=%u %s(%u) ... ", this, node_->GetName().CString(), node_->GetID());

    const ContactInfo& cinfo = eventType == E_PHYSICSBEGINCONTACT2D ? GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[PhysicsBeginContact2D::P_CONTACTINFO].GetUInt())
                               : GameContext::Get().physicsWorld_->GetEndContactInfo(eventData[PhysicsEndContact2D::P_CONTACTINFO].GetUInt());

    RigidBody2D* b1 = cinfo.bodyA_;
    RigidBody2D* b2 = cinfo.bodyB_;
    RigidBody2D* other = 0;
    CollisionShape2D* csBody = 0;
    CollisionShape2D* csOther = 0;

    if (b1 == body_)
    {
        other = b2;
        csBody = cinfo.shapeA_;
        csOther = cinfo.shapeB_;
    }
    else if (b2 == body_)
    {
        other = b1;
        csBody = cinfo.shapeB_;
        csOther = cinfo.shapeA_;
    }
    else
        return;

    if (!other->GetNode() || !csOther || !csBody)
        return;

    if (viewDetector_ && csOther->GetViewZ() != csBody->GetViewZ())
        return;

    if (!attackDetector_ && csOther->IsTrigger())
        return;

    unsigned otherNodeID = other->GetNode()->GetID();

//    URHO3D_LOGINFOF("GOC_Detector() - HandleContact() : this=%u ... thisNode=%s(%u) with otherNode=%s(%u)", this,
//                    node_->GetName().CString(), node_->GetID(), other->GetNode()->GetName().CString(), otherNodeID);

    /// other begin contact == other go inside the detector
    if (eventType == E_PHYSICSBEGINCONTACT2D)
    {
        if (!wallDetector_)
        {
            // Body CollisionShape == Trigger (required for coffre that has 2 collisionShapes (one trigger+one solid)) && Other CollisionShape == Solid
            if (!attackDetector_ && !csBody->IsTrigger())
                return;

            if (attackDetector_)
            {
                if (!csOther->IsTrigger() || !csOther->GetNode()->GetName().StartsWith(TRIGATTACK))
                    return;

//                URHO3D_LOGINFOF("GOC_Detector() - HandleContact this=%u attackDetector_ with Node=%s(%u) viewZ=%d otherViewZ=%d !",
//                                this, other->GetNode()->GetName().CString(), otherNodeID, csBody->GetViewZ(), csOther->GetViewZ());
            }

            if (detectedNodeIDs_.Contains(otherNodeID))
                return;

            if (!other->GetNode()->GetComponent<GOC_Destroyer>())
                return;

            // Filter following categories to detect
            if (!detectedCategories_.Empty())
            {
                const StringHash& got = other->GetNode()->GetVar(GOA::GOT).GetStringHash();
                if (got)
                {
                    if (COT::IsInCategories(got, detectedCategories_) == -1)
                        return;
                }
            }

//            URHO3D_LOGINFOF("GOC_Detector() - HandleContact this=%u directionOut_=%s", this, directionOut_.ToString().CString());

            /// if directionOut_ is set => Check if Node Other go to directionOut_
            if (directionOut_ != IntVector2::ZERO)
            {
                bool dirOk;

                Vector2 vel = other->GetNode()->GetVar(GOA::VELOCITY).GetVector2();
                if (vel == Vector2::ZERO)
                    vel = other->GetLinearVelocity();
                if (vel == Vector2::ZERO)
                    return;

                Vector2 delta = node_->GetWorldPosition2D() - other->GetNode()->GetWorldPosition2D();
                IntVector2 directionIn(-directionOut_);

//                if (directionIn.x_)
//                    dirOk = ((directionIn.x_ > 0 && delta.x_ > 0.f) || (directionIn.x_ < 0 && delta.x_ < 0.f)) && Abs(delta.x_) < Abs(delta.y_);
//                else
//                    dirOk = ((directionIn.y_ > 0 && delta.y_ > 0.f) || (directionIn.y_ < 0 && delta.y_ < 0.f)) && Abs(delta.y_) > Abs(delta.x_);

                if (directionIn.x_)
                    dirOk = ((directionIn.x_ > 0 && delta.x_ > 0.f && vel.x_ > 0.f) ||
                             (directionIn.x_ < 0 && delta.x_ < 0.f && vel.x_ < 0.f)) &&
                            (Abs(delta.x_) > Abs(delta.y_) || Abs(vel.x_) > 2.f * Abs(vel.y_));
                else
                    dirOk = ((directionIn.y_ > 0 && delta.y_ > 0.f && vel.y_ > 0.f) ||
                             (directionIn.y_ < 0 && delta.y_ < 0.f && vel.y_ < 0.f)) &&
                            (Abs(delta.y_) > Abs(delta.x_) || Abs(vel.y_) > 2.f * Abs(vel.x_));

                if (!dirOk)
                    return;
            }

//            URHO3D_LOGINFOF("GOC_Detector() - HandleContact this=%u : %s(%u) Begin Contact with node %s(%u) directionIn_=%s => sendEvent %s",
//                    this, node_->GetName().CString(), node_->GetID(), other->GetNode()->GetName().CString(), otherNodeID,
//                    IntVector2(-directionOut_).ToString().CString(), GetTriggerEventInAttr().CString());

            detectedNodeIDs_ += otherNodeID;
        }

//        URHO3D_LOGINFOF("GOC_Detector() - HandleContact() : this=%u ... thisNode=%s(%u) with otherNode=%s(%u) stick=%s", this,
//                        node_->GetName().CString(), node_->GetID(), other->GetNode()->GetName().CString(), otherNodeID, stickTarget_?"true":"false");

        if (stickTarget_)
            BecomeStickOn(csOther, cinfo);

        if (eventIn_.Value() != 0)
        {
//            URHO3D_LOGINFOF("GOC_Detector() - HandleContact() : this=%u ... thisNode=%s(%u) with otherNode=%s(%u) send event=%s", this,
//                        node_->GetName().CString(), node_->GetID(), other->GetNode()->GetName().CString(), otherNodeID, GOE::GetEventName(eventIn_).CString());

            VariantMap& eventData2 = context_->GetEventDataMap();
            eventData2[Go_Detector::GO_GETTER] = otherNodeID;
            eventData2[Go_Detector::GO_IMPACTPOINT] = cinfo.contactPoint_;
            node_->SendEvent(eventIn_, eventData2);
        }
    }
    /// other end contact == other go outside the detector
    else
    {
        if (!wallDetector_)
        {
            if (!detectedNodeIDs_.Contains(otherNodeID))
                return;

            // Body CollisionShape == Trigger (required for coffre that has 2 collisionShapes (one trigger+one solid)) && Other CollisionShape == Solid
            if (!csBody->IsTrigger() || csOther->IsTrigger()) return;
            if (!other->GetNode()->GetComponent<GOC_Destroyer>()) return;

            /// if directionOut_ is set => Check if other go to directionOut_
            if (directionOut_ != IntVector2::ZERO)
            {
                bool dirOk;

                Vector2 vel = other->GetNode()->GetVar(GOA::VELOCITY).GetVector2();
                if (vel == Vector2::ZERO)
                    vel = other->GetLinearVelocity();
                if (vel == Vector2::ZERO)
                    return;

                Vector2 delta = other->GetNode()->GetWorldPosition2D() - node_->GetWorldPosition2D();

                if (directionOut_.x_)
                    dirOk = ((directionOut_.x_ > 0 && delta.x_ > 0.f && vel.x_ > 0.f) ||
                             (directionOut_.x_ < 0 && delta.x_ < 0.f && vel.x_ < 0.f)) &&
                            (Abs(delta.x_) > Abs(delta.y_) || Abs(vel.x_) > 2.f * Abs(vel.y_));
                else
                    dirOk = ((directionOut_.y_ > 0 && delta.y_ > 0.f && vel.y_ > 0.f) ||
                             (directionOut_.y_ < 0 && delta.y_ < 0.f && vel.y_ < 0.f)) &&
                            (Abs(delta.y_) > Abs(delta.x_) || Abs(vel.y_) > 2.f * Abs(vel.x_));

                if (!dirOk)
                    return;
            }

//            URHO3D_LOGINFOF("GOC_Detector() - HandleContact this=%u : %s(%u) End Contact with node %s(%u) directionOut_=%s => sendEvent %s",
//                    this, node_->GetName().CString(), node_->GetID(), other->GetNode()->GetName().CString(), otherNodeID,
//                    directionOut_.ToString().CString(), GetTriggerEventOutAttr().CString());

            detectedNodeIDs_.Erase(otherNodeID);
        }

        if (eventOut_.Value() != 0)
        {
//            URHO3D_LOGINFOF("GOC_Detector() - HandleContact() : this=%u ... thisNode=%s(%u) with otherNode=%s(%u) send event=%s", this,
//                        node_->GetName().CString(), node_->GetID(), other->GetNode()->GetName().CString(), otherNodeID, GOE::GetEventName(eventOut_).CString());

            VariantMap& eventData2 = context_->GetEventDataMap();
            eventData2[Go_Detector::GO_GETTER] = otherNodeID;
            node_->SendEvent(eventOut_, eventData2);
        }
    }
}

void GOC_Detector::BecomeStickOn(CollisionShape2D* targetshape, const ContactInfo& cinfo)
{
    if (!targetshape || target_)
        return;

    // don't stick at water surface
    if (targetshape->GetColliderInfo() == WATERCOLLIDER)
        return;

    RigidBody2D* targetbody = targetshape->GetRigidBody();

    if (!body_ || !targetbody)
        return;

    StaticSprite2D* targetsprite = targetbody->GetNode()->GetDerivedComponent<StaticSprite2D>();
    if (targetsprite && !targetsprite->GetWorldBoundingBox().IsInside(cinfo.contactPoint_))
        return;

    if (targetbody->GetNode()->GetName().StartsWith(TRIGATTACK))
        return;

//    URHO3D_LOGINFOF("GOC_Detector() - BecomeStickOn : node=%s(%u) with target=%s(%u) stick=%s", node_->GetName().CString(), node_->GetID(),
//                    targetbody->GetNode()->GetName().CString(), targetbody->GetNode()->GetID(), stickTarget_?"true":"false");

    // prevent the collision with other welded entities like the projectiles Lance
    if (targetbody->GetNode()->GetComponent<ConstraintWeld2D>())
        return;

    // don't stick on Owner
    if (node_->GetVar(GOA::OWNERID).GetUInt() == targetbody->GetNode()->GetID())
    {
        URHO3D_LOGINFOF("GOC_Detector() - BecomeStickOn : node=%s(%u) with target=%s(%u) don't stick with Owner !",  node_->GetName().CString(), node_->GetID(),
                        targetbody->GetNode()->GetName().CString(), targetbody->GetNode()->GetID());
        return;
    }

    // don't stick when not enough move
    if (targetbody->GetBodyType() != BT_STATIC && ((cinfo.normal_.y_ > 0.f && Abs(cinfo.normal_.y_) > Abs(cinfo.normal_.x_)) || body_->GetLinearVelocity().LengthSquared() < 2.f))
        return;

    target_ = targetbody->GetNode();

    ConstraintWeld2D* constraintWeld = node_->CreateComponent<ConstraintWeld2D>();
    constraintWeld->SetOtherBody(targetbody);
    constraintWeld->SetAnchor(node_->GetWorldPosition2D());
    constraintWeld->SetFrequencyHz(4.0f);
    constraintWeld->SetDampingRatio(0.5f);

    if (node_->GetVar(GOA::PLATEFORM).GetBool())
    {
        CollisionBox2D* shape = node_->GetComponent<CollisionBox2D>();
        if (shape)
        {
            shape->SetExtraContactBits(CONTACT_ISSTABLE|CONTACT_TOP);
            shape->SetColliderInfo(PLATEFORMCOLLIDER);

            if (node_->GetVar(GOA::ONVIEWZ).GetInt() == INNERVIEW)
                shape->SetFilterBits(CC_INSIDEWALL, CM_INSIDEWALL);
            else
                shape->SetFilterBits(CC_OUTSIDEWALL, CM_OUTSIDEWALL);
        }
        GOC_Destroyer* destroyer = GetComponent<GOC_Destroyer>();
        if (destroyer && targetbody->GetBodyType() != BT_STATIC)
            destroyer->SetLifeTime(2.f);
    }

    StaticSprite2D* sprite = node_->GetDerivedComponent<StaticSprite2D>();
    sprite->SetLayer2(targetsprite ? targetsprite->GetLayer2()+IntVector2(-1,0) : IntVector2(targetshape->GetViewZ()+LAYER_PLATEFORMS, -1));

}
