#pragma once

namespace Urho3D
{
class Object;
class Context;
}

using namespace Urho3D;


enum GameLevelMode
{
    LVL_NEW = 0,
    LVL_LOAD,
    LVL_ARENA,
    LVL_TEST,
    LVL_CREATE,
    LVL_CHANGE
};

class GameStateManager;

class GameState : public Object
{
    URHO3D_OBJECT(GameState, Object);

    friend class GameStateManager;

public:

    GameState(Context* context);
    GameState(Context* context, const String & stateId);
    virtual ~GameState();

    void SetStateId(const String & stateId);
    const String& GetStateId() const;
    bool IsActive() const;
    bool IsSuspended() const;
    GameStateManager* GetManager();

    virtual bool Initialize();
    virtual void Begin();
    virtual void End();

    // State Management
    virtual void Suspend();
    virtual void Resume();

    // Query Routines
    virtual bool IsBegun() const
    {
        return begun_;
    }
    // delete all resources
    virtual void Dispose();

protected:
    GameStateManager* stateManager_;
    String stateId_;
    bool stateSuspended_;
    bool begun_;
};

class GameStateManager : public Object
{
    URHO3D_OBJECT(GameStateManager, Object);

    friend class GameState;

public:
    GameStateManager(Context* context);
    virtual ~GameStateManager();

    static GameStateManager* Get()
    {
        return stateManager_;
    }

    void Stop();

    //  Getters & Setters
    bool RegisterState(GameState * pGameState);
    GameState* GetState(const String & stateId);
    bool SetActiveState(const String & stateId);

    GameState* GetActiveState();
    GameState* GetPreviousActiveState() const;

    // Stack Management
    /// clear the history stack of the states
    void ClearStack();
    /// start manually the state from the back of the stack (added manually with AddToStack)
    void StartStack();
    /// add state to the stack manually without activating this state
    bool AddToStack(const String & stateId);
    /// add a state to the state stack and SetActiveState. The active state will be stopped (Stop function call)
    /// and the new state will be started (Begin function call)
    /// returns true if state was found and started
    bool PushToStack(const String & stateId);
    /// the active state will be stopped (Stop function call) and the last state pushed
    /// to the stack will be started (Begin function call)
    /// returns the new active state name/id or empty string
    const String& PopStack();

private:

    typedef HashMap<String, SharedPtr<GameState> > StateMap;
    typedef List<WeakPtr<GameState> > StateStack;

    void StateEnded(GameState* state);

    /// All state types that have been registered with the system
    StateMap registeredStates_;

    /// The currently active state that the game is currently in (restorable states will be available via its parent)
    WeakPtr<GameState> activeState_;

    /// The State Stack
    StateStack stateStack_;

    static GameStateManager* stateManager_;
};


