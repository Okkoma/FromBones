#pragma once

#include "Actor.h"

namespace Urho3D
{
class Scene;
class Font;
class BorderImage;
class Button;
class Sprite;
class Text;
class UIElement;
class Light;
class Timer;
}

struct WorldMapPosition;
class Equipment;
class UIPanel;
class MissionManager;

class GOC_Inventory;
class GOC_Collectable;
class GOC_Life;
class GOC_Controller;
class GOC_Destroyer;

using namespace Urho3D;

//
//struct LosTemplate
//{
//    LosTempate();
//
//    char numVerticalLines_;
//    int centerX_;
//    int centerY_;
//    int* spanData_;
//    int* lines_;
//};

enum Panels
{
    STATUSPANEL = 0,
    BAGPANEL,
    EQUIPMENTPANEL,
    CRAFTPANEL,
    ABILITYPANEL,
    DIALOGUEPANEL,
    SHOPPANEL,
    JOURNALPANEL,
    MISSIONPANEL,
    MINIMAPPANEL
};

class Player : public Actor
{
    URHO3D_OBJECT(Player, Actor);

public :
    Player(unsigned id);
    Player(Context* context, unsigned id);
    ~Player();

    void Reset(Context* context=0, unsigned id=0);

//    void SetPlayerID(unsigned id);
    void SetPlayerName(const char *n);
    void SetMissionEnable(bool enable);
    void Set(UIElement* elem, bool missionEnable, bool multiLocalPlayerMode, unsigned id=0, const char *n=0);
    void SetScene(Scene* scene, const Vector2& position, int viewZ, bool loadmode, bool initmode, bool restartmode, bool forceposition=false);
    void SetFaction(unsigned faction);
    void SetDirty()
    {
        dirtyPlayer_ = true;
    }
    void SetMissionWin();
    void SetWorldPosition(const WorldMapPosition& position);

    void InitAvatar(Scene* root, const Vector2& position, int viewZ);
    void ChangeAvatar(unsigned type, unsigned char entityid, bool instant=false);
    void ChangeAvatar(int avatarIndex, bool instant=false);
    void SwitchViewZ(int viewZ=0);
    void MountOn(Node* target);
    void Unmount();

    bool AddAvatar(const StringHash& got);
    bool AddAvatar(int avatarIndex);
    int GetAvatarIndex() const
    {
        return avatarIndex_;
    }
    int GetViewZ() const;
    const WorldMapPosition& GetWorldMapPosition() const;
    const PODVector<Light*>& GetLights() const;
    unsigned GetFaction() const
    {
        return faction_;
    }
    const Vector<int>& GetAvatars()
    {
        return avatars_;
    }

    void LoadAvatar(Scene* root);
    void LoadStuffOnly(bool initialstuff=false);
    void LoadState();
    void LoadStateFrom(Node* node);
    void SaveState();
    void SaveAll();

    void RemoveUI();
    void ResizeUI();
    void SetVisibleUI(bool state, bool all=false);
    void UpdatePoints(const unsigned& points);
    void NextPanelFocus();
    void DebugDrawUI();

    void UpdateAvatar(bool forced=false);
    void UpdateControls(bool restartcontroller=false);

    void UseWeaponAbilityAt(const StringHash& abi, const Vector2& position);

    unsigned score;
    bool active;

    GOC_Life* gocLife;
    GOC_Controller* gocController;
    GOC_Inventory* gocInventory;
    GOC_Destroyer* gocDestroyer_;

    MissionManager* missionManager_;
    Equipment* equipment_;

    void Dump() const;

    void Start();
    void Stop();

    virtual void StartSubscribers();
    virtual void StopSubscribers();

    void DrawDebugGeometry(DebugRenderer* debugRenderer) const;

protected :

    void OnPostUpdate(StringHash eventType, VariantMap& eventData);

    void OnFire1(StringHash eventType, VariantMap& eventData);
    void OnFire2(StringHash eventType, VariantMap& eventData);
    void OnFire3(StringHash eventType, VariantMap& eventData);
    void OnStatus(StringHash eventType, VariantMap& eventData);
    void OnCollideWall(StringHash eventType, VariantMap& eventData);
    void OnAvatarDestroy(StringHash eventType, VariantMap& eventData);
    void OnInventoryEmpty(StringHash eventType, VariantMap& eventData);
    void OnGetCollectable(StringHash eventType, VariantMap& eventData);
    void OnStartTransferCollectable(StringHash eventlights_Type, VariantMap& eventData);
    void OnEntitySelection(StringHash eventType, VariantMap& eventData);
    void OnMountControlUpdate(StringHash eventType, VariantMap& eventData);

    virtual void OnReceiveCollectable(StringHash eventType, VariantMap& eventData);
    virtual void OnDropCollectable(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkBegin(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkQuestion(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkShowResponses(StringHash eventType, VariantMap& eventData);
    void OnTalkResponseSelected(StringHash eventType, VariantMap& eventData);
    virtual void OnTalkEnd(StringHash eventType, VariantMap& eventData);
    void OnBargain(StringHash eventType, VariantMap& eventData);
    virtual void OnDead(StringHash eventType, VariantMap& eventData);

private :
    void CreateUI(UIElement* root, bool multiLocalPlayerMode);
    void ResetUI();
    void UpdateUI();

    void TransferCollectableToUI(const Variant& varpos, UIElement* dest, unsigned idslot, unsigned qty, const ResourceRef& collectable);

    virtual void UpdateComponents();

    void UpdateTriggerAttacks();
    void UpdateZones();
    void UpdateNativeAbilities();
    void UpdateLineOfSight();

    void HandleDelayedActions(StringHash eventType, VariantMap& eventData);
    void HandleClic(StringHash eventType, VariantMap& eventData);
    void HandleUpdateTimePeriod(StringHash eventType, VariantMap& eventData);

    WeakPtr<UIElement> uiStatusBar;
    WeakPtr<Text> scoreText;

//    LosTemplate* losTemplate_;
    unsigned lastInventoryUpdate_;
    float lastTransferDelay_;
    float lastTime_;

    bool dirtyPlayer_;
    bool missionEnable_;
    IntVector3 zone_;

    unsigned faction_;
    // index avatar
    int avatarIndex_, avatarAccumulatorIndex_;
    int lastxonui_, lastyonui_;

    Vector<int> avatars_;
    PODVector<Light*> lights_;
};
