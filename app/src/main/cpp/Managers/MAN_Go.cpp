#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Core/CoreEvents.h>

#include "GameAttributes.h"
#include "GameEvents.h"

#include "MAN_Go.h"


GOManager* GOManager::goManager_=0;

unsigned GOManager::GetNumEnemies()
{
    return goManager_ ? goManager_->enemy.Size() : 0;
}

unsigned GOManager::GetNumPlayers()
{
    return goManager_ ? goManager_->player.Size() : 0;
}

unsigned GOManager::GetNumActiveBots()
{
    return goManager_ ? goManager_->activeBots.Size() : 0;
}

unsigned GOManager::GetNumActivePlayers()
{
    return goManager_ ? goManager_->activePlayer.Size() : 0;
}

unsigned GOManager::GetNumActiveEnemies()
{
    return goManager_ ? goManager_->activeEnemy.Size() : 0;
}

unsigned GOManager::GetNumActiveAiNodes()
{
    return goManager_ ? goManager_->activeAiNodes.Size() : 0;
}

unsigned GOManager::GetLastUpdate()
{
    return goManager_ ? goManager_->lastUpdate_ : 0;
}

bool GOManager::IsA(unsigned nodeId, int controllerType)
{
    if (!goManager_)
        return false;

    if (controllerType & (GO_Player|GO_NetPlayer))
    {
        return goManager_->player.Size() && goManager_->player.Contains(nodeId);
    }
    else if (controllerType & GO_AI_Enemy)
    {
        return goManager_->enemy.Size() && goManager_->enemy.Contains(nodeId);
    }
    else if (controllerType & GO_AI_Ally)
    {
        return goManager_->activeAiNodes.Size() && goManager_->activeAiNodes.Contains(nodeId);
    }

    return false;
}

GOManager::GOManager(Context* context) :
    Object(context)
{
    URHO3D_LOGINFO("GOManager()");
    goManager_ = this;
}

GOManager::~GOManager()
{
    URHO3D_LOGINFO("~GOManager()");
    goManager_ = 0;
}

void GOManager::RegisterObject(Context* context)
{
//    context->RegisterObject<GOManager>();
}

void GOManager::Start()
{
    URHO3D_LOGINFO("GOManager() - ---------------------------------------");
    URHO3D_LOGINFO("GOManager() - Start !");
    URHO3D_LOGINFO("GOManager() - ---------------------------------------");

    Reset(false);

    SubscribeToEvent(GO_APPEAR, URHO3D_HANDLER(GOManager, HandleGOAppear));
    SubscribeToEvent(GO_DESTROY, URHO3D_HANDLER(GOManager, HandleGODestroy));

    SubscribeToEvent(GOC_LIFERESTORE, URHO3D_HANDLER(GOManager,HandleGOActive));
    SubscribeToEvent(GOC_LIFEDEAD, URHO3D_HANDLER(GOManager,HandleGODead));

    SubscribeToEvent(GOC_CONTROLLERCHANGE, URHO3D_HANDLER(GOManager,HandleGOChangeType));
}

void GOManager::Stop()
{
    UnsubscribeFromEvent(GO_APPEAR);
    UnsubscribeFromEvent(GO_DESTROY);

    UnsubscribeFromEvent(GOC_LIFERESTORE);
    UnsubscribeFromEvent(GOC_LIFEDEAD);

    UnsubscribeFromEvent(GOC_CONTROLLERCHANGE);

    URHO3D_LOGINFO("GOManager() - ---------------------------------------");
    URHO3D_LOGINFO("GOManager() - Stop !");
    URHO3D_LOGINFO("GOManager() - ---------------------------------------");
}



void GOManager::Reset(bool keepPlayers)
{
    if (!bots.Empty())
        bots.Clear();
    if (!activeBots.Empty())
        activeBots.Clear();
    if (!enemy.Empty())
        enemy.Clear();
    if (!activeEnemy.Empty())
        activeEnemy.Clear();
    if (!activeAiNodes.Empty())
        activeAiNodes.Clear();

    if (!keepPlayers)
    {
        if (!player.Empty())
            player.Clear();
        if (!activePlayer.Empty())
            activePlayer.Clear();
    }
    URHO3D_LOGINFOF("GOManager() - Reset : activePlayer Size=%u player Size=%u (keepedPlayer=%s)",
                    activePlayer.Size(), player.Size(), keepPlayers?"true":"false");
}

int GOManager::firstActivePlayer()
{
    if (activePlayer.Size() > 0)
        return (player[0]);

    return -1;
}

void GOManager::HandleGOAppear(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GO_APPEAR)
    {
        if (eventData[Go_Appear::GO_TYPE].GetInt() == GO_None)
            return;

        unsigned nodeId = eventData[Go_Appear::GO_ID].GetUInt();
        int goType = eventData[Go_Appear::GO_TYPE].GetInt();
        bool mainController = eventData[Go_Appear::GO_MAINCONTROL].GetBool();

//        URHO3D_LOGERRORF("GOManager() - HandleGOAppear : nodeid=%u type=%d mainctrl=%u", nodeId, goType, mainController);

        switch (goType)
        {
        case GO_AI :
            if (!bots.Contains(nodeId))
            {
                bots.Push(nodeId);
                activeBots.Push(nodeId);
            }
        case GO_AI_Enemy :
            if (!enemy.Contains(nodeId))
                enemy.Push(nodeId);
            if (mainController)
            {
//                URHO3D_LOGERRORF("GOManager() - HandleGOAppear : Enemy id=%u appeared : num = %u", nodeId, enemy.Size());
                if (!activeEnemy.Contains(nodeId))
                    activeEnemy.Push(nodeId);
                if (!activeAiNodes.Contains(nodeId))
                {
                    activeAiNodes.Push(nodeId);
                    lastUpdate_++;
                }
            }
            break;
        case GO_AI_None :
        case GO_AI_Ally :
            if (mainController && !activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Push(nodeId);
                lastUpdate_++;
//                URHO3D_LOGINFOF("GOManager() - HandleGOAppear : Ally id=%u appeared : num = %u", nodeId, enemy.Size() - activeAiNodes.Size());
            }
            break;
        case GO_Player :
        case GO_NetPlayer :
            if (!player.Contains(nodeId))
            {
                player.Push(nodeId);
            }
            if (!activePlayer.Contains(nodeId))
            {
                activePlayer.Push(nodeId);
                URHO3D_LOGERRORF("GOManager() - HandleGOAppear : Player appeared : nodeid=%u numplayers=%u", nodeId, activePlayer.Size());
            }
            break;
        }
    }
}

void GOManager::HandleGODestroy(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GO_DESTROY)
    {
        if (eventData[Go_Destroy::GO_TYPE].GetUInt() == GO_None)
            return;

        unsigned nodeId = eventData[Go_Destroy::GO_ID].GetUInt();
        int goType = eventData[Go_Destroy::GO_TYPE].GetInt();

//        URHO3D_LOGINFOF("GOManager() - HandleGODestroy : GO DESTROY node=%u type=%d", nodeId, goType);

        switch (goType)
        {
        case GO_AI :
            if (bots.Contains(nodeId))
            {
                bots.Remove(nodeId);
                activeBots.Remove(nodeId);
            }
        case GO_AI_Enemy :
            if (enemy.Contains(nodeId))
            {
                enemy.Remove(nodeId);
                activeEnemy.Remove(nodeId);
//                URHO3D_LOGINFOF("GOManager() - HandleGODestroy : Enemy destroyed : num = %u", enemy.Size());
            }
            if (activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Remove(nodeId);
                lastUpdate_++;
            }
            break;
        case GO_AI_None :
        case GO_AI_Ally :
            if (activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Remove(nodeId);
                lastUpdate_++;
//                URHO3D_LOGINFOF("GOManager() - HandleGOAppear : Ally destroyed : num = %u", activeAiNodes.Size());
            }
            break;
        case GO_Player :
        case GO_NetPlayer :
            if (player.Contains(nodeId))
            {
                player.Remove(nodeId);
//                URHO3D_LOGINFOF("GOManager() - HandleGODestroy : Player destroyed : numPlayerAlive = %u", player.Size());
            }
            if (activePlayer.Contains(nodeId))
            {
                activePlayer.Remove(nodeId);
            }
            break;

        }
    }
}

void GOManager::HandleGOActive(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GOC_LIFERESTORE)
    {
        unsigned nodeId = eventData[GOC_Life_Events::GO_ID].GetUInt();
        int goType =  eventData[GOC_Life_Events::GO_TYPE].GetInt();
        bool mainController = eventData[GOC_Life_Events::GO_MAINCONTROL].GetBool();

//        URHO3D_LOGINFOF("GOManager() - HandleGOActive : GO GOC_LIFERESTORE node=%u type=%d", nodeId, goType);
        switch (goType)
        {
        case GO_AI :
            if (!activeBots.Contains(nodeId))
            {
                activeBots.Push(nodeId);
            }
        case GO_AI_Enemy :
            if (mainController)
            {
                if (!activeEnemy.Contains(nodeId))
                    activeEnemy.Push(nodeId);
                if (!activeAiNodes.Contains(nodeId))
                {
                    activeAiNodes.Push(nodeId);
                    lastUpdate_++;
                }
//                URHO3D_LOGINFOF("GOManager() - HandleGOActive : Enemy %u active, numActiveEnemies = %u", nodeId, activeEnemy.Size());
            }
            break;
        case GO_AI_None :
        case GO_AI_Ally :
            if (mainController && !activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Push(nodeId);
                lastUpdate_++;
//                URHO3D_LOGINFOF("GOManager() - HandleGOActive : Ally %u active : num = %u", nodeId, activeAiNodes.Size() - enemy.Size());
            }
            break;
        case GO_NetPlayer :
        case GO_Player :
            if (!activePlayer.Contains(nodeId))
            {
                activePlayer.Push(nodeId);
                URHO3D_LOGERRORF("GOManager() - HandleGOActive : Avatar %u active, numActiveplayers = %u", nodeId, activePlayer.Size());
            }
            break;
        }
    }
}

void GOManager::HandleGODead(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GOC_LIFEDEAD)
    {
        unsigned nodeId = eventData[GOC_Life_Events::GO_ID].GetUInt();
        int goType =  eventData[GOC_Life_Events::GO_TYPE].GetInt();
//        URHO3D_LOGINFOF("GOManager() - HandleGODead : GO GOC_LIFEDEAD node=%u type=%d", nodeId, goType);
        switch (goType)
        {
        case GO_AI :
            if (activeBots.Contains(nodeId))
            {
                activeBots.Remove(nodeId);
            }
        case GO_AI_None :
        case GO_AI_Ally :
            if (activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Remove(nodeId);
                lastUpdate_++;
//                URHO3D_LOGINFOF("GOManager() - HandleGODead : Ally %u dead : num = %u", nodeId, activeAiNodes.Size() - enemy.Size());
            }
            break;
        case GO_AI_Enemy :
            if (activeEnemy.Contains(nodeId))
            {
                activeEnemy.Remove(nodeId);
                activeAiNodes.Remove(nodeId);
                lastUpdate_++;
//                URHO3D_LOGINFOF("GOManager() - HandleGODead : Enemy %u dead, numActiveEnemies = %u", nodeId, activeEnemy.Size());
            }
            break;
        case GO_NetPlayer :
        case GO_Player :
            if (activePlayer.Contains(nodeId))
            {
                activePlayer.Remove(nodeId);
//                URHO3D_LOGINFOF("GOManager() - HandleGODead : Avatar %u dead, numActiveplayers = %u", nodeId, activePlayer.Size());
            }
            break;
        }
    }
}

void GOManager::HandleGOChangeType(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOManager() - HandleGOChangeType");
    if (eventType == GOC_CONTROLLERCHANGE)
    {
        unsigned nodeId = eventData[ControllerChange::NODE_ID].GetUInt();
        int goType = eventData[ControllerChange::GO_TYPE].GetInt();
        bool mainController = eventData[ControllerChange::GO_MAINCONTROL].GetBool();

        switch (goType)
        {
        case GO_AI_None :
        case GO_AI_Ally :
            if (enemy.Contains(nodeId))
            {
                enemy.Remove(nodeId);
                activeEnemy.Remove(nodeId);
            }
            if (mainController && !activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Push(nodeId);
                lastUpdate_++;
//                URHO3D_LOGINFOF("GOManager() - HandleGOChangeType : Enemy change to AI Ally : numActiveAIs = %u", enemy.Size() - activeAiNodes.Size());
            }
            break;
        case GO_AI_Enemy :
            if (player.Contains(nodeId))
            {
                player.Remove(nodeId);
                activePlayer.Remove(nodeId);
//                URHO3D_LOGINFOF("GOManager() - HandleGOChangeType : Player change to Enemy : numPlayers = %u", player.Size());
            }
            if (!enemy.Contains(nodeId))
            {
                enemy.Push(nodeId);
                activeEnemy.Push(nodeId);
                if (mainController && !activeAiNodes.Contains(nodeId))
                {
                    activeAiNodes.Push(nodeId);
                    lastUpdate_++;
                }
//                URHO3D_LOGINFOF("GOManager() - HandleGOChangeType : Enemy %u Added to GOManager : numEnemies = %u", nodeId, enemy.Size());
            }
            break;
        case GO_Player :
        case GO_NetPlayer :
            if (enemy.Contains(nodeId))
            {
                enemy.Remove(nodeId);
                activeEnemy.Remove(nodeId);
                activeAiNodes.Remove(nodeId);
                lastUpdate_++;
                URHO3D_LOGERRORF("GOManager() - HandleGOChangeType : Enemy change to Player : numEnemies = %u", enemy.Size());
                if (!player.Contains(nodeId))
                {
                    player.Push(nodeId);
                    activePlayer.Push(nodeId);
                    URHO3D_LOGERRORF("GOManager() - HandleGOChangeType : Player Added to GOManager : numPlayers = %u", player.Size());
                }
            }
            else if (activeAiNodes.Contains(nodeId))
            {
                activeAiNodes.Remove(nodeId);
                lastUpdate_++;
                URHO3D_LOGERRORF("GOManager() - HandleGOChangeType : AI Player change to Player : numActiveAiNodes = %u", activeAiNodes.Size());
            if (!player.Contains(nodeId))
            {
                player.Push(nodeId);
                activePlayer.Push(nodeId);
                    URHO3D_LOGERRORF("GOManager() - HandleGOChangeType : Player Added to GOManager : numPlayers = %u", player.Size());
            }
            }
            break;
        }
    }
}

void GOManager::Dump() const
{
    URHO3D_LOGINFOF("GOManager() - Dump : Players size = %u ...", player.Size());
    for (unsigned i=0; i < player.Size(); i++)
        URHO3D_LOGINFOF("GOManager() - Dump : Player[%d] = (id=%u)", i, player[i]);

    URHO3D_LOGINFOF("GOManager() - Dump : ActivePlayers size = %u ...", activePlayer.Size());
    for (unsigned i=0; i < activePlayer.Size(); i++)
        URHO3D_LOGINFOF("GOManager() - Dump : ActivePlayer[%d] = (id=%u)", i, activePlayer[i]);
}
