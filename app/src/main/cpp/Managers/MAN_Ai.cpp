#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include "GameAttributes.h"
#include "GameEvents.h"

#include "GOC_ControllerAI.h"

#include "Behavior.h"
#include "MAN_Go.h"

#include "MAN_Ai.h"

#define MAX_AIMANAGER 5
#define CLEANFACTOR 2

#define MAXTIMEUPDATE 6U   // milliseconds for update pass


static const int DEFAULT_AIUPDATE_FPS = 30;

Vector<AIManager> AIManager::aiManagers;

AIManager::AIManager() :
    Object(0),
    idManager_(0),
    timer_(0),
    updateFps_(DEFAULT_AIUPDATE_FPS),
    updateInterval_(1.0f / (float)DEFAULT_AIUPDATE_FPS),
    updateAcc_(0.0f)
{ }

AIManager::AIManager(Context* context) :
    Object(context),
    idManager_(0),
    timer_(0),
    updateFps_(DEFAULT_AIUPDATE_FPS),
    updateInterval_(1.0f / (float)DEFAULT_AIUPDATE_FPS),
    updateAcc_(0.0f)
{ }

AIManager::AIManager(const AIManager& ai) :
    Object(ai.context_),
    scene_(ai.scene_),
    idManager_(0),
    timer_(0),
    updateFps_(DEFAULT_AIUPDATE_FPS),
    updateInterval_(1.0f / (float)DEFAULT_AIUPDATE_FPS),
    updateAcc_(0.0f)
{ }

AIManager::~AIManager()
{
    Stop();
}

void AIManager::InitManagers(Context* context)
{
    aiManagers.Reserve(MAX_AIMANAGER);

    AIManager aiManagerDefault(context);
    aiManagerDefault.Register();
}

void AIManager::Register()
{
    AddManager(this);
}

void AIManager::Set(unsigned id, Scene* scene, GOManager* goManager)
{
    if (id != 0)
        idManager_ = id;

    if (scene_ != scene && scene != 0)
        scene_ = scene;

    if (goManager_ != goManager && goManager != 0)
        goManager_ = goManager;
}

void AIManager::SetUpdateFps(int fps)
{
    updateFps_ = Max(fps, 1);
    updateInterval_ = 1.0f / (float)updateFps_;
    updateAcc_ = 0.0f;
}

void AIManager::Start()
{
    GetNodeIDs();

    if (scene_)
        SubscribeToEvent(scene_, E_SCENEPOSTUPDATE, URHO3D_HANDLER(AIManager, HandlePostUpdate));
}

void AIManager::Stop()
{
    if (scene_)
        UnsubscribeFromEvent(scene_, E_SCENEPOSTUPDATE);
}

bool AIManager::HasToManageNode(unsigned nodeId) const
{
    Vector<unsigned>::ConstIterator it=nodeIDs_.Find(nodeId);
    return (it != nodeIDs_.End());
}

void AIManager::AddManager(AIManager* aiManager)
{
    unsigned id = aiManagers.Size();

    if (id == MAX_AIMANAGER)
        return;

    aiManager->Set(id+1);
    aiManagers.Push(*aiManager);
}

AIManager* AIManager::GetManagerFor(unsigned nodeID)
{
    if (nodeID == 0)
        return 0;

    for (Vector<AIManager>::Iterator it=aiManagers.Begin(); it!=aiManagers.End(); ++it)
    {
        if (it->HasToManageNode(nodeID)) return &(*it);
    }
    return 0;
}

unsigned AIManager::GetNumManagers()
{
    return aiManagers.Size();
}

void AIManager::SetManagers(Scene* scene, GOManager* goManager)
{
    for (Vector<AIManager>::Iterator it=aiManagers.Begin(); it!=aiManagers.End(); ++it)
        it->Set(0, scene, goManager);
}

void AIManager::RemoveManagers()
{
    aiManagers.Clear();
}

void AIManager::StartManagers()
{
    URHO3D_LOGINFO("AIManager() - StartManagers");

    for (Vector<AIManager>::Iterator it=aiManagers.Begin(); it!=aiManagers.End(); ++it)
        it->Start();
}

void AIManager::StopManagers()
{
    URHO3D_LOGINFO("AIManager() - StopManagers");
    for (Vector<AIManager>::Iterator it=aiManagers.Begin(); it!=aiManagers.End(); ++it)
        it->Stop();
}

void AIManager::DumpAll()
{
    URHO3D_LOGINFOF("AIManager() - Dump : num managers=%u", aiManagers.Size());

    for (Vector<AIManager>::Iterator it=aiManagers.Begin(); it!=aiManagers.End(); ++it)
        it->Dump();
}

void AIManager::Update()
{
    Controls control;

    Behavior* behavior = 0;

    timer_.SetExpirationTime(MAXTIMEUPDATE);

    unsigned time = Time::GetSystemTime();

    Vector<unsigned>::Iterator itend = iCount_;
    Vector<unsigned>::Iterator it = iCount_;
    do
    {
        unsigned& nodeId = *it;

        // get node
        if (nodeId)
        {
            Node* node = scene_->GetNode(nodeId);
            if (node)
            {
                GOC_AIController* aiControl = node->GetComponent<GOC_AIController>();
                if (!aiControl)
                {
//                    URHO3D_LOGERRORF("AIManager() - Update() : node %u has no aicontroller ! index deleted", nodeId);
                    nodeId = 0;
                }

                // update ai controller
                else if (!aiControl->Update(time))
                {
//                    URHO3D_LOGERRORF("AIManager() - Update() : can't update aicontrol node %u ! index deleted", nodeId);
                    nodeId = 0;
                }
            }
            else
            {
//                URHO3D_LOGERRORF("AIManager() - Update() : Scene can't find node %u ! index deleted", nodeId);
                nodeId = 0;
            }
        }

        it++;

        if (it == nodeIDs_.End())
            it = nodeIDs_.Begin();

        // check timer expiration of the pass update
        if (timer_.Expired())
        {
            iCount_ = it;
//            URHO3D_LOGERRORF("AIManager() - Update() : Timer Expiration(%umsec) at index =%u", TIMEUPDATE, iCount_ - nodeIDs_.Begin());
            break;
        }

    }
    while (it != itend);

//    iCount_ = nodeIDs_.Begin();

//    URHO3D_LOGINFOF("AIManager() - Update() : Finish in %u (%u passes)",
//             timer_.GetCurrentTime() + TIMEUPDATE * (numPasses_-1), numPasses_);
}

void AIManager::GetNodeIDs()
{
    if (goManager_)
    {
        nodeIDs_ = goManager_->GetActiveAiNodes();
        iCount_ = nodeIDs_.Begin();
        lastUpdate_ = GOManager::GetLastUpdate();
//        URHO3D_LOGINFOF("AIManager() - GetNodeIDs : Size = %u", nodeIDs_.Size());
    }
}

void AIManager::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    URHO3D_PROFILE(AIManager);

    if (lastUpdate_ != GOManager::GetLastUpdate())
        GetNodeIDs();
//    else if (GOManager::GetNumActiveAiNodes() * CLEANFACTOR < nodeIDs_.Size())
//        GetNodeIDs();

    if (!nodeIDs_.Size())
        return;

    float timeStep = eventData[ScenePostUpdate::P_TIMESTEP].GetFloat();

    updateAcc_ += timeStep;
    if (updateAcc_ >= updateInterval_)
    {
        updateAcc_ = fmodf(updateAcc_, updateInterval_);
        Update();
    }
}

void AIManager::Dump() const
{
    URHO3D_LOGINFOF("  num managed nodes=%u (gomanager has %u nodes)", nodeIDs_.Size(), GOManager::GetNumActiveAiNodes());
    for (unsigned i=0; i < nodeIDs_.Size(); i++)
        URHO3D_LOGINFOF("  node[%d]=%u", i, nodeIDs_[i]);
}

