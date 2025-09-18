//#include <ctime>
//#include <chrono>

#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameData.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "GOC_Collide2D.h"
#include "GOC_Inventory.h"
#include "GOC_Destroyer.h"
#include "GOC_ControllerAI.h"

#include "MapWorld.h"

#include "Actor.h"
#include "CraftRecipes.h"
#include "GameGoal.h"


#define MAXTIMEUPDATE 2U   // milliseconds for update pass

static const float DEFAULT_MISSIONUPDATE_INTERVAL = 2.f;

/// Class Objective Implementation

const unsigned OBJECTIVEVARSLOT1 = StringHash("$1").Value();
const unsigned OBJECTIVEVARSLOT2 = StringHash("$2").Value();

unsigned Objective::time_;
HashMap<unsigned, StringHash > Objective::statCategories_;

const char* GoalStateNames_[] =
{
    "NotActived=0",
    "IsActived=1",
    "IsPaused=2",
    "IsRunning=3",
    "IsCompleted=4",
    "IsCompletedOutTime=5",
    "IsFailed=6",
    "IsFinished=7",
    0
};

const char* GoalConditionStr[] =
{
    " ",
    "HasAttacked",
    "HasItem"
};

const char* GoalActionStr[] =
{
    " ",
    "GivesItem",
    "ChangesEntity",
    "PlayAnimation",
    "ReceivesFollower"
};

void ObjectiveCommand::Clear()
{
    actor1NodeId_ = 0;
    actor2NodeId_ = 0;
}

void ObjectiveCommand::SetNodes(Node*& actor1Node, Node*& actor2Node, Node* actorNode, Mission* mission, const ObjectiveCommandData& data)
{
    unsigned& var1 = mission->GetObjectiveVars()[0];
    unsigned& var2 = mission->GetObjectiveVars()[1];

    // get actor1 node
    if (data.actor1Type_)
    {
//        if (!actor1NodeId_)
        {
            if (data.actor1Type_ == OBJECTIVEVARSLOT1)
            {
                if (!var1)
                {
                    actor1Node = actorNode;
                    actor1NodeId_ = actor1Node->GetID();
                    // here define the variable $1
                    var1 = actor1NodeId_;
                }
                else
                    actor1NodeId_ = var1;
            }
            else if (data.actor1Type_ == OBJECTIVEVARSLOT2)
            {
                if (var2)
                    actor1NodeId_ = var2;
            }
        }

        if (actor1NodeId_ && !actor1Node)
            actor1Node = GameContext::Get().rootScene_->GetNode(actor1NodeId_);
    }

    // get actor2 node
    if (data.actor2Type_)
    {
//        if (!actor2NodeId_)
        {
            if (data.actor2Type_ == OBJECTIVEVARSLOT1)
            {
                if (var1)
                    actor2NodeId_ = var1;
            }
            else if (data.actor2Type_ == OBJECTIVEVARSLOT2)
            {
                if (var2)
                    actor2NodeId_ = var2;
            }
        }

        if (actor2NodeId_ && !actor2Node)
            actor2Node = GameContext::Get().rootScene_->GetNode(actor2NodeId_);
    }

    // get actor node for arg type
    if (data.argType_)
    {
        if (!actor1Node && data.argType_ == OBJECTIVEVARSLOT1 && var1)
            actor1Node = GameContext::Get().rootScene_->GetNode(var1);
        else if (!actor2Node && data.argType_ == OBJECTIVEVARSLOT2 && var2)
            actor2Node = GameContext::Get().rootScene_->GetNode(var2);
    }
}

bool ObjectiveCondition::Check(Actor* actor, Mission* mission, const ObjectiveCommandData& data)
{
    Node* actor1Node = 0;
    Node* actor2Node = 0;
    unsigned& var1 = mission->GetObjectiveVars()[0];
    unsigned& var2 = mission->GetObjectiveVars()[1];

    SetNodes(actor1Node, actor2Node, actor->GetAvatar(), mission, data);

    bool result = false;

    // can't succeed if no node found for main actor !
    if (!actor1Node)
        return false;

    // evaluate the condition
    switch (data.cmd_)
    {
    case HasAttacked:
    {
        GOC_Collide2D* collider = actor1Node->GetComponent<GOC_Collide2D>();
        if (!collider)
            return false;

        // get actor2node from a generic type (not from a varslot)
        if (!actor2Node && collider->HasAttacked(DEFAULT_MISSIONUPDATE_INTERVAL) && data.actor2Type_ != OBJECTIVEVARSLOT1 && data.actor2Type_ != OBJECTIVEVARSLOT2)
        {
            if (collider->GetLastAttackedNode()->GetVar(GOA::GOT).GetUInt() == data.actor2Type_)
            {
                actor2Node = collider->GetLastAttackedNode();
                actor2NodeId_ = actor2Node->GetID();
                // here define the variable $2
                var2 = actor2NodeId_;
            }
        }

        if (!actor2Node)
            return false;

        // check if attackednode is actor2node
        if (collider->GetLastAttackedNode() == actor2Node)
        {
            // check if arg is the same type than the attacker's weapon.
            if (data.argType_)
            {
                GOC_Inventory* gocinventory = actor1Node->GetComponent<GOC_Inventory>();
                if (gocinventory)
                {
                    int slotindex = gocinventory->GetSlotIndex(CMAP_WEAPON1);
                    if (slotindex != -1 && data.argType_ == gocinventory->GetSlot(slotindex).type_.Value())
                    {
                        URHO3D_LOGINFOF("ObjectiveCondition() - Check : Actor %s(%u) has attack actor %s(%u) with item %s(type=%u) !",
                                        actor1Node->GetName().CString(), actor1Node->GetID(),
                                        actor2Node->GetName().CString(), actor2Node->GetID(),
                                        GOT::GetType(StringHash(data.argType_)).CString(), data.argType_);
                        result = true;
                        collider->ResetLastAttack();
                    }
                }
            }
            else
            {
                result = true;
            }
        }
    }
    break;
    case HasItem:
        if (data.argType_)
        {
            GOC_Inventory* gocinventory = actor1Node->GetComponent<GOC_Inventory>();
            result = (gocinventory && gocinventory->GetQuantityfor(StringHash(data.argType_)) > 0);
            URHO3D_LOGINFOF("ObjectiveCondition() - Check : Actor %s(%u) has item %s(type=%u) = %s !",
                            actor1Node->GetName().CString(), actor1Node->GetID(),
                            GOT::GetType(StringHash(data.argType_)).CString(), data.argType_,
                            result ? "true":"false");
        }
        break;
    }

    return result;
}

void ObjectiveAction::Execute(Actor* actor, Mission* mission, const ObjectiveCommandData& data)
{
    Node* avatar = actor->GetAvatar();
    Node* actor1Node = 0;
    Node* actor2Node = 0;
    unsigned& var1 = mission->GetObjectiveVars()[0];
    unsigned& var2 = mission->GetObjectiveVars()[1];

    SetNodes(actor1Node, actor2Node, avatar, mission, data);

    // can't succeed if no node found for main actor !
    if (!actor1Node)
        return;

    // get arg
    StringHash argtype;
    if (data.argType_)
    {
        if (data.argType_ == OBJECTIVEVARSLOT1)
            argtype = StringHash(actor1Node->GetVar(GOA::GOT).GetUInt());
        else if (data.argType_ == OBJECTIVEVARSLOT2)
            argtype = StringHash(actor2Node->GetVar(GOA::GOT).GetUInt());
        else
            argtype = StringHash(data.argType_);
    }

    // execute the action
    switch (data.cmd_)
    {
    case GivesItem:
    {
        if (!actor2Node || !argtype.Value())
            return;

        // if the item is in the actor1's inventory, get it to actor2
        GOC_Inventory* gocinventory1 = actor1Node->GetComponent<GOC_Inventory>();
        GOC_Inventory* gocinventory2 = actor2Node->GetComponent<GOC_Inventory>();
        if (gocinventory1 && gocinventory2 && gocinventory1->GetQuantityfor(argtype) > 0)
        {
            int slotIndex1 = gocinventory1->RemoveCollectableOfType(argtype, 1);
            int slotIndex2 = gocinventory2->AddCollectableOfType(argtype, 1, 0, true);

            URHO3D_LOGINFOF("ObjectiveAction() - Execute : Actor %s(%u) gives to actor %s(%u) a item %s(type=%u) (slotindex=%d) OK !",
                            actor1Node->GetName().CString(), actor1Node->GetID(),
                            actor2Node->GetName().CString(), actor2Node->GetID(),
                            GOT::GetType(argtype).CString(), data.argType_, slotIndex2);

            if (avatar == actor1Node)
            {
                VariantMap& eventData = GameContext::Get().context_->GetEventDataMap();
                eventData[Go_InventoryGet::GO_IDSLOT] = slotIndex1;
                actor->SendEvent(PANEL_SLOTUPDATED, eventData);
            }
            else if (avatar == actor2Node)
            {
                VariantMap& eventData = GameContext::Get().context_->GetEventDataMap();
                eventData[Go_InventoryGet::GO_IDSLOT] = slotIndex2;
                actor->SendEvent(PANEL_SLOTUPDATED, eventData);
            }
        }
    }
    break;
    case ChangesEntity:
    {
        StringHash got(data.argType_);
        URHO3D_LOGINFOF("ObjectiveAction() - Execute : %s(%u) changes entity to %s(%u) ...",
                        actor1Node->GetName().CString(), actor1Node->GetID(), GOT::GetType(got).CString(), data.argType_);

        if (avatar == actor1Node)
        {
            // TODO
            URHO3D_LOGWARNINGF("ObjectiveAction() - Execute : %s(%u) changes entity to %s(%u) ... TO IMPLEMENT",
                               actor1Node->GetName().CString(), actor1Node->GetID(), GOT::GetType(got).CString(), data.argType_);
        }
        else
        {
            // destroy actor1Node and replace by a new entity of type got
            actor1Node->SetEnabled(false);
            GOC_Destroyer* destroyer = actor1Node->GetComponent<GOC_Destroyer>();
            const WorldMapPosition& wposition = destroyer->GetWorldMapPosition();
            Node* newEntity = World2D::SpawnEntity(got, RandomEntityFlag|RandomMappingFlag, 0, 0, wposition.viewZ_, PhysicEntityInfo(wposition.position_.x_,wposition.position_.y_));
            World2D::DestroyEntity(wposition.mPoint_, actor1Node);
            actor1Node = newEntity;

            // if this action uses a varslot, update it with this new node id
            if (data.actor1Type_ == OBJECTIVEVARSLOT1)
            {
                actor1NodeId_ = actor1Node->GetID();
                var1 = actor1NodeId_;
            }
            else if (data.actor1Type_ == OBJECTIVEVARSLOT2)
            {
                actor2NodeId_ = actor1Node->GetID();
                var2 = actor2NodeId_;
            }

            // Particle spawn
            Drawable2D* drawable = actor1Node->GetDerivedComponent<Drawable2D>();
            GameHelpers::SpawnParticleEffect(GameContext::Get().context_, ParticuleEffect_[PE_LIFEFLAME], drawable->GetLayer(), drawable->GetViewMask(), wposition.position_, 0.f, 1.f, true, 1.f, Color::BLUE, LOCAL);
        }
    }
    break;
    case PlayAnimation:
    {
        URHO3D_LOGINFOF("ObjectiveAction() - Execute : Actor %s(%u) play animation %u...",
                        actor1Node->GetName().CString(), actor1Node->GetID(), data.argType_);
    }
    break;
    case ReceivesFollower:
    {
        GOC_AIController* aicontroller = actor2Node->GetComponent<GOC_AIController>();
        if (aicontroller)
        {
            URHO3D_LOGINFOF("ObjectiveAction() - Execute : Actor %s(%u) receives follower %s type=%u ...",
                            actor1Node->GetName().CString(), actor1Node->GetID(), GOT::GetType(argtype).CString(), data.argType_);

            aicontroller->SetControllerType(GO_AI_Ally, true);
            aicontroller->SetAlwaysUpdate(true);
            aicontroller->StartBehavior(GOB_FOLLOW, true);
            aicontroller->SetTarget(actor1NodeId_);

            aicontroller->GetNode()->ApplyAttributes();
            aicontroller->Dump();
        }
    }
    break;
    }
}

void Objective::Start(Mission* mission, unsigned owner)
{
    if (state_ > IsPaused)
        return;

    if (mission_ != mission)
        mission_ = mission;

    if (state_ == NotActived)
    {
        const Actor* actor = Actor::Get(owner);
        if (!actor)
            return;

        lastQty_ = (int) actor->GetStat(statCategories_[type_], StringHash(attribut_));

        elapsedQty_ = 0;
        elapsedTime_ = 0;

        if (actor->GetAvatar())
        {
            GOC_Collide2D *collider = actor->GetAvatar()->GetComponent<GOC_Collide2D>();
            if (collider)
                collider->ResetLastAttack();
        }
    }

    if (delay_)
        lastTime_ = Time::GetSystemTime();

    state_ = IsRunning;

    URHO3D_LOGINFOF("Objective() - Start : mission=%s(%u) state=%s(%d) attribut=%s(%u)",
                     mission_->GetMissionData() ? mission_->GetMissionData()->name_.CString() : "unamed", mission_, GoalStateNames_[state_], state_, GOT::GetType(StringHash(attribut_)).CString(), attribut_);
}

void Objective::Stop()
{
    if (state_ == IsRunning)
        state_ = IsPaused;
}

void Objective::Set(ObjectiveData* od, bool init)
{
    data_ = od;

    numConditions_ = od ? od->conditionDatas_.Size() : 0;
    numActions_ = od ? od->actionDatas_.Size() : 0;

    if (od && init)
    {
        type_              = od->type_;
        state_             = NotActived;
        isMajor_           = od->isMajor_;

        attribut_          = od->objectType_;
        quantity_          = od->objectQty_;

        delay_             = od->delay_;
        elapsedTime_       = 0;
        allowReachOutTime_ = true;
        SetActors(0);

        for (unsigned i=0; i < numConditions_; i++)
            conditions_[i].Clear();
        for (unsigned i=0; i < numActions_; i++)
            actions_[i].Clear();
    }
}

void Objective::SetActors(unsigned giver, unsigned receiver, unsigned enabler)
{
    enabler_ = receiver_ = 0;
    giver_ = giver;
    godMode_ = (giver_==0);

    if (enabler)
        enabler_ = enabler;

    if (receiver)
        receiver_ = receiver;
}

void Objective::Update(unsigned owner)
{
    if (state_ != IsRunning)
        return;

    /// Update Time
    if (delay_)
    {
        elapsedTime_ += time_ - lastTime_;
        lastTime_ = time_;

        if (!allowReachOutTime_)
        {
            if (elapsedTime_ > delay_)
                state_ = IsFailed;
        }
    }

    /// Check Completed
    if (!godMode_)
    {
        Actor* actor = Actor::Get(receiver_);
        if (!actor)
        {
            URHO3D_LOGWARNINGF("Objective() - Update : No Receiver Actor %u for this objective !", receiver_);
            state_ = IsFailed;
            return;
        }
    }

    Actor* actor = Actor::Get(owner);
    if (!actor)
        return;

    Node* actorNode = actor->GetAvatar();
    const String& objectiveTypeName = GOA::GetAttributeName(StringHash(type_));

    unsigned numcheckedconditions = 0;
    unsigned numsucceedconditions = 0;
    bool conditionsSuccess = false;

    if (data_)
    {
        /// Evaluate the commands type "conditions"
        for (unsigned i = 0; i < numConditions_; i++)
        {
            const ObjectiveCommandData &conditiondata = data_->conditionDatas_[i];
            ObjectiveCondition &condition = conditions_[i];

            numcheckedconditions++;

            if (!condition.Check(actor, mission_, conditiondata))
                break;

            numsucceedconditions++;
        }

        conditionsSuccess = (numcheckedconditions && numcheckedconditions == numsucceedconditions);
    }

    if (type_ == GOA::GGOAT_COLLECTOR.Value() || type_ == GOA::GGOAT_KILLER.Value())
    {
        int qty = (int)actor->GetStat(statCategories_[type_], StringHash(attribut_));
        updateQty_ = (qty - lastQty_) != 0;
        if (updateQty_)
        {
            elapsedQty_ += (qty - lastQty_); //(qty - lastQty_ > 0 ? qty - lastQty_ : 0);
            URHO3D_LOGINFOF("Objective() - Update : attribut=%s elapsedQty=%d/%d", GOT::GetType(StringHash(attribut_)).CString(), elapsedQty_, quantity_);
            if (elapsedQty_ <= 0)
                updateQty_ = false;
        }

        lastQty_ = qty;

        if (elapsedQty_ >= quantity_)
        {
            state_ = elapsedTime_ <= delay_ ? IsCompleted : IsCompletedOutTime;
            URHO3D_LOGINFOF("Objective() - Update : type=%s(%u) state=%s(%d) attribut=%s qty=%u/%u q=%u", objectiveTypeName.CString(), type_,
                            GoalStateNames_[state_], state_, GOT::GetType(StringHash(attribut_)).CString(), elapsedQty_, quantity_, qty);
        }
    }
    else if (type_ == GOA::GGOAT_CREATOR.Value())
    {
        /// use a defined recipe
        if (attribut_)
        {
            // find the recipe
            StringHash recipe = CraftRecipes::GetRecipe(StringHash(attribut_));
            if (!recipe.Value())
            {
                URHO3D_LOGINFOF("Objective() - Update : type=%s(%u) can't find a recipe !", objectiveTypeName.CString(), type_);
                return;
            }

            // get the elements of the recipe
            Vector<String> materials;
            Vector<String> tools;
            CraftRecipes::GetElementsForRecipe(recipe, materials, tools);

            if (!materials.Size() && !tools.Size())
            {
                URHO3D_LOGINFOF("Objective() - Update : type=%s(%u) no element for recipe=%u !", objectiveTypeName.CString(), type_, attribut_);
                return;
            }

            // check if materials are in the bag
            if (materials.Size())
            {
                unsigned numMaterials = 0;
                for (int i=0; i < materials.Size(); i++)
                {
                    int collected = (int)actor->GetStat(GOA::ACS_ITEMS_COLLECTED, StringHash(materials[i]));
                    int gived     = (int)actor->GetStat(GOA::ACS_ITEMS_GIVED, StringHash(materials[i]));

                    if (collected-gived > 0)
                        numMaterials++;
                }

                if (numMaterials < materials.Size())
                {
                    URHO3D_LOGINFOF("Objective() - Update : type=%s(%u) not enough materials !", objectiveTypeName.CString(), type_);
                    return;
                }
            }

            // check if tools are around the owner
            if (tools.Size())
            {
                unsigned numTools = 0;
                bool envSearch = false;
                PODVector<RigidBody2D*> results;

                for (int i=0; i < tools.Size(); i++)
                {
                    bool toolFound = false;
                    StringHash tool(tools[i]);
                    // Check for tools in inventory
                    {
                        // TODO
                    }

                    // Check for the external tools
                    if (!toolFound)
                    {
                        if (!envSearch)
                        {
                            Vector2 pos = actor->GetAvatar()->GetWorldPosition2D();
                            Rect aabb(pos - Vector2::ONE, pos + Vector2::ONE);
                            GameContext::Get().physicsWorld_->GetRigidBodies(results, aabb);
                            envSearch = true;
                        }

                        for (unsigned j=0; j < results.Size(); j++)
                        {
                            Node* node = results[j]->GetNode();

                            //                        URHO3D_LOGERRORF("Objective() - Update : type=%s(%u) searching tool=%s compare with finding node=%s !",
                            //                                         objectiveTypeName.CString(), type_, tools[i].CString(), node->GetName().CString());

                            if (tool.Value() == node->GetVar(GOA::GOT).GetUInt())
                            {
                                toolFound = true;
                                break;
                            }
                        }
                    }

                    if (toolFound)
                        numTools++;
                }

                if (numTools < tools.Size())
                {
                    URHO3D_LOGINFOF("Objective() - Update : type=%s(%u) not enough tools !", objectiveTypeName.CString(), type_);
                    return;
                }
            }

            state_ = IsCompleted;
        }
        /// use conditions
        else if (conditionsSuccess)
        {
            state_ = IsCompleted;
        }

        if (state_ == IsCompleted)
        {
            // remove the materials ?

            URHO3D_LOGINFOF("Objective() - Update : type=%s(%u) Is Completed !", objectiveTypeName.CString(), type_);
        }
    }
    else
    {
        URHO3D_LOGERRORF("Objective() - Update : type=%s(%u) NOT IMPLEMENTED !", objectiveTypeName.CString(), type_);
    }

    /// if Conditions are successful then execute commands type "actions"
    if (conditionsSuccess && state_ == IsCompleted)
    {
        for (unsigned i=0; i < numActions_; i++)
            actions_[i].Execute(actor, mission_, data_->actionDatas_[i]);
    }

    URHO3D_LOGDEBUGF("Objective() - Update : mission=%s(%u) state=%s(%d) attribut=%s qty=%u/%u numconds=%u",
                     mission_->GetMissionData() ? mission_->GetMissionData()->name_.CString() : "unamed", mission_->GetID(), GoalStateNames_[state_], 
                     state_, GOT::GetType(StringHash(attribut_)).CString(), elapsedQty_, quantity_, numConditions_);
}

/// Class Mission Implementation

bool Mission::Load(Deserializer& source)
{
    id_ = source.ReadUInt();
    objIndex_ = source.ReadUInt();
    state_ = source.ReadInt();
    sequenced_ = source.ReadBool();

    PODVector<unsigned char> buffer;
/// objectives
    buffer = source.ReadBuffer();
    objectives_ = PODVector<Objective>((const Objective*) &buffer[0], buffer.Size() / sizeof(Objective));
/// rewards
    buffer = source.ReadBuffer();
    rewards_ = PODVector<Reward>((const Reward*) &buffer[0], buffer.Size() / sizeof(Reward));
/// indextocomplete
    buffer = source.ReadBuffer();
    indexToComplete_ = PODVector<unsigned>((const unsigned*) &buffer[0], buffer.Size() / sizeof(unsigned));
    CheckCompleted(true);
/// objective vars
    buffer = source.ReadBuffer();
    objectiveVars_ = PODVector<unsigned>((const unsigned*) &buffer[0], buffer.Size() / sizeof(unsigned));

    return true;
}

bool Mission::LoadXML(XMLElement& element)
{
    objectives_.Clear();
    rewards_.Clear();

    id_         = element.GetUInt("ID");
    objIndex_   = element.GetUInt("objIndex");
    state_      = element.GetInt("State");
    sequenced_  = element.GetBool("Sequenced");

    MissionData* md = GameData::GetJournalData()->GetMissionData(StringHash(id_));
    if (md)
    {
        objectives_.Resize(md->objectives_.Size());
        rewards_.Resize(md->rewards_.Size());
    }

/// Objectives
    int i=0;
    for (XMLElement objElem = element.GetChild("objective"); objElem; objElem = objElem.GetNext("objective"), i++)
    {
        if (!md)
            objectives_.Resize(objectives_.Size()+1);

        Objective& o = md ? objectives_[i] : objectives_.Back();
        o.Clear();

        o.type_              = StringHash(objElem.GetAttribute("type")).Value();
        o.state_             = objElem.GetInt("state");
        o.isMajor_           = objElem.GetBool("major");
        o.godMode_           = objElem.GetBool("godmode");

        XMLElement attrElem  = objElem.GetChild("object");
        o.attribut_          = StringHash(attrElem.GetAttribute("type")).Value();
        o.quantity_          = attrElem.GetInt("qty");
        o.lastQty_           = attrElem.GetInt("lastqty");
        o.elapsedQty_        = attrElem.GetInt("elapsedqty");

        XMLElement delayElem = objElem.GetChild("time");
        o.delay_             = delayElem.GetUInt("delay");
        o.elapsedTime_       = delayElem.GetUInt("elapsedtime");
        o.allowReachOutTime_ = delayElem.GetBool("allowReachOut");

        XMLElement actorElem = objElem.GetChild("actors");
        o.giver_             = actorElem.GetUInt("giver");
        o.enabler_           = actorElem.GetUInt("enabler");
        o.receiver_          = actorElem.GetUInt("receiver");
    }

/// Rewards
    i=0;
    for (XMLElement rwdElem = element.GetChild("reward"); rwdElem; rwdElem = rwdElem.GetNext("reward"), i++)
    {
        if (!md)
            rewards_.Resize(rewards_.Size()+1);

        Reward& r = md ? rewards_[i] : rewards_.Back();

        r.rewardCategory_ = StringHash(rwdElem.GetAttribute("category")).Value();
        r.type_           = StringHash(rwdElem.GetAttribute("type")).Value();
        r.fromtype_       = StringHash(rwdElem.GetAttribute("fromtype")).Value();
        r.partindex_      = rwdElem.GetInt("partindex");
        r.quantity_       = rwdElem.GetInt("qty");
    }

/// Objective List To Complete
    indexToComplete_.Clear();
    for (XMLElement indElem = element.GetChild("tocompleted"); indElem; indElem = indElem.GetNext("tocompleted"))
        indexToComplete_.Push(indElem.GetUInt("objindex"));
    CheckCompleted(true);

/// Objectives Vars
    objectiveVars_.Clear();
    for (XMLElement var = element.GetChild("objvars"); var; var = var.GetNext("objvars"))
        objectiveVars_.Push(var.GetUInt("var"));

    return true;
}

bool Mission::Save(Serializer& dest)
{
    objectives_.Compact();
    rewards_.Compact();
    indexToComplete_.Compact();
    objectiveVars_.Compact();

    dest.WriteUInt(id_);
    dest.WriteUInt(objIndex_);
    dest.WriteInt(state_);
    dest.WriteBool(sequenced_);

    unsigned size,saved;
/// objectives
    size = objectives_.Size() * sizeof(Objective);
    dest.WriteVLE(size);
    saved = dest.Write(&objectives_[0], size);
    URHO3D_LOGINFOF("Mission() - Save : objective size=%u saved=%u", size, saved);
/// rewards
    size = rewards_.Size() * sizeof(Reward);
    dest.WriteVLE(size);
    saved = dest.Write(&rewards_[0], size);
    URHO3D_LOGINFOF("Mission() - Save : rewards size=%u saved=%u", size, saved);
/// indextocomplete
    size = indexToComplete_.Size() * sizeof(unsigned);
    dest.WriteVLE(size);
    saved = dest.Write(&indexToComplete_[0], size);
    URHO3D_LOGINFOF("Mission() - Save : indextoComplete size=%u saved=%u", size, saved);
/// objectiveVars
    size = objectiveVars_.Size() * sizeof(unsigned);
    dest.WriteVLE(size);
    saved = dest.Write(&objectiveVars_[0], size);
    URHO3D_LOGINFOF("Mission() - Save : objectiveVars_ size=%u saved=%u", size, saved);
    return true;
}

bool Mission::SaveXML(XMLElement& element)
{
    objectives_.Compact();
    rewards_.Compact();
    indexToComplete_.Compact();
    objectiveVars_.Compact();

    bool success = true;
    success &= element.SetUInt("ID", id_);
    success &= element.SetUInt("objIndex", objIndex_);
    success &= element.SetInt("State", state_);
    success &= element.SetBool("Sequenced", sequenced_);

/// Objectives
    for (PODVector<Objective>::ConstIterator it=objectives_.Begin(); it!=objectives_.End(); ++it)
    {
        const Objective& obj = *it;
        XMLElement objElem = element.CreateChild("objective");
        success &= objElem.SetAttribute("type", GOA::GetAttributeName(StringHash(obj.type_)));
        success &= objElem.SetInt("state", obj.state_);
        success &= objElem.SetBool("major", obj.isMajor_);
        success &= objElem.SetBool("godmode", obj.godMode_);

        XMLElement attrElem = objElem.CreateChild("object");
        success &= attrElem.SetAttribute("type", GOT::GetType(StringHash(obj.attribut_)));
        success &= attrElem.SetInt("qty", obj.quantity_);
        success &= attrElem.SetInt("lastqty", obj.lastQty_);
        success &= attrElem.SetInt("elapsedqty", obj.elapsedQty_);

        XMLElement delayElem = objElem.CreateChild("time");
        success &= delayElem.SetUInt("delay", obj.delay_);
        success &= delayElem.SetUInt("elapsedtime", obj.elapsedTime_);
        success &= delayElem.SetBool("allowReachOut", obj.allowReachOutTime_);

        XMLElement actorElem = objElem.CreateChild("actors");
        success &= actorElem.SetUInt("giver", obj.giver_);
        success &= actorElem.SetUInt("enabler", obj.enabler_);
        success &= actorElem.SetUInt("receiver", obj.receiver_);
    }

/// Rewards
    for (PODVector<Reward>::ConstIterator it=rewards_.Begin(); it!=rewards_.End(); ++it)
    {
        XMLElement rwdElem = element.CreateChild("reward");
        success &= rwdElem.SetAttribute("category", COT::GetName(StringHash(it->rewardCategory_)));
        success &= rwdElem.SetAttribute("type", GOT::GetType(StringHash(it->type_)));
        success &= rwdElem.SetAttribute("fromtype", GOT::GetType(StringHash(it->fromtype_)));
        success &= rwdElem.SetInt("partindex", it->partindex_);
        success &= rwdElem.SetInt("qty", it->quantity_);
    }

/// Objective List To Complete
    for (PODVector<unsigned>::ConstIterator it=indexToComplete_.Begin(); it!=indexToComplete_.End(); ++it)
        success &= element.CreateChild("tocompleted").SetUInt("objindex", *it);

/// Objective Vars
    for (PODVector<unsigned>::ConstIterator it=objectiveVars_.Begin(); it!=objectiveVars_.End(); ++it)
        success &= element.CreateChild("objvars").SetUInt("var", *it);

//    success &= element.CreateChild("ToComplete").SetBuffer("values", &indexToComplete_[0], indexToComplete_.Size() * sizeof(unsigned));

    return success;
}

void Mission::Set(MissionData* md, bool init)
{
    data_ = md;

    if (md)
    {
        if (init)
        {
            sequenced_ = md->sequenced_;

            objectives_.Resize(md->objectives_.Size());
            indexToComplete_.Resize(md->objectives_.Size());
        }

        for (unsigned i=0; i < objectives_.Size(); i++)
        {
            objectives_[i].Set(&md->objectives_[i], init);
            if (init)
                indexToComplete_[i] = i;
        }

        if (init)
        {
            objectiveVars_.Resize(2);
            objectiveVars_[0] = objectiveVars_[1] = 0;
            objIndex_ = sequenced_ ? 0 : objectives_.Size()-1;
            state_ = IsActived;
        }

        if (init && md->rewards_.Size())
            rewards_ = md->rewards_;
    }
}

void Mission::Start(unsigned owner)
{
    if (state_ == NotActived)
    {
        objIndex_ = sequenced_ ? 0 : objectives_.Size()-1;
        state_ = IsActived;
//        URHO3D_LOGINFOF("Mission() - Start : id=%u state_=%s(%d)", id_, GoalStateNames_[state_], state_);
    }
    else if (state_ > NotActived && state_ < IsRunning)
    {
        state_ = IsRunning;
//        URHO3D_LOGINFOF("Mission() - Start : id=%u state_=%s(%d)", id_, GoalStateNames_[state_], state_);
    }

    if (!sequenced_)
        for (unsigned i=0; i < objectives_.Size(); i++)
            objectives_[i].Start(this, owner);
    else
        objectives_[objIndex_].Start(this, owner);

    URHO3D_LOGINFOF("Mission() - Start : id=%u state_=%s(%d) ... OK !", id_, GoalStateNames_[state_], state_);
}

void Mission::Stop()
{
    if (state_ == NotActived)
        return;

    for (unsigned i=0; i < objectives_.Size(); i++)
        objectives_[i].Stop();

    state_ = (state_ == IsRunning ? IsPaused : IsActived);

    URHO3D_LOGINFOF("Mission() - Stop : id=%u state_=%s(%d)", id_, GoalStateNames_[state_], state_);
}

bool Mission::Update(unsigned owner, Object* manager)
{
    if (state_ < IsRunning)
        return false;

//    URHO3D_LOGINFOF("Mission() - Update : mission id=%u state_=%s(%d)", id_, GoalStateNames_[state_], state_);

    bool objectiveupdated = false;
    int laststate = state_;

    /// Update All Objectives (majors and no majors)
    for (unsigned i = 0; i <= objIndex_; i++)
    {
        Objective& obj = objectives_[i];

        if (obj.state_ == IsRunning)
        {
            int laststate = obj.state_;
            obj.Update(owner);

            if (laststate != obj.state_ && (obj.state_ > IsRunning || obj.updateQty_))
            {
                URHO3D_LOGINFOF("Mission() - Update : objective index=%u/%u state=%s(%d) attribut=%s qty=%u/%u", i, objIndex_, GoalStateNames_[obj.state_], obj.state_,
                                GOT::GetType(StringHash(obj.attribut_)).CString(), obj.elapsedQty_, obj.quantity_);

                VariantMap eventData;
                eventData[Go_ObjectiveUpdated::GO_MISSIONID] = id_;
                eventData[Go_ObjectiveUpdated::GO_OBJECTIVEID] = i;
                eventData[Go_ObjectiveUpdated::GO_STATE] = obj.state_;
                manager->SendEvent(data_ ? GO_NAMEDOBJECTIVEUPDATED : GO_OBJECTIVEUPDATED, eventData);

                objectiveupdated = true;

                if (obj.state_ > IsRunning && sequenced_)
                {
                    objIndex_++;

                    if (objIndex_ < objectives_.Size())
                    {
                        objectives_[objIndex_].Start(this, owner);
                        eventData[Go_ObjectiveUpdated::GO_MISSIONID] = id_;
                        eventData[Go_ObjectiveUpdated::GO_OBJECTIVEID] = objIndex_;
                        eventData[Go_ObjectiveUpdated::GO_STATE] = objectives_[objIndex_].state_;
                        manager->SendEvent(data_ ? GO_NAMEDOBJECTIVEUPDATED : GO_OBJECTIVEUPDATED, eventData);
                    }
                    else
                    {
                        objIndex_--;
                        state_ = IsCompleted;
                        URHO3D_LOGINFOF("Mission() - Update : objective id=%u completed", id_);
                        break;
                    }
                }
            }
        }
    }

    /// Check if all objectives are Completed, if one is Failed, the mission is Failed
    if (state_ == IsRunning)
    {
        CheckCompleted();
    }

    return (laststate != state_) || objectiveupdated;
}

void Mission::CheckCompleted(bool fullcheck)
{
    bool completed = true;
    int state;
    unsigned newsize = indexToComplete_.Size();

    for (unsigned i=0; i < indexToComplete_.Size(); i++)
    {
        state = objectives_[indexToComplete_[i]].state_;

        if (!fullcheck && state == IsFailed)
        {
            state_ = IsFailed;
            break;
        }

        completed = completed && (state == IsCompleted || state == IsCompletedOutTime || state == IsFailed);
        if (completed)
        {
            indexToComplete_[i] = M_MAX_UNSIGNED;
            newsize--;
        }
    }

    if (!newsize || completed || state_ == IsFailed)
    {
        indexToComplete_.Clear();
    }
    else if (newsize < indexToComplete_.Size())
    {
        // remove the completed indexes
        Sort(indexToComplete_.Begin(), indexToComplete_.End());
        indexToComplete_.Resize(newsize);
    }

    if (completed)
        state_ = IsCompleted;
}

void Mission::Dump() const
{
    URHO3D_LOGINFOF("MissionManager() - Dump : id=%u objind=%u state=%s(%d) seq=%u numobj=%u numrewards=%u", id_, objIndex_, GoalStateNames_[state_], state_,
                    sequenced_, objectives_.Size(), rewards_.Size());

    for (unsigned i=0; i < objectives_.Size(); i++)
    {
        const Objective& o = objectives_[i];
        URHO3D_LOGINFOF(" => Objective : id=%u type=%u attribut=%u state=%s(%d) qty=%u/%u", i, o.type_, o.attribut_, GoalStateNames_[o.state_], o.state_, o.elapsedQty_, o.quantity_);
    }
    for (unsigned i=0; i < rewards_.Size(); i++)
    {
        const Reward& r = rewards_[i];
        URHO3D_LOGINFOF(" => Reward : id=%u type=%u fromtype=%u partindex=%d qty=%u", i, r.type_, r.fromtype_, r.partindex_, r.quantity_);
    }
    for (unsigned i=0; i < indexToComplete_.Size(); i++)
        URHO3D_LOGINFOF(" => ToComplete : id=%u", indexToComplete_[i]);

    for (unsigned i=0; i < objectiveVars_.Size(); i++)
        URHO3D_LOGINFOF(" => Var[%u] : nodeid=%u", i, objectiveVars_[i]);
}


/// Class MissionManager Implementation

MissionManager::MissionManager(Context* context) :
    Object(context),
    timer_(0),
    owner_(0),
    started_(false),
    updateInterval_(DEFAULT_MISSIONUPDATE_INTERVAL),
    updateAcc_(0.0f)
{
    missions_.Clear();
    missions_.Reserve(100);
}

void MissionManager::RegisterObject(Context* context)
{
    context->RegisterFactory<MissionManager>();
}

void MissionManager::SetUpdateInterval(float delay)
{
    updateInterval_ = delay;
    updateAcc_ = 0.0f;
}

void MissionManager::Clear()
{
    Stop();

    missions_.Clear();
    namedmissions_.Clear();
    missionsToComplete_.Clear();
    timer_.Reset();
}

void MissionManager::UpdateLoadedMissions()
{
    /// Update Missions
    for (unsigned i=0; i < missions_.Size(); ++i)
    {
        Mission& m = missions_[i];
        /// update mission rewards sprites ptr
        for (unsigned j=0; j < m.rewards_.Size(); j++)
        {
            Reward& r = m.rewards_[j];
            r.sprite_ = GameHelpers::GetSpriteForType(StringHash(r.type_), StringHash(r.fromtype_), r.partindex_);
        }
        /// update mission to complete list
        if (m.state_ <= IsRunning)
            missionsToComplete_.Push(&(m));
    }

    /// Update Named missions (Quests)
    for (HashMap<StringHash, Mission>::Iterator it = namedmissions_.Begin(); it != namedmissions_.End(); ++it)
    {
        Mission& m = it->second_;
        m.Set(GameData::GetJournalData()->GetMissionData(it->first_), false);
        /// update mission to complete list
        if (m.state_ <= IsRunning)
            missionsToComplete_.Push(&(m));
    }

    /// Restart Missions Paused
    for (List<Mission*>::Iterator it = missionsToComplete_.Begin(); it != missionsToComplete_.End(); ++it)
    {
        Mission& mission = **it;
        if (mission.state_ == IsPaused)
        {
            mission.Start(owner_);

//            VariantMap& eventData = context_->GetEventDataMap();
//            eventData[Go_MissionUpdated::GO_MISSIONID] = mission.id_;
//            eventData[Go_MissionUpdated::GO_STATE] = mission.state_;
//            SendEvent(mission.data_ ? GO_NAMEDMISSIONUPDATED : GO_MISSIONUPDATED, eventData);
        }
    }
}

bool MissionManager::Load(Deserializer& source)
{
    if (source.ReadFileID() != "FBMI")
    {
        URHO3D_LOGERROR("MissionManager() - Load : " + source.GetName() + " is not a valid mission file");
        return false;
    }

    URHO3D_LOGINFO("MissionManager() - Load : Loading missions from " + source.GetName());

    Clear();

    /// Load missions
    unsigned numMissions = source.ReadVLE();
    for (unsigned i = 0; i < numMissions; ++i)
    {
        missions_.Push(Mission(context_));
        Mission& m = missions_.Back();
        m.Load(source);
    }

    UpdateLoadedMissions();

    Dump();

    return true;
}

bool MissionManager::LoadXML(Deserializer& source)
{
    SharedPtr<XMLFile> xml(new XMLFile(context_));
    if (!xml->Load(source))
        return false;

    URHO3D_LOGINFO("MissionManager() - LoadXML : Loading missions from " + source.GetName());

    Clear();

    XMLElement rootElem = xml->GetRoot("missions");

    /// Load missions
    XMLElement missions = rootElem.GetChild("missions");
    if (missions)
    {
        for (XMLElement missionElem = missions.GetChild("mission"); missionElem; missionElem = missionElem.GetNext("mission"))
        {
            missions_.Push(Mission(context_));
            Mission& m = missions_.Back();
            m.LoadXML(missionElem);
        }
    }

    /// Load Named missions (Quests)
    XMLElement quests = rootElem.GetChild("quests");
    if (quests)
    {
        for (XMLElement missionElem = quests.GetChild("mission"); missionElem; missionElem = missionElem.GetNext("mission"))
        {
            Mission& m = namedmissions_[StringHash(missionElem.GetUInt("ID"))];
            m.LoadXML(missionElem);
        }
    }

    UpdateLoadedMissions();

    Dump();

    return true;
}

bool MissionManager::Save(Serializer& dest)
{
    Dump();

    // Write ID first
    if (!dest.WriteFileID("FBMI"))
    {
        URHO3D_LOGERROR("MissionManager() - Save : Could not save missions, writing to stream failed");
        return false;
    }

    Deserializer* ptr = dynamic_cast<Deserializer*>(&dest);
    if (ptr)
        URHO3D_LOGINFO("MissionManager() - Save : Saving missions to " + ptr->GetName());

    /// Save missions
    dest.WriteVLE(missions_.Size());
    for (unsigned i = 0; i < missions_.Size(); i++)
    {
        if (!missions_[i].Save(dest))
            return false;
    }

    return true;
}

bool MissionManager::SaveXML(Serializer& dest, const String& indentation)
{
    Dump();

    SharedPtr<XMLFile> xml(new XMLFile(context_));

    XMLElement rootElem = xml->CreateRoot("missions");

    /// Save missions
    if (missions_.Size())
    {
        XMLElement missionsElem = rootElem.CreateChild("missions");
        for (unsigned i = 0; i < missions_.Size(); i++)
        {
            XMLElement missionElem = missionsElem.CreateChild("mission");
            if (!missions_[i].SaveXML(missionElem))
                return false;
        }
    }

    /// Save Named missions (Quests)
    if (namedmissions_.Size())
    {
        XMLElement questsElem = rootElem.CreateChild("quests");
        for (HashMap<StringHash, Mission>::Iterator it = namedmissions_.Begin(); it != namedmissions_.End(); ++it)
        {
            XMLElement missionElem = questsElem.CreateChild("mission");
            if (!it->second_.SaveXML(missionElem))
                return false;
        }
    }

    Deserializer* ptr = dynamic_cast<Deserializer*>(&dest);
    if (ptr)
        URHO3D_LOGINFO("MissionManager() - SaveXML : Saving missions to " + ptr->GetName());

    return (xml->Save(dest, indentation));
}

void MissionManager::GenerateNewMission()
{
    unsigned id = missions_.Size();

    missions_.Push(Mission(context_));
    Mission& mission = missions_[id];
    mission.id_ = id;

    MissionGenerator::GenerateMission(&mission);

    missionsToComplete_ += (&mission);

    EnableMission(id);

    VariantMap& data = context_->GetEventDataMap();
    data[Go_MissionUpdated::GO_MISSIONID] = id;
    data[Go_MissionUpdated::GO_STATE] = mission.state_;
    SendEvent(GO_MISSIONADDED, data);
}

void MissionManager::RemoveMission(unsigned id)
{
    StopMission(id);

    if (id >= missions_.Size())
        return;

    Mission& mission = missions_[id];

    mission.state_ = NotActived;

    List<Mission* >::Iterator it = missionsToComplete_.Find(&mission);
    if (it != missionsToComplete_.End())
    {
        missionsToComplete_.Erase(it);
        iCount_= missionsToComplete_.Begin();
    }

    missions_.Erase(id);
}

bool MissionManager::AddMission(const String& name)
{
    StringHash hashname(name);

    /// check if the mission is already in the missions
    if (namedmissions_.Find(hashname) != namedmissions_.End())
        return false;

    URHO3D_LOGINFOF("MissionManager() - AddMission : name=%s", name.CString());

    Mission& mission = namedmissions_[hashname];
    mission.id_ = hashname.Value();
    mission.Set(GameData::GetJournalData()->GetMissionData(hashname));

    VariantMap& data = context_->GetEventDataMap();
    data[Go_NamedMissionAdded::GO_MISSIONID] = mission.id_;
    SendEvent(GO_NAMEDMISSIONADDED, data);

    mission.Start(owner_);
    missionsToComplete_.Push(&mission);
    return true;
}

void MissionManager::EnableMission(unsigned id)
{
    if (id >= missions_.Size())
        return;

    Mission& mission = missions_[id];
    mission.state_ = NotActived;
    mission.Start(owner_);
    mission.Stop();
}

void MissionManager::StartMission(unsigned id)
{
    URHO3D_LOGINFOF("MissionManager() - StartMission : id=%u", id);

    if (id >= missions_.Size())
        return;

    missions_[id].Start(owner_);
}

void MissionManager::StopMission(unsigned id)
{
    URHO3D_LOGINFOF("MissionManager() - StopMission : id=%u", id);

    if (id >= missions_.Size())
        return;

    missions_[id].Stop();
}

void MissionManager::NextMission()
{
    URHO3D_LOGINFO("MissionManager() - NextMission !");

    if (!HasMissionsToComplete())
    {
        URHO3D_LOGINFO("MissionManager() - NextMission : no mission to complete => add a new one !");
        GenerateNewMission();
    }
    else
    {
        Mission* mission = GetLastMissionToComplete();

        URHO3D_LOGINFOF("MissionManager() - NextMission : mission to complete = %u !",  mission->id_);

        VariantMap& data = context_->GetEventDataMap();
        data[Go_MissionUpdated::GO_MISSIONID] = mission->id_;
        data[Go_MissionUpdated::GO_STATE] = mission->state_;
        SendEvent(GO_MISSIONADDED, data);
    }
}

void MissionManager::Start()
{
    URHO3D_LOGINFOF("MissionManager() - Start ...");

    if (!started_)
    {
        SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(MissionManager, HandlePostUpdate));

        for (List<Mission*>::Iterator it=missionsToComplete_.Begin(); it!=missionsToComplete_.End(); ++it)
        {
            Mission& mission = **it;
            if (mission.state_ == IsPaused)
                mission.Start(owner_);
        }

        NextMission();

        iCount_ = missionsToComplete_.Begin();

        started_ = true;
        URHO3D_LOGINFOF("MissionManager() - Start ... missionsToComplete_=%u OK !", missionsToComplete_.Size());
    }
    else
    {
        URHO3D_LOGINFO("MissionManager() - Start ... already started !");
    }
}

void MissionManager::Stop()
{
//    URHO3D_LOGINFO("MissionManager() - Stop ...");

    if (started_)
    {
        UnsubscribeFromEvent(E_POSTUPDATE);
        for (List<Mission*>::Iterator it=missionsToComplete_.Begin(); it!=missionsToComplete_.End(); ++it)
        {
            (*it)->Stop();
        }

        URHO3D_LOGINFOF("MissionManager() - Stop ... OK !");
    }
//    else
//    {
//        URHO3D_LOGINFO("MissionManager() - Stop ... already stopped !");
//    }

    started_ = false;
}

void MissionManager::SetMissionToState(Mission* mission, int state)
{
    if (mission)
    {
        URHO3D_LOGINFOF("MissionManager() - SetMissionToState : mission=%u state=%d",
                        mission, state);

        mission->SetState(state);
        UpdateMission(mission);
    }
}

void MissionManager::UpdateMission(Mission* mission)
{
    List<Mission*>::Iterator it=missionsToComplete_.Find(mission);

    if (it != missionsToComplete_.End())
    {
        URHO3D_LOGINFOF("MissionManager() - UpdateMission : mission=%u", mission);

        Mission& mission = *(*it);
        if (mission.Update(owner_, this))
        {
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Go_MissionUpdated::GO_MISSIONID] = mission.id_;
            eventData[Go_MissionUpdated::GO_STATE] = mission.state_;
            SendEvent(mission.data_ ? GO_NAMEDMISSIONUPDATED : GO_MISSIONUPDATED, eventData);
        }

        if (mission.state_ > IsRunning)
            it = missionsToComplete_.Erase(it);
    }
}

void MissionManager::Update()
{
//    URHO3D_LOGINFOF("MissionManager() - Update : ...");

    timer_.SetExpirationTime(MAXTIMEUPDATE);

    Objective::time_ = Time::GetSystemTime();

    if (iCount_ == missionsToComplete_.End())
        iCount_ = missionsToComplete_.Begin();

    List<Mission*>::Iterator it = iCount_;
    while (it != missionsToComplete_.End())
//    for (List<Mission*>::Iterator it=iCount_; it!=missionsToComplete_.End(); ++it)
    {
        Mission& mission = *(*it);
        if (mission.Update(owner_, this))
        {
            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Go_MissionUpdated::GO_MISSIONID] = mission.id_;
            eventData[Go_MissionUpdated::GO_STATE] = mission.state_;
            SendEvent(mission.data_ ? GO_NAMEDMISSIONUPDATED : GO_MISSIONUPDATED, eventData);
        }

        if (mission.state_ > IsRunning)
            it = missionsToComplete_.Erase(it);
        else
            it++;

        // check timer expiration of the pass update
        if (timer_.Expired())
        {
            iCount_ = it;
            return;
        }
    }

    iCount_ = missionsToComplete_.Begin();
}

void MissionManager::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!GameContext::Get().rootScene_->IsUpdateEnabled())
        return;

    URHO3D_PROFILE(MissionManager);

    float timeStep = eventData[PostUpdate::P_TIMESTEP].GetFloat();

    updateAcc_ += timeStep;
    if (updateAcc_ >= updateInterval_)
    {
        updateAcc_ = fmodf(updateAcc_, updateInterval_);
        Update();
    }
}

void MissionManager::OnMissionRemoved(StringHash eventType, VariantMap& eventData)
{
    RemoveMission(eventData[Go_MissionAdded::GO_MISSIONID].GetUInt());
}

void MissionManager::Dump() const
{
    // force log
//    GameHelpers::SetGameLogLevel(0);

    URHO3D_LOGINFOF("MissionManager() - Dump : NumMissions To Complete=%u/%u", missionsToComplete_.Size(), missions_.Size());

    for (List<Mission* >::ConstIterator it = missionsToComplete_.Begin(); it != missionsToComplete_.End(); ++it)
        (*it)->Dump();

    unsigned datasize = missions_.Size() * (MAX_OBJECTIVES * sizeof(Objective) + MAX_REWARDS * sizeof(Reward) + 2 * sizeof(unsigned) + sizeof(int) + sizeof(bool));
    URHO3D_LOGINFOF("MissionManager() - Dump : datasize=%u !", datasize);
}


/// Class MissionGenerator Implementation

PODVector<Objective> MissionGenerator::preObjectives_;

void MissionGenerator::InitTable()
{
    Objective::statCategories_[GOA::GGOAT_COLLECTOR.Value()] = GOA::ACS_ITEMS_COLLECTED;
    Objective::statCategories_[GOA::GGOAT_KILLER.Value()] = GOA::ACS_ENTITIES_KILLED;
    Objective::statCategories_[GOA::GGOAT_EXPLORER.Value()] = GOA::ACS_ACTOR_FOUND;
    Objective::statCategories_[GOA::GGOAT_PROTECTOR.Value()] = GOA::ACS_ACTOR_PROTECTED;
    Objective::statCategories_[GOA::GGOAT_CREATOR.Value()] = GOA::ACS_NODE_CREATED;

    preObjectives_.Resize(MAX_OBJECTIVES);
}

void MissionGenerator::GenerateMission(Mission* mission, unsigned giver, int level, int minRewards)
{
    int numObjectives = Random(1, Min(level+1, MAX_OBJECTIVES+1));
    int numMajors = Random(numObjectives+1);
    int numRewards = Random(minRewards, numMajors+1);

    URHO3D_LOGINFOF("MissionGenerator() - GenerateMission : PreObj numobj=%d, nummajors=%d, numrewards=%d", numObjectives, numMajors, numRewards);

    /// pre generate objectives

    int lastindex=0;
    {
        unsigned type;
        unsigned attribut;
        int quantity;
        int addto;
        for (int i=0; i<numObjectives; i++)
        {
            if (Random(1,100) < 70)
            {
                type = GOA::GGOAT_KILLER.Value();
                attribut = COT::GetRandomTypeFrom(COT::MONSTERS).Value();
                quantity = Random(1, level + numRewards);
            }
            else
            {
                type = GOA::GGOAT_COLLECTOR.Value();
                attribut = COT::GetRandomTypeFrom(COT::ITEMS).Value();
                quantity = Random(1, level + numRewards);

                if (attribut == 0)
                {
                    type = GOA::GGOAT_KILLER.Value();
                    attribut = COT::GetRandomTypeFrom(COT::MONSTERS).Value();
                    quantity = Random(1, level + numRewards);
                }
            }

            addto = lastindex;
            /// check if an objective is the same
            for (int j=0; j<lastindex; j++)
                if (preObjectives_[j].type_ == type && preObjectives_[j].attribut_ == attribut)
                {
                    addto = j;
                    break;
                }
            Objective& obj = preObjectives_[addto];
            if (addto == lastindex)
            {
                obj.state_ = 0;
                obj.type_= type;
                obj.attribut_ = attribut;
                obj.quantity_ = quantity;
                obj.SetActors(giver);
                obj.SetDelay(0);
                obj.numConditions_ = 0;
                obj.numActions_ = 0;
                lastindex++;
            }
            else
                obj.quantity_ += quantity;
        }
    }

    numObjectives = lastindex;
    numMajors = Min(numMajors, numObjectives);

    /// create mission

    mission->objectives_.Clear();
    mission->objectives_.Resize(numObjectives);
    mission->indexToComplete_.Clear();
    mission->indexToComplete_.Resize(numMajors ? numMajors : numObjectives);

    /// Set Sequenced mission

    mission->sequenced_ = Random(1,100) > 50;

    /// add Objectives

    int j = 0;
    int missionvalue=0;
    for (int i=0; i < numObjectives; i++)
    {
        Objective& obj = mission->objectives_[i];

        obj = preObjectives_[i];
        obj.isMajor_ = (i<numMajors);

        if (!numMajors || obj.isMajor_)
        {
            mission->indexToComplete_[j] = i;
            missionvalue += GOT::GetDefaultValue(StringHash(obj.attribut_)) * obj.quantity_;
            j++;
            URHO3D_LOGINFOF("MissionGenerator() - objective %d : type=%s qty=%d value=%d", i, GOT::GetType(StringHash(obj.attribut_)).CString(), obj.quantity_, GOT::GetDefaultValue(StringHash(obj.attribut_)) * obj.quantity_);
        }
    }

    /// Add Rewards

    mission->rewards_.Clear();
    bool addgold = false;
    int keepvalue = missionvalue;

    /// No Rewards if no mission Value
    if (missionvalue == 0)
        numRewards = 0;

//    /// TESTTTT : Skip Rewards
//    numRewards = 0;

    mission->rewards_.Resize(numRewards);

    if (numRewards)
    {
        StringHash category;
        StringHash type, fromtype;
        int partindex;
        unsigned maxDrop;

        Vector<unsigned> rewardTypeUsed;

        for (int i=0; i<numRewards; i++)
        {
            Reward& reward = mission->rewards_[i];

            /// Generate unique reward
            int itemvalue;
            do
            {
                category = Random(100) < 60 ? COT::MONEY : COT::ITEMS;
                type = StringHash::ZERO;
                while (type == StringHash::ZERO) type = COT::GetRandomTypeFrom(category);
                itemvalue = GOT::GetDefaultValue(type);
                if (itemvalue > keepvalue)
                    continue;
            }
            while (rewardTypeUsed.Contains(type.Value()));

            rewardTypeUsed.Push(type.Value());

            fromtype = GOT::GetBuildableType(type);
            partindex = GOT::GetPartIndexFor(fromtype, String("Random"));

            reward.rewardCategory_ = category.Value();
            reward.type_ = type.Value();
            reward.fromtype_ = fromtype.Value();
            reward.partindex_ = partindex;

            maxDrop = GOT::GetMaxDropQuantityFor(type);

            reward.quantity_ = Min(unsigned(1 + (keepvalue / (itemvalue * numRewards))), maxDrop);
            reward.sprite_ = GameHelpers::GetSpriteForType(type, fromtype, partindex);

            keepvalue -= itemvalue * reward.quantity_;
            URHO3D_LOGINFOF("MissionGenerator() - GenerateMission : Reward %u = type=%s(%u) from=%s qty=%u",
                            i, GOT::GetType(type).CString(), type.Value(), GOT::GetType(fromtype).CString(), reward.quantity_);
        }

        /// if not give enough, add a gold reward

        addgold = keepvalue > missionvalue/4;
        if (addgold)
        {
            numRewards++;
            mission->rewards_.Resize(numRewards);
            Reward& reward = mission->rewards_[numRewards-1];
            reward.rewardCategory_ = COT::MONEY.Value();
            reward.type_ = GOT::MONEY.Value();//COT::GetRandomTypeFrom(COT::MONEY).Value();
            reward.fromtype_ = 0;
            reward.partindex_ = 0;
            reward.quantity_ = Random(1 + keepvalue/2, keepvalue);
            reward.sprite_ = GameHelpers::GetSpriteForType(StringHash(reward.type_));

            keepvalue -= reward.quantity_;
        }
    }

    URHO3D_LOGINFOF("MissionGenerator() - GenerateMission : mission=%u numobj=%d, nummajors=%d, numrewards=%d, sequenced=%s, missionvalue=%d, keepvalue=%d, addgold=%s",
                    mission, numObjectives, numMajors, numRewards, mission->sequenced_ ? "true" : "false", missionvalue, keepvalue, addgold ? "true" : "false");
}


