#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Engine/Engine.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/MessageBox.h>

#include "GameOptions.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameNetwork.h"

#include "MapWorld.h"
#include "ViewManager.h"

#include "GEF_Scrolling.h"

#include "sPlay.h"
#include "sOptions.h"

#include "sMainMenu.h"


const int numMenuChoices_ = 2;

WeakPtr<Node> titlescene_;
Scene* rootScene_;
UI* ui_;

int menustate_ = 0, lastmenustate_ = 0;
bool drawDebug_;
IntRect arenaRect_, worldRect_;

Node* buttonarena = 0;
Node* buttonworld = 0;

extern const char* levelModeNames[];


MenuState::MenuState(Context* context) :
    GameState(context, "MainMenu")
{
    drawDebug_ = 0;
    URHO3D_LOGDEBUG("MenuState()");
}

MenuState::~MenuState()
{
    URHO3D_LOGDEBUG("~MenuState()");
}


bool MenuState::Initialize()
{
    CreateScene();

    return GameState::Initialize();
}

void MenuState::Begin()
{
    if (IsBegun())
        return;

    URHO3D_LOGINFO("MenuState() - ----------------------------------------");
    URHO3D_LOGINFO("MenuState() - Begin ...                              -");
    URHO3D_LOGINFO("MenuState() - ----------------------------------------");

    GameContext::Get().lastLevelMode_ = LVL_NEW;
    if (!GameContext::Get().ServerMode_ && !GameContext::Get().numPlayers_)
        GameContext::Get().numPlayers_ = 1;

    // Reset controls
    GetSubsystem<Input>()->ResetStates();

    // Set UI
    CreateUI();

    StartScene();

    if (GameNetwork::Get())
    {
        GameNetwork::Get()->Start();
        GameNetwork::Get()->SetGameStatus(MENUSTATE);
    }

    // Play Theme
    GameContext::Get().currentMusic = "Music/MainTheme.ogg";
    GameHelpers::PlayMusic(rootScene_, GameContext::Get().currentMusic, true);

    //  Subscribers
    if (GameContext::Get().IsPreloading())
    {
        SubscribeToEvent(GAME_PRELOADINGFINISHED, URHO3D_HANDLER(MenuState, HandlePreloadingFinished));
    }
    else
    {
        OptionState* optionState = (OptionState*)stateManager_->GetState("Options");
        if (optionState)
        {
            optionState->SetVisibleAccessButton(true);
            optionState->CloseFrame();

            SubscribeToEvent(optionState, GAME_OPTIONSFRAME_OPEN, URHO3D_HANDLER(MenuState, HandleOptionsFrame));
            SubscribeToEvent(optionState, GAME_OPTIONSFRAME_CLOSED, URHO3D_HANDLER(MenuState, HandleOptionsFrame));
        }

        SubscribeToMenuEvents();
    }

    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(MenuState, HandleScreenResized));
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(MenuState, HandleKeyDown));

    GameState::Begin();

    SendEvent(E_BEGINVIEWUPDATE);
    SendEvent(E_SCREENMODE);

    GameContext::Get().ResetLuminosity();

    URHO3D_LOGINFO("MenuState() - ----------------------------------------");
    URHO3D_LOGINFO("MenuState() - Begin ...  OK !                        -");
    URHO3D_LOGINFO("MenuState() - ----------------------------------------");
}

void MenuState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("MenuState() - ----------------------------------------");
    URHO3D_LOGINFO("MenuState() - End ...                                -");
    URHO3D_LOGINFO("MenuState() - ----------------------------------------");

    if (titlescene_)
        titlescene_->SetName("MainMenu");

    if (ui_)
    {
        ui_->SetFocusElement(0);
    }

    OptionState* optionState = (OptionState*)GameContext::Get().stateManager_->GetState("Options");
    if (optionState)
        optionState->SetVisibleAccessButton(false);

    UnsubscribeFromAllEvents();

    // Reset controls
    GetSubsystem<Input>()->ResetStates();

    // Call base class implementation
    GameState::End();

    URHO3D_LOGINFO("MenuState() - ----------------------------------------");
    URHO3D_LOGINFO("MenuState() - End ... OK !                           -");
    URHO3D_LOGINFO("MenuState() - ----------------------------------------");
}


void MenuState::CreateScene()
{
    URHO3D_LOGINFO("MenuState() - CreateScene ...");

    // Get the scene instantiate by Game
    rootScene_ = GameContext::Get().rootScene_;

    // Update Default Components for the Scene
    rootScene_->GetOrCreateComponent<Octree>(LOCAL);
    if (GameContext::Get().gameConfig_.debugRenderEnabled_)
        rootScene_->GetOrCreateComponent<DebugRenderer>(LOCAL);

    // Load MainMenu Scene Animation
    titlescene_ = rootScene_->GetChild("title") ? rootScene_->GetChild("title") : rootScene_->CreateChild("title", LOCAL);
    Node* mainscene = titlescene_->GetChild("MainScene") ? titlescene_->GetChild("MainScene") : titlescene_->CreateChild("MainScene", LOCAL);

    GameHelpers::LoadNodeXML(context_, mainscene, "UI/Menu.xml", LOCAL);

    if (ViewManager::Get())
        ViewManager::Get()->GetCameraNode(0)->SetWorldPosition(Vector3::ZERO);
#ifdef ACTIVE_WEATHEREFFECTS
    Node* scrollRootNode = mainscene->GetChild("ScrollRootNode");
    if (!scrollRootNode)
    {
        scrollRootNode = mainscene->CreateChild("ScrollRootNode", LOCAL);
        scrollRootNode->SetPosition(Vector3::ZERO);

#ifdef ACTIVE_LAYERMATERIALS
        Material* material = GameContext::Get().layerMaterials_[LAYERBACKGROUNDS];
#else
        Material* material = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/nuages.xml");
#endif

#ifdef ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE

#ifdef ACTIVE_LAYERMATERIALS
        const int textureunit = 3;
#else
        const int textureunit = 0;
#endif

#ifdef ACTIVE_SCROLLINGSHAPE_FRUSTUMCAMERA
        Vector2 repeat(0.5, 1.f);
#else
        Vector2 repeat(1.f, 2.f);
#endif
        ScrollingShape::AddQuadScrolling(scrollRootNode, 700, 0,  material, textureunit, 16, Vector2::ZERO, repeat, Vector2(0.05f, 0.01f));
        ScrollingShape::AddQuadScrolling(scrollRootNode, 800, 0,  material, textureunit, 16, Vector2(2.f, 2.f), repeat, Vector2(0.075f, 0.02f));
        ScrollingShape::AddQuadScrolling(scrollRootNode, 900, 0,  material, textureunit, 48, Vector2(-2.f, -2.f), repeat, Vector2(0.1f, 0.01f));
        ScrollingShape::AddQuadScrolling(scrollRootNode, 1100, 0, material, textureunit, 48, Vector2::ZERO, repeat, Vector2(0.15f, -0.01f));
#else
        GEF_Scrolling::AddScrolling(material, 1, "SpriteSheet2D;Data/2D/nuages.xml@nuage2", scrollRootNode, 700, 0, 0.f, Vector2::ZERO, Vector2(-0.35f, 0.1f), false, Vector2::ZERO, Color::WHITE);
        GEF_Scrolling::AddScrolling(material, 1, "SpriteSheet2D;Data/2D/nuages.xml@nuage2", scrollRootNode, 800, 0, 0.f, Vector2(2.f, 2.f), Vector2(-0.7f, 0.2f), false, Vector2::ZERO, Color::WHITE);
        GEF_Scrolling::AddScrolling(material, 1, "SpriteSheet2D;Data/2D/nuages.xml@nuage2", scrollRootNode, 900, 0, 0.f, Vector2(-2.f, -2.f), Vector2(-1.f, 0.1f), false, Vector2::ZERO, Color::WHITE);
        GEF_Scrolling::AddScrolling(material, 1, "SpriteSheet2D;Data/2D/nuages.xml@nuage3", scrollRootNode, 1100, 0, 0.f, Vector2::ZERO, Vector2(-2.f, 0.2f), true, Vector2::ZERO, Color::WHITE);
#endif
    }
#endif // ACTIVE_WEATHEREFFECTS

    buttonarena = mainscene->GetChild("ButtonArena");
    buttonworld = mainscene->GetChild("ButtonWorld");

    mainscene->SetEnabledRecursive(false);

    GameHelpers::CreateMusicNode(rootScene_);

    URHO3D_LOGINFO("MenuState() - CreateScene ... OK !");
}

void MenuState::CreateUI()
{
    ui_ = GetSubsystem<UI>();

    if (!ui_) return;

    URHO3D_LOGINFO("MenuState() - CreateUI ...");

    // Reset Focus, Keep Update restore the focus
    ui_->SetFocusElement(0);

    if (GameContext::Get().gameConfig_.touchEnabled_ && GameContext::Get().gameConfig_.screenJoystick_)
    {
        GameContext::Get().input_->SetScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoystickID_, false);
        GameContext::Get().input_->SetScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoysticksettingsID_, false);
    }

//	// Load XML file containing default UI style sheet
//	ui_->GetRoot()->SetDefaultStyle(GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/DefaultStyle.xml"));

    // Load XML file containing MainMenu UI Layout
//	SharedPtr<UIElement> w = ui_->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/mainui.xml"));

    URHO3D_LOGINFO("MenuState() - CreateUI ... OK !");
}

void MenuState::StartScene()
{
    URHO3D_LOGINFO("MenuState() - StartScene ...");

    if (!titlescene_)
        CreateScene();

    /// Force To Calculate Drawable2D BoundingBoxes
    titlescene_->GetChild("MainScene")->SetEnabledRecursive(true);

#ifdef ACTIVE_WEATHEREFFECTS
    titlescene_->GetChild("MainScene")->GetChild("ScrollRootNode")->SetEnabledRecursive(true);
#endif

    rootScene_->SetUpdateEnabled(true);

    GameContext::Get().cameraNode_->SetPosition(Vector3::ZERO);
    GameContext::Get().camera_->SetViewMask(Urho3D::DRAWABLE_ANY);

    // Lock Mouse Vibilility on True
    GameContext::Get().SetMouseVisible(true, false, true);

    menustate_ = 0;
    lastmenustate_ = 0;

    // Debug
//    GameContext::Get().DumpNode(rootScene_);

    URHO3D_LOGINFO("MenuState() - StartScene ... OK !");
}



bool MenuState::IsNetworkReady() const
{
    if (!GameContext::Get().gameNetwork_)
        return true;

    return GameContext::Get().ServerMode_ | GameContext::Get().ClientMode_;
}

void MenuState::Quit()
{
    OptionState* optionState = (OptionState*)GameContext::Get().stateManager_->GetState("Options");
    if (optionState)
        optionState->OpenQuitMessage();
}

void MenuState::UpdateAnimButtons()
{
    // no button over (mouse mode)
    if (menustate_ == 0)
    {
        Node* skel = titlescene_->GetChild("MainScene")->GetChild("Skel");
        if (skel) skel->GetComponent<AnimatedSprite2D>()->SetAnimation("idle");
        if (buttonarena) buttonarena->GetComponent<AnimatedSprite2D>()->SetAnimation("off");
        if (buttonworld) buttonworld->GetComponent<AnimatedSprite2D>()->SetAnimation("off");
    }
    // ARENA on
    else if (menustate_ == 1)
    {
        Node* skel = titlescene_->GetChild("MainScene")->GetChild("Skel");
        if (skel) skel->GetComponent<AnimatedSprite2D>()->SetAnimation("idle");
        if (buttonarena) buttonarena->GetComponent<AnimatedSprite2D>()->SetAnimation("on");
        if (buttonworld) buttonworld->GetComponent<AnimatedSprite2D>()->SetAnimation("off");
    }
    // WORLD on
    else if (menustate_ == 2)
    {
        Node* skel = titlescene_->GetChild("MainScene")->GetChild("Skel");
        if (skel) skel->GetComponent<AnimatedSprite2D>()->SetAnimation("idle");
        if (buttonarena) buttonarena->GetComponent<AnimatedSprite2D>()->SetAnimation("off");
        if (buttonworld) buttonworld->GetComponent<AnimatedSprite2D>()->SetAnimation("on");
    }

    URHO3D_LOGINFOF("MenuState() - UpdateAnimButtons menustate=%d", menustate_);
}

void MenuState::SetMenuColliderPositions()
{
    if (buttonarena)
    {
        AnimatedSprite2D* arenabutton = buttonarena->GetComponent<AnimatedSprite2D>();
        if (!arenabutton->GetWorldBoundingBox().Defined())
        {
            URHO3D_LOGERRORF("MenuState() - SetMenuColliderPositions : arenabutton wbox=%s !", arenabutton->GetWorldBoundingBox().ToString().CString());
            arenabutton->MarkDirty();
        }
        GameHelpers::OrthoWorldToScreen(arenaRect_, arenabutton->GetWorldBoundingBox());
        URHO3D_LOGINFOF("MenuState() - SetMenuColliderPositions : arenabutton=%s !", arenaRect_.ToString().CString());
    }

    if (buttonworld)
    {
        AnimatedSprite2D* worldbutton = buttonworld->GetComponent<AnimatedSprite2D>();
        if (!worldbutton->GetWorldBoundingBox().Defined())
        {
            URHO3D_LOGERRORF("MenuState() - SetMenuColliderPositions : worldbutton wbox=%s !", worldbutton->GetWorldBoundingBox().ToString().CString());
            worldbutton->MarkDirty();
        }
        GameHelpers::OrthoWorldToScreen(worldRect_, worldbutton->GetWorldBoundingBox());
        URHO3D_LOGINFOF("MenuState() - SetMenuColliderPositions : worldbutton=%s !", worldRect_.ToString().CString());
    }
}


void MenuState::SubscribeToMenuEvents()
{
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(MenuState, HandleMenu));
#ifndef __ANDROID__
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(MenuState, HandleMenu));
#endif
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(MenuState, HandleMenu));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(MenuState, HandleMenu));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(MenuState, HandleMenu));
}

void MenuState::UnsubscribeToMenuEvents()
{
    UnsubscribeFromEvent(E_KEYUP);
    UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
#ifndef __ANDROID__
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
#endif
    UnsubscribeFromEvent(E_TOUCHMOVE);
    UnsubscribeFromEvent(E_TOUCHEND);
    UnsubscribeFromEvent(E_TOUCHBEGIN);

    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONDOWN);
}


void MenuState::BeginNewLevel(GameLevelMode mode, unsigned seed)
{
    URHO3D_LOGINFO("MenuState() - ---------------------------------------");
    URHO3D_LOGINFOF("MenuState() - BeginNewLevel : %s seed=%u          -", levelModeNames[mode], seed);
    URHO3D_LOGINFO("MenuState() - ---------------------------------------");

    GameContext::Get().ResetGameStates();

    if (mode == LVL_NEW)
    {
        if (GameContext::Get().indexLevel_ > GameContext::Get().MAX_NUMLEVELS)
            GameContext::Get().SetLevel(1);
        mode = (GameLevelMode)GameContext::Get().lastLevelMode_;
    }
    else
        GameContext::Get().lastLevelMode_ = mode;

    if (mode == LVL_LOAD)
    {
        GameContext::Get().loadSavedGame_ = true;
    }
    else if (mode == LVL_ARENA)
    {
        GameContext::Get().arenaZoneOn_ = true;
    }
    else if (mode == LVL_TEST)
    {
        GameContext::Get().testZoneOn_ = true;
        if (!GameContext::Get().testZoneId_)
            GameContext::Get().testZoneId_ = 2;
    }
    else if (mode == LVL_CREATE)
    {
        GameContext::Get().createModeOn_ = true;
    }

    if (GameContext::Get().createModeOn_)
        GameContext::Get().numPlayers_= 0;

    if (seed)
    {
        PlayState* playstate = (PlayState*)stateManager_->GetState("Play");
        if (playstate)
            playstate->SetWorldSeed(seed);
    }

    GameContext::Get().SetLevel();

    GameContext::Get().SetConsoleVisible(false);

    stateManager_->PushToStack("Play");
}

void MenuState::Launch(int selection)
{
    if (!IsNetworkReady())
        return;

    URHO3D_LOGINFOF("MenuState() - Launch %d", selection);

    switch (selection)
    {
    // ARENA
    case 1:
        BeginNewLevel(LVL_ARENA);
        break;

    // WORLD
    case 2:
        BeginNewLevel(LVL_TEST);
        break;
    }

    // Lock Mouse Vibilility on False
    if (selection != 0)
        GameContext::Get().SetMouseVisible(false, false, true);
}


void MenuState::HandlePreloadingFinished(StringHash eventType, VariantMap& eventData)
{
    OptionState* optionState = (OptionState*)stateManager_->GetState("Options");
    if (optionState)
    {
        optionState->SetVisibleAccessButton(true);
        optionState->CloseFrame();

        SubscribeToEvent(optionState, GAME_OPTIONSFRAME_OPEN, URHO3D_HANDLER(MenuState, HandleOptionsFrame));
        SubscribeToEvent(optionState, GAME_OPTIONSFRAME_CLOSED, URHO3D_HANDLER(MenuState, HandleOptionsFrame));
    }

    SubscribeToMenuEvents();
}

String DumpButtonStates(JoystickState* joystate)
{
    String s;
    for (int i= 0; i < joystate->GetNumButtons(); ++i)
        if (joystate->GetButtonDown(i))
            s += "," + String(i);
    return s;
}

void MenuState::HandleMenu(StringHash eventType, VariantMap& eventData)
{
    Input* input = GetSubsystem<Input>();

    // touchEnabled KEY_SELECT Settings
    if (input->GetKeyPress(KEY_SELECT) && GameContext::Get().gameConfig_.touchEnabled_ && GameContext::Get().gameConfig_.screenJoystick_)
    {
        input->SetScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoysticksettingsID_, !input->IsScreenJoystickVisible(GameContext::Get().gameConfig_.screenJoysticksettingsID_));
        return;
    }

    if (ui_->GetFocusElement())
        return;

    bool launch = false;
    int move = 0;

    /// Set Mouse Mouvements in Menu Items

    if (eventType == E_MOUSEMOVE || eventType == E_TOUCHEND || eventType == E_TOUCHMOVE || eventType == E_TOUCHBEGIN)
    {
        IntVector2 position;

        if (eventType == E_MOUSEMOVE)
        {
            GameHelpers::GetInputPosition(position.x_, position.y_);

//            URHO3D_LOGINFOF("MainMenu() - HandleMenu : mouseposition=%s!", position.ToString().CString());
        }
        else if (eventType == E_TOUCHEND)
        {
            position.x_ = eventData[TouchEnd::P_X].GetInt();
            position.y_ = eventData[TouchEnd::P_Y].GetInt();
        }
        else if (eventType == E_TOUCHMOVE)
        {
            position.x_ = eventData[TouchMove::P_X].GetInt();
            position.y_ = eventData[TouchMove::P_Y].GetInt();

//            URHO3D_LOGINFOF("MainMenu() - HandleMenu : E_TOUCHBEGIN=%s !", position.ToString().CString());
        }
        else if (eventType == E_TOUCHBEGIN)
        {
            position.x_ = eventData[TouchBegin::P_X].GetInt();
            position.y_ = eventData[TouchBegin::P_Y].GetInt();
        }

        if (arenaRect_.IsInside(position) == INSIDE)
        {
            if (menustate_ != 1)
            {
                menustate_ = 1;
                move = 1;
            }
            launch = (eventType == E_TOUCHEND);// && eventData[TouchBegin::P_PRESSURE].GetFloat() > 0.7f);
            if (eventType == E_TOUCHBEGIN) lastmenustate_ = menustate_;
        }
        else if (worldRect_.IsInside(position) == INSIDE)
        {
            if (menustate_ !=2)
            {
                menustate_ = 2;
                move = 1;
            }
            launch = (eventType == E_TOUCHEND);// && eventData[TouchBegin::P_PRESSURE].GetFloat() > 0.7f);
            if (eventType == E_TOUCHBEGIN) lastmenustate_ = menustate_;
        }
        else if (menustate_ != 0 && (eventType == E_MOUSEMOVE || eventType == E_TOUCHMOVE))
        {
            menustate_ = 0;
            move = 1;
            lastmenustate_ = menustate_;
        }
    }

    /// Set KeyBoard/JoyPad Mouvements in Menu Items

    else if (eventType == E_KEYUP || eventType == E_JOYSTICKAXISMOVE || eventType == E_JOYSTICKHATMOVE)
    {
        if (eventType == E_KEYUP)
        {
            int key = eventData[KeyUp::P_KEY].GetInt();
            if (key == KEY_UP)
                move = -1;
            else if (key == KEY_DOWN)
                move = 1;
            launch = (key == KEY_SPACE || key == KEY_RETURN);
        }

        if (eventType == E_JOYSTICKAXISMOVE)
        {
            float motion = eventData[JoystickAxisMove::P_POSITION].GetFloat();
            int joyid = eventData[JoystickAxisMove::P_JOYSTICKID].GetInt();
            int axis = eventData[JoystickAxisMove::P_AXIS].GetInt();

            JoystickState* joystick = input->GetJoystick(joyid);
            if (!joystick)
                return;

            move = abs(motion) >= 0.9f ? (motion < 0.f ? -1 : 1) : 0;
            if (move)
                URHO3D_LOGINFOF("MenuState() - HandleMenu JoystickAxisMove on joyid=%d axis=%d motion=%F(%F)",
                                joyid, axis, motion, joystick->GetAxisPosition(axis));
        }

        if (eventType == E_JOYSTICKHATMOVE)
        {
            float motion = eventData[JoystickHatMove::P_POSITION].GetFloat();

            move = motion != 0.f ? (motion < 3.f ? -1 : 1) : 0;
            if (move)
                URHO3D_LOGINFOF("MenuState() - HandleMenu JoystickHatMove motion=%F", motion);
        }

        if (move != 0)
        {
            menustate_ = menustate_ + move;

            if (menustate_ >= numMenuChoices_)
            {
                menustate_ = 2;
            }
            else if (menustate_ <= 0)
            {
                menustate_ = 1;
            }
        }
    }

    /// Set Item Selection

    else if (eventType == E_JOYSTICKBUTTONDOWN)
    {
        int joyid = eventData[JoystickButtonDown::P_JOYSTICKID].GetInt();
        JoystickState* joystick = input->GetJoystick(joyid);
        if (!joystick)
            return;

        URHO3D_LOGINFOF("MenuState() - HandleMenu E_JOYSTICKBUTTONDOWN on joyid=%d buttons=%s", joyid, DumpButtonStates(joystick).CString());

        launch = eventData[JoystickButtonDown::P_BUTTON] == SDL_CONTROLLER_BUTTON_A;  // PS4controller = X
    }

    else if (eventType == E_MOUSEBUTTONDOWN)
        launch = (eventData[MouseButtonDown::P_BUTTON].GetInt() == MOUSEB_LEFT);

    /// Animate Items

    if (move)
        UpdateAnimButtons();

    /// Launch Selected Item

    if (launch)
        Launch(menustate_);
}

void MenuState::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    using namespace KeyDown;

    switch (eventData[P_SCANCODE].GetInt())
    {
//    case KEY_D:
//        if (rootScene_->GetComponent<Renderer2D>())
//            rootScene_->GetComponent<Renderer2D>()->Dump();
//        GameHelpers::SaveNodeXML(context_, titlescene_, "essaimain.xml");
//        break;
    case SCANCODE_G:
        GameContext::Get().DrawDebug_ = !GameContext::Get().DrawDebug_;
        URHO3D_LOGINFOF("MenuState() - HandleKeyDown : KeyG DrawDebug=%s", GameContext::Get().DrawDebug_ ? "true":"false");

        if (GameContext::Get().DrawDebug_)
            SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(MenuState, OnPostRenderUpdate));
        else
            UnsubscribeFromEvent(E_POSTRENDERUPDATE);
        break;
    case SCANCODE_W :
        if (GameContext::Get().camera_)
            GameContext::Get().camera_->SetZoom(GameContext::Get().camera_->GetZoom() * 1.01f);
        break;
    case SCANCODE_S :
        if (GameContext::Get().camera_)
            GameContext::Get().camera_->SetZoom(GameContext::Get().camera_->GetZoom() * 0.99f);
        break;
    case SCANCODE_A:
        GameContext::Get().cameraNode_->Translate(Vector3::LEFT * 0.1f);
        break;
    case SCANCODE_D:
        GameContext::Get().cameraNode_->Translate(Vector3::RIGHT * 0.1f);
        break;
    }
}

void MenuState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("MenuState() - HandleScreenResized ...");

    /// Set Camera
    GameContext::Get().cameraNode_->SetPosition(Vector3::ZERO);

#ifdef __ANDROID__
    float zoom = 1.2f;
    GameContext::Get().camera_->SetZoom(zoom);
#endif

    /// Set Buttons
    SetMenuColliderPositions();

    UpdateAnimButtons();

    eventData[World_CameraChanged::VIEWPORT] = -1;
    SendEvent(WORLD_CAMERACHANGED, eventData);

    URHO3D_LOGINFO("MenuState() - HandleScreenResized ... OK !");
}

//void MenuState::HandleQuitMessageAck(StringHash eventType, VariantMap& eventData)
//{
//	using namespace MessageACK;
//
//	if (eventData[P_OK].GetBool())
//    {
//        GameContext::Get().Exit();
//		return;
//    }
//
//    // Initialize controls
//    GetSubsystem<Input>()->ResetStates();
//
//    SubscribeToMenuEvents();
//}

void MenuState::HandleOptionsFrame(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_OPTIONSFRAME_OPEN)
    {
        UnsubscribeToMenuEvents();
    }
    else if (eventType == GAME_OPTIONSFRAME_CLOSED)
    {
        SubscribeToMenuEvents();
    }
}

void MenuState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().rootScene_)
    {
        Octree* tree = GameContext::Get().rootScene_->GetComponent<Octree>();
        tree->DrawDebugGeometry(false);

        DebugRenderer* debugrenderer = GameContext::Get().rootScene_->GetComponent<DebugRenderer>();

        if (!debugrenderer)
            return;

//        const Rect& visibleRect = World2D::GetExtendedVisibleRect(0);
//        GameHelpers::DrawDebugRect(visibleRect, debugrenderer, false, Color::CYAN);

//        if (GameContext::Get().gameConfig_.debugLights_)
//        {
//            PODVector<Light*> lights;
//            GameContext::Get().rootScene_->GetComponents<Light>(lights, true);
////            URHO3D_LOGINFOF("nb Light=%d",lights.Size());
//            for (unsigned i = 0; i < lights.Size(); ++i)
//                lights[i]->DrawDebugGeometry(GameContext::Get().rootScene_->GetComponent<DebugRenderer>(), false);
//        }
        if (GameContext::Get().gameConfig_.debugSprite2D_)
        {
            PODVector<StaticSprite2D*> drawables;
            GameContext::Get().rootScene_->GetDerivedComponents<StaticSprite2D>(drawables, true);
//            URHO3D_LOGINFOF("nb Sprite2D=%d", drawables.Size());
            for (unsigned i = 0; i < drawables.Size(); ++i)
                drawables[i]->DrawDebugGeometry(debugrenderer, false);
        }

//        if (GameContext::Get().gameConfig_.physics2DEnabled_ && GameContext::Get().gameConfig_.debugPhysics_)
//        {
//            PhysicsWorld2D* physicsWorld2D = GameContext::Get().rootScene_->GetComponent<PhysicsWorld2D>();
//            if (physicsWorld2D) physicsWorld2D->DrawDebugGeometry();
//        }

        // For Debugging GameHelpers::OrthoWorldToScreen : Ok finished!

        GameHelpers::DrawDebugUIRect(arenaRect_, Color::BLUE);
        GameHelpers::DrawDebugUIRect(worldRect_, Color::BLUE);
        if (buttonarena)
            debugrenderer->AddBoundingBox(buttonarena->GetComponent<AnimatedSprite2D>()->GetWorldBoundingBox(), Color::CYAN, false);
        if (buttonworld)
            debugrenderer->AddBoundingBox(buttonworld->GetComponent<AnimatedSprite2D>()->GetWorldBoundingBox(), Color::CYAN, false);
        /*
                if (GameContext::Get().gameConfig_.debugUI_)
                {
                    ui_->DebugDraw(ui_->GetRoot());

                    PODVector<UIElement*> children;
                    ui_->GetRoot()->GetChildren(children, true);
                    for (PODVector<UIElement*>::ConstIterator it=children.Begin();it!=children.End();++it)
                        ui_->DebugDraw(*it);
                }
        */
    }
}

