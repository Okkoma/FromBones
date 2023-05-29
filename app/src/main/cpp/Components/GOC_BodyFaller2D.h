#pragma once

using namespace Urho3D;

class GOC_BodyFaller2D : public Component
{
    URHO3D_OBJECT(GOC_BodyFaller2D, Component);

public :
    GOC_BodyFaller2D(Context* context);
    virtual ~GOC_BodyFaller2D();

    static void RegisterObject(Context* context);

    void SetTrigEvent(const String& s);
    const String& GetTrigEvent() const
    {
        return trigEventString_;
    }

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void OnTrigEvent(StringHash eventType, VariantMap& eventData);

    void RemoveFixtures();
    void BecomeStatic();

    String trigEventString_;
    StringHash trigEvent_;
};


