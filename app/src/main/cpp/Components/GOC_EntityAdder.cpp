#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Scene/Scene.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameNetwork.h"
#include "ObjectPool.h"
#include "MapWorld.h"

#include "GOC_Destroyer.h"

#include "GOC_EntityAdder.h"


GOC_EntityAdder::GOC_EntityAdder(Context* context) :
    Component(context),
    autoEnableEntities_(true)
{ }

GOC_EntityAdder::~GOC_EntityAdder()
{
//    URHO3D_LOGINFO("~GOC_EntityAdder()");
}

void GOC_EntityAdder::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_EntityAdder>();
    URHO3D_ACCESSOR_ATTRIBUTE("Entity", GetEntityAttr, SetEntityAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ATTRIBUTE("Auto Enable Entities", bool, autoEnableEntities_, true, AM_DEFAULT);
}

void GOC_EntityAdder::SetEntityAttr(const String& value)
{
    if (value != entry_)
    {
        entry_ = value;

        CleanDependences();
        entitiesInfos_.Clear();

        if (value.Empty())
            return;

//        URHO3D_LOGINFOF("GOC_EntityAdder() - SetEntityAttr : %s", value.CString());

        Vector<String> entitystr = value.Split(':');
        if (!entitystr.Size())
            return;

//        for (Vector<String>::Iterator it = entitystr.Begin(); it != entitystr.End(); ++it)
        {
            StringHash hash(entitystr[0]);

            if (GOT::HasObject(hash))
            {
                entitiesInfos_.Resize(entitiesInfos_.Size()+1);
                EntitiesInfos& info = entitiesInfos_.Back();
                info.type_ = hash;
                info.position_ = ToVector2(entitystr[1]);
            }
        }
    }
}

const String& GOC_EntityAdder::GetEntityAttr() const
{
    return entry_;
}

void GOC_EntityAdder::CleanDependences()
{
//    URHO3D_LOGINFOF("GOC_EntityAdder() - CleanDependences : node=%s(%u) num entities=%u", node_->GetName().CString(), node_->GetID(), entities_.Size());

    if (ObjectPool::Get() && entities_.Size())
    {
        for (Vector<WeakPtr<Node> >::Iterator it = entities_.Begin(); it != entities_.End(); ++it)
        {
            if (*it)
                ObjectPool::Get()->Free(it->Get(), true);
        }
        entities_.Clear();
    }

    if (root_)
    {
//        URHO3D_LOGINFOF("GOC_EntityAdder() - CleanDependences : node=%s(%u) root removing children size=%u", node_->GetName().CString(), node_->GetID(), root_->GetNumChildren());
        root_->RemoveAllChildren();
    }
}

void GOC_EntityAdder::OnSetEnabled()
{
    UpdateAttributes();
}

void GOC_EntityAdder::OnNodeSet(Node* node)
{

}

void GOC_EntityAdder::SetEnabledEntities(bool enable)
{
    UpdateAttributes();

    if (root_)
        root_->SetEnabledRecursive(enable);
}

void GOC_EntityAdder::UpdateAttributes()
{
    if (GetScene() && IsEnabledEffective() && entitiesInfos_.Size())
    {
        if (!root_)
        {
            root_ = node_->GetChild("EntityAdderRoot", LOCAL);
            if (!root_)
                root_ = node_->CreateChild("EntityAdderRoot", LOCAL);
        }

        if (!entities_.Size())
        {
            int viewz = node_->GetVar(GOA::ONVIEWZ).GetInt();
            for (Vector<EntitiesInfos>::ConstIterator it = entitiesInfos_.Begin(); it != entitiesInfos_.End(); ++it)
            {
                const EntitiesInfos& info = *it;

                int entityid = 0;

                // by default (dynamic body) add to rootscene
                WeakPtr<Node> node(ObjectPool::CreateChildIn(info.type_, entityid, World2D::GetEntitiesRootNode(GameContext::Get().rootScene_, GameNetwork::Get() ? REPLICATED : LOCAL), 0, viewz));
                if (!node)
                    continue;

                node->SetWorldPosition2D(node_->GetWorldTransform2D() * info.position_);

                RigidBody2D* body = node->GetComponent<RigidBody2D>();
                bool staticEntity = !body || body->GetType() == BT_STATIC;
                if (staticEntity)
                {
                    // for static entity add to the root node
                    // don't use setparent (hang program), prefer addchild
                    Vector2 scale = node->GetWorldScale2D();
                    root_->AddChild(node);
                    // preserve the scale
                    node->SetWorldScale2D(scale);
                    node->SetPosition2D(info.position_);
                }

                entities_.Push(node);

                GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
                if (destroyer)
                    node->SendEvent(WORLD_ENTITYCREATE);

                GameHelpers::UpdateLayering(node);

//                URHO3D_LOGINFOF("GOC_EntityAdder() - UpdateAttributes : %s(%u) position=%s Adds Entity=%s(%u) position=%s ...",
//								 node_->GetName().CString(), node_->GetID(), node_->GetWorldPosition2D().ToString().CString(),
//								 node->GetName().CString(), node->GetID(), node->GetWorldPosition2D().ToString().CString());

                if (!staticEntity)
                    node->SetEnabledRecursive(true);
            }
        }

//        if (autoEnableEntities_)
        root_->SetEnabledRecursive(true);
    }
    else
    {
        // needed to keep dynamic candle on table for example
        if (entities_.Size())
            for (unsigned i=0; i < entities_.Size(); i++)
                entities_[i]->SetEnabled(false);

        if (root_)
            root_->SetEnabledRecursive(false);
    }
}

void GOC_EntityAdder::Dump()
{
    URHO3D_LOGINFOF("GOC_EntityAdder() - Dump : entitiesInfos_.Size=%u", entitiesInfos_.Size());

    for (unsigned i=0; i < entitiesInfos_.Size(); ++i)
        URHO3D_LOGINFOF("   => entitiesInfos_[%u] = %u", i, entitiesInfos_[i].type_.Value());
}
