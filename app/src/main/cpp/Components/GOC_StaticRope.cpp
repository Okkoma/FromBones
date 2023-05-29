#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Core/StringUtils.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include "DefsColliders.h"

#include "GameAttributes.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameEvents.h"

#include "ViewManager.h"

#include "GOC_Move2D.h"

#include "GOC_StaticRope.h"


GOC_StaticRope::GOC_StaticRope(Context* context) :
    Component(context),
    parent_(0),
    gocMove_(0),
    ropeDrawable_(0),
    minLength_(0.5f),
    maxLength_(5.f),
    attached_(false),
    dirtyreentrance_(false)
{ }

GOC_StaticRope::~GOC_StaticRope()
{ }

void GOC_StaticRope::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_StaticRope>();
    URHO3D_ATTRIBUTE("Min Length", float, minLength_, 0.5f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Length", float, maxLength_, 5.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Appears on Parent Events", GetAppearEventsAttr, SetAppearEventsAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Breaks on Parent Events", GetBreakEventsAttr, SetBreakEventsAttr, String, String::EMPTY, AM_DEFAULT);
}

void GOC_StaticRope::SetAppearEventsAttr(const String& value)
{
    if (value != appearEventStr_)
    {
        for (Vector<StringHash>::ConstIterator it=appearEvents_.Begin(); it!=appearEvents_.End(); ++it)
            UnsubscribeFromEvent(*it);

        appearEventStr_ = value;
        appearEvents_.Clear();

        if (value.Empty())
            return;

        Vector<String> eventstr = value.Split(';');
        if (!eventstr.Size())
            return;

        for (Vector<String>::Iterator it = eventstr.Begin(); it != eventstr.End(); ++it)
        {
            appearEvents_.Push(StringHash(*it));
        }
    }
}

const String& GOC_StaticRope::GetAppearEventsAttr() const
{
    return appearEventStr_;
}

void GOC_StaticRope::SetBreakEventsAttr(const String& value)
{
    if (value != breakEventStr_)
    {
        for (Vector<StringHash>::ConstIterator it=breakEvents_.Begin(); it!=breakEvents_.End(); ++it)
            UnsubscribeFromEvent(*it);

        breakEventStr_ = value;
        breakEvents_.Clear();

        if (value.Empty())
            return;

        Vector<String> eventstr = value.Split(';');
        if (!eventstr.Size())
            return;

        for (Vector<String>::Iterator it = eventstr.Begin(); it != eventstr.End(); ++it)
        {
            breakEvents_.Push(StringHash(*it));
        }
    }
}

const String& GOC_StaticRope::GetBreakEventsAttr() const
{
    return breakEventStr_;
}

void GOC_StaticRope::OnSetEnabled()
{
    UpdateAttributes();
}

void GOC_StaticRope::SubscribeToEvents()
{
    for (Vector<StringHash>::ConstIterator it=appearEvents_.Begin(); it!=appearEvents_.End(); ++it)
        SubscribeToEvent(parent_, *it, URHO3D_HANDLER(GOC_StaticRope, OnAppearEvent));

    for (Vector<StringHash>::ConstIterator it=breakEvents_.Begin(); it!=breakEvents_.End(); ++it)
        SubscribeToEvent(parent_, *it, URHO3D_HANDLER(GOC_StaticRope, OnBreakEvent));
}

void GOC_StaticRope::UnsubscribeFromEvents()
{
    for (Vector<StringHash>::ConstIterator it=appearEvents_.Begin(); it!=appearEvents_.End(); ++it)
        UnsubscribeFromEvent(parent_, *it);

    for (Vector<StringHash>::ConstIterator it=breakEvents_.Begin(); it!=breakEvents_.End(); ++it)
        UnsubscribeFromEvent(parent_, *it);
}

void GOC_StaticRope::UpdateAttributes()
{
    parent_ = node_->GetParent();
    if (parent_->GetName() == "EntityAdderRoot")
        parent_ = parent_->GetParent();

    gocMove_ = parent_ ? parent_->GetComponent<GOC_Move2D>() : 0;

    ropeDrawable_ = node_->GetChild(0U)->GetDerivedComponent<StaticSprite2D>();

    // force breaking rope
    attached_ = true;
    Break();
    attached_ = false;

    if (GetScene() && IsEnabledEffective())
    {
        if (gocMove_)
        {
            if (ropeDrawable_)
            {
                ropeDrawable_->GetSprite()->GetDrawRectangle(initialRopeRect_, ropeDrawable_->GetHotSpot());
            }

            SubscribeToEvents();

//            URHO3D_LOGINFOF("GOC_StaticRope() - UpdateAttributes : this=%u ... parent=%s(%u) enabled initialRopeRect_=%s !", this, parent_->GetName().CString(), parent_->GetID(), initialRopeRect_.ToString().CString());
        }
    }
    else
    {
        if (gocMove_)
        {
            UnsubscribeFromEvents();

//            URHO3D_LOGINFOF("GOC_StaticRope() - UpdateAttributes : this=%u ... parent=%s(%u) disabled !", this, parent_->GetName().CString(), parent_->GetID());
        }
    }
}

bool GOC_StaticRope::UpdateRope()
{
    Vector2 delta = anchor_ - node_->GetWorldPosition2D();
    if (delta.y_ < 0.f)
        return false;

    float length = delta.Length();

    if (length > maxLength_)
        return false;

    Rect drawrect = initialRopeRect_;
    drawrect.max_.x_ = length / node_->GetWorldScale2D().x_;
    ropeDrawable_->SetUseDrawRect(true);
    ropeDrawable_->SetDrawRect(drawrect);

    Node* node = ropeDrawable_->GetNode();
    node->SetWorldRotation2D(delta.Angle(Vector2::RIGHT));

//    URHO3D_LOGINFOF("GOC_StaticRope() - UpdateRope : this=%u ... node=%s(%u) pos=%s rot=%F length=%F (Vdelta=%s)",
//                    this, node->GetName().CString(), node->GetID(), node->GetWorldPosition2D().ToString().CString(),
//					node->GetWorldRotation2D(), drawrect.max_.x_, delta.ToString().CString());
    return true;
}

void GOC_StaticRope::AttachOnRoof()
{
    if (attached_ || !ropeDrawable_ || !gocMove_)
        return;

    // Find a wall over the parent node

    Vector2 position(parent_->GetWorldPosition2D());
    int viewZ = parent_->GetVar(GOA::ONVIEWZ).GetInt();
    unsigned collisionMask = viewZ == INNERVIEW ? CC_INSIDEWALLS : CC_OUTSIDEWALLS;
    PhysicsRaycastResult2D result;

    GameContext::Get().physicsWorld_->RaycastSingle(result, position, Vector2(position.x_, position.y_ + maxLength_), collisionMask);

    if (result.body_)
    {
        float length = result.position_.y_ - node_->GetWorldPosition2D().y_;
        if (length < maxLength_*0.75f && (length >= minLength_ || (length >= 0.f && result.body_->GetLinearVelocity().y_ <= 0.f)))
        {
            anchor_ = result.position_;

            if (UpdateRope())
            {
                attached_ = true;

                node_->SetVar(GOA::ONVIEWZ, viewZ);
                GameHelpers::UpdateLayering(node_);

                ropeDrawable_->SetLayer2(IntVector2(viewZ + LAYER_ACTOR - 1, -1));
                ropeDrawable_->SetViewMask(ViewManager::GetLayerMask(viewZ));

                ropeDrawable_->GetNode()->AddListener(this);
                ropeDrawable_->GetNode()->SetEnabled(true);

                ropeDrawable_->ForceUpdateBatches();

                gocMove_->AddFlagToMoveState(MV_TOUCHOBJECT);

//                URHO3D_LOGINFOF("GOC_StaticRope() - AttachOnRoof : node=%s(%u) parent=%s(%u) ... anchor=%s viewZ=%d initiallength=%f ... OK !", node_->GetName().CString(), node_->GetID(),
//                                parent_->GetName().CString(), parent_->GetID(), anchor_.ToString().CString(), viewZ, length);
            }
        }
        else
        {
//            URHO3D_LOGINFOF("GOC_StaticRope() - AttachOnRoof : this=%u ... initiallength=%f (must be between min=%f/max=%f) ... NOK !", this, length, minLength_, maxLength_*0.75f);

            ropeDrawable_->GetNode()->SetEnabled(false);
        }
    }
}

void GOC_StaticRope::Break()
{
    if (!attached_ || !ropeDrawable_)
        return;

    if (!dirtyreentrance_)
        ropeDrawable_->GetNode()->RemoveListener(this);

    // prevent showing rope on re-enable
    Rect drawrect = initialRopeRect_;
    drawrect.max_.x_ = 0.f;
    ropeDrawable_->SetDrawRect(drawrect);
    ropeDrawable_->SetUseDrawRect(true);
    ropeDrawable_->ClearSourceBatches();
    ropeDrawable_->GetNode()->SetEnabled(false);

    if (gocMove_)
        gocMove_->RemoveFlagToMoveState(MV_TOUCHOBJECT);

    attached_ = false;

//    URHO3D_LOGINFOF("GOC_StaticRope() - Break : this=%u ... length=%f OK !", this, ropeDrawable_->GetDrawRect().max_.x_);
}

void GOC_StaticRope::OnMarkedDirty(Node* node)
{
    if (dirtyreentrance_ || !attached_ || !ropeDrawable_ || !ropeDrawable_->IsEnabledEffective())
        return;

    dirtyreentrance_ = true;

    if (!UpdateRope())
    {
//        URHO3D_LOGINFOF("GOC_StaticRope() - OnMarkedDirty : this=%u ... node=%s(%u) max move reached ! Break rope !", this, node->GetName().CString(), node->GetID());
        Break();
    }

    dirtyreentrance_ = false;
}

void GOC_StaticRope::OnAppearEvent(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_StaticRope() - OnAppearEvent : this=%u ... eventType=%s(%u)", this, GOE::GetEventName(eventType).CString(), eventType.Value());

    AttachOnRoof();
}

void GOC_StaticRope::OnBreakEvent(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_StaticRope() - OnBreakEvent : this=%u ... eventType=%s(%u)", this, GOE::GetEventName(eventType).CString(), eventType.Value());

    Break();
}

void GOC_StaticRope::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        if (ropeDrawable_->GetNode())
            debug->AddNode(ropeDrawable_->GetNode(), 1.f, false);
    }
}
