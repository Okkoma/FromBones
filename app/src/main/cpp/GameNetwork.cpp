#ifdef WINDOWS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#endif // WINDOWS

#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/MemoryBuffer.h>

#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Graphics/Graphics.h>

#include <Urho3D/Resource/Localization.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SmoothedTransform.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>

#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameStateManager.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameCommands.h"
#include "GameStateManager.h"
#include "sPlay.h"

#include "GOC_Animator2D.h"
#include "GOC_BodyExploder2D.h"
#include "GOC_Destroyer.h"
#include "GOC_DropZone.h"
#include "GOC_Controller.h"
#include "GOC_Collectable.h"
#include "GOC_Inventory.h"
#include "GOC_Life.h"
#include "GOC_Move2D.h"
#include "GOC_Collide2D.h"
#include "GOC_PhysicRope.h"

#include "MAN_Weather.h"
#include "Player.h"

#include "Map.h"
#include "MapStorage.h"
#include "MapWorld.h"
#include "ViewManager.h"

#include "ObjectPool.h"

#include "TimerRemover.h"

#include "TextMessage.h"

#include "GameNetwork.h"

#include <cstdio>


#define NET_DEFAULT_UPDATEFPS 15
/// Latency recommended = 100ms to 500ms
/// Lost Packet recommended = 5% to 50%
#define NET_DEFAULT_LATENCY 300 // latency 500ms
#define NET_DEFAULT_LOSTPACKET 0.25f // probability of 25%
#define NET_DEFAULT_PACKETRESEND_TICK 10

#define NET_DEFAULT_SMOOTHCONSTANT 10.f
#define NET_DEFAULT_SMOOTHTHRESHOLD 0.001f

//#define NETSERVER_CHECKACCOUNT

#define NET_CLIENTMAXDESACTIVETIME 100

#define NET_MAXSERVEROBJECTCONTROLTOSEND 60
#define NET_MAXCLIENTOBJECTCONTROLTOSEND 20

//#define FORCE_SERVERMODE
//#define FORCE_LOCALMODE

//#define ACTIVE_NETWORK_DEBUGSERVER_RECEIVE
//#define ACTIVE_NETWORK_DEBUGSERVER_RECEIVE_ALL
//#define ACTIVE_NETWORK_DEBUGSERVER_SEND_SERVEROBJECTS
//#define ACTIVE_NETWORK_DEBUGSERVER_SEND_CLIENTOBJECTS
//#define ACTIVE_NETWORK_DEBUGSERVER_SEND_CLIENTUPDATE

//#define ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE
//#define ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE_ALL
//#define ACTIVE_NETWORK_DEBUGCLIENT_SEND

#define ACTIVE_NETWORK_DEBUGSERVER_OBJECTCOMMANDS
#define ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS

const String NET_DEFAULT_SERVERIP = String("localhost");
const int NET_DEFAULT_SERVERPORT  = 2345;

const float NET_DELAYCHECKCONNECTION = 5.f;
const float NET_DELAYCONNECTSERVER   = 25.f;

//const float DELAY_UPDATENETPOSITIONS = 2.f;

extern const char* gameStatusNames[];
extern const char* netCommandNames[];



inline bool IsNewStamp(unsigned short int stamp, unsigned short int stampref, unsigned short int maxstamp=STAMP_MAXDELTASHORT)
{
    // delta = a - b;
    // (delta >= 0) check if it's an acceptable gap
    // (delta < 0) check if it's an acceptable overflow (a<MAX_DELTASTAMP && b >65535-MAX_DELTASTAMP)
    int delta = stamp-(int)stampref;
    return !delta ? false : (delta > 0 ? (delta < maxstamp) : (65535U + delta < maxstamp));
}

inline bool IsNewStamp(unsigned char stamp, unsigned char stampref, unsigned char maxstamp=STAMP_MAXDELTABYTE)
{
    // delta = a - b;
    // (delta >= 0) check if it's an acceptable gap
    // (delta < 0) check if it's an acceptable overflow (a<MAX_DELTASTAMP && b >255-MAX_DELTASTAMP)
    int delta = stamp-(int)stampref;
    return !delta ? false : (delta > 0 ? (delta < maxstamp) : (255U + delta < maxstamp));
}

inline bool IsNewOrEqualStamp(unsigned char stamp, unsigned char stampref, unsigned char maxstamp=STAMP_MAXDELTABYTE)
{
    // delta = a - b;
    // (delta >= 0) check if it's an acceptable gap
    // (delta < 0) check if it's an acceptable overflow (a<MAX_DELTASTAMP && b >255-MAX_DELTASTAMP)
    int delta = stamp-(int)stampref;
    return !delta ? true : (delta > 0 ? (delta < maxstamp) : (255U + delta < maxstamp));
}

template< typename T > bool GameNetwork::CheckNewStamp(T stamp, T stampref, T maxdelta) { return IsNewStamp(stamp, stampref, maxdelta); }
template bool GameNetwork::CheckNewStamp(unsigned short int stamp, unsigned short int stampref, unsigned short int maxdelta);
template bool GameNetwork::CheckNewStamp(unsigned char stamp, unsigned char stampref, unsigned char maxdelta);


void LogStatNetObject::Clear()
{
    memset(stats_, 0, 9 * sizeof(unsigned int));
}

void LogStatNetObject::Copy(LogStatNetObject& logstat)
{
    memcpy(logstat.stats_, stats_, 9 * sizeof(unsigned int));
}


void ObjectCommandInfo::Clear()
{
    objCmdHead_ = 1U;
    for (int i = 0; i < 256; i++)
        objCmdPacketsReceived_[i].Clear();

    objCmdToSendStamp_ = 0U;
    newSpecificPacketToSend_ = 0;

    objCmdPacketsToSend_.Clear();
    objCmdPacketStampsToSend_.Clear();
}

/// ClientInfo

ClientInfo::ClientInfo() :
    clientID_(0),
    gameStatus_(MENUSTATE),
    playersStarted_(false),
    requestPlayers_(0),
    rebornRequest_(false),
    mapsDirty_(true)
{
#ifdef ACTIVE_NETWORK_LOGSTATS
    logStats_.Clear();
#endif
}

ClientInfo::~ClientInfo()
{
    Clear();
}

Node* ClientInfo::CreateAvatarFor(unsigned playerindex)
{
    URHO3D_LOGINFOF("ClientInfo() - CreateAvatarFor : clientid=%d playerindex=%u ...", clientID_, playerindex);

    Player* player = players_[playerindex];

    if (!player)
    {
        URHO3D_LOGERRORF("ClientInfo() - CreateAvatarFor : clientid=%d playerindex=%u ... no player !", clientID_, playerindex);
        return 0;
    }

    unsigned nodeID = GameContext::Get().FIRSTAVATARNODEID + clientID_ * GameContext::Get().MAX_NUMPLAYERS + playerindex;
    unsigned playerID = player->GetID();

    player->Set(0, false, playerindex);
    player->active = true;
    player->SetController(false, connection_.Get(), nodeID, clientID_);

    WorldMapPosition startposition;
    if (GameContext::Get().arenaZoneOn_)
    {
        bool result = GameContext::Get().GetStartPosition(startposition, clientID_);
    }
    else
    {
        startposition = GameContext::Get().worldStartPosition_;
    }

    player->SetScene(GameContext::Get().rootScene_, startposition.position_ + Vector2(WORLD_TILE_WIDTH * playerID, 0.f), startposition.viewZ_, false, true, true);
    player->SetFaction((clientID_ << 8) + GO_Player);

    Node* avatar = player->GetAvatar();

    URHO3D_LOGINFOF("ClientInfo() - CreateAvatarFor : ... ClientID=%d connection=%u ... playerID=%u nodeID=%u position=%s faction=%u !",
                    clientID_, connection_.Get(), playerID, avatar->GetID(), avatar->GetWorldPosition().ToString().CString(), player->GetFaction());

    if (avatar)
    {
        ObjectControlInfo& objectControlInfo = GameNetwork::Get()->GetOrCreateServerObjectControl(avatar->GetID(), avatar->GetID(), clientID_, avatar);
        if (GameNetwork::Get()->PrepareControl(objectControlInfo))
        {
            objectControlInfo.SetActive(true, true);
            URHO3D_LOGINFOF("ClientInfo() - CreateAvatarFor : ClientID=%d player activated servernodeid=%u clientnodeid=%u oinfo=%u node=%s(%u) ... OK !",
                            clientID_, objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_, &objectControlInfo, avatar->GetName().CString(), avatar->GetID());
        }

        objects_.Push(WeakPtr<Node>(avatar));
        avatar->AddTag("Player");

        World2D::GetOrCreateTraveler(avatar).oinfo_ = &objectControlInfo;
    }

    return avatar;
}

void ClientInfo::AddPlayer()
{
    URHO3D_LOGINFOF("---------------------------------------------------------------------------------");
    URHO3D_LOGINFOF("ClientInfo() - AddPlayer : ... for clientid=%d  ...", clientID_, connection_.Get());

    players_.Resize(players_.Size()+1);
    Player* player = players_.Back() = new Player(connection_->GetContext(), 0);

    URHO3D_LOGINFOF("ClientInfo() - AddPlayer : ... for clientid=%d connection=%u ... playerID=%u ... OK !", clientID_, connection_.Get(), player->GetID());
    URHO3D_LOGINFOF("---------------------------------------------------------------------------------");
}

void ClientInfo::Clear()
{
    URHO3D_LOGINFOF("ClientInfo() - Clear : ...");

    gameStatus_ = MENUSTATE;
    mapsDirty_ = true;
    mapRequests_.Clear();
    rebornRequest_ = false;

    ClearObjects();
    ClearPlayers();

    URHO3D_LOGINFOF("ClientInfo() - Clear : ... OK !");
}

void ClientInfo::ClearObjects()
{
//    URHO3D_LOGINFOF("ClientInfo() - ClearObjects : ...");

    for (unsigned i=0; i < objects_.Size(); i++)
    {
        if (objects_[i])
        {
            Node* node = objects_[i];
            if (connection_)
                node->CleanupConnection(connection_);

            // Umount if need
            MountInfo mountinfo(node);
            GameHelpers::UnmountNode(mountinfo);

            if (node->HasTag(PLAYERTAG))
                World2D::RemoveTraveler(node);

            node->Remove();
        }
    }

    objects_.Clear();

//    URHO3D_LOGINFOF("ClientInfo() - ClearObjects : ... OK !");
}

void ClientInfo::ClearPlayers()
{
//    URHO3D_LOGINFOF("ClientInfo() - ClearPlayers : ...");

    playersStarted_ = false;
    requestPlayers_ = 0;
    players_.Clear();

//    URHO3D_LOGINFOF("ClientInfo() - ClearPlayers : ... OK !");
}

void ClientInfo::StartPlayers()
{
    if (playersStarted_)
        return;

    URHO3D_LOGINFOF("ClientInfo() - StartPlayers ... ClientID=%d connection=%u ...", clientID_, connection_.Get());

    for (Vector<SharedPtr<Player> >::Iterator it=players_.Begin(); it != players_.End(); ++it)
    {
        Player* player = *it;
        if (player && !player->IsStarted())
        {
            player->Start();

            if (GameNetwork::Get())
                GameNetwork::Get()->SetEnableObject(true, player->GetAvatar()->GetID(), true);
        }
    }

    URHO3D_LOGINFOF("ClientInfo() - StartPlayers ... ClientID=%d connection=%u ... OK !", clientID_, connection_.Get());

    playersStarted_ = true;
}

void ClientInfo::StopPlayers()
{
    if (!playersStarted_)
        return;

    URHO3D_LOGINFOF("ClientInfo() - StopPlayers ... ClientID=%d connection=%u ...", clientID_, connection_.Get());

    for (Vector<SharedPtr<Player> >::Iterator it=players_.Begin(); it != players_.End(); ++it)
    {
        Player* player = *it;
        if (player && player->IsStarted())
        {
            if (GameNetwork::Get())
                GameNetwork::Get()->SetEnableObject(false, player->GetAvatar()->GetID(), true);

            player->Stop();
        }
    }

    URHO3D_LOGINFOF("ClientInfo() - StopPlayers ... ClientID=%d connection=%u ... OK !", clientID_, connection_.Get());

    playersStarted_ = false;
}

bool ClientInfo::NeedAllocatePlayers() const
{
    return requestPlayers_ > players_.Size();
}

int ClientInfo::GetNumActivePlayers() const
{
    int numactiveplayers = 0;
    for (Vector<SharedPtr<Player> >::ConstIterator it=players_.Begin(); it!=players_.End(); ++it)
    {
        Player* player = it->Get();
        if (player->active)
            numactiveplayers++;
    }
    return numactiveplayers;
}

Player* ClientInfo::GetPlayerFor(unsigned nodeid) const
{
    for (Vector<SharedPtr<Player> >::ConstIterator it=players_.Begin(); it != players_.End(); ++it)
    {
        if ((*it)->GetAvatar()->GetID() == nodeid)
            return it->Get();
    }
    return 0;
}

bool ClientInfo::ContainsObject(Node* node) const
{
    return false;
//    return objects_.Contains(node);
}

void ClientInfo::Dump() const
{
    URHO3D_LOGINFOF("ClientInfo() - Dump : clientID=%d connection=%u status=%s(%d) playersStarted=%s requestPlayers=%d ... numplayers=%u numobjects=%u !",
                    clientID_, connection_.Get(), gameStatusNames[gameStatus_], gameStatus_, playersStarted_?"true":"false" , requestPlayers_, players_.Size(), objects_.Size());
}


/// GameNetwork

Vector<Connection*> connectionsToSetScene;
VariantMap sServerEventData_;
VariantMap sClientEventData_;

GameNetwork* GameNetwork::network_=0;

GameNetwork::GameNetwork(Context* context)  :
    Object(context),
    serverIP_(NET_DEFAULT_SERVERIP),
    serverPort_(NET_DEFAULT_SERVERPORT),
    delayForConnection_(NET_DELAYCHECKCONNECTION),
    delayForServer_(NET_DELAYCONNECTSERVER),
    networkMode_(AUTOMODE)
{
    if (network_)
        network_->~GameNetwork();

    network_ = this;
    GameContext::Get().gameNetwork_ = this;

    // Urho3D Network Configuration
    net_ = GetSubsystem<Network>();

    // Events sent between client & server (remote events) must be explicitly registered or else they are not allowed to be received
    net_->RegisterRemoteEvent(NET_GAMESTATUSCHANGED);

    net_->SetUpdateFps(NET_DEFAULT_UPDATEFPS);

    objCmdPool_.Resize(100U);
    objCmdPckPool_.Resize(500U);

#ifdef ACTIVE_NETWORK_SIMULATOR
    net_->SetSimulatedLatency(NET_DEFAULT_LATENCY);
    net_->SetSimulatedPacketLoss(NET_DEFAULT_LOSTPACKET);
#endif
#ifdef ACTIVE_NETWORK_LOGSTATS
    logStatEnable_ = false;
#endif
    Reset();
}

GameNetwork::~GameNetwork()
{
    GameContext::Get().gameNetwork_ = network_ = 0;

    if (net_)
        net_->Stop();
}

/// static ClientMode() : return if this peer is a client (if true client or not defined (neither server))
bool GameNetwork::ClientMode()
{
    if (!network_)
        return false;

    return network_->IsClient() || (!network_->IsServer() && !network_->IsClient());
}

/// static ServerMode() : return if this peer is the server
bool GameNetwork::ServerMode()
{
    if (!network_)
        return false;

    return network_->IsServer();
}

/// static LocalMode() : return if this peer is not on a network
bool GameNetwork::LocalMode()
{
    return !network_;
}

void GameNetwork::AddGraphicMessage(Context* context, const String& msg, const IntVector2& position, int colortype, float duration, float delaystart)
{
#ifdef ACTIVE_NETWORK_GRAPHICMESSAGES
    TextMessage* netMessage = new TextMessage(context);

    if (colortype == WHITEGRAY30)
    {
        netMessage->Set(msg, GameContext::Get().txtMsgFont_, 30, duration, position, 1.5f, true, delaystart);
        netMessage->SetColor(Color::WHITE, Color::WHITE, Color::GRAY, Color::GRAY);
    }
    else if (colortype == MAGENTABLACK30)
    {
        netMessage->Set(msg, GameContext::Get().txtMsgFont_, 30, duration, position, 1.f, true, delaystart);
        netMessage->SetColor(Color::BLACK, Color::BLACK, Color::MAGENTA, Color::MAGENTA);
    }
    else
    {
        netMessage->Set(msg, GameContext::Get().txtMsgFont_, 35, duration, position, 1.f, true, delaystart);
        netMessage->SetColor(Color::RED, Color::RED, Color::YELLOW, Color::YELLOW);
    }
#endif
}


/// GameNetwork Setters

void GameNetwork::Start()
{
    if (started_)
        return;

    Reset();

    if (net_)
        net_->Start();

    // Subscribe to network events
    SubscribeToEvent(E_CONNECTFAILED, URHO3D_HANDLER(GameNetwork, HandleConnectionStatus));
    SubscribeToEvent(E_SERVERDISCONNECTED, URHO3D_HANDLER(GameNetwork, HandleConnectionStatus));

#ifdef FORCE_LOCALMODE
    networkMode_ = LOCALMODE;
#endif // STARTSERVER

#ifdef FORCE_SERVERMODE
    networkMode_ = SERVERMODE;
#endif // STARTSERVER

    switch (networkMode_)
    {
    case LOCALMODE:
        URHO3D_LOGINFO("GameNetwork() - Start() : LocalMode ... OK ! ");
        StartLocal();
        break;

    case CLIENTMODE:
        URHO3D_LOGINFO("GameNetwork() - Start() : ClientMode ... ");
        TryConnection();
        break;

    case SERVERMODE:
        URHO3D_LOGINFO("GameNetwork() - Start() : ServerMode ... OK ! ");
        StartServer();
        break;

    // AUTODETECT MODE
    default:
        TryConnection();
    }

    // Set Scene Smoothing
    {
        GameContext::Get().rootScene_->SetSmoothingConstant(NET_DEFAULT_SMOOTHCONSTANT);
        GameContext::Get().rootScene_->SetSnapThreshold(NET_DEFAULT_SMOOTHTHRESHOLD);
    }

    started_ = true;
}

void GameNetwork::TryConnection()
{
    SubscribeToEvent(E_SERVERCONNECTED, URHO3D_HANDLER(GameNetwork, HandleSearchServer));
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(GameNetwork, HandleSearchServer));
    net_->Connect(serverIP_, serverPort_, GameContext::Get().rootScene_);
    AddGraphicMessage(context_, "Searching.Network...", IntVector2(0, -10), WHITEGRAY30, 5.f);
}

void GameNetwork::StartServer()
{
    serverMode_ = true;
    clientMode_ = false;
    clientID_ = 0;
    UnsubscribeFromEvent(E_UPDATE);

    /// Reserve Object Controls
    serverObjectControls_.Reserve(1000);
    clientObjectControls_.Reserve(1000);

    GameContext::Get().ResetNetworkStatics();
    GameContext::Get().numPlayers_ = 0;
    SubscribeToEvents();

    GetSubsystem<Network>()->StartServer(serverPort_, GameContext::Get().MAX_NUMNETPLAYERS);

    URHO3D_LOGINFOF("GameNetwork() - ServerMode !");

    AddGraphicMessage(context_, "Server.Mode.", IntVector2(20, -10), REDYELLOW35);

    SendEvent(NET_MODECHANGED);
}

void GameNetwork::StartClient()
{
    serverMode_ = false;
    clientMode_ = true;
    UnsubscribeFromEvent(E_UPDATE);

    clientSideConnection_ = GetSubsystem<Network>()->GetServerConnection();
    serverGameStatus_ = PLAYSTATE_INITIALIZING;

    if (clientSideConnection_)
    {
        /// Reserve Object Controls
        serverObjectControls_.Reserve(1000);
        clientObjectControls_.Reserve(1000);

        /// At start, enable only client side
        //SetEnabledServerObjectControls(clientSideConnection_, false);
        SetEnabledServerObjectControls(clientSideConnection_, true);
        SetEnabledClientObjectControls(clientSideConnection_, true);
    }

    GameContext::Get().ResetNetworkStatics();
    SubscribeToEvents();

    URHO3D_LOGINFOF("GameNetwork() - ClientMode !");

    AddGraphicMessage(context_, "Client.Mode.", IntVector2(20, -10), REDYELLOW35);

    SendEvent(NET_MODECHANGED);
}

void GameNetwork::StartLocal()
{
    Stop();

    serverMode_ = false;
    clientMode_ = false;
    clientID_ = 0;
    UnsubscribeFromEvent(E_UPDATE);

    clientSideConnection_.Reset();
    Clear();

    URHO3D_LOGINFOF("GameNetwork() - LocalMode !");

    AddGraphicMessage(context_, "Local.Mode.", IntVector2(20, -10), REDYELLOW35);

    // TODO Player respawn in local
    SendEvent(NET_MODECHANGED);

    if (GameContext::Get().stateManager_ && GameContext::Get().stateManager_->GetActiveState())
    {
        if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() == "Play")
        {
            PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
            playstate->RestartLevel(true);
        }
    }
}

void GameNetwork::Stop()
{
    UnsubscribeFromAllEvents();

    ClearScene();

    if (clientMode_ && clientSideConnection_)
    {
        SetEnabledServerObjectControls(clientSideConnection_, false);
        SetEnabledClientObjectControls(clientSideConnection_, false);

        clientSideConnection_->Disconnect(100);
    }

    if (net_)
        net_->Stop();

    URHO3D_LOGINFO("GameNetwork() - Stop ... OK !");

    Reset();
}

void GameNetwork::Clear()
{
    URHO3D_LOGINFO("GameNetwork() - Clear ... ");

    if (network_)
    {
        GameContext::Get().rootScene_->Clear(!network_->ClientMode(), false);
        network_->~GameNetwork();
    }
    else
        GameContext::Get().rootScene_->Clear(true, false);

    network_ = 0;
    GameContext::Get().gameNetwork_ = 0;

    GameContext::Get().ResetNetworkStatics();

    URHO3D_LOGINFO("GameNetwork() - Clear ... OK !");
}

void GameNetwork::Reset()
{
    seedTime_ = 0;
    clientID_ = 0;
    serverMode_ = false;
    clientMode_ = false;
    started_ = false;
    gameStatus_ = MENUSTATE;

    needSynchronization_ = true;
    allClientsSynchronized_ = allClientsRunning_ = false;

    timerCheckForServer_ = timerCheckForConnection_ = 0.f;

    /// Spawn Controls & Stamps
    spawnControls_.Resize(GameContext::Get().MAX_NUMNETPLAYERS);
    localSpawnStamps_.Resize(GameContext::Get().MAX_NUMNETPLAYERS);
    receivedSpawnStamps_.Resize(GameContext::Get().MAX_NUMNETPLAYERS);

    ClearSpawnControls();

    for (unsigned i=0; i < GameContext::Get().MAX_NUMNETPLAYERS; i++)
        receivedSpawnStamps_[i] = localSpawnStamps_[i] = 0;

#ifdef ACTIVE_NETWORK_LOGSTATS
    logStats_.Clear();
#endif

    PurgeObjects();
}

void GameNetwork::ClearScene()
{
    if (clientMode_)
    {
        if (clientSideConnection_)
        {
            /// TEST ...
            GameContext::Get().rootScene_->CleanupConnection(clientSideConnection_);
            /// ... TEST
            clientSideConnection_->SetScene(0);
        }
    }
//    else
//        updatePositions_ = 0.f;
}

void GameNetwork::SetMode(const String& mode)
{
    URHO3D_LOGINFO("GameNetwork() - SetMode() : mode=" + mode);

    if (mode == "server")
    {
        networkMode_ = SERVERMODE;
        Server_PopulateClientIDs();
    }
    else if (mode == "local")
        networkMode_ = LOCALMODE;
    else
        networkMode_ = AUTOMODE;
}

void GameNetwork::SetMode(int mode)
{
    URHO3D_LOGINFOF("GameNetwork() - SetMode() : mode=%d", mode);
    networkMode_ = (GameNetworkMode)mode;

    if (networkMode_ == SERVERMODE)
        Server_PopulateClientIDs();
}

void GameNetwork::SetServerInfo(const String& ip, int port)
{
    if (ip.Empty())
        return;

    URHO3D_LOGINFOF("GameNetwork() - SetServerInfo : IP=%s PORT=%d !", ip.CString(), port);
    serverIP_ = ip;
    serverPort_ = port;
}

void GameNetwork::SetSeedTime(unsigned time)
{
    seedTime_ = time;
}

void GameNetwork::SetGameStatus(GameStatus status, bool send)
{
    URHO3D_LOGINFOF("GameNetwork() - SetGameStatus : gameStatus_=%d status=%d ...", gameStatus_, status);

    if (serverMode_ && status != MENUSTATE && status < gameStatus_)
        return;

    if (status == MENUSTATE)
        SubscribeToMenuEvents();

    else if (status == PLAYSTATE_INITIALIZING)
        SubscribeToPlayEvents();

    if (gameStatus_ != status)
    {
        lastGameStatus_ = gameStatus_;
        gameStatus_ = status;
    }

    if (send)
    {
        if (serverMode_)
            Server_SendGameStatus(status);

        if (clientMode_)
            Client_SendGameStatus();
    }

    URHO3D_LOGINFOF("GameNetwork() - SetGameStatus : gameStatus_=%d ... OK !", gameStatus_);
}

#ifdef ACTIVE_NETWORK_SIMULATOR
void GameNetwork::SetSimulatorActive(bool enable)
{
    if (!net_)
        return;

    int latency = 0;
    float lostpacket = 0.f;

    if (enable)
    {
        latency = NET_DEFAULT_LATENCY;
        lostpacket = NET_DEFAULT_LOSTPACKET;
    }

    net_->SetSimulatedLatency(latency);
    net_->SetSimulatedPacketLoss(lostpacket);
}

bool GameNetwork::IsSimulatorActive() const
{
    if (net_)
        return net_->GetSimulatorActive();

    return false;
}
#endif

#ifdef ACTIVE_NETWORK_LOGSTATS
void GameNetwork::SetLogStatistics(bool enable)
{
    logStatEnable_ = enable;

//    if (clientSideConnection_)
//    {
//        clientSideConnection_->SetLogStatistics(enable);
//    }
//    else
//    {
//        for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
//        {
//            Connection* connection = it->first_;
//            connection->SetLogStatistics(enable);
//        }
//    }
}
bool GameNetwork::GetLogStatistics() const
{
//    if (clientSideConnection_)
//    {
//        return clientSideConnection_->GetLogStatistics();
//    }
//
//    for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
//    {
//        return it->first_->GetLogStatistics();
//    }

    return logStatEnable_;
}
#endif

void GameNetwork::SetEnabledClientObjectControls(Connection* connection, bool enable)
{
    if (connection)
        connection->AllowClientObjectControls(enable);
}

void GameNetwork::SetEnabledServerObjectControls(Connection* connection, bool enable)
{
    if (!connection)
        return;

    // On Client, Allow/Prevent connection to receive Server Object Controls Messages
    if (clientSideConnection_ && connection == clientSideConnection_)
    {
        clientSideConnection_->AllowServerObjectControls(enable);
    }
    // On Server, Allow/Prevent connection to read in the preparedServerMessageBuffer
    else
    {
        connection->AllowServerObjectControls(enable);
        connection->SetCommonServerObjectControlMessageBuffer(enable ? &preparedServerMessageBuffer_ : 0);
    }
}

void GameNetwork::SetEnableObject(bool enable, ObjectControlInfo& controlInfo, Node* node, bool forceactivation)
{
//    URHO3D_LOGINFOF("GameNetwork() - SetEnableObject : node=%u enable=%s ...", controlInfo.serverNodeID_, enable ? "true":"false");

    if (enable || forceactivation)
        controlInfo.SetActive(enable, serverMode_);

    controlInfo.SetEnable(enable);

    if (node)
    {
        node->SetEnabled(enable);
    }

    if (serverMode_ && forceactivation)
    {
        sServerEventData_.Clear();
        sServerEventData_[Net_ObjectCommand::P_NODEID] = controlInfo.serverNodeID_;
        sServerEventData_[Net_ObjectCommand::P_NODEISENABLED] = enable;
        PushObjectCommand(ENABLENODE);
    }

//    URHO3D_LOGINFOF("GameNetwork() - SetEnableObject : node=%u enable=%s ... OK !", controlInfo.serverNodeID_, enable ? "true":"false");
}

void GameNetwork::SetEnableObject(bool enable, unsigned nodeid, bool forceactivation)
{
    if (nodeid < 2)
        return;
//    URHO3D_LOGINFOF("GameNetwork() - SetEnableObject : node=%u enable=%s ...", nodeid, enable ? "true":"false");

    // set state "enable" server Object Control
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
        {
            if (serverMode_ && it->active_ == enable)
                break;

            SetEnableObject(enable, *it, GameContext::Get().rootScene_->GetNode(clientMode_ ? it->clientNodeID_: nodeid), forceactivation);

//            URHO3D_LOGINFOF("GameNetwork() - SetEnableObject : serverObjectControl id=%u active=%s !", nodeid, enable ? "true":"false");

            break;
        }
    }

    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
        {
            if (it->active_ == enable)
                break;

            SetEnableObject(enable, *it, GameContext::Get().rootScene_->GetNode(clientMode_ ? it->clientNodeID_: nodeid), forceactivation);

//            URHO3D_LOGINFOF("GameNetwork() - SetEnableObject : clientObjectControl id=%u active=%s !", nodeid, enable ? "true":"false");

            break;
        }
    }

//    URHO3D_LOGINFOF("GameNetwork() - SetEnableObject : node=%u ... OK !", nodeid);
}

void GameNetwork::PurgeObjects()
{
    // Remove server Object Controls
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        ObjectControlInfo& control = *it;

        control.SetActive(false, serverMode_);
        if (control.node_)
        {
            if (control.node_->IsEnabled())
                control.node_->SendEvent(WORLD_ENTITYDESTROY);

            URHO3D_LOGINFOF("GameNetwork() - PurgeObjects : serverObjectControls_ %u removed !", control.node_->GetID());

            control.node_.Reset();
        }
    }

    // Remove client Object Controls
    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        ObjectControlInfo& control = *it;

        control.SetActive(false, serverMode_);
        if (control.node_)
        {
            if (control.node_->IsEnabled())
                control.node_->SendEvent(WORLD_ENTITYDESTROY);

            URHO3D_LOGINFOF("GameNetwork() - PurgeObjects : clientObjectControls_ %u removed !", control.node_->GetID());

            control.node_.Reset();
        }
    }

    // Object Controls
    serverObjectControls_.Clear();
    clientObjectControls_.Clear();
    netPlayersInfos_.Clear();

    // Server Stuff : Purge ServerClientInfo
    if (serverMode_)
    {
        DumpClientInfos();
        Server_RemoveAllPlayers();
        Server_CleanUpAllConnections();
        DumpClientInfos();
    }

    // Client Stuff
//    if (clientMode_)
//        clientNodeIDs_.Clear();
}


/// GameNetwork Object Commands

void GameNetwork::ClearObjectCommands()
{
    CleanObjectCommandInfo(objCmdInfo_);

    objCmdPool_.Restore();
    objCmdPckPool_.Restore();
    objCmdNew_.Clear();
}

void GameNetwork::CleanObjectCommandInfo(ObjectCommandInfo& info)
{
    for (List<ObjectCommandPacket* >::Iterator it = info.objCmdPacketsToSend_.Begin(); it != info.objCmdPacketsToSend_.End(); ++it)
        objCmdPckPool_.RemoveRef(*it);

    info.Clear();
}

VariantMap& GameNetwork::GetClientEventData() { return sClientEventData_; }
VariantMap& GameNetwork::GetServerEventData() { return sServerEventData_; }

// Send Command
// if broadcast and client=0  : server send to all clients
// if broadcast and client>0  : send to all clients but exclude client
// if !broadcast and client>0 : send to client only
void GameNetwork::PushObjectCommand(NetCommand pcmd, VariantMap* eventDataPtr, bool broadcast, int client)
{
    VariantMap& eventData = !eventDataPtr ? (clientSideConnection_ ? sClientEventData_ : sServerEventData_) : *eventDataPtr;
    eventData[Net_ObjectCommand::P_COMMAND] = (int)pcmd;

    ObjectCommand& cmd = *objCmdPool_.Get();
    cmd.clientId_ = client;
    cmd.broadCast_ = broadcast;
    cmd.cmd_ = eventData;

    objCmdNew_.Push(&cmd);

    URHO3D_LOGERRORF("GameNetwork() - PushObjectCommand : clientid=%d(from:%d) broadcast=%s cmd=%s nodeid=%u !",
                     cmd.clientId_, clientID_, broadcast?"true":"false", netCommandNames[pcmd], eventData[Net_ObjectCommand::P_NODEID].GetUInt());
}

void GameNetwork::PushObjectCommand(NetCommand pcmd, ObjectCommand& cmd, bool broadcast, int client)
{
    cmd.clientId_ = client;
    cmd.broadCast_ = broadcast;
    cmd.cmd_[Net_ObjectCommand::P_COMMAND] = (int)pcmd;

    objCmdNew_.Push(&cmd);

    URHO3D_LOGERRORF("GameNetwork() - PushObjectCommand : clientid=%d(from:%d) broadcast=%s cmd=%s nodeid=%u !",
                     cmd.clientId_, clientID_, broadcast?"true":"false", netCommandNames[pcmd], cmd.cmd_[Net_ObjectCommand::P_NODEID].GetUInt());
}

void GameNetwork::PushObjectCommand(VariantMap& cmdvar, bool broadcast, int client)
{
    ObjectCommand& cmd = *objCmdPool_.Get();
    cmd.clientId_ = client;
    cmd.broadCast_ = broadcast;
    cmd.cmd_ = cmdvar;
    objCmdNew_.Push(&cmd);

    int pcmd = cmdvar[Net_ObjectCommand::P_COMMAND].GetInt();

    URHO3D_LOGERRORF("GameNetwork() - PushObjectCommand : clientid=%d(from:%d) broadcast=%s cmd=%s nodeid=%u !",
                     cmd.clientId_, clientID_, broadcast?"true":"false", netCommandNames[pcmd], cmd.cmd_[Net_ObjectCommand::P_NODEID].GetUInt());
}


void GameNetwork::ExplodeNode(VariantMap& eventData)
{
    const unsigned nodeid = eventData[Net_ObjectCommand::P_NODEID].GetUInt();
    Node* node = GameContext::Get().rootScene_->GetNode(nodeid);
    if (!node)
    {
        URHO3D_LOGERRORF("GameNetwork() - ExplodeNode : nodeid=%u no node !", nodeid);
        return;
    }

    URHO3D_LOGINFOF("GameNetwork() - ExplodeNode : %s(%u) ...", node->GetName().CString(), nodeid);

    GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
    if (animator)
        animator->SetNetState(StringHash(STATE_EXPLODE), 0, true);

    GOC_BodyExploder2D* bodyExploder = node->GetComponent<GOC_BodyExploder2D>();
    if (bodyExploder)
    {
        URHO3D_LOGINFOF("GameNetwork() - ExplodeNode : %s(%u) launch explode !", node->GetName().CString(), nodeid);
        bodyExploder->Explode(true);
    }
}

void GameNetwork::Server_UpdateInventory(int cmd, VariantMap& eventData)
{
    const unsigned holderid = eventData[Net_ObjectCommand::P_NODEIDFROM].GetUInt();
    Node* holder = holderid ? GameContext::Get().rootScene_->GetNode(holderid) : 0;
    if (!holder)
    {
        URHO3D_LOGERRORF("GameNetwork() - Server_UpdateInventory : no holder=%u !", holderid);
        return;
    }
    GOC_Inventory* inventory = holder->GetComponent<GOC_Inventory>();
    if (!inventory)
    {
        URHO3D_LOGERRORF("GameNetwork() - Server_UpdateInventory : holder=%s(%u) no inventory !", holder->GetName().CString(), holder->GetID());
        return;
     }

    if (cmd == DROPITEM)
    {
        // Client use UISlotPanel::UseSlotItem() that sends GO_DROPITEM and push the command DROP_ITEM
        //   if dropmode == 0 (allowdrop) that also create an Entity (who has surely an ObjectControl with spawncontrol ?)
        // Server receives a DropItem command with : nodeid of the holder, the idslot to drop, the dropmode
        //   check the qty and set the inventory on server side
        GOC_Inventory::NetServerDropItem(holder, eventData);

        URHO3D_LOGERRORF("GameNetwork() - Server_UpdateInventory : holder=%s(%u) DROPITEM !", holder->GetName().CString(), holder->GetID());
    }
    else if (cmd == SETITEM)
    {
        const unsigned idslot = eventData[Net_ObjectCommand::P_INVENTORYIDSLOT].GetUInt();
        StringHash got(eventData[Net_ObjectCommand::P_CLIENTOBJECTTYPE].GetUInt());

        // slot inventory update
        Slot::SetSlotData(inventory->GetSlot(idslot), eventData);

        URHO3D_LOGERRORF("GameNetwork() - Server_UpdateInventory : holder=%s(%u) SETITEM idslot=%u got=%s!", holder->GetName().CString(), holder->GetID(), idslot, GOT::GetType(got).CString());

//        inventory->Dump();
    }
}


void GameNetwork::Client_TransferItem(VariantMap& eventData)
{
    //URHO3D_LOGWARNINGF("GameNetwork() - Client_TransferItem : Dump eventData ...");
    //GameHelpers::DumpVariantMap(eventData);

    ObjectControlInfo* giverInfo = GetObjectControl(eventData[Go_InventoryGet::GO_GIVER].GetUInt());
    if (!giverInfo || !giverInfo->node_)
    {
        URHO3D_LOGWARNINGF("GameNetwork() - Client_TransferItem : nodeid=%u giverInfo=0 !", eventData[Go_InventoryGet::GO_GIVER].GetUInt());
        return;
    }

    ObjectControlInfo* getterInfo = GetObjectControl(eventData[Go_InventoryGet::GO_GETTER].GetUInt());
    if (!getterInfo || !getterInfo->node_)
    {
        URHO3D_LOGWARNINGF("GameNetwork() - Client_TransferItem : getterInfo=0 !");
        return;
    }

    unsigned int qty = eventData[Go_InventoryGet::GO_QUANTITY].GetUInt();
    if (!qty)
    {
        URHO3D_LOGWARNINGF("GameNetwork() - Client_TransferItem : nodeid=%u ... qty=0 !", getterInfo->node_->GetID());
        return;
    }

    GOC_Inventory* inventory = giverInfo->node_->GetComponent<GOC_Inventory>();
    if (inventory)
    {
        unsigned slotid = eventData[Go_InventoryGet::GO_IDSLOTSRC].GetUInt();

        URHO3D_LOGINFOF("GameNetwork() - Client_TransferItem : inventory giver transferslot slotid=%u qty=%u ...", slotid, qty);

        // how to be sure to have the slot setted from the server, in case of not good replication of the inventories client/server ?
        //inventory->Dump();

        inventory->TransferSlotTo(slotid, getterInfo->node_, qty);
//        bool ok = inventory->CheckEmpty();

        return;
    }

    GOC_Collectable* collectable = giverInfo->node_->GetComponent<GOC_Collectable>();
    if (collectable)
    {
//        GameHelpers::DumpVariantMap(eventData);

        GOC_Collectable::TransferSlotTo(collectable->Get(), giverInfo->node_, getterInfo->node_, Variant(giverInfo->node_->GetWorldPosition()), qty);
        URHO3D_LOGINFOF("GameNetwork() - Client_TransferItem : collectable giver transferslot qty=%u ...", qty);
        bool ok = collectable->CheckEmpty();
    }
    else
    {
        URHO3D_LOGERRORF("GameNetwork() - Client_TransferItem : no collectable or inventory on the giver ...");
        GameHelpers::DumpNode(giverInfo->node_);
    }
}

void GameNetwork::Client_SetWorldMaps(VariantMap& eventData)
{
    URHO3D_LOGINFOF("GameNetwork() - Client_SetWorldMaps ...");

    // Load MapDatas
    {
        MemoryBuffer buffer(eventData[Net_ObjectCommand::P_DATAS].GetBuffer());
        buffer.Seek(0);

        while (buffer.Tell() < buffer.GetSize())
        {
            ShortIntVector2 mpoint;
            mpoint.x_ = buffer.ReadShort();
            mpoint.y_ = buffer.ReadShort();
            MapData* mapdata = MapStorage::GetMapDataAt(mpoint, true);
            mapdata->Load(buffer);
            mapdata->state_ = MAPASYNC_LOADSUCCESS;
            if (mapdata->map_ && mapdata->map_->GetStatus() == Loading_Map)
            {
                mapdata->map_->SetStatus(mapdata->savedmapstate_);
                URHO3D_LOGINFOF("GameNetwork() - Client_SetWorldMaps : mpoint=%s ... status=%d ... OK !", mpoint.ToString().CString(), mapdata->savedmapstate_);
            }
        }
    }
}

void GameNetwork::Client_SetWorldObjects(VariantMap& eventData)
{
    URHO3D_LOGINFOF("GameNetwork() - Client_SetWorldObjects ...");

    // Set World Objects
    //World2D::GetWorld()->SetNetWorldObjects(eventData[Net_ObjectCommand::P_DATAS].GetVariantVector());

    // Set NoNetSpawnable Server Objects
    {
        MemoryBuffer buffer(eventData[Net_ObjectCommand::P_SERVEROBJECTS].GetBuffer());
        buffer.Seek(0);
        ObjectControlInfo tinfo;
        ObjectControlInfo* oinfo;

        while (buffer.Tell() < buffer.GetSize())
        {
            unsigned char type = buffer.ReadUByte();

    #ifdef ACTIVE_SHORTHEADEROBJECTCONTROL
            const int nodeclientid = buffer.ReadUByte();
            unsigned servernodeid = buffer.ReadUShort();
            unsigned clientnodeid = buffer.ReadUShort();
            if (servernodeid) servernodeid += FIRST_LOCAL_ID;
            if (clientnodeid) clientnodeid += FIRST_LOCAL_ID;
    #else
            const int nodeclientid = buffer.ReadInt();
            const unsigned servernodeid = buffer.ReadUInt();
            const unsigned clientnodeid = buffer.ReadUInt();
    #endif
            // copy the data to temporary object control
            tinfo.Read(buffer);

            if (!tinfo.GetReceivedControl().HasNetSpawnMode())
            {
                const unsigned spawnid = tinfo.GetReceivedControl().states_.spawnid_;

                // get the current server object control for the server nodeid
                ObjectControlInfo& oinfo = GetOrCreateServerObjectControl(servernodeid, 0, nodeclientid);

                // create the node
                if (!oinfo.node_)
                {
                    oinfo.clientId_ = nodeclientid;
                    // copy the temporary object control in receivedcontrol before add object
                    tinfo.CopyReceivedControlTo(oinfo.GetReceivedControl());

                    bool ok = NetAddEntity(oinfo);
                }

                if (oinfo.node_)
                    oinfo.SetEnable(true);
            }
        }
    }

    // Reconciliate spawnstamps with the server
    localSpawnStamps_[0] = receivedSpawnStamps_[0];

    URHO3D_LOGINFOF("GameNetwork() - Client_SetWorldObjects : spawnstamp[SERVER]=%u !", localSpawnStamps_[0]);
}

void GameNetwork::Client_MountNode(VariantMap& eventData)
{
    Node* node = GameContext::Get().rootScene_->GetNode(eventData[Net_ObjectCommand::P_NODEID].GetUInt());
    if (!node)
        return;

    const unsigned targetid = eventData[Net_ObjectCommand::P_NODEPTRFROM].GetUInt();
    if (targetid == node->GetID())
        return;

    const bool isMounted = node->GetVar(GOA::ISMOUNTEDON).GetUInt();
    Node* target = targetid ? GameContext::Get().rootScene_->GetNode(targetid) : 0;

    // Unmount
    if (!target && isMounted)
    {
        URHO3D_LOGINFOF("GameNetwork() - Client_MountNode : node=%s(%u) target=%u ... unmount !", node->GetName().CString(), node->GetID(), targetid);
        GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
        if (controller)
            bool ok = controller->Unmount();
    }
    // Mount
    else if (!isMounted)
    {
        URHO3D_LOGINFOF("GameNetwork() - Client_MountNode : node=%s(%u) target=%u ... mount !", node->GetName().CString(), node->GetID(), targetid);
        GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
        if (controller)
            bool ok = controller->MountOn(target);
    }
}

void GameNetwork::ChangeEquipment(VariantMap& eventData)
{
    unsigned nodeid = eventData[Net_ObjectCommand::P_NODEID].GetUInt();
    Node* node = GameContext::Get().rootScene_->GetNode(nodeid);
    if (!node)
    {
        URHO3D_LOGERRORF("GameNetwork() - ChangeEquipment : no node %u", nodeid);
        return;
    }

    URHO3D_LOGINFOF("GameNetwork() - ChangeEquipment : %s(%u) ...", node->GetName().CString(), node->GetID());

    // Change Avatar before Equipment
    unsigned type = eventData[Net_ObjectCommand::P_CLIENTOBJECTTYPE].GetUInt();
    if (type)
    {
        unsigned char entityid = eventData[Net_ObjectCommand::P_CLIENTOBJECTENTITYID].GetUInt();
        GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
        if (controller && controller->ChangeAvatarOrEntity(type, entityid))
            if (clientMode_)
                GOC_Inventory::NetClientSetEquipment(node, node->GetID());
    }

    // Change a slot of Equipment
    if (serverMode_)
        GOC_Inventory::NetServerSetEquipmentSlot(node, eventData);
    else
        GOC_Inventory::NetClientSetEquipmentSlot(node, eventData);

    URHO3D_LOGINFOF("GameNetwork() - ChangeEquipment : %s(%u) ... OK !", node->GetName().CString(), node->GetID());
}

bool GameNetwork::ChangeTile(VariantMap& eventData)
{
    unsigned operation = eventData[Net_ObjectCommand::P_TILEOP].GetUInt();
    WorldMapPosition position;
    position.tileIndex_ = eventData[Net_ObjectCommand::P_TILEINDEX].GetUInt();
    position.mPoint_ = ShortIntVector2(eventData[Net_ObjectCommand::P_TILEMAP].GetUInt());
    position.viewZIndex_ = eventData[Net_ObjectCommand::P_TILEVIEW].GetInt();
    position.mPosition_ = IntVector2(position.tileIndex_%MapInfo::info.width_, position.tileIndex_/MapInfo::info.width_);

    bool ok = false;

    URHO3D_LOGINFOF("GameNetwork() - ChangeTile : operation=%s on mpoint=%s tileindex=%u viewZIndex=%u !", operation ? "ADD":"REMOVE", position.mPoint_.ToString().CString(), position.tileIndex_, position.viewZIndex_);

    if (operation == 0)
    {
        ok = (GameHelpers::RemoveTile(position, false, false, false) != -1);
    }
    else
    {
        ok = GameHelpers::AddTile(position);
    }

    URHO3D_LOGINFOF("GameNetwork() - ChangeTile : %s !", ok ? "OK":"NOK");

    return ok;
}

void GameNetwork::SendChangeEquipment(const StringHash& eventType, VariantMap& eventData)
{
    Node* node = GameContext::Get().rootScene_->GetNode(eventData[Net_ObjectCommand::P_NODEID].GetUInt());
    if (!node)
        return;

    // Send Avatar for Equipment changing
    GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
    if (controller)
    {
        eventData[Net_ObjectCommand::P_CLIENTOBJECTTYPE] = controller->control_.type_;
        eventData[Net_ObjectCommand::P_CLIENTOBJECTENTITYID] = controller->control_.entityid_;
    }

    PushObjectCommand(UPDATEEQUIPMENT, &eventData, true, clientID_);
}

void GameNetwork::Client_CommandAddObject(VariantMap& eventData)
{
    unsigned nodeid = eventData[Net_ObjectCommand::P_NODEID].GetUInt();

    if (nodeid < 2)
        return;

    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
        {
            // reactivating nodeid
            if (!it->active_)
            {
                it->SetActive(true, false);
                URHO3D_LOGINFOF("GameNetwork() - Client_CommandAddObject : clientObjectControls_ id=%u actived !", nodeid);
            }
            return;
        }
    }
}

void GameNetwork::Client_CommandEnableObject(VariantMap& eventData)
{
    if (eventData[Net_ObjectCommand::P_NODEISENABLED].GetBool())
    {
        SetEnableObject(true, eventData[Net_ObjectCommand::P_NODEID].GetUInt());
    }
    else
    {
        URHO3D_LOGINFOF("GameNetwork() - Client_CommandEnableObject : enable=false node=%u ...", eventData[Net_ObjectCommand::P_NODEID].GetUInt());
        Client_CommandRemoveObject(eventData, true);
    }
}

void GameNetwork::Client_CommandRemoveObject(VariantMap& eventData, bool skipIfNotDead)
{
    unsigned nodeid = eventData[Net_ObjectCommand::P_NODEID].GetUInt();
    if (nodeid < 2)
        return;

    URHO3D_LOGINFOF("GameNetwork() - Client_CommandRemoveObject : node=%u ...", nodeid);

    // Inactive server Object Control
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
        {
            if (it->active_)
            {
                it->SetActive(false, false);

                if (it->node_)
                {
                    if (skipIfNotDead && !it->node_->GetVar(GOA::ISDEAD).GetBool())
                        continue;

                    it->node_->SendEvent(WORLD_ENTITYDESTROY);
                    GOC_Destroyer* destroyer = it->node_->GetComponent<GOC_Destroyer>();
                    if (destroyer)
                        destroyer->Reset(false);
                }
            }

            bool oldenable = it->IsEnable();
            it->SetEnable(false);

            URHO3D_LOGINFOF("GameNetwork() - Client_CommandRemoveObject : serverObjectControl id=%u inactived oldenable=%u enable=%u !",
                            nodeid, oldenable, it->IsEnable());
            break;
        }
    }

    // Inactive client Object Control
//    if (clientNodeIDs_.Contains(nodeid))
//        clientNodeIDs_.Remove(nodeid);

    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
        {
            if (it->active_)
            {
                it->SetActive(false, false);

                if (it->node_)
                {
                    if (skipIfNotDead && !it->node_->GetVar(GOA::ISDEAD).GetBool())
                        continue;

                    it->node_->SendEvent(WORLD_ENTITYDESTROY);
                    GOC_Destroyer* destroyer = it->node_->GetComponent<GOC_Destroyer>();
                    if (destroyer)
                        destroyer->Reset(false);
                }
            }

            it->SetEnable(false);

            URHO3D_LOGINFOF("GameNetwork() - Client_CommandRemoveObject : clientObjectControl id=%u inactived !", nodeid);
            break;
        }
    }

//    URHO3D_LOGINFOF("GameNetwork() - Client_CommandRemoveObject : node=%u ... OK !", nodeid);
}

void GameNetwork::Client_DisableObjectControl(VariantMap& eventData)
{
    unsigned id = eventData[Net_ObjectCommand::P_NODEID].GetUInt();

    ObjectControlInfo* info = GetClientObjectControl(id);

    if (info && info->active_ && info->IsEnable())
    {
        URHO3D_LOGINFOF("GameNetwork() - Client_DisableObjectControl : id=%u !", id);
        info->SetActive(false, false);
        info->SetEnable(false);
    }
    else
        URHO3D_LOGERRORF("GameNetwork() - Client_DisableObjectControl : unsatisfied requirements for clientObjectControl nodeid=%u !", id);
}

void GameNetwork::Client_LinkNodeId(VariantMap& eventData)
{
    unsigned servernodeid = eventData[Net_ObjectCommand::P_NODEID].GetUInt();
    unsigned clientnodeid = eventData[Net_ObjectCommand::P_NODEIDFROM].GetUInt();

    Node* node = GameContext::Get().rootScene_->GetNode(clientnodeid);
    ObjectControlInfo& oinfo = GetOrCreateServerObjectControl(servernodeid, clientnodeid, 0, node);

    oinfo.SetEnable(oinfo.node_ ? oinfo.node_->IsEnabled() : false);

    URHO3D_LOGINFOF("GameNetwork() - Client_LinkNodeId : servernodeid=%u clientnodeid=%u node=%s(%u) enable=%s", servernodeid, clientnodeid,
                    oinfo.node_ ? oinfo.node_->GetName().CString() : "none", oinfo.node_ ? oinfo.node_->GetID() : 0, oinfo.node_ && oinfo.node_->IsEnabled() ? "true":"false");
}

void GameNetwork::Server_ApplyObjectCommand(int fromclientid, VariantMap& eventData)
{
    int cmd = eventData[Net_ObjectCommand::P_COMMAND].GetInt();

    if (gameStatus_ == MENUSTATE && cmd > REQUESTMAP)
    {
        URHO3D_LOGERRORF("GameNetwork() - Server_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) not apply in MENUSTATE !", cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown");
        return;
    }

    URHO3D_LOGINFOF("GameNetwork() - Server_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) !", cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown");

    switch (cmd)
    {
    case GAMESTATUS:
        ApplyReceivedGameStatus(eventData, fromclientid);
        break;
    case ENABLENODE:
        SetEnableObject(eventData[Net_ObjectCommand::P_NODEISENABLED].GetBool(), eventData[Net_ObjectCommand::P_NODEID].GetUInt());
        break;
    case REQUESTMAP:
    {
        ClientInfo* clientinfo = serverClientID2Infos_[fromclientid];
        if (clientinfo)
        {
            ShortIntVector2 mpoint(eventData[Net_ObjectCommand::P_TILEMAP].GetUInt());
            if (!clientinfo->mapRequests_.Contains(mpoint))
            {
                clientinfo->mapRequests_.Push(mpoint);
                URHO3D_LOGERRORF("GameNetwork() - Server_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) mpoint=%s !",
                              cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", mpoint.ToString().CString());
            }
        }
    }

    case EXPLODENODE:
        ExplodeNode(eventData);
        break;
    case ADDFURNITURE:
        World2D::SpawnFurniture(eventData);
        break;
    case DROPITEM:
    case SETITEM:
        Server_UpdateInventory(cmd, eventData);
        break;
    case UPDATEEQUIPMENT:
        ChangeEquipment(eventData);
        break;
    case CHANGETILE:
    {
        bool ok = ChangeTile(eventData);
    }
        break;
    case SETPLAYERPOSITION:
    {
        ClientInfo* clientinfo = serverClientID2Infos_[fromclientid];
        if (clientinfo)
        {
            Player* player = clientinfo->GetPlayerFor(eventData[Net_ObjectCommand::P_NODEID].GetUInt());
            if (player)
            {
                player->SetWorldMapPosition(eventData);
                URHO3D_LOGERRORF("GameNetwork() - Server_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) nodeid=%u set worldposition=%s !",
                              cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", eventData[Net_ObjectCommand::P_NODEID].GetUInt(), player->GetWorldMapPosition().ToString().CString());
            }
        }
    }
        break;
    case TRIGCLICKED:
    {
        Node* node = GameContext::Get().rootScene_->GetNode(eventData[Net_ObjectCommand::P_NODEID].GetUInt());
        if (node)
            node->SendEvent(NET_TRIGCLICKED);
        else
            URHO3D_LOGERRORF("GameNetwork() - Server_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) nodeid=%u no node !",
                              cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", eventData[Net_ObjectCommand::P_NODEID].GetUInt());
    }
        break;
    case ENTITYSELECTED:
    {
        Node* node = GameContext::Get().rootScene_->GetNode(eventData[Net_ObjectCommand::P_NODEID].GetUInt());
        if (node)
            node->SendEvent(GO_SELECTED, eventData);
    }
        break;
    }
}

void GameNetwork::Client_ApplyObjectCommand(ObjectCommand& objCmd)
{
    VariantMap& eventData = objCmd.cmd_;

    int cmd = eventData[Net_ObjectCommand::P_COMMAND].GetInt();

    if (gameStatus_ == MENUSTATE && cmd > REQUESTMAP)
    {
        URHO3D_LOGERRORF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) not apply in MENUSTATE !", cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown");
        return;
    }

    bool apply = (!clientID_ && cmd == GAMESTATUS) || (!objCmd.broadCast_ && clientID_ == objCmd.clientId_) || (objCmd.broadCast_ && clientID_ != objCmd.clientId_);
    if (!apply)
    {
        URHO3D_LOGERRORF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) not apply !", cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown");
        return;
    }

    URHO3D_LOGERRORF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=%d(%s) nodeid=%u ...",
                    cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", eventData[Net_ObjectCommand::P_NODEID].GetUInt());

    switch (cmd)
    {
    case GAMESTATUS:
        ApplyReceivedGameStatus(eventData);
        break;
    case ERASENODE:
        Client_CommandRemoveObject(eventData);
        break;
    case ENABLENODE:
        Client_CommandEnableObject(eventData);
        break;
    case DISABLECLIENTOBJECTCONTROL:
        Client_DisableObjectControl(eventData);
        break;
    case SETSEEDTIME:
        if (!seedTime_)
        {
            seedTime_ = eventData[Net_ServerSeedTime::P_SEEDTIME].GetUInt();
            URHO3D_LOGINFOF("GameNetwork() - Client_ApplyObjectCommand : NET_SERVERSEEDTIME : seedtime=%u", seedTime_);
        }
        break;

    case ADDNODE:
        Client_CommandAddObject(eventData);
        break;
    case EXPLODENODE:
        ExplodeNode(eventData);
        break;
    case LINKNODEID:
        Client_LinkNodeId(eventData);
        break;

    case ADDFURNITURE:
        World2D::SpawnFurniture(eventData);
        break;
    case ADDCOLLECTABLE:
        World2D::SpawnCollectable(eventData);
        break;

    case TRANSFERITEM:
        Client_TransferItem(eventData);
        break;
    case UPDATEEQUIPMENT:
        ChangeEquipment(eventData);
        break;
    case SETFULLEQUIPMENT:
        GOC_Inventory::NetClientSetEquipment(eventData[Net_ObjectCommand::P_NODEID].GetUInt(), eventData);
        break;
    case SETFULLINVENTORY:
        GOC_Inventory::NetClientSetInventory(eventData[Net_ObjectCommand::P_NODEID].GetUInt(), eventData);
        break;

    case CHANGETILE:
    {
        bool ok = ChangeTile(eventData);
    }
        break;

    case SETWEATHER:
        WeatherManager::Get()->SetNetWorldInfos(eventData[Net_ObjectCommand::P_DATAS].GetVariantVector());
        break;
    case SETMAPDATAS:
        Client_SetWorldMaps(eventData);
        break;
    case SETWORLD:
        Client_SetWorldObjects(eventData);
        break;

    case TRIGCLICKED:
    {
        Node* node = GameContext::Get().rootScene_->GetNode(eventData[Net_ObjectCommand::P_NODEID].GetUInt());
        if (node)
            node->SendEvent(NET_TRIGCLICKED);
        else
            URHO3D_LOGERRORF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=(%s) nodeid=%u no node !", netCommandNames[cmd], eventData[Net_ObjectCommand::P_NODEID].GetUInt());
    }
        break;
    case MOUNTENTITYON:
        Client_MountNode(eventData);
        break;

    case UPDATEITEMSSTORAGE:
        GOC_DropZone::NetClientUpdateStorage(eventData[Net_ObjectCommand::P_NODEID].GetUInt(), eventData);
        break;
    case UPDATEZONEDATA:
    {
        ShortIntVector2 mpoint(eventData[Net_ObjectCommand::P_TILEMAP].GetUInt());
        const int zoneid = eventData[Net_ObjectCommand::P_TILEINDEX].GetInt();
        const PODVector<unsigned char>& buffer = eventData[Net_ObjectCommand::P_DATAS].GetBuffer();
        MapData* mapdata = MapStorage::Get()->GetMapDataAt(mpoint, true);
        URHO3D_LOGINFOF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=UPDATEZONEDATA receive mapdata mpoint=%s zoneid=%d ... ", mpoint.ToString().CString(), zoneid);
        if (mapdata)
        {
            if (zoneid >= mapdata->zones_.Size())
                mapdata->zones_.Resize(zoneid+1);

            int zonestate = mapdata->zones_[zoneid].state_;
            memcpy(&mapdata->zones_[zoneid], buffer.Buffer(), sizeof(ZoneData));
            mapdata->zones_[zoneid].state_ = zonestate;

            URHO3D_LOGINFOF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=UPDATEZONEDATA receive mapdata mpoint=%s zoneid=%d ... OK !", mpoint.ToString().CString(), zoneid);
        }
    }
        break;
    }

//    URHO3D_LOGINFOF("GameNetwork() - Client_ApplyObjectCommand : NET_OBJECTCOMMAND : cmd=(%s) for nodeid=%u ... OK !",
//                      cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", eventData[Net_ObjectCommand::P_NODEID].GetUInt());
}


/// Spawn Controls

void GameNetwork::ResyncSpawnControlStamps()
{
    for (unsigned i=0; i < GameContext::Get().MAX_NUMNETPLAYERS; i++)
        localSpawnStamps_[i] = receivedSpawnStamps_[i];
}

void GameNetwork::ClearSpawnControls()
{
    for (unsigned i=0; i < GameContext::Get().MAX_NUMNETPLAYERS; i++)
        spawnControls_[i].Clear();
}

unsigned GameNetwork::GetSpawnID(unsigned holderid, unsigned char stamp) const
{
    return ((((unsigned)stamp) << 24) | (LAST_REPLICATED_ID & (holderid - FIRST_LOCAL_ID)));
}

unsigned GameNetwork::GetSpawnHolder(unsigned spawnID) const
{
    return spawnID ? FIRST_LOCAL_ID + (spawnID & LAST_REPLICATED_ID) : 0;
}

unsigned char GameNetwork::GetSpawnStamp(unsigned spawnID) const
{
    return spawnID ? (unsigned char)((spawnID >> 24) & 0xFF) : 0;
}

ObjectControlInfo* GameNetwork::GetSpawnControl(int clientid, unsigned spawnid) const
{
    const HashMap<unsigned, ObjectControlInfo* >& spawncontrols = spawnControls_[clientid];
    if (!spawncontrols.Size())
        return 0;

    HashMap<unsigned, ObjectControlInfo* >::ConstIterator it = spawncontrols.Find(spawnid);
    return it != spawncontrols.End() ? it->second_ : 0;
}

bool GameNetwork::NetSpawnControlAlreadyUsed(int clientid, unsigned char localstamp) const
{
    // for the foreign clientid, skip local clientid (net usage only)
    if (clientid == clientID_)
        return false;

    if (!localstamp)
        localstamp = localSpawnStamps_[clientid];

    unsigned char receivedstamp = receivedSpawnStamps_[clientid];

    /// TODO : overflow case

    if (localstamp > receivedstamp )
        URHO3D_LOGINFOF("GameNetwork() - NetSpawnControlAlreadyUsed : clientid=%d spawnstamps local=%u > received=%u !", clientid, localstamp, receivedstamp);

    return localstamp > receivedstamp;
}

ObjectControlInfo* GameNetwork::AddSpawnControl(Node* node, Node* holder, bool server, bool enable, bool allowNetSpawn)
{
    if (GameContext::Get().LocalMode_)
        return 0;

    if (!node && !holder)
    {
        URHO3D_LOGERRORF("GameNetwork() - AddSpawnControl : node=%s(%u) holder=%s(%u) error !",
                        node ? node->GetName().CString() : "null", node ? node->GetID() : 0,
                        holder ? holder->GetName().CString() : "null", holder ? holder->GetID() : 0);
        return 0;
    }

    unsigned holderid = 0;
    int holderclientid = 0;
    if (holder)
    {
        holderid = holder->GetID();
        holderclientid = server ? 0 : holder->GetVar(GOA::CLIENTID).GetInt();
    }

    unsigned nodeid = node ? node->GetID() : holderid;

    // if the node is not the holder => it's an avatar, so the spawnstamp is always zero
    unsigned char spawnstamp = 0;
    // else get the updated spawnstamp
    if (node && holder != node)
    {
        unsigned char& localstamp = localSpawnStamps_[holderclientid];
        localstamp++;

        // skip the stamp=0 reserve for the player
        if (!localstamp)
            localstamp++;

        spawnstamp = localstamp;
    }

    const unsigned spawnid = GetSpawnID(holderid, spawnstamp);
    const bool isClientControl = clientID_ > 0 && holderclientid == clientID_;
    bool newinfo = false;

//    URHO3D_LOGINFOF("GameNetwork() - AddSpawnControl : PASS1 ... holderid=%u holderclientid=%d spawnstamp=%u spawnid=%u", holderid, holderclientid, spawnstamp, spawnid);

    // check if an objectControl is already available in spawnControls_
    ObjectControlInfo* oinfo = GetSpawnControl(holderclientid, spawnid);
    if (!oinfo)
    {
        // check if an objectControl is already available in ObjectControls Storage
        if (isClientControl)
        {
            for (unsigned i=0; i < clientObjectControls_.Size(); i++)
            {
                ObjectControlInfo& info = clientObjectControls_[i];
                if (!info.active_ && info.clientNodeID_ == nodeid)
                {
                    oinfo = &info;
                    break;
                }
            }
            newinfo = !oinfo;
            // create a new ObjectControlInfo
            if (newinfo)
            {
                clientObjectControls_.Resize(clientObjectControls_.Size()+1);
                oinfo = &clientObjectControls_.Back();
            }
        }
        else
        {
            for (unsigned i=0; i < serverObjectControls_.Size(); i++)
            {
                ObjectControlInfo& info = serverObjectControls_[i];
                if (!info.active_ && (info.serverNodeID_ == nodeid || info.clientNodeID_ == nodeid))
                {
                    oinfo = &info;
                    break;
                }
            }
            newinfo = !oinfo;
            // create a new ObjectControlInfo
            if (newinfo)
            {
                serverObjectControls_.Resize(serverObjectControls_.Size()+1);
                oinfo = &serverObjectControls_.Back();
            }
        }

        // Register in SpawnControls
        spawnControls_[holderclientid][spawnid] = oinfo;

        // Set the Objectinfo
        oinfo->clientId_ = holderclientid;
        oinfo->node_ = node;

        if (clientMode_)
        {
            oinfo->clientNodeID_ = nodeid;
            oinfo->serverNodeID_ = 0;
        }
        else
        {
            oinfo->serverNodeID_ = nodeid;
            oinfo->clientNodeID_ = 0;
        }
    }

    oinfo->prepared_ = false;
    PrepareControl(*oinfo);

    oinfo->GetPreparedControl().states_.spawnid_ = spawnid;
    oinfo->GetPreparedControl().states_.stamp_ = 0;
    oinfo->GetPreparedControl().SetFlagBit(OFB_NETSPAWNMODE, allowNetSpawn);
    oinfo->active_ = true;
    if (enable)
        oinfo->SetEnable(enable);

    // set the initial position that will be used by GameNetwork::NetSpawnEntity
    oinfo->CopyPreparedPositionToInitial();

    URHO3D_LOGINFOF("GameNetwork() - AddSpawnControl : %s node=%s(%u) ... spawnid=%u(holderid=%u,spawnstamp=%u) ... ",
                    isClientControl ? "ClientControl" : "ServerControl", node ? node->GetName().CString() : "null", nodeid, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
    URHO3D_LOGINFOF("GameNetwork() - AddSpawnControl : ... %s oinfo=%u srvnodeid=%u cltnodeid=%u holderptr=%u(nodeid=%u,clientid=%d) spawnstamp=%u !",
                    newinfo ? "new":"", oinfo, oinfo->serverNodeID_, oinfo->clientNodeID_, holder, holderid, holderclientid, spawnstamp);

    return oinfo;
}

void GameNetwork::RemoveSpawnControl(int clientid, unsigned spawnid)
{
    if (clientid >= spawnControls_.Size())
        return;

    HashMap<unsigned, ObjectControlInfo* >& spawncontrols = spawnControls_[clientid];
    if (!spawncontrols.Size())
        return;

    HashMap<unsigned, ObjectControlInfo* >::Iterator it = spawncontrols.Find(spawnid);
    if (it != spawncontrols.End())
    {
        spawncontrols.Erase(it);

//        URHO3D_LOGERRORF("GameNetwork() - RemoveSpawnControl : clientid=%d ... spawnid=%u(holderid=%u,spawnstamp=%u) ... remove spawncontrol !",
//                        clientID_, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
    }
}

void GameNetwork::LinkSpawnControl(int clientid, unsigned spawnid, unsigned servernodeid, unsigned clientnodeid, ObjectControlInfo* oinfo)
{
    bool change = false;
    if (clientMode_ && oinfo->clientId_ != clientid)
    {
        oinfo->clientId_ = clientid;
        change = true;
    }

    if (oinfo->serverNodeID_ != servernodeid)
    {
        oinfo->serverNodeID_ = servernodeid;
        change = true;
    }

    if (oinfo->clientNodeID_ != clientnodeid)
    {
        oinfo->clientNodeID_ = clientnodeid;
        change = true;
    }

    if (change)
    {
        URHO3D_LOGINFOF("GameNetwork() - LinkSpawnControl : oinfo=%u from clientid=%d spawnid=%u links serverNodeID_=%u clientNodeID_=%u !",
                        oinfo, oinfo->clientId_, spawnid, oinfo->serverNodeID_, oinfo->clientNodeID_);
    }
}

void GameNetwork::NetSpawnEntity(unsigned netnodeid, const ObjectControlInfo& refinfo, ObjectControlInfo*& oinfo)
{
    const unsigned spawnid = refinfo.GetReceivedControl().states_.spawnid_;
    const unsigned holderid = GetSpawnHolder(spawnid);

    URHO3D_LOGERRORF("GameNetwork() - NetSpawnEntity : clientid=%d ... spawnid=%u(holderid=%u,spawnstamp=%u) posreceived=%F,%F oinfo=%u ...",
                     clientID_, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid), refinfo.GetReceivedControl().holderinfo_.point1x_, refinfo.GetReceivedControl().holderinfo_.point1y_, oinfo);

    int holderclientid = 0;
    Node* holder = 0;
    if (holderid)
    {
        holder = GameContext::Get().rootScene_->GetNode(holderid);
        if (holder)
            holderclientid = holder->GetVar(GOA::CLIENTID).GetInt();
    }

    // Get or Create the ObjectControlInfo
    bool newinfo = false;
    if (!oinfo)
    {
        // check if an objectControl is already available in ObjectControls Storage
        Vector<ObjectControlInfo>& objectControls = clientMode_ && holderclientid == clientID_ ? clientObjectControls_ : serverObjectControls_;
        if (clientMode_)
        {
            for (unsigned i=0; i < objectControls.Size(); i++)
            {
                ObjectControlInfo& info = objectControls[i];
                if (!info.active_ && netnodeid == info.clientNodeID_)
                {
                    oinfo = &info;
                    break;
                }
            }
        }
        else
        {
            for (unsigned i=0; i < objectControls.Size(); i++)
            {
                ObjectControlInfo& info = objectControls[i];
                if (!info.active_ && netnodeid == info.serverNodeID_)
                {
                    oinfo = &info;
                    break;
                }
            }
        }
        // create a new ObjectControlInfo
        if (!oinfo)
        {
            objectControls.Resize(objectControls.Size()+1);
            oinfo = &objectControls.Back();
            newinfo = true;
        }

        refinfo.CopyReceivedControlTo(oinfo->GetReceivedControl());
    }

    oinfo->clientId_ = holderclientid;
    if (serverMode_)
        oinfo->clientNodeID_ = netnodeid;
    else
        oinfo->serverNodeID_ = netnodeid;
    oinfo->GetPreparedControl().states_.spawnid_ = spawnid;
    oinfo->GetPreparedControl().states_.stamp_ = 0;

    // Spawn the entity at the initial position
    oinfo->UseInitialPosition(refinfo.GetReceivedControl());

    // Register in the SpawnControl
    spawnControls_[holderclientid][spawnid] = oinfo;

    // Update the localstamp
    localSpawnStamps_[holderclientid]++;

    // Instantiate the entity
    bool ok = NetAddEntity(*oinfo, holder);

    const Vector2& pos = oinfo->node_ ? oinfo->node_->GetWorldPosition2D() : Vector2::ZERO;

    URHO3D_LOGERRORF("GameNetwork() - NetSpawnEntity : clientid=%d node=%s(%u) ... pos=%F,%F spawnid=%u(holderid=%u,spawnstamp=%u) ... %s oinfo=%u holderptr=%u(nodeid=%u,clientid=%d) !",
                     clientID_, oinfo->node_ ? oinfo->node_->GetName().CString() : "null", oinfo->node_ ? oinfo->node_->GetID() : 0, pos.x_, pos.y_,
                     spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid), newinfo ? "new" :"", oinfo, holder, holderid, holderclientid);
}

bool GameNetwork::NetAddEntity(ObjectControlInfo& info, Node* holder, VariantMap* eventData, bool enable)
{
    if (gameStatus_ < PLAYSTATE_SYNCHRONIZING)
        return false;

    if (!info.GetReceivedControl().states_.type_ && !enable)
        return false;

    StringHash got(info.GetReceivedControl().states_.type_);
    if (!GOT::IsRegistered(got))
    {
        URHO3D_LOGERRORF("GameNetwork() - NetAddEntity : serverNodeID=%u got=%u not registered !", info.serverNodeID_, got.Value());
        return false;
    }

    if (enable)
    {
        info.SetActive(true, serverMode_);
        info.SetEnable(true);
    }

    if (!info.IsEnable())
        return false;

    URHO3D_LOGINFOF("GameNetwork() - NetAddEntity : got=%s(%u) ... serverNodeID=%u clientNodeID=%u ...", GOT::GetType(got).CString(), got.Value(), info.serverNodeID_, info.clientNodeID_);

    // get the node in reference
    Node* node = GameContext::Get().rootScene_->GetNode(serverMode_ ? info.clientNodeID_ : info.serverNodeID_);

    if (clientMode_ && (GOT::GetTypeProperties(got) & GOT_Controllable) && (!node || !node->isPoolNode_))
        Client_AddServerControllable(node, info);
    else if (GOT::GetTypeProperties(got) & GOT_Furniture)
        node = World2D::NetSpawnFurniture(info);
    else
        node = World2D::NetSpawnEntity(info, holder, eventData);

    if (node && enable)
    {
//        bool nodeEnabled = info.GetReceivedControl().IsEnabled();
        SetEnableObject(true, info, node);
//        URHO3D_LOGINFOF("GameNetwork() - NetAddEntity : serverNodeID=%u clientNodeID=%u ... nodeclientid=%d ... OK !", info.serverNodeID_, info.clientNodeID_, info.clientId_);
    }
    else
    {
        URHO3D_LOGWARNINGF("GameNetwork() - NetAddEntity : serverNodeID=%u clientNodeID=%u ... NOK !", info.serverNodeID_, info.clientNodeID_);
    }

    info.node_ = node;

    URHO3D_LOGINFOF("GameNetwork() - NetAddEntity : got=%s(%u) ... node=%s(%u) enable=%s !", GOT::GetType(got).CString(), got.Value(), node ? node->GetName().CString() : "null", node ? node->GetID() : 0, node && node->IsEnabled() ? "true":"false");

    // always reset totalDpsReceived_ after spawning otherwise it can be setted to -1.f that indicates the node is dead.
    info.GetReceivedControl().states_.totalDpsReceived_ = 0.f;

    return info.node_.Get() != 0;
}


/// GameNetwork Server Setters

void GameNetwork::Server_SendGameStatus(int status, Connection* specificConnection)
{
    if (specificConnection)
    {
        ClientInfo& clientinfo = serverClientInfos_.Find(specificConnection)->second_;
        sServerEventData_[Net_ObjectCommand::P_COMMAND] = (int)GAMESTATUS;
        sServerEventData_[Net_GameStatusChanged::P_STATUS] = status;
        sServerEventData_[Net_GameStatusChanged::P_GAMEMODE] = (int)(GameContext::Get().arenaZoneOn_ ? 1 : 2);
        sServerEventData_[Net_GameStatusChanged::P_CLIENTID] = clientinfo.clientID_;
        PushObjectCommand(sServerEventData_, false, clientinfo.clientID_);

        URHO3D_LOGINFOF("GameNetwork() - Server_SendGameStatus : Send Server NET_GAMESTATUSCHANGED Status=%s(%d) to ClientID=%d !", gameStatusNames[status], status, clientinfo.clientID_);
    }
    else
    {
        sServerEventData_[Net_ObjectCommand::P_COMMAND] = (int)GAMESTATUS;
        sServerEventData_[Net_GameStatusChanged::P_STATUS] = status;
        sServerEventData_[Net_GameStatusChanged::P_GAMEMODE] = (int)(GameContext::Get().arenaZoneOn_ ? 1 : 2);
        sServerEventData_[Net_GameStatusChanged::P_CLIENTID] = 0;
        PushObjectCommand(sServerEventData_);

        URHO3D_LOGINFOF("GameNetwork() - Server_SendGameStatus : Send Server NET_GAMESTATUSCHANGED Status=%s(%d) to AllClients !", gameStatusNames[status], status);
    }
}

void GameNetwork::Server_SendSeedTime(unsigned time)
{
    if (!serverMode_)
        return;

    VariantMap& remoteEventData = context_->GetEventDataMap();
    remoteEventData[Net_ServerSeedTime::P_SEEDTIME] = seedTime_;
    PushObjectCommand(SETSEEDTIME, &remoteEventData, true);
}

void GameNetwork::Server_SendSnap(ClientInfo& clientInfo)
{
    // Send Weather Infomations
    Server_SendWeatherInfos(clientInfo);

    // Prepare Added/Removed Tiles/Nodes List
    Server_SendWorldObjects(clientInfo);

    // Prepare ObjectCommands for the Scene Modifiers
    // Lights / Player Inventory / Entity Equipments
    Server_SendInventories(clientInfo);
}

// Used by GOC_Destroyer::Destroy, GameNetwork::Server_RemovePlayers
void GameNetwork::Server_RemoveObject(unsigned nodeid, bool sendnetevent, bool allconnections)
{
//    URHO3D_LOGINFOF("GameNetwork() - Server_RemoveObject : nodeid=%u ... ", nodeid);

    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ != nodeid)
            continue;

        ObjectControlInfo& cinfo = *it;

//        URHO3D_LOGINFOF("GameNetwork() - Server_RemoveObject : nodeid=%u ... clientid=%u ...", nodeid, cinfo.clientId_);

        if (sendnetevent)
        {
            sServerEventData_.Clear();
            sServerEventData_[Net_ObjectCommand::P_NODEID] = nodeid;
            PushObjectCommand(ERASENODE, 0, true, 0);
        }

        cinfo.SetActive(false, true);
        cinfo.SetEnable(false);
        cinfo.clientId_ = 0;

        URHO3D_LOGINFOF("GameNetwork() - Server_RemoveObject : servernodeid=%u oinfo=%u active=%s clientnodeid=%u ... OK !", nodeid, &cinfo, cinfo.active_ ?"true":"false", cinfo.clientNodeID_);
    }
}

void GameNetwork::RemoveServerObject(ObjectControlInfo& cinfo)
{
    cinfo.SetActive(false, true);
    cinfo.SetEnable(false);
    cinfo.clientId_ = 0;

    URHO3D_LOGINFOF("GameNetwork() - RemoveServerObject : servernodeid=%u oinfo=%u active=%s clientnodeid=%u ... OK !", cinfo.serverNodeID_, &cinfo, cinfo.active_ ?"true":"false", cinfo.clientNodeID_);
}

void GameNetwork::Server_PurgeInactiveObjects()
{
    if (!serverObjectControls_.Size())
        return;

    unsigned initialSize = serverObjectControls_.Size();
    Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin();
    while (it != serverObjectControls_.End())
    {
        if (it->active_)
        {
            it++;
            continue;
        }

        URHO3D_LOGINFOF("GameNetwork() - Server_PurgeInactiveObjects : nodeid=%u control purged ! ", it->serverNodeID_);
        it = serverObjectControls_.Erase(it);
    }

    URHO3D_LOGINFOF("GameNetwork() - Server_PurgeInactiveObjects : %u/%u purged ! ", initialSize-serverObjectControls_.Size(), initialSize);
}

void GameNetwork::Server_SetEnableAllObjects(bool enable)
{
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->active_ != enable)
        {
            it->SetActive(enable, true);
            Node* node = GameContext::Get().rootScene_->GetNode(it->serverNodeID_);
            if (node) node->SetEnabled(enable);
        }
    }
}

void GameNetwork::Server_AllocatePlayers(ClientInfo& clientInfo)
{
    if (!serverMode_)
        return;

    if (!clientInfo.connection_)
    {
        URHO3D_LOGWARNINGF("GameNetwork() - Server_AllocatePlayers : ClientID=%d Connection=0 !", clientInfo.clientID_);
        return;
    }

    URHO3D_LOGINFOF("GameNetwork() - Server_AllocatePlayers : ClientID=%d ...", clientInfo.clientID_);

    // Check for new players to allocate
    if (clientInfo.players_.Size() != clientInfo.requestPlayers_)
    {
        int deltaPlayers = abs(int(clientInfo.players_.Size()) - int(clientInfo.requestPlayers_));
        if (deltaPlayers)
        {
            if (clientInfo.requestPlayers_ > (int)clientInfo.players_.Size())
            {
                URHO3D_LOGINFOF("GameNetwork() - Server_AllocatePlayers : ClientID=%d ... Add Players ...", clientInfo.clientID_);
                for (int j=0; j < deltaPlayers; j++)
                {
                    clientInfo.AddPlayer();
                    clientInfo.playersStarted_ = false;
                }
            }
            else
            {
//                URHO3D_LOGINFOF("GameNetwork() - Server_AllocatePlayers : ClientID=%d ... Remove Players ...", clientInfo.clientID_);
                clientInfo.players_.Resize(clientInfo.requestPlayers_);
            }
        }
    }

    // Check for new avatars to create
    sServerEventData_.Clear();
    for (Vector<SharedPtr<Player > >::Iterator it = clientInfo.players_.Begin(); it != clientInfo.players_.End(); ++it)
    {
        Player* player = *it;
        if (!player)
            continue;

        Node* avatar = player->GetAvatar();
        if (!avatar)
        {
            avatar = clientInfo.CreateAvatarFor(it-clientInfo.players_.Begin());
            clientInfo.playersStarted_ = false;
        }
    }

//    URHO3D_LOGINFOF("GameNetwork() - Server_AllocatePlayers : ClientID=%d ... OK !", clientInfo.clientID_);
}

void GameNetwork::Server_SendWeatherInfos(ClientInfo& clientInfo)
{
    VariantVector weatherInfos;
    WeatherManager::Get()->GetNetWorldInfos(weatherInfos);
    sServerEventData_[Net_ObjectCommand::P_DATAS] = weatherInfos;
    PushObjectCommand(SETWEATHER, 0, false, clientInfo.clientID_);
    sServerEventData_.Clear();
}

void GameNetwork::Server_SendWorldMaps(ClientInfo& clientInfo)
{
    if (!MapStorage::Get())
        return;

    VectorBuffer buffer;

    if (clientInfo.mapsDirty_)
    {
        const HashMap<ShortIntVector2, Map*>& mapStorage = MapStorage::Get()->GetMapsInMemory();
        for (HashMap<ShortIntVector2, Map*>::ConstIterator it = mapStorage.Begin(); it != mapStorage.End(); ++it)
        {
            Map* map = it->second_;
            if (map && map->IsSerializable())
            {
                // update MapData
                map->UpdateMapData(0);

                // save MapData in memory buffer
                buffer.WriteShort(it->first_.x_);
                buffer.WriteShort(it->first_.y_);
                map->GetMapData()->Save(buffer);
            }
            else if (!clientInfo.mapRequests_.Contains(it->first_))
                clientInfo.mapRequests_.Push(it->first_);
        }

        clientInfo.mapsDirty_ = false;
    }
    else if (clientInfo.mapRequests_.Size())
    {
        Vector<ShortIntVector2>::Iterator it = clientInfo.mapRequests_.Begin();
        while (it != clientInfo.mapRequests_.End())
        {
            Map* map = World2D::GetWorld()->GetMapAt(*it);
            if (map && map->IsSerializable())
            {
                // update MapData
                map->UpdateMapData(0);

                // save MapData in memory buffer
                buffer.WriteShort(it->x_);
                buffer.WriteShort(it->y_);
                map->GetMapData()->Save(buffer);

                it = clientInfo.mapRequests_.Erase(it);
            }
            else
                it++;
        }
    }

    if (buffer.GetSize())
    {
        URHO3D_LOGINFOF("GameNetwork() - Server_SendWorldMaps to clientid=%d", clientInfo.clientID_);

        ObjectCommand& cmd = *objCmdPool_.Get();
        cmd.cmd_[Net_ObjectCommand::P_DATAS] = buffer;
        PushObjectCommand(SETMAPDATAS, cmd, false, clientInfo.clientID_);
    }
}

void GameNetwork::Server_SendWorldObjects(ClientInfo& clientInfo)
{
    URHO3D_LOGINFOF("GameNetwork() - Server_SendWorldObjects ...");

    ObjectCommand& cmd = *objCmdPool_.Get();
    //cmd.cmd_[Net_ObjectCommand::P_DATAS] = World2D::GetWorld()->GetNetWorldObjects(clientInfo);

    // Write the Server ObjectControls
    {
        VectorBuffer buffer;
        for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
        {
            ObjectControlInfo& oinfo = *it;
            if (oinfo.clientId_ != clientInfo.clientID_ && oinfo.active_)
                oinfo.Write(buffer);
        }

        cmd.cmd_[Net_ObjectCommand::P_SERVEROBJECTS] = buffer;
    }

    PushObjectCommand(SETWORLD, cmd, false, clientInfo.clientID_);
}

void GameNetwork::Server_SendInventories(ClientInfo& clientInfo)
{
    // local players on server : send equipment to the client
    for (unsigned i = 0; i < GameContext::MAX_NUMPLAYERS; i++)
    {
        Node* avatar = GameContext::Get().playerAvatars_[i];
        if (!avatar)
            continue;

        GOC_Inventory* inventory = avatar->GetComponent<GOC_Inventory>();
        VariantVector equipmentSet;
        if (inventory->GetSectionSlots("Equipment", equipmentSet))
        {
            sServerEventData_[Net_ObjectCommand::P_NODEID] = avatar->GetID();
            sServerEventData_[Net_ObjectCommand::P_INVENTORYTEMPLATE] = inventory->GetTemplateHashName().Value();
            sServerEventData_[Net_ObjectCommand::P_INVENTORYSLOTS] = equipmentSet;
            PushObjectCommand(SETFULLEQUIPMENT, 0, false, clientInfo.clientID_);
            sServerEventData_.Clear();
        }
    }
    // send objects storages (inventory, dropzone) to the client
    {
        // TODO : which mappoint ?
        ShortIntVector2 mappoint;
        PODVector<Node*> objects;
        World2D::GetFilteredEntities(mappoint, objects, GO_Entity); // don't get Ais and players
        for (unsigned i = 0; i < objects.Size(); i++)
        {
            GOC_Inventory* inventory = objects[i]->GetComponent<GOC_Inventory>();
            if (inventory)
            {
                sServerEventData_[Net_ObjectCommand::P_NODEID] = inventory->GetNode()->GetID();
                sServerEventData_[Net_ObjectCommand::P_INVENTORYTEMPLATE] = inventory->GetTemplateHashName().Value();
                sServerEventData_[Net_ObjectCommand::P_INVENTORYSLOTS] = inventory->GetInventoryAttr();
                PushObjectCommand(SETFULLINVENTORY, 0, true, 0);
                sServerEventData_.Clear();
            }
            GOC_DropZone* dropzone   = objects[i]->GetComponent<GOC_DropZone>();
            if (dropzone)
            {
                sServerEventData_[Go_StorageChanged::GO_ACTIONTYPE] = 3;
                sServerEventData_[Net_ObjectCommand::P_NODEID] = dropzone->GetNode()->GetID();
                sServerEventData_[Net_ObjectCommand::P_DATAS] = dropzone->GetStorageAttr();
                PushObjectCommand(UPDATEITEMSSTORAGE, 0, true, 0);
                sServerEventData_.Clear();
            }
        }
    }
    // remote clients players : send equipment to the client
    for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
    {
        if (it->first_ == clientInfo.connection_.Get())
            continue;

        const ClientInfo& cinfo = it->second_;
        for (Vector<SharedPtr<Player > >::ConstIterator jt = cinfo.players_.Begin(); jt != cinfo.players_.End(); ++jt)
        {
            Player* player = *jt;
            if (!player)
                continue;

            Node* avatar = player->GetAvatar();
            if (!avatar)
                continue;

            GOC_Inventory* inventory = avatar->GetComponent<GOC_Inventory>();
            VariantVector equipmentSet;
            if (inventory->GetSectionSlots("Equipment", equipmentSet))
            {
                sServerEventData_[Net_ObjectCommand::P_NODEID] = avatar->GetID();
                sServerEventData_[Net_ObjectCommand::P_INVENTORYTEMPLATE] = inventory->GetTemplateHashName().Value();
                sServerEventData_[Net_ObjectCommand::P_INVENTORYSLOTS] = equipmentSet;
                PushObjectCommand(SETFULLEQUIPMENT, 0, false, clientInfo.clientID_);
                sServerEventData_.Clear();
            }
        }
    }
    // client players : send inventory to the client and equipments to others clients
    sServerEventData_.Clear();
    for (Vector<SharedPtr<Player > >::Iterator it = clientInfo.players_.Begin(); it != clientInfo.players_.End(); ++it)
    {
        Player* player = *it;
        if (!player)
            continue;

        Node* avatar = player->GetAvatar();
        if (!avatar)
            continue;

        GOC_Inventory* inventory = avatar->GetComponent<GOC_Inventory>();
        sServerEventData_[Net_ObjectCommand::P_NODEID] = avatar->GetID();
        sServerEventData_[Net_ObjectCommand::P_INVENTORYTEMPLATE] = inventory->GetTemplateHashName().Value();
        sServerEventData_[Net_ObjectCommand::P_INVENTORYSLOTS] = inventory->GetInventoryAttr();
        PushObjectCommand(SETFULLINVENTORY, 0, false, clientInfo.clientID_);
        sServerEventData_.Clear();

        // client send the equipment to other client via a broadcast
        VariantVector equipmentSet;
        if (inventory->GetSectionSlots("Equipment", equipmentSet))
        {
            sServerEventData_[Net_ObjectCommand::P_NODEID] = avatar->GetID();
            sServerEventData_[Net_ObjectCommand::P_INVENTORYTEMPLATE] = inventory->GetTemplateHashName().Value();
            sServerEventData_[Net_ObjectCommand::P_INVENTORYSLOTS] = equipmentSet;
            PushObjectCommand(SETFULLEQUIPMENT, 0, true, clientInfo.clientID_);
            sServerEventData_.Clear();
        }
    }
}

void GameNetwork::Server_SetActivePlayer(Player* player, bool active)
{
    if (!player)
        return;

    ObjectControlInfo* cinfo = GetServerObjectControl(player->GetAvatar()->GetID());
    if (cinfo)
        cinfo->SetActive(active, true);

    if (active && !player->IsStarted())
        player->Start();
    else
        player->Stop();

    sServerEventData_.Clear();
    sServerEventData_[Net_ObjectCommand::P_NODEID] = player->GetAvatar()->GetID();
    sServerEventData_[Net_ObjectCommand::P_NODEISENABLED] = active;
    PushObjectCommand(ENABLENODE);
}

void GameNetwork::Server_SetEnablePlayers(ClientInfo& clientInfo, bool enable)
{
    if (!serverMode_)
        return;

    if (!clientInfo.connection_)
    {
        URHO3D_LOGWARNINGF("GameNetwork() - Server_SetEnablePlayers : Connection=0 !");
        return;
    }

    URHO3D_LOGINFOF("GameNetwork() - Server_SetEnablePlayers : Connection=%u ...  players enable=%s ... ", clientInfo.connection_.Get(), enable ? "true" : "false");

    // Scan for players
    Vector<SharedPtr<Player> >& players = clientInfo.players_;
    for (Vector<SharedPtr<Player> >::Iterator it=players.Begin(); it!=players.End(); ++it)
    {
        Player* player = it->Get();
        if (!player)
            continue;

        if (!enable)
        {
            SetEnableObject(false, player->GetAvatar()->GetID(), true);
            player->Stop();
        }
        else if (!player->IsStarted())
        {
            player->Start();
            SetEnableObject(true, player->GetAvatar()->GetID(), true);
        }
    }

    URHO3D_LOGINFOF("GameNetwork() - Server_SetEnablePlayers : Connection=%u ... players enable=%s OK !", clientInfo.connection_.Get(), enable ? "true" : "false");
}

void GameNetwork::Server_SetEnableAllActiveObjectsToClient(ClientInfo& clientInfo)
{
    Connection* connection = clientInfo.connection_;

    if (!connection)
        return;

    for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
    {
        const ClientInfo& clinfo = it->second_;
        for (Vector<WeakPtr<Node> >::ConstIterator jt = clinfo.objects_.Begin(); jt != clinfo.objects_.End(); ++jt)
        {
            Node* node = jt->Get();
            if (!node || !node->IsEnabled())
                continue;

            sServerEventData_.Clear();
            sServerEventData_[Net_ObjectCommand::P_NODEID] = node->GetID();
            sServerEventData_[Net_ObjectCommand::P_NODEISENABLED] = true;
            PushObjectCommand(ENABLENODE);
        }
    }
}

void GameNetwork::Server_RemovePlayers(ClientInfo& clientInfo)
{
    if (!serverMode_)
        return;

    URHO3D_LOGINFOF("GameNetwork() - Server_RemovePlayers : Connection=%u ... ", clientInfo.connection_.Get());

    // Scan for serverObjectControls_ to remove
    for (unsigned i=0; i < clientInfo.objects_.Size(); ++i)
    {
        if (!clientInfo.objects_[i]) continue;

        Server_RemoveObject(clientInfo.objects_[i]->GetID(), true, true);
    }

    if (clientInfo.connection_)
        clientInfo.connection_->SetScene(0);

    clientInfo.ClearObjects();
    clientInfo.ClearPlayers();

    URHO3D_LOGINFOF("GameNetwork() - Server_RemovePlayers : Connection=%u ... OK !", clientInfo.connection_.Get());
}

void GameNetwork::Server_RemoveAllPlayers()
{
    HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin();
    while (it != serverClientInfos_.End())
    {
        Server_RemovePlayers(it->second_);
        URHO3D_LOGINFOF("GameNetwork() - Server_RemoveAllPlayers : connection=%u players removed !", it->first_);
        it++;
    }
}


void GameNetwork::Server_PopulateClientIDs()
{
    serverFreeClientIds_.Clear();

    for (int i=0; i < GameContext::Get().MAX_NUMNETPLAYERS; i++)
        serverFreeClientIds_.Push(GameContext::Get().MAX_NUMNETPLAYERS-i);
}

int GameNetwork::Server_GetNextClientID()
{
    if (!serverFreeClientIds_.Size())
        return 0;

    int id = serverFreeClientIds_.Back();
    serverFreeClientIds_.Pop();
    return id;
}

void GameNetwork::Server_FreeClientID(int id)
{
    serverFreeClientIds_.PushFront(id);
}

bool GameNetwork::Server_CheckAccount(const StringHash& key)
{
    // Already logged ?
    if (loggedClientAccounts_.Contains(key))
        return false;

#ifndef NETSERVER_CHECKACCOUNT
    return true;
#endif

    // Check registered Accounts
    if (clientAccounts_.Contains(key))
    {
        return true;
    }

    return false;
}

void GameNetwork::Server_AddConnection(Connection* connection, bool clean)
{
    if (!connection)
    {
        URHO3D_LOGERRORF("GameNetwork() - Server_AddConnection : noconnection !");
        return;
    }

    if (!serverClientInfos_.Contains(connection))
    {
        // if Identity ok => Add connection
        StringHash connectkey(connection->ToString());
        if (Server_CheckAccount(connectkey))
        {
            loggedClientAccounts_.Push(connectkey);

            ClientInfo& clientInfo = serverClientInfos_[connection];
            clientInfo.connection_ = connection;
            clientInfo.clientID_   = Server_GetNextClientID();

            serverClientID2Infos_[clientInfo.clientID_] = &clientInfo;
            serverClientConnections_[clientInfo.clientID_] = connection;

            // Allow Sending ObjectControls for this connection
            SetEnabledServerObjectControls(connection, true);
            SetEnabledClientObjectControls(connection, true);

            URHO3D_LOGINFOF("GameNetwork() - Server_AddConnection : connection=%u IP=%s clientID=%d ... OK !", connection, connection->ToString().CString(), clientInfo.clientID_);

            Server_SendGameStatus(gameStatus_, connection);
            Server_SendSeedTime(seedTime_);

            DumpClientInfos();
        }
    }
    else if (clean)
    {
        // Cleanup last client connection
        URHO3D_LOGINFOF("GameNetwork() - Server_AddConnection : connection = %u ... cleaned ...", connection);
        Server_CleanUpConnection(connection);
        serverClientInfos_[connection].connection_ = connection;
        URHO3D_LOGINFOF("GameNetwork() - Server_AddConnection : connection = %u ... cleaned ... OK !", connection);
    }

    allClientsSynchronized_ = allClientsRunning_ = false;
}

void GameNetwork::Server_KillConnections()
{
    URHO3D_LOGINFOF("GameNetwork() - Server_KillConnections : num connections=%u !", serverClientInfos_.Size());

    if (!serverMode_)
        return;

    Server_SendGameStatus(KILLCLIENTS);

    HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin();
    while (it != serverClientInfos_.End())
    {
        if (it->second_.connection_)
        {
            if (gameStatus_ >= PLAYSTATE_READY)
                Server_RemovePlayers(it->second_);

            Server_CleanUpConnection(it->second_.connection_);
            Server_FreeClientID(it->second_.clientID_);

            URHO3D_LOGINFOF("GameNetwork() - Server_KillConnections : connection=%u killed !", it->first_);
        }

        it = serverClientInfos_.Erase(it);
    }

    serverClientID2Infos_.Clear();
    serverClientConnections_.Clear();
}

void GameNetwork::Server_PurgeConnections()
{
    if (!serverMode_)
        return;

    HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin();
    while (it != serverClientInfos_.End())
    {
        if (!it->second_.connection_)
        {
            URHO3D_LOGINFOF("GameNetwork() - Server_PurgeConnections : connection=%u purged !", it->first_);
            int clientid = it->second_.clientID_;
            Server_FreeClientID(clientid);
            it = serverClientInfos_.Erase(it);
            serverClientID2Infos_.Erase(clientid);
            serverClientConnections_.Erase(clientid);
        }
        else
        {
            it++;
        }
    }
}

void GameNetwork::Server_PurgeConnection(Connection* connection)
{
    if (!connection)
        return;

    HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Find(connection);
    if (it == serverClientInfos_.End())
        return;

    URHO3D_LOGINFOF("GameNetwork() - Server_PurgeConnection : connection=%u ...", connection);

    if (gameStatus_ >= PLAYSTATE_READY)
        Server_RemovePlayers(it->second_);

    Server_CleanUpConnection(connection);

    int clientid = it->second_.clientID_;
    Server_FreeClientID(clientid);
    serverClientInfos_.Erase(it);
    serverClientID2Infos_.Erase(clientid);
    serverClientConnections_.Erase(clientid);
}

void GameNetwork::Server_CleanUpConnection(Connection* connection)
{
    if (!serverMode_)
        return;

    if (!connection)
        return;

    URHO3D_LOGINFOF("GameNetwork() - Server_CleanUpConnection : connection=%u ... ", connection);

    // Cleanup server client info for the connection
    if (serverClientInfos_.Contains(connection))
    {
        ClientInfo& clientInfo = serverClientInfos_[connection];
        connection = clientInfo.connection_;

        for (List<ObjectCommandPacket* >::Iterator it = clientInfo.objCmdInfo_.objCmdPacketsToSend_.Begin(); it != clientInfo.objCmdInfo_.objCmdPacketsToSend_.End(); ++it)
            if (*it)
                objCmdPckPool_.RemoveRef(*it);

        clientInfo.Clear();

//        connection->ResetObjectControlAndCommands();
    }

    // Cleanup scene from connection
    GameContext::Get().rootScene_->CleanupConnection(connection);

    URHO3D_LOGINFOF("GameNetwork() - Server_CleanUpConnection : connection=%u ... OK !", connection);
}

void GameNetwork::Server_CleanUpAllConnections()
{
    HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin();
    while (it++ != serverClientInfos_.End())
    {
        Server_CleanUpConnection(it->second_.connection_);
        URHO3D_LOGINFOF("GameNetwork() - Server_CleanUpConnection : connection=%u cleaned !", it->first_);
    }
}


/// GameNetwork Client Setters Only

void GameNetwork::Client_SendGameStatus()
{
    if (!clientSideConnection_)
        return;

    sClientEventData_[Net_ObjectCommand::P_COMMAND] = (int)GAMESTATUS;
    sClientEventData_[Net_GameStatusChanged::P_STATUS] = (int)gameStatus_;
    sClientEventData_[Net_GameStatusChanged::P_GAMEMODE] = (int)(GameContext::Get().arenaZoneOn_ ? 1 : 2);
    sClientEventData_[Net_GameStatusChanged::P_NUMPLAYERS] = GameContext::Get().numPlayers_;

    PushObjectCommand(sClientEventData_, false, clientID_);

    URHO3D_LOGINFOF("GameNetwork() - Client_SendGameStatus : send NET_GAMESTATUSCHANGED to Server : gameStatus_=%s(%d) !", gameStatusNames[gameStatus_], gameStatus_);
}

void GameNetwork::Client_AddServerControllable(Node*& node, ObjectControlInfo& info)
{
    if (node)
    {
        GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
        if (animator)
            animator->ResetState();

        URHO3D_LOGINFOF("GameNetwork() - Client_AddServerControllable : avatar node=%u(%u) activated ... OK !", node->GetID(), info.clientNodeID_);
    }
    else
    {
        node = GOT::GetClonedControllable(StringHash(info.GetReceivedControl().states_.type_), info.clientNodeID_);
        if (!node)
        {
            URHO3D_LOGERRORF("GameNetwork() - Client_AddServerControllable : ... NOK !");
            return;
        }

        node->AddTag(PLAYERTAG);
        node->SetName("Avatar");
        node->SetTemporary(true);

        node->SetVar(GOA::ONVIEWZ, (int)info.GetReceivedControl().states_.viewZ_);
        node->SetVar(GOA::CLIENTID, info.clientId_);
        GOC_Controller* gocController = node->GetOrCreateComponent<GOC_Controller>(LOCAL);
        SmoothedTransform* smooth = node->GetOrCreateComponent<SmoothedTransform>(LOCAL);

        node->ApplyAttributes();
        node->SendEvent(WORLD_ENTITYCREATE);

        GOC_Inventory::NetClientSetEquipment(node, node->GetID());

        URHO3D_LOGINFOF("GameNetwork() - Client_AddServerControllable : avatar node=%u(%u) nodeclientid=%d - add new controllable ... OK !", node->GetID(), info.clientNodeID_, info.clientId_);
    }

    if (!node)
        return;

    node->SetVar(GOA::ONVIEWZ, (int)info.GetReceivedControl().states_.viewZ_);
    node->SetVar(GOA::FACTION, (info.clientId_ << 8) + GO_Player);

    bool physicOk = GameHelpers::SetPhysicProperties(node, *((PhysicEntityInfo*) &info.GetReceivedControl().physics_));

    GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
    if (destroyer)
        destroyer->SetDestroyMode(node->isPoolNode_ ? POOLRESTORE : DISABLE);

    Light* light = node->GetComponent<Light>();
    if (light)
        light->SetEnabled(false);
}

void GameNetwork::Client_RemoveObject(unsigned nodeid)
{
    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->clientNodeID_ != nodeid)
            continue;

        Client_RemoveObject(*it);
    }
}

void GameNetwork::Client_RemoveObject(ObjectControlInfo& cinfo)
{
    URHO3D_LOGINFOF("GameNetwork() - Client_RemoveObject : objectcontrol=%u servernodeid=%u clientnodeid=%u ... OK !", &cinfo, cinfo.serverNodeID_, cinfo.clientNodeID_);
    cinfo.SetEnable(false);
    cinfo.SetActive(false, false);
}

void GameNetwork::Client_RemoveObjects()
{
    // Remove server Object Controls
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        ObjectControlInfo& control = *it;
        it->SetActive(false, false);
        it->SetEnable(false);
        Node* node = GameContext::Get().rootScene_->GetNode(control.clientNodeID_);
        if (node)
        {
            URHO3D_LOGINFOF("GameNetwork() - Client_RemoveObjects : serverObjectControls_ %s(%u) removed !", node->GetName().CString(), node->GetID());
            node->SendEvent(WORLD_ENTITYDESTROY);
        }
    }

    // Remove client Object Controls
    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        ObjectControlInfo& control = *it;
        Node* node = GameContext::Get().rootScene_->GetNode(control.clientNodeID_);
        if (node)
        {
            URHO3D_LOGINFOF("GameNetwork() - Client_RemoveObjects : clientObjectControls_ %s(%u) removed !", node->GetName().CString(), node->GetID());
            node->SendEvent(WORLD_ENTITYDESTROY);
        }

        Client_RemoveObject(control);
    }
}

void GameNetwork::Client_PurgeConnection()
{
    ClearObjectCommands();

    if (clientSideConnection_)
    {
        SetEnabledServerObjectControls(clientSideConnection_, false);
        SetEnabledClientObjectControls(clientSideConnection_, false);
        clientSideConnection_->ResetObjectControlAndCommands();
    }

    ClearScene();

    // Disconnect Time = 100 to prevent crash
    if (clientSideConnection_)
        clientSideConnection_->Disconnect(100);
}


/// GameNetwork Getters

void GameNetwork::GetLocalAddresses(Vector<String>& adresses)
{
//    adresses.Clear();
//
//    if (!net_)
//        return;
//
//    String hostname(net_->GetLocalAddress());
//    Vector<String> split = hostname.Split('.');
//
//    if (split.Size() == 4)
//    {
//        // IPv4 Type
//        adresses.Push(hostname);
//    }
//    else
//    {
//        // HostName Type
//        struct hostent *phe = gethostbyname(hostname.CString());
//        /// TODO : try getaddrinfo instead of gethostbyname => IPv6 compliant
//        // On Linux : http://man7.org/linux/man-pages/man3/getaddrinfo.3.html
//        // On Win   : https://msdn.microsoft.com/en-us/library/windows/desktop/ms738520(v=vs.85).aspx
//        if (phe > 0)
//        {
//            for (int i = 0; phe->h_addr_list[i] != 0; ++i)
//            {
//                struct in_addr addr;
//                memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
//		/// TODO : try inet_ntop() or InetNtop() instead of inet_ntoa => IPv6 compliant
//                adresses.Push(String(inet_ntoa(addr))); ///\todo inet_ntoa is deprecated! doesn't handle IPv6!
//            }
//        }
//        else
//            URHO3D_LOGERROR("GameNetwork() - GetLocalAddress : Bad host lookup !");
//    }
}

/// Used Only by Server
GameStatus GameNetwork::GetGameStatus(Connection* connection) const
{
    HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Find(connection);
    return it != serverClientInfos_.End() ? it->second_.gameStatus_ : NOGAMESTATE;
}

/// Used by Client or Server
ObjectControlInfo& GameNetwork::GetOrCreateServerObjectControl(unsigned servernodeid, unsigned clientnodeid, int clientid, Node* node)
{
    if (servernodeid)
    {
        for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
        {
            ObjectControlInfo& controlInfo = *it;

            if (controlInfo.serverNodeID_ == servernodeid)
            {
                if (clientnodeid != 0 && controlInfo.clientNodeID_ != clientnodeid)
                {
                    controlInfo.clientNodeID_ = clientnodeid;
                    controlInfo.clientId_ = clientid;
                }

                if (node)
                {
                    if (controlInfo.node_ && ((serverMode_ && controlInfo.node_->GetID() != servernodeid) || (clientMode_ && controlInfo.node_->GetID() != clientnodeid)))
                        URHO3D_LOGERRORF("GameNetwork() - GetOrCreateServerObjectControl : servernodeid=%u clientnodeid=%u replace existing node !", servernodeid, clientnodeid);

                    controlInfo.node_ = node;
                }

                controlInfo.SetActive(true, true);
                return controlInfo;
            }
        }
    }

    // if not find, create a new ObjectControlInfo with owner (0 == server)
    serverObjectControls_.Resize(serverObjectControls_.Size()+1);

    ObjectControlInfo& controlInfo = serverObjectControls_.Back();
    controlInfo.serverNodeID_ = servernodeid;
    controlInfo.clientNodeID_ = clientnodeid;
    controlInfo.clientId_ = clientid;

    if (servernodeid && serverMode_)
    {
        if (!controlInfo.node_)
            controlInfo.node_ = !node ? GameContext::Get().rootScene_->GetNode(servernodeid) : node;

        if (!controlInfo.node_ && !GameContext::Get().rootScene_->IsPoolNodeReserved(servernodeid) && !GameContext::Get().rootScene_->IsNodeReserved(servernodeid))
            GameContext::Get().rootScene_->ReserveNodeID(servernodeid);
    }
    else if (clientnodeid && clientMode_)
    {
        if (!controlInfo.node_)
            controlInfo.node_ = !node ? GameContext::Get().rootScene_->GetNode(clientnodeid) : node;

        if (!controlInfo.node_ && !GameContext::Get().rootScene_->IsPoolNodeReserved(clientnodeid) && !GameContext::Get().rootScene_->IsNodeReserved(clientnodeid))
            GameContext::Get().rootScene_->ReserveNodeID(clientnodeid);
    }

    controlInfo.SetActive(true, true);
    controlInfo.SetEnable(false);

    URHO3D_LOGINFOF("GameNetwork() - GetOrCreateServerObjectControl : servernodeid=%u clientnodeid=%u clientId=%d ... node=%u => Added to serverObjectControls_.size=%u !",
                    servernodeid, clientnodeid, controlInfo.clientId_, controlInfo.node_.Get(), serverObjectControls_.Size());

    return controlInfo;
}

ObjectControlInfo* GameNetwork::GetServerObjectControl(unsigned nodeid)
{
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
            return &(*it);
    }

    return 0;
}



/// Used Only by Client
ObjectControlInfo* GameNetwork::GetOrCreateClientObjectControl(unsigned servernodeid, unsigned clientnodeid)
{
    if (!clientnodeid)
        clientnodeid = servernodeid;

    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        ObjectControlInfo& controlInfo = *it;

        if (controlInfo.clientNodeID_ == clientnodeid)
        {
            controlInfo.clientId_ = clientID_;

            if (controlInfo.active_)
            {
                controlInfo.serverNodeID_ = servernodeid;
                if (!controlInfo.node_)
                    controlInfo.node_ = GameContext::Get().rootScene_->GetNode(clientnodeid);

//                if (!GameContext::Get().IsAvatarNodeID(servernodeid))
//                URHO3D_LOGINFOF("GameNetwork() - GetOrCreateClientObjectControl : servernodeid=%u clientnodeid=%u clientId=%d => Get Existing oinfo=%u !",
//                                servernodeid, clientnodeid, controlInfo.clientId_, &controlInfo);
            }

            return &controlInfo;
        }
    }

    // if not find, create a new ObjectControlInfo with this connection as owner
    bool newcontrol = false;
    Node* node = GameContext::Get().rootScene_->GetNode(clientnodeid);
    if (GameContext::Get().IsAvatarNodeID(servernodeid, clientID_))
        newcontrol = true;

    if (!newcontrol && (node || (!GameContext::Get().rootScene_->IsPoolNodeReserved(clientnodeid) && !GameContext::Get().rootScene_->IsNodeReserved(clientnodeid) && clientnodeid == servernodeid)))
    {
        if (!node)
            GameContext::Get().rootScene_->ReserveNodeID(clientnodeid);
        newcontrol = true;
    }

    if (!newcontrol)
    {
        URHO3D_LOGERRORF("GameNetwork() - GetOrCreateClientObjectControl : ClientID=%d servernodeid=%u clientnodeid=%u => can't create control !",
                         clientID_, servernodeid, clientnodeid);
        return 0;
    }

    clientObjectControls_.Resize(clientObjectControls_.Size()+1);

    ObjectControlInfo& controlInfo = clientObjectControls_.Back();
    controlInfo.clientId_ = clientID_;
    controlInfo.serverNodeID_ = servernodeid;
    controlInfo.clientNodeID_ = clientnodeid;

    if (node)
        controlInfo.node_ = node;

    controlInfo.SetActive(true, false);
    controlInfo.SetEnable(false);

//    if (!GameContext::Get().IsAvatarNodeID(servernodeid))
//    URHO3D_LOGINFOF("GameNetwork() - GetOrCreateClientObjectControl : ClientID=%d servernodeid=%u clientnodeid=%u => Added new objectcontrol=%u to clientObjectControls.size=%u !",
//                    clientID_, servernodeid, clientnodeid, &controlInfo, clientObjectControls_.Size());

    return &controlInfo;
}

ObjectControlInfo* GameNetwork::GetObjectControl(unsigned nodeid)
{
    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->clientNodeID_ == nodeid)
            return &(*it);
    }
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
            return &(*it);
    }

    return 0;
}

const ObjectControlInfo* GameNetwork::GetObjectControl(unsigned nodeid) const
{
    for (Vector<ObjectControlInfo>::ConstIterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->clientNodeID_ == nodeid)
            return &(*it);
    }
    for (Vector<ObjectControlInfo>::ConstIterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->serverNodeID_ == nodeid)
            return &(*it);
    }

    return 0;
}

ObjectControlInfo* GameNetwork::Client_GetServerObjectControlFromLocalNodeID(unsigned clientnodeid)
{
    for (Vector<ObjectControlInfo>::Iterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
    {
        if (it->clientNodeID_ == clientnodeid)
            return &(*it);
    }

    return 0;
}

ObjectControlInfo* GameNetwork::GetClientObjectControl(unsigned nodeid)
{
    for (Vector<ObjectControlInfo>::Iterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
    {
        if (it->clientNodeID_ == nodeid)
            return &(*it);
    }

    return 0;
}

Connection* GameNetwork::GetConnection() const
{
    return clientSideConnection_.Get();
}


/// GameNetwork Subscribers

void GameNetwork::SubscribeToEvents()
{
    URHO3D_LOGINFOF("GameNetwork() - SubscribeToEvents ...");

    // Generic Events ServerMode
    if (serverMode_)
    {
        SubscribeToEvent(E_CLIENTCONNECTED, URHO3D_HANDLER(GameNetwork, HandleServer_MessagesFromClient));
        SubscribeToEvent(E_CLIENTDISCONNECTED, URHO3D_HANDLER(GameNetwork, HandleServer_MessagesFromClient));
        SubscribeToEvent(E_NETWORKUPDATE, URHO3D_HANDLER(GameNetwork, HandlePlayServer_NetworkUpdate));
        SubscribeToEvent(OBJECTCOMMANDSRECEIVED, URHO3D_HANDLER(GameNetwork, HandlePlayServer_ReceiveCommands));

//        SubscribeToEvent(NET_GAMESTATUSCHANGED, URHO3D_HANDLER(GameNetwork, HandleServer_MessagesFromClient));
    }
    else // Generic Events ClientMode
    {
        SubscribeToEvent(E_NETWORKUPDATE, URHO3D_HANDLER(GameNetwork, HandlePlayClient_NetworkUpdate));
        SubscribeToEvent(OBJECTCOMMANDSRECEIVED, URHO3D_HANDLER(GameNetwork, HandlePlayClient_ReceiveCommands));

//        SubscribeToEvent(NET_GAMESTATUSCHANGED, URHO3D_HANDLER(GameNetwork, HandleClient_MessagesFromServer));
    }


    // Game Status Specific Events
    if (gameStatus_ < PLAYSTATE_INITIALIZING)
    {
        UnsubscribeToPlayEvents();
        SubscribeToMenuEvents();
    }
    else
    {
        UnsubscribeToMenuEvents();
        SubscribeToPlayEvents();
    }

    URHO3D_LOGINFOF("GameNetwork() - SubscribeToEvents ... OK !");
}

void GameNetwork::SubscribeToMenuEvents()
{
    ClearSpawnControls();
    PurgeObjects();
}

void GameNetwork::UnsubscribeToMenuEvents()
{

}

void GameNetwork::SubscribeToPlayEvents()
{
    if (serverMode_)
        Server_SubscribeToPlayEvents();

    if (clientMode_)
        Client_SubscribeToPlayEvents();
}

void GameNetwork::UnsubscribeToPlayEvents()
{
    if (serverMode_)
        Server_UnsubscribeToPlayEvents();

    if (clientMode_)
        Client_UnsubscribeToPlayEvents();
}


/// Server Subscribers For sPlay

void GameNetwork::Server_SubscribeToPlayEvents()
{
    URHO3D_LOGINFOF("GameNetwork() - Server_SubscribeToPlayEvents ...");

    SubscribeToEvent(E_NETWORKSCENELOADFAILED, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));

    SubscribeToEvent(GO_INVENTORYGET, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));
    SubscribeToEvent(GO_INVENTORYSLOTEQUIP, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));
    SubscribeToEvent(MAP_ADDFURNITURE, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));
    SubscribeToEvent(MAP_ADDCOLLECTABLE, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));
    SubscribeToEvent(GO_TRIGCLICKED, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));
    SubscribeToEvent(GO_MOUNTEDON, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));
    SubscribeToEvent(GO_STORAGECHANGED, URHO3D_HANDLER(GameNetwork, HandlePlayServer_Messages));

    SubscribeToEvent(CLIENTOBJECTCONTROLSRECEIVED, URHO3D_HANDLER(GameNetwork, HandlePlayServer_ReceiveControls));

    Server_PurgeInactiveObjects();
}

void GameNetwork::Server_UnsubscribeToPlayEvents()
{
    URHO3D_LOGINFOF("GameNetwork() - Server_UnsubscribeToPlayEvents ...");

    UnsubscribeFromEvent(E_NETWORKSCENELOADFAILED);

    UnsubscribeFromEvent(GO_INVENTORYGET);
    UnsubscribeFromEvent(GO_INVENTORYSLOTEQUIP);
    UnsubscribeFromEvent(MAP_ADDFURNITURE);
    UnsubscribeFromEvent(MAP_ADDCOLLECTABLE);
    UnsubscribeFromEvent(GO_TRIGCLICKED);
    UnsubscribeFromEvent(GO_MOUNTEDON);
    UnsubscribeFromEvent(GO_STORAGECHANGED);

    UnsubscribeFromEvent(CLIENTOBJECTCONTROLSRECEIVED);

    Server_RemoveAllPlayers();
    Server_SetEnableAllObjects(false);
}

/// Client Subscribers For sPlay

void GameNetwork::Client_SubscribeToPlayEvents()
{
    URHO3D_LOGINFOF("GameNetwork() - Client_SubscribeToPlayEvents !");

    SubscribeToEvent(GO_INVENTORYSLOTEQUIP, URHO3D_HANDLER(GameNetwork, HandlePlayClient_Messages));
    SubscribeToEvent(GO_INVENTORYSLOTSET, URHO3D_HANDLER(GameNetwork, HandlePlayClient_Messages));
    SubscribeToEvent(GO_DROPITEM, URHO3D_HANDLER(GameNetwork, HandlePlayClient_Messages));
    SubscribeToEvent(MAP_ADDFURNITURE, URHO3D_HANDLER(GameNetwork, HandlePlayClient_Messages));
    SubscribeToEvent(GO_TRIGCLICKED, URHO3D_HANDLER(GameNetwork, HandlePlayClient_Messages));
    SubscribeToEvent(GO_SELECTED, URHO3D_HANDLER(GameNetwork, HandlePlayClient_Messages));

    if (clientSideConnection_)
    {
        SubscribeToEvent(SERVEROBJECTCONTROLSRECEIVED, URHO3D_HANDLER(GameNetwork, HandlePlayClient_ReceiveServerControls));
        SubscribeToEvent(CLIENTOBJECTCONTROLSRECEIVED, URHO3D_HANDLER(GameNetwork, HandlePlayClient_ReceiveClientControls));

        SetEnabledServerObjectControls(clientSideConnection_, true);
        SetEnabledClientObjectControls(clientSideConnection_, true);
    }
}

void GameNetwork::Client_UnsubscribeToPlayEvents()
{
    URHO3D_LOGINFOF("GameNetwork() - Client_UnsubscribeToPlayEvents !");

    UnsubscribeFromEvent(GO_INVENTORYSLOTEQUIP);
    UnsubscribeFromEvent(GO_INVENTORYSLOTSET);
    UnsubscribeFromEvent(GO_DROPITEM);
    UnsubscribeFromEvent(MAP_ADDFURNITURE);
    UnsubscribeFromEvent(GO_TRIGCLICKED);

    UnsubscribeFromEvent(SERVEROBJECTCONTROLSRECEIVED);
    UnsubscribeFromEvent(CLIENTOBJECTCONTROLSRECEIVED);


    if (clientSideConnection_)
    {
        SetEnabledServerObjectControls(clientSideConnection_, false);
        SetEnabledClientObjectControls(clientSideConnection_, false);
    }
}


/// GameNetwork Generic Handles : connection establishing

void GameNetwork::HandleSearchServer(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_SERVERCONNECTED)
    {
        URHO3D_LOGINFOF("GameNetwork() - HandleSearchServer : E_SERVERCONNECTED !");

        if (!serverMode_)
            StartClient();
    }

    else if (eventType == E_UPDATE)
    {
        float timestep = eventData[Update::P_TIMESTEP].GetFloat();
        timerCheckForServer_ += timestep;
        timerCheckForConnection_ += timestep;

        if (timerCheckForConnection_ > delayForConnection_ && !serverMode_)
        {
            // Start Local
            if (timerCheckForServer_ >= delayForServer_)
            {
                StartLocal();
            }
            else
            {
                // Try Connection to Server
                URHO3D_LOGINFOF("GameNetwork() - HandleSearchServer : connecting to Server=%s:%d ...", serverIP_.CString(), serverPort_);
                AddGraphicMessage(context_, "Searching.Network...", IntVector2(0, -10), WHITEGRAY30, 5.f);
                net_->Connect(serverIP_, serverPort_, GameContext::Get().rootScene_);
            }

            timerCheckForConnection_ = 0.f;
        }
    }
}

void GameNetwork::HandleConnectionStatus(StringHash eventType, VariantMap& eventData)
{
    if (eventType == E_CONNECTFAILED)
    {
        URHO3D_LOGERRORF("GameNetwork() - HandleConnectionStatus : E_CONNECTFAILED !");

        if (serverMode_)
            return;

        timerCheckForConnection_ = 0;

        Client_RemoveObjects();
        Client_PurgeConnection();

        SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(GameNetwork, HandleSearchServer));
    }
    else if (eventType == E_SERVERDISCONNECTED)
    {
        URHO3D_LOGERRORF("GameNetwork() - HandleConnectionStatus : E_SERVERDISCONNECTED !");

        Client_RemoveObjects();
        Client_PurgeConnection();

        AddGraphicMessage(context_, "No.Server.!", IntVector2(20, -60), MAGENTABLACK30, 5.f, 0.f);

        if (clientMode_)
            StartLocal();
    }
}


/// GameNetwork Handle Server Receive Messages

void GameNetwork::Server_ApplyReceivedGameStatus(GameStatus newStatus, VariantMap& eventData, ClientInfo& clientInfo)
{
    URHO3D_LOGERRORF("GameNetwork() - Server_ApplyReceivedGameStatus : NET_GAMESTATUSCHANGED from clientID=%d ... ServerStatus=%s(%d) ... receveived clientNewStatus=%s(%d) ...",
                     clientInfo.clientID_, gameStatusNames[gameStatus_], gameStatus_, gameStatusNames[newStatus], newStatus);

    if (newStatus != clientInfo.gameStatus_)
    {
        if (newStatus == MENUSTATE && clientInfo.gameStatus_ > MENUSTATE)
        {
            // Remove Client Players

            URHO3D_LOGERRORF("GameNetwork() - Server_ApplyReceivedGameStatus : clientID=%d MENUSTATE ... Remove Client Players !", clientInfo.clientID_);
            Server_RemovePlayers(clientInfo);
            Server_CleanUpConnection(clientInfo.connection_);

            allClientsRunning_ = allClientsSynchronized_ = false;
            GameContext::Get().AllowUpdate_ = false;
        }
        else if (newStatus > MENUSTATE && newStatus < PLAYSTATE_SYNCHRONIZING && gameStatus_ == MENUSTATE)
        {
            // Launch Server in PlayState

            int gameMode = eventData[Net_GameStatusChanged::P_GAMEMODE].GetInt();
            if (gameMode == 1)
                GameCommands::Launch("playarena");
            else if (gameMode == 2)
                GameCommands::Launch("playworld");
        }
        else if (newStatus >= PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS && newStatus <= PLAYSTATE_SYNCHRONIZING)
        {
            // Set Num Players for the connection

            int numRequestPlayers = eventData[Net_GameStatusChanged::P_NUMPLAYERS].GetInt();
            if (clientInfo.requestPlayers_ <= numRequestPlayers)
            {
                clientInfo.requestPlayers_ = numRequestPlayers;
                URHO3D_LOGERRORF("GameNetwork() - Server_ApplyReceivedGameStatus : clientID=%d request %d players (numactiveplayers=%d) !", clientInfo.clientID_, numRequestPlayers, clientInfo.GetNumActivePlayers());
            }

            if (newStatus == PLAYSTATE_SYNCHRONIZING && gameStatus_ >= PLAYSTATE_READY)
            {
                Server_SendGameStatus(PLAYSTATE_RUNNING, clientInfo.connection_);
                URHO3D_LOGINFOF("GameNetwork() - Server_ApplyReceivedGameStatus : clientID=%d send PLAYSTATE_RUNNING !", clientInfo.clientID_);
            }
        }
        else if (newStatus == PLAYSTATE_STARTGAME || newStatus == PLAYSTATE_RUNNING)
        {
            if (clientInfo.gameStatus_ == PLAYSTATE_ENDGAME)
            {
                if (!clientInfo.rebornRequest_)
                {
                    URHO3D_LOGERRORF("GameNetwork() - Server_ApplyReceivedGameStatus : clientID=%d ... wait for reborn !",  clientInfo.clientID_);
                    clientInfo.rebornRequest_ = true;
                }

                newStatus = PLAYSTATE_ENDGAME;
            }
        }

        clientInfo.gameStatus_ = newStatus;

        URHO3D_LOGERRORF("GameNetwork() - Server_ApplyReceivedGameStatus : clientID=%d ... clientNewStatus=%s(%d) !",  clientInfo.clientID_, gameStatusNames[newStatus], newStatus);
    }
}

void GameNetwork::Client_ApplyReceivedGameStatus(GameStatus newStatus, VariantMap& eventData)
{
    URHO3D_LOGERRORF("GameNetwork() - ApplyReceivedGameStatus : NET_GAMESTATUSCHANGED clientID=%d ... ServerStatus=%s(%d) => ServerNewStatus=%s(%d)",
                     clientID_, gameStatusNames[serverGameStatus_], serverGameStatus_, gameStatusNames[newStatus], newStatus);

    if (newStatus == PLAYSTATE_RUNNING && gameStatus_ == PLAYSTATE_SYNCHRONIZING && needSynchronization_)
    {
        serverGameStatus_ = newStatus;
        lastGameStatus_ = gameStatus_ = PLAYSTATE_READY;

        // All Clients Ready : launch play
        URHO3D_LOGINFOF("GameNetwork() - ApplyReceivedGameStatus : NET_GAMESTATUSCHANGED clientID=%d Status=%s(%d) ... synchronized ... ServerStatus=%s(%d)",
                        clientID_, gameStatusNames[gameStatus_], gameStatus_, gameStatusNames[serverGameStatus_], serverGameStatus_);
    }
    else if (newStatus >= PLAYSTATE_SYNCHRONIZING && gameStatus_ == PLAYSTATE_SYNCHRONIZING && !needSynchronization_)
    {
        serverGameStatus_ = PLAYSTATE_RUNNING;
        lastGameStatus_ = gameStatus_ = PLAYSTATE_READY;

        // Launch Playing Directly
        URHO3D_LOGINFOF("GameNetwork() - ApplyReceivedGameStatus : NET_GAMESTATUSCHANGED clientID=%d Status=%s(%d) ... ready ... ServerStatus=%s(%d)",
                        clientID_, gameStatusNames[gameStatus_], gameStatus_, gameStatusNames[serverGameStatus_], serverGameStatus_);
    }
    else if (newStatus != serverGameStatus_)
    {
        serverGameStatus_ = newStatus;

        if (newStatus == KILLCLIENTS)
        {
            serverGameStatus_ = MENUSTATE;
            URHO3D_LOGINFOF("GameNetwork() - ApplyReceivedGameStatus : NET_GAMESTATUSCHANGED clientID=%d receive KILLCLIENTS !", clientID_);

            SendEvent(E_SERVERDISCONNECTED);
        }
        else if (newStatus == MENUSTATE)
        {
            serverGameStatus_ = newStatus;

            // When GO_Network send this message the client is disconnect From the Server
            if (gameStatus_ == MENUSTATE)
            {
                PurgeObjects();
            }
            else
            {
                Client_RemoveObjects();
                GameContext::Get().stateManager_->PopStack();
            }

            if (clientSideConnection_)
            {
                SetEnabledServerObjectControls(clientSideConnection_, false);
                SetEnabledClientObjectControls(clientSideConnection_, false);
            }
        }
        else if (newStatus > MENUSTATE && gameStatus_ == MENUSTATE)
        {
            // Launch Client in PlayState
            serverGameStatus_ = PLAYSTATE_SYNCHRONIZING;

            int gameMode = eventData[Net_GameStatusChanged::P_GAMEMODE].GetInt();
            if (gameMode == 1)
                GameCommands::Launch("playarena");
            else if (gameMode == 2)
                GameCommands::Launch("playworld");
        }
        else if (newStatus == PLAYSTATE_WINGAME)
        {
            gameStatus_ = PLAYSTATE_WINGAME;
        }
    }
}

void GameNetwork::ApplyReceivedGameStatus(VariantMap& eventData, int clientid)
{
    GameStatus newStatus = (GameStatus) eventData[Net_GameStatusChanged::P_STATUS].GetInt();

    if (serverMode_)
    {
        HashMap<int, ClientInfo* >::Iterator it = serverClientID2Infos_.Find(clientid);
        ClientInfo* cinfo = it == serverClientID2Infos_.End() ? 0 : it->second_;
        if (cinfo)
            Server_ApplyReceivedGameStatus(newStatus, eventData, *cinfo);
    }

    if (clientMode_)
    {
        clientid = eventData[Net_GameStatusChanged::P_CLIENTID].GetInt();
        if (!clientID_ && clientid)
        {
            clientID_ = clientid;
            AddGraphicMessage(context_, ToString("ClientID=%d", clientID_), IntVector2(20, -60), REDYELLOW35);
        }

        Client_ApplyReceivedGameStatus(newStatus, eventData);
    }
}


void GameNetwork::HandleServer_MessagesFromClient(StringHash eventType, VariantMap& eventData)
{
    if (!GetSubsystem<Network>()->IsServerRunning())
        return;

    if (eventType == E_CLIENTCONNECTED)
    {
        URHO3D_LOGINFOF("GameNetwork() - HandleServer_MessagesFromClient : E_CLIENTCONNECTED !");
        Connection* newConnection = static_cast<Connection*>(eventData[ClientConnected::P_CONNECTION].GetPtr());
        Server_AddConnection(newConnection, true);
    }

    if (eventType == E_CLIENTDISCONNECTED)
    {
        Connection* lostConnection = static_cast<Connection*>(eventData[ClientConnected::P_CONNECTION].GetPtr());

        AddGraphicMessage(context_, String("Client." + lostConnection->GetAddress() + ":" + String(lostConnection->GetPort()) + ".Disconnected.!"), IntVector2(20, -10), MAGENTABLACK30, 4.f, 1.f);

        Server_PurgeConnection(lostConnection);
    }
}

void GameNetwork::HandlePlayServer_Messages(StringHash eventType, VariantMap& eventData)
{
    if (!GetSubsystem<Network>()->IsServerRunning())
        return;

    if (eventType == E_NETWORKSCENELOADFAILED)
    {
        Connection* connection = static_cast<Connection*>(eventData[NetworkSceneLoadFailed::P_CONNECTION].GetPtr());
        URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_Messages : Scene Load FAILED on Client connection=%u", connection);
        return;
    }

    if (eventType == GO_INVENTORYGET)
    {
        const unsigned givernodeid = eventData[Go_InventoryGet::GO_GIVER].GetUInt();
        const unsigned getternodeid = eventData[Go_InventoryGet::GO_GETTER].GetUInt();
        // TODO : change scene->GetNode by a cache access
        Node* giver = givernodeid ? GameContext::Get().rootScene_->GetNode(givernodeid) : 0;
        Node* getter = GameContext::Get().rootScene_->GetNode(getternodeid);
        const int giverclientid = giver ? giver->GetVar(GOA::CLIENTID).GetInt() : 0;
        const int getterclientid = getter ? getter->GetVar(GOA::CLIENTID).GetInt() : 0;

        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_Messages : TRANSFERITEM giver(node=%u/nodeid=%u/clientid=%d) getter(node=%u/nodeid=%u/clientid=%d) !",
                        giver, givernodeid, giverclientid, getter, getternodeid, getterclientid);

        //URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_Messages : TRANSFERITEM Dump eventData ...");
        //GameHelpers::DumpVariantMap(eventData);

        if (giverclientid)
            PushObjectCommand(TRANSFERITEM, &eventData, false, giverclientid);

        if (getterclientid && getterclientid != giverclientid)
            PushObjectCommand(TRANSFERITEM, &eventData, false, getterclientid);
    }

    else if (eventType == GO_INVENTORYSLOTEQUIP)
        SendChangeEquipment(eventType, eventData);

    else if (eventType == MAP_ADDFURNITURE)
        PushObjectCommand(ADDFURNITURE, &eventData, true, 0);

    else if (eventType == MAP_ADDCOLLECTABLE)
        PushObjectCommand(ADDCOLLECTABLE, &eventData, true, eventData[Net_ObjectCommand::P_CLIENTID].GetInt());

    else if (eventType == GO_TRIGCLICKED)
    {
        Node* node = static_cast<Node*>(context_->GetEventSender());
        if (node)
        {
            eventData.Clear();
            eventData[Net_ObjectCommand::P_NODEID] = node->GetID();
            PushObjectCommand(TRIGCLICKED, &eventData, true, 0);
        }
    }

    else if (eventType == GO_MOUNTEDON)
        PushObjectCommand(MOUNTENTITYON, &eventData, true, eventData[Net_ObjectCommand::P_CLIENTID].GetInt());

    else if (eventType == GO_STORAGECHANGED)
        PushObjectCommand(UPDATEITEMSSTORAGE, &eventData, true, 0);
}


/// GameNetwork Handle Client Receive Messages

void GameNetwork::HandleClient_MessagesFromServer(StringHash eventType, VariantMap& eventData)
{
}

void GameNetwork::HandlePlayClient_Messages(StringHash eventType, VariantMap& eventData)
{
    if (!clientSideConnection_)
    {
        URHO3D_LOGWARNINGF("GameNetwork() - HandlePlayClient_Messages : no connection to server !");
        return;
    }

    if (eventType == GO_INVENTORYSLOTEQUIP)
        SendChangeEquipment(eventType, eventData);

    else if (eventType == GO_INVENTORYSLOTSET)
        PushObjectCommand(SETITEM, &eventData, false, clientID_); // only to server

    else if (eventType == GO_DROPITEM)
        PushObjectCommand(DROPITEM, &eventData, false, clientID_);

    else if (eventType == MAP_ADDFURNITURE)
        PushObjectCommand(ADDFURNITURE, &eventData, true, clientID_);

    else if (eventType == GO_TRIGCLICKED)
    {
        Node* node = static_cast<Node*>(context_->GetEventSender());
        if (node)
        {
            eventData.Clear();
            eventData[Net_ObjectCommand::P_NODEID] = node->GetID();
            PushObjectCommand(TRIGCLICKED, &eventData, true, clientID_);
        }
    }

    else if (eventType == GO_SELECTED)
    {
        PushObjectCommand(ENTITYSELECTED, &eventData, false, clientID_);
    }
}


bool GameNetwork::PrepareControl(ObjectControlInfo& info)
{
    if (info.prepared_)
        return true;

    if (!info.node_)
        return false;

    bool maincontroller = false;
    GOC_Controller* controller = info.node_->GetDerivedComponent<GOC_Controller>();
    if (controller)
        maincontroller = controller->IsMainController();
    else
        maincontroller = (serverMode_ && info.clientId_ == 0) || (clientMode_ && info.clientId_ == clientID_);

    // check dead node
    if (info.preparedControl_.IsEnabled() && info.node_->isPoolNode_ && info.node_->isInPool_)
        info.preparedControl_.states_.totalDpsReceived_ = -1.f;

    // only authoritative server can prepare the following state
    if (serverMode_)
    {
        GOC_Life* goclife = info.node_->GetComponent<GOC_Life>();
        if (goclife)
        {
            // if alive set totaldps
            if (goclife->GetLife() > 0)
                info.preparedControl_.states_.totalDpsReceived_ = goclife->GetTotalEnergyLost();
            // if dead set max total dps
            else
                info.preparedControl_.states_.totalDpsReceived_ = 2.f*goclife->GetTemplateMaxEnergy();
        }
    }

    if (serverMode_ && info.preparedControl_.states_.stamp_ == 0)
        maincontroller = true;

    if (maincontroller)
    {
        // prepare physics only if not mounted
        if (info.node_->GetVar(GOA::ISMOUNTEDON).GetUInt() == 0)
        {
            info.preparedControl_.physics_.positionx_ = info.node_->GetPosition().x_;
            info.preparedControl_.physics_.positiony_ = info.node_->GetPosition().y_;
            info.preparedControl_.physics_.rotation_  = info.node_->GetRotation2D();
            RigidBody2D* rigidbody = info.node_->GetComponent<RigidBody2D>();
            if (rigidbody && rigidbody->GetBody())
                rigidbody->GetBody()->GetVelocity(info.preparedControl_.physics_.velx_, info.preparedControl_.physics_.vely_);
        }

        GOC_Animator2D* gocanimator = info.node_->GetComponent<GOC_Animator2D>();

        if (controller)
            info.preparedControl_.physics_.direction_ = controller->control_.direction_;
        else if (gocanimator)
            info.preparedControl_.physics_.direction_ = gocanimator->GetDirection().x_;
        else
        {
            const Variant& vardir = info.node_->GetVar(GOA::DIRECTION);
            if (!vardir.IsEmpty())
                info.preparedControl_.physics_.direction_ = vardir.GetVector2().x_;
        }

        // prepare states
        info.preparedControl_.states_.viewZ_ = (char)info.node_->GetVar(GOA::ONVIEWZ).GetInt();

        info.preparedControl_.SetFlagBit(OFB_ENABLED, info.node_->IsEnabled());

        if (controller)
        {
            info.preparedControl_.states_.type_ = controller->control_.type_;
            if (maincontroller)
                info.preparedControl_.states_.buttons_ = controller->control_.buttons_;

            info.preparedControl_.SetAnimationState(StringHash(controller->control_.animation_));
            info.preparedControl_.states_.entityid_ = controller->control_.entityid_;
        }
        else
        {
            info.preparedControl_.states_.type_ = info.node_->GetVar(GOA::GOT).GetUInt();

            if (gocanimator)
                info.preparedControl_.SetAnimationState(gocanimator->GetStateValue());

            AnimatedSprite2D* animatedSprite = info.node_->GetComponent<AnimatedSprite2D>();
            if (animatedSprite)
                info.preparedControl_.states_.entityid_ = animatedSprite->GetSpriterEntityIndex();
        }

        if (gocanimator)
        {
            if (gocanimator->GetNetChangeCounter() != info.lastNetChangeCounter_)
            {
                info.preparedControl_.SetFlagBit(OFB_ANIMATIONCHANGED, true);
                info.lastNetChangeCounter_ = gocanimator->GetNetChangeCounter();
            }
            else
            {
                info.preparedControl_.SetFlagBit(OFB_ANIMATIONCHANGED, false);
            }

            info.preparedControl_.states_.animversion_ = gocanimator->GetCurrentAnimVersion();
        }
    }

#ifdef ACTIVE_PACKEDOBJECTCONTROL
    info.packed_ = false;
#endif

    info.prepared_ = true;
    return true;
}

bool GameNetwork::UpdateControlAck(ObjectControlInfo& dest, const ObjectControlInfo& src)
{
    if (dest.active_ && (!dest.node_ || (dest.node_->isPoolNode_ && dest.node_->isInPool_)))
    {
        URHO3D_LOGINFOF("GameNetwork() - UpdateControlAck : node=%u servernodeid=%u clientnodeid=%u ... node restored ! desactive objectcontrol",
                        dest.node_.Get(), dest.serverNodeID_, dest.clientNodeID_);

        Client_RemoveObject(dest);
        return false;
    }

    Node* node = dest.node_;

    // copy the temporary object control in receivedcontrol
    src.CopyReceivedControlAckTo(dest.GetReceivedControl());

    if (!dest.active_)
        return false;

    /// Client : Update Life from received Server Authoritative State
    GOC_Life* goclife = node->GetComponent<GOC_Life>();
    if (goclife)
    {
//        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u receive TDPS=%f !", node->GetID(), src.GetReceivedControl().states_.totalDpsReceived_);

        float deltatdps = src.GetReceivedControl().states_.totalDpsReceived_ - goclife->GetTotalEnergyLost();
        if (Abs(deltatdps) > 0.01f)
        {
            URHO3D_LOGINFOF("GameNetwork() - UpdateControlAck : node=%u Delta TDPS %f (%f - %f)!",
                            node->GetID(), deltatdps, dest.GetReceivedControl().states_.totalDpsReceived_, goclife->GetTotalEnergyLost());

            goclife->SetTotalEnergyLost(src.GetReceivedControl().states_.totalDpsReceived_);
        }
    }

    /// Client : Server Reconciliation for a local entity (owner=this client)
#ifdef ACTIVE_NETWORK_SERVERRECONCILIATION
    if (!src.GetReceivedControl().IsEnabled())
    {
        URHO3D_LOGERRORF("GameNetwork() - UpdateControlAck : node=%u Error : No enabled on server !", node->GetID());
        return false;
    }

    if (!dest.sendedControls_.Size())
    {
        URHO3D_LOGERRORF("GameNetwork() - UpdateControlAck : node=%u Error : No SendedControl for reconciliation !", node->GetID());
        return false;
    }

    int serverstamp = src.GetReceivedControl().states_.stamp_;
    if (!serverstamp)
        return false;

    // remove old sended controls already processed by the server
    int clientstamp;
    for (;;)
    {
        clientstamp = dest.sendedControls_.Front().states_.stamp_;

        if (serverstamp < clientstamp && clientstamp > dest.sendedControls_.Back().states_.stamp_)
        {
            dest.sendedControls_.PopFront();
            continue;
        }

        if (serverstamp <= clientstamp)
            break;

        dest.sendedControls_.PopFront();

        if (dest.sendedControls_.Size() <= 1)
            break;
    }

    if (!dest.sendedControls_.Size())
        return true;

    // check delta between server and client states at serverstamp
    clientstamp = dest.sendedControls_.Front().states_.stamp_;
    if (serverstamp == clientstamp)
    {
        float dx = dest.sendedControls_.Front().physics_.positionx_ - src.GetReceivedControl().physics_.positionx_;
        float dy = dest.sendedControls_.Front().physics_.positiony_ - src.GetReceivedControl().physics_.positiony_;

        if (Abs(dx) > 0.1f || Abs(dy) > 0.1f)
        {
            // Reconciliation : Move of the deltaposition
            URHO3D_LOGINFOF("GameNetwork() - UpdateControlAck : node=%u stamp=%u : reconciliation position server=%F,%F client=%F,%F delta=%F,%F !",
                            node->GetID(), clientstamp, src.GetReceivedControl().physics_.positionx_, src.GetReceivedControl().physics_.positiony_,
                            dest.sendedControls_.Front().physics_.positionx_, dest.sendedControls_.Front().physics_.positiony_, dx, dy);

//            for (List<ObjectControl>::Iterator it = dest.sendedControls_.Begin(); it != dest.sendedControls_.End(); ++it)
//            {
//                ObjectControl& sendedcontrol = *it;
//                sendedcontrol.physics_.positionx_ -= dx;
//                sendedcontrol.physics_.positiony_ -= dy;
//            }

            dest.preparedControl_.physics_.positionx_ -= dx;
            dest.preparedControl_.physics_.positiony_ -= dy;
            bool physicOk = GameHelpers::SetPhysicProperties(node, &dest.preparedControl_.physics_, false, false);
        }

        dest.sendedControls_.PopFront();
    }
#endif

    return true;
}

bool GameNetwork::UpdateControl(ObjectControlInfo& dest, const ObjectControlInfo& src, bool setposition)
{
    if (dest.active_ && (!dest.node_ || (dest.node_->isPoolNode_ && dest.node_->isInPool_)))
    {
        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u servernodeid=%u clientnodeid=%u ... node restored ! desactive objectcontrol",
                        dest.node_.Get(), dest.serverNodeID_, dest.clientNodeID_);

        RemoveServerObject(dest);

        return false;
    }

    GOC_Controller* controller = 0;

    Node* node = dest.node_;
    if (node)
    {
        controller = node->GetDerivedComponent<GOC_Controller>();

        // Check change enable
        // 22/01/2024 : temporary patch to solve visibility problem on network for client nodes (when a netplayer goes away from dropped item, the item disappears on the remote hosts (server/client))
        // but it's not resolve for controllable client nodes
        // TODO : prepares to remove "controller" in the following condition because it's not appropriated for all cases.
        bool enable = src.IsEnable();
        if (controller && dest.active_ && enable != dest.IsEnable())
        {
            if (!node->GetVar(GOA::ISDEAD).GetBool())
            {
                SetEnableObject(src.IsEnable(), dest, node);
            }
//            else
//                URHO3D_LOGERRORF("GameNetwork() - UpdateControl : node=%u ... wait DESTROY animation state !", node->GetID());
        }
        // Check change holder
        if (dest.GetPreparedControl().holderinfo_.id_ != src.GetReceivedControl().holderinfo_.id_)
        {
            GOC_PhysicRope* grapin = node->GetComponent<GOC_PhysicRope>();
            if (grapin)
            {
                const ObjectControl& control = src.GetReceivedControl();

                if (control.holderinfo_.id_ == M_MIN_UNSIGNED || control.holderinfo_.id_ == M_MAX_UNSIGNED)
                {
                    URHO3D_LOGERRORF("GameNetwork() - UpdateControl : node=%u ... grapin holder src=%u dest=%u ... Release !", node->GetID(), control.holderinfo_.id_, dest.GetPreparedControl().holderinfo_.id_);

                    grapin->Detach();
                    grapin->Release(2.f);
                    if (!grapin->IsAnchorOnMap())
                        grapin->BreakContact();
                }
                else
                {
                    Node* holder = GameContext::Get().rootScene_->GetNode(control.holderinfo_.id_);
                    if (holder)
                    {
                        holder->SetWorldPosition2D(Vector2(control.holderinfo_.point1x_, control.holderinfo_.point1y_));
                        grapin->SetAttachedNode(holder);
                        bool ok = grapin->AttachOnRoof(Vector2(control.holderinfo_.point2x_, control.holderinfo_.point2y_), control.holderinfo_.rot2_);
                        URHO3D_LOGERRORF("GameNetwork() - UpdateControl : node=%u ... grapin holder src=%u dest=%u ... AttachOnRoof=%s !",
                                        node->GetID(), control.holderinfo_.id_, dest.GetPreparedControl().holderinfo_.id_, ok?"true":"false");
                    }
                    else
                    {
                        URHO3D_LOGERRORF("GameNetwork() - UpdateControl : node=%u ... grapin holder src=%u dest=%u ... No Holder Node !", node->GetID(), control.holderinfo_.id_, dest.GetPreparedControl().holderinfo_.id_);
                        grapin->Release(2.f);
                    }
                }

                dest.GetPreparedControl().holderinfo_.id_ = control.holderinfo_.id_;
            }
        }
    }

    // copy the temporary object control in receivedcontrol
    dest.BackupReceivedControl();
    src.CopyReceivedControlTo(dest.GetReceivedControl());

    // Check if Server Object is Disable or Dead
    if (!src.IsEnable())
    {
        if (node && src.GetReceivedControl().states_.totalDpsReceived_ == -1.f)
        {
            URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... dead !", node->GetID());

            // the server id is not setted, so an unfinished spawncontrol must be removed !
            if (!dest.serverNodeID_)
                RemoveSpawnControl(dest.clientId_, src.GetReceivedControl().states_.spawnid_);

            node->SendEvent(WORLD_ENTITYDESTROY);
            GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
            if (destroyer)
                destroyer->Reset(false);
        }

//        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... no enable !", node->GetID());
        return false;
    }

    // reactive netplayer
    if (node && node->HasTag(PLAYERTAG) && src.IsEnable())
    {
        if (node->GetVar(GOA::ISDEAD).GetBool())
        {
            if (clientMode_ && src.GetReceivedControl().states_.totalDpsReceived_ == 0)
            {
                URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u reactive !", node->GetID());

                GOC_Life* goclife = node->GetComponent<GOC_Life>();
                if (goclife)
                    goclife->Reset();

                GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
                if (destroyer)
                {
                    destroyer->SetEnablePositionUpdate(true);
                    destroyer->SetEnableUnstuck(false);
                    destroyer->Reset(true);
                }

                node->ApplyAttributes();
                SetEnableObject(true, dest, node);

                node->SendEvent(WORLD_ENTITYCREATE);

                // Spawn Effect when Players Appear
                Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
                GameHelpers::SpawnParticleEffect(node->GetContext(), ParticuleEffect_[PE_LIFEFLAME], drawable->GetLayer(), drawable->GetViewMask(), node->GetWorldPosition2D(), 0.f, 2.f, true, 3.f, Color::BLUE, LOCAL);
            }
        }
        else if (!node->IsEnabled())
        {
            GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
            if (destroyer && destroyer->GetCurrentMap() && destroyer->GetCurrentMap()->IsVisible())
            {
                URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u map is visible => reactive node !", node->GetID());
                SetEnableObject(true, dest, node);
            }
        }
    }

    if (!node || !dest.active_)
        return false;

    /// Client : Update Life from received Server Authoritative State
    GOC_Life* goclife = node->GetComponent<GOC_Life>();
    if (clientMode_ && goclife)
    {
//        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u receive TDPS=%f !", node->GetID(), src.GetReceivedControl().states_.totalDpsReceived_);

        float deltatdps = src.GetReceivedControl().states_.totalDpsReceived_ - goclife->GetTotalEnergyLost();
        if (Abs(deltatdps) > 0.01f)
        {
//            URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u Delta TDPS %f (%f - %f)!",
//                            node->GetID(), deltatdps, dest.GetReceivedControl().states_.totalDpsReceived_, goclife->GetTotalEnergyLost());

            goclife->SetTotalEnergyLost(src.GetReceivedControl().states_.totalDpsReceived_);
        }
    }

    GOC_Animator2D* gocanimator = node->GetComponent<GOC_Animator2D>();
    GOC_Move2D* move2D = node->GetComponent<GOC_Move2D>();

    ObjectControl& objectControl = serverMode_ ? dest.GetPreparedControl() : dest.GetReceivedControl();

    /// Server : an client states update => Check States from client for preventing cheating
    if (serverMode_)
    {
        // Backup current prepared control
        ObjectControl prevcontrol;
        dest.CopyPreparedControlTo(prevcontrol);

        // copy the received to prepared control (get the client stamp for client reconciliation)
        dest.CopyReceivedControlTo(objectControl);
    #ifdef ACTIVE_NETWORK_DEBUGSERVER_RECEIVE
        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u stamp=%u prevstamp=%u : check position new=%F,%F prev=%F,%F ...",
                        node->GetID(), objectControl.states_.stamp_, prevcontrol.states_.stamp_, objectControl.physics_.positionx_,
                        objectControl.physics_.positiony_, prevcontrol.physics_.positionx_, prevcontrol.physics_.positiony_);
    #endif
        // check position cheating
//        if (Abs(objectControl.physics_.positionx_ - prevcontrol.physics_.positionx_) > 1.f ||
//            Abs(objectControl.physics_.positiony_ - prevcontrol.physics_.positiony_) > 1.f)
//        {
//            URHO3D_LOGERRORF("GameNetwork() - UpdateControl : node=%u stamp=%u prevstamp=%u : check position new=%F,%F prev=%F,%F delta>1 => cheating ! restore position",
//                        node->GetID(), objectControl.states_.stamp_, prevcontrol.states_.stamp_, objectControl.physics_.positionx_,
//                        objectControl.physics_.positiony_, prevcontrol.physics_.positionx_, prevcontrol.physics_.positiony_);
//
//            // restore previous position
//            objectControl.physics_.positionx_ = prevcontrol.physics_.positionx_;
//            objectControl.physics_.positiony_ = prevcontrol.physics_.positiony_;
//        }
    }

    /// Server and Client no local entity : Apply the received states
//    if (node->IsEnabled())
    {
//        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... ", node->GetID());
        // Check if the node is mounted, if an mountnode is ok
        if (controller)
            controller->CheckMountNode();

        // Update Avatar
        if (controller && objectControl.states_.type_)
        {
            if (controller->ChangeAvatarOrEntity(objectControl.states_.type_, objectControl.states_.entityid_))
                if (clientMode_)
                    GOC_Inventory::NetClientSetEquipment(node, node->GetID());
        }

        // Update Buttons
        if (controller && objectControl.states_.buttons_ != controller->control_.buttons_)
        {
            if (move2D && move2D->IsPhysicEnable())
            {
                controller->Update(objectControl.states_.buttons_, true);
            }
            else
            {
                controller->control_.buttons_ = objectControl.states_.buttons_;
                node->SetVar(GOA::BUTTONS, objectControl.states_.buttons_);
                node->SendEvent(GOC_CONTROLUPDATE);
            }
        }

        const bool isMounted = node->GetVar(GOA::ISMOUNTEDON).GetUInt();
        // Update Physics only if not mounted
        if (setposition && !isMounted)
            bool physicOk = GameHelpers::SetPhysicProperties(node, &objectControl.physics_, false, true);

//        bool physicOk = setposition ? GameHelpers::SetPhysicProperties(node, &objectControl.physics_, controller == 0, true) : false;

        // Update Direction (only after buttons because of Move2D controlupdate)
        if (gocanimator)
        {
            gocanimator->SetDirection(Vector2(objectControl.physics_.direction_, 0.f));
        }
        else if (controller)
        {
            controller->control_.direction_ = objectControl.physics_.direction_;
        }

        // Update Animation
        if (gocanimator && !node->GetVar(GOA::ISDEAD).GetBool())
        {
            if (dest.GetPreviousReceivedControl().states_.animstateindex_ != objectControl.states_.animstateindex_ ||
                    dest.GetPreviousReceivedControl().states_.animversion_ != objectControl.states_.animversion_ ||
                    src.GetReceivedControl().HasAnimationChanged())
            {
                StringHash state = objectControl.GetAnimationState();

                // STATE_DEAD
                if (state.Value() == STATE_DEAD)
                {
                    if (gocanimator->GetStateValue().Value() != STATE_DEAD)
                    {
                        // Only Used by server when a client suicide (tips)
                        if (serverMode_ || (clientMode_ && src.GetReceivedControl().states_.totalDpsReceived_ > 0.f))
                        {
                            URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... apply dead state TDPS=%f !", node->GetID(), src.GetReceivedControl().states_.totalDpsReceived_);

                            GOC_Life* goclife = node->GetComponent<GOC_Life>();
                            if (goclife)
                            {
                                node->SetVar(GOA::ISDEAD, true);
                                goclife->ApplyAttributes();
                            }

                            gocanimator->SetNetState(state, objectControl.states_.animversion_, src.GetReceivedControl().HasAnimationChanged());
                        }
                    }
                }
                // Never directly change to STATE_HURT
                else if (state.Value() != STATE_HURT && gocanimator->GetStateValue().Value() != STATE_HURT)
                {
//                    URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... apply animstate=%s hasAnimChanged=%s ...", node->GetID(), GOS::GetStateName(state).CString(), src.GetReceivedControl().HasAnimationChanged()?"true":"false");
                    if (state.Value() == STATE_SHOOT)
                        gocanimator->SetShootTarget(Vector2(objectControl.holderinfo_.point1x_, objectControl.holderinfo_.point1y_));

                    gocanimator->SetNetState(state, objectControl.states_.animversion_, src.GetReceivedControl().HasAnimationChanged());
                }
            }
        }

        // Update Viewz
        int viewZ = objectControl.states_.viewZ_;
        if (viewZ != node->GetVar(GOA::ONVIEWZ).GetInt())
        {
            //        URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... viewz !", node->GetID());
            ViewManager::Get()->SwitchToViewZ(viewZ, node);
        }
    }

//    URHO3D_LOGINFOF("GameNetwork() - UpdateControl : node=%u ... OK !", node->GetID());

    return true;
}


/// GameNetwork Handle Receive Object Controls

static ObjectControlInfo sDumpObject_;
static ObjectCommand sObjCmd_;
static ObjectCommand sObjCmds_[256];
static PODVector<unsigned char> sReceiveStamps_;

void GameNetwork::HandlePlayServer_ReceiveControls(StringHash eventType, VariantMap& eventData)
{
    Connection* sender = static_cast<Connection*>(context_->GetEventSender());
    if (!sender)
    {
        URHO3D_LOGERROR("GameNetwork() - HandlePlayServer_ReceiveControls : connection = 0 !");
        return;
    }

    VectorBuffer& buffer = sender->GetReceivedClientObjectControlsBuffer();
    if (!buffer.GetBuffer().Size())
        return;

    buffer.Seek(0);

    // Receive the client netstatus
    {
        int clientstatus = buffer.ReadInt();
        if (clientstatus < PLAYSTATE_SYNCHRONIZING)
        {
            buffer.Clear();
//            URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveControls : clientid=%d clientstatus=%d !", serverClientInfos_[sender].clientID_, clientstatus);
            return;
        }
    }

    int senderclientid = GetClientID(sender);

    // Receive the client spawn stamp
    {
        unsigned char receivedstamp = buffer.ReadUByte();
        if (senderclientid >= receivedSpawnStamps_.Size())
        {
            receivedSpawnStamps_.Resize(senderclientid+1);
            receivedSpawnStamps_[senderclientid] = receivedstamp;
        }
        else if (IsNewStamp(receivedstamp, receivedSpawnStamps_[senderclientid]))
        {
        #ifdef ACTIVE_NETWORK_DEBUGSERVER_RECEIVE
            URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveControls : from clientid=%d newsSpawntamp=%u lastSpawnstamp=%u buffersize=%u ...", senderclientid, receivedstamp, receivedSpawnStamps_[senderclientid], buffer.GetBuffer().Size());
        #endif
            receivedSpawnStamps_[senderclientid] = receivedstamp;
        }
        else if (receivedstamp != receivedSpawnStamps_[senderclientid])
        {
            URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveControls : spawnStamps desync : received=%u lastreceived=%u !", receivedstamp, receivedSpawnStamps_[senderclientid]);
        }
    }

    if (buffer.IsEof())
    {
        buffer.Clear();
        return;
    }

#ifdef ACTIVE_NETWORK_LOGSTATS
    ClientInfo* clientinfo = GetServerClientInfo(sender);
#endif

    do
    {
        int type = buffer.ReadUByte();
        if (type == OBJECTCONTROL)
        {
#ifdef ACTIVE_NETWORK_LOGSTATS
            unsigned lastbufferpos = buffer.Tell();
#endif
            ObjectControlInfo* oinfo = 0;

#ifdef ACTIVE_SHORTHEADEROBJECTCONTROL
            int nodeclientid = buffer.ReadUByte();
            unsigned servernodeid = buffer.ReadUShort();
            unsigned clientnodeid = buffer.ReadUShort();
            if (servernodeid) servernodeid += FIRST_LOCAL_ID;
            if (clientnodeid) clientnodeid += FIRST_LOCAL_ID;
#else
            int nodeclientid = buffer.ReadInt();
            unsigned servernodeid = buffer.ReadUInt();
            unsigned clientnodeid = buffer.ReadUInt();
#endif

            // copy the data to temporary object control
            sDumpObject_.Read(buffer);

#ifdef ACTIVE_NETWORK_LOGSTATS
            tmpLogStats_.stats_[ObjCtrlReceived]++;
            tmpLogStats_.stats_[ObjCtrlBytesReceived] += (buffer.Tell() - lastbufferpos);
            if (clientinfo)
            {
                clientinfo->tmpLogStats_.stats_[ObjCtrlReceived]++;
                clientinfo->tmpLogStats_.stats_[ObjCtrlBytesReceived] += (buffer.Tell() - lastbufferpos);
            }
#endif

            if (!clientnodeid && !servernodeid)
            {
                URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveControls : server receive empty nodeids from client=%d(%d) !", senderclientid, nodeclientid);
                continue;
            }

            const unsigned spawnid = sDumpObject_.GetReceivedControl().states_.spawnid_;
            bool setposition = true;

            // no servernodeid received => check if a node has been spawned for this spawnid
            if (!servernodeid)
            {
                oinfo = GetSpawnControl(nodeclientid, spawnid);

#ifdef ACTIVE_NETWORK_DEBUGSERVER_RECEIVE_ALL
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveControls : server receive from clientid=%d servernodeid=%u clientnodeid=%u oinfo=%u active=%u enable=%u allownetspawn=%s spawnid=%u(holder=%u,stamp=%u) ...",
                                senderclientid, servernodeid, clientnodeid, oinfo, oinfo ? oinfo->active_ : 0, sDumpObject_.IsEnable(),
                                sDumpObject_.GetReceivedControl().HasNetSpawnMode() ?"true":"false", spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
#endif
                // net spawn mode allowed ?
                if (sDumpObject_.GetReceivedControl().HasNetSpawnMode())
                {
                    // no node spawned, spawn a node and Add a SpawnControl
                    if (!oinfo || !oinfo->node_ || (oinfo->node_->isPoolNode_ && oinfo->node_->isInPool_))
                    {
                        NetSpawnEntity(clientnodeid, sDumpObject_, oinfo);
                        setposition = false;
                    }
                }
                else
                {
                    if (!GameContext::Get().IsAvatarNodeID(clientnodeid))
                        URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveControls : server receive from clientid=%d servernodeid=%u clientnodeid=%u oinfo=%u active=%u enable=%u ... no spawn entity allowed ...",
                                        senderclientid, servernodeid, clientnodeid, oinfo, oinfo ? oinfo->active_ : 0, sDumpObject_.IsEnable());
                }

                // A node is spawned, set the client node id for this spawned entity.
                if (oinfo)
                {
                    LinkSpawnControl(nodeclientid, spawnid, oinfo->node_ ? oinfo->node_->GetID() : oinfo->serverNodeID_, clientnodeid, oinfo);

                    // After the remove of the spawncontrol, the server can't process this spawnid again if servernodeid=0 (loose 2x latence)
//                    RemoveSpawnControl(spawnid);
                }
            }
            else
                // servernodeid received => get server object control
            {
                oinfo = GetServerObjectControl(servernodeid);
#ifdef ACTIVE_NETWORK_DEBUGSERVER_RECEIVE_ALL
//            if (!GameContext::Get().IsAvatarNodeID(clientnodeid))
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveControls : server receive from clientid=%d servernodeid=%u clientnodeid=%u oinfo=%u active=%u enable=%u spawnid=%u(holder=%u,stamp=%u) ...",
                                senderclientid, servernodeid, clientnodeid, oinfo, oinfo ? oinfo->active_ : 0, sDumpObject_.IsEnable(), spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
#endif
                // Let the server process with this spawnid until client received and send back the servernodeid to server (gain 1x latence)
                RemoveSpawnControl(nodeclientid, spawnid);
            }

            if (!oinfo)
            {
                URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveControls : server receive from clientid=%d servernodeid=%u clientnodeid=%u spawnid=%u(holder=%u,stamp=%u) => has no object control !",
                                 senderclientid, servernodeid, clientnodeid, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
                continue;
            }

            if (oinfo->active_ && oinfo->clientId_ != senderclientid)
            {
                // URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveControls : server receive from clientid=%d servernodeid=%u clientnodeid=%u with nodeclientid=%d != senderclientid=%d !",
                //                  senderclientid, servernodeid, clientnodeid, oinfo->clientId_, senderclientid);
                continue;
            }

            ObjectControlInfo& objectControlInfo = *oinfo;

            if (objectControlInfo.IsEnable() != sDumpObject_.IsEnable())
            {
                // check clientnodeid
                if (sDumpObject_.IsEnable() && clientnodeid && clientnodeid != objectControlInfo.clientNodeID_)
                    objectControlInfo.clientNodeID_ = clientnodeid;
            }

            // Update Components with received datas
            if (UpdateControl(objectControlInfo, sDumpObject_, setposition))
            {
#if defined(ACTIVE_NETWORK_DEBUGSERVER_RECEIVE) || defined(ACTIVE_NETWORK_DEBUGSERVER_RECEIVE_ALL)
//            if (!GameContext::Get().IsAvatarNodeID(clientnodeid))
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveControls : nodeclientid=%d servernodeid=%u clientnodeid=%u oinfo=%u spawnid=%u(holder=%u,stamp=%u) node=%s(%u) ... updated !",
                                objectControlInfo.clientId_, objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_,
                                oinfo, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid),
                                objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null",
                                objectControlInfo.node_ ? objectControlInfo.node_->GetID() : 0);
#endif
            }
        }
    }
    while (!buffer.IsEof());

//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveControls : stamp=%u ... numObjects=%u/%u size=%u fromclientid=%u ",
//                     sender->GetObjectTimeStampReceived(), buffer.GetSize(), senderclientid);

    buffer.Clear();

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif
}

void GameNetwork::HandlePlayClient_ReceiveServerControls(StringHash eventType, VariantMap& eventData)
{
    VectorBuffer& buffer = clientSideConnection_->GetReceivedServerObjectControlsBuffer();
    if (!buffer.GetBuffer().Size())
        return;

    if (gameStatus_ < PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS)
    {
        buffer.Clear();
        return;
    }

    buffer.Seek(0);

//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveServerControls : ... buffer.size=%u stamp=%u", buffer.GetSize(), GetConnection()->GetObjectTimeStampReceived());

    // Receive the client netstatus
    int serverstatus = buffer.ReadInt();
    if (serverstatus < PLAYSTATE_READY)
    {
        buffer.Clear();
        return;
    }

    // Receive all the spawn stamps of clients and server
    sReceiveStamps_ = buffer.ReadBuffer();
    for (unsigned i=0; i < sReceiveStamps_.Size(); i++)
    {
        if (i >= receivedSpawnStamps_.Size())
            receivedSpawnStamps_.Push(sReceiveStamps_[i]);
        else if (sReceiveStamps_[i] > receivedSpawnStamps_[i])
            receivedSpawnStamps_[i] = sReceiveStamps_[i];
        /// TODO case overflow
    }

    if (buffer.IsEof())
    {
        buffer.Clear();
        return;
    }

    // Receive the object controls
    do
    {
        int type = buffer.ReadUByte();
        if (type == OBJECTCONTROL)
        {
#ifdef ACTIVE_NETWORK_LOGSTATS
            unsigned lastbufferpos = buffer.Tell();
#endif

#ifdef ACTIVE_SHORTHEADEROBJECTCONTROL
            int nodeclientid = buffer.ReadUByte();
            unsigned servernodeid = buffer.ReadUShort();
            unsigned clientnodeid = buffer.ReadUShort();
            if (servernodeid) servernodeid += FIRST_LOCAL_ID;
            if (clientnodeid) clientnodeid += FIRST_LOCAL_ID;
#else
            int nodeclientid = buffer.ReadInt();
            unsigned servernodeid = buffer.ReadUInt();
            unsigned clientnodeid = buffer.ReadUInt();
#endif

            // copy the data to temporary object control
            sDumpObject_.Read(buffer);

#ifdef ACTIVE_NETWORK_LOGSTATS
            tmpLogStats_.stats_[ObjCtrlReceived]++;
            tmpLogStats_.stats_[ObjCtrlBytesReceived] += (buffer.Tell() - lastbufferpos);
#endif

            if (servernodeid < 2)
                continue;

            ObjectControlInfo* oinfo = 0;
            const unsigned spawnid = sDumpObject_.GetReceivedControl().states_.spawnid_;
            bool setposition = true;

            // check for avatar nodeid
            bool avatarnode = GameContext::Get().IsAvatarNodeID(servernodeid, nodeclientid);
            if (avatarnode)
            {
                oinfo = &GetOrCreateServerObjectControl(servernodeid, servernodeid, nodeclientid);
//                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveServerControls : avatarnode clientid=%d nodeclientid=%d servernodeid=%u clientnodeid=%u(%u) oinfo=%u... spawnid=%u(holder=%u,stamp=%u) ...",
//                                clientID_, nodeclientid, servernodeid, clientnodeid, oinfo ? oinfo->clientNodeID_:0, oinfo, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
            }
            else
            {
                // check if a node has been spawned for this spawnid
                oinfo = GetSpawnControl(nodeclientid, spawnid);

                const bool spawncontrol = oinfo != 0;

                // no spawncontrol, get the current server object control for the server nodeid
                if (!oinfo)
                    oinfo = GetServerObjectControl(servernodeid);

                // net spawn mode allowed ?
                if ((!oinfo || !oinfo->active_) && sDumpObject_.GetReceivedControl().HasNetSpawnMode() && sDumpObject_.IsEnable())
                {
                    ShortIntVector2 mpoint;
                    World2D::GetWorldInfo()->Convert2WorldMapPoint(sDumpObject_.GetReceivedControl().physics_.positionx_, sDumpObject_.GetReceivedControl().physics_.positiony_, mpoint);
                    Map* map = World2D::GetAvailableMapAt(mpoint);
                    if (map)
                    {
//                        URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveServerControls : clientid=%d nodeclientid=%d servernodeid=%u oinfo=%u spawnid=%u(holder=%u,stamp=%u) spawncontrol=%s ... spawn entity ...",
//                                        clientID_, nodeclientid, servernodeid, oinfo, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid), spawncontrol?"true":"false");

                        // entity to instantiate
                        NetSpawnEntity(servernodeid, sDumpObject_, oinfo);
                        setposition = false;
                    }
                }

                if (oinfo && oinfo->node_)
                {
                    // A node is spawned, set the server node id for this spawned entity.
                    LinkSpawnControl(nodeclientid, spawnid, servernodeid, oinfo->node_->GetID(), oinfo);
                }

                // A node is spawned, and the server return the clientnodeid too, so we can remove the spawncontrol
                if (oinfo && clientnodeid)
                    RemoveSpawnControl(nodeclientid, spawnid);
            }

#ifdef ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE_ALL
//            if (!avatarnode)
            URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveServerControls : clientid=%d nodeclientid=%d servernodeid=%u clientnodeid=%u(%u) oinfo=%u... spawnid=%u(holder=%u,stamp=%u) ...",
                            clientID_, nodeclientid, servernodeid, clientnodeid, oinfo ? oinfo->clientNodeID_:0, oinfo, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
#endif

            if (!oinfo)
            {
//                URHO3D_LOGERRORF("GameNetwork() - HandlePlayClient_ReceiveServerControls : clientid=%d nodeclientid=%d servernodeid=%u clientnodeid=%u(%u) oinfo=%u... spawnid=%u(holder=%u,stamp=%u) ...",
//                            clientID_, nodeclientid, servernodeid, clientnodeid, oinfo ? oinfo->clientNodeID_:0, oinfo, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
                continue;
            }

            ObjectControlInfo& objectControlInfo = *oinfo;

            // instantiate the avatar
            if (avatarnode && !objectControlInfo.node_)
            {
                // TODO : allowHotAcces_ Logic ...
                const bool allowHotAcces_ = false;
                if (sDumpObject_.IsEnable() || allowHotAcces_)
                {
                    objectControlInfo.serverNodeID_ = servernodeid;
                    objectControlInfo.clientId_ = nodeclientid;
                    // copy the temporary object control in receivedcontrol before add object
                    sDumpObject_.CopyReceivedControlTo(objectControlInfo.GetReceivedControl());
                    //objectControlInfo.SetEnable(true);
                    bool ok = NetAddEntity(objectControlInfo);
                    if (ok)
                    {
                        NetPlayerInfo netplayerinfo(objectControlInfo.node_);
                        if (!netPlayersInfos_.Contains(netplayerinfo))
                            netPlayersInfos_.Push(netplayerinfo);
                    }
//#if defined(ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE) || defined(ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE_ALL)
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveServerControls : clientid=%d nodeclientid=%u servernodeid=%u clientnodeid=%u node=%s(%u) spawnid=%u(holder=%u,stamp=%u) ... instantiate entity !",
                                    clientID_, objectControlInfo.clientId_, objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_,
                                    objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null",
                                    objectControlInfo.node_ ? objectControlInfo.node_->GetID() : 0, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
//#endif
                }
            }

            // Here it's always an server entity : update the new state
            if (UpdateControl(objectControlInfo, sDumpObject_, setposition))
            {
//                if (!avatarnode)
#if defined(ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE) || defined(ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE_ALL)
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveServerControls : clientid=%d nodeclientid=%u servernodeid=%u clientnodeid=%u node=%s(%u) spawnid=%u(holder=%u,stamp=%u) ... updated !",
                                clientID_, objectControlInfo.clientId_, objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_,
                                objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null",
                                objectControlInfo.node_ ? objectControlInfo.node_->GetID() : 0,
                                spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
#endif
            }
        }
    }
    while (!buffer.IsEof());

    buffer.Clear();

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif
}

void GameNetwork::HandlePlayClient_ReceiveClientControls(StringHash eventType, VariantMap& eventData)
{
    VectorBuffer& buffer = GetConnection()->GetReceivedClientObjectControlsBuffer();
    buffer.Seek(0);

//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveClientControls : clientid=%d servernodeid=%u nodeclientid=%d stamp=%u ...", clientID_, servernodeid, objectControlInfo.clientId_, objectControlInfo.GetReceivedControl().states_.stamp_);

//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveClientControls : ... buffer.size=%u stamp=%u", buffer.GetSize(), GetConnection()->GetObjectTimeStampReceived());

    // Receive Client Object Controls
    do
    {
#ifdef ACTIVE_NETWORK_LOGSTATS
        unsigned lastbufferpos = buffer.Tell();
#endif

        int type = buffer.ReadUByte();
        if (type == OBJECTCONTROL)
        {
            ObjectControlInfo* oinfo = 0;

    #ifdef ACTIVE_SHORTHEADEROBJECTCONTROL
            int nodeclientid = buffer.ReadUByte();
            unsigned servernodeid = buffer.ReadUShort();
            unsigned clientnodeid = buffer.ReadUShort();
            if (servernodeid) servernodeid += FIRST_LOCAL_ID;
            if (clientnodeid) clientnodeid += FIRST_LOCAL_ID;
    #else
            int nodeclientid = buffer.ReadInt();
            unsigned servernodeid = buffer.ReadUInt();
            unsigned clientnodeid = buffer.ReadUInt();
    #endif

            // copy the data to temporary object control
            sDumpObject_.ReadAck(buffer);

    #ifdef ACTIVE_NETWORK_LOGSTATS
            tmpLogStats_.stats_[ObjCtrlAckReceived]++;
            tmpLogStats_.stats_[ObjCtrlReceived]++;
            tmpLogStats_.stats_[ObjCtrlBytesReceived] += (buffer.Tell() - lastbufferpos);
    #endif

            if (servernodeid < 2)
                continue;

            const unsigned spawnid = sDumpObject_.GetReceivedControl().states_.spawnid_;

            if (GameContext::Get().IsAvatarNodeID(servernodeid, clientID_))
            {
//                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveClientControls : player nodeid=%u ...", servernodeid);
                if (gameStatus_ == PLAYSTATE_STARTGAME && sDumpObject_.IsEnable())
                {
                    // receive a player respawn
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveClientControls : nodeid=%u player restart !", servernodeid);
                    SetGameStatus(PLAYSTATE_RUNNING, true);
                }
            }

            oinfo = GetOrCreateClientObjectControl(servernodeid, clientnodeid);

    #ifdef ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE_ALL
    //        if (!GameContext::Get().IsAvatarNodeID(servernodeid))
            URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveClientControls : clientid=%d servernodeid=%u clientnodeid=%u oinfo=%u... spawnid=%u(holder=%u,stamp=%u) ...",
                            clientID_, servernodeid, clientnodeid, oinfo, spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
    #endif

            if (!oinfo)
                continue;

            RemoveSpawnControl(clientID_, spawnid);

            // Update with received control from server (info like totaldpsreceived)
            if (UpdateControlAck(*oinfo, sDumpObject_))
            {
    #if defined(ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE) || defined(ACTIVE_NETWORK_DEBUGCLIENT_RECEIVE_ALL)
    //            if (!GameContext::Get().IsAvatarNodeID(servernodeid))
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveClientControls : clientid=%d nodeclientid=%u servernodeid=%u clientnodeid=%u node=%s(%u) spawnid=%u(holder=%u,stamp=%u) ... updated !",
                                clientID_, oinfo->clientId_, oinfo->serverNodeID_, oinfo->clientNodeID_,
                                oinfo->node_ ? oinfo->node_->GetName().CString() : "null",
                                oinfo->node_ ? oinfo->node_->GetID() : 0,
                                spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
    #endif
            }
        }
    }
    while (!buffer.IsEof());

    buffer.Clear();

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif
}


void GameNetwork::HandlePlayServer_ReceiveCommands(StringHash eventType, VariantMap& eventData)
{
    Connection* connection = static_cast<Connection*>(context_->GetEventSender());
    if (!connection)
    {
        URHO3D_LOGERROR("GameNetwork() - HandlePlayServer_ReceiveCommands : connection = 0 !");
        return;
    }

    ClientInfo* clientinfo = GetServerClientInfo(connection);
    if (!clientinfo)
        return;

    // New Ack for Ouput Stream : Remove the ObjectCommand Packets Reference that have been received by the peer
    if (connection->HasNewAckReceived())
    {
        const unsigned oldsize       = clientinfo->objCmdInfo_.objCmdPacketsToSend_.Size();
        const int numPacketsToKeep   = Min(oldsize, connection->GetDeltaObjectCommandPackets());
        if (numPacketsToKeep != oldsize)
        {
            const int numPacketsToRemove = Max(0, clientinfo->objCmdInfo_.objCmdPacketsToSend_.Size() - numPacketsToKeep);
            List<ObjectCommandPacket* >::Iterator jt = --clientinfo->objCmdInfo_.objCmdPacketsToSend_.End();
            for (int i=0; i < numPacketsToRemove; i++, jt--)
                objCmdPckPool_.RemoveRef(*jt);
            clientinfo->objCmdInfo_.objCmdPacketsToSend_.Resize(numPacketsToKeep);
            clientinfo->objCmdInfo_.objCmdPacketStampsToSend_.Resize(numPacketsToKeep);

            URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveCommands : receive newAck -> Clean old packets size = %u => %d(%u) ... Pool(%u/%u) ",
                            oldsize, numPacketsToKeep, objCmdInfo_.objCmdPacketsToSend_.Size(), objCmdPckPool_.FreeSize(), objCmdPckPool_.Size());
        }
    }

    VectorBuffer& buffer = connection->GetObjCmdReceivedBuffer();
    if (!buffer.GetSize())
        return;

    int clientid = GetClientID(connection);

    // Store the new packets (the extra packets are those that have been resubmitted (=lost packets))
    buffer.Seek(0);
    while (buffer.GetSize()-buffer.Tell() > 0)
    {
        // Read the Command Packet Stamp and get the good storage location
        const unsigned char stamp = buffer.ReadUByte();
        ObjectCommandPacket& packet = clientinfo->objCmdInfo_.objCmdPacketsReceived_[stamp];

        const unsigned packetsize = buffer.ReadUInt();

        // Check if the Packet is already received or applied
        if (packet.state_ >= PACKET_RECEIVED)
        {
            // same packet received, so skip.
            if (packet.buffer_.GetBuffer().Size() == packetsize)
            {
                buffer.SeekRelative(packetsize);
                continue;
            }
            else if (packet.state_ == PACKET_RECEIVED)
                URHO3D_LOGWARNINGF("GameNetwork() - HandlePlayServer_ReceiveCommands : clientid=%d Packet stamp=%u previously received with a different size but never apply (overwrite) !",
                                   clientid, stamp);
        }

        // Store the packet
        URHO3D_LOGERRORF("GameNetwork() - HandlePlayServer_ReceiveCommands : Packet Received stamp=%u size=%u", stamp, packetsize);
        packet.state_  = PACKET_RECEIVED;
        packet.buffer_.Resize(packetsize);
        buffer.Read(packet.buffer_.GetModifiableData(), packetsize);
        packet.buffer_.Seek(0);
    }
    buffer.Clear();

    // From the head, try to apply Commands in each packet to the next unreceived Packet
    unsigned char head = clientinfo->objCmdInfo_.objCmdHead_;
    int headinc = 0;
    for (;;)
    {
        ObjectCommandPacket& packet = clientinfo->objCmdInfo_.objCmdPacketsReceived_[head];

        if (packet.state_ < PACKET_RECEIVED)
            break;

        if (packet.state_ == PACKET_RECEIVED)
        {
            packet.buffer_.Seek(0);
            do
            {
                int type = packet.buffer_.ReadUByte();
                if (type == OBJECTCOMMAND)
                {
                    sObjCmd_.Read(packet.buffer_);

                #ifdef ACTIVE_NETWORK_DEBUGSERVER_OBJECTCOMMANDS
                    int cmd = sObjCmd_.cmd_[Net_ObjectCommand::P_COMMAND].GetInt();
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveCommands : try ObjectCommand cmd=%d(%s) for nodeid=%u broadcast=%s cmdclientid=%d...",
                                    cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", sObjCmd_.cmd_[Net_ObjectCommand::P_NODEID].GetUInt(), sObjCmd_.broadCast_?"true":"false", sObjCmd_.clientId_);
                #endif
                    // TODO : Do we need this condition ? because that can't apply a none broadcasted msg that come from a clientid!=0, so inactive this line
                    //if ((!sObjCmd_.broadCast_ && clientID_ == sObjCmd_.clientId_) || (sObjCmd_.broadCast_ && clientID_ != sObjCmd_.clientId_))
                    {
                #ifdef ACTIVE_NETWORK_DEBUGSERVER_OBJECTCOMMANDS
                        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveCommands : apply ObjectCommand from clientid=%d ... OK !", sObjCmd_.clientId_);
                #endif
                        // Apply ObjectCommand
                        Server_ApplyObjectCommand(sObjCmd_.clientId_, sObjCmd_.cmd_);

                        /// Relay Client BroadCasted Command
                        if (sObjCmd_.broadCast_ && sObjCmd_.clientId_)
                            PushObjectCommand(sObjCmd_.cmd_, true, sObjCmd_.clientId_);
                    }
                }
            }
            while (!packet.buffer_.IsEof());
            packet.buffer_.Clear();
            packet.state_ = PACKET_APPLIED;
        }

        head++;
        headinc++;

        // stop infinite loop.
        if (!head)
        {
            // clear the already APPLIED PACKET
            for (int i = 0; i < 256; i++)
            {
                ObjectCommandPacket& packet = clientinfo->objCmdInfo_.objCmdPacketsReceived_[i];
                packet.buffer_.Clear();
                packet.state_ = PACKET_CLEARED;
            }
            break;
        }
    }

    // Move the head
    if (headinc)
    {
        clientinfo->objCmdInfo_.objCmdHead_ = head;
        connection->UpdateObjCmdInStampAck(headinc);
    #ifdef ACTIVE_NETWORK_DEBUGSERVER_OBJECTCOMMANDS
        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_ReceiveCommands : ... clientid=%d move Head=%u headinc=%d objCmdInStampAck=%u", clientid, head, headinc, connection->GetObjCmdInStampAck());
    #endif
    }

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif
}

void GameNetwork::HandlePlayClient_ReceiveCommands(StringHash eventType, VariantMap& eventData)
{
    // New Ack for Ouput Stream : Remove the ObjectCommand Packets Reference that have been received by the peer
    if (clientSideConnection_->HasNewAckReceived())
    {
        const unsigned oldsize       = objCmdInfo_.objCmdPacketsToSend_.Size();
        const int numPacketsToKeep   = Min(oldsize, clientSideConnection_->GetDeltaObjectCommandPackets());
        if (numPacketsToKeep != oldsize)
        {
            const int numPacketsToRemove = Max(0, objCmdInfo_.objCmdPacketsToSend_.Size() - numPacketsToKeep);
            List<ObjectCommandPacket* >::Iterator jt = --objCmdInfo_.objCmdPacketsToSend_.End();
            for (int i=0; i < numPacketsToRemove; i++, jt--)
                objCmdPckPool_.RemoveRef(*jt);
            objCmdInfo_.objCmdPacketsToSend_.Resize(numPacketsToKeep);
            objCmdInfo_.objCmdPacketStampsToSend_.Resize(numPacketsToKeep);

            URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveCommands : receive newAck -> Clean old packets size = %u => %d(%u) ... Pool(%u/%u) ",
                        oldsize, numPacketsToKeep, objCmdInfo_.objCmdPacketsToSend_.Size(), objCmdPckPool_.FreeSize(), objCmdPckPool_.Size());
        }
    }

    VectorBuffer& buffer = clientSideConnection_->GetObjCmdReceivedBuffer();
    if (!buffer.GetSize())
    {
//        URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_ReceiveCommands : ... buffer empty !");
        return;
    }

    URHO3D_LOGERRORF("GameNetwork() - HandlePlayClient_ReceiveCommands : ...");

    // Store the new packets (the extra packets are those that have been resubmitted (=lost packets))
    buffer.Seek(0);
    while (buffer.GetSize()-buffer.Tell() > 0)
    {
        // Read the Command Packet Stamp and get the good storage location
        unsigned char stamp = buffer.ReadUByte();
        ObjectCommandPacket& packet = objCmdInfo_.objCmdPacketsReceived_[stamp];
        packet.stamp_ = stamp;

        unsigned packetsize = buffer.ReadUInt();

        // Check if the Packet is already received or applied
        if (packet.state_ >= PACKET_RECEIVED)
        {
            // same packet received or already applied so skip.
            if (packet.buffer_.GetBuffer().Size() == packetsize)
            {
                URHO3D_LOGWARNINGF("GameNetwork() - HandlePlayClient_ReceiveCommands : Packet stamp=%u head=%u size=%u ... Packet already received ... skip !", stamp, objCmdInfo_.objCmdHead_, packetsize);
                buffer.SeekRelative(packetsize);
                continue;
            }
            else if (packet.state_ == PACKET_RECEIVED)
            {
                URHO3D_LOGWARNINGF("GameNetwork() - HandlePlayClient_ReceiveCommands : Packet stamp=%u previously received with a different size but never apply (overwrite) !", stamp);
            }
        }

        // Store the packet
        URHO3D_LOGERRORF("GameNetwork() - HandlePlayClient_ReceiveCommands : Packet Received stamp=%u head=%u size=%u", stamp, objCmdInfo_.objCmdHead_, packetsize);
        packet.state_  = PACKET_RECEIVED;
        if (packetsize)
        {
            packet.buffer_.Resize(packetsize);
            buffer.Read(packet.buffer_.GetModifiableData(), packetsize);
            packet.buffer_.Seek(0);
        }
    }
    buffer.Clear();

    // From the head, try to apply Commands in each packet to the next unreceived Packet
    unsigned char head = objCmdInfo_.objCmdHead_;
    int headinc = 0;
    bool loop = false;
    unsigned char lastCmdID = 0U;

    for (;;)
    {
        ObjectCommandPacket& packet = objCmdInfo_.objCmdPacketsReceived_[head];

        // unreceived Packet => break
        if (packet.state_ < PACKET_RECEIVED)
            break;

        if (packet.state_ == PACKET_RECEIVED)
        {
            packet.buffer_.Seek(0);

            do
            {
                int type = packet.buffer_.ReadUByte();
                if (type == OBJECTCOMMAND)
                {
                    // store Received objectCmd in the good index
                    unsigned char cmdid = packet.buffer_.ReadUByte();
                    ObjectCommand& objCmd = sObjCmds_[cmdid];

                    objCmd.Read(packet.buffer_);
                    objCmd.state_ = OBJCMD_RECEIVED;

                    if (lastCmdID < cmdid)
                        lastCmdID = cmdid;

                #ifdef ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS
                    int cmd = objCmd.cmd_[Net_ObjectCommand::P_COMMAND].GetInt();
                    URHO3D_LOGERRORF("GameNetwork() - HandlePlayClient_ReceiveCommands : packet stamp=%u receive cmdid=%u lastcmdid=%u cmd=%d(%s) broadcast=%s cmdclientid=%d ...",
                                    packet.stamp_, cmdid, lastCmdID, cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", objCmd.broadCast_?"true":"false", objCmd.clientId_);
                #endif
                }
            }
            while (!packet.buffer_.IsEof());
            packet.buffer_.Clear();
            packet.state_ = PACKET_APPLIED;

            // Apply All Received ObjectCommand in Order
            for (int cmdid = 0; cmdid <= lastCmdID; cmdid++)
            {
                ObjectCommand& objCmd = sObjCmds_[cmdid];

                if (objCmd.state_ == OBJCMD_CLEARED)
                    break;

            #ifdef ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS
                int cmd = objCmd.cmd_[Net_ObjectCommand::P_COMMAND].GetInt();
                URHO3D_LOGERRORF("GameNetwork() - HandlePlayClient_ReceiveCommands : clientID_=%d cmdid=%u/%u try ObjectCommand cmd=%d(%s) for nodeid=%u broadcast=%s cmdclientid=%d...",
                                clientID_, cmdid, lastCmdID, cmd, cmd < MAX_NETCOMMAND ? netCommandNames[cmd] : "unknown", objCmd.cmd_[Net_ObjectCommand::P_NODEID].GetUInt(),
                                 objCmd.broadCast_?"true":"false", objCmd.clientId_);
             #endif

                // Apply ObjectCommand
                Client_ApplyObjectCommand(objCmd);
                objCmd.state_ = OBJCMD_APPLIED;
            }
        }

        head++;
        headinc++;

        // stop infinite loop.
        if (!head)
        {
            // clear the already APPLIED PACKET
            for (int i = 0; i < 256; i++)
            {
                ObjectCommandPacket& packet = objCmdInfo_.objCmdPacketsReceived_[i];
                packet.buffer_.Clear();
                packet.state_ = PACKET_CLEARED;
            }
            break;
        }
    }

    // Clean All Received ObjectCommand in Order
    for (int cmdid = 0; cmdid <= lastCmdID; cmdid++)
        sObjCmds_[cmdid].state_ = OBJCMD_CLEARED;

    // Move the head
    if (headinc)
    {
        objCmdInfo_.objCmdHead_ = head;
        clientSideConnection_->UpdateObjCmdInStampAck(headinc);
    #ifdef ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS
        URHO3D_LOGERRORF("GameNetwork() - HandlePlayClient_ReceiveCommands : ... move Head=%u headinc=%d objCmdInStampAck=%u", head, headinc, clientSideConnection_->GetObjCmdInStampAck());
    #endif
    }

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif
}

/// GameNetwork Handle Network Update / Prepare Object Controls

unsigned objCtrlSentCounter_ = 0;

void GameNetwork::HandlePlayServer_NetworkUpdate(StringHash eventType, VariantMap& eventData)
{
    // URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate ... gameStatus=%s(%d) ...", gameStatusNames[gameStatus_], gameStatus_);

    /// Remove players from empty connection
    {
        HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin();
        while (it != serverClientInfos_.End())
        {
            ClientInfo& clientInfo = it->second_;

            if (!clientInfo.connection_ || !clientInfo.connection_->IsConnected() || clientInfo.connection_->IsConnectPending())
            {
                Server_RemovePlayers(clientInfo);
                CleanObjectCommandInfo(clientInfo.objCmdInfo_);
                it = serverClientInfos_.Erase(it);
                continue;
            }

            it++;
        }
    }

    /// Allocate players ids and Set Scene Replication if used
    if (gameStatus_ >= PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS)
    {
        for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
        {
            ClientInfo& clientInfo = it->second_;
            if (!clientInfo.connection_)
            {
                URHO3D_LOGWARNING("GameNetwork() - HandlePlayServer_NetworkUpdate : allocate players ... noconnection=0 !");
                continue;
            }
            if (clientInfo.gameStatus_ < PLAYSTATE_SYNCHRONIZING)
            {
                allClientsRunning_ = allClientsSynchronized_ = false;
                GameContext::Get().AllowUpdate_ = false;
            }
            if (clientInfo.NeedAllocatePlayers() && clientInfo.gameStatus_ >= PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS && clientInfo.gameStatus_ < PLAYSTATE_RUNNING)
            {
                // Create Players ids for the connections who dont have the good requestPlayers
                Server_AllocatePlayers(clientInfo);
                Server_SendSnap(clientInfo);
            }
#ifdef SCENE_REPLICATION_ENABLE
            else if (clientInfo.gameStatus_ == PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES)
            {
                if (!clientInfo.connection_->GetScene())
                {
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : set scene to connection=%u ... ", clientInfo.connection_.Get());
                    clientInfo.connection_->SetScene(GameContext::Get().rootScene_);
                }
            }
#endif
        }
    }

    /// Check Client States and Send Game Server State if need
    if (gameStatus_ >= PLAYSTATE_READY && gameStatus_ <= PLAYSTATE_RUNNING)
    {
        if (!GameContext::Get().AllowUpdate_ && !serverClientInfos_.Size())
        {
            allClientsSynchronized_ = allClientsRunning_ = true;
            GameContext::Get().AllowUpdate_ = true;

            SetGameStatus(PLAYSTATE_RUNNING, false);

            URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : No Clients => Run !");
        }
        else
        {
            if (needSynchronization_ && !allClientsSynchronized_)
            {
                int synchronizedClients = 0;
                for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
                {
                    if (it->second_.gameStatus_ >= PLAYSTATE_SYNCHRONIZING)
                        synchronizedClients++;
                }

                if (synchronizedClients == serverClientInfos_.Size())
                {
                    allClientsSynchronized_ = true;

                    URHO3D_LOGINFOF("GameNetwork() -----------------------------------------------------------------");
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : All Clients are Synchronized !");
                    URHO3D_LOGINFOF("GameNetwork() -----------------------------------------------------------------");

                    Server_SendGameStatus(PLAYSTATE_RUNNING);
                }
            }

            int runningClients = 0;
            for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
            {
                if (it->second_.gameStatus_ == PLAYSTATE_RUNNING)
                    runningClients++;
            }

//            URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : running clients = %d/%d allClientsRunning_=%d!", runningClients, serverClientInfos_.Size(), allClientsRunning_);

            if (!allClientsRunning_ && (!needSynchronization_ || (needSynchronization_ && allClientsSynchronized_)))
            {
                if (runningClients == serverClientInfos_.Size())
                {
                    for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
                    {
                        ClientInfo& clientInfo = it->second_;
                        if (!clientInfo.playersStarted_)
                        {
                            clientInfo.StartPlayers();
                            Server_SetEnableAllActiveObjectsToClient(clientInfo);
                        }
                    }

                    allClientsRunning_ = true;
                    GameContext::Get().AllowUpdate_ = true;

                    SetGameStatus(PLAYSTATE_RUNNING, false);

                    URHO3D_LOGINFOF("GameNetwork() ------------------------------------------------------------");
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : All Clients are Running !");
                    URHO3D_LOGINFOF("GameNetwork() ------------------------------------------------------------");
                }
            }
        }

        /// Check for a restart game request
        if (GameContext::Get().AllowUpdate_)
        {
            for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
            {
                ClientInfo& clientInfo = it->second_;
                if (clientInfo.rebornRequest_ && clientInfo.gameStatus_ == PLAYSTATE_ENDGAME && clientInfo.players_.Size())
                {
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : client=%d ... Check for Restart Players ... ", clientInfo.clientID_);

                    bool allPlayersDeadOrAlive = true;
                    for (Vector<SharedPtr<Player> >::ConstIterator jt = clientInfo.players_.Begin(); jt != clientInfo.players_.End(); ++jt)
                    {
                        // if the sum of these 2 states is equal to 1 then the player is dying
                        if ((*jt)->active + (*jt)->IsStarted() == 1)
                        {
                            allPlayersDeadOrAlive = false;
                            URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : client=%d ... Check for Restart Players ... player %d is not deadOrAlive (active=%d started=%d) !",
                                            clientInfo.clientID_, (int)(jt-clientInfo.players_.Begin()), (*jt)->active, (*jt)->IsStarted());
                            break;
                        }
                    }

                    // if all the players are well dead and destroy or still alive, reborn
                    if (allPlayersDeadOrAlive)
                    {
                        Server_SetEnablePlayers(clientInfo, true);
                        Server_SetEnableAllActiveObjectsToClient(clientInfo);

                        clientInfo.rebornRequest_ = false;
                        clientInfo.gameStatus_ = PLAYSTATE_RUNNING;

                        URHO3D_LOGINFOF("GameNetwork() --------------------------------------------------------------");
                        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : client=%d Restart Players !", clientInfo.clientID_);
                        URHO3D_LOGINFOF("GameNetwork() --------------------------------------------------------------");
                    }
                }
            }
        }
    }

    /// Prepare Server&Client Object Controls Message
    if (gameStatus_ >= PLAYSTATE_READY)
    {
    //    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : Prepare ObjectControls ...");

        // Clear Buffers
        preparedServerMessageBuffer_.Clear();
        for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
            it->first_->ClearPreparedObjectControlMessageBuffers();

        // Prepare netstatus and the spawn stamps
        preparedServerMessageBuffer_.WriteInt(gameStatus_);
        preparedServerMessageBuffer_.WriteBuffer(localSpawnStamps_);

        // Write ObjectControls to Buffer
        unsigned iobj, numobjsent = 0;
        if (objCtrlSentCounter_ >= serverObjectControls_.Size())
            objCtrlSentCounter_ = 0;

        for (iobj = objCtrlSentCounter_; iobj < serverObjectControls_.Size(); iobj++)
        {
            ObjectControlInfo& objectControlInfo = serverObjectControls_[iobj];

    //        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : oinfo=%u servernodeid=%u clientnodeid=%u active=%u node=%s(%u)...",
    //                        &objectControlInfo, objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_, objectControlInfo.active_,
    //                        objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null", objectControlInfo.node_ ? objectControlInfo.node_->GetID() : 0);

            if (!objectControlInfo.active_)
                continue;

            // Server never send ObjectControl with no serverNodeID
            if (!objectControlInfo.serverNodeID_)
                continue;

            // Prepare Local Server Object Controls
            if (!PrepareControl(objectControlInfo))
                continue;

            // If clientId==0, Distribute an Server Object Control Message
            if (objectControlInfo.clientId_ == 0)
            {
    #ifdef ACTIVE_NETWORK_DEBUGSERVER_SEND_SERVEROBJECTS
                const unsigned spawnid = objectControlInfo.GetPreparedControl().states_.spawnid_;
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : server send object servernodeid=%s(%u) clientnodeid=%u spawnid=%u(holderid=%u, spawnstamp=%u) to all clients !",
                                objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null", objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_,
                                spawnid, GetSpawnHolder(spawnid), GetSpawnStamp(spawnid));
    #endif
    #ifdef ACTIVE_NETWORK_LOGSTATS
                unsigned lastbuffersize = preparedServerMessageBuffer_.GetBuffer().Size();
    #endif

                // Write an Server Object to all clients : Store in ServerObjectMessageBuffer => sended by MSG_SERVEROBJECTCONTROLS (Connection::ProcessSendServerObjectControls)
                objectControlInfo.Write(preparedServerMessageBuffer_);
                numobjsent++;

    #ifdef ACTIVE_NETWORK_LOGSTATS
                tmpLogStats_.stats_[ObjCtrlSent]++;
                tmpLogStats_.stats_[ObjCtrlBytesSent] += (preparedServerMessageBuffer_.GetBuffer().Size() - lastbuffersize);
    #endif
            }
            // If ObjectControl is Owned By a client, Distribute a Specific Message
            else
            {
                for (HashMap<Connection*, ClientInfo>::Iterator jt = serverClientInfos_.Begin(); jt != serverClientInfos_.End(); ++jt)
                {
                    Connection* connection = jt->first_;
                    if (!connection)
                        continue;

                    ClientInfo& clientinfo = jt->second_;
                    int clientid = clientinfo.clientID_;

                    // Write an Client Object to the client "owner" : for return state (like totaldpsreceived)
                    if (clientid == objectControlInfo.clientId_)
                    {
                        if (!connection->IsClientObjectControlsAllowed())
                            continue;

    #ifdef ACTIVE_NETWORK_DEBUGSERVER_SEND_CLIENTUPDATE
                        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : server send servernodeid=%s(%u) clientnodeid=%u to clientid=%d (owner) oinfo=%u stamp=%u enable=%s !",
                                        objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null", objectControlInfo.serverNodeID_,
                                        objectControlInfo.clientNodeID_, clientid, &objectControlInfo, objectControlInfo.GetPreparedControl().states_.stamp_,
                                        objectControlInfo.GetPreparedControl().IsEnabled() ? "true":"false");
    #endif
    #ifdef ACTIVE_NETWORK_LOGSTATS
                        unsigned lastbuffersize = connection->GetPreparedClientObjectControlMessageBuffer()->GetBuffer().Size();
    #endif

                        objectControlInfo.WriteAck(*connection->GetPreparedClientObjectControlMessageBuffer());
                        // never count ack has an objsent

    #ifdef ACTIVE_NETWORK_LOGSTATS
                        clientinfo.tmpLogStats_.stats_[ObjCtrlSent]++;
                        clientinfo.tmpLogStats_.stats_[ObjCtrlBytesSent] += (connection->GetPreparedClientObjectControlMessageBuffer()->GetBuffer().Size() - lastbuffersize);
    #endif
                    }

                    // Write an Client Object from a client to others clients : Store in ServerObjectMessageBuffer => sended by MSG_SERVEROBJECTCONTROLS (Connection::ProcessSendServerObjectControls)
                    else
                    {
                        if (!connection->IsServerObjectControlsAllowed())
                            continue;

    //                    if (!GameContext::Get().IsAvatarNodeID(objectControlInfo.serverNodeID_))
    #ifdef ACTIVE_NETWORK_DEBUGSERVER_SEND_CLIENTOBJECTS
                        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : server send servernodeid=%s(%u) clientnodeid=%u to clientid=%d (owned by clientid=%d) oinfo=%u enable=%s !",
                                        objectControlInfo.node_ ? objectControlInfo.node_->GetName().CString() : "null", objectControlInfo.serverNodeID_,
                                        objectControlInfo.clientNodeID_, clientid, objectControlInfo.clientId_, &objectControlInfo,
                                        objectControlInfo.GetPreparedControl().IsEnabled() ? "true":"false");
    #endif
    #ifdef ACTIVE_NETWORK_LOGSTATS
                        unsigned lastbuffersize = connection->GetPreparedSpecificServerObjectControlMessageBuffer()->GetBuffer().Size();
    #endif

                        objectControlInfo.Write(*connection->GetPreparedSpecificServerObjectControlMessageBuffer());
                        numobjsent++;

    #ifdef ACTIVE_NETWORK_LOGSTATS
                        clientinfo.tmpLogStats_.stats_[ObjCtrlSent]++;
                        clientinfo.tmpLogStats_.stats_[ObjCtrlBytesSent] += (connection->GetPreparedSpecificServerObjectControlMessageBuffer()->GetBuffer().Size() - lastbuffersize);
                        tmpLogStats_.stats_[ObjCtrlSent]++;
                        tmpLogStats_.stats_[ObjCtrlBytesSent] += (connection->GetPreparedSpecificServerObjectControlMessageBuffer()->GetBuffer().Size() - lastbuffersize);
    #endif
                    }
                }
            }

            objectControlInfo.prepared_ = false;

            if (numobjsent == NET_MAXSERVEROBJECTCONTROLTOSEND)
                break;
        }

        objCtrlSentCounter_ = iobj+1;
    }

//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : Prepare ObjectControls ... OK !");

    /// Prepare Object Commands Message
    {
//        URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : Prepare ObjectCommands ... ");

        HashMap<int, ObjectCommandPacket* > newpackets;

        for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
        {
            if (it->second_.gameStatus_ > MENUSTATE)
            {
                // Send Requested MapDatas if exist
                Server_SendWorldMaps(it->second_);
            }
        }

        // Prepare the new ObjectCommand Packets from the pool
        if (objCmdNew_.Size())
        {
//            URHO3D_LOGINFOF(" ... New Commands Size = %u ...", objCmdNew_.Size());

            // write new commands into new packets
            for (unsigned char cmdid = 0 ; cmdid < objCmdNew_.Size(); cmdid++)
            {
                ObjectCommand& cmd = *objCmdNew_[cmdid];

                if (cmd.clientId_ == 0)
                {
                    ObjectCommandPacket*& serverpacket = newpackets[0];
                    if (!serverpacket)
                        serverpacket = objCmdPckPool_.Get(false); // don't addref here

                    cmd.Write(serverpacket->buffer_, cmdid, 0);
                }
                else
                {
                    for (HashMap<Connection*, ClientInfo>::Iterator jt = serverClientInfos_.Begin(); jt != serverClientInfos_.End(); ++jt)
                    {
                        ClientInfo& clientinfo = jt->second_;

                        if (!clientinfo.objCmdInfo_.newSpecificPacketToSend_)
                            clientinfo.objCmdInfo_.newSpecificPacketToSend_ = objCmdPckPool_.Get();

                        if ((cmd.broadCast_ && cmd.clientId_ != clientinfo.clientID_) || (!cmd.broadCast_ && cmd.clientId_ == clientinfo.clientID_))
                            cmd.Write(clientinfo.objCmdInfo_.newSpecificPacketToSend_->buffer_, cmdid, cmd.clientId_);
                        else // put an empty command
                            ObjectCommand::EMPTY.Write(clientinfo.objCmdInfo_.newSpecificPacketToSend_->buffer_, cmdid, cmd.clientId_);
                    }
                }
            }
        }

        // For each ClientInfo, send all packets
        for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
        {
            Connection* connection = it->first_;
            if (!connection)
                continue;

            ClientInfo& clientinfo = it->second_;

            // if new packets, store them to clientinfo.objCmdInfo_.objCmdPacketsToSend_ for future resends
            if (objCmdNew_.Size())
            {
                int incStamp = 0;
                if (clientinfo.objCmdInfo_.newSpecificPacketToSend_)
                {
                    incStamp++;
                    clientinfo.objCmdInfo_.objCmdPacketsToSend_.PushFront(clientinfo.objCmdInfo_.newSpecificPacketToSend_);
                    clientinfo.objCmdInfo_.objCmdPacketStampsToSend_.PushFront(clientinfo.objCmdInfo_.objCmdToSendStamp_+incStamp);
                    clientinfo.objCmdInfo_.newSpecificPacketToSend_ = 0;
                }
                for (HashMap<int, ObjectCommandPacket*>::ConstIterator it = newpackets.Begin(); it != newpackets.End(); ++it)
                {
                    if (it->first_ != clientinfo.clientID_)
                    {
                        incStamp++;
                        it->second_->AddRef();
                        clientinfo.objCmdInfo_.objCmdPacketsToSend_.PushFront(it->second_);
                        clientinfo.objCmdInfo_.objCmdPacketStampsToSend_.PushFront(clientinfo.objCmdInfo_.objCmdToSendStamp_+incStamp);
                    }
                }
                if (incStamp)
                {
                    clientinfo.objCmdInfo_.objCmdToSendStamp_ += incStamp;
                    connection->UpdateObjCmdOutStamp(incStamp);
                }
            }

            bool sendpackets = clientinfo.objCmdInfo_.objCmdPacketsToSend_.Size();

            // Temporization for resending old packets
            if (sendpackets && !objCmdNew_.Size())
            {
                clientinfo.objCmdInfo_.resendPacketTimer_++;
                if (clientinfo.objCmdInfo_.resendPacketTimer_ < NET_DEFAULT_PACKETRESEND_TICK)
                    sendpackets = false;
            #ifdef ACTIVE_NETWORK_DEBUGSERVER_OBJECTCOMMANDS
                if (sendpackets)
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : objcmd=%u clientid=%d numCmdPackets=%u deltapackets=%d resend packets !",
                                clientinfo.objCmdInfo_.objCmdToSendStamp_, clientinfo.clientID_, clientinfo.objCmdInfo_.objCmdPacketsToSend_.Size(), connection->GetDeltaObjectCommandPackets());
            #endif
            }

            // write all the packets to send (new ones and the ones to resend)
            if (sendpackets)
            {
                VectorBuffer& buffer = connection->GetObjCmdSendBuffer();
                buffer.Clear();
                List<ObjectCommandPacket*>::ConstIterator jt = clientinfo.objCmdInfo_.objCmdPacketsToSend_.Begin();
                List<unsigned char>::ConstIterator kt = clientinfo.objCmdInfo_.objCmdPacketStampsToSend_.Begin();
                for (jt; jt != clientinfo.objCmdInfo_.objCmdPacketsToSend_.End(); ++jt, ++kt)
                {
                    ObjectCommandPacket* packet = *jt;
                    buffer.WriteUByte(*kt); // write the client stamp for the packet
                    buffer.WriteUInt(packet->buffer_.GetSize());
                    // write the packet buffer
                    buffer.Write(packet->buffer_.GetData(), packet->buffer_.GetSize());
                }

            #ifdef ACTIVE_NETWORK_LOGSTATS
                if (buffer.GetSize())
                {
                    tmpLogStats_.stats_[ObjCmdSent] += clientinfo.objCmdInfo_.objCmdPacketsToSend_.Size();
                    tmpLogStats_.stats_[ObjCmdBytesSent] += (buffer.GetSize());
                }
            #endif
            #ifdef ACTIVE_NETWORK_DEBUGSERVER_OBJECTCOMMANDS
                if (buffer.GetSize())
                    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate : objcmd=%u clientid=%d numCmdPackets=%u deltapackets=%d sendbuffer.Size()=%u",
                                clientinfo.objCmdInfo_.objCmdToSendStamp_, clientinfo.clientID_, clientinfo.objCmdInfo_.objCmdPacketsToSend_.Size(), connection->GetDeltaObjectCommandPackets(), buffer.GetSize());
            #endif

                clientinfo.objCmdInfo_.resendPacketTimer_ = 0;
            }
        }

        if (objCmdNew_.Size())
        {
            objCmdPool_.Restore();
            objCmdNew_.Clear();
        }
    }

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif

//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayServer_NetworkUpdate ... OK !");
}

void GameNetwork::HandlePlayClient_NetworkUpdate(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate ... gamestatus=%s(%d)", gameStatusNames[gameStatus_], gameStatus_);

    if (!clientSideConnection_)
        return;

    // Clear Buffer
    VectorBuffer& preparedClientMessageBuffer = *clientSideConnection_->GetPreparedClientObjectControlMessageBuffer();
    preparedClientMessageBuffer.Clear();

    // Prepare NetStatus
    preparedClientMessageBuffer.WriteInt(gameStatus_);

    /// Prepare Client Object Controls Message
    if (gameStatus_ >= PLAYSTATE_SYNCHRONIZING && clientSideConnection_->IsClientObjectControlsAllowed())
    {
    #ifdef ACTIVE_NETWORK_LOGSTATS
        unsigned lastbuffersize = preparedClientMessageBuffer.GetBuffer().Size();
    #endif

        // Write local spawn stamp
        preparedClientMessageBuffer.WriteUByte(localSpawnStamps_[clientID_]);

        // Write ObjectControls to Buffer
        if (clientObjectControls_.Size())
        {
            unsigned iobj, numobjsent = 0;
            if (objCtrlSentCounter_ >= clientObjectControls_.Size())
                objCtrlSentCounter_ = 0;

    //        for (Vector<ObjectControlInfo>::Iterator it=clientObjectControls_.Begin();it != clientObjectControls_.End(); ++it)
            for (iobj = objCtrlSentCounter_; iobj < clientObjectControls_.Size(); iobj++)
            {
                ObjectControlInfo& objectControlInfo = clientObjectControls_[iobj];
    //            ObjectControlInfo& objectControlInfo = *it;

    //            URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate : servernodeid=%u clientnodeid=%u nodeptr=%u active=%u iobj=%u ...",
    //                            objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_, objectControlInfo.node_.Get(), objectControlInfo.active_, iobj);

                if (!objectControlInfo.active_ || !objectControlInfo.node_)
                    continue;

                if (!PrepareControl(objectControlInfo))
                    continue;

                // For reconciliation : increment the stamp of the client object control.
                if (objectControlInfo.GetPreparedControl().states_.stamp_ == 255)
                    objectControlInfo.GetPreparedControl().states_.stamp_ = 1;
                else
                    objectControlInfo.GetPreparedControl().states_.stamp_++;

    #ifdef ACTIVE_NETWORK_DEBUGCLIENT_SEND
    //            if (!GameContext::Get().IsAvatarNodeID(objectControlInfo.serverNodeID_))
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate : clientid=%d writing client servernodeid=%u clientnodeid=%u oinfo=%u nodeptr=%u stamp=%u enable=%s!",
                                clientID_, objectControlInfo.serverNodeID_, objectControlInfo.clientNodeID_, &objectControlInfo, objectControlInfo.node_.Get(), objectControlInfo.GetPreparedControl().states_.stamp_,
                                objectControlInfo.GetPreparedControl().IsEnabled() ? "true":"false");
    #endif

                // Write to buffer
                objectControlInfo.Write(preparedClientMessageBuffer);

                // For reconciliation : Save to Sended Controls
    #ifdef ACTIVE_NETWORK_SERVERRECONCILIATION
                objectControlInfo.CopyPreparedToSendedControl();
    #endif

                objectControlInfo.prepared_ = false;

                numobjsent++;
                if (numobjsent == NET_MAXCLIENTOBJECTCONTROLTOSEND)
                    break;
            }

            objCtrlSentCounter_ = iobj+1;

    #ifdef ACTIVE_NETWORK_LOGSTATS
            tmpLogStats_.stats_[ObjCtrlSent] += numobjsent;
            tmpLogStats_.stats_[ObjCtrlBytesSent] += (preparedClientMessageBuffer.GetBuffer().Size() - lastbuffersize);
    #endif
        }
    }

    /// Prepare Object Commands Message
    {
        VectorBuffer& buffer = clientSideConnection_->GetObjCmdSendBuffer();
        buffer.Clear();

        // Prepare the new ObjectCommand Packets
        if (objCmdNew_.Size())
        {
            ObjectCommandPacket* packet = objCmdPckPool_.Get();
            for (PODVector<ObjectCommand* >::Iterator it = objCmdNew_.Begin(); it != objCmdNew_.End(); ++it)
            {
                (*it)->Write(packet->buffer_);
                objCmdPool_.RemoveRef(*it);
            }

            objCmdInfo_.objCmdPacketsToSend_.PushFront(packet);

            // update the stamp
            objCmdInfo_.objCmdToSendStamp_++;

            // set the stamp for the packet
            packet->stamp_ = objCmdInfo_.objCmdToSendStamp_;
            clientSideConnection_->UpdateObjCmdOutStamp(1);
        }

        bool sendpackets = objCmdInfo_.objCmdPacketsToSend_.Size();

        // Temporization for resending old packets
        if (sendpackets && !objCmdNew_.Size())
        {
            objCmdInfo_.resendPacketTimer_++;
            if (objCmdInfo_.resendPacketTimer_ < NET_DEFAULT_PACKETRESEND_TICK)
                sendpackets = false;
            #ifdef ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS
            if (sendpackets)
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate : objcmd=%u clientid=%d numCmdPackets=%u deltapackets=%d resend packets !",
                                objCmdInfo_.objCmdToSendStamp_, clientID_, objCmdInfo_.objCmdPacketsToSend_.Size(), clientSideConnection_->GetDeltaObjectCommandPackets());
            #endif
        }

        // Write the ObjectCommand Packets to send
        if (sendpackets)
        {
            for (List<ObjectCommandPacket* >::ConstIterator it = objCmdInfo_.objCmdPacketsToSend_.Begin(); it != objCmdInfo_.objCmdPacketsToSend_.End(); ++it)
            {
                ObjectCommandPacket* packet = *it;
                buffer.WriteUByte(packet->stamp_);
                buffer.WriteUInt(packet->buffer_.GetSize()); // write the stamp for the packet
                // write the packet buffer
                buffer.Write(packet->buffer_.GetData(), packet->buffer_.GetSize());
            }
            #ifdef ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS
            if (buffer.GetSize())
                URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate : objcmd=%u clientid=%d numCmdPackets=%u deltapackets=%d sendbuffer.Size()=%u",
                                objCmdInfo_.objCmdToSendStamp_, clientID_, objCmdInfo_.objCmdPacketsToSend_.Size(), clientSideConnection_->GetDeltaObjectCommandPackets(), buffer.GetSize());
            #endif

            objCmdInfo_.resendPacketTimer_ = 0;
        }

        if (objCmdNew_.Size())
            objCmdNew_.Clear();

//    #ifdef ACTIVE_NETWORK_DEBUGCLIENT_OBJECTCOMMANDS
//        if (buffer.GetSize())
//            URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate : numCmdPackets=%u objcmd=%u deltapackets=%d sendbuffer.Size()=%u",
//                        objCmdInfo_.objCmdPacketsToSend_.Size(), objCmdInfo_.objCmdToSendStamp_, clientSideConnection_->GetDeltaObjectCommandPackets(), buffer.GetSize());
//    #endif

    #ifdef ACTIVE_NETWORK_LOGSTATS
        if (buffer.GetSize())
        {
            tmpLogStats_.stats_[ObjCmdSent] += objCmdInfo_.objCmdPacketsToSend_.Size();
            tmpLogStats_.stats_[ObjCmdBytesSent] += (buffer.GetSize());
        }
    #endif
    }

#ifdef ACTIVE_NETWORK_LOGSTATS
    UpdateLogStats();
#endif
//    URHO3D_LOGINFOF("GameNetwork() - HandlePlayClient_NetworkUpdate ... OK !");
}


/// GameNetwork Dump

void GameNetwork::DumpClientInfos() const
{
    if (serverMode_)
    {
        URHO3D_LOGINFOF("GameNetwork() - DumpClientInfos : serverClientInfos_.Size=%u", serverClientInfos_.Size());
        for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
        {
            const ClientInfo& clientinfo = it->second_;
            Connection* connection = it->first_;

            URHO3D_LOGINFOF(" => connection=%u NbPacket(in:%d-out:%d) BytespSec(in:%F-out:%F) clientInfoConnection=%u address=%s clientid=%d gamestatus=%s(%u)",
                            connection, connection->GetPacketsInPerSec(), connection->GetPacketsOutPerSec(), connection->GetBytesInPerSec(), connection->GetBytesOutPerSec(),
                            clientinfo.connection_.Get(), clientinfo.connection_ ? clientinfo.connection_->ToString().CString() : "0",
                            clientinfo.clientID_, gameStatusNames[clientinfo.gameStatus_], clientinfo.gameStatus_);

            if (clientinfo.gameStatus_ >= PLAYSTATE_READY)
            {
                if (clientinfo.players_.Size())
                    for (Vector<SharedPtr<Player> >::ConstIterator jt = clientinfo.players_.Begin(); jt != clientinfo.players_.End(); ++jt)
                    {
                        Player* player = jt->Get();
                        if (!player)
                            continue;

                        Node* avatar = player->GetAvatar();
                        if (!avatar)
                            continue;

                        const ObjectControlInfo* cinfo = GetObjectControl(avatar->GetID());
                        URHO3D_LOGINFOF(" ==> player avatarid=%u enable=%s active=%s enable=%s",
                                        avatar->GetID(), avatar->IsEnabled() ? "true":"false",
                                        cinfo ? (cinfo->active_ ? "true":"false") : "none",
                                        cinfo ? (cinfo->IsEnable() ? "true":"false")  : "none");

                        //            player->Dump();
                    }
            }
        }
    }
    else if (clientMode_ && clientSideConnection_)
    {
        URHO3D_LOGINFOF("GameNetwork() - DumpClientInfos : connectionToServer=%u clientid=%d gamestatus=%s(%u) objcmd=%u deltapackets=%d",
                         clientSideConnection_.Get(), clientID_, gameStatusNames[gameStatus_], gameStatus_, objCmdInfo_.objCmdToSendStamp_, clientSideConnection_->GetDeltaObjectCommandPackets());

        URHO3D_LOGINFOF(" => NbPacket(in:%d-out:%d) BytespSec(in:%F-out:%F) ",
                        clientSideConnection_->GetPacketsInPerSec(), clientSideConnection_->GetPacketsOutPerSec(), clientSideConnection_->GetBytesInPerSec(), clientSideConnection_->GetBytesOutPerSec());
    }
}

void GameNetwork::DumpNetObjects() const
{
    URHO3D_LOGINFOF("GameNetwork() - DumpNetObjects : serverObjectControls_.Size=%u ", serverObjectControls_.Size());
    for (Vector<ObjectControlInfo>::ConstIterator it = serverObjectControls_.Begin(); it != serverObjectControls_.End(); ++it)
        it->Dump();

    URHO3D_LOGINFOF("GameNetwork() - DumpNetObjects : clientObjectControls_.Size=%u ", clientObjectControls_.Size());
    for (Vector<ObjectControlInfo>::ConstIterator it = clientObjectControls_.Begin(); it != clientObjectControls_.End(); ++it)
        it->Dump();

    URHO3D_LOGINFOF("GameNetwork() - DumpNetObjects : objCmdPoolFree=%u/%u objCmdPckPoolFree=%u/%u ", objCmdPool_.FreeSize(), objCmdPool_.Size(), objCmdPckPool_.FreeSize(), objCmdPckPool_.Size());
}

void GameNetwork::DumpSpawnStamps() const
{
//    const unsigned spawnStampSize = localSpawnStamps_.Size();
    const unsigned spawnStampSize = 2;
    URHO3D_LOGINFOF("GameNetwork() - DumpSpawnStamps : localSpawnStamps_.Size=%u ", localSpawnStamps_.Size());
    for (unsigned i = 0; i < spawnStampSize; i++)
        URHO3D_LOGINFOF("   client=%u localstamp=%u ", i, localSpawnStamps_[i]);
    URHO3D_LOGINFOF("GameNetwork() - DumpSpawnStamps : receivedSpawnStamps_.Size=%u ", receivedSpawnStamps_.Size());
    for (unsigned i = 0; i < spawnStampSize; i++)
        URHO3D_LOGINFOF("   client=%u receivedstamp=%u ", i, receivedSpawnStamps_[i]);
}

#ifdef ACTIVE_NETWORK_LOGSTATS
void DumpLogStatNetObject(int clientid, const LogStatNetObject& logstats)
{
    char statsBuffer[256];
    sprintf(statsBuffer, "clid:%i ObjCtrl(in:%i(%ukb/s)-out:%i(%ukb/s)) ObjCmd(in:%i(%ukb/s)-out:%i(%ukb/s))",
            clientid, logstats.stats_[ObjCtrlReceived], logstats.stats_[ObjCtrlBytesReceived]/1024,
            logstats.stats_[ObjCtrlSent], logstats.stats_[ObjCtrlBytesSent]/1024,
            logstats.stats_[ObjCmdReceived], logstats.stats_[ObjCmdBytesReceived]/1024,
            logstats.stats_[ObjCmdSent], logstats.stats_[ObjCmdBytesSent]/1024);

    URHO3D_LOGINFO(statsBuffer);
}

void GameNetwork::UpdateLogStats()
{
    if (!logStatEnable_)
        return;

    if (logStatTimer_.GetMSec(false) > 2000)
    {
        logStatTimer_.Reset();

        DumpLogStatNetObject(clientID_, logStats_);
//        for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
//        {
//            const ClientInfo& clientinfo = it->second_;
//            DumpLogStatNetObject(clientinfo.clientID_, clientinfo.logStats_);
//        }
    }

    if (logStatCountTimer_.GetMSec(false) > 1000)
    {
        logStatCountTimer_.Reset();
        tmpLogStats_.Copy(logStats_);
        tmpLogStats_.Clear();
//        for (HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
//        {
//            ClientInfo& clientinfo = it->second_;
//            clientinfo.tmpLogStats_.Copy(clientinfo.logStats_);
//            clientinfo.tmpLogStats_.Clear();
//        }
    }
}
#endif

void GameNetwork::Dump() const
{
    URHO3D_LOGINFOF("GameNetwork() - Dump : netgamestatus=%s", gameStatusNames[(int)gameStatus_]);
    DumpClientInfos();
    DumpNetObjects();
    DumpSpawnStamps();

#ifdef ACTIVE_NETWORK_LOGSTATS
    DumpLogStatNetObject(clientID_, logStats_);
    if (clientSideConnection_)
    {
        clientSideConnection_->DumpStatistics(2);
    }
    else
    {
        for (HashMap<Connection*, ClientInfo>::ConstIterator it = serverClientInfos_.Begin(); it != serverClientInfos_.End(); ++it)
        {
            const ClientInfo& clientinfo = it->second_;
            DumpLogStatNetObject(clientinfo.clientID_, clientinfo.logStats_);
            it->first_->DumpStatistics(2);
        }
    }
#endif
}
