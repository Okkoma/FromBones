#pragma once


using namespace Urho3D;


class GOC_Portal : public Component
{
    URHO3D_OBJECT(GOC_Portal, Component);

public :
    GOC_Portal(Context* context);
    ~GOC_Portal();
    static void RegisterObject(Context* context);

    void SetDestinationMap(const ShortIntVector2& map);
    void SetDestinationMap(const IntVector2& map);
    void SetDestinationPosition(const IntVector2& position);
    void SetDestinationViewZ(int viewz);

    bool IsDestinationDefined() const;
    const IntVector2& GetDestinationMap() const
    {
        return dMap_;
    }
    const IntVector2& GetDestinationPosition() const
    {
        return dPosition_;
    }
    int GetDestinationViewZ() const
    {
        return dViewZ_;
    }

    virtual void OnSetEnabled();

private :
    virtual void OnNodeSet(Node* node);

    void Desactive();
    void Teleport(Node* node);

    void HandleAppear(StringHash eventType, VariantMap& eventData);
    void HandleBeginContact(StringHash eventType, VariantMap& eventData);
    void HandleReactivePortal(StringHash eventType, VariantMap& eventData);
    void HandleApplyDestination(StringHash eventType, VariantMap& eventData);
    void HandleTransferBodies(StringHash eventType, VariantMap& eventData);

    IntVector2 dMap_, dPosition_;
    int dViewZ_;

    bool transferOk_;
    int numTransferPeriods_;

    Vector<int> dViewports_;
    struct TeleportInfo
    {
        unsigned nodeid_;
        IntVector2 map_;
        IntVector2 position_;
        int viewZ_;
    };
    Vector<TeleportInfo> teleportedInfos_;
};
