#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/SmoothedTransform.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include "DefsViews.h"
#include "DefsWorld.h"

#include "GameOptionsTest.h"
#include "GameAttributes.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameRand.h"

#include "GOC_Destroyer.h"

#include "ObjectPool.h"

#define DEFAULT_NUMOBJECTS 10


ObjectPoolCategory::ObjectPoolCategory()
{ }

ObjectPoolCategory::ObjectPoolCategory(const ObjectPoolCategory& obj)
{ }

ObjectPoolCategory::~ObjectPoolCategory()
{ }

void ObjectPoolCategory::SetIds(CreateMode mode, unsigned firstNodeID, unsigned firstComponentID)
{
    if (mode == REPLICATED)
    {
        currentReplicatedNodeID_ = firstReplicatedNodeID_ = firstNodeID;
        currentReplicatedComponentID_ = firstReplicatedComponentID_ = firstComponentID;
    }
    else
    {
        firstNodeID_ = firstNodeID;
        firstComponentID_ = firstComponentID;
    }
}

void ObjectPoolCategory::SynchronizeReplicatedNodes(unsigned startNodeID)
{
    if (startNodeID >= firstReplicatedNodeID_ && startNodeID <= lastReplicatedNodeID_)
    {
        currentReplicatedNodeID_ = startNodeID;
        currentReplicatedComponentID_ = firstReplicatedComponentID_ + (startNodeID-firstReplicatedNodeID_) * numComponentIdsByObject_;
    }
}

#define NUMLATENCYOBJECTS 0U

bool ObjectPoolCategory::HasReplicatedMode() const
{
    return replicatedState_;
}

bool ObjectPoolCategory::IsNodeInPool(Node* node) const
{
    return node->isPoolNode_ && node->isInPool_;
//    return !node->HasTag("InUse");
}

unsigned ObjectPoolCategory::GetNextReplicatedNodeID() const
{
    return currentReplicatedNodeID_+NUMLATENCYOBJECTS+1 <= lastReplicatedNodeID_ ? currentReplicatedNodeID_ + numNodeIdsByObject_*(NUMLATENCYOBJECTS+1) : firstReplicatedNodeID_;
}

void ObjectPoolCategory::SetCurrentReplicatedIDs(unsigned id)
{
    if (currentReplicatedNodeID_ != id)
    {
        currentReplicatedNodeID_ = id;
        currentReplicatedComponentID_ = firstReplicatedComponentID_ + numComponentIdsByObject_ * (id - firstReplicatedNodeID_);
    }
}

void ObjectPoolCategory::ChangeToReplicatedID(Node* node, unsigned replicatedid)
{
    if (replicatedid == 0)
    {
        usedLocalIds_[currentReplicatedNodeID_] = NodeComponentID(node->GetID(), node->GetNumComponents() ? node->GetComponents().Front()->GetID() : firstComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... before NodeIDChanged ...", GOT::GetType(GOT_).CString(), node->GetID());
        nodeCategory_->GetScene()->NodeIDChanged(node, currentReplicatedNodeID_, currentReplicatedComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... after NodeIDChanged !", GOT::GetType(GOT_).CString(), node->GetID());

        currentReplicatedNodeID_ += numNodeIdsByObject_;
        currentReplicatedComponentID_ += numComponentIdsByObject_;

        if (currentReplicatedNodeID_ > lastReplicatedNodeID_)
        {
            currentReplicatedNodeID_ = firstReplicatedNodeID_;
            currentReplicatedComponentID_ = firstReplicatedComponentID_;
        }

        return;
    }

    if (node->GetID() == replicatedid)
    {
        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... with replicatedid=%u same id !", GOT::GetType(GOT_).CString(), node->GetID(), replicatedid);
        return;
    }

    if (replicatedid >= firstReplicatedNodeID_ && replicatedid <= lastReplicatedNodeID_)
    {
        SetCurrentReplicatedIDs(replicatedid);
        usedLocalIds_[currentReplicatedNodeID_] = NodeComponentID(node->GetID(), node->GetNumComponents() ? node->GetComponents().Front()->GetID() : firstComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... before NodeIDChanged with replicatedid=%u ...", GOT::GetType(GOT_).CString(), node->GetID(), replicatedid);
        nodeCategory_->GetScene()->NodeIDChanged(node, currentReplicatedNodeID_, currentReplicatedComponentID_);
//        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... after NodeIDChanged !", GOT::GetType(GOT_).CString(), node->GetID());
    }
    else
    {
        URHO3D_LOGWARNINGF("ObjectPoolCategory() - ChangeToReplicatedID : %s(%u) ... with replicatedid=%u not in the replicatedRange=(%u->%u) ...",
                           GOT::GetType(GOT_).CString(), node->GetID(), replicatedid, firstReplicatedNodeID_, lastReplicatedNodeID_);
    }
}

void ObjectPoolCategory::ChangeToID(Node* node, unsigned newid)
{
    if (replicatedState_ && newid != LOCAL && newid < FIRST_LOCAL_ID)
    {
        ChangeToReplicatedID(node, newid);
    }
    else if (newid >= firstNodeID_ && newid <= lastNodeID_)
    {
        Scene* scene = nodeCategory_->GetScene();
        Node* node2 = scene->GetNode(newid);
        if (node2)
            scene->NodeIDChanged(node2, node->GetID(), 0);
        scene->NodeIDChanged(node, newid, 0);
    }
    else
    {
        URHO3D_LOGERRORF("ObjectPoolCategory() - ChangeToID : type=%s(%u) id=%u ptr=%u ... can't change to id=%u (not in the range) !",
                         GOT::GetType(GOT_).CString(), GOT_.Value(), node->GetID(), node, newid);
    }
}

Node* ObjectPoolCategory::GetPoolNode(unsigned newid)
{
    if (!freenodes_.Empty())
    {
        Node* node;
        unsigned oldid;

        // specific LOCAL node
        if (newid >= firstNodeID_ && newid <= lastNodeID_)
        {
            node = GameContext::Get().rootScene_->GetNode(newid);
            freenodes_.RemoveSwap(node);
            oldid = node->GetID();
        }
        else
        {
            node = freenodes_.Back();

            if (!nodeCategory_)
            {
                URHO3D_LOGERRORF("ObjectPoolCategory() - GetPoolNode : type=%s(%u) id=%u ptr=%u ... no Category Node !",
                                 GOT::GetType(GOT_).CString(), GOT_.Value(), node->GetID(), node);
                return 0;
            }

            freenodes_.Pop();

            // if Replicated State, change to ReplicatedIDs
            unsigned oldid = node->GetID();

            if (replicatedState_ && newid != LOCAL  && newid < FIRST_LOCAL_ID)
            {
                ChangeToReplicatedID(node, newid);
            }
        }

//        node->AddTag("InUse");
        node->isInPool_ = false;

//        URHO3D_LOGINFOF("ObjectPoolCategory() - GetPoolNode : %s(oldid=%u replicatedID=%u newid=%u(check=%u)) free=%u/%u",
//                        GOT::GetType(GOT_).CString(), oldid, currentReplicatedNodeID_, newid, node->GetID(), freenodes_.Size(), nodes_.Size());

        return node;
    }

//    URHO3D_LOGERRORF("ObjectPoolCategory() - GetPoolNode : %s(hash=%u) ... No Free Node !!!", GOT::GetType(GOT_).CString(), GOT_.Value());
    return 0;
}

bool ObjectPoolCategory::FreePoolNode(Node* node, bool cleanDependences)
{
    if (!freenodes_.Contains(node))
    {
        // clean dependences in the components (ex : clear triggers in animatedSprite2D)
        if (cleanDependences)
        {
            const Vector<SharedPtr<Component> >& components = node->GetComponents();
            for (unsigned i=0; i < components.Size(); i++)
                components[i]->CleanDependences();
        }

        unsigned oldid = node->GetID();
        unsigned newid = oldid;

        // if ReplicatedID => Restore to LocalID
        if (oldid && replicatedState_ && oldid < FIRST_LOCAL_ID)
        {
            node->CleanupReplicationStates();

            HashMap<unsigned, NodeComponentID >::Iterator it = usedLocalIds_.Find(oldid);

            if (it != usedLocalIds_.End())
            {
                {
                    const NodeComponentID& localids = it->second_;
                    newid = localids.nodeID_;
//                    URHO3D_LOGWARNINGF("ObjectPoolCategory() - FreePoolNode : %s(%u) restore to localid=%u ... before NodeIDChanged !", GOT::GetType(GOT_).CString(), oldid, newid);
                    nodeCategory_->GetScene()->NodeIDChanged(node, newid, localids.componentID_);
                    newid = node->GetID();

//                    URHO3D_LOGWARNINGF("ObjectPoolCategory() - FreePoolNode : %s(%u) ... after NodeIDChanged !", GOT::GetType(GOT_).CString(), newid);
                }

                it = usedLocalIds_.Erase(it);
            }
            else
            {
                URHO3D_LOGERRORF("ObjectPoolCategory() - FreePoolNode : %s(%u) ... don't find a localID to restore to !", GOT::GetType(GOT_).CString(), oldid);
                return false;
            }
        }

//        URHO3D_LOGINFOF("ObjectPoolCategory() - FreePoolNode : %s(%u-local=%u) cleanDependences=%s ...", GOT::GetType(GOT_).CString(), newid, oldid, cleanDependences ? "true":"false");

        if (newid < firstNodeID_ || newid > lastNodeID_)
        {
            URHO3D_LOGERRORF("ObjectPoolCategory() - FreePoolNode : type=%s oldid=%u newid=%u nodeptr=%u ... out of local category range (%u->%u) : Remove it !",
                             GOT::GetType(GOT_).CString(), oldid, newid, node, firstNodeID_, lastNodeID_);
//            Dump(true);
            node->Remove();
            return false;
        }

//        URHO3D_LOGINFOF("ObjectPoolCategory() - FreePoolNode : type=%s CopyAttributes Before nodeEnabled=%s ... ",
//                GOT::GetType(GOT_).CString(), node->IsEnabled() ? "true" : "false");

        GameHelpers::CopyAttributes(template_, node, false, false);

        node->RemoveAllTags();
        node->isInPool_ = true;

//        URHO3D_LOGINFOF("ObjectPoolCategory() - FreePoolNode : type=%s CopyAttributes After nodeEnabled=%s !",
//                GOT::GetType(GOT_).CString(), node->IsEnabled() ? "true" : "false");

        if (gotinfo_->scaleVariation_)
            ApplyScaleVariation(node);

        /// Restore poolnode to Category node
        // Prevent SceneCleaner to Remove ObjectPool Nodes
        // For Example : GOC_BodyExploder2D reparent ExplodedParts to the mapnode
        // For Example : GOC_Animator2D::SpawnAnimation reparent animation to the localmapnode
        if (node->GetParent() != nodeCategory_)
            nodeCategory_->AddChild(node);

        // Need to Reset if gocdestroyer : gocdestroyer don't have attribute Enabled registered (that could be set by applyattributes)
        GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
        if (destroyer)
            destroyer->Reset(false);
        else
            node->SetEnabledRecursive(false);
        //node->SetEnabled(false);

        node->ApplyAttributes();

        freenodes_.Push(node);

//        URHO3D_LOGINFOF("ObjectPoolCategory() - FreePoolNode : type=%s oldid=%u newid=%u ptr=%u restored to the pool (free=%u/%u) !",
//                        GOT::GetType(GOT_).CString(), oldid, newid, node, freenodes_.Size(), nodes_.Size());

        return true;
    }

    return false;
}

void ObjectPoolCategory::ApplyScaleVariation(Node* node)
{
    unsigned rand = node->GetID();
    float scalef = (1.f-(float)gotinfo_->scaleVariation_*0.05f) + 0.1f * (float)(rand%gotinfo_->scaleVariation_);

//    URHO3D_LOGINFOF("ObjectPoolCategory() - ApplyScaleVariation : type=%s(%u) : node=%u ... scalef=%f", GOT::GetType(GOT_).CString(), GOT_.Value(), node->GetID(), scalef);
    node->SetScale2D(node->GetScale2D()*scalef);
}

bool ObjectPoolCategory::Create(bool replicate, const StringHash& got, Node* nodePool, Node* templateNode, unsigned* ids)
{
    template_.Reset();
    nodes_.Clear();
    freenodes_.Clear();

    if (!templateNode)
    {
        return false;
    }

    replicatedState_ = replicate;
    template_ = templateNode;
    GOT_ = got;
    gotinfo_ = &GOT::GetConstInfo(GOT_);
    requestedSize_ = 0;

    if (nodeCategory_)
    {
        nodeCategory_->RemoveAllChildren();
        nodeCategory_.Reset();
    }
    nodeCategory_ = nodePool->CreateChild(GOT::GetType(GOT_), LOCAL, ids[0]);
    nodeCategory_->SetVar(GOA::GOT, GOT_);
    nodeCategory_->SetTemporary(true);

    // Set Template Node
    template_->SetVar(GOA::GOT, GOT_.Value());
    template_->SetEnabledRecursive(false);
//	template_->SetEnabled(false);

    // Mark Template before Cloning
    template_->isPoolNode_ = true;
    for (unsigned j=0; j < template_->GetChildren().Size(); j++)
        template_->GetChildren()[j]->isPoolNode_ = true;

    // Create Smooth Transform Component in LOCAL and disable ChangeMode for this component
    if (replicatedState_)
    {
        SmoothedTransform* smooth = template_->GetOrCreateComponent<SmoothedTransform>(LOCAL);
        smooth->SetChangeModeEnable(false);
    }

    // Set Destroyer to ObjectPool Mode
    GOC_Destroyer* destroyer = template_->GetComponent<GOC_Destroyer>();
    if (destroyer)
        destroyer->SetDestroyMode(POOLRESTORE);

    // Set Ids
    SetIds(LOCAL, ids[0] + 1, ids[1]);
    if (replicatedState_)
        SetIds(REPLICATED, ids[2], ids[3]);
    else
        SetIds(REPLICATED, 0, 0);

    numNodeIdsByObject_ = 1 + template_->GetNumChildren(true);
    numComponentIdsByObject_ = template_->GetNumComponents(true);

//    numNodeIdsByObject_ = 1 + template_->GetNumChildren();
//    numComponentIdsByObject_ = template_->GetNumComponents();

    URHO3D_LOGINFOF("ObjectPoolCategory() - Create : type=%s(%u) ... CreateMode=%s templateID=%u NodeCategoryId=%u LOCAL n=%u c=%u REPLI n=%u c=%u(%u) ... OK !",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), replicatedState_ ? "REPLICATED":"LOCAL", template_->GetID(), nodeCategory_->GetID(),
                    firstNodeID_, firstComponentID_, firstReplicatedNodeID_, firstReplicatedComponentID_, ids[3]);

//    GameHelpers::DumpNode(template_, 0, true);

    return true;
}

void ObjectPoolCategory::Resize(unsigned size)
{
    requestedSize_ = size;
    updateState_ = 0;

    nodes_.Reserve(size);
    freenodes_.Reserve(size);

    // Calculate the last Ids
    lastNodeID_ = firstNodeID_ + numNodeIdsByObject_ * size - 1;
    lastComponentID_ = firstComponentID_ + numComponentIdsByObject_ * size - 1;
    if (replicatedState_)
    {
        lastReplicatedNodeID_ = firstReplicatedNodeID_ + numNodeIdsByObject_ * size - 1 ;
        lastReplicatedComponentID_ = firstReplicatedComponentID_ + numComponentIdsByObject_ * size - 1;
    }
    else
    {
        lastReplicatedNodeID_ = 0;
        lastReplicatedComponentID_ = 0;
    }

    URHO3D_LOGINFOF("ObjectPoolCategory() - Resize type=%s(%u) CreateMode=%s templateID=%u size=%u LOCAL n=%u->%u c=%u->%u REPLI n=%u->%u c=%u->%u ... OK !",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), replicatedState_ ? "REPLICATED":"LOCAL", template_->GetID(), size,
                    firstNodeID_, lastNodeID_, firstComponentID_, lastComponentID_,
                    firstReplicatedNodeID_, lastReplicatedNodeID_, firstReplicatedComponentID_, lastReplicatedComponentID_);
}

bool ObjectPoolCategory::Update(HiresTimer* timer, const long long& delay)
{
    if (updateState_ == 0)
    {
        unsigned nodeid = firstNodeID_ + numNodeIdsByObject_ * nodes_.Size();
        unsigned componentid = firstComponentID_ + numComponentIdsByObject_ * nodes_.Size();

        URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize type=%s(%u) ... Clone Template => PoolSize=%u/%u ... (nodeid=%u, componentid=%u)",
                        GOT::GetType(GOT_).CString(), GOT_.Value(), nodes_.Size(), requestedSize_, nodeid, componentid);

        while (nodes_.Size() < requestedSize_)
        {
            SharedPtr<Node> node(template_->Clone(LOCAL, false, nodeid, componentid));
            //Node* Clone(CreateMode mode = REPLICATED, bool applyAttr = true, unsigned nodeid=0, unsigned componentid=0, Node* parent=0);

            if (!node)
            {
                URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize Error => Can't Clone Template !");
                return true;
            }

            nodes_.Push(node);
            freenodes_.Push(node.Get());

            if (nodes_.Size() < requestedSize_ && TimeOver(timer, delay))
                return false;

            nodeid += numNodeIdsByObject_;
            componentid += numComponentIdsByObject_;
        }

        updateState_++;
    }

    if (updateState_ > 0)
    {
        unsigned inode = updateState_-1;
        Node* node;

        URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize type=%s(%u) ... Apply Attributes => PoolSize=%u/%u ...",
                        GOT::GetType(GOT_).CString(), GOT_.Value(), inode, requestedSize_);

        while (inode < requestedSize_)
        {
            node = nodes_[inode];

            if (gotinfo_->scaleVariation_)
                ApplyScaleVariation(node);

            /// TODO : supprimer ceci après implementation du charactermapping avec bodyexploder2D
            AnimatedSprite2D* animatedsprite = node->GetComponent<AnimatedSprite2D>();
            if (animatedsprite)
            {
                int entityid = 0;
                GameHelpers::SetEntityVariation(animatedsprite, entityid, gotinfo_, false, false);
            }

            node->ApplyAttributes();

            inode++;

            if (inode < requestedSize_ && TimeOver(timer, delay))
            {
                updateState_ = inode + 1;
                return false;
            }
        }

        URHO3D_LOGINFOF("ObjectPoolCategory() - Update : Resize type=%s(%u) ... OK !", GOT::GetType(GOT_).CString(), GOT_.Value());

//        Dump();
        return true;
    }

    return false;
}


/// restore all the categories to the pool
void ObjectPoolCategory::RestoreObjects(bool allObjects)
{
    if (GetFreeSize() == GetSize())
        return;

    URHO3D_LOGINFOF("ObjectPoolCategory() - RestoreObjects : Category = %s(%u)  ... templateID=%u IDs=%u->%u",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), template_->GetID(), firstNodeID_, lastNodeID_);

    bool r;
    unsigned count=0;

    if (!allObjects)
    {
        Node* node;
        for (unsigned i=0; i < GetSize(); i++)
        {
            node = nodes_[i];

//            if (replicatedState_ && ObjectPool::createChildMode_ == REPLICATED)
//                node->GetScene()->NodeIDChanged(node, linkedIds_[node->GetID()]);

//            URHO3D_LOGINFOF(" -> Object(%u) : Ptr=%u ID=%u...", i, node, node->GetID());

            if (node->HasTag("UsedAsPart"))
                continue;

            count++;
            r = FreePoolNode(node, true);
        }
    }
    else
    {
        for (unsigned i=0; i < GetSize(); i++)
        {
            count++;
            r = FreePoolNode(nodes_[i], true);
        }
    }

    // Reset IDs Generator
    currentReplicatedNodeID_ = firstReplicatedNodeID_;
    currentReplicatedComponentID_ = firstReplicatedComponentID_;

    URHO3D_LOGINFOF("ObjectPoolCategory() - RestoreObjects : Category = %s(%u) - RestoreMode=%s ... %u/%u restored OK !",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), allObjects ? "allObjects" : "NotUsedAsPart", count, GetSize());

//    Dump(true);
}

void ObjectPoolCategory::Dump(bool logonlyerrors) const
{
    URHO3D_LOGINFOF("ObjectPoolCategory() - Dump() : category=%s(%u) - free=%u/%u template=%u localids=(%u->%u) replicatedids=(%u->%u)",
                    GOT::GetType(GOT_).CString(), GOT_.Value(), GetFreeSize(), GetSize(), template_->GetID(),
                    firstNodeID_, lastNodeID_, firstReplicatedNodeID_, lastReplicatedNodeID_);

    if (!logonlyerrors)
        for (unsigned i=0; i <freenodes_.Size(); i++)
        {
            Node* node = freenodes_[i];
            if (!node || node->GetID() < firstNodeID_ || node->GetID() > lastNodeID_)
                URHO3D_LOGERRORF("-> freenode[%d] : ptr=%u id=%u", i, node, node ? node->GetID():0);
            else
                URHO3D_LOGINFOF("-> freenode[%d] : ptr=%u id=%u", i, node, node ? node->GetID():0);
        }
    else
        for (unsigned i=0; i <freenodes_.Size(); i++)
        {
            Node* node = freenodes_[i];
            if (!node || node->GetID() < firstNodeID_ || node->GetID() > lastNodeID_)
                URHO3D_LOGERRORF("-> freenode[%d] : ptr=%u id=%u", i, node, node ? node->GetID():0);
        }
}



ObjectPool* ObjectPool::pool_ = 0;
String ObjectPool::debugTxt_;
bool ObjectPool::forceLocalMode_ = true;

void ObjectPool::Reset(Node* node)
{
    if (pool_)
    {
        delete pool_;
        pool_ = 0;

        if (!node)
        {
            URHO3D_LOGINFOF("ObjectPool() - Reset ... OK !");
            return;
        }
    }

    if (node)
    {
        pool_ = new ObjectPool(node->GetContext());
        pool_->categories_.Clear();
        pool_->nodePool_ = node->CreateChild("ObjectPool", LOCAL);

        URHO3D_LOGINFOF("ObjectPool() - Reset ... Root=%u NodePool=%u ... OK !", node->GetID(), pool_->nodePool_->GetID());
    }
}

ObjectPool::ObjectPool(Context* context) :
    Object(context),
    firstLocalNodeID_(0),
    createstate_(0)
{
    SetForceLocalMode(true);
}

ObjectPool::~ObjectPool()
{
    Stop();
}

/// restore all the categories to the pool
void ObjectPool::RestoreCategories(bool allObjects)
{
    URHO3D_LOGINFO("-----------------------------------------------------");
    URHO3D_LOGINFO("- ObjectPool() - RestoreCategories ...              -");
    URHO3D_LOGINFO("-----------------------------------------------------");

    for (HashMap<StringHash, ObjectPoolCategory >::Iterator it=categories_.Begin(); it!=categories_.End(); ++it)
        it->second_.RestoreObjects(allObjects);

    SetForceLocalMode(true);

    URHO3D_LOGINFO("-----------------------------------------------------");
    URHO3D_LOGINFO("- ObjectPool() - RestoreCategories ...          OK! -");
    URHO3D_LOGINFO("-----------------------------------------------------");
}

void ObjectPool::Stop()
{
    URHO3D_LOGINFO("ObjectPool() - Stop ...");

    createstate_ = 0;
    categories_.Clear();

    GameHelpers::RemoveNodeSafe(nodePool_, false);
    nodePool_.Reset();

    SetForceLocalMode(true);

    URHO3D_LOGINFO("ObjectPool() - Stop ... OK !");
}

ObjectPoolCategory* ObjectPool::CreateCategory(const GOTInfo& info, Node* templateNode, unsigned* categoryids)
{
    const StringHash& got = info.got_;

    if (GOT::GetType(got) == String::EMPTY || !templateNode)
        return 0;

    HashMap<StringHash, ObjectPoolCategory >::Iterator it = categories_.Find(got);
    ObjectPoolCategory& category = categories_[got];

    if (it == categories_.End())
    {
        if (!category.Create(info.replicatedMode_, got, nodePool_, templateNode, categoryids))
        {
            categories_.Erase(got);
            URHO3D_LOGERRORF("ObjectPool() - CreateCategory : can't create category(GOT=%u type=%s) !", got.Value(), GOT::GetType(got).CString());
            return 0;
        }
    }

    category.Resize(info.poolqty_);

    return &category;
}

bool ObjectPool::CreateCategories(Scene* scene, const HashMap<StringHash, GOTInfo >& infos, const HashMap<StringHash, WeakPtr<Node> >& templates, HiresTimer* timer, const long long& delay)
{
    // Reserve Scene Ids
    if (createstate_ == 0)
    {
        // Get Scene Next Free Ids
        firstReplicatedNodeID_ = lastReplicatedNodeID_ = scene->GetNextFreeReplicatedNodeID();
        firstReplicatedComponentID_ = lastReplicatedComponentID_ = scene->GetNextFreeReplicatedComponentID();
        firstLocalNodeID_ = lastLocalNodeID_ = scene->GetNextFreeLocalNodeID();
        firstLocalComponentID_ = lastLocalComponentID_ = scene->GetNextFreeLocalComponentID() + 0x01000000;

//        GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, true);

        for (HashMap<StringHash, GOTInfo >::ConstIterator it = infos.Begin(); it != infos.End(); ++it)
        {
            const StringHash& got = it->first_;
            const GOTInfo& info = it->second_;

            if (info.pooltype_ != GOT_ObjectPool)
                continue;

            if (!info.poolqty_)
            {
                URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) no poolqty => Skip !", info.typename_.CString(), got.Value());
                continue;
            }

            const HashMap<StringHash, WeakPtr<Node> >::ConstIterator itt = templates.Find(got);
            if (itt == templates.End())
            {
                URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) no template => Skip !", info.typename_.CString(), got.Value());
                continue;
            }

            Node* node = itt->second_;
            unsigned numObjects = info.poolqty_;

            unsigned numNodes = 1 + node->GetNumChildren(true);
            unsigned numComponents = node->GetNumComponents(true);

//            unsigned numNodes = 1 + node->GetNumChildren();
//            unsigned numComponents = node->GetNumComponents();
            if (info.replicatedMode_)
                numComponents++;

            URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) (numNodes=%u numComponents=%u) Reserve LOCAL nodeCategory=%u nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                            info.typename_.CString(), got.Value(), numNodes, numComponents, lastLocalNodeID_, lastLocalNodeID_ + 1, lastLocalNodeID_ + numObjects * numNodes,
                            lastLocalComponentID_, lastLocalComponentID_ + numObjects * numComponents - 1);

            if (info.replicatedMode_)
                URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Object=%s(%u) Reserve REPLI nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                                info.typename_.CString(), got.Value(), lastReplicatedNodeID_, lastReplicatedNodeID_ + numObjects * numNodes - 1,
                                lastReplicatedComponentID_, lastReplicatedComponentID_ + numObjects * numComponents - 1);

            // Calculate the next Local Ids
            {
                lastLocalNodeID_ += numObjects * numNodes + 1; // Add Category Object Nodes + 1 Category Local Node
                lastLocalComponentID_ += numObjects * numComponents;

                // Calculate the next Replicate Ids
                if (info.replicatedMode_)
                {
                    lastReplicatedNodeID_ += numObjects * numNodes; // Add Category Object Nodes
                    lastReplicatedComponentID_ += numObjects * numComponents;
                }
            }
        }

        // Set Scene Reserved Ids
        scene->SetFirstReservedLocalNodeID(firstLocalNodeID_);
        scene->SetLastReservedLocalNodeID(lastLocalNodeID_);
        scene->SetFirstReservedReplicateNodeID(firstReplicatedNodeID_);
        scene->SetLastReservedReplicateNodeID(lastReplicatedNodeID_);
        scene->SetFirstReservedLocalComponentID(firstLocalComponentID_);
        scene->SetLastReservedLocalComponentID(lastLocalComponentID_);
        scene->SetFirstReservedReplicateComponentID(firstReplicatedComponentID_);
        scene->SetLastReservedReplicateComponentID(lastReplicatedComponentID_);

        createstate_++;

        URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Scene Reserve LOCAL nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                        firstLocalNodeID_, lastLocalNodeID_, firstLocalComponentID_, lastLocalComponentID_);
        URHO3D_LOGINFOF("ObjectPool() - CreateCategories : Scene Reserve REPLI nodesIds(%u->%u) ComponentsIds(%u->%u) !",
                        firstReplicatedNodeID_, lastReplicatedNodeID_, firstReplicatedComponentID_, lastReplicatedComponentID_);

//        GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, false);

        return false;
    }

    // Create Categories
    if (createstate_ == 1)
    {
        unsigned categoryids[4];
        categoryids[0] = firstLocalNodeID_;
        categoryids[1] = firstLocalComponentID_;
        categoryids[2] = firstReplicatedNodeID_;
        categoryids[3] = firstReplicatedComponentID_;

//        URHO3D_LOGERROR(GameHelpers::DumpHashMap(infos));
//        URHO3D_LOGERROR(GameHelpers::DumpHashMap(templates));

        for (HashMap<StringHash, GOTInfo >::ConstIterator it = infos.Begin(); it != infos.End(); ++it)
        {
            const GOTInfo& info = it->second_;

            if (info.pooltype_ != GOT_ObjectPool)
                continue;

            if (!info.poolqty_)
                continue;

            if (info.filename_.Empty())
                continue;

            const StringHash& got = it->first_;
            const HashMap<StringHash, WeakPtr<Node> >::ConstIterator itt = templates.Find(got);
            if (itt == templates.End())
                continue;

            Node* node = itt->second_;
            unsigned numObjects = info.poolqty_;

            unsigned numNodes = 1 + node->GetNumChildren(true);
            unsigned numComponents = node->GetNumComponents(true);

//            unsigned numNodes = 1 + node->GetNumChildren();
//            unsigned numComponents = node->GetNumComponents();

            if (info.replicatedMode_)
                numComponents++;

            ObjectPoolCategory* category = CreateCategory(info, node, categoryids);

            if (category)
                categoriesToUpdate_.Push(category);

            // Calculate the next Ids
            categoryids[0] += numObjects * numNodes + 1; // Add Category Object Nodes + 1 Category Local Node
            categoryids[1] += numObjects * numComponents;
            if (info.replicatedMode_)
            {
                categoryids[2] += numObjects * numNodes; // Add Category Object Nodes
                categoryids[3] += numObjects * numComponents;
            }
        }

        createstate_++;
//        URHO3D_LOGERRORF("ObjectPool() - CreateCategories : numCategoryToUpdate=%u !", categoriesToUpdate_.Size());
        return false;
    }

    // Update Categories Size
    if (createstate_ == 2)
    {
        ObjectPoolCategory* category;

        while (categoriesToUpdate_.Size())
        {
            category = categoriesToUpdate_.Front();

            assert(category);

            if (!category->Update(timer, delay))
                return false;

            categoriesToUpdate_.PopFront();
        }

        return true;
    }

    return false;
}

ObjectPoolCategory* ObjectPool::GetCategory(const StringHash& got)
{
    HashMap<StringHash, ObjectPoolCategory >::Iterator it = categories_.Find(got);

    return it != categories_.End() ? &(it->second_) : 0;
}

Node* ObjectPool::CreateChildIn(const StringHash& got, int& entityid, Node* parent, unsigned id, int viewZ, const NodeAttributes* nodeAttr, bool applyAttr, ObjectPoolCategory** retcategory, bool outsidePool)
{
    if (got == StringHash::ZERO)
        return 0;

    ObjectPoolCategory* category = 0;

    // Check Collectable Part property
    if ((GOT::GetTypeProperties(got) & GOT_CollectablePart) == GOT_CollectablePart)
        category = pool_->GetCategory(GOT::COLLECTABLEPART);
    else
        category = pool_->GetCategory(got);

    if (retcategory)
        *retcategory = category;

    if (!category && !outsidePool)
    {
        URHO3D_LOGERRORF("ObjectPool() - CreateChildIn : got=%s(%u) entityid=%d ... no category !", GOT::GetType(got).CString(), got.Value(), entityid);
        return 0;
    }

#ifdef OBJECTPOOL_LOCALIDSONLY
    if (id < FIRST_LOCAL_ID)
    {
#else
    if (forceLocalMode_ && id < FIRST_LOCAL_ID)
    {
        URHO3D_LOGWARNINGF("ObjectPool() - CreateChildIn node id=%u => force change to LOCAL !", id);
#endif // OBJECTPOOL_LOCALIDSONLY
        id = LOCAL;
    }

    Node* node = 0;

    if (category)
        node = category->GetPoolNode(id);

    if (!node)
    {
        if (outsidePool)
        {
            Node* templateNode = GOT::GetObject(got);
            node = GameContext::Get().rootScene_->CreateChild(String::EMPTY, LOCAL);
            if (templateNode)
            {
                GameHelpers::CopyAttributes(templateNode, node, false, false);
            }
            else if (!GameHelpers::LoadNodeXML(GameContext::Get().context_, node, GOT::GetObjectFile(got).CString(), LOCAL, false))
            {
                URHO3D_LOGERRORF("ObjectPool() - CreateChildIn : got=%s(%u) entityid=%d ... can't create node outsidePool !", GOT::GetType(got).CString(), got.Value(), entityid);
                return 0;
            }

            if (category && category->gotinfo_->scaleVariation_)
                category->ApplyScaleVariation(node);

            applyAttr = true;
        }
        else
        {
//			URHO3D_LOGERRORF("ObjectPool() - CreateChildIn : got=%s(%u) entityid=%d ... no more node !", GOT::GetType(got).CString(), got.Value(), entityid);
            return 0;
        }
    }
    else
    {
        outsidePool = false;
    }

    if (node->GetID() == 0)
    {
        URHO3D_LOGERRORF("ObjectPool() - CreateChildIn node id=0 !");
        return 0;
    }

    if (viewZ != NOVIEW)
        node->SetVar(GOA::ONVIEWZ, viewZ);

    // don't use setparent (hang program), prefer addchild
    if (parent)
    {
        Vector2 scale = node->GetWorldScale2D();

        parent->AddChild(node);

        // preserve the scale
        node->SetWorldScale2D(scale);
    }

    if (nodeAttr)
    {
//        URHO3D_LOGWARNINGF("ObjectPool() - CreateChildIn node id=%u => load nodeattributes !", id);
        GameHelpers::LoadNodeAttributes(node, *nodeAttr, false);
        viewZ = node->GetVar(GOA::ONVIEWZ).GetInt();
    }

    // 20/01/2023 : Important to SetEntityVariation after GameHelpers:LoadNodeAttributes() for the case of Map::SetEntities_Load() with the character Mapping of the Equipment
    // then the mapping will be well randomized when necessary.
    AnimatedSprite2D* animatedsprite = node->GetComponent<AnimatedSprite2D>();
    if (animatedsprite)
        GameHelpers::SetEntityVariation(animatedsprite, entityid, category ? category->gotinfo_ : 0, false, false);

//    if (parent)
//        node->SetEnabledRecursive(parent->IsEnabled());
//		node->SetEnabled(parent->IsEnabled());

    GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
    if (destroyer)
        destroyer->SetDestroyMode(outsidePool ? FREEMEMORY : POOLRESTORE);

    if (applyAttr)
    {
//        if (parent)
//            node->SetEnabledRecursive(parent->IsEnabled());

        node->ApplyAttributes();

        if (viewZ != NOVIEW)
            GameHelpers::SetDrawableLayerView(node, viewZ);
    }

//    URHO3D_LOGINFOF("ObjectPool() - CreateChild got=%s node=%s(%u) in parent=%s enabled=%s ... OK !", GOT::GetType(got).CString(),
//                    node->GetName().CString(), node->GetID(), parent ? parent->GetName().CString() : "", node->IsEnabled() ? "true" : "false");

    return node;
}

void ObjectPool::ChangeToReplicatedID(Node* node, unsigned newid)
{
    if (!node)
    {
        URHO3D_LOGERRORF("ObjectPool() - ChangeToReplicatedID : newid=%u no node !", newid);
        return;
    }

    StringHash got = node->GetVar(GOA::GOT).GetStringHash();
    ObjectPoolCategory* category = pool_->GetCategory(got);
    if (category)
    {
        if (category->IsNodeInPool(node))
        {
            URHO3D_LOGWARNINGF("ObjectPool() - ChangeToReplicatedID : node in the pool => don't change it !");
            return;
        }
        category->ChangeToReplicatedID(node, newid);
    }
    else
        URHO3D_LOGERRORF("ObjectPool() - ChangeToReplicatedID : newid=%u node=%s(%u) got=%s  no category ... OK !", newid, node->GetName().CString(), node->GetID(), GOT::GetType(got).CString());
}

void ObjectPool::ChangeToID(Node* node, unsigned newid)
{
    if (!node)
    {
        URHO3D_LOGERRORF("ObjectPool() - ChangeToID : newid=%u no node !", newid);
        return;
    }

    StringHash got(node->GetVar(GOA::GOT).GetStringHash());
    ObjectPoolCategory* category = pool_->GetCategory(got);
    if (category)
        category->ChangeToID(node, newid);
    else
        URHO3D_LOGERRORF("ObjectPool() - ChangeToID : newid=%u node=%s(%u) got=%s no category ... OK !", newid, node->GetName().CString(), node->GetID(), GOT::GetType(got).CString());
}

bool ObjectPool::Free(Node* node, bool cleanDependences)
{
    if (!node)
        return false;

    StringHash got = node->GetVar(GOA::GOT).GetStringHash();

    if (!got)
    {
        URHO3D_LOGWARNINGF("ObjectPool() - Free : Node=%s(%u) no GOT : Remove Node !",
                           node->GetName().CString(), node->GetID());
        node->Remove();
        return false;
    }

    ObjectPoolCategory* category = pool_->GetCategory(got);
    if (!category)
    {
        URHO3D_LOGERRORF("ObjectPool() - Free : Node=%s(%u) GOT=%u... No category : Remove Node !",
                         node->GetName().CString(), node->GetID(), got.Value());
        node->Remove();
        return false;
    }

    if (!(category->FreePoolNode(node, cleanDependences)))
    {
        URHO3D_LOGERRORF("ObjectPool() - Free : Node=%s(%u) ... already inside Pool !!!",
                         node->GetName().CString(), node->GetID());
        return false;
    }

//    URHO3D_LOGINFOF("ObjectPool() - Free : Node=%s(%u)", node->GetName().CString(), node->GetID());

    return true;
}

bool ObjectPool::IsInPool(Node* node)
{
    if (!node)
        return true;

    const StringHash& got = node->GetVar(GOA::GOT).GetStringHash();

    if (!got)
        return true;

    ObjectPoolCategory* category = pool_->GetCategory(got);
    if (!category)
        return true;

    return category->IsNodeInPool(node);
}

void ObjectPool::DumpCategories() const
{
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : ----------------------------------------------");
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : size=%u", categories_.Size());
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : LOCAL n=%u->%u c=%u->%u REPLI n=%u->%u c=%u->%u",
                    firstLocalNodeID_, lastLocalNodeID_, firstLocalComponentID_, lastLocalComponentID_,
                    firstReplicatedNodeID_, lastReplicatedNodeID_, firstReplicatedComponentID_, lastReplicatedComponentID_);

    for (HashMap<StringHash, ObjectPoolCategory >::ConstIterator it=categories_.Begin(); it!=categories_.End(); ++it)
    {
        const ObjectPoolCategory& category = it->second_;
//        URHO3D_LOGINFOF("  PoolCategory %s(%u) : free=%u/%u LOCAL n=%u->%u c=%u->%u REPLI n=%u->%u c=%u->%u",
//                    GOT::GetType(it->first_).CString(), it->first_.Value(), category.GetFreeSize(), category.GetSize(),
//                    category.firstNodeID_, category.lastNodeID_, category.firstComponentID_, category.lastComponentID_,
//                    category.firstReplicatedNodeID_, category.lastReplicatedNodeID_, category.firstReplicatedComponentID_, category.lastReplicatedComponentID_);
        category.Dump(true);
    }

    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : ... OK !                                     -");
    URHO3D_LOGINFOF("ObjectPool() - DumpCategories : ----------------------------------------------");
}

void ObjectPool::UpdateDebugData()
{
    String line;
    debugTxt_.Clear();
    const int tabs = 1;

    unsigned i=0;
    for (HashMap<StringHash, ObjectPoolCategory >::ConstIterator it=pool_->categories_.Begin(); it!=pool_->categories_.End(); ++it)
    {
        const ObjectPoolCategory& category = it->second_;
        line.Clear();
        line.AppendWithFormat(" %s(%u/%u) ", GOT::GetType(it->first_).CString(), category.GetFreeSize(), category.GetSize());
        debugTxt_ += line;
        i++;
        if (i == tabs)
        {
            debugTxt_ += "\n";
            i = 0;
        }
    }
}
