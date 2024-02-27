#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/SmoothedTransform.h>
#include <Urho3D/Network/NetworkEvents.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>
#include <Urho3D/Urho2D/ConstraintWheel2D.h>

#include "DefsViews.h"

#include "GameOptions.h"
#include "GameNetwork.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Destroyer.h"
#include "GOC_Collide2D.h"
#include "GOC_Controller.h"
#include "GOC_ControllerAI.h"
#include "GOC_Animator2D.h"

#include "ObjectMaped.h"
#include "Map.h"
#include "MapWorld.h"
#include "ViewManager.h"

#include "GOC_Move2D.h"

//#define DUMP_DEBUG_MOVEUPDATE

const char* velocityNames_[] =
{
    "MIN",
    "WALK",
    "MAX",
    "RUN",
    "CLIMB",
    "CLIMBMIN",
    "CLIMBMAX",
    "JUMP",
    "JUMPMIN",
    "JUMPMAX",
    "DBLEJUMP",
    "FALLMAX",
    "FALLMIN",
    "FLY",
    "FLYMAX",
    "SWIM",
    "SWIMMAX",
    0
};


const GOC_Move2D_Template GOC_Move2D_Template::DEFAULT = { "default", StringHash("default"), { 0.5f, 5.f, 5.5f, 5.f, 4.f, 1.f, 5.f, 1.f, 3.f, 6.f, -0.3f, 15.f, 2.f, 7.f, 9.f, 3.f, 4.f } };

HashMap<StringHash, GOC_Move2D_Template> GOC_Move2D_Template::templates_;

void GOC_Move2D_Template::RegisterTemplate(const String& s)
{
    if (s.Empty())
        return;

    Vector<String> strings;
    strings = s.Split('|');
    StringHash key(strings[0]);

    if (templates_.Contains(key))
        return;

    GOC_Move2D_Template& t = templates_[key];
    t.name_ = strings[0];
    t.hashName_ = key;

    // copy default velocities
    memcpy(t.vel_, GOC_Move2D_Template::DEFAULT.vel_, NUM_VELOCITIES * sizeof(float));

    // get the defined velocities
    Vector<String> values;
    for (unsigned i=1; i < strings.Size(); i++)
    {
        values = strings[i].Split(':');
        if (values.Size() > 1)
        {
            int velindex = GetEnumValue(values[0], velocityNames_);
            if (velindex < NUM_VELOCITIES)
                t.vel_[velindex] = ToFloat(values[1]);
            else
                URHO3D_LOGERRORF("GOC_Move2D_Template() - RegisterTemplate : %s => can't find %s ! ", s.CString(), strings[i].CString());
        }
        else
        {
            t.vel_[i] = ToFloat(strings[i]);
        }
    }
}

const GOC_Move2D_Template& GOC_Move2D_Template::GetTemplate(const String& s)
{
    HashMap<StringHash, GOC_Move2D_Template>::ConstIterator it = templates_.Find(StringHash(s));
    if (it != templates_.End())
    {
        return it->second_;
    }

//	URHO3D_LOGERRORF("GOC_Move2D_Template() - GetTemplate : can't find template %s return default one ! ", s.CString());

    return GOC_Move2D_Template::DEFAULT;
}

void GOC_Move2D_Template::DumpAll()
{

}

void GOC_Move2D_Template::Dump() const
{

}

const float GOC_Move2D::heightMax = 1.f;
const float GOC_Move2D::velJumpMax = 6.f;

const float MOVE_EPSILON = 0.01f;

const char* moveTypeModes[] =
{
    "Walk",
    "WalkAndClimb",
    "WalkAndJumpFirstBox",
    "WalkAndClimbAndSwim",
    "WalkAndJumpFirstBoxAndSwim",
    "WalkAndFly",
    "WalkAndFlyAndSwim",
    "Fly",
    "FlyAndClimb",
    "Swim",
    "SwimAndClimb",
    "WalkAndSwim",
    "Mount",
    0
};


unsigned MoveTypeDefaultStates[] =
{
    MV_WALK,                                // MOVE2D_WALK
    MV_WALK | MV_CLIMB,                     // MOVE2D_WALKCLIMB
    MV_WALK | MV_CLIMBFIRSTBOX,             // MOVE2D_WALKANDJUMPFIRSTBOX
    MV_WALK | MV_CLIMB | MV_SWIM,           // MOVE2D_WALKANDCLIMBANDSWIM        => Chapanze
    MV_WALK | MV_CLIMBFIRSTBOX | MV_SWIM,   // MOVE2D_WALKANDJUMPFIRSTBOXANDSWIM => Petit
    MV_WALK | MV_FLY,                       // MOVE2D_WALKANDFLY                 => Vampire
    MV_WALK | MV_FLY | MV_SWIM,             // MOVE2D_WALKANDFLYANDSWIM          => Vampire Marin
    MV_FLY,                                 // MOVE2D_FLY                        => JunkelSpil
    MV_FLY | MV_CLIMB,                      // MOVE2D_FLYCLIMB                   => Bat
    MV_SWIM,                                // MOVE2D_SWIM                       => Poisson
    MV_SWIM | MV_CLIMB,                     // MOVE2D_SWIMCLIMB
    MV_WALK | MV_SWIM,                      // MOVE2D_WALKANDSWIM                => Elsarion
    MV_MOUNT,
    0
};


GOC_Move2D::GOC_Move2D(Context* context) :
    Component(context),
    template_(&GOC_Move2D_Template::DEFAULT),
    body(0),
    destroyer_(0),
    controller_(0),
    moveType_(MOVE2D_WALK),
    lastMoveType_(MOVE2D_UNDEFINED),
    buttons_(0),
    activeLOS_(0),
    numJumps_(2),
    moveStates_(0),
    physicsEnable_(true),
    physicsReduced_(false)
{ }

GOC_Move2D::~GOC_Move2D()
{ }

void GOC_Move2D::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Move2D>();

    GOC_Move2D_Template::RegisterTemplate("default");

    URHO3D_ACCESSOR_ATTRIBUTE("Register Template", GetEmptyAttr, RegisterTemplate, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Template", GetTemplateName, SetTemplateName, String, String::EMPTY, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Type", GetMoveType, SetMoveType, MoveTypeMode, moveTypeModes, MOVE2D_WALK, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Active LOS", GetActiveLOS, SetActiveLOS, int, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Jumps", GetNumJumps, SetNumJumps, int, 2, AM_FILE);
}

void GOC_Move2D::RegisterTemplate(const String& s)
{
    if (s.Empty())
        return;

    GOC_Move2D_Template::RegisterTemplate(s);
}

void GOC_Move2D::SetTemplateName(const String& s)
{
    if (s.Empty())
        return;

    template_ = &GOC_Move2D_Template::GetTemplate(s);

//	URHO3D_LOGERRORF("GOC_Move2D() - SetTemplateName : %s(%u) : %s => template_=%u ! ", node_->GetName().CString(), node_->GetID(), s.CString(), template_);
}

void GOC_Move2D::SetMoveType(MoveTypeMode type)
{
    if (moveType_ != type)
    {
//        URHO3D_LOGINFOF("GOC_Move2D() - SetMoveType MoveType = %s(%u) ... OK !", moveTypeModes[type], type);
        lastMoveType_ = moveType_;
        moveType_ = type;
        UpdateAttributes();
    }
}

void GOC_Move2D::ResetMoveType()
{
    if (lastMoveType_ != MOVE2D_UNDEFINED)
    {
        moveType_ = lastMoveType_;
        lastMoveType_ = MOVE2D_UNDEFINED;
        UpdateAttributes();
    }
}

void GOC_Move2D::SetActiveLOS(int active)
{
    if (active != activeLOS_)
    {
        activeLOS_ = active;
        UpdateAttributes();
    }
}

void GOC_Move2D::SetPhysicEnable(bool enable, const void* control)
{
    if (!control && controller_)
#ifdef ACTIVE_NETWORK_SERVERRECONCILIATION
        enable = GameContext::Get().ServerMode_ || controller_->IsMainController();
#else
        enable = controller_->IsMainController();
#endif

//    if (enable == physicsEnable_)
//        return;

    URHO3D_LOGINFOF("GOC_Move2D() - SetPhysicEnable : Node=%s(%u) enable=%s ...", node_->GetName().CString(), node_->GetID(), enable ? "true":"false");

    physicsEnable_ = enable;

//    GetComponent<RigidBody2D>()->SetGravityScale(enable ? AIRGRAVITY : NOGRAVITY);

//    SmoothedTransform* smoothing = GetComponent<SmoothedTransform>();
//    if (smoothing)
//        smoothing->SetEnabled(!enable);

    // Set Initial Dynamics
    if (!GameContext::Get().LocalMode_ && enable && control)
    {
        const ObjectControl& ctrl = *((const ObjectControl*)control);

        body->GetBody()->SetVelocity(ctrl.physics_.velx_, ctrl.physics_.vely_);

        URHO3D_LOGINFOF("GOC_Move2D() - SetPhysicEnable : Node=%s(%u) vel=%f %f",
                        node_->GetName().CString(), node_->GetID(), ctrl.physics_.velx_, ctrl.physics_.vely_);

//        body->GetBody()->Dump();
    }

    URHO3D_LOGERRORF("GOC_Move2D() - SetPhysicEnable : Node=%s(%u) enable=%s OK !", node_->GetName().CString(), node_->GetID(), physicsEnable_ ? "true" : "false");
}

void GOC_Move2D::SetNumJumps(int numJumps)
{
    numJumps_ = numJumps;

//    URHO3D_LOGINFOF("GOC_Move2D() - SetNumJumps : Node=%s(%u) numJumps=%d", node_->GetName().CString(), node_->GetID(), numJumps_);
}

void GOC_Move2D::SetVehicleWheels()
{
    if (node_->GetComponent<ObjectMaped>())
        vehicleWheels_ = node_->GetComponent<ObjectMaped>()->GetVehicleWheels();
}


void GOC_Move2D::UpdateAttributes()
{
    Scene* scene = GetScene();
    if (scene && IsEnabledEffective())
    {
        if (!body)
            body = GetComponent<RigidBody2D>();

        assert(body);

//        node_->SetVar(GOA::KEEPDIRECTION, false);

        // Always Active Box2D CCD
//        body->SetBullet(true);

        node_->SetVar(GOA::VELOCITY, Vector2::ZERO);

        destroyer_ = node_->GetComponent<GOC_Destroyer>();
        controller_ = node_->GetDerivedComponent<GOC_Controller>();

        UnsubscribeFromAllEvents();

        if (!controller_)
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) no Controller => UnsubscribeToEvents !", node_->GetName().CString(), node_->GetID());
            return;
        }
        else if (controller_->IsInstanceOf<GOC_AIController>())
        {
            static_cast<GOC_AIController*>(controller_)->UpdateRangeValues(moveType_);
        }

//        URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) mainController_=%s", node_->GetName().CString(), node_->GetID(), controller_->IsMainController() ? "true" : "false");

        if (!controller_->IsMainController())
            activeLOS_ = 0;

#ifdef ACTIVE_NETWORK_LOCALPHYSICSIMULATION_BUTTON_MAINONLY
        /// active Physic simulation only if mainController
        SetPhysicEnable(controller_->IsMainController());
#endif
//        URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) last moveState=%u MV_FALL=%s", node_->GetName().CString(), node_->GetID(), moveStates_, moveStates_ & MV_INFALL ? "true":"false");

        defaultStates_ = MoveTypeDefaultStates[moveType_];

        if ((defaultStates_ & MV_FLY) && !(defaultStates_ & MV_WALK))
            defaultStates_ |= MV_INAIR;

        if (defaultStates_ & MV_MOUNT)
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) Subscribe To MV_MOUNT Events", node_->GetName().CString(), node_->GetID());

            unsigned mountId = node_->GetVar(GOA::ISMOUNTEDON).GetUInt();
            Node* mountNode = mountId ? GetScene()->GetNode(mountId) : 0;
            if (mountNode)
            {
                SubscribeToEvent(mountNode, GO_COLLIDEGROUND, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate_Mount));
                SubscribeToEvent(mountNode, GO_CHANGEDIRECTION, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate_Mount));
                if (lastMoveType_ & MV_FLY)
                {
                    SubscribeToEvent(mountNode, EVENT_FALL, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate_Mount));
                    SubscribeToEvent(mountNode, EVENT_FLYDOWN, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate_Mount));
                    SubscribeToEvent(mountNode, EVENT_FLYUP, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate_Mount));
                }
//                if (lastMoveType_ & MV_WALK)
//                    SubscribeToEvent(mountNode, EVENT_MOVE, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate_Mount));
            }
        }
        else if (defaultStates_ & (MV_WALK | MV_FLY | MV_SWIM))
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) Subscribe To Standard Events", node_->GetName().CString(), node_->GetID());

            SubscribeToEvent(node_, GOC_CONTROLUPDATE, URHO3D_HANDLER(GOC_Move2D, HandleControlUpdate));
            SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_Move2D, HandleUpdate));
        }

        SubscribeToEvent(node_, EVENT_CHANGEGRAVITY, URHO3D_HANDLER(GOC_Move2D, HandleGravityChanged));

        /// reset movetype flags and keep the other flags
        moveStates_ = defaultStates_ + (moveStates_ & MSK_MV_RESETMOVETYPE);

        /// if no contact, reset the contact flags
        int numcontactsbytype[3] = { 0, 0, 0 };

        GOC_Collide2D* collide2D = node_->GetComponent<GOC_Collide2D>();
        if (collide2D)
            collide2D->GetNumContacts(numcontactsbytype);

        if (!numcontactsbytype[Wall_Roof])
            moveStates_ &= ~MV_TOUCHROOF;
        if (!numcontactsbytype[Wall_Border])
            moveStates_ &= ~MV_TOUCHWALL;
        if (!numcontactsbytype[Wall_Ground])
            moveStates_ &= ~MV_TOUCHGROUND;

//        URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) update1 moveState=%u MV_FALL=%s", node_->GetName().CString(), node_->GetID(), moveStates_, moveStates_ & MV_INFALL ? "true":"false");

        if (defaultStates_ & MV_MOUNT)
            moveStates_ |= (MV_WALK | MV_TOUCHGROUND);
        else if (moveStates_ & MSK_MV_CANWALKONGROUND)
        {
            numRemainJumps_ = numJumps_;
            moveStates_ &= ~(MV_INAIR | MV_INFALL | MV_INJUMP);
        }
        else if ((moveStates_ & (MV_FLY | MV_WALK)) || ((moveStates_ & MV_CLIMB) && !(moveStates_ & MSK_MV_TOUCHWALLORROOF)))
        {
            numRemainJumps_ = 0;
            moveStates_ |= MV_INAIR;

            if ((moveStates_ & MV_WALK) && !(moveStates_ & MV_FLY))
                moveStates_ |= MV_INFALL;
        }

//        URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) update moveState=%u MV_FALL=%s", node_->GetName().CString(), node_->GetID(), moveStates_, moveStates_ & MV_INFALL ? "true":"false");

        node_->SetVar(GOA::MOVESTATE, moveStates_);

        SetVehicleWheels();
    }
    else
    {
//        URHO3D_LOGINFOF("GOC_Move2D() - UpdateAttributes : Node=%s(%u) Enabled=false or no Scene => UnsubscribeToEvents !", node_->GetName().CString(), node_->GetID());
        UnsubscribeFromAllEvents();
    }
}


void GOC_Move2D::GetLinearVelocity(Vector2& vel)
{
    lastvel_ = vel;
    vel = body->GetLinearVelocity();
    node_->SetVar(GOA::VELOCITY, vel);
}

void GOC_Move2D::StopMove()
{
    lastvel_ = body->GetLinearVelocity();
    body->GetBody()->SetLinearVelocity(ToB2Vec2(Vector2::ZERO));

//    URHO3D_LOGINFOF("GOC_Move2D() - StopMove : Node=%s(%u) !", node_->GetName().CString(), node_->GetID());
}

void GOC_Move2D::ReduceMove(bool reducex, bool reducey)
{
    if (reducex && fabs(vel_.x_) < template_->vel_[vMINI] && fabs(vel_.x_) < fabs(lastvel_.x_))
        vel_.x_ = 0.f;

    if (reducey && fabs(vel_.y_) < template_->vel_[vMINI] && fabs(vel_.y_) < fabs(lastvel_.y_))
        vel_.y_ = 0.f;

    body->SetLinearVelocity(vel_);
}

void GOC_Move2D::ApplyForce(const Vector2& force, const Vector2& impactPoint, bool rotate)
{
    if (!physicsEnable_)
        return;

    body->ApplyForce(force, impactPoint, true);

    if (rotate)
        body->SetAngularVelocity(-force.x_*force.y_/5.f);
}


void GOC_Move2D::CleanDependences()
{
    moveStates_ = 0;
}

void GOC_Move2D::OnSetEnabled()
{
//    URHO3D_LOGINFOF("GOC_Move2D() - OnSetEnabled");
    UpdateAttributes();
}

void GOC_Move2D::OnNodeSet(Node* node)
{
    if (node)
    {
        UpdateAttributes();
    }
}


/// 32 differents LOS possible (32 players) (1bit/32bits)
void GOC_Move2D::UpdateLineOfSight()
{
    if (activeLOS_ == 0)
        return;

    if (!destroyer_)
        return;

    if (!World2D::GetWorldInfo())
        return;
    if (!destroyer_->GetCurrentMap())
    {
        URHO3D_LOGERRORF("GOC_Move2D() - UpdateLineOfSight : Node=%s(%u) No Current Map !", node_->GetName().CString(), node_->GetID());
        return;
    }

    const WorldMapPosition& mapWorldPosition = destroyer_->GetWorldMapPosition();
    const int& viewZIndex = mapWorldPosition.viewZIndex_;

    if (viewZIndex == NOVIEW)
    {
        URHO3D_LOGERRORF("GOC_Move2D() - UpdateLineOfSight : Node=%s(%u) No View !", node_->GetName().CString(), node_->GetID());
        return;
    }

    unsigned maskLOS = 0x0001 << activeLOS_;

    Map* map = destroyer_->GetCurrentMap();
    PODVector<unsigned>& los = map->GetLosTable();

    const int& width = World2D::GetWorldInfo()->mapWidth_;
    const int& height = World2D::GetWorldInfo()->mapHeight_;
    const int& xpos = mapWorldPosition.mPosition_.x_;
    const int& ypos = mapWorldPosition.mPosition_.y_;

    unsigned addr = ypos*width + xpos;
    if (addr >= los.Size())
    {
        URHO3D_LOGERRORF("GOC_Move2D() - UpdateLineOfSight activeLos=%d : addr(addr=%u,x:%d,y=%d) > los.Size(%u)",
                         activeLOS_, addr, xpos, ypos, los.Size());
        return;
    }

//    URHO3D_LOGINFOF("GOC_Move2D() - UpdateLineOfSight activeLos=%d map=%s", activeLOS_, mapWorldPosition.mPoint_.ToString().CString());

    if (destroyer_->GetCurrentCell())
    {
        float mass = destroyer_->GetCurrentCell()->GetMass();
        int material = destroyer_->GetCurrentCell()->type_;
        los[addr] = material >= WATER ? (mass > FLUID_MAXDRAWF * 0.5f ? material : AIR) : material;
    }
    else
    {
        URHO3D_LOGERRORF("GOC_Move2D() - UpdateLineOfSight : No Current Cell !");
        los[addr] |= maskLOS;
    }

    /// default rectangular losTemplate
    /// TODO : LosTemplate
    const int halflenghtx = 4;
    const int halflenghty = 4;

    /// define rectangular borders
    int xborder[2] = { xpos - halflenghtx, xpos + halflenghtx };
    int yborder[2] = { ypos - halflenghty, ypos + halflenghty };
    const int inc[2] = { -1, 1 };

    /// clamp to current map
    /// TODO : map partitioning
    if (xborder[0] < 0) xborder[0] = -1;
    if (xborder[1] >= width) xborder[1] = width;
    if (yborder[0] < 0) yborder[0] = -1;
    if (yborder[1] >= height) yborder[1] = height;

    /// To Top : block by line
//	for (int i = 0; i<2; i++)
    for (int i = 0; i<1; i++)
    {
        bool blockedy = false;
        for (int y = ypos; y != yborder[i]; y += inc[i])
        {
            if (y != ypos)
                blockedy = (los[(y-inc[i])*width + xpos] == BLOCK);

            addr = y*width;

            for (int j=0; j<2; j++)
                for (int x = xpos; x != xborder[j]; x += inc[j])
                {
                    if (blockedy)
                    {
                        while (x != xborder[j])
                        {
//                        los[addr + x] &= !maskLOS;
//                        los[addr + x] = 0;
                            los[addr + x] = BLOCK;
                            x += inc[j];
                        }
                        break;
                    }
                    int type = map->GetMaterialType(addr+x, viewZIndex);
                    if (type == BLOCK)
                    {
                        while (x != xborder[j])
                        {
                            los[addr + x] = BLOCK;
//                        los[addr + x] &= !maskLOS;
//                        los[addr + x] = 0;
                            x += inc[j];
                        }
                        break;
                    }
                    los[addr + x] = type;
//                los[addr + x] |= maskLOS;
                }
        }
    }
    /// To Bottom : noblock
    for (int y = ypos+inc[1]; y != yborder[1]; y += inc[1])
    {
        addr = y*width;

        for (int j=0; j<2; j++)
            for (int x = xpos; x != xborder[j]; x += inc[j])
            {
                los[addr + x] = map->GetMaterialType(addr+x, viewZIndex);
//                if (map->GetMaterialType(addr+x, viewZIndex) == BLOCK)
//                {
//                    los[addr + x] &= !maskLOS;
//                    continue;
//                }
//                los[addr + x] |= maskLOS;
            }
    }
}


void GOC_Move2D::Dump() const
{
    URHO3D_LOGINFOF("GOC_Move2D() - Dump : moveStates_=%u", moveStates_);

    DumpPhysics();

//    DumpLOS();

//    DumpFeatures();
}

void GOC_Move2D::DumpPhysics() const
{
    const WorldMapPosition& mapWorldPosition = destroyer_->GetWorldMapPosition();

    URHO3D_LOGINFOF("GOC_Move2D() - DumpPhysics : physicenable=%s mainCtrl=%s map=%s pos=%s vZ=%d posInTile=%s(mCenter=%s) vel=%s gravity=%f",
                    physicsEnable_ ? "true":"false", controller_->IsMainController() ? "true":"false",
                    mapWorldPosition.mPoint_.ToString().CString(), mapWorldPosition.mPosition_.ToString().CString(), mapWorldPosition.viewZ_,
                    mapWorldPosition.positionInTile_.ToString().CString(),
                    GetComponent<RigidBody2D>()->GetMassCenter().ToString().CString(),
                    vel_.ToString().CString(), body->GetGravityScale());
}

void GOC_Move2D::DumpLOS() const
{
    const WorldMapPosition& mapWorldPosition = destroyer_->GetWorldMapPosition();

    Map* map = World2D::GetAvailableMapAt(mapWorldPosition.mPoint_);
    if (!map)
        return;

    const int& width = World2D::GetWorldInfo()->mapWidth_;
    const int& height = World2D::GetWorldInfo()->mapHeight_;
    const int& xpos = mapWorldPosition.mPosition_.x_;
    const int& ypos = mapWorldPosition.mPosition_.y_;

    const PODVector<unsigned>& los = map->GetLosTable();
    unsigned addr = ypos*width + xpos;
    if (addr >= los.Size())
    {
        URHO3D_LOGERRORF("GOC_Move2D() - DumpLOS activeLos=%d : addr > los.Size()", activeLOS_);
        return;
    }

    /// default rectangular losTemplate
    /// TODO : LosTemplate
    const int halflenghtx = 3;
    const int halflenghty = 3;

    /// define rectangular borders
    int xborder[2] = { xpos - halflenghtx, xpos + halflenghtx + 1 };
    int yborder[2] = { ypos - halflenghty, ypos + halflenghty + 1 };
    const int inc[2] = { -1, 1 };

    /// clamp to current map
    /// TODO : map partitioning
    if (xborder[0] < 0) xborder[0] = 0;
    if (xborder[1] >= width) xborder[1] = width;
    if (yborder[0] < 0) yborder[0] = 0;
    if (yborder[1] >= height) yborder[1] = height;

    URHO3D_LOGINFO("GOC_Move2D() - DumpLOS : ");

    char blockChar;
    String str;
    str.Reserve((halflenghtx+1)*(halflenghty+1)*4 + 10);
    str = "LOS : \n";
    for (int y=yborder[0]; y < yborder[1]; y++)
        for (int x=xborder[0]; x < xborder[1]; x++)
        {
            if (x == xpos && y == ypos)
            {
                if (los[y*width+x] == BLOCK)
                    blockChar = '*';
                else if (los[y*width+x] > BLOCK)
                    blockChar = 'W';
                else
                    blockChar = 'X';
            }
            else
            {
                if (los[y*width+x] == BLOCK)
                    blockChar = '*';
                else if (los[y*width+x] > BLOCK)
                    blockChar = '~';
                else
                    blockChar = '.';
            }
            str += blockChar;
            if (x == xborder[1]-1) str += "\n";
        }

    URHO3D_LOGINFOF("%s", str.CString());
}

void GOC_Move2D::DumpFeatures() const
{
    const WorldMapPosition& mapWorldPosition = destroyer_->GetWorldMapPosition();

    Map* map = World2D::GetAvailableMapAt(mapWorldPosition.mPoint_);
    if (!map)
        return;

    const int& width = World2D::GetWorldInfo()->mapWidth_;
    const int& height = World2D::GetWorldInfo()->mapHeight_;
    const int& xpos = mapWorldPosition.mPosition_.x_;
    const int& ypos = mapWorldPosition.mPosition_.y_;

    unsigned addr = ypos*width + xpos;

    /// default rectangular losTemplate
    /// TODO : LosTemplate
    const int halflenghtx = 3;
    const int halflenghty = 3;

    /// define rectangular borders
    int xborder[2] = { xpos - halflenghtx, xpos + halflenghtx+1 };
    int yborder[2] = { ypos - halflenghty, ypos + halflenghty+1 };
    const int inc[2] = { -1, 1 };

    /// clamp to current map
    /// TODO : map partitioning
    if (xborder[0] < 0) xborder[0] = 0;
    if (xborder[1] >= width) xborder[1] = width;
    if (yborder[0] < 0) yborder[0] = 0;
    if (yborder[1] >= height) yborder[1] = height;

    URHO3D_LOGINFO("GOC_Move2D() - DumpFeatures : ");

    String str, codes;
    str = "FEATURES CODES (FRONT|INNER) : \n";
    unsigned tileindex;
    for (int y=yborder[0]; y < yborder[1]; y++)
    {
        for (int x=xborder[0]; x < xborder[1]; x++)
        {
            tileindex = map->GetTileIndex(x, y);
            codes = String((unsigned)map->GetFeatureAtZ(tileindex, FRONTVIEW)) + "|" + String((unsigned)map->GetFeatureAtZ(tileindex, INNERVIEW));
            if (x == xpos && y == ypos)
                codes = "(" + codes + ") ";
            else
                codes += " ";

            str += codes;
            if (x == xborder[1]-1) str += "\n";
        }
    }
    bool frontfreetiles = destroyer_->IsOnFreeTiles(FRONTVIEW);
    bool innerfreetiles = destroyer_->IsOnFreeTiles(INNERVIEW);

    URHO3D_LOGINFOF("%s FTFREE=%s INFREE=%s", str.CString(), frontfreetiles ? "true":"false", innerfreetiles ? "true":"false");
}


bool GOC_Move2D::Update_Walk()
{
    if ((moveStates_ & MV_WALK) == 0)
        return false;

    // Handle Update Walk if not CLIMBER+TOUCHROOF
    if (!((moveStates_ & MV_CLIMB) && (moveStates_ & MV_TOUCHROOF)))
    {
        if (buttons_ & (CTRL_RIGHT | CTRL_LEFT) )// && !(moveStates_ & MV_TOUCHROOF && moveStates_ & MV_CLIMB))
        {
            if ((buttons_ & CTRL_RIGHT) && vel_.x_ < template_->vel_[vWALK])
            {
                vel_.x_ = template_->vel_[vWALK];
                ApplyForceX(body->GetMass() > 2.f ? 2.f * vel_.x_ * body->GetMass() : vel_.x_*4.f);

                if (fabs(vel_.x_) <= 2.f * template_->vel_[vMINI])
                    ApplyImpulseX(vel_.x_);
            }
            if ((buttons_ & CTRL_LEFT) && vel_.x_ > -template_->vel_[vWALK])
            {
                vel_.x_ = -template_->vel_[vWALK];
                ApplyForceX(body->GetMass() > 2.f ? 2.f * vel_.x_ * body->GetMass() : vel_.x_*4.f);

                if (fabs(vel_.x_) <= 2 * template_->vel_[vMINI])
                    ApplyImpulseX(vel_.x_);
            }

            // Climb for the first box height for Walker only
            if ((moveStates_ & MV_TOUCHWALL) && (moveStates_ & MV_CLIMBFIRSTBOX) && numRemainJumps_ > 0)
            {
                if (vel_.y_ < template_->vel_[vCLIMB] && (moveStates_ & MV_TOUCHROOF) == 0)
                {
                    ApplyImpulseY(template_->vel_[vJUMP]);
                    //                if (vel_.y_ < template_->velJumpMin_)
                    //                    ApplyForceY(template_->velClimb_);
#ifdef DUMP_DEBUG_MOVEUPDATE
                    URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) update firstbox climbing => MV_TOUCHWALL ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif
                }

                // Stop Climb the Wall, if max or touch ground/roof when no climbing (Patch 29/04/2021 adding mv_touchground)
                if ((vel_.y_ > template_->vel_[vJUMPMIN] && vel_.y_ <= lastvel_.y_) || ((moveStates_ & MV_CLIMB) == 0 && (moveStates_ & (MV_TOUCHROOF|MV_TOUCHGROUND))))
                {
                    // Patch (TODO 03/08/2017 : Petit fige en FALL apres Stop FIRSTBOX) => correctif moveStates
                    moveStates_ = (moveStates_ & ~(MV_TOUCHGROUND | MV_INJUMP | MV_TOUCHWALL)) | MV_INFALL;
#ifdef DUMP_DEBUG_MOVEUPDATE
                    URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) update firstbox climbing => Stop MV_TOUCHWALL FIRSTBOX ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif
                    node_->SendEvent(EVENT_FALL);
                }
            }
        }
    }

    // Handle Update Jumping / changing to Fall
    if (moveStates_ & MV_INJUMP)
    {
        // allow jump when MV_TOUCHOBJECT (GOC_StaticRope)
        if (buttons_ & CTRL_JUMP && (vel_.y_ > 0.f || (moveStates_ & MV_TOUCHOBJECT)) && vel_.y_ < template_->vel_[vJUMPMAX])
        {
            if (body->GetMass() >= 1.5f)
                ApplyImpulseY(0.5f * body->GetMass() * template_->vel_[vJUMPMIN]);
            else
                ApplyImpulseY(template_->vel_[vJUMP]);

            if (vel_.y_ <= template_->vel_[vJUMPMIN])
            {
                ApplyForceY(template_->vel_[vJUMPMAX]);
                vel_.y_ = template_->vel_[vJUMPMIN];
            }
//        #ifdef DUMP_DEBUG_MOVEUPDATE
//            URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) Update Jumping ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
//        #endif
        }

        if (((vel_.y_ < template_->vel_[vJUMPMIN] || destroyer_->GetWorldMapPosition().position_.y_ - startjumpy_ > heightMax) && !(moveStates_ & MV_TOUCHOBJECT)) || moveStates_ & MV_TOUCHROOF)
        {
            moveStates_ = (moveStates_ & ~(MV_INJUMP | MSK_MV_TOUCHWALLS)) | MV_INFALL | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) update jump => ONFALL height=%f vel.y=%f lastvely=%f (m=%u) ", node_->GetName().CString(), node_->GetID(),
                            destroyer_->GetWorldMapPosition().position_.y_ - startjumpy_, vel_.y_, lastvel_.y_, moveStates_);
#endif
            node_->SendEvent(EVENT_FALL);
        }
    }

    // Handle Update Falling
    if (moveStates_ & MV_INFALL)
    {
//    #ifdef DUMP_DEBUG_MOVEUPDATE
//        URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) INFALL ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
//    #endif
        if (vel_.y_ < -template_->vel_[vFALLMAX])
            SetLinearVelocityY(-template_->vel_[vFALLMAX]);
    }
    // Change to Fall if velocity is enough to fall and if it doesn't climb a wall (2020-02-21 for chapanzé pop on fall)
    // 2020-11-01 Remove the MV_TOUCHGROUND condition
    else if (vel_.y_ < -template_->vel_[vFALLMIN] /*&& !(moveStates_ & MV_TOUCHGROUND)*/ && (!(moveStates_ & MV_CLIMB) || ((moveStates_ & MV_CLIMB) && !(moveStates_ & MSK_MV_TOUCHWALLORROOF))))
    {
        if (moveStates_ & MV_TOUCHGROUND)
            moveStates_ = (moveStates_ & ~MV_TOUCHGROUND);

        moveStates_ = (moveStates_ & ~MV_INJUMP) | MV_INFALL | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) ONFALL Remove MV_TOUCHGROUND ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif
        node_->SendEvent(EVENT_FALL);
    }

    // 2021-02-24 : Test for some cases of loose of ground contacts.
    if (!(moveStates_ & MV_CLIMB) && !(moveStates_ & MV_TOUCHGROUND) && (Abs(vel_.y_) < MOVE_EPSILON))
    {
        moveStates_ = (moveStates_ & ~(MV_INFALL | MV_INJUMP)) | MV_TOUCHGROUND;
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - Update_Walk : Node=%s(%u) update => Force on Ground vel.y=%f (m=%u) ", node_->GetName().CString(), node_->GetID(),
                        vel_.y_, moveStates_);
#endif
    }

    return true;
}

bool GOC_Move2D::Update_Fly()
{
    if ((moveStates_ & MSK_MV_FLYAIR) != MSK_MV_FLYAIR || (moveStates_ & MSK_MV_CANWALKONGROUND) == MSK_MV_CANWALKONGROUND)  // (FLY WITH INAIR) ONLY, SKIP FLY IF CAN WALK AND IS ON GROUND (VAMPIRE)
        return false;

    if (buttons_ & (CTRL_RIGHT | CTRL_LEFT))
    {
        // move if not CLIMBER+TOUCHROOF
//        if (fabs(vel_.x_) <= velWalk && !(moveStates_ & MV_TOUCHROOF && moveStates_ & MV_CLIMB))
//        {
//            vel_.x_ = controller_->control_.direction_*velWalk;
//            ApplyForceX(vel_.x_*4.f);
//        }
        if (!(moveStates_ & MV_TOUCHROOF && moveStates_ & MV_CLIMB))
        {
            if ((buttons_ & CTRL_RIGHT) && vel_.x_ < template_->vel_[vWALK])
            {
                vel_.x_ = template_->vel_[vWALK];
                ApplyForceX(vel_.x_*4.f);
            }
            if ((buttons_ & CTRL_LEFT) && vel_.x_ > -template_->vel_[vWALK])
            {
                vel_.x_ = -template_->vel_[vWALK];
                ApplyForceX(vel_.x_*4.f);
            }
        }
    }

    if (vel_ == Vector2::ZERO)
        return true;

    if (vel_.y_ <= template_->vel_[vMINI] && !(moveStates_ &  MV_INLIQUID))
        ApplyForceY(template_->vel_[vFLY]);

#ifdef DUMP_DEBUG_MOVEUPDATE
    URHO3D_LOGINFOF("GOC_Move2D() - Update_Fly mv=%u vel.y_=%f !", moveStates_, vel_.y_);
#endif

    if (vel_.y_ < -template_->vel_[vFALLMIN] && !(moveStates_ &  MV_CLIMB))
    {
        bool touchground = (moveStates_ & MV_TOUCHGROUND);
        moveStates_ = (moveStates_ & ~MV_TOUCHGROUND) | MV_INFALL | MV_INAIR;
        node_->SetVar(GOA::MOVESTATE, moveStates_);

        // Remove Ground Contact in GOC_Collide2D
        if (touchground)
            node_->SendEvent(EVENT_FALL);

        if (vel_.y_ < -template_->vel_[vFALLMAX])
            SetLinearVelocityY(-template_->vel_[vFALLMAX]);

        if (controller_->control_.animation_ != STATE_FLYDOWN)
        {
            node_->SendEvent(EVENT_FLYDOWN);
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - Update_Fly : FLY DOWN");
#endif
        }
    }

    return true;
}

bool GOC_Move2D::Update_Swim()
{
    if (!GameContext::Get().gameConfig_.fluidEnabled_ || (moveStates_ & MSK_MV_SWIMLIQUID) != MSK_MV_SWIMLIQUID || (moveStates_ & MSK_MV_TOUCHWALLORROOF))
        return false;

#ifdef DUMP_DEBUG_MOVEUPDATE
//    URHO3D_LOGINFOF("GOC_Move2D() - Update_Swim !");
#endif

    // move if not CLIMBER+TOUCHROOF
    if (!(moveStates_ & MV_TOUCHROOF && moveStates_ & MV_CLIMB))
    {
        if ((buttons_ & CTRL_RIGHT) && vel_.x_ < template_->vel_[vSWIM])
        {
            vel_.x_ = template_->vel_[vSWIM];
            ApplyForceX(vel_.x_ * 4.f);
        }
        else if ((buttons_ & CTRL_LEFT) && vel_.x_ > -template_->vel_[vSWIM])
        {
            vel_.x_ = -template_->vel_[vSWIM];
            ApplyForceX(vel_.x_ * 4.f);
        }
        if ((buttons_ & CTRL_UP) && vel_.y_ < template_->vel_[vSWIM])
        {
            vel_.y_ = template_->vel_[vSWIM];
            ApplyForceY(vel_.y_ * 2.f);
        }
        else if ((buttons_ & CTRL_DOWN) && vel_.y_ > -template_->vel_[vSWIM])
        {
            vel_.y_ = -template_->vel_[vSWIM];
            ApplyForceY(vel_.y_ * 2.f);
        }
    }

    return true;
}

void GOC_Move2D::Update_Climb()
{
    if (!(moveStates_ & MV_CLIMB))
        return;

    // Handle Update CLIMB
    if (moveStates_ & MV_TOUCHWALL)
    {
        if (buttons_ & CTRL_UP)
        {
            if (vel_.y_ <= template_->vel_[vCLIMBMIN])
            {
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - Update_Climb : CTRL_UP : Go up along the wall !");
#endif
                ApplyImpulseY(template_->vel_[vCLIMBMAX]);
            }

            if (moveStates_ & MV_TOUCHGROUND)
            {
                if (buttons_ & CTRL_RIGHT)
                {
//                    ApplyForceX(velWalk*4.f);
                    ApplyImpulseX(template_->vel_[vCLIMBMAX]);
                }
                else if (buttons_ & CTRL_LEFT)
                {
//                    ApplyForceX(-velWalk*4.f);
                    ApplyImpulseX(-template_->vel_[vCLIMBMAX]);
                }
            }
        }
        else if (buttons_ & CTRL_DOWN)
        {
            if (vel_.y_ >= -template_->vel_[vCLIMBMIN])
            {
                ApplyImpulseY(-template_->vel_[vCLIMBMAX]);
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - Update_Climb : CTRL_DOWN : Go down along the wall !");
#endif
                if (moveStates_ & MV_TOUCHROOF)
                {
                    ApplyForceY(-template_->vel_[vWALK]*4.f);
                }
            }

            if (moveStates_ & MV_TOUCHGROUND)
            {
                if (buttons_ & CTRL_RIGHT)
                {
//                    ApplyForceX(velWalk*4.f);
                    ApplyImpulseX(template_->vel_[vCLIMBMAX]);
                }
                else if (buttons_ & CTRL_LEFT)
                {
//                    ApplyForceX(-velWalk*4.f);
                    ApplyImpulseX(-template_->vel_[vCLIMBMAX]);
                }
            }
        }
        else if (vel_.y_ != 0.f)
        {
            StopMove();
        }
    }

    if (moveStates_ & MV_TOUCHROOF)
    {
        if (buttons_ & CTRL_RIGHT)
        {
            if (vel_.x_ <= template_->vel_[vCLIMBMIN])
                ApplyImpulseX(template_->vel_[vCLIMBMAX]);
        }
        else if (buttons_ & CTRL_LEFT)
        {
            if (vel_.x_ >= -template_->vel_[vCLIMBMIN])
                ApplyImpulseX(-template_->vel_[vCLIMBMAX]);
        }
        else if (vel_.x_ != 0.f)
        {
            StopMove();
        }
    }
}



bool GOC_Move2D::ControlUpdate_Air()
{
    if ((moveStates_ & MSK_MV_FLYAIR) != MSK_MV_FLYAIR || (moveStates_ & MSK_MV_CANWALKONGROUND) == MSK_MV_CANWALKONGROUND)  // (FLY WITH INAIR) ONLY, SKIP FLY IF CAN WALK AND IS ON GROUND (VAMPIRE)
        return false;

//    URHO3D_PROFILE(GOC_Move2D);

#ifdef DUMP_DEBUG_MOVEUPDATE
    URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate_Fly : buttons=%u", buttons_);
#endif
    if (buttons_ & CTRL_LEFT)
    {
        if (controller_->control_.direction_ >= 0.f)
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate_Fly : Change direction To LEFT");
            ApplyForceX(-template_->vel_[vWALK] * 4.f);
//            ApplyImpulseX(-velWalk);

            node_->SendEvent(GO_CHANGEDIRECTION);
        }
    }

    else if (buttons_ & CTRL_RIGHT)
    {
        if (controller_->control_.direction_ <= 0.f)
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate_Fly : Change direction To RIGHT");
//            ApplyImpulseX(velWalk);
            ApplyForceX(template_->vel_[vWALK] * 4.f);

            node_->SendEvent(GO_CHANGEDIRECTION);
        }
    }

    if ((buttons_ & CTRL_JUMP) && !(moveStates_ & MV_TOUCHROOF))
    {
//        moveStates_ = moveStates_ | MV_INAIR;
        moveStates_ = moveStates_ | MV_INJUMP;
        moveStates_ = moveStates_ & ~MV_INFALL;

        // Go Down
        if ((buttons_ & CTRL_DOWN) && (moveStates_ & MV_TOUCHGROUND))
        {
            if (GoDownPlateform())
            {
                moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS) | MV_INFALL | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Air : Node=%s(%u) Go DOWN ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif
                node_->SetVar(GOA::MOVESTATE, moveStates_);
                ApplyImpulseY(-template_->vel_[vJUMP]);
                node_->SendEvent(EVENT_FALL);
            }

            return true;
        }
        else if (moveStates_ & MSK_MV_TOUCHWALLS)
        {
            moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS);

            // Remove Ground Contact in GOC_Collide2D
            node_->SendEvent(EVENT_JUMP);

//            if (moveStates_ & MV_CLIMB)
//                body->SetGravityScale(1.f);

            node_->SetVar(GOA::MOVESTATE, moveStates_);
        }

        if (vel_.y_ < template_->vel_[vFLYMAX])
            ApplyImpulseY(template_->vel_[vFLY]);

        if (controller_->control_.animation_ != STATE_FLYUP)
        {
            node_->SendEvent(EVENT_FLYUP);
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate_Fly : FLY UP");
#endif
        }
    }

    return true;
}

bool GOC_Move2D::ControlUpdate_Liquid()
{
    if (!GameContext::Get().gameConfig_.fluidEnabled_ || (moveStates_ & MSK_MV_SWIMLIQUID) != MSK_MV_SWIMLIQUID || (moveStates_ & MSK_MV_TOUCHWALLORROOF))
        return false;

//    URHO3D_PROFILE(GOC_Move2D);

#ifdef DUMP_DEBUG_MOVEUPDATE
//    URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Liquid : buttons=%u", buttons_);
#endif
    if (buttons_ & CTRL_LEFT)
    {
        if (controller_->control_.direction_ >= 0.f)
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate_Swim : Change direction To LEFT");
            ApplyForceX(-template_->vel_[vSWIM] * 4.f);

            node_->SendEvent(GO_CHANGEDIRECTION);
        }
    }

    else if (buttons_ & CTRL_RIGHT)
    {
        if (controller_->control_.direction_ <= 0.f)
        {
//            URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate_Swim : Change direction To RIGHT");
            ApplyForceX(template_->vel_[vSWIM] * 4.f);

            node_->SendEvent(GO_CHANGEDIRECTION);
        }
    }

    if (buttons_ & CTRL_UP)
    {
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Liquid : CTRL_UP !");
#endif
        ApplyForceY(template_->vel_[vSWIM] * 4.f);
    }
    else if (buttons_ & CTRL_DOWN)
    {
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Liquid : CTRL_DOWN !");
#endif
        ApplyForceY(-template_->vel_[vSWIM] * 4.f);
    }

    if ((buttons_ & CTRL_JUMP) && !(moveStates_ & MV_TOUCHROOF))
    {
//        moveStates_ = moveStates_ | MV_INLIQUID;

        if (moveStates_ & MSK_MV_TOUCHWALLS)
        {
            moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS);
//            if (moveStates_ & MV_CLIMB)
//                body->SetGravityScale(0.2f);

            node_->SetVar(GOA::MOVESTATE, moveStates_);

            // Go Down
            if (buttons_ & CTRL_DOWN)
            {
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Liquid : Node=%s(%u) buttons=%u Try JUMP DOWN", node_->GetName().CString(), node_->GetID(), buttons_);
#endif

                if (GoDownPlateform())
                {
#ifdef DUMP_DEBUG_MOVEUPDATE
                    URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Liquid : Node=%s(%u) Go DOWN ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif

                    ApplyImpulseY(-template_->vel_[vSWIM]);
                }

                return true;
            }
        }

        ApplyImpulseY(buttons_ & CTRL_DOWN ? -template_->vel_[vSWIM] : template_->vel_[vSWIMMAX]);

        // Remove Ground Contact in GOC_Collide2D
        node_->SendEvent(EVENT_JUMP);

        if (moveStates_ & MV_FLY)
            node_->SendEvent(EVENT_MOVE_AIR);
    }

    return true;
}

bool GOC_Move2D::ControlUpdate_Ground()
{
    if (!(moveStates_ & MV_WALK))
        return false;

//    URHO3D_PROFILE(GOC_Move2D);

#ifdef DUMP_DEBUG_MOVEUPDATE
    URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) buttons=%u mv=%s(%u) ... ", node_->GetName().CString(), node_->GetID(), buttons_, GameHelpers::GetMoveStateString(moveStates_).CString(), moveStates_);
#endif

    if (moveStates_ & MV_CLIMB && moveStates_ & MV_TOUCHWALL)
    {
        // come down from the wall to the Left
        if ((buttons_ & CTRL_LEFT) && !(moveStates_ & MV_DIRECTION))
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Come from the wall to the Left", node_->GetName().CString(), node_->GetID());
#endif
            ApplyForceX(-template_->vel_[vWALK] * 4.f);

            if (!node_->GetVar(GOA::KEEPDIRECTION).GetBool())
            {
                lastDirectionX_ = -1;
                node_->SendEvent(GO_CHANGEDIRECTION);
            }

            if (!(moveStates_ & (MV_TOUCHROOF|MV_TOUCHGROUND)))
            {
                startjumpy_ = destroyer_->GetWorldMapPosition().position_.y_;
                numRemainJumps_--;
                moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS) | MV_INJUMP | MV_INAIR;
                node_->SetVar(GOA::MOVESTATE, moveStates_);
                ApplyImpulseY(body->GetMass() > 2.f ? 0.5f* body->GetMass() * template_->vel_[vJUMPMIN] : template_->vel_[vJUMPMIN]);
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Climber On Wall Go JUMP startjumpy_=%f (numjumps=%d/%d) ! (m=%u)", node_->GetName().CString(), node_->GetID(), startjumpy_, numRemainJumps_, numJumps_, moveStates_);
#endif
                node_->SendEvent(EVENT_JUMP);
            }
            return true;
        }
        // come down from the wall to the right
        else if ((buttons_ & CTRL_RIGHT) && (moveStates_ & MV_DIRECTION))
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Come from the wall to the right", node_->GetName().CString(), node_->GetID());
#endif
            ApplyForceX(template_->vel_[vWALK] * 4.f);

            if (!node_->GetVar(GOA::KEEPDIRECTION).GetBool())
            {
                lastDirectionX_ = 1;
                node_->SendEvent(GO_CHANGEDIRECTION);
            }
            if (!(moveStates_ & (MV_TOUCHROOF|MV_TOUCHGROUND)))
            {
                startjumpy_ = destroyer_->GetWorldMapPosition().position_.y_;
                numRemainJumps_--;
                moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS) | MV_INJUMP | MV_INAIR;
                node_->SetVar(GOA::MOVESTATE, moveStates_);
                ApplyImpulseY(body->GetMass() > 2.f ? 0.5f* body->GetMass() * template_->vel_[vJUMPMIN] : template_->vel_[vJUMPMIN]);
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Climber On Wall Go JUMP startjumpy_=%f (numjumps=%d/%d) ! (m=%u)", node_->GetName().CString(), node_->GetID(), startjumpy_, numRemainJumps_, numJumps_, moveStates_);
#endif
                node_->SendEvent(EVENT_JUMP);
            }
            return true;
        }
        // Change Direction to Up
        if ((buttons_ & CTRL_UP) && controller_->control_.direction_ >= 0.f)
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Change direction To UP", node_->GetName().CString(), node_->GetID());
#endif
            if (!node_->GetVar(GOA::KEEPDIRECTION).GetBool())
            {
                lastDirectionY_ = -1;
                node_->SendEvent(GO_CHANGEDIRECTION);
            }
        }
        // Change Direction to Down
        else if ((buttons_ & CTRL_DOWN) && controller_->control_.direction_ <= 0.f)
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Change direction To DOWN", node_->GetName().CString(), node_->GetID());
#endif
            if (!node_->GetVar(GOA::KEEPDIRECTION).GetBool())
            {
                lastDirectionY_ = 1;
                node_->SendEvent(GO_CHANGEDIRECTION);
            }
        }
    }
    else
    {
        // Change Direction To Left
        if ((buttons_ & CTRL_LEFT) && controller_->control_.direction_ >= 0.f)
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Change direction To LEFT", node_->GetName().CString(), node_->GetID());
#endif
            ApplyForceX(-template_->vel_[vWALK] * 4.f);

            if (!node_->GetVar(GOA::KEEPDIRECTION).GetBool())
            {
                lastDirectionX_ = -1;
                node_->SendEvent(GO_CHANGEDIRECTION);
            }
        }
        // Change Direction To Right
        else if ((buttons_ & CTRL_RIGHT) && controller_->control_.direction_ <= 0.f)
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Change direction To RIGHT", node_->GetName().CString(), node_->GetID());
#endif
            ApplyForceX(template_->vel_[vWALK] * 4.f);

            if (!node_->GetVar(GOA::KEEPDIRECTION).GetBool())
            {
                lastDirectionX_ = 1;
                node_->SendEvent(GO_CHANGEDIRECTION);
            }
        }
    }

    // Handle Go Down / Jumping
    if (buttons_ & CTRL_JUMP)
    {
        // Go Down
        if (buttons_ & CTRL_DOWN)
        {
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) buttons=%u Try JUMP DOWN", node_->GetName().CString(), node_->GetID(), buttons_);
#endif
            if (moveStates_ & MSK_MV_TOUCHWALLS)
            {
                if (GoDownPlateform())
                {
                    moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS) | MV_INFALL | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
                    URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Go DOWN ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif
                    node_->SetVar(GOA::MOVESTATE, moveStates_);
                    ApplyImpulseY(-template_->vel_[vJUMP]);
                    node_->SendEvent(EVENT_FALL);
                }
                else
                {
#ifdef DUMP_DEBUG_MOVEUPDATE
                    URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Can't Go DOWN ! (m=%u)", node_->GetName().CString(), node_->GetID(), moveStates_);
#endif
                }
                return true;
            }
        }
        // TouchRoof Falling
        if (moveStates_ & MV_TOUCHROOF)
        {
            moveStates_ = (moveStates_ & ~(MV_TOUCHROOF | MV_INJUMP)) | MV_INFALL | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGWARNING("GOC_Move2D() - ControlUpdate_Ground : Can't Jump ; Touch Roof !");
#endif
            node_->SetVar(GOA::MOVESTATE, moveStates_);
            ApplyImpulseY(-template_->vel_[vJUMP]);
            node_->SendEvent(EVENT_FALL);
            return true;
        }
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) buttons=%u Try JUMP", node_->GetName().CString(), node_->GetID(), buttons_);
#endif
        // Jumping
        if (numRemainJumps_ > 0 || ((moveStates_ & MV_TOUCHOBJECT) && !(moveStates_ & MV_TOUCHROOF)))
        {
            // allow jump when MV_TOUCHOBJECT (GOC_StaticRope)
            if ((moveStates_ & (MV_TOUCHGROUND | MV_TOUCHWALL | MV_TOUCHOBJECT)) && numJumps_ > 0)
            {
                startjumpy_ = destroyer_->GetWorldMapPosition().position_.y_;

                if (!(moveStates_ & MV_TOUCHOBJECT))
                    numRemainJumps_--;

                moveStates_ = (moveStates_ & ~MSK_MV_TOUCHWALLS) | MV_INJUMP | MV_INAIR;

                node_->SetVar(GOA::MOVESTATE, moveStates_);

                if (body->GetMass() > 2.f)
                    ApplyImpulseY(0.5f* body->GetMass() * template_->vel_[vJUMPMIN]);
                else
                    ApplyImpulseY(template_->vel_[vJUMPMIN]);
#ifdef DUMP_DEBUG_MOVEUPDATE
                URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Go JUMP startjumpy_=%f (numjumps=%d/%d) ! (m=%u)", node_->GetName().CString(), node_->GetID(), startjumpy_, numRemainJumps_, numJumps_, moveStates_);
#endif
                node_->SendEvent(EVENT_JUMP);
            }
            else if ((moveStates_ & MV_INAIR) && (numJumps_ > 1))
            {
                if (numRemainJumps_ == numJumps_ || vel_.y_ > template_->vel_[vMINDOUBLEJUMP])
                {
                    startjumpy_ = destroyer_->GetWorldMapPosition().position_.y_;

                    numRemainJumps_--;

                    moveStates_ = moveStates_ | MV_INJUMP;
                    node_->SetVar(GOA::MOVESTATE, moveStates_);
                    ApplyImpulseY(template_->vel_[vJUMPMIN]);
#ifdef DUMP_DEBUG_MOVEUPDATE
                    URHO3D_LOGINFOF("GOC_Move2D() - ControlUpdate_Ground : Node=%s(%u) Go Double JUMP (numjumps=%d/%d) ! (m=%u)",
                                    node_->GetName().CString(), node_->GetID(), numRemainJumps_, numJumps_, moveStates_);
#endif
                    node_->SendEvent(EVENT_JUMP);
                }
            }
#ifdef DUMP_DEBUG_MOVEUPDATE
            else
            {
                URHO3D_LOGWARNING("GOC_Move2D() - ControlUpdate_Ground : Can't Jump ; Not on Ground or dbleJumpEnable !");
            }
#endif
        }
#ifdef DUMP_DEBUG_MOVEUPDATE
        else
        {
            URHO3D_LOGWARNING("GOC_Move2D() - ControlUpdate_Ground : Can't Jump ; numRemainJumps_=0 !");
        }
#endif
    }
    else
    {
        moveStates_ = moveStates_ & ~MV_INJUMP;
    }

    return true;
}


void GOC_Move2D::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_PROFILE(GOC_Move2D);

    GetLinearVelocity(vel_);

    if (physicsReduced_)
    {
        ReduceMove(false, true);
    }
    else
    {
        body->SetBullet(vel_.LengthSquared() > 8.f);
    }

    if (vehicleWheels_ && fabs(vel_.x_) > 0.2f && Sign(vel_.x_) == Sign(lastvel_.x_))
    {
        const float speed = 1.f;

        PODVector<Node*> wheels;
        vehicleWheels_->GetChildrenWithName(wheels, "WheelDrawable", true);
        for (unsigned i=0; i < wheels.Size(); i++)
        {
            Node* drawablenode = wheels[i];
            if (drawablenode)
                drawablenode->Rotate2D(-speed * Sign(vel_.x_));
        }
    }

    moveStates_ = fabs(vel_.x_) < template_->vel_[vMINI] && fabs(vel_.y_) < template_->vel_[vMINI] ? moveStates_ & ~MV_INMOVE : moveStates_ | MV_INMOVE;

    bool moveUpdated = Update_Fly();
    if (!moveUpdated) moveUpdated = Update_Swim();
    if (!moveUpdated) moveUpdated = Update_Walk();

    if (moveUpdated)
    {
        Update_Climb();
        UpdateLineOfSight();
    }

    node_->SetVar(GOA::MOVESTATE, moveStates_);
}

void GOC_Move2D::HandleControlUpdate(StringHash eventType, VariantMap& eventData)
{
    if (controller_->control_.buttons_ != buttons_)
    {
        buttons_ = controller_->control_.buttons_;
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - OnControlUpdate : Node=%s(%u) buttons=%u ... ", node_->GetName().CString(), node_->GetID(), buttons_);
#endif
    }

    bool controlUpdated = ControlUpdate_Air();
    if (!controlUpdated) controlUpdated = ControlUpdate_Liquid();
    if (!controlUpdated) controlUpdated = ControlUpdate_Ground();
}

void GOC_Move2D::HandleControlUpdate_Mount(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Move2D() - HandleControlUpdate_Mount : node=%s(%u) event=%s movestate=%s ...", node_->GetName().CString(), node_->GetID(), GOE::GetEventName(eventType).CString(), GameHelpers::GetMoveStateString(moveStates_).CString());

    if (eventType == GO_COLLIDEGROUND)
        node_->GetComponent<GOC_Animator2D>()->SetState(STATE_IDLE);
    else
        node_->SendEvent(eventType);
}

void GOC_Move2D::HandleGravityChanged(StringHash eventType, VariantMap& eventData)
{
    if (body->GetGravityScale() == WATERGRAVITY)
    {
        moveStates_ = (moveStates_ & ~MV_INAIR) | MV_INLIQUID;
        node_->SetVar(GOA::MOVESTATE, moveStates_);

        if (!node_->GetVar(GOA::ISDEAD).GetBool())
            node_->SendEvent(EVENT_CHANGEAREA);
    }
    else if (body->GetGravityScale() == AIRGRAVITY)
    {
        moveStates_ = (moveStates_ & ~MV_INLIQUID) | MV_INAIR;
        node_->SetVar(GOA::MOVESTATE, moveStates_);

        if (!node_->GetVar(GOA::ISDEAD).GetBool())
            node_->SendEvent(EVENT_CHANGEAREA);
    }
}


bool GOC_Move2D::GoDownPlateform()
{
    b2ContactEdge* list = body->GetBody()->GetContactList();

    if (!list)
        return false;

    const int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();

    const int switchViewZ = ViewManager::Get()->GetCurrentViewZ();

#ifdef DUMP_DEBUG_MOVEUPDATE
    URHO3D_LOGINFOF("GOC_Move2D() - GoDownPlateform : %s(%u) ... contactlist=%u on viewz=%d ...", node_->GetName().CString(), node_->GetID(), list, viewZ);
#endif

    CollisionShape2D* otherShape;
    int otherviewZ;
    float normaly;
    b2Contact* contact;
    bool hasAPlateformContact = false;

    while (list)
    {
        contact = list->contact;

        if (contact->IsTouching())
        {
            normaly = contact->GetManifold()->localNormal.y;

            // Get the shape that is from otherbody
            otherShape = (CollisionShape2D*)contact->GetFixtureB()->GetUserData();
            if (otherShape->GetRigidBody() == body)
            {
                // swap the othershape so invert the normaly
                normaly = -normaly;

                // Get the othershape and inverse normaly
                otherShape = (CollisionShape2D*)contact->GetFixtureA()->GetUserData();
                if (otherShape->GetRigidBody() == body)
                {
                    list = list->next;
                    continue;
                }
            }

            // skip trigger
            if (!otherShape->IsTrigger())
            {
                otherviewZ = otherShape->GetViewZ();

                // a wall or a plateform or an entity roof shape
                if (otherShape->GetColliderInfo() && otherShape->GetColliderInfo() != WATERCOLLIDER)
                {
                    // a plateform : adjust viewz
                    if (otherShape->GetColliderInfo() == PLATEFORMCOLLIDER)
                        otherviewZ--;

                    // check normaly : if normaly is negative => othershape is under the bodyshape
                    // if normal is positive, the othershape is a roof => skip
                    if (normaly <= 0.f)
                    {
#ifdef DUMP_DEBUG_MOVEUPDATE
                        URHO3D_LOGINFOF("GOC_Move2D() - GoDownPlateform : %s(%u) touches ... collider=%u otherShape=%u node=%s(%u) viewZ=%d otherViewZ=%d normaly=%F ...",
                                        node_->GetName().CString(), node_->GetID(),  otherShape->GetColliderInfo(), otherShape, otherShape->GetNode()->GetName().CString(),
                                        otherShape->GetNode()->GetID(), viewZ, otherviewZ, normaly);
#endif

                        // if in contact with ground, the entity never go down.
                        if (otherShape->GetColliderInfo() != PLATEFORMCOLLIDER && normaly < 0.f && (otherviewZ >= viewZ && otherviewZ <= switchViewZ))
                        {
                            hasAPlateformContact = false;
                            break;
                        }

                        if (otherviewZ < viewZ)
                            hasAPlateformContact = true;
                    }
                }
            }
        }

        list = list->next;
    }

    if (hasAPlateformContact)
    {
        b2ContactEdge* list = body->GetBody()->GetContactList();
        while (list)
        {
            list->contact->SetEnabled(false);
            list = list->next;
        }
    }

    return hasAPlateformContact;
}


extern const char* wallTypeNames[];

void GOC_Move2D::OnWallContactBegin(int walltype, int wallside)
{
    switch (walltype)
    {
    case Wall_Ground:
        numRemainJumps_ = numJumps_;
        moveStates_ = (moveStates_ & ~(MV_INJUMP | MV_INFALL | MV_INAIR)) | defaultStates_ | MV_TOUCHGROUND;
        node_->SetVar(GOA::MOVESTATE, moveStates_);
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactBegin : Node=%s(%u) Add MV_TOUCHGROUND ... on Ground (m=%u) numRemainJumps_=%d", node_->GetName().CString(), node_->GetID(), moveStates_, numRemainJumps_);
#endif
        break;
    case Wall_Border:
        numRemainJumps_ = numJumps_;
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactBegin : Node=%s(%u) Border mv=%s numjumps=%d/%d", node_->GetName().CString(), node_->GetID(), GameHelpers::GetMoveStateString(moveStates_).CString(), numRemainJumps_, numJumps_);
#endif
        moveStates_ = (moveStates_ & ~(MV_INJUMP | MV_INFALL | MV_INAIR)) | defaultStates_ | MV_TOUCHWALL;
        // tag with MV_DIRECTION for wall on left
        if (moveStates_ & MV_CLIMB)
        {
            StopMove();
            moveStates_ = wallside < 0 ? moveStates_ | MV_DIRECTION : moveStates_ & ~MV_DIRECTION;
            node_->SetVar(GOA::MOVESTATE, moveStates_);
            node_->SendEvent(EVENT_CLIMB);
            break;
        }

        node_->SetVar(GOA::MOVESTATE, moveStates_);
        break;
    case Wall_Roof:
        if (moveStates_ & MV_CLIMB)
        {
            numRemainJumps_ = numJumps_;
            moveStates_ = (moveStates_ & ~(MV_INJUMP | MV_INFALL | MV_INAIR)) | defaultStates_ | MV_TOUCHROOF;
            node_->SetVar(GOA::MOVESTATE, moveStates_);
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactBegin : Node=%s(%u) Roof", node_->GetName().CString(), node_->GetID());
#endif
            StopMove();
            node_->SendEvent(EVENT_CLIMB);
        }
        else
        {
            moveStates_ = moveStates_ | MV_TOUCHROOF;
            node_->SetVar(GOA::MOVESTATE, moveStates_);
        }
        break;
    }

//    URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactBegin : Node=%s(%u) walltype=%s ! (m=%u)", node_->GetName().CString(), node_->GetID(), wallTypeNames[walltype], moveStates_);
}

void GOC_Move2D::OnWallContactEnd(int walltype, int numgroundcontacts, int numcontacts)
{
    switch (walltype)
    {
    case Wall_Ground:
        if (numgroundcontacts <= 0 && vel_.y_ < -template_->vel_[vFALLMIN])
        {
            moveStates_ = moveStates_ & ~MV_TOUCHGROUND;
            node_->SetVar(GOA::MOVESTATE, moveStates_);
#ifdef DUMP_DEBUG_MOVEUPDATE
            URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactEnd : Node=%s(%u) Remove MV_TOUCHGROUND ... NumGroundContacts = %d ! (m=%u)", node_->GetName().CString(), node_->GetID(), numgroundcontacts, moveStates_);
#endif
        }
        break;
    case Wall_Border:
        moveStates_ = moveStates_ & ~MV_TOUCHWALL;
        if (!(moveStates_ & MV_TOUCHGROUND) && (!(moveStates_ & MV_CLIMB) || (moveStates_ & MV_TOUCHROOF)))
            moveStates_ = moveStates_ | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactEnd : Node=%s(%u) ~MV_TOUCHWALL NumContacts = %d ! (m=%u)", node_->GetName().CString(), node_->GetID(), numcontacts, moveStates_);
#endif
        node_->SetVar(GOA::MOVESTATE, moveStates_);
        if ((moveStates_ & MV_CLIMB))
        {
            if (numcontacts <= 0)
                body->SetGravityScale(1.f);

            node_->SendEvent(GO_UPDATEDIRECTION);
        }
        break;
    case Wall_Roof:
        moveStates_ = moveStates_ & ~MV_TOUCHROOF;
        if (!(moveStates_ & MV_TOUCHGROUND) && (!(moveStates_ & MV_CLIMB) || (moveStates_ & MV_TOUCHWALL)))
            moveStates_ = moveStates_ | MV_INAIR;
#ifdef DUMP_DEBUG_MOVEUPDATE
        URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactEnd : Node=%s(%u) ~MV_TOUCHROOF NumContacts = %d ! (m=%u)", node_->GetName().CString(), node_->GetID(), numcontacts, moveStates_);
#endif
        node_->SetVar(GOA::MOVESTATE, moveStates_);
        if ((moveStates_ & MV_CLIMB))
        {
            if (numcontacts <= 0)
                body->SetGravityScale(1.f);

            node_->SendEvent(GO_UPDATEDIRECTION);
        }
        break;
    }

//    if (numcontacts == 0)
//    {
//    #ifdef DUMP_DEBUG_MOVEUPDATE
//        URHO3D_LOGINFOF("GOC_Move2D() - OnWallContactEnd : Node=%s(%u) no more contact => FALL ! ", node_->GetName().CString(), node_->GetID());
//    #endif
//		moveStates_ = (moveStates_ & ~(MSK_MV_TOUCHWALLS|MV_INJUMP)) | MV_INFALL | MV_INAIR;
//		node_->SetVar(GOA::MOVESTATE, moveStates_);
//        node_->SendEvent(EVENT_FALL);
//    }
}


