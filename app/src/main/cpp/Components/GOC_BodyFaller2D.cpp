#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"

#include "DefsViews.h"
#include "DefsColliders.h"

#include "GOC_Move2D.h"
#include "GOC_Destroyer.h"

#include "GOC_BodyFaller2D.h"

enum
{
    StaticHook = 0,
    WeldHook,
};

GOC_BodyFaller2D::GOC_BodyFaller2D(Context* context)
    : Component(context),
      trigEvent_(GOC_LIFEDEAD),
      trigEventString_(String::EMPTY)
{ ; }

GOC_BodyFaller2D::~GOC_BodyFaller2D()
{
//    URHO3D_LOGDEBUG("~GOC_BodyFaller2D()");
    UnsubscribeFromAllEvents();
}

void GOC_BodyFaller2D::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_BodyFaller2D>();
    URHO3D_ACCESSOR_ATTRIBUTE("Trig Event", GetTrigEvent, SetTrigEvent, String, String::EMPTY, AM_DEFAULT);
}

void GOC_BodyFaller2D::SetTrigEvent(const String& s)
{
//    URHO3D_LOGINFOF("GOC_BodyFaller2D() - SetTrigEvent");
    if (trigEventString_ != s && s != String::EMPTY)
    {
        trigEventString_ = s;
        trigEvent_ = StringHash(s);

        UnsubscribeFromAllEvents();
        if (GetComponent<RigidBody2D>())
        {
            SubscribeToEvent(GetNode(), trigEvent_, URHO3D_HANDLER(GOC_BodyFaller2D, OnTrigEvent));
        }
    }
}

void GOC_BodyFaller2D::OnSetEnabled()
{
//    URHO3D_LOGINFO("GOC_BodyFaller2D() - OnSetEnabled !");
}

void GOC_BodyFaller2D::OnNodeSet(Node* node)
{
//    URHO3D_LOGINFO("GOC_BodyFaller2D() - OnNodeSet !");
    if (node && trigEvent_ && GetComponent<RigidBody2D>())
    {
        SubscribeToEvent(GetNode(), trigEvent_, URHO3D_HANDLER(GOC_BodyFaller2D, OnTrigEvent));
    }
    else
    {
        UnsubscribeFromAllEvents();
    }
}

void GOC_BodyFaller2D::RemoveFixtures()
{
    if (GetComponent<GOC_Move2D>())
        GetComponent<GOC_Move2D>()->SetEnabled(false);

    if (GetComponent<RigidBody2D>())
        GetComponent<RigidBody2D>()->ReleaseShapesFixtures();
}

void GOC_BodyFaller2D::BecomeStatic()
{
    RemoveFixtures();

    if (GetComponent<RigidBody2D>())
        GetComponent<RigidBody2D>()->SetBodyType(BT_STATIC);
}

void GOC_BodyFaller2D::OnTrigEvent(StringHash eventType, VariantMap& eventData)
{
    if (node_->GetVar(GOA::DESTROYING).GetBool())
        return;

    BecomeStatic();
}
