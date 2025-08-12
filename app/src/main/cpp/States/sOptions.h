#pragma once

#include "GameStateManager.h"

namespace Urho3D
{
class UIElement;
class Sprite;
}

using namespace Urho3D;

class OptionState : public GameState
{
    URHO3D_OBJECT(OptionState, GameState);

public:

    OptionState(Context* context);
    ~OptionState();

    static void RegisterObject(Context* context);

    virtual bool Initialize();
    virtual void Begin();
    virtual void End();

    void SetVisibleAccessButton(bool state);
    void OpenQuitMessage();
    void OpenFrame();
    void CloseFrame();
    void ToggleFrame();
    void Hide();

    void ResetToMainCategory();

    bool IsVisible() const;

protected:

    void LoadJSONServerList();
    void GetNetServerParams(int selection, String& serverip, int& serverport);
    int GetNetServerIndex(const String& serverip, int serverport);

    void SetDefaultWorldParameters();

    bool CreateUI();
    void RemoveUI();
    void SwitchToCategory(const String& category);

    void SynchronizeParameters();
    bool IsFullscreenResolution(unsigned monitor, const IntVector2& resolution) const;    
    void CheckParametersChanged();
    void ApplyParameters();
    void ApplyWorldChange();
    void ApplyNumPlayers();
    void ApplyLanguage();
    void ApplyEnlightment();
    void ApplyDebugRttScene();

    void ResetToCurrentConfigControlKeys();
    void ResetToDefaultConfigControlKeys(int configid);
    void ResetToCurrentConfigControlKeys(int configid);
    void ApplyConfigControlKeys(int configid);
    void UpdateConfigControls(int configid);

    void SubscribeToEvents();
    void UnsubscribeToEvents();

    void UpdateSnapShots(bool highres=false);
    void UpdateSnapShotGrid();

    void HandleClickCategory(StringHash eventType, VariantMap& eventData);
    void HandleClickConfigControl(StringHash eventType, VariantMap& eventData);
    void HandleResetConfigControl(StringHash eventType, VariantMap& eventData);
    void HandleApplyConfigControl(StringHash eventType, VariantMap& eventData);
    void HandleClickConfigActionButton(StringHash eventType, VariantMap& eventData);
    void HandleCaptureKey(StringHash eventType, VariantMap& eventData);
    void HandleCaptureJoy(StringHash eventType, VariantMap& eventData);

    void HandleApplyParameters(StringHash eventType, VariantMap& eventData);
    void HandleCloseFrame(StringHash eventType, VariantMap& eventData);

    // Game Category Handle
    void HandleLoad(StringHash eventType, VariantMap& eventData);
    void HandleSave(StringHash eventType, VariantMap& eventData);
    void HandleQuit(StringHash eventType, VariantMap& eventData);

    // World Category Handle
    void HandleWorldNameChanged(StringHash eventType, VariantMap& eventData);
    void HandleWorldModelChanged(StringHash eventType, VariantMap& eventData);
    void HandleWorldSizeChanged(StringHash eventType, VariantMap& eventData);
    void HandleWorldSeedChanged(StringHash eventType, VariantMap& eventData);
    void HandleWorldSnapShotChanged(StringHash eventType, VariantMap& eventData);
    void HandleWorldSnapShotClicked(StringHash eventType, VariantMap& eventData);
    void HandleGenerateWorldMapButton(StringHash eventType, VariantMap& eventData);

    // Player Category Handle
    void HandleMultiViewsChanged(StringHash eventType, VariantMap& eventData);
    void HandleNumPlayersChanged(StringHash eventType, VariantMap& eventData);
    void HandleControlP1Changed(StringHash eventType, VariantMap& eventData);
    void HandleControlP2Changed(StringHash eventType, VariantMap& eventData);
    void HandleControlP3Changed(StringHash eventType, VariantMap& eventData);
    void HandleControlP4Changed(StringHash eventType, VariantMap& eventData);
    void HandleLanguageChanged(StringHash eventType, VariantMap& eventData);
    void HandleReactivityChanged(StringHash eventType, VariantMap& eventData);

    // Graphics Category Handle
    void HandleResolutionFullScreenChanged(StringHash eventType, VariantMap& eventData);
    void HandleTextureQualityChanged(StringHash eventType, VariantMap& eventData);
    void HandleTextureFilterChanged(StringHash eventType, VariantMap& eventData);
    void HandleUndergroundChanged(StringHash eventType, VariantMap& eventData);

    // Sound Category Handle
    void HandleMusicChanged(StringHash eventType, VariantMap& eventData);
    void HandleSoundChanged(StringHash eventType, VariantMap& eventData);

    // Network Category Handle
    void HandleNetworkMode(StringHash eventType, VariantMap& eventData);
    void HandleNetworkServer(StringHash eventType, VariantMap& eventData);

    // Debug Category Handle
    void HandleFluidEnableChanged(StringHash eventType, VariantMap& eventData);
    void HandleRenderShapesChanged(StringHash eventType, VariantMap& eventData);
    void HandleRenderDebugChanged(StringHash eventType, VariantMap& eventData);
    void HandleDebugPhysicsChanged(StringHash eventType, VariantMap& eventData);
    void HandleDebugWorldChanged(StringHash eventType, VariantMap& eventData);
    void HandleDebugRenderShapesChanged(StringHash eventType, VariantMap& eventData);
    void HandleDebugRttSceneChanged(StringHash eventType, VariantMap& eventData);
    void HandleResetMaps(StringHash eventType, VariantMap& eventData);
    void HandleSaveMapFeatures(StringHash eventType, VariantMap& eventData);

    void HandleQuitMessageAck(StringHash eventType, VariantMap& eventData);
    void HandleMenuButton(StringHash eventType, VariantMap& eventData);
    void HandleKeyEscape(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    void OnPostRenderUpdate(StringHash eventType, VariantMap& eventData);

    bool snapshotdirty_;

    String category_;
};
