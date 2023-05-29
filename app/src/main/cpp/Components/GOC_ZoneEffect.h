#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
class RigidBody2D;
class CollisionCircle2D;
}

using namespace Urho3D;


#include "DefsEffects.h"


class GOC_ZoneEffect : public Component
{
    URHO3D_OBJECT(GOC_ZoneEffect, Component);

public :
    GOC_ZoneEffect(Context* context);
    virtual ~GOC_ZoneEffect();

    static void RegisterObject(Context* context);

    void RegisterEffectAttr(const String& effectStr);

    void SetActived(bool actived);
    void SetApplyToHolder(bool state);
    void SetHolder(unsigned id);
    void SetCenter(const Vector2& center);
    void SetRadius(float radius);
    void SetEffect(int effectid);
    void SetEffect(const String& effectName);
    void SetParticulesOnHolder(const String& effectName);

    void UseEffectOn(unsigned id);

    bool GetActived() const;
    const String& GetParticulesOnHolder() const;
    bool GetApplyToHolder() const;
    unsigned GetHolder() const;
    const Vector2& GetCenter() const;
    float GetRadius() const;
    String GetEffectAttr() const;
    int GetEffectID() const;
    const String& GetEffectName() const;
    const StringHash& GetEffectElt() const;
    const EffectType& GetEffect() const;

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void SetArea(bool enable);
    void UpdateAttributes();

    void HandleActive(StringHash eventType, VariantMap& eventData);
    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);
    void HandleBeginContact(StringHash eventType, VariantMap& eventData);
    void HandleTouchFluid(StringHash eventType, VariantMap& eventData);

    Vector2 center_;

    RigidBody2D* body_;
    CollisionCircle2D* zoneArea_;

    int effectID_;
    int particuleOnHolderID_;

    float radius_;
    unsigned holder_;

    bool actived_;
    bool appliedToHolder_;
};


