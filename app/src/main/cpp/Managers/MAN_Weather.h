#pragma once

#include <Urho3D/Core/Object.h>

namespace Urho3D
{
class Node;
class Light;
}

using namespace Urho3D;

#include "DefsViews.h"

enum
{
    TIME_NIGHT = 0,
    TIME_DAWN,
    TIME_DAY,
    TIME_TWILIGHT
};

enum WeatherEffect
{
    WeatherNone = 0,
    WeatherRain,
    WeatherCloud,
};

class GEF_Rain;
class GEF_Scrolling;

class WeatherManager : public Object
{
    URHO3D_OBJECT(WeatherManager, Object);

public :
    WeatherManager(Context* context);
    virtual ~WeatherManager();

    static void RegisterObject(Context* context);
    static WeatherManager* Get()
    {
        return weatherManager_;
    }

    void SetWorldTime(float time, unsigned day=0, unsigned year=0);
    void SetWorldTimeRate(float rate = 0.f);
    void ResetTimePeriod();

    void SetRainTime(int viewport, float hour, float delayinhour=3.f, int direction=1, int intensity=3);
    void SetSunLight(int viewport);
    void SetEffect(int viewport, WeatherEffect effect, int intensity);

    float GetWorldTime() const
    {
        return worldtime_;
    }
    int GetWorldTimePeriod() const
    {
        return timeperiod_;
    }
    float GetWorldTimeRate() const
    {
        return worldtimerate_;
    }

    int GetRainIntensity(int viewport=0) const
    {
        return weatherviewdatas_[viewport].rainintensity_;
    }
    int GetCloudIntensity(int viewport=0) const
    {
        return weatherviewdatas_[viewport].cloudintensity_;
    }

    // Net usage
    void SetNetWorldInfos(const VariantVector& infos);
    void GetNetWorldInfos(VariantVector& infos);

    void DumpRain(int viewport=0) const;

    void Start();
    void Stop();

    void ToggleStartStop();
    void Active(int viewport=-1, bool state=true, bool forced=false);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

private :
    void Init();

    void UpdateTimePeriod();
    void Update();

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);
    void HandleCameraPositionChanged(StringHash eventType, VariantMap& eventData);

    // time data
    int timeperiod_;
    int day_, year_;

    float worldtime_, worldtimerate_;
    float startworldtime_;
    Timer wtimer_;

    float luminariesOffset_;

    struct WeatherViewData
    {
        void Clear();

        bool active_;

        // rain data
        Timer rtimer_;
        float raintime_, raindelay_;
        int rainintensity_;

        // cloud data
        int cloudintensity_;

        // nodes
        WeakPtr<Node> luminaries_;
        WeakPtr<Node> sun_;
        WeakPtr<Node> moon_;
        WeakPtr<Node> rain_;
        WeakPtr<Node> cloud_;

        // effect components
        WeakPtr<Light> sunlight_;
        WeakPtr<GEF_Rain> raineffect_;
        WeakPtr<GEF_Scrolling> cloudeffect_;
    };

    WeatherManager::WeatherViewData weatherviewdatas_[MAX_VIEWPORTS];

    bool active_;

    static WeatherManager* weatherManager_;
};


