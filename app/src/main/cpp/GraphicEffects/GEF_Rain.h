#pragma once


#include <Urho3D/Scene/Component.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/SpriterData2D.h>

namespace Urho3D
{
class Node;
class DebugRenderer;
class AnimationSet2D;
class AnimatedSprite2D;
}

using namespace Urho3D;

#include "TimerSimple.h"

struct Droplet
{
    void Set(Node* root, AnimationSet2D* animationSet, int viewport, unsigned x, unsigned y, unsigned layerIndex);
    void Reset();
    void Update(float timeStep);
    void Clear();

    Node* node_;
    AnimatedSprite2D* animatedSprite_;
    int viewport_;
    unsigned layerIndex_;
    int coordx_;
    int coordy_;
    Vector2 iPos_;
    Vector2 fPos_;
    Vector2 impact_;
    float time_;
    float maxtime_;
    Vector2 position_;

    static PhysicsWorld2D* physicsWorld_;
    static PhysicsRaycastResult2D rayresult_;
    static float intensity_[MAX_VIEWPORTS];
    static Spriter::Animation* animationHit_;
    static Vector<unsigned> hittedTiles_;
    static const unsigned NUM_SCREENPARTS;
    static const unsigned NUM_DROPLET_ROW;
    static const unsigned NUM_DROPLET_COL;
    static const unsigned NUM_DROPLETS;
    static const float DROPLET_OFFSETMINY;
    static const float DROPLET_ANGLE;
    static const float DROPLET_MINTIME;
    static const float DROPLET_MAXTIME;
    static const float DROPLET_DISPERSION; // % de dispersion sur positions des droplets
};

class GEF_Rain : public Component
{
    URHO3D_OBJECT(GEF_Rain, Component);

public :
    GEF_Rain(Context* context);
    virtual ~GEF_Rain();

    static void RegisterObject(Context* context);

    void Start();
    void Stop();

    void SetViewport(int viewport)
    {
        viewport_ = viewport;
    }
    void SetAnimationSet(const String& filename);
    void SetIntensity(float intensity);
    void SetDirection(float dir);

    const String& GetAnimationSet() const
    {
        return filename_;
    }
    float GetIntensity() const
    {
        return intensity_;
    }
    float GetDirection() const
    {
        return incIntensity_;
    }
    bool IsStarted() const
    {
        return started_;
    }

    virtual void OnSetEnabled();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

private :
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    String filename_;

    PODVector<Droplet> droplets_;
    TimerSimple raintimer_;
    unsigned iCount_;

    int viewport_;
    float intensity_, incIntensity_;
    bool started_;
    bool dropletsSetted_;
    bool paused_;
};


