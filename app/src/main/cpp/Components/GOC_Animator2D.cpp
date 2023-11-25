#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Light.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/SequencedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "MAN_Weather.h"

#include "GameOptions.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameNetwork.h"

#include "GOC_Abilities.h"
#include "CommonComponents.h"

#include "MapWorld.h"
#include "ViewManager.h"
#include "ObjectPool.h"
#include "TimerRemover.h"

#include "GOC_Animator2D.h"

//#define LOGDEBUG_ANIMATOR2D

#define STATE_FINDER -1

enum AnimLoopType
{
    AL_Start,
    AL_End,
    AL_Tick,
    AL_Instant,
    NumAnimLoopTypes
};

const char* animLoopStrings_[] =
{
    "Start",
    "End",
    "Tick",
    "Instant",
    0
};

const char* EventSenderTypeNames_[] =
{
    "Self",
    "Owner",
    "All",
    0
};
const Action Action::EMPTY = Action(String::EMPTY, 0);

HashMap<unsigned, GOC_Animator2D_Template> GOC_Animator2D_Template::templates_;
HashMap<StringHash, Action> GOC_Animator2D_Template::actions_;

const StringHash ANIMATOR_TEMPLATE_EMPTY    = StringHash("AnimatorTemplate_Empty");
const StringHash ANIMATOR_TEMPLATE_DEFAULT  = StringHash("AnimatorTemplate_Default");

const StringHash AEVENT_STARTLOOP   = StringHash("AEvent_StartLoop");
const StringHash AEVENT_TICKLOOP    = StringHash("AEvent_TickLoop");
const StringHash AEVENT_ENDLOOP     = StringHash("AEvent_EndLoop");

String GetActionsString(const Transition& t)
{
    String actionsStr;
    for (unsigned k=0; k < t.actions.Size(); k++)
    {
        String paramStr;
        const VariantMap& params = t.actions[k].param;
        if (params.Size())
        {
            paramStr = "(";
            int ii = 0;
            for (VariantMap::ConstIterator it = params.Begin(); it != params.End(); ++it, ii++)
            {
                if (it->first_ == ANIM_Event::DATAS)
                    paramStr += "data=" + it->second_.ToString();
                else
                    paramStr += GOA::GetAttributeName(it->first_) + "=" + it->second_.ToString();

                if (ii < params.Size()-1)
                    paramStr += "|";
            }
            paramStr += ")";
        }
        actionsStr += t.actions[k].name + (paramStr.Empty() ? "" : paramStr);
        if (k < t.actions.Size()-1)
            actionsStr += ";";
    }
    return actionsStr;
}

void GetActionParams(const String& str, Action& action)
{
    Vector<String> actionSplitStr = str.Split('(');

    if (actionSplitStr.Size() > 1)
    {
        action.name = actionSplitStr[0].Trimmed();

        String paramStr = actionSplitStr[1].Substring(0, actionSplitStr[1].Find(")"));

        Vector<String> params = paramStr.Split(',');

        if (!params.Size())
            return;

        VariantMap& paramMap = action.param;

        for (unsigned i=0; i < params.Size(); ++i)
        {
            /// Get Param Type = param[0]
            Vector<String> param = params[i].Split('=');
            if (param.Size() > 1)
            {
                if (param[0] == "hash")
                    paramMap[ANIM_Event::DATAS] = StringHash(param[1]);
                else if (param[0] == "float")
                    paramMap[ANIM_Event::DATAS] = ToFloat(param[1]);
                else if (param[0] == "int")
                    paramMap[ANIM_Event::DATAS] = ToInt(param[1]);
                else if (param[1] == "true")
                    paramMap[StringHash(param[0])] = true;
                else if (param[1] == "false")
                    paramMap[StringHash(param[0])] = false;
            }

            else
            {
                /// Param Type = Bool
                if (param[0] == "true")
                    paramMap[ANIM_Event::DATAS] = true;
                else if (param[0] == "false")
                    paramMap[ANIM_Event::DATAS] = false;
                /// Default Type = String
                else
                    paramMap[ANIM_Event::DATAS] = param[0];
            }
        }
    }
    else
        action.name = str;
}

/// Action

String Action::ToString() const
    {
    String str("action(name:");
    str += name;

    if (param.Size())
        {
        str += " param:";
        unsigned ii=0;
        for (VariantMap::ConstIterator it = param.Begin(); it != param.End(); ++it, ii++)
        {
            if (it->first_ == ANIM_Event::DATAS)
                str += "data=" + it->second_.ToString();
            else
                str += GOA::GetAttributeName(it->first_) + "=" + it->second_.ToString();

            if (ii < param.Size()-1)
                str += "|";
    }
}

    str +=")";

    return str;
}

/// Transition

void Transition::Dump() const
{
    URHO3D_LOGERRORF("Transition() - Dump : event:%s(%u) sender:%d cond:%u conval:%u nextState:%u actions:%s ",
                    GOE::GetEventName(event).CString(), event.Value(), eventSender, condition.Value(), conditionValue.Value(),
                    nextState, GetActionsString(*this).CString());
}

/// AnimatorState

void AnimatorState::AddActions(const String& aNames)
{
//    URHO3D_LOGINFOF("GOC_Animator2D_Template() - AddActions : actionName = %s", aNames.CString());

    actions.Clear();
    Vector<String> actionsStr = aNames.Split(';');

    for (unsigned i=0; i < actionsStr.Size(); i++)
    {
        if (!actionsStr[i].Empty())
    {
        /// Get Action Params from actionName
            Action temp = Action::EMPTY;
            GetActionParams(actionsStr[i].Trimmed(), temp);

            /// Check if exist in Template
            HashMap<StringHash, Action>::ConstIterator ita = GOC_Animator2D_Template::actions_.Find(StringHash(temp.name));
        if (ita == GOC_Animator2D_Template::actions_.End())
        {
                URHO3D_LOGERRORF("GOC_Animator2D_Template() - AddActions : No Function Action[%d] for %s", i, temp.name.CString());
            }
            else
            {
                temp.ptr = ita->second_.ptr;
                actions.Push(temp);
            }
        }
    }
        }

bool AnimatorState::HasEvent(const String& eventname, int sender) const
{
    return GetEvent(eventname, sender) != -1;
    }

int AnimatorState::GetEvent(const String& eventname, int sender) const
    {
    if (eventTable.Size())
    {
        StringHash hashEventName = StringHash(eventname);
        for (int i=0; i < eventTable.Size(); i++)
        {
            const Transition& transition = eventTable[i];
            if (transition.event == hashEventName && transition.eventSender == sender)
                return i;
    }
    }
    return -1;
}

int AnimatorState::GetEvent(StringHash hashEventName, int sender) const
    {
    if (eventTable.Size())
    {
        for (int i=0; i < eventTable.Size(); i++)
        {
            const Transition& transition = eventTable[i];
            if (transition.event == hashEventName && transition.eventSender == sender)
                return i;
    }
    }
    return -1;
}

void AnimatorState::AddTransition(const String& eventname, int sender, StringHash cond, StringHash condvalue, unsigned nextState, const String& aNames)
{
//    URHO3D_LOGINFOF("GOC_Animator2D_Template() - AddTransition : event = %s, nextState = %u, actionName = %s", eventname.CString(), nextState, aNames.CString());

    StringHash event(eventname);
    int eventindex = GetEvent(event, sender);
    bool modify = true;
    if (eventindex == -1)
    {
        modify = false;
        hashEventTable.Push(event);
        eventindex = eventTable.Size();
        eventTable.Resize(eventTable.Size()+1);
    }

    Transition& t    = eventTable[eventindex];
    t.eventSender    = sender;
    t.event          = event;
    t.condition      = cond;
    t.conditionValue = condvalue;
    t.nextState      = nextState;
    t.actions.Clear();

    // Parse Actions String
    {
        Vector<String> actionsStr = aNames.Split(';');
        for (unsigned i=0; i < actionsStr.Size(); i++)
        {
            if (actionsStr[i].Empty())
                continue;

            // Get Action Params from actionName
            Action temp = Action::EMPTY;
            GetActionParams(actionsStr[i].Trimmed(), temp);

            // Check if exist in Template
            HashMap<StringHash, Action>::ConstIterator ita = GOC_Animator2D_Template::actions_.Find(StringHash(temp.name));
            if (ita == GOC_Animator2D_Template::actions_.End())
            {
                URHO3D_LOGERRORF("GOC_Animator2D_Template() - AddTransition : No Function Action[%d] for %s", i, temp.name.CString());
                continue;
            }

            temp.ptr = ita->second_.ptr;
            t.actions.Push(temp);
        }
    }

#ifdef LOGDEBUG_ANIMATOR2D
    String actionsStr = GetActionsString(t);
    URHO3D_LOGERRORF("GOC_Animator2D_Template() - AddTransition : state=%s(%u) event=%s(%u) sender=%s(%d) %s eventTable[%u] with Transition(nextState=%u %s)",
                      name.CString(), hashName.Value(), eventname.CString(), event.Value(), EventSenderTypeNames_[sender], sender, modify ? "Modify":"Add", eventindex, nextState, !actionsStr.Empty() ? (String("actions:") + actionsStr).CString() : "noaction");
#endif
}

void AnimatorState::RemoveTransition(const String& eventname, int sender)
{
    int eventindex = GetEvent(eventname, sender);
    if (eventindex != -1)
    {
        hashEventTable.Erase(eventindex);
        eventTable.Erase(eventindex);
    }
}

void AnimatorState::FindTransitionStates()
{
    nextStateTable.Clear();
    for (Vector<Transition>::ConstIterator it = eventTable.Begin(); it != eventTable.End(); it++)
    {
        unsigned nextstate = template_->transitionTable[it->nextState].hashName.Value();

        if (!nextStateTable.Contains(nextstate))
            nextStateTable.Push(nextstate);
    }
}

bool AnimatorState::CanTransitToState(unsigned nextState) const
{
    return nextStateTable.Contains(nextState);
}


/// GOC_Animator2D_Template

void GOC_Animator2D_Template::RegisterTemplate(const String& s, const GOC_Animator2D_Template& t)
{
    StringHash hash(s);
    unsigned key = hash.Value();

    if (templates_.Contains(key)) return;

    templates_ += Pair<unsigned, GOC_Animator2D_Template>(key, t);

    GOC_Animator2D_Template& animtemplate = templates_[key];
    animtemplate.name = s;
    animtemplate.hashName = hash;

    animtemplate.Bake();

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGERRORF("GOC_Animator2D_Template() - RegisterTemplate %s(%u) size=%u", s.CString(), key, templates_.Size());
#endif
}

void GOC_Animator2D_Template::DumpAll()
{
    unsigned index = 0;
    for (HashMap<unsigned, GOC_Animator2D_Template>::ConstIterator it=templates_.Begin(); it!=templates_.End(); ++it)
    {
        URHO3D_LOGINFOF("GOC_Animator2D_Template() - DumpAll : templates_[%u]",index);
        it->second_.Dump();
        ++index;
    }
}

void GOC_Animator2D_Template::Dump() const
{
    URHO3D_LOGINFOF("GOC_Animator2D_Template() - Dump : name=%s hash=%u base=%u - hashStateTable size=%u - transitionTable size=%u", name.CString(), hashName.Value(), baseTemplate.Value(), hashStateTable.Size(), transitionTable.Size());

    for (unsigned i=0; i<hashStateTable.Size(); ++i)
    {
        AnimatorState* state = GetState(i);
        URHO3D_LOGINFOF("  => hashStateTable[%u] = %u => %s(%u) event=%s(%u)", i, hashStateTable[i].Value(), state->name.CString(), state->hashName.Value(), GOE::GetEventName(state->event).CString(), state->event.Value());
    }
    for (unsigned i=0; i<transitionTable.Size(); ++i)
    {
        const AnimatorState& a = transitionTable[i];
        URHO3D_LOGINFOF("  => transitionTable[%u] : AnimatorState(name:%s hash=%u event=%u keyWords=%s tickDelay=%f)"
                        , i, a.name.CString(), a.hashName.Value(), a.event.Value(), a.animStateKeywords.CString(), a.tickDelay);
        URHO3D_LOGINFOF("    => nextStateTable size=%u", a.nextStateTable.Size());
        for (unsigned j=0; j<a.nextStateTable.Size(); ++j)
        {
            URHO3D_LOGINFOF("      => nextStateTable[%u] : Can Transit to %s(%u) ", j, GOS::GetStateName(a.nextStateTable[j]).CString(), a.nextStateTable[j]);
        }
        URHO3D_LOGINFOF("    => hashEventTable size=%u", a.hashEventTable.Size());
        for (unsigned j=0; j<a.hashEventTable.Size(); ++j)
        {
            URHO3D_LOGINFOF("      => hashEventTable[%u] = %u (%s)", j, a.hashEventTable[j].Value(), GOE::GetEventName(a.hashEventTable[j]).CString());
        }
        URHO3D_LOGINFOF("    => eventTable size=%u", a.eventTable.Size());
        for (unsigned j=0; j<a.eventTable.Size(); ++j)
        {
            const Transition& t = a.eventTable[j];
            URHO3D_LOGINFOF("      => eventTable[%u] : Transition( event:%s(%u) sender:%d cond:%u conval:%u nextState:%u(%s) actions:%s )",
                            j, GOE::GetEventName(t.event).CString(), t.event.Value(), t.eventSender, t.condition.Value(), t.conditionValue.Value(),
                            t.nextState, transitionTable[t.nextState].name.CString(), GetActionsString(t).CString());
        }
    }
}

void GOC_Animator2D_Template::AddState(const String& name, const String& AnimStateKeywords, bool directionalAnimations, float tickDelay, const String& actionnames)
{
    StringHash hashStateName(GOS::Register(name));

    int index = GetSignedStateIndex(hashStateName);

    if (index != -1)
    {
        /// Replace State
        transitionTable[index] = AnimatorState(name, tickDelay, AnimStateKeywords, directionalAnimations, actionnames);
    }
    else
    {
        /// Add State
        hashStateTable.Push(hashStateName);
        transitionTable.Push(AnimatorState(name, tickDelay, AnimStateKeywords, directionalAnimations, actionnames));
    }
}

void GOC_Animator2D_Template::ApplyEventToStates(const String& eventName, int sender, const String& listStates, const String& nextStateName, const String& condition, const String& actionNames)
{
    StringHash cond, condVal;

    if (!condition.Empty())
    {
        Vector<String> params = condition.Split('=');
        if (params.Size() > 0)
        {
            cond = StringHash(params[0]);
            if (params.Size() > 1)
                condVal = StringHash(params[1]);
        }
    }

    // First : by default add event to nextState
    if (!nextStateName.Empty() && eventName != "AEvent_EndLoop")
    {
        StringHash hash(nextStateName);
        AnimatorState* nextstate = GetState(hash);
        if (nextstate && !nextstate->HasEvent(eventName, sender))
        {
            nextstate->event = GOE::Register(eventName);
//            URHO3D_LOGINFOF("GOC_Animator2D_Template() - ApplyEventToStates (%s,%s) to first newstate=%s", eventName.CString(), actionNames.CString(), nextstate->name.CString());
            nextstate->AddTransition(eventName, sender, cond, condVal, GetStateIndex(hash), actionNames);
        }
    }

    // Second : Add event to listStates
    Vector<String> states = listStates.Split(';');
    if (states.Size() == 0)
    {
        URHO3D_LOGERRORF("GOC_Animator2D_Template() - ApplyEventToStates %s to state %s Empty !", eventName.CString(), listStates.CString());
        return;
    }
    for (unsigned i=0; i < states.Size(); ++i)
    {
        String stateStr = states[i].Trimmed();
        if (stateStr == "ALL")
        {
//            if (actionNames.Empty())
//                URHO3D_LOGINFOF("GOC_Animator2D_Template() - ApplyEventToStates (%s) to ALL", eventName.CString());
//            else
//                URHO3D_LOGINFOF("GOC_Animator2D_Template() - ApplyEventToStates (%s,action:%s) to ALL", eventName.CString(), actionNames.CString());

            for (unsigned j=0; j < hashStateTable.Size() ; ++j)
            {
                AnimatorState* state = GetState(hashStateTable[j]);
                if (state == 0) continue;

                StringHash nextState = nextStateName.Empty() ? hashStateTable[j] : StringHash(nextStateName);
//                URHO3D_LOGINFOF("  => To state %s",state->name.CString());
                state->AddTransition(eventName, sender, cond, condVal, GetStateIndex(nextState), actionNames);
            }
        }
        else
        {
            bool AddingTransition = (stateStr.At(0) != '!');
            if (!AddingTransition)
                stateStr.Erase(0);

            AnimatorState* state = GetState(StringHash(stateStr));
            if (state == 0) continue;

            if (AddingTransition)
            {
                StringHash nextState = nextStateName.Empty() ? StringHash(stateStr) : StringHash(nextStateName);
#ifdef LOGDEBUG_ANIMATOR2D
                if (actionNames.Empty())
                    URHO3D_LOGERRORF("GOC_Animator2D_Template() - ApplyEventToStates (%s) to state=%s", eventName.CString(), state->name.CString());
                else
                    URHO3D_LOGERRORF("GOC_Animator2D_Template() - ApplyEventToStates (%s,action:%s) to state=%s", eventName.CString(), actionNames.CString(), state->name.CString());
#endif
                state->AddTransition(eventName, sender, cond, condVal, GetStateIndex(nextState), actionNames);
            }
            else
            {
//                URHO3D_LOGINFOF("GOC_Animator2D_Template() - Remove Event %s to state %s",eventName.CString(),state->name.CString());
                state->RemoveTransition(eventName, sender);
            }
        }
    }
}

void GOC_Animator2D_Template::AddAnimationSet(AnimationSet2D* animSet2D)
{
    if (animInfoTables_.Contains(animSet2D->GetNameHash()))
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGERRORF("GOC_Animator2D_Template - AddAnimationSet : name=%s ... already inside Tables !", animSet2D->GetName().CString());
#endif
        return;
    }

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGERRORF("GOC_Animator2D_Template - AddAnimationSet : name=%s ...", animSet2D->GetName().CString());
#endif

    unsigned numEntities = animSet2D->GetSpriterData()->entities_.Size();
    unsigned numStates = transitionTable.Size();

    Vector<Vector<AnimInfoTable> >& animtables = animInfoTables_[animSet2D->GetNameHash()];
    animtables.Reserve(numEntities);

    for (unsigned e=0; e < numEntities; e++)
    {
        Vector<AnimInfoTable> animInfoTable;
        animInfoTable.Reserve(numStates);

        Spriter::Entity* entity = animSet2D->GetSpriterData()->entities_[e];
        unsigned numAnimations = entity->animations_.Size();

        for (unsigned i=0; i < numStates; i++)
        {
#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGERRORF("GOC_Animator2D_Template() - AddAnimationSet : entity=%s state=%s keywords=%s ...", entity->name_.CString(), transitionTable[i].name.CString(), transitionTable[i].animStateKeywords.CString());
#endif
            AnimInfoTable animInfos;

            Vector<String> animStateKeywords = transitionTable[i].animStateKeywords.Split(';');

            for (unsigned j=0; j < animStateKeywords.Size(); j++)
            {
                const String& keyword = animStateKeywords[j];
                for (unsigned k=0; k < numAnimations; k++)
                {
                    Spriter::Animation* anim2d = entity->animations_[k];
                    const String& animName = anim2d->name_;

                    unsigned l = animName.Find(keyword, 0, false);

                    // take only animation names that have a substring matching with the keyword without the separator underscore following
                    // that allow to have multiple animation for a same state
                    // ex : for the keyword "idle" the animation names idle1, idle2, idle3 will be animations for the state but not idle_air
                    if (l != String::NPOS && (animName.Length() <= l + keyword.Length() || animName[l + keyword.Length()] != '_'))
//                    if (animName.Find(keyword, 0, false) != String::NPOS)
                    {
                        animInfos.Push(AnimInfo(k, animName, anim2d->length_));
#ifdef LOGDEBUG_ANIMATOR2D
                        URHO3D_LOGERRORF("GOC_Animator2D_Template() - AddAnimationSet : entity=%s state=%s keyword=%s add animation=%s ... OK !", entity->name_.CString(), transitionTable[i].name.CString(), keyword.CString(), animName.CString());
#endif
                    }
                }
            }

            if (!animInfos.Size())
                animInfos.Push(AnimInfo(0, String::EMPTY, 0.f));

            animInfoTable.Push(animInfos);
        }

        animtables.Push(animInfoTable);
    }
#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGERRORF("GOC_Animator2D_Template - AddAnimationSet : name=%s ... OK !", animSet2D->GetName().CString());
#endif
}

void GOC_Animator2D_Template::Bake()
{
    for (Vector<AnimatorState>::Iterator it = transitionTable.Begin(); it != transitionTable.End(); ++it)
    {
        GOS::Register(it->name);
        it->template_ = this;
        it->FindTransitionStates();
    }
}

void GOC_Animator2D_Template::DumpAnimationInfoTable() const
{
    /// TODO
}

unsigned GOC_Animator2D_Template::GetStateIndex(const StringHash& hashname) const
{
    Vector<StringHash>::ConstIterator it = hashStateTable.Find(hashname);
#ifdef LOGDEBUG_ANIMATOR2D
    if (it == hashStateTable.End())
    {
        URHO3D_LOGERRORF("GOC_Animator2D_Template() - GetStateIndex : not finding hashValue = %u => return default 0 !", hashname.Value());
        return 0;
    }
#endif
    return unsigned(it-hashStateTable.Begin());
}

int GOC_Animator2D_Template::GetSignedStateIndex(const StringHash& hashname) const
{
    Vector<StringHash>::ConstIterator it = hashStateTable.Find(hashname);
    return it != hashStateTable.End() ? it-hashStateTable.Begin() : -1;
}

unsigned GOC_Animator2D_Template::GetStateIndex(unsigned hashvalue) const
{
    Vector<StringHash>::ConstIterator it = hashStateTable.Find(StringHash(hashvalue));
#ifdef LOGDEBUG_ANIMATOR2D
    if (it == hashStateTable.End())
    {
        URHO3D_LOGERRORF("GOC_Animator2D_Template() - GetStateIndex : not finding hashValue = %u => return default 0 !", hashvalue);
        return 0;
    }
#endif
    return unsigned(it-hashStateTable.Begin());
}

int GOC_Animator2D_Template::GetSignedStateIndex(unsigned hashvalue) const
{
    Vector<StringHash>::ConstIterator it = hashStateTable.Find(StringHash(hashvalue));
    return it != hashStateTable.End() ? it-hashStateTable.Begin() : -1;
}

AnimatorState* GOC_Animator2D_Template::GetState(const StringHash& hashname) const
{
//    Vector<StringHash>::ConstIterator it = hashStateTable.Find(hashname);
////    return it != hashStateTable.End() ? (AnimatorState*) &(transitionTable[it-hashStateTable.Begin()]) : 0;
//    return it != hashStateTable.End() ? &(transitionTable[it-hashStateTable.Begin()]) : 0;
//
    for (Vector<AnimatorState>::ConstIterator it=transitionTable.Begin(); it!=transitionTable.End(); ++it)
    {
        if (it->hashName == hashname) return (AnimatorState*)it.ptr_;
    }
    return 0;
}

AnimatorState* GOC_Animator2D_Template::GetState(unsigned index) const
{
    return index < transitionTable.Size() ? (AnimatorState*)&transitionTable[index] : 0;
}

AnimatorState* GOC_Animator2D_Template::GetStateFromEvent(const StringHash& hashevent) const
{
    for (Vector<AnimatorState>::ConstIterator it=transitionTable.Begin(); it!=transitionTable.End(); ++it)
    {
        if (it->event == hashevent) return (AnimatorState*)it.ptr_;
    }
    return 0;
}


/// GOC_Animator2D

enum AnimationAlignement
{
    HORIZONTAL=0,
    HORIZONTALFLIP,
    ROTATELEFT,
    ROTATERIGHT,
};

unsigned GOC_Animator2D::sFirstSpecificStateIndex_;

GOC_Animator2D::GOC_Animator2D(Context* context) :
    Component(context),
    roofFlipping_(0.f),
    orientation(1.f),
    currentTemplate(0),
    currentState(0),
    currentStateTime(0),
    forceNextState(0),
    customTemplate(false),
    autoSwitchAnimation_(false),
    subscribeOk_(false),
    physicFlipX_(false),
    followOwnerY_(false),
    alignment_(0),
    forceAnimVersion_(-1),
    spawnEntityMode_(SPAWNENTITY_ALWAYS)
{
    eventActions_.Resize(NumEventSenderType);
}

GOC_Animator2D::~GOC_Animator2D()
{
//    URHO3D_LOGINFOF("~GOC_Animator2D()");

    Stop();

    ClearCustomTemplate();
}

void GOC_Animator2D::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Animator2D>();

//    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Define Template", GetTemplateAttr, DefineTemplateOn, String, String::EMPTY, AM_FILE);

    URHO3D_ACCESSOR_ATTRIBUTE("New State", GetStateAttr, SetStateAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("New Event", GetEventAttr, SetEventAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Register Template", GetTemplateAttr, RegisterTemplate, String, String::EMPTY, AM_FILE);

    URHO3D_ACCESSOR_ATTRIBUTE("Default Orientation", GetDefaultOrientationAttr, SetDefaultOrientationAttr, float, 1.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Direction", GetDirection, SetDirection, Vector2, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Template", GetTemplateName, SetTemplateName, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("State", GetState, SetState, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Auto Switch Animations", GetAutoSwitchAnimations, SetAutoSwitchAnimations, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Add Event Actions", GetEventActions, SetEventActions, String, String::EMPTY, AM_DEFAULT);

    URHO3D_ATTRIBUTE("Flipping On Wall", Vector2, wallFlipping_, Vector2::ZERO, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Flipping On Roof", float, roofFlipping_, 0.f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Follow Owner In Y", bool, followOwnerY_, false, AM_DEFAULT);

    GOC_Animator2D_Template::actions_.Clear();
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_Null"), Action::EMPTY);
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_FindState"), Action("AAction_FindState", &GOC_Animator2D::FindNextState));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_ChangeEntity"), Action("AAction_ChangeEntity", &GOC_Animator2D::ChangeEntity));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_ChangeAnim"), Action("AAction_ChangeAnim", &GOC_Animator2D::ChangeAnimation));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_CheckTimer"), Action("AAction_CheckTimer", &GOC_Animator2D::CheckTimer));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_CheckAnim"), Action("AAction_CheckAnim", &GOC_Animator2D::CheckAnim));
//    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_CheckDirection"), Action("AAction_CheckDirection", &GOC_Animator2D::CheckDirection));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_CheckEmpty"), Action("AAction_CheckEmpty", &GOC_Animator2D::CheckEmpty));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_SendEvent"), Action("AAction_SendEvent", &GOC_Animator2D::SendAnimEvent));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_SpawnParticule"), Action("AAction_SpawnParticule", &GOC_Animator2D::SpawnParticule));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_SpawnEntity"), Action("AAction_SpawnEntity", &GOC_Animator2D::SpawnEntity));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_SpawnAnimation"), Action("AAction_SpawnAnimation", &GOC_Animator2D::SpawnAnimation));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_SpawnFurniture"), Action("AAction_SpawnFurniture", &GOC_Animator2D::SpawnFurniture));
//    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_SpawnAnimationIn"), Action("AAction_SpawnAnimationIn", &GOC_Animator2D::SpawnAnimationInside));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_LightOn"), Action("AAction_LightOn", &GOC_Animator2D::LightOn));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_LightOff"), Action("AAction_LightOff", &GOC_Animator2D::LightOff));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_CheckFireLight"), Action("AAction_CheckFireLight", &GOC_Animator2D::CheckFireLight));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_ToDisappear"), Action("AAction_ToDisappear", &GOC_Animator2D::ToDisappear));
    GOC_Animator2D_Template::actions_ += Pair<StringHash,Action>(StringHash("AAction_ToDestroy"), Action("AAction_ToDestroy", &GOC_Animator2D::ToDestroy));

    GOE::Register("AEvent_StartLoop");
    GOE::Register("AEvent_TickLoop");
    GOE::Register("AEvent_EndLoop");

    GOC_Animator2D_Template::templates_.Clear();

    GOC_Animator2D_Template defaultProps = GOC_Animator2D_Template();
    defaultProps.baseTemplate = 0;

    GOC_Animator2D_Template::RegisterTemplate("AnimatorTemplate_Empty", defaultProps);

    defaultProps.AddState("State_Appear", "appear");
    defaultProps.AddState("State_Default", "idle");
    defaultProps.AddState("State_Disappear", "disappear");
    defaultProps.AddState("State_Destroy");
    sFirstSpecificStateIndex_ = 4;

    defaultProps.ApplyEventToStates("AEvent_StartLoop", Sender_Self, "ALL", String::EMPTY, String::EMPTY, "AAction_ChangeAnim");
    defaultProps.ApplyEventToStates("AEvent_StartLoop", Sender_Self, "State_Destroy", String::EMPTY, String::EMPTY, "AAction_ToDestroy");
    defaultProps.ApplyEventToStates("AEvent_TickLoop", Sender_Self, "State_Appear", String::EMPTY, String::EMPTY, "AAction_CheckAnim");
    defaultProps.ApplyEventToStates("AEvent_TickLoop", Sender_Self, "State_Disappear", String::EMPTY, String::EMPTY, "AAction_CheckTimer");
    defaultProps.ApplyEventToStates("AEvent_EndLoop", Sender_Self, "State_Appear", String::EMPTY, String::EMPTY, "AAction_FindState");
    defaultProps.ApplyEventToStates("AEvent_EndLoop", Sender_Self, "State_Disappear", "State_Destroy", String::EMPTY, "AAction_Null");

    defaultProps.ApplyEventToStates("Event_Default", Sender_Self, "State_Default", "State_Default", String::EMPTY, "AAction_Null");
    defaultProps.ApplyEventToStates("Event_OnAppear", Sender_Self, "State_Appear", "State_Appear", String::EMPTY, "AAction_Null");
    defaultProps.ApplyEventToStates("Event_OnDisappear", Sender_Self, "State_Appear;State_Default", "State_Disappear", String::EMPTY, "AAction_Null");
    defaultProps.ApplyEventToStates("Event_OnDestroy", Sender_Self, "ALL", "State_Destroy", String::EMPTY, "AAction_Null");

    GOC_Animator2D_Template::RegisterTemplate("AnimatorTemplate_Default", defaultProps);
}

void GOC_Animator2D::DefineTemplateOn(const String& baseTemplate)
{
    if (baseTemplate.Empty())
        return;

    templateName_ = baseTemplate;

    SetTemplate();
}

void GOC_Animator2D::RegisterTemplate(const String& s)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - RegisterTemplate=%s(%u)", s.CString(), StringHash(s).Value());
    if (s.Empty())
        return;

    GOC_Animator2D_Template::RegisterTemplate(s, *currentTemplate);

    templateName_ = s;
}

void GOC_Animator2D::SetTemplateName(const String& s)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - SetTemplateName : Node=%s(%u) ... template=%s OK !", node_->GetName().CString(), node_->GetID(), s.CString());

    if (s.Empty())
        return;

    if (templateName_ == s)
        return;

    ClearCustomTemplate();
    currentTemplate = 0;
    currentState = 0;

    templateName_ = s;

    MarkNetworkUpdate();
}

void GOC_Animator2D::SetStateAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - SetStateAttr = %s ...", value.CString());

    if (value.Empty())
        return;

    String stateName  = String::EMPTY;
    String animStateKeywords = String::EMPTY;
    bool directionalAnimations = false;
    float tickDelay = 0.f;
    String action = String::EMPTY;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
    {
        URHO3D_LOGERROR("GOC_Animator2D() - SetStateAttr NOK! split=0");
        return;
    }

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0) continue;

        String str = s[0].Trimmed();
//        URHO3D_LOGINFOF("GOC_Animator2D() : SetStateAttr str=%s", str.CString());
        if (str == "name")
            stateName = s[1].Trimmed();
        else if (str == "animKeyWords")
            animStateKeywords = s[1].Trimmed();
        else if (str == "directional")
            directionalAnimations = s[1].Trimmed() == "true";
        else if (str == "tick")
            tickDelay = ToFloat(s[1].Trimmed());
        else if (str == "action")
            action = s[1].Trimmed();
    }

    if (stateName.Empty())
    {
        URHO3D_LOGERRORF("GOC_Animator2D() - SetStateAttr NOK! stateName=%s", stateName.CString());
        return;
    }

    StringHash hashStateName = StringHash(stateName);
//    if (currentTemplate->hashStateTable.Contains(hashStateName)) { LOGINFO("GOC_Animator2D() : SetStateAttr NOK! hashStateName exist!"); return; }

    if (!customTemplate)
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - SetStateAttr - create new custom Template (dont't forget to register it) !");
        GOC_Animator2D_Template* newTemplate = new GOC_Animator2D_Template("AnimatorTemplate_Custom", *currentTemplate);
        customTemplate = true;
        currentTemplate = newTemplate;
    }

    currentTemplate->AddState(stateName, animStateKeywords, directionalAnimations, tickDelay, action);

//    URHO3D_LOGINFOF("GOC_Animator2D() - AddState=%u(%s,%s,%f) OK!", hashStateName.Value(), stateName.CString(),animStateKeywords.CString(),tickDelay);
}

void GOC_Animator2D::SetEventAttr(const String& value)
{
    if (value.Empty())
        return;

    String eventName = String::EMPTY;
    String actionName = String::EMPTY;
    String nextStateName = String::EMPTY;
    String listStates = String::EMPTY;
    String condition = String::EMPTY;
    int sender = 0;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0)
            continue;

        String str = s[0].Trimmed();
        if (str == "name")
        {
            Vector<String> evstr = s[1].Split('@');
            eventName = evstr.Front().Trimmed();
            sender = Sender_Self;
            if (evstr.Size() > 1)
            {
                if (evstr[1].Trimmed().ToLower() == "owner")
                    sender = Sender_Owner;
                else if (evstr[1].Trimmed().ToLower() == "all")
                    sender = Sender_All;
            }

//            URHO3D_LOGERRORF("GOC_Animator2D() - SetEventAttr (%s) : eventnamestr=%s name=%s sender=%s(%d) ", value.CString(), s[1].CString(), eventName.CString(), evstr.Size() > 1 ? evstr[1].CString() : "", sender);
        }
        else if (str == "nextState")
            nextStateName = s[1].Trimmed();
        else if (str == "action")
            actionName = s[1].Trimmed();
        else if (str == "applyToStates")
            listStates = s[1].Trimmed();
        else if (str == "condition")
            condition = s[1].Trimmed();
    }

    if (eventName.Empty() && (nextStateName.Empty() || listStates.Empty()))
    {
        URHO3D_LOGERRORF("GOC_Animator2D() - SetEventAttr NOK! eventName=%s | nextStateName=%s | listStates=%s | actionName=%s",
                         eventName.CString(),nextStateName.CString(),listStates.CString(),actionName.CString());
        return;
    }

    if (!customTemplate)
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - SetEventAttr - create new custom Template (dont't forget to register it) !");
        GOC_Animator2D_Template* newTemplate = new GOC_Animator2D_Template("AnimatorTemplate_Custom", *currentTemplate);
        customTemplate = true;
        currentTemplate = newTemplate;
    }

    currentTemplate->ApplyEventToStates(eventName, sender, listStates, nextStateName, condition, actionName);

//    URHO3D_LOGINFOF("GOC_Animator2D() - SetEventAttr OK!");
}

void GOC_Animator2D::SetAutoSwitchAnimations(bool state)
{
    autoSwitchAnimation_ = state;

    MarkNetworkUpdate();
}

void GOC_Animator2D::SetEventActions(const String& eventactions)
{
    if (eventActionStr_ == eventactions)
        return;

    eventActionStr_ = eventactions;
    eventActions_.Clear();
    eventActions_.Resize(NumEventSenderType);

    if (eventactions.Empty())
        return;

    Vector<String> vString = eventactions.Split('|');
    if (vString.Size() == 0)
    {
        URHO3D_LOGERROR("GOC_Animator2D() - SetEventActions NOK! split=0");
        return;
    }

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() < 2)
            continue;

        Vector<String> t = s[0].Split('@');

        // Get Event
        StringHash event(t[0].Trimmed());

        // Get Event Sender
        const int sender = t.Size() > 1 ? GetEnumValue(t[1].Trimmed(), EventSenderTypeNames_) : Sender_Self;

        Actions& actions = eventActions_[sender][event];

        // Get Action Parameters
        t = s[1].Split(';');
        for (unsigned i=0; i < t.Size(); i++)
        {
        Action action;
            GetActionParams(t[i].Trimmed(), action);

            // Add the Event Action
        const Action& templatedAction = GOC_Animator2D_Template::GetAction(StringHash(action.name));
        if (templatedAction.ptr)
        {
            action.ptr = templatedAction.ptr;
                actions.Push(action);
#ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGERRORF("GOC_Animator2D() - SetEventActions : sender=%s(%d) event=%s(%u) action=%s",
                                EventSenderTypeNames_[sender], sender, GOE::GetEventName(event).CString(), event.Value(), action.ToString().CString());
#endif
            }
        }
    }
}

void GOC_Animator2D::SetOwner(Node* node)
{
    if (owner_ != node)
    {
        owner_ = node;
        deltaYWithOwner_ = owner_->GetWorldPosition2D().y_ - node_->GetWorldPosition2D().y_;

//        URHO3D_LOGINFOF("GOC_Animator2D() - SetOwner : Node=%s(%u) owner_=%s(%u) ...", node_->GetName().CString(), node_->GetID(), owner_ ? owner_->GetName().CString() : "", owner_ ? owner_->GetID() : 0);

        // Remove Subscribers
        if (owner_)
            for (HashMap<StringHash, Actions>::ConstIterator it = eventActions_[Sender_Owner].Begin(); it != eventActions_[Sender_Owner].End(); ++it)
                UnsubscribeFromEvent(owner_, it->first_);

        // Add Subscribers
        if (owner_)
            for (HashMap<StringHash, Actions>::ConstIterator it = eventActions_[Sender_Owner].Begin(); it != eventActions_[Sender_Owner].End(); ++it)
                SubscribeToEvent(owner_, it->first_, URHO3D_HANDLER(GOC_Animator2D, OnEventActions));

        SequencedSprite2D* sequencedsprite = node_->GetComponent<SequencedSprite2D>();
        if (sequencedsprite)
            sequencedsprite->SetOwner(owner_);
    }
}

void GOC_Animator2D::SetDefaultOrientationAttr(float o)
{
    orientation = o;
    MarkNetworkUpdate();
}

void GOC_Animator2D::SetDirection(const Vector2& dir)
{
    if (dir.x_ != 0.f && dir != direction)
    {
        // update direction if change
        direction = dir;

#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SetDirection : Node=%s(%u) ... direction=%s ... OK!", node_->GetName().CString(), node_->GetID(), direction.ToString().CString());
#endif

        // notify
        node_->SetVar(GOA::DIRECTION, direction);

        MarkNetworkUpdate();
    }

    if (animatedSprite)
    {
        // Check the animatedSprite and flip it if not good orientation
        animatedSprite->SetFlipX(orientation * direction.x_ < 0.f);
        if (physicFlipX_ != animatedSprite->GetFlipX())
        {
            physicFlipX_ = animatedSprite->GetFlipX();
            GameHelpers::SetPhysicFlipX(node_);
//            URHO3D_LOGINFOF("GOC_Animator2D() - SetDirection : Node=%s(%u) ... physflipX=%s ! ", GetNode()->GetName().CString(), GetNode()->GetID(), physicFlipX_ ? "true":"false");
        }
    }

    if (controller_)
        controller_->control_.direction_ = direction.x_;
}

void GOC_Animator2D::SetDirectionSilent(const Vector2& dir)
{
    if (dir.x_ == 0.f || dir.x_ == direction.x_)
        return;

//    URHO3D_LOGINFOF("GOC_Animator2D() - SetDirectionSilent : Node=%s(%u) ... dir=%s ...", node_->GetName().CString(), node_->GetID(), dir.ToString().CString());

    // Check the animatedSprite and flip it for the good orientation
    if (animatedSprite)
    {
        animatedSprite->SetFlipX(orientation * direction.x_ < 0.f);
        if (physicFlipX_ != animatedSprite->GetFlipX())
        {
            physicFlipX_ = animatedSprite->GetFlipX();
            GameHelpers::SetPhysicFlipX(node_);
            //        URHO3D_LOGINFOF("GOC_Animator2D() - SetDirectionSilent : Node=%s(%u) ... physflipX=%s ! ", GetNode()->GetName().CString(), GetNode()->GetID(), physicFlipX_ ? "true":"false");
        }
    }

    // update direction if change
    direction = dir;
//    URHO3D_LOGINFOF("GOC_Animator2D() - SetDirectionSilent : Node=%s(%u) ... direction=%s ... OK!",
//                    node_->GetName().CString(), node_->GetID(), direction.ToString().CString());
}

void GOC_Animator2D::SetShootTarget(const Vector2& targetpoint)
{
    shootTarget_ = targetpoint;
    if (shootTarget_ != Vector2::ZERO)
    {
        Vector2 direction = shootTarget_ - node_->GetWorldPosition2D();
        // Precalculate the spawnAngle with the animator node position (needed for ApplyDirectionalAnimations)
        // the exact angle is calculated with the trigger position
        spawnAngle_ = Atan(direction.y_/direction.x_);
        if (direction.x_ < 0.f)
            spawnAngle_ += 180.f;
    }
    else
    {
        spawnAngle_ = 0.f;
    }

//    URHO3D_LOGINFOF("GOC_Animator2D() - SetShootTarget : %s(%u) spawnangle=%F !",
//                    node_->GetName().CString(), node_->GetID(), spawnAngle_);
}

void GOC_Animator2D::SetState(const String& value)
{
    if (value.Empty())
        return;

    StringHash state(value);

    if (state == state_)
        return;

//    URHO3D_LOGINFOF("GOC_Animator2D() - SetState : Node=%s(%u) %s ...", node_->GetName().CString(), node_->GetID(), value.CString());

    if (!SetState(state) && currentTemplate)
        CheckAnimation();

//    URHO3D_LOGERRORF("GOC_Animator2D() - SetState : Node=%s(%u) entrystate=%s... state=%s(%u) OK !", node_->GetName().CString(), node_->GetID(), value.CString(), GOS::GetStateName(state_).CString(), state_.Value());
}

bool GOC_Animator2D::SetState(const StringHash& state)
{
    if (state == StringHash::ZERO)
        return false;

    if (!currentTemplate)
    {
//        SetTemplate();
        state_ = state;
        return false;
    }

    AnimatorState* oldState = currentState;
    AnimatorState* newState = currentTemplate->GetState(state);

    if (newState)
    {
        if (currentState != newState)
            currentState = newState;
    }
    else if (!currentState)
    {
        FindNextState();
        currentState = nextState;
    }

    if (currentState != oldState)
    {
        state_ = currentState->hashName;
        currentStateIndex = currentTemplate->GetStateIndex(state_);

#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SetState : Node=%s(%u) entryState=%u state=%s(%u) ... OK !",
                        node_->GetName().CString(), node_->GetID(), state.Value(), currentState->name.CString(), state_.Value());
#endif
        if (oldState)
            UpdateSubscriber(oldState, currentState);
        else
            UpdateSubscriber();

        if (animatedSprite && animatedSprite->GetSpriterInstance())
            ChangeAnimation();

        ApplyNewStateAction();

        NotifyCurrentState();

        MarkNetworkUpdate();

        return true;
    }

    return false;
}

void GOC_Animator2D::SetNetState(const StringHash& state, unsigned animVersion, bool animchanged)
{
    if (state == StringHash::ZERO)
        return;

    if (!currentTemplate)
    {
        state_ = state;
        URHO3D_LOGERRORF("GOC_Animator2D() - SetNetState : Node=%s(%u) no Template !", node_->GetName().CString(), node_->GetID());
        return;
    }

    AnimatorState* newState = currentTemplate->GetState(state);
    int currentAnimVersion = animInfoIndexes_[currentStateIndex];

    if (currentState == newState && animVersion == currentAnimVersion && !animchanged)
        return;

    // check if currenstate can transit to newstate
    if (currentState && newState && currentState != newState && forceNextState != newState)
    {
        // if not, allow to force to the next state only after playing the animation fully
        if (!currentState->CanTransitToState(state.Value()))
        {
            forceNextState = newState;

//            URHO3D_LOGINFOF("GOC_Animator2D() - SetNetState : Node=%s(%u) can't transit from %s to %s !",
//                            node_->GetName().CString(), node_->GetID(), currentState->name.CString(), newState->name.CString());
            return;
        }
    }

    bool statechanged = newState && currentState != newState;
    if (statechanged)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        AnimatorState* oldState = currentState;
#endif

        state_ = newState->hashName;
        currentStateIndex = currentTemplate->GetStateIndex(state_);

        if (currentState)
        {
            UpdateSubscriber(currentState, newState);
            currentState = newState;
        }
        else
        {
            currentState = newState;
            UpdateSubscriber();
        }

        currentAnimVersion = 0;

#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SetNetState : Node=%s(%u) %s to %s ...", node_->GetName().CString(), node_->GetID(), oldState->name.CString(), newState->name.CString());
#endif
    }

    if (statechanged || animVersion != currentAnimVersion)
        animchanged = true;

    // force change animation
    if (animchanged && state != STATE_SHOOT)
        forceAnimVersion_ = animVersion;

    if (animatedSprite && animchanged)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SetNetState : Node=%s(%u) change animation from version %u to %u ...", node_->GetName().CString(), node_->GetID(), currentAnimVersion, forceAnimVersion_);
#endif
        ChangeAnimation();
    }

    if (statechanged)
        ApplyNewStateAction();
}

void GOC_Animator2D::SetSpawnEntityMode(int mode)
{
    spawnEntityMode_ = mode;
}


bool GOC_Animator2D::HasAnimationForState(unsigned state) const
{
    int stateindex = currentTemplate->GetSignedStateIndex(state);

//    URHO3D_LOGINFOF("GOC_Animator2D() - HasAnimationForState : Node=%s(%u) ... stateindex=%d currentAnimInfoTableSize=%u",
//                    node_->GetName().CString(), node_->GetID(), stateindex, currentAnimInfoTable ? currentAnimInfoTable->Size() : 0);

    if (stateindex == -1 || !currentAnimInfoTable || currentAnimInfoTable->Size() <= stateindex)
        return false;

    return !currentAnimInfoTable->At(stateindex)[0].animName.Empty();
}

int GOC_Animator2D::GetCurrentAnimIndex() const
{
    return GetCurrentAnimInfo(animInfoIndexes_[currentStateIndex]).animIndex;
}

Spriter::Entity* GOC_Animator2D::GetEntity() const
{
    return animatedSprite->GetSpriterInstance()->GetEntity();
}


/// URHO3D_HANDLER

static int AnimatorForcedSender = -1;

void GOC_Animator2D::Start()
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - Start : %s(%u) ... subscribeOk_=%s physicFlipX=%s ...",
//                    node_->GetName().CString(), node_->GetID(), subscribeOk_ ? "true":"false", physicFlipX_ ? "true":"false");

    if (subscribeOk_)
        return;

    delayParticule = 0.f;
    currentStateTime = 0.f;
    forceNextState = 0;
    spawnAngle_ = 0.f;
    forceAnimVersion_ = -1;
    netChangeCounter_ = 0;
    toDisappearCounter_ = -1;

    SubscribeToEvent(node_, SPRITER_PARTICULE, URHO3D_HANDLER(GOC_Animator2D, OnAnimation2D_Particule));
//    SubscribeToEvent(node_, SPRITER_LIGHT, URHO3D_HANDLER(GOC_Animator2D, OnAnimation2D_Light));
    SubscribeToEvent(node_, SPRITER_ENTITY, URHO3D_HANDLER(GOC_Animator2D, OnAnimation2D_Entity));
    SubscribeToEvent(node_, SPRITER_ANIMATION, URHO3D_HANDLER(GOC_Animator2D, OnAnimation2D_Animation));
    SubscribeToEvent(node_, SPRITER_ANIMATIONINSIDE, URHO3D_HANDLER(GOC_Animator2D, OnAnimation2D_Animation));
    SubscribeToEvent(node_, GO_COLLIDEGROUND, URHO3D_HANDLER(GOC_Animator2D, OnTouchGround));
    SubscribeToEvent(node_, GO_COLLIDEFLUID, URHO3D_HANDLER(GOC_Animator2D, OnTouchFluid));

//    if ((!controller_ && !GameContext::Get().ClientMode_) || (controller_ && controller_->IsMainController()))
//    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - Start : %s(%u) controller_=%u => Main Controlled Mode !", node_->GetName().CString(), node_->GetID(), controller_);
    /*
    	if (!controller_ || controller_->IsMainController())
        {
    //        URHO3D_LOGINFOF("GOC_Animator2D() - Start : %s(%u) Main Controlled Mode !", node_->GetName().CString(), node_->GetID());
            SubscribeToEvent(node_, GO_COLLIDEGROUND, URHO3D_HANDLER(GOC_Animator2D, OnTouchGround));

    //        if (GetScene())
    //            SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_Animator2D, OnUpdate));
        }
    //    else
    //        URHO3D_LOGINFOF("GOC_Animator2D() - Start : %s(%u) Net Controlled Mode !", node_->GetName().CString(), node_->GetID());
    */
    if (GetScene())
        SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_Animator2D, OnUpdate));

#ifdef ACTIVE_NETWORK_SERVERRECONCILIATION
    if (controller_ && (controller_->IsMainController() || GameContext::Get().ServerMode_))
#else
    if (controller_ && controller_->IsMainController())
#endif
    {
        SubscribeToEvent(node_, GO_CHANGEDIRECTION, URHO3D_HANDLER(GOC_Animator2D, OnChangeDirection));
        SubscribeToEvent(node_, GO_UPDATEDIRECTION, URHO3D_HANDLER(GOC_Animator2D, OnUpdateDirection));
        SubscribeToEvent(node_, AI_CHANGEORDER, URHO3D_HANDLER(GOC_Animator2D, OnChangeState));
    }

    if (currentState)
        UpdateSubscriber();

    ApplyNewStateAction();

    // Subscribe to ChangeArea (used when entity go from fluid to air for example : cf GOC_Move2D::Update_Swim)
    SubscribeToEvent(node_, EVENT_CHANGEAREA, URHO3D_HANDLER(GOC_Animator2D, OnEvent));

    // Subscribe To each Event Action Sender=None
    if (eventActions_[Sender_Self].Size())
        for (HashMap<StringHash, Actions>::ConstIterator it = eventActions_[Sender_Self].Begin(); it != eventActions_[Sender_Self].End(); ++it)
            SubscribeToEvent(it->first_, URHO3D_HANDLER(GOC_Animator2D, OnEventActions));

    // Subscribe To each Event Action Sender=Owner
    if (owner_ && eventActions_[Sender_Owner].Size())
    {
        for (HashMap<StringHash, Actions>::ConstIterator it = eventActions_[Sender_Owner].Begin(); it != eventActions_[Sender_Owner].End(); ++it)
            SubscribeToEvent(owner_, it->first_, URHO3D_HANDLER(GOC_Animator2D, OnEventActions));
    }

    // Subscribe To each Event Action Sender=None
    if (eventActions_[Sender_All].Size())
        for (HashMap<StringHash, Actions>::ConstIterator it = eventActions_[Sender_All].Begin(); it != eventActions_[Sender_All].End(); ++it)
        SubscribeToEvent(it->first_, URHO3D_HANDLER(GOC_Animator2D, OnEventActions));

    // Check for Event like Weather, Time
    if (WeatherManager::Get() && eventActions_[Sender_All].Size())
    {
        int timeperiod = WeatherManager::Get()->GetWorldTimePeriod();
        StringHash event;
        for (HashMap<StringHash, Actions>::ConstIterator it = eventActions_[Sender_All].Begin(); it != eventActions_[Sender_All].End(); ++it)
        {
            event = it->first_;
            if (((timeperiod == TIME_NIGHT || timeperiod == TIME_TWILIGHT) && event == WEATHER_TWILIGHT) ||
                ((timeperiod == TIME_DAY   || timeperiod == TIME_DAWN)     && event == WEATHER_DAWN))
            {
                URHO3D_LOGERRORF("GOC_Animator2D() - Start : Node=%s(%u) ... On Weather Event=%s(%u) ...",node_->GetName().CString(), node_->GetID(), GOE::GetEventName(event).CString(), event.Value());
                AnimatorForcedSender = Sender_All;
                OnEventActions(event, context_->GetEventDataMap(false));
                AnimatorForcedSender = -1;
            }
        }
    }

    subscribeOk_ = true;

//    URHO3D_LOGERRORF("GOC_Animator2D() - Start : %s(%u) state=%s(%u) currentStateTime=%f !",
//                     node_->GetName().CString(), node_->GetID(), GetState().CString(), GetStateValue().Value(), currentStateTime);
}

void GOC_Animator2D::Stop()
{
    if (!node_)
        return;

//    URHO3D_LOGERRORF("GOC_Animator2D() - Stop %s(%u) - physicFlipX_=%s !", node_->GetName().CString(), node_->GetID(), physicFlipX_ ? "true":"false");

    owner_.Reset();
    UnsubscribeFromAllEvents();

    if (node_->GetParent() != GameContext::Get().preloadGOTNode_ && animatedSprite && animatedSprite->GetComponent<AnimatedSprite2D>() == animatedSprite && !animatedSprite->GetRenderTargetAttr().Empty())
    {
        URHO3D_LOGERRORF("GOC_Animator2D() - Stop : Node=%s(%u) ... use Rendered Target AnimatedSprite not already ... subscribe to E_COMPONENTCHANGED !",
                         node_->GetName().CString(), node_->GetID());

        SubscribeToEvent(animatedSprite, E_COMPONENTCHANGED, URHO3D_HANDLER(GOC_Animator2D, OnComponentChanged));
    }

//    ResetState();

    subscribeOk_ = false;

    if (physicFlipX_)
    {
        GameHelpers::SetPhysicFlipX(node_);
        physicFlipX_ = false;
    }
}

void GOC_Animator2D::ResetState()
{
    if (currentTemplate)
    {
        currentStateIndex = Min(sFirstSpecificStateIndex_, currentTemplate->hashStateTable.Size()-1);
        state_ = currentTemplate->hashStateTable[currentStateIndex];
        currentState = &currentTemplate->transitionTable[currentStateIndex];

        nextState = forceNextState = 0;
        forceAnimVersion_ = -1;
        netChangeCounter_ = 0;

//        URHO3D_LOGERRORF("GOC_Animator2D() - ResetState : %s(%u) state=%s(%u) hashStateTable=%u currentStateIndex=%u!",
//                         node_->GetName().CString(), node_->GetID(), GetState().CString(),
//                         GetStateValue().Value(), currentTemplate->hashStateTable.Size(), currentStateIndex);
    }

    CheckAnimation();
}

bool GOC_Animator2D::PlugDrawables()
{
    animatedSprite = node_->GetComponent<AnimatedSprite2D>();

    animatedSprites.Clear();

    if (animatedSprite && animatedSprite->GetRenderTarget())
    {
        if (!animatedSprite->GetRenderTargetAttr().Empty())
        {
            URHO3D_LOGERRORF("GOC_Animator2D() - PlugDrawables : Node=%s(%u) ... animationComponentID=%u => renderTargetComponentID=%u !",
                             node_->GetName().CString(), node_->GetID(), animatedSprite->GetID(), animatedSprite->GetRenderTarget()->GetID());

            animatedSprite->SetFlipX(animatedSprite->GetRenderTarget()->GetFlipX());

            animatedSprites.Push(animatedSprite);
            animatedSprites.Push(animatedSprite->GetRenderTarget());

            // at this point, the main animatedsprite becomes the rendertarget animation
            animatedSprite = animatedSprite->GetRenderTarget();
        }
    }
    else
    {
        node_->GetComponents<AnimatedSprite2D>(animatedSprites, true);

        if (!animatedSprite && animatedSprites.Size())
            animatedSprite = animatedSprites[0];

//        URHO3D_LOGERRORF("GOC_Animator2D() - PlugDrawables : Node=%s(%u) ... num animatedSprites=%u ... main=%s(%u)",
//                          node_->GetName().CString(), node_->GetID(), animatedSprites.Size(), animatedSprite ? animatedSprite->GetNode()->GetName().CString() : "none", animatedSprite ? animatedSprite->GetNode()->GetID() : 0);
    }

//	if (node_->GetID() == 16782481)
//    {
//        URHO3D_LOGERRORF("GOC_Animator2D() - CheckAnimatedSprite : Node=%s(%u) ... num animatedSprites = %u !",
//                          node_->GetName().CString(), node_->GetID(), animatedSprites.Size());
//    }

    return animatedSprites.Size() != 0;
}

void GOC_Animator2D::UnplugDrawables()
{
    for (unsigned i=0; i < animatedSprites.Size(); i++)
        animatedSprites[i]->ClearRenderedAnimations();
    animatedSprites.Clear();
    animatedSprite.Reset();
}

void GOC_Animator2D::CheckAttributes()
{
//    bool ok = PlugDrawables();
    if (!PlugDrawables())
        return;

    if (!controller_)
        controller_ = node_->GetDerivedComponent<GOC_Controller>();

    if (!abilities_)
        abilities_ = node_->GetComponent<GOC_Abilities>();

    if (!gocmove_)
        gocmove_ = node_->GetComponent<GOC_Move2D>();

    // Template
    SetTemplate();

    SetAnimationSet();

    // Direction
    if (direction.x_ == 0.f)
    {
        if (node_->GetVar(GOA::DIRECTION) != Variant::EMPTY && node_->GetVar(GOA::DIRECTION).GetVector2().x_ != 0.f)
            SetDirection(node_->GetVar(GOA::DIRECTION).GetVector2());
        else
            SetDirection(Vector2(1.f, 0.f));
    }
    else
    {
        SetDirection(direction);
    }

    // State
    if ((!currentState || currentState->hashName != state_) && !SetState(state_))
    {
        SetState(currentTemplate->hashStateTable[0]);
//        URHO3D_LOGERRORF("GOC_Animator2D() - CheckAttributes : %s(%u) state=%s(%u) !",
//                     node_->GetName().CString(), node_->GetID(), GetState().CString(), GetStateValue().Value());
    }

    /// Required for the first activation on net
    if (!subscribeOk_)
        OnSetEnabled();
}

void GOC_Animator2D::ApplyAttributes()
{
//    URHO3D_LOGERRORF("GOC_Animator2D() - ApplyAttributes : %s(%u) ...", node_->GetName().CString(), node_->GetID());

    CheckAttributes();
    ResetState();

//    URHO3D_LOGERRORF("GOC_Animator2D() - ApplyAttributes : %s(%u) ... OK !", node_->GetName().CString(), node_->GetID());
}

void GOC_Animator2D::OnSetEnabled()
{
    bool enable = IsEnabledEffective();
    for (unsigned i=0; i < animatedSprites.Size(); i++)
    {
        AnimatedSprite2D* anim = animatedSprites[i];
        anim->MarkDirty();
        anim->SetLayer2(animatedSprite->GetLayer2());
        anim->SetEnabled(enable);
        anim->GetNode()->SetEnabled(enable);
//        URHO3D_LOGERRORF("GOC_Animator2D() - OnSetEnabled : %s(%u) animnode=%s enabled=%s ...",
//                         node_->GetName().CString(), node_->GetID(), anim->GetNode()->GetName().CString(), enable ? "true" : "false");
    }

    if (enable)
    {
        Start();
    }
    else
    {
        Stop();
    }

//    URHO3D_LOGINFOF("GOC_Animator2D() - OnSetEnabled : Node=%s(%u) enabled=%s ... OK !", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
}

void GOC_Animator2D::OnNodeSet(Node* node)
{
    if (node)
    {
//        URHO3D_LOGERRORF("GOC_Animator2D() - OnNodeSet : Node=%s(%u) ... ", node->GetName().CString(), node->GetID());

//        gocSound = node->GetComponent<GOC_SoundEmitter>();
        gocmove_ = node_->GetComponent<GOC_Move2D>();

//        URHO3D_LOGERRORF("GOC_Animator2D() - OnNodeSet : Node=%s(%u) ... OK !", node->GetName().CString(), node->GetID());
    }
}

void GOC_Animator2D::OnComponentChanged(StringHash eventType, VariantMap& eventData)
{
    if (!animatedSprite)
        return;

#ifdef ACTIVE_LAYERMATERIALS
    Material* material = GameContext::Get().layerMaterials_[LAYERACTORS];
#else
    Material* material = context_->GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/RenderTarget.xml");
#endif

    // desactive the rendertarget bind
    if (!animatedSprite->GetRenderTarget())
    {
        material->SetTexture(TU_DIFFUSE, 0);

        URHO3D_LOGERRORF("GOC_Animator2D() - OnComponentChanged : Node=%s(%u) ... animation=%u renderTarget=%u inactive texture unit=0 !",
                         node_->GetName().CString(), node_->GetID(), animatedSprite.Get(), animatedSprite->GetRenderTarget());
    }
    // active the rendertarget bind and check attributes
    else
    {
        material->SetTexture(TU_DIFFUSE, animatedSprite->GetRenderTexture());

        URHO3D_LOGERRORF("GOC_Animator2D() - OnComponentChanged : Node=%s(%u) ... animationComponentID=%u renderTargetComponentID=%u active texture unit=%u !",
                         node_->GetName().CString(), node_->GetID(), animatedSprite->GetID(), animatedSprite->GetRenderTarget()->GetID(), animatedSprite->GetRenderTexture());

        CheckAttributes();

        GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
        if (destroyer)
            destroyer->SetViewZ();
    }
}

void GOC_Animator2D::OnEvent(StringHash eventType, VariantMap& eventData)
{
//    if (node_->GetID() == 16777274)
//    URHO3D_LOGINFOF("GOC_Animator2D() - OnEvent : Node=%s(%u) - currentState %s - event %s", node_->GetName().CString(), node_->GetID(), currentState->name.CString(), GOE::GetEventName(eventType).CString());

    Dispatch(eventType, eventData);
}

void GOC_Animator2D::OnEventActions(StringHash eventType, VariantMap& eventData)
{
    int sender = Sender_Self;

    if (AnimatorForcedSender != -1)
    {
        sender = AnimatorForcedSender;
    }
    else if (GetEventSender() != 0)
    {
        if (GetEventSender() == node_)
            sender = Sender_Self;
        else if (GetEventSender() == owner_)
            sender = Sender_Owner;
        else
            sender = Sender_All;
    }

//    URHO3D_LOGINFOF("GOC_Animator2D() - OnEventActions : Node=%s(%u) - sender=%d event=%s ...", node_->GetName().CString(), node_->GetID(), sender, GOE::GetEventName(eventType).CString());

    HashMap<StringHash, Actions >::ConstIterator it = eventActions_[sender].Find(eventType);
    if (it != eventActions_[sender].End())
    {
        const Actions& actions = it->second_;
        for (unsigned i=0; i < actions.Size(); i++)
        {
            const Action& action = actions[i];
        if (action.ptr)
        {
    #ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - OnEventActions Node=%s(%u) : Run Action=%s for event=%s(%u) ... ",
                                node_->GetName().CString(), node_->GetID(), action.name.CString(), GOE::GetEventName(eventType).CString(), eventType.Value());
    #endif
                (this->*(action.ptr))(action.param);
            }
        }
    }
}

#define PARTICULE_INACTIVEDELAY 0.2f

void GOC_Animator2D::OnUpdate(StringHash eventType, VariantMap& eventData)
{
    timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

//    URHO3D_PROFILE(GOC_Animator2D);
    delayParticule += timeStep;

    Update();
}

void GOC_Animator2D::OnChangeDirection(StringHash eventType, VariantMap& eventData)
{
    if (!controller_)
        return;

    // Not Already Set
    if (controller_->control_.direction_ == 0.f)
    {
        direction.y_ = 0.f;
        float dirx = direction.x_ != 0.f ? direction.x_ : node_->GetVar(GOA::DIRECTION) != Variant::EMPTY ? node_->GetVar(GOA::DIRECTION).GetVector2().x_ : 0.f;
        controller_->control_.direction_ = dirx != 0.f ? dirx : (Random(100) < 50 ? -1.f : 1.f);
    }

    // Prepare changing direction
    direction.x_ = -controller_->control_.direction_;

    if (!UpdateDirection())
    {
        // Reset changing direction
        direction.x_ = controller_->control_.direction_;
    }

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - OnChangeDirection : Node=%s(%u) direction=%F", GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_);
#endif
}

void GOC_Animator2D::OnUpdateDirection(StringHash eventType, VariantMap& eventData)
{
    if (!controller_)
        return;

    if (!UpdateDirection())
    {
        // Reset changing direction
        direction.x_ = controller_->control_.direction_;
    }
}

void GOC_Animator2D::OnChangeState(StringHash eventType, VariantMap& eventData)
{
    /*
    const StringHash& state = eventData[Ai_ChangeOrder::AI_ORDER].GetStringHash();

    /// don't force change state
    if (state != state_)
        SetState(state);*/

    SetState(eventData[Ai_ChangeOrder::AI_ORDER].GetStringHash());
}

void GOC_Animator2D::OnAnimation2D_Particule(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - OnAnimation2D_Particule :  Node=%s(%u)", node_->GetName().CString(), node_->GetID());

    SpawnParticule(eventData);
}

void GOC_Animator2D::OnAnimation2D_Entity(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - OnAnimation2D_Entity :  Node=%s(%u)", node_->GetName().CString(), node_->GetID());

    SpawnEntity(eventData);
}

void GOC_Animator2D::OnAnimation2D_Animation(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - OnAnimation2D_Animation :  Node=%s(%u)", node_->GetName().CString(), node_->GetID());

    if (eventType == SPRITER_ANIMATION)
        SpawnAnimation(eventData);

//    else if (eventType == SPRITER_ANIMATIONINSIDE)
//        SpawnAnimationInside(eventData);
}

void GOC_Animator2D::OnAnimation2D_Light(StringHash eventType, VariantMap& eventData)
{
//    Node* light = GetNode()->GetChild("Light");
//    if (!light)
//    {
////        URHO3D_LOGINFOF("GOC_Animator2D() - OnAnimation2D_Light : AddLight to Node=%s(%u)", GetNode()->GetName().CString(),GetNode()->GetID());
//        GameHelpers::AddLight(GetNode(), LIGHT_SPOT, Vector3(2.0f,4.0f,-5.0f), Vector3(0.0f,-0.3f,1.0f), 90.0f, +10.f, 5.0f, true);
//    }
//    else
//    {
////        URHO3D_LOGINFOF("GOC_Animator2D() - OnAnimation2D_Light : RemoveLight on Node=%s(%u)", GetNode()->GetName().CString(),GetNode()->GetID());
//        light->Remove();
//    }
}

void GOC_Animator2D::OnTouchGround(StringHash eventType, VariantMap& eventData)
{
    /// limit particules (1parteffect/delay)
    if (delayParticule > PARTICULE_INACTIVEDELAY)
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - OnTouchGround : Node=%s(%u) !", GetNode()->GetName().CString(), GetNode()->GetID());
        delayParticule = 0.f;
        Material* material = 0;
        const String* particleEffectName = &ParticuleEffect_[PE_DUST];
        float zf = 0.f;

        if (node_->GetVar(GOA::INFLUID).GetBool())
        {
            particleEffectName = &ParticuleEffect_[PE_BULLEDAIR];
#ifdef ACTIVE_LAYERMATERIALS
            material = GameContext::Get().layerMaterials_[LAYERWATER];
#endif
            zf = 1.f - (ViewManager::GetNearViewZ(animatedSprite->GetLayer()) + LAYER_FLUID) * PIXEL_SIZE;
        }

        GameHelpers::SpawnParticleEffect(context_, *particleEffectName, animatedSprite->GetLayer(), animatedSprite->GetViewMask(),
                                         node_->GetWorldPosition2D(), GetDirection().x_ > 0.f ? 180.f : 0.f, 0.5f, true, 1.f, Color::WHITE, LOCAL, material, zf);
    }
}

void GOC_Animator2D::OnTouchFluid(StringHash eventType, VariantMap& eventData)
{
    /// limit particules (1parteffect/delay)
    if (delayParticule > PARTICULE_INACTIVEDELAY)
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - OnTouchGround : Node=%s(%u) !", GetNode()->GetName().CString(), GetNode()->GetID());

        delayParticule = 0.f;
        const float zf = 1.f - (ViewManager::GetNearViewZ(animatedSprite->GetLayer()) + LAYER_FLUID) * PIXEL_SIZE;
        Material* material = 0;
#ifdef ACTIVE_LAYERMATERIALS
        material = GameContext::Get().layerMaterials_[LAYERWATER];
#endif
        GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_BULLEDAIR], animatedSprite->GetLayer(), animatedSprite->GetViewMask(),
                                         node_->GetWorldPosition2D(), 90.f, 0.5f, true, 1.f, Color::WHITE, LOCAL, material, zf);
    }

    CheckFireLight(eventData);
}


/// PRIVATE Methods

void GOC_Animator2D::SetTemplate()
{
    if (!currentTemplate || (!templateName_.Empty() &&  currentTemplate->name != templateName_))
    {
        StringHash key = StringHash(templateName_);

//        URHO3D_LOGINFOF("GOC_Animator2D() - SetTemplate : %s(%u)", templateName_.CString(), key.Value());

        unsigned k = key.Value();

        ClearCustomTemplate();

        if (GOC_Animator2D_Template::templates_.Contains(k))
            currentTemplate = &(GOC_Animator2D_Template::templates_.Find(k)->second_);
        else
        {
            currentTemplate = &(GOC_Animator2D_Template::templates_.Find(ANIMATOR_TEMPLATE_DEFAULT.Value())->second_);
            URHO3D_LOGWARNINGF("GOC_Animator2D() - SetTemplate  : %s(%u) => %s(%u) Not Found => Apply Default Template !",
                               node_->GetName().CString(), node_->GetID(), templateName_.CString(), k);
        }
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SetTemplate : %s(%u) => apply %s(%u) ... OK !",
                        node_->GetName().CString(), node_->GetID(), currentTemplate->name.CString(), currentTemplate->hashName.Value());
#endif
    }
}

void GOC_Animator2D::ClearCustomTemplate()
{
    if (customTemplate)
    {
        if (currentTemplate)
        {
//            URHO3D_LOGINFOF("GOC_Animator2D() - ClearCustomTemplate : Delete Template Name=%s",currentTemplate->name.CString());
            delete currentTemplate;
            currentTemplate = 0;
        }
        customTemplate = false;
    }
}

void GOC_Animator2D::SetAnimationSet()
{
    if (!animatedSprite)
        return;

    AnimationSet2D* animSet2D = animatedSprite->GetAnimationSet();

    if (!animSet2D && !animatedSprite->GetRenderTargetAttr().Empty())
    {
        if (PlugDrawables())
        {
            animSet2D = animatedSprite->GetAnimationSet();
            if (!animSet2D)
            {
                Vector<String> params = animatedSprite->GetRenderTargetAttr().Split('|');
                String scmlset = params[0];
                String customssheet = params[1];
                AnimationSet2D::customSpritesheetFile_ = customssheet;
                animSet2D = GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>(scmlset);
                AnimationSet2D::customSpritesheetFile_ = String::EMPTY;

                URHO3D_LOGERRORF("GOC_Animator2D() - SetAnimationSet : %s(%u) use Rendered Target AnimatedSprite not already setted (animset=%s) !",
                                 animatedSprite->GetNode()->GetName().CString(), animatedSprite->GetNode()->GetID(), scmlset.CString());
            }
        }
    }

    if (animSet2D)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGERRORF("GOC_Animator2D() - SetAnimationSet entity=%s ... animSet2D=%u currentAnimSet=%u ...", animatedSprite->GetEntityName().CString(), animSet2D->GetNameHash().Value(), currentAnimSet.Value());
#endif

        if (currentAnimSet != animSet2D->GetNameHash())
        {
            currentTemplate->AddAnimationSet(animSet2D);

            currentAnimSet = animSet2D->GetNameHash();

            animInfoIndexes_.Resize(currentTemplate->transitionTable.Size());
            for (unsigned i=0; i < animInfoIndexes_.Size(); i++)
                animInfoIndexes_[i] = 0;
        }

        UpdateEntity();
    }

#ifdef LOGDEBUG_ANIMATOR2D
    if (!animSet2D)
        URHO3D_LOGINFOF("GOC_Animator2D() - SetAnimationSet : No AnimationSet in entity=%s", animatedSprite->GetEntityName().CString());
#endif
}

void GOC_Animator2D::MoveLayer(int layerinc, bool alldrawables)
{
    if (alldrawables)
    {
        for (unsigned i=0; i < animatedSprites.Size(); i++)
        {
            AnimatedSprite2D* drawable = animatedSprites[i];
            drawable->SetLayer(drawable->GetLayer()+layerinc);
        }
    }
    else if (animatedSprite)
    {
        animatedSprite->SetLayer(animatedSprite->GetLayer()+layerinc);
    }
}

void GOC_Animator2D::UpdateEntity()
{
    if (!currentTemplate || !animatedSprite)
        return;

    int entityid = animatedSprite->GetSpriterInstance() ? animatedSprite->GetSpriterInstance()->GetEntity()->id_ : 0;

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGERRORF("GOC_Animator2D() - UpdateEntity : entityname=%s entityid=%d currentAnimSet=%u", animatedSprite->GetEntityName().CString(), entityid, currentAnimSet.Value());
#endif

    currentAnimInfoTable = &(currentTemplate->GetAnimInfoTable(currentAnimSet, entityid));
}


void GOC_Animator2D::CheckAnimation()
{
    if (!animatedSprite || !animatedSprite->GetSpriterInstance())
        return;

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - CheckAnimation : %s(%u) !", node_->GetName().CString(), node_->GetID());
#endif

    if (currentStateIndex >= animInfoIndexes_.Size())
        return;

    int animID = GetCurrentAnimInfo(animInfoIndexes_[currentStateIndex]).animIndex;
    if (animatedSprite->GetSpriterAnimation() != animatedSprite->GetSpriterAnimation(animID))
    {
        ChangeAnimation();
        NotifyCurrentState();
    }
}

void GOC_Animator2D::UpdateSubscriber()
{
    if (subscribeOk_)
        return;

    for (unsigned i=0; i<currentState->hashEventTable.Size(); ++i)
    {
        StringHash event = currentState->hashEventTable[i];
        int sender = currentState->eventTable[i].eventSender;
        if (sender == Sender_Self)
        {
            if (!HasSubscribedToEvent(node_, event))
            {
                SubscribeToEvent(node_, event, URHO3D_HANDLER(GOC_Animator2D, OnEvent));
//                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateSubscriber All - %s(%u) subscribe state=%s(%u) To Event %s sender=self!",
//                            node_->GetName().CString(), node_->GetID(), currentState->name.CString(), currentState->hashName.Value(), GOE::GetEventName(event).CString());
            }
        }
        else if (sender == Sender_Owner && owner_)
        {
            if (!HasSubscribedToEvent(owner_, event))
            {
                SubscribeToEvent(owner_, event, URHO3D_HANDLER(GOC_Animator2D, OnEvent));
//                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateSubscriber All - %s(%u) subscribe state=%s(%u) To Event %s sender=owner=%s(%u)!",
//                            node_->GetName().CString(), node_->GetID(), currentState->name.CString(), currentState->hashName.Value(), GOE::GetEventName(event).CString(), owner_ ? owner_->GetName().CString() : "", owner_ ? owner_->GetID() : 0);
            }
        }
        else if (!HasSubscribedToEvent(event))
        {
            SubscribeToEvent(event, URHO3D_HANDLER(GOC_Animator2D, OnEvent));
//            URHO3D_LOGINFOF("GOC_Animator2D() - UpdateSubscriber All - %s(%u) subscribe state=%s(%u) To Event %s sender=all!",
//                            node_->GetName().CString(), node_->GetID(), currentState->name.CString(), currentState->hashName.Value(), GOE::GetEventName(event).CString());
        }
    }
}

inline void GOC_Animator2D::UpdateSubscriber(const AnimatorState* fromState, const AnimatorState* toState)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - UpdateSubscriber : %s(%u)",
//             node_->GetName().CString(), node_->GetID());

#ifdef LOGDEBUG_ANIMATOR2D
    if (!fromState || !toState)
    {
        URHO3D_LOGERRORF("GOC_Animator2D() - UpdateSubscriber : %s(%u) fromState=%u toState=%u",
                         node_->GetName().CString(), node_->GetID(), fromState, toState);
        return;
    }
#endif

    if (subscribeOk_ && fromState->hashName == toState->hashName)
        return;

    for (unsigned i=0; i<fromState->hashEventTable.Size(); ++i)
    {
        StringHash event = fromState->hashEventTable[i];
        int sender = fromState->eventTable[i].eventSender;

        unsigned numcommonevents = 0;
        for (unsigned j=0; j<toState->hashEventTable.Size(); ++j)
        {
            if (event == toState->hashEventTable[j])
                numcommonevents++;
        }

        if (!numcommonevents)
        {
            if (sender == Sender_Self)
            UnsubscribeFromEvent(node_, event);
            else if (sender == Sender_Owner && owner_)
                UnsubscribeFromEvent(owner_, event);
            else
                UnsubscribeFromEvent(event);
        }
    }

    for (unsigned i=0; i<toState->hashEventTable.Size(); ++i)
    {
        StringHash event = toState->hashEventTable[i];
        int sender = toState->eventTable[i].eventSender;

        if (sender == Sender_Self)
        {
            if (!HasSubscribedToEvent(node_, event))
            SubscribeToEvent(node_, event, URHO3D_HANDLER(GOC_Animator2D, OnEvent));
        }
        else if (sender == Sender_Owner && owner_)
        {
            if (!HasSubscribedToEvent(owner_, event))
                SubscribeToEvent(owner_, event, URHO3D_HANDLER(GOC_Animator2D, OnEvent));
        }
        else if (!HasSubscribedToEvent(event))
        {
            SubscribeToEvent(event, URHO3D_HANDLER(GOC_Animator2D, OnEvent));
        }
    }

//    URHO3D_LOGINFOF("GOC_Animator2D() - UpdateSubscriber State : %s(%u) fromState=%s toState=%s ... OK !",
//                    node_->GetName().CString(), node_->GetID(), fromState->name.CString(), toState->name.CString());
}

bool GOC_Animator2D::UpdateDirection()
{
    if (!controller_)
        return false;

    unsigned moveState = node_->GetVar(GOA::MOVESTATE).GetUInt();
    bool reverseflip = false;

//    URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) ... ", GetNode()->GetName().CString(), GetNode()->GetID());

    if (moveState & MV_CLIMB)
    {
        if (!wallFlipping_.x_ && (moveState & MV_TOUCHWALL) && controller_->control_.IsButtonDown(CTRL_UP|CTRL_DOWN) &&
                ((moveState & MV_DIRECTION && !controller_->control_.IsButtonDown(CTRL_RIGHT)) ||
                 (!(moveState & MV_DIRECTION) && !controller_->control_.IsButtonDown(CTRL_LEFT))))
        {
#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) skip case (no changing direction on climbing wall) !", GetNode()->GetName().CString(), GetNode()->GetID());
#endif
            return false;
        }

        if (roofFlipping_ && (moveState & MV_TOUCHROOF))
        {
            if (alignment_ >= ROTATELEFT)
            {
                controller_->control_.direction_ = direction.x_ = alignment_ == ROTATELEFT ? 1.f : -1.f;
#ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at right|left touchroof change direction=%F",
                                GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_);
#endif
            }
            else if (gocmove_)
            {
                controller_->control_.direction_ = direction.x_ = (float)gocmove_->GetLastDirectionX();
#ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on roof update direction=%F(vel=%F)",
                                GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_, gocmove_->GetLastVelocity().x_);
#endif
            }

            alignment_ = HORIZONTALFLIP;

#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on roof", GetNode()->GetName().CString(), GetNode()->GetID());
#endif
            animatedSprite->SetLocalPosition(node_->GetWorldScale2D() * Vector2(0.f, roofFlipping_));
            animatedSprite->SetLocalRotation(0.f);
            animatedSprite->SetFlipY(true);
        }
        else if (wallFlipping_.x_ && (moveState & MV_TOUCHWALL))
        {
            // Wall On Left Side
            if (moveState & MV_DIRECTION)
            {
                if (alignment_ != ROTATELEFT)
                {
                    if (alignment_ == HORIZONTALFLIP)
                    {
                        direction.x_ = 1.f;
#ifdef LOGDEBUG_ANIMATOR2D
                        URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at left previously on roof change direction=%F",
                                        GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_);
#endif
                    }
                    // Set direction considering the last velocity
                    else if (gocmove_)
                    {
                        direction.x_ = (float)gocmove_->GetLastDirectionY(); //gocmove_->GetLastVelocity().y_ >= -0.1f ? -1.f : 1.f;
#ifdef LOGDEBUG_ANIMATOR2D
                        URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at right change direction=%F(vel=%F)",
                                        GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_, gocmove_->GetLastVelocity().y_);
#endif
                    }
                }

                alignment_ = ROTATELEFT;

#ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at left", GetNode()->GetName().CString(), GetNode()->GetID());
#endif
                animatedSprite->SetLocalPosition(node_->GetWorldScale2D() * Vector2(-wallFlipping_.x_, wallFlipping_.y_));
                animatedSprite->SetLocalRotation(-90.f);
                animatedSprite->SetFlipY(false);
            }
            // Wall On Right Side
            else
            {
                if (alignment_ != ROTATERIGHT)
                {
                    if (alignment_ == HORIZONTALFLIP)
                    {
                        direction.x_ = 1.f;
#ifdef LOGDEBUG_ANIMATOR2D
                        URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at right previously on roof change direction=%F",
                                        GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_);
#endif
                    }
                    // Set direction considering the last velocity
                    else if (gocmove_)
                    {
                        direction.x_ = (float)gocmove_->GetLastDirectionY(); //gocmove_->GetLastVelocity().y_ >= -0.1f ? -1.f : 1.f;
#ifdef LOGDEBUG_ANIMATOR2D
                        URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at right change direction=%F(vel=%F)",
                                        GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_, gocmove_->GetLastVelocity().y_);
#endif
                    }
                }

                alignment_ = ROTATERIGHT;
                reverseflip = true;

#ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at right", GetNode()->GetName().CString(), GetNode()->GetID());
#endif
                animatedSprite->SetLocalPosition(node_->GetWorldScale2D() * Vector2(wallFlipping_.x_, wallFlipping_.y_));
                animatedSprite->SetLocalRotation(90.f);
                animatedSprite->SetFlipY(false);
            }
        }
        else if (alignment_ != HORIZONTAL)
        {
            if (alignment_ >= ROTATELEFT)
            {
                direction.x_ = alignment_ == ROTATELEFT ? 1.f : -1.f;
#ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on bloc at right|left touchground change direction=%F",
                                GetNode()->GetName().CString(), GetNode()->GetID(), direction.x_);
#endif
            }

            alignment_ = HORIZONTAL;

            animatedSprite->SetLocalPosition(Vector2::ZERO);
            animatedSprite->SetLocalRotation(0.f);
            animatedSprite->SetFlipY(false);

#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) on ground", GetNode()->GetName().CString(), GetNode()->GetID());
#endif
        }
    }

    // Apply direction and flipping
    controller_->control_.direction_ = direction.x_;

    animatedSprite->SetFlipX(!reverseflip ? orientation * direction.x_ < 0.f : orientation * direction.x_ > 0.f);
    if (physicFlipX_ != animatedSprite->GetFlipX())
    {
        physicFlipX_ = animatedSprite->GetFlipX();
        GameHelpers::SetPhysicFlipX(node_);
//        URHO3D_LOGINFOF("GOC_Animator2D() - UpdateDirection : Node=%s(%u) ... physflipX=%s ! ", GetNode()->GetName().CString(), GetNode()->GetID(), physicFlipX_ ? "true":"false");
    }

    if (animatedSprites.Size() > 1)
        for (PODVector<AnimatedSprite2D*>::Iterator it=animatedSprites.Begin()+1; it!=animatedSprites.End(); ++it)
            (*it)->SetFlipX(physicFlipX_);

    // notify
    node_->SetVar(GOA::DIRECTION, direction);
    MarkNetworkUpdate();

    return true;
}

inline void GOC_Animator2D::ApplyNewStateAction()
{
    // if exist Apply Action for this state
    if (currentState && currentState->actions.Size())
    {
        for (unsigned i=0; i < currentState->actions.Size(); i ++)
        {
            const Action& action = currentState->actions[i];
            if (action.ptr)
            {

#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - ApplyNewStateAction : Node=%s(%u) Apply Action=%s !", node_->GetName().CString(), node_->GetID(), action.name.CString());
#endif
        (this->*(action.ptr))(action.param);
    }
}
    }
}

inline void GOC_Animator2D::ApplyCurrentStateEventActions(int eventindex, const VariantMap& param)
{
    if (currentState)
    {
        if (currentState->eventTable.Size() <= eventindex)
        {
        #ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGWARNINGF("GOC_Animator2D() - ApplyCurrentStateEventActions node=%s(%u) : state=%s(%u) noaction found at eventIndex=%u !",
                           node_->GetName().CString(), node_->GetID(), currentState->name.CString(), currentState->hashName.Value(), eventindex);
        #endif
            return;
        }

        const Actions& actions = currentState->eventTable[eventindex].actions;
        for (unsigned i=0; i < actions.Size(); i ++)
        {
            const Action& action = actions[i];
            if (action.ptr)
            {
            #ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGINFOF("GOC_Animator2D() - ApplyCurrentStateEventActions : Node=%s(%u) Apply %s !", node_->GetName().CString(), node_->GetID(), action.ToString().CString());
            #endif
                (this->*(action.ptr))(!action.param.Empty() ? action.param : param);
            }
        }
    }
}

inline void GOC_Animator2D::ApplyDirectionalAnimations()
{
    int& animversion = animInfoIndexes_[currentStateIndex];

    if (forceAnimVersion_ != -1)
        forceAnimVersion_ = -1;

    if (spawnAngle_ != 0.f && spawnAngle_ != 180.f)
    {
        if (spawnAngle_ > 15.f && spawnAngle_ < 165.f)
        {
            // 45°
            if (spawnAngle_ < 65.f || spawnAngle_ > 115.f)
                animversion = 1;
            else
                // 90°
                animversion = 2;
        }
        else if (spawnAngle_ < -15.f && spawnAngle_ >= -90.f)
        {
            // -45°
            if (spawnAngle_ > -65.f)
                animversion = 3;
            else
                // -90°
                animversion = 4;
        }
        else if (spawnAngle_ > 195.f && spawnAngle_ <= 270.f)
        {
            // -45°
            if (spawnAngle_ < 245.f)
                animversion = 3;
            else
                // -90°
                animversion = 4;
        }
        else
            // 0°
            animversion = 0;
    }
    else
    {
        if (!controller_)
            return;

        /// Get Direction from controller's buttons
        const ObjectControlLocal& control = controller_->control_;
        if (control.IsButtonDown(CTRL_UP))
        {
            // 45°
            if (control.IsButtonDown(CTRL_LEFT) || control.IsButtonDown(CTRL_RIGHT))
                animversion = 1;
            else
                // 90°
                animversion = 2;
        }
        else if (control.IsButtonDown(CTRL_DOWN))
        {
            // -45°
            if (control.IsButtonDown(CTRL_LEFT) || control.IsButtonDown(CTRL_RIGHT))
                animversion = 3;
            else
                // -90°
                animversion = 4;
        }
        else
            // 0°
            animversion = 0;
    }

    if (animversion < GetCurrentAnimInfoTable().Size())
    {
        animatedSprite->ResetAnimation();
        animatedSprite->SetSpriterAnimation(GetCurrentAnimInfo(animversion).animIndex);
    }
    else
    {
        animversion = 0;
    }

    currentStateTime = 0.f;

//    URHO3D_LOGINFOF("GOC_Animator2D() - ApplyDirectionalAnimations : %s(%u) name=%s animIndex=%d animsize=%u currentStateTime=%f",
//                        node_->GetName().CString(), node_->GetID(),
//                        GetCurrentAnimInfo(animversion).animName.CString(), animversion,
//                        GetCurrentAnimInfoTable().Size(), currentStateTime);
}

inline void GOC_Animator2D::ApplySwitchableAnimations()
{
    int& animversion = animInfoIndexes_[currentStateIndex];

    animversion = (animversion + 1) % GetCurrentAnimInfoTable().Size();

    const AnimInfo& animinfo = GetCurrentAnimInfo(animversion);
    if (!animinfo.animName.Empty())
    {
        if (animatedSprite->GetAnimationIndex() != animinfo.animIndex)
        {
            if (animatedSprites.Size() > 1)
                for (PODVector<AnimatedSprite2D*>::Iterator it=animatedSprites.Begin(); it!=animatedSprites.End(); ++it)
                    (*it)->SetSpriterAnimation(animinfo.animIndex);
            else
                animatedSprite->SetSpriterAnimation(animinfo.animIndex);

            currentStateTime = 0.f;

#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGINFOF("GOC_Animator2D() - ApplySwitchableAnimations : %s(%u) animname=%s animversion=%d/%u animationIndex=%d",
                            node_->GetName().CString(), node_->GetID(), animinfo.animName.CString(), animversion+1, GetCurrentAnimInfoTable().Size(), animinfo.animIndex);
#endif
        }
    }

    if (forceAnimVersion_ == animversion)
        forceAnimVersion_ = -1;
}

inline void GOC_Animator2D::ApplySimpleAnimations()
{
    int& animversion = animInfoIndexes_[currentStateIndex];
    animversion = 0;

    if (forceAnimVersion_ != -1)
        forceAnimVersion_ = -1;

    const AnimInfo& animinfo = GetCurrentAnimInfo(animversion);
    if (!animinfo.animName.Empty())
    {
        if (animatedSprite->GetAnimationIndex() != animinfo.animIndex)
        {
            if (animatedSprites.Size() > 1)
                for (PODVector<AnimatedSprite2D*>::Iterator it=animatedSprites.Begin(); it!=animatedSprites.End(); ++it)
                    (*it)->SetSpriterAnimation(animinfo.animIndex);
            else
                animatedSprite->SetSpriterAnimation(animinfo.animIndex);

            currentStateTime = 0.f;
#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGINFOF("GOC_Animator2D() - ApplySimpleAnimations : %s(%u) name=%s animIndex=%d animsize=%u animationIndex=%d currentStateTime=%f",
                            node_->GetName().CString(), node_->GetID(), animinfo.animName.CString(), animversion, GetCurrentAnimInfoTable().Size(), animinfo.animIndex, currentStateTime);
#endif
        }
    }
#ifdef LOGDEBUG_ANIMATOR2D
    else
    {
        URHO3D_LOGWARNINGF("GOC_Animator2D() - ApplySimpleAnimations : %s(%u) currentStateIndex=%u no animationname !",
                           node_->GetName().CString(), node_->GetID(), currentStateIndex);
    }
#endif
}

inline void GOC_Animator2D::NotifyCurrentState()
{
    unsigned state = currentState->hashName.Value();

    if (controller_) controller_->control_.animation_ = state;

    // Send New Current State
    VariantMap& eventData = context_->GetEventDataMap();
    eventData[Go_ChangeState::GO_STATE] = state;
    node_->SendEvent(GO_CHANGESTATE, eventData);
}

void GOC_Animator2D::Update()
{
    unsigned moveState = node_->GetVar(GOA::MOVESTATE).GetUInt();

    if (moveState)
    {

    if (((moveState & MSK_MV_FLYAIR) == MSK_MV_FLYAIR) && ((moveState & MSK_MV_CANWALKONGROUND) != MSK_MV_CANWALKONGROUND))  // (FLY WITH INAIR) ONLY, SKIP FLY IF CAN WALK AND IS ON GROUND (VAMPIRE)
        currentEvent = (moveState & MV_INMOVE) ? ((moveState & MV_INJUMP) == 0 ? EVENT_FLYDOWN : EVENT_FLYUP) : EVENT_DEFAULT_AIR;

    else if ((moveState & MV_CLIMB) && (moveState & (MV_TOUCHWALL | MV_TOUCHROOF)))
        currentEvent = (moveState & MV_INMOVE) ? EVENT_CLIMB : EVENT_DEFAULT_CLIMB;

    else if ((moveState & MSK_MV_SWIMLIQUID) == MSK_MV_SWIMLIQUID)
        currentEvent = (moveState & MV_INMOVE) ? EVENT_MOVE_FLUID : EVENT_DEFAULT_FLUID;

    else if (moveState & MV_WALK)
    {
        if (moveState & MV_TOUCHGROUND)
            currentEvent = (moveState & MV_INMOVE) ? EVENT_MOVE_GROUND : EVENT_DEFAULT_GROUND;
        else
            currentEvent = (moveState & MV_INJUMP) ? EVENT_JUMP : EVENT_FALL;
    }
    }

    if (lastEvent == currentEvent)
        PlayLoop();
    else
        Dispatch(currentEvent);

    if (toDisappearCounter_ > 0)
    {
        toDisappearCounter_--;
        if (animatedSprite)
            animatedSprite->SetAlpha(Max(animatedSprite->GetAlpha() - 0.025f, 0.f));

        if (toDisappearCounter_ == 0)
            ToDestroy();
    }
    else if (owner_ && followOwnerY_)
    {
        // follow the owner in Y
        node_->SetWorldPosition2D(Vector2(node_->GetWorldPosition2D().x_, owner_->GetWorldPosition2D().y_ - deltaYWithOwner_));
    }
}

inline void GOC_Animator2D::PlayLoop()
{
    if (!currentState)
        return;

    Vector<StringHash>::ConstIterator it = currentState->hashEventTable.Find(AEVENT_TICKLOOP);
    if (it == currentState->hashEventTable.End())
        return;

    /// Net Usage : for Switchable Animation
    if (forceAnimVersion_ != -1)
    {
        if (forceAnimVersion_ != GetCurrentAnimVersion())
        {
            if (currentStateTime >= currentTemplate->transitionTable[currentStateIndex].tickDelay)
            {
//                if (node_->GetID() == 16777274)
//                    URHO3D_LOGINFOF("GOC_Animator2D() - PlayLoop : %s(%u) tickdelay=%F update version current=%u goal=%u ...",
//                                    node_->GetName().CString(), node_->GetID(), currentTemplate->transitionTable[currentStateIndex].tickDelay, GetCurrentAnimVersion()+1, forceAnimVersion_+1);

                ChangeAnimation();
            }
        }
    }

    ApplyCurrentStateEventActions(it - currentState->hashEventTable.Begin());
}

bool GOC_Animator2D::SucceedTransition(AnimatorState* state, unsigned eventIndex) const
{
    bool succeed = true;

    if (eventIndex >= state->eventTable.Size())
    {
        URHO3D_LOGWARNINGF("GOC_Animator2D() - SucceedTransition : State=%s eventIndex(%u) >= state table size(%u) !",
                           state->name.CString(), eventIndex, state->eventTable.Size());
        return succeed;
    }

    const Transition& transition = state->eventTable[eventIndex];
    if (transition.condition == GOA::COND_ACTIVEABILITY)
        succeed = abilities_ ? abilities_->HasActiveAbility(transition.conditionValue) : false;
    else if (transition.condition == GOA::COND_ABILITY)
        succeed = abilities_ ? abilities_->HasAbility(transition.conditionValue) : false;
    else if (transition.condition == GOA::COND_MAXTICKDELAY)
        succeed = currentStateTime < state->tickDelay;
    else if (transition.condition == GOA::COND_BUTTONHOLD)
    {
        succeed = controller_ ? ((controller_->IsButtonHold() && transition.conditionValue == GOA::COND_TRUE) || (!controller_->IsButtonHold() && transition.conditionValue == GOA::COND_FALSE)) : false;
    #ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SucceedTransition Node=%s(%u) : check COND_BUTTONHOLD => controller=%u condval=%u(TRUE=%u,FALSE=%u) buttonhold=%s ... succeed=%s", node_->GetName().CString(), node_->GetID(),
                        controller_ ? "true":"false", transition.condition.Value(), GOA::COND_TRUE.Value(), GOA::COND_FALSE.Value(), controller_->IsButtonHold()? "true":"false", succeed ? "true":"false");
    #endif
    }

    return succeed;
}

inline void GOC_Animator2D::Dispatch(const StringHash& event, const VariantMap& param)
{
    if (!currentState)
        return;

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - Dispatch Node=%s(%u) : event=%s(%u) on currentState=%s ...", node_->GetName().CString(), node_->GetID(),
                    GOE::GetEventName(event).CString(), event.Value(), currentState->name.CString());
#endif
    int looptype;
    Vector<StringHash>::ConstIterator it = currentState->hashEventTable.End();

    if (event == EVENT_CHANGEAREA)
    {
        /// Get nextState in action (FindNextState)
        FindNextState();

        /// Start New State => Event StartLoop
        if (nextState)
        {
            UpdateSubscriber(currentState, nextState);
            currentState = nextState;
            currentStateIndex = currentTemplate->GetStateIndex(currentState->hashName);
            it = currentState->hashEventTable.Find(AEVENT_STARTLOOP);
            looptype = AL_Start;
            ApplyNewStateAction();
            NotifyCurrentState();
        }
    }
    else
    {
        it = currentState->hashEventTable.Find(event);
        if (it != currentState->hashEventTable.End())
        {
            unsigned eventIndex = it - currentState->hashEventTable.Begin();
            unsigned nextStateIndex = currentState->eventTable[eventIndex].nextState;

            if (nextStateIndex != currentStateIndex)
            {
                /// Skip if Event Condition is not Respected.
                if (!SucceedTransition(currentState, eventIndex))
                {
                #ifdef LOGDEBUG_ANIMATOR2D
                    URHO3D_LOGINFOF("GOC_Animator2D() - Dispatch Node=%s(%u) : event=%s(%u) currentStateIndex=%u nextStateIndex=%u check cond=%u => false ... skip !", node_->GetName().CString(), node_->GetID(),
                                    GOE::GetEventName(event).CString(), event.Value(), currentStateIndex, nextStateIndex, currentState->eventTable[eventIndex].condition.Value());
                #endif
                    return;
                }

                /// Function action in EndLoop :: Warning don't use findNextState
                ApplyCurrentStateEventActions(eventIndex, param);

                /// Begin new state => Event StartLoop
                nextState = &(currentTemplate->transitionTable[nextStateIndex]);

                UpdateSubscriber(currentState, nextState);
                currentState = nextState;
                currentStateIndex = nextStateIndex;
                it = currentState->hashEventTable.Find(AEVENT_STARTLOOP);
                looptype = AL_Start;
                ApplyNewStateAction();
                NotifyCurrentState();
            }
            else
            {
                if (event == AEVENT_ENDLOOP)
                {
                    /// Stop State (End) Or Switch : end_loop function or switch
//                if (node_->GetID() == 16777274)
//                URHO3D_LOGINFOF("GOC_Animator2D() - Dispatch : End State - %s(%u) action=%s",
//                         currentState->name.CString(), currentStateIndex,
//                         currentState->eventTable[eventIndex].action.name.CString());

                    /// Get nextState in action (FindNextState)
                    FindNextState();

                    /// Start New State => Event StartLoop
                    if (nextState)
                    {
                        UpdateSubscriber(currentState, nextState);
                        currentState = nextState;
                        currentStateIndex = currentTemplate->GetStateIndex(currentState->hashName);
                        it = currentState->hashEventTable.Find(AEVENT_STARTLOOP);
                        looptype = AL_Start;
                        ApplyNewStateAction();
                        NotifyCurrentState();
                    }
                }
                /// If Same State, Same Event
                else
                {
                    /// Skip if Event Condition is not Respected.
                    if (!SucceedTransition(currentState, eventIndex))
                    {
                        #ifdef LOGDEBUG_ANIMATOR2D
                            URHO3D_LOGINFOF("GOC_Animator2D() - Dispatch Node=%s(%u) : event=%s(%u) currentStateIndex=%u nextStateIndex=%u check cond=%u => false ... skip !", node_->GetName().CString(), node_->GetID(),
                                            GOE::GetEventName(event).CString(), event.Value(), currentStateIndex, nextStateIndex, currentState->eventTable[eventIndex].condition.Value());
                        #endif
                        return;
                    }
                    const AnimatorState& animatorstate = currentTemplate->transitionTable[currentStateIndex];

                    /// check tickdelay for quick replay animation
                    if (currentStateTime > animatorstate.tickDelay && GetCurrentAnimInfoTable().Size() > 1)
                    {
                        /// If Directional Animations
                        if (animatorstate.directionalAnimations_)
                            ApplyDirectionalAnimations();
                        /// If Switchable Animations
                        else if (autoSwitchAnimation_)
                            ApplySwitchableAnimations();
                    }
                    else
                    {
                        /// Play Instant action
                        looptype = AL_Instant;
                    }
                }
            }
        }
        else
        {
            /// if event don't exist, play TickLoop if exist
            it = currentState->hashEventTable.Find(AEVENT_TICKLOOP);
            if (it == currentState->hashEventTable.End())
            {
            #ifdef LOGDEBUG_ANIMATOR2D
                URHO3D_LOGWARNINGF("GOC_Animator2D() - Dispatch : State - %s(%u) NO TICKLOOP",
                        currentState->name.CString(), currentState->hashName.Value());
            #endif
                return;
            }
            looptype = AL_Tick;
        }
    }

    //lastEvent = event;
    lastEvent = currentEvent;

    unsigned eventIndex = it - currentState->hashEventTable.Begin();

    // StartLoop or TickLoop Action
    ApplyCurrentStateEventActions(eventIndex, param);

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - Dispatch Node=%s(%u) : event=%s(%u) %s on currentState=%s entity=%s ... OK !", node_->GetName().CString(), node_->GetID(),
                    GOE::GetEventName(event).CString(), event.Value(), animLoopStrings_[looptype], currentState->name.CString(), animatedSprite->GetEntity().CString());
#endif
}



/// Actions

inline void GOC_Animator2D::FindNextState(const VariantMap& param)
{
    unsigned moveState = node_->GetVar(GOA::MOVESTATE).GetUInt();

    if (!forceNextState)
    {
        if (moveState == 0)
        {
            nextState = currentTemplate->GetStateFromEvent(EVENT_DEFAULT_GROUND);
        }
        else
        {
            if ((moveState & MSK_MV_FLYAIR) == MSK_MV_FLYAIR && (moveState & MSK_MV_CANWALKONGROUND) != MSK_MV_CANWALKONGROUND)  // (FLY WITH INAIR) ONLY, SKIP FLY IF CAN WALK AND IS ON GROUND (VAMPIRE)
                nextState = (moveState & MV_INMOVE) ? currentTemplate->GetStateFromEvent((moveState & MV_INJUMP) == 0 ? EVENT_FLYDOWN : EVENT_FLYUP) : currentTemplate->GetStateFromEvent(EVENT_DEFAULT_AIR);

            else if ((moveState & MV_CLIMB) && (moveState & (MV_TOUCHWALL | MV_TOUCHROOF)))
                nextState = (moveState & MV_INMOVE) ?  currentTemplate->GetStateFromEvent(EVENT_CLIMB) : currentTemplate->GetStateFromEvent(EVENT_DEFAULT_CLIMB);

            else if ((moveState & MSK_MV_SWIMLIQUID) == MSK_MV_SWIMLIQUID)
                nextState = (moveState & MV_INMOVE) ? currentTemplate->GetStateFromEvent(EVENT_MOVE_FLUID) : currentTemplate->GetStateFromEvent(EVENT_DEFAULT_FLUID);

            else if (moveState & MV_WALK)
            {
                if (moveState & MV_TOUCHGROUND)
                    nextState = (moveState & MV_INMOVE) ? currentTemplate->GetStateFromEvent(EVENT_MOVE_GROUND) : currentTemplate->GetStateFromEvent(EVENT_DEFAULT_GROUND);
                else
                    nextState = (moveState & MV_INJUMP) ? currentTemplate->GetStateFromEvent(EVENT_JUMP) : currentTemplate->GetStateFromEvent(EVENT_FALL);
            }
        }
    }
    else
    {
        nextState = forceNextState;
        forceNextState = 0;
    }

    if (!nextState)
    {
        nextState = currentTemplate->GetState(StringHash(STATE_DEFAULT_GROUND));
    }

#ifdef LOGDEBUG_ANIMATOR2D
//    if (!nextState)
    {
        URHO3D_LOGERRORF("GOC_Animator2D() - FindNextState : Node=%s(%u) currentState=%s nextState=%s moveState=%s(%u)",
                         node_->GetName().CString(), node_->GetID(), currentState->name.CString(), nextState ? nextState->name.CString() : "None",
                         GameHelpers::GetMoveStateString(moveState).CString(), moveState);
    }
#endif
}

void GOC_Animator2D::ChangeEntity(const VariantMap& param)
{
    if (!animatedSprite)
        return;

    if (!animatedSprite->GetSpriterInstance() || animatedSprite->GetSpriterInstance()->GetNumEntities() < 2)
        return;

    int entityid = ToInt(param.Begin()->second_.GetString()) % Min(animatedSprite->GetSpriterInstance()->GetNumEntities(), MaxEntityIds);

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGERRORF("GOC_Animator2D() - ChangeEntity : Node=%s(%u) change to entity=%s index=%d",
                     node_->GetName().CString(), node_->GetID(), animatedSprite->GetEntityName().CString(), entityid);
#endif

    if (entityid == animatedSprite->GetSpriterInstance()->GetEntity()->id_)
        return;

    // Spawn Effect
    GameHelpers::SpawnParticleEffect(context_, ParticuleEffect_[PE_LIFEFLAME], animatedSprite->GetLayer(), animatedSprite->GetViewMask(), node_->GetWorldPosition2D(), 0.f, 1.f, true, 2.f, Color::BLUE, LOCAL);

    entityid |= SetEntityFlag;
    GameHelpers::SetEntityVariation(animatedSprite, entityid);

    if (currentTemplate)
        FindNextState(context_->GetEventDataMap(false));

    node_->SendEvent(CHARACTERUPDATED);
}

void GOC_Animator2D::ChangeAnimation(const VariantMap& param)
{
    if (currentState==0)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGWARNINGF("GOC_Animator2D() - ChangeAnimation : Node=%s(%u) - change to %s(stateIndex=%u) no CurrentState !",
                           GetNode()->GetName().CString(), GetNode()->GetID(), GetCurrentAnimInfo(animInfoIndexes_[currentStateIndex]).animName.CString(), currentStateIndex);
#endif
        return;
    }

    if (currentStateIndex >= animInfoIndexes_.Size())
    {
        URHO3D_LOGERRORF("GOC_Animator2D() - ChangeAnimation : Node=%s(%u) - change to %s(%u) hashName=%u => animIndexSize(%u) < currentStateIndex(%u) !",
                         node_->GetName().CString(), node_->GetID(), currentState->name.CString(), StringHash(currentState->name).Value(), currentState->hashName.Value(), animInfoIndexes_.Size(), currentStateIndex);
        return;
    }

#ifdef LOGDEBUG_ANIMATOR2D
    else
        URHO3D_LOGINFOF("GOC_Animator2D() - ChangeAnimation : Node=%s(%u) - change to %s(stateIndex=%u) (Name=%s(%u) hashName=%u)",
                        node_->GetName().CString(), node_->GetID(), GetCurrentAnimInfo(animInfoIndexes_[currentStateIndex]).animName.CString(), currentStateIndex, currentState->name.CString(),
                        StringHash(currentState->name).Value(), currentState->hashName.Value());
#endif

    if (!GetCurrentAnimInfoTable().Size())
        return;

    /// If Directional Animation
    if (currentTemplate->transitionTable[currentStateIndex].directionalAnimations_)
        ApplyDirectionalAnimations();
    /// If Switchable Animation
    else if (autoSwitchAnimation_)
        ApplySwitchableAnimations();
    /// Simple Animation
    else
        ApplySimpleAnimations();

    if (currentStateTime == 0.f)
        netChangeCounter_++;

    bool ok = UpdateDirection();
}

inline void GOC_Animator2D::CheckTimer(const VariantMap& param)
{
    currentStateTime += timeStep;
    if (currentStateTime >= currentTemplate->transitionTable[currentStateIndex].tickDelay)
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - CheckTimer : Node=%s(%u) - State=%s - Timer finished END_LOOP",
//                 GetNode()->GetName().CString(),GetNode()->GetID(),currentState->name.CString());
        Dispatch(AEVENT_ENDLOOP);
        currentStateTime = 0.0f;
    }
}

inline void GOC_Animator2D::CheckAnim(const VariantMap& param)
{
#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - CheckAnim : Node=%s(%u) - state=%s ... currentStateIndex=%u < animInfoIndexes_.Size()=%u !",
              GetNode()->GetName().CString(), GetNode()->GetID(), currentState->name.CString(), currentStateIndex, animInfoIndexes_.Size());
#endif

    if (currentStateIndex < animInfoIndexes_.Size())
    {
    const AnimInfo& currentAnimInfo = GetCurrentAnimInfo(animInfoIndexes_[currentStateIndex]);

//    URHO3D_LOGINFOF("GOC_Animator2D() - CheckAnim : Node=%s(%u) - time=%F/%F",
//              GetNode()->GetName().CString(), GetNode()->GetID(), animatedSprite->GetCurrentAnimationTime(),
//              currentAnimInfo.animLength - timeStep);

    // Test End Animation on AnimatedSprite
    if (animatedSprite->GetCurrentAnimationTime() >= currentAnimInfo.animLength - timeStep)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - CheckAnim : Node=%s(%u) - Animation Finished END_LOOP animationtime=%F (animationlength=%F) timeStep=%F",
                        GetNode()->GetName().CString(), GetNode()->GetID(), animatedSprite->GetCurrentAnimationTime(), currentAnimInfo.animLength, timeStep);
#endif
        Dispatch(AEVENT_ENDLOOP);
        currentStateTime = 0.f;
    }
    else
    {
        // Update for replaying Animation in the same state (see Dispatch - State Attack)
        currentStateTime += timeStep;
    }
}
}

inline void GOC_Animator2D::CheckEmpty(const VariantMap& param)
{
    bool empty = false;

    GOC_Inventory* inventory = node_->GetComponent<GOC_Inventory>();
    if (inventory)
        empty = inventory->CheckEmpty();

    node_->SendEvent(empty ? GO_INVENTORYEMPTY : GO_INVENTORYFULL);
}

inline void GOC_Animator2D::SendAnimEvent(const VariantMap& param)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - SendAnimEvent : Node=%s(%u) event=%s", GetNode()->GetName().CString(), GetNode()->GetID(),
//                      GOE::GetEventName(param[ANIMEVENT::DATAS].GetStringHash()).CString());
    VariantMap::ConstIterator itp = param.Find(ANIM_Event::DATAS);
    if (itp != param.End())
        node_->SendEvent(itp->second_.GetStringHash());
}

extern const char* ParticuleEffectTypeNames_[];
void GOC_Animator2D::SpawnParticule(const VariantMap& param)
{
#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnParticule : Node=%s(%u) ...", GetNode()->GetName().CString(), GetNode()->GetID());
#endif

    /// limit particules (1parteffect/delay)
    if (delayParticule > PARTICULE_INACTIVEDELAY)
    {
        delayParticule = 0.f;
        /// TODO : with SequencedSprite need too! crash if no animatedsprite
        if (!animatedSprite)
            return;

        const EventTriggerInfo& triggerInfo = animatedSprite->GetEventTriggerInfo();
        const int effectid = param.Size() ? GetEnumValue(param.Front().second_.GetString(), ParticuleEffectTypeNames_) : triggerInfo.type_.Value();
        float duration = param.Size() ? 1.f : (float)triggerInfo.type2_.Value() * 0.1f;

        // Set angle in case of flipping sprite
        float angle = triggerInfo.rotation_;
        if (angle > 180.f)
            angle = angle - 360.f;
        if (GetDirection().x_ * GetDefaultOrientationAttr() < 0.f)
            angle = 180.f - angle;

        if (effectid > -1)

        GameHelpers::SpawnParticleEffectInNode(context_, node_, ParticuleEffect_[effectid], animatedSprite->GetLayer(), animatedSprite->GetViewMask(), triggerInfo.position_, angle, 1.f, true, duration, Color::WHITE, LOCAL);
        else
            URHO3D_LOGERRORF("GOC_Animator2D() - SpawnParticule : Node=%s(%u) effect=%s(%d) angle=%f trigangle=%f zindex=%d duration=%f!",
                         GetNode()->GetName().CString(), GetNode()->GetID(), ParticuleEffect_[effectid].CString(), effectid, angle, triggerInfo.rotation_, triggerInfo.zindex_, duration);
    }
}

static PhysicEntityInfo sPhysicInfo_;
static SceneEntityInfo sSceneInfo_;


void GOC_Animator2D::SpawnEntity(const VariantMap& param)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (spawnEntityMode_ != SPAWNENTITY_ALWAYS)
    {
        if (spawnEntityMode_ == SPAWNENTITY_SKIPONCE)
            spawnEntityMode_ = SPAWNENTITY_ALWAYS;
        return;
    }

    const EventTriggerInfo& triggerInfo = animatedSprite->GetEventTriggerInfo();

    if (!triggerInfo.type_)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGWARNINGF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) no GOT=%u !", node_->GetName().CString(), node_->GetID(), triggerInfo.type_.Value());
#endif
        spawnAngle_ = 0.f;
        return;
    }

//    if (!GameContext::Get().LocalMode_)
//    {
//        // prevent the double spawn with the NetSpawning
//        if (GameNetwork::Get()->NetSpawnControlAlreadyUsed(node_->GetVar(GOA::CLIENTID).GetInt()))
//        {
//            URHO3D_LOGERRORF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) has already used a netspawncontrol !", node_->GetName().CString(), node_->GetID());
//            return;
//        }
//    }

    Node* node = 0;

    Ability* ability = abilities_ ? abilities_->GetAbility(triggerInfo.type_) : 0;
    if (ability)
    {
        URHO3D_LOGINFOF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) Try to use ability for spawning entity=%s ...", node_->GetName().CString(), node_->GetID(), GOT::GetType(triggerInfo.type_).CString());

        node = ability->Use(node_->GetWorldPosition2D() + Vector2(GetDirection().x_ * GetDefaultOrientationAttr(), 0.f));
        if (!node)
        {
#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGWARNINGF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) noSpawn !", node_->GetName().CString(), node_->GetID());
#endif
            spawnAngle_ = 0.f;
            return;
        }

        // In "Bad" case from Ability that never generates node but return 1 if successfull spawning
        if (node == (Node*)1)
        {
            spawnAngle_ = 0.f;
            return;
        }
    }
    else
    {
        // PhysicInfo Position
        sPhysicInfo_.positionx_ = triggerInfo.position_.x_;
        sPhysicInfo_.positiony_ = triggerInfo.position_.y_;
        // PhysicInfo Direction
        sPhysicInfo_.direction_ = GetDirection().x_;

        // Angle
        float angle, cangle;

        // Use spawnAngle_
        if (spawnAngle_ != 0.f)
        {
            Vector2 direction = shootTarget_ - triggerInfo.position_;
            // angle in degree
            cangle = Atan(direction.y_/direction.x_);
            if (direction.x_ < 0.f)
                cangle += 180.f;
        }
        // Use Trigger Angle
        else
        {
            cangle = triggerInfo.rotation_;
            // correct triggerAngle to conform to Spawn Angle
            if (cangle < 0.f)
                cangle = 360.f + cangle;
            else if (cangle > 270.f)
                cangle -= 360.f;
        }
        // GameHelpers::SetPhysicProperties operate an Node::SetRotation2D with the following angle
        // so correct the angle if the direction is inversed
        angle = cangle;
        if (angle > 90.f)
            angle = 180.f - angle;
        if (sPhysicInfo_.direction_ < 0.f)
            angle = -angle;
        sPhysicInfo_.rotation_ = angle;

        // PhysicInfo Velocity
        if (!triggerInfo.datas_.Empty())
        {
            float velocity = ToFloat(triggerInfo.datas_);
            sPhysicInfo_.velx_ = Sign(sPhysicInfo_.direction_) * velocity * Cos(sPhysicInfo_.rotation_);
            sPhysicInfo_.vely_ = Sign(sPhysicInfo_.direction_) * velocity * Sin(sPhysicInfo_.rotation_);
        }

#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGINFOF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) ... Try to spawn=%s dirx=%f rotation=%f (trigAngle=%f spawnAngle=%f cangle=%f)...",
                        node_->GetName().CString(), node_->GetID(), GOT::GetType(triggerInfo.type_).CString(), sPhysicInfo_.direction_, sPhysicInfo_.rotation_, triggerInfo.rotation_, spawnAngle_, cangle);
#endif

        const GOTInfo& gotinfo = GOT::GetConstInfo(triggerInfo.type_);
        if (gotinfo.pooltype_ == GOT_GOPool)
        {
            URHO3D_LOGINFOF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) ... Try to use static ability for spawning entity=%s ...", node_->GetName().CString(), node_->GetID(), GOT::GetType(triggerInfo.type_).CString());
            node = Ability::Use(triggerInfo.type_, node_, sPhysicInfo_, gotinfo.replicatedMode_);
        }
        else
        {
            // SceneInfo
            sSceneInfo_.faction_ = node_->GetVar(GOA::NOCHILDFACTION).GetBool() ? 0 : node_->GetVar(GOA::FACTION).GetUInt();
            sSceneInfo_.zindex_ = triggerInfo.zindex_;
            sSceneInfo_.ownerNode_ = node_;

            URHO3D_LOGINFOF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) Try to spawn entity=%s entityid=%u ... dirx=%f rot=%f faction=%u", node_->GetName().CString(), node_->GetID(), GOT::GetType(triggerInfo.type_).CString(), triggerInfo.entityid_, GetDirection().x_, sSceneInfo_.faction_);
            node = World2D::SpawnEntity(triggerInfo.type_, triggerInfo.entityid_ | SetEntityFlag, 0, node_->GetID(), node_->GetVar(GOA::ONVIEWZ).GetInt(), sPhysicInfo_, sSceneInfo_);
        }

        if (!node)
        {
//    #ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGWARNINGF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) noSpawn !", node_->GetName().CString(), node_->GetID());
//    #endif
            spawnAngle_ = 0.f;
            return;
        }

        GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
        if (animator)
        {
            animator->SetDirection(Vector2(sPhysicInfo_.direction_, 0.f));
        }
        else
        {
            StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
            if (sprite)
                sprite->SetFlip(GetDirection().x_ < 0.f, false);
        }

        // Set Velocity only here and in GameNetwork::Server_CommandCreateRequestObject
        GameHelpers::SetPhysicVelocity(node, sPhysicInfo_.velx_, sPhysicInfo_.vely_);

        // Reset Spawn Angle
        spawnAngle_ = 0.f;
    }

    node->SetVar(GOA::OWNERID, node_->GetID());

#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnEntity : Node=%s(%u) ... spawn=%s(%u) zindex=%d dir=%s position=%f,%f ... OK !",
                    node_->GetName().CString(), node_->GetID(), node->GetName().CString(), node->GetID(),
                    triggerInfo.zindex_, GetDirection().ToString().CString(), triggerInfo.position_.x_, triggerInfo.position_.y_);
#endif
}

void GOC_Animator2D::SpawnAnimation(const VariantMap& param)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnAnimation : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

    const EventTriggerInfo& triggerInfo = animatedSprite->GetEventTriggerInfo();

    if (!triggerInfo.type_)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGWARNINGF("GOC_Animator2D() - SpawnAnimation : Node=%s(%u) no GOT !", node_->GetName().CString(), node_->GetID());
#endif
        return;
    }

    Node* localScene = node_->GetScene()->GetChild("LocalScene");
    Node* node = 0;

    if (ObjectPool::Get())
    {
        int entityid = triggerInfo.entityid_;
        node = ObjectPool::CreateChildIn(triggerInfo.type_, entityid, localScene);
    }

    if (!node)
    {
        node = localScene->CreateChild(String::EMPTY, LOCAL);
        if (!GameHelpers::LoadNodeXML(context_, node, GOT::GetObjectFile(triggerInfo.type_).CString(), LOCAL))
        {
            node->Remove();
#ifdef LOGDEBUG_ANIMATOR2D
            URHO3D_LOGWARNINGF("GOC_Animator2D() - SpawnAnimation : type=%s(%u) LoadNodeXML Error !", GOT::GetType(triggerInfo.type_).CString(), triggerInfo.type_.Value());
#endif
            return;
        }
    }

    if (!node)
    {
#ifdef LOGDEBUG_ANIMATOR2D
        URHO3D_LOGWARNINGF("GOC_Animator2D() - SpawnAnimation : Node=%s(%u) noSpawn !", node_->GetName().CString(), node_->GetID());
#endif
        return;
    }

    node->SetWorldPosition2D(triggerInfo.position_);

//    node->SetWorldScale(node_->GetWorldScale());
    node->SetEnabled(true);

    GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
    if (animator)
    {
        animator->SetDirection(GetDirection());
    }
    else
    {
        StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
        if (sprite)
            sprite->SetFlipX(GetDirection().x_ < 0.f);
    }

    Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
    if (drawable)
    {
        drawable->SetLayer2(animatedSprite->GetLayer2());
        drawable->SetOrderInLayer(triggerInfo.zindex_);
        drawable->SetViewMask(node_->GetDerivedComponent<Drawable2D>()->GetViewMask());
    }
#ifdef LOGDEBUG_ANIMATOR2D
    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnAnimation : Node=%s(%u) ... spawn=%s(%u) dir=%s ... OK !",
                    node_->GetName().CString(), node_->GetID(), node->GetName().CString(), node->GetID(), GetDirection().ToString().CString());
#endif
}

void GOC_Animator2D::SpawnFurniture(const VariantMap& param)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnFurniture : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

    VariantMap::ConstIterator it = param.Find(ANIM_Event::DATAS);
    if (it != param.End())
    {
        StringHash got = it->second_.GetStringHash();
        int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();
        viewZ = viewZ >= FRONTVIEW ? OUTERBIOME : viewZ >= BACKVIEW ? BACKINNERBIOME : BACKBIOME;

        URHO3D_LOGINFOF("GOC_Animator2D() - SpawnFurniture : Node=%s(%u) got=%s(%u) pos=%s viewZ=%d ... ",
                        node_->GetName().CString(), node_->GetID(), GOT::GetType(got).CString(), got.Value(), node_->GetWorldPosition2D().ToString().CString(), viewZ);

        World2D::GetWorld()->SpawnFurniture(got, node_->GetWorldPosition2D(), viewZ, false);
    }
}

//void GOC_Animator2D::SpawnAnimationInside(const VariantMap& param)
//{
//    const EventTriggerInfo& triggerInfo = animatedSprite->GetEventTriggerInfo();
//
//    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnAnimationInside : Node=%s(%u) inside=%s(%u) type=%u zindex=%d",
//                   node_->GetName().CString(), node_->GetID(), triggerInfo.node_ ? triggerInfo.node_->GetName().CString() : "None", triggerInfo.node_ ? triggerInfo.node_->GetID() : 0, triggerInfo.type_.Value(), triggerInfo.zindex_);
//
//    if (!triggerInfo.node_)
//        return;
//
//    Node* gotObj = 0;
//
//    if (triggerInfo.type_ != StringHash::ZERO)
//        gotObj = GOT::GetObject(triggerInfo.type_);
//
//    else if (triggerInfo.type2_ != StringHash::ZERO)
//    {
//        /// TODO : prevent at each tick to check for inventory
//
//        GOC_Inventory* gocinventory = node_->GetComponent<GOC_Inventory>();
//        if (gocinventory)
//        {
//            int slotindex = gocinventory->GetSlotIndex(triggerInfo.type2_);
//            if (slotindex != -1) gotObj = GOT::GetObject(gocinventory->GetSlot(slotindex).type_);
//        }
//    }
//
//    if (!gotObj)
//        return;
//
//    AnimatedSprite2D* gotAnim = gotObj->GetComponent<AnimatedSprite2D>();
//
////    URHO3D_LOGINFOF("GOC_Animator2D() - SpawnAnimationInside : Node=%s(%u) inside=%u type=%s(%u) type2=%u zindex=%d animated=%u ...",
////                    GetNode()->GetName().CString(), GetNode()->GetID(), triggerInfo.node_, GOT::GetType(triggerInfo.type_ ).CString(), triggerInfo.type2_.Value(), triggerInfo.type_.Value(), triggerInfo.zindex_, gotAnim);
//
//    if (!gotAnim)
//        return;
//
//	AnimatedSprite2D* anim = triggerInfo.node_->GetOrCreateComponent<AnimatedSprite2D>();
//	if (!anim)
//		return;
//
//    anim->SetRenderEnable(false, triggerInfo.zindex_);
//    anim->SetAnimationSet(gotAnim->GetAnimationSet());
//    anim->SetAnimation("use");
//
//    triggerInfo.node_->SetEnabled(true);
//    triggerInfo.node_->SetScale2D(Vector2::ONE);
//}

void GOC_Animator2D::LightOn(const VariantMap& param)
{
//    URHO3D_LOGERRORF("GOC_Animator2D() - LightOn : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

//    if (GameContext::Get().gameConfig_.enlightScene_)
    {
        node_->SetVar(GOA::LIGHTSTATE, true);
        bool lightstate = GameHelpers::SetLightActivation(node_);
        node_->SetVar(GOA::LIGHTSTATE, lightstate);
    }
}

void GOC_Animator2D::LightOff(const VariantMap& param)
{
//    URHO3D_LOGERRORF("GOC_Animator2D() - LightOff : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

//    if (GameContext::Get().gameConfig_.enlightScene_)
    {
        node_->SetVar(GOA::LIGHTSTATE, false);
        bool lightstate = GameHelpers::SetLightActivation(node_);
        node_->SetVar(GOA::LIGHTSTATE, lightstate);
//        Light* light = GetComponent<Light>();
//        if (light)
//            light->SetEnabled(false);
    }
}

void GOC_Animator2D::CheckFireLight(const VariantMap& param)
{
    if (!currentState || !IsEnabledEffective())
        return;

    // GOA::INFLUID is used only when a fluid is touched by the node => use the method 1 in GameHelpers::SetLightActivation().
    // if the method is called without Go_CollideFluid::GO_WETTED (interactive mode or update attributes) then remove GOA::INFLUID
    VariantMap::ConstIterator itp = param.Find(Go_CollideFluid::GO_WETTED);
    if (itp != param.End())
        node_->SetVar(GOA::INFLUID, itp->second_.GetBool());
    else
        node_->RemoveVar(GOA::INFLUID);

    if (currentState->hashName.Value() == STATE_LIGHTED || currentState->hashName.Value() == STATE_UNLIGHTED)
    {
        bool lightWanted = currentState->hashName.Value() == STATE_LIGHTED;
        node_->SetVar(GOA::LIGHTSTATE, lightWanted);

        int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();
        if (!viewZ)
            node_->SetVar(GOA::ONVIEWZ, node_->GetParent()->GetVar(GOA::ONVIEWZ).GetInt());

//        URHO3D_LOGERRORF("GOC_Animator2D() - CheckFireLight : Node=%s(%u) currentanimstate=%s ... wantedlightstate=%s ... viewZ=%d ...",
//                         node_->GetName().CString(), node_->GetID(), currentState->name.CString(), lightWanted ? "true":"false", node_->GetVar(GOA::ONVIEWZ).GetInt());

        bool lighted = GameHelpers::SetLightActivation(node_);

        SetState(StringHash(lighted ? STATE_LIGHTED : STATE_UNLIGHTED));

        node_->SetVar(GOA::LIGHTSTATE, lighted);

        URHO3D_LOGDEBUGF("GOC_Animator2D() - CheckFireLight : Node=%s(%u) currentanimstate=%s ... lighted=%s ... OK !",
                         node_->GetName().CString(), node_->GetID(), currentState->name.CString(), lighted ? "true":"false");
    }

    if (animatedSprites.Size())
    {
        const StringHash& cmap = node_->GetVar(GOA::INFLUID).GetBool() ? CMAP_NOFIRE : CMAP_FIRE;

        for (unsigned i = 0; i < animatedSprites.Size(); i++)
        {
            if (animatedSprites[i]->HasCharacterMap(CMAP_FIRE))
            {
                animatedSprites[i]->ApplyCharacterMap(cmap);
//				URHO3D_LOGINFOF("GOC_Animator2D() - CheckFireLight : Node=%s(%u) currentanimstate=%s isWetted=%s ... apply cmap=%s to node=%s(%u)",
//								node_->GetName().CString(), node_->GetID(), currentState->name.CString(),
//								node_->GetVar(GOA::INFLUID).GetBool() ? "true":"false", node_->GetVar(GOA::INFLUID).GetBool() ? "CMAP_NOFIRE":"CMAP_FIRE",
//								animatedSprites[i]->GetNode()->GetName().CString(), animatedSprites[i]->GetNode()->GetID());
            }
        }
    }
}

void GOC_Animator2D::ToDisappear(const VariantMap& param)
{
    if (toDisappearCounter_ != -1)
        return;

    if (param.Size())
    {
        // check if owner and no button hold
        if (owner_)
        {
            VariantMap::ConstIterator it = param.Find(GOA::COND_BUTTONHOLD);
            if (it != param.End())
            {
                GOC_Controller* ownercontroller = owner_->GetDerivedComponent<GOC_Controller>();
//                URHO3D_LOGERRORF("GOC_Animator2D() - ToDisappear : %s(%u) owner=%s(%u) ownerController=%u check COND_BUTTONHOLD=%s ControllerIsButtonHold=%s(%u) !",
//                                 GetNode()->GetName().CString(), GetNode()->GetID(), owner_ ? owner_->GetName().CString() : "", owner_ ? owner_->GetID() : 0, ownercontroller,
//                                 it->second_.GetBool() ? "true":"false", ownercontroller ? (ownercontroller->IsButtonHold() ? "true":"false") : "true",
//                                 ownercontroller ? ownercontroller->GetButtonHoldTime() : 0U);

                if (ownercontroller && ownercontroller->IsButtonHold() != it->second_.GetBool())
                    return;
            }
        }

//        URHO3D_LOGERRORF("GOC_Animator2D() - ToDisappear : %s(%u) paramSize=%u...", GetNode()->GetName().CString(), GetNode()->GetID(), param.Size());
//        GameHelpers::DumpVariantMap(param);
    }

    SequencedSprite2D* sequencedsprite = node_->GetComponent<SequencedSprite2D>();
    if (sequencedsprite)
        sequencedsprite->SetShrink(true);

    toDisappearCounter_ = 100;
}

void GOC_Animator2D::ToDestroy(const VariantMap& param)
{
//    URHO3D_LOGINFOF("GOC_Animator2D() - ToDestroy : Node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (GOT::GetTypeProperties(node_->GetVar(GOA::GOT).GetStringHash()) & GOT_Effect)
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - ToDestroy : Node=%s(%u) Restore Effect to the Pool", node_->GetName().CString(), node_->GetID());
        World2D::RemoveEntity(ShortIntVector2(node_->GetVar(GOA::ONMAP).GetUInt()), node_->GetID());
        TimerRemover::Get()->Start(node_, 0.f, POOLRESTORE);
        return;
    }

    GOC_Destroyer* destroyer = node_->GetComponent<GOC_Destroyer>();
    if (destroyer && destroyer->HasSubscribedToEvent(node_, WORLD_ENTITYDESTROY))
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - ToDestroy : Node=%s(%u) Send WORLD_ENTITYDESTROY", node_->GetName().CString(), node_->GetID());
        node_->SendEvent(WORLD_ENTITYDESTROY);
    }
    else
    {
//        URHO3D_LOGINFOF("GOC_Animator2D() - ToDestroy : Node=%s(%u) Send GO_DESTROY", node_->GetName().CString(), node_->GetID());
        VariantMap& eventData = context_->GetEventDataMap();
        eventData[Go_Destroy::GO_PTR] = node_;
        node_->SendEvent(GO_DESTROY, eventData);
    }
    toDisappearCounter_ = -1;
}


void GOC_Animator2D::DumpTemplate() const
{
    if (!currentTemplate)
        return;

    currentTemplate->Dump();
}

