#pragma once

#include <Urho3D/Scene/Component.h>

using namespace Urho3D;


class GOC_EntityFollower : public Component
{
    URHO3D_OBJECT(GOC_EntityFollower, Component);

public :
    GOC_EntityFollower(Context* context);
    virtual ~GOC_EntityFollower();

    static void RegisterObject(Context* context);

    void SetNodeToFollow(Node* node);
    void SetSelected(bool enable) { selected_ = enable; }

    Node* GetFollowerNode() { return nodeToFollow_; }
    bool IsSelected() const { return selected_; }

    void Dump();

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void UpdateProperties();

    void OnDirty(StringHash eventType, VariantMap& eventData);
    virtual void OnMarkedDirty(Node* node);

    WeakPtr<Node> nodeToFollow_;
    bool selected_;
};


