#pragma once

#include <Urho3D/Urho3D.h>

#include "TimerSimple.h"



namespace Urho3D
{
class Serializer;
class Deserializer;
class File;
class Sprite2D;
}

using namespace Urho3D;


// exemple de mission propos�e au joueur :
//
// => objectif simple
// -> COLLECTOR : obtenir 5 cranes
// -> KILLER : tuer 10 chauve-souris, defaire le boss
// -> PROTECTOR : proteger tel actor contre les ennemis
// -> EXPLORER : trouver le marchand, trouver l'entr�e du donjon x
// -> CREATOR : creer un skeleton � partir de ces os
//
// => combinaisons d'objectif simple
// -> (EXPLORER & COLLECTOR) trouver le marchand et lui acheter x objets de tel type ou bien une liste quantitative d'objet
// -> (KILLER & COLLECTOR) tuer x monstres (KILLER : NUMTOREACH, TYPETOREACH)
//                         et rapporter y items (COLLECTOR : NUMTOREACH, TYPETOREACH) au donneur de missions (ATTR_GIVER), ou au PNJ z (ATTR_RECEIVER)
// -> (EXPLORER & COLLECTOR) : trouver l'entr�e d'une zone x (ATTR_LOCATION) et trouver l'objet y


class MissionManager;
class MissionGenerator;
class Mission;
class Actor;

enum GoalState// : int
{
    NotActived = 0,
    IsActived = 1,
    IsPaused,
    IsRunning,
    IsCompleted,
    IsCompletedOutTime,
    IsFailed,
    IsFinished,
};

enum GoalCondition
{
    EmptyCondition = 0,

    HasAttacked,
    HasItem,
};

enum GoalAction
{
    EmptyAction = 0,

    GivesItem,
    ChangesEntity,
    PlayAnimation,
    ReceivesFollower
};

struct ObjectiveCommandData
{
    int cmd_;
    unsigned actor1Type_;
    unsigned actor2Type_;
    unsigned argType_;
};

struct ObjectiveCommand
{
    void SetNodes(Node*& actor1Node, Node*& actor2Node, Node* actorNode, Mission* mission, const ObjectiveCommandData& data);
    void Clear();

    unsigned actor1NodeId_;
    unsigned actor2NodeId_;
};

struct ObjectiveCondition : public ObjectiveCommand
{
    bool Check(Actor* actor, Mission* mission, const ObjectiveCommandData& data);
};

struct ObjectiveAction : public ObjectiveCommand
{
    void Execute(Actor* actor, Mission* mission, const ObjectiveCommandData& data);
};

struct ObjectiveData
{
    unsigned type_;

    unsigned objectType_;
    int      objectQty_;

    unsigned actorGiver_;
    unsigned actorReceiver_;

    unsigned delay_;

    bool     isMajor_;

    HashMap<StringHash, String> texts_;
    PODVector<ObjectiveCommandData> conditionDatas_;
    PODVector<ObjectiveCommandData> actionDatas_;
};

class Objective
{
public :
    Objective()
    {
        Clear();
    }
    Objective(const Objective& obj) : mission_(obj.mission_), data_(obj.data_), state_(0), type_(obj.type_), attribut_(obj.attribut_),
        quantity_(obj.quantity_), lastQty_(obj.lastQty_), elapsedQty_(obj.lastQty_),
        delay_(obj.delay_), lastTime_(obj.lastTime_), elapsedTime_(obj.elapsedTime_),
        numConditions_(obj.numConditions_), numActions_(obj.numActions_),
        isMajor_(obj.isMajor_), godMode_(obj.isMajor_),
        updateQty_(obj.updateQty_), allowReachOutTime_(obj.allowReachOutTime_)
    {
        SetActors(obj.giver_);
        for (int i=0; i < obj.numConditions_; i++) conditions_[i] = obj.conditions_[i];
        for (int i=0; i < obj.numActions_; i++) actions_[i] = obj.actions_[i];
    }

    void Clear()
    {
        mission_ = 0;
        data_ = 0;
        state_ = 0;

        type_ = attribut_ = 0;
        quantity_ = lastQty_ = elapsedQty_ = 0;
        delay_ = lastTime_ = elapsedTime_ = 0;
        giver_ = enabler_ = receiver_ = 0;
        numConditions_ =  numActions_ = 0;

        isMajor_ = godMode_ = false;
        updateQty_ = allowReachOutTime_ = false;
    }

    void Set(ObjectiveData* od, bool init=true);
    void SetDelay(unsigned delay, bool allowReachOutTime = false)
    {
        delay_ = delay;
        allowReachOutTime_ = allowReachOutTime;
    }
    void SetActors(unsigned giver, unsigned receiver=0, unsigned enabler=0);

    void Start(Mission* mission, unsigned owner);
    void Stop();

    void Update(unsigned owner);

    Mission* mission_;
    ObjectiveData* data_;

    int state_;
    unsigned type_, attribut_;
    int quantity_, lastQty_, elapsedQty_;
    unsigned delay_, lastTime_, elapsedTime_;
    unsigned giver_, enabler_, receiver_;
    unsigned numConditions_, numActions_;

    bool isMajor_, godMode_;
    bool updateQty_, allowReachOutTime_;

    ObjectiveCondition conditions_[5];
    ObjectiveAction actions_[5];

    static unsigned time_;
    static HashMap<unsigned, StringHash> statCategories_;
};

class Reward
{
public :
    Reward() : type_(0), fromtype_(0), quantity_(0) { ; }
    Reward(const Reward& r) : rewardCategory_(r.rewardCategory_), type_(r.type_), fromtype_(r.fromtype_),
        partindex_(r.partindex_), quantity_(r.quantity_), sprite_(r.sprite_)    { ; }

    bool operator != (const Reward& r) const
    {
        return type_ != r.type_ || fromtype_ != r.fromtype_ || partindex_ != r.partindex_;
    }

    unsigned rewardCategory_; // Money, Item ...
    unsigned type_;
    unsigned fromtype_;
    int partindex_;
    int quantity_;

    Sprite2D* sprite_;
};

#define MAX_OBJECTIVES 5
#define MAX_REWARDS 5
#define MAX_OBJECTIVEVARSLOTS 2

struct MissionData
{
    int      id_;

    bool     sequenced_;

    String name_;
    String title_;

    HashMap<StringHash, String> texts_;

    Vector<ObjectiveData> objectives_;
    PODVector<Reward>        rewards_;
};

class Mission
{
public :
    Mission() { }
    Mission(Context* context) : id_(0), objIndex_(0), state_(0), data_(0), sequenced_(0) { }
    Mission(const Mission& m) : id_(m.id_), objIndex_(m.objIndex_), state_(m.state_), data_(m.data_),
        sequenced_(m.sequenced_), objectives_(m.objectives_), rewards_(m.rewards_),
        indexToComplete_(m.indexToComplete_), objectiveVars_(m.objectiveVars_) { }

    Mission& operator = (const Mission& m)
    {
        this->id_              = m.id_;
        this->objIndex_        = m.objIndex_;
        this->state_           = m.state_;
        this->data_            = m.data_;
        this->sequenced_       = m.sequenced_;
        this->objectives_      = m.objectives_;
        this->rewards_         = m.rewards_;
        this->indexToComplete_ = m.indexToComplete_;
        this->objectiveVars_   = m.objectiveVars_;
        return *this;
    }

    bool Load(Deserializer& source);
    bool Save(Serializer& dest);
    bool LoadXML(XMLElement& element);
    bool SaveXML(XMLElement& element);

    void Set(MissionData* md, bool init=true);
    void SetID(unsigned id)
    {
        id_ = id;
    }
    void SetObjIndex(unsigned objIndex)
    {
        objIndex_ = objIndex;
    }
    void SetState(int state)
    {
        state_ = state;
    }
    void SetSequenced(bool sequenced)
    {
        sequenced_ = sequenced;
    }

    unsigned GetID() const
    {
        return id_;
    }
    unsigned GetObjIndex() const
    {
        return objIndex_;
    }
    int GetState() const
    {
        return state_;
    }
    bool GetSequenced() const
    {
        return sequenced_;
    }
    MissionData* GetMissionData() const
    {
        return data_;
    }

    const PODVector<Objective>& GetObjectives() const
    {
        return objectives_;
    }
    const PODVector<Reward>& GetRewards() const
    {
        return rewards_;
    }
    PODVector<unsigned>& GetObjectiveVars()
    {
        return objectiveVars_;
    }
//    template <class T, class U> PODVector<U> copyPOD(const PODVector<T>& src) const
//    {
//        unsigned size = src.Size() / sizeof(U) + (src.Size() % sizeof(U) ? 1 : 0);
//
////        LOGINFOF("size of original buffer=%u ; sizeof(element class from)=%u ; num elt=%u ; remain=%s", src.Size(), sizeof(U) , size, src.Size() % sizeof(U) ? "true" : "false");
//
//        const U* buffer = reinterpret_cast<const U*>(&(src[0]));
//        return PODVector<U>(buffer, size);
//    }

    void Start(unsigned owner);
    void Stop();
    bool Update(unsigned owner, Object* manager);
    void CheckCompleted(bool fullcheck=false);

    void Dump() const;

protected :
    friend class MissionGenerator;
    friend class MissionManager;

    unsigned id_;
    unsigned objIndex_;
    int state_;

    MissionData* data_;

    bool sequenced_;

    PODVector<Objective> objectives_;
    PODVector<Reward> rewards_;
    PODVector<unsigned> indexToComplete_;
    PODVector<unsigned> objectiveVars_;
};

class MissionManager : public Object
{
    URHO3D_OBJECT(MissionManager, Object);

public :
    MissionManager(Context* context);

    static void RegisterObject(Context* context);

    void SetUpdateInterval(float delay);
    void SetOwner(unsigned owner)
    {
        owner_ = owner;
    }
    unsigned GetOwner() const
    {
        return owner_;
    }

    void Clear();

    bool IsStarted() const
    {
        return started_;
    }

    bool Load(Deserializer& source);
    bool Save(Serializer& dest);
    bool LoadXML(Deserializer& source);
    bool SaveXML(Serializer& dest, const String& indentation = "\t");

    Mission* GetMission(unsigned id)
    {
        return (id < missions_.Size() ? &(missions_[id]) : 0);
    }
    Mission* GetNamedMission(unsigned id) const
    {
        HashMap<StringHash, Mission>::ConstIterator it = namedmissions_.Find(StringHash(id));
        return it != namedmissions_.End() ? (Mission*)(&it->second_) : 0;
    }
    const HashMap<StringHash, Mission>& GetNamedMissions() const
    {
        return namedmissions_;
    }
    Mission* GetFirstMissionToComplete() const
    {
        return HasMissionsToComplete() ? missionsToComplete_.Front() : 0;
    }
    Mission* GetLastMissionToComplete() const
    {
        return HasMissionsToComplete() ? missionsToComplete_.Back() : 0;
    }

    void SetMissionToState(Mission* mission, int state);

    void StartMission(unsigned id);
    void StopMission(unsigned id);

    void NextMission();
    void EnableMission(unsigned id);
    void RemoveMission(unsigned id);

    bool AddMission(const String& name);

    bool HasMissionsToComplete() const
    {
        return (missionsToComplete_.Size() > 0);
    }

    void Start();
    void Stop();

    void Dump() const;

private :

    void UpdateMission(Mission* mission);
    void Update();

    void UpdateLoadedMissions();

    void GenerateNewMission();

    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);
    void OnMissionRemoved(StringHash eventType, VariantMap& eventData);

    Vector<Mission> missions_;
    HashMap<StringHash, Mission> namedmissions_;

    List<Mission* > missionsToComplete_;
    List<Mission* >::Iterator iCount_;
    TimerSimple timer_;

    bool started_;
    unsigned owner_;
    /// Update time interval.
    float updateInterval_;
    /// Update time accumulator.
    float updateAcc_;
};

class MissionGenerator
{
public :
    static void InitTable();
    static void GenerateMission(Mission* mission, unsigned giver=0, int level=4, int minRewards=1);

private :
    static PODVector<Objective> preObjectives_;
};



