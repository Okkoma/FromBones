#pragma once

#include "GameStateManager.h"

namespace Urho3D
{
    class Camera;
    class Connection;
    class UIElement;
    class Text;
}

class PlayState;
class SplashScreen;
class Player;
class GOManager;

using namespace Urho3D;

struct SceneCleaner
{
    SceneCleaner(PlayState* playstate) : playstate_(playstate), sceneDirty_(false) { }
    struct CleaningSceneStep
    {
        String sceneStr_;
        int phase_;
        bool launchLevel_, initState_, restartLevel_;
    };
    void AddStep(const String& sceneName, int phase, bool launchlevel=true, bool initstate=true, bool restartlevel=false);
    void Execute();
    void Clear()
    {
        steps_.Clear();
    }
    Vector<CleaningSceneStep> steps_;
    PlayState* playstate_;
    bool sceneDirty_;
};

class PlayState : public GameState
{
    URHO3D_OBJECT(PlayState, GameState);

    friend struct SceneCleaner;

public:
    PlayState(Context* context);
    ~PlayState();

    virtual bool Initialize();
    virtual void Begin();
    virtual void End();

    void SetStartPoint();
    void SetWorldSeed(unsigned seed);

    unsigned GetWorldSeed() const;
    const WorldMapPosition& GetStartPoint() const;

    void BeginNewLevel(GameLevelMode mode=LVL_NEW, unsigned seed=0);
    void ChangeLevel();
    void ReloadLevel();    // when broken network in loading state
    void RestartLevel(bool forcenet);
    void SetGameWin();
    void SetGamePause();
    void SaveGame();

    const Vector<SharedPtr<Player> >& GetLocalPlayers() const
    {
        return localPlayers_;
    }

    void OnPostRenderUpdate(StringHash eventType, VariantMap& eventData);

private:
    void SetStatus(GameStatus status, bool send=true);

    void ResetGameLogic();
    void CheckGameLogic();

    void CheckSplitScreen();

    void CreateLevel(bool restart, bool updatelevel);

    void InitLevel(bool init, bool restart);
    bool CreateScene();
    void EndScene();

    void SetGameOver();

    void CreateUI();
    void SetVisibleUI(bool ok);
    void RemoveUI();
    void ResizeUI();

    void AllocatePlayers();
	void GetLocalPlayers(PODVector<Player* >& playersindexes, bool getactiveonly=false, bool restartactive=false, unsigned controltypemask=0);
	void GetLocalPlayers(PODVector<int>& playersindexes, bool getactiveonly=false, bool restartactive=false, unsigned controltypemask=0);
    void SetPlayers(bool init, bool restart);
    void ResetPlayers();
    void RemovePlayers();
    void UpdateNumActivePlayers();
    void SetViewports(bool dynamic, bool init);

    void SubscribeToEvents();
    void UnsubscribeToEvents();
    void SubscribeToNetworkEvents();
    void UnsubscribeToNetworkEvents();

    void HandleInitialize(StringHash eventType, VariantMap& eventData);
    void HandleAppearPlayer(StringHash eventType, VariantMap& eventData);
    void HandleUpdateScores(StringHash eventType, VariantMap& eventData);
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleStop(StringHash eventType, VariantMap& eventData);
    void HandleCamera(float timeStep);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);
    void HandlePlayQuit(StringHash eventType, VariantMap& eventData);

    void OnGameStart(StringHash eventType, VariantMap& eventData);
    void OnPlayerDied(StringHash eventType, VariantMap& eventData);
    void OnGameOver(StringHash eventType, VariantMap& eventData);
    void OnGameOverMessage(StringHash eventType, VariantMap& eventData);
    void OnGameWin(StringHash eventType, VariantMap& eventData);
    void OnGameWinNextBattle(StringHash eventType, VariantMap& eventData);
    void OnContinueMessageAck(StringHash eventType, VariantMap& eventData);

    void OnDelayedActions();
    void OnDelayedActions_Server(StringHash eventType, VariantMap& eventData);
    void OnDelayedActions_Client(StringHash eventType, VariantMap& eventData);
    void OnDelayedActions_Local(StringHash eventType, VariantMap& eventData);

    Scene* rootScene_;
    Node* scene_;

    float cameraYaw_;
    float cameraPitch_;
    Vector3 cameraMotion_;
    float camMotionSpeed_;
    float currentCamMotionSpeed_;

    bool activeGameLogic_;
    bool initMode_;
    bool restartMode_;
    bool gameOver_;
    bool paused_;
    bool toLoadGame_;

    bool debugCameraWithMouse_;

    unsigned numActivePlayers_;
    unsigned lastKillerID_;
    Vector<Connection*> activeConnections_;

    unsigned hiScore;
    WeakPtr<Text> hiscoreText;
    GOManager* goManager;

    Vector<SharedPtr<Player> > localPlayers_;
    Vector<int> rankedCliendIDs_;
    Vector<UIElement* > playerStatusZone;

    WeakPtr<SplashScreen> splash_;

    GameStatus gameStatus_;

    /// Server Mode
    Vector<Connection* > connectionsToSet_;
    Vector<Connection* > connectionsToStart_;
    Vector<Connection* > connectionsToRemove_;

    WorldMapPosition startPoint_;
    unsigned worldSeed_;

    SceneCleaner sceneCleaner_;
};
