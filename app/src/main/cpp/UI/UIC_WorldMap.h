#pragma once


using namespace Urho3D;

class UIC_MiniMap;

class UIC_WorldMap : public Component
{
    URHO3D_OBJECT(UIC_WorldMap, Component);

public :

    UIC_WorldMap(Context* context);
    virtual ~UIC_WorldMap();

    static void RegisterObject(Context* context);

    void AddActivatorButton(UIElement* element);
    void RemoveActivatorButton(const String& eltname, UIElement** element=0);
    void SetMiniMap(UIC_MiniMap* miniMap);

    bool IsVisible() const;
    void SetVisible(bool state);

    void Start();
    void Stop();
    void Clear();

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

    void Initialize();
    void Resize();

    Sprite* CreatePoint(UIElement* root, const Color& color, int size);

    void UpdateSnapShots();
    void UpdateSnapShotGrid();

    void OnSwitchVisible(StringHash eventType, VariantMap& eventData);
    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    void HandleWorldSnapShotChanged(StringHash eventType, VariantMap& eventData);
    void HandleWorldSnapShotClicked(StringHash eventType, VariantMap& eventData);
    void HandleWorldSnapShotHoverBegin(StringHash eventType, VariantMap& eventData);
    void HandleWorldSnapShotHoverEnd(StringHash eventType, VariantMap& eventData);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    WeakPtr<Window> uiElement_;
    WeakPtr<Button> canevas_;
    Vector<WeakPtr<UIElement> > activators_;
    Vector<Sprite*> uiPoints_;

    SharedPtr<Texture2D> worldMapTexture_;

    UIC_MiniMap* miniMap_;

    IntVector2 lastPosition_;

    float worldMapScale_;
    int worldMapMaxMaps_;
    int worldMapNumMaps_;
    bool worldMapDirty_, canevasHovering_;
    IntVector2 worldMapTopLeftMap_;
    Vector2 worldMapCenter_;
};
