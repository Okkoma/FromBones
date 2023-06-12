#include <iostream>
#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/WorkQueue.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Container/Ptr.h>

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Engine/Console.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/File.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/GraphicsImpl.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Viewport.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Model.h>
#include <Urho3D/Graphics/StaticModel.h>
#include <Urho3D/Graphics/Texture.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/RenderPath.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Audio/Sound.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/FontFaceBitmap.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Cursor.h>
#include <Urho3D/UI/Button.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/ParticleEffect2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>

#include <Urho3D/Math/AreaAllocator.h>

#include "DefsViews.h"

#include "Predefined.h"
#include "GameAttributes.h"
#include "GameNetwork.h"
#include "GameHelpers.h"
#include "GameLibrary.h"
#include "GameOptions.h"

#include "GameStateManager.h"
#include "sSplash.h"
#include "sMainMenu.h"
#include "sPlay.h"
#include "sOptions.h"

#include "GOC_Abilities.h"
#include "GOC_Animator2D.h"
#include "GOC_BodyExploder2D.h"
#include "GOC_Collectable.h"
#include "GOC_Destroyer.h"
#include "GOC_ZoneEffect.h"
#include "GOC_EntityAdder.h"
#include "GOC_ControllerPlayer.h"
#include "CraftRecipes.h"
#include "ScrapsEmitter.h"

#include "ViewManager.h"
#include "MapWorld.h"
#include "Map.h"
#include "ObjectPool.h"

#ifdef ACTIVE_CREATEMODE
#include "MapEditor.h"
#endif

#include "GameContext.h"


/// Game Config

GameConfig::GameConfig() :
    networkMode_("auto"),

    language_(0),
    soundEnabled_(true),
    musicEnabled_(true),

    deviceDPI_(1),
    uiDeviceDPI_(1),
    frameLimiter_(0),
    entitiesLimiter_(-1),
    enlightScene_(false),
    fluidEnabled_(false),
    renderShapes_(true),
    multiviews_(false),
    asynLoadingWorldMap_(false),

    physics3DEnabled_(false),
    physics2DEnabled_(true),

    touchEnabled_(false),
    forceTouch_(false),  // force show touch Emulation
    screenJoystick_(false),
    autoHideCursorEnable_(true),
    ctrlCameraEnabled_(false),

    HUDEnabled_(true),
    logLevel_(LOG_DEBUG),

    debugRenderEnabled_(true),

    debugPathFinder_(false),
    debugPhysics_(true),
    debugLights_(false),
    debugWorld2D_(false),
    debugWorld2DTagged_(false),
    debugUI_(false),
    debugFluid_(false),

    debugSprite2D_(false),
    debugBodyExploder_(false),
    debugScrollingShape_(false),
    debugObjectTiled_(false),
    debugRenderShape_(false),

    debugPlayer_(true),
    debugDestroyers_(false),

    debugRttScene_(false),

    splashviewed_(0),
    initState_(String::EMPTY),
    saveDir_(String::EMPTY),

    uiUpdateDelay_(2),
    screenJoystickID_(-1),
    screenJoysticksettingsID_(-1),

    tileSpanning_(0.f)
{ }

const int GameContext::targetwidth_ = 1920;
const int GameContext::targetheight_ = 1080;
#ifdef URHO3D_VULKAN
const long long GameContext::preloadDelayUsec_ = 15000;
#else
const long long GameContext::preloadDelayUsec_ = 50000;
#endif

const float GameContext::cameraZoomFactor_ = 1.15f;
const float GameContext::CameraZoomDefault_ = 1.f;
const float GameContext::CameraZoomBoss_ = 0.5f;

const char* GameContext::cursorShapeNames_[NUMCURSORS] =
{
    "cursor",
    "cursorL",
    "cursorR",
    "cursorT",
    "cursorTL",
    "cursorTR",
    "cursorB",
    "cursorBL",
    "cursorBR",
    "cursorZP",
    "cursorZM",
    "cursorTP"
};

const float GameContext::uiAdjustTabletScale_ = 1.25f;
const float GameContext::uiAdjustMobileScale_ = 1.33f;

/// Game States
const String GameContext::GAMENAME     		= String("FromBones");
const char* GameContext::savedGameFile_ 	= "Save/Game.bin";
const char* GameContext::savedSceneFile_ 	= "Save/Scene.xml";
const char* GameContext::savedPlayerFile_[MAX_NUMPLAYERS] =
{
    "Save/Player_0.xml",
    "Save/Player_1.xml",
    "Save/Player_2.xml",
    "Save/Player_3.xml",
};
const char* GameContext::savedPlayerMissionFile_[MAX_NUMPLAYERS] =
{
    "Save/Missions_0",
    "Save/Missions_1",
    "Save/Missions_2",
    "Save/Missions_3",
};

const char* GameContext::sceneToLoad_[MAX_NUMLEVELS+4][3] = // MAX_NUMLEVELS+ArenaZone+TestZones+CreateMode
{
    {"Scenes/Scene_01.xml", "Music/scene01.ogg", "Level.1"},
    {"Scenes/Scene_02.xml", "Music/scene02.ogg", "Level.2"},
    {"Scenes/Scene_03.xml", "Music/scene03.ogg", "Level.3"},
//    {"Scenes/Scene_01.xml", "Music/WT - The Cave V2.wav", "Level.1"},
//    {"Scenes/Scene_02.xml", "Music/WT - Peaceful Rest V2.wav", "Level.2"},
//    {"Scenes/Scene_03.xml", "Music/WT - The Cave V2.wav", "Level.3"},
    {"Scenes/ArenaZone.xml", "Music/scene01.ogg", "Arena.Zone"},
    {"Scenes/TestZone1.xml", "Music/scene01.ogg", "World.Zone.1"},
    {"Scenes/TestZone2.xml", "Music/scene02.ogg", "World.Zone.2"},
    {"Scenes/CreateMode.xml", "", "Create.Mode"},
};

/// Player's States

const GameContext::PlayerState GameContext::PlayerState::EMPTY =
{
    false, 0, 0.f, 0.f, 0, 0, 0, 0,
    SCANCODE_W, SCANCODE_S, SCANCODE_A, SCANCODE_D, SCANCODE_SPACE, SCANCODE_E, SCANCODE_F, SCANCODE_R, SCANCODE_Q, SCANCODE_TAB,
    SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK,
};

void GameContext::GameState::Dump() const
{
//    URHO3D_LOGINFOF("GameContext::GameState() - Dump");

    for (unsigned i=0; i<MAX_NUMPLAYERS; ++i)
    {
        URHO3D_LOGINFOF("  => playerState[%u] : active=%u avatar=%d score=%u energyLost=%f position=%s viewZ=%d",
                        i, playerState_[i].active, playerState_[i].avatar, playerState_[i].score,
                        playerState_[i].energyLost, playerState_[i].position.ToString().CString(), playerState_[i].viewZ);
    }

    URHO3D_LOGINFOF("  => indexLevel_  = %d", indexLevel_);
    URHO3D_LOGINFOF("  => numPlayers_  = %d", numPlayers_);
    URHO3D_LOGINFOF("  => TestZoneOn_  = %s", testZoneOn_ ? "true" : "false");
    URHO3D_LOGINFOF("  => ArenaZoneOn_ = %s", arenaZoneOn_ ? "true" : "false");
    URHO3D_LOGINFOF("  => CreateModeOn_  = %s", createModeOn_ ? "true" : "false");
    URHO3D_LOGINFOF("  => enableWinLevel_  = %s", enableWinLevel_ ? "true" : "false");
    URHO3D_LOGINFOF("  => enableMissions_  = %s", enableMissions_ ? "true" : "false");
}

const Plane GameContext::GroundPlane_ = Plane(Vector3(0.0f, 0.0f, 1.0f), Vector3(0.0f, 0.0f, 0.0f));

/// UI Textures & Fonts

//const char* GameContext::txtMsgFont_ = "Fonts/CastleDracustein.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/CoralineCat.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/GypsyCurse.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/GypsyMoon.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/KreepyKrawly.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/OctoberCrow.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/PhantomFingers.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/Spiderfingers.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/SwampWitch.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/UnquietSpirits.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/MostlyGhostly.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/WolfMoon.ttf";
//const char* GameContext::txtMsgFont_ = "Fonts/RavenScream.ttf";
const char* GameContext::txtMsgFont_ = "Fonts/Lycanthrope.ttf";


const char * textureFiles_[] =
{
    "Textures/UI/game_ui.png",
    "Textures/UI/game_equipment.png",
    "Textures/Actors/collectable1.png",
};

const char * spriteSheetFiles_[] =
{
    "UI/game_ui.xml",
    "UI/game_equipment.xml",
    "2D/collectable1.xml"
};

#ifdef ACTIVE_LAYERMATERIALS
const char * layerMaterialFiles_[] =
{
    "Materials/LayerBackgrounds.xml",
    "Materials/LayerGrounds.xml",
    "Materials/LayerRenderShapes.xml",
    "Materials/LayerFrontShapes.xml",
    "Materials/LayerFurnitures.xml",
    "Materials/LayerActors.xml",
    "Materials/LayerEffects.xml",
    "Materials/LayerWaterInside.xml",
    "Materials/LayerDialog.xml",
    "Materials/LayerDialogText.xml",
};
#endif

const int GameContext::defaultkeysMap_[MAX_NUMPLAYERS][MAX_NUMACTIONS] =
{
    { SCANCODE_W, SCANCODE_S, SCANCODE_A, SCANCODE_D, SCANCODE_SPACE, SCANCODE_E, SCANCODE_F, SCANCODE_R, SCANCODE_Q, SCANCODE_TAB },
    { SCANCODE_UP, SCANCODE_DOWN, SCANCODE_LEFT, SCANCODE_RIGHT, SCANCODE_RCTRL, SCANCODE_RSHIFT, SCANCODE_SLASH, SCANCODE_PERIOD, SCANCODE_Q, SCANCODE_TAB },
    { SCANCODE_W, SCANCODE_S, SCANCODE_A, SCANCODE_D, SCANCODE_SPACE, SCANCODE_E, SCANCODE_F, SCANCODE_R, SCANCODE_Q, SCANCODE_TAB },
    { SCANCODE_UP, SCANCODE_DOWN, SCANCODE_LEFT, SCANCODE_RIGHT, SCANCODE_RCTRL, SCANCODE_RSHIFT, SCANCODE_SLASH, SCANCODE_PERIOD, SCANCODE_Q, SCANCODE_TAB }
};

const int GameContext::defaultbuttonsMap_[MAX_NUMPLAYERS][MAX_NUMACTIONS] =
{
    { SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK },
    { SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK },
    { SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK },
    { SDL_CONTROLLER_BUTTON_DPAD_UP, SDL_CONTROLLER_BUTTON_DPAD_DOWN, SDL_CONTROLLER_BUTTON_DPAD_LEFT, SDL_CONTROLLER_BUTTON_DPAD_RIGHT, SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y, SDL_CONTROLLER_BUTTON_START, SDL_CONTROLLER_BUTTON_BACK },
};


GameContext::GameContext(Context* context) :
    Object(context),
    FIRSTAVATARNODEID(0),
    preloaderIcon_(0),
    preloaderState_(0),
    context_(0),
    input_(0),
    console_(0),
    fs_(0),
    ui_(0),
    time_(0),
    camera_(0),
    cursorShape_(-1),
    uiScale_(1.f),
    uiScaleMin_(0.75f),
#ifdef DESKTOP_GRAPHICS
    uiScaleMax_(1.5f),
#else
    uiScaleMax_(3.f),
#endif
    ddpi_(160.f),
    hdpi_(0.f),
    vdpi_(0.f),
    screenInches_(0.f),
    screenRatioX_(1.f),
    screenRatioY_(1.f),
    dpiScale_(1.f),
    uiDpiScale_(1.f),
    luminosity_(1.f),
    lastluminosity_(0.f),
    NetMaxDistance_(5.f),
    loadSavedGame_(false),
    mainMenuScene_(0),
    currentMusic(sceneToLoad_[0][1]),
    GameLogLock_(0),
    mouseLockState_(-1),
    stateManager_(0),
    gameWorkQueue_(0),
    gameNetwork_(0),
    AllowUpdate_(false),
    LocalMode_(true),
    ServerMode_(false),
    ClientMode_(false),
    DrawDebug_(false),//    if (!GameContext::Get().ui_->HasModalElement() || GameContext::Get().ui_->GetFocusElement() == uioptionsframe_)

    playerState_(gameState_.playerState_),
    CameraZ_(10.f),
    indexLevel_(gameState_.indexLevel_),
    numPlayers_(gameState_.numPlayers_),
    testZoneId_(0),
    testZoneCustomSize_(0.f),
    testZoneCustomSeed_(0),
    testZoneOn_(gameState_.testZoneOn_),
    arenaZoneOn_(gameState_.arenaZoneOn_),
    arenaMod_(0),
    createModeOn_(gameState_.createModeOn_),
    resetWorldMaps_(false),
    allMapsPrefab_(false),
    forceCreateMode_(false),
    enableWinLevel_(gameState_.enableWinLevel_),
    enableMissions_(gameState_.enableMissions_),
    tipsWinLevel_(false),
    lastLevelMode_(0)
{
    for (int i=0; i < MAX_NUMPLAYERS; i++)
        joystickIndexes_[i] = -1;

    for (int i=0; i < MAX_NUMPLAYERS; i++)
        gameState_.playerState_[i] = PlayerState::EMPTY;

    gameState_.indexLevel_ = 1;
    gameState_.numPlayers_ = 1;
    gameState_.testZoneOn_ = false;
    gameState_.arenaZoneOn_ = false;
    gameState_.createModeOn_ = false;
    gameState_.enableWinLevel_ = false;
    gameState_.enableMissions_ = false;

    for (int i=0; i < MAX_NUMPLAYERS; i++)
        playerNodes_[i] = 0;

    playerColor_[0] = Color(1.f, 1.f, 1.f, 1.f);
    playerColor_[1] = Color(0.5f, 1.f, 0.9f, 1.f);
    playerColor_[2] = Color(1.f, 0.9f, 0.5f, 1.f);
    playerColor_[3] = Color(0.5f, 0.9f, 1.0f, 1.f);
}

GameContext* GameContext::gameContext_ = 0;

GameContext::~GameContext()
{ }

void GameContext::RegisterObject(Context* context)
{
    if (!GameContext::gameContext_)
    {
        GameContext::gameContext_ = new GameContext(context);
        GameContext::gameContext_->context_ = context;
    }
}

void GameContext::Destroy()
{
    if (GameContext::gameContext_)
    {
        delete GameContext::gameContext_;
    }
}

GameContext& GameContext::Get()
{
    return *GameContext::gameContext_;
}

void GameContext::InitializeTextures()
{
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - InitializeTextures  ....              -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");

    // Get UI Textures & Fonts
    textures_.Clear();
    spriteSheets_.Clear();

    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
    textures_.Resize(NUMUITEXTURES);
    for (int i = 0; i < NUMUITEXTURES; i++)
    {
        SharedPtr<Texture2D>& texture = textures_[i];
        texture = (cache->GetResource<Texture2D>(textureFiles_[i]));
        if (!texture)
        {
            URHO3D_LOGERRORF("GameContext() - InitializeTextures : ... can't find Texture=%s", textureFiles_[i]);
            continue;
        }

        URHO3D_LOGINFOF("GameContext() - InitializeTextures : ... Load UI Texture=%s Ptr=%s", textureFiles_[i], String((void*)texture.Get()).CString());
    }
    spriteSheets_.Resize(NUMUITEXTURES);
    for (int i = 0; i < NUMUITEXTURES; i++)
    {
        if (!textures_[i])
            continue;

        SharedPtr<SpriteSheet2D>& spritesheet = spriteSheets_[i];

        spritesheet = cache->GetResource<SpriteSheet2D>(spriteSheetFiles_[i]);
        if (!spritesheet)
        {
            URHO3D_LOGERRORF("GameContext() - InitializeTextures : ... can't find SpriteSheet=%s", spriteSheetFiles_[i]);
            continue;
        }

        URHO3D_LOGINFOF("GameContext() - InitializeTextures : ... Load UI SpriteSheet=%s TexturePtr=%s", spriteSheetFiles_[i], String((void*)spritesheet->GetTexture()).CString());
    }

#ifdef ACTIVE_RENDERTARGET
    // create the render target scene for rendering alpha animatesdsprites
    int texturesize = 1024;
    SharedPtr<Texture2D> renderedTexture(new Texture2D(context_));
    renderedTexture->SetSize(texturesize, texturesize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
    renderedTexture->SetMipsToSkip(QUALITY_LOW, 0);
    renderedTexture->SetNumLevels(1);
    renderedTexture->SetName("RenderTarget2D");
    cache->AddManualResource(renderedTexture);
#endif

#ifdef ACTIVE_LAYERMATERIALS
#if defined(URHO3D_OPENGL)
    {
        int texture_units;
        glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &texture_units);
        URHO3D_LOGERRORF("GameContext() - InitializeTextures : ... GL_MAX_TEXTURE_IMAGE_UNITS=%d", texture_units);
    }
#endif
    layerMaterials_.Resize(NUMLAYERMATERIALS);
    for (int i = 0; i < NUMLAYERMATERIALS; i++)
    {
        layerMaterials_[i] = cache->GetResource<Material>(layerMaterialFiles_[i]);
    }

    // Dump Material Texture Units
    for (int i = 0; i < NUMLAYERMATERIALS; i++)
    {
        Material* material = layerMaterials_[i];
        if (material)
        {
            URHO3D_LOGINFOF("GameContext() - InitializeTextures : ... LayerMaterial %s  ...", material->GetName().CString());
            for (int i = 0; i < 16; i++)
            {
                Texture* texture = material->GetTexture((TextureUnit)i);
                if (texture)
                    URHO3D_LOGINFOF("  Texture Unit=%d Name=%s", i, texture->GetName().CString());
            }
        }
    }
#endif

//    uiFonts_.Push(WeakPtr<Font>(cache->GetResource<Font>("Fonts/digits22.xml")));
//    uiFonts_[UIFONTS_DIGITS22]->AddCustomTexture(22, textures_[UIEQUIPMENT]);
    uiFonts_.Push(WeakPtr<Font>(cache->GetResource<Font>("Fonts/aby12.xml")));
    uiFonts_[UIFONTS_ABY12]->AddCustomTexture(12, textures_[UIEQUIPMENT]);
    uiFonts_.Push(WeakPtr<Font>(cache->GetResource<Font>("Fonts/aby22.xml")));
    uiFonts_[UIFONTS_ABY22]->AddCustomTexture(22, textures_[UIEQUIPMENT]);
    uiFonts_.Push(WeakPtr<Font>(cache->GetResource<Font>("Fonts/aby32.xml")));
    uiFonts_[UIFONTS_ABY32]->AddCustomTexture(32, textures_[UIEQUIPMENT]);
}

void GameContext::Initialize()
{
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - Initialize  ....                      -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");

    InitializeTextures();

    RegisterGameLibrary(context_);

    resourceCache_ = context_->GetSubsystem<ResourceCache>();
    renderer_ = context_->GetSubsystem<Renderer>();
    input_ = context_->GetSubsystem<Input>();
    time_ = context_->GetSubsystem<Time>();
    fs_ = context_->GetSubsystem<FileSystem>();

    testZoneId_ = 2;
    testZoneCustomModel_ = String::EMPTY;
    testZoneCustomSize_ = 0.f;
    testZoneCustomSeed_ = 0U;
    // in TestZone, don't allow custom default map if not choosen on the snapshot : prefer use de default map defined in TestZone2.xml by default (see World2D::SetDefaultMapPoint)
    testZoneCustomDefaultMap_.x_ = 65535;
    testZoneCustomDefaultMap_.y_ = 0;

    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();

    // Set default UI style
    ui_ = context_->GetSubsystem<UI>();
    UIElement* uiroot = ui_->GetRoot();
    uiroot->SetDefaultStyle(cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    // Create Root Scene
    rootScene_ = new Scene(context_);
    rootScene_->SetName("RootScene");
#ifdef ACTIVE_CREATEMODE
    GOA::RegisterToScene();
#endif
    octree_ = rootScene_->CreateComponent<Octree>(LOCAL);
    renderer2d_ = rootScene_->CreateComponent<Renderer2D>(LOCAL);
    renderer2d_->SetInitialVertexBufferSize(30000U);
    physicsWorld_ = rootScene_->CreateComponent<PhysicsWorld2D>(LOCAL);

    // Create Stuff for rendered Target Textures
#ifdef ACTIVE_RENDERTARGET
    {
        // Create the scene which will be rendered to a texture
        rttScene_ = new Scene(context_);
        rttScene_->CreateComponent<Octree>();

//        SharedPtr<Texture2D> renderedTexture(cache->GetResource<Texture2D>("RenderTarget2D"));

        int texturesize = 1024;
        SharedPtr<Texture2D> renderedTexture = SharedPtr<Texture2D>(new Texture2D(context_));
        renderedTexture->SetSize(texturesize, texturesize, Graphics::GetRGBFormat(), TEXTURE_RENDERTARGET);
        renderedTexture->SetFilterMode(FILTER_BILINEAR);
        renderedTexture->SetName("RenderTarget2D");
        // Don't use Mipmapping
        renderedTexture->SetNumLevels(1);
        cache->AddManualResource(renderedTexture);

        // Load the material and assign render Texture to a textureunit
        Material* rttMaterial = 0;
#ifdef ACTIVE_LAYERMATERIALS
        rttMaterial = layerMaterials_[LAYERACTORS];
#else
        rttMaterial = context_->GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/RenderTarget.xml");
#endif
        rttMaterial->SetTexture((TextureUnit)0, renderedTexture.Get());

        // Create a camera for the render-to-texture scene. Simply leave it at the world origin and let it observe the scene
        Node* rttCameraNode = rttScene_->CreateChild("Camera");
        Camera* camera = rttCameraNode->CreateComponent<Camera>();
//        int texturesize = 1024;
//        camera->SetOrthoSize((float)texturesize * PIXEL_SIZE);
//        camera->SetFarClip(50.f+1.f);
//        rttCameraNode->SetPosition(Vector3(0.0f, 0.0f, -50.f));

        camera->SetNearClip(-100.f);
        camera->SetFarClip(100.f);
        camera->SetFov(60.f);
        camera->SetOrthographic(true);
        camera->SetOrthoSize((float)renderedTexture->GetHeight() * PIXEL_SIZE);
        rttCameraNode->SetPosition(Vector3(0.0f, 0.0f, -50.f));

        // Get the texture's RenderSurface object (exists when the texture has been created in rendertarget mode)
        // and define the viewport for rendering the second scene, similarly as how backbuffer viewports are defined
        // to the Renderer subsystem. By default the texture viewport will be updated when the texture is visible
        // in the main view
        SharedPtr<RenderPath> renderpath(new RenderPath());
        renderpath->Load(cache->GetResource<XMLFile>("RenderPaths/Urho2DRenderTarget.xml"));
        Viewport* rttViewport = new Viewport(context_, rttScene_, rttCameraNode->GetComponent<Camera>(), renderpath);

        //renderedTexture->GetRenderSurface()->SetUpdateMode(SURFACE_UPDATEALWAYS);
        renderedTexture->GetRenderSurface()->SetViewport(0, rttViewport);

        AnimatedSprite2D::SetRenderTargetContext(renderedTexture, rttViewport, rttMaterial);
        URHO3D_LOGINFOF("GameContext() - Initialize : ... RenderedTexture=%u RttSceneId=%u RttMaterial=%s ...", renderedTexture.Get(), rttScene_->GetID(), rttMaterial ? rttMaterial->GetName().CString() : "0");
    }
#endif

    // Create View Manager
    if (!ViewManager::Get())
        new ViewManager(context_);

    ViewManager::Get()->SetViewportLayout(1);
    cameraNode_->SetPosition(Vector3::ZERO);

    ResetScreen();

    URHO3D_LOGINFOF("GameContext() - Initialize : ... rootSceneId=%u cameraNodeId=%u cameraId=%u dpiScale=%f",
                    rootScene_->GetID(), cameraNode_->GetID(), camera_->GetID(), dpiScale_);

    if (gameConfig_.debugRenderEnabled_)
    {
        context_->GetSubsystem<Renderer>()->GetViewport(0)->SetDrawDebug(true);
        rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);
    }

#ifdef ACTIVE_PRELOADER
    preloading_ = true;
#endif

    // Create Game State Manager
    stateManager_ = new GameStateManager(context_);
    stateManager_->RegisterState(new SplashState(context_));
    stateManager_->RegisterState(new MenuState(context_));
    stateManager_->RegisterState(new PlayState(context_));
    stateManager_->RegisterState(new OptionState(context_));
    stateManager_->AddToStack("MainMenu");

    // Create Threaded Game Work Queue
#if defined(ACTIVE_WORLD2D_THREADING) && MODULETHREAD_VERSION == 2
    if (!gameWorkQueue_)
    {
        gameWorkQueue_ = new WorkQueue(context_);
        gameWorkQueue_->CreateThreads(GENWORLD_NUMTHREADS);
        gameWorkQueue_->SetNonThreadedWorkMs(5);
    }
#endif

    if (gameConfig_.initState_.Empty() && !gameConfig_.forceTouch_)
    {
        URHO3D_LOGINFO("GameContext() - Initialize : ... Add Default State : Splash ...");
        stateManager_->AddToStack("Splash");
    }
    else
    {
        URHO3D_LOGINFO("GameContext() - Initialize : ... Add Default State : MainMenu ...");
    }

    GameHelpers::sRemovableComponentsOnCopyAttributes_.Clear();
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(GOC_BodyExploder2D::GetTypeStatic().Value());
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(GOC_Abilities::GetTypeStatic().Value());
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(GOC_ZoneEffect::GetTypeStatic().Value());
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(GOC_EntityAdder::GetTypeStatic().Value());
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(CollisionCircle2D::GetTypeStatic().Value());
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(CollisionBox2D::GetTypeStatic().Value());
    GameHelpers::sRemovableComponentsOnCopyAttributes_.Push(Light::GetTypeStatic().Value());

    // Populate Keys & Joysticks Maps
    keysMap_.Resize(MAX_NUMPLAYERS);
    buttonsMap_.Resize(MAX_NUMPLAYERS);
    for (int i=0; i < MAX_NUMPLAYERS; i++)
    {
        keysMap_[i] = PODVector<int>(defaultkeysMap_[i], MAX_NUMACTIONS);
        buttonsMap_[i] = PODVector<int>(defaultbuttonsMap_[i], MAX_NUMACTIONS);
    }

    sMapStack_.Resize(10000);

    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - Initialize ....     OK !              -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
}

void GameContext::InitMouse(int mode)
{
//    if (gameConfig_.touchEnabled_)
//        return;

    Input* input = context_->GetSubsystem<Input>();

    if (!cursor_)
    {
        URHO3D_LOGINFOF("GameContext() - InitMouse : Set Cursor ...");

        SharedPtr<Image> image(context_->GetSubsystem<ResourceCache>()->GetResource<Image>(textureFiles_[UIEQUIPMENT]));

        if (image)
        {
            cursor_ = new Cursor(context_);

            context_->GetSubsystem<UI>()->SetCursor(cursor_);

            cursor_->SetUseSystemShapes(false);
            cursor_->SetBlendMode(BLEND_ALPHA);
            cursor_->SetOpacity(0.9f);

            CursorShapeInfo shapeinfo;
            shapeinfo.image_ = image;
            shapeinfo.texture_ = textures_[UIEQUIPMENT];
            shapeinfo.hotSpot_ = IntVector2(1,1);

            // Add Shapes
            for (unsigned i = 0; i < NUMCURSORS; i++)
            {
                String shapename(cursorShapeNames_[i]);
                if (spriteSheets_[UIEQUIPMENT]->GetSprite(shapename))
                {
                    URHO3D_LOGINFOF("GameContext() - InitMouse : Set Cursor ... Add Shape %s", shapename.CString());

                    shapeinfo.imageRect_ = spriteSheets_[UIEQUIPMENT]->GetSprite(shapename)->GetRectangle();
                    cursor_->DefineShape(shapename, shapeinfo);

                    // Always set the default cursor shape "normal"
                    if (i == 0)
                        cursor_->DefineShape("Normal", shapeinfo);
                }
            }

            cursor_ = context_->GetSubsystem<UI>()->GetCursor();
            cursorShape_ = CURSOR_DEFAULT;
        }

        if (!cursor_)
        {
            cursor_ = context_->GetSubsystem<UI>()->GetCursor();
            cursorShape_ = -1;
        }

        // initialize cursor to invisible
        if (cursor_)
        {
            cursor_->SetPosition(input->GetMousePosition());
            cursor_->SetVisible(false);
        }
    }

    if (cursor_)
    {
        if (createModeOn_)
        {
            cursor_->SetUseSystemShapes(true);
            cursorShape_ = -1;
        }
        else
        {
            cursor_->SetUseSystemShapes(false);
        }
    }

    input->SetMouseMode((MouseMode)mode, true);
    input->SetMouseVisible(false, true);

    // 20200516 - Important : to not be grab to an element at start (ensure a reactive ui)
    input->SetMouseGrabbed(false, true);

    // Lock Mouse Vibilility on False
    SetMouseVisible(false, false, true);
}

void GameContext::InitJoysticks()
{
    URHO3D_LOGINFOF("GameContext() - InitJoysticks : ...");

#ifdef ACTIVE_SDLMAPPINGJOYSTICK_DB
    /*
             "There are a couple of things you can do to fix this.  First, check the SDL database to see if your stick has been added."
             "Copy the contents of https://github.com/gabomdq/SDL_GameControllerDB/blob/master/gamecontrollerdb.txt into the copy of"
             "gamecontrollerdb.txt that was shipped with Bitfighter(it should be in the install folder) and restart the game.  "

             "If that doesn't help, you may need to create a new joystick definition for your controller. To do this, download the"
             "SDL2 Gamepad Tool from http://www.generalarcade.com/gamepadtool, uncompress it, and run the  executable.  This tool will"
             "help you create a definition string for your joystick.  Copy it to the clipboard, and add it to the"
             "usergamecontrollerdb.txt file in the Bitfighter install folder, and restart the game.  If you know how to use GitHub,"
             "you can also create a pull request to submit your definition to the https://github.com/gabomdq/SDL_GameControllerDB"
             "project."
    */

    int numMappedControllers;
    numMappedControllers = SDL_GameControllerAddMappingsFromFile("Data/Input/gamecontrollerdb.txt");
    if (numMappedControllers == -1)
        URHO3D_LOGERROR("GameContext() - InitJoysticks : ... SDL Controller Mapping Error ! ");
    else
        URHO3D_LOGINFOF("GameContext() - InitJoysticks : ... SDL Controller Mapping numControllers=%d... OK ! ", numMappedControllers);

    URHO3D_LOGINFOF("GameContext() - InitJoysticks : ... SDL Controller Mapping numControllers=%d ! ", numMappedControllers);

    for (unsigned i = 0; i < SDL_NumJoysticks(); ++i)
    {
        if (SDL_IsGameController(i))
        {
            char *mapping = SDL_GameControllerMapping(SDL_GameControllerOpen(i));
            URHO3D_LOGINFOF("GameContext() - InitJoysticks : GameController(%u) : name=%s mapped=%s", i, SDL_GameControllerNameForIndex(i), mapping);
            SDL_free(mapping);
        }
        else
        {
            URHO3D_LOGERRORF("GameContext() - InitJoysticks : GameController(%u) : unknown controller !", i);
        }
    }
#endif

    ValidateJoysticks();
}

void GameContext::ValidateJoysticks()
{
    URHO3D_LOGINFOF("GameContext() - ValidateJoysticks ...");

    for (unsigned i=0; i<4; i++)
        joystickIndexes_[i] = -1;

    Input* input = context_->GetSubsystem<Input>();
    int numjoysticks = input->GetNumJoysticks();

    if (!numjoysticks)
        return;

    // Check Joysticks
    int j = 0;

    URHO3D_LOGINFOF("GameContext() - ValidateJoysticks : Dump Joysticks numJoysticks=%u :", numjoysticks);

    for (int i=0; i < numjoysticks; i++)
    {
        JoystickState* joystate = input->GetJoystickByIndex(i);
        if (!joystate)
            continue;

        URHO3D_LOGINFOF(" -> Joystick[%u] : ID=%d controller=%s name=%s screenjoy=%u numbuttons=%d numhats=%d numaxis=%d",
                        i, joystate->joystickID_, joystate->IsController() ? "true":"false", joystate->name_.CString(), joystate->screenJoystick_, joystate->GetNumButtons(), joystate->GetNumHats(), joystate->GetNumAxes());

        // skip playstation sensor controller
        if (j < 4 && !joystate->name_.Contains("Sensors"))
        {
            joystickIndexes_[j] = i;
            j++;
        }
    }

    URHO3D_LOGINFOF("GameContext() - ValidateJoysticks : Dump Validated Joysticks :", numjoysticks);

    for (unsigned i=0; i<4; i++)
    {
        if (joystickIndexes_[i] == -1)
            break;

        URHO3D_LOGINFOF(" -> Joystick Id[%u] : ID=%d ", i, joystickIndexes_[i]);

        if (playerAvatars_[i])
            playerAvatars_[i]->GetComponent<GOC_PlayerController>()->SetJoystickControls(i);
    }

    URHO3D_LOGINFOF("GameContext() - ValidateJoysticks : ... OK !");
}

void GameContext::InitTouchInput()
{
    gameConfig_.touchEnabled_ = true;

    ResourceCache* cache = context_->GetSubsystem<ResourceCache>();
    Input& input = *context_->GetSubsystem<Input>();

    if (gameConfig_.screenJoystick_)
    {
        gameConfig_.screenJoystickID_ = input.AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystick.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
        gameConfig_.screenJoysticksettingsID_ = input.AddScreenJoystick(cache->GetResource<XMLFile>("UI/ScreenJoystickSettings2.xml"), cache->GetResource<XMLFile>("UI/DefaultStyle.xml"));
        input.SetScreenJoystickVisible(gameConfig_.screenJoystickID_, false);
        input.SetScreenJoystickVisible(gameConfig_.screenJoysticksettingsID_, false);

        Graphics* graphics = context_->GetSubsystem<Graphics>();
        UIElement* uiscreenjoy = input.GetJoystick(gameConfig_.screenJoystickID_)->screenJoystick_;
        uiscreenjoy->SetSize(graphics->GetWidth(), graphics->GetHeight());
    }

    URHO3D_LOGINFOF("GameContext() - InitTouchInput : screenJoystickID_=%d ", gameConfig_.screenJoystickID_);
}

void GameContext::PreselectBestControllers()
{
    int numjoysticks = context_->GetSubsystem<Input>()->GetNumJoysticks();

    // mobile devices or desktop devices with test touches
    if (gameConfig_.touchEnabled_ || gameConfig_.forceTouch_)
    {
        // remove the virtual screen joystick and the screenjoysticksettings from the list
        numjoysticks -= 2;

        // touch first and joystick after
        playerState_[0].controltype = CT_SCREENJOYSTICK;
        playerState_[1].controltype = numjoysticks > 0 ? CT_JOYSTICK : CT_CPU;
        playerState_[2].controltype = numjoysticks > 1 ? CT_JOYSTICK : CT_CPU;
        playerState_[3].controltype = numjoysticks > 2 ? CT_JOYSTICK : CT_CPU;
    }
    // desktop devices
    else
    {
        int numkeyconfigs = 2;

        // joystick first and keyboard after
        playerState_[0].controltype = numjoysticks > 0 ? CT_JOYSTICK : CT_KEYBOARD;
        if (playerState_[0].controltype == CT_KEYBOARD)
            numkeyconfigs--;
        playerState_[1].controltype = numjoysticks > 1 ? CT_JOYSTICK : numkeyconfigs > 0 ? CT_KEYBOARD : CT_CPU;
        if (playerState_[1].controltype == CT_KEYBOARD)
            numkeyconfigs--;
        playerState_[2].controltype = numjoysticks > 2 ? CT_JOYSTICK : numkeyconfigs > 0 ? CT_KEYBOARD : CT_CPU;
        if (playerState_[2].controltype == CT_KEYBOARD)
            numkeyconfigs--;
        playerState_[3].controltype = numjoysticks > 3 ? CT_JOYSTICK : numkeyconfigs > 0 ? CT_KEYBOARD : CT_CPU;
    }
}

float GameContext::GetAdjustUIFactor() const
{
    float adjusttouchfactor = 1.f;
    if (gameConfig_.touchEnabled_ || gameConfig_.forceTouch_)
    {
        if (screenInches_ < 9.f || gameConfig_.forceTouch_)
            adjusttouchfactor = uiAdjustMobileScale_;
        else if (screenInches_ < 11.f)
            adjusttouchfactor = uiAdjustTabletScale_;
    }

    return adjusttouchfactor;
}

void GameContext::ResetScreen()
{
    URHO3D_LOGINFO("GameContext() - ResetScreen ...");

    // Get Screen Size
    Graphics* graphics = context_->GetSubsystem<Graphics>();
    screenwidth_ = graphics->GetWidth();
    screenheight_ = graphics->GetHeight();
    SDL_GetDisplayDPI(0, &ddpi_, &hdpi_, &vdpi_);
    screenInches_ = Sqrt(Pow(screenwidth_ / hdpi_, 2.f) + Pow(screenheight_ / vdpi_, 2.f));

    // recalculate DPI if fullscreen in none native resolution
    // DPI = Resolution Diagonal In Pixels/Device Diagonal In Inches
    // DeviceDiagInches = Resolution Diagonal In Pixels/DPI
    // Diagonal In Pixels = √(width² + height²)

    float dresolution = sqrt(screenwidth_ * screenwidth_ + screenheight_ * screenheight_);
    float dscreenInches = 32.f;
    if (graphics->GetFullscreen())
        ddpi_ = dresolution/dscreenInches;

    dscreenInches = dresolution / ddpi_;

    screenRatioX_ = (float)screenwidth_ / (float)targetwidth_;
    screenRatioY_ = (float)screenheight_ / (float)targetheight_;

    // Set UI Size (different from screen size if uiscale)
    if (!ui_)
        ui_ = context_->GetSubsystem<UI>();

    // Set UI Scale
    float adjusttouchfactor = GetAdjustUIFactor();
    float newscale = Clamp((float)screenwidth_ / (float)targetwidth_, uiScaleMin_, uiScaleMax_) * adjusttouchfactor;
    if (newscale != uiScale_)
        uiScale_ = newscale;

#ifdef ACTIVE_CREATEMODE
    ui_->SetScale(!MapEditor::Get() ? uiScale_ : 1.f);
#else
    ui_->SetScale(uiScale_);
#endif

    URHO3D_LOGINFOF("GameContext() - ResetScreen ... screensize=%dx%d screen=%F\"(%F) DDPI=%F screenRatio=%F,%F uiscale=%f adjusttouchfactor=%f rootelementsize=%s",
                    screenwidth_, screenheight_, screenInches_, dscreenInches, ddpi_, screenRatioX_, screenRatioY_,
                    uiScale_, adjusttouchfactor, ui_->GetRoot()->GetSize().ToString().CString());

    // Resize Viewports
    ViewManager::Get()->ResizeViewports();

    if (camera_)
        rootScene_->GetOrCreateComponent<Renderer2D>()->UpdateFrustumBoundingBox(camera_);

    // Resize Preloader Icon
    if (preloaderIcon_)
    {
        Texture2D* texture = static_cast<Texture2D*>(preloaderIcon_->GetTexture());
        preloaderIcon_->SetSize(floor((float)texture->GetWidth() * uiScale_), floor((float)texture->GetHeight() * uiScale_));
        preloaderIcon_->SetHotSpot(preloaderIcon_->GetSize()/2);

        float maxoffset = (float)Max(preloaderIcon_->GetWidth(), preloaderIcon_->GetHeight()) / 2.f;

        preloaderIcon_->SetPosition(maxoffset, ui_->GetRoot()->GetSize().y_-maxoffset);
    }

    // Resize the Screen Joystick
    if (gameConfig_.screenJoystickID_ != -1)
    {
        float uijoyscale = 1.f;
        if (gameConfig_.touchEnabled_ || gameConfig_.forceTouch_)
        {
            if (screenInches_ < 9.f)
                uijoyscale = uiAdjustMobileScale_;// * 1.25f;
            else if (screenInches_ < 11.f)
                uijoyscale = uiAdjustTabletScale_;// * 1.25f;
        }

        UIElement* uiscreenjoy = input_->GetJoystick(gameConfig_.screenJoystickID_)->screenJoystick_;
        if (uiscreenjoy)
        {
            uiscreenjoy->SetSize(ui_->GetRoot()->GetSize());

            // Resize the buttons Actions
            /*
                *4
             3*    *1
                *2
            */
            const float border = 15.f;
            const float spacing = 2.f;
            const float buttonsize = 80.f;
            const float coordx[4] = { -border, -(border+spacing+buttonsize), -(border+2.f*spacing+2.f*buttonsize), -(border+spacing+buttonsize) };
            const float coordy[4] = { -(border+spacing+buttonsize), -border, -(border+spacing+buttonsize), -(border+2.f*spacing+2.f*buttonsize) };
            for (int i=0; i < 4; i++)
            {
                int size = (int)(buttonsize * uijoyscale);
                UIElement* button = uiscreenjoy->GetChild(String("Button")+String(i));
                button->SetSize(size, size);
                button->SetPosition((int)(coordx[i] * uijoyscale), (int)(coordy[i] * uijoyscale));
            }

            // Resize the hat
            const float borderhat = 30.f;
            const float hatsize = 200.f;
            int size = (int)(hatsize * uijoyscale);
            UIElement* hat = uiscreenjoy->GetChild(String("Hat0"));
            hat->SetSize(size, size);
            int coord = (int)(borderhat * uijoyscale);
            hat->SetPosition(coord, -coord);
        }
    }
}


void GameContext::CreatePreloaderIcon()
{
    Texture2D* iconTexture = context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/UI/loadericon.png");

    if (!iconTexture)
    {
        URHO3D_LOGERRORF("No Loader Icon Texture !");
        return;
    }

    if (!ui_)
        ui_ = context_->GetSubsystem<UI>();

    preloaderIcon_ = ui_->GetRoot()->CreateChild<Sprite>();
    preloaderIcon_->SetTexture(iconTexture);
    preloaderIcon_->SetSize(floor((float)iconTexture->GetWidth() * uiScale_), floor((float)iconTexture->GetHeight() * uiScale_));
    preloaderIcon_->SetHotSpot(preloaderIcon_->GetSize()/2);
    float maxoffset = (float)Max(preloaderIcon_->GetWidth(), preloaderIcon_->GetHeight()) / 2.f;
    preloaderIcon_->SetPosition(maxoffset, ui_->GetRoot()->GetSize().y_-maxoffset);
    preloaderIcon_->SetOpacity(0.98f);
    preloaderIcon_->SetPriority(1000);

    SharedPtr<ObjectAnimation> objectAnimation(new ObjectAnimation(context_));
    SharedPtr<ValueAnimation> rotateAnimation(new ValueAnimation(context_));
    rotateAnimation->SetKeyFrame(0.f, 0.f);
    rotateAnimation->SetKeyFrame(0.5f, 360.f);
    objectAnimation->AddAttributeAnimation("Rotation", rotateAnimation, WM_LOOP);
    preloaderIcon_->SetObjectAnimation(objectAnimation);

    preloaderIcon_->SetVisible(true);

    URHO3D_LOGINFOF("GameContext() - CreateLoaderIcon ... OK !");
}

bool GameContext::PreloadResources()
{
#ifdef ACTIVE_PRELOADER
    HiresTimer timer;
    bool ok = false;

    if (preloaderState_ == 0)
    {
        if (!rootScene_)
        {
            URHO3D_LOGERROR("GameContext() - PreLoadResources : no scene !");
            return true;
        }

        URHO3D_LOGINFO("GameContext() ---------------------------------------------------------");
        URHO3D_LOGINFO("GameContext() - PreLoadResources ...                                  -");
        URHO3D_LOGINFO("GameContext() ---------------------------------------------------------");

        ReserveAvatarNodes();

        GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, false);

        GOT::LoadJSONFile(context_, "Data/Objects/Avatars.json");

        GOT::LoadJSONFile(context_, "Data/Objects/Objects.json");
#if defined(USE_ANIMATEDFURNITURES)
        GOT::LoadJSONFile(rootScene_->GetContext(), "Data/Furnitures/Furnitures.json");
#else
        GOT::LoadJSONFile(rootScene_->GetContext(), "Data/Furnitures/FurnituresStatic.json");
#endif
        GOT::LoadJSONFile(context_, "Data/Effects/Effects.json");

        CreatePreloaderIcon();

        preloading_ = true;
        preloaderState_++;
    }

    if (preloaderState_ == 1)
    {
        CraftRecipes::LoadJSONFile(context_, "Data/Objects/Recipes.json");

        ScrapsEmitter::RegisterType(context_, "Scraps_Bone|2D/scrapsbone.xml");
        ScrapsEmitter::RegisterType(context_, "Scraps_Bomb|2D/scrapsbomb.xml");
        ScrapsEmitter::RegisterType(context_, "Scraps_ElsarionMeat|2D/scrapselsarionmeat.xml");

        preloaderState_++;

        if (TimeOver(&timer, preloadDelayUsec_))
            return false;
    }

    if (preloaderState_ == 2)
    {
        Node* preloader = rootScene_->GetChild("PreLoad");
        if (!preloader)
        {
            URHO3D_LOGINFOF("GameContext() - PreLoadResources : PreLoader.xml ...");

            preloader = rootScene_->CreateChild("PreLoad", LOCAL);

            if (GameHelpers::LoadNodeXML(context_, preloader, "Data/Scenes/PreLoader.xml", LOCAL))
                preloader->SetEnabled(false);
        }

        preloadGOTNode_ = rootScene_->GetChild("PreLoadGOT");
        if (!preloadGOTNode_)
        {
            URHO3D_LOGINFOF("GameContext() - PreLoadResources : PreLoadGOT ...");
            preloadGOTNode_ = rootScene_->CreateChild("PreLoadGOT", LOCAL);
            preloaderState_++;
        }
        else
        {
            ok = true;
        }
    }

    if (preloaderState_ > 2)
    {
#ifdef ACTIVE_POOL
        if (GOT::PreLoadObjects(preloaderState_, &timer, preloadDelayUsec_, preloadGOTNode_, true))
#else
        if (GOT::PreLoadObjects(preloaderState_, &timer, preloadDelayUsec_, preloadGOTNode_, false))
#endif
        {
            ok = true;
        }
    }

    if (ok)
    {
        GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, true);

#ifdef DUMP_RESOURCES
        URHO3D_LOGINFO("GameContext() ----------------------------------------------------------------");
        URHO3D_LOGINFO("GameContext() - PreLoadResources ... Dump ...                                -");
        URHO3D_LOGINFO("GameContext() ----------------------------------------------------------------");

        if (ObjectPool::Get())
            ObjectPool::Get()->DumpCategories();
#endif

        DumpGameLibrary();

        unsigned int lastid = rootScene_->GetFreeNodeID(LOCAL) - 1;
        URHO3D_LOGINFOF("GameContext() - PreLoadResources : Total Nodes ID=%u to ID=%u (size=%u) !", FIRSTAVATARNODEID, lastid, lastid-FIRSTAVATARNODEID+1);

        URHO3D_LOGINFO("GameContext() ----------------------------------------------------------------");
        URHO3D_LOGINFO("GameContext() - PreLoadResources ... OK !                                    -");
        URHO3D_LOGINFO("GameContext() ----------------------------------------------------------------");

        preloadGOTNode_->SetEnabled(false);
        preloaderState_ = 0;
        if (preloaderIcon_)
        {
            preloaderIcon_->SetVisible(false);
            preloaderIcon_->SetAnimationEnabled(false);
        }
        preloading_ = false;

        return true;
    }
#endif

    return false;
}

void GameContext::ReserveAvatarNodes()
{
    if (!rootScene_)
    {
        URHO3D_LOGERRORF("GameContext() - ReserveAvatarNodes : no scene !");
        return;
    }

    unsigned numavatarnodes = MAX_NUMPLAYERS * MAX_NUMNETPLAYERS;
    unsigned id = 0;

    for (unsigned i=0; i < numavatarnodes; i++)
    {
        id = rootScene_->GetFreeNodeID(LOCAL);
        if (i == 0)
            FIRSTAVATARNODEID = id;
        rootScene_->ReserveNodeID(id);
    }

    URHO3D_LOGINFOF("GameContext() - ReserveAvatarNodes : Reserve Avatar Nodes ID=%u to ID=%u !", FIRSTAVATARNODEID, id);
}

bool GameContext::IsAvatarNodeID(unsigned id, int clientid) const
{
    if (clientid == -1)
        return (id >= FIRSTAVATARNODEID && id < FIRSTAVATARNODEID + MAX_NUMPLAYERS * MAX_NUMNETPLAYERS);

    return (id >= FIRSTAVATARNODEID + clientid*MAX_NUMPLAYERS && id < FIRSTAVATARNODEID + (clientid+1)*MAX_NUMPLAYERS);
}

bool GameContext::UnloadResources()
{
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - UnloadRessources  ....                -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");

//    GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, false);

    if (rootScene_)
    {
        if (preloadGOTNode_)
        {
            URHO3D_LOGINFO("UnloadRessources ... -> PreLoaderGOT To Remove ...");

            GOT::UnLoadObjects(preloadGOTNode_);

            URHO3D_LOGINFO("UnloadRessources ... -> PreLoaderGOT Removed OK !");
        }

        Node* preloader = rootScene_->GetChild("PreLoad");
        if (preloader)
        {
            URHO3D_LOGINFO("UnloadRessources ... -> PreLoader To Remove ...");

            preloader->Remove();

            URHO3D_LOGINFO("UnloadRessources ... -> PreLoader Removed OK !");
        }
    }

//    GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, true);

    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - UnloadRessources  .... OK !           -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");

    return true;
}

void GameContext::Start()
{
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - Start  ....                           -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");

#ifdef ACTIVE_NETWORK
    // Start Network
    if (GameNetwork::Get())
        GameNetwork::Get()->Start();
#endif
    // Start GameState
    stateManager_->StartStack();

    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - Start  .... OK !                      -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
}

void GameContext::SetMouseVisible(bool uistate, bool osstate, bool lockstate)
{
    if (mouseLockState_ == uistate && !lockstate)
        mouseLockState_ = -1;

    if (mouseLockState_ != -1 && !lockstate)
        return;

    Input* input = context_->GetSubsystem<Input>();

    if (lockstate)
        mouseLockState_ = uistate;

    if (gameConfig_.touchEnabled_)
    {
        uistate = false;
        osstate = false;
    }

    if (cursor_)
    {
        input->SetMouseVisible(osstate);
        cursor_->SetVisible(uistate);

        if (uistate)
            cursor_->SetOpacity(1.f);
    }
    else
    {
        input->SetMouseVisible(uistate);
    }

//    URHO3D_LOGINFOF("GameContext() - SetMouseVisible : uicursor=%s oscursor=%s touchenabled=%s",
//                    uistate ? "true":"false", osstate ? "true":"false", gameConfig_.touchEnabled_ ? "true":"false");
}

void GameContext::SetRenderDebug(bool enable)
{
//    if (!gameConfig_.debugRenderEnabled_)
//        return;

    DrawDebug_ = enable;

//    if (stateManager_)
//    {
//        const String& stateId = stateManager_->GetActiveState()->GetStateId();
//        if (stateId == "Play")
//        {
//            PlayState* playstate = (PlayState*)stateManager_->GetActiveState();
//            if (DrawDebug_ && rootScene_->GetOrCreateComponent<DebugRenderer>())
//            {
//                playstate->SubscribeToEvent(E_POSTRENDERUPDATE, new Urho3D::EventHandlerImpl<PlayState>(playstate, &PlayState::OnPostRenderUpdate));
//            }
//            else
//            {
//                playstate->UnsubscribeFromEvent(E_POSTRENDERUPDATE);
//            }
//        }
//    }
}

void GameContext::ResetLuminosity()
{
    luminosity_ = 1.f;
    context_->GetSubsystem<Renderer>()->SetMaterialQuality(QUALITY_LOW);
}

void GameContext::Stop()
{
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - Stop  ....                            -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");

    SendEvent(GAME_EXIT);

    gameContext_->UnsubscribeFromAllEvents();

    // Stop current State (Menu, Play etc...) and delete Manager
    if (stateManager_)
    {
        stateManager_->Stop();
        delete stateManager_;
        stateManager_ = 0;
    }

    if (gameConfig_.touchEnabled_)
        input_->RemoveScreenJoystick(gameConfig_.screenJoystickID_);

    if (ui_)
        ui_->GetRoot()->RemoveAllChildren();

    if (gameWorkQueue_)
    {
        delete gameWorkQueue_;
        gameWorkQueue_ = 0;
    }

    AnimatedSprite2D::SetRenderTargetContext();
    rttScene_.Reset();

    if (rootScene_)
        rootScene_->CleanupNetwork();

    if (rootScene_)
        GameHelpers::StopMusic(rootScene_);

    if (ViewManager::Get())
        delete ViewManager::Get();

#ifdef ACTIVE_PRELOADER
    UnloadResources();
#endif

    uiFonts_.Clear();
    spriteSheets_.Clear();
#ifdef ACTIVE_LAYERMATERIALS
    layerMaterials_.Clear();
#endif
    textures_.Clear();

#ifdef DUMP_SCENE_BEFORERESET
    URHO3D_LOGINFO("GameContext() - Stop  .... Dump Scene Before Reset ...");
    GameHelpers::DumpNode(rootScene_, 2, true);
    GameHelpers::DumpObject(rootScene_);
#endif // DUMP_SCENE

    cameraNode_.Reset();
    rootScene_.Reset();

#ifdef DUMP_SCENE_AFTERRESET
    URHO3D_LOGINFO("GameContext() - Stop  .... Dump Scene After Reset ...");
    GameHelpers::DumpObject(rootScene_);
#endif // DUMP_SCENE

    UnRegisterGameLibrary(context_);

    URHO3D_LOGINFO("GameContext() -----------------------------------------");
    URHO3D_LOGINFO("GameContext() - Stop  .... OK !                       -");
    URHO3D_LOGINFO("GameContext() -----------------------------------------");
}

void GameContext::Exit()
{
    if (context_)
        context_->GetSubsystem<Engine>()->Exit();
}

void GameContext::SetConsoleVisible(bool state)
{
    if (console_)
        console_->SetVisible(state);
}

bool GameContext::HasConsoleFocus()
{
    return console_ && console_->HasLineEditFocus();
}

void GameContext::ResetNetworkStatics()
{
    // Net Statics
    gameNetwork_ = GameNetwork::Get();
    LocalMode_ = GameNetwork::LocalMode();
    ServerMode_ = GameNetwork::ServerMode();
    ClientMode_ = GameNetwork::ClientMode();
}

void GameContext::ResetGameStates()
{
    testZoneOn_ = false;
    arenaZoneOn_ = false;
    createModeOn_ = false;
    loadSavedGame_ = false;
#ifdef ACTIVE_CREATEMODE
    if (forceCreateMode_)
        createModeOn_ = true;
#endif
}

int GameContext::SetLevel(int level)
{
    enableWinLevel_ = true;

    if (level > 0)
        indexLevel_ = level;

    if (!testZoneOn_ && !arenaZoneOn_ && !createModeOn_)
    {
        enableMissions_ = true;
    }
    else
    {
        enableMissions_ = testZoneOn_;

        if (testZoneOn_)
            indexLevel_ = MAX_NUMLEVELS + 1 + testZoneId_;
        else if (arenaZoneOn_)
            indexLevel_ = MAX_NUMLEVELS + 1;
    }

    URHO3D_LOGINFOF("GameContext() - SetLevel indexLevel=%d testZoneId_=%d !", indexLevel_, testZoneId_);

    enableMissions_ = true;

    currentMusic = sceneToLoad_[indexLevel_-1][1];

    return indexLevel_;
}

void GameContext::UpdateLevel()
{
    indexLevel_++;
    if (indexLevel_ > MAX_NUMLEVELS)
        indexLevel_ = 1;

    SetLevel(indexLevel_);
}

Node* GameContext::FindMapPositionAt(const String& favoriteArea, const ShortIntVector2& mPoint, IntVector2& position, int& viewZ, Node* excludeNode)
{
    URHO3D_LOGINFOF("GameContext() - FindMapPositionAt : entry area=%s mPoint=%s viewZ=%d ...", favoriteArea.CString(), mPoint.ToString().CString(), viewZ);

    // Find a named area zone
    if (!favoriteArea.Empty() && favoriteArea != "GOT_Start")
    {
        PODVector<Node*> areas = World2D::FindEntitiesAt(favoriteArea, mPoint, viewZ);
        if (areas.Size())
        {
            if (areas.Size() == 1)
            {
                Node* area = areas[0];
                World2D::GetWorldInfo()->Convert2WorldMapPosition(mPoint, area->GetWorldPosition2D(), position);
                viewZ = area->GetVar(GOA::ONVIEWZ).GetInt();
                URHO3D_LOGINFOF("GameContext() - FindMapPositionAt : ... find just one %s NodeId=%u at viewZ=%d => no excludeNode",
                                favoriteArea.CString(), area->GetID(), viewZ);
                return area;
            }
            else
            {
                for (unsigned i=0; i < areas.Size(); ++i)
                {
                    Node* area = areas[i];
                    if (area != excludeNode)
                    {
                        World2D::GetWorldInfo()->Convert2WorldMapPosition(mPoint, area->GetWorldPosition2D(), position);
                        viewZ = area->GetVar(GOA::ONVIEWZ).GetInt();
                        URHO3D_LOGINFOF("GameContext() - FindMapPositionAt : ... find a %s NodeId=%u at viewZ=%d (excludeNodeID=%u)",
                                        favoriteArea.CString(), area->GetID(), viewZ, excludeNode ? excludeNode->GetID() : 0);
                        return area;
                    }
                }
            }
        }
    }
    // Else Find a GOT_Start
    {
        PODVector<Node*> areas = World2D::FindEntitiesAt("GOT_Start", mPoint, viewZ);
        if (areas.Size())
        {
            Node* area = areas[0];
            World2D::GetWorldInfo()->Convert2WorldMapPosition(mPoint, area->GetWorldPosition2D(), position);
            viewZ = area->GetVar(GOA::ONVIEWZ).GetInt();

            // correct GOT_START y position
            position.y_--;

            URHO3D_LOGINFOF("GameContext() - FindMapPositionAt : ... find a GOT_Start NodeId=%u at viewZ=%d position=%s !",
                            area->GetID(), viewZ, position.ToString().CString());
            return area;
        }
    }

    URHO3D_LOGWARNINGF("GameContext() - FindMapPositionAt : entry area=%s mPoint=%s ... NOT FOUND", favoriteArea.CString(), mPoint.ToString().CString());
    return 0;
}

//#define SETWORLDSTART_SKIP 3
void GameContext::SetWorldStartPosition()
{
    World2D* world = World2D::GetWorld();
    if (!world)
        return;

    bool findstart = false;

    ShortIntVector2 mpoint = ShortIntVector2(world->GetDefaultMapPoint().x_, world->GetDefaultMapPoint().y_);

    // Reset Start Point
    worldStartPosition_.position_ = Vector2::ZERO;
    worldStartPosition_.viewZ_ = FRONTVIEW;

#if (SETWORLDSTART_SKIP < 1)
    // Find a startPoint with the needed viewZ
    if (!findstart)
    {
        Node* toFocus = 0;

        PODVector<Node*> startNodes;
        World2D::GetEntities(mpoint, startNodes, GOT::START);

        if (startNodes.Size())
        {
            URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : numStartPoints=%u on mpoint=%s ...", startNodes.Size(), mpoint.ToString().CString());

            const int tryViewZ[2] = { FRONTVIEW, INNERVIEW };

            for (unsigned i=0; i < 2; i++)
            {
                for (unsigned j=0; j < startNodes.Size(); j++)
                {
                    if (startNodes[j]->GetVar(GOA::ONVIEWZ).GetInt() == tryViewZ[i])
                    {
                        toFocus = startNodes[j];
                        URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : startindex=%u on mpoint=%s for viewZ=%d... toFocus=%s(%u) ...",
                                        j, mpoint.ToString().CString(), tryViewZ[i], toFocus ? toFocus->GetName().CString() : "NONE", toFocus ? toFocus->GetID() : 0);
                        break;
                    }
                }

                if (toFocus)
                    break;
            }
        }

        if (toFocus)
        {
            worldStartPosition_.position_ = toFocus->GetPosition2D();
            worldStartPosition_.viewZ_ = toFocus->GetVar(GOA::ONVIEWZ).GetInt();
        }
    }
#endif
    // Find a safe place in the map
    if (!findstart)
    {
        Map* map = world->GetMapAt(mpoint, true);
#if (SETWORLDSTART_SKIP < 2)
        if (map->GetMapData() && map->GetMapData()->spots_.Size())
        {
            PODVector<const MapSpot*> typedspots;
            MapSpot::GetSpotsOfType(SPOT_START, map->GetMapData()->spots_, typedspots);
            if (typedspots.Size())
            {
                findstart = true;

                worldStartPosition_.mPosition_ = typedspots[0]->position_;
                worldStartPosition_.viewZ_ = ViewManager::Get()->GetViewZ(typedspots[0]->viewZIndex_);
                worldStartPosition_.viewZIndex_ = typedspots[0]->viewZIndex_;

                URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : Find a Start Spot on mpoint=%s at %s viewZ=%d !",
                                mpoint.ToString().CString(), worldStartPosition_.mPosition_.ToString().CString(), worldStartPosition_.viewZ_);
            }
        }
#endif
        if (!findstart)
        {
#if (SETWORLDSTART_SKIP < 3)
            if (map->GetMapData() && map->GetMapData()->spots_.Size())
            {
                findstart = true;

                worldStartPosition_.mPosition_ = map->GetMapData()->spots_[0].position_;
                worldStartPosition_.viewZ_ = ViewManager::Get()->GetViewZ(map->GetMapData()->spots_[0].viewZIndex_);
                worldStartPosition_.viewZIndex_ = map->GetMapData()->spots_[0].viewZIndex_;

                URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : Get First Spot on mpoint=%s at %s viewZ=%d !",
                                mpoint.ToString().CString(), worldStartPosition_.mPosition_.ToString().CString(), worldStartPosition_.viewZ_);
            }
#endif
            if (!findstart && map->GetTopography().fullSky_)
            {
                findstart = true;

                worldStartPosition_.mPosition_.x_ = map->GetWidth() / 2;
                worldStartPosition_.mPosition_.y_ = map->GetHeight() / 2;
                worldStartPosition_.viewZ_ = FRONTVIEW;

                URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : Get Center Position on FullSky on mpoint=%s at %s viewZ=%d !",
                                mpoint.ToString().CString(), worldStartPosition_.mPosition_.ToString().CString(), worldStartPosition_.viewZ_);
            }

            if (!findstart && map->GetTopography().fullGround_)
            {
                if (map->FindEmptySpace(3, 2, InnerView_ViewId, worldStartPosition_.mPosition_, 1))
                {
                    findstart = true;

                    worldStartPosition_.viewZ_ = INNERVIEW;

                    URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : Find Position on FullGround on mpoint=%s at %s viewZ=%d !",
                                    mpoint.ToString().CString(), worldStartPosition_.mPosition_.ToString().CString(), worldStartPosition_.viewZ_);
                }
            }

            if (!findstart)
            {
                // the worst case : must check the frontview and the innerview
                if (map->FindEmptySpace(3, 2, FrontView_ViewId, worldStartPosition_.mPosition_, 1))
                {
                    findstart = true;

                    worldStartPosition_.viewZ_ = FRONTVIEW;

                    URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : Find Position on mpoint=%s at %s viewZ=%d !",
                                    mpoint.ToString().CString(), worldStartPosition_.mPosition_.ToString().CString(), worldStartPosition_.viewZ_);
                }
                else if (map->FindEmptySpace(3, 2, InnerView_ViewId, worldStartPosition_.mPosition_, 1))
                {
                    findstart = true;

                    worldStartPosition_.viewZ_ = INNERVIEW;

                    URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : Find Position on mpoint=%s at %s viewZ=%d !",
                                    mpoint.ToString().CString(), worldStartPosition_.mPosition_.ToString().CString(), worldStartPosition_.viewZ_);
                }
            }
        }

        if (!findstart)
        {
            findstart = true;

            worldStartPosition_.mPosition_.x_ = map->GetWidth() / 2;
            worldStartPosition_.mPosition_.y_ = map->GetHeight() / 2;
            worldStartPosition_.viewZ_ = INNERVIEW;

            URHO3D_LOGERRORF("GameContext() - SetWorldStartPosition : Can't find Position on mpoint=%s => set centered tile !", mpoint.ToString().CString());
        }

        if (findstart)
            world->GetWorldInfo()->Convert2WorldPosition(mpoint, worldStartPosition_.mPosition_, worldStartPosition_.position_);
    }

    URHO3D_LOGINFOF("GameContext() - SetWorldStartPosition : StartPosition=%s ... ", worldStartPosition_.ToString().CString());
}

bool GameContext::GetStartPosition(WorldMapPosition& position, int i)
{
    World2D* world = World2D::GetWorld();
    if (!world)
        return false;

    ShortIntVector2 mpoint = ShortIntVector2(world->GetDefaultMapPoint().x_, world->GetDefaultMapPoint().y_);
    Node* toFocus = 0;

    PODVector<Node*> startNodes;
    World2D::GetEntities(mpoint, startNodes, GOT::START);

    if (!startNodes.Size())
    {
        position.viewZ_ = FRONTVIEW;
        world->GetWorldInfo()->Convert2WorldPosition(mpoint, IntVector2(1,2), position.position_);
        return false;
    }

    Node* startnode = startNodes[i % startNodes.Size()];
    if (startnode)
    {
        position.position_ = startnode->GetPosition2D();
        position.viewZ_ = startnode->GetVar(GOA::ONVIEWZ).GetInt();
    }

    URHO3D_LOGINFOF("GameContext() - GetStartPosition : StartPosition=%f %f mpoint=%s startnode=%s(%u) ... ",
                    position.position_.x_, position.position_.y_, mpoint.ToString().CString(),
                    startnode ? startnode->GetName().CString() : "NONE", startnode ? startnode->GetID() : 0);
    return true;
}

void GameContext::Dump() const
{
    gameState_.Dump();

    URHO3D_LOGINFOF("  => AllowUpdate_   = %s", AllowUpdate_ ? "true" : "false");
    URHO3D_LOGINFOF("  => LocalMode_  = %s", LocalMode_ ? "true" : "false");
    URHO3D_LOGINFOF("  => ServerMode_  = %s", ServerMode_ ? "true" : "false");
    URHO3D_LOGINFOF("  => ClientMode_  = %s", ClientMode_ ? "true" : "false");
}


#define DELAY_INACTIVECURSOR 5.f
float timerInactiveCursor_ = 0.f;

void GameContext::SubscribeToCursorVisiblityEvents()
{
    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(GameContext, HandleBeginUpdate));
    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(GameContext, HandleCursorVisibility));
}

void GameContext::UnsubscribeFromCursorVisiblityEvents()
{
    // Inactive Cursor Visibility Handle
    UnsubscribeFromEvent(E_SCENEUPDATE);
    UnsubscribeFromEvent(E_MOUSEMOVE);
}

void GameContext::HandleBeginUpdate(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFO("GameContext() - HandleBeginUpdate ... ");

    if (cursor_)
    {
        bool uIElementNeedsCursor = createModeOn_;

        if (gameConfig_.touchEnabled_ && input_->GetTouch(0) && input_->GetTouch(0)->GetTouchedElement())
        {
            uIElementNeedsCursor = input_->GetTouch(0)->GetTouchedElement()->HasTag("AllowTouchCursor");

            if (uIElementNeedsCursor)
            {
                // Set Cursor Position
                IntVector2 cursorposition = input_->GetTouch(0)->position_;
                cursorposition.x_ = cursorposition.x_ / uiScale_;
                cursorposition.y_ = cursorposition.y_ / uiScale_;
                cursor_->SetPosition(cursorposition);

                // Force Cursor Visibility
                timerInactiveCursor_ = 0.f;
                cursor_->SetVisible(true);
                cursor_->SetOpacity(1.f);
            }
        }

        if (!uIElementNeedsCursor && gameConfig_.autoHideCursorEnable_ && cursor_->IsVisible())
        {
            timerInactiveCursor_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();
            if (timerInactiveCursor_ > DELAY_INACTIVECURSOR)
            {
//                URHO3D_LOGINFO("GameContext() - HandleBeginUpdate ... Mouse Cursor Disable !");
                SetMouseVisible(false, false);
            }
            else
            {
                cursor_->SetOpacity((DELAY_INACTIVECURSOR - timerInactiveCursor_) / DELAY_INACTIVECURSOR);
            }
        }
    }

    if (cursorShape_ != -1 && cursor_ && cursor_->IsVisible())
    {
        cursor_->SetShape(cursorShapeNames_[cursorShape_]);
    }
}

void GameContext::HandleCursorVisibility(StringHash eventType, VariantMap& eventData)
{
//    if (!input_->HasFocus())
//        return;

    if (eventType == E_MOUSEMOVE)
    {
        if (!cursor_)
            InitMouse(MM_FREE);

        if (mouseLockState_ == -1)
        {
            if (!cursor_->IsVisible())
                SetMouseVisible(true, false);
            else
                cursor_->SetOpacity(1.f);
            timerInactiveCursor_ = 0.f;

//				URHO3D_LOGINFO("GameContext() - HandleCursorVisibility ... E_MOUSEMOVE Cursor ! ");
        }
    }
}
