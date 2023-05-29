#pragma once


#include <Urho3D/Scene/Component.h>


namespace Urho3D
{
class Node;
class Component;
}

using namespace Urho3D;


class GEF_NodeShaker : public Component
{
    URHO3D_OBJECT(GEF_NodeShaker, Component);

public:
    GEF_NodeShaker(Context* context);
    ~GEF_NodeShaker();

    static void RegisterObject(Context* context);

    void SetNumShakes(int numshakes);
    void SetDuration(float duration);
    void SetAmplitude(const Vector2& aplitude);

    void Start();

//    virtual void OnSetEnabled();

private :
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    int ishake_, numShakes_;
    float duration_, delay_;
    float shakedelay_;
    Vector2 amplitude_;
};
