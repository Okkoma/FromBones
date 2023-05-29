#pragma once

#include <Urho3D/Resource/Image.h>
#include <Urho3D/Graphics/Texture2D.h>

namespace Urho3D
{
class BorderImage;
class Sprite;
class Window;
class UIElement;
class Texture2D;
class Image;
}

using namespace Urho3D;

class GOC_Destroyer;
class UIC_WorldMap;

class UIC_MiniMap : public Component
{
    URHO3D_OBJECT(UIC_MiniMap, Component);

public :

    UIC_MiniMap(Context* context);
    virtual ~UIC_MiniMap();

    static void RegisterObject(Context* context);

    void SetNumTilesShown(const IntVector2& numtilesshown);
    void SetFollowedNode(Node* node);
    void AddNodeToDisplay(Node* node);

    void AddActivatorButton(UIElement* element);
    void RemoveActivatorButton(const String& eltname, UIElement** element=0);

    void SetWorldMap(UIC_WorldMap* worldMap = 0);

    void MarkPositionDirty();
    void MarkTextureDirty();
    void MarkEntitiesDirty();

    bool IsVisible() const;
    void SetVisible(bool state);

    void Start();
    void Stop();
    void Clear();

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

    void FillMapTexture(Texture2D* texture, int x, int y, void* data);
    void InitMapTexture();
    void Initialize();
    void Resize();
    void PopulatePoints();
    Sprite* CreatePoint(const Color& color, int size);

    void UpdateMapViewRect();
    bool UpdateMapImage();
    void UpdateMapTexture();
    void UpdateVisibleNodes();

    void OnSwitchVisible(StringHash eventType, VariantMap& eventData);

    void OnUpdate(StringHash eventType, VariantMap& eventData);
    void OnMapUpdate(StringHash eventType, VariantMap& eventData);
    void OnWorldUpdate(StringHash eventType, VariantMap& eventData);
    void OnWorldEntityUpdate(StringHash eventType, VariantMap& eventData);

    void HandleScreenResized(StringHash eventType, VariantMap& eventData);

    WeakPtr<Node> followedNode_;
    WeakPtr<GOC_Destroyer> destroyer_;

    bool minimapPositionDirty_;
    bool minimapTextureDirty_;
    bool minimapEntitiesDirty_;

    int mapWidth_, mapHeight_;
    IntVector2 numtilesshown_;
    Vector2 worldToUIRatio_;

    ShortIntVector2 mpointCenter_;
    Vector2 lastNodePosition_;

    IntRect minimapImageRect_;
    Rect minimapWorldRect_;

    WeakPtr<Window> uiElement_;
    Vector<WeakPtr<UIElement> > activators_;

    WeakPtr<BorderImage> mapLayer_;
    SharedPtr<Texture2D> mapTexture_;
    Sprite* nodePoint_;

    Image emptyImage_;

    Vector<WeakPtr<Node> > nodesToDisplay_;
    Vector<WeakPtr<Node> > visibleNodes_;
    Vector<ShortIntVector2> discoveredMaps_;
    Map* currentMaps_[9];

    IntVector2 lastPosition_;

    Vector<Sprite*> uiPoints_;

    UIC_WorldMap* worldMap_;
};
