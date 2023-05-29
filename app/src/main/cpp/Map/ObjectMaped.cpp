#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/VectorBuffer.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLElement.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>
#include <Urho3D/Urho2D/CollisionEdge2D.h>
#include <Urho3D/Urho2D/ConstraintWheel2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsUtils2D.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include "DefsColliders.h"

#include "GameRand.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"

//#ifdef USE_RENDERCOLLIDERS
#include "RenderShape.h"
//#endif

#include "GameContext.h"

#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
#include "GameNetwork.h"
#include "GOC_Animator2D.h"
#include "GOC_Detector.h"
#include "GOC_Destroyer.h"
#include "GOC_Controller.h"
#include "GOC_Collectable.h"
#include "ObjectPool.h"
#include "MAN_Go.h"
#endif
#include "GOC_Move2D.h"

#include "ViewManager.h"

#include "MapGeneratorDungeon.h"
#include "MapColliderGenerator.h"
#include "MapWorld.h"
#include "MapStorage.h"
#include "Map.h"
#include "MapCreator.h"

#include "ObjectMaped.h"


/// CONSTANTS



extern const char* mapStatusNames[];
extern const char* mapGenModeNames[];

static const unsigned MAX_PHYSICOBJECTMAPEDS = 10;
static const unsigned MAX_ORDERINLAYER = 1023;

static PODVector<int> physicObjectsFreeIds_;
static PODVector<ObjectMaped* > physicObjects_;
WorldMapPosition ObjectMaped::sObjectMapPositionResult_;

static int nextObjectId_ = MAX_PHYSICOBJECTMAPEDS;
int GetNextObjectID()
{
    nextObjectId_++;
    return nextObjectId_-1;
}

/// INITIALIZERS / STATUS SETTERS

ObjectMaped::ObjectMaped(Context* context) :
    Component(context),
    physicObjectId_(0),
    dataId_(0),
    physicsEnabled_(true),
    switchViewZEnabled_(true)
{
    Init();
}

ObjectMaped::~ObjectMaped()
{
    URHO3D_LOGINFOF("~ObjectMaped() - ptr=%u ... ", this);

    physicColliders_.Clear();

//#ifdef USE_RENDERCOLLIDERS
    renderColliders_.Clear();
//#endif

#ifdef USE_TILERENDERING
    objectTiled_.Reset();
#else
    skinnedMap_.Reset();
#endif

    mapStatus_.map_ = 0;

    ObjectMaped::FreePhysicObjectID(physicObjectId_);

    Init();

    URHO3D_LOGINFOF("~ObjectMaped() - ptr=%u ... OK !", this);
}

void ObjectMaped::RegisterObject(Context* context)
{
    context->RegisterFactory<ObjectMaped>();

    URHO3D_ACCESSOR_ATTRIBUTE("MapData", GetMapDataPoint, SetMapDataPoint, int, 0, AM_FILE);

    Reset();
}

void ObjectMaped::Reset()
{
    // Populate PhysicObjectIds
    physicObjectsFreeIds_.Resize(MAX_PHYSICOBJECTMAPEDS);
    physicObjects_.Resize(MAX_PHYSICOBJECTMAPEDS);

    for (int i = 1 ; i <= MAX_PHYSICOBJECTMAPEDS; i++)
    {
        physicObjects_[MAX_PHYSICOBJECTMAPEDS-i] = 0;
        physicObjectsFreeIds_[MAX_PHYSICOBJECTMAPEDS-i] = i;
    }

    // reset the start Id for the other object types
    // decremental id
    nextObjectId_ = MAX_PHYSICOBJECTMAPEDS;
}

bool ObjectMaped::SetPhysicObjectID(ObjectMaped* object)
{
    int id = 0;

    // get the first free id
    if (physicObjectsFreeIds_.Size())
    {
        id = physicObjectsFreeIds_.Back();
        physicObjectsFreeIds_.RemoveSwap(id);
    }

    // register the object at the index=id-1
    if (id > 0)
    {
        physicObjects_[id-1] = object;
        object->physicObjectId_ = id;
        return true;
    }

    return false;
}

void ObjectMaped::FreePhysicObjectID(int id)
{
    if (id && !physicObjectsFreeIds_.Contains(id))
    {
        physicObjectsFreeIds_.Push(id);
        physicObjects_[id-1] = 0;
    }
}


/// Initializers


void ObjectMaped::Init()
{
    info_.createmode_ = CREATEAUTO;

    mapStatus_.Clear();
    mapStatus_.map_ = this;

    nodeStatic_.Reset();
    nodeFurniture_.Reset();
    nodeChains_.Clear();
    nodeBoxes_.Clear();
    nodePlateforms_.Clear();

    if (mapData_)
        mapData_->SetMap(0);

    mapData_ = 0;

    cacheTileModifiers_.Clear();
}

void ObjectMaped::SetSize(int width, int height, int numviews)
{
    MapBase::SetSize(width, height);

    const float centerratio = 0.5f;
    center_ = centerratio * Vector2(width * Map::info_->tileWidth_, height * Map::info_->tileHeight_);

    // Create Features / Skin / Tiles Objects

#ifdef USE_TILERENDERING
    objectTiled_->Resize(width, height, numviews, 0);
    objectTiled_->SetHotSpot(Vector2(centerratio, centerratio));
//    objectTiled_->SetLayerModifier(physicsEnabled_ ? LAYER_OBJECTMAPED : LAYER_BOBJECTMAPED);
//    objectTiled_->SetOrderInLayer(dataId_%MAX_ORDERINLAYER);
#else
    skinnedMap_->Resize(width, height, numviews);
#endif

    canSwitchViewZ_ = numviews > 1;

    skinnedMap_->map_ = 0;

    // Resize Colliders

    unsigned size = width * height;

    physicColliders_.Resize(MAP_NUMMAXCOLLIDERS);

    for (unsigned i=0; i < physicColliders_.Size(); i++)
    {
        physicColliders_[i].id_ = i;
        physicColliders_[i].ReservePhysic(size);
        physicColliders_[i].map_ = this;
    }

    if (GameContext::Get().gameConfig_.renderShapes_)
    {
        renderColliders_.Resize(MAP_NUMMAXRENDERCOLLIDERS);
        for (unsigned i=0; i < renderColliders_.Size(); i++)
        {
            renderColliders_[i].renderShape_ = 0;
            renderColliders_[i].map_ = this;
        }
    }

    // Allocate Shape Pointers for SetVisibleStatic
    sboxes_.Reserve(size / 2);
    splateforms_.Reserve(size / 10);
    schains_.Reserve(size / 10);

//    mapData_->SetMap(this);

    SetStatus(Initializing);

    URHO3D_LOGINFOF("ObjectMaped() - SetSize: this=%u numviews=%d canSwitchViewZ=%s", this, numviews, canSwitchViewZ_ ? "true":"false");
}

bool ObjectMaped::RemoveNodes(HiresTimer* timer)
{
#if defined(HANDLE_FURNITURES)
    if (nodeFurniture_)
    {
        PODVector<Node*> children;
        nodeFurniture_->GetChildren(children);

        URHO3D_LOGINFOF("ObjectMaped() - RemoveNodes ... Remove Furnitures ... size=%u ...", children.Size());

        for (PODVector<Node*>::ConstIterator it=children.Begin(); it != children.End(); ++it)
        {
            Node* node = *it;

            if (!node)
                continue;

            if (node->GetComponent<GOC_Destroyer>())
                node->GetComponent<GOC_Destroyer>()->Destroy();
            else
                ObjectPool::Free(node, true);
        }

        World2D::PurgeEntities(GetMapPoint());

        GameHelpers::RemoveNodeSafe(nodeFurniture_);

        if (TimeOver(timer))
            URHO3D_LOGERRORF("ObjectMaped() - RemoveNodes ... Furnitures Cleared ... timer=%d/%d msec",
                             timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);

        GetMapCounter(MAP_FUNC3) = 0;
    }
    else
    {
        URHO3D_LOGWARNINGF("ObjectMaped() - RemoveNodes: this=%u no furniture node", this);
    }
#endif

    if (vehicleWheels_ && vehicleWheels_ != nodeStatic_)
        GameHelpers::RemoveNodeSafe(vehicleWheels_);

    if (nodeStatic_ && node_ != nodeStatic_)
        GameHelpers::RemoveNodeSafe(nodeStatic_);

    URHO3D_LOGINFOF("ObjectMaped() - RemoveNodes ... OK !");

    return true;
}

bool ObjectMaped::Clear(HiresTimer* timer)
{
    int& mcount = GetMapCounter(MAP_FUNC1);

    if (mcount == 0)
    {
#ifdef USE_TILERENDERING
        if (objectTiled_)
        {
            if (node_)
                node_->RemoveComponent(objectTiled_);

            objectTiled_->Clear();

            URHO3D_LOGINFOF("ObjectMaped() - Clear ... objectTiled Cleared ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);
        }
#else
        if (skinnedMap_)
        {
            skinnedMap_->Clear();
            URHO3D_LOGINFOF("ObjectMaped() - Clear ... skinnedMap Cleared ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);
        }
#endif
        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 1)
    {
        for (unsigned i=0; i < physicColliders_.Size(); i++)
            physicColliders_[i].Clear();

        URHO3D_LOGINFOF("ObjectMaped() - Clear ... PhysicColliders Cleared ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);

        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 2)
    {

        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            for (unsigned i=0; i < renderColliders_.Size(); i++)
            {
                renderColliders_[i].Clear();
                renderColliders_[i].map_ = 0;
            }
        }

        URHO3D_LOGINFOF("ObjectMaped() - Clear ... RenderColliders Cleared ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);

        mcount++;
        GetMapCounter(MAP_FUNC2) = 0;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 3)
    {
        if (!RemoveNodes(timer))
            return false;

        URHO3D_LOGINFOF("ObjectMaped() - Clear ... Nodes Removed ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);

        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 4)
    {
        Init();

        URHO3D_LOGINFOF("ObjectMaped() - Clear ... Reinitialized ... timer=%d/%d msec", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);
    }

    URHO3D_LOGINFOF("ObjectMaped() - Clear ... timer=%d/%d msec OK !", timer ? timer->GetUSec(false) / 1000 : 0, World2DInfo::delayUpdateUsec_/1000);

    return true;
}


/// Setters

void ObjectMaped::SetMapDataPoint(int x)
{
    // if physicEnabled_, put the object in the physicObjects_
    if (physicsEnabled_)
    {
        physicsEnabled_ = ObjectMaped::SetPhysicObjectID(this);
        if (x == 0 && physicsEnabled_)
            x = physicObjectId_;
    }

    if (x == 0)
        x = GetNextObjectID();

    if (dataId_ != x)
    {
        dataId_ = x;

        // Get or Create the mapdata for the point
        ShortIntVector2 mpoint(dataId_, fixedObjeMapedPointY);
        SetMapData(MapStorage::GetMapDataAt(mpoint));

        // always clear mapdata sections if mode!=CREATEFROMMAPDATA
        if (info_.createmode_ != CREATEFROMMAPDATA)
            mapData_->Clear();

        // Associate map with mapdata
        mapData_->mpoint_ = mpoint;
        mapData_->mapfilename_ = MapStorage::GetWorldDirName("Custom") + "/object_" + String(dataId_) + ".dat";
        mapData_->SetMap(this);
        mapData_->prefab_ = 1;

        mapStatus_.mappoint_ = mapData_->mpoint_;

        SetStatus(Initializing);

        SubscribeToEvent(E_BEGINFRAME, URHO3D_HANDLER(ObjectMaped, HandleSet));
    }
}

void ObjectMaped::SetPhysicEnabled(bool enable)
{
    physicsEnabled_ = enable;
}

void ObjectMaped::SetChangeViewZEnabled(bool enable)
{
    switchViewZEnabled_ = enable;
}

void ObjectMaped::CreateFrom(const ObjectMapedInfo& info) //int width, int height, const Vector2& initialPosition)
{
    URHO3D_LOGINFOF("ObjectMaped() - CreateFrom : ... ");

    info_.createmode_ = info.createmode_;
    info_.position_ = info.position_;
    info_.scale_ = info.scale_;
    if (info_.scale_ == Vector2::ZERO)
        info_.scale_ = MapStorage::Get()->GetNode()->GetWorldScale2D();

    info_.ref_ = info.ref_;

    if (info_.createmode_ == CREATEFROMMAPDATA)
    {
        SetMapDataPoint(info.rect_.top_);
    }
    else if (info.createmode_ == CREATEFROMMAP)
    {
        info_.rect_ = info.rect_;

        SetMapDataPoint();

        mapData_->width_ = info_.rect_.Width()+1;
        mapData_->height_ = info_.rect_.Height()+1;
        mapData_->numviews_ = MAP_NUMMAXVIEWS;
        mapData_->collidertype_ = DUNGEONINNERTYPE;
        mapData_->gentype_ = GEN_DUNGEON;

        mapData_->seed_ = ((Map*)info_.ref_)->GetMapSeed();
        mapData_->skinid_ = ((Map*)info_.ref_)->GetMapData()->skinid_;

        SetSize(mapData_->width_, mapData_->height_, mapData_->numviews_);

        if (physicsEnabled_)
            SetPhysicProperties(BT_DYNAMIC, 10.f, -(physicObjectId_+1));
    }
    else if (info.createmode_ == CREATEFROMGENERATOR)
    {
        SetMapModel((MapModel*)info_.ref_);
        SetPhysicEnabled(mapModel_->hasPhysics_);

        SetMapDataPoint();

        mapData_->width_ = info.rect_.left_;
        mapData_->height_ = info.rect_.top_;
        mapData_->numviews_ = mapModel_->numviews_;
        mapData_->collidertype_ = mapModel_->colliderType_;
        mapData_->gentype_ = mapModel_->genType_;

        SetSize(mapData_->width_, mapData_->height_, mapData_->numviews_);

        if (physicsEnabled_)
            SetPhysicProperties((BodyType2D)mapModel_->colliderBodyType_, 1000.f, -(physicObjectId_+1), mapModel_->colliderBodyRotate_);

        // Test disable switch viewZ
        if (!physicsEnabled_)
        {
            SetChangeViewZEnabled(false);
        }

        // Test for serialization
        SetSerializable(physicsEnabled_);

        mapStatus_.model_ = mapModel_->anlModel_;
        mapStatus_.mappoint_ = GetMapPoint();
        if (info.rect_.right_ != 0)
            mapStatus_.wseed_ = info.rect_.right_;
        else if (mapStatus_.model_)
            mapStatus_.wseed_ = mapStatus_.model_->GetSeed();
        else
            mapStatus_.wseed_ = 100;
        mapStatus_.rseed_ = mapStatus_.wseed_ + mapStatus_.cseed_;

        Vector2 modelsize = mapStatus_.model_ ? mapStatus_.model_->GetSize() : Vector2::ONE;
        mapStatus_.mappingRange_.mapx0 = -modelsize.x_ * 0.5f;
        mapStatus_.mappingRange_.mapx1 = modelsize.x_ * 0.5f;
        mapStatus_.mappingRange_.mapy0 = -modelsize.y_ * 0.5f;
        mapStatus_.mappingRange_.mapy1 = modelsize.y_ * 0.5f;

        GameRand::SetSeedRand(ALLRAND, mapStatus_.rseed_);

        MapGenerator::SetSize(mapData_->width_, mapData_->height_);
        MapCreator::Get()->SetGenerator(GetMapGeneratorStatus(), (MapGeneratorType)mapData_->gentype_);
    }

    node_->SetName("ObjectMaped");
    node_->SetWorldPosition2D(info_.position_);
    node_->SetWorldScale2D(info_.scale_);
    node_->SetEnabled(false);

    URHO3D_LOGINFOF("ObjectMaped() - CreateFrom : ... OK !");
}

void ObjectMaped::HandleSet(StringHash eventType, VariantMap& eventData)
{
    HiresTimer tobject;
    HiresTimer* timer = &tobject;

//    URHO3D_LOGINFOF("ObjectMaped() - HandleSet : status=%s(%d) mcount=%d...", mapStatusNames[mapStatus_.status_], mapStatus_.status_, GetMapCounter(MAP_GENERAL));

    switch (mapStatus_.status_)
    {
    case Initializing:
    {
        SetStatus(Generating_Map);

        if (info_.createmode_ == CREATEAUTO || info_.createmode_ == CREATEFROMMAPDATA)
        {
            // the mapdata layers ares set and mapdata is a prefab => copy layers
            if (mapData_->IsSectionSet(MAPDATASECTION_LAYER) && mapData_->prefab_)
            {
                mapData_->CopyPrefabLayers();
            }
            // the mapdata need the layer section => load it
            else if (!mapData_->IsSectionSet(MAPDATASECTION_LAYER) && MapStorage::GetMapSerializer())
            {
                // Load the map data if a map file exists
                // Change to Loading_Map status
                bool success = MapStorage::GetMapSerializer()->LoadMapData(mapData_);
            }
        }

        break;
    }
    case Generating_Map:
    {
        if (info_.createmode_ == CREATEAUTO || info_.createmode_ == CREATEFROMMAPDATA)
        {
            if (!mapData_->IsSectionSet(MAPDATASECTION_LAYER))
            {
                // Load error => abort
                URHO3D_LOGERRORF("ObjectMaped() - HandleSet : status=%s(%d) ... LOAD ERROR => Remove node !", mapStatusNames[mapStatus_.status_], mapStatus_.status_);
                bool state = MapStorage::RemoveMapDataAt(ShortIntVector2(dataId_, -32767));
                node_->Remove();
                return;
            }

            // allocate size
            SetSize(mapData_->width_, mapData_->height_, mapData_->numviews_);
            if (physicsEnabled_)
                SetPhysicProperties(BT_DYNAMIC, 10.f, -(physicObjectId_+1));
        }

        SetColliderType((ColliderType)mapData_->collidertype_);
        featuredMap_->SetViewConfiguration(mapData_->gentype_);

        featuredMap_->indexToSet_ = 0;
        GetMapCounter(MAP_GENERAL) = GetMapCounter(MAP_FUNC1) = 0;

        SetStatus(Creating_Map_Layers);

        break;
    }
    case Creating_Map_Layers:
    {
        int& mcount0 = GetMapCounter(MAP_GENERAL);

        if (mcount0 < 6)
        {
            if (info_.createmode_ == CREATEAUTO || info_.createmode_ == CREATEFROMMAPDATA)
            {
                skinnedMap_->SetAtlas(World2DInfo::currentAtlas_);
                skinnedMap_->SetSkin("dungeon", mapData_->skinid_);
                skinnedMap_->SetNumViews(mapData_->numviews_);
                mapData_->SetSection(MAPDATASECTION_LAYER);
                mapData_->SetMapsLinks();
            }
            else if (info_.createmode_ == CREATEFROMMAP)
            {
                // copy the map layers
                ((Map*)info_.ref_)->GetObjectFeatured()->Copy(info_.rect_.left_, info_.rect_.top_, *featuredMap_);

                skinnedMap_->SetAtlas(((Map*)info_.ref_)->skinnedMap_->GetAtlas());
                skinnedMap_->SetSkin(((Map*)info_.ref_)->skinnedMap_->GetSkin());
                skinnedMap_->SetNumViews(mapData_->numviews_);
                mapData_->SetSection(MAPDATASECTION_LAYER);
                mapData_->SetMapsLinks();
            }
            else if (info_.createmode_ == CREATEFROMGENERATOR)
            {
                if (!MapCreator::Get()->GenerateLayersBase(this, timer))
                    return;

                URHO3D_LOGINFOF("ObjectMaped() - HandleSet : CREATEFROMGENERATOR generate layers base ... OK !");
//                SetColliderType((ColliderType)mapData_->collidertype_);
            }

            mcount0 = 6;
            featuredMap_->indexToSet_ = 0;

            break;
        }
        // Set Fluid Cells
        if (mcount0 == 6)
        {
            if (featuredMap_->SetFluidCells(timer, World2DInfo::delayUpdateUsec_))
                mcount0++;

            break;
        }
        // Set Skinned Views
        if (mcount0 == 7)
        {
            if (skinnedMap_->SetViews(timer, World2DInfo::delayUpdateUsec_))
                mcount0++;

            break;
        }
        // Set Mask Views
        if (mcount0 == 8)
        {
            if (!featuredMap_->UpdateMaskViews(timer, World2DInfo::delayUpdateUsec_, skinnedMap_->GetSkin() ? skinnedMap_->GetSkin()->neighborMode_ : Connected0))
                break;
        }

        mcount0 = 0;
        SetStatus(Creating_Map_Colliders);

        break;
    }
    case Creating_Map_Colliders:
    {
        if (!CreateColliders(timer, physicsEnabled_))
            return;

        GetMapCounter(MAP_GENERAL) = 0;
        SetStatus(Creating_Map_Entities);

        break;
    }
    case Creating_Map_Entities:
    {
        if (info_.createmode_ == CREATEFROMMAP)
        {
            ((Map*)info_.ref_)->GetFurnitures(info_.rect_, mapData_->furnitures_, true);
            mapData_->SetSection(MAPDATASECTION_FURNITURE);
        }
        else if (info_.createmode_ == CREATEFROMGENERATOR)
        {
            URHO3D_LOGERRORF("ObjectMaped() - HandleSet : status=%s(%d) ... ", mapStatusNames[mapStatus_.status_], mapStatus_.status_);

            if (!MapCreator::Get()->GenerateEntities(this, timer))
                return;

            URHO3D_LOGERRORF("ObjectMaped() - HandleSet : status=%s(%d) ... OK !", mapStatusNames[mapStatus_.status_], mapStatus_.status_);
        }

        SetStatus(Setting_Map);

        break;
    }
    case Setting_Map:
    {
//        URHO3D_LOGINFOF("ObjectMaped() - Set mPoint = %s ... status=%d", GetMapPoint().ToString().CString(), mapStatus_.status_);

        nodeRoot_ = node_;
        nodeStatic_ = node_->CreateChild("Statics", LOCAL);
        nodeFurniture_ = node_->CreateChild("Furnitures", LOCAL);

        // Really important to disabled node here for RenderShape
        // is also important for the jump of the MobileCastle
        node_->SetEnabled(false);

        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            // Create RenderShape
            unsigned numrendercolliders = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
            for (unsigned i=0; i< numrendercolliders; i++)
            {
                RenderCollider& collider = renderColliders_[i];
                collider.CreateRenderShape(node_, false, true);
//                collider.renderShape_->SetLayerModifier(physicsEnabled_ ? LAYER_OBJECTMAPED : LAYER_BOBJECTMAPED);
//                collider.renderShape_->SetOrderInLayer(dataId_ % MAX_ORDERINLAYER);
            }

//const int LAYER_BOBJECTMAPED = -(INNERVIEW - 10 + 1);
//const int LAYER_OBJECTMAPED  = -2;
//    const ColliderParams castleRenderColliderParams[] =
//    {
//        ///           { indz_, indv_, colliderz_, layerz_, mode_, filtercategory, filtermask }
//        /*BackView*/  { 0, 0, BACKVIEW, BACKVIEW, BackRenderMode, SHT_CHAIN, BACKVIEW_MASK, 0 },                      RENDERSHAPE + BACKVIEW + 2  = 3 + 40 + 2 = 45
//        /*InnerView*/ { 0, 1, INNERVIEW, INNERVIEW, WallMode, SHT_CHAIN, BACKVIEW_MASK, 0 },                          RENDERSHAPE + INNERVIEW + 2 = 3 + 70 + 2 = 75
//        /*Plateform*/ { 0, 1, INNERVIEW, INNERVIEW + LAYER_PLATEFORMS, PlateformMode, SHT_CHAIN, BACKVIEW_MASK, 0 },  RENDERSHAPE + INNERVIEW + LAYER_PLATEFORMS + 2 = 3 + 70 - 7 + 2 = 68
//        /*OuterView*/ { 1, 2, OUTERVIEW, OUTERVIEW, FrontMode, SHT_CHAIN, OUTERVIEW_MASK, 0 },                        RENDERSHAPE + OUTERVIEW - 2 = 3 + 90 -2 = 92
//    };

        }

        GetMapCounter(MAP_GENERAL) = 0;

        SetStatus(Setting_Colliders);

        break;
    }
    case Setting_Colliders:
    {
        if (physicsEnabled_)
        {
            if (!SetCollisionShapes(timer))
                break;

            // Set Enable All the Collision Shapes
            PODVector<CollisionShape2D*> shapes;
            GetStaticNode()->GetDerivedComponents<CollisionShape2D>(shapes, true);
            for (unsigned i=0; i < shapes.Size(); i++)
                shapes[i]->SetEnabled(true);
        }

        SetStatus(Updating_ViewBatches);

        GetMapCounter(MAP_GENERAL) = 0;

#ifdef USE_TILERENDERING
        objectTiled_->indexToSet_ = 0;
#endif

        break;
    }
    case Updating_ViewBatches:
    {
#ifdef RENDER_VIEWBATCHES
        if (ViewManager::Get())
        {
#ifdef USE_TILERENDERING
            if (!objectTiled_->UpdateViewBatches(ViewManager::Get()->GetNumViewZ(), timer, World2DInfo::delayUpdateUsec_))
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("ObjectMaped() - HandleSet : map=%s ... Updating ViewBatches", GetMapPoint().ToString().CString()), timer, World2DInfo::delayUpdateUsec_);
#endif
                break;
            }
#endif
        }
#endif

//        ChangeViewZ(ViewManager::Get()->GetCurrentViewZIndex());

#ifdef USE_TILERENDERING
#ifdef RENDER_OBJECTTILED
        if (objectTiled_ && ViewManager::Get())
        {
            int numviewports = ViewManager::Get()->GetNumViewports();
            for (int viewport=0; viewport < numviewports; viewport++)
            {
                int viewZindex = ViewManager::Get()->GetCurrentViewZIndex(viewport);
                objectTiled_->SetCurrentViewZ(viewport, viewZindex);
            }
        }
#endif
#endif

        GetMapCounter(MAP_GENERAL) = 0;

        SetStatus(Updating_RenderShapes);

        break;
    }
    case Updating_RenderShapes:
    {
        if (GameContext::Get().gameConfig_.renderShapes_)
            if (!UpdateRenderShapes(timer))
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("ObjectMaped() - HandleSet : map=%s ... Updating RenderShapes", GetMapPoint().ToString().CString()), timer, World2DInfo::delayUpdateUsec_);
#endif
                break;
            }

        GetMapCounter(MAP_GENERAL) = 0;

        SetStatus(Setting_Furnitures);

        break;
    }
    case Setting_Furnitures:
    {
        if (!SetFurnitures(timer, false))
            break;

        GetMapCounter(MAP_GENERAL) = 0;

        UnsubscribeFromEvent(E_BEGINFRAME);

        if (nodeFurniture_)
        {
            PODVector<Node*> children;
            nodeFurniture_->GetChildren(children);
            for (unsigned i=0; i < children.Size(); i++)
            {
                GameHelpers::UpdateLayering(children[i]);
                children[i]->SetEnabledRecursive(enabled_);
            }

            nodeFurniture_->SetEnabled(enabled_);
        }

        node_->SetEnabled(true);

        SetStatus(Available);
        Dump();

        URHO3D_LOGERRORF("ObjectMaped() - HandleSet : node=%s(%u) ... status=%s(%d) ... OK !", node_->GetName().CString(), node_->GetID(), mapStatusNames[mapStatus_.status_], mapStatus_.status_);
    }
    }
}

void ObjectMaped::CreateWheel(Node* wheelsNode, Node* vehicleNode, const Vector2& center, float scale)
{
    float wheelsizeratio = 1.5f;

    Node* wheelNode = wheelsNode->CreateChild("Wheel", LOCAL);

    // Simple Wheel
    if (wheelsNode == vehicleNode)
    {
//        wheelNode->SetPosition2D(center);

        // Create shape
        CollisionCircle2D* collisionCircle = wheelNode->CreateComponent<CollisionCircle2D>(LOCAL);
        collisionCircle->SetCenter(center);
        collisionCircle->SetRadius(MapInfo::info.tileHalfSize_.x_ / scale / wheelsizeratio);
        collisionCircle->SetFriction(0.5f);
        // we don't want collision with entities use CC_INSIDEEFFECT
        collisionCircle->SetFilterBits(CC_INSIDEOBJECT, CM_INSIDEOBJECT);
//        collisionCircle->SetFilterBits(CC_INSIDEEFFECT, CM_INSIDEEFFECT);
        collisionCircle->SetViewZ(INNERVIEW);
        collisionCircle->SetGroupIndex(colliderGroupIndex_);
    }
    // Wheel with String => ConstraintWheel2D
    else
    {
        wheelNode->SetWorldPosition2D(vehicleNode->GetWorldPosition2D() + center * vehicleNode->GetWorldScale2D());
        wheelNode->SetWorldScale2D(wheelsizeratio * scale * vehicleNode->GetWorldScale2D());

        // Create rigid body
        RigidBody2D* body = wheelNode->CreateComponent<RigidBody2D>(LOCAL);
        body->SetBodyType(BT_DYNAMIC);

        // Create shape
        CollisionCircle2D* collisionCircle = wheelNode->CreateComponent<CollisionCircle2D>(LOCAL);
        collisionCircle->SetRadius(MapInfo::info.tileHalfSize_.x_ / scale);
        collisionCircle->SetFriction(0.5f);
        // we don't want collision with entities use CC_INSIDEEFFECT
//        collisionCircle->SetFilterBits(CC_INSIDEOBJECT, CM_INSIDEOBJECT);
        collisionCircle->SetFilterBits(CC_INSIDEEFFECT, CM_INSIDEEFFECT);
        collisionCircle->SetViewZ(INNERVIEW);
        collisionCircle->SetGroupIndex(colliderGroupIndex_);

        // Create joint
        ConstraintWheel2D* jointwheel = vehicleNode->CreateComponent<ConstraintWheel2D>();
        jointwheel->SetOtherBody(wheelNode->GetComponent<RigidBody2D>());
        jointwheel->SetAnchor(wheelNode->GetPosition2D());
        jointwheel->SetAxis(Vector2(0.f, 1.f));
        jointwheel->SetMaxMotorTorque(10.f);
        jointwheel->SetFrequencyHz(28.f);
        jointwheel->SetDampingRatio(100.f);
    }

    // Drawables
    Node* nodedrawable = wheelNode->CreateChild("WheelDrawable", LOCAL);
    nodedrawable->SetVar(GOA::ONVIEWZ, THRESHOLDVIEW);
    nodedrawable->SetWorldPosition2D(vehicleNode->GetWorldPosition2D() + center * vehicleNode->GetWorldScale2D());
    nodedrawable->SetWorldScale2D(wheelsizeratio * scale * vehicleNode->GetWorldScale2D());

    // Drawable in Front
    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(GameContext::Get().context_, ResourceRef(Sprite2D::GetTypeStatic(), "Textures/Actors/roue1.png"));
    StaticSprite2D* wheeldrawable = nodedrawable->CreateComponent<StaticSprite2D>();
    wheeldrawable->SetSprite(sprite);
    wheeldrawable->SetLayer2(IntVector2(OUTERVIEW-1,-1));
    wheeldrawable->SetOrderInLayer(100);
    wheeldrawable->SetViewMask(ViewManager::GetLayerMask(FRONTVIEW));

    // Drawable in Back
    wheeldrawable = nodedrawable->CreateComponent<StaticSprite2D>();
    wheeldrawable->SetSprite(sprite);
    wheeldrawable->SetLayer2(IntVector2(BACKVIEW+1,-1));
    wheeldrawable->SetOrderInLayer(0);
    wheeldrawable->SetViewMask(ViewManager::GetLayerMask(INNERVIEW));
}

void ObjectMaped::OnPhysicsSetted()
{
    if (physicsEnabled_)
    {
        int viewid = GetViewId(INNERVIEW);

        // Get the Bottom Positions for the wheels
        IntVector2 wheelPosition[3];
        wheelPosition[1].x_ = 0;
        wheelPosition[2].x_ = GetWidth() - 1;
        wheelPosition[1].y_ = wheelPosition[2].y_ = GetHeight() - 1;
        GetBlockPositionAt(UpDir | RightDir, viewid, wheelPosition[1]);
        GetBlockPositionAt(UpDir | LeftDir, viewid, wheelPosition[2]);
        wheelPosition[0] = (wheelPosition[1] + wheelPosition[2]) / 2.f;

        Vector2 offset(0.f, -2.f*MapInfo::info.tileHalfSize_.y_);

        RigidBody2D* body = GetStaticNode()->GetComponent<RigidBody2D>(LOCAL);

        // Put the mass center between the wheels
        Vector2 massCenter((2.f * wheelPosition[0].x_ + 1.f) * MapInfo::info.tileHalfSize_.x_, (2.f * (GetHeight() - wheelPosition[0].y_ - 1) + 1.f) * MapInfo::info.tileHalfSize_.y_);
        massCenter -= center_;
        massCenter *= GetStaticNode()->GetWorldScale2D();

        // Add 2 Wheels
        // Vehicle with Strings
        //vehicleWheels_ = GetScene()->GetChild("LocalScene")->CreateChild("Wheels", LOCAL);
        // Vehicle no Strings
        vehicleWheels_ = GetStaticNode();

        for (int i=1; i <= 2; i++)
            CreateWheel(vehicleWheels_, GetStaticNode(), Vector2(2.f * wheelPosition[i].x_ + 1.f, 2.f * (GetHeight() - wheelPosition[i].y_ - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_ + offset, 0.5f);

        GOC_Move2D* gocmove = node_->GetComponent<GOC_Move2D>();
        if (gocmove)
            gocmove->SetVehicleWheels();

        // Modify Inertia to always satisfy to b2Assert(m_I > 0.0f) in b2Body::SetMassData (m_I = massData->I - m_mass * b2Dot(massData->center, massData->center)
        body->SetInertia(1.2f * body->GetMass() * massCenter.LengthSquared());
        body->SetMassCenter(massCenter);

//        body->GetBody()->Dump();
    }
}


/// VISIBILITY HANDLER

void ObjectMaped::MarkDirty()
{
#ifdef USE_TILERENDERING
#ifdef RENDER_VIEWBATCHES
    if (objectTiled_)
        objectTiled_->MarkChunkGroupDirty(objectTiled_->GetChunkInfo()->GetDefaultChunkGroup(MapDirection::All));
#endif
#endif // USE_TILERENDERING
}

bool ObjectMaped::Update(HiresTimer* timer)
{
#ifdef USE_TILERENDERING
#ifdef RENDER_VIEWBATCHES
    if (objectTiled_ && !objectTiled_->UpdateDirtyChunks(timer, World2DInfo::delayUpdateUsec_))
        return false;
#endif
#endif
    return true;
}

void ObjectMaped::OnSetEnabled()
{
    if (enabled_)
    {
        if (switchViewZEnabled_)
            SubscribeToEvent(ViewManager::Get(), VIEWMANAGER_SWITCHVIEW, URHO3D_HANDLER(ObjectMaped, HandleChangeViewIndex));

        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            unsigned numrenderviews = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
            if (renderColliders_.Size() >= numrenderviews)
            {
                for (unsigned i=0; i < numrenderviews; i++)
                {
                    Node* node = renderColliders_[i].node_;
                    if (node)
                        node->SetEnabled(true);
                }
            }
        }
    }
    else
    {
        UnsubscribeFromEvent(ViewManager::Get(), VIEWMANAGER_SWITCHVIEW);
    }

    if (nodeFurniture_)
    {
        PODVector<Node*> children;
        nodeFurniture_->GetChildren(children);
        for (unsigned i=0; i < children.Size(); i++)
        {
            GameHelpers::UpdateLayering(children[i]);
            children[i]->SetEnabledRecursive(enabled_);
        }

        nodeFurniture_->SetEnabled(enabled_);
    }
}

void ObjectMaped::OnNodeSet(Node* node)
{
    if (node)
    {
#ifdef USE_TILERENDERING
        if (!objectTiled_)
        {
            objectTiled_ = SharedPtr<ObjectTiled>(node_->CreateComponent<ObjectTiled>(LOCAL));
            objectTiled_->SetDynamic(true);
            objectTiled_->map_ = this;
            skinnedMap_ = objectTiled_->GetObjectSkinned();
        }
#else
        if (!skinnedMap_)
            skinnedMap_ = SharedPtr<ObjectSkinned>(new ObjectSkinned());
#endif
        skinnedMap_->SetFeature();
        featuredMap_ = skinnedMap_->GetFeatureData();
        featuredMap_->featuredView_.Resize(MAP_NUMMAXVIEWS);
        skinnedMap_->map_ = this;
        featuredMap_->map_ = this;

        RigidBody2D* body = node_->GetComponent<RigidBody2D>(LOCAL);
        if (!body)
            body = node_->CreateComponent<RigidBody2D>(LOCAL);
    }
    else
    {
        UnsubscribeFromAllEvents();
    }
}

void ObjectMaped::OnNodeRemoved()
{
    if (mapData_)
    {
        if (mapData_->IsSectionSet(MAPDATASECTION_LAYER))
        {
            UpdateMapData(0);
            if (MapStorage::GetMapSerializer())
                MapStorage::GetMapSerializer()->SaveMapData(mapData_, false);
        }

        mapData_->SetMap(0);
    }

    Clear();
}

void ObjectMaped::HandleChangeViewIndex(StringHash eventType, VariantMap& eventData)
{
    URHO3D_LOGINFOF("ObjectMaped() - HandleChangeViewIndex : ptr=%u !", this);

    int viewport = eventData[ViewManager_SwitchView::VIEWPORT].GetInt();
    int viewZindex = eventData[ViewManager_SwitchView::VIEWZINDEX].GetInt();
    bool forced = eventData[ViewManager_SwitchView::FORCEDMODE].GetBool();
    int viewZ = ViewManager::Get()->GetViewZ(viewZindex);

#ifdef USE_TILERENDERING
    if (objectTiled_)
    {
        objectTiled_->SetCurrentViewZ(viewport, viewZindex, forced);
    }
#endif // USE_TILERENDERING

    currentViewId_ = featuredMap_->GetViewId(viewZ);

//    Map* map = World2D::GetMapAt(ShortIntVector2(node_->GetVar(GOA::ONMAP).GetUInt()));
//    if (map)

    // Update the availability of the collisionshapes
    // if ObjectMaped is inside a Map, the collisionshapes will be disabled if the avatar is outside
    // TODO : it's not a good behavior, just for one player => use physic filter bits instead
    {
        bool masked = IsMaskedByOtherMap(viewZ);
//        bool masked = IsMaskedBy(map, viewZ);
        {
            PODVector<CollisionChain2D*> shapes;
            GetStaticNode()->GetComponents<CollisionChain2D>(shapes, true);
            for (unsigned i=0; i < shapes.Size(); i++)
                shapes[i]->SetEnabled(!masked);
        }
        if (nodePlateforms_.Size())
        {
            PODVector<CollisionBox2D*> shapes;
            for (unsigned i=0; i < nodePlateforms_.Size(); i++)
            {
                if (!nodePlateforms_[i])
                    continue;

                nodePlateforms_[i]->GetComponents<CollisionBox2D>(shapes, true);
                nodePlateforms_[i]->SetEnabled(!masked);

                URHO3D_LOGINFOF("ObjectMaped() - HandleChangeViewIndex ptr=%u node plateforms[%d]=%u numboxes=%u", this, i, nodePlateforms_[i], shapes.Size());

                for (unsigned j=0; j < shapes.Size(); j++)
                    shapes[j]->SetEnabled(!masked);
            }
        }
//
//        URHO3D_LOGINFOF("ObjectMaped() - HandleChangeViewIndex ptr=%u on map=%s masked=%s!", this, map ? map->GetMapPoint().ToString().CString() : "none", masked ? "true":"false");
    }
}


/// TILE MODIFIERS

void ObjectMaped::OnTileModified(int x, int y)
{
    URHO3D_LOGINFOF("ObjectMaped() - OnTileModified ptr=%u x=%d y=%d !", this, x, y);

//    // Update mask views
//    bool state = featuredMap_->UpdateMaskViews(0, 0, skinnedMap_->GetSkin() ? skinnedMap_->GetSkin()->neighborMode_ : Connected0);
//
//    MarkDirty();

#ifdef USE_TILERENDERING
    // Update chunk batch
    if (objectTiled_)
    {
        const ChunkGroup& chunkgroup = objectTiled_->GetChunkInfo()->GetUniqueChunkGroup(objectTiled_->GetChunkInfo()->GetChunk(x, y));
        objectTiled_->MarkChunkGroupDirty(chunkgroup);

        // Instant update Here to prevent Renderer2D crash
        objectTiled_->UpdateDirtyChunks(0, World2DInfo::delayUpdateUsec_);

        // Update chunk maskViews
        /// already made by objectTiled_->MarkChunkGroupDirty->UpdateChunkGroup but need in instant here for updatecollider
        /// TODO : bypass objectTiled_->MarkChunkGroupDirty->UpdateChunkGroup->UpdateMaskViews
//        featuredMap_->UpdateMaskViews(chunkgroup.GetTileGroup(), 0, 0, skinnedMap_->GetSkin() ? skinnedMap_->GetSkin()->neighborMode_ : Connected0);
    }
#endif
}


/// MAPDATA UPDATER

bool ObjectMaped::OnUpdateMapData(HiresTimer* timer)
{
    if (!GetMapData())
        return true;

    if (!GetMapData()->maps_.Size())
        return true;

    int& mcount = GetMapCounter(MAP_FUNC2);

    while (mcount < MAP_NUMMAXVIEWS+2)
    {
        GetMapData()->UpdatePrefabLayer(mcount);

        mcount++;

        if (TimeOver(timer))
            return false;
    }

    mcount = 0;

    return true;
}


/// GETTERS

void ObjectMaped::GetPhysicObjectAt(const Vector2& worldposition, MapBase*& map, bool calculateMapPosition)
{
    map = 0;

    for (int i = 0; i < MAX_PHYSICOBJECTMAPEDS; i++)
    {
        ObjectMaped* object = physicObjects_[i];

        // Test with GetWorldBoundingBox2D
        if (object && object->GetWorldBoundingBox2D().IsInside(worldposition) == INSIDE)
//        if (object && object->GetObjectTiled()->GetWorldBoundingBox().IsInside(worldposition) == INSIDE)
        {
//            URHO3D_LOGINFOF("ObjectMaped() - GetPhysicObjectAt : position=%s find objectmapedid=%d ... OK !", sObjectMapPositionResult_.mPosition_.ToString().CString(), objectMapeds_[i]->GetPhysicObjectID());
            map = object;
            break;
        }
    }

    if (calculateMapPosition && map && map->GetRootNode())
    {
        Vector2 localposition = map->GetRootNode()->GetWorldTransform2D().Inverse() * worldposition;
        sObjectMapPositionResult_.mPosition_.x_ = floor((localposition.x_+map->center_.x_) / Map::info_->tileWidth_);
        sObjectMapPositionResult_.mPosition_.y_ = map->GetHeight() - (int)floor((localposition.y_+map->center_.y_) / Map::info_->tileHeight_) - 1;
        sObjectMapPositionResult_.tileIndex_    = map->GetTileIndex(sObjectMapPositionResult_.mPosition_.x_, sObjectMapPositionResult_.mPosition_.y_);

        if (sObjectMapPositionResult_.tileIndex_ >= map->GetSize())
        {
            URHO3D_LOGERRORF("ObjectMaped() - GetPhysicObjectAt : objectmapedid=%d ... position=%s tileindex=%u > size=%u ... NOK !", ((ObjectMaped*)map)->GetPhysicObjectID(), sObjectMapPositionResult_.mPosition_.ToString().CString(), sObjectMapPositionResult_.tileIndex_, map->GetSize());
            sObjectMapPositionResult_.tileIndex_ = 0;
        }
    }
}

void ObjectMaped::GetVisibleFurnitures(PODVector<Node*>& furnitureRootNodes)
{
    for (unsigned i=0; i < physicObjects_.Size(); i++)
    {
        ObjectMaped* object = physicObjects_[i];
        if (object && object->IsEnabledEffective())
        {
            furnitureRootNodes.Push(object->GetFurnitureNode());
        }
    }
}

const PODVector<ObjectMaped* >& ObjectMaped::GetPhysicObjects()
{
    return physicObjects_;
}


void ObjectMaped::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (debug && IsEnabledEffective())
    {
        if (vehicleWheels_)
            debug->AddNode(vehicleWheels_, 3.f, false);

        if (GetStaticNode())
            debug->AddCross(Vector3(ToVector2(GetStaticNode()->GetComponent<RigidBody2D>(LOCAL)->GetBody()->GetWorldCenter())), 5.f, Color::CYAN);
    }
}


/// DUMP

void ObjectMaped::Dump() const
{
    URHO3D_LOGINFOF("ObjectMaped() - Dump : status=%s(%d) numviews=%d", mapStatusNames[mapStatus_.status_], mapStatus_.status_, skinnedMap_ ? skinnedMap_->GetNumViews() : 0);

    if (skinnedMap_ && skinnedMap_->GetNumViews())
        skinnedMap_->Dump();
}
