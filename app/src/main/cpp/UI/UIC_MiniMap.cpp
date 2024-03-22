#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Window.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>

#include "GameEvents.h"
#include "GameAttributes.h"
#include "GameOptionsTest.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "GOC_Controller.h"
#include "GOC_Destroyer.h"

#include "Map.h"
#include "MapWorld.h"
#include "MapStorage.h"
#include "ViewManager.h"
#include "TimerRemover.h"

#include "UIC_WorldMap.h"
#include "UIC_MiniMap.h"


#define MINIMAP_BORDERSIZE 20
#define MINIMAP_MAXNUMPOINTS 20

#define ACTIVE_MINIMAP_MANAGEPOINTS


int minimapInitialUiElts_;
WorldMapPosition miniMapWorldPosition_;


UIC_MiniMap::UIC_MiniMap(Context* context) :
    Component(context),
    mapWidth_(0),
    mapHeight_(0),
    numtilesshown_(IntVector2(48, 48)),
    emptyImage_(context),
    worldMap_(0)
{ }

UIC_MiniMap::~UIC_MiniMap()
{
    Clear();
}

void UIC_MiniMap::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_MiniMap>();
}

// View Size of the map (tiles shown)
void UIC_MiniMap::SetNumTilesShown(const IntVector2& numtilesshown)
{
    numtilesshown_ = numtilesshown;
}

void UIC_MiniMap::SetFollowedNode(Node* node)
{
    followedNode_ = node;
    destroyer_ = node->GetComponent<GOC_Destroyer>();

    URHO3D_LOGINFOF("UIC_MiniMap() - SetFollowedNode : node=%s(%u) !", node->GetName().CString(), node->GetID());
}

void UIC_MiniMap::AddNodeToDisplay(Node* node)
{
    WeakPtr<Node> ptr = WeakPtr<Node>(node);
    if (!nodesToDisplay_.Contains(ptr))
    {
        nodesToDisplay_.Push(ptr);
        URHO3D_LOGINFOF("UIC_MiniMap() - AddNodeToDisplay : node=%s(%u) !", node->GetName().CString(), node->GetID());
    }
}

void UIC_MiniMap::AddActivatorButton(UIElement* element)
{
    if (element)
    {
        activators_.Push(WeakPtr<UIElement>(element));
        SubscribeToEvent(element, E_RELEASED, URHO3D_HANDLER(UIC_MiniMap, OnSwitchVisible));
    }
}

void UIC_MiniMap::RemoveActivatorButton(const String& eltname, UIElement** element)
{
    for (unsigned i=0; i < activators_.Size(); i++)
    {
        if (activators_[i] && activators_[i]->GetName() == eltname)
        {
            if (element)
                *element = activators_[i].Get();

            UnsubscribeFromEvent(activators_[i].Get(), E_RELEASED);

            activators_.EraseSwap(i);
        }
    }
}

void UIC_MiniMap::SetWorldMap(UIC_WorldMap* worldMap)
{
    if (!World2D::GetWorldInfo()->worldModel_)
        return;

    // Set or Create WorldMap
    worldMap_ = worldMap ? worldMap : new UIC_WorldMap(context_);

    // Add Button
    Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
    if (!switchMapButton)
        switchMapButton = uiElement_->CreateChild<Button>();

    switchMapButton->SetName("SwitchMapButton");
    switchMapButton->SetStyle("OptionButton");
    switchMapButton->SetAlignment(HA_RIGHT, VA_TOP);
    switchMapButton->SetFocusMode(FM_RESETFOCUS);
    worldMap_->AddActivatorButton(switchMapButton);

    Text* text = switchMapButton->CreateChild<Text>();
    text->SetColor(Color(0.85f, 0.85f, 0.85f));
    text->SetFont(GameContext::Get().uiFonts_[UIFONTS_ABY12], 12);
    text->SetAlignment(HA_CENTER, VA_CENTER);
    text->SetText("W");

    worldMap_->SetMiniMap(this);
}

bool UIC_MiniMap::IsVisible() const
{
    return uiElement_ ? uiElement_->IsVisible() : false;
}

void UIC_MiniMap::SetVisible(bool state)
{
    if (state != IsVisible())
    {
        if (state)
            Start();
        else
            Stop();
    }
}

void UIC_MiniMap::MarkPositionDirty()
{
    minimapPositionDirty_ = true;
}

void UIC_MiniMap::MarkTextureDirty()
{
    minimapTextureDirty_ = true;
}

void UIC_MiniMap::MarkEntitiesDirty()
{
    minimapEntitiesDirty_ = true;
}

void UIC_MiniMap::Start()
{
    Initialize();
    Resize();

    if (uiElement_)
    {
        URHO3D_LOGINFOF("UIC_MiniMap() - Start : gocptr=%u on node=%s(%u) ... followedNode=%s(%u) !",
                        this, node_->GetName().CString(), node_->GetID(), followedNode_ ? followedNode_->GetName().CString() : "none", followedNode_ ? followedNode_->GetID() : 0);

        PopulatePoints();

        MarkPositionDirty();
        MarkTextureDirty();

        uiElement_->SetVisible(true);
        uiElement_->SetEnabled(true);

        int width = GetSubsystem<UI>()->GetRoot()->GetSize().x_;

        GameHelpers::SetMoveAnimationUI(uiElement_, IntVector2(lastPosition_.x_ < width / 2 ? -uiElement_->GetSize().x_ : width, lastPosition_.y_), lastPosition_, 0.f, SWITCHSCREENTIME);

        SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(UIC_MiniMap, OnUpdate));
        SubscribeToEvent(MAP_UPDATE, URHO3D_HANDLER(UIC_MiniMap, OnMapUpdate));
        SubscribeToEvent(WORLD_MAPUPDATE, URHO3D_HANDLER(UIC_MiniMap, OnWorldUpdate));
        SubscribeToEvent(WORLD_ENTITYUPDATE, URHO3D_HANDLER(UIC_MiniMap, OnWorldEntityUpdate));

        SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(UIC_MiniMap, HandleScreenResized));

        if (worldMap_)
        {
            Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
            SubscribeToEvent(switchMapButton, E_RELEASED, URHO3D_HANDLER(UIC_MiniMap, OnSwitchVisible));
        }
    }
    else
    {
        URHO3D_LOGERRORF("UIC_MiniMap() - Start : node=%s(%u) gocptr=%u NO UI !", node_->GetName().CString(), node_->GetID(), this);
    }

    for (unsigned i = 0; i < activators_.Size(); i++)
    {
        if (activators_[i])
            SubscribeToEvent(activators_[i].Get(), E_RELEASED, URHO3D_HANDLER(UIC_MiniMap, OnSwitchVisible));
    }
}

void UIC_MiniMap::Stop()
{
    URHO3D_LOGINFO("UIC_MiniMap() - Stop !");

    UnsubscribeFromAllEvents();

    if (uiElement_)
    {
        int width = GetSubsystem<UI>()->GetRoot()->GetSize().x_;

        lastPosition_ = uiElement_->GetPosition();
        GameHelpers::SetMoveAnimationUI(uiElement_, lastPosition_, IntVector2(lastPosition_.x_ < width / 2 ? -uiElement_->GetSize().x_ : width, lastPosition_.y_), 0.f, SWITCHSCREENTIME);
        TimerRemover::Get()->Start(uiElement_, SWITCHSCREENTIME + 0.5f, DISABLE);

        if (worldMap_)
        {
            Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
            UnsubscribeFromEvent(switchMapButton, E_RELEASED);
        }
    }

    for (unsigned i = 0; i < activators_.Size(); i++)
    {
        if (activators_[i])
            SubscribeToEvent(activators_[i].Get(), E_RELEASED, URHO3D_HANDLER(UIC_MiniMap, OnSwitchVisible));
    }
}

void UIC_MiniMap::Clear()
{
    URHO3D_LOGINFO("UIC_MiniMap() - Clear !");

    Stop();

    MarkPositionDirty();
    MarkTextureDirty();

    if (worldMap_)
        delete worldMap_;

    worldMap_ = 0;

    mapWidth_ = 0;
    mapHeight_ = 0;

    mapLayer_.Reset();
    mapTexture_.Reset();

    nodesToDisplay_.Clear();

    if (uiElement_)
        uiElement_->Remove();

    uiPoints_.Clear();
}

void UIC_MiniMap::OnNodeSet(Node* node)
{
    if (node)
    {
        URHO3D_LOGINFO("UIC_MiniMap() - OnNodeSet ...");

        SetFollowedNode(node);
        // no serialize
        SetTemporary(true);

        URHO3D_LOGINFO("UIC_MiniMap() - OnNodeSet ... OK !");
    }
    else
    {
        Clear();
    }
}

void UIC_MiniMap::OnSetEnabled()
{
    if (IsEnabledEffective())
    {
        Start();
    }
    else
    {
        Stop();

        if (worldMap_)
            worldMap_->Stop();
    }
}

void UIC_MiniMap::FillMapTexture(Texture2D* texture, int x, int y, void* data)
{
    texture->SetData(0, x * mapWidth_ * MINIMAP_PIXBYTILE, y * mapHeight_ * MINIMAP_PIXBYTILE, mapWidth_ * MINIMAP_PIXBYTILE, mapHeight_ * MINIMAP_PIXBYTILE, data);
}

Sprite* UIC_MiniMap::CreatePoint(const Color& color, int size)
{
    Sprite* uipoint = mapLayer_->CreateChild<Sprite>();
    uipoint->SetName("MinimapPoint");

//    GameHelpers::SetUIElementFrom(uipoint, UIMAIN, "UIPoint");
    GameHelpers::SetUIElementFrom(uipoint, UIEQUIPMENT, "point");

    uipoint->SetUseDerivedOpacity(false);
    uipoint->SetColor(color);
    uipoint->SetOpacity(0.8f);
    uipoint->SetSize(size, size);
    uipoint->SetHotSpot(size/2, size/2);
    uipoint->SetAlignment(HA_CENTER, VA_CENTER);

    return uipoint;
}

void UIC_MiniMap::Initialize()
{
    if (uiElement_)
        return;

    URHO3D_LOGINFO("UIC_MiniMap() - Initialize ...");

    UI* ui = GetSubsystem<UI>();
    uiElement_ = dynamic_cast<Window*>(ui->GetRoot()->GetChild(String("MiniMap")));

    // Create UI Element
    SharedPtr<Window> uielt(new Window(context_));
    ui->GetRoot()->AddChild(uielt);
    uiElement_ = uielt;
    uiElement_->SetName("MiniMap");
    uiElement_->SetVar(GOA::OWNERID, GetNode()->GetID());

    // Set Window
//    GameHelpers::SetUIElementFrom(uiElement_.Get(), UIMAIN, "UIWindowFrame");
    GameHelpers::SetUIElementFrom(uiElement_.Get(), UIEQUIPMENT, "windowframe1");
    uiElement_->SetUseDerivedOpacity(false);
    uiElement_->SetOpacity(GameContext::Get().gameConfig_.uiMapOpacityBack_);
    uiElement_->SetMovable(true);
    uiElement_->SetResizable(false);
    uiElement_->SetBorder(IntRect(4, 4, 4, 4));
    uiElement_->SetResizeBorder(IntRect(8, 8, 8, 8));

    // Map layers initialize
    mapTexture_ = SharedPtr<Texture2D>(new Texture2D(context_));
    mapTexture_->SetNumLevels(1);
    mapTexture_->SetFilterMode(FILTER_BILINEAR);
    mapLayer_ = uiElement_->CreateChild<BorderImage>();
    mapLayer_->SetName("MinimapLayer");
    mapLayer_->SetUseDerivedOpacity(false);
    mapLayer_->SetOpacity(GameContext::Get().gameConfig_.uiMapOpacityFrame_);
    mapLayer_->SetAlignment(HA_CENTER, VA_CENTER);
    mapLayer_->SetTexture(mapTexture_);

    // Create Points
    uiPoints_.Clear();
    uiPoints_.Push(CreatePoint(Color::YELLOW, 10));

#ifdef ACTIVE_MINIMAP_MANAGEPOINTS
    for (unsigned i=1; i<MINIMAP_MAXNUMPOINTS; i++)
        uiPoints_.Push(CreatePoint(Color::RED, 8));
#endif

    SetWorldMap();

    URHO3D_LOGINFO("UIC_MiniMap() - Initialize ... OK !");
}

void UIC_MiniMap::Resize()
{
    if (!uiElement_)
        return;

    const int uiwidth  = GameContext::Get().ui_->GetRoot()->GetSize().x_;
    const int uiheight = GameContext::Get().ui_->GetRoot()->GetSize().y_;

    int size = Clamp((int)floor(200.f * GameContext::Get().uiScale_), 100, Max(100, uiheight / 3));

    URHO3D_LOGINFOF("UIC_MiniMap() - Resize ... size=%d uiscale=%f", size, GameContext::Get().uiScale_);

    uiElement_->SetSize(size, size);
    uiElement_->SetPosition(uiwidth-size, 0.f);
    lastPosition_ = uiElement_->GetPosition();

    if (mapWidth_ != World2D::GetWorldInfo()->mapWidth_ || mapHeight_ != World2D::GetWorldInfo()->mapHeight_)
    {
        mapWidth_ = World2D::GetWorldInfo()->mapWidth_;
        mapHeight_ = World2D::GetWorldInfo()->mapHeight_;
    }

    SetNumTilesShown(IntVector2(mapWidth_-6, mapHeight_-6));

    worldToUIRatio_.x_ = (float)(size-MINIMAP_BORDERSIZE) / (numtilesshown_.x_ * World2D::GetWorldInfo()->mTileWidth_);
    worldToUIRatio_.y_ = (float)(size-MINIMAP_BORDERSIZE) / (numtilesshown_.y_ * World2D::GetWorldInfo()->mTileHeight_);

    // todo clamp size if > 1024pix in width or in height
    // map size must be less than 340 (1024/3)
    assert(mapWidth_ < 1024/MINIMAP_PIXBYTILE && mapHeight_ < 1024/MINIMAP_PIXBYTILE);

    // texture of 3 maps squared
    int textureSize = 3 * MINIMAP_PIXBYTILE * Max(mapWidth_, mapHeight_);

    // Use a Texture RenderTarget and not a Texture Static (otherwise don't show the texture on Android)
    if (!mapTexture_->SetSize(textureSize, textureSize, Graphics::GetRGBAFormat(), TEXTURE_DYNAMIC, 0))
        URHO3D_LOGWARNING("UIC_MiniMap() - Initialize : Texture SetSize Error !");

    // Set Empty Image
    emptyImage_.SetSize(mapWidth_ * MINIMAP_PIXBYTILE, mapHeight_ * MINIMAP_PIXBYTILE, 4);
    emptyImage_.Clear(Color::TRANSPARENT);

    // Fill with EmptyMap
    for (int j = 0; j < 3 ; ++j)
        for (int i = 0; i < 3 ; ++i)
            FillMapTexture(mapTexture_, i, j, emptyImage_.GetData());

    int layersize = size-MINIMAP_BORDERSIZE;
    mapLayer_->SetSize(layersize, layersize);

    size = 32;
    Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
    if (switchMapButton)
    {
        switchMapButton->SetSize(size, size);
    }

    // Update points scale
    //size = 20 * GameContext::Get().uiScale_;
    size = 20;
    uiPoints_[0]->SetSize(size, size);
    uiPoints_[0]->SetHotSpot(size/2, size/2);
    uiPoints_[0]->SetColor(Color::YELLOW);

    //size = 16 * GameContext::Get().uiScale_;
    size = 16;
    for (unsigned i=1; i<MINIMAP_MAXNUMPOINTS; i++)
    {
        uiPoints_[i]->SetSize(size, size);
        uiPoints_[i]->SetHotSpot(size/2, size/2);
    }

    URHO3D_LOGINFO("UIC_MiniMap() - Resize ... OK !");
}

void UIC_MiniMap::PopulatePoints()
{
#ifdef ACTIVE_MINIMAP_MANAGEPOINTS
//    URHO3D_LOGINFO("UIC_MiniMap() - PopulatePoints ... ");

    nodesToDisplay_.Clear();

    PODVector<Node*> entities;
    World2D::GetVisibleEntities(entities);
    for (unsigned i=0; i < entities.Size(); i++)
    {
        Node* node = entities[i];

        if (node == followedNode_)
            continue;

        GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
        if (!controller)
            continue;

        if (controller->GetControllerType() & GO_Entity)
            nodesToDisplay_.Push(WeakPtr<Node>(node));
    }

//    URHO3D_LOGINFOF("UIC_MiniMap() - PopulatePoints ... numEntities=%u ... OK !", nodesToDisplay_.Size());
#endif

    minimapEntitiesDirty_ = false;
}

void UIC_MiniMap::UpdateMapViewRect()
{
    WorldMapPosition& wmposition = destroyer_ ? destroyer_->GetWorldMapPosition() : miniMapWorldPosition_;
    lastNodePosition_ = wmposition.position_;

    if (!destroyer_ && followedNode_)
    {
        miniMapWorldPosition_.position_ = followedNode_->GetWorldPosition2D();
        World2D::GetWorldInfo()->Convert2WorldMapPosition(miniMapWorldPosition_.position_, miniMapWorldPosition_, miniMapWorldPosition_.positionInTile_);
    }

    if (minimapPositionDirty_)
    {
        minimapPositionDirty_ = false;

        if (mpointCenter_ != wmposition.mPoint_)
        {
            mpointCenter_ = wmposition.mPoint_;

            MarkTextureDirty();
        }

        // update the currentmaps
        for (int i=0; i < 9; i++)
        {
            const HashMap<ShortIntVector2, Map* >& loadedmaps = World2D::GetStorage()->GetMapsInMemory();
            HashMap<ShortIntVector2, Map*>::ConstIterator it = loadedmaps.Find(ShortIntVector2(mpointCenter_.x_ + i%3 - 1, mpointCenter_.y_ + i/3 - 1));
            currentMaps_[i] = it != loadedmaps.End() ? it->second_ : 0;
        }
    }

    minimapWorldRect_.min_.x_ = wmposition.position_.x_ - 0.5f * (float)numtilesshown_.x_ * World2D::GetWorldInfo()->mTileWidth_;
    minimapWorldRect_.min_.y_ = wmposition.position_.y_ - 0.5f * (float)numtilesshown_.y_ * World2D::GetWorldInfo()->mTileHeight_;
    minimapWorldRect_.max_.x_ = wmposition.position_.x_ + 0.5f * (float)numtilesshown_.x_ * World2D::GetWorldInfo()->mTileWidth_;
    minimapWorldRect_.max_.y_ = wmposition.position_.y_ + 0.5f * (float)numtilesshown_.y_ * World2D::GetWorldInfo()->mTileHeight_;

    IntVector2 minimapCenterTile(mapWidth_ + wmposition.mPosition_.x_, mapHeight_ + wmposition.mPosition_.y_);
    minimapImageRect_.left_   = (minimapCenterTile.x_ - numtilesshown_.x_/2) * MINIMAP_PIXBYTILE;
    minimapImageRect_.top_    = (minimapCenterTile.y_ - numtilesshown_.y_/2) * MINIMAP_PIXBYTILE;
    minimapImageRect_.right_  = (minimapCenterTile.x_ + numtilesshown_.x_/2) * MINIMAP_PIXBYTILE;
    minimapImageRect_.bottom_ = (minimapCenterTile.y_ + numtilesshown_.y_/2) * MINIMAP_PIXBYTILE;
    // increase precision with positionInTile
    minimapImageRect_.left_   += (int)(MINIMAP_PIXBYTILE * wmposition.positionInTile_.x_);
    minimapImageRect_.top_    += Max(0, (int)(MINIMAP_PIXBYTILE * (1.f - wmposition.positionInTile_.y_)));
    minimapImageRect_.right_  += (int)(MINIMAP_PIXBYTILE * wmposition.positionInTile_.x_);
    minimapImageRect_.bottom_ += Max(0, (int)(MINIMAP_PIXBYTILE * (1.f - wmposition.positionInTile_.y_)));

//    minimapImageRect_.left_   = (mapWidth_ + wmposition.mPosition_.x_ - numtilesshown_.x_/2) * MINIMAP_PIXBYTILE;
//    minimapImageRect_.left_   += (int)(MINIMAP_PIXBYTILE * wmposition.positionInTile_.x_);
//    minimapImageRect_.top_    = (mapHeight_ + wmposition.mPosition_.y_ - numtilesshown_.y_/2) * MINIMAP_PIXBYTILE;
//    minimapImageRect_.top_    += Max(0, (int)(MINIMAP_PIXBYTILE * (1.f - wmposition.positionInTile_.y_)));
//    minimapImageRect_.right_  = (mapWidth_ + wmposition.mPosition_.x_ + numtilesshown_.x_/2) * MINIMAP_PIXBYTILE;
//    minimapImageRect_.right_  += (int)(MINIMAP_PIXBYTILE * wmposition.positionInTile_.x_);
//    minimapImageRect_.bottom_ = (mapHeight_ + wmposition.mPosition_.y_ + numtilesshown_.y_/2) * MINIMAP_PIXBYTILE;
//    minimapImageRect_.bottom_ += Max(0, (int)(MINIMAP_PIXBYTILE * (1.f - wmposition.positionInTile_.y_)));

//    URHO3D_LOGERRORF("UIC_MiniMap() - UpdateMapViewRect : position=%s mpoint=%s mpointcenter=%s imgrect=%s posintile=%s!",
//                     wmposition.position_.ToString().CString(), wmposition.mPoint_.ToString().CString(), mpointCenter_.ToString().CString(),
//                     minimapImageRect_.ToString().CString(), wmposition.positionInTile_.ToString().CString());

    mapLayer_->SetImageRect(minimapImageRect_);
}

void UIC_MiniMap::UpdateMapTexture()
{
    int viewzindex = ViewManager::Get()->GetCurrentViewZIndex();
    if (viewzindex == -1)
    {
        URHO3D_LOGERRORF("UIC_MiniMap() - UpdateMapTexture ... viewzindex=-1 !");
        viewzindex = 0;
    }

    for (int i = 0; i < 9; i++)
    {
        int mapstatus = currentMaps_[i] ? currentMaps_[i]->GetStatus() : Uninitialized;
        Image* minimap = mapstatus > Setting_MiniMap && mapstatus <= Available ? currentMaps_[i]->GetMiniMap(viewzindex) : 0;
        FillMapTexture(mapTexture_, i%3, 2 - i/3, minimap ? minimap->GetData() : emptyImage_.GetData());
    }

    minimapTextureDirty_ = false;

//    URHO3D_LOGINFOF("UIC_MiniMap() - UpdateMapTexture ... viewzindex=%d on mPoint=%s OK !", viewzindex, mpointCenter_.ToString().CString());
}

void UIC_MiniMap::UpdateVisibleNodes()
{
    visibleNodes_.Clear();

    int numpoints = uiElement_->GetChildren().Size() - minimapInitialUiElts_;

    // Always add player node
    if (followedNode_)
    {
        visibleNodes_.Push(followedNode_);
    }

#ifdef ACTIVE_MINIMAP_MANAGEPOINTS
    // Check visibility of the other nodes to display
    for (unsigned i=0; i < nodesToDisplay_.Size(); i++)
    {
        WeakPtr<Node>& node = nodesToDisplay_[i];
        if (node && minimapWorldRect_.IsInside(node->GetWorldPosition2D()) == INSIDE)
            visibleNodes_.Push(node);
    }
#endif

    // Update ui positions of the ui points
    Vector2 wposition;
    Vector2 camPosition = minimapWorldRect_.Center();
    for (unsigned i=0; i < MINIMAP_MAXNUMPOINTS; i++)
    {
        Sprite* uipoint = uiPoints_[i];

        if (i < visibleNodes_.Size())
        {
            Node* node = visibleNodes_[i];

            wposition = node->GetWorldPosition2D();
            if (i > 0)
            {
                unsigned controllertype = node->GetVar(GOA::TYPECONTROLLER).GetUInt();

                if (controllertype == GO_Player || controllertype == GO_NetPlayer)
                    uipoint->SetColor(Color::GREEN);
                else if (controllertype == GO_AI_Enemy)
                    uipoint->SetColor(Color::RED);
                else
                    uipoint->SetColor(Color::BLUE);
            }

            uipoint->SetPosition((wposition.x_ - camPosition.x_) * worldToUIRatio_.x_, (camPosition.y_ - wposition.y_) * worldToUIRatio_.y_);
            uipoint->SetVisible(true);
        }
        else
        {
            uipoint->SetVisible(false);
        }
    }

//    URHO3D_LOGINFOF("UIC_MiniMap() - UpdateVisibleNodes ... visiblenodes=%u !", visibleNodes_.Size());
}

void UIC_MiniMap::OnUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!followedNode_)
        URHO3D_LOGWARNINGF("UIC_MiniMap() - OnUpdate ... !followednode !");

    if (followedNode_ && lastNodePosition_ != followedNode_->GetWorldPosition2D())
        MarkPositionDirty();

    if (minimapPositionDirty_)
        UpdateMapViewRect();

    if (minimapTextureDirty_)
        UpdateMapTexture();

    if (minimapEntitiesDirty_)
        PopulatePoints();

    UpdateVisibleNodes();
}

void UIC_MiniMap::OnMapUpdate(StringHash eventType, VariantMap& eventData)
{
    MarkTextureDirty();

    OnUpdate(eventType, eventData);
}

void UIC_MiniMap::OnWorldUpdate(StringHash eventType, VariantMap& eventData)
{
    URHO3D_PROFILE(MiniMapUIUpdate);

    MarkPositionDirty();
    MarkTextureDirty();
    MarkEntitiesDirty();

    OnUpdate(eventType, eventData);
}

void UIC_MiniMap::OnSwitchVisible(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("UIC_MiniMap() - OnSwitchVisible ...");

    if (IsVisible())
    {
        SetVisible(false);

        if (worldMap_)
        {
            UIElement* mapbutton = 0;
            RemoveActivatorButton("MapButton", &mapbutton);
            if (mapbutton)
            {
                worldMap_->AddActivatorButton(mapbutton);
            }
        }
    }
    else
    {
        SetVisible(true);
    }
}

void UIC_MiniMap::OnWorldEntityUpdate(StringHash eventType, VariantMap& eventData)
{
#ifdef ACTIVE_MINIMAP_MANAGEPOINTS
    Node* node = static_cast<Node*>(context_->GetEventSender());
    if (!node || followedNode_ == node)
        return;

    // Entity Created
    if (eventData[World_EntityUpdate::GO_STATE].GetInt() == 0)
    {
//        URHO3D_LOGINFOF("UIC_MiniMap() - OnWorldEntityUpdate ... add node=%u !", node->GetID());
        AddNodeToDisplay(node);
    }
    // Entity Destroyed
    else
    {
//        URHO3D_LOGINFOF("UIC_MiniMap() - OnWorldEntityUpdate ... remove node=%u !", node->GetID());
        nodesToDisplay_.Remove(WeakPtr<Node>(node));
    }
#endif
}

void UIC_MiniMap::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_MiniMap() - HandleScreenResized ...");

    Resize();
    OnWorldUpdate(eventType, eventData);

    URHO3D_LOGINFOF("UIC_MiniMap() - HandleScreenResized ... OK !");
}
