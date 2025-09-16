#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/Graphics/Graphics.h>
//#include <Urho3D/Graphics/GraphicsEvents.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Engine/Engine.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/ListView.h>
#include <Urho3D/UI/DropDownList.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/MessageBox.h>

#include "GameOptions.h"
#include "GameAttributes.h"
#include "GameEvents.h"

#include "GameContext.h"
#include "GameHelpers.h"

//#include "GameDialogue.h"
#include "GameCommands.h"
#include "GameStateManager.h"
#include "GameNetwork.h"
#include "../cpplauncher/Game.h"

#include "MapStorage.h"
#include "MapWorld.h"
#include "Map.h"

#include "Actor.h"
#include "Player.h"
#include "GOC_Controller.h"

#include "TimerRemover.h"
#include "TimerInformer.h"

#include "ViewManager.h"
#include "MAN_Weather.h"

#include "sPlay.h"

#include "sOptions.h"


extern bool drawDebug_;

const String OPTION_ROOTUI("optionrootui");

const unsigned OPTION_NUMCATEGORIES = 8;

const char* optionCategories_[] =
{
    "Game",
    "World",
    "Player",
    "Interface",
    "Graphics",
    "Sounds",
    "Network",
    "Developper"
};

const char* ActionNames_[] =
{
    "Up",
    "Down",
    "Left",
    "Right",
    "Jump",
    "Fire1",
    "Fire2",
    "Fire3",
    "Interact",
    "Status",
    "Focus-",
    "Focus+",
    "Entity-",
    "Entity+",
    ""
};

const char* ControlTypeNames_[] =
{
    "Keyboard",
    "Joystick",
    "ScreenJoy",
    "Touch",
    "CPU",
};

enum OptionParameters
{
    OPTION_WorldName,
    OPTION_WorldModel,
    OPTION_WorldSize,
    OPTION_WorldSeed,
    OPTION_MultiViews,
    OPTION_NumPlayers,
    OPTION_ControlP1,
    OPTION_ControlP2,
    OPTION_ControlP3,
    OPTION_ControlP4,
    OPTION_Language,
    OPTION_Reactivity,
    OPTION_Music,
    OPTION_Sound,
    OPTION_Resolution,
    OPTION_Fullscreen,
    OPTION_TextureQuality,
    OPTION_TextureFilter,
    OPTION_Underground,
    OPTION_FluidEnable,
    OPTION_RenderShapes,
    OPTION_RenderDebug,
    OPTION_DebugRenderShapes,
    OPTION_DebugPhysics,
    OPTION_DebugWorld,
    OPTION_DebugRttScene,
    OPTION_NetMode,
    OPTION_NetServer,
    OPTION_NUMPARAMETERS
};

struct OptionParameter
{
    OptionParameter(const String& name) : name_(name), control_(0) { }

    String name_;
    DropDownList* control_;
};

OptionParameter optionParameters_[OPTION_NUMPARAMETERS] =
{
    OptionParameter("WorldName"),
    OptionParameter("WorldModel"),
    OptionParameter("WorldSize"),
    OptionParameter("WorldSeed"),
    OptionParameter("MultiViews"),
    OptionParameter("NumPlayers"),
    OptionParameter("ControlP1"),
    OptionParameter("ControlP2"),
    OptionParameter("ControlP3"),
    OptionParameter("ControlP4"),
    OptionParameter("Language"),
    OptionParameter("Reactivity"),
    OptionParameter("Music"),
    OptionParameter("Effect"),
    OptionParameter("Resolution"),
    OptionParameter("Fullscreen"),
    OptionParameter("TextureQuality"),
    OptionParameter("TextureFilter"),
    OptionParameter("Underground"),
    OptionParameter("FluidEnable"),
    OptionParameter("RenderShapes"),
    OptionParameter("RenderDebug"),
    OptionParameter("DebugRenderShapes"),
    OptionParameter("DebugPhysics"),
    OptionParameter("DebugWorld"),
    OptionParameter("DebugRttScene"),
    OptionParameter("NetMode"),
    OptionParameter("Server"),
};

WeakPtr<Window> uioptions_;
WeakPtr<UIElement> uioptionsframe_;
WeakPtr<UIElement> uioptionscontent_;
WeakPtr<Button> menubutton_;
WeakPtr<UIElement> closebutton_;
WeakPtr<UIElement> applybutton_;

// undef mingw messagebox declaration
#undef MessageBox
WeakPtr<MessageBox> quitMessageBox_;

IntVector2 initialFramePosition_;

Vector<PODVector<int> > tempkeysmap;
Vector<PODVector<int> > tempbuttonsmap;

int currentPlayerID_;
int currentActionButton_;


static String sOptionsWorldModelFilename_;
static float sOptionsWorldModelSize_;
static unsigned sOptionsWorldModelSeed_;
static int sOptionsLastSeedSelection_;
static Vector2 sOptionsWorldSnapShotCenter_;
static IntVector2 sOptionsWorldSnapShotTopLeftMap_;
static int sOptionsWorldSnapShotMaxMaps_;
static float sOptionsWorldSnapShotScale_ = 1.f;
static bool sOptionsWorldSnapshotGridOn_ = false;
static int sOptionsWorldIndex_;
static SharedPtr<Texture2D> sOptionsWorldSnapShotTexture_;


OptionState::OptionState(Context* context) :
    GameState(context, "Options")
{
//    URHO3D_LOGDEBUG("OptionState()");
    // Create worldMap Texture
    sOptionsWorldSnapShotTexture_ = SharedPtr<Texture2D>(new Texture2D(context_));
    sOptionsWorldSnapShotTexture_->SetNumLevels(1);
    sOptionsWorldSnapShotTexture_->SetFilterMode(FILTER_NEAREST);
}

OptionState::~OptionState()
{
    URHO3D_LOGDEBUG("~OptionState()");
    sOptionsWorldSnapShotTexture_.Reset();
}


bool OptionState::Initialize()
{
    tempkeysmap = GameContext::Get().keysMap_;
    tempbuttonsmap = GameContext::Get().buttonsMap_;
    return GameState::Initialize();
}

void OptionState::Begin()
{
    if (IsBegun())
        return;

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - Begin ...                             -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");

    // Set UI
    if (!CreateUI())
    {
        URHO3D_LOGERROR("OptionState() - ----------------------------------------");
        URHO3D_LOGERROR("OptionState() - Begin ... UI NOK !                     -");
        URHO3D_LOGERROR("OptionState() - ----------------------------------------");

        stateManager_->PopStack();
        return;
    }

    SubscribeToEvents();

    // Call base class implementation
    GameState::Begin();

//    GameHelpers::SetMoveAnimation(GameContext::Get().rootScene_->GetChild("Options"), Vector3(10.f * stateManager_->GetStackMove(), 0.f, 0.f), Vector3::ZERO, 0.f, SWITCHSCREENTIME);
//    GameHelpers::SetMoveAnimationUI(uioptions_, IntVector2(-uioptions_->GetSize().x_, 0), IntVector2::ZERO, 0.f, SWITCHSCREENTIME);

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - Begin ... OK !                      -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
}

void OptionState::End()
{
    if (!IsBegun())
        return;

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - End ...                               -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");

    UnsubscribeToEvents();

    RemoveUI();

    // Call base class implementation
    GameState::End();

    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
    URHO3D_LOGINFO("OptionState() - End ... OK !                           -");
    URHO3D_LOGINFO("OptionState() - ----------------------------------------");
}

void OptionState::LoadJSONServerList()
{
    URHO3D_LOGINFO("OptionState() - LoadJSONServerList : ...");

    DropDownList* control = optionParameters_[OPTION_NetServer].control_;
    control->RemoveAllItems();

    Text* text = new Text(context_);
    control->AddItem(text);
    text->SetStyle("OptionDropDownSelectorText");
    text->SetText("thismachine");
    text->AddTag("127.0.0.1:2345");

    SharedPtr<JSONFile> serverlist = context_->GetSubsystem<ResourceCache>()->GetTempResource<JSONFile>("Texts/netservers.json");
    if (serverlist)
    {
        const JSONValue& servers = serverlist->GetRoot();
        for (JSONObject::ConstIterator i = servers.Begin(); i != servers.End(); ++i)
        {
            const String& servername = i->first_;
            if (servername.Empty())
            {
                URHO3D_LOGWARNING("OptionState() - LoadJSONServerList : servername is empty !");
                continue;
            }

            const JSONValue& serverip = i->second_;
            Text* text = new Text(context_);
            control->AddItem(text);
            text->SetStyle("OptionDropDownSelectorText");
            text->SetText(servername);
            text->AddTag(serverip.GetString());
        }
    }
}

void OptionState::GetNetServerParams(int selection, String& serverip, int& serverport)
{
    UIElement* servertextelt = optionParameters_[OPTION_NetServer].control_->GetItem(selection);
    if (servertextelt->GetTags().Size())
    {
        Vector<String> s = servertextelt->GetTags().Front().Split(':');
        serverip = s.Front();
        serverport = ToInt(s.Back());
    }
    URHO3D_LOGINFOF("OptionState() - GetNetServerParams : selection=%d uielt=%u ...serverip=%s serverport=%d", selection, servertextelt, serverip.CString(), serverport);
}

int OptionState::GetNetServerIndex(const String& serverip, int serverport)
{
    for (int i=0; i < optionParameters_[OPTION_NetServer].control_->GetNumItems(); i++)
    {
        String serverip;
        int serverport = 0;
        GetNetServerParams(i, serverip, serverport);
        if (serverip == GameNetwork::Get()->GetServerIP() && serverport == GameNetwork::Get()->GetServerPort())
            return i;
    }
    return 0;
}

void OptionState::SetDefaultWorldParameters()
{
    GameContext::Get().testZoneId_ = 2;

    sOptionsWorldIndex_ = MapStorage::GetWorldIndex(IntVector2(0, GameContext::Get().MAX_NUMLEVELS + GameContext::Get().testZoneId_));
    optionParameters_[OPTION_WorldName].control_->SetSelection(0);

    sOptionsWorldModelFilename_ = "anlworldVM-ellipsoid-zone2.xml";
    sOptionsWorldModelFilename_ = GetNativePath(RemoveTrailingSlash(World2DInfo::ATLASSETDIR + sOptionsWorldModelFilename_));
    optionParameters_[OPTION_WorldModel].control_->SetSelection(1);
    optionParameters_[OPTION_WorldModel].control_->SetEditable(false);

    sOptionsWorldModelSize_ = 10.f;
    optionParameters_[OPTION_WorldSize].control_->SetSelection(3);
    optionParameters_[OPTION_WorldSize].control_->SetEditable(false);

    sOptionsWorldModelSeed_ = 66536;
    optionParameters_[OPTION_WorldSeed].control_->SetSelection(3);
    optionParameters_[OPTION_WorldSeed].control_->SetEditable(false);

    snapshotdirty_ = true;

    URHO3D_LOGINFOF("OptionState() - SetDefaultWorldParameters ! sOptionsWorldModelFilename_ = %s", sOptionsWorldModelFilename_.CString());

    ApplyWorldChange();
}

bool OptionState::CreateUI()
{
    if (!GameContext::Get().ui_)
        return false;

    if (uioptions_)
        return true;

    URHO3D_LOGINFO("OptionState() - CreateUI ...");

    Graphics* graphics = GetSubsystem<Graphics>();

    // Create Root Transparent Window (For Modal Handle)
    uioptions_ = GameContext::Get().ui_->GetRoot()->CreateChild<Window>(OPTION_ROOTUI);
    uioptions_->SetSize(GameContext::Get().ui_->GetRoot()->GetSize());
    uioptions_->SetTexture(GameContext::Get().textures_[UIEQUIPMENT]);
    uioptions_->SetOpacity(0.9f);
//    uioptions_->SetEnableDpiScale(false);
    uioptions_->SetImageRect(IntRect(0, 0, 1, 1));
    uioptions_->SetBorder(IntRect(0, 0, 0, 0));
    uioptions_->SetResizeBorder(IntRect(0, 0, 0, 0));
    uioptions_->SetFocusMode(FM_NOTFOCUSABLE);
    uioptions_->SetEnabled(false);

    // Create Option Access Button
    menubutton_ = uioptions_->CreateChild<Button>("menubutton");
    menubutton_->SetSprite("UI/game_equipment.xml@menuicon_off");
    menubutton_->SetSpriteHover("menuicon_on");
    menubutton_->SetSize(40 * GameContext::Get().uiScale_, 40 * GameContext::Get().uiScale_);
    menubutton_->SetPosition(10 * GameContext::Get().uiScale_, 10 * GameContext::Get().uiScale_);
    menubutton_->SetPriority(199);
    menubutton_->SetOpacity(0.9f);
    menubutton_->SetVisible(false);
    menubutton_->SetFocusMode(FM_RESETFOCUS);

    // Create Menu Frame
    uioptionsframe_ = GameHelpers::AddUIElement(String("UI/Options.xml"), IntVector2(0, 0), HA_CENTER, VA_CENTER);
    uioptions_->AddChild(uioptionsframe_);

    closebutton_ = uioptionsframe_->GetChild(String("closebutton"), true);
    applybutton_ = uioptionsframe_->GetChild(String("applybutton"), true);
    uioptionscontent_ = uioptionsframe_->GetChild(String("Content"), true);

    for (unsigned i = 0; i < OPTION_NUMPARAMETERS; i++)
    {
        OptionParameter& parameter = optionParameters_[i];
        parameter.control_ = uioptionsframe_->GetChildStaticCast<DropDownList>(parameter.name_, true);
    }

    SwitchToCategory(String("Game"));

    uioptionsframe_->SetPivot(0.5f, 0.5f);
    uioptionsframe_->SetMinAnchor(0.5f, 0.5f);
    uioptionsframe_->SetMaxAnchor(0.5f, 0.5f);
    uioptionsframe_->SetEnableAnchor(true);

    initialFramePosition_ = uioptionsframe_->GetPosition();
    uioptionsframe_->SetFocusMode(FM_FOCUSABLE);
    uioptionsframe_->SetVisible(false);
    uioptionsframe_->SetEnabled(false);

    GameHelpers::SetEnableScissor(uioptions_, false);

    //GameHelpers::SetUIScale(uioptionsframe_, IntVector2(782, 522));

    SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(OptionState, HandleScreenResized));

    URHO3D_LOGINFO("OptionState() - CreateUI ... OK !");

    return true;
}

void OptionState::RemoveUI()
{
    UnsubscribeToEvents();

    TimerRemover::Get()->Start(uioptions_, SWITCHSCREENTIME + 0.05f);
}

void OptionState::SetVisibleAccessButton(bool state)
{
    if (state && !menubutton_)
    {
        if (!CreateUI())
            return;
    }

    if (menubutton_)
        menubutton_->SetVisible(state);

    if (state)
    {
        if (menubutton_)
        {
            IntVector2 position = menubutton_->GetPosition();
            Graphics* graphics = GetSubsystem<Graphics>();
            int width = graphics->GetWidth();
            int height = graphics->GetHeight();
            GameHelpers::SetMoveAnimationUI(menubutton_, IntVector2(position.x_, height+100), position, 0.f, SWITCHSCREENTIME);
            SubscribeToEvent(menubutton_, E_RELEASED, URHO3D_HANDLER(OptionState, HandleMenuButton));
        }
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(OptionState, HandleKeyEscape));
        SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(OptionState, HandleKeyEscape));
        GameContext::Get().SubscribeToCursorVisiblityEvents();
    }
    else
    {
        UnsubscribeFromEvent(E_KEYDOWN);
        UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
        if (menubutton_)
            UnsubscribeFromEvent(menubutton_, E_RELEASED);
    }
}

void OptionState::OpenQuitMessage()
{
    if (!GameContext::Get().context_->GetSubsystem<UI>())
    {
        GameContext::Get().Exit();
        return;
    }

    Localization* l10n = GameContext::Get().context_->GetSubsystem<Localization>();

    quitMessageBox_ = new MessageBox(GameContext::Get().context_, String::EMPTY, l10n->Get("gamequit"));

    UIElement* window = quitMessageBox_->GetWindow();
    if (window)
    {
        UIElement* contentmsg = window->GetChild("MessageText", true);
        if (contentmsg)
        {
            contentmsg->SetVisible(false);
            contentmsg->SetEnabled(false);
        }

        Button* noButton = static_cast<Button*>(window->GetChild("CancelButton", true));
        static_cast<Text*>(noButton->GetChild("Text", false))->SetText(l10n->Get("no"));
        noButton->SetVisible(true);
        noButton->SetFocus(true);
        Button* yesButton = (Button*)window->GetChild("OkButton", true);
        static_cast<Text*>(yesButton->GetChild("Text", false))->SetText(l10n->Get("yes"));

        GameHelpers::ToggleModalWindow(quitMessageBox_, window);

        SubscribeToEvent(quitMessageBox_, E_MESSAGEACK, URHO3D_HANDLER(OptionState, HandleQuitMessageAck));
    }
}

void OptionState::OpenFrame()
{
    if (!uioptions_)
        if (!CreateUI())
            return;

    if (uioptionsframe_->IsVisible())
        return;

    URHO3D_LOGINFO("OptionState() - OpenFrame ...");

    LoadJSONServerList();

    SynchronizeParameters();

    SubscribeToEvents();

    // Android : Hide Quit Button in MainMenu
#ifdef __ANDROID__
    UIElement* gameCategory = uioptionscontent_->GetChild("Game", true);
    if (gameCategory)
    {
        UIElement* quitButton = gameCategory->GetChild("Quit", true);
        bool enable = GameContext::Get().stateManager_ && GameContext::Get().stateManager_->GetActiveState()->GetStateId() != "MainMenu";
        if (quitButton)
        {
            quitButton->SetEnabled(enable);
            quitButton->SetVisible(enable);
        }
    }
#endif

    uioptionsframe_->SetVisible(true);
    uioptionsframe_->SetEnabled(true);

    HandleScreenResized(StringHash::ZERO, context_->GetEventDataMap(false));

    if (applybutton_)
        applybutton_->SetVisible(false);

    ResetToCurrentConfigControlKeys();

    GameHelpers::SetMoveAnimationUI(uioptionsframe_, IntVector2(initialFramePosition_.x_, -GetSubsystem<Graphics>()->GetHeight()), initialFramePosition_, 0.f, SWITCHSCREENTIME);

    GameHelpers::ToggleModalWindow(0, uioptions_.Get());

    uioptionsframe_->SetFocus(true);

    // Lock Mouse Vibilility on True
    GameContext::Get().SetMouseVisible(true, false, true);

    // Inactive Cursor Visibility Handle
    GameContext::Get().UnsubscribeFromCursorVisiblityEvents();

    // Pause the play
    const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
    if (stateId == "Play")
        GameContext::Get().rootScene_->SetUpdateEnabled(false);

    URHO3D_LOGINFO("OptionState() - OpenFrame ... OK !");
}

void OptionState::CloseFrame()
{
    if (!uioptionsframe_)
        return;

    URHO3D_LOGINFO("OptionState() - CloseFrame ...");

    if (uioptionsframe_->IsVisible())
    {
        // Stop Generating SnapShots
        World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);
        if (info.worldModel_)
            info.worldModel_->StopUnfinishedWorks();

        // Unpause the play
        const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();
        if (stateId == "Play")
            GameContext::Get().rootScene_->SetUpdateEnabled(true);

        UnsubscribeToEvents();

        if (uioptionsframe_)
        {
            GameHelpers::SetMoveAnimationUI(uioptionsframe_, initialFramePosition_, IntVector2(initialFramePosition_.x_, -GetSubsystem<Graphics>()->GetHeight()), 0.f, SWITCHSCREENTIME);
            TimerRemover::Get()->Start(uioptionsframe_, SWITCHSCREENTIME + 0.05f, DISABLE);

            new DelayInformer(this, SWITCHSCREENTIME + 0.1f, GAME_OPTIONSFRAME_CLOSED);
        }
    }

    // Unlock Mouse Vibilility on True
    GameContext::Get().SetMouseVisible(false, false, false);
    GameContext::Get().SetMouseVisible(true, false, false);

    // Reactive Cursor Visibility Handle
    GameContext::Get().SubscribeToCursorVisiblityEvents();

    GameHelpers::ToggleModalWindow();

    URHO3D_LOGINFO("OptionState() - CloseFrame ... OK !");
}

void OptionState::Hide()
{
    uioptionsframe_->SetVisible(false);
    GameHelpers::ToggleModalWindow();
}

void OptionState::ToggleFrame()
{
    URHO3D_LOGINFO("OptionState() - ToggleFrame ...");

    if (!uioptions_)
    {
        if (!CreateUI())
            return;
    }

    if (!uioptionsframe_->IsVisible())
    {
        OpenFrame();
        SendEvent(GAME_OPTIONSFRAME_OPEN);
    }
    else
    {
        CloseFrame();
        SendEvent(GAME_OPTIONSFRAME_CLOSED);
    }

    URHO3D_LOGINFO("OptionState() - ToggleFrame ... OK !");
}

void OptionState::ResetToMainCategory()
{
    if (uioptionsframe_)
        SwitchToCategory(String("Game"));
}

bool OptionState::IsVisible() const
{
    return (uioptionsframe_ && uioptionsframe_->IsVisible());
}


void OptionState::ResetToCurrentConfigControlKeys()
{
    tempkeysmap = GameContext::Get().keysMap_;
    tempbuttonsmap = GameContext::Get().buttonsMap_;
}

void OptionState::ResetToDefaultConfigControlKeys(int controlid)
{
    if (GameContext::Get().playerState_[controlid].controltype == CT_KEYBOARD)
    {
        tempkeysmap[controlid] = PODVector<int>(GameContext::Get().defaultkeysMap_[controlid], MAX_NUMACTIONS);
    }
    else if (GameContext::Get().playerState_[controlid].controltype == CT_JOYSTICK)
    {
        int joyid = GameContext::Get().joystickIndexes_[controlid] != -1 ? GameContext::Get().joystickIndexes_[controlid] : 0;
        tempbuttonsmap[joyid] = PODVector<int>(GameContext::Get().defaultbuttonsMap_[joyid], MAX_NUMACTIONS);
    }
}

void OptionState::ResetToCurrentConfigControlKeys(int controlid)
{
    if (GameContext::Get().playerState_[controlid].controltype == CT_KEYBOARD)
    {
        tempkeysmap[controlid] = GameContext::Get().keysMap_[controlid];
    }
    else if (GameContext::Get().playerState_[controlid].controltype == CT_JOYSTICK)
    {
        int joyid = GameContext::Get().joystickIndexes_[controlid] != -1 ? GameContext::Get().joystickIndexes_[controlid] : 0;
        tempbuttonsmap[joyid] = GameContext::Get().buttonsMap_[joyid];
    }
}

void OptionState::ApplyConfigControlKeys(int controlid)
{
    if (GameContext::Get().playerState_[controlid].controltype == CT_KEYBOARD)
    {
        GameContext::Get().keysMap_[controlid] = tempkeysmap[controlid];
        for (int action=0; action < MAX_NUMACTIONS; action++)
            GameContext::Get().playerState_[controlid].keys[action] = tempkeysmap[controlid][action];
    }
    else if (GameContext::Get().playerState_[controlid].controltype == CT_JOYSTICK)
    {
        int joyid = GameContext::Get().joystickIndexes_[controlid] != -1 ? GameContext::Get().joystickIndexes_[controlid] : 0;
        GameContext::Get().buttonsMap_[joyid] = tempbuttonsmap[joyid];
        for (int action=0; action < MAX_NUMACTIONS; action++)
            GameContext::Get().playerState_[controlid].joybuttons[action] = tempbuttonsmap[joyid][action];
    }
}

void OptionState::UpdateConfigControls(int controlid)
{
    URHO3D_LOGINFOF("OptionState() - UpdateConfigControls ... %d", controlid);

    UIElement* configcontrolframe = uioptionscontent_->GetChild(String("ConfigControls"));

    int joyid = GameContext::Get().joystickIndexes_[controlid] != -1 ? GameContext::Get().joystickIndexes_[controlid] : 0;

    for (int action=0; action < MAX_NUMACTIONS; action++)
    {
        String actionname(ActionNames_[action]);
        Button* actionbutton = configcontrolframe->GetChildStaticCast<Button>(actionname, true);
        Text* actiontext = actionbutton ? actionbutton->GetChildStaticCast<Text>(0) : 0;

        if (actiontext)
        {
            if (GameContext::Get().playerState_[controlid].controltype == CT_KEYBOARD)
            {
                int key = GameContext::Get().input_->GetKeyFromScancode(tempkeysmap[controlid][action]);
                actiontext->SetText(GameContext::Get().input_->GetKeyName(key));
            }
            else if (GameContext::Get().playerState_[controlid].controltype == CT_JOYSTICK)
            {
                int value = tempbuttonsmap[joyid][action];
                actiontext->SetText((value & JOY_AXI ? "AXI_" : value & JOY_HAT ? "HAT_" : "BTN_") + String(value & JOY_VALUE));
            }
        }
    }
}


void OptionState::SwitchToCategory(const String& category)
{
    URHO3D_LOGINFOF("OptionState() - SwitchToCategory ... cat=%s", category.CString());

    if (!uioptionscontent_)
        return;

    const Vector<SharedPtr<UIElement > >& children = uioptionscontent_->GetChildren();
    for (unsigned i=0; i < children.Size(); i++)
    {
        UIElement* elt = children[i];
        if (!elt)
            continue;

        // Show the category and hide the other ones
        bool state = (elt->GetName() == category);
        elt->SetEnabled(state);
        elt->SetVisible(state);
    }

    Text* titleText = uioptionsframe_->GetChildStaticCast<Text>(String("TitleText"), true);
    if (titleText)
    {
        titleText->SetText(category);
    }

    category_ = category;
    uioptionsframe_->SetFocus(true);

    URHO3D_LOGINFO("OptionState() - SwitchToCategory ... OK !");
}

String ResolutionToString(const IntVector3& resolution)
{
    if (resolution.z_)
        return ToString("%dx%d-%dHz", resolution.x_, resolution.y_, resolution.z_);        
    return ToString("%dx%d", resolution.x_, resolution.y_); 
}

static unsigned currentResolutionIndex_ = M_MAX_UNSIGNED;

void OptionState::SynchronizeParameters()
{
    URHO3D_LOGINFO("OptionState() - SynchronizeParameters ...");

    Text* mastertext = static_cast<Text*>(optionParameters_[OPTION_Fullscreen].control_->GetItem(0));
    int fontsize = mastertext->GetFontSize();
    Font* font = mastertext->GetFont();
    if (font->GetFontType() == FONT_BITMAP)
    {
        if (font == GameContext::Get().uiFonts_[UIFONTS_ABY12])
            fontsize = 12;
        else if (font == GameContext::Get().uiFonts_[UIFONTS_ABY22])
            fontsize = 22;
        else
            fontsize = 32;
    }

    if (optionParameters_[OPTION_MultiViews].control_)
    {
        optionParameters_[OPTION_MultiViews].control_->SetSelection(GameContext::Get().gameConfig_.multiviews_);
//        ApplyNumPlayers();
    }

    if (optionParameters_[OPTION_NumPlayers].control_)
    {
        optionParameters_[OPTION_NumPlayers].control_->SetSelection(GameContext::Get().numPlayers_-1);
        ApplyNumPlayers();
    }

	for (int i=0; i < 4; i++)
	{
		OptionParameter& controlParameter = optionParameters_[OPTION_ControlP1+i];
		if (controlParameter.control_)
		{
			controlParameter.control_->SetSelection(GameContext::Get().playerState_[i].controltype);
			Button* configcontrolP1button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>(String("ConfigControls_")+String(i+1), true);
			if (configcontrolP1button)
				configcontrolP1button->SetVisible(GameContext::Get().playerState_[i].controltype < CT_SCREENJOYSTICK);
		}
	}

    if (optionParameters_[OPTION_Language].control_)
    {
        int langindex = GetSubsystem<Localization>()->GetLanguageIndex(GameContext::Get().gameConfig_.language_);
        if (langindex != optionParameters_[OPTION_Language].control_->GetSelection())
        {
            optionParameters_[OPTION_Language].control_->SetSelection(langindex);
            ApplyLanguage();
        }
    }

    if (optionParameters_[OPTION_Reactivity].control_ && optionParameters_[OPTION_Reactivity].control_->GetSelection() != GameContext::Get().gameConfig_.uiUpdateDelay_)
    {
        optionParameters_[OPTION_Reactivity].control_->SetSelection(GameContext::Get().gameConfig_.uiUpdateDelay_);
    }

    Graphics* graphics = GetSubsystem<Graphics>();
    const unsigned currentMonitor = graphics->GetMonitor();

    if (optionParameters_[OPTION_Resolution].control_ && !optionParameters_[OPTION_Resolution].control_->GetNumItems())
    {

        URHO3D_LOGINFOF("OptionState() - SynchronizeParameters : video monitor=%u initialize resolutions list...", currentMonitor);        
        
        currentResolutionIndex_ = M_MAX_UNSIGNED;

        const PODVector<IntVector3> resolutions = graphics->GetResolutions(currentMonitor);
        if (resolutions.Size())
        {            
            bool oneresolution = resolutions.Size() == 1;

            // get the initial resolution index
            unsigned initialResolution = graphics->FindBestResolutionIndex(graphics->GetWidth(), graphics->GetHeight(), graphics->GetRefreshRate(), resolutions);

            // initialize resolutions list
            DropDownList* control = optionParameters_[OPTION_Resolution].control_;
            control->RemoveAllItems();
            control->SetMinSize((float)control->GetMinSize().x_, fontsize * Max(1.f, GameContext::Get().uiDpiScale_) + 8);
            control->SetMaxSize((float)control->GetMaxSize().x_, fontsize * Max(1.f, GameContext::Get().uiDpiScale_) + 8);
            control->SetVerticalAlignment(VA_CENTER);
            control->SetLayoutMode(LM_VERTICAL);

            if (!oneresolution)
            {
                UIElement* popup = control->GetPopup();
                popup->SetSize(popup->GetWidth(), Min(resolutions.Size()+1, 10) * (fontsize * Max(1.f, GameContext::Get().uiDpiScale_) + 2));
            }

            for (unsigned index = 0; index < resolutions.Size(); index++)
            {
                const IntVector3& resolution = resolutions[index];
                if (resolution.x_ < GameContext::minResolution.x_ || resolution.y_ < GameContext::minResolution.y_)
                    continue;

                Text* resolutionEntry = new Text(context_);
                control->AddItem(resolutionEntry);

                resolutionEntry->SetStyle(mastertext->GetAppliedStyle());
                resolutionEntry->SetAutoLocalizable(false);
                resolutionEntry->SetTextAlignment(HA_CENTER);
                resolutionEntry->SetFont(fontsize == 12 ? GameContext::Get().uiFonts_[UIFONTS_ABY12] : fontsize == 22 ? GameContext::Get().uiFonts_[UIFONTS_ABY22] : GameContext::Get().uiFonts_[UIFONTS_ABY32]);
                resolutionEntry->SetFontSize(fontsize);
                resolutionEntry->SetText(ResolutionToString(resolution));
                // Set the graphics resolution
                resolutionEntry->SetVar(GOA::CLIENTID, resolution);
                resolutionEntry->SetMinWidth(CeilToInt(resolutionEntry->GetRowWidth(0) + 10));

                if (index == initialResolution)
                    currentResolutionIndex_ = control->GetNumItems()-1;
//                URHO3D_LOGINFOF("OptionState() - SynchronizeParameters : add resolution=%dx%d-%dHz", resolution.x_, resolution.y_, resolution.z_);
            }

            ListView* listview = control->GetListView();
            listview->SetScrollBarsVisible(false, !oneresolution);

            GameHelpers::SetEnableScissor(control, true);

            URHO3D_LOGINFOF("OptionState() - SynchronizeParameters : get initial resolution=%s => currentResolutionIndex_=%u !", resolutions[initialResolution].ToString().CString(), currentResolutionIndex_);

            if (currentResolutionIndex_ == M_MAX_UNSIGNED)
                currentResolutionIndex_ = 0;
        }    
        else
        {
            URHO3D_LOGINFOF("OptionState() - SynchronizeParameters : can't get monitor resolutions => resolution controls are disabled !");
            optionParameters_[OPTION_Resolution].control_->GetParent()->SetEnabled(false);
            optionParameters_[OPTION_Resolution].control_->GetParent()->SetVisible(false);
            optionParameters_[OPTION_Fullscreen].control_->GetParent()->SetEnabled(false);
            optionParameters_[OPTION_Fullscreen].control_->GetParent()->SetVisible(false);
        }
    }

    if (currentResolutionIndex_ != M_MAX_UNSIGNED && optionParameters_[OPTION_Resolution].control_ && optionParameters_[OPTION_Resolution].control_->GetSelection() != currentResolutionIndex_)
    {               
        optionParameters_[OPTION_Resolution].control_->SetSelection(currentResolutionIndex_);
    }
    if (optionParameters_[OPTION_Fullscreen].control_ && optionParameters_[OPTION_Fullscreen].control_->GetSelection() != graphics->GetFullscreen())
    {
        optionParameters_[OPTION_Fullscreen].control_->SetSelection(graphics->GetFullscreen());
    }

    Renderer* renderer = GetSubsystem<Renderer>();
    if (optionParameters_[OPTION_TextureQuality].control_ && optionParameters_[OPTION_TextureQuality].control_->GetSelection() != renderer->GetTextureQuality())
    {
        optionParameters_[OPTION_TextureQuality].control_->SetSelection(renderer->GetTextureQuality());
    }

    if (optionParameters_[OPTION_TextureFilter].control_ &&  optionParameters_[OPTION_TextureFilter].control_->GetSelection() != renderer->GetTextureFilterMode())
    {
        optionParameters_[OPTION_TextureFilter].control_->SetSelection(renderer->GetTextureFilterMode());
    }

    if (optionParameters_[OPTION_Underground].control_ && optionParameters_[OPTION_Underground].control_->GetSelection() != GameContext::Get().gameConfig_.enlightScene_)
    {
        optionParameters_[OPTION_Underground].control_->SetSelection(GameContext::Get().gameConfig_.enlightScene_);
        ApplyEnlightment();
    }

    if (optionParameters_[OPTION_FluidEnable].control_ && optionParameters_[OPTION_FluidEnable].control_->GetSelection() != GameContext::Get().gameConfig_.fluidEnabled_)
    {
        optionParameters_[OPTION_FluidEnable].control_->SetSelection(GameContext::Get().gameConfig_.fluidEnabled_);
    }

    if (optionParameters_[OPTION_RenderShapes].control_ && optionParameters_[OPTION_RenderShapes].control_->GetSelection() != GameContext::Get().gameConfig_.renderShapes_)
    {
        optionParameters_[OPTION_RenderShapes].control_->SetSelection(GameContext::Get().gameConfig_.renderShapes_);
    }

    if (optionParameters_[OPTION_RenderDebug].control_ && optionParameters_[OPTION_RenderDebug].control_->GetSelection() != GameContext::Get().DrawDebug_)
    {
        optionParameters_[OPTION_RenderDebug].control_->SetSelection(GameContext::Get().DrawDebug_);
    }

    if (optionParameters_[OPTION_DebugPhysics].control_ && optionParameters_[OPTION_DebugPhysics].control_->GetSelection() != GameContext::Get().gameConfig_.debugPhysics_)
    {
        optionParameters_[OPTION_DebugPhysics].control_->SetSelection(GameContext::Get().gameConfig_.debugPhysics_);
    }

    if (optionParameters_[OPTION_DebugWorld].control_ && optionParameters_[OPTION_DebugWorld].control_->GetSelection() != GameContext::Get().gameConfig_.debugWorld2D_)
    {
        optionParameters_[OPTION_DebugWorld].control_->SetSelection(GameContext::Get().gameConfig_.debugWorld2D_);
    }

    if (optionParameters_[OPTION_DebugRenderShapes].control_ && optionParameters_[OPTION_DebugRenderShapes].control_->GetSelection() != GameContext::Get().gameConfig_.debugRenderShape_)
    {
        optionParameters_[OPTION_DebugRenderShapes].control_->SetSelection(GameContext::Get().gameConfig_.debugRenderShape_);
    }

    if (optionParameters_[OPTION_DebugRttScene].control_ && optionParameters_[OPTION_DebugRttScene].control_->GetSelection() != GameContext::Get().gameConfig_.debugRttScene_)
    {
        optionParameters_[OPTION_DebugRttScene].control_->SetSelection(GameContext::Get().gameConfig_.debugRttScene_);
    }

    if (optionParameters_[OPTION_Music].control_ && optionParameters_[OPTION_Music].control_->GetSelection() != GameContext::Get().gameConfig_.musicEnabled_)
    {
        optionParameters_[OPTION_Music].control_->SetSelection(GameContext::Get().gameConfig_.musicEnabled_);
    }

    if (optionParameters_[OPTION_Sound].control_ && optionParameters_[OPTION_Sound].control_->GetSelection() != GameContext::Get().gameConfig_.soundEnabled_)
    {
        optionParameters_[OPTION_Sound].control_->SetSelection(GameContext::Get().gameConfig_.soundEnabled_);
    }

    int netmode = GameNetwork::Get() ? GameNetwork::Get()->GetMode() : LOCALMODE;

    if (optionParameters_[OPTION_NetMode].control_ && optionParameters_[OPTION_NetMode].control_->GetSelection()+1 != netmode)
    {
        if (netmode == AUTOMODE)
            netmode = CLIENTMODE;

        optionParameters_[OPTION_NetMode].control_->SetSelection(netmode-1);
    }

    optionParameters_[OPTION_NetServer].control_->GetParent()->SetVisible(netmode == CLIENTMODE);

    if (GameNetwork::Get())
    {
        int netserverindex = GetNetServerIndex(GameNetwork::Get()->GetServerIP(), GameNetwork::Get()->GetServerPort());
        optionParameters_[OPTION_NetServer].control_->SetSelection(netserverindex);
    }
    else
    {
        optionParameters_[OPTION_NetServer].control_->SetSelection(0);
    }

    URHO3D_LOGINFO("OptionState() - SynchronizeParameters ... Updating DropDownLists Popups !");

    // Needed to update world parameters
    GameContext::Get().testZoneId_ = 0;

    // Use ShowPopup to force Batching the SelectedItem (allow to center correctly the holdertext that is a offseted batch from the selecteditem of the inside listview) see DropDownList::GetBatches
    for (int i=0; i < OPTION_NUMPARAMETERS; i++)
    {
        if (optionParameters_[i].control_)
        {
            optionParameters_[i].control_->ShowPopup(true);
            optionParameters_[i].control_->ShowPopup(false);
        }
    }

    URHO3D_LOGINFO("OptionState() - SynchronizeParameters ... OK !");
}

void OptionState::CheckParametersChanged()
{
    int change = 0;

    if (category_ == "Graphics")
    {
        if (optionParameters_[OPTION_Resolution].control_->GetSelection() != currentResolutionIndex_)
            change++;
        if (optionParameters_[OPTION_Fullscreen].control_->GetSelection() != GetSubsystem<Graphics>()->GetFullscreen())
            change++;
        if (optionParameters_[OPTION_TextureQuality].control_->GetSelection() != GetSubsystem<Renderer>()->GetTextureQuality())
            change++;
        if (optionParameters_[OPTION_TextureFilter].control_->GetSelection() != GetSubsystem<Renderer>()->GetTextureFilterMode())
            change++;

        URHO3D_LOGDEBUGF("OptionState() - CheckParametersChanged ... graphics change = %d !", change);
    }
    else if (category_ == "Network")
    {
        int netmode = (GameNetwork::Get() ? GameNetwork::Get()->GetMode() : LOCALMODE) - 1;
        if (optionParameters_[OPTION_NetMode].control_->GetSelection() != netmode)
            change++;

        if (GameNetwork::Get())
        {
            String serverip;
            int serverport = 0;
            GetNetServerParams(optionParameters_[OPTION_NetServer].control_->GetSelection(), serverip, serverport);
            if (serverip != GameNetwork::Get()->GetServerIP() || serverport != GameNetwork::Get()->GetServerPort())
                change++;
        }
        URHO3D_LOGDEBUGF("OptionState() - CheckParametersChanged ... network change = %d !", change);
    }

    applybutton_->SetVisible(change);
    applybutton_->SetEnabled(change);
}


void OptionState::ApplyParameters()
{
    if (category_ == "Graphics")
    {
        Renderer* renderer = GetSubsystem<Renderer>();
        if (renderer)
        {
            // Texture Quality
            if (optionParameters_[OPTION_TextureQuality].control_->GetSelection() != renderer->GetTextureQuality())
            {
                renderer->SetTextureQuality(optionParameters_[OPTION_TextureQuality].control_->GetSelection());
                Sprite2D::SetTextureLevels(renderer->GetTextureQuality());
                SendEvent(WORLD_DIRTY);
            }
            // Texture Filter
            if (optionParameters_[OPTION_TextureFilter].control_->GetSelection() != renderer->GetTextureFilterMode())
            {
                renderer->SetTextureFilterMode((TextureFilterMode)optionParameters_[OPTION_TextureFilter].control_->GetSelection());
                SendEvent(WORLD_DIRTY);
            }
        }

        Graphics* graphics = GetSubsystem<Graphics>();
        if (graphics)
        {
            const unsigned monitor = graphics->GetMonitor();

            IntVector3 resolution = optionParameters_[OPTION_Resolution].control_->GetItem(currentResolutionIndex_)->GetVar(GOA::CLIENTID).GetIntVector3();
            bool fullscreen = graphics->GetFullscreen();
            int videomodechanged = 0;

            URHO3D_LOGINFOF("OptionState() - ApplyParameters ... currentResolutionIndex_=%u resolution=%s", currentResolutionIndex_, resolution.ToString().CString());

            // Texture Filter
            if (optionParameters_[OPTION_TextureFilter].control_->GetSelection() != graphics->GetDefaultTextureFilterMode())
            {
                graphics->SetDefaultTextureFilterMode((TextureFilterMode)optionParameters_[OPTION_TextureFilter].control_->GetSelection());
            }

            if (optionParameters_[OPTION_Resolution].control_->GetSelection() != currentResolutionIndex_)
            {
                currentResolutionIndex_ = optionParameters_[OPTION_Resolution].control_->GetSelection();
                resolution = optionParameters_[OPTION_Resolution].control_->GetItem(currentResolutionIndex_)->GetVar(GOA::CLIENTID).GetIntVector3();                                
                videomodechanged++;
            }
            
            if (optionParameters_[OPTION_Fullscreen].control_->GetSelection() != fullscreen)
            {
                fullscreen = optionParameters_[OPTION_Fullscreen].control_->GetSelection();
                videomodechanged++;
            }

            if (videomodechanged)
            {
                const bool borderless = graphics->GetBorderless();
                const bool resizable = graphics->GetResizable();
                const bool highDPI = graphics->GetHighDPI();
                const bool vsync = fullscreen ? true : false;
                const bool tripleBuffer = graphics->GetTripleBuffer();
                const int multiSample = graphics->GetMultiSample();

                graphics->SetMode(resolution.x_, resolution.y_, fullscreen, borderless, resizable, highDPI, vsync, tripleBuffer, multiSample, monitor, resolution.z_);

    //            bool cursorvisible = GameContext::Get().cursor_ ? GameContext::Get().cursor_->IsVisible() : false;
    //            GameContext::Get().InitMouse(GetContext(), MM_FREE);
    //            if (GameContext::Get().cursor_)
    //                GameContext::Get().cursor_->SetVisible(cursorvisible);
            }
        }
    }
    else if (category_ == "Network")
    {
        const int currentnetmode = GameNetwork::Get() ? GameNetwork::Get()->GetMode() : LOCALMODE;
        const int requirednetmode = optionParameters_[OPTION_NetMode].control_->GetSelection()+1;
        bool change = requirednetmode != currentnetmode;

        String serverip;
        int serverport = 0;
        GetNetServerParams(optionParameters_[OPTION_NetServer].control_->GetSelection(), serverip, serverport);
        if (!change)
            change = GameNetwork::Get() && (serverip != GameNetwork::Get()->GetServerIP() || serverport != GameNetwork::Get()->GetServerPort());

        if (change)
        {
            if (currentnetmode == LOCALMODE && !GameNetwork::Get())
                new GameNetwork(context_);

            URHO3D_LOGINFOF("OptionState() - ApplyParameters ... mode=%d serverip=%s serverport=%d ...", requirednetmode, serverip.CString(), serverport);

            GameNetwork::Get()->Stop();
            GameNetwork::Get()->SetMode(requirednetmode);
            GameNetwork::Get()->SetServerInfo(serverip, serverport);
            GameNetwork::Get()->Start();
        }
    }

    if (applybutton_)
        applybutton_->SetVisible(false);
}

void OptionState::ApplyNumPlayers()
{
    for (unsigned i=0; i < GameContext::Get().numPlayers_; i++)
        GameContext::Get().playerState_[i].avatar = i;

    for (unsigned i=0; i < GameContext::Get().MAX_NUMPLAYERS; i++)
    {
        if (optionParameters_[OPTION_ControlP1+i].control_)
        {
            optionParameters_[OPTION_ControlP1+i].control_->SetEnabled(i < GameContext::Get().numPlayers_);
            optionParameters_[OPTION_ControlP1+i].control_->GetParent()->SetVisible(i < GameContext::Get().numPlayers_);
        }
    }
}

void OptionState::ApplyLanguage()
{
    Localization* l10n = GetSubsystem<Localization>();
    if (l10n)
    {
        l10n->SetLanguage(GameContext::Get().gameConfig_.language_);
    }
}

void OptionState::ApplyEnlightment()
{
    if (GetSubsystem<Renderer>())
        GetSubsystem<Renderer>()->SetMaterialQuality(GameContext::Get().gameConfig_.enlightScene_);

    if (WeatherManager::Get())
        WeatherManager::Get()->ResetTimePeriod();
}

void OptionState::ApplyDebugRttScene()
{
    if (GameContext::Get().stateManager_->GetActiveState()->GetStateId() == "Play")
        ViewManager::Get()->ResizeViewports();
}


void OptionState::SubscribeToEvents()
{
    if (GameContext::Get().gameConfig_.debugUI_)
        SubscribeToEvent(E_POSTRENDERUPDATE, URHO3D_HANDLER(OptionState, OnPostRenderUpdate));

    for (unsigned i=0; i < OPTION_NUMCATEGORIES; i++)
    {
        Button* button = uioptionsframe_->GetChildStaticCast<Button>(optionCategories_[i], true);
        if (button)
            SubscribeToEvent(button, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickCategory));
    }

    // GameApplyWorldChange();

    Button* quitbutton = uioptionsframe_->GetChildStaticCast<Button>("Quit", true);
    if (quitbutton)
        SubscribeToEvent(quitbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleQuit));

    Button* savebutton = uioptionsframe_->GetChildStaticCast<Button>("Save", true);
    if (savebutton)
    {
        bool actived = GameContext::Get().stateManager_->GetActiveState()->IsInstanceOf<PlayState>();
        savebutton->SetEnabled(actived);
        savebutton->SetVisible(actived);
        if (actived)
            SubscribeToEvent(savebutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleSave));
    }

    Button* loadbutton = uioptionsframe_->GetChildStaticCast<Button>("Load", true);
    if (loadbutton)
        SubscribeToEvent(loadbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleLoad));

    // World

    if (optionParameters_[OPTION_WorldName].control_)
        SubscribeToEvent(optionParameters_[OPTION_WorldName].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleWorldNameChanged));

    if (optionParameters_[OPTION_WorldModel].control_)
        SubscribeToEvent(optionParameters_[OPTION_WorldModel].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleWorldModelChanged));

    if (optionParameters_[OPTION_WorldSize].control_)
        SubscribeToEvent(optionParameters_[OPTION_WorldSize].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleWorldSizeChanged));

    if (optionParameters_[OPTION_WorldSeed].control_)
        SubscribeToEvent(optionParameters_[OPTION_WorldSeed].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleWorldSeedChanged));

    Button* generateworldmapbutton = uioptionsframe_->GetChildStaticCast<Button>("GenerateWorldMap", true);
    if (generateworldmapbutton)
        SubscribeToEvent(generateworldmapbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleGenerateWorldMapButton));

    BorderImage* worldsnapshotimage = uioptionsframe_->GetChildStaticCast<BorderImage>("WorldSnapShot", true);
    if (worldsnapshotimage)
        SubscribeToEvent(worldsnapshotimage, E_CLICKEND, URHO3D_HANDLER(OptionState, HandleWorldSnapShotClicked));

    // Player

    if (optionParameters_[OPTION_MultiViews].control_)
        SubscribeToEvent(optionParameters_[OPTION_MultiViews].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleMultiViewsChanged));

    if (optionParameters_[OPTION_NumPlayers].control_)
        SubscribeToEvent(optionParameters_[OPTION_NumPlayers].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleNumPlayersChanged));

    if (optionParameters_[OPTION_ControlP1].control_)
    {
        SubscribeToEvent(optionParameters_[OPTION_ControlP1].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleControlP1Changed));
        Button* configcontrolP1button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_1", true);
        if (configcontrolP1button)
            SubscribeToEvent(configcontrolP1button, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickConfigControl));
    }

    if (optionParameters_[OPTION_ControlP2].control_)
    {
        SubscribeToEvent(optionParameters_[OPTION_ControlP2].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleControlP2Changed));
        Button* configcontrolP2button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_2", true);
        if (configcontrolP2button)
            SubscribeToEvent(configcontrolP2button, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickConfigControl));
    }

    if (optionParameters_[OPTION_ControlP3].control_)
    {
        SubscribeToEvent(optionParameters_[OPTION_ControlP3].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleControlP3Changed));
        Button* configcontrolP3button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_3", true);
        if (configcontrolP3button)
            SubscribeToEvent(configcontrolP3button, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickConfigControl));
    }

    if (optionParameters_[OPTION_ControlP4].control_)
    {
        SubscribeToEvent(optionParameters_[OPTION_ControlP4].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleControlP4Changed));
        Button* configcontrolP4button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_4", true);
        if (configcontrolP4button)
            SubscribeToEvent(configcontrolP4button, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickConfigControl));
    }

    UIElement* configcontrolframe = uioptionscontent_->GetChild(String("ConfigControls"));
    Button* resetbutton = configcontrolframe->GetChildStaticCast<Button>("ResetControl", true);
    if (resetbutton)
        SubscribeToEvent(resetbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleResetConfigControl));
    Button* applybutton = configcontrolframe->GetChildStaticCast<Button>("ApplyControl", true);
    if (applybutton)
        SubscribeToEvent(applybutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleApplyConfigControl));
    Button* backtoplayerbutton = configcontrolframe->GetChildStaticCast<Button>("Player", true);
    if (backtoplayerbutton)
        SubscribeToEvent(backtoplayerbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickCategory));
    for (int action=0; action < MAX_NUMACTIONS; action++)
    {
        Button* actionbutton = configcontrolframe->GetChildStaticCast<Button>(String(ActionNames_[action]), true);
        SubscribeToEvent(actionbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleClickConfigActionButton));
    }

    if (optionParameters_[OPTION_Language].control_)
        SubscribeToEvent(optionParameters_[OPTION_Language].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleLanguageChanged));

    if (optionParameters_[OPTION_Reactivity].control_)
        SubscribeToEvent(optionParameters_[OPTION_Reactivity].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleReactivityChanged));

    if (optionParameters_[OPTION_Resolution].control_)
        SubscribeToEvent(optionParameters_[OPTION_Resolution].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleResolutionFullScreenChanged));

    if (optionParameters_[OPTION_Fullscreen].control_)
        SubscribeToEvent(optionParameters_[OPTION_Fullscreen].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleResolutionFullScreenChanged));

    if (optionParameters_[OPTION_TextureQuality].control_)
        SubscribeToEvent(optionParameters_[OPTION_TextureQuality].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleTextureQualityChanged));

    if (optionParameters_[OPTION_TextureFilter].control_)
        SubscribeToEvent(optionParameters_[OPTION_TextureFilter].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleTextureFilterChanged));

    if (optionParameters_[OPTION_Underground].control_)
        SubscribeToEvent(optionParameters_[OPTION_Underground].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleUndergroundChanged));

    if (optionParameters_[OPTION_FluidEnable].control_)
        SubscribeToEvent(optionParameters_[OPTION_FluidEnable].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleFluidEnableChanged));

    if (optionParameters_[OPTION_RenderShapes].control_)
        SubscribeToEvent(optionParameters_[OPTION_RenderShapes].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleRenderShapesChanged));

    if (optionParameters_[OPTION_RenderDebug].control_)
        SubscribeToEvent(optionParameters_[OPTION_RenderDebug].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleRenderDebugChanged));

    if (optionParameters_[OPTION_DebugPhysics].control_)
        SubscribeToEvent(optionParameters_[OPTION_DebugPhysics].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleDebugPhysicsChanged));

    if (optionParameters_[OPTION_DebugWorld].control_)
        SubscribeToEvent(optionParameters_[OPTION_DebugWorld].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleDebugWorldChanged));

    if (optionParameters_[OPTION_DebugRenderShapes].control_)
        SubscribeToEvent(optionParameters_[OPTION_DebugRenderShapes].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleDebugRenderShapesChanged));

    if (optionParameters_[OPTION_DebugRttScene].control_)
        SubscribeToEvent(optionParameters_[OPTION_DebugRttScene].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleDebugRttSceneChanged));

    if (optionParameters_[OPTION_Music].control_)
        SubscribeToEvent(optionParameters_[OPTION_Music].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleMusicChanged));

    if (optionParameters_[OPTION_Sound].control_)
        SubscribeToEvent(optionParameters_[OPTION_Sound].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleSoundChanged));

    if (optionParameters_[OPTION_NetMode].control_)
        SubscribeToEvent(optionParameters_[OPTION_NetMode].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleNetworkMode));

    if (optionParameters_[OPTION_NetServer].control_)
        SubscribeToEvent(optionParameters_[OPTION_NetServer].control_, E_ITEMSELECTED, URHO3D_HANDLER(OptionState, HandleNetworkServer));


    Button* resetmapsbutton = uioptionsframe_->GetChildStaticCast<Button>("ResetMaps", true);
    if (resetmapsbutton)
        SubscribeToEvent(resetmapsbutton, E_RELEASED, URHO3D_HANDLER(OptionState, HandleResetMaps));

    Button* savemapswithfeatures = uioptionsframe_->GetChildStaticCast<Button>("SaveMapsFeatures", true);
    if (savemapswithfeatures)
        SubscribeToEvent(savemapswithfeatures, E_RELEASED, URHO3D_HANDLER(OptionState, HandleSaveMapFeatures));

    if (applybutton_)
        SubscribeToEvent(applybutton_, E_RELEASED, URHO3D_HANDLER(OptionState, HandleApplyParameters));

    if (closebutton_)
        SubscribeToEvent(closebutton_, E_RELEASED, URHO3D_HANDLER(OptionState, HandleCloseFrame));
}

void OptionState::UnsubscribeToEvents()
{
    for (unsigned i=0; i < OPTION_NUMCATEGORIES; i++)
    {
        Button* button = uioptionsframe_->GetChildStaticCast<Button>(optionCategories_[i], true);
        if (button)
            UnsubscribeFromEvent(button, E_RELEASED);
    }

    // Game

    Button* quitbutton = uioptionsframe_->GetChildStaticCast<Button>("Quit", true);
    if (quitbutton)
        UnsubscribeFromEvent(quitbutton, E_RELEASED);

    Button* savebutton = uioptionsframe_->GetChildStaticCast<Button>("Save", true);
    if (savebutton)
        UnsubscribeFromEvent(savebutton, E_RELEASED);

    Button* loadbutton = uioptionsframe_->GetChildStaticCast<Button>("Load", true);
    if (loadbutton)
        UnsubscribeFromEvent(loadbutton, E_RELEASED);

    // World

    if (optionParameters_[OPTION_WorldName].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_WorldName].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_WorldModel].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_WorldModel].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_WorldSize].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_WorldSize].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_WorldSeed].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_WorldSeed].control_, E_ITEMSELECTED);

    Button* generateworldmapbutton = uioptionsframe_->GetChildStaticCast<Button>("GenerateWorldMap", true);
    if (generateworldmapbutton)
        UnsubscribeFromEvent(generateworldmapbutton, E_RELEASED);

    BorderImage* worldsnapshotimage = uioptionsframe_->GetChildStaticCast<BorderImage>("WorldSnapShot", true);
    if (worldsnapshotimage)
        UnsubscribeFromEvent(worldsnapshotimage, E_CLICKEND);

    // Player

    if (optionParameters_[OPTION_MultiViews].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_MultiViews].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_NumPlayers].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_NumPlayers].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_ControlP1].control_)
    {
        UnsubscribeFromEvent(optionParameters_[OPTION_ControlP1].control_, E_ITEMSELECTED);
        Button* configcontrolP1button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_1", true);
        if (configcontrolP1button)
            UnsubscribeFromEvent(configcontrolP1button, E_RELEASED);
    }

    if (optionParameters_[OPTION_ControlP2].control_)
    {
        UnsubscribeFromEvent(optionParameters_[OPTION_ControlP2].control_, E_ITEMSELECTED);
        Button* configcontrolP2button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_2", true);
        if (configcontrolP2button)
            UnsubscribeFromEvent(configcontrolP2button, E_RELEASED);
    }

    if (optionParameters_[OPTION_ControlP3].control_)
    {
        UnsubscribeFromEvent(optionParameters_[OPTION_ControlP3].control_, E_ITEMSELECTED);
        Button* configcontrolP3button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_3", true);
        if (configcontrolP3button)
            UnsubscribeFromEvent(configcontrolP3button, E_RELEASED);
    }

    if (optionParameters_[OPTION_ControlP4].control_)
    {
        UnsubscribeFromEvent(optionParameters_[OPTION_ControlP4].control_, E_ITEMSELECTED);
        Button* configcontrolP4button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_4", true);
        if (configcontrolP4button)
            UnsubscribeFromEvent(configcontrolP4button, E_RELEASED);
    }

    UIElement* configcontrolframe = uioptionscontent_->GetChild(String("ConfigControls"));
    Button* resetbutton = configcontrolframe->GetChildStaticCast<Button>("ResetControl", true);
    if (resetbutton)
        UnsubscribeFromEvent(resetbutton, E_RELEASED);
    Button* applybutton = configcontrolframe->GetChildStaticCast<Button>("ApplyControl", true);
    if (applybutton)
        UnsubscribeFromEvent(applybutton, E_RELEASED);
    Button* backtoplayerbutton = configcontrolframe->GetChildStaticCast<Button>("Player", true);
    if (backtoplayerbutton)
        UnsubscribeFromEvent(backtoplayerbutton, E_RELEASED);
    for (int action=0; action < MAX_NUMACTIONS; action++)
    {
        Button* actionbutton = configcontrolframe->GetChildStaticCast<Button>(String(ActionNames_[action]), true);
        UnsubscribeFromEvent(actionbutton, E_RELEASED);
    }

    if (optionParameters_[OPTION_Language].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Language].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_Reactivity].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Reactivity].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_Resolution].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Resolution].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_Fullscreen].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Fullscreen].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_TextureQuality].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_TextureQuality].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_TextureFilter].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_TextureFilter].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_Underground].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Underground].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_FluidEnable].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_FluidEnable].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_RenderShapes].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_RenderShapes].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_RenderDebug].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_RenderDebug].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_DebugPhysics].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_DebugPhysics].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_DebugWorld].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_DebugWorld].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_DebugRenderShapes].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_DebugRenderShapes].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_DebugRttScene].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_DebugRttScene].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_Music].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Music].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_Sound].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_Sound].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_NetMode].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_NetMode].control_, E_ITEMSELECTED);

    if (optionParameters_[OPTION_NetServer].control_)
        UnsubscribeFromEvent(optionParameters_[OPTION_NetServer].control_, E_ITEMSELECTED);

    Button* resetmapsbutton = uioptionsframe_->GetChildStaticCast<Button>("ResetMaps", true);
    if (resetmapsbutton)
        UnsubscribeFromEvent(resetmapsbutton, E_RELEASED);

    Button* savemapswithfeatures = uioptionsframe_->GetChildStaticCast<Button>("SaveMapsFeatures", true);
    if (savemapswithfeatures)
        UnsubscribeFromEvent(savemapswithfeatures, E_RELEASED);

    if (applybutton_)
        UnsubscribeFromEvent(applybutton_, E_RELEASED);

    if (closebutton_)
        UnsubscribeFromEvent(closebutton_, E_RELEASED);
}

void OptionState::HandleClickCategory(StringHash eventType, VariantMap& eventData)
{
    Button* button = dynamic_cast<Button*>(context_->GetEventSender());
    if (!button)
        return;

//    URHO3D_LOGINFOF("OptionState() - HandleClickCategory ... click on category=%s !", button->GetName().CString());

    SwitchToCategory(button->GetName());
}

void OptionState::HandleClickConfigControl(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleClickConfigControl ... !");

    Button* button = dynamic_cast<Button*>(context_->GetEventSender());
    if (!button)
        return;

    Vector<String> args = button->GetName().Split('_');
    currentPlayerID_ = ToInt(args[1]);

    UpdateConfigControls(currentPlayerID_-1);

    SwitchToCategory("ConfigControls");

    Text* titleText = uioptionsframe_->GetChildStaticCast<Text>(String("TitleText"), true);
    if (titleText)
        titleText->SetText(String("Configure") + String(".") + String(ControlTypeNames_[GameContext::Get().playerState_[currentPlayerID_-1].controltype]) + String(".for.P") + String(currentPlayerID_));
}

void OptionState::HandleResetConfigControl(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleResetConfigControl ... configid=%d !", currentPlayerID_-1);

    ResetToCurrentConfigControlKeys(currentPlayerID_-1);
    UpdateConfigControls(currentPlayerID_-1);
}

void OptionState::HandleApplyConfigControl(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleApplyConfigControl ... !");

    ApplyConfigControlKeys(currentPlayerID_-1);
}

void OptionState::HandleClickConfigActionButton(StringHash eventType, VariantMap& eventData)
{
    Button* button = dynamic_cast<Button*>(context_->GetEventSender());
    if (!button)
        return;

    currentActionButton_ = GetEnumValue(button->GetName(), ActionNames_);

    URHO3D_LOGINFOF("OptionState() - HandleClickConfigActionButton ... actionbutton=%s ... ready to Capture Key !", button->GetName().CString());

    if (GameContext::Get().playerState_[currentPlayerID_-1].controltype == CT_KEYBOARD)
    {
        SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(OptionState, HandleCaptureKey));
    }
    else if (GameContext::Get().playerState_[currentPlayerID_-1].controltype == CT_JOYSTICK)
    {
        GameContext::Get().ui_->SetHandleJoystickEnable(false);
        SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(OptionState, HandleCaptureJoy));
        SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(OptionState, HandleCaptureJoy));
        SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(OptionState, HandleCaptureJoy));
    }

    UIElement* configcontrolframe = uioptionscontent_->GetChild(String("ConfigControls"));
    Text* text = configcontrolframe->GetChildStaticCast<Button>(String(ActionNames_[currentActionButton_]), true)->GetChildStaticCast<Text>(0);
    text->SetText("PressKey");
    text->SetColor(Color::YELLOW);
}

void OptionState::HandleCaptureKey(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(E_KEYDOWN);

    int key = eventData[KeyDown::P_KEY].GetInt();
    int scancode = eventData[KeyDown::P_SCANCODE].GetInt();
    tempkeysmap[currentPlayerID_-1][currentActionButton_] = scancode;

    UIElement* configcontrolframe = uioptionscontent_->GetChild(String("ConfigControls"));
    Text* text = configcontrolframe->GetChildStaticCast<Button>(String(ActionNames_[currentActionButton_]), true)->GetChildStaticCast<Text>(0);
    text->SetText(GameContext::Get().input_->GetKeyName(GameContext::Get().input_->GetKeyFromScancode(scancode)));
//    text->SetText(GameContext::Get().input_->GetScancodeName(scancode));
    text->SetColor(Color::WHITE);

    URHO3D_LOGINFOF("OptionState() - HandleCaptureKey...  Capture Key '%s' keycode=%s(%d) scancode=%s(%d) !",
                     text->GetText().CString(), GameContext::Get().input_->GetKeyName(key).CString(), key,
                     GameContext::Get().input_->GetScancodeName(scancode).CString(), scancode);
}

void OptionState::HandleCaptureJoy(StringHash eventType, VariantMap& eventData)
{
    int value = -1;

    if (eventType == E_JOYSTICKBUTTONDOWN)
    {
        int joy = eventData[JoystickButtonDown::P_JOYSTICKID].GetInt();
        value   = eventData[JoystickButtonDown::P_BUTTON].GetInt();

        URHO3D_LOGINFOF("OptionState() - HandleCaptureJoy ... E_JOYSTICKBUTTONDOWN joy=%d button=%d !", joy, value);

        int joyid = GameContext::Get().joystickIndexes_[currentPlayerID_-1] != -1 ? GameContext::Get().joystickIndexes_[currentPlayerID_-1] : 0;
        tempbuttonsmap[joyid][currentActionButton_] = value;
    }
    else if (eventType == E_JOYSTICKAXISMOVE)
    {
        if (Abs(eventData[JoystickAxisMove::P_POSITION].GetFloat()) > 0.5f)
        {
            int joy = eventData[JoystickAxisMove::P_JOYSTICKID].GetInt();
            value   = eventData[JoystickAxisMove::P_AXIS].GetInt() | JOY_AXI;

            URHO3D_LOGINFOF("OptionState() - HandleCaptureJoy ... E_JOYSTICKAXISMOVE joy=%d axis=%d !", joy, eventData[JoystickAxisMove::P_AXIS].GetInt());

            int joyid = GameContext::Get().joystickIndexes_[currentPlayerID_-1] != -1 ? GameContext::Get().joystickIndexes_[currentPlayerID_-1] : 0;
            tempbuttonsmap[joyid][currentActionButton_] = value;
        }
    }
    else if (eventType == E_JOYSTICKHATMOVE)
    {
        int joy = eventData[JoystickHatMove::P_JOYSTICKID].GetInt();
        value   = eventData[JoystickHatMove::P_HAT].GetInt() | JOY_HAT;

        URHO3D_LOGINFOF("OptionState() - HandleCaptureJoy ... E_JOYSTICKHATMOVE joy=%d hat=%d !", joy, eventData[JoystickHatMove::P_HAT].GetInt());

        int joyid = GameContext::Get().joystickIndexes_[currentPlayerID_-1] != -1 ? GameContext::Get().joystickIndexes_[currentPlayerID_-1] : 0;
        tempbuttonsmap[joyid][currentActionButton_] = value;
    }

    if (value != -1)
    {
        UIElement* configcontrolframe = uioptionscontent_->GetChild(String("ConfigControls"));
        Text* text = configcontrolframe->GetChildStaticCast<Button>(String(ActionNames_[currentActionButton_]), true)->GetChildStaticCast<Text>(0);
        text->SetText((value & JOY_AXI ? "AXI_" : value & JOY_HAT ? "HAT_" : "BTN_") + String(value & JOY_VALUE));
        text->SetColor(Color::WHITE);

        UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
        UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
        UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
        GameContext::Get().ui_->SetHandleJoystickEnable(true);
    }
}

void OptionState::HandleApplyParameters(StringHash eventType, VariantMap& eventData)
{
    ApplyParameters();
}

void OptionState::HandleCloseFrame(StringHash eventType, VariantMap& eventData)
{
    CloseFrame();
}


// Game Category Handle

void OptionState::HandleLoad(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleLoad ... !");

    CloseFrame();
    uioptionsframe_->SetVisible(false);

    GameCommands::Launch("load");
}

void OptionState::HandleSave(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGWARNINGF("OptionState() - HandleSave ... !");

    CloseFrame();

    GameCommands::Launch("save");
}

void OptionState::HandleQuit(StringHash eventType, VariantMap& eventData)
{
    const String& stateId = GameContext::Get().stateManager_->GetActiveState()->GetStateId();

    // Quit ?
    if (stateId == "MainMenu")
    {
        OpenQuitMessage();
    }
    // Continue ?
    else if (stateId == "Play")
    {
        PlayState* playstate = (PlayState*)GameContext::Get().stateManager_->GetActiveState();
        playstate->SetGamePause();
    }
}


// World Category Handle

void OptionState::HandleWorldNameChanged(StringHash eventType, VariantMap& eventData)
{
    const String& worldname = static_cast<Text*>(optionParameters_[OPTION_WorldName].control_->GetSelectedItem())->GetText();

    URHO3D_LOGINFOF("OptionState() - HandleWorldNameChanged ! name = %s", worldname.CString());

    int testzoneid = worldname != "TestZone2" ? 3 : 2;
    if (testzoneid != GameContext::Get().testZoneId_)
    {
        GameContext::Get().testZoneId_ = testzoneid;

        // Reload the last parameters from anlworlmodel
        sOptionsWorldIndex_ = MapStorage::GetWorldIndex(IntVector2(0, GameContext::Get().MAX_NUMLEVELS + GameContext::Get().testZoneId_));

        if (sOptionsWorldIndex_ != -1)
        {
            World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);

            if (GameContext::Get().testZoneId_ == 2)
            {
                SetDefaultWorldParameters();
            }
            else
            {
                optionParameters_[OPTION_WorldModel].control_->SetEditable(true);
                optionParameters_[OPTION_WorldSize].control_->SetEditable(true);
                optionParameters_[OPTION_WorldSeed].control_->SetEditable(true);

                if (info.worldModel_)
                {
                    sOptionsWorldModelFilename_ = GetNativePath(RemoveTrailingSlash("Data/" + info.worldModel_->GetName()));
                    int selection = 0;
                    if (sOptionsWorldModelFilename_.Contains("ellipsoid-zone2"))
                        selection = 1;
                    else if (sOptionsWorldModelFilename_.Contains("circle"))
                        selection = 2;
                    optionParameters_[OPTION_WorldModel].control_->SetSelection(selection);

                    sOptionsWorldModelSize_ = info.worldModel_->GetRadius();
                    selection = 0;
                    if (sOptionsWorldModelSize_ == 2.f)
                        selection = 1;
                    else if (sOptionsWorldModelSize_ == 4.f)
                        selection = 2;
                    else if (sOptionsWorldModelSize_ == 10.f)
                        selection = 3;
                    else if (sOptionsWorldModelSize_ == 16.f)
                        selection = 4;
                    optionParameters_[OPTION_WorldSize].control_->SetSelection(selection);

                    sOptionsWorldModelSeed_ = info.worldModel_->GetSeed();
                    selection = 0;
                    if (sOptionsWorldModelSeed_ == 1000)
                        selection = 1;
                    else if (sOptionsWorldModelSeed_ == 2000)
                        selection = 2;
                    else if (sOptionsWorldModelSeed_ == 66536)
                        selection = 3;
                    optionParameters_[OPTION_WorldSeed].control_->SetSelection(selection);
                }

                snapshotdirty_ = true;

                ApplyWorldChange();
            }
        }
        else
        {
            URHO3D_LOGERRORF("OptionState() - HandleWorldNameChanged ! name = %s ... ERROR no world index !", worldname.CString());
        }
    }
}

void OptionState::HandleWorldModelChanged(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().testZoneId_ == 2)
        return;

    int selection = optionParameters_[OPTION_WorldModel].control_->GetSelection();

    URHO3D_LOGINFOF("OptionState() - HandleWorldModelChanged ! selection = %d", selection);

    if (selection == 1)
        sOptionsWorldModelFilename_ = "anlworldVM-ellipsoid-zone2.xml";
    else if (selection == 2)
        sOptionsWorldModelFilename_ = "anlworldVM-circle.xml";
    else
        sOptionsWorldModelFilename_ = "anlworldVM-plane.xml";

    sOptionsWorldModelFilename_ = GetNativePath(RemoveTrailingSlash(World2DInfo::ATLASSETDIR + sOptionsWorldModelFilename_));

    URHO3D_LOGINFOF("OptionState() - HandleWorldModelChanged ! filename = %s", sOptionsWorldModelFilename_.CString());

    ApplyWorldChange();
}

void OptionState::HandleWorldSizeChanged(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().testZoneId_ == 2)
        return;

    int selection = optionParameters_[OPTION_WorldSize].control_->GetSelection();

    URHO3D_LOGINFOF("OptionState() - HandleWorldSizeChanged ! selection = %d", selection);

    if (selection == 0)
        sOptionsWorldModelSize_ = 1.f;
    else if (selection == 1)
        sOptionsWorldModelSize_ = 2.f;
    else if (selection == 2)
        sOptionsWorldModelSize_ = 4.f;
    else if (selection == 3)
        sOptionsWorldModelSize_ = 10.f;
    else if (selection == 4)
        sOptionsWorldModelSize_ = 16.f;

    URHO3D_LOGINFOF("OptionState() - HandleWorldSizeChanged ! size = %f", sOptionsWorldModelSize_);

    ApplyWorldChange();
}

void OptionState::HandleWorldSeedChanged(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().testZoneId_ == 2)
        return;

    URHO3D_LOGINFOF("OptionState() - HandleWorldSeedChanged !");

    DropDownList* control = optionParameters_[OPTION_WorldSeed].control_;
    int selection = control->GetSelection();

    URHO3D_LOGINFOF("OptionState() - HandleWorldSeedChanged ! selection = %d", selection);

    if (selection > 0)
        sOptionsWorldModelSeed_ = ToUInt(static_cast<Text*>(control->GetSelectedItem())->GetText());
    else if (control->IsHovering() || sOptionsLastSeedSelection_ != selection)
        sOptionsWorldModelSeed_ = GameRand::GetRand(ALLRAND, 65535);

    URHO3D_LOGINFOF("OptionState() - HandleWorldSeedChanged ! seed = %u", sOptionsWorldModelSeed_);

    sOptionsLastSeedSelection_ = selection;

    ApplyWorldChange();
}

void OptionState::HandleWorldSnapShotChanged(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleWorldSnapShotChanged !");

    BorderImage* bimage = uioptionsframe_->GetChildStaticCast<BorderImage>("WorldSnapShot", true);
    if (bimage)
    {
        bimage->SetTexture(sOptionsWorldSnapShotTexture_);
        bimage->SetImageRect(IntRect(0, 0, sOptionsWorldSnapShotTexture_->GetWidth(), sOptionsWorldSnapShotTexture_->GetHeight()));
        bimage->SetAlignment(HA_CENTER, VA_CENTER);

        UpdateSnapShotGrid();
    }
}

void OptionState::HandleWorldSnapShotClicked(StringHash eventType, VariantMap& eventData)
{
    World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);
    int button = eventData[ClickEnd::P_BUTTON].GetInt();

    if (info.worldModel_ && (button == MOUSEB_LEFT || button == MOUSEB_RIGHT || button == MOUSEB_MIDDLE))
    {
        float scalefactor = 1.f;

        // Mouse Right : Zoom In
        if (button == MOUSEB_RIGHT)
        {
            scalefactor = 0.5f;
        }
        // Mouse Middle : Zoom out
        else if (button == MOUSEB_MIDDLE)
        {
            scalefactor = 2.f;
        }

        const float diameter = info.worldModel_->GetRadius() != 0.f ? 2.f * info.worldModel_->GetRadius() : 1.f;
        const int lastnummaps = CeilToInt(diameter * sOptionsWorldSnapShotScale_);
        const int nummaps = CeilToInt(diameter * sOptionsWorldSnapShotScale_ * scalefactor);

        if ((scalefactor == 1.f || lastnummaps != nummaps) && nummaps >= 1 && nummaps <= sOptionsWorldSnapShotMaxMaps_)
        {
            BorderImage* canevas = uioptionsframe_->GetChildStaticCast<BorderImage>("WorldSnapShot", true);
            const int canevassize = canevas->GetSize().x_;

            IntVector2 mapcoordoffset;
            const IntVector2 position = canevas->ScreenToElement(IntVector2(eventData[ClickEnd::P_X].GetInt(), eventData[ClickEnd::P_Y].GetInt()));

            if (button == MOUSEB_RIGHT || button == MOUSEB_LEFT)
            {
                const float threshold = 0.1f;
                const int halfnummaps = CeilToInt((float)nummaps / 2.f);
                float posx, posy;
                int dirx, diry;

                if (nummaps > 1 || button == MOUSEB_LEFT)
                {
                    posx = (float)position.x_ / canevassize - 0.5f;
                    dirx = Sign(posx);
                    posx = Abs(posx);
                    if (posx > threshold)
                        mapcoordoffset.x_ = dirx * CeilToInt(posx * halfnummaps);

                    posy = (float)(canevassize-position.y_) / canevassize - 0.5f;
                    diry = Sign(posy);
                    posy = Abs(posy);
                    if (posy > threshold)
                        mapcoordoffset.y_ = diry * CeilToInt(posy * halfnummaps);
                }
                else
                {
                    posx = (float)position.x_ / canevassize - 0.5f;
                    dirx = Sign(posx);
                    posx = Abs(posx);
                    if (posx > threshold)
                        mapcoordoffset.x_ = dirx > 0 ? 0 : -1;

                    posy = (float)(canevassize-position.y_) / canevassize - 0.5f;
                    diry = Sign(posy);
                    posy = Abs(posy);
                    if (posy > threshold)
                        mapcoordoffset.y_ = diry > 0 ? 1 : 0;
                }

                URHO3D_LOGINFOF("OptionState() - HandleWorldSnapShotClicked : button=%d scale=%F izoom=%d ... halfnummaps=%d dirx=%d posx=%F diry=%d posy=%F ... mapoffset=%d %d",
                                button, sOptionsWorldSnapShotScale_, nummaps, halfnummaps, dirx, posx, diry, posy, mapcoordoffset.x_, mapcoordoffset.y_);
            }

            int mapzoomrecenter = RoundToInt(diameter * sOptionsWorldSnapShotScale_ * (1.f - scalefactor) * 0.5f);

            sOptionsWorldSnapShotTopLeftMap_.x_ = sOptionsWorldSnapShotTopLeftMap_.x_ + mapcoordoffset.x_ + mapzoomrecenter;
            sOptionsWorldSnapShotTopLeftMap_.y_ = sOptionsWorldSnapShotTopLeftMap_.y_ + mapcoordoffset.y_ - mapzoomrecenter;

            GameContext::Get().testZoneCustomDefaultMap_.x_ = sOptionsWorldSnapShotTopLeftMap_.x_;
            GameContext::Get().testZoneCustomDefaultMap_.y_ = sOptionsWorldSnapShotTopLeftMap_.y_;
            if (nummaps > 2)
            {
                int mapdefaultrecenter = CeilToInt(nummaps/2.f) - 1;
                GameContext::Get().testZoneCustomDefaultMap_.x_ += mapdefaultrecenter;
                GameContext::Get().testZoneCustomDefaultMap_.y_ -= mapdefaultrecenter;
            }

            sOptionsWorldSnapShotCenter_.x_ = (float)sOptionsWorldSnapShotTopLeftMap_.x_ / nummaps;
            sOptionsWorldSnapShotCenter_.y_ = (float)sOptionsWorldSnapShotTopLeftMap_.y_ / nummaps;

            sOptionsWorldSnapShotScale_ *= scalefactor;

            URHO3D_LOGINFOF("OptionState() - HandleWorldSnapShotClicked : button=%d scale=%F izoom=%d mapoffset=%d %d maprecenter=%d map=%d %d defaultmap=%d %d anlcenter=%F %F", button, sOptionsWorldSnapShotScale_, nummaps,
                            mapcoordoffset.x_, mapcoordoffset.y_, mapzoomrecenter, sOptionsWorldSnapShotTopLeftMap_.x_, sOptionsWorldSnapShotTopLeftMap_.y_,
                            GameContext::Get().testZoneCustomDefaultMap_.x_, GameContext::Get().testZoneCustomDefaultMap_.y_, sOptionsWorldSnapShotCenter_.x_, sOptionsWorldSnapShotCenter_.y_);

            UpdateSnapShots();
        }
    }
}

void OptionState::HandleGenerateWorldMapButton(StringHash eventType, VariantMap& eventData)
{
    /*
    Vector<String> modulerenderablenames;
    modulerenderablenames.Push("CaveMap");

    // Get the world point for test
    World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);
    info.worldModel_->GenerateWorldMapShots("worldmap", modulerenderablenames);
    */

    sOptionsWorldSnapshotGridOn_ = !sOptionsWorldSnapshotGridOn_;
    UpdateSnapShotGrid();
}

void OptionState::UpdateSnapShots(bool highres)
{
    snapshotdirty_ = false;

    // Get the world point for test
    World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);

    if (info.worldModel_)
    {
        info.worldModel_->StopUnfinishedWorks();

        Vector<String> modulerenderablenames;
        modulerenderablenames.Push("CaveMap");

        info.worldModel_->GenerateSnapShots(64, MiniMapColorFrontBlocked, sOptionsWorldSnapShotCenter_, sOptionsWorldSnapShotScale_, modulerenderablenames.Size(), modulerenderablenames.Buffer(), sOptionsWorldSnapShotTexture_);

        if (highres)
            info.worldModel_->GenerateSnapShots(1024, MiniMapColorFrontBlocked, sOptionsWorldSnapShotCenter_, sOptionsWorldSnapShotScale_, modulerenderablenames.Size(), modulerenderablenames.Buffer(), sOptionsWorldSnapShotTexture_);

        SubscribeToEvent(info.worldModel_.Get(), GAME_WORLDSNAPSHOTSAVED, URHO3D_HANDLER(OptionState, HandleWorldSnapShotChanged));
    }
}

void OptionState::UpdateSnapShotGrid()
{
    BorderImage* bimage = uioptionsframe_->GetChildStaticCast<BorderImage>("WorldSnapShot", true);
    const int canevassize = bimage->GetSize().x_;

    UIElement* gridroot = bimage->GetChild(0);
    if (gridroot)
        gridroot->Remove();

    if (sOptionsWorldSnapshotGridOn_)
    {
        World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);
        if (info.worldModel_)
        {
            SharedPtr<Texture2D> texture(context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/UI/point.png"));
            const int pointsize = texture->GetWidth();

            gridroot = bimage->CreateChild<UIElement>("GridRoot");
            gridroot->SetLayout(LM_FREE);
            gridroot->SetAlignment(HA_LEFT, VA_TOP);

            const float diameter = info.worldModel_->GetRadius() != 0.f ? 2.f * info.worldModel_->GetRadius() : 1.f;
            const int nummaps = CeilToInt(diameter * sOptionsWorldSnapShotScale_);
            const int minspacing = pointsize + 4;
            const int minnumpoints = canevassize / minspacing;

            int numpoints = nummaps;
            int spacing = canevassize / nummaps;

            // adjust spacing
            if (spacing < minspacing)
            {
                numpoints = minnumpoints;
                spacing = minspacing;
            }

            // adjust number of grid points
            if (spacing * numpoints < canevassize)
            {
                numpoints = canevassize / spacing;
            }

            URHO3D_LOGINFOF("OptionState() - UpdateSnapShotGrid : add grid for world mapping => nummaps=%d numpoints=%d canevassize=%dpix spacing=%dpix", nummaps, numpoints, canevassize, spacing);

            const Color color(0.8f, 0.4f, 0.f);

            for (int y=0; y <= numpoints; y++)
            {
                for (int x=0; x <= numpoints; x++)
                {
                    BorderImage* gridpoint = gridroot->CreateChild<BorderImage>();
                    gridpoint->SetTexture(texture);
                    gridpoint->SetImageRect(IntRect(0, 0, texture->GetWidth(), texture->GetHeight()));
                    gridpoint->SetLayout(LM_FREE);
                    gridpoint->SetAlignment(HA_CENTER, VA_CENTER);
                    gridpoint->SetSize(pointsize, pointsize);
                    gridpoint->SetPosition(x * spacing, y * spacing);
                    gridpoint->SetVisible(true);
                    gridpoint->SetOpacity(float(numpoints) / nummaps);
                    gridpoint->SetColor(gridpoint->GetOpacity() == 1.f ? color : Color::GRAY);
                }
            }
        }
    }
}

void OptionState::ApplyWorldChange()
{
    if (sOptionsWorldIndex_ == -1)
        return;

    // Get the world point for test
    World2DInfo& info = MapStorage::GetWorld2DInfo(sOptionsWorldIndex_);

    String currentworldmodelname = info.worldModel_ ? GetNativePath(RemoveTrailingSlash("Data/" + info.worldModel_->GetName())) : String::EMPTY;

    bool changemodel = GameContext::Get().testZoneId_ == 3 && !sOptionsWorldModelFilename_.Empty() && currentworldmodelname != sOptionsWorldModelFilename_;
    bool changesize  = GameContext::Get().testZoneId_ == 3 && info.worldModel_ && info.worldModel_->GetRadius() != sOptionsWorldModelSize_;
    bool changeseed  = GameContext::Get().testZoneId_ == 3 && info.worldModel_ && info.worldModel_->GetSeed() != sOptionsWorldModelSeed_;

    // check if previous snapshots workers unfinished ?
    if (info.worldModel_ && (changemodel || changesize || changeseed))
    {
        info.worldModel_->StopUnfinishedWorks();
    }

    // Change WorldModel
    if (changemodel)
    {
        String& name = MapStorage::GetWorldName(sOptionsWorldIndex_);

        URHO3D_LOGINFOF("OptionState() - ApplyWorldChange ... name=%s currentWorldModel=%s newWorldModel=%s ...",
                        name.CString(), currentworldmodelname.CString(), sOptionsWorldModelFilename_.CString());

        const String& worldname = static_cast<Text*>(optionParameters_[OPTION_WorldName].control_->GetSelectedItem())->GetText();
        if (!worldname.Empty())
            name = worldname;

        info.worldModel_.Reset();
        info.worldModel_ = context_->GetSubsystem<ResourceCache>()->GetTempResource<AnlWorldModel>(sOptionsWorldModelFilename_);

        URHO3D_LOGINFOF("OptionState() - ApplyWorldChange ... name=%s new model : ptr=%u", name.CString(), info.worldModel_.Get());
    }

    // Change Model Size
    if (changesize)
    {
        URHO3D_LOGINFOF("OptionState() - ApplyWorldChange ... new size = %f (old=%f)", sOptionsWorldModelSize_, info.worldModel_->GetRadius());
        info.worldModel_->SetRadius(sOptionsWorldModelSize_);
    }

    // Change Model Seed
    if (changeseed)
    {
        URHO3D_LOGINFOF("OptionState() - ApplyWorldChange ... new seed = %u (old=%u)", sOptionsWorldModelSeed_, info.worldModel_->GetSeed());

        info.worldModel_->SetSeedAllModules(sOptionsWorldModelSeed_);
        sOptionsWorldModelSeed_ = info.worldModel_->GetSeed();
    }

    // Apply the change
    if (changemodel || changesize || changeseed || snapshotdirty_)
    {
        // get the power of 2 for initial scale
        const float diameter = info.worldModel_->GetRadius() != 0.f ? 2.f * info.worldModel_->GetRadius() : 1.f;
        float scale = diameter * 2.f / info.worldModel_->GetScale().x_;
        int power = 0;
        while (scale > 1.f)
        {
            scale /= 2.f;
            power++;
        }
        sOptionsWorldSnapShotMaxMaps_ = Pow(2, power);
        sOptionsWorldSnapShotScale_ = sOptionsWorldSnapShotMaxMaps_ / diameter;

        sOptionsWorldSnapShotTopLeftMap_.x_ = -sOptionsWorldSnapShotMaxMaps_ / 2;
        sOptionsWorldSnapShotTopLeftMap_.y_ = sOptionsWorldSnapShotMaxMaps_ / 2;
        sOptionsWorldSnapShotCenter_.x_= (float)sOptionsWorldSnapShotTopLeftMap_.x_ / sOptionsWorldSnapShotMaxMaps_;
        sOptionsWorldSnapShotCenter_.y_= (float)sOptionsWorldSnapShotTopLeftMap_.y_ / sOptionsWorldSnapShotMaxMaps_;

        GameContext::Get().testZoneCustomModel_ = sOptionsWorldModelFilename_;
        GameContext::Get().testZoneCustomSize_ = sOptionsWorldModelSize_;
        GameContext::Get().testZoneCustomSeed_ = sOptionsWorldModelSeed_;

        // in TestZone, don't allow custom default map if not choosen on the snapshot : prefer use de default map defined in TestZone2.xml by default (see World2D::SetDefaultMapPoint)
        GameContext::Get().testZoneCustomDefaultMap_.x_ = GameContext::Get().testZoneId_ == 2 ? 65535 : 0;
        GameContext::Get().testZoneCustomDefaultMap_.y_ = 0;

        UpdateSnapShots(false);
//		UpdateSnapShots(GameContext::Get().testZoneId_ == 3);

        URHO3D_LOGINFOF("OptionState() - ApplyWorldChange ... sOptionsWorldSnapShotMaxMaps_=%d !", sOptionsWorldSnapShotMaxMaps_);
    }
}


// Player Category Handle

void OptionState::HandleMultiViewsChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_MultiViews].control_->GetSelection() != GameContext::Get().gameConfig_.multiviews_)
    {
        GameContext::Get().gameConfig_.multiviews_ = optionParameters_[OPTION_MultiViews].control_->GetSelection();

        URHO3D_LOGINFOF("OptionState() - HandleMultiViewsChanged ... ");
    }
}

void OptionState::HandleNumPlayersChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_NumPlayers].control_->GetSelection()+1 != GameContext::Get().numPlayers_)
    {
        GameContext::Get().numPlayers_ = optionParameters_[OPTION_NumPlayers].control_->GetSelection()+1;
        ApplyNumPlayers();
//        URHO3D_LOGINFOF("OptionState() - HandleNumPlayersChanged ... numplayers=%d !", GameContext::Get().numPlayers_);
    }
}

void OptionState::HandleControlP1Changed(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().playerState_[0].controltype != optionParameters_[OPTION_ControlP1].control_->GetSelection())
    {
        GameContext::Get().playerState_[0].controltype = optionParameters_[OPTION_ControlP1].control_->GetSelection();
        Button* configcontrolP1button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_1", true);
        if (configcontrolP1button)
            configcontrolP1button->SetVisible(GameContext::Get().playerState_[0].controltype < CT_SCREENJOYSTICK);
        if (GameContext::Get().players_[0])
            GameContext::Get().players_[0]->UpdateControls(true);


        URHO3D_LOGINFOF("OptionState() - HandleControlP1Changed ... controltype=%d !", GameContext::Get().playerState_[0].controltype);

    }
}

void OptionState::HandleControlP2Changed(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().playerState_[1].controltype != optionParameters_[OPTION_ControlP2].control_->GetSelection())
    {
        GameContext::Get().playerState_[1].controltype = optionParameters_[OPTION_ControlP2].control_->GetSelection();
        Button* configcontrolP2button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_2", true);
        if (configcontrolP2button)
            configcontrolP2button->SetVisible(GameContext::Get().playerState_[1].controltype < CT_SCREENJOYSTICK);
        if (GameContext::Get().players_[1])
            GameContext::Get().players_[1]->UpdateControls(true);

        URHO3D_LOGINFOF("OptionState() - HandleControlP2Changed ... controltype=%d !", GameContext::Get().playerState_[1].controltype);
    }
}

void OptionState::HandleControlP3Changed(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().playerState_[2].controltype != optionParameters_[OPTION_ControlP3].control_->GetSelection())
    {
        GameContext::Get().playerState_[2].controltype = optionParameters_[OPTION_ControlP3].control_->GetSelection();
        Button* configcontrolP3button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_3", true);
        if (configcontrolP3button)
            configcontrolP3button->SetVisible(GameContext::Get().playerState_[2].controltype < CT_SCREENJOYSTICK);
        if (GameContext::Get().players_[2])
            GameContext::Get().players_[2]->UpdateControls(true);

        URHO3D_LOGINFOF("OptionState() - HandleControlP3Changed ... controltype=%d !", GameContext::Get().playerState_[2].controltype);
    }
}

void OptionState::HandleControlP4Changed(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().playerState_[3].controltype != optionParameters_[OPTION_ControlP4].control_->GetSelection())
    {
        GameContext::Get().playerState_[3].controltype = optionParameters_[OPTION_ControlP4].control_->GetSelection();
        Button* configcontrolP4button = uioptionscontent_->GetChild(String("Player"))->GetChildStaticCast<Button>("ConfigControls_4", true);
        if (configcontrolP4button)
            configcontrolP4button->SetVisible(GameContext::Get().playerState_[3].controltype < CT_SCREENJOYSTICK);
        if (GameContext::Get().players_[3])
            GameContext::Get().players_[3]->UpdateControls(true);

        URHO3D_LOGINFOF("OptionState() - HandleControlP4Changed ... controltype=%d !", GameContext::Get().playerState_[3].controltype);
    }
}

void OptionState::HandleLanguageChanged(StringHash eventType, VariantMap& eventData)
{
    String lang = GetSubsystem<Localization>()->GetLanguage(optionParameters_[OPTION_Language].control_->GetSelection());
    if (lang != GameContext::Get().gameConfig_.language_)
    {
        GameContext::Get().gameConfig_.language_ = lang;
        ApplyLanguage();

//        URHO3D_LOGINFOF("OptionState() - HandleLanguageChanged ... language=%d !", GameContext::Get().gameConfig_.language_);
    }
}

void OptionState::HandleReactivityChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_Reactivity].control_->GetSelection() != GameContext::Get().gameConfig_.uiUpdateDelay_)
    {
        GameContext::Get().gameConfig_.uiUpdateDelay_ = optionParameters_[OPTION_Reactivity].control_->GetSelection();

//        URHO3D_LOGINFOF("OptionState() - HandleReactivityChanged ... reactivity=%d !", GameContext::Get().gameConfig_.uiUpdateDelay_);
    }
}


// Graphics Category Handle

void OptionState::HandleResolutionFullScreenChanged(StringHash eventType, VariantMap& eventData)
{
    CheckParametersChanged();
}

void OptionState::HandleTextureQualityChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_TextureQuality].control_->GetSelection() != GetSubsystem<Renderer>()->GetTextureQuality())
    {
        CheckParametersChanged();
//        URHO3D_LOGINFOF("OptionState() - HandleTextureQualityChanged ... texture Quality = %d !", optionParameters_[OPTION_TextureQuality].control_->GetSelection());
    }
}

void OptionState::HandleTextureFilterChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_TextureFilter].control_->GetSelection() != GetSubsystem<Renderer>()->GetTextureFilterMode())
    {
        CheckParametersChanged();
//        URHO3D_LOGINFOF("OptionState() - HandleTextureFilterChanged ... texture Filter = %d !", optionParameters_[OPTION_TextureFilter].control_->GetSelection());
    }
}

void OptionState::HandleUndergroundChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_Underground].control_->GetSelection() != GameContext::Get().gameConfig_.enlightScene_)
    {
        GameContext::Get().gameConfig_.enlightScene_ = optionParameters_[OPTION_Underground].control_->GetSelection();
        URHO3D_LOGINFOF("OptionState() - HandleUndergroundChanged ... enlightScene_=%d !", GameContext::Get().gameConfig_.enlightScene_);

        ApplyEnlightment();
    }
}


// Sound Category Handle

void OptionState::HandleMusicChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_Music].control_->GetSelection() != GameContext::Get().gameConfig_.musicEnabled_)
    {
        GameContext::Get().gameConfig_.musicEnabled_ = optionParameters_[OPTION_Music].control_->GetSelection();

        if (!GameContext::Get().gameConfig_.musicEnabled_)
        {
            GameHelpers::StopMusic(GameContext::Get().rootScene_);
        }
        else
        {
            GameHelpers::SetMusicVolume(((float)GameContext::Get().gameConfig_.musicEnabled_)/(optionParameters_[OPTION_Music].control_->GetNumItems()-1));
            GameHelpers::PlayMusic(GameContext::Get().rootScene_, GameContext::Get().currentMusic, true);
        }

//        URHO3D_LOGINFOF("OptionState() - HandleMusicChanged ... music=%d !", GameContext::Get().gameConfig_.musicEnabled_);
    }
}

void OptionState::HandleSoundChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_Sound].control_->GetSelection() != GameContext::Get().gameConfig_.soundEnabled_)
    {
        GameContext::Get().gameConfig_.soundEnabled_ = optionParameters_[OPTION_Sound].control_->GetSelection();

        GameHelpers::SetSoundVolume(((float)GameContext::Get().gameConfig_.soundEnabled_)/(optionParameters_[OPTION_Sound].control_->GetNumItems()-1));

        if (GameContext::Get().gameConfig_.soundEnabled_)
            GameHelpers::SpawnSound(GameContext::Get().cameraNode_.Get(), "Sounds/087-Action02.ogg");

        URHO3D_LOGINFOF("OptionState() - HandleSoundChanged ... sound=%d !", GameContext::Get().gameConfig_.soundEnabled_);

        SendEvent(GAME_SOUNDVOLUMECHANGED);
    }
}


// Network Category Handle

void OptionState::HandleNetworkMode(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleNetworkMode ... ");
    CheckParametersChanged();

    optionParameters_[OPTION_NetServer].control_->GetParent()->SetVisible(optionParameters_[OPTION_NetMode].control_->GetSelection() == CLIENTMODE-1);
}

void OptionState::HandleNetworkServer(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleNetworkServer ... ");
    CheckParametersChanged();
}


// Debug Category Handle

void OptionState::HandleFluidEnableChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_FluidEnable].control_->GetSelection() != GameContext::Get().gameConfig_.fluidEnabled_)
    {
        GameContext::Get().gameConfig_.fluidEnabled_ = optionParameters_[OPTION_FluidEnable].control_->GetSelection();
        GameContext::Get().gameConfig_.forceChangeRenderPath_ = true;

        if (World2D::GetWorld()->GetStorage() && !GameContext::Get().gameConfig_.fluidEnabled_)
        {
            const HashMap<ShortIntVector2, Map* >&  maps = World2D::GetWorld()->GetStorage()->GetMapsInMemory();
            for (HashMap<ShortIntVector2, Map* >::ConstIterator it=maps.Begin(); it != maps.End(); ++it)
            {
                if (it->second_)
                    it->second_->PullFluids();
            }
        }

        URHO3D_LOGINFOF("OptionState() - HandleFluidEnableChanged ... fluid=%s !", GameContext::Get().gameConfig_.fluidEnabled_ ? "enable":"disable");
    }
}

void OptionState::HandleRenderShapesChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_RenderShapes].control_->GetSelection() != GameContext::Get().gameConfig_.renderShapes_)
    {
        GameContext::Get().gameConfig_.renderShapes_ = optionParameters_[OPTION_RenderShapes].control_->GetSelection();
    }
}

void OptionState::HandleRenderDebugChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_RenderDebug].control_->GetSelection() != GameContext::Get().DrawDebug_)
    {
        GameContext::Get().SetRenderDebug(optionParameters_[OPTION_RenderDebug].control_->GetSelection());
    }
}

void OptionState::HandleDebugPhysicsChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_DebugPhysics].control_->GetSelection() != GameContext::Get().gameConfig_.debugPhysics_)
    {
        GameContext::Get().gameConfig_.debugPhysics_ = optionParameters_[OPTION_DebugPhysics].control_->GetSelection();
        if (GameContext::Get().gameConfig_.debugPhysics_)
        {
            optionParameters_[OPTION_RenderDebug].control_->SetSelection(1);
            GameContext::Get().SetRenderDebug(true);
        }
    }
}

void OptionState::HandleDebugWorldChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_DebugWorld].control_->GetSelection() != GameContext::Get().gameConfig_.debugWorld2D_)
    {
        GameContext::Get().gameConfig_.debugWorld2D_ = optionParameters_[OPTION_DebugWorld].control_->GetSelection();
        if (GameContext::Get().gameConfig_.debugWorld2D_)
        {
            optionParameters_[OPTION_RenderDebug].control_->SetSelection(1);
            GameContext::Get().SetRenderDebug(true);
        }
    }
}

void OptionState::HandleDebugRenderShapesChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_DebugRenderShapes].control_->GetSelection() != GameContext::Get().gameConfig_.debugRenderShape_)
    {
        GameContext::Get().gameConfig_.debugRenderShape_ = optionParameters_[OPTION_DebugRenderShapes].control_->GetSelection();
        if (GameContext::Get().gameConfig_.debugRenderShape_)
        {
            optionParameters_[OPTION_RenderDebug].control_->SetSelection(1);
            GameContext::Get().SetRenderDebug(true);
        }
    }
}

void OptionState::HandleDebugRttSceneChanged(StringHash eventType, VariantMap& eventData)
{
    if (optionParameters_[OPTION_DebugRttScene].control_->GetSelection() != GameContext::Get().gameConfig_.debugRttScene_)
    {
        GameContext::Get().gameConfig_.debugRttScene_ = optionParameters_[OPTION_DebugRttScene].control_->GetSelection();
        ApplyDebugRttScene();
    }
}

void OptionState::HandleResetMaps(StringHash eventType, VariantMap& eventData)
{
    GameContext::Get().resetWorldMaps_ = true;

    URHO3D_LOGINFOF("OptionState() - HandleResetMaps ... : the reset of the maps will be made at start of PlayState !");
}

void OptionState::HandleSaveMapFeatures(StringHash eventType, VariantMap& eventData)
{
    GameContext::Get().allMapsPrefab_ = !GameContext::Get().allMapsPrefab_;

    URHO3D_LOGINFOF("OptionState() - HandleSaveMapFeatures ... : %s !", GameContext::Get().allMapsPrefab_ ? "ON":"OFF");
}



void OptionState::HandleQuitMessageAck(StringHash eventType, VariantMap& eventData)
{
    using namespace MessageACK;

    UnsubscribeFromEvent(quitMessageBox_, E_MESSAGEACK);

    if (eventData[P_OK].GetBool())
    {
        GameContext::Get().Exit();
        return;
    }

    // Initialize controls
    GetSubsystem<Input>()->ResetStates();

    GameHelpers::ToggleModalWindow();
}

void OptionState::HandleMenuButton(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("OptionState() - HandleMenuButton ...");

    Window* modelWindow = GameHelpers::GetModalWindow();
    if (!modelWindow || modelWindow == uioptions_.Get())
    {
        if ((eventType == E_KEYDOWN || eventType == E_JOYSTICKBUTTONDOWN) && !IsVisible())
            ResetToMainCategory();

        ToggleFrame();
    }
}

void OptionState::HandleKeyEscape(StringHash eventType, VariantMap& eventData)
{
    bool escape = false;

    if (eventType == E_KEYDOWN)
    {
        escape = eventData[KeyDown::P_KEY].GetInt() == KEY_ESCAPE;
    }
    else if (eventType == E_JOYSTICKBUTTONDOWN)
    {
        JoystickState* joystick = GameContext::Get().input_->GetJoystick(eventData[JoystickButtonDown::P_JOYSTICKID].GetInt());
        escape = joystick && joystick->GetButtonPress(4);
    }

    if (escape)
    {
        SendEvent(UI_ESCAPEPANEL);

        UIElement* focusedelement = GameContext::Get().ui_->GetFocusElement();
        if (!focusedelement || focusedelement == uioptionsframe_)
            HandleMenuButton(eventType, eventData);
    }
}

void OptionState::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("OptionState() - HandleScreenResized ...");

    Graphics* graphics = GetSubsystem<Graphics>();
    UI* ui = GameContext::Get().ui_;
    uioptions_->SetSize(ui->GetRoot()->GetSize());
    GameHelpers::SetUIScale(uioptionsframe_, IntVector2(782, 522));

    uioptionsframe_->SetPivot(0.5f, 0.5f);
    uioptionsframe_->SetMinAnchor(0.5f, 0.5f);
    uioptionsframe_->SetMaxAnchor(0.5f, 0.5f);
    uioptionsframe_->SetEnableAnchor(true);

    menubutton_->SetSize(40 * GameContext::Get().uiScale_, 40 * GameContext::Get().uiScale_);
    menubutton_->SetPosition(10 * GameContext::Get().uiScale_, 10 * GameContext::Get().uiScale_);

    initialFramePosition_ = uioptionsframe_->GetPosition();

    SynchronizeParameters();

    URHO3D_LOGINFOF("OptionState() - HandleScreenResized ... : graphics=%d,%d frame position=%d,%d ... OK !",
                    graphics->GetWidth(), graphics->GetHeight(), initialFramePosition_.x_, initialFramePosition_.y_);
}


void OptionState::OnPostRenderUpdate(StringHash eventType, VariantMap& eventData)
{
    if (GameContext::Get().gameConfig_.debugUI_ && GameContext::Get().DrawDebug_)
    {
        UI* ui = GetSubsystem<UI>();
        if (!ui || !uioptions_)
            return;

        ui->DebugDraw(uioptions_);

        PODVector<UIElement*> children;
        uioptions_->GetChildren(children, true);
        for (PODVector<UIElement*>::ConstIterator it=children.Begin(); it!=children.End(); ++it)
        {
            UIElement* element = *it;
            if (element->IsVisibleEffective())
                ui->DebugDraw(element);
        }
    }
}

