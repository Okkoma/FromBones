#pragma once

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Urho2D/SpriterData2D.h>

//#include "DefsNetwork.h"

namespace Urho3D
{
class AnimatedSprite2D;
class AnimationSet2D;
}

using namespace Urho3D;

class GOC_Move2D;
class GOC_Controller;
class GOC_Abilities;
class GOC_Animator2D;

//#include "TimerRemover.h"

#include "GameAttributes.h"
#include "GameHelpers.h"
#include "GameEvents.h"

struct GOC_Animator2D_Template;

enum EventSenderType
{
    Sender_Self = 0,
    Sender_Owner,
    Sender_All,
    NumEventSenderType
};
struct AnimInfo
{
    AnimInfo() : animIndex(0), animLength(0.f) { }
    AnimInfo(unsigned i, const String& name, float length) : animIndex(i), animName(name), animLength(length) {  }
    AnimInfo(const AnimInfo& props) : animIndex(props.animIndex), animName(props.animName), animLength(props.animLength) {  }

    unsigned animIndex;
    String animName;
    float animLength;
};

typedef Urho3D::Vector<AnimInfo> AnimInfoTable;

typedef void (GOC_Animator2D::*FuncPtr)(const VariantMap& param);

struct Action
{
    Action() : name(String::EMPTY), ptr(0) { ; }
    Action(const String& n, FuncPtr pt, const VariantMap& pa = Variant::emptyVariantMap) : name(n), ptr(pt), param(pa) { ; }
    Action(const Action &a) : name(a.name), ptr(a.ptr), param(a.param) { ; }
    String ToString() const;

    String name;
    FuncPtr ptr;
    VariantMap param;

    static const Action EMPTY;
};
typedef Urho3D::Vector<Action> Actions;

struct Transition
{
    Transition() : nextState(0) { ; }
    Transition(int sender, StringHash e, StringHash cond, StringHash condVal, unsigned nextstate, const Actions& a) : eventSender(sender), event(e), condition(cond), conditionValue(condVal), nextState(nextstate), actions(a) { ; }
    Transition(const Transition& t) : eventSender(t.eventSender), event(t.event), condition(t.condition), conditionValue(t.conditionValue), nextState(t.nextState), actions(t.actions) { ; }

    void Dump() const;

    int eventSender;
    StringHash event;
    StringHash condition;
    StringHash conditionValue;
    unsigned nextState;
    Actions actions;
};

struct AnimatorState
{
    AnimatorState() : directionalAnimations_(false) { }
    AnimatorState(const String& n, float t, const String& keyWords, bool directionalAnimations=false, const String& actions=String::EMPTY) :
        name(n),
        animStateKeywords(keyWords),
        hashName(StringHash(n)),
        directionalAnimations_(directionalAnimations),
        tickDelay(t)
    {
        AddActions(actions);
    }
    AnimatorState(const AnimatorState& c) :
        name(c.name),
        animStateKeywords(c.animStateKeywords),
        hashName(c.hashName),
        event(c.event),
        directionalAnimations_(c.directionalAnimations_),
        tickDelay(c.tickDelay),
        actions(c.actions),
        hashEventTable(c.hashEventTable),
        eventTable(c.eventTable) { }

    void AddActions(const String& actions);
    void AddTransition(const String& event, int sender, StringHash cond, StringHash condvalue, unsigned nextState, const String& actions);
    void RemoveTransition(const String& event, int sender);
    void FindTransitionStates();

    bool HasEvent(const String& event, int sender) const;
    int GetEvent(const String& eventname, int sender) const;
    int GetEvent(StringHash hashEventName, int sender) const;
    bool CanTransitToState(unsigned nextState) const;

    String name;
    String animStateKeywords;

    StringHash hashName;

    StringHash event;

    bool directionalAnimations_;

    float tickDelay;

    Vector<StringHash> hashEventTable;
    Vector<Transition> eventTable;
    Vector<unsigned> nextStateTable;
    Vector<Action> actions;

    GOC_Animator2D_Template* template_;
};

struct GOC_Animator2D_Template
{
    GOC_Animator2D_Template() { }
    GOC_Animator2D_Template(String n, const GOC_Animator2D_Template& c)
        :  name(n),
           hashName(StringHash(n)),
           baseTemplate(c.hashName),
           transitionTable(c.transitionTable),
           hashStateTable(c.hashStateTable) { }

    static void RegisterTemplate(const String& s, const GOC_Animator2D_Template& t);

    static const Action& GetAction(const StringHash& actionhash)
    {
        HashMap<StringHash, Action>::ConstIterator it = actions_.Find(actionhash);
        return it != actions_.End() ? it->second_ : Action::EMPTY;
    }
    static void DumpAll();

    void AddState(const String& state, const String& AnimStateKeywords=String::EMPTY, bool directionalAnimations=false, float tickDelay=0.f, const String& actionnames=String::EMPTY);
//    void RemoveState(StringHash hashname);
    void ApplyEventToStates(const String& eventName, int sender, const String& listStates, const String& nextStateName, const String& condition, const String& actionnames=String::EMPTY);
    void AddAnimationSet(AnimationSet2D* animSet2D);
    void Bake();

    const Vector<AnimInfoTable>& GetAnimInfoTable(const StringHash& animationSetHash, int entityid)
    {
        return animInfoTables_[animationSetHash][entityid];
    }
    unsigned GetStateIndex(const StringHash& hashname) const;
    int GetSignedStateIndex(const StringHash& hashname) const;
    unsigned GetStateIndex(unsigned hashvalue) const;
    int GetSignedStateIndex(unsigned hashvalue) const;
    AnimatorState* GetState(const StringHash& hashname) const;
    AnimatorState* GetState(unsigned index) const;
    AnimatorState* GetStateFromEvent(const StringHash& hashevent) const;

    void Dump() const;
    void DumpAnimationInfoTable() const;

    String name;
    StringHash hashName;
    StringHash baseTemplate;

    Vector<StringHash> hashStateTable;
    Vector<AnimatorState> transitionTable;

    // indexed by AnimationSet, EntityId, State Indexes
    HashMap<StringHash, Vector<Vector<AnimInfoTable> > > animInfoTables_;

    static HashMap<unsigned, GOC_Animator2D_Template> templates_;
    static HashMap<StringHash, Action> actions_;
};

class GOC_Animator2D : public Component
{
    URHO3D_OBJECT(GOC_Animator2D, Component);

public:
    GOC_Animator2D(Context* context);
    virtual ~GOC_Animator2D();

    static void RegisterObject(Context* context);

    /// Template Setters
    void DefineTemplateOn(const String& s);
    void RegisterTemplate(const String& s);
    void SetTemplateName(const String& s);
    void SetStateAttr(const String& value);
    void SetEventAttr(const String& value);

    /// Component Setters
    void SetAutoSwitchAnimations(bool state);
    void SetDefaultOrientationAttr(float o);
    void SetDirection(const Vector2& v);
    void SetDirectionSilent(const Vector2& dir);
    void SetShootTarget(const Vector2& wpoint);
    void SetState(const String& state);
    bool SetState(const StringHash& state);
//        void SetStateSilent(const StringHash& state);
    void SetNetState(const StringHash& state, unsigned animindex, bool forcechange);
    void SetEventActions(const String& eventactions);
    void SetOwner(Node* node);

    void MoveLayer(int layerinc, bool alldrawables=true);

    void UpdateEntity();
    bool UpdateDirection();

    /// Template Setters
    const String& GetTemplateAttr() const
    {
        return String::EMPTY;
    }
    const String& GetStateAttr() const
    {
        return String::EMPTY;
    }
    const String& GetEventAttr() const
    {
        return String::EMPTY;
    }

    /// Component Getters
    float GetDefaultOrientationAttr() const
    {
        return orientation;
    }
    const String& GetTemplateName() const
    {
        return currentTemplate ? currentTemplate->name : String::EMPTY;
    }
    bool GetAutoSwitchAnimations() const
    {
        return autoSwitchAnimation_;
    }

    const String& GetState() const
    {
        return currentState ? currentState->name : String::EMPTY;
    }
    const StringHash& GetStateValue() const
    {
        return currentState ? currentState->hashName : state_;
    }
    const String& GetEventActions() const
    {
        return eventActionStr_;
    };

    const AnimInfoTable& GetCurrentAnimInfoTable() const
    {
        return currentAnimInfoTable->At(currentStateIndex);
    }
    const AnimInfo& GetCurrentAnimInfo(int version) const
    {
        return currentAnimInfoTable->At(currentStateIndex)[version];
    }
    const AnimInfo& GetAnimInfo(unsigned state) const
    {
        return currentAnimInfoTable->At(currentTemplate->GetStateIndex(state))[0];
    }
    const AnimInfo& GetAnimInfo(const StringHash& state) const
    {
        return currentAnimInfoTable->At(currentTemplate->GetStateIndex(state))[0];
    }
    bool HasAnimationForState(unsigned state) const;
    int GetCurrentStateIndex() const
    {
        return currentStateIndex;
    }
    int GetCurrentAnimVersion() const
    {
        return animInfoIndexes_[currentStateIndex];
    }
    int GetCurrentAnimIndex() const;

    AnimatedSprite2D* GetAnimated() const
    {
        return animatedSprite.Get();
    }
    const PODVector<AnimatedSprite2D*>& GetDrawables() const
    {
        return animatedSprites;
    }

    Spriter::Entity* GetEntity() const;

    const Vector2& GetDirection() const
    {
        return direction;
    }

    unsigned GetNetChangeCounter() const
    {
        return netChangeCounter_;
    }

    void Start();
    void Stop();
    void ResetState();

    bool PlugDrawables();
    void CheckAttributes();
    virtual void ApplyAttributes();

    virtual void OnSetEnabled();

    /// Actions
    inline void FindNextState(const VariantMap& param=Variant::emptyVariantMap);
    void ChangeEntity(const VariantMap& param=Variant::emptyVariantMap);
    void ChangeAnimation(const VariantMap& param=Variant::emptyVariantMap);
    inline void CheckTimer(const VariantMap& param=Variant::emptyVariantMap);
    inline void CheckAnim(const VariantMap& param=Variant::emptyVariantMap);
    inline void CheckEmpty(const VariantMap& param);
    void ToDisappear(const VariantMap& param=Variant::emptyVariantMap);
    void ToDestroy(const VariantMap& param=Variant::emptyVariantMap);
    inline void SendAnimEvent(const VariantMap& param);
    void SpawnParticule(const VariantMap& param);
    void SpawnEntity(const VariantMap& param);
    void SpawnAnimation(const VariantMap& param);
    void SpawnFurniture(const VariantMap& param);
//        void SpawnAnimationInside(const VariantMap& param);
    void LightOn(const VariantMap& param);
    void LightOff(const VariantMap& param);
    void CheckFireLight(const VariantMap& param=Variant::emptyVariantMap);

    void DumpTemplate() const;

protected :
    virtual void OnNodeSet(Node* node);

private :
    /// Handler
    void OnComponentChanged(StringHash eventType, VariantMap& eventData);
    void OnEvent(StringHash eventType, VariantMap& eventData);
    void OnEventActions(StringHash eventType, VariantMap& eventData);
    void OnUpdate(StringHash eventType, VariantMap& eventData);
    void OnChangeDirection(StringHash eventType, VariantMap& eventData);
    void OnUpdateDirection(StringHash eventType, VariantMap& eventData);
    void OnChangeState(StringHash eventType, VariantMap& eventData);

    void OnAnimation2D_Particule(StringHash eventType, VariantMap& eventData);
    void OnAnimation2D_Light(StringHash eventType, VariantMap& eventData);
    void OnAnimation2D_Entity(StringHash eventType, VariantMap& eventData);
    void OnAnimation2D_Animation(StringHash eventType, VariantMap& eventData);
    void OnTouchGround(StringHash eventType, VariantMap& eventData);
    void OnTouchFluid(StringHash eventType, VariantMap& eventData);

    /// Setters
    void SetTemplate();
    void ClearCustomTemplate();
    void SetAnimationSet();

    /// Updater
    void CheckAnimation();
    void UpdateSubscriber();
    inline void UpdateSubscriber(const AnimatorState* originState, const AnimatorState* newState);

    inline void ApplyNewStateAction();
    inline void ApplyCurrentStateEventActions(int eventindex, const VariantMap& param=Variant::emptyVariantMap);
    inline void ApplySwitchableAnimations();
    inline void ApplyDirectionalAnimations();
    inline void ApplySimpleAnimations();
    inline void NotifyCurrentState();

    bool SucceedTransition(AnimatorState* state, unsigned eventIndex) const;

    inline void PlayLoop();
    inline void Dispatch(const StringHash& event, const VariantMap& param=Variant::emptyVariantMap);
    inline void Update();

    /// Variables
    String templateName_;
    String eventActionStr_;

    Vector2 direction;
    Vector2 wallFlipping_;
    Vector2 shootTarget_;
    float deltaYWithOwner_;
    float roofFlipping_;
    float orientation;
    float timeStep;
    float currentStateTime;
    float delayParticule;
    float spawnAngle_;

    GOC_Animator2D_Template* currentTemplate;
    AnimatorState* currentState;
    AnimatorState* nextState;
    AnimatorState* forceNextState;
    WeakPtr<AnimatedSprite2D> animatedSprite;
    PODVector<AnimatedSprite2D*> animatedSprites;
    WeakPtr<GOC_Controller> controller_;
    WeakPtr<Node> owner_;
    WeakPtr<GOC_Abilities> abilities_;
    GOC_Move2D* gocmove_;

    bool customTemplate;
    bool autoSwitchAnimation_;
    bool subscribeOk_;
    bool lastOnGround;
    bool lastInTheAir;
    bool physicFlipX_;
    bool followOwnerY_;

    StringHash state_;
    StringHash lastEvent;
    StringHash currentEvent;
    StringHash currentAnimSet;

    unsigned lastOnMove;
    unsigned lastAction;
    int alignment_;
    unsigned currentStateIndex;
    int forceAnimVersion_;
    unsigned netChangeCounter_;
    int toDisappearCounter_;

    // animation Info Indexes for currentStateIndex
    Vector<int> animInfoIndexes_;
    const Vector<AnimInfoTable>* currentAnimInfoTable;

    Vector<HashMap<StringHash, Actions> > eventActions_;

    static unsigned sFirstSpecificStateIndex_;
};



