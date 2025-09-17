#pragma once


using namespace Urho3D;

#include "DefsCore.h"

class FROMBONES_API GameCommands : public Object
{
    URHO3D_OBJECT(GameCommands, Object);

public:
    GameCommands(Context* context);
    virtual ~GameCommands();

    static GameCommands& Get();

    static void Launch(const String& input);
    
    static void Show();
    static void Hide();
    static void Toggle();
    static bool IsVisible();

    static void Stop();
private:
    void SubscribeToEvents();

    void HandleCloseFrame(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    static GameCommands* gameCommands_;

    WeakPtr<UIElement> uiCommandsFrame_;
};
