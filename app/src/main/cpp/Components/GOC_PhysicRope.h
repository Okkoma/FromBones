#pragma once

using namespace Urho3D;

struct b2RevoluteJointDef;

enum RopeModel
{
    RM_ThrowableRope,
    RM_FixedRope,
    RM_FixedBridge,
};

class GOC_PhysicRope : public Component
{
    URHO3D_OBJECT(GOC_PhysicRope, Component);

public :
    GOC_PhysicRope(Context* context);
    ~GOC_PhysicRope();

    static void RegisterObject(Context* context);

    void SetModelAttr(RopeModel model);
    void SetHookNodeAttr(const StringHash& hashname);
    void SetAttachedNodeAttr(const StringHash& hashname);
    void SetSpritesAttr(const ResourceRefList& spritelist);

    RopeModel GetModelAttr() const;
    const StringHash& GetHookNodeAttr() const;
    const StringHash& GetAttachedNodeAttr() const;
    ResourceRefList GetSpritesAttr() const;

    void SetMaxLength(float length);
    void SetDefaultLength(float length);
    void SetAttachedNode(Node* node);
    void SetBridgeAnchors(const Vector2& anchor1, const Vector2& anchor2);

    void BreakContact();
    void Detach();
    void Release(float time=10.f);

    bool AttachOnRoof(const Vector2& anchorOnRoof, float rotation, const void* cinfo=0);
    bool AttachOnWalls();

    bool IsAttached() const
    {
        return isAttached_;
    }
    bool IsAnchorOnMap() const;

    Node* GetAttachedNode() const
    {
        return attachedNode_;
    }

    virtual void CleanDependences();
    virtual void OnSetEnabled();

    virtual void DrawDebugGeometry(DebugRenderer* debugRenderer, bool depthTest) const;

private :
    float CreateRope(const Vector2& startAnchor, const Vector2& endAnchor, int viewZ, int layer, int orderLayer, float align=0.5f, float spacing=0.5f);
    float CreateBridge(const Vector2& startAnchor, const Vector2& endAnchor, int viewZ, int layer, int orderLayer);
    void ResetBridgeElementsPositions();

    void HandleRoofCollision(StringHash eventType, VariantMap& eventData);
    void HandleBreakContact(StringHash eventType, VariantMap& eventData);

    RopeModel model_;

    bool isAttached_, isReleasing_;

    unsigned numLinks_;
    float lengthDefault_, lengthMax_;
    float softness_;

    StringHash hook_, attached_;

    WeakPtr<Node> attachedNode_;
    WeakPtr<Node> rootNode_;

    WeakPtr<Node> anchorNode1_;
    WeakPtr<Node> anchorNode2_;

    PODVector<Node* > chainNodes_;

    Vector2 anchorPosition1_, anchorPosition2_;
    ShortIntVector2 mapInContact1_, mapInContact2_;
    unsigned anchorTileIndex1_, anchorTileIndex2_;
    Vector<CollisionShape2D*> shapesInContact1_, shapesInContact2_;

    PODVector<Sprite2D*> sprites_;
};
