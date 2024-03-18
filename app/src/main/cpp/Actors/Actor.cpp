#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/JSONFile.h>
#include <Urho3D/Resource/JSONValue.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
//#include <Urho3D/Urho2D/CollisionCircle2D.h>

#include "DefsColliders.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameData.h"
#include "GameNetwork.h"

// Object's Components
#include "CommonComponents.h"

#include "MapWorld.h"
#include "ViewManager.h"
#include "DialogueFrame.h"
#include "UISlotPanel.h"

#include "Actor.h"


#ifdef SCENE_REPLICATION_ENABLE
const CreateMode sceneReplicationMode = REPLICATED;
#else
const CreateMode sceneReplicationMode = LOCAL;
#endif // SCENE_REPLICATION_ENABLE



/// Class ActorStats Implementation

Vector<StringHash> ActorStats::categories_;

void ActorStats::InitTable()
{
    categories_.Clear();
    categories_.Push(GOA::ACS_ENTITIES_KILLED);
    categories_.Push(GOA::ACS_ITEMS_COLLECTED);
    categories_.Push(GOA::ACS_ITEMS_GIVED);
}

ActorStats::ActorStats()
{
    stats_.Resize(categories_.Size());
}

unsigned& ActorStats::Value(const StringHash& category, const StringHash& stat)
{
//    URHO3D_LOGINFO("RefValue");
    return stats_[GetCategoryIndex(category)][stat];
}

unsigned ActorStats::Value(const StringHash& category, const StringHash& stat) const
{
//    URHO3D_LOGINFO("Value");
    const HashMap<StringHash, unsigned>& categoryMap = stats_[GetCategoryIndex(category)];
    HashMap<StringHash, unsigned>::ConstIterator it = categoryMap.Find(stat);
    if (it != categoryMap.End())
        return it->second_;
    return 0;
}

unsigned ActorStats::GetCategoryIndex(const StringHash& category) const
{
    for (unsigned i=0; i<categories_.Size(); i++)
    {
        if (categories_[i] == category)
            return i;
    }
    return 0;
}

void ActorStats::DumpStat(const StringHash& category, const StringHash& stat) const
{
    URHO3D_LOGINFOF("ActorStats() - DumpStat : category=%u, stat=%s, value=%d",
                    category.Value(), GOT::GetType(stat).CString(), Value(category, stat));
}

void ActorStats::DumpAllStats() const
{
    URHO3D_LOGINFOF("ActorStats() - DumpAllStats ...");
    for (unsigned i=0; i < stats_.Size(); i++)
    {
        const HashMap<StringHash, unsigned>& category = stats_[i];
        URHO3D_LOGINFOF(" => Category = %s(%u)", GOA::GetAttributeName(categories_[i]).CString(), categories_[i].Value());
        for (HashMap<StringHash, unsigned>::ConstIterator it=category.Begin(); it!=category.End(); ++it)
            URHO3D_LOGINFOF("  - type = %s(%u) : Value = %d", GOT::GetType(it->first_).CString(), it->first_.Value(), it->second_);
    }
    URHO3D_LOGINFOF("ActorStats() - DumpAllStats ... OK !");
}


/// Class Actor Implementation

Vector<WeakPtr<Actor> > Actor::actors_;

void Actor::RegisterObject(Context* context)
{
    actors_.Clear();
    actors_.Reserve(100);

    RemoveActors();
}

void Actor::RemoveActor(unsigned id)
{
    // don't remove players here
    if (id <= GameContext::Get().MAX_NUMPLAYERS)
        return;

    unsigned index = id-1;

    if (index < actors_.Size())
    {
        if (actors_[index])
            actors_[index]->Stop();
        actors_[index].Reset();
}
}

void Actor::RemoveActors()
{
    URHO3D_LOGINFOF("Actor() - RemoveActors ... size=%u", actors_.Size());

    if (actors_.Size() >= GameContext::Get().MAX_NUMPLAYERS)
    {
        for (unsigned i=GameContext::Get().MAX_NUMPLAYERS; i < actors_.Size(); ++i)
            RemoveActor(i+1);
    }

    actors_.Resize(GameContext::Get().MAX_NUMPLAYERS);

    URHO3D_LOGINFOF("Actor() - RemoveActors ... OK !");
}

void Actor::AddActor(Actor* actor, unsigned id)
{
    if (id > 0 && id <= GameContext::Get().MAX_NUMPLAYERS)
    {
        URHO3D_LOGINFOF("Actor() - Add Actor ID=%u ...", id);

        // specific id (for player)

        if (actors_[id-1])
            URHO3D_LOGWARNINGF("Actor() - AddActor : actor id=%u already exist ! => replace it", id);

        actors_[id-1] = WeakPtr<Actor>(actor);
        actor->info_.actorID_ = id;
    }
    else
    {
        // before push, prefer to get a deleted actor if exist
        if (actors_.Size() > GameContext::Get().MAX_NUMPLAYERS)
        {
            for (unsigned i=GameContext::Get().MAX_NUMPLAYERS; i < actors_.Size(); i++)
            {
                if (actors_[i] == 0)
                {
                    actors_[i]  = actor;
                    actor->info_.actorID_ = i+1;
                    URHO3D_LOGINFOF("Actor() - Add Actor ID=%u ...", actor->info_.actorID_);
                    actor->SetController();
                    return;
                }
            }
        }
        actors_.Push(WeakPtr<Actor>(actor));
        actor->info_.actorID_ = actors_.Size();
        URHO3D_LOGINFOF("Actor() - Add Actor ID=%u ...", actor->info_.actorID_);
        actor->SetController();
    }
}


bool Actor::LoadJSONFile(const String& name)
{
    JSONFile* jsonFile = GameContext::Get().context_->GetSubsystem<ResourceCache>()->GetResource<JSONFile>(name);
    if (!jsonFile)
    {
        URHO3D_LOGWARNINGF("Actor() - LoadJSONFile : %s not exist !", name.CString());
        return false;
    }

    const JSONValue& source = jsonFile->GetRoot();

    URHO3D_LOGINFOF("Actor() - LoadJSONFile : %s ...", name.CString());

    unsigned numactors = 0;
    for (JSONObject::ConstIterator it = source.Begin(); it != source.End(); ++it)
    {
        String name = it->first_;
        unsigned char entityid = 0;
        ActorInfo actorinfo;
        actorinfo.dialog_ = StringHash::ZERO;

        if (name.Empty())
        {
            URHO3D_LOGWARNING("Actor() - LoadJSONFile : actorName is empty !");
            continue;
        }

        String gotname;
        String positionStr;
        const JSONValue& attributes = it->second_;
        for (JSONObject::ConstIterator jt = attributes.Begin(); jt != attributes.End(); ++jt)
        {
            const String& attribute = jt->first_;
            if (attribute == "got")
                gotname = jt->second_.GetString();
            else if (attribute == "entityid")
                entityid = jt->second_.GetInt();
            else if (attribute == "pos")
                positionStr = jt->second_.GetString();
            else if (attribute == "dialog")
                actorinfo.dialog_ = StringHash(jt->second_.GetString());
            else
            {
                URHO3D_LOGWARNINGF("Actor() - LoadJSONFile : attribute=%s is unknown, actorName=%s", attribute.CString(), name.CString());
                continue;
            }
        }

        if (gotname.Empty())
        {
            URHO3D_LOGERRORF("Actor() - LoadJSONFile : actor=%s empty got !", name.CString());
            continue;
        }

        StringHash type(gotname);

        // check if got is registered
        if (!GOT::IsRegistered(type))
        {
            URHO3D_LOGERRORF("Actor() - LoadJSONFile : actor=%s got=%s(%) not registered in GOT system !", name.CString(), gotname.CString(), type.Value());
            continue;
        }

        Vector<String> coords = positionStr.Split(',');
        if (coords.Size() < 5)
        {
            URHO3D_LOGERRORF("Actor() - LoadJSONFile : actor=%s position=%s must have 5 int in order 'xmap,ymap,xtile,ytile,viewZ' !", name.CString(), positionStr.CString());
            continue;
        }

        // positions map, tile, viewZ
        actorinfo.position_.mPoint_.x_ = ToInt(coords[0]);
        actorinfo.position_.mPoint_.y_= ToInt(coords[1]);
        actorinfo.position_.mPosition_.x_ = ToInt(coords[2]);
        actorinfo.position_.mPosition_.y_ = ToInt(coords[3]);
        actorinfo.position_.viewZ_ = ToInt(coords[4]);

        Actor* actor = new Actor(GameContext::Get().context_, GameContext::Get().MAX_NUMPLAYERS+1);
        actor->SetName(name);
        actor->SetObjectType(type, entityid);
        actor->SetController(true);
        actor->SetInfo(actorinfo);

        numactors++;
    }

    URHO3D_LOGINFOF("Actor() - LoadJSONFile : %s ... numactors=%u ... OK !", name.CString(), numactors);

    return true;
}

bool Actor::SaveJSONFile(const String& name)
{
    int numactors = GetNumActors() - GameContext::Get().MAX_NUMPLAYERS;
    if (numactors <= 0)
        return false;

    File file(GameContext::Get().context_);
    if (!file.Open(name, FILE_WRITE))
        return false;

    URHO3D_LOGINFOF("Actor() - SaveJSONFile : %s ...", name.CString());

    SharedPtr<JSONFile> json(new JSONFile(GameContext::Get().context_));

    JSONValue& rootElem = json->GetRoot();
    for (int i=0; i < numactors; i++)
    {
        Actor* actor = actors_[GameContext::Get().MAX_NUMPLAYERS+i];
        if (!actor)
            continue;

        ActorInfo& actorinfo = actor->GetInfo();

        // update actorinfo position
        if (actor->GetAvatar() && actorinfo.state_ == Activated)
            actorinfo.position_ = actor->GetAvatar()->GetComponent<GOC_Destroyer>()->GetWorldMapPosition();

        JSONValue attributes;
        attributes.Set("got", GOT::GetType(actorinfo.type_));
        attributes.Set("entityid", actorinfo.entityid_);
        attributes.Set("pos", ToString("%d,%d,%d,%d,%d", actorinfo.position_.mPoint_.x_, actorinfo.position_.mPoint_.y_, actorinfo.position_.mPosition_.x_, actorinfo.position_.mPosition_.y_, actorinfo.position_.viewZ_));
        attributes.Set("dialog", GameData::GetDialogueData()->Get(actorinfo.dialog_)->name_);

        rootElem.Set(actorinfo.name_, attributes);
    }

    if (rootElem.IsNull())
    {
        URHO3D_LOGINFOF("Actor() - SaveJSONFile : %s ... is null ... OK !", name.CString());
        return false;
    }

    bool ok = json->Save(file);

    URHO3D_LOGINFOF("Actor() - SaveJSONFile : %s ... OK !", name.CString());

    return true;
}


unsigned Actor::GetNumActors()
{
    return actors_.Size();
}

const Vector<WeakPtr<Actor> >& Actor::GetActors()
{
    return actors_;
}

Actor* Actor::Get(unsigned id)
{
    return (id-1 < actors_.Size() ? actors_[id-1].Get() : 0);
}

Actor* Actor::Get(const StringHash& got)
{
    // before push, prefer to get a deleted actor if exist
    if (actors_.Size() <= GameContext::Get().MAX_NUMPLAYERS)
        return 0;

    for (unsigned i=GameContext::Get().MAX_NUMPLAYERS; i < actors_.Size(); i++)
    {
        Actor* actor = actors_[i];
        if (actor && actor->GetAvatar() && (actor->GetAvatar()->GetVar(GOA::GOT).GetStringHash() == got))
        {
            URHO3D_LOGINFOF("Actor() - Get : GOT=%u ID=%u ...", got.Value(), actor->info_.actorID_);
            return actor;
        }
    }

    return 0;
}

Actor* Actor::GetWithNode(Node* node)
{
    for (Vector<WeakPtr<Actor> >::ConstIterator it = actors_.Begin(); it != actors_.End(); ++it)
    {
        Actor* actor = it->Get();
        if (!actor) continue;

        if (actor->GetAvatar() == node)
            return actor;
    }
    return 0;
}


Actor::Actor() :
    Object(0),
    scene_(0),
    stats_(0),
    mainController_(false),
    started_(false),
    nodeID_(0),
    controlID_(0),
    clientID_(0),
    viewZ_(0)
{
    info_.actorID_ = 0;
}

/// Compilers C++11
#if __cplusplus > 199711L
Actor::Actor(Context* context) :
    Actor()
{
    Init(context);
}

Actor::Actor(Context* context, unsigned id) :
    Actor()
{
    Init(context, id);
}

Actor::Actor(Context* context, const StringHash& type) :
    Actor(context)
{
    info_.type_ = type;
}

Actor::Actor(Context* context, const String& name) :
    Actor(context)
{
    info_.name_ = name;
}

Actor::Actor(const Actor& actor) :
    Actor(actor.GetContext())
{
    info_.type_ = actor.info_.type_;
    info_.name_ = actor.info_.name_;
}
#else
Actor::Actor(Context* context) :
    Object(context),
    scene_(0),
    stats_(0),
    mainController_(false),
    started_(false),
    nodeID_(0),
    controlID_(0),
    clientID_(0)
{
    Init(context);
}

Actor::Actor(Context* context, unsigned id) :
    Object(context),
    scene_(0),
    stats_(0),
    mainController_(false),
    started_(false),
    nodeID_(0),
    controlID_(0),
    clientID_(0)
{
    Init(context, id);
}

Actor::Actor(Context* context, const StringHash& type) :
    Object(context),
    scene_(0),
    stats_(0),
    mainController_(false),
    started_(false),
    nodeID_(0),
    controlID_(0),
    clientID_(0)
{
    Init(context);
    info_.type_ = type;
}

Actor::Actor(Context* context, const String& name) :
    Object(context),
    scene_(0),
    stats_(0),
    mainController_(false),
    started_(false),
    nodeID_(0),
    controlID_(0),
    clientID_(0)
{
    Init(context);
    info_.name_ = name;
}

Actor::Actor(const Actor& actor) :
    Object(actor.GetContext()),
    scene_(0),
    stats_(0),
    mainController_(false),
    started_(false),
    nodeID_(0),
    controlID_(0),
    clientID_(0)
{
    Init();
    info_.type_ = actor.info_.type_;
    info_.name_ = actor.info_.name_;
}
#endif

Actor::~Actor()
{
//    Stop();
    URHO3D_LOGINFOF("~Actor() - actorID_=%d", info_.actorID_);
    RemoveActor(info_.actorID_);
}

void Actor::Init(Context* context, unsigned id)
{
    URHO3D_LOGINFOF("Actor() - Init : id=%u", id);

    info_.type_ = StringHash::ZERO;

    Clear();

    if (!context_ && context)
        context_ = context;

    AddActor(this, id);

    abilities_ = new GOC_Abilities(context);
}

void Actor::Clear()
{
    dialogueName_ = dialogueFirst_ = StringHash::ZERO;
    dialogInteractor_.Reset();
}


/// Setters
// Set MainControlled and the Connection
// Used by GO_Network::CreateAvatarFor, sPlay::SetPlayer
void Actor::SetController(bool main, Connection* connection, unsigned nodeid, int clientid)
{
    mainController_ = main;
    connection_ = connection;
    nodeID_ = nodeid;
    clientID_ = clientid;

    URHO3D_LOGINFOF("Actor() - SetController : actorID=%u nodeid=%u mainController_=%s connection_=%u clientid=%d",
                    info_.actorID_, nodeID_, mainController_ ? "true" : "false", connection_.Get(), clientID_);
}

void Actor::SetName(const String& name)
{
    info_.name_ = name;
}

void Actor::SetObjectType(const StringHash& type, unsigned char entityid)
{
    if (info_.type_ != type)
    {
        info_.type_ = type;
        info_.entityid_ = entityid;
        SetObjectFile(GOT::GetObjectFile(info_.type_));
    }
}

void Actor::SetObjectFile(const String& name)
{
    URHO3D_LOGINFOF("Actor() : SetObjectFile %s ... ", name.CString());

    if (objectFile_ != name)
    {
        objectFile_ = name;

        URHO3D_LOGINFOF("Actor() : SetObjectFile %s ... OK => Reset Avatar ...", name.CString());

        if (scene_ && avatar_)
            ResetAvatar(Vector2::ZERO);
        else
            URHO3D_LOGWARNINGF("Actor() : SetObjectFile %s - scene=%u avatar=%u ... No ResetAvatar Here !", name.CString(), scene_, avatar_.Get());
    }
}

void Actor::SetInfo(const ActorInfo& info)
{
    info_.state_ = Desactivated;
    info_.dialog_ = info.dialog_;
    info_.position_ = info.position_;
}

void Actor::SetEnableStats(bool enable)
{
    if (!enable && stats_)
    {
        delete stats_;
        stats_ = 0;
    }
    else if (enable && !stats_)
    {
        stats_ = new ActorStats();
    }
}

void Actor::SetViewZ(int viewZ)
{
    viewZ_ = viewZ;
}

void Actor::SetScene(Scene* scene, const Vector2& position, int viewZ)
{
    URHO3D_LOGERRORF("Actor() - SetScene ...");

    scene_ = scene;
    SetViewZ(viewZ);

    ResetAvatar(position);

    URHO3D_LOGERRORF("Actor() - SetScene ... !");
}

void Actor::SetAnimationSet(const String& nameSet)
{
    AnimatedSprite2D* animatedSprite = avatar_->GetComponent<AnimatedSprite2D>();
    if (animatedSprite)
    {
        if (!nameSet.Empty())
        {
//            URHO3D_LOGINFOF("Actor() - SetAnimationSet : apply %s ...", nameSet.CString());

            AnimationSet2D* animationSet = GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>("2D/" + nameSet + ".scml");

            if (animationSet)
            {
                animatedSprite->ResetCharacterMapping();
                animatedSprite->SetAnimationSet(animationSet);
                animatedSprite->SetAnimation();
            }

            GOC_Animator2D* gocAnimator = avatar_->GetComponent<GOC_Animator2D>();
            if (gocAnimator)
                gocAnimator->ApplyAttributes();

//            URHO3D_LOGINFOF("Actor() - SetAnimationSet : apply %s ... OK !", nameSet.CString());
        }
    }
}


/// Getters

unsigned& Actor::GetStat(const StringHash& category, const StringHash& stat)
{
    return stats_->Value(category, stat);
}

unsigned Actor::GetStat(const StringHash& category, const StringHash& stat) const
{
    return ((const ActorStats *) stats_)->Value(category, stat);
}

UIPanel* Actor::GetPanel(int idpanel) const
{
    HashMap<int, WeakPtr<UIPanel> >::ConstIterator it = panels_.Find(idpanel);
    return it != panels_.End() ? it->second_.Get() : 0;
}

void Actor::SetFocusPanel(int idpanel)
{
    if (idpanel == -1)
        focusPanel_.Reset();
    else
        focusPanel_ = WeakPtr<UIPanel>(GetPanel(idpanel));
}

UIPanel* Actor::GetFocusPanel() const
{
    return focusPanel_;
}

/// Commands

void Actor::SetFaceTo(Node* interactor)
{
    GOC_Animator2D* animator = avatar_->GetComponent<GOC_Animator2D>();
    if (animator)
        animator->SetDirection(Vector2(avatar_->GetWorldPosition2D().x_ - interactor->GetWorldPosition2D().x_ > 0.f ? -1.f : 1.f, animator->GetDirection().y_));
}

void Actor::SetEnableShop(bool enable)
{
    URHO3D_LOGINFOF("Actor() - SetEnableShop ... enable=%s actor=%s(%u) interactor=%s", enable ? "true":"false",
                    avatar_->GetName().CString(), avatar_->GetID(), dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetName().CString() : "NONE");

    if (dialogInteractor_)
    {
        // show/hide the player shop panel
        dialogInteractor_->SendEvent(DIALOG_BARGAIN);
    }

    // Swith dialogue to enable (if shop hidden) or to disable (if shop shown)
//    if (enable)
//    {
//        // Hide Dialog Frame only (don't use SetEnableDialogue : no reset animation state
//        DialogueFrame* dialogFrame = avatar_->GetComponent<DialogueFrame>();
//        if (dialogFrame)
//            dialogFrame->SetEnabled(false);
//    }
}

void Actor::SetEnableDialogue(bool enable)
{
    DialogueFrame* dialogFrame = avatar_->GetComponent<DialogueFrame>();

    enable = enable && dialogInteractor_ && dialogInteractor_->GetAvatar();

    if (enable)
    {
        if (!dialogFrame)
            return;

        int viewport = ViewManager::Get()->GetControllerViewport(dialogInteractor_);

        URHO3D_LOGINFOF("Actor() - SetEnableDialogue ... viewport=%d actor=%s(%u) interactor=%s(%u)", viewport,
                        avatar_->GetName().CString(), avatar_->GetID(),
                        dialogInteractor_->GetAvatar()->GetName().CString(), dialogInteractor_->GetAvatar()->GetID());

        // Face To Player
        SetFaceTo(dialogInteractor_->GetAvatar());

        // Set Animation State to Talk
        GOC_AIController* aiController = avatar_->GetComponent<GOC_AIController>();
        if (aiController)
            aiController->ChangeOrder(STATE_TALK);

        dialogFrame->SetFramePosition(avatar_->GetChild("DialogMarker")->GetPosition2D() + Vector2(0.f, 1.5f));
        dialogFrame->SetFrameLayer(ViewManager::Get()->GetCurrentViewZ(viewport), viewport);

        // Set Dialogue Contributor
        dialogFrame->SetContributor(dialogInteractor_);
    }
    else
    {
        dialogInteractor_.Reset();

        if (avatar_ && !avatar_->GetVar(GOA::ISDEAD).GetBool())
        {
            // Set Animation State to Default (don't force change)
            GOC_Animator2D* animator = avatar_->GetComponent<GOC_Animator2D>();
            if (animator)
            {
                if (animator->GetStateValue().Value() == STATE_TALK)
                {
                    GOC_AIController* aiController = avatar_->GetComponent<GOC_AIController>();
                    if (aiController)
                        aiController->ChangeOrder(STATE_DEFAULT_GROUND);
                }
            }
        }
    }

    // Show/Hide Dialog Frame
    if (dialogFrame)
    {
        URHO3D_LOGINFOF("Actor() - SetEnableDialogue ... actor=%s(%u) dialogFrame enable=%s", avatar_->GetName().CString(), avatar_->GetID(), enable ? "true":"false");
        if (dialogFrame->IsEnabled() != enable)
            dialogFrame->SetEnabled(enable);
        else
            dialogFrame->ToggleFrame(enable);
    }
}


/// Dialogue

void Actor::SetDialogue(const StringHash& dialogueName)
{
    if (dialogueName.Value())
    {
        if (dialogueFirst_.Value() == 0)
            dialogueFirst_ = dialogueName;

        dialogueName_ = dialogueName;
    }
    else
    {
        dialogueName_ = dialogueFirst_;
    }

    Node* dialogSensorNode = avatar_->GetChild("DialogSensor");
    Node* dialogMarkerNode = avatar_->GetChild("DialogMarker");
    DialogueFrame* dialogFrame = avatar_->GetComponent<DialogueFrame>();

    URHO3D_LOGINFOF("Actor() - SetDialogue ... %s(%u) dialogueName=%u", avatar_->GetName().CString(), avatar_->GetID(), dialogueName_.Value());

    if (dialogueName_.Value())
    {
        // Add Dialog Sensor for finding player
        if (!dialogSensorNode)
        {
            dialogSensorNode = avatar_->CreateChild("DialogSensor", LOCAL);
            dialogSensorNode->SetWorldScale(Vector3::ONE);
            CollisionBox2D* dialogSensor = dialogSensorNode->CreateComponent<CollisionBox2D>();
            dialogSensor->SetSize(6.f, 1.f);
            dialogSensor->SetCenter(0.f, 0.25f);
            dialogSensor->SetTrigger(true);
            dialogSensor->SetCategoryBits(CC_TRIGGER);
            dialogSensor->SetMaskBits(CM_DETECTPLAYER);
        }

        // Add Dialog Marker
        if (!dialogMarkerNode)
        {
            dialogMarkerNode = avatar_->CreateChild("DialogMarker", LOCAL);
            GameHelpers::LoadNodeXML(GetContext(), dialogMarkerNode, "UI/DialogAIMarker.xml", LOCAL);
            AnimatedSprite2D* dialogMarkerAnimatedSprite = dialogMarkerNode->GetComponent<AnimatedSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
            dialogMarkerAnimatedSprite->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERDIALOG]);
#else
            Material* material = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/LayerDialog.xml");
            dialogMarkerAnimatedSprite->SetCustomMaterial(material);
#endif
            BoundingBox mbox = dialogMarkerAnimatedSprite->GetWorldBoundingBox2D();
            BoundingBox bbox = avatar_->GetComponent<AnimatedSprite2D>()->GetWorldBoundingBox2D();
            Vector3 position = avatar_->GetWorldPosition();
            dialogMarkerNode->SetWorldPosition(Vector3(position.x_, position.y_ + Abs(bbox.max_.y_-position.y_) + Abs(mbox.max_.y_-mbox.min_.y_) * 0.5f, position.z_));
        }

        dialogMarkerNode->SetEnabled(false);

        // Add Dialog Frame
        if (!dialogFrame)
        {
            dialogFrame = avatar_->CreateComponent<DialogueFrame>();
            dialogFrame->SetLayoutName("UI/DialogAI.xml");
            dialogFrame->SetEnabled(false);
        }

        if (!dialogFrame->GetFrameNode())
        {
            dialogFrame->SetLayoutName("UI/DialogAI.xml");
        }

        dialogFrame->SetDialogue(dialogueName, this);
    }
    else
    {
        if (dialogSensorNode)
            dialogSensorNode->Remove();
        if (dialogMarkerNode)
            dialogMarkerNode->Remove();
        if (dialogFrame)
            dialogFrame->Remove();
    }
}
/*
void Actor::AddDetectInteractor(Actor* interactor)
{
    /// TODO : Purge list of empty weakptr or null avatar or avatar dead
    for (List<WeakPtr<Actor> >::ConstIterator it=detectedInteractors_.Begin(); it!=detectedInteractors_.End(); ++it)
    {
        if (it->Get() == interactor)
            return;
    }
    detectedInteractors_.Push(WeakPtr<Actor>(interactor));
}

void Actor::RemoveDetectedInteractor(Actor* interactor)
{
    for (List<WeakPtr<Actor> >::Iterator it=detectedInteractors_.Begin(); it!=detectedInteractors_.End(); ++it)
    {
        if (it->Get() == interactor)
        {
            SetDialogMarkerEnable(interactor, false);
            detectedInteractors_.Erase(it);
            return;
        }
    }
}

void Actor::SetDialogMarkerEnable(Actor* interactor, bool enable)
{
    if (!interactor)
        return;

    Node* avatar = interactor->GetAvatar();
    if (!avatar)
        return;

    Node* dialogueMarker = avatar->GetChild("DialogMarker", LOCAL);
    if (!dialogueMarker)
        return;

    dialogueMarker->SetEnabled(enable);
}

void Actor::SetAllDialogMarkersEnable(bool enable)
{
    for (List<WeakPtr<Actor> >::ConstIterator it=detectedInteractors_.Begin(); it!=detectedInteractors_.End(); ++it)
        SetDialogMarkerEnable(it->Get(), enable);
}
*/

void Actor::ResetDialogue()
{
    SetDialogue(dialogueFirst_);
    SetEnableDialogue(false);
}

DialogMessage* Actor::GetMessage(const String& msgkey) const
{
    return 0;
}

//#define LOAD_METHOD
#define CLONE_METHOD
void Actor::ResetAvatar(const Vector2& newposition)
{
    URHO3D_LOGINFOF("Actor() - ResetAvatar : ... newposition=%s ... avatar=%u", newposition.ToString().CString(), avatar_.Get());

    if (avatar_)
    {
        nodeID_ = 0;

//        GOC_Controller* controller = avatar_->GetDerivedComponent<GOC_Controller>();
//        if (controller)
//            controller->SetEnableObjectControl(false);

        URHO3D_LOGINFOF("Actor() - ResetAvatar : ... Existing Avatar=%s(%u) ...", avatar_->GetName().CString(), avatar_->GetID());

        StopSubscribers();

        //avatar_->SetEnabledRecursive(false);
        avatar_->SetEnabled(false);
    }

    if (!avatar_ && nodeID_)
    {
        if (scene_ && !info_.name_.Empty())
        {
            URHO3D_LOGINFOF("Actor() - ResetAvatar : ... Existing nodeId=%u try to get Avatar in the scene ...", nodeID_);
            avatar_ = scene_->GetNode(nodeID_);
        }
        else
        {
            URHO3D_LOGWARNINGF("Actor() - ResetAvatar : ... no scene =%u or no name =%s ... skip !", scene_, info_.name_.CString());
            return;
        }

        URHO3D_LOGINFOF("Actor() - ResetAvatar : ... Avatar=%u ...", avatar_.Get());
    }

    Vector2 position = newposition;
    Vector2 dir;
    if (avatar_)
    {
        if (position == Vector2::ZERO)
            position = avatar_->GetWorldPosition2D();

        GOC_Animator2D* animator = avatar_->GetComponent<GOC_Animator2D>();
        if (animator)
            dir = animator->GetDirection();
    }

#ifdef LOAD_METHOD
    if (!avatar_)
    {
        if (!GameContext::Get().controllablesNode_)
            GameContext::Get().controllablesNode_ = GameContext::Get().rootScene_->CreateChild("Controllables");

        URHO3D_LOGWARNINGF("Actor() - ResetAvatar : ... No Avatar => Create it ...");
        avatar_ = GameContext::Get().controllablesNode_->CreateChild("controllable", sceneReplicationMode, nodeID_);
    }

    if (!GameHelpers::LoadNodeXML(context_, avatar_, objectFile_, sceneReplicationMode, false))
    {
        URHO3D_LOGERRORF("Actor() - ResetAvatar ... NOK !");
        return;
    }
#else
    if (avatar_)
    {
//		// Remove All Shape Triggers
//		PODVector<CollisionShape2D*> shapes;
//		avatar_->GetDerivedComponents<CollisionShape2D>(shapes, true);
//		for (unsigned i=shapes.Size()-1; i > 0; i--)
//        {
//            if (shapes[i]->IsTrigger())
//            {
//                URHO3D_LOGERRORF("Actor() - ResetAvatar : ... Remove Trigger on nodeId=%u ...", avatar_->GetID());
////                shapes[i]->Remove();
//                if (shapes[i]->GetNode()->GetName().StartsWith(TRIGATTACK))
//                    shapes[i]->GetNode()->Remove();
//            }
//        }
//
//        GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, true);
//        GameHelpers::DumpNode(avatar_, 3, true);
//        GAME_SETGAMELOGENABLE(GAMELOG_PLAYER, false);

        Node* templateNode = info_.type_ ? GOT::GetObject(info_.type_) : 0;
        if (templateNode)
        {
            URHO3D_LOGINFOF("Actor() - ResetAvatar : ... CopyAttributes on nodeId=%u From Template ...", avatar_->GetID());

            GameHelpers::CopyAttributes(templateNode, avatar_, false, true);

//            GOC_Controller* controller = avatar_->GetDerivedComponent<GOC_Controller>();
//            if (controller)
//                controller->SetEnableObjectControl(false);

            // Prevent apparition of Children like AileDark or SpiderThread (see TODO BEHAVIOR:26/04/2020)
//            avatar_->RemoveAllChildren();
        }
        else if (!GameHelpers::LoadNodeXML(context_, avatar_, objectFile_, sceneReplicationMode, false))
        {
            URHO3D_LOGERRORF("Actor() - ResetAvatar : ... NOK !");
            return;
        }
    }
    else
    {
        URHO3D_LOGWARNINGF("Actor() - ResetAvatar : ... No Avatar => Create it ...");

#ifdef CLONE_METHOD
        avatar_ = GOT::GetClonedControllable(info_.type_, nodeID_, sceneReplicationMode);
        if (avatar_)
        {
            URHO3D_LOGINFOF("Actor() - ResetAvatar : ... Clone Template=%s on nodeId=%u", GOT::GetType(info_.type_).CString(), avatar_->GetID());
        }
        else if (info_.type_)
        {
            avatar_ = World2D::SpawnEntity(info_.type_, info_.entityid_, nodeID_, 0, viewZ_ ? viewZ_ : ViewManager::Get()->GetCurrentViewZ(controlID_), PhysicEntityInfo(position.x_, position.y_), SceneEntityInfo());
            URHO3D_LOGINFOF("Actor() - ResetAvatar : ... WorldSpawnEntity type_=%u nodeId_=%u ... ", info_.type_.Value(), nodeID_);
        }
#else
        if (!GameContext::Get().controllablesNode_)
            GameContext::Get().controllablesNode_ = GameContext::Get().rootScene_->CreateChild("Controllables");

        avatar_ = GameContext::Get().controllablesNode_->CreateChild("controllable", sceneReplicationMode, nodeID_);

        if (!LoadNodeXML(context_, avatar_, objectFile_, sceneReplicationMode, false))
        {
            URHO3D_LOGERRORF("Actor() - ResetAvatar : ... NOK !");
            return;
        }
#endif
    }
#endif

    if (!avatar_)
    {
        URHO3D_LOGERRORF("Actor() - ResetAvatar : ... 1 ... nodeID=%u ... NOK !", nodeID_);
        return;
    }

    if (position != Vector2::ZERO)
        avatar_->SetWorldPosition2D(position);
    else
        URHO3D_LOGWARNINGF("Actor() - ResetAvatar :... position Error ? ... NOK !");

//    GOC_Controller* controller = avatar_->GetDerivedComponent<GOC_Controller>();
//    if (controller)
//        controller->SetEnableObjectControl(false);

    avatar_->SetName(info_.name_);
    nodeID_ = avatar_->GetID();

    ResetMapping();

    GOC_Animator2D* animator = avatar_->GetComponent<GOC_Animator2D>();
    if (animator)
    {
        animator->PlugDrawables();
        if (dir != Vector2::ZERO)
            animator->SetDirection(dir);
    }

    UpdateComponents();

    if (!avatar_)
    {
        URHO3D_LOGERRORF("Actor() - ResetAvatar : ... 2 ... nodeID=%u ... NOK !", nodeID_);
        return;
    }

    avatar_->SetTemporary(true);

    avatar_->SetEnabled(false);

    avatar_->ApplyAttributes();

    if (GameContext::Get().ServerMode_)
    {
        URHO3D_LOGERRORF("Actor() - ResetAvatar : ... nodeID=%u  ... servermode ... add ocontrol ...", nodeID_);
        ObjectControlInfo* oinfo = GameNetwork::Get()->AddSpawnControl(avatar_, avatar_, false, false);
        oinfo->clientId_ = GameNetwork::Get()->GetClientID(connection_.Get());
    }

    URHO3D_LOGERRORF("Actor() - ResetAvatar : ... nodeID=%u  ... OK !", nodeID_);
}

void Actor::ResetMapping()
{
    AnimatedSprite2D* animatedSprite = avatar_->GetComponent<AnimatedSprite2D>();
    if (animatedSprite)
    {
//        URHO3D_LOGINFOF("Actor() - ResetMapping ...");
        animatedSprite->ResetCharacterMapping();
        animatedSprite->ApplyCharacterMap(CMAP_HEAD2);
        animatedSprite->ApplyCharacterMap(CMAP_NAKED);
//        URHO3D_LOGINFOF("Actor() - ResetMapping ... OK !");
    }
}

void Actor::UpdateComponents()
{
    URHO3D_LOGINFOF("Actor() - UpdateComponents ...");

    avatar_->SetVar(GOA::CLIENTID, clientID_);

    GOC_Controller* gocController = avatar_->GetDerivedComponent<GOC_Controller>();
    if (gocController)
    {
        gocController->SetMainController(mainController_);
        gocController->ResetDirection();
    }
//    else
//        URHO3D_LOGERROR("Actor() - UpdateComponents : !gocController");

    AnimatedSprite2D* animatedSprite = avatar_->GetComponent<AnimatedSprite2D>();
    if (animatedSprite)
        animatedSprite->MarkDirty();

    GOC_Move2D* gocMove = avatar_->GetComponent<GOC_Move2D>();
    if (gocMove)
    {
        if (mainController_)
            gocMove->SetActiveLOS(controlID_+1);

#ifdef ACTIVE_NETWORK_LOCALPHYSICSIMULATION_BUTTON_MAINONLY
        /// active Physic simulation only if mainController
        gocMove->SetPhysicEnable(mainController_);
#endif
    }

    RigidBody2D* body = avatar_->GetComponent<RigidBody2D>();
    if (body)
    {
        body->SetAllowSleep(false);
    }

    GOC_Destroyer* gocDestroyer = avatar_->GetComponent<GOC_Destroyer>();
    if (gocDestroyer)
    {
        gocDestroyer->Reset(true);
//        gocDestroyer->UpdatePositions();
        URHO3D_LOGINFOF("Actor() - UpdateComponents ... gocDestroyer controlid=%d viewZ_=%d currentviewz=%d ...", controlID_, viewZ_, ViewManager::Get()->GetCurrentViewZ(controlID_));
        gocDestroyer->SetViewZ(viewZ_ ? viewZ_ : ViewManager::Get()->GetCurrentViewZ(controlID_), 0, 1);
    }

    if (abilities_)
    {
        avatar_->AddComponent(abilities_, 0, LOCAL);
        abilities_->SetNode(avatar_);
        abilities_->SetHolder(this);
    }

    URHO3D_LOGINFOF("Actor() - UpdateComponents ... OK !");
}


/// Handles
WorldMapPosition savedposition_;

void Actor::Start(bool resurrection, const WorldMapPosition& position)
{
    if (!avatar_)
        return;

//    URHO3D_LOGERRORF("Actor() - Start : name=%s %s(%u) enabled=%s resurrection=%s position=%s viewmask=%u ...",
//                    info_.name_.CString(), avatar_->GetName().CString(), avatar_->GetID(), avatar_->IsEnabled() ? "true" : "false", resurrection ? "true" : "false",
//                    position.ToString().CString(), avatar_->GetDerivedComponent<AnimatedSprite2D>()->GetViewMask());

    // needed to reactive explicitly these components because GOC_BodyExploder desactive them at explosion
    // if exists, enable the rendertarget animatedsprite of the "avatar" node's animatedsprite (see Urho3D::AnimatedSprite2D)
    avatar_->GetDerivedComponent<Drawable2D>()->SetEnabled(true);
    avatar_->GetComponent<RigidBody2D>()->SetEnabled(true);

//    if (dialogueName_.Value() || dialogueFirst_.Value())
//        SetDialogue(dialogueName_.Value() ? dialogueName_ : dialogueFirst_);

    avatar_->SetEnabled(true);
    // TODO : 04/04/2023 : here quick error patch : do it better !
    if (avatar_->isInPool_)
        avatar_->isInPool_ = false;

    URHO3D_LOGINFOF("Actor() - Start : name=%s %s(%u) enabled=%s resurrection=%s position=%s viewmask=%u ...",
                    info_.name_.CString(), avatar_->GetName().CString(), avatar_->GetID(), avatar_->IsEnabled() ? "true" : "false", resurrection ? "true" : "false",
                    position.ToString().CString(), avatar_->GetDerivedComponent<AnimatedSprite2D>()->GetViewMask());

    // Adjust position after re-activation
    GOC_Destroyer* destroyer = avatar_->GetComponent<GOC_Destroyer>();
    if (destroyer)
    {
        if (position.defined_)
            destroyer->AdjustPositionInTile(position);
    }

    if (!avatar_)
    {
        URHO3D_LOGERRORF("Actor() - Start : name=%s actorID=%u avatarID=%u no avatar  NOK ! => check spawning, GOC_Destroyer ...", info_.name_.CString(), GetID(), nodeID_);
        return;
    }

    StartSubscribers();

    if (resurrection)
    {
        GOC_Life* goclife = avatar_->GetComponent<GOC_Life>();
        if (goclife)
            goclife->Reset();

        avatar_->SendEvent(WORLD_ENTITYCREATE);
    }

    // 27/05/2023 : Here, be sure to set the viewZ, viewmask and filterbits
    if (destroyer)
        destroyer->SetViewZ();

    viewZ_ = 0;

//    GOC_Controller* controller = avatar_->GetDerivedComponent<GOC_Controller>();
//    if (controller)
//        controller->SetEnableObjectControl(true);

    started_ = true;

    URHO3D_LOGINFOF("Actor() - Start : name=%s actorID=%u avatarID=%u enabled=%s resurrection=%s viewmask=%u ... OK !",
                    info_.name_.CString(), GetID(), nodeID_, avatar_->IsEnabled() ? "true" : "false", resurrection ? "true" : "false", avatar_->GetDerivedComponent<AnimatedSprite2D>()->GetViewMask());
}

void Actor::Stop()
{
    started_ = false;

    if (!avatar_)
        return;

    URHO3D_LOGINFOF("Actor() - Stop ...");

    StopSubscribers();

    if (connection_)
    {
        avatar_->CleanupConnection(connection_);

//        if (GameNetwork::Get())
//            GameNetwork::Get()->CleanUpConnection(connection_);
    }

    avatar_->SetEnabled(false);

    if (GameContext::Get().LocalMode_)
    {
        avatar_.Reset();
        Clear();
    }

    URHO3D_LOGINFOF("Actor() - Stop ... OK !");
}

void Actor::StartSubscribers()
{
    if (avatar_)
    {
        SubscribeToEvent(avatar_, GOC_LIFEDEAD, URHO3D_HANDLER(Actor, OnDead));
        SubscribeToEvent(avatar_, GO_DESTROY, URHO3D_HANDLER(Actor, OnDead));

        SubscribeToEvent(this, DIALOG_DETECTED, URHO3D_HANDLER(Actor, OnDialogueDetected));
        SubscribeToEvent(this, DIALOG_OPEN, URHO3D_HANDLER(Actor, OnTalkBegin));
        SubscribeToEvent(this, DIALOG_NEXT, URHO3D_HANDLER(Actor, OnTalkNext));
        SubscribeToEvent(this, DIALOG_CLOSE, URHO3D_HANDLER(Actor, OnTalkEnd));

        URHO3D_LOGINFOF("Actor() - StartSubscribers : %s(%u) ... OK !", avatar_->GetName().CString(), avatar_->GetID());
    }
}

void Actor::StopSubscribers()
{
    UnsubscribeFromAllEvents();
}

void Actor::OnSceneSet(Scene* scene)
{
    scene_ = scene;
}

bool Actor::CheckDialogueRequirements() const
{
    if (!dialogueName_)
    {
        URHO3D_LOGINFOF("Actor() - CheckDialogueRequirements : node %s(%u) no dialogue name !",
                        avatar_->GetName().CString(), avatar_->GetID());
        return false;
    }

    // show dialogue marker only if not dead and no dialog in progress
    if (avatar_->GetVar(GOA::ISDEAD).GetBool() || dialogInteractor_)
    {
        URHO3D_LOGINFOF("Actor() - CheckDialogueRequirements : node %s(%u) is dead (%s) or has dialog interactor (%s)!",
                        avatar_->GetName().CString(), avatar_->GetID(), avatar_->GetVar(GOA::ISDEAD).GetBool() ? "true":"false", dialogInteractor_ ? "true":"false");
        return false;
    }

    /// TODO : prevent creating StringHash each time
    /*
        if (dialogueName_ == StringHash("merchandise1") && avatar_ && avatar_->GetComponent<GOC_Inventory>()->Empty())
            return false;
    */
    return true;
}

void Actor::OnDialogueDetected(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("Actor() - OnDialogDetected : node %s(%u) ...",
                    avatar_->GetName().CString(), avatar_->GetID());

    // show dialogue marker only if has a dialogue interest
    if (!CheckDialogueRequirements())
        return;

    Node* dialogMarkerNode = avatar_->GetChild("DialogMarker");
    if (dialogMarkerNode)
    {
        /// TODO : properly setlayer and mask in GOC_Destroyer to be able of changing view
        Drawable2D* drawable = dialogMarkerNode->GetDerivedComponent<Drawable2D>();
        if (drawable)
        {
            int viewZ = avatar_->GetVar(GOA::ONVIEWZ).GetInt();
            if (viewZ == INNERVIEW)
                viewZ = FRONTINNERBIOME;
            else
                viewZ = FRONTBIOME;
            drawable->SetLayer2(IntVector2(viewZ+LAYERADDER_DIALOGS, -1));
            drawable->SetOrderInLayer(0);
            drawable->SetViewMask(avatar_->GetDerivedComponent<Drawable2D>()->GetViewMask());
        }

        // show interaction marker only if has no dialogframe or dialogframe's hidden
        DialogueFrame* dialogFrame = avatar_->GetComponent<DialogueFrame>();
        bool enable = !dialogFrame || !dialogFrame->IsEnabled();
        dialogMarkerNode->SetEnabled(enable);
        if (enable)
            URHO3D_LOGINFOF("Actor() - OnDialogDetected : detected DialogMarker on node %s(%u) markernode=%s(%u) viewmask=%u",
                            avatar_->GetName().CString(), avatar_->GetID(), dialogMarkerNode->GetName().CString(), dialogMarkerNode->GetID(), drawable->GetViewMask());
    }
    else
    {
        URHO3D_LOGINFOF("Actor() - OnDialogDetected : node %s(%u) has no dialog marker",
                        avatar_->GetName().CString(), avatar_->GetID());
    }
}

void Actor::OnTalkBegin(StringHash eventType, VariantMap& eventData)
{
    if (!dialogInteractor_ && eventData[Dialog_Open::PLAYER_ID].GetUInt())
        dialogInteractor_ = Actor::Get(eventData[Dialog_Open::PLAYER_ID].GetUInt());

    URHO3D_LOGINFOF("Actor() - OnTalkBegin ... actor=%s(%u) interactor=%s(%u)", avatar_->GetName().CString(), avatar_->GetID(),
                    dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetName().CString() : "NONE",
                    dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetID() : 0);

    if (!dialogInteractor_)
        return;

//    // Hide All Dialog Markers related to interactor
//    dialogInteractor_->SetAllDialogMarkersEnable(false);
    // Hide Dialog Marker
    Node* dialogMarkerNode = avatar_->GetChild("DialogMarker");
    dialogMarkerNode->SetEnabled(false);

    SetEnableDialogue(true);
}

void Actor::OnTalkNext(StringHash eventType, VariantMap& eventData)
{
    if (!dialogInteractor_ && eventData[Dialog_Open::PLAYER_ID].GetUInt())
        dialogInteractor_ = Actor::Get(eventData[Dialog_Open::PLAYER_ID].GetUInt());

    if (dialogInteractor_->GetID() == eventData[Dialog_Open::PLAYER_ID].GetUInt())
    {
        const StringHash& next = eventData[Dialog_Next::DIALOGUENAME].GetStringHash();

        URHO3D_LOGINFOF("Actor() - OnTalkNext ... next=%u", next.Value());
        SetDialogue(next);
    }
}

void Actor::OnTalkEnd(StringHash eventType, VariantMap& eventData)
{
    if (!dialogInteractor_ || (GetID() == eventData[Dialog_Close::ACTOR_ID].GetUInt() &&
                               dialogInteractor_->GetID() == eventData[Dialog_Close::PLAYER_ID].GetUInt()))
    {
        URHO3D_LOGINFOF("Actor() - OnTalkEnd ... actor=%s(%u) interactor=%s(%u)", avatar_->GetName().CString(), avatar_->GetID(),
                        dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetName().CString() : "NONE",
                        dialogInteractor_ ? dialogInteractor_->GetAvatar()->GetID() : 0);

        ResetDialogue();
    }

    if (!dialogInteractor_)
    {
        // Hide Dialog Marker
        Node* dialogMarkerNode = avatar_->GetChild("DialogMarker");
        dialogMarkerNode->SetEnabled(false);
    }
}

void Actor::OnDead(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GOC_LIFEDEAD)
    {
        if (avatar_->GetVar(GOA::DESTROYING).GetBool())
            return;

        // Remove Dialog Marker and Dialog Frame

        StopSubscribers();

        if (avatar_)
        {
            Node* dialogMarkerNode = avatar_->GetChild("DialogMarker");
            if (dialogMarkerNode)
                dialogMarkerNode->SetEnabled(false);

            ResetDialogue();

            avatar_->SetVar(GOA::ISDEAD, true);

            URHO3D_LOGINFOF("Actor() - OnDead : %s(%u) !", avatar_->GetName().CString(), avatar_->GetID());
        }

        info_.state_ = ActorState::Dead;

//        Actor::RemoveActor(GetID());
    }

    if (eventType == GO_DESTROY)
    {
        URHO3D_LOGINFOF("Actor() - OnDead : ... GO_DESTROY");

        Actor::RemoveActor(GetID());
    }
}
