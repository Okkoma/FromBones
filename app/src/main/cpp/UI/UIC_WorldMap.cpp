#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Input/Input.h>

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
#include "GameNetwork.h"
#include "Player.h"

#include "GOC_Destroyer.h"

#include "Map.h"
#include "MapWorld.h"
#include "MapStorage.h"
#include "ViewManager.h"
#include "TimerRemover.h"

#include "UIC_MiniMap.h"
#include "UIC_WorldMap.h"


UIC_WorldMap::UIC_WorldMap(Context* context) :
    Component(context),
    miniMap_(0)
{
    // Create worldMap Texture
    worldMapTexture_ = SharedPtr<Texture2D>(new Texture2D(context_));
    worldMapTexture_->SetNumLevels(1);
    worldMapTexture_->SetFilterMode(FILTER_NEAREST);
}

UIC_WorldMap::~UIC_WorldMap()
{
    Clear();

    worldMapTexture_.Reset();
}

void UIC_WorldMap::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_WorldMap>();
}


void UIC_WorldMap::AddActivatorButton(UIElement* element)
{
    if (element)
    {
        activators_.Push(WeakPtr<UIElement>(element));
        SubscribeToEvent(element, E_RELEASED, URHO3D_HANDLER(UIC_WorldMap, OnSwitchVisible));
    }
}

void UIC_WorldMap::RemoveActivatorButton(const String& eltname, UIElement** element)
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

void UIC_WorldMap::SetMiniMap(UIC_MiniMap* miniMap)
{
    // Set or Create WorldMap
    miniMap_ = miniMap;
}


bool UIC_WorldMap::IsVisible() const
{
    return uiElement_ ? uiElement_->IsVisible() : false;
}

void UIC_WorldMap::SetVisible(bool state)
{
    if (state != IsVisible())
    {
        if (state)
            Start();
        else
            Stop();
    }
}


void UIC_WorldMap::Start()
{
    URHO3D_LOGINFO("UIC_WorldMap() - Start !");

    Initialize();

    Resize();

    canevasHovering_ = false;
    worldMapDirty_ = false;

    if (uiElement_)
    {
        uiElement_->SetVisible(true);
        uiElement_->SetEnabled(true);

        int width = GetSubsystem<UI>()->GetRoot()->GetSize().x_;

        GameHelpers::SetMoveAnimationUI(uiElement_, IntVector2(lastPosition_.x_ < width / 2 ? -uiElement_->GetSize().x_ : width, lastPosition_.y_), lastPosition_, 0.f, SWITCHSCREENTIME);

        SubscribeToEvent(GAME_SCREENRESIZED, URHO3D_HANDLER(UIC_WorldMap, HandleScreenResized));

        if (canevas_)
        {
            SubscribeToEvent(canevas_, E_CLICKEND, URHO3D_HANDLER(UIC_WorldMap, HandleWorldSnapShotClicked));
            SubscribeToEvent(canevas_, E_HOVERBEGIN, URHO3D_HANDLER(UIC_WorldMap, HandleWorldSnapShotHoverBegin));
            SubscribeToEvent(canevas_, E_HOVEREND, URHO3D_HANDLER(UIC_WorldMap, HandleWorldSnapShotHoverEnd));
        }

        if (miniMap_)
        {
            Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
            SubscribeToEvent(switchMapButton, E_RELEASED, URHO3D_HANDLER(UIC_WorldMap, OnSwitchVisible));
        }

        SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(UIC_WorldMap, HandleUpdate));
    }
    else
    {
        URHO3D_LOGERRORF("UIC_WorldMap() - Start : node=%s(%u) gocptr=%u NO UI !", node_->GetName().CString(), node_->GetID(), this);
    }

    for (unsigned i = 0; i < activators_.Size(); i++)
    {
        if (activators_[i])
            SubscribeToEvent(activators_[i].Get(), E_RELEASED, URHO3D_HANDLER(UIC_WorldMap, OnSwitchVisible));
    }
}

void UIC_WorldMap::Stop()
{
    URHO3D_LOGINFO("UIC_WorldMap() - Stop !");

    UnsubscribeFromAllEvents();

    if (uiElement_)
    {
        int width = GetSubsystem<UI>()->GetRoot()->GetSize().x_;

        lastPosition_ = uiElement_->GetPosition();
        GameHelpers::SetMoveAnimationUI(uiElement_, lastPosition_, IntVector2(lastPosition_.x_ < width / 2 ? -uiElement_->GetSize().x_ : width, lastPosition_.y_), 0.f, SWITCHSCREENTIME);
        TimerRemover::Get()->Start(uiElement_, SWITCHSCREENTIME + 0.5f, DISABLE);
    }

    for (unsigned i = 0; i < activators_.Size(); i++)
    {
        if (activators_[i])
            SubscribeToEvent(activators_[i].Get(), E_RELEASED, URHO3D_HANDLER(UIC_WorldMap, OnSwitchVisible));
    }
}

void UIC_WorldMap::Clear()
{
    URHO3D_LOGINFO("UIC_WorldMap() - Clear !");

    Stop();

    if (uiElement_)
        uiElement_->Remove();

    uiPoints_.Clear();
}


void UIC_WorldMap::OnNodeSet(Node* node)
{
    if (node)
    {
        URHO3D_LOGINFO("UIC_WorldMap() - OnNodeSet ...");

        SetTemporary(true);

        URHO3D_LOGINFO("UIC_WorldMap() - OnNodeSet ... OK !");
    }
    else
    {
        Clear();
    }
}

void UIC_WorldMap::OnSetEnabled()
{
    if (IsEnabledEffective())
        Start();
    else
        Stop();
}

Sprite* UIC_WorldMap::CreatePoint(UIElement* root, const Color& color, int size)
{
    Sprite* uipoint = root->CreateChild<Sprite>("WorldMapPoint");

    GameHelpers::SetUIElementFrom(uipoint, UIEQUIPMENT, "point");

    uipoint->SetFocusMode(FM_NOTFOCUSABLE);
    uipoint->SetColor(color);
    uipoint->SetSize(size, size);
    uipoint->SetHotSpot(size/2, size/2);
    uipoint->SetAlignment(HA_LEFT, VA_TOP);
    uipoint->SetVisible(false);

    return uipoint;
}


void UIC_WorldMap::Initialize()
{
    if (uiElement_)
        return;

    URHO3D_LOGINFO("UIC_WorldMap() - Initialize ...");

    UI* ui = GetSubsystem<UI>();
    uiElement_ = dynamic_cast<Window*>(ui->GetRoot()->GetChild(String("WorldMap")));

    // Create UI Element
    SharedPtr<Window> uielt(new Window(context_));
    ui->GetRoot()->AddChild(uielt);
    uiElement_ = uielt;
    uiElement_->SetName("WorldMap");
//    uiElement_->SetVar(GOA::OWNERID, GetNode()->GetID());

    // Set Window
    GameHelpers::SetUIElementFrom(uiElement_.Get(), UIEQUIPMENT, "windowframe1");
    uiElement_->SetUseDerivedOpacity(false);
    uiElement_->SetOpacity(GameContext::Get().gameConfig_.uiMapOpacityBack_);
    uiElement_->SetMovable(true);
    uiElement_->SetResizable(true);
    uiElement_->SetBorder(IntRect(4, 4, 4, 4));
    uiElement_->SetResizeBorder(IntRect(8, 8, 8, 8));

    canevas_ = uiElement_->CreateChild<Button>("WorldSnapShot");
    GameHelpers::SetUIElementFrom(canevas_.Get(), UIEQUIPMENT, "UIWindowEmpty");
    canevas_->SetUseDerivedOpacity(false);
    canevas_->SetOpacity(GameContext::Get().gameConfig_.uiMapOpacityFrame_);
    canevas_->SetFocusMode(FM_FOCUSABLE_DEFOCUSABLE);
    canevas_->SetAlignment(HA_CENTER, VA_CENTER);
    canevas_->SetHoverOffset(IntVector2::ZERO);
    canevas_->SetPressedOffset(IntVector2::ZERO);
    canevas_->SetPressedChildOffset(IntVector2::ZERO);
    canevas_->AddTag("AllowTouchCursor");

    // Add SwitchMapButton
    if (miniMap_)
    {
        Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
        if (!switchMapButton)
            switchMapButton = uiElement_->CreateChild<Button>("SwitchMapButton");

        switchMapButton->SetStyle("OptionButton");
        switchMapButton->SetAlignment(HA_RIGHT, VA_TOP);
        switchMapButton->SetFocusMode(FM_RESETFOCUS);

        Text* text = switchMapButton->CreateChild<Text>();
        text->SetColor(Color(0.85f, 0.85f, 0.85f));
        text->SetFont(GameContext::Get().uiFonts_[UIFONTS_ABY12], 12);
        text->SetAlignment(HA_CENTER, VA_CENTER);
        text->SetText("M");

        miniMap_->AddActivatorButton(switchMapButton);
    }

    UIElement* gridroot = canevas_->CreateChild<UIElement>("GridRoot");
    gridroot->SetFocusMode(FM_NOTFOCUSABLE);
    gridroot->SetLayout(LM_FREE);
    gridroot->SetAlignment(HA_LEFT, VA_TOP);

    UIElement* pointroot = canevas_->CreateChild<UIElement>("PointRoot");
    pointroot->SetFocusMode(FM_NOTFOCUSABLE);
    pointroot->SetLayout(LM_FREE);
    pointroot->SetAlignment(HA_LEFT, VA_TOP);

    // Create Points
    uiPoints_.Clear();

    for (unsigned i = 0; i < GameContext::Get().MAX_NUMPLAYERS; i++)
        uiPoints_.Push(CreatePoint(pointroot, Color::YELLOW, 10));

    // check if previous snapshots workers unfinished ?
    World2DInfo* info = World2D::GetWorldInfo();
    if (info && info->worldModel_)
    {
        info->worldModel_->StopUnfinishedWorks();
    }

    // initial zoom = 4 maps
    const float diameter = info->worldModel_->GetRadius() != 0.f ? 2.f * info->worldModel_->GetRadius() : 1.f;
    worldMapScale_ = 4.f / diameter;
    worldMapNumMaps_ = CeilToInt(diameter * worldMapScale_);

    // get the maximal Num of Maps (max dezoom)
    float scale = diameter * 2.f / info->worldModel_->GetScale().x_;
    int power = 0;
    while (scale > 1.f)
    {
        scale /= 2.f;
        power++;
    }
    worldMapMaxMaps_ = Pow(2, power);

    worldMapTopLeftMap_.x_ = World2D::GetCurrentMapPoint(0).x_ - 2;
    worldMapTopLeftMap_.y_ = World2D::GetCurrentMapPoint(0).y_ + 2;

    worldMapCenter_.x_ = (float)worldMapTopLeftMap_.x_ / worldMapNumMaps_;
    worldMapCenter_.y_ = (float)worldMapTopLeftMap_.y_ / worldMapNumMaps_;

    UpdateSnapShots();

    URHO3D_LOGINFOF("UIC_WorldMap() - Initialize ... worldMapTopLeftMap=%d %d scale=%F maxMaps=%d ... OK !", worldMapTopLeftMap_.x_, worldMapTopLeftMap_.y_, worldMapScale_, worldMapMaxMaps_);
}

void UIC_WorldMap::Resize()
{
    if (!uiElement_)
        Initialize();

    const int uiwidth  = GameContext::Get().ui_->GetRoot()->GetSize().x_;
    const int uiheight = GameContext::Get().ui_->GetRoot()->GetSize().y_;
    const int border = 16;

    float csize = floor(uiheight/2);
    int power = 0;
    while (csize > 2.f)
    {
        csize /= 2.f;
        power++;
    }
    int canevassize = Pow(2, power);
    int size = canevassize + 2 * border;

    URHO3D_LOGINFOF("UIC_WorldMap() - Resize ... size=%d canevassize=%d", size, canevassize);

    uiElement_->SetSize(size, size);
    uiElement_->SetPosition(uiwidth-size, 0.f);//uiheight-size);
    lastPosition_ = uiElement_->GetPosition();

    canevas_->SetSize(canevassize, canevassize);
    canevas_->SetAlignment(HA_CENTER, VA_CENTER);

    UpdateSnapShotGrid();

    size = 32;
    Button* switchMapButton = dynamic_cast<Button*>(uiElement_->GetChild(String("SwitchMapButton"), true));
    if (switchMapButton)
    {
        switchMapButton->SetSize(size, size);
    }

    // Update points scale
    size = 20;
    for (unsigned i = 0; i < GameContext::Get().MAX_NUMPLAYERS; i++)
    {
        uiPoints_[i]->SetSize(size, size);
        uiPoints_[i]->SetHotSpot(size/2, size/2);
    }

    URHO3D_LOGINFO("UIC_WorldMap() - Resize ... OK !");
}


void UIC_WorldMap::UpdateSnapShots()
{
    World2DInfo* info = World2D::GetWorldInfo();
    if (info && info->worldModel_)
    {
        info->worldModel_->StopUnfinishedWorks();

        Vector<String> modulerenderablenames;
        modulerenderablenames.Push("CaveMap");

        info->worldModel_->GenerateSnapShots(worldMapNumMaps_ > 1 ? 128 : 64, MiniMapColorFrontBlocked, worldMapCenter_, worldMapScale_, modulerenderablenames.Size(), modulerenderablenames.Buffer(), worldMapTexture_);

        SubscribeToEvent(info->worldModel_.Get(), GAME_WORLDSNAPSHOTSAVED, URHO3D_HANDLER(UIC_WorldMap, HandleWorldSnapShotChanged));
    }
}

void UIC_WorldMap::UpdateSnapShotGrid()
{
    bool gridOn_ = true;

    UIElement* gridroot = canevas_->GetChild(0);
    gridroot->SetVisible(gridOn_);

    if (gridOn_)
    {
        World2DInfo* info = World2D::GetWorldInfo();
        if (info && info->worldModel_)
        {
            const int canevassize = canevas_->GetWidth();
            const int minspacing = 32;

            int spacing = canevassize / worldMapNumMaps_;

            URHO3D_LOGINFOF("UIC_WorldMap() - UpdateSnapShotGrid : add grid for world mapping => nummaps=%d canevassize=%dpix spacing=%dpix", worldMapNumMaps_, canevassize, spacing);

            if (spacing < minspacing)
            {
                return;
            }

            int numgridpoints = Pow(worldMapNumMaps_+1, 2);
            if (numgridpoints > gridroot->GetNumChildren())
            {
                SharedPtr<Texture2D> texture(context_->GetSubsystem<ResourceCache>()->GetResource<Texture2D>("Textures/UI/point.png"));
                const int pointsize = (texture->GetWidth() + 1 ) * GameContext::Get().uiScale_;
                const Color color(0.8f, 0.4f, 0.f);

                int numchildren = gridroot->GetNumChildren();
                for (int i = numchildren; i < numgridpoints; i++)
                {
                    BorderImage* gridpoint = gridroot->CreateChild<BorderImage>();
                    gridpoint->SetFocusMode(FM_NOTFOCUSABLE);
                    gridpoint->SetTexture(texture);
                    gridpoint->SetImageRect(IntRect(0, 0, texture->GetWidth(), texture->GetHeight()));
                    gridpoint->SetFullImageRect();
                    gridpoint->SetLayout(LM_FREE);
                    gridpoint->SetAlignment(HA_CENTER, VA_CENTER);
                    gridpoint->SetSize(pointsize, pointsize);
                    gridpoint->SetColor(color);
                }
            }

            unsigned addr = 0;
            for (int y=0; y <= worldMapNumMaps_; y++)
            {
                for (int x=0; x <= worldMapNumMaps_; x++, addr++)
                {
                    BorderImage* gridpoint = static_cast<BorderImage*>(gridroot->GetChild(addr));
                    gridpoint->SetPosition(x * spacing, y * spacing);
                    gridpoint->SetVisible(true);
                    gridpoint->SetOpacity(Min(1.f, 2.f / worldMapNumMaps_));
                }
            }

            if (numgridpoints < gridroot->GetNumChildren())
            {
                unsigned numchildren = gridroot->GetNumChildren();
                for (unsigned i = numgridpoints; i < numchildren; i++)
                    gridroot->GetChild(i)->SetVisible(false);
            }
        }
    }
}

void UIC_WorldMap::OnSwitchVisible(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFO("UIC_WorldMap() - OnSwitchVisible ...");

    if (IsVisible())
    {
        SetVisible(false);

        if (miniMap_)
        {
            UIElement* mapbutton = 0;
            RemoveActivatorButton("MapButton", &mapbutton);
            if (mapbutton)
            {
                miniMap_->AddActivatorButton(mapbutton);
            }
        }
    }
    else
    {
        SetVisible(true);
    }
}


void UIC_WorldMap::HandleScreenResized(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_WorldMap() - HandleScreenResized ...");

    Resize();

    URHO3D_LOGINFOF("UIC_WorldMap() - HandleScreenResized ... OK !");
}

void UIC_WorldMap::HandleWorldSnapShotChanged(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("UIC_WorldMap() - HandleWorldSnapShotChanged !");

    canevas_->SetTexture(worldMapTexture_.Get());
    canevas_->SetFullImageRect();

    UpdateSnapShotGrid();

    worldMapDirty_ = false;
}

void UIC_WorldMap::HandleWorldSnapShotHoverBegin(StringHash eventType, VariantMap& eventData)
{
    canevasHovering_ = true;

    if (GameContext::Get().gameConfig_.touchEnabled_ && GameContext::Get().input_->GetTouch(0))
    {
        GameContext::Get().input_->GetTouch(0)->touchedElement_ = canevas_;
    }

    URHO3D_LOGINFO("UIC_WorldMap() - HandleWorldSnapShotHoverBegin  !");
}

void UIC_WorldMap::HandleWorldSnapShotHoverEnd(StringHash eventType, VariantMap& eventData)
{
    canevasHovering_ = false;

    // restore cursor
    if (!GameContext::Get().gameConfig_.touchEnabled_)
        GameContext::Get().cursorShape_ = CURSOR_DEFAULT;

    URHO3D_LOGINFO("UIC_WorldMap() - HandleWorldSnapShotHoverEnd  !");
}

void UIC_WorldMap::HandleWorldSnapShotClicked(StringHash eventType, VariantMap& eventData)
{
    int button = eventData[ClickEnd::P_BUTTON].GetInt();

    World2DInfo* info = World2D::GetWorldInfo();
    if (info && info->worldModel_)
    {
        if (button == MOUSEB_LEFT)
        {
            if (GameContext::Get().cursorShape_ == CURSOR_TELEPORTATION)
            {
                IntVector2 position;
                GameHelpers::GetInputPosition(position.x_, position.y_);
                position.x_ = position.x_ / GameContext::Get().uiScale_;
                position.y_ = position.y_ / GameContext::Get().uiScale_;
                position = canevas_->ScreenToElement(position);

                const float canevassize = (float)canevas_->GetSize().x_;
                ShortIntVector2 hovermap;
                hovermap.x_ = worldMapTopLeftMap_.x_ + ((int)floor(worldMapNumMaps_ * Clamp((float)position.x_, 0.f, canevassize-1) / canevassize));
                hovermap.y_ = worldMapTopLeftMap_.y_ - ((int)floor(worldMapNumMaps_ * Clamp((float)position.y_, 0.f, canevassize-1) / canevassize));

                URHO3D_LOGINFOF("UIC_WorldMap() - HandleWorldSnapShotClicked : canevassize=%F position=%d %d worldMapTopLeftMap=%d %d worldMapNumMaps_=%d => click on map=%d %d",
                                canevassize, position.x_, position.y_, worldMapTopLeftMap_.x_, worldMapTopLeftMap_.y_, worldMapNumMaps_, hovermap.x_, hovermap.y_);

                URHO3D_LOGINFOF("go2map(%s) => waiting ...", hovermap.ToString().CString());

                GameHelpers::TransferPlayersToMap(hovermap);
            }
            else
            {
                float scalefactor = 1.f;

                // Mouse Right : Zoom In
                if (GameContext::Get().cursorShape_ == CURSOR_ZOOMPLUS)
                {
                    scalefactor = 0.5f;
                }
                // Mouse Middle : Zoom out
                else if (GameContext::Get().cursorShape_ == CURSOR_ZOOMMINUS)
                {
                    scalefactor = 2.f;
                }

                const int worldMapNumMapsLast = worldMapNumMaps_;
                const float diameter = info->worldModel_->GetRadius() != 0.f ? 2.f * info->worldModel_->GetRadius() : 1.f;
                const int worldMapNumMaps = CeilToInt(diameter * worldMapScale_ * scalefactor);

                if ((scalefactor == 1.f || worldMapNumMapsLast != worldMapNumMaps) && worldMapNumMaps >= 1 && worldMapNumMaps <= worldMapMaxMaps_)
                {
                    const int canevassize = canevas_->GetSize().x_;

                    IntVector2 mapcoordoffset;
                    const IntVector2 position = canevas_->ScreenToElement(IntVector2(eventData[ClickEnd::P_X].GetInt(), eventData[ClickEnd::P_Y].GetInt()));

                    if (GameContext::Get().cursorShape_ != CURSOR_ZOOMMINUS)
                    {
                        const float threshold = 0.1f;
                        const int halfnummaps = CeilToInt((float)worldMapNumMaps / 2.f);
                        float posx, posy;
                        int dirx, diry;

                        // Move
                        if (worldMapNumMaps > 1 || GameContext::Get().cursorShape_ != CURSOR_ZOOMPLUS)
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
                        // Zoom In
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

                        URHO3D_LOGINFOF("UIC_WorldMap() - HandleWorldSnapShotClicked : scale=%F izoom=%d ... halfnummaps=%d dirx=%d posx=%F diry=%d posy=%F ... mapoffset=%d %d",
                                        worldMapScale_, worldMapNumMaps, halfnummaps, dirx, posx, diry, posy, mapcoordoffset.x_, mapcoordoffset.y_);
                    }

                    int mapzoomrecenter = RoundToInt(diameter * worldMapScale_ * (1.f - scalefactor) * 0.5f);

                    worldMapTopLeftMap_.x_ = worldMapTopLeftMap_.x_ + mapcoordoffset.x_ + mapzoomrecenter;
                    worldMapTopLeftMap_.y_ = worldMapTopLeftMap_.y_ + mapcoordoffset.y_ - mapzoomrecenter;

                    worldMapCenter_.x_ = (float)worldMapTopLeftMap_.x_ / worldMapNumMaps;
                    worldMapCenter_.y_ = (float)worldMapTopLeftMap_.y_ / worldMapNumMaps;

                    worldMapScale_ *= scalefactor;
                    worldMapNumMaps_ = worldMapNumMaps;

                    URHO3D_LOGINFOF("UIC_WorldMap() - HandleWorldSnapShotClicked : scale=%F izoom=%d mapoffset=%d %d maprecenter=%d map=%d %d defaultmap=%d %d anlcenter=%F %F", worldMapScale_, worldMapNumMaps_,
                                    mapcoordoffset.x_, mapcoordoffset.y_, mapzoomrecenter, worldMapTopLeftMap_.x_, worldMapTopLeftMap_.y_,
                                    GameContext::Get().testZoneCustomDefaultMap_.x_, GameContext::Get().testZoneCustomDefaultMap_.y_, worldMapCenter_.x_, worldMapCenter_.y_);

                    UpdateSnapShots();

                    worldMapDirty_ = true;
                }
            }
        }
    }
}

bool UIC_WorldMap::SetUIPointPosition(int index, const float uitilesize, const WorldMapPosition& wpos)
{
    if (wpos.mPoint_.x_ >= worldMapTopLeftMap_.x_ && wpos.mPoint_.x_ < worldMapTopLeftMap_.x_ + worldMapNumMaps_ &&
        wpos.mPoint_.y_ > worldMapTopLeftMap_.y_ - worldMapNumMaps_ && wpos.mPoint_.y_ <= worldMapTopLeftMap_.y_)
    {
        if (index >= uiPoints_.Size())
            uiPoints_.Push(CreatePoint(uiPoints_.Front()->GetParent(), Color::GREEN, 10));

        int x = ((wpos.mPoint_.x_ - worldMapTopLeftMap_.x_) * World2D::GetWorldInfo()->mapWidth_ + wpos.mPosition_.x_) * uitilesize;
        int y = ((worldMapTopLeftMap_.y_ - wpos.mPoint_.y_) * World2D::GetWorldInfo()->mapHeight_ + wpos.mPosition_.y_) * uitilesize;

        uiPoints_[index]->SetPosition(x, y);
        return true;
    }

    return false;
}

void UIC_WorldMap::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (worldMapDirty_)
        return;

    // Show Players position

    const float canevassize = (float)canevas_->GetSize().x_;
    const float uimapsize = canevassize / worldMapNumMaps_;
    const float uitilesize = canevassize / (worldMapNumMaps_ * World2D::GetWorldInfo()->mapWidth_);

    int i=0;
    for (i=0; i < GameContext::Get().MAX_NUMPLAYERS; i++)
    {
        bool pointenabled = GameContext::Get().playerAvatars_[i] && GameContext::Get().playerAvatars_[i]->IsEnabled() &&
                            SetUIPointPosition(i, uitilesize, GameContext::Get().playerAvatars_[i]->GetComponent<GOC_Destroyer>()->GetWorldMapPosition());

        uiPoints_[i]->SetVisible(pointenabled);
    }

    // Show NetPlayers

    if (GameContext::Get().ServerMode_)
    {
        const HashMap<Connection*, ClientInfo >& clientInfos = GameNetwork::Get()->GetServerClientInfos();
        for (HashMap<Connection*, ClientInfo >::ConstIterator it = clientInfos.Begin(); it != clientInfos.End(); ++it)
        {
            const Vector<SharedPtr<Player> >& players = it->second_.players_;
            for (Vector<SharedPtr<Player> >::ConstIterator jt = players.Begin(); jt != players.End(); ++jt)
            {
                bool pointenabled = SetUIPointPosition(i, uitilesize, (*jt)->GetWorldMapPosition());

                if (i < uiPoints_.Size())
                    uiPoints_[i]->SetVisible(pointenabled);

                if (pointenabled)
                    i++;
            }
        }
        for (int j=i; j < uiPoints_.Size(); j++)
            uiPoints_[j]->SetVisible(false);
    }
    else if (GameContext::Get().ClientMode_)
    {
        WorldMapPosition wpos;
        const Vector<NetPlayerInfo >& netplayersInfos = GameNetwork::Get()->GetNetPlayersInfos();
        for (Vector<NetPlayerInfo >::ConstIterator it = netplayersInfos.Begin(); it != netplayersInfos.End(); ++it)
        {
            if (!it->node_)
                continue;

            World2D::GetWorldInfo()->Convert2WorldMapPosition(it->node_->GetWorldPosition2D(), wpos);

            bool pointenabled = SetUIPointPosition(i, uitilesize, wpos);

            if (i < uiPoints_.Size())
                uiPoints_[i]->SetVisible(pointenabled);

            if (pointenabled)
                i++;
        }
        for (int j=i; j < uiPoints_.Size(); j++)
            uiPoints_[j]->SetVisible(false);
    }

    // change the cursor
    if (canevasHovering_)
    {
        if (GameContext::Get().input_->GetQualifierDown(QUAL_CTRL))
        {
            GameContext::Get().cursorShape_ = CURSOR_TELEPORTATION;
        }
        else
        {
            IntVector2 position;
            GameHelpers::GetInputPosition(position.x_, position.y_);
            position.x_ = position.x_ / GameContext::Get().uiScale_;
            position.y_ = position.y_ / GameContext::Get().uiScale_;
            position = canevas_->ScreenToElement(position);

            Vector2 normalizedposition;
            normalizedposition.x_ = (float)position.x_ / canevassize - 0.5f;
            normalizedposition.y_ = (float)(canevassize - position.y_) / canevassize - 0.5f;

            const float threshold = 0.25f;
            if (normalizedposition.x_ < -threshold)
            {
                if (normalizedposition.y_ > threshold)
                    GameContext::Get().cursorShape_ = CURSOR_TOPLEFT;
                else if (normalizedposition.y_ < -threshold)
                    GameContext::Get().cursorShape_ = CURSOR_BOTTOMLEFT;
                else
                    GameContext::Get().cursorShape_ = CURSOR_LEFT;
            }
            else if (normalizedposition.x_ > threshold)
            {
                if (normalizedposition.y_ > threshold)
                    GameContext::Get().cursorShape_ = CURSOR_TOPRIGHT;
                else if (normalizedposition.y_ < -threshold)
                    GameContext::Get().cursorShape_ = CURSOR_BOTTOMRIGHT;
                else
                    GameContext::Get().cursorShape_ = CURSOR_RIGHT;
            }
            else
            {
                if (normalizedposition.y_ > threshold)
                    GameContext::Get().cursorShape_ = CURSOR_TOP;
                else if (normalizedposition.y_ < -threshold)
                    GameContext::Get().cursorShape_ = CURSOR_BOTTOM;
                else
                {
                    if (normalizedposition.y_ > 0.f)
                        GameContext::Get().cursorShape_ = CURSOR_ZOOMPLUS;
                    else
                        GameContext::Get().cursorShape_ = CURSOR_ZOOMMINUS;
                }
            }
        }
//		URHO3D_LOGINFOF("UIC_WorldMap() - HandleUpdate : canevassize=%F uiscale=%F position=%d %d cursor=%d worldMapTopLeftMap=%d %d worldMapNumMaps_=%d",
//						canevassize, GameContext::Get().uiScale_, position.x_, position.y_, GameContext::Get().cursorShape_, worldMapTopLeftMap_.x_, worldMapTopLeftMap_.y_, worldMapNumMaps_);
    }
}

