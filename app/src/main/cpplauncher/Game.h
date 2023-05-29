#pragma once

#include <Urho3D/Engine/Application.h>


using namespace Urho3D;

class GameConfig;

class Game : public Application
{
    URHO3D_OBJECT(Game, Application);

public:
    Game(Context* context);
    virtual ~Game();

    virtual void Setup();
    virtual void Start();
    virtual void Stop();

    static Game* Get()
    {
        return game_;
    }
    static VariantMap& GetEngineParameters()
    {
        return game_->engineParameters_;
    }

private:
    String LoadGameConfig(const String& fileName, GameConfig* config);

    void SetupDirectories();
    void SetupControllers();
    void SetupSubSystems();

    void HandlePreloadResources(StringHash eventType, VariantMap& eventData);
    void HandleAsynchronousUpdate(StringHash eventType, VariantMap& eventData);

    void HandleTouchBegin(StringHash eventType, VariantMap& eventData);
    void HandleKeyDownHUD(StringHash eventType, VariantMap& eventData);

    void HandleWindowResize(StringHash eventType, VariantMap& eventData);

    void HandleBeginRendering(StringHash eventType, VariantMap& eventData);

    void HandleJoystickChange(StringHash eventType, VariantMap& eventData);
    void HandleConsoleCommand(StringHash eventType, VariantMap& eventData);

    static Game* game_;
};
