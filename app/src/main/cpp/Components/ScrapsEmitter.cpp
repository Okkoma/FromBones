#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "DefsColliders.h"

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"

#include "ObjectPool.h"
#include "TimerRemover.h"
#include "ViewManager.h"

#include "ScrapsEmitter.h"


HashMap<StringHash, String > ScrapsEmitter::typeNames_;
HashMap<StringHash, Vector<WeakPtr<Sprite2D> > > ScrapsEmitter::spriteLists_;



ScrapsEmitter::ScrapsEmitter(Context* context) :
    Component(context),
    type_(StringHash::ZERO),
    trigEvent_(GO_RECEIVEEFFECT),
    trigEventString_(String::EMPTY),
    lifeTime_(0.f),
    density_(1.f),
    initialImpulse_(5.0f),
    isCollider_(false),
    destroyNode_(true)
{
//    URHO3D_LOGINFOF("ScrapsEmitter()");
}


ScrapsEmitter::~ScrapsEmitter()
{ }

void ScrapsEmitter::RegisterObject(Context* context)
{
    context->RegisterFactory<ScrapsEmitter>();

    URHO3D_ACCESSOR_ATTRIBUTE("Register Type", GetScrapsTypeName, RegisterType, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Type", GetScrapsTypeName, SetScrapsType, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Size", GetSize, SetSize, IntVector2, IntVector2::ZERO, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Num Scraps", GetNumScraps, SetNumScraps, int, 0, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Life Time", GetLifeTime, SetLifeTime, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Centered Initial Impulse", GetInitialImpulse, SetInitialImpulse, float, 0.f, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Trig Event", GetTrigEvent, SetTrigEvent, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Density", float, density_, 1.f, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Is Collider", bool, isCollider_, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Destroy Node", bool, destroyNode_, true, AM_DEFAULT);

    spriteLists_.Clear();

    URHO3D_LOGINFOF("ScrapsEmitter() - RegisterObject : hash=%u ... OK !", ScrapsEmitter::GetTypeStatic().Value());
}

void ScrapsEmitter::RegisterType(Context* context, const String& typeStr)
{
    ScrapsEmitter ScrapsEmitter(context);
    ScrapsEmitter.RegisterType(typeStr);
}

void ScrapsEmitter::RegisterType(Context* context, const String& typeStr, int terrainid)
{
    ScrapsEmitter ScrapsEmitter(context);
    ScrapsEmitter.RegisterType(typeStr, terrainid);
}

void ScrapsEmitter::RegisterType(const String& typeStr)
{
    if (typeStr == String::EMPTY)
        return;

    Vector<String> var = typeStr.Split('|');
    const String& typeName = var[0];

    StringHash type(typeName);
    if (typeNames_.Contains(type))
    {
//        URHO3D_LOGERRORF("Contains Already this type !");
        return;
    }

    const String& spriteSheetFile = var[1];

    SpriteSheet2D* spriteSheet = context_->GetSubsystem<ResourceCache>()->GetResource<SpriteSheet2D>(spriteSheetFile);
    if (!spriteSheet)
    {
        URHO3D_LOGERRORF("ScrapsEmitter() - RegisterType : %s => no spriteSheet !", typeStr.CString());
        return;
    }

    typeNames_[type] = typeName;

    Vector<WeakPtr<Sprite2D> >& spriteList = spriteLists_[type];
    spriteList.Clear();

    const HashMap<String, SharedPtr<Sprite2D> >& sprites = spriteSheet->GetSpriteMapping();

    for (HashMap<String, SharedPtr<Sprite2D> >::ConstIterator it=sprites.Begin(); it!=sprites.End(); ++it)
        spriteList.Push(WeakPtr<Sprite2D>(it->second_.Get()));

    URHO3D_LOGINFOF("ScrapsEmitter() - RegisterType : %s ... OK !", typeStr.CString());
}

void ScrapsEmitter::RegisterType(const String& typeStr, int terrainid)
{
    if (typeStr == String::EMPTY)
        return;

    const String& spriteSheetFile = "2D/collectable1.xml";

    SpriteSheet2D* spriteSheet = context_->GetSubsystem<ResourceCache>()->GetResource<SpriteSheet2D>(spriteSheetFile);
    if (!spriteSheet)
    {
        URHO3D_LOGERRORF("ScrapsEmitter() - RegisterType : %s => no spriteSheet %s !", typeStr.CString(), spriteSheetFile.CString());
        return;
    }

    String typeName = "Scraps_Terrain" + String(terrainid);
    StringHash type(typeName);
    typeNames_[type] = typeName;

    Vector<WeakPtr<Sprite2D> >& spriteList = spriteLists_[type];
    spriteList.Clear();

    const HashMap<String, SharedPtr<Sprite2D> >& sprites = spriteSheet->GetSpriteMapping();

    for (HashMap<String, SharedPtr<Sprite2D> >::ConstIterator it=sprites.Begin(); it!=sprites.End(); ++it)
    {
        const SharedPtr<Sprite2D>& sprite = it->second_;
        if (sprite->GetName().StartsWith("scrap"+typeStr))
            spriteList.Push(WeakPtr<Sprite2D>(sprite.Get()));
    }

    URHO3D_LOGINFOF("ScrapsEmitter() - RegisterType : type=%s terrainid=%d ... OK !", typeStr.CString(), terrainid);
}

void ScrapsEmitter::SetScrapsType(const String& typeName)
{
    StringHash type(typeName);

    if (type_ != type && typeName != GetScrapsTypeName())
        type_ = type;
}

void ScrapsEmitter::SetScrapsType(const StringHash& type)
{
    if (type_ != type)
        type_ = type;
}

void ScrapsEmitter::SetSize(const IntVector2& size)
{
    size_ = size;
}

void ScrapsEmitter::SetNumScraps(int num)
{
    numScraps_ = num;
}

void ScrapsEmitter::SetTrigEvent(const StringHash& hash)
{
    trigEvent_ = hash;
}

void ScrapsEmitter::SetTrigEvent(const String& s)
{
    if (trigEventString_ != s && s != String::EMPTY)
    {
        trigEventString_ = s;
        SetTrigEvent(StringHash(s));

//        URHO3D_LOGINFOF("ScrapsEmitter() - SetTrigEvent : trigEvent=%u", trigEvent_.Value());
    }
}

void ScrapsEmitter::SetLifeTime(float time)
{
    if (lifeTime_ != time)
        lifeTime_ = time;

//    URHO3D_LOGINFOF("ScrapsEmitter() - SetScrapsLifeTime = %f", lifeTime_);
}

void ScrapsEmitter::SetInitialImpulse(float impulse)
{
    if (initialImpulse_ != impulse)
        initialImpulse_ = impulse;
}

void ScrapsEmitter::ApplyAttributes()
{
//    URHO3D_LOGINFOF("ScrapsEmitter() - ApplyAttributes : %s(%u)", node_->GetName().CString(), node_->GetID());

    enabled_ = type_ ? spriteLists_.Contains(type_) : false;

    OnSetEnabled();
}

void ScrapsEmitter::OnSetEnabled()
{
    if (IsEnabledEffective() && trigEvent_)
    {
        SubscribeToEvent(node_, trigEvent_, URHO3D_HANDLER(ScrapsEmitter, OnTrigEvent));
    }
    else
    {
        UnsubscribeFromAllEvents();
    }

//    URHO3D_LOGINFOF("ScrapsEmitter() - OnSetEnabled : %s(%u) enable=%s", node_->GetName().CString(), node_->GetID(), IsEnabledEffective() ? "true" : "false");
}

void ScrapsEmitter::OnTrigEvent(StringHash eventType, VariantMap& eventData)
{
    UnsubscribeFromEvent(node_, trigEvent_);

    //Vector2 impactPoint = initialImpulse_ != 5.f ? node_->GetWorldPosition2D() : eventData[GOC_Life_Events::GO_WORLDCONTACTPOINT].GetVector2();
    Vector2 impactPoint = node_->GetWorldPosition2D();

//    URHO3D_LOGINFOF("ScrapsEmitter() - OnTrigEvent : Node=%s(%u) ... initialImpulse=%f impactPoint=%s nodepos=%s",
//                    node_->GetName().CString(), node_->GetID(), initialImpulse_, impactPoint.ToString().CString(),
//                    node_->GetWorldPosition2D().ToString().CString());

    Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
    unsigned viewMask = drawable->GetViewMask();
    int layer = drawable->GetLayer();
    int viewZ = node_->GetVar(GOA::ONVIEWZ).GetInt();

    if (GameHelpers::SpawnScraps(type_, numScraps_, Color::WHITE, isCollider_, layer, viewZ, viewMask, impactPoint, node_->GetWorldScale2D(), Max(size_.x_, size_.y_), lifeTime_, initialImpulse_, density_))
    {
        UnsubscribeFromAllEvents();

        if (destroyNode_)
            node_->SendEvent(WORLD_ENTITYDESTROY);
    }
}
