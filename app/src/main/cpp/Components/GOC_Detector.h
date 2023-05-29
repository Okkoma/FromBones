#pragma once


namespace Urho3D
{
class RigidBody2D;
class CollisionShape2D;
struct ContactInfo;
}

using namespace Urho3D;


class GOC_Detector : public Component
{
    URHO3D_OBJECT(GOC_Detector, Component);

public :
    GOC_Detector(Context* context);
    virtual ~GOC_Detector();

    static void RegisterObject(Context* context);
    void SetTriggerEventInAttr(const String& eventname);
    const String& GetTriggerEventInAttr() const;
    void SetTriggerEventOutAttr(const String& eventname);
    const String& GetTriggerEventOutAttr() const;
    void SetDirectionOut(const IntVector2& dirOut);
    const IntVector2& GetDirectionOut() const
    {
        return directionOut_;
    }
    void SetSameViewOnly(bool enable);
    bool GetSameViewOnly() const
    {
        return viewDetector_;
    }
    void SetDetectedCategoriesAttr(const String& value);
    const String& GetDetectedCategoriesAttr() const
    {
        return detectedCategoriesStr_;
    }

    unsigned GetFirstNodeDetect() const
    {
        return detectedNodeIDs_.Empty() ? 0 : detectedNodeIDs_.Front();
    }
    bool Contains(unsigned id) const
    {
        return detectedNodeIDs_.Contains(id);
    }
    void RemoveDetectedNode(unsigned id)
    {
        detectedNodeIDs_.Erase(id);
    }
    void ClearDetectedNodes()
    {
        detectedNodeIDs_.Clear();
    }
    virtual void OnSetEnabled();
    virtual void CleanDependences();

    RigidBody2D* body_;

protected :
    virtual void OnNodeSet(Node* node);

private :
    void HandleContact(StringHash eventType, VariantMap& eventData);

    void BecomeStickOn(CollisionShape2D* targetshape, const ContactInfo& cinfo);

    HashSet<unsigned> detectedNodeIDs_;
    StringHash eventIn_;
    StringHash eventOut_;
    IntVector2 directionOut_;
    bool wallDetector_, viewDetector_, attackDetector_, stickTarget_;
    String detectedCategoriesStr_;
    Vector<StringHash> detectedCategories_;
    Node* target_;
};


