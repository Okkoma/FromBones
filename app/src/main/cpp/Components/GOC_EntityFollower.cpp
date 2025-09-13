#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_EntityFollower.h"


GOC_EntityFollower::GOC_EntityFollower(Context* context) :
    Component(context),
    selected_(false)
{
}

GOC_EntityFollower::~GOC_EntityFollower()
{
//    URHO3D_LOGDEBUG("~GOC_EntityFollower()");
}

void GOC_EntityFollower::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_EntityFollower>();
}

void GOC_EntityFollower::OnSetEnabled()
{
//    URHO3D_LOGINFOF("GOC_EntityFollower() - OnSetEnabled : nodeToFollow=%s(%u)", nodeToFollow_ ? nodeToFollow_->GetName().CString() : "none", nodeToFollow_ ? nodeToFollow_->GetID() : 0);

    UpdateProperties();
}

void GOC_EntityFollower::OnNodeSet(Node* node)
{

}

void GOC_EntityFollower::SetNodeToFollow(Node* node)
{
    if (!nodeToFollow_ || nodeToFollow_ != node)
    {
        if (nodeToFollow_ && node_ != nodeToFollow_)
        {
            nodeToFollow_->RemoveListener(this);
            UnsubscribeFromEvent(nodeToFollow_, GO_DIRTY);
        }

        nodeToFollow_ = node;

        if (nodeToFollow_)
        {
            if (node_ != nodeToFollow_)
                nodeToFollow_->AddListener(this);

            UpdateProperties();

            SubscribeToEvent(nodeToFollow_, GO_DIRTY, URHO3D_HANDLER(GOC_EntityFollower, OnDirty));
        }
        else
        {
            node_->SetEnabled(false);
        }
    }
}

void GOC_EntityFollower::UpdateProperties()
{
    if (!nodeToFollow_)
        return;

    Drawable2D* drawable = nodeToFollow_->GetDerivedComponent<Drawable2D>();
    if (drawable)
    {
        IntVector2 layer = drawable->GetLayer2();
        layer.x_--;
        PODVector<Drawable2D*> drawables;
        node_->GetDerivedComponents<Drawable2D>(drawables);
        for (unsigned i=0; i < drawables.Size(); i++)
            drawables[i]->SetLayer2(layer);

        const BoundingBox& bbox = drawable->GetWorldBoundingBox();
        Vector2 size = bbox.Size().ToVector2();
        node_->SetWorldScale2D(Vector2::ONE * 4.f * Max(size.x_, size.y_));
    }

    OnMarkedDirty(nodeToFollow_);
}

void GOC_EntityFollower::OnDirty(StringHash eventType, VariantMap& eventData)
{
    UpdateProperties();
}

void GOC_EntityFollower::OnMarkedDirty(Node* node)
{
    if (node)
    {
        node_->SetEnabled(IsEnabled() && node->IsEnabled());

        Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
        if (drawable)
            node_->SetWorldPosition2D(node->GetWorldPosition2D().x_, drawable->GetWorldBoundingBox().Center().y_);
    }
}


void GOC_EntityFollower::Dump()
{
    URHO3D_LOGINFOF("GOC_EntityFollower() - Dump : ...");


}
