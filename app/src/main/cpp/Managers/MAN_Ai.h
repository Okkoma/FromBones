#pragma once

#include "TimerSimple.h"

namespace Urho3D
{
class Scene;
}

using namespace Urho3D;

class GOManager;

class AIManager : public Object
{
    URHO3D_OBJECT(AIManager, Object);

public :
    AIManager();
    AIManager(Context* context);
    AIManager(const AIManager& ai);

    virtual ~AIManager();
    static void InitManagers(Context* context);

    void Register();
    void Set(unsigned id=0, Scene* scene=0, GOManager* goManager=0);
    void SetUpdateFps(int fps);
    void Start();
    void Stop();
    bool HasToManageNode(unsigned nodeId) const;

    void Dump() const;

    static void AddManager(AIManager* aiManager);
    static AIManager* GetManagerFor(unsigned nodeID);
    static unsigned GetNumManagers();
    static void SetManagers(Scene* scene, GOManager* goManager);
    static void StartManagers();
    static void StopManagers();
    static void RemoveManagers();
    static void DumpAll();

private :
    void Update();

    void GetNodeIDs();
    void HandlePostUpdate(StringHash eventType, VariantMap& eventData);

    Scene* scene_;
    GOManager* goManager_;
    unsigned idManager_;        // index in aiManagers

    Vector<unsigned>::Iterator iCount_;          // current counter in the update of nodeIDs_
    TimerSimple timer_;
    /// Update FPS.
    int updateFps_;
    /// Update time interval.
    float updateInterval_;
    /// Update time accumulator.
    float updateAcc_;
    Vector<unsigned> nodeIDs_;  // ID nodes managed

    unsigned lastUpdate_;

    static Vector<AIManager> aiManagers;     // AI managers
};


