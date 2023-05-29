#pragma once

#include <Urho3D/Scene/Component.h>


namespace Urho3D
{
class Node;
class RigidBody2D;
class StaticSprite2D;
}

using namespace Urho3D;

class GOC_Move2D;

class GOC_StaticRope : public Component
{
    URHO3D_OBJECT(GOC_StaticRope, Component);

public :
    GOC_StaticRope(Context* context);
    virtual ~GOC_StaticRope();

    static void RegisterObject(Context* context);

    void SetAppearEventsAttr(const String& value);
    const String& GetAppearEventsAttr() const;
    void SetBreakEventsAttr(const String& value);
    const String& GetBreakEventsAttr() const;

    virtual void OnSetEnabled();

    void AttachOnRoof();
    void Break();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

protected :
    virtual void OnMarkedDirty(Node* node);

private :
    void SubscribeToEvents();
    void UnsubscribeFromEvents();

    void UpdateAttributes();
    bool UpdateRope();

    void OnAppearEvent(StringHash eventType, VariantMap& eventData);
    void OnBreakEvent(StringHash eventType, VariantMap& eventData);

    Node* parent_;
    GOC_Move2D* gocMove_;
    StaticSprite2D* ropeDrawable_;
    String appearEventStr_, breakEventStr_;
    Vector2 anchor_;
    float minLength_, maxLength_;
    bool attached_, dirtyreentrance_;
    Rect initialRopeRect_;
    Vector<StringHash> appearEvents_;
    Vector<StringHash> breakEvents_;
};


