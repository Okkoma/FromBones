#pragma once

#include <Urho3D/Scene/Component.h>

#include <Urho3D/Urho2D/Sprite2D.h>

namespace Urho3D
{
class Context;
class Node;
}

using namespace Urho3D;


class ScrapsEmitter : public Component
{
    URHO3D_OBJECT(ScrapsEmitter, Component);

public :
    ScrapsEmitter(Context* context);
    virtual ~ScrapsEmitter();

    static void RegisterObject(Context* context);

    static void RegisterType(Context* context, const String& typeStr);
    void RegisterType(const String& typeStr);
    static void RegisterType(Context* context, const String& typeStr, int terrainid);
    void RegisterType(const String& typeStr, int terrainid);

    void SetScrapsType(const String& typeName);
    void SetScrapsType(const StringHash& type);
    void SetSize(const IntVector2& size);
    void SetNumScraps(int num);
    void SetTrigEvent(const String& s);
    void SetTrigEvent(const StringHash& hash);
    void SetLifeTime(float time);
    void SetInitialImpulse(float impulse);

    const String& GetScrapsTypeName() const
    {
        return typeNames_.Contains(type_) ? typeNames_[type_] : String::EMPTY;
    }
    const IntVector2& GetSize() const
    {
        return size_;
    }
    int GetNumScraps() const
    {
        return numScraps_;
    }
    const String& GetTrigEvent() const
    {
        return trigEventString_;
    }
    float GetLifeTime() const
    {
        return lifeTime_;
    }
    float GetInitialImpulse() const
    {
        return initialImpulse_;
    }

    static const Vector<WeakPtr<Sprite2D> >& GetScrapSpriteList(const StringHash& type)
    {
        return spriteLists_[type];
    }

    virtual void ApplyAttributes();

    virtual void OnSetEnabled();

private :
    void OnTrigEvent(StringHash eventType, VariantMap& eventData);

    StringHash type_;
    StringHash trigEvent_;
    String trigEventString_;
    unsigned numScraps_;
    float lifeTime_;
    float density_;
    float initialImpulse_;
    IntVector2 size_;
    bool isCollider_;
    bool destroyNode_;

    static HashMap<StringHash, String > typeNames_;
    static HashMap<StringHash, Vector<WeakPtr<Sprite2D> > > spriteLists_;
};

