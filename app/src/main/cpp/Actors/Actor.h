#pragma once

#include "DefsGame.h"
#include "Ability.h"
#include "GOC_Abilities.h"

#include "DefsEntityInfo.h"

namespace Urho3D
{
class Connection;
}

using namespace Urho3D;

enum ActorState
{
    Desactivated = 0,
    Activated,
    Dead
};

// used by World2D
struct ActorInfo
{
    unsigned actorID_;
    StringHash type_;
    StringHash dialog_;
    unsigned char state_;
    unsigned char entityid_;
    String name_;

    WorldMapPosition position_;
};

class ActorStats
{
public :
    static void InitTable();

    ActorStats();
    unsigned& Value(const StringHash& category, const StringHash& stat);
    unsigned Value(const StringHash& category, const StringHash& stat) const;

    void DumpStat(const StringHash& category, const StringHash& stat) const;
    void DumpAllStats() const;

private :
    unsigned GetCategoryIndex(const StringHash& category) const;

    Vector<HashMap<StringHash, unsigned> > stats_;
    static Vector<StringHash> categories_;
};

class ViewManager;
struct Dialogue;
class DialogMessage;
class UIPanel;

class Actor : public Object
{
    URHO3D_OBJECT(Actor, Object);

public :
    Actor();
    Actor(Context* context);
    Actor(Context* context, unsigned id);
    Actor(Context* context, const StringHash& type);
    Actor(Context* context, const String& name);
    Actor(const Actor& actor);
    ~Actor();

    bool operator == (const Actor& rhs) const
    {
        return info_.type_ == rhs.info_.type_ && info_.name_ == rhs.info_.name_;
    }
    bool operator != (const Actor& rhs) const
    {
        return info_.type_ != rhs.info_.type_ || info_.name_ != rhs.info_.name_;
    }

    static void RegisterObject(Context* context);
    static void RemoveActor(unsigned id);
    static void RemoveActors();
    static void AddActor(Actor* actor, unsigned id=0);
    static bool LoadJSONFile(const String& name);
    static bool SaveJSONFile(const String& name);

    static unsigned GetNumActors();
    static const Vector<WeakPtr<Actor> >& GetActors();
    static Actor* Get(unsigned id);
    static Actor* Get(const StringHash& got);
    static Actor* GetWithNode(Node* node);

    /// Setters
    void SetController(bool main=false, Connection* connection=0, unsigned nodeid=0, int clientid=0);
    void SetName(const String& name);
    void SetObjectType(const StringHash& type, unsigned char entityid=0);
    void SetObjectFile(const String& filename);
    void SetInfo(const ActorInfo& info);
    void SetEnableStats(bool enable);
    void SetViewZ(int viewZ);
    void SetScene(Scene* scene, const Vector2& position=Vector2::ZERO, int viewZ = 0);
    void SetControlID(int ID)
    {
        controlID_ = ID;
    }
    void SetAnimationSet(const String& nameSet = String::EMPTY);
    bool SetDialogueInterator(Actor* actor)
    {
        if (!dialogInteractor_)
        {
            dialogInteractor_ = actor;
            return true;
        }
        return false;
    }

    /// Getters
    bool CheckDialogueRequirements() const;
    bool IsStarted() const
    {
        return started_;
    }
    bool IsMainController() const
    {
        return mainController_;
    }
    Node* GetAvatar() const
    {
        return avatar_;
    }
    Connection* GetConnection() const
    {
        return connection_;
    }
    Scene* GetScene() const
    {
        return scene_;
    }
    unsigned GetID() const
    {
        return info_.actorID_;
    }
    ActorInfo& GetInfo()
    {
        return info_;
    }
    unsigned& GetStat(const StringHash& category, const StringHash& stat);
    unsigned GetStat(const StringHash& category, const StringHash& stat) const;
    ActorStats* GetStats() const
    {
        return stats_;
    }
    GOC_Abilities* GetAbilities() const
    {
        return abilities_.Get();
    }
    int GetControlID() const
    {
        return controlID_;
    }
    bool HasTag(const String& tag) const
    {
        return avatar_ ? avatar_->HasTag(tag) : false;
    }

    UIPanel* GetPanel(int idpanel) const;
    void SetFocusPanel(int idpanel);
    UIPanel* GetFocusPanel() const;

    /// Commands
    void SetFaceTo(Node* interactor);
    void SetEnableShop(bool enable);
    void SetEnableDialogue(bool enable);

    /// Dialog
    void SetDialogue(const StringHash& hashname=StringHash::ZERO);
    /*
    void AddDetectInteractor(Actor* interactor);
    void RemoveDetectedInteractor(Actor* interactor);
    void SetDialogMarkerEnable(Actor* interactor, bool enable);
    void SetAllDialogMarkersEnable(bool enable);
    */
    void ResetDialogue();
    DialogMessage* GetMessage(const String& msgkey) const;
    int GetDialogueCategory() const
    {
        return 0;
    }
    Actor* GetDialogueInteractor() const
    {
        return dialogInteractor_;
    }

    void Start(bool resurrection=true, const WorldMapPosition& position=WorldMapPosition::EMPTY);
    void Stop();
    virtual void StartSubscribers();
    virtual void StopSubscribers();

    void OnSceneSet(Scene* scene);

protected:
    void ResetAvatar(const Vector2& position = Vector2::ZERO);
    void ResetMapping();
    virtual void UpdateComponents();

    virtual void OnReceiveCollectable(StringHash eventType, VariantMap& eventData) { };
    virtual void OnDropCollectable(StringHash eventType, VariantMap& eventData) { };
    void OnDialogueDetected(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkBegin(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkNext(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkEnd(StringHash eventType, VariantMap& eventData);
    virtual void OnDead(StringHash eventType, VariantMap& eventData);

    ActorInfo info_;

    WeakPtr<Node> avatar_;
    WeakPtr<Connection> connection_;

//    List<WeakPtr<Actor> > detectedInteractors_;
    WeakPtr<Actor> dialogInteractor_;
    SharedPtr<GOC_Abilities> abilities_;

    // UI Panels
    HashMap<int, WeakPtr<UIPanel> > panels_;
    WeakPtr<UIPanel> focusPanel_;

    Scene* scene_;
    ActorStats* stats_;
    bool mainController_;
    bool started_;

public :
    unsigned nodeID_;
    int controlID_;
    int clientID_;
    String objectFile_;

private:
    void Init(Context* context=0, unsigned id=0);
    void Clear();

    static Vector<WeakPtr<Actor> > actors_;

    int viewZ_;
    /// Current Dialogue
    StringHash dialogueFirst_;
    StringHash dialogueName_;
};

