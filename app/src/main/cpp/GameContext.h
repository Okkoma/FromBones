#pragma once

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Input/Input.h>

#include "GameOptions.h"

#include "DefsCore.h"
#include "DefsEntityInfo.h"
#include "DefsColliders.h"

#include "GameAttributes.h"

namespace Urho3D
{
class Object;
class Context;
class Console;
class Input;
class Renderer;
class Viewport;
class Scene;
class Octree;
class Camera;
class Light;
class UIElement;
class Button;
class Cursor;
class Plane;
class Sprite;
class PhysicsWorld2D;
class SpriteSheet2D;
class Font;
class Renderer2D;
class Sound;
class WorkQueue;
class UI;
class FileSystem;
class ResourceCache;
}

using namespace Urho3D;


class GameNetwork;
class GameStateManager;
class Player;

struct FROMBONES_API GameConfig
{
    GameConfig();

    // network
    String networkMode_;
    String networkServerIP_;
    int networkServerPort_;

    // localization
    int language_;

    // sounds
    int soundEnabled_;
    int musicEnabled_;

    // graphics
    int deviceDPI_;
    int uiDeviceDPI_;
    int frameLimiter_;
    int entitiesLimiter_;
    bool enlightScene_;
    bool fluidEnabled_;
    bool multiviews_;
    bool renderShapes_;
    bool asynLoadingWorldMap_;

    // physics
    bool physics3DEnabled_;
    bool physics2DEnabled_;

    // control
    bool touchEnabled_;
    bool forceTouch_;
    bool screenJoystick_;
    bool autoHideCursorEnable_;
    bool ctrlCameraEnabled_;

    // UI
    bool HUDEnabled_;

    // log
    int logLevel_;
    // debug
    bool debugRenderEnabled_;

    bool debugPathFinder_;
    bool debugPhysics_;
    bool debugLights_;
    bool debugWorld2D_;
    bool debugWorld2DTagged_;
    bool debugUI_;
    bool debugFluid_;

    bool debugSprite2D_;
    bool debugBodyExploder_;
    bool debugScrollingShape_;
    bool debugObjectTiled_;
    bool debugRenderShape_;

    bool debugPlayer_;
    bool debugDestroyers_;

    bool debugRttScene_;

    // living data ?
    bool splashviewed_;
    String initState_;
    String logString;
    String saveDir_;

    int uiUpdateDelay_;
    int screenJoystickID_;
    int screenJoysticksettingsID_;

    float tileSpanning_;
};

enum
{
    UIMAIN = 0,
    UIEQUIPMENT,
    COLLECTABLE,

    NUMUITEXTURES
};

#ifdef ACTIVE_LAYERMATERIALS
enum
{
    LAYERBACKGROUNDS = 0,
    LAYERGROUNDS,
    LAYERRENDERSHAPES,
    LAYERFRONTSHAPES,
    LAYERFURNITURES,
    LAYERACTORS,
    LAYEREFFECTS,
    LAYERWATER,
    LAYERDIALOG,
    LAYERDIALOGTEXT,

    NUMLAYERMATERIALS
};
#endif

enum TextureFXEnum
{
    LIT = 0,
    UNLIT = 1,
    BACKINNERLAYER = 0x0000 << 4,
    BACKGROUNDLAYER = 0x0001 << 4,
    BACKVIEWLAYER = 0x0002 << 4,
    BACKROCK = 0x0003 << 4,
};

enum
{
//    UIFONTS_DIGITS22 = 0,
    UIFONTS_ABY12 = 0,
    UIFONTS_ABY22 = 1,
    UIFONTS_ABY32 = 2,
};

enum
{
    CURSOR_DEFAULT = 0,
    CURSOR_LEFT,
    CURSOR_RIGHT,
    CURSOR_TOP,
    CURSOR_TOPLEFT,
    CURSOR_TOPRIGHT,
    CURSOR_BOTTOM,
    CURSOR_BOTTOMLEFT,
    CURSOR_BOTTOMRIGHT,
    CURSOR_ZOOMPLUS,
    CURSOR_ZOOMMINUS,

    CURSOR_TELEPORTATION,
    NUMCURSORS
};

const String MOUNTNODE  = String("MT");
const String TRIGATTACK = String("TA");
const float DISTANCEFORMOUNT = 2.f;

class FROMBONES_API GameContext : public Object
{
    URHO3D_OBJECT(GameContext, Object);

public :
    GameContext(Context* context);

    virtual ~GameContext();

    static void RegisterObject(Context* context);
    static void Destroy();
    static GameContext& Get();

    void SubscribeToCursorVisiblityEvents();
    void UnsubscribeFromCursorVisiblityEvents();

private :
    void HandleCursorVisibility(StringHash eventType, VariantMap& eventData);
    void HandleBeginUpdate(StringHash eventType, VariantMap& eventData);

    static GameContext* gameContext_;

public :
    void AssembleFonts();

    void InitializeTextures();
    void Initialize();
    void InitTouchInput();
    void InitMouse(int mode);
    void InitJoysticks();
    void ValidateJoysticks();
    int GetControllerIDForJoystick(SDL_JoystickID joyid) const;
    bool GetActionKeyDown(int controlid, int actionkey) const;
    void PreselectBestControllers();

    float GetAdjustUIFactor() const;
    void ResetScreen();

    void ReserveAvatarNodes();
    bool IsAvatarNodeID(unsigned id, int clientid=-1) const;

    void CreatePreloaderIcon();
    bool PreloadResources();
    bool UnloadResources();
    bool IsPreloading() const { return preloading_; }

    void Start();
    void Stop();
    void Exit();
    void Pause(bool enable);

    void SetConsoleVisible(bool state);
    bool HasConsoleFocus();

    void SetMouseVisible(bool uistate, bool osstate, bool lockstate=false);

    void SetRenderDebug(bool enable);

    void ResetLuminosity();
    void ResetNetworkStatics();
    void ResetGameStates();

    int SetLevel(int level=-1);
    void UpdateLevel();

    Node* FindMapPositionAt(const String& favoriteArea, const ShortIntVector2& mPoint, IntVector2& position, int& viewZ, Node* excludeNode=0);
    void SetWorldStartPosition();
    bool GetStartPosition(WorldMapPosition& position, int index=0);

    void Dump() const;

    static const String GAMENAME;

    // Game Config
    GameConfig gameConfig_;
    unsigned FIRSTAVATARNODEID;
    static const int MAX_NUMPLAYERS = 4;
    static const int MAX_NUMNETPLAYERS = 64;
    static const int MAX_NUMLEVELS = 3;
    bool preloading_;
    Sprite* preloaderIcon_;
    int preloaderState_;
    static const long long preloadDelayUsec_;

    // Game Systems
    Context* context_;
    Engine* engine_;
    ResourceCache* resourceCache_;
    Renderer* renderer_;
    Input* input_;
    Console* console_;
    UI* ui_ ;
    Time* time_;
    FileSystem* fs_;
    WeakPtr<PhysicsWorld2D> physicsWorld_;
    WeakPtr<Renderer2D> renderer2d_;
    WorkQueue* gameWorkQueue_;
    GameStateManager* stateManager_;
    GameNetwork* gameNetwork_;

    // Game Scene
    SharedPtr<Scene> rootScene_, rttScene_;
    Octree* octree_;
    Scene* mainMenuScene_;
    SharedPtr<Node> cameraNode_;
    WeakPtr<Node> preloadGOTNode_;
    WeakPtr<Node> controllablesNode_;
    Camera* camera_;
    static const float cameraZoomFactor_;
    static const float CameraZoomDefault_;
    static const float CameraZoomBoss_;

    // Game Cursor
    WeakPtr<Cursor> cursor_;
    int cursorShape_;
    static const char* cursorShapeNames_[NUMCURSORS];

    // Game Screen Parameters
    static const int targetwidth_;
    static const int targetheight_;
    int screenwidth_;
    int screenheight_;
    float uiScale_, uiScaleMax_, uiScaleMin_;
    static const float uiAdjustTabletScale_;
    static const float uiAdjustMobileScale_;
    float ddpi_, hdpi_, vdpi_, screenInches_;
    float screenRatioX_, screenRatioY_, dpiScale_, uiDpiScale_;
    float luminosity_, lastluminosity_;
    static const Plane GroundPlane_;
    float CameraZ_;
    Rect MapBounds_;

    // Player States
    struct PlayerState
    {
        bool active;
        unsigned score;
        float energyLost;
        float invulnerability;
        int viewZ;
        int avatar;
        unsigned unblockedAvatars_;
        int controltype;
        int keys[MAX_NUMACTIONS];
        int joybuttons[MAX_NUMACTIONS];
        Vector2 position;

        static const PlayerState EMPTY;
    };
    unsigned playerNodes_[MAX_NUMPLAYERS];
    WeakPtr<Player> players_[MAX_NUMPLAYERS];
    WeakPtr<Node> playerAvatars_[MAX_NUMPLAYERS];
    PlayerState* playerState_;
    Color playerColor_[MAX_NUMPLAYERS];
    Vector<Slot> playerInventory_[MAX_NUMPLAYERS];
    WorldMapPosition worldStartPosition_;

    // Controls Mapping
    Vector<PODVector<int> > keysMap_;
    Vector<PODVector<int> > buttonsMap_;
    int joystickIndexes_[MAX_NUMPLAYERS];
    HashMap<SDL_JoystickID, int> joystickControllerIds_;
    JoystickState* joystickByControllerIds_[MAX_NUMPLAYERS];
    static const int defaultkeysMap_[MAX_NUMPLAYERS][MAX_NUMACTIONS];
    static const int defaultbuttonsMap_[MAX_NUMPLAYERS][MAX_NUMACTIONS];

    bool uiLockSceneControllers_;

    // Game States
    struct GameState
    {
        PlayerState playerState_[MAX_NUMPLAYERS];
        int indexLevel_;
        int numPlayers_;
        bool testZoneOn_;
        bool arenaZoneOn_;
        bool createModeOn_;
        bool enableWinLevel_;
        bool enableMissions_;

        void Dump() const;
    };
    GameState gameState_;
    bool loadSavedGame_;
    static const char* savedGameFile_;
    static const char* savedSceneFile_;
    static const char* savedPlayerFile_[MAX_NUMPLAYERS];
    static const char* savedPlayerMissionFile_[MAX_NUMPLAYERS];
    static const char* sceneToLoad_[MAX_NUMLEVELS+4][3]; // MAX_NUMLEVELS+ 3 Special Zones (Arena & Test1+2 & Create)
    const char* currentMusic;
    int GameLogLock_;
    int mouseLockState_;

    // Level States
    int& indexLevel_;
    int& numPlayers_;
    int testZoneId_;
    String testZoneCustomModel_;
    float testZoneCustomSize_;
    unsigned testZoneCustomSeed_;
    IntVector2 testZoneCustomDefaultMap_;
    bool& testZoneOn_;
    bool& arenaZoneOn_;
    int arenaMod_;
    bool& createModeOn_;
    bool resetWorldMaps_;
    bool allMapsPrefab_;
    bool forceCreateMode_;
    bool& enableWinLevel_;
    bool& enableMissions_;
    bool tipsWinLevel_;
    int lastLevelMode_;

    // Network States
    bool AllowUpdate_;
    bool LocalMode_;
    bool ServerMode_;
    bool ClientMode_;
    float NetMaxDistance_;
    bool DrawDebug_;

    // UI Textures
    Vector<SharedPtr<Texture2D> > textures_;
    Vector<SharedPtr<SpriteSheet2D> > spriteSheets_;
#ifdef ACTIVE_LAYERMATERIALS
    Vector<SharedPtr<Material> > layerMaterials_;
#endif
    Vector<WeakPtr<Font> > uiFonts_;
    static const char* txtMsgFont_;

    // Static Memory Containers
    HashMap<unsigned, PixelShape> sConstPixelShapes_;
    Stack<unsigned> sMapStack_;
};
