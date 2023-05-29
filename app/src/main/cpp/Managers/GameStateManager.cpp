#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Container/List.h>

#include <Urho3D/Resource/ResourceCache.h>


#include "GameStateManager.h"



const char* levelModeNames[] =
{
    "LVL_NEW",
    "LVL_LOAD",
    "LVL_ARENA",
    "LVL_TEST",
    "LVL_CREATE",
    "LVL_CHANGE",
};



GameState::GameState(Context* context) : Object(context)
{
    // Initialize variables to sensible defaults
    stateManager_ = 0;
    stateSuspended_ = false;
    begun_ = false;
}

GameState::GameState(Context* context, const String& stateId) :
    Object(context)
{
    // Initialize variables to sensible defaults
    stateManager_ = 0;
    stateSuspended_ = false;
    begun_ = false;

    SetStateId(stateId);
}

GameState::~GameState()
{
    // Clean up
    Dispose();
}

void GameState::Dispose()
{
    // End the state if it is begun.
    if (begun_ == true)
        End();

    // Clear variables
    stateManager_ = 0;
    stateSuspended_ = false;

    begun_ = false;
    URHO3D_LOGINFOF("GameState() - Dispose : id=%s",stateId_.CString());
}

bool GameState::IsActive() const
{
    if (!stateManager_) return false;
    return (this == stateManager_->GetActiveState());
}

bool GameState::IsSuspended() const
{
    return stateSuspended_;
}

const String & GameState::GetStateId() const
{
    return stateId_;
}

GameStateManager* GameState::GetManager()
{
    return stateManager_;
}


bool GameState::Initialize()
{
    URHO3D_LOGINFOF("GameState() - Initialize : id=%s",stateId_.CString());
    // Success!
    return true;
}

void GameState::Begin()
{
    begun_ = true;
    URHO3D_LOGINFOF("GameState() - Begin : id=%s", stateId_.CString());
}

void GameState::End()
{
    // Notify manager that the state ended
    stateManager_->StateEnded(this);

    // We are no longer attached to anything (simply being stored in
    // the registered state list maintained by the manager).
    stateSuspended_ = false;
    begun_ = false;
    URHO3D_LOGINFOF("GameState() - End : id=%s",stateId_.CString());
}


void GameState::Suspend()
{
    // Is this a no-op?
    if (stateSuspended_)
        return;

    // Suspend
    stateSuspended_ = true;
    URHO3D_LOGINFOF("GameState() - Suspend : id=%s",stateId_.CString());
}

void GameState::Resume()
{
    // Is this a no-op?
    if (!stateSuspended_)
        return;

    // Resume
    stateSuspended_ = false;
    URHO3D_LOGINFOF("GameState() - Resume : id=%s",stateId_.CString());
}

void GameState::SetStateId(const String & stateId)
{
    stateId_ = stateId;
}


/// GameStateManager

GameStateManager* GameStateManager::stateManager_ = 0;

GameStateManager::GameStateManager(Context* context) :
    Object(context)
{
    // Initialize variables to sensible defaults
    activeState_ = 0;

    if (!stateManager_)
        stateManager_ = this;
}

GameStateManager::~GameStateManager()
{
    URHO3D_LOGINFO("~GameStateManager() ...");

    // End all currently running states
    Stop();

    // Reset variables
    activeState_.Reset();

    stateStack_.Clear();

    // Destroy any registered states
    URHO3D_LOGINFO("~GameStateManager() - Clear States ...");
    registeredStates_.Clear();
    URHO3D_LOGINFO("~GameStateManager() - Clear States ... OK !");

    if (stateManager_ == this)
        stateManager_ = 0;

    URHO3D_LOGINFO("~GameStateManager() ... OK ! ");
}

bool GameStateManager::RegisterState(GameState* pGameState)
{
    StateMap::Iterator itState;

    // Validate requirements
    if (!pGameState) return false;

    // State already exists in the state map?
    itState = registeredStates_.Find(pGameState->GetStateId());
    if (itState != registeredStates_.End()) return false;

    // If it doesn't exist, add it to the list of available state types
    registeredStates_[pGameState->GetStateId()] = pGameState;

    // Set up state properties
    pGameState->stateManager_ = this;

    // Initialize the state and return result
    return pGameState->Initialize();
}

GameState* GameStateManager::GetActiveState()
{
    return activeState_;
}

GameState* GameStateManager::GetPreviousActiveState() const
{
    return stateStack_.Back();
}

bool GameStateManager::SetActiveState(const String & stateId)
{
    StateMap::Iterator itState;

    // First find the requested state
    itState = registeredStates_.Find(stateId);
    if (itState == registeredStates_.End()) return false;

    // The state was found, end any currently executing states
    if (activeState_)
        activeState_->End();

    // Link this new state
    activeState_ = itState->second_;

    // Start it running.
    if (activeState_->IsBegun() == false)
        activeState_->Begin();

    // Success!!
    return true;
}

GameState* GameStateManager::GetState(const String & stateId)
{
    // First retrieve the details about the state specified, if none
    // was found this is clearly an invalid state identifier.
    StateMap::Iterator itState = registeredStates_.Find(stateId);
    if (itState == registeredStates_.End())
        return 0;
    return itState->second_;
}

void GameStateManager::Stop()
{
    URHO3D_LOGINFO("GameStateManager() - Stop");
    // Stop all active states
    if (activeState_)
    {
        URHO3D_LOGINFOF("GameStateManager() - Stop : End activeState = %s ...", activeState_->GetStateId().CString());
        activeState_->End();
    }
}

void GameStateManager::StateEnded(GameState * pState)
{
    if (pState == 0) return;

    // If this was at the root of the state history, we're all out of states
    if (pState == activeState_)
        activeState_.Reset();
}

void GameStateManager::ClearStack()
{
    stateStack_.Clear();
}

void GameStateManager::StartStack()
{
    GameState* state = stateStack_.Back();
    if (state)
        bool b = SetActiveState(state->GetStateId());
}

bool GameStateManager::AddToStack(const String& stateId)
{
    SharedPtr<GameState> state(GetState(stateId));

    if (state)
    {
        stateStack_.Push(state);
        return true;
    }
    return false;
}

bool GameStateManager::PushToStack(const String& stateId)
{
    bool b = SetActiveState(stateId);
    if (b)
    {
        URHO3D_LOGINFOF("PushToStack %s !", stateId.CString());
        stateStack_.Push(activeState_);
    }
    return b;
}

const String& GameStateManager::PopStack()
{
    if (stateStack_.Size() != 0)
    {
        // pop active state
        stateStack_.Pop();
        // get the last active state
        GameState* s = 0;
        if (stateStack_.Size() != 0)
        {
            s = stateStack_.Back();
        }

        if (s && SetActiveState(s->GetStateId()))
            return s->GetStateId();
    }
    URHO3D_LOGINFO("StateStack Empty!");
    return String::EMPTY;
}

