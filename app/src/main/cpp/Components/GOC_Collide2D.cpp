#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "DefsColliders.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Move2D.h"
#include "GOC_Destroyer.h"
#include "GOC_Controller.h"
#include "GOC_Life.h"

#include "MAN_Effects.h"
#include "MapWorld.h"
#include "ViewManager.h"
#include "Actor.h"
#include "Player.h"

#include "GOC_Collide2D.h"

//#define CLIENTMODE_INACTIVERIGIBOBIES
//#define ACTIVE_LINEOFSIGHT

//#define DUMP_DEBUG_MAPCOLLIDEUPDATE

extern const char* wallTypeNames[];

void GOC_Collide2D::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Collide2D>();
}


GOC_Collide2D::GOC_Collide2D(Context* context) :
    Component(context),
    body(0),
    numGroundContacts_(0)
{ }

GOC_Collide2D::~GOC_Collide2D()
{ }

void GOC_Collide2D::OnSetEnabled()
{
#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
    URHO3D_LOGINFOF("GOC_Collide2D() - OnSetEnabled : %s(%u) ... scene=%u enable=%u body=%u shapes=%u",
                    node_->GetName().CString(), node_->GetID(), GetScene(), IsEnabledEffective(), body, body->GetCollisionsShapes().Size());
#endif

    if (GetScene() && IsEnabledEffective() && body && body->GetCollisionsShapes().Size())
    {
#ifdef CLIENTMODE_INACTIVERIGIBOBIES
        {
            GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
            if (GameContext::Get().ClientMode_ && (!controller || !controller->IsMainController()))
            {
                body->SetEnabled(false);
                UnsubscribeFromAllEvents();
                return;
            }
            else
                body->SetEnabled(true);
        }
#endif

        ClearContacts();

        unsigned gotprops = GOT::GetTypeProperties(node_->GetVar(GOA::GOT).GetStringHash());
        if (!(gotprops & (GOT_CollectablePart | GOT_Part)))
        {
//        #ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
//            URHO3D_LOGINFOF("GOC_Collide2D() - OnSetEnabled : %s(%u) subscribe To Jump/Fall/Dead ...", node_->GetName().CString(), node_->GetID());
//        #endif
            SubscribeToEvent(node_, EVENT_JUMP, URHO3D_HANDLER(GOC_Collide2D, OnBreakGroundContacts));
            SubscribeToEvent(node_, EVENT_FALL, URHO3D_HANDLER(GOC_Collide2D, OnBreakGroundContacts));
            SubscribeToEvent(node_, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_Collide2D, HandleDead));
        }

//    #ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
//        URHO3D_LOGINFOF("GOC_Collide2D() - OnSetEnabled : %s(%u) subscribe To Begin/End PhysicContact2D ...", node_->GetName().CString(), node_->GetID());
//    #endif
        SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Collide2D, HandleBeginContact));
        SubscribeToEvent(node_, E_PHYSICSENDCONTACT2D, URHO3D_HANDLER(GOC_Collide2D, HandleEndContact));

        // Active all colliders
        SetCollidersEnable(true, CONTACT_ALL);

        lastHurtTime_ = 0.f;
    }
    else
        UnsubscribeFromAllEvents();
}

void GOC_Collide2D::OnNodeSet(Node* node)
{
    if (node)
    {
        body = node->GetComponent<RigidBody2D>();

        OnSetEnabled();
    }
}


void GOC_Collide2D::SetCollidersEnable(bool state, unsigned contactBits, bool inverse)
{
    PODVector<CollisionShape2D*> shapes;
    node_->GetDerivedComponents<CollisionShape2D>(shapes, true);

    unsigned numprocessed = 0;
    for (unsigned i=0; i < shapes.Size(); i++)
    {
        CollisionShape2D* shape = shapes[i];
        if (shape && !shape->IsTrigger())
        {
            if (inverse)
            {
                if ((shape->GetExtraContactBits() & contactBits) == 0)
                {
                    shape->SetEnabled(state);
                    numprocessed++;
                }
            }
            else
            {
                if ((shape->GetExtraContactBits() & contactBits) != 0)
                {
                    shape->SetEnabled(state);
                    numprocessed++;
                }
            }
        }
    }

//    URHO3D_LOGINFOF("GOC_Collide2D() - SetCollidersEnable : Node=%s(%u) ... state=%s ... shapesprocessed=%u/%u ... OK !",
//                    node_->GetName().CString(), node_->GetID(), state?"true":"false", numprocessed, shapes.Size());

    GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
    if (destroyer)
        destroyer->UpdateFilterBits();
}

void GOC_Collide2D::ClearContacts()
{
#if (WALLCONTACTMODE == 0)
    groundContacts.Clear();
#else
//    URHO3D_LOGINFOF("GOC_Collide2D() - ClearContacts : Node=%s(%u) ... clear contacts !", node_->GetName().CString(), node_->GetID());
    wallContacts.Clear();
#endif
    numGroundContacts_ = 0;
    lastVictim_.Reset();
    lastAggressor_.Reset();
}

#if (WALLCONTACTMODE != 0)
void GOC_Collide2D::GetNumContacts(int (&numcontacts)[3]) const // pass the array by reference
{
    for (int i=0; i < 3; i++)
        numcontacts[i] = 0;

    for (HashMap<unsigned, WallContact>::ConstIterator it = wallContacts.Begin(); it != wallContacts.End(); ++it)
    {
        if (it->second_.type_ == Wall_Roof)
            numcontacts[Wall_Roof]++;
        else if (it->second_.type_ == Wall_Border)
            numcontacts[Wall_Border]++;
        else
            numcontacts[Wall_Ground]++;
    }
}
#endif

#if (WALLCONTACTMODE == 0)
void GOC_Collide2D::AddGroundContact2D(CollisionShape2D* cs)
{
    assert(body);

    numGroundContacts_++;

    if (groundContacts.Size() > 0)
    {
        // Search CollisionShape2D in hashmap
        HashMap<CollisionShape2D*, unsigned >::Iterator i = groundContacts.Find(cs);
        if (i != groundContacts.End())
            i->second_++;
        else
            groundContacts += Pair<CollisionShape2D*, unsigned>(cs, 1u);
    }
    else
    {
        groundContacts += Pair<CollisionShape2D*, unsigned>(cs, 1u);

//        URHO3D_LOGINFOF("GOC_Collide2D() - AddGroundContact2D : Node=%s(%u) COLLIDEGROUND !", node_->GetName().CString(), node_->GetID());
        GetNode()->SendEvent(GO_COLLIDEGROUND);
    }

//    URHO3D_LOGINFOF("GOC_Collide2D() - AddGroundContact2D : nb=%d", groundContacts.Size());
//    DumpContacts();
}

void GOC_Collide2D::RemoveGroundContact2D(CollisionShape2D* cs)
{
    assert(body);

    if (groundContacts.Size() > 0)
    {
        numGroundContacts_--;
        // Search shape in hashmap
        HashMap<CollisionShape2D*, unsigned>::Iterator i = groundContacts.Find(cs);
        if (i != groundContacts.End())
        {
            if (i->second_>1)
                i->second_--;
            else
                groundContacts.Erase(i);
        }
    }

//    URHO3D_LOGINFOF("GOC_Collide2D() - RemoveGroundContact2D : nb=%d", groundContacts.Size());
//    DumpContacts();
}
#else
inline unsigned Hash(CollisionShape2D* cs, unsigned ishape)
{
    return (unsigned)(((GameHelpers::ToUIntAddr(cs) & 0xffff) << 16) + (ishape & 0xffff));
}

bool CompareHash(CollisionShape2D* cs, unsigned hash)
{
    return hash >> 16 == (GameHelpers::ToUIntAddr(cs) & 0xffff);
}

inline unsigned Hash(b2Contact* contact)
{
    return (unsigned)(GameHelpers::ToUIntAddr(contact));
}

#if (WALLCONTACTMODE == 1)
void GOC_Collide2D::AddWallContact2D(CollisionShape2D* cs, unsigned ishape, const Vector2& normal)
{
    assert(body);

    if (normal == Vector2::ZERO)
        return;

    unsigned wallContactRef = Hash(cs, ishape);

    WallType walltype = Wall_Border;
    if (Abs(normal.y_) > Abs(normal.x_) || (cs->GetExtraContactBits() & CONTACT_STAIRS))
        walltype = normal.y_ >= 0.f ? Wall_Ground : Wall_Roof;

    bool newContact = true;

//    URHO3D_LOGINFOF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) wallType=%s normal=%s ...",
//                    node_->GetName().CString(), node_->GetID(), wallTypeNames[walltype], normal.ToString().CString());

    if (wallContacts.Size() > 0)
    {
        // Search CollisionShape2D in hashmap
        HashMap<unsigned, WallContact>::Iterator i = wallContacts.Find(wallContactRef);
        if (i != wallContacts.End())
        {
            if (walltype == i->second_.type_)
            {
                i->second_.count_++;
                newContact = false;
#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
                URHO3D_LOGINFOF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) wallType=%s normal=%s cs=%u ishape=%u wref=%u EXISTING ENTRY count=%d numgrd=%d numcontacts=%u !",
                                node_->GetName().CString(), node_->GetID(), wallTypeNames[walltype], normal.ToString().CString(), cs->GetID(), ishape, wallContactRef, i->second_.count_, numGroundContacts_, wallContacts.Size());
#endif
            }
            // Wall Type has been changed (may be a tile has been modified) => reset counter
            else
            {
#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
                URHO3D_LOGWARNINGF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) normal=%s wallType=%s cs=%u ishape=%u => detects walltype changed to %s => stop !",
                                   node_->GetName().CString(), node_->GetID(), normal.ToString().CString(), wallTypeNames[i->second_.type_ ], cs->GetID(), ishape, wallTypeNames[walltype]);
#endif
                return;
//                if (i->second_.type_ == Wall_Ground)
//                    numGroundContacts_--;
//                i->second_.type_ = walltype;
//                i->second_.count_ = 1;
            }
        }
        else
            wallContacts += Pair<unsigned, WallContact>(wallContactRef, WallContact(walltype, 1));
    }
    else
        wallContacts += Pair<unsigned, WallContact>(wallContactRef, WallContact(walltype, 1));

    /*
        if (walltype == Wall_Ground && !numGroundContacts_)
        {
            numGroundContacts_ = 1;
            node_->SendEvent(GO_COLLIDEGROUND);
        }

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
        URHO3D_LOGINFOF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) normal=%s wallType=%s cs=%u ishape=%u wref=%u numgrd=%d numcontacts=%u!",
                        node_->GetName().CString(), node_->GetID(), normal.ToString().CString(), wallTypeNames[walltype], cs->GetID(), ishape, wallContactRef, numGroundContacts_, wallContacts.Size());
#endif

        if (newContact || (walltype == Wall_Ground && numGroundContacts_ == 1))
        {
            SubscribeToEvent(cs, MAPTILEREMOVED, URHO3D_HANDLER(GOC_Collide2D, HandleBreakContact));

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
            URHO3D_LOGINFOF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) normal=%s wallType=%s cs=%u ishape=%u wref=%u !",
                            node_->GetName().CString(), node_->GetID(), normal.ToString().CString(), wallTypeNames[walltype], cs->GetID(), ishape, wallContactRef);
#endif

            GOC_Move2D* gocmove = node_->GetComponent<GOC_Move2D>();
            if (gocmove)
                gocmove->OnWallContactBegin((int)walltype, (int)(normal.x_ < 0.f ? 1 : -1));

            // For WallBreaker Ability
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[CollideWallBegin::WALLCONTACTTYPE] = (int)walltype;
            node_->SendEvent(COLLIDEWALLBEGIN, eventData);
        }
    */

    if (newContact)
        SubscribeToEvent(cs, MAPTILEREMOVED, URHO3D_HANDLER(GOC_Collide2D, HandleBreakContact));

    if (newContact)
    {
        if (walltype == Wall_Ground)
        {
            if (!numGroundContacts_)
            {
                /// Remove bounce for net entities
//                body->SetAwake(false);
                // for spawning dust effect
                node_->SendEvent(GO_COLLIDEGROUND);
            }

            numGroundContacts_++;
        }

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
        URHO3D_LOGINFOF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) new contact normal=%s wallType=%s cs=%u ishape=%u wref=%u numgrd=%d!",
                        node_->GetName().CString(), node_->GetID(), normal.ToString().CString(), wallTypeNames[walltype], cs->GetID(), ishape, wallContactRef, numGroundContacts_);
#endif
        /*
                GOC_Move2D* gocmove = node_->GetComponent<GOC_Move2D>();
                if (gocmove)
                    gocmove->OnWallContactBegin((int)walltype, (int)(normal.x_ < 0.f ? 1 : -1));
        */
        // For WallBreaker Ability
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[CollideWallBegin::WALLCONTACTTYPE] = (int)walltype;
        node_->SendEvent(COLLIDEWALLBEGIN, eventData);
    }

    GOC_Move2D* gocmove = node_->GetComponent<GOC_Move2D>();
    if (gocmove)
        gocmove->OnWallContactBegin((int)walltype, (int)(normal.x_ < 0.f ? 1 : -1));
}

void GOC_Collide2D::RemoveWallContact2D(CollisionShape2D* cs, unsigned ishape)
{
    assert(body);

    if (wallContacts.Size() == 0)
        return;

    // Search shape in hashmap
    unsigned wallContactRef = Hash(cs, ishape);

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
    URHO3D_LOGINFOF("GOC_Collide2D() - RemoveWallContact2D : Node=%s(%u) cs=%u ishape=%u wref=%u ...",
                    node_->GetName().CString(), node_->GetID(), cs->GetID(), ishape, wallContactRef);
#endif

    int walltype = 0;
    {
        HashMap<unsigned, WallContact>::Iterator it = wallContacts.Find(wallContactRef);
        if (it == wallContacts.End())
            return;

        WallContact& wallcontact = it->second_;

        walltype = (int)(wallcontact.type_);

        if (wallcontact.count_ > 1)
        {
            wallcontact.count_--;
        }
        else
        {
//            if (walltype == (int)Wall_Ground && numGroundContacts_ > 0)
//                numGroundContacts_--;

            wallContacts.Erase(it);
            UnsubscribeFromEvent(cs, MAPTILEREMOVED);
        }
    }

    if (walltype == (int)Wall_Ground)
        numGroundContacts_--;

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
    URHO3D_LOGINFOF("GOC_Collide2D() - RemoveWallContact2D : Node=%s(%u) wallType=%s(%d) cs=%u ishape=%u wref=%u numgrd=%d numcontacts=%u !",
                    node_->GetName().CString(), node_->GetID(), wallTypeNames[walltype], walltype, cs->GetID(), ishape, wallContactRef, numGroundContacts_, wallContacts.Size());
#endif

    GOC_Move2D* gocmove = node_->GetComponent<GOC_Move2D>();
    if (gocmove)
        gocmove->OnWallContactEnd(walltype, numGroundContacts_, wallContacts.Size());
}
#else
void GOC_Collide2D::AddWallContact2D(b2Contact* contact, const Vector2& normal)
{
    assert(body);

    if (normal == Vector2::ZERO)
        return;

    unsigned wallContactRef = Hash(contact);
    WallType walltype = (Abs(normal.y_) > 0.1f ? (normal.y_ > 0.f ? Wall_Ground : Wall_Roof) : Wall_Border);
    bool newContact = true;

    if (wallContacts.Size() > 0)
    {
        // Search CollisionShape2D in hashmap
        HashMap<unsigned, WallContact>::Iterator it = wallContacts.Find(wallContactRef);
        if (it != wallContacts.End())
        {
            it->second_.count_++;
            newContact = false;
        }
        else
            wallContacts += Pair<unsigned, WallContact>(wallContactRef, WallContact(walltype, 1));
    }
    else
        wallContacts += Pair<unsigned, WallContact>(wallContactRef, WallContact(walltype, 1));

    if (walltype == Wall_Ground)
    {
        numGroundContacts_++;
    }

    if (newContact)
    {
//        URHO3D_LOGINFOF("GOC_Collide2D() - AddWallContact2D : Node=%s(%u) normal=%s wallType=%s wref=%u wallContacts=%u !",
//                        node_->GetName().CString(), node_->GetID(), normal.ToString().CString(), wallTypeNames[walltype], wallContactRef, wallContacts.Size());

        if (walltype == Wall_Ground)
        {
            node_->SendEvent(GO_COLLIDEGROUND);
        }

        VariantMap& eventData = context_->GetEventDataMap();
        eventData[CollideWallBegin::WALLCONTACTTYPE] = (int)walltype;
        node_->SendEvent(COLLIDEWALLBEGIN, eventData);
    }
}

void GOC_Collide2D::RemoveWallContact2D(b2Contact* contact)
{
    assert(body);

//    URHO3D_LOGINFOF("GOC_Collide2D() - RemoveWallContact2D : Node=%s(%u) ... wallContacts=%u",
//                    node_->GetName().CString(), node_->GetID(), wallContacts.Size());

    if (wallContacts.Size() == 0)
        return;

    unsigned wallContactRef = Hash(contact);
    HashMap<unsigned, WallContact>::Iterator it = wallContacts.Find(wallContactRef);
    if (it == wallContacts.End())
        return;

    WallContact& wallcontact = it->second_;
    int walltype = (int)(wallcontact.type_);

    if (wallcontact.count_ > 1)
        wallcontact.count_--;
    else
        wallContacts.Erase(it);

//    URHO3D_LOGINFOF("GOC_Collide2D() - RemoveWallContact2D : Node=%s(%u) wallType=%s wref=%u !",
//                    node_->GetName().CString(), node_->GetID(), wallTypeNames[walltype], wallContactRef);

    VariantMap& eventData = context_->GetEventDataMap();
    eventData[CollideWallEnd::ENDCONTACTWALLTYPE] = walltype;
    if (walltype == Wall_Ground)
    {
        numGroundContacts_--;
        eventData[CollideWallEnd::WALLCONTACTNUM] = numGroundContacts_;
    }
    else
    {
        eventData[CollideWallEnd::WALLCONTACTNUM] = wallContacts.Size();
    }

    node_->SendEvent(COLLIDEWALLEND, eventData);
}
#endif
#endif

//bool GOC_Collide2D::HasGroundContacts() const
//{
//    /// TODO
//    return false;
//}

bool GOC_Collide2D::HasAttacked(float delay) const
{
    return lastVictim_ && (lastAttackTime_ + delay > GameContext::Get().time_->GetElapsedTime());
}

void GOC_Collide2D::ResetLastAttack()
{
    lastVictim_.Reset();
}

void GOC_Collide2D::DumpContacts()
{
#if (WALLCONTACTMODE == 0)
    HashMap<CollisionShape2D*, unsigned>& contacts = groundContacts;
    if (groundContacts.Size() == 0)
        return;

    URHO3D_LOGINFOF("GOC_Collide2D() - DumpContacts node=%s(%u) : nb=%u",
                    body->GetNode()->GetName().CString(), body->GetNode()->GetID(), groundContacts.Size());

    for (HashMap<CollisionShape2D*, unsigned>::ConstIterator it = groundContacts.Begin(); it!=groundContacts.End(); ++it)
    {
        URHO3D_LOGINFOF("-> ShapePtr=%u numGroundContacts=%u", it->first_, it->second_);
    }
#else
    URHO3D_LOGINFOF("GOC_Collide2D() - DumpContacts node=%s(%u) ...",
                    body->GetNode()->GetName().CString(), body->GetNode()->GetID());

    for (HashMap<unsigned, WallContact>::ConstIterator it=wallContacts.Begin(); it != wallContacts.End(); ++it)
    {
        URHO3D_LOGINFOF("-> Ref=%u type=%s(%d) numContacts=%u", it->first_, wallTypeNames[it->second_.type_], it->second_.type_, it->second_.count_);
    }
#endif
}

void GOC_Collide2D::HandleBeginContact(StringHash eventType, VariantMap& eventData)
{
    assert(body);

    using namespace PhysicsBeginContact2D;

    const Urho3D::ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[P_CONTACTINFO].GetUInt());

    RigidBody2D* other = 0;
    CollisionShape2D* csBody = 0;
    CollisionShape2D* csOther = 0;
    unsigned ishape;
    bool swapped = false;

    if (cinfo.bodyA_ == body)
    {
        other = cinfo.bodyB_;
        csBody = cinfo.shapeA_;
        csOther = cinfo.shapeB_;
        ishape = cinfo.iShapeB_;
    }
    else if (cinfo.bodyB_ == body)
    {
        swapped = true;
        other = cinfo.bodyA_;
        csBody = cinfo.shapeB_;
        csOther = cinfo.shapeA_;
        ishape = cinfo.iShapeA_;
    }

    if (!csBody || !csOther)
        return;

//    URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : actor=%s(%u) Begin Contact with node %s(%u) ... shape=%u cm=%u",
//                    body->GetNode()->GetName().CString(), body->GetNode()->GetID(),
//                    other->GetNode()->GetName().CString(), other->GetNode()->GetID(), csBody->IsTrigger(), csBody->GetMaskBits());

    /// Body has a Trigger shape in contact with other body
    if (csBody->IsTrigger())
    {
        /// Dialog Detected (other is the player)
        if (csBody->GetMaskBits() == CM_DETECTPLAYER)
        {
            Actor* actor = Actor::GetWithNode(body->GetNode());
            Actor* player = Actor::GetWithNode(other->GetNode());

//            player->AddDetectInteractor(actor);

            /// player has no interactor in progress
            if (actor && player->GetDialogueInteractor() == 0)
            {
                URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : actor=%s(%u) Begin Contact with node %s(%u) ... send DIALOG_DETECTED",
                                body->GetNode()->GetName().CString(), body->GetNode()->GetID(),
                                other->GetNode()->GetName().CString(), other->GetNode()->GetID());
                actor->SendEvent(DIALOG_DETECTED);
            }
        }

        return;
    }

    /// Body has a Detector shape in contact
    /// with this, the shape can be a Box2d Sensor or not
    if (csBody->GetMaskBits() == CM_INSIDEAVATAR_DETECTOR || csBody->GetMaskBits() == CM_OUTSIDEAVATAR_DETECTOR)
    {
        Player* player = static_cast<Player*>(Actor::GetWithNode(body->GetNode()));
        if (player)
        {
            /// Other is a Map Collider or a Platform (but not Water)
            if (csOther->GetColliderInfo() && csOther->GetColliderInfo() != WATERCOLLIDER)
            {
                /// Hit a wall
//                URHO3D_LOGERRORF("GOC_Collide2D() - HandleBeginContact : node=%s(%u) hits a wall on %s(%u) at %s ... csBodyNode=%s(%u) filters(%u,%u) ",
//                                    body->GetNode()->GetName().CString(), body->GetNode()->GetID(),
//                                    other->GetNode()->GetName().CString(), other->GetNode()->GetID(),
//                                    cinfo.contactPoint_.ToString().CString(),
//                                    csBody->GetNode()->GetName().CString(), csBody->GetNode()->GetID(), csBody->GetCategoryBits(), csBody->GetMaskBits());
                ABI_WallBreaker::OnSameHolderLayerOnly_ = true;
                player->UseWeaponAbilityAt(ABI_WallBreaker::GetTypeStatic(), cinfo.contactPoint_);
                ABI_WallBreaker::OnSameHolderLayerOnly_ = false;
            }

            /// Add Contact Informations for MissionManager or GOC_Collectable
//            URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : actor=%s(%u) Begin Contact with node %s(%u) ... get Attack Information",
//                            body->GetNode()->GetName().CString(), body->GetNode()->GetID(),
//                            other->GetNode()->GetName().CString(), other->GetNode()->GetID());

            lastAttackTime_ = GameContext::Get().time_->GetElapsedTime();
            lastVictim_     = other->GetNode();
        }

        return;
    }

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
    URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) Begin Contact with node %s(%u) ... csBodyNode=%s(%u) filters(%u,%u)",
                    body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID(),
                    csBody->GetNode()->GetName().CString(), csBody->GetNode()->GetID(), csBody->GetCategoryBits(), csBody->GetMaskBits());
#endif

    /// Other is not a sensor and is not attacking
    if (!csOther->IsTrigger() && !csOther->GetNode()->GetName().StartsWith(TRIGATTACK) && !csBody->GetNode()->GetName().StartsWith(TRIGATTACK))
    {
        /// Body is a Collectable/Part Collider
        if (GOT::GetTypeProperties(node_->GetVar(GOA::GOT).GetStringHash()) & (GOT_Collectable|GOT_Part))
        {
            /// None here : implemented in GOC_Collectable::HandleContact
        }
        /// Other is a Map Collider or A Platform (but not Water)
        else if (csOther->GetColliderInfo())
        {
            if (csOther->GetColliderInfo() == WATERCOLLIDER)
                return;

            Vector2 normal = cinfo.normal_;
            if (csBody->GetColliderInfo() && swapped)
                normal.y_ = -normal.y_;
#if (WALLCONTACTMODE == 0)
            if (normal.y_ > 0.f)
                AddGroundContact2D(csOther);
#elif (WALLCONTACTMODE == 1)
            AddWallContact2D(csOther, ishape, normal);
#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
            URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) Begin Contact with Map=%s(%u) with cshape=%u ishape=%u normal=%s numgrd=%d numcontacts=%u ... OK !",
                            body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID(),
                            csOther->GetID(), ishape, normal.ToString().CString(), numGroundContacts_, wallContacts.Size());
#endif
#else
            b2Contact* contact = (b2Contact*)eventData[PhysicsBeginContact2D::P_CONTACTPTR].GetVoidPtr();
            AddWallContact2D(contact, normal);
#endif
        }
        /// Other is an Entity Collider
        else if (csOther->GetExtraContactBits() & CONTACT_ISSTABLE)
        {
            Vector2 delta  = body->GetNode()->GetWorldPosition2D() - other->GetNode()->GetWorldPosition2D();
            Vector2 normal = cinfo.normal_;
            // Only add ground contact
            if (delta.y_ > 0.f && normal.y_ > 0.f && normal.y_ > Abs(normal.x_))
            {
#if (WALLCONTACTMODE == 0)
                AddGroundContact2D(csOther);
#elif (WALLCONTACTMODE == 1)
                AddWallContact2D(csOther, ishape, normal);
#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
                URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) Begin Contact with %s(%u) with cshape=%u ishape=%u normal=%s numgrd=%d numcontacts=%u ... OK !",
                                body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID(),
                                csOther->GetID(), ishape, normal.ToString().CString(), numGroundContacts_, wallContacts.Size());
#endif
#else
                b2Contact* contact = (b2Contact*)eventData[P_CONTACTPTR].GetVoidPtr();
                AddWallContact2D(contact, cinfo.normal_);
#endif
            }
        }

        return;
    }

    // Not a Trigger Attack
    if (!csOther->GetNode()->GetName().StartsWith(TRIGATTACK))
        return;

//    URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) Receive Attack from %s(%u) step1 ...",
//                     body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());

    if (GameContext::Get().time_->GetElapsedTime() - lastHurtTime_ < 0.2f)
        return;

    /// Other is attacking, Body receives an attack

    // No Receive Attack in ClientMode / let Server computing this event
//    if (GameContext::Get().ClientMode_)
//        return;

    // Skip Attack if not on the same ViewZ
    if (other->GetNode()->GetVar(GOA::ONVIEWZ) != Variant::EMPTY)
    {
        if (other->GetNode()->GetVar(GOA::ONVIEWZ).GetInt() != body->GetNode()->GetVar(GOA::ONVIEWZ).GetInt())
        {
//            URHO3D_LOGERRORF("GOC_Collide2D() - HandleBeginContact : Trig_Attack %s(%u) viewz=%d check viewz with %s(%u) viewz=%d !",
//                             body->GetNode()->GetName().CString(), body->GetNode()->GetID(), body->GetNode()->GetVar(GOA::ONVIEWZ).GetInt(),
//                             other->GetNode()->GetName().CString(), other->GetNode()->GetID(), other->GetNode()->GetVar(GOA::ONVIEWZ).GetInt());
            return;
        }
    }

//    /// No Attack with attacker at upper ViewZ
//    int zOther = csOther->GetViewZ();
//    int zBody = csBody->GetViewZ();
//    if (zOther != zBody)
//        if (zOther == INNERVIEW || zOther > zBody)
//            return;

//    URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) Receive Attack from %s(%u) step2 ...",
//                     body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());

    // Skip during Invulnerability
    GOC_Life* life = node_->GetComponent<GOC_Life>();
    if (!life)
    {
//        URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) Receive Attack from %s(%u)!!!",
//                     body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());
        node_->SendEvent(GO_RECEIVEEFFECT);
        return;
    }
    else if (life->GetInvulnerability() > 0.f)
    {
        return;
    }

#ifdef ACTIVE_LINEOFSIGHT
    if (other->GetNode()->GetVar(GOA::TYPECONTROLLER).GetInt() != GO_None)
    {
        GOC_Move2D* gocmove = other->GetNode()->GetComponent<GOC_Move2D>();
        if (gocmove)
        {
            unsigned mask = gocmove->GetActiveMaskLOS();
            if (mask && GetComponent<GOC_Destroyer>())
                if (!World2D::GetLineOfSight(GetComponent<GOC_Destroyer>()->GetWorldMapPosition()) & mask)
                {
                    URHO3D_LOGWARNINGF("GOC_Collide2D() - HandleBeginContact %s(%u) : %s(%u) Attack but NO Line of Sight !!!",
                                       body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID());
                    return;
                }
        }
    }
#endif

    Node* sender = csOther->GetNode()->GetVar(GOA::EFFECTID1) != Variant::EMPTY ? csOther->GetNode() : other->GetNode();

//    URHO3D_LOGINFOF("GOC_Collide2D() - HandleBeginContact : %s(%u) BEGIN with %s(%u) triggerName=%s !!!",
//                     body->GetNode()->GetName().CString(), body->GetNode()->GetID(), sender->GetName().CString(), sender->GetID(), csOther->GetNode()->GetName().CString());

    lastHurtTime_  = GameContext::Get().time_->GetElapsedTime();
    lastAggressor_ = other->GetNode();

    EffectsManager::SetEffectsOn(sender, node_, csOther->GetNode()->GetName(), csOther->GetMassCenter() + csOther->GetNode()->GetWorldPosition2D());
}

void GOC_Collide2D::HandleEndContact(StringHash eventType, VariantMap& eventData)
{
    assert(body);

    using namespace PhysicsEndContact2D;

    const ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetEndContactInfo(eventData[P_CONTACTINFO].GetUInt());

    RigidBody2D* other;
    CollisionShape2D *csBody, *csOther;
    unsigned ishape;

    if (cinfo.bodyA_ == body)
    {
        other = cinfo.bodyB_;
        csBody = cinfo.shapeA_;
        csOther = cinfo.shapeB_;
        ishape = cinfo.iShapeB_;
    }
    else if (cinfo.bodyB_ == body)
    {
        other = cinfo.bodyA_;
        csBody = cinfo.shapeB_;
        csOther = cinfo.shapeA_;
        ishape = cinfo.iShapeA_;
    }

    /// Body Trigger
    if (csBody->IsTrigger())
    {
        /// Close Dialog (Marker and Frame)
        if (csBody->GetMaskBits() == CM_DETECTPLAYER)
        {
            Actor* player = Actor::GetWithNode(other->GetNode());
            Actor* actor = Actor::GetWithNode(body->GetNode());
            if (actor) eventData[Dialog_Close::ACTOR_ID] = actor->GetID();
            if (player) eventData[Dialog_Close::PLAYER_ID] = player->GetID();
            if (actor) actor->SendEvent(DIALOG_CLOSE, eventData);
            if (player)
            {
                player->SendEvent(DIALOG_CLOSE, eventData);
//                player->RemoveDetectedInteractor(actor);
            }
        }
        return;
    }

    if (csBody->GetMaskBits() == CM_INSIDEAVATAR_DETECTOR || csBody->GetMaskBits() == CM_OUTSIDEAVATAR_DETECTOR)
    {
        return;
    }

    if (!csOther->IsTrigger())
    {
        if (csOther->GetColliderInfo() == WATERCOLLIDER)
            return;

#if (WALLCONTACTMODE == 0)
        RemoveGroundContact2D(csOther);
//        URHO3D_LOGINFOF("GOC_Collide2D() - HandleEndContact : %s(%u) End Contact with csOther=%u",
//                        body->GetNode()->GetName().CString(), body->GetNode()->GetID(), csOther);
#elif (WALLCONTACTMODE == 1)
        RemoveWallContact2D(csOther, ishape);
#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
        URHO3D_LOGINFOF("GOC_Collide2D() - HandleEndContact : %s(%u) End Contact with %s(%u) cshape=%u ishape=%u numgrd=%d numcontacts=%u ... OK !",
                        body->GetNode()->GetName().CString(), body->GetNode()->GetID(), other->GetNode()->GetName().CString(), other->GetNode()->GetID(),
                        csOther->GetID(), ishape, numGroundContacts_, wallContacts.Size());
#endif
#else
        b2Contact* contact = (b2Contact*)eventData[P_CONTACTPTR].GetVoidPtr();
        RemoveWallContact2D(contact);
//        URHO3D_LOGINFOF("GOC_Collide2D() - HandleEndContact : %s(%u) End Contact with contact=%u",
//                        body->GetNode()->GetName().CString(), body->GetNode()->GetID(), contact);
#endif
    }
}

void GOC_Collide2D::HandleBreakContact(StringHash eventType, VariantMap& eventData)
{
#if (WALLCONTACTMODE == 1)
    CollisionShape2D* cs = (CollisionShape2D*)GetEventSender();

    if (!cs->IsTrigger())
    {
        /// TODO : check the tile where the node is hanged on, if same remove the contacts
        WallType walltype;

        HashMap<unsigned, WallContact>::Iterator it = wallContacts.Begin();
        while (it != wallContacts.End())
        {
            if (CompareHash(cs, it->first_))
            {
                walltype = it->second_.type_;
                it = wallContacts.Erase(it);
                continue;
            }
            ++it;
        }

        UnsubscribeFromEvent(cs, MAPTILEREMOVED);

        if (walltype == Wall_Ground)
            //if (walltype == Wall_Ground && numGroundContacts_ > 0)
            numGroundContacts_--;

    #ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
        URHO3D_LOGINFOF("GOC_Collide2D() - HandleBreakContact : %s(%u) Remove WallContact with cs=%u numgrd=%d ",
                        GetNode()->GetName().CString(), GetNode()->GetID(), cs, numGroundContacts_);
    #endif

        GOC_Move2D* gocmove = node_->GetComponent<GOC_Move2D>();
        if (gocmove)
            gocmove->OnWallContactEnd(walltype, numGroundContacts_, wallContacts.Size());
    }
#endif
}

void GOC_Collide2D::HandleDead(StringHash eventType, VariantMap& eventData)
{
    if (node_->GetVar(GOA::DESTROYING).GetBool())
        return;

//    URHO3D_LOGINFOF("GOC_Collide2D() - HandleDead : Node=%s(%u) ... disable topcontact and updatefilterbits for dead", node_->GetName().CString(), node_->GetID());

    SetCollidersEnable(false, CONTACT_BOTTOM, true);
    ClearContacts();

//    GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
//    if (destroyer)
//        destroyer->UpdateFilterBits();
}

// Purge contact before jump and fall (purge ground contact from collectable)
void GOC_Collide2D::OnBreakGroundContacts(StringHash eventType, VariantMap& eventData)
{
#if (WALLCONTACTMODE == 0)
    groundContacts.Clear();
#else
    HashMap<unsigned, WallContact>::Iterator it = wallContacts.Begin();
    while (it != wallContacts.End())
    {
        if (it->second_.type_ == Wall_Ground)
        {
            it = wallContacts.Erase(it);
            continue;
        }
        ++it;
    }
#endif
    numGroundContacts_ = 0;

#ifdef DUMP_DEBUG_MAPCOLLIDEUPDATE
    URHO3D_LOGINFOF("GOC_Collide2D() - OnBreakGround eventType=%s(%u) : %s(%u) Clear GroundContact",
                    GOE::GetEventName(eventType).CString(), eventType.Value(), body->GetNode()->GetName().CString(), body->GetNode()->GetID());
#endif

//    DumpContacts();
}

