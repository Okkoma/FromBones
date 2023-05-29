#pragma once


using namespace Urho3D;


class GOManager : public Object
{
    URHO3D_OBJECT(GOManager, Object);

public :
    GOManager(Context* context);
    virtual ~GOManager();

    static void RegisterObject(Context* context);

    static GOManager* Get()
    {
        return goManager_;
    }

    void Start();
    void Stop();
    void Reset(bool keepPlayers);

    int firstActivePlayer();
    const Vector<unsigned>& GetActiveEnemies()
    {
        return activeEnemy;
    }
    const Vector<unsigned>& GetActiveAiNodes()
    {
        return activeAiNodes;
    }

    void Dump() const;
    static unsigned GetNumEnemies();
    static unsigned GetNumPlayers();
    static unsigned GetNumActiveBots();
    static unsigned GetNumActivePlayers();
    static unsigned GetNumActiveEnemies();
    static unsigned GetNumActiveAiNodes();
    static unsigned GetLastUpdate();

    static bool IsA(unsigned nodeId, int controllerType);

private :
    void HandleGOAppear(StringHash eventType, VariantMap& eventData);
    void HandleGODestroy(StringHash eventType, VariantMap& eventData);
    void HandleGOActive(StringHash eventType, VariantMap& eventData);
    void HandleGODead(StringHash eventType, VariantMap& eventData);
    void HandleGOChangeType(StringHash eventType, VariantMap& eventData);

    Vector<unsigned > bots;
    Vector<unsigned > enemy;
    Vector<unsigned > player;
    Vector<unsigned > activeBots;
    Vector<unsigned > activePlayer;
    Vector<unsigned > activeEnemy;
    Vector<unsigned > activeAiNodes;

    unsigned lastUpdate_;

    static GOManager* goManager_;
};


