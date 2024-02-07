#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/RigidBody2D.h>

#include "GameAttributes.h"
#include "GameHelpers.h"
#include "GameEvents.h"
#include "ObjectPool.h"
#include "MapWorld.h"

#include "GOC_Spawner.h"

const Vector2 DEFAULTSPAWNINTERVAL(10.f, 30.f);

GOC_Spawner::GOC_Spawner(Context* context) :
    Component(context), actived_(false), maxLivingSpawnables_(5), spawnInterval_(DEFAULTSPAWNINTERVAL), delay_(0.f)
{ }

GOC_Spawner::~GOC_Spawner()
{ }

void GOC_Spawner::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Spawner>();
    URHO3D_ACCESSOR_ATTRIBUTE("Is Actived", GetActived, SetActived, bool, false, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Max Living Spawnables", int, maxLivingSpawnables_, 5, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Objects Spawnables", GetObjectTypesAttr, SetObjectTypesAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Time Between spawn", Vector2, spawnInterval_, DEFAULTSPAWNINTERVAL, AM_DEFAULT);
}

void GOC_Spawner::OnSetEnabled()
{
    UpdateAttributes();
}

void GOC_Spawner::OnNodeSet(Node* node)
{
    UpdateAttributes();
}

void GOC_Spawner::SetActived(bool actived)
{
    if (actived_ == actived)
        return;

//    URHO3D_LOGINFOF("GOC_Spawner() - SetActived : %u", actived);
    actived_ = actived;

    UpdateAttributes();
}

void GOC_Spawner::SetObjectTypesAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_Spawner() - SetObjectTypesAttr : %s", value.CString());

    spawnableObjectTypes_.Clear();

    if (value.Empty())
        return;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        StringHash hash = StringHash(*it);
        if (GOT::HasObject(hash))
            spawnableObjectTypes_.Push(hash);
    }

    UpdateAttributes();

    Dump();
}

String GOC_Spawner::GetObjectTypesAttr() const
{
    String value;

    for (unsigned i=0; i < spawnableObjectTypes_.Size(); ++i)
    {
        value += GOT::GetType(spawnableObjectTypes_[i]);
        if (i!=spawnableObjectTypes_.Size()-1)
            value += "|";
    }
    return value;
}

void GOC_Spawner::UpdateAttributes()
{
//    URHO3D_LOGINFOF("GOC_Spawner() - UpdateAttributes : actived_(%u), enabled(%u)", actived_, IsEnabledEffective());

    if (GetScene() && IsEnabledEffective() && actived_ && !GameContext::Get().ClientMode_)
    {
        SubscribeToEvent(GetScene(), E_SCENEUPDATE, URHO3D_HANDLER(GOC_Spawner, HandleUpdate));
        SubscribeToEvent(GAME_STOP, URHO3D_HANDLER(GOC_Spawner, HandleUpdate));
        spawnDelay_ = Random(spawnInterval_.x_, spawnInterval_.y_);
    }
    else
    {
        UnsubscribeFromAllEvents();

        // Desactive the drawable of the Spawner if exist
//        if (node_)
//        {
//            Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
//            if (drawable)
//                drawable->SetEnabled(false);
//        }
    }
}

void GOC_Spawner::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    if (eventType == GAME_STOP)
    {
        UnsubscribeFromAllEvents();
        return;
    }

    delay_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();
    if (delay_ > spawnDelay_)
    {
        spawnDelay_ = Random(spawnInterval_.x_, spawnInterval_.y_);
        delay_ = 0.f;

        // check for max living spawnables
        if (spawnables_.Size() >= maxLivingSpawnables_)
        {
//            URHO3D_LOGINFOF("GOC_Spawner() - HandleUpdate : Check Living Spawnables ...");
            unsigned i = 0;
            while (i < spawnables_.Size())
            {
//                URHO3D_LOGINFOF("GOC_Spawner() - HandleUpdate : spawnables_[%u] = %s(%u) inpool=%s", i, spawnables_[i] ? spawnables_[i]->GetName().CString() : "none", spawnables_[i] ? spawnables_[i]->GetID() : 0,
//                                spawnables_[i] ? (ObjectPool::IsInPool(spawnables_[i]) ? "true":"false") : "false");

                if (!spawnables_[i] || ObjectPool::IsInPool(spawnables_[i]))
                    spawnables_.EraseSwap(i);

                i++;
            }
        }

        if (spawnables_.Size() < maxLivingSpawnables_)
        {
            // Spawn
            const StringHash& got = GameHelpers::GetRandomMonsters(spawnableObjectTypes_.Size() ? &spawnableObjectTypes_ : 0);
//            URHO3D_LOGINFOF("GOC_Spawner() - HandleUpdate : numSpawnableObjects = %u, spawn index = %u", numSpawnableObjects, index);

            PhysicEntityInfo physicInfo;
            physicInfo.positionx_ = node_->GetWorldPosition2D().x_;
            physicInfo.positiony_ = node_->GetWorldPosition2D().y_+0.5f;
            SceneEntityInfo sceneInfo;

            Node* node = World2D::SpawnEntity(got, RandomEntityFlag|RandomMappingFlag, 0, node_->GetID(), node_->GetVar(GOA::ONVIEWZ).GetInt(), physicInfo, sceneInfo);
            if (node)
            {
                bool duplicated = false;
                // skip duplicated entry
                for (unsigned i=0; i < spawnables_.Size(); i++)
                {
                    if (spawnables_[i]->GetID() == node->GetID())
                        duplicated = true;
                }
                if (!duplicated)
                    spawnables_.Push(WeakPtr<Node>(node));
            }

            // Active the drawable of the Spawner if exist
//            Drawable2D* drawable = node_->GetDerivedComponent<Drawable2D>();
//            if (drawable)
//                drawable->SetEnabled(true);
        }
        else
        {
            URHO3D_LOGINFOF("GOC_Spawner() - HandleUpdate : Max Living Spawnables Reached (%u)", maxLivingSpawnables_);
        }
    }
}

void GOC_Spawner::Dump()
{
    URHO3D_LOGINFOF("GOC_Spawner() - DumpAll : spawnableObjectTypes_ Size =%u", spawnableObjectTypes_.Size());
    for (unsigned i=0; i<spawnableObjectTypes_.Size(); ++i)
        URHO3D_LOGINFOF("   => spawnableObjectTypes_[%u] = %u", i, spawnableObjectTypes_[i].Value());
}
