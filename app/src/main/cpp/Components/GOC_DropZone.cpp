#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"
#include "GameNetwork.h"

#include "MapWorld.h"
#include "ViewManager.h"

#include "GOC_ControllerAI.h"
#include "GOC_Collectable.h"
#include "GOC_Inventory.h"
#include "GOC_Move2D.h"
#include "GOC_DropZone.h"

#include "DefsColliders.h"

#define DELAY_BETWEEN_HITS 50
#define DELAY_BETWEEN_SPAWN 0.2f
#define DELAY_INITIALMOVE 0.07f


GOC_DropZone::GOC_DropZone(Context* context) :
    Component(context), dropZone_(0),
    actived_(0), activeStorage_(0), activeThrow_(0), buildObjects_(0), removeParts_(0),
    throwItemsRunning_(false), radius_(0), numHitsToTrig_(3), hitCount_(0), lastHitTime_(0),
    linearVelocity_(10.f), angularVelocity_(3.f), impulse_(0.5f), throwDelay_(0.2f) { }

GOC_DropZone::~GOC_DropZone()
{
//    URHO3D_LOGINFO("~GOC_DropZone()");
    Stop();
}

void GOC_DropZone::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_DropZone>();
    URHO3D_ACCESSOR_ATTRIBUTE("Is Actived", GetActived, SetActived, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Active Storage", GetActiveStorage, SetActiveStorage, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Active Throw Stored Items", GetActiveThrowItems, SetActiveThrowItems, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Active Build Objects From Parts", GetBuildObjects, SetBuildObjects, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Active Remove Parts After Build", GetRemoveParts, SetRemoveParts, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Radius", GetRadius, SetRadius, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Number Hits Before Trig", GetNumHitBeforeTrig, SetNumHitBeforeTrig, int, 3, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Throw Linear Velocity", GetThrowLinearVelocity, SetThrowLinearVelocity, float, 10.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Throw Angular Velocity", GetThrowAngularVelocity, SetThrowAngularVelocity, float, 3.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Throw Impulse", GetThrowImpulse, SetThrowImpulse, float, 0.5f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Time Between throw", GetThrowDelay, SetThrowDelay, float, 0.2f, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Storage", GetStorageAttr, SetStorageAttr, VariantVector, Variant::emptyVariantVector, AM_FILE);
}

void GOC_DropZone::OnSetEnabled()
{
    if (IsEnabledEffective())
    {
        if (actived_ && dropZone_)
        {
//            URHO3D_LOGINFOF("GOC_DropZone() - OnSetEnabled : %s(%u) actived(%s) && dropZone(%s) !",
//                     GetNode()->GetName().CString(), GetNode()->GetID(), actived_ ? "true":"false", dropZone_ ? "true":"false");

            SubscribeToEvent(GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_DropZone, HandleContact));
        }
        else
        {
//            URHO3D_LOGINFOF("GOC_DropZone() - OnSetEnabled : %s(%u) actived(%s) && dropZone(%s) => UnsubscribeEvents !",
//                     GetNode()->GetName().CString(), GetNode()->GetID(), actived_ ? "true":"false", dropZone_ ? "true":"false");
            UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
        }
    }
    else
    {
//        URHO3D_LOGINFO("GOC_DropZone() - OnSetEnabled - not enabled => not actived and UnsubscribeEvents !");
//        actived_ = false;
//        dropZone_ = 0;

        UnsubscribeFromAllEvents();
    }
}

void GOC_DropZone::OnNodeSet(Node* node)
{
//    URHO3D_LOGINFOF("GOC_DropZone() - OnNodeSet : %u", node);
    if (node)
    {
        Clear();
        SetActived(actived_);
    }
    else
        SetActived(false);
}

void GOC_DropZone::CleanDependences()
{
    Clear();
}

void GOC_DropZone::SetActived(bool actived)
{
    if (actived_ == actived)
        return;

//    URHO3D_LOGINFOF("GOC_DropZone() - SetActived : %u", actived);
    actived_ = actived;

    OnSetEnabled();
}

void GOC_DropZone::SetActiveStorage(bool activeStorage)
{
    if (activeStorage_ == activeStorage)
        return;

//    URHO3D_LOGINFOF("GOC_DropZone() - SetActiveStorage : %u", activeStorage);
    activeStorage_ = activeStorage;
}

void GOC_DropZone::SetActiveThrowItems(bool activeThrow)
{
    if (activeThrow_ != activeThrow)
        activeThrow_ = activeThrow;
}

void GOC_DropZone::SetBuildObjects(bool buildobjects)
{
    if (buildObjects_ != buildobjects)
        buildObjects_ = buildobjects;
}

void GOC_DropZone::SetRemoveParts(bool removeparts)
{
    if (removeParts_ != removeparts)
        removeParts_ = removeparts;
}

void GOC_DropZone::SetRadius(float radius)
{
    if (radius != 0.f && radius != radius_)
    {
//        URHO3D_LOGINFOF("GOC_DropZone() - SetRadius : %f - Set collisionCircle", radius);
        body_ = GetComponent<RigidBody2D>();
        assert(body_);

        if (!dropZone_)
        {
            // Get collisionCircle if is trigger
            PODVector<CollisionCircle2D*> collisionCircles;
            GetNode()->GetComponents<CollisionCircle2D>(collisionCircles);
            PODVector<CollisionCircle2D*>::Iterator it = collisionCircles.Begin();

            while (!((*it)->IsTrigger() || it == collisionCircles.End())) it++;
            dropZone_ = (it != collisionCircles.End() ? *it : 0);

            // else create a new trigger collision circle
            if (!dropZone_)
            {
                dropZone_ = GetNode()->CreateComponent<CollisionCircle2D>();
                dropZone_->SetTrigger(true);
            }
        }
        dropZone_->SetRadius(radius);
        radius_ = radius;
    }

    if (radius == 0.f)
    {
        radius_ = 0.f;
        dropZone_ = 0;
    }
}

void GOC_DropZone::SetNumHitBeforeTrig(int numHits)
{
    if (numHitsToTrig_ != numHits)
        numHitsToTrig_ = numHits;
}

void GOC_DropZone::SetThrowLinearVelocity(float vel)
{
    if (linearVelocity_ != vel)
        linearVelocity_ = vel;
}

void GOC_DropZone::SetThrowAngularVelocity(float ang)
{
    if (angularVelocity_ != ang)
        angularVelocity_ = ang;
}

void GOC_DropZone::SetThrowImpulse(float imp)
{
    if (impulse_ != imp)
        impulse_ = imp;
}

void GOC_DropZone::SetThrowDelay(float delay)
{
    if (throwDelay_ != delay)
        throwDelay_ = delay;
}

bool GOC_DropZone::GetActived() const
{
    return actived_;
}

bool GOC_DropZone::GetActiveStorage() const
{
    return activeStorage_;
}

bool GOC_DropZone::GetActiveThrowItems() const
{
    return activeThrow_;
}

bool GOC_DropZone::GetBuildObjects() const
{
    return buildObjects_;
}

bool GOC_DropZone::GetRemoveParts() const
{
    return removeParts_;
}

float GOC_DropZone::GetRadius() const
{
    return dropZone_ ? dropZone_->GetRadius() : 0.f;
}

int GOC_DropZone::GetNumHitBeforeTrig() const
{
    return numHitsToTrig_;
}

float GOC_DropZone::GetThrowLinearVelocity() const
{
    return linearVelocity_;
}

float GOC_DropZone::GetThrowAngularVelocity() const
{
    return angularVelocity_;
}

float GOC_DropZone::GetThrowImpulse() const
{
    return impulse_;
}

float GOC_DropZone::GetThrowDelay() const
{
    return throwDelay_;
}

void GOC_DropZone::Clear()
{
    items_.Clear();
    itemsAttr_.Clear();
    itemsNodes_.Clear();
}

void GOC_DropZone::Stop()
{
    Clear();
    UnsubscribeFromAllEvents();
}

bool GOC_DropZone::AddItem(const Slot& slot)
{
//    if (!activeStorage_)
//    {
//        URHO3D_LOGERRORF("GOC_DropZone() - AddItem : not actived storage !");
//        return false;
//    }

    String slotstr = Slot::GetSlotAttr(slot);
    if (!slotstr.Empty())
    {
//        URHO3D_LOGINFOF("GOC_DropZone() - AddItem : %s", slotstr.CString());
        items_.Push(slot);
        itemsAttr_.Push(slotstr);

        return true;
    }

    return false;
}

void GOC_DropZone::AddItemAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_DropZone() - AddItemAttr (%s)", value.CString());

    if (value.Empty())
        return;

    Slot slot;

    if (Slot::SetSlotAttr(value, slot))
    {
        AddItem(slot);
//        URHO3D_LOGERRORF("GOC_DropZone() - AddItemAttr : %s(%u) %s OK !", node_->GetName().CString(), node_->GetID(), value.CString());
    }
//    else
//        URHO3D_LOGERRORF("GOC_DropZone() - AddItemAttr : %s(%u) %s NOK !", node_->GetName().CString(), node_->GetID(), value.CString());
}

void GOC_DropZone::SetStorageAttr(const VariantVector& value)
{
//    URHO3D_LOGINFOF("GOC_DropZone() - SetStorageAttr");
    for (unsigned i=0; i < value.Size(); ++i)
        AddItemAttr(value[i].GetString());
//    URHO3D_LOGINFOF("GOC_DropZone() - SetStorageAttr OK!");
}

VariantVector GOC_DropZone::GetStorageAttr() const
{
    return Slot::GetSlotDatas(items_);
}

void GOC_DropZone::HandleContact(StringHash eventType, VariantMap& eventData)
{
    assert(body_);

    if (eventType != E_PHYSICSBEGINCONTACT2D)
        return;

//    URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : %s(%u) E_PHYSICSBEGINCONTACT2D ...", node_->GetName().CString(), node_->GetID());

    using namespace PhysicsBeginContact2D;

    const ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[P_CONTACTINFO].GetUInt());

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

    if (!csBody || !csOther)
        return;

    if (!csBody->IsTrigger() || csBody != dropZone_)
        return;

//    URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : csBody is trigger && is dropzone");

    if (!csOther->IsTrigger() && !csOther->GetNode()->GetName().StartsWith(TRIGATTACK))
    {
        // if drop collectable on the trigger, add it to storage
        GOC_Collectable* collectable = other->GetNode()->GetComponent<GOC_Collectable>();
        if (!collectable)
            return;

//        URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : csOther is collectable");

        if (activeStorage_ && !GameContext::Get().ClientMode_)
        {
            if (AddItem(collectable->Get()))
            {
                // Spawn Effect
                Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
                GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_GREENSPIRAL], drawable->GetLayer(), drawable->GetViewMask(), node_->GetWorldPosition2D(), 90.f, 1.f, true, 0.5f, Color::WHITE, LOCAL);
                GameHelpers::SpawnSound(GetNode(), "Sounds/096-Attack08.ogg");
                hitCount_ = 0;

                if (GameContext::Get().ServerMode_)
                {
                    VariantMap& eventData = context_->GetEventDataMap();
                    eventData[Go_StorageChanged::GO_ACTIONTYPE] = 1;
                    eventData[Net_ObjectCommand::P_NODEID] = node_->GetID();
                    eventData[Net_ObjectCommand::P_DATAS] = Slot::GetSlotDatas(items_);
                    node_->SendEvent(GO_STORAGECHANGED, eventData);
                }

                other->GetNode()->SendEvent(GO_INVENTORYEMPTY);
//                Dump();
            }
        }
    }
    else
    {
        URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : csOther is trigger");

        if (items_.Empty())
        {
            URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : no items to throw out !", hitCount_);
            return;
        }

        // if Attack , Spawn storage and clear
        if (csOther->GetNode()->GetName().StartsWith(TRIGATTACK))
        {
            unsigned time = Time::GetSystemTime();
            unsigned delta = time - lastHitTime_;
            URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : on Trig_Attack time=%u lashit=%u delta=%u(%u) hitcount=%u(%u)",
                     time, lastHitTime_, delta, DELAY_BETWEEN_HITS, hitCount_, numHitsToTrig_);

            if (delta > DELAY_BETWEEN_HITS)
            {
                hitCount_++;
                lastHitTime_ = time;
                URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : hitCount=%u", hitCount_);
            }

            if (hitCount_ == numHitsToTrig_)
            {
                hitCount_ = 0;

                const bool skipBuild = GameContext::Get().ClientMode_ && other->GetNode()->GetVar(GOA::CLIENTID).GetInt() != GameNetwork::Get()->GetClientID();
                if (skipBuild)
                    URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : otherbody is not the client ! skip !");

                if (buildObjects_ && !skipBuild)
                {
                    Vector<StringHash> buildableObjectTypes;
                    SearchForBuildableObjects(items_, buildableObjectTypes);

                    unsigned num_BuildableObj = buildableObjectTypes.Size();
                    if (num_BuildableObj)
                    {
                        const StringHash& got = buildableObjectTypes[num_BuildableObj > 1 ? (unsigned) Random(0, num_BuildableObj) : 0];

                        URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : build a %s(%u)", GOT::GetType(got).CString(), got.Value());

                        /// TODO : apply character maps following founded items
                        EquipmentList equipment;
                        int entityid = SelectBuildableEntity(got, items_, equipment);
                        if (entityid >= 0)
                        {
                            PhysicEntityInfo physicInfo;
                            physicInfo.positionx_ = node_->GetWorldPosition2D().x_;
                            physicInfo.positiony_ = node_->GetWorldPosition2D().y_+1.f;
                            SceneEntityInfo sceneInfo;
                            sceneInfo.equipment_ = &equipment;
                            Node* node = World2D::SpawnEntity(got, entityid, 0, node_->GetID(), csBody->GetViewZ(), physicInfo, sceneInfo);
                            if (node)
                            {
//                                if (GOT::GetTypeProperties(got) & GOT_Controllable)
                                {
                                    URHO3D_LOGINFOF("GOC_DropZone() - HandleContact : add a %s follower", GOT::GetType(got).CString());
                                    GOC_AIController* gocFollower = node->GetOrCreateComponent<GOC_AIController>();
                                    gocFollower->Stop();

                                    gocFollower->SetControllerType(GO_AI_Ally, true);
//                                    gocFollower->SetTarget();
                                    gocFollower->SetBehavior(GOB_FOLLOW);
                                    gocFollower->Start();

                                    GOC_Move2D* gocMove = node->GetComponent<GOC_Move2D>();
                                    if (gocMove)
                                        gocMove->SetMoveType(MOVE2D_WALKANDJUMPFIRSTBOXANDSWIM);
                                }

                                if (removeParts_)
                                    RemovePartsOfBuildableObject(got);

                                /// TODO : Remove items used to spawn the entity
                                // Spawn Effect
                                Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
                                GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_GREENSPIRAL], drawable->GetLayer(), drawable->GetViewMask(), node_->GetWorldPosition2D(), 90.f, 1.f, true, 0.5f, Color::WHITE, LOCAL);
                            }
                            else
                                URHO3D_LOGERRORF("GOC_DropZone() - HandleContact : impossible to spawn a %s(%u) !", GOT::GetType(got).CString(), got.Value());
                        }
                        else
                            URHO3D_LOGERRORF("GOC_DropZone() - HandleContact : impossible to spawn a %s(%u) !", GOT::GetType(got).CString(), got.Value());
                    }
                }

                if (activeThrow_ && !throwItemsRunning_ && !GameContext::Get().ClientMode_)
                    ThrowOutItems();
            }
        }
    }
}

void GOC_DropZone::ThrowOutItems()
{
    if (items_.Empty())
    {
//        URHO3D_LOGINFOF("GOC_DropZone() - ThrowOutItems : items_ empty !");
        return;
    }

    URHO3D_LOGINFOF("GOC_DropZone() - ThrowOutItems");

    throwItemsRunning_ = true;

    throwItemsBeginIndex_ = 0;
    throwItemsEndIndex_ = 0;
    throwItemsTimer_ = 0;

    if (GameContext::Get().ServerMode_)
    {
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[Go_StorageChanged::GO_ACTIONTYPE] = 2;
        eventData[Net_ObjectCommand::P_NODEID] = node_->GetID();
        node_->SendEvent(GO_STORAGECHANGED, eventData);
    }

    SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_DropZone, HandleThrowOutItems));
    UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);
}

void GOC_DropZone::HandleThrowOutItems(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_DropZone() - HandleThrowOutItems");
    throwItemsTimer_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    // Spawn Items with Delay Between each one
    if (throwItemsEndIndex_ < items_.Size())
        if (throwItemsTimer_ > DELAY_BETWEEN_SPAWN * throwItemsEndIndex_)
        {
//            URHO3D_LOGINFOF("GOC_DropZone() - HandleThrowOutItems - Spawn item index=%u", throwItemsEndIndex_);
            // SPAWN item
            if (!itemsAttr_[throwItemsEndIndex_].Empty())
            {
                WeakPtr<Node> node(GOC_Collectable::DropSlotFrom(GetNode(), items_[throwItemsEndIndex_], true, 1));

                throwItemsEndIndex_++;
                itemsNodes_.Push(node);
            }
        }

    // Handle initial move (geyser)
    float offset;
    unsigned j = throwItemsBeginIndex_;
    for (unsigned i = j; i < throwItemsEndIndex_; ++i)
    {
        if (!itemsNodes_[i]) continue;
        if (throwItemsTimer_ > throwDelay_ * i + DELAY_INITIALMOVE)
        {
            throwItemsBeginIndex_ = i+1;
            continue;
        }
        RigidBody2D* body = itemsNodes_[i]->GetComponent<RigidBody2D>();
        if (!body) continue;
        const Vector2& position = itemsNodes_[i]->GetWorldPosition2D();
        offset = IsPowerOfTwo(i) ? 1.f : -1.f;
        body->SetLinearVelocity(Vector2(offset, linearVelocity_));
        if (!body->IsFixedRotation())
            body->SetAngularVelocity(offset * angularVelocity_);
        body->ApplyLinearImpulse(Vector2(0.f, impulse_), position, true);
    }

    // At End of spawn - stop initial move
    if (throwItemsTimer_ > throwDelay_ * items_.Size() + DELAY_INITIALMOVE)
    {
//        URHO3D_LOGINFOF("GOC_DropZone() - HandleThrowOutItems : Stop Initial Move !");
        for (unsigned i=0; i < itemsNodes_.Size(); ++i)
        {
            if (!itemsNodes_[i]) continue;
            RigidBody2D* body = itemsNodes_[i]->GetComponent<RigidBody2D>();
            body->GetBody()->SetLinearDamping(1.5f);
            if (!body->IsFixedRotation())
                body->GetBody()->SetAngularDamping(1.5f);
        }

        throwItemsRunning_ = false;

        Clear();
        UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
        SubscribeToEvent(GetNode(), E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_DropZone, HandleContact));
    }
//    URHO3D_LOGINFOF("GOC_DropZone() - HandleThrowOutItems OK");
}

void GOC_DropZone::NetClientUpdateStorage(unsigned servernodeid, VariantMap& eventData)
{
    // TODO : Attention, il faut que le nodeid soit bien identique sur le serveur et les clients.
    // ce qui ne sera pas le cas pour les Furnitures spawnées au fur et à mesure !
    // il faudrait alors declaré un ObjectControl ? et utiliser GetServerObjectControl pour obtenir un node correspondant au servernodeid

    int actiontype = eventData[Go_StorageChanged::GO_ACTIONTYPE].GetInt();
    Node* node = GameContext::Get().rootScene_->GetNode(servernodeid);


    URHO3D_LOGINFOF("GOC_DropZone() - NetClientUpdateStorage : servernodeid=%u actiontype=%d ...", servernodeid, actiontype);

    if (!node)
        return;

    GOC_DropZone* dropzone = node->GetComponent<GOC_DropZone>();
    if (!dropzone)
        return;

    if (actiontype == 1)
    {
        dropzone->Clear();
        dropzone->SetStorageAttr(eventData[Net_ObjectCommand::P_DATAS].GetVariantVector());

        Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
        GameHelpers::SpawnParticleEffect(GameContext::Get().context_, ParticuleEffect_[PE_GREENSPIRAL], drawable->GetLayer(), drawable->GetViewMask(), node->GetWorldPosition2D(), 90.f, 1.f, true, 0.5f, Color::WHITE, LOCAL);
        GameHelpers::SpawnSound(node, "Sounds/096-Attack08.ogg");
        dropzone->hitCount_ = 0;
    }
    else if (actiontype == 2)
    {
        if (!dropzone->throwItemsRunning_)
        {
            dropzone->ThrowOutItems();
        }
    }
    else if (actiontype == 3)
    {
        dropzone->Clear();
        dropzone->SetStorageAttr(eventData[Net_ObjectCommand::P_DATAS].GetVariantVector());
    }
}

void GOC_DropZone::Dump()
{
//    URHO3D_LOGINFOF("GOC_DropZone() - DumpAll : items_ Size =%u", items_.Size());
//    for (unsigned i=0;i<items_.Size();++i)
//    {
//        const Slot& s = items_[i];
//        URHO3D_LOGINFOF(" => items_[%u] = (collectable=%u;quantity=%d;sprite=%u)",i,
//                 s.type_.Value(), s.quantity_, s.sprite_.Get());
//        if (s.sprite_)
//            URHO3D_LOGINFOF("   => sprite_ = (scale=%f,%f;name=%s)",
//                 s.scale_.x_, s.scale_.y_, s.sprite_->GetName().CString());
//    }

    URHO3D_LOGINFOF("GOC_DropZone() - DumpAll : itemsAttr_ Size =%u", itemsAttr_.Size());
    for (unsigned i=0; i<itemsAttr_.Size(); ++i)
        URHO3D_LOGINFOF("   => itemsAttr_[%u] = %s", i, itemsAttr_[i].CString());

}

void GOC_DropZone::SearchForBuildableObjects(const Vector<Slot>& slots, Vector<StringHash>& buildableObjectTypes)
{
    Vector<String> parts;
    Vector<StringHash> foundObjectTypes;

    for (Vector<Slot>::ConstIterator it=slots.Begin(); it!=slots.End(); ++it)
    {
        const Slot& slot = *it;
        if (!(GOT_Part & GOT::GetTypeProperties(slot.type_))) continue;

        if (GOT::HasObject(slot.partfromtype_))
            if (!foundObjectTypes.Contains(slot.partfromtype_))
                foundObjectTypes.Push(slot.partfromtype_);

        parts.Push(slot.sprite_->GetName());
    }

//    URHO3D_LOGINFOF("GOC_DropZone() - SearchForBuildableObjects : foundObjectTypes num=%u", foundObjectTypes.Size());
    if (foundObjectTypes.Empty()) return;

    unsigned numParts = parts.Size();
//    URHO3D_LOGINFOF("GOC_DropZone() - SearchForBuildableObjects : parts num=%u", numParts);

    for (Vector<StringHash>::ConstIterator it=foundObjectTypes.Begin(); it!=foundObjectTypes.End(); ++it)
    {
        const Vector<String>& partsMap = *(GOT::GetPartsNamesFor(*it));
        int numPartsRequired = partsMap.Size()-1;

//        URHO3D_LOGINFOF("GOC_DropZone() - SearchForBuildableObjects : buildableObject=%s ... numPartsRequired = %d ...", GOT::GetType(*it).CString(), numPartsRequired);

        /*
        if (numParts < numPartsRequired)
            continue;
        */

        int found(0);
        for (Vector<String>::ConstIterator jt=partsMap.Begin(); jt!=partsMap.End(); ++jt)
        {
            if (parts.Contains(*jt))
                found++;
        }

//        URHO3D_LOGINFOF("GOC_DropZone() - SearchForBuildableObjects : buildableObject=%s ... numPartsFounded = %d", GOT::GetType(*it).CString(), found);

        //if (found >= numPartsRequired)
        buildableObjectTypes.Push(*it);
    }

//    URHO3D_LOGINFOF("GOC_DropZone() - SearchForBuildableObjects : buildableObjectTypes Num=%u", buildableObjectTypes.Size());
}

int GOC_DropZone::SelectBuildableEntity(const StringHash& got, const Vector<Slot>& slots, EquipmentList& equipment)
{
    // 1 for skeleton warrior for the moment
    int entityid = 1;

    Node* node = GOT::GetObject(got);
    if (!node)
        return -1;

    AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
    if (!animatedSprite)
        return 0;

    if (animatedSprite->GetNumSpriterEntities() == 1)
        return 0;

    const HashMap<StringHash, unsigned >* entityselection = GOT::GetBuildableEntitySelection(got);

    // find an entity id corresponding with the given items
    if (entityselection)
    {
        for (Vector<Slot>::ConstIterator it=slots.Begin(); it!=slots.End(); ++it)
        {
            const StringHash& type = it->type_;
            if (GOT_Part & GOT::GetTypeProperties(type))
                continue;

            URHO3D_LOGINFOF("GOC_DropZone() - SelectBuildableEntity ... try to find a entityid for type=%s", GOT::GetType(type).CString());

            HashMap<StringHash, unsigned >::ConstIterator jt = entityselection->Find(GOT::GetConstInfo(type).category_);
            if (jt != entityselection->End())
            {
                entityid = (int)jt->second_;
                URHO3D_LOGINFOF("GOC_DropZone() - SelectBuildableEntity ... find an entityid=%d", entityid);
            }
        }

        URHO3D_LOGINFOF("GOC_DropZone() - SelectBuildableEntity ... entityid=%d", entityid);
    }

    // set the equipment list with the equipable items
    const GOC_Inventory_Template* equipmentTemplate = GOC_Inventory_Template::Get(EQUIPMENTTEMPLATE);
    if (equipmentTemplate)
    {
        HashMap<unsigned, StringHash> acceptedSlots;
        for (Vector<Slot>::ConstIterator it=slots.Begin(); it!=slots.End(); ++it)
        {
            const StringHash& type = it->type_;

            // Skip parts
            if (GOT_Part & GOT::GetTypeProperties(type))
                continue;

            equipmentTemplate->GetAcceptedSlotsFor(type, acceptedSlots);
        }

        for (HashMap<unsigned, StringHash>::ConstIterator it=acceptedSlots.Begin(); it!=acceptedSlots.End(); ++it)
        {
            const String& slotname = equipmentTemplate->GetSlotName(it->first_);
            const StringHash& item = it->second_;
            equipment.Push(EquipmentPart(slotname, item));
//            URHO3D_LOGINFOF("GOC_DropZone() - SelectBuildableEntity ... add equipment=%s(%u) to slot=%s", GOT::GetType(item).CString(), item.Value(), slotname.CString());
        }
    }

    return entityid;
}

void GOC_DropZone::RemovePartsOfBuildableObject(const StringHash& buildableObjectType)
{
    if (!items_.Size())
        return;

//    URHO3D_LOGINFOF("GOC_DropZone() - RemovePartsOfBuildableObject : ...");

    const Vector<String>& partsMap = *(GOT::GetPartsNamesFor(buildableObjectType));
    for (Vector<String>::ConstIterator it=partsMap.Begin(); it!=partsMap.End(); ++it)
    {
        const String& partName = *it;

        Vector<Slot>::Iterator jt=items_.Begin();
        while (jt != items_.End())
        {
            Slot& slot = *jt;

            if (!slot.sprite_)
            {
                itemsAttr_.Erase(jt-items_.Begin());
                jt = items_.Erase(jt);
                continue;
            }

            if (slot.sprite_->GetName() == partName)
            {
                slot.quantity_--;
                if (slot.quantity_ <= 0)
                {
                    itemsAttr_.Erase(jt-items_.Begin());
                    jt = items_.Erase(jt);
                }
                break;
            }

            ++jt;
        }
    }

    if (GameContext::Get().ServerMode_)
    {
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[Go_StorageChanged::GO_ACTIONTYPE] = 3;
        eventData[Net_ObjectCommand::P_NODEID] = node_->GetID();
        eventData[Net_ObjectCommand::P_DATAS] = Slot::GetSlotDatas(items_);
        node_->SendEvent(GO_STORAGECHANGED, eventData);
    }
//    Dump();

//    URHO3D_LOGINFOF("GOC_DropZone() - RemovePartsOfBuildableObject : ... OK !");
}
