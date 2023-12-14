#pragma once

#include <Urho3D/Input/Input.h>

#include "GOC_Controller.h"

const float mouseSensitivity = 0.125f;
const float touchSensitivity = 2.0f;
const float joySensitivity = 0.5f;
const float joyMoveDeadZone = 0.333f;
const float joyLookDeadZone = 0.05f;

using namespace Urho3D;


class GOC_PlayerController : public GOC_Controller
{
    URHO3D_OBJECT(GOC_PlayerController, GOC_Controller);

public :
    GOC_PlayerController(Context* context);
    virtual ~GOC_PlayerController();
    static void RegisterObject(Context* context);

    virtual void Start();
    virtual void Stop();

    virtual void MountOn(Node* node);
    virtual void Unmount();

    void SetKeyControls(int controlID);
    void SetJoystickControls(int controlID);
    void SetTouchControls(int screenJoyId=-1);

    void InitTouchInput();

    unsigned playerID;

    bool touchEnabled;
    int screenJoystickID;
    int screenJoystickSettingsID;

private :
//        void HandleNetworkUpdateSent(StringHash eventType, VariantMap& eventData);
    void HandleNetworkUpdate(StringHash eventType, VariantMap& eventData);
    void HandleStop(StringHash eventType, VariantMap& eventData);
    void HandleLocalUpdate(StringHash eventType, VariantMap& eventData);

    inline void UpdateKeyControls(Input& input);
    inline void UpdateTouchControls(Input& input);
    inline void UpdateJoystickControls(Input& input);

    int joystickindex_;
    int* keysMap_;
    int* buttonsMap_;
};



