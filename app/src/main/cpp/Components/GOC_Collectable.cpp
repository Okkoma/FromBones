#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Collide2D.h"
#include "GOC_Inventory.h"
#include "MapWorld.h"
#include "ViewManager.h"

#include "GOC_Collectable.h"


void GOC_Collectable::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Collectable>();
    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Catchers", GetCatcherAttr, SetCatcherAttr, String, String::EMPTY, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Add To Slot", GetSlotAttr, AddToSlotAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ATTRIBUTE("AutoDestroy", bool, selfDestroy_, true, AM_DEFAULT);
    URHO3D_ATTRIBUTE("WallCollide", bool, wallCollide_, false, AM_DEFAULT);
}

GOC_Collectable::GOC_Collectable(Context* context) :
    Component(context),
    selfDestroy_(true),
    wallCollide_(false)
{
    Init();
}

void GOC_Collectable::Init()
{
//    URHO3D_LOGINFOF("GOC_Collectable() - Init");

    Set();
}

void GOC_Collectable::Set()
{
//    URHO3D_LOGINFOF("GOC_Collectable() - Set");
    // Set variables with constProps by default
}

GOC_Collectable::~GOC_Collectable()
{
//    URHO3D_LOGINFOF("~GOC_Collectable()");
    UnsubscribeFromAllEvents();
}

void GOC_Collectable::SetCatcherAttr(const String& value)
{
    if (value.Empty())
        return;

    Vector<String> values = value.Split('|');
    for (unsigned i=0; i < values.Size(); i++)
    {
        StringHash catcher(values[i]);
        if (!catchers_.Contains(catcher))
            catchers_.Push(catcher);
    }
}

String GOC_Collectable::GetCatcherAttr() const
{
    if (!catchers_.Size())
        return String::EMPTY;

    String value;
    for (unsigned i=0; i < catchers_.Size(); i++)
    {
        value += GOT::GetType(catchers_[i]);
        if (i < catchers_.Size()-1)
            value += "|";
    }

    return value;
}

void GOC_Collectable::AddToSlotAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_Collectable() - AddToSlotAttr %s", value.CString());

    if (value.Empty())
        return;

    if (!Slot::SetSlotAttr(value, slot_))
    {
        URHO3D_LOGERRORF("GOC_Collectable() - AddToSlotAttr : %s NOK !", value.CString());
        return;
    }

//    URHO3D_LOGINFOF("GOC_Collectable() - AddToSlotAttr : node=%s(%u) set slot type=%s qty=%d", node_->GetName().CString(), node_->GetID(),
//                    GOT::GetType(slot_.type_).CString(), slot_.quantity_);
}

String GOC_Collectable::GetSlotAttr() const
{
    return Slot::GetSlotAttr(slot_);
}

void GOC_Collectable::SetSpriteProps(const StaticSprite2D* staticSprite)
{
    slot_.sprite_ = staticSprite->GetSprite();
//    slot_.UpdateUISprite();
    slot_.color_ = staticSprite->GetColor();
    Node* node = staticSprite->GetNode();
    if (node)
        slot_.scale_ = node->GetWorldScale();
}

void GOC_Collectable::SetSlot(const StringHash& type, unsigned int quantity, const StringHash& fromtype)
{
    slot_.Set(type, quantity, fromtype);
//    URHO3D_LOGINFOF("GOC_Collectable() - SetSlot : node=%s(%u) set slot type=%s qty=%d", node_->GetName().CString(), node_->GetID(),
//                    GOT::GetType(slot_.type_).CString(), slot_.quantity_);
}

bool GOC_Collectable::Empty() const
{
    return slot_.quantity_ <= 0;
}

bool GOC_Collectable::CheckEmpty()
{
    if (Empty())
    {
//        URHO3D_LOGINFOF("GOC_Collectable() - CheckEmpty : %s(%u) - Slot Empty (Q=%d) ! ", node_->GetName().CString(), node_->GetID(), slot_.quantity_);
        node_->SendEvent(GO_INVENTORYEMPTY);
        return true;
    }
    return false;
}

void GOC_Collectable::TransferSlotTo(Slot& slotToGet, Node* nodeGiver, Node* nodeGetter, const Variant& position, unsigned int quantity)
{
    if (!slotToGet.type_.Value())
    {
        URHO3D_LOGWARNINGF("GOC_Collectable() - TransferSlotTo() : nodeGiver=%s(%u) to nodeGetter=%s(%u) slotToGet.type=0 !",
                           nodeGiver ? nodeGiver->GetName().CString() : "", nodeGiver ? nodeGiver->GetID() : 0,
                           nodeGetter ? nodeGetter->GetName().CString() : "", nodeGetter ? nodeGetter->GetID() : 0);
        return;
    }

    unsigned int qty;
    if (quantity == QTY_MAX)
        quantity = slotToGet.quantity_;

    GOC_Inventory* toInventory = nodeGetter ? nodeGetter->GetComponent<GOC_Inventory>() : 0;
    if (!toInventory)
        return;

    if (!slotToGet.type_.Value())
    {
        qty = slotToGet.TransferTo(0, quantity);
        return;
    }

    Slot* slot = 0;

    ResourceRef ref;
    if (slotToGet.type_ == GOT::MONEY)
    {
        ref.type_ = SpriteSheet2D::GetTypeStatic();
        ref.name_ = "UI/game_equipment.xml@or_pepite04";
    }
    else
    {
        ref = Sprite2D::SaveToResourceRef(slotToGet.sprite_);
        if (ref.name_.Empty())
        {
            URHO3D_LOGWARNINGF("GOC_Collectable() - TransferSlotTo() : nodeGiver=%s(%u) to nodeGetter=%s(%u) no ref for type=%s(%u)",
                               nodeGiver ? nodeGiver->GetName().CString() : "FromManager", nodeGiver ? nodeGiver->GetID() : 0,
                               nodeGetter->GetName().CString(), nodeGetter->GetID(), GOT::GetType(slotToGet.type_).CString(), slotToGet.type_.Value());
            return;
        }
    }

    unsigned idSlotGetter = 0;

    do
    {
        toInventory->GetSlotfor(slot, slotToGet, qty, &idSlotGetter);

        if (slot)
        {
//            URHO3D_LOGINFOF("GOC_Collectable() - TransferSlotTo() : toInventory slot(type=%u, sprite=%s(%u), Q=%u) remainCapacity=%d idSlotGetter=%u",
//                             slotToGet.type_.Value(), slotToGet.sprite_ ? slotToGet.sprite_->GetName().CString() : "", slotToGet.sprite_.Get(), quantity, qty, idSlotGetter);

            qty = slotToGet.TransferTo(slot, quantity, qty);
            if (qty)
            {
//                URHO3D_LOGINFOF("GOC_Collectable() - TransferSlotTo() : nodeGiver=%s(%u) to nodeGetter=%s(%u) qty=%u refname=%s",
//                                nodeGiver ? nodeGiver->GetName().CString() : "FromManager", nodeGiver ? nodeGiver->GetID() : 0,
//                                nodeGetter->GetName().CString(), nodeGetter->GetID(), qty,
//                                ref.name_.CString());

                VariantMap& eventData = nodeGetter->GetContext()->GetEventDataMap();
                eventData[Go_InventoryGet::GO_GIVER] = nodeGiver ? nodeGiver->GetID() : 0;
                eventData[Go_InventoryGet::GO_GETTER] = nodeGetter->GetID();
                eventData[Go_InventoryGet::GO_POSITION] = position;
                eventData[Go_InventoryGet::GO_RESOURCEREF] = ref;
                eventData[Go_InventoryGet::GO_IDSLOTSRC] = 0;
                eventData[Go_InventoryGet::GO_IDSLOT] = idSlotGetter;
                eventData[Go_InventoryGet::GO_QUANTITY] = qty;
                nodeGetter->SendEvent(GO_INVENTORYGET, eventData);
//                nodeGetter->SendEvent(GO_INVENTORYRECEIVE, eventData);

                quantity -= qty;
            }
            // if no qty transfered it's a matter in Slot::TransferTo => exit the loop otherwise it's entering in an infinite loop
            else break;
        }

    }
    while (slot && quantity);

//    URHO3D_LOGINFOF("GOC_Collectable() - TransferSlotTo() : nodeGiver=%s(%u) to nodeGetter=%s(%u) OK !",
//                                nodeGiver ? nodeGiver->GetName().CString() : "FromManager", nodeGiver ? nodeGiver->GetID() : 0, nodeGetter->GetName().CString(), nodeGetter->GetID());
}

VariantMap GOC_Collectable::tempSlotData_;

Node* GOC_Collectable::DropSlotFrom(Node* owner, Slot& slot, bool skipNetSpawn, unsigned int qty, VariantMap* slotData)
{
    if (!slot.Empty())
    {
        const StringHash& got = slot.type_;

        if (!slot.sprite_)
        {
            URHO3D_LOGERRORF("GOC_Collectable() - DropSlotFrom : no sprite in slot %s(%u) !", GOT::GetType(got).CString(), got.Value());
            return 0;
        }

        PhysicEntityInfo physicInfo;
        GameHelpers::GetDropPoint(owner, physicInfo.positionx_, physicInfo.positiony_);

        if (qty > slot.quantity_)
            qty = slot.quantity_;

       URHO3D_LOGINFOF("GOC_Collectable() - DropSlotFrom : type=%s(%u) qty=%u on viewZ=%d droppoint=%F %F ...",
                       GOT::GetType(got).CString(), got.Value(), qty, owner->GetVar(GOA::ONVIEWZ).GetInt(), physicInfo.positionx_, physicInfo.positiony_);

        if (!slotData)
            slotData = &tempSlotData_;

        Slot::GetSlotData(slot, *slotData, qty);

        SceneEntityInfo sceneinfo;
        sceneinfo.skipNetSpawn_ = skipNetSpawn;
        sceneinfo.zindex_ = 1000;

        int viewZ = ViewManager::GetNearViewZ(owner->GetVar(GOA::ONVIEWZ).GetInt());

        Node* node = World2D::SpawnEntity(got, 0, 0, owner->GetID(), viewZ, physicInfo, sceneinfo, slotData);
        if (!node)
        {
            URHO3D_LOGERRORF("GOC_Collectable() - DropSlotFrom : can't Spawn %s(%u) !", GOT::GetType(got).CString(), got.Value());
            return 0;
        }

       URHO3D_LOGINFOF("GOC_Collectable() - DropSlotFrom : type=%s(%u) nodeid=%u qty=%u ... OK !", GOT::GetType(got).CString(), got.Value(), node->GetID(), qty);

        // send DROP for updating player states, if need
        if (owner)
        {
            VariantMap& eventData = GameContext::Get().context_->GetEventDataMap();
            eventData[Go_CollectableDrop::GO_TYPE] = got.Value();
            eventData[Go_CollectableDrop::GO_QUANTITY] = qty;
            owner->SendEvent(GO_COLLECTABLEDROP, eventData);
        }

        // remove quantity from slot
        if (slot.quantity_ > qty)
            slot.quantity_ -= qty;
        else
            slot.Clear();

        return node;
    }
//    else
//    {
//        URHO3D_LOGERRORF("GOC_Collectable() - DropSlotFrom : empty slot !");
//    }

    return 0;
}

void GOC_Collectable::CleanDependences()
{
    // need to be reseted for ObjectPool Use
//    URHO3D_LOGINFOF("GOC_Collectable() - CleanDependences : %s(%u)", node_->GetName().CString(), node_->GetID());
    slot_.Clear();
}

void GOC_Collectable::OnSetEnabled()
{
    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
        {
            /// Keep GOC_Animator2D take care of Destroying node if Empty (via State_Destroy => AACTION_ToDestroy)
            if (selfDestroy_ || !GetNode()->HasComponent("GOC_Animator2D"))
                SubscribeToEvent(GetNode(), GO_INVENTORYEMPTY, URHO3D_HANDLER(GOC_Collectable, HandleEmpty));

            if (!GameContext::Get().ClientMode_)
                SubscribeToEvent(GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Collectable, HandleContact));
        }
        else
        {
            UnsubscribeFromAllEvents();
//            UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
        }
    }
    else
//        UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
        UnsubscribeFromAllEvents();
}

void GOC_Collectable::OnNodeSet(Node* node)
{
    Scene* scene = GetScene();
    if (node)
    {
//        URHO3D_LOGINFO("GOC_Collectable() - OnNodeSet");
        if (scene && IsEnabledEffective())
        {
//            if (GetNode()->GetVar(GOA::TYPECONTROLLER) == Variant::EMPTY)
//                GetNode()->SetVar(GOA::TYPECONTROLLER, GO_Collectable);

            /// Keep GOC_Animator2D take care of Destroying node if Empty (via State_Destroy => AACTION_ToDestroy)
            if (selfDestroy_ || !GetNode()->HasComponent("GOC_Animator2D"))
                SubscribeToEvent(node, GO_INVENTORYEMPTY, URHO3D_HANDLER(GOC_Collectable, HandleEmpty));

            if (!GameContext::Get().ClientMode_)
                SubscribeToEvent(GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Collectable, HandleContact));
        }
        else
            UnsubscribeFromAllEvents();
    }
    else
    {
//        UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
        UnsubscribeFromAllEvents();
    }
}

void GOC_Collectable::HandleEmpty(StringHash eventType, VariantMap& data)
{
//    if (!selfDestroy_)
//        return;

    // Clear slot
    slot_.Clear();

    // Send Destroy Event
//    URHO3D_LOGINFOF("GOC_Collectable() - HandleEmpty : %s(%u) Slot Empty ! SendEvent Destroy !", node_->GetName().CString(), node_->GetID());

    if (World2D::GetWorld())
    {
        node_->SendEvent(WORLD_ENTITYDESTROY);
        return;
    }

    VariantMap eventData;
    eventData[Go_Destroy::GO_ID] = GetNode()->GetID();
    eventData[Go_Destroy::GO_TYPE] = GetNode()->GetVar(GOA::TYPECONTROLLER).GetInt();
    eventData[Go_Destroy::GO_MAP] = GetNode()->GetVar(GOA::ONMAP).GetUInt();
    node_->SendEvent(GO_DESTROY, eventData);
}


void GOC_Collectable::HandleContact(StringHash eventType, VariantMap& eventData)
{
    if (!GameContext::Get().AllowUpdate_)
    {
//        URHO3D_LOGWARNINGF("GOC_Collectable() - HandleContact : %s(%u) ! GameContext::Get().AllowUpdate_=false => no contact allowed", node_->GetName().CString(), node_->GetID());
        return;
    }

    if (eventType != E_PHYSICSBEGINCONTACT2D)
        return;

    using namespace PhysicsBeginContact2D;

    const ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[P_CONTACTINFO].GetUInt());

    RigidBody2D* body = GetComponent<RigidBody2D>();
    RigidBody2D* b1 = cinfo.bodyA_;
    RigidBody2D* b2 = cinfo.bodyB_;
    RigidBody2D* other = 0;
    CollisionShape2D* csBody = 0;
    CollisionShape2D* csOther = 0;
    unsigned index;

    if (b1 == body)
    {
        if (cinfo.shapeA_ && cinfo.shapeA_->IsTrigger())
            return;

        other = b2;
        csBody = cinfo.shapeA_;
        csOther = cinfo.shapeB_;
        index = cinfo.iShapeB_;
    }
    else if (b2 == body)
    {
        if (cinfo.shapeB_ && cinfo.shapeB_->IsTrigger())
            return;

        other = b1;
        csBody = cinfo.shapeB_;
        csOther = cinfo.shapeA_;
        index = cinfo.iShapeA_;
    }
    else
    {
        return;
    }

//    const String& nameBody = body->GetNode()->GetName();
//    const String& nameOther = other->GetNode()->GetName();

//    URHO3D_LOGINFOF("GOC_Collectable() - HandleContact : nodeID=%u %s(%u) contact with node=%s(%u)", node_->GetID(), GOT::GetType(slot_.type_).CString(),
//                    slot_.type_.Value(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());

    if (other->GetNode()->GetVar(GOA::TYPECONTROLLER).GetInt() != GO_None && other->GetNode()->GetVar(GOA::ISDEAD).GetBool() == false)
    {
        bool catched = false;

        if (catchers_.Size())
        {
            GOC_Collide2D* goccollider  = other->GetNode()->GetComponent<GOC_Collide2D>();
            if (goccollider && goccollider->GetLastAttackedNode() == node_)
            {
                URHO3D_LOGINFOF("GOC_Collectable() - HandleContact : %s(%u) try transfer slot to node=%s(%u) ... check catchers ...", GOT::GetType(slot_.type_).CString(),
                                slot_.type_.Value(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());

                GOC_Inventory* gocinventory = other->GetNode()->GetComponent<GOC_Inventory>();
                if (gocinventory)
                {
                    int slotindex = gocinventory->GetSlotIndex(CMAP_WEAPON1);
                    if (slotindex != -1)
                    {
                        if (catchers_.Contains(gocinventory->GetSlot(slotindex).type_))
                        {
                            catched = true;
                            goccollider->ResetLastAttack();
                        }
                    }
                }
            }
        }
        else if (csOther && !csOther->IsTrigger())
        {
            catched = true;
        }

        if (catched)
        {
//            URHO3D_LOGINFOF("GOC_Collectable() - HandleContact : %s(%u) try transfer slot to node=%s(%u)", GOT::GetType(slot_.type_).CString(),
//                            slot_.type_.Value(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());

            // Locked for the moment : No more ressources to collect
            UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);

            TransferSlotTo(slot_, node_, other->GetNode(), Variant(GetNode()->GetWorldPosition2D()));

            // Unlocked if not Empty
            if (!CheckEmpty())
            {
                SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Collectable, HandleContact));

                if (!body->IsFixedRotation())
                {
                    Vector2 position(node_->GetWorldPosition2D());
                    float force(Pow((position.x_ - cinfo.contactPoint_.x_) > 0.001f ? 10.f * (position.x_ - cinfo.contactPoint_.x_) : 0.f, 2.f) +
                                Pow((position.y_ - cinfo.contactPoint_.y_) > 0.001f ? 10.f * (position.y_ - cinfo.contactPoint_.y_) : 0.f, 2.f));

                    if (force != 0.f)
                    {
                        force = Clamp(force, 1.f, 100.f);
                        body->SetAwake(true);
                        body->ApplyForce(other->GetLinearVelocity() * force, cinfo.contactPoint_, true);
                        body->SetAngularVelocity(-Sign(position.x_ - cinfo.contactPoint_.x_) * force);

//                        URHO3D_LOGINFOF("GOC_Collectable() - HandleContact : %s(%u) contact with node=%s(%u) position=%F %F contact=%F %F force=%F", GOT::GetType(slot_.type_).CString(),
//                                        slot_.type_.Value(), other->GetNode()->GetName().CString(), other->GetNode()->GetID(), position.x_, position.y_, cinfo.contactPoint_.x_, cinfo.contactPoint_.y_, force);
                    }
                }
            }
        }
    }
    else if (wallCollide_ && csOther->GetColliderInfo())
    {
        if (csOther->GetColliderInfo() == WATERCOLLIDER)
            return;

        if (cinfo.normal_.y_ > 0.f)
        {
            URHO3D_LOGINFOF("GOC_Collectable() - HandleContact : %s(%u) contact with node=%s(%u) ... sendevent=COLLIDEWALLBEGIN", GOT::GetType(slot_.type_).CString(),
                            slot_.type_.Value(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());

            VariantMap& eventData = context_->GetEventDataMap();
            eventData[CollideWallBegin::WALLCONTACTTYPE] = (int)Wall_Ground;
            node_->SendEvent(COLLIDEWALLBEGIN, eventData);
            node_->SendEvent(COLLIDEWALLBEGIN);
        }
    }
}


