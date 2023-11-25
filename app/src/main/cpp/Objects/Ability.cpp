#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include "DefsViews.h"
#include "DefsMove.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameNetwork.h"

#include "Map.h"
#include "MapWorld.h"
#include "ViewManager.h"

#include "Actor.h"

#include "GOC_Animator2D.h"
#include "GOC_Controller.h"
#include "GOC_Move2D.h"
#include "GOC_Destroyer.h"
#include "GOC_PhysicRope.h"

#include "GO_Pool.h"

#include "Ability.h"

const float& GRAPIN_VEL = 15.f;


HashMap<StringHash, SharedPtr<Ability> > Ability::staticAbilities_;

Ability::Ability(Context* context) :
    Object(context),
    pool_(0), isInUse_(false),
    usePool_(false), useNetwork_(false),
    icon_(86, 245, 166, 325)
{ }

Ability::~Ability()
{
//    LOGINFO("~Ability()");
//    node_.Reset();
}

//void Ability::RegisterObject(Context* context)
//{
//
//}

void Ability::RegisterLibraryAbility(Context* context)
{
//    Ability::RegisterObject(context);
    ABI_Grapin::RegisterObject(context);
    ABI_CrossWalls::RegisterObject(context);
    ABI_WallBreaker::RegisterObject(context);
    ABI_WallBuilder::RegisterObject(context);
    ABI_Shooter::RegisterObject(context);
    ABIBomb::RegisterObject(context);
    ABI_AnimShooter::RegisterObject(context);
    ABI_Fly::RegisterObject(context);

    staticAbilities_[ABI_Grapin::GetTypeStatic()]      = SharedPtr<ABI_Grapin>(new ABI_Grapin(context));
    staticAbilities_[ABI_CrossWalls::GetTypeStatic()]  = SharedPtr<ABI_CrossWalls>(new ABI_CrossWalls(context));
    staticAbilities_[ABI_WallBreaker::GetTypeStatic()] = SharedPtr<ABI_WallBreaker>(new ABI_WallBreaker(context));
    staticAbilities_[ABI_WallBuilder::GetTypeStatic()] = SharedPtr<ABI_WallBuilder>(new ABI_WallBuilder(context));
    staticAbilities_[ABI_Shooter::GetTypeStatic()]     = SharedPtr<ABI_Shooter>(new ABI_Shooter(context));
    staticAbilities_[ABIBomb::GetTypeStatic()]         = SharedPtr<ABIBomb>(new ABIBomb(context));
    staticAbilities_[ABI_AnimShooter::GetTypeStatic()] = SharedPtr<ABI_AnimShooter>(new ABI_AnimShooter(context));
    staticAbilities_[ABI_Fly::GetTypeStatic()]         = SharedPtr<ABI_Fly>(new ABI_Fly(context));
}

void Ability::Clear()
{
    staticAbilities_.Clear();

    URHO3D_LOGINFOF("Ability() - Clear : Static Abilities Cleared !");
}

Ability* Ability::Get(const StringHash& abi)
{
    HashMap<StringHash, SharedPtr<Ability> >::Iterator it = staticAbilities_.Find(abi);
    return it != staticAbilities_.End() ? it->second_.Get() : 0;
}

void Ability::SetObjetType(const StringHash& type)
{
    if (GOT_ != type && type != StringHash::ZERO)
    {
        GOT_ = type;
        Update();
    }
}

// Static Use : local and net usage
Node* Ability::Use(const StringHash& got, Node* holder, const PhysicEntityInfo& physics, bool replicatemode, int viewZ, ObjectControlInfo** oinfo)
{
    int holderclientid = holder ? holder->GetVar(GOA::CLIENTID).GetInt() : 0;
    Node* node = GO_Pools::Get(got, holderclientid);
    if (!node)
    {
        URHO3D_LOGERRORF("Ability() - Use (static) : Node=0 !");
        return 0;
    }

    bool physicOk = GameHelpers::SetPhysicProperties(node, physics);

    if (physics.direction_ < 0.f)
    {
        StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
        if (sprite)
            sprite->SetFlip(true, false);
    }

//    int faction = holder ? (holder->GetVar(GOA::NOCHILDFACTION).GetBool() ? 0 : holder->GetVar(GOA::FACTION).GetUInt()) : 0;
//    node->SetVar(GOA::FACTION, (holderclientid << 8) + GO_Player);
    node->SetVar(GOA::FACTION, holder ? holder->GetVar(GOA::FACTION).GetUInt() : (holderclientid << 8) + GO_Player);

    GOC_PhysicRope* grapin = node->GetComponent<GOC_PhysicRope>();
    if (grapin)
    {
        grapin->SetAttachedNode(holder);
        grapin->SetMaxLength(5.f);
        node->GetComponent<RigidBody2D>()->SetLinearVelocity(node->GetDirection().ToVector2() * GRAPIN_VEL);
    }

    if (holder && got == ABIBomb::GetTypeStatic())
    {
        GOC_Animator2D* holderAnimator = holder->GetComponent<GOC_Animator2D>();
        if (holderAnimator)
        {
            holderAnimator->SetSpawnEntityMode(SPAWNENTITY_SKIPONCE);
            holderAnimator->SetNetState(StringHash(STATE_ATTACK), 0, true);
        }
    }

    node->SetVar(GOA::ONVIEWZ, holder ? holder->GetVar(GOA::ONVIEWZ).GetInt() : viewZ);

    node->SendEvent(WORLD_ENTITYCREATE);

    node->SetEnabled(true);

    const Vector2& pos = node->GetWorldPosition2D();

    URHO3D_LOGINFOF("Ability() - Use (static) holder=%s(%u-clientid=%d) node=%s(%u) faction=%u pos=%F,%F dir=%F rot=%F mode=%s",
                    holder ? holder->GetName().CString() : "null", holder ? holder->GetID() : 0, holderclientid, node->GetName().CString(), node->GetID(),
                    node->GetVar(GOA::FACTION).GetUInt(), pos.x_, pos.y_, physics.direction_, physics.rotation_, replicatemode ? "replicate":"local");

    if (!GameContext::Get().LocalMode_)
    {
        if (replicatemode)
        {
            ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node, holder, false, true, true);
            if (cinfo && oinfo)
                *oinfo = cinfo;
        }
    }

    return node;
}

// Static Use
bool Ability::UseAtPoint(const Vector2& position, Node* holder, const StringHash& GOT2Spawn)
{
    if (GOT2Spawn)
    {
        return false;

        // TO IMPLEMENT
        int holderclientid = holder ? holder->GetVar(GOA::CLIENTID).GetInt() : 0;
        Node* node = GO_Pools::Get(GOT2Spawn, holderclientid);
        if (!node)
        {
            URHO3D_LOGERRORF("Ability() - UseAtPoint (static) : Node=0 !");

        }
    }

//	URHO3D_LOGINFOF("Ability() - UseAtPoint (static) ability=%s holder=%s(%u) pos=%F,%F ... ", GetTypeName().CString(),
//                    holder ? holder->GetName().CString() : "null", holder ? holder->GetID() : 0, position.x_, position.y_);

    holder_ = holder;
    isInUse_ = false;
    ObjectControlInfo* oinfo = 0;
    Node* node = Use(position, &oinfo);

//    URHO3D_LOGINFOF("Ability() - UseAtPoint (static) ability=%s holder=%s(%u) pos=%F,%F ... OK !", GetTypeName().CString(),
//                    holder ? holder->GetName().CString() : "null", holder ? holder->GetID() : 0, position.x_, position.y_);

    return node != 0;
}

// Local Use for Player::HandleClic
void Ability::UseAtPoint(const Vector2& wpoint)
{
    ObjectControlInfo* oinfo = 0;

    Node* node = Use(wpoint, &oinfo);

//    URHO3D_LOGINFOF("Ability() - UseAtPoint point=%F,%F ... %s", wpoint.x_, wpoint.y_, node ? "OK !":"NOK !");
}

void Ability::Update()
{
    if (!UsePool())
        return;

    int clientid = 0;

    if (!GameContext::Get().LocalMode_)
    {
        if (!holder_)
        {
            clientid = GameNetwork::Get()->GetClientID();
        }
        else
        {
            const Variant& cvar = holder_->GetVar(GOA::CLIENTID);
            clientid = cvar != Variant::EMPTY ? cvar.GetInt() : GameNetwork::Get()->GetClientID();
        }
    }

    URHO3D_LOGWARNINGF("%s() - Update : get pool with clientid=%d holder=%s(%u) !", GetTypeName().CString(), clientid, holder_ ? holder_->GetName().CString() : "None", holder_ ? holder_->GetID() : 0);

    pool_ = GO_Pools::GetOrCreatePool(GOT_, "ABI", clientid);
}



/// ABI_GRAPIN


void ABI_Grapin::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_Grapin>();
}

ABI_Grapin::ABI_Grapin(Context* context) :
    Ability(context)
{
    usePool_ = useNetwork_ = true;
    icon_ = IntRect(82, 413, 162, 493);
}

ABI_Grapin::~ABI_Grapin()
{
    Release(0.f);
}

void ABI_Grapin::Release(float time)
{
    if (!node_)
        return;

    URHO3D_LOGINFOF("ABI_Grapin() - Release : node=%u !", node_.Get());

    if (!GameContext::Get().LocalMode_)
    {
        ObjectControlInfo* cinfo = GameNetwork::Get()->GetObjectControl(node_->GetID());
        if (cinfo)
        {
            ObjectControl& control = cinfo->GetPreparedControl();
            control.holderinfo_.id_ = 0;
        }
    }

    GOC_PhysicRope* grapin = node_->GetComponent<GOC_PhysicRope>();
    if (grapin)
    {
        grapin->Detach();
        grapin->Release(time);
    }

    UnsubscribeFromEvent(node_, ABI_RELEASE);
    node_.Reset();

    isInUse_ = false;
}

void ABI_Grapin::HandleRelease(StringHash eventType, VariantMap& eventData)
{
    Release(2.f);
}

Node* ABI_Grapin::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
    if (!GOT_)
        return 0;

    if (isInUse_)
    {
        URHO3D_LOGINFOF("ABI_Grapin() - Use : %u in Use ... restore it !", node_->GetID());
        Release(2.f);
        return 0;
    }

    // be sure to have released the last one
    if (node_)
    {
        Release(2.f);
        return 0;
    }

    Node* node = 0;
    Vector2 direction = wpoint - holder_->GetWorldPosition2D();
    direction.Normalize();

    // Create a Dynamic Grapin from holder to wpoint
    if (direction.y_ < 0.f)
        return 0;

    if (!holder_->GetComponent<GOC_Destroyer>())
    {
        URHO3D_LOGERROR("ABI_Grapin() - Use : holder_ don't have a gocDestroyer !");
        return 0;
    }

    const WorldMapPosition& holderPosition = holder_->GetComponent<GOC_Destroyer>()->GetWorldMapPosition();

//    if (World2D::GetTileProperty(holderPosition) == TilePropertiesFlag::Blocked)
//        return 0;

    WorldMapPosition wposition;
    World2D::GetWorldInfo()->Convert2WorldMapPosition(holderPosition.position_ + direction, wposition);
    wposition.viewZ_ = holderPosition.viewZ_;
    if (World2D::GetTileProperty(wposition) == TilePropertiesFlag::Blocked)
        return 0;

    if (pool_)
    {
        node = pool_->GetGO();
    }
    else
    {
        node = holder_->GetScene()->CreateChild(String::EMPTY, LOCAL);
        GameHelpers::SpawnGOtoNode(context_, GOT_, node);
    }

    if (!node)
    {
        node_.Reset();
        isInUse_ = false;
        return 0;
    }

    isInUse_ = true;
    node_ = node;

//    URHO3D_LOGINFOF("ABI_Grapin() - Use holder=%s direction=%s node=%s(%u) ...",
//                    holderPosition.position_.ToString().CString(), direction.ToString().CString(), node_->GetName().CString(), node_->GetID());

    node_->SetWorldPosition2D(wposition.position_);
    node_->SetWorldRotation2D(Atan(-direction.x_/direction.y_));
    node_->SetVar(GOA::FACTION, holder_->GetVar(GOA::FACTION).GetUInt());
    node_->SetVar(GOA::OWNERID, holder_->GetID());
    node_->SetVar(GOA::ONVIEWZ, wposition.viewZ_);

    GOC_PhysicRope* grapin = node_->GetComponent<GOC_PhysicRope>();
    grapin->SetMaxLength(5.f);
    grapin->SetAttachedNode(holder_);
    node_->GetComponent<RigidBody2D>()->SetLinearVelocity(direction * GRAPIN_VEL);

    node_->SendEvent(WORLD_ENTITYCREATE);

    node_->SetEnabled(true);

    SubscribeToEvent(node_, ABI_RELEASE, URHO3D_HANDLER(ABI_Grapin, HandleRelease));

    if (!GameContext::Get().LocalMode_ && UseNetworkReplication())
    {
        ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node_, holder_, false, true, true);
        if (oinfo && cinfo)
            *oinfo = cinfo;
    }

//    URHO3D_LOGINFOF("ABI_Grapin() - Use ... OK !");

    return node_;
}


/// ABI_CROSSWALLS


void ABI_CrossWalls::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_CrossWalls>();
}

ABI_CrossWalls::ABI_CrossWalls(Context* context) :
    Ability(context)
{
    icon_ = IntRect(82, 331, 162, 411);
}

Node* ABI_CrossWalls::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
    // Cross Through the Walls if possible

    GOC_Destroyer* gocDestroyer = holder_->GetComponent<GOC_Destroyer>();
    if (!gocDestroyer)
    {
        URHO3D_LOGERROR("ABI_CrossWalls() - Use : holder_ don't have a gocDestroyer !");
        return 0;
    }

    const WorldMapPosition& holderPosition = gocDestroyer->GetWorldMapPosition();
    if (!holderPosition.viewZ_)
        return 0;

    int newViewZ = holderPosition.viewZ_ == FRONTVIEW ? INNERVIEW : FRONTVIEW;

    if (newViewZ != holderPosition.viewZ_)
    {
        // Check if not blocked
        if (gocDestroyer->IsOnFreeTiles(newViewZ))
        {
            // Send Event to ViewManager
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Go_Detector::GO_GETTER] = holder_->GetID();
            holder_->SendEvent(newViewZ == INNERVIEW ? GO_DETECTORSWITCHVIEWIN : GO_DETECTORSWITCHVIEWOUT, eventData);

            // Spawn Effect
            Drawable2D* drawable = holder_->GetDerivedComponent<Drawable2D>();
            GameHelpers::SpawnParticleEffect(holder_->GetContext(), ParticuleEffect_[PE_SUN], drawable->GetLayer(), drawable->GetViewMask(), holder_->GetWorldPosition2D(), 0.f, 1.f, true, 0.1f, Color::YELLOW, LOCAL);
            return (Node*)1;
        }
        else
        {
            // Spawn Effect Fail
            Drawable2D* drawable = holder_->GetDerivedComponent<Drawable2D>();
            GameHelpers::SpawnParticleEffect(holder_->GetContext(), ParticuleEffect_[PE_BLACKSPIRAL], drawable->GetLayer(), drawable->GetViewMask(), holder_->GetWorldPosition2D(), 0.f, 1.f, true, 1.f, Color::YELLOW, LOCAL);
            return 0;
        }
    }

    return (Node*)1;
}


/// ABI_WALLBREAKER


bool ABI_WallBreaker::OnSameHolderLayerOnly_ = false;

void ABI_WallBreaker::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_WallBreaker>();
}

ABI_WallBreaker::ABI_WallBreaker(Context* context) :
    Ability(context)
{
    useNetwork_ = true;
    icon_ = IntRect(82, 331, 162, 411);
    isInUse_ = true;
}

Node* ABI_WallBreaker::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
    if (!isInUse_)
    {
        URHO3D_LOGINFOF("ABI_WallBreaker() - Use : holder=%s(%u) ...", holder_->GetName().CString(), holder_->GetID());

        GOC_Destroyer* gocDestroyer = holder_->GetComponent<GOC_Destroyer>();
        if (!gocDestroyer)
        {
            URHO3D_LOGERROR("ABI_CrossWalls() - Use : holder_ don't have a gocDestroyer !");
            return 0;
        }

        int viewport = 0;
        if (GameContext::Get().gameConfig_.multiviews_)
        {
            GOC_Controller* controller = holder_->GetDerivedComponent<GOC_Controller>();
            if (controller && controller->GetThinker())
                viewport = controller->GetThinker()->GetControlID();
        }

        Vector2 tilepos;
        gocDestroyer->GetUpdatedWorldPosition2D(tilepos);
//        Vector2 direction = wpoint - tilepos;
        Vector2 direction = wpoint - holder_->GetWorldPosition2D();
        direction.Normalize();
        tilepos += Vector2(World2D::GetWorldInfo()->mTileWidth_, World2D::GetWorldInfo()->mTileHeight_) * direction;

        WorldMapPosition position;
        World2D::GetWorldInfo()->Convert2WorldMapPosition(tilepos, position);
        position.viewZIndex_ = ViewManager::Get()->GetCurrentViewZIndex(viewport);

        int terrainid = GameHelpers::RemoveTile(position, true, false, false, OnSameHolderLayerOnly_);
        if (terrainid != -1)
        {
            isInUse_ = true;

            if (!GameContext::Get().LocalMode_)
            {
                VariantMap& eventData = holder_->GetContext()->GetEventDataMap();
                eventData[Net_ObjectCommand::P_TILEOP] = 0U;
                eventData[Net_ObjectCommand::P_TILEINDEX] = position.tileIndex_;
                eventData[Net_ObjectCommand::P_TILEMAP] = position.mPoint_.ToHash();
                eventData[Net_ObjectCommand::P_TILEVIEW] = position.viewZIndex_;
                //            GameNetwork::Get()->SendObjectCommand(CHANGETILE, eventData);
                GameNetwork::Get()->PushObjectCommand(CHANGETILE, &eventData, true, GameNetwork::Get()->GetClientID());
            }

            // Earthquake
            GameHelpers::ShakeNode(ViewManager::Get()->GetCameraNode(viewport), 7, 1.f, Vector2(0.1f, 0.8f));

            // Spawn Dust Effect
            Drawable2D* drawable = holder_->GetDerivedComponent<Drawable2D>();
            GameHelpers::SpawnParticleEffect(holder_->GetContext(), ParticuleEffect_[PE_DUST], drawable->GetLayer(), drawable->GetViewMask(), position.position_, 90.f, 2.f, true, 2.f, Color::WHITE, LOCAL);

            return (Node*)1;
        }
    }

    return 0;
}


/// ABI_WALLBUILDER


void ABI_WallBuilder::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_WallBuilder>();
}

ABI_WallBuilder::ABI_WallBuilder(Context* context) :
    Ability(context)
{
    useNetwork_ = true;
    icon_ = IntRect(82, 331, 162, 411);
}

Node* ABI_WallBuilder::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
//    if (!GameContext::Get().LocalMode_)
//    {
//        URHO3D_LOGWARNING("ABI_WallBuilder() - Use : only used in localmode for the moment !");
//        return (Node*)1;
//    }

    GOC_Destroyer* gocDestroyer = holder_->GetComponent<GOC_Destroyer>();
    if (!gocDestroyer)
    {
        URHO3D_LOGERROR("ABI_WallBuilder() - Use : holder_ don't have a gocDestroyer !");
        return 0;
    }

    Vector2 tilepos;
    gocDestroyer->GetUpdatedWorldPosition2D(tilepos);
    Vector2 direction = wpoint - tilepos;
    direction.Normalize();
    tilepos += World2D::GetWorldInfo()->mTileWidth_ * direction;

    GOC_Controller* gocController = holder_->GetDerivedComponent<GOC_Controller>();
    if (gocController)
    {
        if (gocController->control_.IsButtonDown(CTRL_UP))
            tilepos += World2D::GetWorldInfo()->mTileHeight_ * Vector2::UP;
        else if (gocController->control_.IsButtonDown(CTRL_DOWN))
            tilepos += World2D::GetWorldInfo()->mTileHeight_ * Vector2::DOWN;
    }

    WorldMapPosition position;
    World2D::GetWorldInfo()->Convert2WorldMapPosition(tilepos, position);
    position.viewZIndex_ = ViewManager::Get()->GetCurrentViewZIndex();

    if (GameHelpers::AddTile(position))
    {
        if (!GameContext::Get().LocalMode_)
        {
            VariantMap& eventData = holder_->GetContext()->GetEventDataMap();
            eventData[Net_ObjectCommand::P_TILEOP] = 1U;
            eventData[Net_ObjectCommand::P_TILEINDEX] = position.tileIndex_;
            eventData[Net_ObjectCommand::P_TILEMAP] = position.mPoint_.ToHash();
            eventData[Net_ObjectCommand::P_TILEVIEW] = position.viewZIndex_;
//            GameNetwork::Get()->SendObjectCommand(CHANGETILE, eventData);
            GameNetwork::Get()->PushObjectCommand(CHANGETILE, &eventData, true, GameNetwork::Get()->GetClientID());
        }

        URHO3D_LOGINFOF("ABI_WallBuilder() - Use : holder=%s(%u) ... holderpos=%s tilepos=%s position=%s !",
                        holder_->GetName().CString(), holder_->GetID(), holder_->GetWorldPosition2D().ToString().CString(),
                        tilepos.ToString().CString(), position.ToString().CString());

        Drawable2D* drawable = holder_->GetDerivedComponent<Drawable2D>();
        GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_LIFEFLAME], drawable->GetLayer(), drawable->GetViewMask(), position.position_, 0.f, 1.f, true, 0.7f, Color::WHITE, LOCAL);

        return (Node*)1;
    }

    return 0;
}



/// ABI_SHOOTER (LAME)


const float& SHOOTER_VEL = 20.f;
const float& SHOOTER_DISTANCESQUARED_MIN = 2.5f;

void ABI_Shooter::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_Shooter>();
}

ABI_Shooter::ABI_Shooter(Context* context) :
    Ability(context)
{
    usePool_ = useNetwork_ = true;
    icon_ = IntRect(0, 331, 80, 411);
}

Node* ABI_Shooter::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
    Node* node = 0;

    if (!isInUse_)
    {
        Vector2 direction = wpoint - holder_->GetWorldPosition2D();
        direction.Normalize();

//        URHO3D_LOGINFOF("ABI_Shooter() - Use : dir=%f,%f ...", direction.x_, direction.y_);

        if (holder_->GetVar(GOA::MOVESTATE).GetUInt() & MV_TOUCHGROUND && direction.y_ < 0.f && direction.x_*direction.x_ < 0.2f)
            return 0;

        GOC_Destroyer* holderdestroyer = holder_->GetComponent<GOC_Destroyer>();
        if (!holderdestroyer)
        {
            URHO3D_LOGERROR("ABI_Shooter() - Use : holder_ don't have a gocDestroyer !");
            return 0;
        }

        if (pool_)
            node = pool_->GetGO();
        else
        {
            node = holder_->GetScene()->CreateChild(String::EMPTY, LOCAL);
            GameHelpers::SpawnGOtoNode(context_, GOT_, node);
        }

        if (!node)
        {
            node_.Reset();
            isInUse_ = false;
            return 0;
        }

        if (node->GetID() < 2)
        {
            URHO3D_LOGERRORF("ABI_Shooter() - Use : node=%u not allowed !", node->GetID());
            node_.Reset();
            return 0;
        }

        isInUse_ = true;
        node_ = node;

//        int nodeclientid = node->GetVar(GOA::CLIENTID).GetInt();
        node_->SetVar(GOA::FACTION, holder_->GetVar(GOA::FACTION).GetUInt());//((unsigned)nodeclientid << 8) + GO_Player);
        node_->SetVar(GOA::OWNERID, holder_->GetID());
        node_->SetVar(GOA::DIRECTION, direction.x_);
        node_->SetVar(GOA::ONVIEWZ, holderdestroyer->GetWorldMapPosition().viewZ_);

        const BoundingBox& holderbox = holder_->GetDerivedComponent<StaticSprite2D>()->GetWorldBoundingBox();
        float angle = Atan(direction.y_/direction.x_);
//        node_->SetWorldPosition2D(holderbox.Center2D() + holderbox.HalfSize2D() * 0.5f * direction);
        node_->SetWorldPosition2D(holderbox.Center2D() + holderbox.HalfSize2D() * 0.75f * direction);
        node_->SetWorldRotation2D(angle);

        node_->SendEvent(WORLD_ENTITYCREATE);

        StaticSprite2D* staticSprite = node_->GetDerivedComponent<StaticSprite2D>();
        if (staticSprite)
            staticSprite->SetFlip(direction.x_ < 0.f, false);

        node_->SetEnabled(true);

        RigidBody2D* body = node_->GetComponent<RigidBody2D>();
        if (body)
        {
            body->SetLinearVelocity(direction * SHOOTER_VEL);
            body->SetAngularVelocity(0.f);
        }

        if (!GameContext::Get().LocalMode_ && UseNetworkReplication())
        {
            ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node_, holder_, false, true, true);
            if (oinfo && cinfo)
                *oinfo = cinfo;
        }

        isInUse_ = false;

        const Vector2& pos = node_->GetWorldPosition2D();
        URHO3D_LOGINFOF("ABI_Shooter() - Use : pos=%F,%F dir=%F,%F faction=%u viewZ=%d", pos.x_, pos.y_, direction.x_, direction.y_, node_->GetVar(GOA::FACTION).GetUInt(), node_->GetVar(GOA::ONVIEWZ).GetInt());
    }
    else
    {
        if (!node_)
        {
            isInUse_ = false;
            return 0;
        }
    }

//    URHO3D_LOGINFOF("ABI_Shooter() - Use : dir=%f,%f ... OK !", direction.x_, direction.y_);

    return node;
}


/// ABIBomb (BOMB)


void ABIBomb::RegisterObject(Context* context)
{
    context->RegisterFactory<ABIBomb>();
}

ABIBomb::ABIBomb(Context* context) :
    Ability(context)
{
    usePool_ = useNetwork_ = true;
    icon_ = IntRect(0, 413, 80, 493);
}

Node* ABIBomb::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
    Node* node = 0;

    if (!isInUse_)
    {
        Vector2 direction = wpoint - holder_->GetWorldPosition2D();
        direction.Normalize();

        if (holder_->GetVar(GOA::MOVESTATE).GetUInt() & MV_TOUCHGROUND && direction.y_ < 0.f && direction.x_*direction.x_ < 0.2f)
            return 0;

        GOC_Destroyer* holderdestroyer = holder_->GetComponent<GOC_Destroyer>();
        if (!holderdestroyer)
        {
            URHO3D_LOGERROR("ABIBomb() - Use : holder_ don't have a gocDestroyer !");
            return 0;
        }

        if (pool_)
        {
            node = pool_->GetGO();
        }
        else
        {
            node = holder_->GetScene()->CreateChild(String::EMPTY, LOCAL);
            GameHelpers::SpawnGOtoNode(context_, GOT_, node);
        }

        if (!node)
        {
            node_.Reset();
            isInUse_ = false;
            return 0;
        }

        if (node->GetID() < 2)
        {
            URHO3D_LOGERRORF("ABIBomb() - Use : node=%u not allowed !", node->GetID());
            node_.Reset();
            return 0;
        }

        isInUse_ = true;
        node_ = node;

        node_->SetVar(GOA::FACTION, holder_->GetVar(GOA::FACTION).GetUInt());
//        node_->SetVar(GOA::FACTION, (node->GetVar(GOA::CLIENTID).GetUInt() << 8) + GO_Player);
        node_->SetVar(GOA::OWNERID, holder_->GetID());
        node_->SetVar(GOA::DIRECTION, direction.x_);
        node_->SetVar(GOA::ONVIEWZ, holderdestroyer->GetWorldMapPosition().viewZ_);

        const BoundingBox& holderbox = holder_->GetDerivedComponent<StaticSprite2D>()->GetWorldBoundingBox();
        float angle = Atan(direction.y_/direction.x_);
        node_->SetWorldPosition2D(holderbox.Center2D() + holderbox.HalfSize2D() * 0.5f * direction);
        node_->SetWorldRotation2D(angle);

        node_->SendEvent(WORLD_ENTITYCREATE);

        StaticSprite2D* staticSprite = node_->GetDerivedComponent<StaticSprite2D>();
        if (staticSprite)
            staticSprite->SetFlip(direction.x_ < 0.f, false);

        node_->SetEnabled(true);

        RigidBody2D* body = node_->GetComponent<RigidBody2D>();
        if (body)
        {
            body->SetLinearVelocity(direction * 7.f);
            body->SetAngularVelocity(0.f);
        }

        if (!GameContext::Get().LocalMode_ && UseNetworkReplication())
        {
            ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node_, holder_, false, true, true);
            if (oinfo && cinfo)
                *oinfo = cinfo;
        }

        isInUse_ = false;
    }
    else
    {
        if (!node_)
        {
            isInUse_ = false;
            return 0;
        }
    }

//    URHO3D_LOGINFOF("ABIBomb() - Use : dir=%f,%f ... OK !", direction.x_, direction.y_);

    return node;
}


/// ABI_ANIMSHOOTER

void ABI_AnimShooter::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_AnimShooter>();
}

ABI_AnimShooter::ABI_AnimShooter(Context* context) :
    Ability(context)
{
    icon_ = IntRect(82, 331, 162, 411);
}

Node* ABI_AnimShooter::Use(const Vector2& wpoint, ObjectControlInfo** oinfo)
{
    URHO3D_PROFILE(ABI_AnimShooter);

    GOC_Animator2D* animator = holder_->GetComponent<GOC_Animator2D>();
    if (animator)
    {
        if (wpoint != Vector2::ZERO)
        {
            Vector2 direction = wpoint - holder_->GetWorldPosition2D();
            direction.Normalize();
            // correct the holder direction
            if (Sign(animator->GetDirection().x_) != Sign(direction.x_))
                animator->SetDirection(Vector2(direction.x_, animator->GetDirection().y_));
        }

        if (!GameContext::Get().LocalMode_)
        {
            ObjectControlInfo* cinfo = GameNetwork::Get()->GetObjectControl(holder_->GetID());
            if (cinfo)
            {
                ObjectControl& control = cinfo->GetPreparedControl();
                control.holderinfo_.point1x_ = wpoint.x_;
                control.holderinfo_.point1y_ = wpoint.y_;
            }
        }

        animator->SetShootTarget(wpoint);
        animator->SetState(StringHash(STATE_SHOOT));

        URHO3D_LOGINFOF("ABI_AnimShooter() - Use : wpoint=%f,%f ... OK !", wpoint.x_, wpoint.y_);
        return (Node*)1;
    }

    return 0;
}


/// ABI_FLY


void ABI_Fly::RegisterObject(Context* context)
{
    context->RegisterFactory<ABI_Fly>();
}

ABI_Fly::ABI_Fly(Context* context) :
    Ability(context)
{
    icon_ = IntRect(82, 331, 162, 411);
}

void ABI_Fly::OnActive(bool active)
{
    URHO3D_LOGINFOF("ABI_Fly() - OnActive : active=%s !", active ? "true":"false");

    GOC_Move2D* gocmove = holder_->GetComponent<GOC_Move2D>();
    if (gocmove)
    {
        gocmove->SetMoveType(active ? MOVE2D_WALKANDFLY : MOVE2D_WALK);
        if (active)
        {
            Drawable2D* drawable = holder_->GetDerivedComponent<Drawable2D>();
            float angle = holder_->GetVar(GOA::DIRECTION).GetVector2().x_ > 0.f ? 180.f : 0.f;
            GameHelpers::SpawnParticleEffectInNode(holder_->GetContext(), holder_, ParticuleEffect_[PE_TORCHE], drawable->GetLayer(), drawable->GetViewMask(),
                                                   holder_->GetWorldPosition2D()+Vector2(0.f, 0.5f), angle, 3.f, true, 2.f, Color::WHITE, LOCAL);
        }

//        GameHelpers::SetRenderedAnimation(holder_->GetComponent<AnimatedSprite2D>(), String("Special1"), active ? GOT_ : StringHash::ZERO);
    }
}
