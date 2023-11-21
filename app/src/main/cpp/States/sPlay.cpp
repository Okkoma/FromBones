#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Timer.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/File.h>

#include <Urho3D/Engine/Engine.h>

#include <Urho3D/DebugNew.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Network/Connection.h>
#include <Urho3D/Network/Network.h>
#include <Urho3D/Network/NetworkEvents.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Graphics/Octree.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>

#include <Urho3D/Input/Input.h>
//#include <Urho3D/Physics/PhysicsWorld.h>

#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/MessageBox.h>
#include <Urho3D/UI/Window.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundListener.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameNetwork.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"
//#include "GameData.h"

#include "MAN_Ai.h"
#include "MAN_Go.h"
#include "MAN_Weather.h"
#include "MAN_Effects.h"

#include "GOC_Animator2D.h"
#include "GOC_ControllerAI.h"
#include "GOC_ControllerPlayer.h"
#include "GOC_Destroyer.h"
#include "GOC_Move2D.h"
#include "GOC_Life.h"
#include "GOC_Attack.h"
#include "GOC_Inventory.h"
#include "GOC_BodyExploder2D.h"
#include "GOC_BodyFaller2D.h"
#include "GOC_StaticRope.h"
#include "GOC_PhysicRope.h"

#include "LSystem.h"
#include "GEF_Scrolling.h"
#include "RenderShape.h"
#include "SplashScreen.h"

#include "GO_Pool.h"
#include "ObjectPool.h"

#include "Actor.h"
#include "Player.h"
#include "PathFinder2D.h"
#include "MapWorld.h"
#include "MapStorage.h"
#include "ObjectMaped.h"
#include "WaterLayer.h"

#ifdef ACTIVE_CREATEMODE
#include "MapEditor.h"
#include "ObjectMaped.h"
#endif

#include "ViewManager.h"
#include "TimerInformer.h"

#include "TextMessage.h"

#include "sOptions.h"

#include "sPlay.h"


//#define PLAYSTATE_STOPATFIRSTFRAME

WeatherManager* weather_ = 0;
EffectsManager* effects_ = 0;
WeakPtr<World2D> world_;

extern const char* gameStatusNames[];
extern const char* levelModeNames[];

void SceneCleaner::AddStep(const String& sceneName, int phase, bool launchlevel, bool initstate, bool restartlevel)
{
    steps_.Resize(steps_.Size()+1);

    CleaningSceneStep& step = steps_.Back();
    step.sceneStr_ = sceneName;
    step.phase_ = phase;
    step.launchLevel_ = launchlevel;
    step.initState_ = initstate;
    step.restartLevel_ = restartlevel;
    sceneDirty_ = true;
}

void SceneCleaner::Execute()
{
    if (sceneDirty_)
    {
        for (unsigned i=0; i < steps_.Size(); i++)
        {
            CleaningSceneStep& step = steps_[i];

            if (!step.sceneStr_.Empty())
                GameHelpers::CleanScene(GameContext::Get().rootScene_, step.sceneStr_, step.phase_);

            if (step.launchLevel_)
                playstate_->InitLevel(step.initState_, step.restartLevel_);
        }

        steps_.Clear();
        sceneDirty_ = false;
    }
}

static WeakPtr<Node> sTestObjectMapedNode;

WeakPtr<TextMessage> createModeText_, levelText_, levelGOText_;
TextMessage* winMessage = 0;
TextMessage* gameOverMessage = 0;
bool gameUIVisible_;

WeakPtr<Urho3D::MessageBox> messageBox;

PlayState::PlayState(Context* context) :
    GameState(context, "Play"),
    ctrlCameraWithMouse_(false),
    goManager(0),
    hiScore(0),
    sceneCleaner_(this)
{
//    URHO3D_LOGINFO("PlayState()");
}

PlayState::~PlayState()
{
    URHO3D_LOGINFO("~PlayState()");
}

bool PlayState::Initialize()
{
//    URHO3D_LOGINFO("PlayState() - Initialize");

    worldSeed_ = 0;
    return GameState::Initialize();
}


void PlayState::Begin()
{
    if (IsBegun())
        return;

    sTestObjectMapedNode.Reset();

//    SetStatus(PLAYSTATE_INITIALIZING);
    GameContext::Get().AllowUpdate_ = false;

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - Begin ...                                -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    // Get the scene instantiate by Game
    rootScene_ = GameContext::Get().rootScene_;
    scene_ = 0;
    activeviewport_ = 0;

    GOC_Inventory::ClearCache();

    goManager = new GOManager(context_);
//    if (!GameContext::Get().ClientMode_)
    {
        AIManager::InitManagers(context_);
        AIManager::SetManagers(rootScene_, goManager);
    }

    rootScene_->StopAsyncLoading();

    if (GetManager()->GetPreviousActiveState())
    {
        URHO3D_LOGINFOF("PlayState() - Begin : Previous Active State = %s", GetManager()->GetPreviousActiveState()->GetStateId().CString());
        // Cleaning the scene from mainMenu Only
        // In Restart Playing, the cleaning is done in EndScene
        if (GetManager()->GetPreviousActiveState()->GetStateId() == "MainMenu")
        {
            sceneCleaner_.AddStep("MainMenu", 0, true);
            sceneCleaner_.AddStep("MainMenu", 1, false);
        }
    }
    else
        URHO3D_LOGWARNINGF("PlayState() - Begin : no Previous Active State to Clean !");

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PlayState, OnDelayedActions_Local));
    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(PlayState, HandleScreenResized));

    GameState::Begin();

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - Begin ... OK !                         -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
}

void PlayState::End()
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - End ...                                   -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    UnsubscribeFromEvent(E_POSTUPDATE);
    UnsubscribeFromEvent(GAME_SCREENRESIZED);

    EndScene();

    if (GameNetwork::Get())
    {
        GameNetwork::Get()->UnsubscribeToPlayEvents();
    }

    if (GetSubsystem<UI>())
    {
        RemoveUI();
    }

    // Release Resource Save Files
    {
        ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
        for (unsigned i=0; i < localPlayers_.Size(); ++i)
        {
            if (localPlayers_[i] && i < GameContext::Get().MAX_NUMPLAYERS)
            {
                String playerFile = GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedPlayerFile_[i];
                URHO3D_LOGINFOF("PlayState() - End : Release Player File %s ...", playerFile.CString());
                cache->ReleaseResource(XMLFile::GetTypeStatic(), playerFile, true);
            }
        }
        cache->ReleaseResource(XMLFile::GetTypeStatic(), String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedSceneFile_), true);
    }

    RemovePlayers();

    AIManager::RemoveManagers();

    if (goManager)
        delete goManager;

    if (weather_)
        delete weather_;

    if (effects_)
        delete effects_;

    ViewManager::Get()->SetViewportLayout(1);
    ViewManager::Get()->ResizeViewports();

#ifdef ACTIVE_CREATEMODE
    if (MapEditor::Get())
        MapEditor::Release();
#endif // ACTIVE_CREATEMODE

    goManager = 0;
    weather_ = 0;
    effects_ = 0;

    // don't release : problem with Texture on Android
//    cache->ReleaseAllResources(true);

    UnsubscribeFromEvent(SPLASHSCREEN_FINISH);
    SendEvent(SPLASHSCREEN_STOP);

    GameState::End();

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - End ... OK !                            -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
}


void PlayState::SetStatus(GameStatus status, bool send)
{
    gameStatus_ = status;

    if (!GameContext::Get().LocalMode_)
        GameNetwork::Get()->SetGameStatus(status, send);

    URHO3D_LOGINFO("------------------------------------------------------------");
    URHO3D_LOGINFOF("- PlayState() - SetStatus : New Status = %s      -", gameStatusNames[(int)gameStatus_]);
    URHO3D_LOGINFO("------------------------------------------------------------");
}

void PlayState::SetWorldSeed(unsigned seed)
{
//    URHO3D_LOGINFOF("PlayState() - SetWorldSeed : %u -", seed);
    worldSeed_ = seed;
}

unsigned PlayState::GetWorldSeed() const
{
    return (!worldSeed_ && world_) ? world_->GetGeneratorSeed() : worldSeed_;
}


void PlayState::ResetGameLogic()
{
    activeGameLogic_ = false;
    gameOver_ = false;
    paused_ = false;

    winMessage = 0;
    gameOverMessage = 0;

    cameraYaw_ = 0.0f;
    cameraPitch_ = 0.0f;
    lastKillerID_ = 0;
}



void PlayState::CheckGameLogic()
{
    if (GameContext::Get().createModeOn_)
        return;

    if (!activeGameLogic_ || gameOver_)
        return;

    if (!GameContext::Get().LocalMode_)
    {
        // Arena
        if (GameContext::Get().arenaZoneOn_)
        {
            if (GameContext::Get().ServerMode_)
            {
                if (GameContext::Get().arenaMod_ == 0)
                {
//                    if (numActivePlayers_ == 0)
//                    {
//                        SetGameOver();
//                    }
                    // Mod Survivor : The Last Player to Survive
                    if (numActivePlayers_ == 1)
                    {
                        if (activeConnections_.Size() == 1)
                        {
                            // Send GameStatus Win To the surviving client
                            GameNetwork::Get()->Server_SendGameStatus(PLAYSTATE_WINGAME, activeConnections_.Front());
                            activeGameLogic_ = false;

                            for (int i=0; i < rankedCliendIDs_.Size(); i++)
                                URHO3D_LOGINFOF("Rank %d : player clientid=%d", i+1, rankedCliendIDs_[i]);
                        }
                        // Server Wins
//                        else if (!activeConnections_.Size())
//                        {
//                            SetGameWin();
//                        }
                    }
                }
                else if (GameContext::Get().arenaMod_ == 1)
                {
                    // Mod Timer : Best Player At the Timer End

                }
                else if (GameContext::Get().arenaMod_ == 2)
                {
                    // Mod Collector : The Player who find the most quantity of a specified object

                }
            }
            else
            {
                if (GameContext::Get().arenaMod_ == 0)
                {
                    // Mod Survivor : The Last Player to Survive

                    // GameOver : Local Players are all dead
                    if (numActivePlayers_ == 0)
                    {
                        SetGameOver();
                    }
                    // GameWin : Local Survivor Player Win the game
                    else if (numActivePlayers_ == 1 && GameNetwork::Get()->GetGameStatus() == PLAYSTATE_WINGAME)
                    {
                        SetGameWin();
                    }
                }
                else if (GameContext::Get().arenaMod_ == 1)
                {
                    // Mod Timer : Best Player At the Timer End

                }
                else if (GameContext::Get().arenaMod_ == 2)
                {
                    // Mod Collector : The Player who find the most quantity of a specified object

                }
            }
        }
        // World
        else
        {
            return;
        }
    }
    else
    {
        // check game over
        if (!numActivePlayers_)
        {
            URHO3D_LOGINFOF("PlayState() - CheckGameLogic : No More Active Player ! => GameOver !");
            SetGameOver();
        }
        // check game win
        else
        {
            if (GameContext::Get().tipsWinLevel_)
            {
                URHO3D_LOGINFOF("PlayState() - CheckGameLogic : Tip Win ! => GameWin !");
                SetGameWin();
            }
            else if (GameContext::Get().enableWinLevel_)
            {
                // check condition to win
                if (GameContext::Get().arenaZoneOn_)
                {
                    if (numActivePlayers_ == 1)
                    {
                        // Survivor Mode
                        if (localPlayers_.Size() > 1)
                        {
                            if (GOManager::GetNumActiveBots() == 0)
                            {
                                URHO3D_LOGINFOF("PlayState() - CheckGameLogic : Arena Mod Player Survivor : the last of them ! => GameWin !");
                                SetGameWin();
                            }
                        }
                        // Enemy Killer Mode
                        else
                        {
                            if (GOManager::GetNumActiveEnemies() == 0)
                            {
                                URHO3D_LOGINFOF("PlayState() - CheckGameLogic : Arena Mod Player Killer : no more active enemies ! => GameWin !");
                                SetGameWin();
                            }
                        }
                    }
                }
                else
                {
                    return;

                    // TODO conditions for World
                    if (GOManager::GetNumActiveEnemies() == 0)
                    {
                        URHO3D_LOGINFOF("PlayState() - CheckGameLogic : No More Enemies ! => GameWin !");
                        SetGameWin();
                    }
                }
            }
        }
    }
}


void PlayState::BeginNewLevel(GameLevelMode mode, unsigned seed)
{
//    if (mode == LVL_LOAD)
//        return;

    GameContext::Get().SetConsoleVisible(false);

    GameContext::Get().ResetGameStates();

    initMode_ = false;

    if (!seed)
    {
        // random seed
        world_->SetGeneratorRandomSeed();
        seed = world_->GetGeneratorSeed();
    }

    if (mode == LVL_NEW)
    {
        // Continue in Create Mode
        if (GameContext::Get().lastLevelMode_ == LVL_CREATE)
        {
            initMode_ = true;
            GameContext::Get().numPlayers_ = 0;
            GameContext::Get().createModeOn_ = true;
            mode = LVL_CREATE;
        }
    }
    else if (mode == LVL_ARENA)
    {
        // Allow exiting Create Mode : create 1 player
        if (GameContext::Get().lastLevelMode_ == LVL_CREATE)
        {
            GameContext::Get().numPlayers_ = 1;
            initMode_ = true;
        }
        GameContext::Get().arenaZoneOn_ = true;
    }
    else if (mode == LVL_TEST)
    {
        // Allow exiting Create Mode : create 1 player
        if (GameContext::Get().lastLevelMode_ == LVL_CREATE)
        {
            GameContext::Get().numPlayers_ = 1;
            initMode_ = true;
        }
        GameContext::Get().testZoneOn_ = true;
    }
#ifdef ACTIVE_CREATEMODE
    else if (mode == LVL_CREATE)
    {
        initMode_ = true;
        GameContext::Get().numPlayers_ = 0;
        GameContext::Get().createModeOn_ = true;
        World2D::SavePositionFocus();
    }
#endif
    else if (mode == LVL_CHANGE)
    {
        // Allow exiting Create Mode : create 1 player
        if (GameContext::Get().lastLevelMode_ == LVL_CREATE)
        {
            GameContext::Get().numPlayers_ = 1;
            initMode_ = true;
        }
    }
    else if (mode == LVL_LOAD)
    {
        GameContext::Get().loadSavedGame_ = true;
        initMode_ = true;
    }

    GameContext::Get().lastLevelMode_ = mode;

    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFOF("PlayState() - BeginNewLevel : %s seed=%u          -", levelModeNames[mode], seed);
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    SetWorldSeed(seed);

    if (mode == LVL_CHANGE && !initMode_)
        ChangeLevel();
    else
        CreateLevel(false, false);
}

void PlayState::ChangeLevel()
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - ChangeLevel                           -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    GameContext::Get().tipsWinLevel_ = false;

    for (unsigned i=0; i<localPlayers_.Size(); ++i)
        localPlayers_[i]->SaveState();

    world_->SaveWorld();

    initMode_ = false;
    CreateLevel(false, true);
}

void PlayState::ReloadLevel()
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - ReloadLevel                            -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    GameContext::Get().ResetNetworkStatics();
    CreateLevel(false, false);
}

void PlayState::RestartLevel(bool forcenet)
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - RestartLevel                           -");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    if (!forcenet)
    {
        CreateLevel(true, false);
    }
    else
    {
        ResetGameLogic();
        // Purge GoManager
        goManager->Start();

        ResetPlayers();

        SubscribeToEvents();
        SubscribeToEvent(SPLASHSCREEN_FINISH, URHO3D_HANDLER(PlayState, HandleAppearPlayer));
        SendEvent(SPLASHSCREEN_FINISH);
    }

    SetStatus(PLAYSTATE_STARTGAME);

    URHO3D_LOGINFOF("PlayState() - RestartLevel ... numlocalplayers=%u OK !", GameContext::Get().numPlayers_);
}

void PlayState::CreateLevel(bool restart, bool updatelevel)
{
//    stateManager_->SetActiveState("Play");
    if (restart)
        world_->SaveWorld();

    if (!restart)
        EndScene();

    SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PlayState, OnDelayedActions_Local));

    if (updatelevel)
        GameContext::Get().UpdateLevel();
    else
        int l = GameContext::Get().SetLevel();

    sceneCleaner_.AddStep(/*restart ? */String::EMPTY/* : GetStateId()*/, 0, true, initMode_, restart);
}

Timer loadingTimer_;

void PlayState::InitLevel(bool init, bool restart)
{
    URHO3D_PROFILE(InitLevel);

    initMode_ = init;
    restartMode_ = restart;
    toLoadGame_ = false;

    SetStatus(PLAYSTATE_INITIALIZING);
    GameContext::Get().AllowUpdate_ = false;

    if (initMode_ || restartMode_)
    {
        toLoadGame_ = GameContext::Get().loadSavedGame_;

        for (int i=0; i < GameContext::Get().MAX_NUMPLAYERS; ++i)
            GameContext::Get().playerState_[i].active = false;

        for (int i=0; i < GameContext::Get().numPlayers_; ++i)
        {
            GameContext::Get().playerState_[i].active = true;
            URHO3D_LOGINFOF("PlayState() - InitLevel : active player id=%d ...", i);
        }
    }

    if (toLoadGame_)
    {
        GameHelpers::LoadData(context_, String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedGameFile_).CString(), &GameContext::Get().gameState_);
        GameContext::Get().SetLevel();
        GameContext::Get().gameState_.Dump();

//        numLocalPlayers_ = GameContext::Get().numPlayers_;
    }

    URHO3D_LOGINFOF("PlayState() - InitLevel(init=%u, load=%u) : Level=%d seed=%u...", initMode_, toLoadGame_, GameContext::Get().indexLevel_, worldSeed_);

    ResetGameLogic();

    if (!restartMode_)
    {
        if (!CreateScene())
        {
            SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PlayState, HandleStop));
            return;
        }

        if (!world_)
        {
            URHO3D_LOGERRORF("PlayState() - InitLevel : no WORLD ! exit !");
            stateManager_->PopStack();
            return;
        }

        // Set World
        if (worldSeed_)
            world_->SetGeneratorSeed(worldSeed_);

#ifdef RANDOMIZE_ARENA
        if (GameContext::Get().arenaZoneOn_ && !initMode_)
        {
            GameRand::SetSeedRand(ALLRAND, worldSeed_);

            // Randomize Map Parameters
            int rgenseed = GameRand::GetRand(ALLRAND, 65535);
            int zonetype = GameRand::GetRand(ALLRAND, 100);
            int defaultmappointx = GameRand::GetRand(ALLRAND, -10, 10);
            int defaultmappointy = GameRand::GetRand(ALLRAND, -20, -10);

            // Randomize Dungeon Parameters
            int maxFeatures = 100 + GameRand::GetRand(ALLRAND, 200);
            int roomprob = 50 + GameRand::GetRand(ALLRAND, 25);
            int corridorProb = 50 + GameRand::GetRand(ALLRAND, 25);
            int roomminsize = 3 + GameRand::GetRand(ALLRAND, 3);
            int roommaxsize = 15 + GameRand::GetRand(ALLRAND, 10);
            int dungeontype = -1;

            // Randomize Monsters Parameters
            int basevalue = 0;
            int numEntityTypes = GameRand::GetRand(ALLRAND, 1, 4);

            // Random World Model
            world_->SetGeneratorSeed(rgenseed);

            if (zonetype > 50)
            {
                world_->SetDefaultMapPoint(IntVector2(defaultmappointx , defaultmappointy));
                world_->SetAnlWorldModelAttr("anlworldVM-ellipsoid-zone2.xml");
            }
            else
            {
                world_->SetDefaultMapPoint(IntVector2::ZERO);
                world_->SetAnlWorldModelAttr(String::EMPTY);
            }

            // Use Dungeon Map
            world_->SetMapGeneratorAttr("MapGeneratorDungeon");

            // Set Dungeon Parameters
            world_->SetMapGeneratorParams(ToString("z:INNERVIEW|i:%d;%d;%d;%d;%d;%d", maxFeatures, roomprob, corridorProb, roomminsize, roommaxsize, dungeontype));

            // Set Monsters Listing
            String entityList;
            for (int i=0; i < numEntityTypes; i++)
            {
                StringHash got(COT::GetRandomTypeFrom(COT::MONSTERS));
                entityList += GOT::GetType(got);
                if (i < numEntityTypes-1 )
                    entityList += String("|");
                basevalue += GOT::GetConstInfo(got).defaultvalue_;
            }
            IntVector2 quantity;
            basevalue /= numEntityTypes;
            if (basevalue <= 20)
            {
                quantity.x_ = 15;
                quantity.y_ = 25;
            }
            else if (basevalue <= 100)
            {
                quantity.x_ = 10;
                quantity.y_ = 20;
            }
            else if (basevalue <= 500)
            {
                quantity.x_ = 5;
                quantity.y_ = 10;
            }
            else
            {
                quantity.x_ = 3;
                quantity.y_ = 5;
            }

            URHO3D_LOGINFOF("PlayState() - InitLevel : ArenaZone RandomSeed=%d MapPoint=%s Model=%s ...", rgenseed, world_->GetDefaultMapPoint().ToString().CString() , world_->GetAnlWorldModelAttr().CString());
            URHO3D_LOGINFOF("PlayState() - InitLevel : ArenaZone Dungeon Params=%s ...", world_->GetMapGeneratorParams().CString());
            URHO3D_LOGINFOF("PlayState() - InitLevel : ArenaZone add %s basevalue=%d qty=%s ...", entityList.CString(), basevalue, quantity.ToString().CString());

            world_->SetGeneratorAuthorizedCategories(entityList);
            world_->SetGeneratorNumEntities(quantity);
        }
#endif

        world_->Set(toLoadGame_);
    }

    // Stop the update of the physics (prevent crash)
    rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(false);

    // Set Default Map Point
    if (!GameContext::Get().createModeOn_ && (toLoadGame_ || restartMode_))
        world_->SetDefaultMapPoint(GameContext::Get().playerState_[0].position);

    // Start World
    // Set Enable allow to Reset Position before map loading
    world_->SetEnabled(true);

    // Centered map Position
    Vector2 toFocus;
    if (toLoadGame_)
    {
        toFocus = GameContext::Get().playerState_[0].position;
    }
    else
    {
        world_->GetWorldInfo()->Convert2WorldPosition(world_->GetDefaultMapPointShort(), world_->GetWorldMapSize() / 2, toFocus);
    }

    URHO3D_LOGINFOF("PlayState() - InitLevel : focus on position = %s", toFocus.ToString().CString());

    URHO3D_LOGINFOF("PlayState() - InitLevel(init=%u) : Level %d asynLoadingWorldMap_=%s  ...",
                    initMode_, GameContext::Get().indexLevel_, GameContext::Get().gameConfig_.asynLoadingWorldMap_ ? "true":"false");

    // Set Camera
    world_->SetCamera(1.f, toFocus);

    ViewManager::Get()->SwitchToViewZ(FRONTVIEW, 0);

    // Initialize World Loading
    if (!restartMode_)
    {
        if (GameContext::Get().gameConfig_.asynLoadingWorldMap_)
        {
            world_->UpdateStep(0.f);
            world_->SetUpdateLoadingDelay(MAP_MAXDELAY_ASYNCLOADING);
        }
        else
        {
            world_->SetUpdateLoadingDelay(MAP_MAXDELAY_NOASYNCLOADING);
            world_->UpdateAll();
        }
    }

    // Set UI
    SendEvent(TEXTMESSAGE_CLEAN);

    CreateUI();

    URHO3D_LOGINFOF("PlayState() - InitLevel(init=%u) : Level %d maxDelayUpdate=%d ... OK !", initMode_, GameContext::Get().indexLevel_, World2DInfo::delayUpdateUsec_);

    // Finish Initialize
    SetStatus(restartMode_ ? PLAYSTATE_FINISHLOADING : PLAYSTATE_LOADING);

    loadingTimer_.Reset();

    const char* SPLASHTEXTURES[2] = { "Textures/UI/splash.png", "Textures/UI/splash2.png" };

    // Start SplashScreen
#ifdef ACTIVE_SPLASHUI
    if (!splash_ && !GameContext::Get().createModeOn_)
        splash_ = new SplashScreen(context_, this, WORLD_FINISHLOADING, SPLASHTEXTURES[Random(2)]);
#endif // ACTIVE_UIEFFECTS

    // Subsribe to worldLoading2.
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PlayState, HandleInitialize));

    // when worldloading finishes => splashscreen finishes then Appearplayers begins
    SubscribeToEvent(SPLASHSCREEN_FINISH, URHO3D_HANDLER(PlayState, HandleAppearPlayer));
}

bool PlayState::CreateScene()
{
    URHO3D_LOGINFO("PlayState() - CreateScene");

    // Initialize Random System
    {
        unsigned rtime = Time::GetSystemTime();

        if (GameContext::Get().gameNetwork_)
        {
            if (GameContext::Get().ServerMode_)
                GameContext::Get().gameNetwork_->SetSeedTime(rtime);
            else
                rtime = GameContext::Get().gameNetwork_->GetSeedTime();
        }

        srand(rtime);
        SetRandomSeed(rtime);
    }

//    LOGINFOF("PlayState() - CreateScene : Dump vars = %s", rootScene_->GetVarNamesAttr().CString());

    if (GameContext::Get().gameConfig_.debugRenderEnabled_)
        rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);

    //if (GameContext::Get().physics3DEnabled_)
    //{
    //    PhysicsWorld* physicsWorld3D_ = rootScene_->GetOrCreateComponent<PhysicsWorld>(LOCAL);
    //    if (GameContext::Get().debugRenderEnabled_)
    //        physicsWorld3D_->setDebugMode(0xFFFF);
    //    URHO3D_LOGINFO("PlayState() - CreateScene : physicsWorld3D_ create");
    //}

    if (GameContext::Get().gameConfig_.physics2DEnabled_)
    {
        PhysicsWorld2D* physicsWorld2D_ = rootScene_->GetOrCreateComponent<PhysicsWorld2D>(LOCAL);
        if (physicsWorld2D_)
        {
            if (GameContext::Get().gameConfig_.debugRenderEnabled_)
            {
                physicsWorld2D_->SetDrawJoint(true); // Display the joints
//                physicsWorld2D_->SetDrawAabb(true);
                physicsWorld2D_->SetDrawShape(true);
//                physicsWorld2D_->SetDrawCenterOfMass(true);
            }
//            physicsWorld2D_->SetVelocityIterations(5);
//            physicsWorld2D_->SetPositionIterations(2);
            physicsWorld2D_->SetContinuousPhysics(true);
            physicsWorld2D_->SetSubStepping(false);
            physicsWorld2D_->SetWarmStarting(true);
            physicsWorld2D_->SetAutoClearForces(true);
            URHO3D_LOGINFO("PlayState() - CreateScene : physicsWorld2D_ create");
        }
        else
            URHO3D_LOGERROR("PlayState() - CreateScene : physicsWorld2D_ NOT CREATE !");
    }

    // Create Structure Nodes for the scene
    scene_ = rootScene_->GetChild("Scene");
    if (!scene_)
        scene_ = rootScene_->CreateChild("Scene", LOCAL);

    // staticScene is for collisionShape generated by MapCollider2D
//    if (!rootScene_->GetChild("StaticScene"))
//        rootScene_->CreateChild("StaticScene", LOCAL);

    // Start Managers
    GO_Pools::Start();
    goManager->Start();

    // Load Scene
    URHO3D_LOGINFOF("PlayState() - CreateScene : ... Load Scene for level=%d", GameContext::Get().indexLevel_);
    if (!GameHelpers::LoadSceneXML(context_, scene_, LOCAL))
    {
        URHO3D_LOGERROR("PlayState() - CreateScene NOK !");
        return false;
    }

    // Setting Nodes Scene / World Scene
    // Default Zone for the moment
    Node* zoneNode = scene_->GetChild("Zone");
    if (!zoneNode)
    {
        zoneNode = scene_->CreateChild("Zone", LOCAL);
        zoneNode->SetTemporary(true);
        Zone* zone = zoneNode->CreateComponent<Zone>(LOCAL);
        zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));
        zone->SetAmbientColor(Color(0.05f, 0.1f, 0.15f));
        zone->SetFogColor(Color(0.1f, 0.2f, 0.3f));
        zone->SetFogStart(10.0f);
        zone->SetFogEnd(100.0f);
    }

    if (scene_->GetChild("World"))
        world_ = scene_->GetChild("World")->GetComponent<World2D>();

    if (!world_)
    {
        splash_->Stop();
        URHO3D_LOGERROR("PlayState() - CreateScene : No World Component !");
        return false;
    }

    if (!GameContext::Get().createModeOn_)
    {
        weather_ = new WeatherManager(context_);
        effects_ = new EffectsManager(context_);
    }

    URHO3D_LOGINFO("PlayState() - CreateScene : OK");
    return true;
}

void PlayState::EndScene()
{
    URHO3D_LOGINFO("PlayState() - EndScene ... ");

    // Dump RigidBodies
//    {
//            PODVector<RigidBody2D*> bodies;
//        rootScene_->GetComponents<RigidBody2D>(bodies, true);
//        URHO3D_LOGINFOF("PlayState() - EndScene ... Dump RigidBodies : nb bodies=%d", bodies.Size());
//        for (unsigned i = 0; i < bodies.Size(); ++i)
//        {
//            URHO3D_LOGINFOF("-> Node name=%s id=%u", bodies[i]->GetNode()->GetName().CString(), bodies[i]->GetNode()->GetID());
//    //        bodies[i]->GetBody()->Dump();
//        }
//    }

    UnsubscribeToNetworkEvents();
    UnsubscribeToEvents();

    SetVisibleUI(false);

    SendEvent(TEXTMESSAGE_CLEAN);

    // Stop Players
    for (unsigned i=0; i < localPlayers_.Size(); ++i)
    {
        localPlayers_[i]->Stop();
    }

    ViewManager::Get()->SetMiniMapEnable(false);

    // Stop Managers
    GameContext::Get().rootScene_->SendEvent(GAME_REMOVESCENE);

    PathFinder2D::Stop();
    AIManager::StopManagers();
    goManager->Stop();
    Actor::RemoveActors();

    if (!GameContext::Get().createModeOn_)
    {
        if (weather_)
            weather_->Stop();
        if (effects_)
            effects_->Stop();
    }

    if (ViewManager::Get())
        ViewManager::Get()->SwitchToViewZ(FRONTVIEW, 0);

    if (world_)
        world_->Stop();

    URHO3D_LOGINFO("PlayState() - EndScene ... Stop Events & Managers OK ! ...");

    if (GameNetwork::Get())
    {
        GameNetwork::Get()->ClearScene();
        if (GameContext::Get().ServerMode_ && GameContext::Get().rootScene_)
            GameContext::Get().rootScene_->CleanupNetwork();
    }

    GameHelpers::CleanScene(GameContext::Get().rootScene_, GetStateId(), 0);

    GetSubsystem<Input>()->ResetStates();

    splash_.Reset();

    URHO3D_LOGINFO("PlayState() - EndScene ... OK !");

    SetStatus(PLAYSTATE_INITIALIZING);
}


void PlayState::SetGamePause()
{
    URHO3D_LOGINFOF("----------------------------");
    URHO3D_LOGINFOF("- PlayState() - GamePause ! -");
    URHO3D_LOGINFOF("----------------------------");

    paused_ = true;
    Localization* l10n = GetSubsystem<Localization>();

    if (!messageBox)
        messageBox = new Urho3D::MessageBox(context_);

    if (messageBox->GetWindow())
    {
        messageBox->SetTitle(l10n->Get("playquit") + "?");
        Button* yesButton = (Button*)messageBox->GetWindow()->GetChild("OkButton", true);
        ((Text*)yesButton->GetChild("Text", false))->SetText(l10n->Get("yes"));

        Button* noButton = (Button*)messageBox->GetWindow()->GetChild("CancelButton", true);
        ((Text*)noButton->GetChild("Text", false))->SetText(l10n->Get("no"));
        noButton->SetVisible(true);
        noButton->SetFocus(true);

        GameHelpers::ToggleModalWindow(messageBox, messageBox->GetWindow());

        SubscribeToEvent(messageBox, E_MESSAGEACK, URHO3D_HANDLER(PlayState, OnContinueMessageAck));
    }
}

void PlayState::SetGameOver()
{
    if (!gameOverMessage)
    {
        activeGameLogic_ = false;
//        UnsubscribeFromEvent(E_UPDATE);

        URHO3D_LOGINFOF("----------------------------");
        URHO3D_LOGINFOF("- PlayState() - GameOver ! -");
        URHO3D_LOGINFOF("----------------------------");

        scene_->SendEvent(GAME_STOP);
        SendEvent(TEXTMESSAGE_CLEAN);

        int loglevel = GameHelpers::GetGameLogLevel();
        GameHelpers::SetGameLogLevel(loglevel);

        // prevent bug when gameover and killer is killed by another entity
        UnsubscribeFromEvent(GOC_LIFEDEAD);

        GameHelpers::StopMusic(rootScene_);
        GameHelpers::PlayMusic(rootScene_, "Music/Oups.ogg", false);

        gameOverMessage = GameHelpers::ShowUIMessage("gameover", " !", true, 120, IntVector2::ZERO, 1.f, 5.f, 3.f);
//        Localization* l10n = GetSubsystem<Localization>();
//        gameOverMessage->Set(l10n->Get("gameover"), GameContext::Get().txtMsgFont_, 120, 2.5f, IntVector2(0, 200), true, 1.f, 5.f);
        gameOverMessage->SetExpireEvent(GAME_OVER);
        SubscribeToEvent(GAME_OVER, URHO3D_HANDLER(PlayState, OnGameOver));

        if (lastKillerID_ != 0)
        {
            Node* nodeKiller = rootScene_->GetNode(lastKillerID_);
            if (nodeKiller && (nodeKiller->GetVar(GOA::FACTION).GetInt() != GO_None))
            {
                ViewManager::Get()->ResetFocus(0);
                ViewManager::Get()->AddFocus(nodeKiller, false, 0);
            }
        }
    }
}

void PlayState::SetGameWin()
{
    if (!winMessage)
    {
        activeGameLogic_ = false;
        UnsubscribeFromEvent(E_UPDATE);

        URHO3D_LOGINFOF("----------------------------");
        URHO3D_LOGINFOF("- PlayState() - GameWin ! -");
        URHO3D_LOGINFOF("----------------------------");

        scene_->SendEvent(GAME_STOP);
        SendEvent(TEXTMESSAGE_CLEAN);

        GameHelpers::StopMusic(rootScene_);
        GameHelpers::PlayMusic(rootScene_, "Music/gagne.wav", false);

        if (GameContext::Get().arenaZoneOn_)
            winMessage = GameHelpers::ShowUIMessage("gamewin", " !", true, 120, IntVector2::ZERO, 2.5f, 1.f, 0.f);
        else
            winMessage = GameHelpers::ShowUIMessage("nextlevel", " !", true, 120, IntVector2::ZERO, 1.f, 3.f, 3.f);

        if (winMessage)
        {
            winMessage->SetExpireEvent(GAME_WIN);
            SubscribeToEvent(GAME_WIN, URHO3D_HANDLER(PlayState, OnGameWin));
        }
    }
}


void PlayState::CreateUI()
{
    URHO3D_LOGINFO("PlayState() - CreateUI");

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    UI* ui = GetSubsystem<UI>();
    // Load XML file containing default UI style sheet
    XMLFile* style = cache->GetResource<XMLFile>("UI/DefaultStyle.xml");

    // Set the loaded style as default style
    ui->GetRoot()->SetDefaultStyle(style);

    // Local Players Status Zone
    if (playerStatusZone.Size() < GameContext::Get().numPlayers_)
    {
        // Update the viewport rects before set zones positions
        ViewManager::ResizeViewportRects(GameContext::Get().numPlayers_);

        int oldsize = playerStatusZone.Size();
        for (int i=oldsize; i < GameContext::Get().numPlayers_; ++i)
        {
            UIElement* playerzone = ui->GetRoot()->GetChild("playerStatusZone"+String(i));
            if (!playerzone)
                playerzone = ui->GetRoot()->CreateChild<UIElement>("playerStatusZone"+String(i));

            playerStatusZone.Push(playerzone);
        }
    }

#ifdef HANDLE_SCORES
    // Create HiScore Text
    if (!hiscoreText)
    {
        Font* font = cache->GetResource<Font>(GameContext::Get().txtMsgFont_);
        hiscoreText = ui->GetRoot()->CreateChild<Text>();
        hiscoreText->SetText(String(hiScore));
        hiscoreText->SetFont(font, 30);
        hiscoreText->SetAlignment(HA_CENTER, VA_TOP);
        hiscoreText->SetPosition(-5, 5);
        hiscoreText->SetColor(C_BOTTOMLEFT, Color(1, 1, 0.25));
        hiscoreText->SetColor(C_BOTTOMRIGHT, Color(1, 1, 0.25));
    }
#endif

    SetVisibleUI(false);
}

void PlayState::SetVisibleUI(bool state)
{
    if (GameContext::Get().createModeOn_)
        state = false;

    OptionState* optionState = (OptionState*)GameContext::Get().stateManager_->GetState("Options");
    if (optionState)
    {
        optionState->SetVisibleAccessButton(state);
#ifdef ACTIVE_CREATEMODE
        if (GameContext::Get().createModeOn_ && !state && optionState->IsVisible())
            optionState->Hide();
#endif
    }

#ifdef HANDLE_SCORES
    if (hiscoreText)
        hiscoreText->SetVisible(state && localPlayers_.Size() > 0);
#endif

    // UIC MiniMap
    ViewManager::Get()->SetMiniMapEnable(state);

    for (unsigned i = 0; i < localPlayers_.Size(); ++i)
    {
        bool enable = state;
        if (!localPlayers_[i]->active)
            enable = false;
        localPlayers_[i]->SetVisibleUI(enable, !enable);
    }

    gameUIVisible_ = state;
}

void PlayState::RemoveUI()
{
    URHO3D_LOGINFO("PlayState() - RemoveUI ... ");

    UIElement* root = GetSubsystem<UI>()->GetRoot();
    if (!root)
        return;

#ifdef HANDLE_SCORES
    if (hiscoreText)
    {
        root->RemoveChild(hiscoreText);
        hiscoreText.Reset();
    }
#endif // HANDLE_SCORES

    root->DisableLayoutUpdate();

    URHO3D_LOGINFO("PlayState() - RemoveUI ... 1 ...");
    for (unsigned i=0; i<localPlayers_.Size(); ++i)
    {
        if (localPlayers_[i])
            localPlayers_[i]->RemoveUI();
    }

    URHO3D_LOGINFOF("PlayState() - RemoveUI ... 2 ... : playerStatusZone size=%u zone0=%u", playerStatusZone.Size(), playerStatusZone.Size() ? playerStatusZone[0] : 0);
    for (unsigned i=0; i<playerStatusZone.Size(); ++i)
    {
        if (playerStatusZone[i])
            root->RemoveChild(playerStatusZone[i]);
    }

    playerStatusZone.Clear();

    root->EnableLayoutUpdate();

    root->UpdateLayout();

    URHO3D_LOGINFO("PlayState() - RemoveUI ... OK !");
}

void PlayState::ResizeUI()
{
    URHO3D_LOGINFO("PlayState() - ResizeUI ... ");

    for (unsigned i=0; i < localPlayers_.Size(); ++i)
    {
        localPlayers_[i]->ResizeUI();
    }

    URHO3D_LOGINFO("PlayState() - ResizeUI ... OK !");
}

void PlayState::SaveGame()
{
    URHO3D_LOGINFO("PlayState() - SaveGame : ...");

    // desactive controllers/behaviors before saving
    for (unsigned i=0; i<localPlayers_.Size(); ++i)
        if (localPlayers_[i]->active)
        {
            localPlayers_[i]->gocController->Stop();
        }
//        if (localPlayers_[i]->active && localPlayers_[i]->gocController->GetControllerType() != GO_Player)
//        {
//            GOC_AIController* controller = static_cast<GOC_AIController*>(localPlayers_[i]->gocController);
//            controller->StopBehavior();
//        }

    // save each active player
    for (unsigned i=0; i<localPlayers_.Size(); ++i)
    {
        GameContext::Get().playerState_[i].active = localPlayers_[i]->active;
        GameContext::Get().playerState_[i].score = localPlayers_[i]->score;

        if (localPlayers_[i]->active)
        {
            URHO3D_LOGINFOF("PlayState() - SaveGame : ... Save Local Player index=%d ...", i);
            localPlayers_[i]->SaveAll();
            URHO3D_LOGINFOF("PlayState() - SaveGame : ... Save Local Player index=%d ... OK ...", i);
        }
    }

    // save game states
    {
        URHO3D_LOGINFO("PlayState() - SaveGame : ... Save Game States ...");
        GameHelpers::SaveData(context_, &GameContext::Get().gameState_, sizeof(GameContext::GameState), String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedGameFile_).CString());
        GameContext::Get().gameState_.Dump();
        URHO3D_LOGINFO("PlayState() - SaveGame : ... Save Game States ... OK !");
    }

    // save scene
    if (scene_)
    {
        URHO3D_LOGINFO("PlayState() - SaveGame : ... Save Scene ...");
        GameHelpers::SaveNodeXML(context_, scene_, String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedSceneFile_).CString());
        URHO3D_LOGINFO("PlayState() - SaveGame : ... Save Scene ... OK ...");
    }

    // save mapworld
    if (world_)
    {
        URHO3D_LOGINFO("PlayState() - SaveGame : ... Save World ...");
        world_->SaveWorld(true);
        URHO3D_LOGINFO("PlayState() - SaveGame : ... Save World ... OK ...");
    }

    // reactive controllers/behaviors
    for (unsigned i=0; i<localPlayers_.Size(); ++i)
        if (localPlayers_[i]->active)
        {
            localPlayers_[i]->gocController->Start();
        }

//    for (unsigned i=0; i<localPlayers_.Size();++i)
//        if (localPlayers_[i]->active && localPlayers_[i]->gocController->GetControllerType() != GO_Player)
//        {
//            GOC_AIController* controller = static_cast<GOC_AIController*>(localPlayers_[i]->gocController);
//            controller->RestartLastBehavior();
//        }

    URHO3D_LOGINFO("PlayState() - SaveGame : ... OK !");
}

void PlayState::AllocatePlayers()
{
    if (GameContext::Get().numPlayers_ != localPlayers_.Size())
    {
        URHO3D_LOGINFOF("PlayState() - AllocatePlayers  ... GameContext::Get().numPlayers_=%d", GameContext::Get().numPlayers_);

        if (GameContext::Get().numPlayers_ < localPlayers_.Size())
        {
            for (int i=GameContext::Get().numPlayers_; i < localPlayers_.Size(); ++i)
            {
                localPlayers_[i].Reset();
                GameContext::Get().playerState_[i].active = false;
            }
        }

        localPlayers_.Resize(GameContext::Get().numPlayers_);

        for (int i=0; i < localPlayers_.Size(); ++i)
        {
            if (!localPlayers_[i])
                localPlayers_[i] = SharedPtr<Player>(new Player(context_, i+1));

            // Reactive Player
            GameContext::Get().playerState_[i].active = true;

            // Set Default Connection : on server and mainController actived
            localPlayers_[i]->SetController(true);
            localPlayers_[i]->active = true;
        }
    }
}

void PlayState::SetPlayers(bool init, bool restart)
{
    URHO3D_LOGINFOF("PlayState() - SetPlayers  ... init=%s restart=%s load=%s numplayers=%u", init ? "true":"false", restart ? "true":"false", toLoadGame_ ? "true":"false", localPlayers_.Size());

    int clientid = GameContext::Get().ClientMode_ ? GameNetwork::Get()->GetClientID() : 0;

    if (init)
    {
        unsigned firstavatarnodeid = GameContext::Get().FIRSTAVATARNODEID + clientid * GameContext::Get().MAX_NUMPLAYERS;
        for (unsigned i=0; i < localPlayers_.Size(); ++i)
        {
            URHO3D_LOGINFOF("PlayState() - SetPlayers : clientID=%d PlayerIndex=%u ... NodeID=%u !", clientid, i, firstavatarnodeid+i);
            localPlayers_[i]->SetController(true, clientid ? GameNetwork::Get()->GetConnection() : 0, firstavatarnodeid+i, clientid);
        }
    }

    // Set Start Position for all
    GameContext::Get().SetWorldStartPosition();

    // Get Active players
    PODVector<int> localActivePlayersIndexes;
    for (int i=0; i < localPlayers_.Size(); ++i)
    {
        Player* player = localPlayers_[i];

        if (!player->active && (GameContext::Get().playerState_[i].active || restart))
            player->active = true;

        if (player->active)
            localActivePlayersIndexes.Push(i);
    }

    // Set local player's avatars
    unsigned numLocalActivePlayers = localActivePlayersIndexes.Size();
    WorldMapPosition startposition;
    for (unsigned i=0; i < numLocalActivePlayers; i++)
    {
        int playerindex = localActivePlayersIndexes[i];
        Player* player = localPlayers_[playerindex];

        // Set Start Position

//        if (GameContext::Get().ClientMode_)
//        {
//            const Vector<ObjectControlInfo>& cinfos = GameNetwork::Get()->GetClientObjectControls();
//            if (i < cinfos.Size())
//            {
//                const ObjectControl& control = cinfos[i].GetReceivedControl();
//                startposition.position_ = Vector2(control.physics_.positionx_, control.physics_.positiony_);
//                startposition.viewZ_ = control.states_.viewZ_;
//
//                URHO3D_LOGINFOF("PlayState() - SetPlayers ... index=%d ClientMode => startPosition=%s ViewZ=%d ...",
//                                i, startposition.position_.ToString().CString(), startposition.viewZ_);
//            }
//        }
//        else
        {
            if (!GameContext::Get().arenaZoneOn_ || localPlayers_.Size() == 1 || !GameContext::Get().gameConfig_.multiviews_)
                startposition = GameContext::Get().worldStartPosition_;
            else
                bool result = GameContext::Get().GetStartPosition(startposition, clientid+playerindex);
        }

        // Set Players => Player::Set(UIElement* elem, bool missionEnable, unsigned id, const char *name)
        player->Set(playerStatusZone[playerindex], playerindex==0 && GameContext::Get().enableMissions_, numLocalActivePlayers > 1, playerindex);
        player->SetScene(rootScene_, startposition.position_, startposition.viewZ_, toLoadGame_, init, restart);

        // Set Faction
        unsigned faction = GO_Player;

        if (GameContext::Get().LocalMode_)
            faction = GameContext::Get().arenaZoneOn_ ? (playerindex << 8) + GO_Player : GO_Player;
        // always together
//            faction = GO_Player;
        else
            faction = (clientid << 8) + GO_Player;

        player->SetFaction(faction);

        if (GameContext::Get().ServerMode_)
        {
            unsigned nodeid = player->GetAvatar()->GetID();
            ObjectControlInfo& objectControlInfo = GameNetwork::Get()->GetOrCreateServerObjectControl(nodeid, nodeid, 0, player->GetAvatar());
        }
    }

    SetVisibleUI(false);

    URHO3D_LOGINFOF("PlayState() - SetPlayers ... num local active players = %u OK !", numLocalActivePlayers);

    if (!numLocalActivePlayers)
        startposition = GameContext::Get().worldStartPosition_;

#ifdef ACTIVE_CREATEMODE
    const bool editoron = MapEditor::Get();
#else
    const bool editoron = false;
#endif
    SetViewports(true);
    if (!numLocalActivePlayers || editoron)
    {
        GameContext::Get().cameraNode_->SetPosition2D(startposition.position_);
        ViewManager::Get()->SwitchToViewZ(startposition.viewZ_);
        World2D::GetWorld()->UpdateInstant(0, startposition.position_, 1.f);
    }
#if defined(ACTIVE_INITIALDEZOOMONSTART)
    const float zoom = 0.1f;
#else
    const float zoom = editoron ? 0.1f : 1.f;
#endif
    GameContext::Get().camera_->SetZoom(zoom);

    ResizeUI();

    UpdateNumActivePlayers();
    rankedCliendIDs_.Clear();

    URHO3D_LOGINFOF("PlayState() - SetPlayers ... OK !");
}

// Network Usage
void PlayState::ResetPlayers()
{
    URHO3D_LOGINFO("PlayState() - ResetPlayers  ... ");

    // Reactive Players
    for (int i=0; i < GameContext::Get().numPlayers_; ++i)
        GameContext::Get().playerState_[i].active = true;
    for (int i=GameContext::Get().numPlayers_; i < GameContext::Get().MAX_NUMPLAYERS; ++i)
        GameContext::Get().playerState_[i].active = false;

    // Reset Players
    SetPlayers(false, true);

    URHO3D_LOGINFOF("PlayState() - ResetPlayers ... numlocalplayers=%u OK !", GameContext::Get().numPlayers_);
}

void PlayState::RemovePlayers()
{
    URHO3D_LOGINFOF("PlayState() - RemovePlayers ...");

    localPlayers_.Clear();

    URHO3D_LOGINFOF("PlayState() - RemovePlayers ... OK !");
}

#define ACTIVE_PLAYERCPU_VIEWPORTS

void PlayState::SetViewports(bool force)
{
    PODVector<unsigned> activeplayerids;

    for (unsigned i=0; i < localPlayers_.Size(); ++i)
    {
#ifndef ACTIVE_PLAYERCPU_VIEWPORTS
        if (localPlayers_[i] && localPlayers_[i]->GetAvatar() && localPlayers_[i]->active && GameContext::Get().playerState_[i].controltype != CT_CPU)
#else
        if (localPlayers_[i] && localPlayers_[i]->GetAvatar() && localPlayers_[i]->active)
#endif
            activeplayerids.Push(i);
    }

    unsigned numviewports = 1;
    if (GameContext::Get().gameConfig_.multiviews_)
        numviewports = Min(activeplayerids.Size(), MAX_VIEWPORTS);

    URHO3D_LOGINFOF("PlayState() - SetViewports : ... numviewports=%u ", numviewports);

    ViewManager::Get()->SetViewportLayout(numviewports, force);
    ViewManager::Get()->ResizeViewports();
    ViewManager::Get()->ResetFocus();

    bool worldpositionsetted = false;
    Vector2 worldposition;

    for (unsigned i=0; i < activeplayerids.Size(); ++i)
    {
        Player* player = localPlayers_[activeplayerids[i]];

        if (numviewports > 1 || !GameContext::Get().arenaZoneOn_ || i == 0)
        {
            int controlid = player->GetControlID();
            ViewManager::Get()->AddFocus(player->GetAvatar(), true, controlid < numviewports ? controlid : 0);
            URHO3D_LOGINFOF("PlayState() - SetViewports : localPlayer ID=%u ControlID=%d ... ", player->GetID(), controlid);
        }

        if (!worldpositionsetted)
        {
            worldpositionsetted = true;
            worldposition = player->GetAvatar()->GetWorldPosition2D();
        }

        URHO3D_LOGINFOF("PlayState() - SetViewports : localPlayer ID=%u focused at %s !",
                        player->GetID(), player->GetAvatar()->GetWorldPosition2D().ToString().CString());
    }

    if (!GameContext::Get().gameConfig_.multiviews_ || !activeplayerids.Size())
    {
        World2D::GetWorld()->UpdateInstant(0, worldposition, 1.f);
    }
    else
    {
        for (unsigned i=0; i < activeplayerids.Size(); ++i)
        {
            Player* player = localPlayers_[activeplayerids[i]];

            URHO3D_LOGINFOF("PlayState() - SetViewports : localPlayer ID=%u Update View ... ", player->GetID());
            World2D::GetWorld()->UpdateInstant(player->GetControlID(), player->GetAvatar()->GetWorldPosition2D(), 1.f);
            URHO3D_LOGINFOF("PlayState() - SetViewports : localPlayer ID=%u Update View ... OK !", player->GetID());
        }
    }

    URHO3D_LOGINFOF("PlayState() - SetViewports : ... OK !");
}


void PlayState::SubscribeToEvents()
{
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");
    URHO3D_LOGINFO("PlayState() - SubscribeToEvents ! ");
    URHO3D_LOGINFO("PlayState() - ---------------------------------------");

    // Subscribe HandleUpdate() function for processing update events
    SubscribeToEvent(E_UPDATE, URHO3D_HANDLER(PlayState, HandleUpdate));

    SubscribeToEvent(GAME_PLAYERDIED, URHO3D_HANDLER(PlayState, OnPlayerDied));

#ifdef HANDLE_SCORES
    if (!GameContext::Get().createModeOn_)
    {
        SubscribeToEvent(GOC_LIFEDEAD, URHO3D_HANDLER(PlayState, HandleUpdateScores));
    }
#endif

    if (!GameContext::Get().createModeOn_)
    {
        weather_->Start();
        SendEvent(WORLD_CAMERACHANGED);
    }

//    if (GameContext::Get().DrawDebug_)
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(PlayState, OnPostRenderUpdate));
}

void PlayState::UnsubscribeToEvents()
{
    UnsubscribeFromEvent(E_UPDATE);

#ifdef HANDLE_SCORES
    UnsubscribeFromEvent(GOC_LIFEDEAD);
#endif
#ifdef ACTIVE_TIPMODE
    UnsubscribeFromEvent(E_MOUSEBUTTONUP);
#endif

//    if (GameContext::Get().DrawDebug_)
        UnsubscribeFromEvent(E_POSTRENDERUPDATE);
}

void PlayState::SubscribeToNetworkEvents()
{
    if (GameNetwork::Get())
    {
        if (GameContext::Get().ServerMode_)
        {
            SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PlayState, OnDelayedActions_Server));
        }
        else
        {
            SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PlayState, OnDelayedActions_Client));
        }
    }
    else
    {
        SubscribeToEvent(E_POSTUPDATE, URHO3D_HANDLER(PlayState, OnDelayedActions_Local));
    }
}

void PlayState::UnsubscribeToNetworkEvents()
{
    UnsubscribeFromEvent(E_POSTUPDATE);
}


/// Update world Loading
/// When finish to load, set players and subscribe to events
static bool sPlayerSetted_ = false;

void PlayState::HandleInitialize(StringHash eventType, VariantMap& eventData)
{
    URHO3D_PROFILE(PlayState_Initialize);

    if (!world_)
        return;

    if (gameStatus_ == PLAYSTATE_LOADING)
    {
        if (!world_->UpdateLoading())
            return;

        URHO3D_LOGINFO("--------------------------------------------------------");
        URHO3D_LOGINFOF("- PlayState() - HandleInitialize : World Loading in %dsec ... OK ! -", loadingTimer_.GetMSec(false) / 1000);
        URHO3D_LOGINFO("--------------------------------------------------------");

        // Reset Loading Delay
        world_->SetUpdateLoadingDelay();

        SetStatus(PLAYSTATE_FINISHLOADING);
    }

    if (gameStatus_ == PLAYSTATE_FINISHLOADING)
    {
        sPlayerSetted_ = false;

        // allocate players if need
        AllocatePlayers();

        SubscribeToNetworkEvents();
        GameHelpers::PlayMusic(rootScene_, GameContext::Get().currentMusic, true, 0.4f);

        SetStatus(GameContext::Get().ClientMode_ ? PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS : PLAYSTATE_SYNCHRONIZING);
    }

    // Client Mode : Waiting for the server objects ids
    if (gameStatus_ == PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS)
    {
//        URHO3D_LOGINFO("PlayState() - HandleInitialize : Client is waiting Server Infos ...");

        // Network Breaking during loading : Reset to local
        if (!GameNetwork::Get())
        {
            ReloadLevel();
            return;
        }

#ifdef SCENE_REPLICATION_ENABLE
        if (GameNetwork::Get()->GetGameStatus() < PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES)
            return;

        if (GameContext::Get().ClientMode_)
            GameNetwork::Get()->GetConnection()->SetScene(GameContext::Get().rootScene_);

        SetStatus(PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES);
#else
        SetStatus(PLAYSTATE_SYNCHRONIZING);
#endif
    }

#ifdef SCENE_REPLICATION_ENABLE
    // Client Mode : Waiting for the scene loaded
    if (gameStatus_ == PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES)
    {
        // Network Breaking during loading : Reset to local
        if (!GameNetwork::Get())
        {
            ReloadLevel();
            return;
        }

        if (!GameNetwork::Get()->GetConnection()->IsSceneLoaded())
            return;

        SetStatus(PLAYSTATE_SYNCHRONIZING);
    }
#endif

    if (gameStatus_ == PLAYSTATE_SYNCHRONIZING)
    {
        // Client Mode : Wait For All Clients have loaded the Scene
        if (GameContext::Get().ClientMode_ && GameNetwork::Get()->GetGameStatus() <= PLAYSTATE_SYNCHRONIZING)
        {
//            URHO3D_LOGINFO("PlayState() - HandleInitialize : Client is synchronizing ...");
            GameNetwork::Get()->Client_SendGameStatus();
            return;
        }

        URHO3D_LOGINFO("PlayState() - HandleInitialize : Client synchronized ...");

        // Set Players
        if (!sPlayerSetted_)
        {
            SetPlayers(initMode_, restartMode_);
            sPlayerSetted_ = true;
        }

        // Set the visibility of the maps
        if (!world_->UpdateVisibility(eventData[Update::P_TIMESTEP].GetFloat()))
        {
            URHO3D_LOGINFO("PlayState() - HandleInitialize : World is not visible ... wait ...");
            return;
        }

//        GameContext::Get().DumpNode(GameContext::Get().rootScene_, true, false);

        UnsubscribeFromEvent(E_UPDATE);

        if (initMode_)
            toLoadGame_ = GameContext::Get().loadSavedGame_ = false;

        // Start Managers
        AIManager::StartManagers();
        PathFinder2D::Init(context_, world_->GetWorldInfo());

#ifdef HANDLE_BACKGROUND_LAYERS
        DrawableScroller::Start();
#endif

        if (!GameContext::Get().createModeOn_)
        {
            weather_->Start();
            effects_->Start();
        }

        rootScene_->StopAsyncLoading();
        // Set timescale to 1.f to prevent bug on client when a message for a scene node is received with a timescale=0.f
        rootScene_->SetTimeScale(1.f);

        SubscribeToEvents();

        // Restart the physics
        rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(!GameContext::Get().createModeOn_);

        URHO3D_LOGINFO("------------------------------------------------------------");
        URHO3D_LOGINFO("- PlayState() - HandleInitialize : ... OK ! -");
        URHO3D_LOGINFO("------------------------------------------------------------");

        SetStatus(PLAYSTATE_READY, GameContext::Get().ClientMode_);

        SendEvent(WORLD_FINISHLOADING);

        if (!GameContext::Get().createModeOn_)
        {
#ifndef ACTIVE_SPLASHUI
            SendEvent(SPLASHSCREEN_FINISH);
#endif
        }
        else
        {
            SendEvent(SPLASHSCREEN_FINISH);
        }

        if (!GameContext::Get().createModeOn_)
        {
            // Unlock Mouse Visibility On False
            GameContext::Get().SetMouseVisible(true, false, false);
            GameContext::Get().SetMouseVisible(false, false, false);
        }

        if (GameContext::Get().ClientMode_)
        {
            // Allow Receive ObjectControl from server only at this moment
            GameNetwork::Get()->SetEnabledServerObjectControls(GameNetwork::Get()->GetConnection(), true);
            GameNetwork::Get()->GetConnection()->SynchronizeObjectCommands();
        }
    }
}

#ifdef PLAYSTATE_STOPATFIRSTFRAME
bool firstFrame_ = true;
#endif

/// Appear Players
/// When SplashScreen finished
void PlayState::HandleAppearPlayer(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("PlayState() - HandleAppearPlayer : ...");

    UnsubscribeFromEvent(SPLASHSCREEN_FINISH);

    int numactiveplayers = 0;

    // start local players
    for (unsigned i=0; i < localPlayers_.Size(); ++i)
    {
        Player* player = localPlayers_[i];
        if (!player)
        {
            URHO3D_LOGERRORF("PlayState() - HandleAppearPlayer : No Local Player i=%u", i);
            continue;
        }

        URHO3D_LOGINFOF("PlayState() - HandleAppearPlayer : Start Local Player i=%u", i);
        if (player->active)
        {
            player->Start();

            Node* avatar = player->GetAvatar();
            if (!avatar)
            {
                URHO3D_LOGWARNINGF("PlayState() - HandleAppearPlayer : localPlayer index=%u ActorID=%u NodeID=%u => has no avatar !", i, player->GetID(), player->nodeID_);
//				player->active = false;
                continue;
            }

            numactiveplayers++;

            if (!GameContext::Get().LocalMode_)
                GameNetwork::Get()->SetEnableObject(true, player->GetAvatar()->GetID());

            URHO3D_LOGINFOF("PlayState() - HandleAppearPlayer : localPlayer ID=%u appears at %s !",
                            player->GetID(), avatar->GetWorldPosition2D().ToString().CString());
        }
    }

    // Set Viewport layout & Player Camera Focus
#ifdef ACTIVE_INITIALDEZOOMONSTART
    if (numactiveplayers > 0)
        SetViewports();
#endif

    SetVisibleUI(true);

#ifdef ACTIVE_CREATEMODE
    if (GameContext::Get().createModeOn_)
    {
        if (!MapEditor::Get())
            MapEditor::Toggle();
    }
#endif

    if (!GameContext::Get().createModeOn_)
    {
#ifdef CAMERA_TESTMODE
        zoom = 0.3f;
        ViewManager::Get()->GetCamera()->SetZoom(zoom);
#endif
#ifdef CAMERA_TESTMODE_ULTRA
        zoom = 0.05f;
        ViewManager::Get()->GetCamera()->SetZoom(zoom);
#endif
        URHO3D_LOGINFOF("PlayState() - HandleAppearPlayer : numActivePlayers=%u !", GOManager::GetNumActivePlayers());

        levelText_ = GameHelpers::ShowUIMessage(GameContext::Get().sceneToLoad_[GameContext::Get().indexLevel_-1][2], String::EMPTY, false, 120, IntVector2::ZERO, 1.f, 2.3f);
        if (GameContext::Get().arenaZoneOn_)
            levelGOText_ = GameHelpers::ShowUIMessage3D(GOManager::GetNumActivePlayers() > 1 ? "killallplayers" : "killallenemies", " !", true, 100, IntVector2::ZERO, 3.f, 4.f, 2.5f);
        else
            levelGOText_ = GameHelpers::ShowUIMessage3D("levelgo", " !", true, 100, IntVector2::ZERO, 3.f, 4.f, 2.5f);

        goManager->Dump();
        SubscribeToEvent(this, GAME_START, URHO3D_HANDLER(PlayState, OnGameStart));
        DelayInformer* delayedGameStart = new DelayInformer(this, 1.f, GAME_START);
    }

    URHO3D_LOGINFOF("PlayState() - HandleAppearPlayer : Level = %s worldSeed=%u OK !", GameContext::Get().sceneToLoad_[GameContext::Get().indexLevel_-1][2], worldSeed_);

    URHO3D_LOGINFOF("PlayState() - HandleAppearPlayer : ... OK !");

    if (!GameContext::Get().ServerMode_)
    {
        SetStatus(PLAYSTATE_RUNNING);
        GameContext::Get().AllowUpdate_ = true;
    }

//    OptionState* optionState = (OptionState*)GameContext::Get().stateManager_->GetState("Options");
//    if (optionState)
//    {
//        optionState->CloseFrame();
//    }
}

void PlayState::HandleUpdateScores(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GOC_LIFEDEAD)
    {
        unsigned ID_Dead = eventData[GOC_Life_Events::GO_ID].GetUInt();
        unsigned ID_Killer = eventData[GOC_Life_Events::GO_KILLER].GetUInt();
        URHO3D_LOGINFOF("PlayState() - HandleUpdateScores : Dead Entity ID=%u, killed by ID=%u", ID_Dead, ID_Killer);

        if (ID_Killer != 0)
        {
            GOC_PlayerController* gocPlayer = 0;
            Node* nodeKiller = rootScene_->GetNode(ID_Killer);

            if (nodeKiller)
                gocPlayer = nodeKiller->GetComponent<GOC_PlayerController>();

            if (gocPlayer)
            {
                TextMessage* scoreText = new TextMessage(context_);
                scoreText->Set(rootScene_->GetNode(ID_Dead),ToString("%dpts",1000), "Fonts/jokerman24pts.sdf", 24, 3.f);

                Player* playerPtr = localPlayers_[gocPlayer->playerID];
                playerPtr->UpdatePoints(1000);

                // Update HiScore
                if (playerPtr->score > hiScore)
                {
                    hiScore = playerPtr->score;
                    hiscoreText->SetText(String(hiScore));
                }
            }
            else
                lastKillerID_ = ID_Killer;
        }
    }
}


void SetRenderShapeVisibilityAt(int viewZcutted, const PODVector<RenderShape*>& rendershapes)
{
    if (!rendershapes.Size())
        return;

    for (unsigned i = 0; i < rendershapes.Size(); ++i)
    {
        RenderShape* rendershape = rendershapes[i];
        rendershape->SetEnabled(rendershape->GetLayer() >= viewZcutted-LAYER_RENDERSHAPE && rendershape->GetLayer() <= viewZcutted+LAYER_RENDERSHAPE);
    }
}

void TextTest(Context* context, Node* rootnode, const Vector3& position, const String& message, const String& fontname, float duration, float fadescale, int fontsize)
{
    Font* font = context->GetSubsystem<ResourceCache>()->GetResource<Font>(fontname);
    Vector3 scale = Vector3::ONE / rootnode->GetWorldScale();

    Node* node = rootnode->CreateChild("Text3D");
    node->SetEnabled(false);
    Text3D* text3D = node->CreateComponent<Text3D>();
    text3D->SetEnabled(false);
    text3D->SetText(message);
    text3D->SetFont(font, fontsize);
    text3D->SetAlignment(HA_CENTER, VA_CENTER);

    SharedPtr<ObjectAnimation> textAnimation(new ObjectAnimation(context));
    SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context));
    alphaAnimation->SetKeyFrame(0.f, 0.f);
    alphaAnimation->SetKeyFrame(0.1f*duration, 1.f);
    alphaAnimation->SetKeyFrame(0.35f*duration, 1.f);
    alphaAnimation->SetKeyFrame(0.5f*duration, 0.f);
    alphaAnimation->SetKeyFrame(duration, 0.f);
    textAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
    text3D->SetObjectAnimation(textAnimation);

    bool translate = true;
    if (translate || fadescale != 1.f)
    {
        SharedPtr<ObjectAnimation> nodeAnimation(new ObjectAnimation(context));
        if (translate)
        {
            Vector3 finalposition = position + Vector3(0.f, 1.f, 0.f) / rootnode->GetWorldScale();
            SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(context));
            positionAnimation->SetKeyFrame(0.f, position);
            positionAnimation->SetKeyFrame(0.5f*duration, finalposition);
            positionAnimation->SetKeyFrame(duration, finalposition);
            nodeAnimation->AddAttributeAnimation("Position", positionAnimation, WM_ONCE);
        }
        if (fadescale != 1.f)
        {
            SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context));
            scaleAnimation->SetKeyFrame(0.f, Vector3::ZERO);
            scaleAnimation->SetKeyFrame(0.5f*duration, fadescale * scale);
            scaleAnimation->SetKeyFrame(duration, fadescale * scale);
            nodeAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
        }
        node->SetObjectAnimation(nodeAnimation);
    }

    node->SetEnabled(true);
    text3D->SetEnabled(true);
    node->SetScale(scale);
    node->SetPosition(position);
}


static bool sRenderShapeCuttingOn_ = false;

void PlayState::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    // Take the frame time step, which is stored as a float
    float timeStep = eventData[P_TIMESTEP].GetFloat();

    // Game Logic
    CheckGameLogic();

    // Start Pause Tip
#ifdef PLAYSTATE_STOPATFIRSTFRAME
    if (firstFrame_)
    {
        rootScene_->SetUpdateEnabled(false);
        SetGamePause();
        firstFrame_ = false;
    }
    else
#endif
    {
        URHO3D_PROFILE(PlayState_UpdateWorld);

        // Update Camera and World
        HandleCamera(timeStep);
        world_->UpdateStep(timeStep);
    }

    Input& input = *GameContext::Get().input_;

    // touchEnabled KEY_SELECT Settings
    if (input.GetKeyPress(KEY_SELECT) && GameContext::Get().gameConfig_.touchEnabled_ && GameContext::Get().gameConfig_.screenJoystick_)
    {
        input.SetScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoysticksettingsID_, !input.IsScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoysticksettingsID_));
        return;
    }

    // Toggle Camera Control
    if (GameContext::Get().gameConfig_.ctrlCameraEnabled_)
    {
        if (input.GetMouseButtonDown(MOUSEB_LEFT) && input.GetKeyDown(KEY_TAB))
        {
            if (!ctrlCameraWithMouse_)
            {
                GameContext::Get().ResetScreen();
                GameContext::Get().cameraNode_->SetPosition(Vector3(GameContext::Get().cameraNode_->GetPosition2D(), -50.f));
                GameContext::Get().camera_->SetOrthographic(false);
                ctrlCameraWithMouse_ = true;

                URHO3D_LOGINFO("Use Mouse to Control Camera !");
            }

            // Move camera with Mouse
            IntVector2 mouseMove = input.GetMouseMove();
            cameraYaw_ += 0.1f * mouseMove.x_;
            cameraPitch_ += 0.1f * mouseMove.y_;
            cameraPitch_ = Clamp(cameraPitch_, -90.f, 90.f);
            GameContext::Get().cameraNode_->SetRotation(Quaternion(cameraPitch_, cameraYaw_, 0.f));
        }
        else if (ctrlCameraWithMouse_)
        {
            ctrlCameraWithMouse_ = false;

            // Reset camera's position
            cameraYaw_ = 0.f;
            cameraPitch_ = 0.f;
            GameContext::Get().cameraNode_->SetRotation2D(0.f);
            GameContext::Get().cameraNode_->SetPosition(Vector3(GameContext::Get().cameraNode_->GetPosition2D(), -10.f));
            GameContext::Get().camera_->SetOrthographic(true);
            GameContext::Get().renderer2d_->UpdateFrustumBoundingBox(GameContext::Get().camera_);

            URHO3D_LOGINFOF("Reset Camera Position cameraNode=%s fbox=%s !", GameContext::Get().cameraNode_->GetPosition().ToString().CString(), GameContext::Get().renderer2d_->GetFrustumBoundingBox().ToString().CString());
        }
    }

    // Editor Mode
#ifdef ACTIVE_CREATEMODE
    if (input.GetKeyPress(KEY_F3) || (MapEditor::Get() && input.GetKeyPress(KEY_ESCAPE)))
    {
        MapEditor::Toggle();

        bool editorIsOff = !MapEditor::Get();

        if (editorIsOff)
        {
            if (weather_)
                weather_->Start();
            if (effects_)
                effects_->Start();
            for (Vector<SharedPtr<Player> >::ConstIterator it = localPlayers_.Begin(); it != localPlayers_.End(); ++it)
                it->Get()->StartSubscribers();
        }
        else
        {
            if (weather_)
                weather_->Stop();
            if (effects_)
                effects_->Stop();
            for (Vector<SharedPtr<Player> >::ConstIterator it = localPlayers_.Begin(); it != localPlayers_.End(); ++it)
                it->Get()->StopSubscribers();
        }

        SetVisibleUI(editorIsOff);
        rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(editorIsOff);

        GameContext::Get().forceCreateMode_ = !editorIsOff;
    }
#endif // ACTIVE_CREATEMODE

    // Tips Mode
#ifdef ACTIVE_TIPMODE
    if (!GameContext::Get().createModeOn_)
    {
        if (GameContext::Get().cursor_ && (input.GetMouseButtonDown(MOUSEB_LEFT|MOUSEB_RIGHT|MOUSEB_MIDDLE) || input.GetKeyDown(KEY_LALT)))
        {
            int x, y;
            UIElement* uielt;
            GameHelpers::GetInputPosition(x, y, &uielt);
            if (uielt)
                return;

            int newactiveviewport = ViewManager::GetViewportAt(x, y);
            if (newactiveviewport != activeviewport_)
            {
                activeviewport_ = newactiveviewport;
                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Active Viewport=%d !", activeviewport_);
            }

            Viewport* viewport = GetSubsystem<Renderer>()->GetViewport(activeviewport_);
            Ray ray = viewport->GetScreenRay(x, y);
            Vector3 wpoint3 = viewport->ScreenToWorldPoint(x, y, ray.HitDistance(GameContext::Get().GroundPlane_));
            Vector2 wpoint = Vector2(wpoint3.x_, wpoint3.y_);
            WorldMapPosition wmPosition;
            World2D::GetWorldInfo()->Convert2WorldMapPosition(wpoint, wmPosition, wmPosition.positionInTile_);
            wmPosition.viewZIndex_ = ViewManager::Get()->GetCurrentViewZIndex(activeviewport_);
            wmPosition.viewZ_ = ViewManager::Get()->GetCurrentViewZ();

            /// Test Tiles
            if (input.GetKeyDown(KEY_LCTRL) && !input.GetKeyDown(KEY_LSHIFT))
            {
                if (input.GetMouseButtonPress(MOUSEB_LEFT) || input.GetMouseButtonPress(MOUSEB_RIGHT))
                {
                    unsigned mode = input.GetMouseButtonPress(MOUSEB_LEFT) ? 1 : 0;

                    bool ok = false;
                    if (mode == 1)
                        ok = GameHelpers::AddTile(wmPosition, input.GetKeyDown(KEY_LGUI));
                    else
                        ok = GameHelpers::RemoveTile(wmPosition, true, true) != -1;

                    if (ok && !GameContext::Get().LocalMode_)
                    {
                        eventData.Clear();
                        eventData[Net_ObjectCommand::P_TILEOP] = mode;
                        eventData[Net_ObjectCommand::P_TILEINDEX] = wmPosition.tileIndex_;
                        eventData[Net_ObjectCommand::P_TILEMAP] = wmPosition.mPoint_.ToHash();
                        eventData[Net_ObjectCommand::P_TILEVIEW] = wmPosition.viewZIndex_;
                        GameNetwork::Get()->PushObjectCommand(CHANGETILE, &eventData, true, GameNetwork::Get()->GetClientID());
                    }
                }
                else if (input.GetMouseButtonPress(MOUSEB_MIDDLE) || input.GetKeyPress(KEY_LALT))
                    GameHelpers::DumpTile(wmPosition);
            }

            /// Test Objects
            else if (input.GetKeyDown(KEY_LSHIFT) && !GameContext::Get().ClientMode_)
            {
                /// Test Entities
                if (input.GetMouseButtonPress(MOUSEB_LEFT))
                {
                    if (input.GetKeyDown(KEY_1))
                    {
                        URHO3D_LOGINFOF("PlayState() - HandleUpdate : SpawnActor at map=%s Point=%s", wmPosition.mPoint_.ToString().CString(), wmPosition.mPosition_.ToString().CString());

                        // add a merchant
                        Actor* actor = World2D::SpawnActor("Eredot", StringHash("GOT_Merchant"), 0, StringHash("merchandise1"), ViewManager::Get()->GetCurrentViewZ(), wpoint);
                        // respawn actor=5
//                        Actor* actor = World2D::SpawnActor(5, wpoint, ViewManager::Get()->GetCurrentViewZ());
                    }
                    else if (input.GetKeyDown(KEY_2))
                    {
//                        StringHash got("BigBomb");
//                        StringHash got("GOT_ShukTuk");
//                        StringHash got("GOT_Raignee");
//                        StringHash got("GOT_RockGolem");
//                        StringHash got("GOT_Mirubil");
//                        StringHash got("GOT_Lizard");
//                        StringHash got("GOT_RedLord");
//                        StringHash got("EliegorGolem");
                        StringHash got("GOT_Skeleton");
//						StringHash got("FUR_Bougie");

                        URHO3D_LOGINFOF("PlayState() - HandleUpdate : SpawnEntity at map=%s Point=%s", wmPosition.mPoint_.ToString().CString(), wmPosition.mPosition_.ToString().CString());

                        Node* node = World2D::SpawnEntity(got, RandomEntityFlag|RandomMappingFlag, 0, 0, ViewManager::Get()->GetCurrentViewZ(activeviewport_), PhysicEntityInfo(wpoint.x_, wpoint.y_), SceneEntityInfo(), 0, false);
                    }
                    else
                    {
                        RigidBody2D* body = rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(wpoint);
                        if (body && body->GetNode())
                        {
                            Node* node = body->GetNode();

                            // Entity Killer
                            if (input.GetKeyDown(KEY_3))
                            {
                                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Entity %s(%u) to kill !",
                                                node->GetName().CString(), node->GetID());

                                GOC_Life* life = node->GetComponent<GOC_Life>();
                                if (life)
                                {
                                    life->ReceiveEffectFrom((Node*)0, GOA::LIFE, -1000.f, Vector2::ZERO, false);
                                    node->SendEvent(GO_RECEIVEEFFECT);
                                }
                                else
                                {
                                    if (node == sTestObjectMapedNode)
                                        sTestObjectMapedNode.Reset();
                                    node->SendEvent(WORLD_ENTITYDESTROY);
                                }
                            }

                            // Entity Info
                            else
                            {
                                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Point=%s - Entity %s(%u) enabled=%s",
                                                wmPosition.ToString().CString(), node->GetName().CString(), node->GetID(), node->IsEnabled() ? "true":"false");

                                GOC_Destroyer* gocdestroyer = node->GetComponent<GOC_Destroyer>();
                                if (gocdestroyer)
                                {
                                    gocdestroyer->DumpWorldMapPositions();
                                }
                            }
                        }
                        /// Test PathFinder
                        else
                        {
                            PathFinder2D::ClearPaths();
                            int pathid = PathFinder2D::FindPath(localPlayers_[0]->GetAvatar()->GetWorldPosition2D(), ViewManager::FRONTVIEW_Index, wpoint, ViewManager::FRONTVIEW_Index, MV_FLY);
                            if (pathid != -1)
                            {
                                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Find a Path from player0 to destination=%s => pathid=%d", wpoint.ToString().CString(), pathid);
                            }
                        }
                    }
                }
                /// Test ObjectMaped
                if (input.GetMouseButtonPress(MOUSEB_RIGHT))
                {
                    /// Test ObjectMaped Created From Map
                    if (input.GetKeyDown(KEY_1))
                    {
                        if (!sTestObjectMapedNode)
                        {
                            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Object Maped - Object Created ... ");

                            IntRect rect(1, 16, 5, 24);
                            Vector2 initialposition = World2D::GetCurrentMap()->GetRootNode()->GetWorldPosition2D() + Map::info_->GetWorldPositionFromMapCenter(rect.left_, rect.bottom_);
                            initialposition = initialposition + Vector2(0.f, (float)(24-16+2) * World2D::GetWorldInfo()->mTileHeight_);
                            sTestObjectMapedNode = GameHelpers::CreateObjectMapedFrom(ObjectMapedInfo(World2D::GetCurrentMap(), rect, initialposition));
                            if (sTestObjectMapedNode)
                            {
                                GameHelpers::SetControllabledNode(sTestObjectMapedNode, 1);
                                sTestObjectMapedNode->SetEnabledRecursive(true);

//                                World2D::GetCurrentMap()->SetTileRect(rect, MapFeatureType::NoMapFeature);

                                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Object Maped - Object Created From Map node=%s(%u) ... OK ! ",
                                                sTestObjectMapedNode->GetName().CString(), sTestObjectMapedNode->GetID());

//                                GameHelpers::DumpNode(sTestObjectMapedNode, 10, true);
                                return;
                            }
                        }
                        else
                        {
                            sTestObjectMapedNode->Remove();
                            sTestObjectMapedNode.Reset();
                        }
                    }
                    /// Test ObjectMaped Created From DataMap existing File=Custom/object_1.dat
                    else if (input.GetKeyDown(KEY_2))
                    {
                        if (!sTestObjectMapedNode)
                        {
                            Vector2 initialposition = wpoint;
                            sTestObjectMapedNode = GameHelpers::CreateObjectMapedFrom(ObjectMapedInfo(1, initialposition));
                            if (sTestObjectMapedNode)
                            {
                                GameHelpers::SetControllabledNode(sTestObjectMapedNode, 1);
                                sTestObjectMapedNode->SetEnabledRecursive(true);

                                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Object Maped - Object Created From DataMap=1 node=%s(%u) ... OK ! ",
                                                sTestObjectMapedNode->GetName().CString(), sTestObjectMapedNode->GetID());
                                return;
                            }
                        }
                        else
                        {
                            sTestObjectMapedNode->SendEvent(WORLD_ENTITYDESTROY);
                            sTestObjectMapedNode.Reset();
                        }
                    }
                    /// Test ObjectMaped Created From Generator
                    else if (input.GetKeyDown(KEY_3))
                    {
                        Vector2 initialposition = wpoint;
                        int sizex = 10;
                        int sizey = 10;
                        unsigned seed = GameRand::GetTimeSeed() % 255;

                        URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Object Maped - Object From Generator seed=%u size=%d %d at position=%s ... ", seed, sizex, sizey, initialposition.ToString().CString());

                        sTestObjectMapedNode = GameHelpers::CreateObjectMapedFrom(ObjectMapedInfo(MapStorage::GetMapModel(MAPMODEL_MOBILECASTLE), sizex, sizey, seed, initialposition));
                        if (sTestObjectMapedNode)
                        {
                            GameHelpers::SetControllabledNode(sTestObjectMapedNode, 1);
                            sTestObjectMapedNode->SetEnabledRecursive(true);

                            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Object Maped - Object Created From Generator node=%s(%u) ... OK ! ",
                                            sTestObjectMapedNode->GetName().CString(), sTestObjectMapedNode->GetID());
                            return;
                        }
                    }
                    /// Test Back Asteroid Falls
                    else if (input.GetKeyDown(KEY_4))
                    {
                        Vector<WeakPtr<Node> > sAsteroidFallNodes;
                        sAsteroidFallNodes.Resize(7);
                        const Rect& worldrect = World2D::GetExtendedVisibleRect(0);
                        Vector2 screencenter = worldrect.Center();
                        Vector2 screensize = worldrect.Size();
                        float spaceBetweenAsteroids = sAsteroidFallNodes.Size() > 1 ? screensize.x_ / (float)(sAsteroidFallNodes.Size()-1) : 0.f;

                        for (int i=0; i < sAsteroidFallNodes.Size(); i++)
                        {
                            Vector2 initialposition;
                            initialposition.x_ = screencenter.x_ + (i - 1) * spaceBetweenAsteroids + Urho3D::Random(-spaceBetweenAsteroids*0.33f, spaceBetweenAsteroids*0.33f);
                            initialposition.y_ = worldrect.max_.y_ + Urho3D::Random(-spaceBetweenAsteroids, spaceBetweenAsteroids);
                            int sizex = 3 + GameRand::GetRand(MAPRAND, 0, 10) % 2;
                            int sizey = 3 + GameRand::GetRand(MAPRAND, 0, 10) % 2;
                            unsigned seed = GameRand::GetRand(MAPRAND, 0, 1000) % 255;

                            ObjectMapedInfo info(MapStorage::GetMapModel(MAPMODEL_BACKASTEROID), sizex, sizey, seed, initialposition);
                            info.scale_ = MapStorage::Get()->GetNode()->GetWorldScale2D() * 0.75f;

                            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Back Asteroid Falls - i=%d/%u size=%d %d ... ", i+1, sAsteroidFallNodes.Size(), sizex, sizey);

                            sAsteroidFallNodes[i] = GameHelpers::CreateObjectMapedFrom(info);

                            Node* node = sAsteroidFallNodes[i];
                            if (node)
                            {
                                node->SetEnabledRecursive(true);

                                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Test Back Asteroid Falls - i=%d/%u position=%s ... ", i+1, sAsteroidFallNodes.Size(), initialposition.ToString().CString());

                                // Add a fall animation
                                const float FALLDURATION = 1.5f;
                                SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
                                SharedPtr<ValueAnimation> fallAnimation(new ValueAnimation(context_));
                                fallAnimation->SetKeyFrame(0.f, Vector3(initialposition.x_, initialposition.y_, 0.f));
                                fallAnimation->SetKeyFrame(FALLDURATION, Vector3(initialposition.x_-5.f, initialposition.y_-screensize.y_, 0.f));
                                objectAnimation->AddAttributeAnimation("Position", fallAnimation, WM_LOOP);
                                SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(context_));
                                rotateAnimation->SetKeyFrame(0.f, Quaternion(0.f));
                                rotateAnimation->SetKeyFrame(FALLDURATION, Quaternion(90.f));
                                objectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_LOOP);

                                node->SetObjectAnimation(objectAnimation);
                            }
                        }
                    }
                }
            }

            /// Dump Clicked point
            else if (input.GetMouseButtonPress(MOUSEB_LEFT))
            {
                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Clic On Point %s", wmPosition.ToString().CString());

                Map* map = World2D::GetMapAt(wmPosition.mPoint_);
                if (map)
                    map->GetObjectTiled()->DumpFluidCellAt(wmPosition.tileIndex_);
            }
        }
    }
#ifdef ACTIVE_WEATHEREFFECTS
    if (weather_)
    {
        // Handle World Time Rate
        // Decrease rate
        if (input.GetKeyDown(KEY_F7))
        {
            weather_->SetWorldTimeRate(Max(15.f, weather_->GetWorldTimeRate() - 15.f));
        }
        // Increase rate
        if (input.GetKeyDown(KEY_F8))
        {
            weather_->SetWorldTimeRate(weather_->GetWorldTimeRate() + 15.f);
        }
        // Restore to default rate
        if (input.GetKeyPress(KEY_F9))
        {
            weather_->SetWorldTimeRate();
        }
        if (!GameContext::Get().HasConsoleFocus()
#ifdef ACTIVE_CREATEMODE
             && !MapEditor::Get())
#else
            )
#endif
        {
            // Tip Cloud
            if (input.GetScancodePress(SCANCODE_T))
            {
    //            if (weather_->GetCloudIntensity())
    //                weather_->SetEffect(0, WeatherCloud, 0);
    //            else
    //                weather_->SetEffect(0, WeatherCloud, 3);
                weather_->SetEffect(0, WeatherCloud, (weather_->GetCloudIntensity() + 1) % 7);
            }
            // Tip Rain
            if (input.GetScancodePress(SCANCODE_Y))
            {
                if (input.GetQualifiers() == QUAL_SHIFT)
                    weather_->DumpRain();
                else
                    weather_->SetEffect(0, WeatherRain, (weather_->GetRainIntensity() + 1) % 7);
            }
        }
    }
    if (!GameContext::Get().HasConsoleFocus()
#ifdef ACTIVE_CREATEMODE
             && !MapEditor::Get())
#else
            )
#endif
    {
        // Tip DrawableScroller
        if (input.GetScancodePress(SCANCODE_U))
        {
            DrawableScroller::ToggleStartStop();
            if (weather_)
                weather_->ToggleStartStop();
        }
        // Tip Light
        if (input.GetScancodePress(SCANCODE_L))
        {
            PODVector<Light*> lights;
            rootScene_->GetComponents<Light>(lights, true);
            for (unsigned i = 0; i < lights.Size(); ++i)
                lights[i]->SetEnabled(!lights[i]->IsEnabled());
        }
    }
#endif
    // Handle Player1 Animation Speed
    if (localPlayers_.Size() && localPlayers_[activeviewport_]->GetAvatar())
    {
        // Tip : Player1 LoadStuff
        if (input.GetKeyPress(KEY_F5))
        {
            GOC_Inventory::LoadInventory(localPlayers_[activeviewport_]->GetAvatar(), false);
        }
        // Save Scene
        if (input.GetKeyPress(KEY_F6))
        {
            SaveGame();
        }
        // Decrease Animation Speed
        if (input.GetKeyPress(KEY_F10))
        {
            AnimatedSprite2D* animatedSprite = localPlayers_[activeviewport_]->GetAvatar()->GetComponent<AnimatedSprite2D>();
            animatedSprite->SetSpeed(animatedSprite->GetSpeed() > 0 ? animatedSprite->GetSpeed() - 0.1f : 0.0f);
        }
        // Increase Animation Speed
        if (input.GetKeyPress(KEY_F11))
        {
            AnimatedSprite2D* animatedSprite = localPlayers_[activeviewport_]->GetAvatar()->GetComponent<AnimatedSprite2D>();
            animatedSprite->SetSpeed(animatedSprite->GetSpeed() + 0.1f);
        }
        // Restore Animation Speed
        if (input.GetKeyPress(KEY_F12))
        {
            AnimatedSprite2D* animatedSprite = localPlayers_[activeviewport_]->GetAvatar()->GetComponent<AnimatedSprite2D>();
            animatedSprite->SetSpeed(1.0f);
        }
    }
    // Don't Check Following Tips if the console has the focus
    if (!GameContext::Get().HasConsoleFocus())
    {
        // Tip : Switch Z show inside or outside Dungeon
        if (input.GetScancodePress(SCANCODE_KP_0))
        {
#ifndef ACTIVE_CREATEMODE
            bool playerswitch = localPlayers_.Size() && localPlayers_.Size() >= activeviewport_;
#else
            bool playerswitch = !MapEditor::Get() && localPlayers_.Size() && localPlayers_.Size() >= activeviewport_ ;
#endif
            if (playerswitch)
            {
                playerswitch = localPlayers_[activeviewport_]->GetAvatar() ? localPlayers_[activeviewport_]->GetAvatar()->IsEnabled() : false;
                if (playerswitch)
                    localPlayers_[activeviewport_]->SwitchViewZ();
            }
            if (!playerswitch)
                ViewManager::Get()->SwitchNextZ(0);
#ifdef USE_TILERENDERING
            ObjectTiled::SetCuttingLevel(0);
#endif
        }
        // Tip : Node Focus Enable/Disable
        if (input.GetScancodePress(SCANCODE_HOME))
        {
            bool enable = !ViewManager::Get()->GetFocusEnable(activeviewport_);
            URHO3D_LOGINFOF("PlayState() - HandleUpdate : SetFocusEnable=%s on activeviewport=%u !", enable ? "true":"false", activeviewport_);
            ViewManager::Get()->SetFocusEnable(enable, activeviewport_);
        }
        // Tip : Pause Without Window
        if (input.GetScancodePress(SCANCODE_P))
        {
            rootScene_->SetUpdateEnabled(!rootScene_->IsUpdateEnabled());
            rootScene_->GetComponent<PhysicsWorld2D>()->SetUpdateEnabled(rootScene_->IsUpdateEnabled());
            if (GameContext::Get().rttScene_)
                GameContext::Get().rttScene_->SetUpdateEnabled(rootScene_->IsUpdateEnabled());
            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Key P pressed => Scene Updated = %s", rootScene_->IsUpdateEnabled() ? "enable" : "disable");
        }
#ifdef ACTIVE_CREATEMODE
        if (!MapEditor::Get())
#endif
        {
            // Tip : Active UI
            if (input.GetScancodePress(SCANCODE_V))
            {
                SetVisibleUI(!gameUIVisible_);
                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Switch Game UI Visibility to %s !", gameUIVisible_ ? "visible" : "hide");
            }
            // Tip : mission win
            if (input.GetScancodePress(SCANCODE_G))
            {
                URHO3D_LOGINFO("PlayState() - HandleUpdate : Tip Mission Win !");
                localPlayers_[0]->SetMissionWin();
                if (GameContext::Get().arenaZoneOn_)
                    SetGameWin();
            }
            // Tip : Info
            if (input.GetScancodePress(SCANCODE_I))
            {
                if (goManager)
                {
                    URHO3D_LOGINFOF("PlayState() - HandleUpdate Info : numEnemies(active) = %u(%u) numPlayers(active) = %u(%u) numActors = %u",
                                    goManager->GetNumEnemies(), goManager->GetNumActiveEnemies(),
                                    goManager->GetNumPlayers(), goManager->GetNumActivePlayers(),
                                    Actor::GetNumActors());
                }

                URHO3D_LOGINFOF("PlayState() - HandleUpdate : PlayerID=%u pos=%s ! wfbounds=%s",
                                localPlayers_[0]->GetID(),
                                localPlayers_[0]->GetAvatar()->GetWorldPosition2D().ToString().CString(),
                                world_->GetWorldFloatBounds().ToString().CString());
                //localPlayers_[0]->Dump();

    //            localPlayers_[0]->GetAvatar()->GetComponent<GOC_Move2D>()->Dump();

                if (localPlayers_.Size() > 1 && localPlayers_[1]->active)
                {
                    GOC_AIController* aicontrol = static_cast<GOC_AIController*>(localPlayers_[1]->gocController);
                    URHO3D_LOGINFOF("PlayState() - HandleUpdate : PlayerID=%u pos=%s AIStart=%s, Behavior=%s, Mounted=%s!",
                                    localPlayers_[1]->GetID(),
                                    localPlayers_[1]->GetAvatar()->GetWorldPosition2D().ToString().CString(),
                                    aicontrol->IsStarted() ? "true" : "false", aicontrol->GetBehaviorAttr().CString(),
                                    localPlayers_[1]->GetAvatar()->GetVar(GOA::ISMOUNTEDON).GetUInt() ? "true" : "false");
    //                LOGINFOF("Dump localPlayers_[1] body !");
    //                localPlayers_[1]->avatar->GetComponent<RigidBody2D>()->GetBody()->Dump();
                }

                GOC_Inventory_Template::DumpAll();
            }
            // Tip : Dump Pool
            if (input.GetScancodePress(SCANCODE_O))
            {
                //        if (ObjectPool::Get())
                //            ObjectPool::Get()->DumpCategories();
                if (world_)
                    world_->World2D::DumpEntitiesInMemory();
            }
            // Tip player suicide
            if (input.GetScancodePress(SCANCODE_J))
            {
                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Player Suicide on viewport=%d ...", activeviewport_);

                if (localPlayers_[activeviewport_]->GetAvatar())
                {
                    eventData[GOC_Life_Events::GO_ID] = localPlayers_[activeviewport_]->nodeID_;
                    eventData[GOC_Life_Events::GO_KILLER] = 0;
                    eventData[GOC_Life_Events::GO_TYPE] = localPlayers_[activeviewport_]->GetAvatar()->GetVar(GOA::TYPECONTROLLER).GetInt();
                    eventData[GOC_Life_Events::GO_WORLDCONTACTPOINT] = 0;

                    localPlayers_[activeviewport_]->GetAvatar()->SetVar(GOA::ISDEAD, true);
                    localPlayers_[activeviewport_]->GetAvatar()->SetVar(GOA::LIFE, 0);
                    localPlayers_[activeviewport_]->GetAvatar()->SendEvent(GOC_LIFEDEAD, eventData);

                    URHO3D_LOGINFOF("PlayState() - HandleUpdate : Player Suicide ... numActivePlayers=%u OK !", GOManager::GetNumActivePlayers());
                }
            }
        }
    }
#endif

    // Debug Stuff
    if (GameContext::Get().gameConfig_.debugRenderEnabled_ && !GameContext::Get().HasConsoleFocus())
    {
        // Toggle physics debug geometry
        if (input.GetScancodePress(SCANCODE_KP_1))
        {
            GameContext::Get().SetRenderDebug(!GameContext::Get().DrawDebug_);
#ifdef USE_TILERENDERING
            ObjectTiled::SetViewRangeMode((ObjectTiled::GetViewRangeMode() + 1) % 2);
#endif
        }
        // Tip : Active Tagged World Info
        if (input.GetScancodePress(SCANCODE_KP_2))
        {
            GameContext::Get().gameConfig_.debugWorld2DTagged_ = !GameContext::Get().gameConfig_.debugWorld2DTagged_;
            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Switch Tagged World Info = %s !",
                            GameContext::Get().gameConfig_.debugWorld2DTagged_ ? "true" : "false");
        }
#ifdef USE_TILERENDERING
        if (input.GetScancodePress(SCANCODE_KP_PERIOD) || input.GetScancodePress(SCANCODE_KP_MINUS) || input.GetScancodePress(SCANCODE_KP_PLUS))
        {
            static PODVector<RenderShape*> rendershapes;
            GameContext::Get().rootScene_->GetComponents<RenderShape>(rendershapes, true);

            // Tip : Enabled/Disabled RenderShapes
            if (input.GetScancodePress(SCANCODE_KP_PERIOD) && rendershapes.Size())
            {
                int viewZcutted = ViewManager::Get()->GetCurrentViewZ() - ObjectTiled::GetCuttingLevel();
                sRenderShapeCuttingOn_ = !sRenderShapeCuttingOn_;

                if (sRenderShapeCuttingOn_)
                {
                    SetRenderShapeVisibilityAt(viewZcutted, rendershapes);
                    URHO3D_LOGINFOF("PlayState() - HandleUpdate : Set RenderShape Cutting(%d) viewZcutted=%d", ObjectTiled::GetCuttingLevel(), viewZcutted);
                }
                else
                {
                    for (unsigned i = 0; i < rendershapes.Size(); ++i)
                        rendershapes[i]->SetEnabled(true);
                }
            }
            // Tip : Cutting TiledObject To Bottom
            if (input.GetScancodePress(SCANCODE_KP_MINUS))
            {
                int cutting = ObjectTiled::GetCuttingLevel()+5;
                if (cutting > ViewManager::Get()->GetCurrentViewZ() - BACKGROUND)
                    cutting = ViewManager::Get()->GetCurrentViewZ() - BACKGROUND;

                int viewZcutted = ViewManager::Get()->GetCurrentViewZ()-cutting;

                SetRenderShapeVisibilityAt(viewZcutted, rendershapes);

                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Set view cutting level=%d => viewZcutted=%d", cutting, viewZcutted);
                ObjectTiled::SetCuttingLevel(cutting);
            }
            // Tip : Cutting TiledObject To Top
            if (input.GetScancodePress(SCANCODE_KP_PLUS))
            {
                int cutting = ObjectTiled::GetCuttingLevel()-5;
                if (cutting < 0)
                    cutting = 0;

                int viewZcutted = ViewManager::Get()->GetCurrentViewZ()-cutting;

                SetRenderShapeVisibilityAt(viewZcutted, rendershapes);

                URHO3D_LOGINFOF("PlayState() - HandleUpdate : Set view cutting level=%d => viewZcutted=%d", cutting, viewZcutted);
                ObjectTiled::SetCuttingLevel(cutting);
            }
        }

        // Tip : MaxDrawView TiledObject
        if (input.GetScancodePress(SCANCODE_KP_ENTER))
        {
            ObjectTiled::SetMaxDrawView(ObjectTiled::GetMaxDrawView() > 1 ? 1 : 0);
            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Set MaxDrawViews = %d", ObjectTiled::GetMaxDrawView());
        }
        // Tip : TileDrawing TiledObject
        if (input.GetScancodePress(SCANCODE_KP_MULTIPLY))
        {
            ObjectTiled::SetTilesEnable(!ObjectTiled::GetTilesEnable());
            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Set TileDrawing = %s", ObjectTiled::GetTilesEnable() ? "enable":"disable");
        }
        // Tip : DecalDrawing TiledObject
        if (input.GetScancodePress(SCANCODE_KP_DIVIDE))
        {
            ObjectTiled::SetDecalsEnable(!ObjectTiled::GetDecalsEnable());
            URHO3D_LOGINFOF("PlayState() - HandleUpdate : Set DecalDrawing = %s", ObjectTiled::GetDecalsEnable() ? "enable":"disable");
        }
#ifdef ACTIVE_CREATEMODE
        if (!MapEditor::Get())
#endif
        {
            // Debug Text3D
            if (input.GetScancodePress(SCANCODE_K))
            {
                Node* text = localPlayers_[activeviewport_]->GetAvatar()->GetChild("Text3D");
                if (!text)
                {
                    Vector3 position;
                    position.y_ = localPlayers_[activeviewport_]->GetAvatar()->GetComponent<GOC_Animator2D>()->GetAnimated()->GetWorldBoundingBox().Size().y_;
                    position.z_ = 0.1f;
                    TextTest(context_, localPlayers_[activeviewport_]->GetAvatar(), position, "Hello !", "Fonts/Lycanthrope.ttf", 2.f, 1.5f, 50);
                }
                else
                    text->Remove();
            }
        }
#endif
    }
}

float moveZoomFactor=1.f;

void PlayState::HandleCamera(float timeStep)
{
    Input* input = GetSubsystem<Input>();
    Viewport* viewport = GetSubsystem<Renderer>()->GetViewport(activeviewport_);

    // Camera Zoom
    if (input->GetScancodeDown(SCANCODE_KP_9))
    {
        viewport->GetCamera()->SetZoom(viewport->GetCamera()->GetZoom() * 1.01f);
        moveZoomFactor = viewport->GetCamera()->GetZoom() < 0.8f ? viewport->GetCamera()->GetZoom() : 1.f;
    }
    if (input->GetScancodeDown(SCANCODE_KP_3))
    {
        viewport->GetCamera()->SetZoom(viewport->GetCamera()->GetZoom() * 0.99f);
        moveZoomFactor = viewport->GetCamera()->GetZoom() < 0.8f ? viewport->GetCamera()->GetZoom() : 1.f;
    }

    // Movement speed as world units per second, faster with dezoom
    const float MOVE_SPEED = 4.0f / moveZoomFactor;

    // Read NumPAD keys and move the camera scene node to the corresponding direction if they are pressed
    if (input->GetScancodeDown(SCANCODE_KP_8))
        viewport->GetCamera()->GetNode()->Translate(Vector3::UP * MOVE_SPEED * timeStep);
    if (input->GetScancodeDown(SCANCODE_KP_5))
        viewport->GetCamera()->GetNode()->Translate(Vector3::DOWN * MOVE_SPEED * timeStep);
    if (input->GetScancodeDown(SCANCODE_KP_4))
        viewport->GetCamera()->GetNode()->Translate(Vector3::LEFT * MOVE_SPEED * timeStep);
    if (input->GetScancodeDown(SCANCODE_KP_6))
        viewport->GetCamera()->GetNode()->Translate(Vector3::RIGHT * MOVE_SPEED * timeStep);
}

void PlayState::HandleStop(StringHash eventType, VariantMap& eventData)
{
    if (!world_)
    {
        stateManager_->PopStack();
    }
}

void PlayState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("PlayState() - HandleScreenResized ...");

    ResizeUI();

//    #ifdef __ANDROID__
//    GameContext::Get().camera_->SetOrthoSize(768.f * PIXEL_SIZE);
//    GameContext::Get().camera_->SetZoom(1.f);
//    #endif

    URHO3D_LOGINFO("PlayState() - HandleScreenResized ... OK !");
}

void PlayState::HandlePlayQuit(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(stateManager_->GetState("Options"), GAME_OPTIONSFRAME_CLOSED);
    stateManager_->PopStack();
}


void PlayState::OnGameStart(StringHash eventType, VariantMap& eventData)
{
    activeGameLogic_ = localPlayers_.Size() > 0;
    GameContext::Get().enableWinLevel_ = GOManager::GetNumActiveEnemies() > 0 || GOManager::GetNumActiveBots() > 0;
    UnsubscribeFromEvent(this, GAME_START);
}

void PlayState::UpdateNumActivePlayers()
{
    numActivePlayers_ = 0;
    int localplayers = 0, netplayers = 0;
    int lastnetclientid = 0;

    // Calculate Local Active Players
    for (unsigned i=0 ; i < localPlayers_.Size(); ++i)
    {
        if (localPlayers_[i]->active)
        {
            localplayers++;
            URHO3D_LOGINFOF("PlayState() - UpdateNumActivePlayers : Find a Local Active Player id=%d", localPlayers_[i]->GetID());
        }
    }

    // Calculate Net Active Players
    if (GameContext::Get().ServerMode_)
    {
        activeConnections_.Clear();
        const HashMap<Connection*, ClientInfo>& clientinfos = GameNetwork::Get()->GetServerClientInfos();
        for (HashMap<Connection*, ClientInfo>::ConstIterator it = clientinfos.Begin(); it!=clientinfos.End(); ++it)
        {
            const ClientInfo& clientinfo = it->second_;
            if (clientinfo.GetNumActivePlayers())
            {
                activeConnections_.Push(it->first_);
                lastnetclientid = clientinfo.clientID_;
                netplayers += clientinfo.GetNumActivePlayers();
                URHO3D_LOGINFOF("PlayState() - UpdateNumActivePlayers : Find Net Active Players (ClientID=%d)", clientinfo.clientID_);
            }
        }
    }

    numActivePlayers_ = localplayers + netplayers;

    URHO3D_LOGINFOF("PlayState() - UpdateNumActivePlayers : ... Num Active Players = %u", numActivePlayers_);

    // Set the First Rank : winner
    if (numActivePlayers_ == 1)
        rankedCliendIDs_.Insert(0, netplayers ? lastnetclientid : 0);
}

void PlayState::OnPlayerDied(StringHash eventType, VariantMap& eventData)
{
    // Get the Client ID for push in the rank
    Player* player = static_cast<Player*>(context_->GetEventSender());
    if (player)
    {
        ClientInfo* clientinfo = GameNetwork::Get() ? GameNetwork::Get()->GetServerClientInfo(player->GetConnection()) : 0;

        URHO3D_LOGINFOF("PlayState() - OnPlayerDied : rank insert clientID=%d", clientinfo ? clientinfo->clientID_ : 0);
        rankedCliendIDs_.Insert(0, clientinfo ? clientinfo->clientID_ : 0);
    }

    UpdateNumActivePlayers();

    URHO3D_LOGINFOF("PlayState() - OnPlayerDied : Num Active Players = %u", numActivePlayers_);
}

void PlayState::OnGameOver(StringHash eventType, VariantMap& eventData)
{
    SetStatus(PLAYSTATE_ENDGAME);

    // TextMessage is AutoRemove so just need to reset ptr to zero
    gameOverMessage = 0;
    gameOver_ = true;
    UnsubscribeFromEvent(GAME_OVER);

    if (!messageBox)
        messageBox = new Urho3D::MessageBox(context_);

    if (messageBox->GetWindow())
    {
        Localization* l10n = GetSubsystem<Localization>();
        //messageBox->SetTitle(l10n->Get("gameover") + " !");
        //messageBox->SetMessage(l10n->Get("restart") + " ?");
        messageBox->SetTitle(l10n->Get("restart") + " ?");
        messageBox->SetAliveOnEscapeKey(true);

        Button* cancelButton = (Button*)messageBox->GetWindow()->GetChild("OkButton", true);
        cancelButton->SetVisible(true);
        cancelButton->SetFocus(true);

        GameHelpers::ToggleModalWindow(messageBox, messageBox->GetWindow());

#if defined(_DEBUG)
        // only for debug in some cases : we can desactive modal on this messagebox to have access of the Console focus
        messageBox->UnsubscribeFromEvent(messageBox->GetWindow(), E_MODALCHANGED);
        GameHelpers::ToggleModalWindow(messageBox, 0, true);
#endif

        SubscribeToEvent(messageBox, E_MESSAGEACK, URHO3D_HANDLER(PlayState, OnGameOverMessage));
    }
}

void PlayState::OnGameOverMessage(StringHash eventType, VariantMap& eventData)
{
    using namespace MessageACK;

    if (GameContext::Get().input_->GetKeyPress(KEY_ESCAPE))
    {
        URHO3D_LOGINFOF("PlayState() - OnGameOverMessage : KEY_ESCAPE Close MsgBox Error !");
        return;
    }

    UnsubscribeFromEvent(E_MESSAGEACK);

    GameHelpers::ToggleModalWindow();
    messageBox.Reset();

    if (eventData[P_OK].GetBool())
    {
        RestartLevel(!GameContext::Get().LocalMode_);
    }
    else
    {
        URHO3D_LOGINFOF("PlayState() - OnGameOverMessage : Exit !");

        gameOver_ = true;
        stateManager_->PopStack();
    }
}

void PlayState::OnGameWin(StringHash eventType, VariantMap& eventData)
{
    // TextMessage is AutoRemove so just need to reset ptr to zero
    winMessage = 0;
    UnsubscribeFromEvent(GAME_WIN);

    if (GameContext::Get().arenaZoneOn_)
    {
        // Show Next Battle Message
        if (!messageBox)
            messageBox = new Urho3D::MessageBox(context_);

        if (messageBox->GetWindow())
        {
            Localization* l10n = GetSubsystem<Localization>();
            //messageBox->SetTitle(l10n->Get("gamewin") + " !");
            //messageBox->SetMessage(l10n->Get("nextbattle") + " ?");
            messageBox->SetTitle(l10n->Get("nextbattle") + " ?");

            Button* cancelButton = (Button*)messageBox->GetWindow()->GetChild("OkButton", true);
            cancelButton->SetVisible(true);
            cancelButton->SetFocus(true);

            GameHelpers::ToggleModalWindow(messageBox, messageBox->GetWindow());

            SubscribeToEvent(messageBox, E_MESSAGEACK, URHO3D_HANDLER(PlayState, OnGameWinNextBattle));
        }
    }
    else
    {
        // change the seed
        SetWorldSeed(GetWorldSeed()+1);
        ChangeLevel();
    }
}

void PlayState::OnGameWinNextBattle(StringHash eventType, VariantMap& eventData)
{
    using namespace MessageACK;

    UnsubscribeFromEvent(E_MESSAGEACK);

    GameHelpers::ToggleModalWindow();
    messageBox.Reset();

    if (eventData[P_OK].GetBool())
    {
        // reactive players before changing level
        for (unsigned i=0; i < localPlayers_.Size(); ++i)
        {
            GameContext::Get().playerState_[i].active = true;
            localPlayers_[i]->active = true;
        }

        // change the seed
        SetWorldSeed(GetWorldSeed()+1);
        ChangeLevel();
    }
    else
    {
        gameOver_ = true;
        stateManager_->PopStack();
    }
}

void PlayState::OnContinueMessageAck(StringHash eventType, VariantMap& eventData)
{
    using namespace MessageACK;

    UnsubscribeFromEvent(E_MESSAGEACK);

    GameHelpers::ToggleModalWindow();
    messageBox.Reset();

    if (!eventData[P_OK].GetBool())
    {
        URHO3D_LOGINFO("PlayState() - UnPaused !");
        paused_ = false;
    }
    else
    {
        URHO3D_LOGINFO("PlayState() - Quit the Play!");

        gameOver_ = true;

        // remove the option menu
        OptionState* optionState = (OptionState*)stateManager_->GetState("Options");
        if (optionState)
        {
            SubscribeToEvent(optionState, GAME_OPTIONSFRAME_CLOSED, URHO3D_HANDLER(PlayState, HandlePlayQuit));
            optionState->CloseFrame();
        }
    }
}


void PlayState::OnDelayedActions()
{
    // Clean Scene if needed
    sceneCleaner_.Execute();
}

void PlayState::OnDelayedActions_Server(StringHash eventType, VariantMap& eventData)
{
    OnDelayedActions();

//    if (!GameNetwork::Get())
//        return;
//
//    URHO3D_LOGINFO("PlayState() - OnDelayedActions_Server : ...");
//
//    GameNetwork::Get()->Server_UpdateClientScenes();
//
//    URHO3D_LOGINFO("PlayState() - OnDelayedActions_Server : ... OK !");
}

void PlayState::OnDelayedActions_Client(StringHash eventType, VariantMap& eventData)
{
    OnDelayedActions();
}

void PlayState::OnDelayedActions_Local(StringHash eventType, VariantMap& eventData)
{
    OnDelayedActions();
}


void PlayState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("PlayState::OnPostRenderUpdate ...");

    if (world_)
    {
        world_->SetEnableDrawDebug(GameContext::Get().DrawDebug_ && GameContext::Get().gameConfig_.debugWorld2D_);

        DebugRenderer* debugRenderer = rootScene_ ? rootScene_->GetComponent<DebugRenderer>() : 0;

        world_->DrawDebugGeometry(GameContext::Get().gameConfig_.debugWorld2D_ && GameContext::Get().DrawDebug_ ? debugRenderer : 0, GameContext::Get().DrawDebug_ && GameContext::Get().gameConfig_.debugWorld2DTagged_);
    }

    if (rootScene_ && GameContext::Get().DrawDebug_)
    {
        DebugRenderer* debugRenderer = rootScene_->GetComponent<DebugRenderer>();
        if (!debugRenderer)
            return;

//        Octree* tree = rootScene_->GetComponent<Octree>();
//        tree->DrawDebugGeometry(false);
//        {
//            PODVector<Text3D*> text3D;
//            rootScene_->GetComponents<Text3D>(text3D, true);
//            //LOGINFOF("nb Text3D=%d",text3D.Size());
//            for (unsigned i = 0; i < text3D.Size(); ++i)
//                text3D[i]->DrawDebugGeometry(debugRenderer, false);
//        }
        if (GameContext::Get().gameConfig_.debugPathFinder_)
        {
            PathFinder2D::DrawDebugGeometry(debugRenderer);
        }
//        if (GameContext::Get().physics3DEnabled_ && GameContext::Get().debugPhysics_)
//        {
//            PhysicsWorld* physicsWorld = rootScene_->GetComponent<PhysicsWorld>();
//            if (physicsWorld) physicsWorld->DrawDebugGeometry(false);
//        }
        if (GameContext::Get().gameConfig_.physics2DEnabled_ && GameContext::Get().gameConfig_.debugPhysics_)
        {
            PhysicsWorld2D* physicsWorld2D = rootScene_->GetComponent<PhysicsWorld2D>();
            if (physicsWorld2D) physicsWorld2D->DrawDebugGeometry();
        }

        if (GameContext::Get().gameConfig_.debugLights_)
        {
            PODVector<Light*> lights;
            rootScene_->GetComponents<Light>(lights, true);
//            LOGINFOF("nb Light=%d",lights.Size());
            for (unsigned i = 0; i < lights.Size(); ++i)
                lights[i]->DrawDebugGeometry(debugRenderer, false);
        }

#ifdef USE_TILERENDERING
        if (GameContext::Get().gameConfig_.debugObjectTiled_ || GameContext::Get().gameConfig_.debugFluid_)
        {
            PODVector<ObjectTiled*> drawables;
            rootScene_->GetComponents<ObjectTiled>(drawables, true);

            for (unsigned i = 0; i < drawables.Size(); ++i)
            {
                if (drawables[i]->IsEnabledEffective())
                    drawables[i]->DrawDebugGeometry(debugRenderer, false);
            }
        }
#endif

        if (GameContext::Get().gameConfig_.debugUI_)
        {
            PODVector<UIElement*> element;
            UI* ui = GetSubsystem<UI>();
            ui->GetRoot()->GetChildren(element, true);
            for (unsigned i = 0; i < element.Size(); ++i)
                if (element[i]->IsVisibleEffective())
                    ui->DebugDraw(element[i]);

//            for (unsigned i=0 ; i < localPlayers_.Size(); ++i)
//            {
//                if (localPlayers_[i]->active)
//                    localPlayers_[i]->DebugDrawUI();
//            }
        }

        /*
        if (GameContext::Get().gameConfig_.debugSprite2D_)
        {
            PODVector<StaticSprite2D*> drawables;
            rootScene_->GetDerivedComponents<StaticSprite2D>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }
*/
        if (GameContext::Get().gameConfig_.debugSprite2D_)
        {
            PODVector<LSystem2D*> drawables;
            rootScene_->GetDerivedComponents<LSystem2D>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }

        if (GameContext::Get().gameConfig_.debugBodyExploder_)
        {
            PODVector<GOC_BodyExploder2D*> drawables;
            rootScene_->GetComponents<GOC_BodyExploder2D>(drawables, true);
//            LOGINFOF("nb GOC_BodyExploder2D=%d",drawables.Size());
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }

//        {
//            PODVector<RigidBody2D*> bodies;
//            rootScene_->GetComponents<RigidBody2D>(bodies, true);
//            for (PODVector<RigidBody2D*>::ConstIterator it = bodies.Begin(); it != bodies.End(); ++it)
//            {
//                RigidBody2D* body = *it;
//                if (body->GetBodyType() == BT_DYNAMIC)
//                    body->DrawDebugGeometry(debugRenderer, false);
//            }
//        }

//        if (GameContext::Get().gameConfig_.debugObjectMaped_)
        {
            PODVector<ObjectMaped*> drawables;
            rootScene_->GetComponents<ObjectMaped>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }

        if (GameContext::Get().rttScene_)
        {
            DebugRenderer* rttdebugRenderer = GameContext::Get().rttScene_->GetOrCreateComponent<DebugRenderer>();
            PODVector<StaticSprite2D*> drawables;
            GameContext::Get().rttScene_->GetDerivedComponents<StaticSprite2D>(drawables, true);

            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(rttdebugRenderer, false);
        }

        if (GameContext::Get().gameConfig_.debugScrollingShape_)
        {
            PODVector<ScrollingShape*> drawables;
            rootScene_->GetComponents<ScrollingShape>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }

        {
            PODVector<DrawableScroller*> drawables;
            rootScene_->GetComponents<DrawableScroller>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }

        if (GameContext::Get().gameConfig_.debugRenderShape_)
        {
            PODVector<RenderShape*> drawables;
            rootScene_->GetComponents<RenderShape>(drawables, true);
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugRenderer, false);
        }

        if (weather_)
        {
            weather_->DrawDebugGeometry(debugRenderer, false);
        }

        if (effects_)
        {
            effects_->DrawDebugGeometry(debugRenderer, false);
        }

        if (GameContext::Get().gameConfig_.debugPlayer_)
        {
            for (unsigned i=0; i < localPlayers_.Size(); ++i)
            {
                Player* player = localPlayers_[i];
                if (player && player->active)
                    player->DrawDebugGeometry(debugRenderer);
            }
        }

        if (GameContext::Get().gameConfig_.debugDestroyers_)
        {
            PODVector<GOC_Destroyer*> destroyers;
            rootScene_->GetComponents<GOC_Destroyer>(destroyers, true);
            for (unsigned i = 0; i < destroyers.Size(); ++i)
                destroyers[i]->DrawDebugGeometry(debugRenderer, false);
        }

        if (GameContext::Get().gameConfig_.fluidEnabled_ && GameContext::Get().gameConfig_.debugFluid_)
        {
            WaterLayer::Get()->DrawDebugGeometry(debugRenderer, false);
        }

//        {
//            PODVector<GOC_StaticRope*> ropes;
//            rootScene_->GetComponents<GOC_StaticRope>(ropes, true);
//            for (unsigned i = 0; i < ropes.Size(); ++i)
//                ropes[i]->DrawDebugGeometry(debugRenderer, false);
//        }

//        {
//            PODVector<GOC_PhysicRope*> ropes;
//            rootScene_->GetComponents<GOC_PhysicRope>(ropes, true);
//            for (unsigned i = 0; i < ropes.Size(); ++i)
//                ropes[i]->DrawDebugGeometry(debugRenderer, false);
//        }
    }
}

