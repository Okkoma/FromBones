#pragma once

#include <Urho3D/Core/Object.h>

namespace Urho3D
{
class Node;
class DebugRenderer;
}

using namespace Urho3D;

#include "DefsEffects.h"

class GOC_ZoneEffect;

class EffectsManager : public Object
{
    URHO3D_OBJECT(EffectsManager, Object);

public :
    EffectsManager(Context* context);
    virtual ~EffectsManager();

    static void RegisterObject(Context* context);

    void Start();
    void Stop();
    void Update(float timestep);

    static void SetEffectsOn(Node* sender, Node* receiver, const String& triggerName, const Vector2& point=Vector2::ZERO);
    static bool AddEffectOn(Node* sender, Node* receiver, EffectType* effect, const Vector2& point=Vector2::ZERO, bool applyinvulnerability=true, bool sendevent=true);
    static bool ApplyEffectOn(Node* sender, Node* receiver, EffectType* effect, const Vector2& point=Vector2::ZERO, bool applyinvulnerability=true);

    static void RemoveEffectOn(GOC_ZoneEffect* zone, Node* node, EffectType* effect);
    static void RemoveInstancesIn(GOC_ZoneEffect* zone);

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

private :
    void Init();

    void HandleSceneUpdate(StringHash eventType, VariantMap& eventData);

    static Vector<EffectInstance>::Iterator GetEffectInstanceFor(GOC_ZoneEffect* zone, Node* node, EffectType* effect);
    static Vector<EffectInstance>::Iterator RemoveInstance(Vector<EffectInstance>::Iterator instanceit);
    static Vector<EffectInstance> instances_;
    static Vector<EffectType> bufferEffects_;
    static unsigned ibufferEffects_;

    static EffectsManager* effectsManager_;
};

