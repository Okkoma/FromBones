#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>

#include "GameContext.h"

#include "DefsCore.h"
#include "DefsGame.h"
#include "DefsWorld.h"
#include "DefsNetwork.h"

#include "GameOptions.h"


using namespace Urho3D;


class Player;

struct ClientAccount
{
    String name_;

    String addressIP_;
    String addressMAC_;
};

#ifdef ACTIVE_NETWORK_LOGSTATS
enum
{
    ObjCtrlSent = 0,
    ObjCtrlBytesSent,
    ObjCtrlReceived,
    ObjCtrlAckReceived,
    ObjCtrlBytesReceived,
    ObjCmdSent,
    ObjCmdBytesSent,
    ObjCmdReceived,
    ObjCmdBytesReceived
};

struct LogStatNetObject
{
    void Clear();
    void Copy(LogStatNetObject& logstat);
    unsigned int stats_[9];
};
#endif

struct ClientInfo
{
    ClientInfo();
    ~ClientInfo();

    void Clear();
    void ClearObjects();

    void AddPlayer();
    Node* CreateAvatarFor(unsigned playerindex);

    void StartPlayers();
    void StopPlayers();
    void ClearPlayers();

    bool NeedAllocatePlayers() const;
    int GetNumActivePlayers() const;
    Player* GetPlayerFor(Node* node) const;
    bool ContainsObject(Node* node) const;

    void Dump() const;

    WeakPtr<Connection> connection_;

    int clientID_;
    GameStatus gameStatus_;
    unsigned char timeStamp_;
    unsigned char lastReceivedTimeStamp_;
    unsigned short int lastObjectCmdStampAck;

    bool playersStarted_;
    int requestPlayers_;

#ifdef ACTIVE_NETWORK_LOGSTATS
    LogStatNetObject logStats_, tmpLogStats_;
#endif

    Vector<SharedPtr<Player> > players_;
    Vector<WeakPtr<Node> > objects_;
};

enum
{
    WHITEGRAY30,
    REDYELLOW35,
};


class FROMBONES_API GameNetwork : public Object
{
    URHO3D_OBJECT(GameNetwork, Object);

public:
    GameNetwork(Context* context);
    ~GameNetwork();

    static GameNetwork* Get()
    {
        return network_;
    }
    static Network* GetNetwork()
    {
        return network_->net_;
    }

    static void Clear();
    static bool ClientMode();
    static bool ServerMode();
    static bool LocalMode();

    static void AddGraphicMessage(Context* context, const String& msg, const IntVector2& position, int colortype, float delaystart=1.f);

    void Start();
    void Stop();
    void ClearScene();

    /// Setters
    void SetMode(const String& mode);
    void SetServerInfo(const String& ip, int port);
    void SetSeedTime(unsigned time);
    void SetGameStatus(GameStatus status, bool send=true);
#ifdef ACTIVE_NETWORK_SIMULATOR
    void SetSimulatorActive(bool enable);
#endif
#ifdef ACTIVE_NETWORK_LOGSTATS
    void SetLogStatistics(bool enable);
#endif
    bool PrepareControl(ObjectControlInfo& info);
    bool UpdateControlAck(ObjectControlInfo& dest, const ObjectControlInfo& src);
    bool UpdateControl(ObjectControlInfo& dest, const ObjectControlInfo& src, bool setposition=true);

    void SetEnabledServerObjectControls(Connection* connection, bool state);
    void SetEnabledClientObjectControls(Connection* connection, bool state);

    void SetEnableObject(bool enable, ObjectControlInfo& controlInfo, Node* node, bool forceactivation=false);
    void SetEnableObject(bool enable, unsigned nodeid, bool forceactivation=false);

    void PurgeObjects();

    /// Object Commands
    void PushObjectCommand(NetCommand pcmd, VariantMap* eventDataPtr=0, bool broadcast=true, int toclient=0);
    void RemoveItem(VariantMap& eventData);
    void ChangeEquipment(VariantMap& eventData);
    bool ChangeTile(VariantMap& eventData);
    void SendChangeEquipment(const StringHash& eventType, VariantMap& eventData);

    /// Spawn Controls
    unsigned GetSpawnID(unsigned holderid, unsigned char stamp) const;
    unsigned GetSpawnHolder(unsigned spawnID) const;
    unsigned char GetSpawnStamp(unsigned spawnID) const;
    ObjectControlInfo* GetSpawnControl(int clientid, unsigned spawnid) const;
    bool NetSpawnControlAlreadyUsed(int clientid, unsigned char spawnstamp=0) const;
    ObjectControlInfo* AddSpawnControl(Node* node, Node* holder=0, bool enable=true);
    void RemoveSpawnControl(int clientid, unsigned spawnid);
    void LinkSpawnControl(int clientid, unsigned spawnid, unsigned servernodeid, unsigned clientnodeid, ObjectControlInfo* oinfo);
    void NetSpawnEntity(unsigned refnodeid, const ObjectControlInfo& inforef, ObjectControlInfo*& oinfo);

    /// Object Controls
    void NetAddEntity(ObjectControlInfo& info, Node* holder=0, VariantMap* eventData=0, bool enable=true);
    ObjectControlInfo* GetObjectControl(unsigned nodeid);
    const ObjectControlInfo* GetObjectControl(unsigned nodeid) const;
    ObjectControlInfo* GetServerObjectControl(unsigned nodeid);
    ObjectControlInfo* GetClientObjectControl(unsigned nodeid);
    ObjectControlInfo& GetOrCreateServerObjectControl(unsigned servernodeid, unsigned clientnodeid=0, int clientid=0, Node* node=0);
    ObjectControlInfo* GetOrCreateClientObjectControl(unsigned servernodeid, unsigned clientnodeid=0);
    const Vector<ObjectControlInfo>& GetServerObjectControls() const
    {
        return serverObjectControls_;
    }
    const Vector<ObjectControlInfo>& GetClientObjectControls() const
    {
        return clientObjectControls_;
    }
    void RemoveServerObject(ObjectControlInfo& cinfo);

    /// Server Setters
    void Server_SendGameStatus(int status, Connection* specificConnection=0);
    void Server_SendSeedTime(unsigned time);
    void Server_SendSnap(ClientInfo& clientInfo);
    void Server_RemoveObject(unsigned nodeid, bool sendnetevent=true, bool allconnections=false);
    void Server_PurgeInactiveObjects();
    void Server_SetEnableAllObjects(bool enable);

    void Server_AllocatePlayers(ClientInfo& clientInfo);
    void Server_SendInventories(ClientInfo& clientInfo);
    void Server_SetActivePlayer(Player* player, bool active);
    void Server_SetEnablePlayers(ClientInfo& clientInfo, bool enable);
    void Server_SetEnableAllActiveObjectsToClient(ClientInfo& clientInfo);
    void Server_RemovePlayers(ClientInfo& clientInfo);
    void Server_RemoveAllPlayers();

    void Server_PopulateClientIDs();
    int Server_GetNextClientID();
    void Server_FreeClientID(int id);
    bool Server_CheckAccount(const StringHash& key);
    void Server_AddConnection(Connection* connection, bool clean);
    void Server_KillConnections();
    void Server_PurgeConnections();
    void Server_PurgeConnection(Connection* connection);
    void Server_CleanUpConnection(Connection* connection);
    void Server_CleanUpAllConnections();

    /// Client Setters
    void Client_SendGameStatus();
    void Client_AddServerControllable(Node*& node, ObjectControlInfo& info);
    void Client_RemoveObject(unsigned nodeid);
    void Client_RemoveObject(ObjectControlInfo& cinfo);
    void Client_RemoveObjects();
    void Client_PurgeConnection();

    /// Getters
    void GetLocalAddresses(Vector<String>& adresses);
    bool IsServer() const
    {
        return serverMode_;
    }
    bool IsClient() const
    {
        return clientMode_;
    }
    bool IsStarted() const
    {
        return started_;
    }
    unsigned GetSeedTime() const
    {
        return seedTime_;
    }
    // Return the id of this client assigned by server : for server or localmode clientid=0
    int GetClientID() const
    {
        return clientID_;
    }

#ifdef ACTIVE_NETWORK_SIMULATOR
    bool IsSimulatorActive() const;
#endif
#ifdef ACTIVE_NETWORK_LOGSTATS
    bool GetLogStatistics() const;
#endif

    /// Server Getters Only
    unsigned GetNumConnections() const
    {
        return serverClientInfos_.Size();
    }
    const HashMap<Connection*, ClientInfo>& GetServerClientInfos() const
    {
        return serverClientInfos_;
    }
    ClientInfo* GetServerClientInfo(Connection* connection)
    {
        HashMap<Connection*, ClientInfo>::Iterator it = serverClientInfos_.Find(connection);
        return it != serverClientInfos_.End() ? &(it->second_) : 0;
    }
    // Server : Get ClientID for the connection
    int GetClientID(Connection* connection)
    {
        ClientInfo* cinfo = GetServerClientInfo(connection);
        return cinfo ? cinfo->clientID_ : 0;
    }
    Connection* GetConnection(int clientid) const
    {
        HashMap<int, Connection* >::ConstIterator it = serverClientConnections_.Find(clientid);
        return it != serverClientConnections_.End() ? it->second_ : 0;
    }
    GameStatus GetGameStatus(Connection* connection) const;

    /// Client Getters Only
    // Return the connection of this client to server
    Connection* GetConnection() const;
    // Return the nodeid of the object "index" owned by this client and assigned by the server for this client
//    unsigned GetClientObjectID(unsigned index) const { return index < clientNodeIDs_.Size() ? clientNodeIDs_[index] : 0; }

    GameStatus GetGameStatus() const
    {
        return gameStatus_;
    }

    /// Generic Subscribers
    void SubscribeToEvents();
    void SubscribeToMenuEvents();
    void SubscribeToPlayEvents();
    void UnsubscribeToMenuEvents();
    void UnsubscribeToPlayEvents();

    void DumpClientInfos() const;
    void DumpNetObjects() const;
    void Dump() const;

private:
    void Reset();
    void StartServer();
    void StartClient();
    void StartLocal();

    /// Server and Client Subscribers
    void Server_SubscribeToPlayEvents();
    void Server_UnsubscribeToPlayEvents();

    void Client_SubscribeToPlayEvents();
    void Client_UnsubscribeToPlayEvents();

    /// Generic Handles : connection establishing
    void HandleSearchServer(StringHash eventType, VariantMap& eventData);
    void HandleConnectionStatus(StringHash eventType, VariantMap& eventData);

    /// Server Handles
    void HandleServer_MessagesFromClient(StringHash eventType, VariantMap& eventData);
    void HandlePlayServer_NetworkUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePlayServer_ReceiveUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePlayServer_Messages(StringHash eventType, VariantMap& eventData);

    /// Server commands from clients
    void Server_ApplyObjectCommand(VariantMap& eventData);

    /// Client Handles
    void HandleClient_MessagesFromServer(StringHash eventType, VariantMap& eventData);
    void HandlePlayClient_NetworkUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePlayClient_ReceiveServerUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePlayClient_ReceiveClientUpdate(StringHash eventType, VariantMap& eventData);
    void HandlePlayClient_MessagesFromServer(StringHash eventType, VariantMap& eventData);
    void HandlePlayClient_Messages(StringHash eventType, VariantMap& eventData);

    /// Client commands from server
    void Client_CommandAddObject(VariantMap& eventData);
    void Client_CommandRemoveObject(VariantMap& eventData);
    void Client_DisableObjectControl(VariantMap& eventData);
    void Client_CommandAddItem(VariantMap& eventData);
    void Client_ApplyObjectCommand(VariantMap& eventData);

    void CleanObjectCommands();
    ObjectCommand& GetFreeObjectCommand();

#ifdef ACTIVE_NETWORK_LOGSTATS
    void UpdateLogStats();
#endif

    Network* net_;
    int serverPort_;
    String serverIP_;
    unsigned seedTime_;

    // the number assigned by the server for this client
    int clientID_;

    GameStatus gameStatus_, lastGameStatus_;
    GameNetworkMode networkMode_;

    bool serverMode_, clientMode_, started_;
    bool needSynchronization_, allClientsRunning_, allClientsSynchronized_;
    float timerCheckForServer_, timerCheckForConnection_;
    float delayForConnection_, delayForServer_;

    /// Object Controls
    Vector<ObjectControlInfo> serverObjectControls_;
    Vector<ObjectControlInfo> clientObjectControls_;

    /// Object Commands
    Vector<ObjectCommand> objectCommands_;
    bool newObjectCommands_;

    /// Temporary Spawned Object Controls by client et by spawnid
    Vector<HashMap<unsigned, ObjectControlInfo* > > spawnControls_;
    /// received and local spawnStamp for each client
    PODVector<unsigned char> receivedSpawnStamps_;
    PODVector<unsigned char> localSpawnStamps_;

    /// Server Only
    VectorBuffer preparedServerMessageBuffer_;
    HashMap<Connection*, ClientInfo> serverClientInfos_;
    HashMap<int, Connection* > serverClientConnections_;
    HashMap<StringHash, ClientAccount> clientAccounts_;
    Vector<StringHash> loggedClientAccounts_;
    List<int> serverFreeClientIds_;

    /// Client Only
    GameStatus serverGameStatus_;
//    PODVector<unsigned> clientNodeIDs_;
    WeakPtr<Connection> clientSideConnection_;
    unsigned char timeStamp_, lastReceivedTimeStamp_;

#ifdef ACTIVE_NETWORK_LOGSTATS
    bool logStatEnable_;
    LogStatNetObject logStats_, tmpLogStats_;
    Timer logStatTimer_;
    Timer logStatCountTimer_;
#endif

    static GameNetwork* network_;
};
