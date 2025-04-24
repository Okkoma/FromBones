#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/SpriteSheet2D.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Text.h>

#include "DefsViews.h"
#include "DefsWorld.h"
#include "ObjectTiled.h"

#include "MapWorld.h"

#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameContext.h"
#include "GameEvents.h"

#include "GEF_Rain.h"
#include "GEF_Scrolling.h"

#include "ViewManager.h"

#include "MAN_Weather.h"


const char* weatherEffectNames[] =
{
    "WeatherNone=0",
    "WeatherRain=1",
    "WeatherCloud=2",
};

const char* timePeriodNames_[] =
{
    "Night",
    "Dawn",
    "DayTime",
    "Twilight"
};

// equivalence between time and worldtime : 1min = 1hours passed in world
const float WORLDTIMERATIO = 60.f * 1.f;
// equivalence between time and worldtime : 1min = 3hours passed in world
//const float WORLDTIMERATIO = 60.f * 3.f;
// equivalence between time and worldtime : 1min = 6hours passed in world
//const float WORLDTIMERATIO = 60.f * 6.f;
// equivalence between time and worldtime : 1min = 12hours passed in world
//const float WORLDTIMERATIO = 60.f * 12.f;

const float HOURTODEG = 360.f / 24.f;
const float NIGHTHOURMIN = 18.f;
const float NIGHTHOURMAX = 6.f;
const float DAYHOURMIN = 8.f;
const float DAYHOURMAX = 16.f;


Text* timeDebugText_ = 0;
WeatherManager* WeatherManager::weatherManager_ = 0;

void WeatherManager::WeatherViewData::Clear()
{
    if (luminaries_)
    {
        luminaries_->Remove();
        luminaries_.Reset();
        sun_.Reset();
        moon_.Reset();
    }

    if (rain_)
    {
        raintime_ = raindelay_ = 0.f;
        rainintensity_ = 0;
        rain_->Remove();
        raineffect_.Reset();
        rain_.Reset();
    }

    if (cloud_)
    {
        cloudintensity_ = 0;
        cloud_->Remove();
        cloudeffect_.Reset();
        cloud_.Reset();
    }
}

WeatherManager::WeatherManager(Context* context)
    : Object(context)
{
    weatherManager_ = this;

    Init();
}

WeatherManager::~WeatherManager()
{
    for (int viewport=0; viewport < MAX_VIEWPORTS; viewport++)
        weatherviewdatas_[viewport].Clear();

    if (timeDebugText_)
    {
        timeDebugText_->Remove();
        timeDebugText_ = 0;
    }

    weatherManager_ = 0;
}

void WeatherManager::RegisterObject(Context* context)
{
    context->RegisterFactory<WeatherManager>();
}


int timeperiodsent_;

void WeatherManager::Init()
{
    URHO3D_LOGINFOF("WeatherManager() - Init !");

    active_ = true;
    timeperiodsent_ = -1;

    SetWorldTime(12.f);
    SetWorldTimeRate();

    ResourceCache* cache = GetSubsystem<ResourceCache>();
    StaticSprite2D* image;

    Node* scene = GameContext::Get().rootScene_->GetChild("Scene");

    for (int viewport=0; viewport < MAX_VIEWPORTS; viewport++)
    {
        unsigned frontviewportmask = (VIEWPORTSCROLLER_OUTSIDE_MASK << viewport);
        unsigned backviewportmask = frontviewportmask | (VIEWPORTSCROLLER_INSIDE_MASK << viewport);

        WeatherViewData& viewdata = weatherviewdatas_[viewport];

        viewdata.active_ = true;
        viewdata.rainintensity_ = 1;
        viewdata.cloudintensity_ = 1;

        // Set Luminaries
        WeakPtr<Node>& luminaries = viewdata.luminaries_;
        luminaries = scene->CreateChild("Luminaries", LOCAL);
        luminaries->SetTemporary(true);

        WeakPtr<Node>& sun = viewdata.sun_;
        sun = luminaries->CreateChild("Sun", LOCAL);
        sun->SetWorldScale(1.5f);
        image = sun->CreateComponent<StaticSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
        image->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERBACKGROUNDS]);
#endif
        image->SetSprite(Sprite2D::LoadFromResourceRef(context_, ResourceRef(SpriteSheet2D::GetTypeStatic(), "2D/sky.xml@sun")));
        image->SetViewMask(backviewportmask);
        image->SetLayer2(IntVector2(BACKSCROLL_0,-1));
        image->SetOrderInLayer(1);
        image->SetTextureFX(UNLIT);

        SetSunLight(viewport);

        WeakPtr<Node>& moon = viewdata.moon_;
        moon = luminaries->CreateChild("Moon", LOCAL);
        moon->SetWorldScale(1.5f);
        image = moon->CreateComponent<StaticSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
        image->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERBACKGROUNDS]);
#endif
        image->SetSprite(Sprite2D::LoadFromResourceRef(context_, ResourceRef(SpriteSheet2D::GetTypeStatic(), "2D/sky.xml@moon")));
        image->SetViewMask(backviewportmask);
        image->SetLayer2(IntVector2(BACKSCROLL_0,-1));
        image->SetOrderInLayer(2);
        image->SetTextureFX(UNLIT);

#ifdef ACTIVE_WEATHEREFFECTS
        // Set Rain Effect
        WeakPtr<Node>& rain = viewdata.rain_;
        rain = scene->CreateChild("Rain", LOCAL);
        rain->SetWorldScale(1.f);
        rain->SetTemporary(true);
        WeakPtr<GEF_Rain>& raineffect = viewdata.raineffect_;
        raineffect = rain->CreateComponent<GEF_Rain>(LOCAL);
        raineffect->SetAnimationSet("2D/droplet.scml");
        raineffect->SetViewport(viewport);

        // Set Cloud Effect
        WeakPtr<Node>& cloud = viewdata.cloud_;
        cloud = scene->CreateChild("Cloud", LOCAL);
        cloud->SetTemporary(true);

#ifdef ACTIVE_LAYERMATERIALS
        Material* material = GameContext::Get().layerMaterials_[LAYERBACKGROUNDS];
#else
        Material* material = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/nuages.xml");
#endif

#ifdef ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE

#ifdef ACTIVE_LAYERMATERIALS
        const int textureunit = 3;
#else
        const int textureunit = 0;
#endif

#ifdef ACTIVE_SCROLLINGSHAPE_FRUSTUMCAMERA
        Vector2 repeat(0.25f, 0.5f);
        //    Vector2 repeat(1.f, 1.f);
#else
        Vector2 repeat(0.5f, 1.f);
#endif
        ScrollingShape::AddQuadScrolling(cloud, BACKSCROLL_0, 0,  material, textureunit, 0, Vector2::ZERO, repeat, Vector2(0.01f, 0.f), Vector2(-0.001f, -0.001f), Color::GRAY);
        ScrollingShape::AddQuadScrolling(cloud, BACKSCROLL_1, 2,  material, textureunit, 16, Vector2::ZERO, repeat, Vector2(0.05f, -0.01f), Vector2(-0.001f, -0.001f), Color::GRAY);
        ScrollingShape::AddQuadScrolling(cloud, BACKSCROLL_2, 4,  material, textureunit, 48, Vector2(2.f, 2.f), repeat, Vector2(0.075f, -0.01f), Vector2(-0.001f, -0.001f), Color::GRAY);
        ScrollingShape::AddQuadScrolling(cloud, BACKSCROLL_4, 0,  material, textureunit, 16, Vector2(-2.f, -2.f), repeat, Vector2(0.1f, 0.01f));
        ScrollingShape::AddQuadScrolling(cloud, FRONTSCROLL_5, 0, material, textureunit, 48, Vector2::ZERO, repeat, Vector2(0.15f, -0.01f));
#else
        WeakPtr<GEF_Scrolling>& cloudeffect = viewdata.cloudeffect_;
        cloudeffect = cloud->CreateComponent<GEF_Scrolling>(LOCAL);
        cloudeffect->SetViewport(viewport);

        // BACKGROUND CLOUDS
        cloudeffect->AddMaterialLayer(material, 1, BACKSCROLL_0, 0, backviewportmask, 2.f*PIXEL_SIZE, "SpriteSheet2D;Data/2D/sky.xml@sky", Vector2::ZERO, Vector2(-0.05f, 0.f), Vector2::ONE, Color::GRAY);
        cloudeffect->AddMaterialLayer(material, 1, BACKSCROLL_1, 2, backviewportmask, 0.f, "SpriteSheet2D;Data/2D/nuages.xml@nuage2", Vector2::ZERO, Vector2(-0.1f, 0.03f), Vector2::ONE, Color::GRAY);

        // MOUNTAIN CLOUDSEA
        cloudeffect->AddMaterialLayer(material, 1, BACKSCROLL_2, 2, backviewportmask, 0.f, "SpriteSheet2D;Data/2D/fonds.xml@nuage", Vector2(1.f, 0.f), Vector2(-1.f, 0.01f), Vector2::ONE, Color::GRAY);
        cloudeffect->AddMaterialLayer(material, 1, BACKSCROLL_3, 4, backviewportmask, 0.f, "SpriteSheet2D;Data/2D/fonds.xml@nuage", Vector2(-1.f, -1.f), Vector2(-0.5f, 0.01f), Vector2::ONE, Color::GRAY);
        cloudeffect->AddMaterialLayer(material, 1, BACKSCROLL_4, 0, backviewportmask, 0.f, "SpriteSheet2D;Data/2D/nuages.xml@nuage2", Vector2(-2.f, -2.f), Vector2(-3.5f, 0.1f), Vector2(0.f,-0.2f), Color::WHITE);

        // FRONT CLOUDS
#ifdef ACTIVE_LAYERMATERIALS
        material = GameContext::Get().gameConfig_.fluidEnabled_ ? GameContext::Get().layerMaterials_[LAYERFRONTSHAPES] : GameContext::Get().layerMaterials_[LAYERBACKGROUNDS];
#endif
        cloudeffect->AddMaterialLayer(material, 1, FRONTSCROLL_5, 0, frontviewportmask, 0.f, "SpriteSheet2D;Data/2D/nuages.xml@nuage3", Vector2::ZERO, Vector2(-7.f, -0.1f), Vector2(0.f,-0.5f), Color::WHITE);
#endif
#endif
    }

    if (!timeDebugText_)
    {
        UI* ui = GetSubsystem<UI>();
        if (ui)
        {
            UIElement* uiRoot = ui->GetRoot();
            timeDebugText_ = new Text(context_);
            timeDebugText_->SetAlignment(HA_CENTER, VA_TOP);
            timeDebugText_->SetPriority(100);
            timeDebugText_->SetStyle("DebugHudText");
            timeDebugText_->SetVisible(false);
            uiRoot->AddChild(timeDebugText_);
        }
    }
}

void WeatherManager::ResetTimePeriod()
{
    Update();
    SendEvent(WEATHER_DAWN);
}

void WeatherManager::SetWorldTime(float time, unsigned day, unsigned year)
{
    URHO3D_LOGINFOF("WeatherManager() - SetWorldTime : time=%F day=%u year=%u", time, day, year);

    startworldtime_ = time;
    worldtime_ = time;
    day_ = day;
    year_ = year;

    UpdateTimePeriod();
}

void WeatherManager::SetWorldTimeRate(float rate)
{
    worldtimerate_ = !rate ? WORLDTIMERATIO : rate;
}

void WeatherManager::SetNetWorldInfos(const VariantVector& infos)
{
    startworldtime_ = infos[0].GetFloat();
    worldtime_ = infos[1].GetFloat();
    day_ = infos[2].GetInt();
    year_ = infos[3].GetInt();

    URHO3D_LOGINFOF("WeatherManager() - SetNetWorldInfos : time=%F(%F) day=%d year=%d rate=%F", worldtime_, startworldtime_, day_, year_);

    UpdateTimePeriod();

    SetWorldTimeRate(infos[4].GetFloat());
}

void WeatherManager::GetNetWorldInfos(VariantVector& infos)
{
    infos.Clear();
    infos.Push(Variant(startworldtime_));
    infos.Push(Variant(worldtime_));
    infos.Push(Variant(day_));
    infos.Push(Variant(year_));
    infos.Push(Variant(worldtimerate_));
}

void WeatherManager::SetRainTime(int viewport, float hour, float delayinhour, int direction, int intensity)
{
#ifdef ACTIVE_WEATHEREFFECTS
    WeatherViewData& viewdata = weatherviewdatas_[viewport];

    viewdata.raintime_ = hour;
    // delay in msec
    viewdata.raindelay_ = delayinhour * 3600000.f;
    viewdata.raineffect_->SetDirection((float)direction);
    viewdata.rainintensity_ = intensity;

    viewdata.rtimer_.Reset();
#endif
}

void WeatherManager::SetSunLight(int viewport)
{
    Node* cameranode = ViewManager::Get()->GetCameraNode(viewport);
    if (!cameranode)
    {
        return;
    }

    String name = "SunLight" + String(viewport);

    Node* sunLightNode = cameranode->GetChild(name, LOCAL);
    if (!sunLightNode)
    {
        sunLightNode = cameranode->CreateChild(name, LOCAL);
        sunLightNode->SetPosition(Vector3(0.f, 15.f, 0.f));
    }

    WeatherViewData& viewdata = weatherviewdatas_[viewport];
    if (!viewdata.sunlight_)
    {
        viewdata.sunlight_ = sunLightNode->GetOrCreateComponent<Light>();
        viewdata.sunlight_->SetLightType(LIGHT_POINT);
        viewdata.sunlight_->SetRange(75.f);
        viewdata.sunlight_->SetPerVertex(true);
        viewdata.sunlight_->SetColor(Color::WHITE);
        viewdata.sunlight_->SetOccludee(false);
        viewdata.sunlight_->SetViewMask(VIEWPORTSCROLLER_OUTSIDE_MASK << viewport);

        URHO3D_LOGINFOF("WeatherManager() - SetSunLight : viewport=%d sunlight node=%s(%u) ... ", viewport, viewdata.sunlight_->GetNode()->GetName().CString(), viewdata.sunlight_->GetNode()->GetID());
    }
}

void WeatherManager::SetEffect(int viewport, WeatherEffect effect, int intensity)
{
//    URHO3D_LOGINFOF("WeatherManager() - SetEffect : effect=%s intensity=%d",  weatherEffectNames[effect], intensity);

#ifdef ACTIVE_WEATHEREFFECTS

//    viewport = Random((int)ViewManager::Get()->GetNumViewports());

    WeatherViewData& viewdata = weatherviewdatas_[viewport];

    if (intensity)
    {
        if (effect == WeatherRain)
        {
            viewdata.rainintensity_ = intensity;

            if (viewdata.raineffect_)
            {
                viewdata.raineffect_->SetIntensity((float)intensity);

                if (!viewdata.raineffect_->IsStarted())
                    viewdata.raineffect_->Start();
            }
        }
        else if (effect == WeatherCloud)
        {
            viewdata.cloudintensity_ = intensity;

            if (viewdata.cloud_)
            {
#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
                if (!viewdata.cloud_->IsEnabled())
                    viewdata.cloud_->SetEnabledRecursive(true);
                ScrollingShape::SetIntensity((float)intensity);
#else
                if (viewdata.cloudeffect_)
                {
                    if (!viewdata.cloudeffect_->IsStarted())
                        viewdata.cloudeffect_->Start();
                    viewdata.cloudeffect_->SetIntensity((float)intensity);
                }
#endif
            }
        }
    }
    else
    {
        if (effect == WeatherRain)
        {
            viewdata.rainintensity_ = 0;

            if (viewdata.raineffect_ && viewdata.raineffect_->IsStarted())
            {
                viewdata.raineffect_->Stop();
                viewdata.raineffect_->SetIntensity(0.f);
                viewdata.raineffect_->SetDirection(viewdata.raineffect_->GetDirection()*-1.f);
            }
        }
        else if (effect == WeatherCloud)
        {
            viewdata.cloudintensity_ = 0;

            if (viewdata.cloud_)
            {
#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
                viewdata.cloud_->SetEnabledRecursive(false);
#else
                if (viewdata.cloudeffect_ && viewdata.cloudeffect_->IsStarted())
                    viewdata.cloudeffect_->Stop();
#endif
            }
        }
    }
#endif
}

void WeatherManager::Start()
{
    luminariesOffset_ = 0.5f * World2D::GetExtendedVisibleRect(0).Size().x_;

    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (int viewport=0; viewport < numviewports; viewport++)
    {
        WeatherViewData& viewdata = weatherviewdatas_[viewport];

        viewdata.active_ = true;

        if (luminariesOffset_ > 0.f)
        {
            if (viewdata.sun_)
            {
                viewdata.sun_->SetPosition2D(Vector2(0.f, -0.7 * luminariesOffset_));
                viewdata.sun_->SetEnabled(true);
            }
            if (viewdata.moon_)
            {
                viewdata.moon_->SetPosition2D(Vector2(0.f, 0.7f * luminariesOffset_));
                viewdata.moon_->SetEnabled(true);
            }
        }
        else
        {
            if (viewdata.sun_)
                viewdata.sun_->SetEnabled(false);
            if (viewdata.moon_)
                viewdata.moon_->SetEnabled(false);
        }

        SetSunLight(viewport);

#ifdef ACTIVE_WEATHEREFFECTS
        if (viewdata.cloud_)
        {
#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
            viewdata.cloud_->SetEnabledRecursive(false);
#else
            if (viewdata.cloudeffect_)
            {
                viewdata.cloudeffect_->SetNodeToFollow(ViewManager::Get()->GetCameraNode(viewport));
                viewdata.cloudeffect_->Start();
            }
#endif
        }
        GameRand& ORand = GameRand::GetRandomizer(WEATHERRAND);
        SetRainTime(viewport, (float)ORand.Get(0, 24), (float)ORand.Get(1, 5), ORand.Get(100) < 50 ? 1 : -1, ORand.Get(2, 7));
#endif
    }

    wtimer_.Reset();

    if (GameContext::Get().gameConfig_.enlightScene_)
        GetSubsystem<Renderer>()->SetMaterialQuality(QUALITY_MEDIUM);
    else
        GetSubsystem<Renderer>()->SetMaterialQuality(QUALITY_LOW);

    SubscribeToEvent(GameContext::Get().rootScene_, E_SCENEUPDATE, URHO3D_HANDLER(WeatherManager, HandleSceneUpdate));
    SubscribeToEvent(WORLD_CAMERACHANGED, URHO3D_HANDLER(WeatherManager, HandleCameraPositionChanged));
}

void WeatherManager::Stop()
{
    UnsubscribeFromAllEvents();

    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (int viewport=0; viewport < numviewports; viewport++)
    {
        WeatherViewData& viewdata = weatherviewdatas_[viewport];

        viewdata.active_ = false;

        if (viewdata.sun_)
            viewdata.sun_->SetEnabled(false);

        if (viewdata.moon_)
            viewdata.moon_->SetEnabled(false);

#ifdef ACTIVE_WEATHEREFFECTS
        if (viewdata.raineffect_->IsStarted())
            viewdata.raineffect_->Stop();

        if (viewdata.cloud_)
        {
#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
            viewdata.cloud_->SetEnabledRecursive(false);
#else
            if (viewdata.cloudeffect_ && viewdata.cloudeffect_->IsStarted())
                viewdata.cloudeffect_->Stop();
#endif
        }
#endif
    }
}

void WeatherManager::SetActive(int viewport, bool state, bool forced)
{
    if (viewport == -1)
    {
        active_ = state;

        unsigned numviewports = ViewManager::Get()->GetNumViewports();
        for (viewport=0; viewport < numviewports; viewport++)
        {
            WeatherViewData& viewdata = weatherviewdatas_[viewport];

            viewdata.active_ = state;

            if (!viewdata.sunlight_)
                SetSunLight(viewport);

            if (viewdata.sun_ && viewdata.sun_->IsEnabled() != state)
                viewdata.sun_->SetEnabled(state);

            if (viewdata.moon_ && viewdata.moon_->IsEnabled() != state)
                viewdata.moon_->SetEnabled(state);

#ifdef ACTIVE_WEATHEREFFECTS
            if (viewdata.rain_->IsEnabled() != state)
                viewdata.rain_->SetEnabledRecursive(state);

#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
            if (viewdata.cloud_->GetChild(0U)->IsEnabled() != state)
                viewdata.cloud_->SetEnabledRecursive(state);
#else
            if (viewdata.cloudeffect_)
            {
                if (state)
                    viewdata.cloudeffect_->Start(forced);
                else if (!state)
                    viewdata.cloudeffect_->Stop(forced);
            }
#endif
#endif
            URHO3D_LOGERRORF("WeatherManager() - SetActive : viewport=%d state=%s sunlight=%s(%s)... OK !",
                             viewport, state?"true":"false", viewdata.sunlight_ ? viewdata.sunlight_->GetNode()->GetName().CString() : "none",
                             viewdata.sunlight_ && viewdata.sunlight_->IsEnabled() ? "true":"false");
        }
    }
    else
    {
        WeatherViewData& viewdata = weatherviewdatas_[viewport];

        if (viewdata.active_ != state || forced)
        {
            URHO3D_LOGERRORF("WeatherManager() - SetActive : viewport=%d state=%s ...", viewport, state?"true":"false");

            if (!viewdata.sunlight_)
                SetSunLight(viewport);

            if (viewdata.sun_ && viewdata.sun_->IsEnabled() != state)
                viewdata.sun_->SetEnabled(state);

            if (viewdata.moon_ && viewdata.moon_->IsEnabled() != state)
                viewdata.moon_->SetEnabled(state);

#ifdef ACTIVE_WEATHEREFFECTS
            if (viewdata.rain_->IsEnabled() != state)
                viewdata.rain_->SetEnabledRecursive(state);

#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
            if (viewdata.cloud_->GetChild(0U)->IsEnabled() != state)
                viewdata.cloud_->SetEnabledRecursive(state);
#else
            if (viewdata.cloudeffect_)
            {
                if (state)
                    viewdata.cloudeffect_->Start(forced);
                else if (!state)
                    viewdata.cloudeffect_->Stop(forced);
            }
#endif
#endif
            viewdata.active_ = state;

            URHO3D_LOGERRORF("WeatherManager() - SetActive ... OK !");
        }
    }
}

// Master Active/Desactive over pass all viewport active states
void WeatherManager::ToggleStartStop()
{
    SetActive(-1, !active_);
}

void WeatherManager::UpdateTimePeriod()
{
    if (worldtime_ < NIGHTHOURMAX || worldtime_ > NIGHTHOURMIN)
    {
        timeperiod_ = TIME_NIGHT;
        GameContext::Get().luminosity_ = 0.2f;
    }
    else if (worldtime_ >= NIGHTHOURMAX && worldtime_ <= DAYHOURMIN)
    {
        timeperiod_ = TIME_DAWN;
        GameContext::Get().luminosity_ = 0.2f + 0.8f * (worldtime_ - NIGHTHOURMAX) / (DAYHOURMIN - NIGHTHOURMAX);
    }
    else if (worldtime_ > DAYHOURMIN && worldtime_ < DAYHOURMAX)
    {
        timeperiod_ = TIME_DAY;
        GameContext::Get().luminosity_ = 1.f;
    }
    else if (worldtime_ >= DAYHOURMAX && worldtime_ <= NIGHTHOURMIN)
    {
        timeperiod_ = TIME_TWILIGHT;
        GameContext::Get().luminosity_ = 1.f - 0.8f * (worldtime_ - DAYHOURMAX) / (NIGHTHOURMIN - DAYHOURMAX);
    }
}

void WeatherManager::Update()
{
    timeDebugText_->SetVisible(GameContext::Get().DrawDebug_);

    // get time ellapsed in seconds
    float time = (float)wtimer_.GetMSec(true) / 1000.f;

    // convert in hour
    time /= 3600.f;

    // Update World Time
    worldtime_ += time * worldtimerate_;
    if (worldtime_ > 24.f)
    {
        day_++;
        worldtime_ = startworldtime_ = 0.f;
        wtimer_.Reset();

        if (day_ > 365)
        {
            year_++;
            day_ = 0;
        }
    }

    UpdateTimePeriod();

    // Send Time Period Event
    if (timeperiodsent_ != timeperiod_)
    {
        if (timeperiod_ == TIME_DAWN && GameContext::Get().luminosity_ > ENLIGHTTHRESHOLD)
        {
            timeperiodsent_ = timeperiod_;
            URHO3D_LOGINFOF("WeatherManager() - Update : Dawn !");
            SendEvent(WEATHER_DAWN);
        }
        else if (timeperiod_ == TIME_TWILIGHT && GameContext::Get().luminosity_ < ENLIGHTTHRESHOLD)
        {
            timeperiodsent_ = timeperiod_;
            URHO3D_LOGINFOF("WeatherManager() - Update : Twilight !");
            SendEvent(WEATHER_TWILIGHT);
        }
    }

    if (!active_)
        return;

    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (int viewport=0; viewport < numviewports; viewport++)
    {
        // Desactive if in fullbackground
        SetActive(viewport, !World2D::IsVisibleRectInFullBackGround(viewport));

        WeatherViewData& viewdata = weatherviewdatas_[viewport];
        if (!viewdata.active_)
            continue;

        // Update luminaries position
        viewdata.luminaries_->SetRotation2D(worldtime_ * HOURTODEG);
        viewdata.luminaries_->SetPosition2D(ViewManager::Get()->GetCameraNode(viewport)->GetPosition2D() - Vector2(0.f, 0.5f * luminariesOffset_));

        // Update SunLight / Inactived during night
        if (viewdata.sunlight_)
        {
            bool enabled = GameContext::Get().gameConfig_.enlightScene_ && GameContext::Get().luminosity_ > 0.2f && ViewManager::Get()->GetCurrentViewZ(viewport) != INNERVIEW;

            if (viewdata.sunlight_->IsEnabled() != enabled)
            {
                URHO3D_LOGINFOF("WeatherManager() - Update : viewport=%d sunlight node=%s(%u) ... enabled=%s", viewport, viewdata.sunlight_->GetNode()->GetName().CString(), viewdata.sunlight_->GetNode()->GetID(), enabled ? "true":"false");
                viewdata.sunlight_->SetEnabled(enabled);
            }

            if (enabled)
            {
                viewdata.sunlight_->SetBrightness(GameContext::Get().luminosity_);
                viewdata.sunlight_->SetRange(GameContext::Get().luminosity_ * 75.f);
            }
        }

#ifdef ACTIVE_WEATHEREFFECTS
        // Check for viewports intersections
        // if intersect synchronize weather effects
        if (viewport < numviewports-1)
        {
            Rect viewrect = ViewManager::Get()->GetViewRect(viewport);

            for (int otherviewport=viewport+1; otherviewport < numviewports; otherviewport++)
            {
                if (ViewManager::Get()->GetViewRect(otherviewport).IsInside(viewrect) != OUTSIDE)
                {
                    WeatherViewData& otherviewdata = weatherviewdatas_[otherviewport];

                    // copy rain effect
                    otherviewdata.raintime_ = viewdata.raintime_;
                    otherviewdata.raindelay_ = viewdata.raindelay_;
                    otherviewdata.raineffect_->SetDirection(viewdata.raineffect_->GetDirection());
                    otherviewdata.rainintensity_ = viewdata.rainintensity_;
                    otherviewdata.rtimer_.SetStartTime(viewdata.rtimer_.GetStartTime());
                    if (viewdata.raineffect_->IsStarted())
                        otherviewdata.raineffect_->Start();
                    else
                        otherviewdata.raineffect_->Stop();

                    // copy cloud effect
                    otherviewdata.cloudeffect_->SetIntensity(viewdata.cloudeffect_->GetIntensity());
                    if (viewdata.cloudeffect_->IsStarted())
                        otherviewdata.cloudeffect_->Start();
                    else
                        otherviewdata.cloudeffect_->Stop();
                }
            }
        }

        // Update Rain Delay
        if (viewdata.raineffect_)
        {
            if (viewdata.raineffect_->IsStarted())
            {
                float time = viewdata.rtimer_.GetMSec(false) * worldtimerate_;
                if (time < viewdata.raindelay_)
                {
                    if (time < viewdata.raindelay_ * 0.25f || time > viewdata.raindelay_ * 0.75f)
                    {
                        float newintensity;

                        if (time < viewdata.raindelay_ * 0.25f)
                        {
                            // increase rain
                            newintensity = (float)viewdata.rainintensity_ * time / (viewdata.raindelay_ * 0.25f);
                            viewdata.raineffect_->SetIntensity(newintensity);
                        }
                        else
                        {
                            // decrease rain
                            newintensity = (float)viewdata.rainintensity_ * (1.f - (time - viewdata.raindelay_ * 0.75f) / (0.25f * viewdata.raindelay_));
                            viewdata.raineffect_->SetIntensity(newintensity);
                        }

                        if (viewdata.cloud_)
                        {
                            // set for cloud
                            newintensity = -1.f * Clamp(0.5f * newintensity, 0.5f, 5.f) * viewdata.raineffect_->GetDirection();

#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
                            if (!viewdata.cloud_->IsEnabled())
                                viewdata.cloud_->SetEnabledRecursive(true);

                            ScrollingShape::SetIntensity(newintensity);
#else
                            if (viewdata.cloudeffect_)
                            {
                                if (!viewdata.cloudeffect_->IsStarted())
                                    viewdata.cloudeffect_->Start();

                                viewdata.cloudeffect_->SetIntensity(newintensity);
                            }
#endif
                        }
                    }
                }
                else
                {
                    // Stop Rain
                    viewdata.raineffect_->Stop();

                    // next rain time
                    GameRand& ORand = GameRand::GetRandomizer(WEATHERRAND);
                    SetRainTime(viewport, (float)ORand.Get(0, 24), (float)ORand.Get(1, 5), ORand.Get(100) < 50 ? 1 : -1, ORand.Get(2, 7));
                }
            }
            else
            {
                float delta = worldtime_ - viewdata.raintime_;
                if (delta >= 0.f && delta < 0.5f)
                {
                    // Start Rain
                    viewdata.raineffect_->Start();
                    viewdata.rtimer_.Reset();
                }

                // reset progressively the cloud intensity to default value (1.f)
                if (viewdata.cloud_)
                {
#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
                    float newintensity = ScrollingShape::GetIntensity();
#else
                    float newintensity = viewdata.cloudeffect_ ? viewdata.cloudeffect_->GetIntensity() : 0.f;
#endif
                    float valueabs = Abs(newintensity);

                    if (valueabs != 1.f)
                    {
                        float sign = newintensity / valueabs;
                        newintensity -= 0.5f * sign * (valueabs - 1.f);
                        valueabs = Abs(newintensity);
                        if (valueabs > 0.9f && valueabs < 1.1f)
                            newintensity = sign;
#if defined(ACTIVE_CLOUDEFFECTS_SCROLLINGSHAPE)
                        ScrollingShape::SetIntensity(newintensity);
#else
                        if (viewdata.cloudeffect_)
                            viewdata.cloudeffect_->SetIntensity(newintensity);
#endif
                    }
                }
            }
        }
#endif
    }
}

void WeatherManager::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    WeatherViewData& viewdata = weatherviewdatas_[0];

#ifdef ACTIVE_WEATHEREFFECTS
//    if (viewdata.raineffect_ && viewdata.raineffect_->IsStarted() && !GameContext::Get().gameConfig_.debugSprite2D_)
//        viewdata.raineffect_->DrawDebugGeometry(debug, depthTest);
#endif

    // DrawDebug Time
    {
        String text;
        text.AppendWithFormat("debug tile cutting=%d\n", ObjectTiled::GetCuttingLevel());
        text.AppendWithFormat("\nday %d - year %d\n", day_, year_);
        text.AppendWithFormat("%s %Fh (speed=%f)\n", timePeriodNames_[timeperiod_], worldtime_, worldtimerate_);
#ifdef ACTIVE_WEATHEREFFECTS
        if (viewdata.raineffect_->IsStarted())
        {
            text.AppendWithFormat("rainintensity %F/%u\n", viewdata.raineffect_->GetIntensity(), viewdata.rainintensity_);
            text.AppendWithFormat("raindirection %f\n", viewdata.raineffect_->GetDirection());
            text.AppendWithFormat("rainduration %F/%Fh\n", (float)viewdata.rtimer_.GetMSec(false) / 3600000.f * worldtimerate_, viewdata.raindelay_ / 3600000.f);
        }
        else
        {
            text.AppendWithFormat("rain at %Fh\n", viewdata.raintime_);
        }
#endif
        timeDebugText_->SetText(text);
        timeDebugText_->SetVisible(true);
    }

//    unsigned numviewports = ViewManager::Get()->GetNumViewports();
//    for (int viewport=0; viewport < numviewports; viewport++)
//    {
//        if (weatherviewdatas_[viewport].sunlight_)
//            weatherviewdatas_[viewport].sunlight_->DrawDebugGeometry(debug, false);
//    }
}

void WeatherManager::DumpRain(int viewport) const
{
    weatherviewdatas_[viewport].raineffect_->Dump();
}

void WeatherManager::HandleSceneUpdate(StringHash eventType, VariantMap& eventData)
{
    Update();
}

void WeatherManager::HandleCameraPositionChanged(StringHash eventType, VariantMap& eventData)
{
    int viewport = eventData[World_CameraChanged::VIEWPORT].GetInt();

    if (viewport == -1)
    {
        unsigned numviewports = ViewManager::Get()->GetNumViewports();
        for (int viewport=0; viewport < numviewports; viewport++)
        {
            // Desactive if in fullbackground
            World2D::GetWorld()->UpdateVisibleRectInfos(viewport);
            bool infullbackground = World2D::IsVisibleRectInFullBackGround(viewport);

            URHO3D_LOGERRORF("WeatherManager() - HandleCameraPositionChanged : viewport=%d fullbackground=%s ... ", viewport, infullbackground?"true":"false");

            SetActive(viewport, !infullbackground, true);
        }
    }
    else
    {
        // Desactive if in fullbackground
        World2D::GetWorld()->UpdateVisibleRectInfos(viewport);
        bool infullbackground = World2D::IsVisibleRectInFullBackGround(viewport);

        URHO3D_LOGERRORF("WeatherManager() - HandleCameraPositionChanged : viewport=%d fullbackground=%s ... ", viewport, infullbackground?"true":"false");

        SetActive(viewport, !infullbackground, true);
    }

    URHO3D_LOGERRORF("WeatherManager() - HandleCameraPositionChanged !");
}
