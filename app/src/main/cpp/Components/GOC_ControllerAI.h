#pragma once

#include "DefsMove.h"

#include "GOC_Controller.h"


using namespace Urho3D;

const unsigned MAX_STACKCOMMAND = 64U;
const unsigned MAX_AIAGGRESSIVEDELAY = 15000U;
const unsigned MAX_AIWAITCALLBACKDELAY = 3000U;
const unsigned MIN_AIUPDATEDELAY = 300U;
const unsigned MAX_AIJUMPDELAY = 1U;

/// Low-Level AI  = Behaviors
/// High-Level AI = AIController

/// TODO : Behavior Instruction codes
/// ces instructions permettent de construire les behaviors en bytecode
/// définir le niveau de détail des instructions (macro ou atomique)
/// macro    = moins d'instructions => +rapide -flexible
/// atomique = plus d'instructions  => -rapide +flexible
enum Behavior_Instruction
{
    BEH_LITTERAL                 = 0x00,
    BEH_MOVE                     = 0x01,
    BEH_FACETO                   = 0x02,
    BEH_ATTACK                   = 0x03,
    BEH_WAIT                     = 0x04,
    BEH_TARGETLASTATTACKER       = 0x05,  // Selectionne le dernier attaquant comme cible
    BEH_TARGETALLY               = 0x06,  // Selectionne un allié (spécifier le critère)
    BEH_CHECKTARGETINCONTACT     = 0x07,  // Vérifie si la cible est à portée en combat rapproché
    BEH_CHECKTARGETINRANGE       = 0x08,  // Vérifie si la cible est à portée de tir
    BEH_CHANGEEQUIPMENT          = 0x09,  // Change d'equipement (arme etc...)
    BEH_CHANGEPOWER              = 0x0A,  // Change de pouvoir
    BEH_SETANIMSTATE             = 0x0B,
    BEH_GETANIMSTATE             = 0x0C,
    BEH_LINKTOTARGET             = 0x0D,  // S'attacher à la cible (spécifier le critère = position sur la target) (utilisable pour MountOn)
};

struct Behavior;

struct AInodeInfos
{
    AInodeInfos() : minRangeTarget(defaultMinWlkRangeTarget), maxRangeTarget(defaultMaxWlkRangeTarget),
        updateAggressivity(false), lastUpdate(0), lastState(0), state(0), buttons(0),
        lastOrder(0), order(0), nextOrder(0), callBackOrder(0),
        targetID(0), aggressiveDelay(0) { }

    void Reset()
    {
        updateNeed = 1;

        lastUpdate = lastState = state = buttons = 0;
        lastOrder = order = nextOrder = callBackOrder = 0;
        targetID = 0;
        waitCallBackOrderOfType = 0;
    }

    void SetState(unsigned s)
    {
        lastState = state;
        state = s;
    }

    WeakPtr<Node> target;
    WeakPtr<Node> lastParent;
    Vector2 lastTargetPosition;
    Vector2 minRangeTarget, maxRangeTarget;

    bool updateNeed, updateAggressivity;

    MoveTypeMode lastMoveType;

    unsigned lastUpdate, lastState, state;
    unsigned buttons, lastLayer, behavior;
    unsigned lastOrder, order, nextOrder, callBackOrder;
    unsigned targetID;
    unsigned aggressiveDelay;

    int waitCallBackOrderOfType;

    /// TODO : use a common stack for commands and command's values
//    int stackCommand_[MAX_STACKCOMMAND];
//    int stackCommandSize_;
};

class GOC_AIController : public GOC_Controller
{
    URHO3D_OBJECT(GOC_AIController, GOC_Controller);

public :
    GOC_AIController(Context* context);
    GOC_AIController(Context* context, int type);

    virtual ~GOC_AIController();
    static void RegisterObject(Context* context);

    void SetControllerTypeAttr(int type);
    void SetBehaviorAttr(const String& name);
    String GetBehaviorAttr() const;
    void SetBehaviorTargetAttr(const String& name);
    String GetBehaviorTargetAttr() const;
    void SetAlwaysUpdate(bool state)
    {
        alwaysUpdate_ = state;
    }
    void SetDetectTarget(bool enable, float radius=2.f);
    void SetTarget(unsigned targetID);
    void SetBehavior(unsigned b);

    bool IsStarted() const
    {
        return started_;
    }
    unsigned GetBehavior()
    {
        return behavior_;
    }
    Node* GetTarget()
    {
        return aiInfos_.target;
    }
    AInodeInfos& GetaiInfos()
    {
        return aiInfos_;
    }
    unsigned GetLastTimeUpdate() const
    {
        return lastTime_;
    }

    virtual void Start();
    virtual void Stop();
    Behavior* StartBehavior(unsigned b=0, bool forced=false);
    void StopBehavior();
    void ResetOrder();
    void ChangeOrder(unsigned order);

    void UpdateRangeValues(MoveTypeMode movetype);
    bool Update(unsigned time);

    void Dump() const;

private :

    void UpdateTargetDetection();
    void UpdateAggressivity(unsigned time);
    Behavior* ChooseBehavior();

    /// TODO : new stack system
//    void PushCommandStack(int value);
//    int PopCommandStack();
//    void ClearCommandStack();

    /// TODO : basic commands for behavior instructions
//    void Move(int direction, int speed) { }
//    void Attack() { }
//    void Wait(int time) { }

    void OnEntitySelection(StringHash eventType, VariantMap& eventData);
    void OnEvent(StringHash eventType, VariantMap& eventData); // need for AIManager CallBack
    void OnDead(StringHash eventType, VariantMap& eventData);
    void OnFollowedNodeDead(StringHash eventType, VariantMap& eventData);

    unsigned behavior_, lastBehavior_;
    unsigned behaviorDefault_, behaviorOnTarget_, behaviorOnTargetDead_;
    unsigned targetID_;
    unsigned lastTime_;

    bool started_;
    bool targetDetection_;
    bool alwaysUpdate_;

    AInodeInfos aiInfos_;
};
