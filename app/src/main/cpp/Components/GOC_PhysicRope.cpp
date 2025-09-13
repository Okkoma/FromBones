#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/ConstraintRevolute2D.h>
#include <Urho3D/Urho2D/ConstraintRope2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include "DefsViews.h"
#include "DefsColliders.h"

#include "GameOptions.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"
#include "GameNetwork.h"

#include "GOC_Controller.h"
#include "GOC_Move2D.h"
#include "GOC_Destroyer.h"

#include "Map.h"
#include "MapWorld.h"
#include "ViewManager.h"

#include "GOC_PhysicRope.h"


static b2RevoluteJointDef sChainJointDef;

const char* ropeModelNames[] =
{
    "ThrowableRope",
    "FixedRope",
    "FixedBridge",
    0
};


GOC_PhysicRope::GOC_PhysicRope(Context* context) :
    Component(context),
    model_(RM_ThrowableRope),
    lengthDefault_(3.f),
    lengthMax_(8.f),
    softness_(2.f),
    isAttached_(false),
    isReleasing_(false),
    anchorTileIndex1_(0),
    anchorTileIndex2_(0)
{
}

GOC_PhysicRope::~GOC_PhysicRope()
{
//    URHO3D_LOGDEBUG("~GOC_PhysicRope");
}

void GOC_PhysicRope::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_PhysicRope>();

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, RopeModel, ropeModelNames, RM_ThrowableRope, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("HookNode", GetHookNodeAttr, SetHookNodeAttr, StringHash, StringHash::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("AttachedNode", GetAttachedNodeAttr, SetAttachedNodeAttr, StringHash, StringHash::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Length", float, lengthDefault_, 3.f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Length", float, lengthMax_, 8.f, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Sprites", GetSpritesAttr, SetSpritesAttr, ResourceRefList, Variant::emptyResourceRefList, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Softness", float, softness_, 2.f, AM_DEFAULT);

    // Chain Joint Def
    sChainJointDef.collideConnected = false;
    sChainJointDef.lowerAngle = -0.25f * b2_pi;
    sChainJointDef.upperAngle = 0.25f * b2_pi;
    sChainJointDef.enableLimit = true;
    sChainJointDef.maxMotorTorque = 1.f;
    sChainJointDef.motorSpeed = 0.f;
    sChainJointDef.enableMotor = true;
}

void GOC_PhysicRope::Release(float time)
{
    if (isReleasing_)
        return;

    if (HasSubscribedToEvent(node_, E_PHYSICSBEGINCONTACT2D))
        UnsubscribeFromEvent(node_, E_PHYSICSBEGINCONTACT2D);

    if (anchorNode1_ && model_ == RM_ThrowableRope)
        TimerRemover::Get()->Start(anchorNode1_.Get(), time);

    if (rootNode_)
        TimerRemover::Get()->Start(rootNode_.Get(), time);

    GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
    if (destroyer)
    {
        destroyer->SetLifeTime(time);
        destroyer->SetEnableLifeTimer(true);

        if (model_ != RM_ThrowableRope)
            destroyer->Destroy(time+0.1f, false);
    }

    ObjectControlInfo* oinfo = 0;
    if (!GameContext::Get().LocalMode_)
    {
        oinfo = GameNetwork::Get()->GetObjectControl(node_->GetID());
        if (oinfo)
            oinfo->GetPreparedControl().holderinfo_.id_ = M_MAX_UNSIGNED;
    }

    isAttached_ = false;
    isReleasing_ = true;

    URHO3D_LOGINFOF("GOC_PhysicRope() - Release : %u oinfo=%u delay=%F!", node_->GetID(), oinfo, time);
}

void GOC_PhysicRope::CleanDependences()
{
//	URHO3D_LOGERRORF("GOC_PhysicRope() - CleanDependences : %s(%u) parent=%s(%u) attachedNode=%s(%u) ...",
//					node_->GetName().CString(), node_->GetID(), node_->GetParent() ? node_->GetParent()->GetName().CString() : "", node_->GetParent() ? node_->GetParent()->GetID() : 0,
//					attachedNode_ ? attachedNode_->GetName().CString() : "", attachedNode_ ? attachedNode_->GetID() : 0);

    if (shapesInContact1_.Size())
    {
        for (int i=0; i < shapesInContact1_.Size(); i++)
            UnsubscribeFromEvent(shapesInContact1_[i], MAPTILEREMOVED);

        shapesInContact1_.Clear();
    }

    if (shapesInContact2_.Size())
    {
        for (int i=0; i < shapesInContact2_.Size(); i++)
            UnsubscribeFromEvent(shapesInContact2_[i], MAPTILEREMOVED);

        shapesInContact2_.Clear();
    }

    if (isAttached_)
        Detach();

//	for (unsigned i=0; i < chainNodes_.Size(); i++)
//		chainNodes_[i]->Remove();

    chainNodes_.Clear();

    if (rootNode_)
        rootNode_->Remove();

    if (anchorNode1_)
        anchorNode1_->Remove();

    if (anchorNode2_)
        anchorNode2_->Remove();

    if (attachedNode_)
        attachedNode_.Reset();
}


void GOC_PhysicRope::SetSpritesAttr(const ResourceRefList& spritelist)
{
    if (spritelist != Variant::emptyResourceRefList)
    {
        sprites_.Clear();
        Sprite2D::LoadFromResourceRefList(context_, spritelist, sprites_);
    }
}

ResourceRefList GOC_PhysicRope::GetSpritesAttr() const
{
    return sprites_.Size() ? Sprite2D::SaveToResourceRefList(sprites_) : Variant::emptyResourceRefList;
}

void GOC_PhysicRope::SetModelAttr(RopeModel model)
{
    if (model_ != model)
    {
        model_ = model;
    }
}

RopeModel GOC_PhysicRope::GetModelAttr() const
{
    return model_;
}

void GOC_PhysicRope::SetHookNodeAttr(const StringHash& hashname)
{
    hook_ = hashname;
}

const StringHash& GOC_PhysicRope::GetHookNodeAttr() const
{
    return hook_;
}

void GOC_PhysicRope::SetAttachedNodeAttr(const StringHash& hashname)
{
    attached_ = hashname;

    if (attached_)
    {
        Node* node = node_->GetChild(attached_, true);
        if (node)
            attachedNode_ = node;
    }

//	URHO3D_LOGERRORF("GOC_PhysicRope() - SetAttachedNodeAttr : %s(%u) attachedNode_=%s(%u) hashname=%u",
//					node_->GetName().CString(), node_->GetID(), attachedNode_ ? attachedNode_->GetName() : "", attachedNode_ ? attachedNode_->GetID() : 0, hashname.Value());
}

const StringHash& GOC_PhysicRope::GetAttachedNodeAttr() const
{
    return attached_;
}

void GOC_PhysicRope::SetMaxLength(float length)
{
    lengthMax_ = length;
}

void GOC_PhysicRope::SetDefaultLength(float length)
{
    lengthDefault_ = length;
}

void GOC_PhysicRope::SetAttachedNode(Node* node)
{
    attachedNode_ = node;
}

void GOC_PhysicRope::SetBridgeAnchors(const Vector2& anchor1, const Vector2& anchor2)
{
    anchorPosition1_ = anchor1;
    anchorPosition2_ = anchor2;
}

bool GOC_PhysicRope::IsAnchorOnMap() const
{
    if (shapesInContact1_.Size())
    {
        Map* map = World2D::GetWorld()->GetMapAt(mapInContact1_);
        if (map)
        {
            int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();
            FeatureType feat = map->GetFeatureAtZ(anchorTileIndex1_, viewZ);

            URHO3D_LOGINFOF("GOC_PhysicRope() - IsAnchorOnMap : node=%u viewZ=%d tileindex=%u feature=%u !", node_->GetID(), viewZ, anchorTileIndex1_, feat);

            return feat > MapFeatureType::Threshold;
        }
    }

    return true;
}


void GOC_PhysicRope::BreakContact()
{
    if (shapesInContact1_.Size())
    {
        URHO3D_LOGINFOF("GOC_PhysicRope() - BreakContact : node=%u anchor1 !", node_->GetID());

        if (anchorNode1_)
            anchorNode1_->SetEnabled(false);

        StaticSprite2D* staticSprite = node_->GetComponent<StaticSprite2D>();
        if (staticSprite)
            staticSprite->SetEnabled(false);

        shapesInContact1_.Clear();
    }

    if (!shapesInContact1_.Size() && !shapesInContact2_.Size())
    {
        UnsubscribeFromEvent(MAPTILEREMOVED);
    }
}

void GOC_PhysicRope::Detach()
{
    URHO3D_LOGINFOF("GOC_PhysicRope() - Detach !");

    if (anchorNode1_)
    {
        PODVector<Constraint2D*> constraints;
        anchorNode1_->GetDerivedComponents<Constraint2D>(constraints);
        for (PODVector<Constraint2D*>::Iterator it=constraints.Begin(); it!=constraints.End(); ++it)
        {
            (*it)->ReleaseJoint();
            (*it)->Remove();
        }
    }

    if (anchorNode2_)
    {
        PODVector<Constraint2D*> constraints;
        anchorNode2_->GetDerivedComponents<Constraint2D>(constraints);
        for (PODVector<Constraint2D*>::Iterator it=constraints.Begin(); it!=constraints.End(); ++it)
        {
            (*it)->ReleaseJoint();
            (*it)->Remove();
        }
    }

    if (attachedNode_)
    {
        PODVector<Constraint2D*> constraints;
        attachedNode_->GetDerivedComponents<Constraint2D>(constraints);
        for (PODVector<Constraint2D*>::Iterator it=constraints.Begin(); it!=constraints.End(); ++it)
        {
            (*it)->ReleaseJoint();
            (*it)->Remove();
        }
    }

    anchorPosition1_ = anchorPosition2_ = Vector2::ZERO;
    isAttached_ = false;
}


Constraint2D* AttachNodesWithRevoluteJoint(Node* nodeA, Node* nodeB, const Vector2& anchor, b2RevoluteJointDef* jointDef)
{
    if (!nodeA || !nodeB)
        return 0;

    ConstraintRevolute2D* joint = nodeA->CreateComponent<ConstraintRevolute2D>();
    if (jointDef)
        joint->CopyJointDef(*jointDef);
    // Revolute Anchor is in World Position (that allows to link the two nodes)
    joint->SetAnchor(anchor);
    joint->SetCollideConnected(false);
    joint->SetOtherBody(nodeB->GetComponent<RigidBody2D>());

    return joint;
}

Constraint2D* AttachNodesWithRopeJoint(Node* nodeA, Node* nodeB, float length, const Vector2& anchorOnA=Vector2::ZERO, const Vector2& anchorOnB=Vector2::ZERO)
{
    if (!nodeA || !nodeB)
        return 0;

    ConstraintRope2D* joint = nodeA->CreateComponent<ConstraintRope2D>();
    // Max Length is in World scale.
    joint->SetMaxLength(length);
    joint->SetCollideConnected(false);
    joint->SetOtherBody(nodeB->GetComponent<RigidBody2D>());

    // Rope Anchors are in Local Position.
    joint->SetOwnerBodyAnchor(anchorOnA);
    joint->SetOtherBodyAnchor(anchorOnB);

    return joint;
}

// return worldScaled Chain Length
float GOC_PhysicRope::CreateRope(const Vector2& startAnchor, const Vector2& endAnchor, int viewZ, int layer, int orderLayer, float align, float spacing)
{
    if (isReleasing_)
        return 0.f;

    if (!sprites_.Size())
        return 0.f;

    Vector2 delta(endAnchor - startAnchor);

    const float scale = model_ == RM_ThrowableRope ? 1.5f : 1.f;
    Rect drawrect;
    sprites_.Front()->GetDrawRectangle(drawrect);
    const Vector2 boxSize = drawrect.Size() * scale;
    const bool vertical = boxSize.y_ > boxSize.x_;
    const float linkLength = spacing * Max(boxSize.x_, boxSize.y_ );
    const Vector2 linkSize(!vertical ? linkLength : 0.f, vertical ? -linkLength : 0.f);
    const Vector2 boxCenter = (vertical ? -1.f : 1.f) * align * linkSize;

    StringHash got = node_->GetVar(GOA::GOT).GetStringHash();
    if (!got.Value())
        got = node_->GetParent()->GetVar(GOA::GOT).GetStringHash();

    const bool plateform = node_->GetVar(GOA::PLATEFORM).GetBool();
    const unsigned extrabits = plateform ? CONTACT_PLATEFORM : CONTACT_STABLE;

    if (plateform)
        layer += LAYER_PLATEFORMS;

    bool isfurniture = GOT::GetTypeProperties(got) & GOT_Furniture;
#ifdef ACTIVE_LAYERMATERIALS
    Material* material = GameContext::Get().layerMaterials_[isfurniture ? LAYERFURNITURES : LAYERACTORS];
#endif

    const unsigned linkCategorybits = viewZ == FRONTVIEW ? CC_OUTSIDEEFFECT : CC_INSIDEEFFECT;
    const unsigned linkMaskbits = viewZ == FRONTVIEW ? CM_OUTSIDEEFFECT : CM_INSIDEEFFECT;

    rootNode_= node_->GetScene()->CreateChild("LinksRoot", LOCAL);
    rootNode_->SetTemporary(true);

    rootNode_->SetWorldPosition2D(startAnchor);
    rootNode_->SetWorldRotation2D(Abs(delta.y_) > 0.001f ? -Atan(delta.x_/delta.y_) : 0.f);
    rootNode_->SetWorldScale2D(node_->GetWorldScale2D());

    chainNodes_.Clear();

    // create links from Start to End
    const unsigned viewMask = ViewManager::Get()->GetLayerMask(viewZ);
//    const int numlinks = (int) ceil(delta.Length() / (linkLength * node_->GetWorldScale2D().y_));
    const int numlinks = (int) ceil((delta.Length() - 0.5f * linkLength * node_->GetWorldScale().y_) / (linkLength * (node_->GetWorldScale2D().y_)));
    RigidBody2D* prevBody = 0;
    Vector2 position;
    position = linkSize * 0.5f;
    for (int i=0; i < numlinks; ++i)
    {
        Node* node = rootNode_->CreateChild("Link", LOCAL);
        node->SetTemporary(true);

        chainNodes_.Push(node);

        node->SetPosition2D(position);
        position += linkSize;

        // Create sprite
        StaticSprite2D* staticSprite = node->CreateComponent<StaticSprite2D>();
        staticSprite->SetSprite(sprites_.Front());
#ifdef ACTIVE_LAYERMATERIALS
        staticSprite->SetCustomMaterial(material);
#endif
        staticSprite->SetLayer2(IntVector2(layer, -1));
        staticSprite->SetOrderInLayer(orderLayer);
        staticSprite->SetViewMask(viewMask);

        // Create rigid body
        RigidBody2D* body = node->CreateComponent<RigidBody2D>();
        body->SetBodyType(BT_DYNAMIC);
        if (model_ == RM_FixedRope)
        {
            body->SetMass(0.01f);
            body->SetMassCenter(Vector2::ZERO);
            body->SetUseFixtureMass(false);
            body->SetFixedRotation(false);
        }
        // Create shape
        CollisionBox2D* box = node->CreateComponent<CollisionBox2D>();
        box->SetSize(boxSize);
        box->SetCenter(boxCenter);
        if (model_ == RM_ThrowableRope)
            box->SetDensity(2.f);
        box->SetFriction(0.02f);
        box->SetFilterBits(linkCategorybits, linkMaskbits);
        box->SetViewZ(viewZ);
        if (plateform)
            box->SetColliderInfo(PLATEFORMCOLLIDER);
        box->SetExtraContactBits(extrabits);

        if (prevBody)
        {
            ConstraintRevolute2D* joint = node->CreateComponent<ConstraintRevolute2D>();
            joint->CopyJointDef(sChainJointDef);
            joint->SetAnchor(node->GetWorldPosition2D());
            joint->SetCollideConnected(false);
            joint->SetOtherBody(prevBody);
        }

        prevBody = body;
    }

    numLinks_ = numlinks;

//    URHO3D_LOGINFOF("GOC_PhysicRope() - CreateRope : numlinks=%u viewZ=%d !", numlinks, viewZ);

    return ((float)numLinks_ - 0.5f) * linkLength * node_->GetWorldScale().y_;
}

float GOC_PhysicRope::CreateBridge(const Vector2& startAnchor, const Vector2& endAnchor, int viewZ, int layer, int orderLayer)
{
    if (isReleasing_)
        return 0.f;

    if (sprites_.Size() < 2)
        return 0.f;

    Vector2 delta(endAnchor - startAnchor);
    if (model_ == RM_ThrowableRope && delta.y_ >= 0.f)
        return 0.f;

    /*
    	sprites_[0] ... sprites_[n-1] = Bridge Elements
    	sprites_[n] = Rope Element
    */
    Rect drawrect;
    sprites_.Front()->GetDrawRectangle(drawrect);
    const Vector2 bridgeBoxSize = drawrect.Size();
    const float bridgeEltLength = bridgeBoxSize.x_;
    sprites_.Back()->GetDrawRectangle(drawrect);
    const Vector2 linkBoxSize = drawrect.Size();
    const float linkLength = linkBoxSize.x_;
    const float linkPivotOffset = 0.4f;
    const Vector2 linkSize(linkLength, 0.f);
    const Vector2 bridgeSize(bridgeEltLength, 0.f);

    StringHash got = node_->GetVar(GOA::GOT).GetStringHash();
    if (!got.Value())
        got = node_->GetParent()->GetVar(GOA::GOT).GetStringHash();

    const bool plateform = node_->GetVar(GOA::PLATEFORM).GetBool();
    const unsigned extrabits = plateform ? CONTACT_PLATEFORM : CONTACT_STABLE;

    if (plateform)
        layer += LAYER_PLATEFORMS;

#ifdef ACTIVE_LAYERMATERIALS
    Material* material = GameContext::Get().layerMaterials_[(GOT::GetTypeProperties(got) & GOT_Furniture) ? LAYERFURNITURES : LAYERACTORS];
#endif

    const unsigned linkCategorybits = viewZ == FRONTVIEW ? CC_OUTSIDEEFFECT : CC_INSIDEEFFECT;
    const unsigned linkMaskbits = viewZ == FRONTVIEW ? CM_OUTSIDEFURNITURE | CM_OUTSIDEPLATEFORM : CM_INSIDEFURNITURE | CM_INSIDEPLATEFORM;    
    const unsigned bridgeCategorybits = viewZ == FRONTVIEW ? CC_OUTSIDEOBJECT : CC_INSIDEOBJECT;
    const unsigned bridgeMaskbits = viewZ == FRONTVIEW ? CM_OUTSIDEOBJECT | CM_OUTSIDEPLATEFORM : CM_INSIDEOBJECT | CM_INSIDEPLATEFORM;

    rootNode_ = node_->GetScene()->CreateChild("LinksRoot", LOCAL);
    rootNode_->SetTemporary(true);
    rootNode_->SetWorldPosition2D(startAnchor);
    rootNode_->SetWorldRotation2D(Abs(delta.y_) > 0.001f ? -Atan(delta.x_/delta.y_) : 0.f);
    rootNode_->SetWorldScale2D(node_->GetWorldScale2D());

    chainNodes_.Clear();

    // create links from Start to End
    const unsigned viewMask = ViewManager::Get()->GetLayerMask(viewZ);
    const int numlinks = (int) floor(delta.Length() / (bridgeEltLength  * node_->GetWorldScale().y_));

    RigidBody2D* prevBody = 0;

    Vector2 position;
    int rand;

    for (int i=0; i < numlinks; ++i)
    {
        Node* linknode = rootNode_->CreateChild("BridgeRope", LOCAL);
        linknode->SetTemporary(true);
        chainNodes_.Push(linknode);
        linknode->SetPosition2D(position);

        // Create sprite
        {
            StaticSprite2D* staticSprite = linknode->CreateComponent<StaticSprite2D>();
            staticSprite->SetSprite(sprites_.Back());
#ifdef ACTIVE_LAYERMATERIALS
            staticSprite->SetCustomMaterial(material);
#endif
            staticSprite->SetLayer2(IntVector2(layer, -1));
            staticSprite->SetOrderInLayer(orderLayer+1);
            staticSprite->SetViewMask(viewMask);
            staticSprite->SetHotSpot(Vector2(0.5f - linkPivotOffset, 0.5f));
            staticSprite->SetUseHotSpot(true);
        }

        // Create rigid body
        RigidBody2D* linkbody = linknode->CreateComponent<RigidBody2D>();
        linkbody->SetBodyType(BT_DYNAMIC);

        // Create shape
        {
            CollisionBox2D* box = linknode->CreateComponent<CollisionBox2D>();
            box->SetSize(linkBoxSize);
            box->SetCenter(linkPivotOffset * linkSize);
            box->SetDensity(2.f);
            box->SetFriction(0.02f);
            box->SetFilterBits(linkCategorybits, linkMaskbits);
            box->SetViewZ(viewZ);
            box->SetGroupIndex(-1);
            if (plateform)
                box->SetColliderInfo(PLATEFORMCOLLIDER);
            box->SetExtraContactBits(extrabits);
        }

        if (prevBody)
        {
            ConstraintRevolute2D* joint = linknode->CreateComponent<ConstraintRevolute2D>();
            joint->CopyJointDef(sChainJointDef);
            joint->SetAnchor(linknode->GetWorldPosition2D()); // Revolute Anchors are in World Position.
            joint->SetCollideConnected(false);
            joint->SetOtherBody(prevBody);
        }

        Node* bridgenode = rootNode_->CreateChild("BridgeSegment", LOCAL);
        bridgenode->SetTemporary(true);
        chainNodes_.Push(bridgenode);

        position += linkSize * 2.f * linkPivotOffset;
        bridgenode->SetPosition2D(position);

        // Create sprite
        {
            rand = Random(0, 100);

            StaticSprite2D* staticSprite = bridgenode->CreateComponent<StaticSprite2D>();
            staticSprite->SetSprite(sprites_[rand % (sprites_.Size()-1)]);

#ifdef ACTIVE_LAYERMATERIALS
            staticSprite->SetCustomMaterial(material);
#endif
            staticSprite->SetLayer2(IntVector2(layer, -1));
            staticSprite->SetOrderInLayer(orderLayer);
            staticSprite->SetViewMask(viewMask);

            // version for modified StaticSprite2D::UpdateDrawRectangle() (with no parameter flipX,flipY)
            // Graphics.TODO 13/03/2024
//            staticSprite->SetFlip(rand > 49, rand % 2);
//            staticSprite->SetHotSpot(Vector2(linkSize.x_ * linkPivotOffset / bridgeSize.x_, 0.5f));

            staticSprite->SetFlip(rand > 49, rand % 2);
            staticSprite->SetHotSpot(Vector2(!staticSprite->GetFlipX() ? linkSize.x_ * linkPivotOffset / bridgeSize.x_ : (1.f - linkSize.x_ * linkPivotOffset / bridgeSize.x_), 0.5f));

            staticSprite->SetUseHotSpot(true);
        }

        // Create rigid body
        RigidBody2D* bridgebody = bridgenode->CreateComponent<RigidBody2D>();
        bridgebody->SetBodyType(BT_DYNAMIC);

        // Create shape
        {
            CollisionBox2D* box = bridgenode->CreateComponent<CollisionBox2D>();
            box->SetSize(bridgeBoxSize);
            box->SetCenter(bridgeSize * 0.5f - linkSize * linkPivotOffset);
            box->SetDensity(2.f);
            box->SetFriction(0.02f);
            box->SetFilterBits(bridgeCategorybits, bridgeMaskbits);
            box->SetViewZ(viewZ);
            if (plateform)
                box->SetColliderInfo(PLATEFORMCOLLIDER);
            box->SetExtraContactBits(extrabits);
            box->SetGroupIndex(-1);
        }

        ConstraintRevolute2D* joint = bridgenode->CreateComponent<ConstraintRevolute2D>();
        joint->CopyJointDef(sChainJointDef);
        joint->SetAnchor(bridgenode->GetWorldPosition2D());
        joint->SetCollideConnected(false);
        joint->SetOtherBody(linkbody);

        prevBody = bridgebody;

        position += bridgeSize - 2.f * linkPivotOffset * linkSize;
    }

    {
        Node* linknode = rootNode_->CreateChild("BridgeRope", LOCAL);
        linknode->SetTemporary(true);
        chainNodes_.Push(linknode);

        linknode->SetPosition2D(position + 2.f * linkPivotOffset * linkSize);

        // Create sprite
        {
            StaticSprite2D* staticSprite = linknode->CreateComponent<StaticSprite2D>();
            staticSprite->SetSprite(sprites_.Back());
#ifdef ACTIVE_LAYERMATERIALS
            staticSprite->SetCustomMaterial(material);
#endif
            staticSprite->SetLayer2(IntVector2(layer, -1));
            staticSprite->SetOrderInLayer(orderLayer+1);
            staticSprite->SetViewMask(viewMask);
            staticSprite->SetHotSpot(Vector2(1.f - linkSize.x_ * linkPivotOffset / bridgeSize.x_, 0.5f));
            staticSprite->SetUseHotSpot(true);
        }

        // Create rigid body
        RigidBody2D* linkbody = linknode->CreateComponent<RigidBody2D>();
        linkbody->SetBodyType(BT_DYNAMIC);

        // Create shape
        {
            CollisionBox2D* box = linknode->CreateComponent<CollisionBox2D>();
            box->SetSize(linkBoxSize);
            box->SetCenter(-linkPivotOffset * linkSize);
            box->SetDensity(2.f);
            box->SetFriction(0.02f);
            box->SetFilterBits(linkCategorybits, linkMaskbits);
            box->SetViewZ(viewZ);
            box->SetGroupIndex(-1);
            if (plateform)
                box->SetColliderInfo(PLATEFORMCOLLIDER);
            box->SetExtraContactBits(extrabits);
        }

        if (prevBody)
        {
            ConstraintRevolute2D* joint = linknode->CreateComponent<ConstraintRevolute2D>();
            joint->CopyJointDef(sChainJointDef);
            joint->SetAnchor(linknode->GetWorldPosition2D() - 2.f * linkPivotOffset * linkSize * linknode->GetWorldScale2D());
            joint->SetCollideConnected(false);
            joint->SetOtherBody(prevBody);
        }
    }

    numLinks_ = numlinks;

    float length = Vector2(chainNodes_.Front()->GetWorldPosition2D() - chainNodes_.Back()->GetWorldPosition2D()).Length();

//    URHO3D_LOGINFOF("GOC_PhysicRope() - CreateBridge : entrylength=%F numlinks=%u bridgeEltLength=%F linkLength=%F totalLength=%F viewZ=%d scale=%F !",
//                    delta.Length(), numlinks, bridgeEltLength, linkLength, length, viewZ, node_->GetWorldScale().y_);

    Constraint2D* constraint = AttachNodesWithRopeJoint(chainNodes_.Front(), chainNodes_.Back(), length+softness_);

    return length;
}

void GOC_PhysicRope::ResetBridgeElementsPositions()
{
//    URHO3D_LOGINFOF("GOC_PhysicRope() - ResetBridgeElementsPositions ... ");

    if (sprites_.Size() < 2)
        return;

    if (!chainNodes_.Size() || chainNodes_.Size() != 2 * numLinks_ + 1)
        return;

    /*
    	sprites_[0] ... sprites_[n-1] = Bridge Elements
    	sprites_[n] = Rope Element
    */
    Rect drawrect;
    sprites_.Front()->GetDrawRectangle(drawrect);
    const Vector2 bridgeSize(drawrect.Size().x_, 0.f);
    sprites_.Back()->GetDrawRectangle(drawrect);
    const Vector2 linkSize(drawrect.Size().x_, 0.f);
    const float linkPivotOffset = 0.4f;

    Vector2 position;
    Node* node;
    ConstraintRevolute2D* joint;

    for (int i=0; i < numLinks_; ++i)
    {
        // rope node
        node = chainNodes_[i*2];
        node->SetPosition2D(position);
        node->SetRotation2D(0.f);
        joint = node->GetComponent<ConstraintRevolute2D>();
        if (joint)
            joint->SetAnchor(node->GetWorldPosition2D());

        position += linkSize * 2.f * linkPivotOffset;

        // bridge node
        node = chainNodes_[i*2 + 1];
        node->SetPosition2D(position);
        node->SetRotation2D(0.f);
        joint = node->GetComponent<ConstraintRevolute2D>();
        if (joint)
            joint->SetAnchor(node->GetWorldPosition2D());

        position += bridgeSize - 2.f * linkPivotOffset * linkSize;
    }

    // last rope node
    {
        node = chainNodes_.Back();
        node->SetPosition2D(position + 2.f * linkPivotOffset * linkSize);
        node->SetRotation2D(0.f);
        joint = node->GetComponent<ConstraintRevolute2D>();
        if (joint)
            joint->SetAnchor(node->GetWorldPosition2D() - 2.f * linkPivotOffset * linkSize * node->GetWorldScale2D());
    }

//    URHO3D_LOGINFOF("GOC_PhysicRope() - ResetBridgeElementsPositions ... OK !");
}

bool GOC_PhysicRope::AttachOnRoof(const Vector2& anchorOnRoofPosition, float anchorRotation, const void* pinfo)
{
    if (isReleasing_)
    {
        URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnRoof : node=%u is Releasing !", node_->GetID());
        return false;
    }

//    URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnRoof ... attachedNode=%s(%u) ...",
//                    attachedNode_ ? attachedNode_->GetName().CString() : "null", attachedNode_ ? attachedNode_->GetID() : 0);

    Vector2 anchorOnAttachedNodePosition;

    if (attachedNode_)
    {
        Node* anchorNode = attachedNode_->GetChild("Anchor");
//        anchorOnAttachedNodePosition = anchorNode ? anchorNode->GetWorldPosition2D() :  attachedNode_->GetComponent<RigidBody2D>()->GetWorldMassCenter();
        anchorOnAttachedNodePosition = anchorNode ? anchorNode->GetWorldPosition2D() :  attachedNode_->GetWorldPosition2D();
    }
    else
    {
        anchorOnAttachedNodePosition = anchorOnRoofPosition - Vector2(0.f, lengthDefault_);
    }

    if ((anchorOnRoofPosition-anchorOnAttachedNodePosition).Length() > lengthMax_)
    {
        URHO3D_LOGWARNINGF("GOC_PhysicRope() - AttachOnRoof : Rope Length > %f ! Release !", lengthMax_);
        Release(2.f);
        return false;
    }

    RigidBody2D* body = hook_ ? node_->GetChild(hook_)->GetComponent<RigidBody2D>() : node_->GetComponent<RigidBody2D>();
    if (!body)
    {
        URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnRoof : node=%u No Body !", node_->GetID());
        Release(2.f);
        return false;
    }

    if (model_ == RM_ThrowableRope)
    {
        // Detach the old chain and remove the hook
        if (anchorNode1_)
        {
            Detach();
            TimerRemover::Get()->Start(anchorNode1_.Get(), 0.f);
        }

        // Remove the old chain
        if (rootNode_)
            TimerRemover::Get()->Start(rootNode_.Get(), 0.f);
    }

    CollisionShape2D* cs = 0;

    // Only One try for Attach
    if (pinfo)
    {
        const ContactInfo& cinfo = *((const ContactInfo*)pinfo);
        bool checkBody = cinfo.bodyA_ == node_->GetComponent<RigidBody2D>();
        cs = checkBody ? cinfo.shapeB_ : cinfo.shapeA_;

        if (!cs)
            return false;

        if (!cs->GetColliderInfo() || cinfo.normal_.y_ > PIXEL_SIZE)
        {
            URHO3D_LOGWARNINGF("GOC_PhysicRope() - AttachOnRoof : node=%u No MapCollider in Collision !", node_->GetID());
            Release(2.f);
            return false;
        }

        if (model_ == RM_ThrowableRope && anchorOnRoofPosition.y_ - anchorOnAttachedNodePosition.y_ < 0.5f)
        {
            URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnRoof : startAnchor=%s endAnchor=%s ... to low", anchorOnRoofPosition.ToString().CString(), anchorOnAttachedNodePosition.ToString().CString());
            Release(2.f);
            return false;
        }
    }

    if (node_->GetVar(GOA::ONVIEWZ) == Variant::EMPTY)
        node_->SetVar(GOA::ONVIEWZ, node_->GetParent()->GetVar(GOA::ONVIEWZ).GetInt());

    int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();

    // release grapin dynamic body and create static grapin
    if (body->GetBodyType() == BT_DYNAMIC)
    {
        node_->GetComponent<CollisionCircle2D>()->SetEnabled(false);
        body->SetEnabled(false);

        // create grapin static body on wall at the same place
        anchorNode1_ = GetScene()->CreateChild("Hook", LOCAL);
        anchorNode1_->CreateComponent<RigidBody2D>();

        CollisionBox2D* shape = anchorNode1_->CreateComponent<CollisionBox2D>();
        shape->SetCenter(0.f, 0.55f);
        shape->SetSize(0.2f, 0.2f);
        shape->SetFriction(0.5f);
        shape->SetExtraContactBits(CONTACT_STABLE);
        shape->SetViewZ(viewZ);
        shape->SetCategoryBits(viewZ == FRONTVIEW ? CC_OUTSIDEOBJECT : CC_INSIDEOBJECT);

        anchorNode1_->SetWorldPosition2D(anchorOnRoofPosition);
        anchorNode1_->SetWorldRotation2D(anchorRotation);
        anchorNode1_->SetWorldScale2D(node_->GetWorldScale2D());
    }
    else
    {
        anchorNode1_ = body->GetNode();
    }

    // create Chains and Joints
    Drawable2D* drawable = hook_ ? node_->GetChild(hook_)->GetDerivedComponent<Drawable2D>() : node_->GetDerivedComponent<Drawable2D>();

    URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnRoof : startAnchor=%s endAnchor=%s ...", anchorOnRoofPosition.ToString().CString(), anchorOnAttachedNodePosition.ToString().CString());

    float ropeLength = CreateRope(anchorOnRoofPosition, anchorOnAttachedNodePosition, viewZ, drawable->GetLayer(), drawable->GetOrderInLayer()-1);
    if (ropeLength > 0.f && chainNodes_.Size())
    {
        Constraint2D* constraint1 = AttachNodesWithRevoluteJoint(anchorNode1_, chainNodes_.Front(), anchorOnRoofPosition, &sChainJointDef);

        if (attachedNode_)
        {
            Constraint2D* constraint2 = AttachNodesWithRevoluteJoint(attachedNode_, chainNodes_.Back(), anchorOnAttachedNodePosition, &sChainJointDef);
            Constraint2D* constraint3 = AttachNodesWithRopeJoint(anchorNode1_, attachedNode_, ropeLength + softness_);

            URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnRoof ... create joint rope length=%f between anchor=%s & attachednode=%s c1=%u c2=%u c3=%u",
                            ropeLength, anchorOnAttachedNodePosition.ToString().CString(), anchorOnAttachedNodePosition.ToString().CString(), constraint1, constraint2, constraint3);
        }

        isAttached_ = true;

        bool isfurniture = GOT::GetTypeProperties(node_->GetVar(GOA::GOT).GetStringHash()) & GOT_Furniture;

        if (!GameContext::Get().LocalMode_ && !isfurniture)
        {
            int clientid = node_->GetVar(GOA::CLIENTID).GetInt();
            ObjectControlInfo* oinfo = clientid && clientid == GameNetwork::Get()->GetClientID() ? GameNetwork::Get()->GetClientObjectControl(node_->GetID()) : GameNetwork::Get()->GetServerObjectControl(node_->GetID());
            if (!oinfo)
                oinfo = GameNetwork::Get()->AddSpawnControl(node_, attachedNode_, model_ != RM_ThrowableRope);
            if (oinfo)
            {
                ObjectControl& control = oinfo->GetPreparedControl();
                control.holderinfo_.id_ = attachedNode_ ? attachedNode_->GetID() : M_MIN_UNSIGNED;
                control.holderinfo_.point1x_ = anchorOnAttachedNodePosition.x_;
                control.holderinfo_.point1y_ = anchorOnAttachedNodePosition.y_;
                control.holderinfo_.point2x_ = anchorOnRoofPosition.x_;
                control.holderinfo_.point2y_ = anchorOnRoofPosition.y_;
                control.holderinfo_.rot2_ = anchorRotation;
                oinfo->GetReceivedControl().holderinfo_.id_ = control.holderinfo_.id_;
            }
        }

        // set the shape in contact and the tileindex at roof
        if (pinfo)
        {
            const ContactInfo& cinfo = *((const ContactInfo*)pinfo);
            World2D::GetWorldInfo()->Convert2WorldMapPosition(cinfo.contactPoint_ - (cinfo.normal_ * MapInfo::info.mTileHalfSize_), mapInContact1_, anchorTileIndex1_);
            shapesInContact1_.Clear();
            shapesInContact1_.Push(cs);
        }
        // if no collisionshape given, get it at the tileindex
        else if (World2D::GetNearestBlockTile(anchorOnRoofPosition, viewZ, mapInContact1_, anchorTileIndex1_))
        {
            shapesInContact1_.Clear();
            World2D::GetCollisionShapesAt(mapInContact1_, anchorTileIndex1_, viewZ, shapesInContact1_);
        }

        if (shapesInContact1_.Size())
        {
            SubscribeToEvent(MAPTILEREMOVED, URHO3D_HANDLER(GOC_PhysicRope, HandleBreakContact));
        }
    }

//    URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnRoof ... OK !");

    return true;
}

bool GOC_PhysicRope::AttachOnWalls()
{
    if (isReleasing_)
        return false;

    if (node_->GetVar(GOA::ONVIEWZ) == Variant::EMPTY)
        node_->SetVar(GOA::ONVIEWZ, node_->GetParent()->GetVar(GOA::ONVIEWZ).GetInt());

    int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();

    if (anchorPosition1_ == anchorPosition2_)
    {
        anchorPosition1_ = node_->GetWorldPosition2D();
        anchorPosition1_.x_ -= 0.05f; // pour etre sur d'obtenir la tile support

        // find the length
        float length = lengthDefault_;

        // check from left to right (minimal : 1 empty cell and 1 block cell after)
        {
            World2DInfo* winfo = World2D::GetWorld()->GetWorldInfo();
            winfo->Convert2WorldMapPosition(anchorPosition1_, mapInContact1_, anchorTileIndex1_);

            Map* map = World2D::GetWorld()->GetMapAt(mapInContact1_);
            if (map)
            {
                IntVector2 anchor1Coords = map->GetTileCoords(anchorTileIndex1_);
                anchor1Coords.x_++;
                IntVector2 anchor2Coords = anchor1Coords;
                int viewid = map->GetViewId(viewZ);

                if (viewid == -1)
                {
                    viewid = map->GetNearestViewId(viewZ);
                    if (viewid == -1)
                    {
                        anchorPosition1_ = anchorPosition2_ = Vector2::ZERO;
                        URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnWalls : viewid=%d anchor1Coords=%d %d anchor2Coords=%d %d... no viewid for viewZ=%d !", anchor1Coords.x_, anchor1Coords.y_, anchor2Coords.x_, anchor2Coords.y_, viewZ);
                        return false;
                    }
                }

                map->GetBlockPositionAt(RightDir, viewid, anchor2Coords);

                if (anchor2Coords.x_ - anchor1Coords.x_ < 1)
                {
                    anchorPosition1_ = anchorPosition2_ = Vector2::ZERO;
                    URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnWalls : anchor1Coords=%d %d anchor2Coords=%d %d... can't get an enough length !", anchor1Coords.x_, anchor1Coords.y_, anchor2Coords.x_, anchor2Coords.y_);
                    return false;
                }

                length = (anchor2Coords.x_ - anchor1Coords.x_) * winfo->mTileWidth_;
                if (length > lengthMax_)
                {
                    anchorPosition1_ = anchorPosition2_ = Vector2::ZERO;
                    URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnWalls : length=%F > maxLength=%F ... skip !", length, lengthMax_);
                    return false;
                }

                mapInContact2_ = mapInContact1_;
                anchorTileIndex2_ = map->GetTileIndex(anchor2Coords.x_, anchor2Coords.y_);

                URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnWalls : anchor1Coords=%d %d anchor2Coords=%d %d...", anchor1Coords.x_, anchor1Coords.y_, anchor2Coords.x_, anchor2Coords.y_);
            }
        }

        anchorPosition2_.x_ = anchorPosition1_.x_ + length;
        anchorPosition2_.y_ = anchorPosition1_.y_;
    }

//    URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnWalls : anchorPosition1_=%s anchorPosition2_=%s ...", anchorPosition1_.ToString().CString(), anchorPosition2_.ToString().CString());

    // Create Anchor Static Bodies
    {
        anchorNode1_ = GetScene()->CreateChild("Hook", LOCAL);
        anchorNode1_->SetTemporary(true);
        anchorNode1_->CreateComponent<RigidBody2D>();
        CollisionBox2D* shape = anchorNode1_->CreateComponent<CollisionBox2D>();
        shape->SetCenter(0.f, 0.f);
        shape->SetSize(0.1f, 0.1f);
        shape->SetFriction(0.5f);
        shape->SetExtraContactBits(CONTACT_STABLE);
        shape->SetViewZ(viewZ);
        shape->SetFilterBits(viewZ == FRONTVIEW ? CC_OUTSIDEEFFECT : CC_INSIDEEFFECT, viewZ == FRONTVIEW ? CM_OUTSIDEFURNITURE | CM_OUTSIDEPLATEFORM  : CM_INSIDEFURNITURE | CM_INSIDEPLATEFORM);
        shape->SetGroupIndex(-1);

        anchorNode1_->SetWorldPosition2D(anchorPosition1_);
        anchorNode1_->SetWorldScale2D(node_->GetWorldScale2D());

        anchorNode2_ = anchorNode1_->Clone(LOCAL);
        anchorNode2_->SetWorldPosition2D(anchorPosition2_);
        anchorNode2_->SetWorldScale2D(node_->GetWorldScale2D());
    }

    // Create Bridge
    {
        float length = CreateBridge(anchorPosition1_, anchorPosition2_, viewZ, viewZ-1, 1);

        if (length > 0.f)
        {
            anchorNode2_->SetWorldPosition2D(chainNodes_.Back()->GetWorldPosition2D());

            Constraint2D* constraint;
            constraint = AttachNodesWithRevoluteJoint(anchorNode1_, chainNodes_.Front(), anchorNode1_->GetWorldPosition2D(), &sChainJointDef);
            constraint = AttachNodesWithRevoluteJoint(chainNodes_.Back(), anchorNode2_, anchorNode2_->GetWorldPosition2D(), &sChainJointDef);

            World2D::GetCollisionShapesAt(mapInContact1_, anchorTileIndex1_, viewZ, shapesInContact1_);
            World2D::GetCollisionShapesAt(mapInContact2_, anchorTileIndex2_, viewZ, shapesInContact2_);

            // Subscribe To Contacts Events
            if (shapesInContact1_.Size() || shapesInContact2_.Size())
            {
                SubscribeToEvent(MAPTILEREMOVED, URHO3D_HANDLER(GOC_PhysicRope, HandleBreakContact));
            }

            URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnWalls : %s(%u) anchor1(map=%s;tile=%u) anchor2(map=%s;tile=%u)",
                             node_->GetName().CString(), node_->GetID(), mapInContact1_.ToString().CString(), anchorTileIndex1_, mapInContact2_.ToString().CString(), anchorTileIndex2_);
        }
        else
        {
            URHO3D_LOGERRORF("GOC_PhysicRope() - AttachOnWalls : %s(%u) can't create Bridge : sprites=%s !",
                             node_->GetName().CString(), node_->GetID(), GetSpritesAttr().ToString().CString());
        }
    }

    isAttached_ = true;

//    URHO3D_LOGINFOF("GOC_PhysicRope() - AttachOnWalls ... OK !");

    return true;
}


void GOC_PhysicRope::OnSetEnabled()
{
    if (GetScene())
    {
        if (IsEnabledEffective())
        {
            GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
            if (destroyer)
            {
                destroyer->SetLifeTime(0.f);
                destroyer->SetEnableLifeTimer(false);
            }

            isReleasing_ = false;

            if (model_ == RM_FixedRope)
            {
                if (!attachedNode_ && attached_)
                    attachedNode_ = node_->GetChild(attached_);

//                URHO3D_LOGINFOF("GOC_PhysicRope() - OnSetEnabled : node=%s(%u) ... attachedNode_=%u attached_=%u ...",
//                                node_->GetName().CString(), node_->GetID(), attachedNode_.Get(), attached_.Value());

                if (attachedNode_ && !isAttached_ && (!node_->isInPool_ || !node_->isPoolNode_))
                {
                    bool ok = AttachOnRoof(node_->GetWorldPosition2D(), node_->GetWorldRotation2D());
//                    URHO3D_LOGINFOF("GOC_PhysicRope() - OnSetEnabled : node=%s(%u) enable=true ok=%s rootNode_=%s(%u) !",
//                                    node_->GetName().CString(), node_->GetID(), ok ? "true":"false", rootNode_ ? rootNode_->GetName().CString() : "none", rootNode_ ? rootNode_->GetID() : 0);
                }
            }
            else if (model_ == RM_FixedBridge)
            {
                if (!node_->isInPool_ || !node_->isPoolNode_)
                {
                    if (!isAttached_)
                        bool ok = AttachOnWalls();
                    else
                        ResetBridgeElementsPositions();

//                    URHO3D_LOGINFOF("GOC_PhysicRope() - OnSetEnabled : node=%s(%u) enable=true  !", node_->GetName().CString(), node_->GetID());
                }

            }
            else if (model_ == RM_ThrowableRope)
            {
                if (!GameNetwork::Get() || node_->GetVar(GOA::CLIENTID).GetInt() == GameNetwork::Get()->GetClientID())
                {
                    SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_PhysicRope, HandleRoofCollision));
//	                URHO3D_LOGINFOF("GOC_PhysicRope() - OnSetEnabled : node=%u subscribe to roof collision !", node_->GetID());
                }
            }

            // Need this to active correctly Lustre's Children.
            node_->SetEnabledRecursive(true);

            if (rootNode_)
                rootNode_->SetEnabledRecursive(true);
        }
        else
        {
            if (rootNode_)
                rootNode_->SetEnabledRecursive(false);

            // In case is attached (network packet lost)
            if (isAttached_ && model_ == RM_ThrowableRope)
            {
//                URHO3D_LOGINFOF("GOC_PhysicRope() - OnSetEnabled : node=%u enable=false => Release !", node_->GetID());
                Detach();
                Release(2.f);
            }

            UnsubscribeFromEvent(node_, E_PHYSICSBEGINCONTACT2D);

//            URHO3D_LOGINFOF("GOC_PhysicRope() - OnSetEnabled : node=%u enable=false unsubscribe to events !", node_->GetID());
        }
    }
}

void GOC_PhysicRope::HandleRoofCollision(StringHash eventType, VariantMap& eventData)
{
    if (isAttached_)
        return;

    URHO3D_LOGINFOF("GOC_PhysicRope() - HandleRoofCollision : %s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (AttachOnRoof(node_->GetWorldPosition2D(), node_->GetWorldRotation2D(), &GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[PhysicsBeginContact2D::P_CONTACTINFO].GetUInt())))
        UnsubscribeFromEvent(node_, E_PHYSICSBEGINCONTACT2D);
}

void GOC_PhysicRope::HandleBreakContact(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_PhysicRope() - HandleBreakContact : %s(%u) ...", node_->GetName().CString(), node_->GetID());

    CollisionShape2D* cs = (CollisionShape2D*)GetEventSender();

    if (!cs || cs->IsTrigger())
        return;

    ShortIntVector2 mapoint(eventData[MapTileRemoved::MAPPOINT].GetUInt());
    unsigned tileindex = eventData[MapTileRemoved::MAPTILEINDEX].GetUInt();

    if (mapoint == mapInContact1_)
    {
        if (tileindex == anchorTileIndex1_ && shapesInContact1_.Size())
        {
            URHO3D_LOGINFOF("GOC_PhysicRope() - HandleBreakContact : %s(%u) Remove WallContact with cs=%u tileindex=%u", node_->GetName().CString(), node_->GetID(), cs, tileindex);

            if (anchorNode1_)
                anchorNode1_->SetEnabled(false);

            StaticSprite2D* staticSprite = node_->GetComponent<StaticSprite2D>();
            if (staticSprite)
                staticSprite->SetEnabled(false);

            shapesInContact1_.Clear();

            if (!shapesInContact1_.Size() && !shapesInContact2_.Size())
            {
                UnsubscribeFromEvent(MAPTILEREMOVED);
                Detach();
                Release(2.f);
            }

            node_->SendEvent(ABI_RELEASE);
        }
    }

    if (mapoint == mapInContact2_)
    {
        if (tileindex == anchorTileIndex2_ && shapesInContact2_.Size())
        {
            URHO3D_LOGINFOF("GOC_PhysicRope() - HandleBreakContact : %s(%u) Remove WallContact with cs=%u tileindex=%u", node_->GetName().CString(), node_->GetID(), cs, tileindex);

            if (anchorNode2_)
                anchorNode2_->Remove();

            shapesInContact2_.Clear();

            if (!shapesInContact1_.Size() && !shapesInContact2_.Size())
            {
                UnsubscribeFromEvent(MAPTILEREMOVED);
                Detach();
                Release(2.f);
            }

            node_->SendEvent(ABI_RELEASE);
        }
    }
}

void GOC_PhysicRope::DrawDebugGeometry(DebugRenderer* debugRenderer, bool depthTest) const
{
    if (isAttached_ && model_ == RM_FixedBridge)
    {
        World2DInfo* winfo = World2D::GetWorld()->GetWorldInfo();
        Vector2 position;
        winfo->Convert2WorldPosition(mapInContact1_, winfo->GetTileCoords(anchorTileIndex1_), position);
        debugRenderer->AddCross(Vector3(position, 0.f), 1.f, Color::WHITE, depthTest);
        winfo->Convert2WorldPosition(mapInContact2_, winfo->GetTileCoords(anchorTileIndex2_), position);
        debugRenderer->AddCross(Vector3(position, 0.f), 1.f, Color::WHITE, depthTest);
    }
}

