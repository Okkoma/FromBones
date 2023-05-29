#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include "GameNetwork.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameContext.h"

#include "GOC_Destroyer.h"

#include "GO_Pool.h"


#define DEFAULT_NUMGOS 10

GO_Pool::GO_Pool(const GO_Pool& pool) :
    Object(pool.context_),
    GOT_(pool.GOT_),
    clientID_(0),
    nodes_(pool.nodes_),
    freenodes_(pool.freenodes_),
    nodePool_(pool.nodePool_)
{ ; }

void GO_Pool::Init(Node* root, const StringHash& GOT, int clientid)
{
    context_ = root->GetContext();
    GOT_ = GOT;
    clientID_= clientid;

    nodePool_ = root->CreateChild(GOT::GetType(GOT_), LOCAL);
    nodePool_->SetVar(GOA::GOT, GOT_);
    nodePool_->SetVar(GOA::CLIENTID, clientID_);
}

void GO_Pool::RegisterObject(Context* context)
{
    context->RegisterFactory<GO_Pool>();
}

Node* GO_Pool::GetGO()
{
    if (!freenodes_.Empty())
    {
//        URHO3D_LOGINFOF("GO_Pool() - GetGO : got=%s(%u), clientID=%d poolSize = %u ...",
//                GOT::GetType(GOT_).CString(), GOT_.Value(), clientID_, nodes_.Size());

        Node* node = freenodes_.Front();
        freenodes_.PopFront();

        if (node)
        {
            node->GetComponent<GOC_Destroyer>()->Reset(true);
            node->isInPool_ = false;

            // Reactive components
            const Vector<SharedPtr<Component> >& components = node->GetComponents();
            for (Vector<SharedPtr<Component> >::ConstIterator it = components.Begin(); it != components.End(); ++it)
                it->Get()->SetEnabled(true);
        }

//        URHO3D_LOGINFOF("GO_Pool() - GetGO : got=%s(%u), clientID=%d poolSize = %u ... OK !",
//                    GOT::GetType(GOT_).CString(), GOT_.Value(), clientID_, nodes_.Size());

        return node;
    }

    return 0;
}

Node* GO_Pool::UseGO(Node* node)
{
    if (!node)
        return GetGO();

    List<Node* >::Iterator it = freenodes_.Find(node);
    if (it != freenodes_.End())
    {
        node = *it;

        it = freenodes_.Erase(it);

        node->GetComponent<GOC_Destroyer>()->Reset(true);
        node->isInPool_ = false;

        // Reactive components
        const Vector<SharedPtr<Component> >& components = node->GetComponents();
        for (Vector<SharedPtr<Component> >::ConstIterator it = components.Begin(); it != components.End(); ++it)
            it->Get()->SetEnabled(true);

//        URHO3D_LOGINFOF("GO_Pool() - UseGO : got=%s(%u) clientID spawn id=%u !", GOT::GetType(GOT_).CString(), GOT_.Value(), clientID_, node->GetID());

        return node;
    }

    unsigned previd = node->GetID();
    node = GetGO();

    URHO3D_LOGWARNINGF("GO_Pool() - UseGO : got=%s(%u) no free id=%u get another id=%u !", GOT::GetType(GOT_).CString(), GOT_.Value(), previd, node ? node->GetID() : 0);

    return node;
}

void GO_Pool::FreeGO(Node* node)
{
    if (!freenodes_.Contains(node))
    {
        // Need to Reset if gocdestroyer : gocdestroyer don't have attribute Enabled registered (that could be set by applyattributes)
        if (node->GetComponent<GOC_Destroyer>())
            node->GetComponent<GOC_Destroyer>()->Reset(false);
        else
            node->SetEnabled(false);

        node->ApplyAttributes();
        node->isInPool_ = true;

//        URHO3D_LOGINFOF("GO_Pool() - FreeGO : node=%s(%u) !", node->GetName().CString(), node->GetID());
        freenodes_.Push(node);
    }
}

void GO_Pool::Resize(unsigned size)
{
    if (nodes_.Size() < size)
    {
        unsigned oldsize = nodes_.Size();
        nodes_.Resize(size);

//        URHO3D_LOGINFOF("GO_Pool() - GOT=%u type=%s - Resize from %u to %u ...", GOT_.Value(), GOT::GetType(GOT_).CString(), oldsize, size);

        for (unsigned i = oldsize; i < size; i++)
        {
            WeakPtr<Node>& nodeptr = nodes_[i];

            nodeptr = nodePool_->CreateChild(String::EMPTY, LOCAL);
            GameHelpers::SpawnGOtoNode(context_, GOT_, nodeptr);
            nodeptr->isPoolNode_ = true;
            nodeptr->SetVar(GOA::GOT, GOT_);
            nodeptr->SetVar(GOA::CLIENTID, clientID_);
            nodeptr->SetEnabled(false);
            nodeptr->isPoolNode_ = nodeptr->isInPool_ = true;

            GOC_Destroyer* destroyer = nodeptr->GetComponent<GOC_Destroyer>();
            if (destroyer)
            {
                destroyer->SetEnableLifeNotifier(false);
                destroyer->SetDestroyMode(FASTPOOLRESTORE);
            }

#ifdef ACTIVE_LAYERMATERIALS
            PODVector<StaticSprite2D*> staticsprites;
            nodeptr->GetComponents<StaticSprite2D>(staticsprites, true);
            for (PODVector<StaticSprite2D*>::Iterator it = staticsprites.Begin(); it != staticsprites.End(); ++it)
            {
                (*it)->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
            }
            PODVector<AnimatedSprite2D*> animatedsprites;
            nodeptr->GetComponents<AnimatedSprite2D>(animatedsprites, true);
            for (PODVector<AnimatedSprite2D*>::Iterator it = animatedsprites.Begin(); it != animatedsprites.End(); ++it)
            {
                if ((*it)->GetRenderTargetAttr().Empty())
                    (*it)->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERACTORS]);
            }
#endif

            freenodes_.Push(nodeptr.Get());
        }

//        URHO3D_LOGINFOF("GO_Pool() - GOT=%u type=%s - clientID=%d Resize from %u to %u ... OK !", GOT_.Value(), GOT::GetType(GOT_).CString(), clientID_, oldsize, size);
    }
}

void GO_Pool::Restore()
{
    for (Vector<WeakPtr<Node> >::Iterator it = nodes_.Begin(); it != nodes_.End(); ++it)
        FreeGO(it->Get());
}

void GO_Pool::TagNodes(const String& tag)
{
    for (unsigned i = 0; i < nodes_.Size(); i++)
        nodes_[i]->AddTag(tag);
}

Vector<StringHash> GO_Pools::poolHashs_;
Vector<Vector<GO_Pool> > GO_Pools::pools_;
WeakPtr<Node> GO_Pools::nodePool_;
GO_Pools* GO_Pools::gopools_ = 0;

void GO_Pools::Start()
{
    URHO3D_LOGINFO("GO_Pools() - Start !");

    if (!nodePool_)
    {
        Node* nodepool = GameContext::Get().rootScene_->GetChild("Pool");
        if (!nodepool)
            nodepool = GameContext::Get().rootScene_->CreateChild("Pool", LOCAL);

        nodePool_ = nodepool;
    }

    if (!gopools_)
        gopools_ = new GO_Pools(nodePool_->GetContext());

    GO_Pools::GetOrCreatePool(StringHash("ABI_Shooter"), "ABI");
    GO_Pools::GetOrCreatePool(StringHash("ABI_Grapin"), "ABI");
    GO_Pools::GetOrCreatePool(StringHash("ABIBomb"), "ABI");

    gopools_->SubscribeToEvents();
}

void GO_Pools::SubscribeToEvents()
{
    SubscribeToEvent(GO_DESTROY, URHO3D_HANDLER(GO_Pools, HandleObjectDestroy));
}

void GO_Pools::Restore()
{
    URHO3D_LOGINFO("GO_Pools() - Restore !");

    if (gopools_)
        gopools_->UnsubscribeFromEvent(GO_DESTROY);

    for (Vector<Vector<GO_Pool> >::Iterator it = pools_.Begin(); it != pools_.End(); ++it)
        for (Vector<GO_Pool>::Iterator jt = it->Begin(); jt != it->End(); ++jt)
            jt->Restore();
}

void GO_Pools::Reset()
{
    if (nodePool_)
        nodePool_->Remove();

    nodePool_.Reset();
    poolHashs_.Clear();
    pools_.Clear();

    delete gopools_;
    gopools_ = 0;

    URHO3D_LOGINFO("GO_Pools() - Reset ... OK !");
}

void GO_Pools::AddPool(const StringHash& got, unsigned numGOs, const String& tag)
{
    if (!nodePool_)
        return;

    if (!poolHashs_.Contains(got))
    {
        pools_.Resize(pools_.Size()+1);
        poolHashs_.Push(got);

        pools_.Back().Resize(GameContext::Get().MAX_NUMNETPLAYERS+1);

        for (int i=0; i <= GameContext::Get().MAX_NUMNETPLAYERS; i++)
        {
            GO_Pool& pool = pools_.Back()[i];
            pool.Init(nodePool_, got, i);
            pool.Resize(numGOs);
            if (!tag.Empty())
                pool.TagNodes(tag);
        }

        URHO3D_LOGINFOF("GO_Pools() - AddPool : new pool(GOT=%u type=%s numGOs=%u ids=%u->%u)",
                        got.Value(), GOT::GetType(got).CString(), numGOs, pools_.Back().Front().GetFirstID(), pools_.Back().Back().GetLastID());
    }
    else
    {
        Vector<StringHash>::ConstIterator it = poolHashs_.Find(got);
        Vector<GO_Pool>& pools = pools_[it - poolHashs_.Begin()];

        for (int i=0; i <= GameContext::Get().MAX_NUMNETPLAYERS; i++)
        {
            if (pools[i].GetSize() < numGOs)
            {
                pools[i].Resize(numGOs);
//                URHO3D_LOGINFOF("GO_Pools() - AddPool : resize pool(GOT=%u type=%s) clientid=%d, GO_Pools Size=%u", got.Value(), GOT::GetType(got).CString(), i, pools[i].GetSize());
            }
            if (!tag.Empty())
                pools[i].TagNodes(tag);
        }
    }
}

GO_Pool* GO_Pools::GetPool(const StringHash& got, int clientid)
{
    Vector<StringHash>::ConstIterator it = poolHashs_.Find(got);
    if (it != poolHashs_.End())
    {
        if (clientid == -1)
            clientid = GameNetwork::Get() ? GameNetwork::Get()->GetClientID() : 0;

        return &(pools_[it - poolHashs_.Begin()][clientid]);
    }

    return 0;
}

GO_Pool* GO_Pools::GetOrCreatePool(const StringHash& got, const String& tag, int clientid)
{
    if (clientid == -1)
        clientid = GameNetwork::Get() ? GameNetwork::Get()->GetClientID() : 0;

    Vector<StringHash>::ConstIterator it = poolHashs_.Find(got);
    unsigned index = poolHashs_.Size();
    if (it == poolHashs_.End())
    {
        const GOTInfo& gotinfo = GOT::GetConstInfo(got);
        unsigned poolqty = (&gotinfo != &GOTInfo::EMPTY ? (gotinfo.poolqty_ ? gotinfo.poolqty_ : DEFAULT_NUMGOS) : DEFAULT_NUMGOS);
        AddPool(got, poolqty, tag);
    }
    else
        index = it - poolHashs_.Begin();

    GO_Pool* pool = pools_.Size() > index ? &(pools_[index][clientid]) : 0;
    if (!pool)
        URHO3D_LOGERRORF("GO_Pools - GetOrCreatePool() : no pool for GOT=%s(%u)", GOT::GetType(got).CString(), got.Value());

    return pool;
}

Node* GO_Pools::Get(const StringHash& got, int clientid)
{
    GO_Pool* pool = GO_Pools::GetPool(got, clientid);
    return pool ? pool->GetGO() : 0;
}

Node* GO_Pools::Use(Node* node)
{
    const Variant& got = node->GetVar(GOA::GOT);
    if (got == Variant::EMPTY)
        return 0;

    const Variant& clientidvar = node->GetVar(GOA::CLIENTID);
    int clientid = clientidvar != Variant::EMPTY ? node->GetVar(GOA::CLIENTID).GetInt() : 0;

    GO_Pool* pool = GO_Pools::GetPool(node->GetVar(GOA::GOT).GetStringHash(), clientid);
    return pool ? pool->UseGO(node) : 0;
}

void GO_Pools::Free(Node* node)
{
    const Variant& got = node->GetVar(GOA::GOT);
    if (got == Variant::EMPTY)
        return;

    const Variant& clientidvar = node->GetVar(GOA::CLIENTID);
    int clientid = clientidvar != Variant::EMPTY ? node->GetVar(GOA::CLIENTID).GetInt() : 0;

    GO_Pool* pool = GO_Pools::GetPool(node->GetVar(GOA::GOT).GetStringHash(), clientid);
    if (pool)
        pool->FreeGO(node);
}

void GO_Pools::HandleObjectDestroy(StringHash eventType, VariantMap& eventData)
{
    Node* node = nodePool_->GetScene()->GetNode(eventData[Go_Destroy::GO_ID].GetUInt());
    if (node && node->GetParent() == nodePool_)
    {
//        URHO3D_LOGINFOF("GO_Pools() - HandleObjectDestroy - FreeGO node=%u", node);
        GO_Pools::Free(node);
    }
}
