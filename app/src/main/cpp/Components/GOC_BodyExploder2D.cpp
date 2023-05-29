#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>

#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameNetwork.h"

#include "GOC_Animator2D.h"
#include "GOC_Collide2D.h"
#include "GOC_Controller.h"
#include "GOC_Destroyer.h"
#ifdef GOC_COMPLETED
#include "GOC_Collectable.h"
#endif

#include "ScrapsEmitter.h"

#include "ObjectPool.h"
#include "MapWorld.h"

#include "GOC_BodyExploder2D.h"


GOC_BodyExploder2D::GOC_BodyExploder2D(Context* context) :
    Component(context),
    numExplodedNodes_(0),
    impactPoint_(Vector2::ZERO),
    csType_(0),
    csRatioXY_(0),
    partLifeTime_(0.f),
    initialImpulse_(5.0f),
    trigEvent_(GOC_LIFEDEAD),
    trigEventString_(String::EMPTY),
    partType_(GOT::BUILDABLEPART),
    toPrepare_(false),
    prepared_(false),
    hideOriginalBody_(false),
    useScrapsEmitter_(false),
    useObjectPool_(true)
{ ; }

GOC_BodyExploder2D::~GOC_BodyExploder2D()
{
//    URHO3D_LOGINFOF("~GOC_BodyExploder2D()");

    UnsubscribeFromAllEvents();
#ifdef ACTIVE_POOL
    if (useObjectPool_)
        return;
#endif

    if (!explodedNodes_.Size())
        return;

    for (unsigned i=0; i < explodedNodes_.Size(); i++)
        if (explodedNodes_[i])
            explodedNodes_[i]->GetComponent<GOC_Destroyer>()->Destroy();

    explodedNodes_.Clear();
}

void GOC_BodyExploder2D::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_BodyExploder2D>();

    URHO3D_ACCESSOR_ATTRIBUTE("Shape RatioXY", GetShapeRatioXY, SetShapeRatioXY, float, 1.0f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Shape Type", GetShapeType, SetShapeType, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Trig Event", GetTrigEvent, SetTrigEvent, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Collectable Parts Type", GetCollectablePartType, SetCollectablePartType, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Parts Life Time", GetPartsLifeTime, SetPartsLifeTime, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Centered Initial Impulse", GetInitialImpulse, SetInitialImpulse, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Hide Original Body", GetHideOriginalBody, SetHideOriginalBody, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Use Scraps Emitter", GetUseScrapsEmitter, SetUseScrapsEmitter, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Scrap Parts Type", GetScrapPartType, SetScrapPartType, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Prepare Nodes", GetToPrepare, SetToPrepare, bool, false, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Use Objects Pool", GetUseObjectPool, SetUseObjectPool, bool, true, AM_DEFAULT);

}

void GOC_BodyExploder2D::SetShapeRatioXY(float csRatio)
{
    if (csRatioXY_== csRatio)
        return;

    csRatioXY_ = csRatio;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::SetShapeType(int cstype)
{
    if (csType_==cstype)
        return;

    csType_ = cstype;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::SetTrigEvent(const String& s)
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetTrigEvent");
    if (trigEventString_ != s && s != String::EMPTY)
    {
        trigEventString_ = s;
        trigEvent_ = StringHash(s);
        MarkNetworkUpdate();
//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetTrigEvent : trigEvent=%u LIFEDEAD=%u LIFEDEC=%u",
//                   trigEvent_.Value(),GOC_LIFEDEAD.Value(),GOC_LIFEDEC.Value());
    }
}

void GOC_BodyExploder2D::SetToPrepare(bool state)
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetToPrepare : nodeID=%u toPrepare = %s prepared_ = %s",
//                     node_->GetID(), state ? "true" : "false", prepared_ ? "true" : "false");

    if (toPrepare_ == state)
        return;

    toPrepare_ = state;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::SetUseObjectPool(bool state)
{
    if (useObjectPool_ == state)
        return;

    useObjectPool_ = state;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::SetHideOriginalBody(bool state)
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetHideOriginalBody !");
    if (hideOriginalBody_ == state)
        return;

    hideOriginalBody_ = state;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::SetCollectablePartType(const String& s)
{
    if (collectablePartTypeName_ != s && !s.Empty())
    {
        collectablePartTypeName_ = s;
        collectablePartType_ = StringHash(collectablePartTypeName_);

        partType_ = GOT::COLLECTABLEPART;

        SetScrapPartType(collectablePartTypeName_);

        MarkNetworkUpdate();
    }
    else if (s.Empty())
    {
        partType_ = GOT::BUILDABLEPART;
        MarkNetworkUpdate();
    }
}

void GOC_BodyExploder2D::SetScrapPartType(const String& s)
{
    if (scrapPartType_ != s && s != String::EMPTY)
    {
        scrapPartTypeName_ = s;
        scrapPartType_ = StringHash("Scraps_"+scrapPartTypeName_);
        MarkNetworkUpdate();
    }
}

void GOC_BodyExploder2D::SetPartsLifeTime(float time)
{
    if (partLifeTime_ == time)
        return;

    partLifeTime_ = time;
    MarkNetworkUpdate();

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetPartsLifeTime = %f", partLifeTime_);
}

void GOC_BodyExploder2D::SetInitialImpulse(float impulse)
{
    if (initialImpulse_ == impulse)
        return;

    initialImpulse_ = impulse;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::SetUseScrapsEmitter(bool use)
{
    useScrapsEmitter_ = use;
    MarkNetworkUpdate();
}

void GOC_BodyExploder2D::ApplyAttributes()
{
//    URHO3D_LOGERRORF("GOC_BodyExploder2D() - ApplyAttributes : %s(%u)", node_->GetName().CString(), node_->GetID());

    if (toPrepare_)
    {
        if (!prepared_)
            PrepareNodes(node_->GetComponent<AnimatedSprite2D>());
    }
}

void GOC_BodyExploder2D::OnSetEnabled()
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - OnSetEnabled : %s(%u) enable=%s effective=%s",
//             node_->GetName().CString(), node_->GetID(), enabled_ ? "true" : "false", IsEnabledEffective()? "true" : "false");

    SetTriggerEnable(IsEnabledEffective());
}

void GOC_BodyExploder2D::SetTriggerEnable(bool state)
{
    if (trigEvent_)
    {
        if (state)
            SubscribeToEvent(node_, trigEvent_, URHO3D_HANDLER(GOC_BodyExploder2D, OnTrigEvent));
        else
            UnsubscribeFromEvent(node_, trigEvent_);
    }
}

bool GOC_BodyExploder2D::SetStateToExplode(AnimatedSprite2D* animatedSprite, String& restoreAnimation)
{
    if (!animatedSprite)
    {
        URHO3D_LOGWARNINGF("GOC_BodyExploder2D() - SetStateToExplode : !animateSprite !");
        return false;
    }

    GOC_Animator2D* gocAnimator = GetComponent<GOC_Animator2D>();
    if (gocAnimator)
    {
        if (gocAnimator->GetTemplateName().Empty())
        {
            URHO3D_LOGWARNINGF("GOC_BodyExploder2D() - SetStateToExplode : !animator template !");
            return false;
        }
    }

    numExplodedNodes_ = 0;
    explodedNodes_.Clear();

    Node* rootPrepareNode = GetScene()->GetChild("PrepareNodes") ? GetScene()->GetChild("PrepareNodes") : GetScene()->CreateChild("PrepareNodes", LOCAL);

    // Create a temporary Node for setting the exploded Parts
    if (!prepareNode_)
    {
        prepareNode_ = rootPrepareNode->CreateChild("BodyExploderNode", LOCAL);
        prepareNode_->SetTemporary(true);
    }

    // Save current animation for backup at the end of function
    restoreAnimation = animatedSprite->GetAnimation();

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetStateToExplode nodeID=%u : restoreAnim=%s ...",
//            node_->GetID(), restoreAnimation.CString());

    // Get Dead Animation or the Last Animation who must contain the exploded Parts
    String deadAnim;
    int animIndex;

    AnimationSet2D* animSet2D = animatedSprite->GetAnimationSet();
    if (gocAnimator)
    {
        deadAnim = gocAnimator->GetAnimInfo(STATE_EXPLODE).animName;
        animIndex = gocAnimator->GetAnimInfo(STATE_EXPLODE).animIndex;
    }
    else
    {
        animIndex = animSet2D->GetNumAnimations()-1;
        deadAnim = animSet2D->GetAnimation(animIndex);
    }

    animatedSprite->SetAnimation(deadAnim);
    // force spriter update
    animatedSprite->ResetAnimation();

    return true;
}

void GOC_BodyExploder2D::PrepareExplodedNodes(AnimatedSprite2D* animatedSprite)
{
    // Get Initial positions
    initialPosition_ = animatedSprite->GetNode()->GetWorldPosition2D();
    initialScale_ = animatedSprite->GetNode()->GetWorldScale2D();
    initialRotation_ = animatedSprite->GetNode()->GetWorldRotation2D();

    // Align preparenode with animatedsprite root node
    prepareNode_->SetWorldTransform2D(initialPosition_, initialRotation_, initialScale_);

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - PrepareExplodedNodes : nodeID=%u fromAnimation=%s wposition=%s usePool=%s ...",
//                    node_->GetID(), animatedSprite->GetAnimation().CString(), prepareNode_->GetWorldPosition2D().ToString().CString(), useObjectPool_ ? "true" : "false");

    // Create the exploded nodes as children of prepareNode
    {
//        const Vector2 hotspot(0.5f, 0.5f);
        Vector2 position;
        Vector2 scale;
        float angle;
        Node* node;
        StaticSprite2D* staticSprite;

        Vector<String>* partnames = GOT::GetPartsNamesFor(StringHash(node_->GetName()));

        const PODVector<SpriteInfo*>& spriteInfos = animatedSprite->GetSpriteInfos();

        for (unsigned i=0; i < spriteInfos.Size(); ++i)
        {
            if (partnames && !partnames->Contains(spriteInfos[i]->sprite_->GetName()))
                continue;

#ifdef ACTIVE_POOL
            if (useObjectPool_)
            {
                int entityid = 0;
                node = ObjectPool::CreateChildIn(partType_, entityid, prepareNode_);
            }
            else
#endif
            {
                node = prepareNode_->CreateChild(String::EMPTY, LOCAL);
                if (!GameHelpers::LoadNodeXML(context_, node, GOT::GetObjectFile(partType_).CString()))
                    node->Remove();
            }

            if (!node)
            {
                URHO3D_LOGWARNINGF("GOC_BodyExploder2D() - PrepareExplodedNodes : i=%u sprite=%s can't spawn no node !",
                                   i, spriteInfos[i]->sprite_ ? spriteInfos[i]->sprite_->GetName().CString() : "Empty");
                return;
            }

            animatedSprite->GetLocalSpritePositions(i, position, angle, scale);

            node->SetTransform2D(position, angle, scale);
            node->AddTag("UsedAsPart");

            staticSprite = node->GetComponent<StaticSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
            staticSprite->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
#endif
            staticSprite->SetSprite(spriteInfos[i]->sprite_);
//            staticSprite->SetHotSpot(hotspot);
            staticSprite->SetColor(animatedSprite->GetColor());
            staticSprite->SetFlip(animatedSprite->GetFlipX(), animatedSprite->GetFlipY());

            GameHelpers::UpdateLayering(node);

//            URHO3D_LOGINFOF("GOC_BodyExploder2D() - PrepareExplodedNodes : i=%u nodeID=%u wposition=%s sprite=%s",
//                     i, node->GetID(), position.ToString().CString(), spriteInfos[i]->sprite_ ? spriteInfos[i]->sprite_->GetName().CString() : "Empty");
        }
    }

    const Vector<SharedPtr<Node> >& children = prepareNode_->GetChildren();
    for (Vector<SharedPtr<Node> >::ConstIterator it = children.Begin(); it != children.End(); ++it)
        explodedNodes_.Push(WeakPtr<Node>(*it));
    numExplodedNodes_ = explodedNodes_.Size();

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - PrepareExplodedNodes nodeID=%u : ... numExplodedNodes=%u OK !", node_->GetID(), numExplodedNodes_);
}

void GOC_BodyExploder2D::PreparePhysics()
{
    // Create 2D RigidBody and shapes
    Node* node;
    StaticSprite2D* staticSprite;
    Urho3D::Sprite2D* sprite;
    Urho3D::Rect drawRect, textureRect;
    RigidBody2D* r;
    Vector2 bbsize;
    float density_ = node_->GetComponent<CollisionCircle2D>()->GetDensity();
    BoundingBox bbox;

//    GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
//    BodyType2D btype = GameContext::Get().ClientMode_ && (!controller || !controller->IsMainController()) ? BT_STATIC : BT_DYNAMIC;
    BodyType2D btype = BT_DYNAMIC;

    if (csType_ == 0)
    {
        CollisionBox2D* b;
        for (unsigned i=0; i < numExplodedNodes_; ++i)
        {
            node = explodedNodes_[i];
            staticSprite = node->GetComponent<StaticSprite2D>();

            // if using centered hotspot, we don't need to calculate sprite center, it is at ZERO
            // and we don't need to specified the mass center and collider center
            staticSprite->SetUseHotSpot(true);
            staticSprite->SetHotSpot(Vector2::HALF);

            sprite = staticSprite->GetSprite();
            sprite->GetDrawRectangle(drawRect, staticSprite->GetFlipX(), staticSprite->GetFlipY());
            sprite->GetTextureRectangle(textureRect, staticSprite->GetFlipX(), staticSprite->GetFlipY());

            staticSprite->SetUseDrawRect(true);
            staticSprite->SetDrawRect(drawRect);
            staticSprite->SetUseTextureRect(true);
            staticSprite->SetTextureRect(textureRect);

            bbsize = drawRect.Size();// / sprite->GetTexture()->GetDpiRatio();
//            bcenter.x_ = (0.5f - sprite->GetHotSpot().x_) * bbsize.x_;
//            bcenter.y_ = (0.5f - sprite->GetHotSpot().y_) * bbsize.y_;

            r = node->GetComponent<RigidBody2D>();
            r->SetBodyType(btype);
            r->SetFixedRotation(false);
            r->SetMass(density_ * bbsize.x_ * bbsize.y_ * csRatioXY_);
//            r->SetMassCenter(drawRect.Center());
            r->SetLinearDamping(1.f);
            r->SetAngularDamping(5.f);

            b = node->GetOrCreateComponent<CollisionBox2D>();
            b->SetSize(csRatioXY_ * bbsize);
            b->SetFriction(0.5f);
            b->SetExtraContactBits(CONTACT_STABLE);
//            b->SetCenter(r->GetMassCenter());
        }
    }
    else if (csType_ == 1)
    {
        CollisionCircle2D* c;
        for (unsigned i=0; i < numExplodedNodes_; ++i)
        {
            node = explodedNodes_[i];
            staticSprite = node->GetComponent<StaticSprite2D>();

            // if using centered hotspot, we don't need to calculate sprite center, it is at ZERO
            // and we don't need to specified the mass center and collider center
            staticSprite->SetUseHotSpot(true);
            staticSprite->SetHotSpot(Vector2::HALF);

            sprite = staticSprite->GetSprite();
            sprite->GetDrawRectangle(drawRect, staticSprite->GetHotSpot(), staticSprite->GetFlipX(), staticSprite->GetFlipY());
            sprite->GetTextureRectangle(textureRect, staticSprite->GetFlipX(), staticSprite->GetFlipY());

            staticSprite->SetUseDrawRect(true);
            staticSprite->SetDrawRect(drawRect);
            staticSprite->SetUseTextureRect(true);
            staticSprite->SetTextureRect(textureRect);

            bbsize = drawRect.Size();// / sprite->GetTexture()->GetDpiRatio();
//            bcenter.x_ = (0.5f - sprite->GetHotSpot().x_) * bbsize.x_;
//            bcenter.y_ = (0.5f - sprite->GetHotSpot().y_) * bbsize.y_;

            r = node->GetComponent<RigidBody2D>();
            r->SetBodyType(btype);
            r->SetFixedRotation(false);
            r->SetUseFixtureMass(false);
            r->SetMass(density_ * bbsize.x_ * bbsize.y_ * csRatioXY_);
//            r->SetMassCenter(drawRect.Center());
            r->SetLinearDamping(1.f);
            r->SetAngularDamping(5.f);

            c = node->GetOrCreateComponent<CollisionCircle2D>();
            c->SetRadius(Min(bbsize.x_, bbsize.y_) * 0.5f);// * csRatioXY_);
            c->SetFriction(0.5f);
            c->SetExtraContactBits(CONTACT_STABLE);
//            c->SetCenter(r->GetMassCenter());
        }
    }
}

void GOC_BodyExploder2D::PrepareNodes(AnimatedSprite2D* animatedSprite)
{
    // Prepare Exploded Nodes

    if (prepared_)
        return;

    String restoreAnim;

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - PrepareNodes node %s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (SetStateToExplode(animatedSprite, restoreAnim))
    {
//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - PrepareNodes node %s(%u) enabled=%s ...",
//                node_->GetName().CString(), node_->GetID(), node_->IsEnabled() ? "true" : "false");

        PrepareExplodedNodes(animatedSprite);

        PreparePhysics();

        prepareNode_->SetEnabledRecursive(false);

        animatedSprite->SetAnimation(restoreAnim);

//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - PrepareNodes node %s(%u) ... OK !", node_->GetName().CString(), node_->GetID());

        prepared_ = true;
    }
}

void GOC_BodyExploder2D::GenerateNodes(AnimatedSprite2D* animatedSprite)
{
    // Create Exploded Nodes with RigidBodies on the fly

    String restoreAnim;

    if (SetStateToExplode(animatedSprite, restoreAnim))
    {
//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - GenerateNodes node %s(%u) ...",
//                node_->GetName().CString(), node_->GetID());

        PrepareExplodedNodes(animatedSprite);

        PreparePhysics();

        animatedSprite->SetAnimation(restoreAnim);

        TransferExplodedNodesPositionsTo(animatedSprite->GetNode());

        SetExplodedNodesComponents();

//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - GenerateNodes ... OK");
    }
}

void GOC_BodyExploder2D::TransferExplodedNodesPositionsTo(Node* rootNode)
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - TransferExplodedNodesPositionsTo node=%s(%u) ...",
//            rootNode->GetName().CString(), rootNode->GetID());

    // Set new positions

    Vector2 deltaPosition = rootNode->GetWorldPosition2D() - initialPosition_;
    if (deltaPosition != Vector2::ZERO)
        prepareNode_->Translate2D(deltaPosition, TS_WORLD);

//    {
//        float deltaRotation = rootNode->GetWorldRotation2D() - initialRotation_;
//        if (deltaRotation != 0.f)
//            prepareNode_->Rotate2D(deltaRotation);
//    }

    {
        Vector2 deltaScale = rootNode->GetWorldScale2D()/initialScale_;
        if (abs(deltaScale.x_) > PIXEL_SIZE)
        {
            prepareNode_->Scale2D(deltaScale);
//            URHO3D_LOGINFOF("GOC_BodyExploder2D() - TransferExplodedNodesPositionsTo : node=%s(%u) scale to %s (rootNodeScale=%s/initialScale_=%s) ... OK!",
//                            rootNode->GetName().CString(), rootNode->GetID(), deltaScale.ToString().CString(), rootNode->GetWorldScale2D().ToString().CString(), initialScale_.ToString().CString());
        }
    }

    // Force Recalculate position
//    deltaPosition = prepareNode_->GetWorldPosition2D();

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - TransferExplodedNodesPositionsTo : node=%s(%u) %s to %s(%s)  ... OK!",
//                    rootNode->GetName().CString(), rootNode->GetID(), initialPosition_.ToString().CString(),
//                    rootNode->GetWorldPosition2D().ToString().CString(), prepareNode_->GetWorldPosition2D().ToString().CString());
}

void GOC_BodyExploder2D::SetExplodedNodesComponents()
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetExplodedNodesComponents : node=%s(%u) ... ", node_->GetName().CString(), node_->GetID());

    Node* node;

    // Add Collectable Component
    if (collectablePartType_ != StringHash::ZERO)
    {
        StringHash fromType(node_->GetName());

        for (unsigned i=0; i < numExplodedNodes_; ++i)
        {
            node = explodedNodes_[i];

            if (!node)
                continue;

            GOC_Collectable* collectable = node->GetOrCreateComponent<GOC_Collectable>(LOCAL);
            collectable->SetSlot(collectablePartType_, 1, fromType);//, false);
            collectable->SetSpriteProps(node->GetComponent<StaticSprite2D>());
        }
    }

    // Add Destroyer Component and add Nodes to the scene node
    {
        int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();
        unsigned viewMask = node_->GetDerivedComponent<Drawable2D>()->GetViewMask();
        GOC_Destroyer* destroyer;
        Vector2 position;
        for (unsigned i=0; i < numExplodedNodes_; ++i)
        {
            node = explodedNodes_[i];

            if (!node)
                continue;

            node->RemoveTag("UsedAsPart");
            node->SetTemporary(false);

            node->GetComponent<RigidBody2D>()->SetAllowSleep(false);
//            node->GetComponent<RigidBody2D>()->SetAwake(true);

            destroyer = node->GetComponent<GOC_Destroyer>();
            destroyer->SetEnableUnstuck(false);
            destroyer->Reset(true);
            destroyer->SetLifeTime(partLifeTime_);
            destroyer->SetEnableLifeNotifier(false);
            destroyer->SetViewZ(viewZ, viewMask, i+1);

//            position = node->GetWorldPosition2D();
//            URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetExplodedNodesComponents : node=%s(%u) partnodeid=%u parent=%u(%u) position=%s ... ",
//                            node_->GetName().CString(), node_->GetID(), node->GetID(), prepareNode_->GetID(), node->GetParent() ? node->GetParent()->GetID() : 0, position.ToString().CString());

            // Reparent from preparenode to entitynode scene
//            World2D::AttachEntityToMapNode(node, destroyer->GetWorldMapPosition().mPoint_, (GameContext::Get().LocalMode_ || node->GetID() > LAST_REPLICATED_ID) ? LOCAL : REPLICATED);
            World2D::AttachEntityToMapNode(node, destroyer->GetWorldMapPosition().mPoint_, LOCAL);

//            position = node->GetWorldPosition2D();

            node->SendEvent(WORLD_ENTITYCREATE);

//            URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetExplodedNodesComponents : node=%s(%u) partnode(%u) mpoint=%s parent=%s(%u) position=%s(%s) ... ",
//                            node_->GetName().CString(), node_->GetID(), node->GetID(), destroyer->GetWorldMapPosition().mPoint_.ToString().CString(),
//                            node->GetParent() ? node->GetParent()->GetName().CString() : "noparent", node->GetParent() ? node->GetParent()->GetID() : 0, position.ToString().CString(), node->GetWorldPosition2D().ToString().CString());

//            position = node->GetWorldPosition2D();
//            URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetExplodedNodesComponents : node=%s(%u) partnodeid=%u parent=%u(%u) position=%s ... ",
//                            node_->GetName().CString(), node_->GetID(), node->GetID(), node->GetParent() ? node->GetParent()->GetID() : 0, prepareNode_->GetID(), position.ToString().CString());


        }
    }

    if (useScrapsEmitter_ && scrapPartType_ != StringHash::ZERO)
    {
        // Set ScrapsEmitter Component
        for (unsigned i=0; i < numExplodedNodes_; ++i)
        {
            node = explodedNodes_[i];

            if (!node)
                continue;

            ScrapsEmitter* scrapsEmitter = node->GetComponent<ScrapsEmitter>();
            if (!scrapsEmitter)
                continue;

            Sprite2D* sprite = node->GetComponent<StaticSprite2D>()->GetSprite();

            if (!sprite)
                continue;

            float dratio = sprite->GetTexture()->GetDpiRatio();
            if (dratio == 0.f)
                dratio = 1.f;

            IntVector2 rectsize = sprite->GetRectangle().Size();
            rectsize.x_ = (float)rectsize.x_ / dratio;
            rectsize.y_ = (float)rectsize.y_ / dratio;

            scrapsEmitter->SetScrapsType(scrapPartType_);
            scrapsEmitter->SetSize(rectsize);
            scrapsEmitter->SetNumScraps(Min(7, Max(rectsize.x_, rectsize.y_)/2));
            scrapsEmitter->SetLifeTime(2.f + Random(2.f));
            scrapsEmitter->SetInitialImpulse(initialImpulse_);
            scrapsEmitter->SetTrigEvent(GO_RECEIVEEFFECT);
            scrapsEmitter->ApplyAttributes();
        }
    }

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - SetExplodedNodesComponents : node=%s(%u) ... OK !", node_->GetName().CString(), node_->GetID());
}

void GOC_BodyExploder2D::UpdatePositions(Node* rootNode)
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - UpdatePositions node %s(%u) ...", node_->GetName().CString(), node_->GetID());

    TransferExplodedNodesPositionsTo(rootNode);

    prepareNode_->SetEnabledRecursive(true);

    SetExplodedNodesComponents();

//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - UpdatePositions node %s(%u) ... position=%s OK !", node_->GetName().CString(), node_->GetID(), node_->GetWorldPosition().ToString().CString());
}

void GOC_BodyExploder2D::Explode()
{
    if (prepared_)
        UpdatePositions(GetComponent<AnimatedSprite2D>()->GetNode());
    else
        GenerateNodes(GetComponent<AnimatedSprite2D>());

    URHO3D_LOGINFOF("GOC_BodyExploder2D() - Explode : node %s(%u) ... in %u nodes ...", node_->GetName().CString(), node_->GetID(), numExplodedNodes_);

    Node* node;
    Vector2 force;
    Vector2 position;

    for (unsigned i=0; i < numExplodedNodes_; ++i)
    {
        node = explodedNodes_[i];

        if (!node)
            continue;

//        if (!node->HasTag("InUse"))
        if (node_->isPoolNode_ && node_->isInPool_)
        {
            URHO3D_LOGWARNINGF("GOC_BodyExploder2D() - Explode : node=%s(%u) partnode(%u) is in the ObjectPool => can't use it ! SKIP !", node_->GetName().CString(), node_->GetID(), node->GetID());
            explodedNodes_[i].Reset();
            continue;
        }

        node->SetEnabled(true);

        position = node->GetWorldPosition2D();
        force.x_ = Abs(position.x_ - impactPoint_.x_) > 0.001f ? position.x_ - impactPoint_.x_ : 0.001f;
        force.x_ = initialImpulse_ * force.x_;
        force.y_ = Abs(position.y_ - impactPoint_.y_) > 0.001f ? position.y_ - impactPoint_.y_ : 0.001f;
        force.y_ = initialImpulse_ * force.y_;

        RigidBody2D* body = node->GetComponent<RigidBody2D>();
        body->SetAwake(true);
        body->ApplyForce(force, impactPoint_, true);
        body->SetAngularVelocity(force.x_ * force.y_);

//        StaticSprite2D* staticSprite = explodedNodes_[i]->GetComponent<StaticSprite2D>();
//
//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - Explode : Part %u=%s nodeID=%u position=%s wscale=%s enabled=%s parent=%s", i , staticSprite->GetSprite()->GetName().CString(),
//                        explodedNodes_[i]->GetID(), position.ToString().CString(), node->GetWorldScale2D().ToString().CString(), staticSprite->IsEnabledEffective() ? "true" : "false",
//                        explodedNodes_[i]->GetParent() ? explodedNodes_[i]->GetParent()->GetName().CString() : "noparent");
    }

    if (!GameContext::Get().LocalMode_)
    {
        GameNetwork& network = *GameNetwork::Get();
//        GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
//        bool servermode = !controller || !controller->IsMainController();
        for (unsigned i=0; i < numExplodedNodes_; ++i)
        {
            node = explodedNodes_[i];

            if (!node)
                continue;

//            network.AddObjetControl(node, true);
            ObjectControlInfo* cinfo = network.AddSpawnControl(node, node_);
        }
    }

    prepared_ = false;

    URHO3D_LOGINFOF("GOC_BodyExploder2D() - Explode !");
}

void GOC_BodyExploder2D::OnTrigEvent(StringHash eventType, VariantMap& eventData)
{
    if (node_->GetVar(GOA::DESTROYING).GetBool())
        return;

    URHO3D_LOGINFOF("GOC_BodyExploder2D() - OnTrigEvent : Node=%s(%u)",
                    GetNode()->GetName().CString(), GetNode()->GetID());

    UnsubscribeFromEvent(node_, trigEvent_);

    if (initialImpulse_ != 5.f)
        impactPoint_ = GetComponent<AnimatedSprite2D>()->GetNode()->GetWorldPosition2D();
    else
        impactPoint_ = eventData[GOC_Life_Events::GO_WORLDCONTACTPOINT].GetVector2();

    impulseTimer_ = 0.f;

    if (hideOriginalBody_)
        SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(GOC_BodyExploder2D, HandleWaitStateForHide));
    else
        Explode();
}

// delai necessaire pour que GOC_Animator2D reagisse a l'event Dead avant le BodyExploder
// autrement affichage des deux bodies (le setenabled ne servant a rien ici dans ce cas,
// car reactiver juste apres par l'Animator)
void GOC_BodyExploder2D::HandleWaitStateForHide(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_BodyExploder2D() - HandleWaitStateForHide : node=%s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (!node_->IsEnabled())
    {
        UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);
        return;
    }

    if (GetComponent<GOC_Animator2D>()->GetStateValue().Value() == STATE_EXPLODE)
    {
//        URHO3D_LOGINFOF("GOC_BodyExploder2D() - HandleWaitStateForHide : node=%s(%u) ... OK !", node_->GetName().CString(), node_->GetID());

        UnsubscribeFromEvent(GetScene(), E_SCENEPOSTUPDATE);

        // after waiting for Dead State => hide drawable and inactive physics
        node_->GetDerivedComponent<Drawable2D>()->SetEnabled(false);
        node_->GetComponent<RigidBody2D>()->SetEnabled(false);

        // apply initial physic force to parts
        Explode();
    }
}

void GOC_BodyExploder2D::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        if (prepareNode_)
            debug->AddNode(prepareNode_, 5.f, false);

        for (unsigned i = 0; i < numExplodedNodes_; ++i)
        {
            if (explodedNodes_[i])
            {
                debug->AddNode(explodedNodes_[i], 1.f, false);
                PODVector<StaticSprite2D * > drawables;
                explodedNodes_[i]->GetComponents<StaticSprite2D>(drawables,true);
                for (unsigned i = 0; i < drawables.Size(); ++i)
                {
                    drawables[i]->DrawDebugGeometry(debug, false);
                }
            }
        }
    }
}
