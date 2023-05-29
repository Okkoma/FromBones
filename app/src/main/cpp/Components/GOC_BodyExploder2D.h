#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
struct SpriteInfo;
class AnimatedSprite2D;
}

using namespace Urho3D;

class GOC_BodyExploder2D : public Component
{
    URHO3D_OBJECT(GOC_BodyExploder2D, Component);

public :
    GOC_BodyExploder2D(Context* context);
    virtual ~GOC_BodyExploder2D();

    static void RegisterObject(Context* context);

    void SetShapeRatioXY(float csRatio);
    void SetShapeType(int cstype);
    void SetTrigEvent(const String& s);
    void SetTriggerEnable(bool state);
    void SetToPrepare(bool state);
    void SetHideOriginalBody(bool state);
    void SetCollectablePartType(const String& s);
    void SetScrapPartType(const String& s);
    void SetPartsLifeTime(float time);
    void SetInitialImpulse(float impulse);
    void SetUseScrapsEmitter(bool use);
    void SetUseObjectPool(bool state);

    float GetShapeRatioXY() const
    {
        return csRatioXY_;
    }
    int GetShapeType() const
    {
        return csType_;
    }
    const String& GetTrigEvent() const
    {
        return trigEventString_;
    }
    bool GetToPrepare() const
    {
        return toPrepare_;
    }
    bool GetHideOriginalBody() const
    {
        return hideOriginalBody_;
    }
    const String& GetCollectablePartType() const
    {
        return collectablePartTypeName_;
    }
    const String& GetScrapPartType() const
    {
        return scrapPartTypeName_;
    }
    float GetPartsLifeTime() const
    {
        return partLifeTime_;
    }
    float GetInitialImpulse() const
    {
        return initialImpulse_;
    }
    bool GetUseScrapsEmitter() const
    {
        return useScrapsEmitter_;
    }
    bool GetUseObjectPool() const
    {
        return useObjectPool_;
    }

    virtual void ApplyAttributes();

    virtual void OnSetEnabled();

    void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

private :
    void OnTrigEvent(StringHash eventType, VariantMap& eventData);
    void HandleWaitStateForHide(StringHash eventType, VariantMap& eventData);

    bool SetStateToExplode(AnimatedSprite2D* animatedSprite, String& restoreAnimation);
    void PrepareExplodedNodes(AnimatedSprite2D* animatedSprite);
    void PreparePhysics();
    void TransferExplodedNodesPositionsTo(Node* rootNode);
    void SetExplodedNodesComponents();

    void PrepareNodes(AnimatedSprite2D* animatedSprite);
    void GenerateNodes(AnimatedSprite2D* animatedSprite);

    void UpdatePositions(Node* rootNode);
    void Explode();

    WeakPtr<Node> prepareNode_;
    Vector<WeakPtr<Node> > explodedNodes_;
    unsigned numExplodedNodes_;

    Vector2 initialPosition_;
    Vector2 initialScale_;
    float initialRotation_;

    float impulseTimer_;
    Vector2 impactPoint_;
    bool flipX_, flipY_;
    int csType_;
    float csRatioXY_;
    float partLifeTime_;
    float initialImpulse_;

    String trigEventString_;
    String collectablePartTypeName_, scrapPartTypeName_;

    StringHash partType_, collectablePartType_, scrapPartType_;
    StringHash trigEvent_;

    bool toPrepare_;
    bool prepared_;
    bool hideOriginalBody_;
    bool useScrapsEmitter_;
    bool useObjectPool_;
};


