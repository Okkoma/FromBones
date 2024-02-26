#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Network/NetworkEvents.h>

#include <Urho3D/Input/Controls.h>

#include <Urho3D/UI/UI.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"

#include "GOC_Animator2D.h"

#include "Player.h"
#include "UISlotPanel.h"

#include "GOC_ControllerPlayer.h"


#define TICKPOSITIONDELAY  50U

GOC_PlayerController::GOC_PlayerController(Context* context) :
    GOC_Controller(context, GO_Player),
    touchEnabled(false),
    keysMap_(0),
    buttonsMap_(0),
    playerID(0),
    joystickindex_(-1),
    controlID_(-1)
//    tickposition_(0)
{
    SetControllerType(GO_Player);
}

GOC_PlayerController::~GOC_PlayerController()
{
//    URHO3D_LOGINFOF("~GOC_PlayerController()");
}

void GOC_PlayerController::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_PlayerController>();
}

void GOC_PlayerController::SetKeyControls(int controlID)
{
    controlID_ = controlID;
    keysMap_ = &GameContext::Get().keysMap_[controlID][0];
    touchEnabled = false;
    joystickindex_ = -1;
}

void GOC_PlayerController::SetJoystickControls(int controlID)
{
    URHO3D_LOGERRORF("GOC_PlayerController() - SetJoystickControls : controlID=%d ", controlID);
    controlID_ = controlID;
    joystickindex_ = GameContext::Get().joystickIndexes_[controlID];
    buttonsMap_ = &GameContext::Get().playerState_[controlID].joybuttons[0];
    keysMap_ = 0;

    for (int action=0; action < MAX_NUMACTIONS; action++)
        GameContext::Get().playerState_[controlID].joybuttons[action] = GameContext::Get().buttonsMap_[joystickindex_][action];
}

void GOC_PlayerController::SetTouchControls(int screenJoyId)
{
    keysMap_ = &GameContext::Get().keysMap_[0][0];
    touchEnabled = true;
    screenJoystickID = screenJoyId;
}

int GOC_PlayerController::GetKeyForAction(int action) const
{
    if (keysMap_)
        return keysMap_[action];
    else if (buttonsMap_)
        return buttonsMap_[action];

    return 0;
}

void GOC_PlayerController::Start()
{
    URHO3D_LOGERRORF("GOC_PlayerController() - Start() !");

    GOC_Controller::Start();

    followPath_ = false;

    if (mainController_)
    {
        if (joystickindex_ != -1)
        {
            JoystickState* joystick = GameContext::Get().joystickByControllerIds_[controlID_];
            if (joystick)
                joystick->Reset();
        }
        control_.Reset();

//        URHO3D_LOGINFOF("GOC_PlayerController() - Start : node = %s(%u) mainController_=%s", node_->GetName().CString(), node_->GetID(), mainController_ ? "true" : "false");
        SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_PlayerController, HandleLocalUpdate));
    }

    SubscribeToEvent(GetNode(), GOC_LIFEDEAD, URHO3D_HANDLER(GOC_PlayerController, HandleStop));
}

void GOC_PlayerController::Stop()
{
    UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
    UnsubscribeFromEvent(GetNode(), GOC_LIFEDEAD);

    GOC_Controller::Stop();
}


void GOC_PlayerController::HandleStop(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_PlayerController() - HandleStop : node = %s(%u) ", node_->GetName().CString(), node_->GetID());
    Stop();
}

void GOC_PlayerController::HandleLocalUpdate(StringHash eventType, VariantMap& eventData)
{
    //URHO3D_LOGINFOF("GOC_PlayerController() - HandleLocalUpdate : Node=%s(%u)  !", node_->GetName().CString(), node_->GetID());

    CheckMountNode();

    if (!GameContext::Get().input_ || GameContext::Get().HasConsoleFocus())
        return;

    prevbuttons_ = control_.buttons_;

    Input& input = *GameContext::Get().input_;

    UpdateKeyControls(input);
    UpdateTouchControls(input);
    UpdateJoystickControls(input);

    if (mainController_)
    {
        if (control_.IsButtonDown(CTRL_INTERACT))
            node_->SendEvent(GOC_CONTROLACTION_INTERACT);
        if (control_.IsButtonDown(CTRL_STATUS))
            node_->SendEvent(GOC_CONTROLACTION_STATUS);
        else if (control_.IsButtonDown(CTRL_PREVPANEL))
            node_->SendEvent(GOC_CONTROLACTION_PREVFOCUSPANEL);
        else if (control_.IsButtonDown(CTRL_NEXTPANEL))
            node_->SendEvent(GOC_CONTROLACTION_NEXTFOCUSPANEL);
        if (control_.IsButtonDown(CTRL_NEXTENTITY))
            node_->SendEvent(GOC_CONTROLACTION_NEXTFOCUSENTITY);
        else if (control_.IsButtonDown(CTRL_PREVENTITY))
            node_->SendEvent(GOC_CONTROLACTION_PREVFOCUSENTITY);
    }

    if (GameContext::Get().uiLockSceneControllers_)
        return;

    bool update = GOC_Controller::Update(control_.buttons_, false);
}

inline void GOC_PlayerController::UpdateKeyControls(Input& input)
{
    if (!keysMap_)
        return;

//    URHO3D_LOGINFOF("GOC_PlayerController() - UpdateKeyControls : Node=%s(%u)  !", node_->GetName().CString(), node_->GetID());

    control_.SetButtons(CTRL_UP, input.GetScancodeDown(keysMap_[ACTION_UP]));
    control_.SetButtons(CTRL_DOWN, input.GetScancodeDown(keysMap_[ACTION_DOWN]));
    if (input.GetScancodeDown(keysMap_[ACTION_LEFT]))
    {
        control_.SetButtons(CTRL_LEFT, true);
        control_.SetButtons(CTRL_RIGHT, false);
    }
    else if (input.GetScancodeDown(keysMap_[ACTION_RIGHT]))
    {
        control_.SetButtons(CTRL_RIGHT, true);
        control_.SetButtons(CTRL_LEFT, false);
    }
    else
    {
        control_.SetButtons(CTRL_RIGHT, false);
        control_.SetButtons(CTRL_LEFT, false);
    }

    control_.SetButtons(CTRL_JUMP, HasInteraction() && keysMap_[ACTION_INTERACT] == keysMap_[ACTION_JUMP]  ? false : input.GetScancodeDown(keysMap_[ACTION_JUMP]) || input.GetScancodePress(keysMap_[ACTION_JUMP]));
    control_.SetButtons(CTRL_FIRE, HasInteraction() && keysMap_[ACTION_INTERACT] == keysMap_[ACTION_FIRE1] ? false : input.GetScancodeDown(keysMap_[ACTION_FIRE1]) || input.GetScancodePress(keysMap_[ACTION_FIRE1]));
    control_.SetButtons(CTRL_FIRE2, input.GetScancodeDown(keysMap_[ACTION_FIRE2]) || input.GetScancodePress(keysMap_[ACTION_FIRE2]));
    control_.SetButtons(CTRL_FIRE3, input.GetScancodePress(keysMap_[ACTION_FIRE3]));

    control_.SetButtons(CTRL_INTERACT, input.GetScancodePress(keysMap_[ACTION_INTERACT]));

    control_.SetButtons(CTRL_STATUS, input.GetScancodePress(keysMap_[ACTION_STATUS]));
    control_.SetButtons(CTRL_PREVPANEL, input.GetScancodePress(keysMap_[ACTION_PREVPANEL]));
    control_.SetButtons(CTRL_NEXTPANEL, input.GetScancodePress(keysMap_[ACTION_NEXTPANEL]));
    control_.SetButtons(CTRL_PREVENTITY, input.GetScancodePress(keysMap_[ACTION_PREVENTITY]));
    control_.SetButtons(CTRL_NEXTENTITY, input.GetScancodePress(keysMap_[ACTION_NEXTENTITY]));
}

inline void GOC_PlayerController::UpdateTouchControls(Input& input)
{
    if (!touchEnabled)
        return;

//    URHO3D_LOGINFOF("GOC_PlayerController() - UpdateTouchControls : Node=%s(%u)  !", node_->GetName().CString(), node_->GetID());

    // use the touches
    if (screenJoystickID == -1)
    {
        for (unsigned i=0; i < input.GetNumTouches(); ++i)
        {
            if (!input.GetTouch(i))
                continue;

            TouchState& touch = *input.GetTouch(i);

            if (touch.touchedElement_)
                continue;

            // move input
            if (touch.position_.x_ < GameContext::Get().screenwidth_/2)
            {
                const int sensibilityX = Clamp(3.f * GameContext::Get().screenRatioX_, 2.f, 50.f);
                const int sensibilityY = Clamp(3.f * GameContext::Get().screenRatioY_, 2.f, 50.f);

                control_.SetButtons(CTRL_UP, touch.delta_.y_ < -sensibilityY);
                control_.SetButtons(CTRL_DOWN, touch.delta_.y_ > sensibilityY);
                control_.SetButtons(CTRL_LEFT, touch.delta_.x_ < -sensibilityX || ((prevbuttons_ & CTRL_LEFT) && touch.delta_.x_ <= 0));
                control_.SetButtons(CTRL_RIGHT, touch.delta_.x_ > sensibilityX || ((prevbuttons_ & CTRL_RIGHT) && touch.delta_.x_ >= 0));
                control_.SetButtons(CTRL_JUMP, (touch.pressure_ >= 0.3f && (touch.delta_.y_ < 0 || control_.IsButtonDown(CTRL_DOWN)))
                                    || ((prevbuttons_ & CTRL_JUMP) && touch.delta_.y_ <= 0));
            }
            else
                // action input
            {
                control_.SetButtons(CTRL_FIRE, touch.pressure_ >= 0.3f);
//                control_.SetButtons(CTRL_FIRE, touch.pressure_ >= 0.3f && touch.pressure_ < 0.6f);
//                control_.SetButtons(CTRL_FIRE2, touch.pressure_ >= 0.6f);
            }
        }
    }
    // use the screen joystick
    else
    {
        control_.SetButtons(CTRL_UP, input.GetKeyDown(KEY_W) || input.GetScancodeDown(keysMap_[ACTION_UP]));
        control_.SetButtons(CTRL_DOWN, input.GetKeyDown(KEY_S) || input.GetScancodeDown(keysMap_[ACTION_DOWN]));
        control_.SetButtons(CTRL_LEFT, input.GetKeyDown(KEY_A) || input.GetScancodeDown(keysMap_[ACTION_LEFT]));
        control_.SetButtons(CTRL_RIGHT, input.GetKeyDown(KEY_D) || input.GetScancodeDown(keysMap_[ACTION_RIGHT]));
        control_.SetButtons(CTRL_JUMP, input.GetKeyDown(KEY_SPACE) || input.GetKeyPress(KEY_SPACE) || input.GetScancodeDown(keysMap_[ACTION_JUMP]) || input.GetScancodePress(keysMap_[ACTION_JUMP]));
        control_.SetButtons(CTRL_FIRE, input.GetKeyDown(KEY_E) || input.GetKeyPress(KEY_E) || input.GetScancodeDown(keysMap_[ACTION_FIRE1]) || input.GetScancodePress(keysMap_[ACTION_FIRE1]));
        control_.SetButtons(CTRL_FIRE2, input.GetKeyDown(KEY_F) || input.GetKeyPress(KEY_F) || input.GetScancodeDown(keysMap_[ACTION_FIRE2]) || input.GetScancodePress(keysMap_[ACTION_FIRE2]));
        control_.SetButtons(CTRL_FIRE3, input.GetKeyPress(KEY_R) || input.GetScancodePress(keysMap_[ACTION_FIRE3]));
    }
}

inline void GOC_PlayerController::UpdateJoystickControls(Input& input)
{
    if (joystickindex_ == -1 || !buttonsMap_)
        return;

    JoystickState* joystick = GameContext::Get().joystickByControllerIds_[controlID_];
    if (!joystick)
        return;

    bool state = false;
    /*
        SDL_CONTROLLER_BUTTON_A,
        SDL_CONTROLLER_BUTTON_B,
        SDL_CONTROLLER_BUTTON_X,
        SDL_CONTROLLER_BUTTON_Y,
        SDL_CONTROLLER_BUTTON_BACK,
        SDL_CONTROLLER_BUTTON_GUIDE,
        SDL_CONTROLLER_BUTTON_START,
        SDL_CONTROLLER_BUTTON_LEFTSTICK,
        SDL_CONTROLLER_BUTTON_RIGHTSTICK,
        SDL_CONTROLLER_BUTTON_LEFTSHOULDER,
        SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
        SDL_CONTROLLER_BUTTON_DPAD_UP,
        SDL_CONTROLLER_BUTTON_DPAD_DOWN,
        SDL_CONTROLLER_BUTTON_DPAD_LEFT,
        SDL_CONTROLLER_BUTTON_DPAD_RIGHT,
    */

    // JUMP
    int button;

    if (HasInteraction() && buttonsMap_[ACTION_INTERACT] == buttonsMap_[ACTION_JUMP])
        control_.SetButtons(CTRL_JUMP, false);
    else
    {
        button = buttonsMap_[ACTION_JUMP];
        if ((button & (JOY_HAT|JOY_AXI)) == 0)
            control_.SetButtons(CTRL_JUMP, joystick->GetButtonPress(button & JOY_VALUE) || joystick->GetButtonDown(button & JOY_VALUE));
        else if (button & JOY_HAT)
            control_.SetButtons(CTRL_JUMP, joystick->GetHatPosition(button & JOY_VALUE));
        else
            control_.SetButtons(CTRL_JUMP, joystick->GetAxisPress(button & JOY_VALUE));
    }

    // FIRE
    if (HasInteraction() && buttonsMap_[ACTION_INTERACT] == buttonsMap_[ACTION_FIRE1])
        control_.SetButtons(CTRL_FIRE, false);
    else
    {
        button = buttonsMap_[ACTION_FIRE1];
        if (button & JOY_HAT)
            control_.SetButtons(CTRL_FIRE, joystick->GetHatPosition(button & JOY_VALUE));
        else if (button & JOY_AXI)
            control_.SetButtons(CTRL_FIRE, joystick->GetAxisPress(button & JOY_VALUE));
        else
            control_.SetButtons(CTRL_FIRE, joystick->GetButtonPress(button & JOY_VALUE) || joystick->GetButtonDown(button & JOY_VALUE));
    }

    // FIRE2
    button = buttonsMap_[ACTION_FIRE2];
    if (button & JOY_HAT)
        control_.SetButtons(CTRL_FIRE2, joystick->GetHatPosition(button & JOY_VALUE));
    else if (button & JOY_AXI)
        control_.SetButtons(CTRL_FIRE2, joystick->GetAxisPress(button & JOY_VALUE));
    else
        control_.SetButtons(CTRL_FIRE2, joystick->GetButtonPress(button & JOY_VALUE) || joystick->GetButtonDown(button & JOY_VALUE));

    // FIRE3
    button = buttonsMap_[ACTION_FIRE3];
    if (button & JOY_HAT)
        control_.SetButtons(CTRL_FIRE3, joystick->GetHatPosition(button & JOY_VALUE));
    else if (button & JOY_AXI)
        control_.SetButtons(CTRL_FIRE3, joystick->GetAxisPress(button & JOY_VALUE));
    else
        control_.SetButtons(CTRL_FIRE3, joystick->GetButtonPress(button & JOY_VALUE));

    // INTERACT
    button = buttonsMap_[ACTION_INTERACT];
    if ((button & (JOY_HAT|JOY_AXI)) == 0)
        control_.SetButtons(CTRL_INTERACT, joystick->GetButtonPress(button & JOY_VALUE));
    else if (button & JOY_HAT)
        control_.SetButtons(CTRL_INTERACT, joystick->GetHatPosition(button & JOY_VALUE));
    else
        control_.SetButtons(CTRL_INTERACT, joystick->GetAxisPress(button & JOY_VALUE));

    // UI : Focus/Defocus
    button = buttonsMap_[ACTION_STATUS];
    if (button & JOY_HAT)
        control_.SetButtons(CTRL_STATUS, joystick->GetHatPosition(button & JOY_VALUE));
    else if (button & JOY_AXI)
        control_.SetButtons(CTRL_STATUS, joystick->GetAxisPress(button & JOY_VALUE));
    else
        control_.SetButtons(CTRL_STATUS, joystick->GetButtonPress(button & JOY_VALUE));

    if (!control_.IsButtonDown(CTRL_STATUS))
    {
        // UI : Prev Focus Panel
        button = buttonsMap_[ACTION_PREVPANEL];
        if (button & JOY_HAT)
            control_.SetButtons(CTRL_PREVPANEL, joystick->GetHatPosition(button & JOY_VALUE));
        else if (button & JOY_AXI)
            control_.SetButtons(CTRL_PREVPANEL, joystick->GetAxisPress(button & JOY_VALUE));
        else
            control_.SetButtons(CTRL_PREVPANEL, joystick->GetButtonPress(button & JOY_VALUE));

        // UI : Next Focus Panel
        if (!control_.IsButtonDown(CTRL_PREVPANEL))
        {
            button = buttonsMap_[ACTION_NEXTPANEL];
            if (button & JOY_HAT)
                control_.SetButtons(CTRL_NEXTPANEL, joystick->GetHatPosition(button & JOY_VALUE));
            else if (button & JOY_AXI)
                control_.SetButtons(CTRL_NEXTPANEL, joystick->GetAxisPress(button & JOY_VALUE));
            else
                control_.SetButtons(CTRL_NEXTPANEL, joystick->GetButtonPress(button & JOY_VALUE));
        }
    }

    // Prev Focus Entity
    button = buttonsMap_[ACTION_PREVENTITY];
    if (button & JOY_HAT)
        control_.SetButtons(CTRL_PREVENTITY, joystick->GetHatPosition(button & JOY_VALUE));
    else if (button & JOY_AXI)
        control_.SetButtons(CTRL_PREVENTITY, joystick->GetAxisPress(button & JOY_VALUE));
    else
        control_.SetButtons(CTRL_PREVENTITY, joystick->GetButtonPress(button & JOY_VALUE));

    // Next Focus Entity
    if (!control_.IsButtonDown(ACTION_PREVENTITY))
    {
        button = buttonsMap_[ACTION_NEXTENTITY];
        if (button & JOY_HAT)
            control_.SetButtons(CTRL_NEXTENTITY, joystick->GetHatPosition(button & JOY_VALUE));
        else if (button & JOY_AXI)
            control_.SetButtons(CTRL_NEXTENTITY, joystick->GetAxisPress(button & JOY_VALUE));
        else
            control_.SetButtons(CTRL_NEXTENTITY, joystick->GetButtonPress(button & JOY_VALUE));
    }

    // DIRECTION : UP, DOWN, LEFT, RIGHT
    // TODO : Joystick Binding for all gamecontroller
    // Here Hat & Axis are for PS4

//    state = input.GetScancodeDown(keysMap_[ACTION_UP]);
    state = false;
    if (!state && joystick->GetNumHats() > 0)
        state = (joystick->GetHatPosition(0) & HAT_UP) != 0;
    if (!state && joystick->GetNumAxes() >= 2)
        state = (joystick->GetAxisPosition(1) < -joyMoveDeadZone);
    if (!state && joystick->GetButtonDown(buttonsMap_[ACTION_UP]))
        state = true;
    control_.SetButtons(CTRL_UP, state);

//    state = input.GetScancodeDown(keysMap_[ACTION_DOWN]);
    state = false;
    if (!state && joystick->GetNumHats() > 0)
        state = (joystick->GetHatPosition(0) & HAT_DOWN) != 0;
    if (!state && joystick->GetNumAxes() >= 2)
        state = (joystick->GetAxisPosition(1) > joyMoveDeadZone);
    if (!state && joystick->GetButtonDown(buttonsMap_[ACTION_DOWN]))
        state = true;
    control_.SetButtons(CTRL_DOWN, state);

//    state = input.GetScancodeDown(keysMap_[ACTION_LEFT]);
    state = false;
    if (!state && joystick->GetNumHats() > 0)
        state = (joystick->GetHatPosition(0) & HAT_LEFT) != 0;
    if (!state && joystick->GetNumAxes() >= 2)
        state = (joystick->GetAxisPosition(0) < -joyMoveDeadZone);
    if (!state && joystick->GetButtonDown(buttonsMap_[ACTION_LEFT]))
        state = true;
    control_.SetButtons(CTRL_LEFT, state);

//    state = input.GetScancodeDown(keysMap_[ACTION_RIGHT]);
    state = false;
    if (!state && joystick->GetNumHats() > 0)
        state = (joystick->GetHatPosition(0) & HAT_RIGHT) != 0;
    if (!state && joystick->GetNumAxes() >= 2)
        state = (joystick->GetAxisPosition(0) > joyMoveDeadZone);
    if (!state && joystick->GetButtonDown(buttonsMap_[ACTION_RIGHT]))
        state = true;
    control_.SetButtons(CTRL_RIGHT, state);

//                if (joystick->GetNumAxes() >= 4)
//                {
//                    float lookX = joystick->GetAxisPosition(2);
//                    float lookY = joystick->GetAxisPosition(3);
//
//                    if (lookX < -joyLookDeadZone) control.yaw_ -= joySensitivity * lookX * lookX;
//                    if (lookX > joyLookDeadZone) control.yaw_ += joySensitivity * lookX * lookX;
//                    if (lookY < -joyLookDeadZone) control.pitch_ -= joySensitivity * lookY * lookY;
//                    if (lookY > joyLookDeadZone) control.pitch_ += joySensitivity * lookY * lookY;
//                }

//    control.yaw_ += mouseSensitivity * input.GetMouseMoveX();
//    control.pitch_ += mouseSensitivity * input.GetMouseMoveY();
//    control.pitch_ = Clamp(control.pitch_, -60.f, 60.f);
}


