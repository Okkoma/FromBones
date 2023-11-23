#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/VectorBuffer.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/Image.h>
#include <Urho3D/Resource/XMLElement.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/Node.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>
#include <Urho3D/Urho2D/CollisionEdge2D.h>
#include <Urho3D/Urho2D/ConstraintRevolute2D.h>
#include <Urho3D/Urho2D/ConstraintWeld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/TmxFile2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>

#include "DefsChunks.h"
#include "DefsWorld.h"
#include "DefsColliders.h"
#include "DefsEffects.h"

#include "GameRand.h"
#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"

#include "GEF_Scrolling.h"
#include "WaterLayer.h"

//#ifdef USE_RENDERCOLLIDERS
#include "RenderShape.h"
//#endif

#include "GameContext.h"
#include "GameNetwork.h"

#include "GOC_Destroyer.h"
#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
#include "GOC_Animator2D.h"
#include "GOC_Detector.h"
#include "GOC_Controller.h"
#include "GOC_Collectable.h"
#include "ObjectPool.h"
#include "MAN_Go.h"
#endif

#include "MapWorld.h"
#include "ViewManager.h"
#include "MAN_Go.h"

#include "MapColliderGenerator.h"

#include "ObjectMaped.h"
#include "Map.h"


#define MAP_CREATECOLLISIONSHAPES
#define MAP_GENERATE_MASKVIEWS
#define MAP_ADDHOLECOLLISIONCHAINS

/// CONSTANTS



long long& MapBase::delayUpdateUsec_ = World2DInfo::delayUpdateUsec_;

const char* MapVisibleStateNames_[] =
{
    "MAP_NOVISIBLE = 0",
    "MAP_VISIBLE = 1",
    "MAP_CHANGETOVISIBLE = 2",
    "MAP_CHANGETONOVISIBLE = 3",
};

extern const char* mapStatusNames[];
extern const char* mapGenModeNames[];




/// MapBase

MapBase::MapBase() :
    mapData_(0),
    mapModel_(0),
    canSwitchViewZ_(true),
    skipInitialTopBorder_(false),
    serializable_(false)
{ }


void MapBase::SetSerializable(bool state)
{
    serializable_ = state;
}

void MapBase::SetSize(int width, int height)
{
    mapStatus_.width_ = width;
    mapStatus_.height_ = height;
}

void MapBase::SetStatus(int status)
{
//    URHO3D_LOGERRORF("MapBase() - SetStatus : ptr=%u mPoint=%s change to status=%s !", this, mapStatus_.mappoint_.ToString().CString(), mapStatusNames[status]);

    mapStatus_.status_ = status;
    mapStatus_.ResetCounters(1);
}



/// SETTERS

/// Collider Setters

void MapBase::SetPhysicProperties(BodyType2D bodytype, float bodymass, int grpindex, bool rotation)
{
    colliderBodyType_ = bodytype;
    colliderBodyMass_ = bodymass;
    colliderBodyRotate_ = rotation;
    colliderGroupIndex_ = grpindex;
}

void MapBase::SetColliderType(ColliderType type)
{
    colliderType_ = type;

    if (colliderType_ == DUNGEONINNERTYPE)
    {
        SetColliderParameters(PHYSICCOLLIDERTYPE, NUMDUNGEONPHYSICCOLLIDERS, dungeonPhysicColliderParams);
//#ifdef USE_RENDERCOLLIDERS
        SetColliderParameters(RENDERCOLLIDERTYPE, NUMDUNGEONRENDERCOLLIDERS, dungeonRenderColliderParams);
//#endif
    }
    else if (colliderType_ == CAVEINNERTYPE)
    {
        SetColliderParameters(PHYSICCOLLIDERTYPE, NUMCAVEPHYSICCOLLIDERS, cavePhysicColliderParams);
//#ifdef USE_RENDERCOLLIDERS
        SetColliderParameters(RENDERCOLLIDERTYPE, NUMCAVERENDERCOLLIDERS, caveRenderColliderParams);
//#endif
    }
    else if (colliderType_ == ASTEROIDTYPE)
    {
        SetColliderParameters(PHYSICCOLLIDERTYPE, NUMASTEROIDPHYSICCOLLIDERS, asteroidPhysicColliderParams);
//#ifdef USE_RENDERCOLLIDERS
        SetColliderParameters(RENDERCOLLIDERTYPE, NUMASTEROIDRENDERCOLLIDERS, asteroidRenderColliderParams);
//#endif
    }
    else if (colliderType_ == MOBILECASTLETYPE)
    {
        SetColliderParameters(PHYSICCOLLIDERTYPE, NUMCASTLEPHYSICCOLLIDERS, castlePhysicColliderParams);
//#ifdef USE_RENDERCOLLIDERS
        SetColliderParameters(RENDERCOLLIDERTYPE, NUMCASTLERENDERCOLLIDERS, castleRenderColliderParams);
//#endif
    }
}

void MapBase::SetColliderParameters(MapColliderType paramtype, int num, const ColliderParams* params)
{
    colliderNumParams_[paramtype] = num;
    colliderParams_[paramtype] = params;
}


/// Entities Setters

void MapBase::AddSpots(const PODVector<MapSpot>& spots, bool adjustPositions)
{
#ifdef HANDLE_ENTITIES
    // skip if in unavailable states
    if (GetStatus() > Available)
        return;

    if (!mapData_)
        return;

//    URHO3D_LOGINFOF("MapBase() - AddSpots ...");

    mapData_->spots_ += spots;

    if (adjustPositions)
    {
        for (unsigned i=0; i < mapData_->spots_.Size(); ++i)
        {
            MapSpot& spot = mapData_->spots_[i];

            int& x = spot.position_.x_;
            int y = spot.position_.y_;
            int viewId = GetViewId(ViewManager::Get()->GetViewZ(spot.viewZIndex_));
            int maxtries = MapInfo::info.height_ - y;
            int yinc = y < MapInfo::info.height_ /2 ? 1 : -1;
            int tries = 0;

            while (tries < maxtries)
            {
                if (!HasTileProperties(x, y, viewId, TilePropertiesFlag::Blocked))
                {
                    spot.position_.y_ = y;
                    break;
                }
                else if (!HasTileProperties(x-1, y, viewId, TilePropertiesFlag::Blocked))
                {
                    spot.position_.y_ = y;
                    x =-1;
                    break;
                }
                else if (!HasTileProperties(x+1, y, viewId, TilePropertiesFlag::Blocked))
                {
                    spot.position_.y_ = y;
                    x =+1;
                    break;
                }
                else
                {
                    y += yinc;
                    tries = y < 0 || y >= MapInfo::info.height_ ? maxtries : tries+1;
                }
            }
        }
    }
//    URHO3D_LOGINFOF("MapBase() - AddSpots ... OK");
#endif
}

void MapBase::PopulateEntities(const IntVector2& numE, const Vector<StringHash>& authorizedCategories)
{
#ifdef HANDLE_ENTITIES
    // skip if in unavailable states
    if (GetStatus() > Available)
        return;

    if (!mapData_)
        return;

    if (!mapData_->spots_.Size())
        return;

    Vector<unsigned> startSpotIndexes;
    Vector<unsigned> entitySpotIndexes;
    Vector<unsigned> fluidSpotIndexes;
    Vector<unsigned> bossSpotIndexes;

    // Sort Spots by category
    for (unsigned i=0; i < mapData_->spots_.Size(); ++i)
    {
        if (mapData_->spots_[i].type_ == SPOT_START)
            startSpotIndexes.Push(i);
        else if (mapData_->spots_[i].type_ == SPOT_LIQUID)
            fluidSpotIndexes.Push(i);
        else if (mapData_->spots_[i].type_ == SPOT_BOSS)
            bossSpotIndexes.Push(i);
        else
            entitySpotIndexes.Push(i);
    }

//    URHO3D_LOGERRORF("MapBase() - PopulateEntities ... numEntitySpots=%u numStarts=%u numFluids=%u (total=%u) ...", entitySpotIndexes.Size(), startSpotIndexes.Size(), fluidSpotIndexes.Size(), mapData_->spots_.Size());
    GameRand& ORand = GameRand::GetRandomizer(OBJRAND);
    unsigned numAccessSpots = startSpotIndexes.Size();
    unsigned numEntities = AllowEntities(0) && entitySpotIndexes.Size() ? (numE != IntVector2::ZERO ? ORand.Get(Min(numE.x_,numE.y_), Max(numE.x_,numE.y_)) : entitySpotIndexes.Size()) : 0;
    unsigned numBosses = bossSpotIndexes.Size() ? bossSpotIndexes.Size() : 0;

    mapData_->entities_.Resize(numAccessSpots + numEntities);
//    URHO3D_LOGERRORF("MapBase() - PopulateEntities ... numEntities=%u(entry=%s) numAuthorizedCategories=%u ...", numEntities, numE.ToString().CString(), authorizedCategories.Size());

    // Set Start Spots
    const unsigned short gotStart = GOT::GetIndex(GOT::START);
    for (unsigned i=0; i < numAccessSpots; ++i)
    {
        const MapSpot& spot = mapData_->spots_[startSpotIndexes[i]];

        EntityData& entitydata = mapData_->entities_[i];

        entitydata.gotindex_ = gotStart;
        entitydata.tileindex_ = GetTileIndex(spot.position_.x_, spot.position_.y_);
        entitydata.sstype_ = 0;
        entitydata.SetDrawableProps(ViewManager::Get()->GetViewZ(spot.viewZIndex_));

//        URHO3D_LOGINFOF("MapBase() - PopulateEntities : spot start (gotStartIndex=%u) => x:%d y:%d z:%s",gotStart, spot.position_.x_, spot.position_.y_, ViewManager::Get()->GetViewZName(spot.viewZIndex_).CString());
    }

    // Set Entities Spots
    if (numEntities)
    {
        unsigned entityType;
        for (unsigned i=0; i < numEntities; ++i)
        {
            const MapSpot& spot = mapData_->spots_[entitySpotIndexes[ORand.Get(entitySpotIndexes.Size())]];

            entityType = MapSpot::GetRandomEntityForSpotType(spot, authorizedCategories);
            if (!entityType)
                continue;

            EntityData& entitydata = mapData_->entities_[numAccessSpots+i];

            entitydata.gotindex_ = GOT::GetIndex(StringHash(entityType));
            entitydata.tileindex_ = GetTileIndex(spot.position_.x_, spot.position_.y_);
            entitydata.sstype_ = RandomEntityFlag|RandomMappingFlag;
            entitydata.SetDrawableProps(ViewManager::Get()->GetViewZ(spot.viewZIndex_));

//            URHO3D_LOGINFOF("MapBase() - PopulateEntities : spot i=%d x=%d y=%d z:%s entity=%s(%u) ...", i, spot.position_.x_, spot.position_.y_,
//                            ViewManager::Get()->GetViewZName(spot.viewZIndex_).CString(), GOT::GetType(StringHash(entityType)).CString(), entityType);
        }
    }

    // Set Boss Spots
    if (numBosses)
    {
        mapData_->zones_.Resize(numBosses);

        for (unsigned i=0; i < numBosses; ++i)
        {
            const MapSpot& spot   = mapData_->spots_[bossSpotIndexes[i]];

            ZoneData& zonedata    = mapData_->zones_[i];

            zonedata.type_        = SPOT_BOSS;;
            zonedata.centerx_     = spot.position_.x_;
            zonedata.centery_     = spot.position_.y_;
            zonedata.zindex_      = spot.viewZIndex_;
            zonedata.SetShapeInfo(spot.geom_, spot.width_, spot.height_);

            zonedata.lastVisit_   = 0;
            zonedata.lastEnabled_ = 0;
            zonedata.nodeid_      = 0;
            zonedata.state_       = 0;

            zonedata.goInAction_  = BOSSZONE;
            zonedata.goOutAction_ = 0;

            zonedata.entityIndex_ = ORand.Get(256);
            zonedata.sstype_      = RandomEntityFlag|RandomMappingFlag;

            URHO3D_LOGINFOF("MapBase() - PopulateEntities : map=%s spot boss i=%d x=%d y=%d w=%d h=%d z:%s wait for populate it ...",
                            GetMapPoint().ToString().CString(), i, spot.position_.x_, spot.position_.y_,spot.width_, spot.height_,
                            ViewManager::Get()->GetViewZName(spot.viewZIndex_).CString());
        }
    }

    // Set Fluid Spots
    if (GameContext::Get().gameConfig_.fluidEnabled_)
    {
        for (unsigned i=0; i < fluidSpotIndexes.Size(); ++i)
        {
            const MapSpot& spot = mapData_->spots_[fluidSpotIndexes[i]];
#ifdef MAP_POPULATE_FLUID_INNERONLY
            /// TEST FLUID SIMULATION // Skip Frontview
            if (spot.viewZIndex_ != ViewManager::INNERVIEW_Index)
                continue;
#endif
            PODVector<FluidSource>& sources = featuredMap_->GetFluidView(spot.viewZIndex_).sources_;
            float qty = FLUID_MAXVALUE * ORand.Get(10, 200);
            float flow = FLUID_MAXVALUE * 0.5f;
            sources.Push(FluidSource(WATER, spot.position_.x_, spot.position_.y_, qty, flow));

//            URHO3D_LOGINFOF("MapBase() - Populate : spot Liquid => x:%d y:%d z:%s qty:%d flow:%d", spot.position_.x_, spot.position_.y_, ViewManager::Get()->GetViewZName(spot.viewZIndex_).CString(), qty, flow);
        }
    }

//    URHO3D_LOGINFO("MapBase() - PopulateEntities ... OK !");
#endif
}


/// Furniture Setters

void MapBase::AddFurnitures(const PODVector<EntityData>& furnitures)
{
// skip if in unavailable states
    if (GetStatus() > Available)
        return;

    if (!mapData_)
        return;

    if (!AllowFurnitures(0))
        return;

    mapData_->furnitures_ += furnitures;
    URHO3D_LOGINFOF("MapBase() - AddFurnitures ... add=%u total=%u", furnitures.Size(), mapData_->furnitures_.Size());
}

Node* MapBase::AddFurniture(const EntityData& entitydata)
{
    mapData_->furnitures_.Push(entitydata);
    return AddFurniture(mapData_->furnitures_.Back());
}

Node* MapBase::AddFurniture(EntityData& entitydata)
{
    Node* node = 0;

#ifdef HANDLE_FURNITURES
    if (entitydata.gotindex_ == 0 || entitydata.gotindex_ >= GOT::GetSize())
        return 0;

    StringHash got = GOT::Get(entitydata.gotindex_);

    int viewZ = ViewManager::Get()->GetLayerZ(entitydata.GetLayerZIndex());
    bool staticFurniture = (GOT::GetTypeProperties(got) & GOT_Static) != 0;
    if (!AllowDynamicFurnitures(viewZ) && !staticFurniture)
        return 0;

    int entityid = entitydata.sstype_;

    Node* attachNode = nodeFurniture_ ? nodeFurniture_ : World2D::GetEntitiesRootNode(GameContext::Get().rootScene_, GameNetwork::Get() ? REPLICATED : LOCAL);
    node = ObjectPool::CreateChildIn(got, entityid, attachNode, 0, viewZ);
    if (!node)
    {
//        URHO3D_LOGERRORF("MapBase() - AddFurniture : Map=%s ... type=%s(%u) tileindex=%s viewZ=%d can not create entity from ObjectPool !",
//                        GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value(), entitydata.tileindex_, viewZ);
        mapData_->RemoveEntityData(&entitydata, true);

        return 0;
    }

    mapData_->AddEntityData(node, &entitydata, true, true);
    mapData_->UpdateEntityNode(node, &entitydata);

//    URHO3D_LOGERRORF("MapBase() - AddFurniture : Map=%s ... id=%u type=%s(%u) entityid=%d(entry=%u), enabled=%s position=%s viewZ=%d parentNodePool=%s(%u)",
//                    GetMapPoint().ToString().CString(), node->GetID(), GOT::GetType(got).CString(), got.Value(), entityid, entitydata.sstype_, node->IsEnabled() ? "true":"false",
//                    node->GetWorldPosition2D().ToString().CString(), viewZ, node->GetParent()->GetName().CString(), node->GetParent()->GetID());

    node->ApplyAttributes();

    GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
    if (destroyer)
    {
        if (staticFurniture)
        {
            destroyer->SetEnableUnstuck(false);
            VariantMap& eventData = node->GetContext()->GetEventDataMap();
            eventData[Go_Appear::GO_MAP] = GetMapPoint().ToHash();
            eventData[Go_Appear::GO_TILE] = entitydata.tileindex_;
            node->SendEvent(WORLD_ENTITYCREATE, eventData);
        }
        else
        {
            node->SendEvent(WORLD_ENTITYCREATE);
        }
    }

    if (staticFurniture)
        World2D::AddStaticFurniture(GetMapPoint(), node, entitydata);

    node->SetEnabled(true);

    GameHelpers::SetDrawableLayerView(node, viewZ);

    // force update batch for staticsprite
    StaticSprite2D* sprite = node->GetComponent<StaticSprite2D>();
    if (sprite)
        sprite->ForceUpdateBatches();

#endif // HANDLE_FURNITURES

    return node;
}

void MapBase::SetFurnitures()
{
    if (!mapData_)
        return;

    for (unsigned i=0; i < mapData_->furnitures_.Size(); i++)
        AddFurniture(mapData_->furnitures_[i]);
}

bool MapBase::SetFurnitures(HiresTimer* timer, bool checkusable)
{
#ifdef HANDLE_FURNITURES
    if (!mapData_)
        return true;

//    URHO3D_LOGINFOF("MapBase() - SetFurnitures : Map=%s numfurnitures=%u ... ", GetMapPoint().ToString().CString(), mapData_->furnitures_.Size());

    int& mcount = GetMapCounter(MAP_GENERAL);

    unsigned i = mcount;
    unsigned gotprops;
    int viewZ;
    StringHash got;
    Node* node;
    Node* attachNode = nodeFurniture_ ? nodeFurniture_ : World2D::GetEntitiesRootNode(GameContext::Get().rootScene_, GameNetwork::Get() ? REPLICATED : LOCAL);

    ObjectPool::SetForceLocalMode(true);

    for (;;)
    {
        if (i >= mapData_->furnitures_.Size())
            break;

        EntityData& entitydata = mapData_->furnitures_[i];

        if (entitydata.gotindex_ == 0 || entitydata.gotindex_ >= GOT::GetSize())
        {
            i++;
            continue;
        }

        got = GOT::Get(entitydata.gotindex_);
        gotprops = GOT::GetTypeProperties(got);

//        URHO3D_LOGERRORF("MapBase() - SetFurnitures : Map=%s furnitures_[%d] : type=%s(%u) tileindex=%u positionInTile=%s ...",
//                        GetMapPoint().ToString().CString(), i, GOT::GetType(got).CString(), got.Value(), entitydata.tileindex_, entitydata.GetNormalizedPositionInTile().ToString().CString());

        // skip if an usable furniture (a usable furniture has attributes to serialize => done by SetEntities_Load)
        if (checkusable && (gotprops & GOT_Usable))
        {
            i++;
            continue;
        }

        viewZ = ViewManager::Get()->GetLayerZ(entitydata.GetLayerZIndex());
        if (!(gotprops & GOT_Static) && !AllowDynamicFurnitures(viewZ))
        {
            i++;
            continue;
        }

        int entityid = entitydata.sstype_;
        node = ObjectPool::CreateChildIn(got, entityid, attachNode, 0, viewZ);
        if (!node)
        {
            mapData_->RemoveEntityData(&entitydata, true);
//            URHO3D_LOGERRORF("MapBase() - SetFurnitures : Map=%s ... furnitures_[%d] : type=%s(%u) tileindex=%u viewZ=%d can not create entity from ObjectPool !",
//                                GetMapPoint().ToString().CString(), i, GOT::GetType(got).CString(), got.Value(),  entitydata.tileindex_, viewZ);
        }
        else
        {
            EntityData* entitydataptr = &entitydata;
            mapData_->AddEntityData(node, entitydataptr, false);
            mapData_->UpdateEntityNode(node, entitydataptr);

//            URHO3D_LOGERRORF("MapBase() - SetFurnitures : Map=%s ... furnitures_[%d] : id=%u type=%s(%u) entitydataptr=%u entityid=%d(entry=%d), enabled=%s position=%s (tindex=%u tposition=%d %d) viewZ=%d parent=%s(%u)",
//                            GetMapPoint().ToString().CString(), i, node->GetID(), GOT::GetType(got).CString(), got.Value(), entitydataptr, entityid, entitydata.sstype_,
//                            node->IsEnabled() ? "true":"false", node->GetWorldPosition2D().ToString().CString(), entitydata.tileindex_, entitydata.tilepositionx_, entitydata.tilepositiony_,
//                            viewZ, node->GetParent()->GetName().CString(), node->GetParent()->GetID());

            node->ApplyAttributes();

            GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
            if (destroyer)
            {
                if (gotprops & GOT_Static)
                {
                    destroyer->SetEnableUnstuck(false);
                    VariantMap& eventData = node->GetContext()->GetEventDataMap();
                    eventData[Go_Appear::GO_MAP] = GetMapPoint().ToHash();
                    eventData[Go_Appear::GO_TILE] = entitydata.tileindex_;
                    node->SendEvent(WORLD_ENTITYCREATE, eventData);
                }
                else
                {
                    node->SendEvent(WORLD_ENTITYCREATE);
                }
            }

            if (gotprops & GOT_Static)
                World2D::AddStaticFurniture(GetMapPoint(), node, entitydata);

            GameHelpers::SetDrawableLayerView(node, viewZ);

            if (node->GetVar(GOA::KEEPVISIBLE).GetBool())
                node->SetEnabledRecursive(true);
            // force update batch for staticsprite
            StaticSprite2D* sprite = node->GetComponent<StaticSprite2D>();
            if (sprite)
                sprite->ForceUpdateBatches();
        }

        i++;

        if (TimeOver(timer))
        {
//            URHO3D_LOGINFOF("Map() - SetFurnitures ... ! timer =%d", timer->GetUSec(false)/1000);
            mcount = i;
            ObjectPool::SetForceLocalMode(true);
            return false;
        }
    }

//	URHO3D_LOGINFOF("MapBase() - SetFurnitures : Map=%s numfurnitures=%u ... OK !", GetMapPoint().ToString().CString(), mapData_->furnitures_.Size());
    mcount = 0;
#endif
    return true;
}

const int BiomeLayerZByViewId[] =
{
    FRONTBIOME,     // for FrontView_ViewId
    BACKBIOME,      // for BackGround_ViewId
    BACKINNERBIOME, // for InnerView_ViewId
    BACKINNERBIOME, // for BackView_ViewId
    FRONTBIOME,     // for OuterView_ViewId
};

bool MapBase::AnchorEntityOnTileAt(EntityData& entitydata, Node* node, bool isabiome, bool checkpositionintile)
{
    const float bordershrinkfactor = 0.4f;
    const float halftilewidth = World2D::GetWorldInfo()->mTileWidth_ * 0.5f;
    const float halftileheight = World2D::GetWorldInfo()->mTileHeight_ * 0.5f;
    const unsigned gotprops = GOT::GetTypeProperties(node ? node->GetVar(GOA::GOT).GetStringHash() : GOT::Get(entitydata.gotindex_));
    const bool anchorOnGroundTile = gotprops & GOT_AnchorOnGround;
    const bool anchorOnBackTile = gotprops & GOT_AnchorOnBack;

    int viewZ, layerZ = -1;
    unsigned tileindex;
    Vector2 position;
    IntVector2 coords;

    if (node)
    {
        viewZ = node->GetVar(GOA::ONVIEWZ).GetInt();
        position = node->GetWorldPosition2D();
        tileindex = GetTileIndexAt(position);
        coords = GetTileCoords(tileindex);
    }
    else
    {
        layerZ = ViewManager::Get()->GetLayerZ(entitydata.drawableprops_ & FlagLayerZIndex_);
        tileindex = entitydata.tileindex_;
        coords = GetTileCoords(tileindex);
        Vector2 positionintile = entitydata.GetNormalizedPositionInTile();
        position = GetWorldTilePosition(coords, positionintile);
//        URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : tileindex=%u position=%s positionintile=%s ...", tileindex, position.ToString().CString(), positionintile.ToString().CString());
    }

    Vector2 offset = position - GetWorldTilePosition(coords);
    if (viewZ <= BACKGROUND)
        viewZ = BACKGROUND;
    else if (viewZ <= INNERVIEW)
        viewZ = INNERVIEW;
    else
        viewZ = FRONTVIEW;

    const Vector<int>& viewids = GetViewIDs(viewZ);

    // skip the first view if backtile needed
    int startindex = viewids.Size()-1;
    if (anchorOnBackTile)
    {
        startindex--;
        if (startindex < 0)
            return false;
    }

    // Find the side where to hang
    ContactSide anchor = NoneSide;
    int viewid = NOVIEW;
    for (int i = startindex; i >= 0; i--)
    {
        anchor = NoneSide;

        const ConnectedMap& connectmap = GetConnectedView(viewids[i]);
        ConnectIndex cindex = connectmap[tileindex];

//        URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : check tileindex=%u x=%d y=%d view=%d cindex=%d ...", tileindex, coords.x_, coords.y_, viewids[i], cindex);

        // is a block with some free sides
        if (anchorOnGroundTile && cindex < MapTilesConnectType::AllConnect)
        {
//            URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : anchorOnGroundTile check x=%d y=%d view=%d cindex=%d ...", coords.x_, coords.y_, viewids[i], cindex);

            // find the nearest side where to spawn
            if (!checkpositionintile || (Abs(offset.x_) >= halftilewidth * bordershrinkfactor && Abs(offset.x_) >= Abs(offset.y_)))
            {
                anchor = offset.x_ <= 0.f ? LeftSide : RightSide;
                // not a free side
                if ((anchor == LeftSide && (cindex & LeftSide) != 0) || (anchor == RightSide && (cindex & RightSide) != 0))
                    anchor = NoneSide;
            }

            if (!checkpositionintile || (Abs(offset.y_) >= halftileheight * bordershrinkfactor && (anchor == NoneSide || Abs(offset.x_) <= Abs(offset.y_))))
            {
                anchor = offset.y_ <= 0.f ? BottomSide : TopSide;
                // not a free side
                if ((anchor == BottomSide && (cindex & BottomSide) != 0) || (anchor == TopSide && (cindex & TopSide) != 0))
                    anchor = NoneSide;
            }

            if (anchor != NoneSide)
            {
                viewid = viewids[i];
                break;
            }
        }
        // is a block in back
        else if (anchorOnBackTile && cindex <= MapTilesConnectType::AllConnect)
        {
//            URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : anchorOnBackTile check x=%d y=%d view=%d cindex=%d ...", x, y, viewids[i], cindex);
            anchor = NoneSide;
            viewid = viewids[i];
            break;
        }
        // no tile, find the nearest block
        else if (cindex == MapTilesConnectType::Void)
        {
            int dirx = 0;
            if (offset.x_ != 0.f)
                dirx = offset.x_ > 0.f ? 1 : -1;

            int diry = 0;
            if (offset.y_ < 0.f || anchorOnGroundTile)
                diry = 1;
            else if (offset.y_ > 0.f)
                diry = -1;

//            URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : Void check x=%d y=%d view=%d cindex=%d offset=%f,%f dirx=%d diry=%d ...",
//                            coords.x_, coords.y_, viewids[i], cindex, offset.x_, offset.y_, dirx, diry);

            if (dirx && !AreCoordsOutside(coords.x_+dirx, coords.y_))
            {
                if (!checkpositionintile || (Abs(offset.x_) >= halftilewidth * bordershrinkfactor && Abs(offset.x_) > Abs(offset.y_)))
                {
                    unsigned newtileindex = GetTileIndex(coords.x_+dirx, coords.y_);
                    cindex = GetConnectedView(viewids[i])[newtileindex];

//                    URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : Void check dirx=%d x=%d y=%d cindex=%d ...", dirx, x+dirx, y, cindex);

                    if (cindex < MapTilesConnectType::AllConnect)
                    {
                        // in the nearest block the side to spawn is the opposite side
                        if (anchorOnGroundTile)
                            anchor = dirx < 0 ? RightSide : LeftSide;
                        tileindex = newtileindex;
                        coords.x_ += dirx;
                        offset = position - GetWorldTilePosition(coords);
                        viewid = viewids[i];
                        break;
                    }
                }
            }

            if (!AreCoordsOutside(coords.x_, coords.y_+diry))
            {
                if (!checkpositionintile || (Abs(offset.y_) >= halftileheight * bordershrinkfactor && (anchor == NoneSide || Abs(offset.x_) <= Abs(offset.y_))))
                {
                    unsigned newtileindex = GetTileIndex(coords.x_, coords.y_+diry);
                    cindex = GetConnectedView(viewids[i])[newtileindex];

//                    URHO3D_LOGINFOF("Map() - AnchorEntityOnTileAt : Void check diry=%d x=%d y=%d cindex=%d ...", diry, coords.x_, coords.y_+diry, cindex);

                    if (cindex < MapTilesConnectType::AllConnect)
                    {
                        // in the nearest block the side to spawn is the opposite side
                        if (anchorOnGroundTile)
                            anchor = diry > 0 ? TopSide : BottomSide;
                        tileindex = newtileindex;
                        coords.y_ += diry;
                        offset = position - GetWorldTilePosition(coords);
                        viewid = viewids[i];
                        break;
                    }
                }
            }
        }
    }

    if (viewid == NOVIEW)
        return false;

    if (anchor == NoneSide && anchorOnGroundTile)
        return false;

    // Update Datas
    entitydata.tileindex_ = tileindex;
    entitydata.SetLayerZ(isabiome ? BiomeLayerZByViewId[viewid] : layerZ > 0 ? layerZ : viewZ);
    //entitydata.drawableprops_ = (unsigned char)(ViewManager::GetLayerZIndex(BiomeLayerZByViewId[viewid]));
    entitydata.SetPositionProps(offset, anchor);

    URHO3D_LOGERRORF("Map() - AnchorEntityOnTileAt : tileindex=%u tilepositionx=%d tilepositiony=%d anchor=%d layerZIndex=%d OK !",
                     entitydata.tileindex_, entitydata.tilepositionx_, entitydata.tilepositiony_, anchor, entitydata.GetLayerZIndex());

    return true;
}

void MapBase::RefreshEntityPosition(Node* node)
{
    node->SetVar(GOA::ONTILE, -1);
    EntityData* entitydata = mapData_->GetEntityData(node);
    if (entitydata)
    {
        mapData_->SetEntityData(node, entitydata, false);
        mapData_->UpdateEntityNode(node, entitydata);
    }
}

/// Tile Modifiers

bool MapBase::FindSolidTile(unsigned index, FeatureType& feat, int& layerZ)
{
    const Vector<int>& viewids = featuredMap_->GetViewIDs(layerZ);

    for (int i=viewids.Size()-1; i >= 0; i--)
    {
        int viewid = viewids[i];
        feat = GetFeature(index, viewid);

        if (feat > MapFeatureType::NoRender || feat == MapFeatureType::Door)
        {
            layerZ = GetViewZ(viewid);
            return true;
        }
    }

    feat = 0;
    return false;
}

bool MapBase::FindSolidTile(unsigned index, int viewZ)
{
    const Vector<int>& viewids = featuredMap_->GetViewIDs(viewZ);
    FeatureType feat;

    for (int i=viewids.Size()-1; i >= 0; i--)
    {
        feat = GetFeature(index, viewids[i]);

        if (feat > MapFeatureType::NoRender || feat == MapFeatureType::Door)
            return true;
    }

    return false;
}

bool MapBase::FindEmptyTile(unsigned index, FeatureType& feat, int& layerZ)
{
    const Vector<int>& viewids = featuredMap_->GetViewIDs(layerZ);

    int viewid;
    bool findok = false;
    FeatureType feature;

    for (int i=viewids.Size()-1; i >= 0; i--)
    {
        feature = GetFeature(index, viewids[i]);

        if (feature > MapFeatureType::NoRender)
            break;

        viewid = viewids[i];
        feat = feature;
        findok = true;
    }

    if (findok)
        layerZ = GetViewZ(viewid);
    else
        feat = 0;

    return findok;
}

bool MapBase::FindEmptySpace(int numtilex, int numtiley, int viewId, IntVector2& position, int maxAcceptedBlocks)
{
    const FeaturedMap& featureMap = GetFeatureView(viewId);

    unsigned xmax = GetWidth() - numtilex;
    unsigned ymax = GetHeight() - numtiley;
    unsigned numEmptyTilesNeeded = Max(numtilex * numtiley - maxAcceptedBlocks, 1);

    for (unsigned y=0; y < ymax; y++)
    {
        for (unsigned x=0; x < xmax; x++)
        {
            if (featureMap[GetTileIndex(x, y)] < MapFeatureType::Threshold)
            {
                // find empty tile : check for empty tiles in neighborhood
                unsigned numEmptyTiles = 0;

                for (unsigned y2=y; y2 < y+numtiley; y2++)
                {
                    for (unsigned x2=x; x2 < x+numtilex; x2++)
                    {
                        if (featureMap[GetTileIndex(x2, y2)] < MapFeatureType::Threshold)
                            numEmptyTiles++;
                    }
                }

                if (numEmptyTiles >= numEmptyTilesNeeded)
                {
                    position.x_ = x;
                    position.y_ = y;
                    return true;
                }
            }
        }
    }

    return false;
}

void MapBase::FindTileIndexesWithPixelShapeAt(bool block, int centerx, int centery, int viewid, int pixelshapeid, Vector<unsigned>& tileindexes)
{
    const FeaturedMap& features = GetFeaturedMap(viewid);

    tileindexes.Clear();

    const int geom   = (pixelshapeid >> 16) & 0xFF;
    const int width  = (pixelshapeid >> 8) & 0xFF;
    const int height = pixelshapeid & 0xFF;
    const int left   = centerx - width/2;
    const int top    = centery - height/2;

    if (geom == PIXSHAPE_RECT)
    {
        unsigned tileindex;

        IntRect rect;
        rect.left_   = left;
        rect.right_  = rect.left_ + width;
        rect.top_    = top;
        rect.bottom_ = rect.top_ + height;

        if (block)
        {
            for (int x = rect.left_; x <= rect.right_; x++)
            {
                tileindex = GetTileIndex(x, rect.top_);
                if (features[tileindex] > MapFeatureType::Threshold)
                    tileindexes.Push(tileindex);
                tileindex = GetTileIndex(x, rect.bottom_);
                if (features[tileindex] > MapFeatureType::Threshold)
                    tileindexes.Push(tileindex);
            }
            for (int y = rect.top_+1; y < rect.bottom_; y++)
            {
                tileindex = GetTileIndex(rect.left_, y);
                if (features[tileindex] > MapFeatureType::Threshold)
                    tileindexes.Push(tileindex);
                tileindex = GetTileIndex(rect.right_, y);
                if (features[tileindex] > MapFeatureType::Threshold)
                    tileindexes.Push(tileindex);
            }
        }
        else
        {
            for (int x = rect.left_; x <= rect.right_; x++)
            {
                tileindex = GetTileIndex(x, rect.top_);
                if (features[tileindex] < MapFeatureType::Threshold && features[tileindex] != MapFeatureType::RoofInnerSpace)
                    tileindexes.Push(tileindex);
                tileindex = GetTileIndex(x, rect.bottom_);
                if (features[tileindex] < MapFeatureType::Threshold && features[tileindex] != MapFeatureType::RoofInnerSpace)
                    tileindexes.Push(tileindex);
            }
            for (int y = rect.top_+1; y < rect.bottom_; y++)
            {
                tileindex = GetTileIndex(rect.left_, y);
                if (features[tileindex] < MapFeatureType::Threshold && features[tileindex] != MapFeatureType::RoofInnerSpace)
                    tileindexes.Push(tileindex);
                tileindex = GetTileIndex(rect.right_, y);
                if (features[tileindex] < MapFeatureType::Threshold && features[tileindex] != MapFeatureType::RoofInnerSpace)
                    tileindexes.Push(tileindex);
            }
        }
    }
    else
    {
        unsigned tileindex;

        PixelShape* pixelshape = GameHelpers::GetPixelShape(pixelshapeid);
        if (pixelshape)
        {
            const Vector<IntVector2>& blockCoords = pixelshape->GetPixelCoords();

            if (block)
            {
                for (unsigned i=0; i < blockCoords.Size(); i++)
                {
                    const IntVector2& coord = blockCoords[i];
                    tileindex = GetTileIndex(left + coord.x_, top + coord.y_);
                    if (features[tileindex] > MapFeatureType::Threshold)
                        tileindexes.Push(tileindex);
                }
            }
            else
            {
                for (unsigned i=0; i < blockCoords.Size(); i++)
                {
                    const IntVector2& coord = blockCoords[i];
                    tileindex = GetTileIndex(left + coord.x_, top + coord.y_);
                    if (features[tileindex] < MapFeatureType::Threshold && features[tileindex] != MapFeatureType::RoofInnerSpace)
                        tileindexes.Push(tileindex);
                }
            }
        }
    }
}

bool MapBase::CanSetTile(FeatureType feat, int x, int y, int viewZ, bool permutesametiles)
{
    // skip if in unavailable states
    if (GetStatus() > Available)
        return false;

    bool newTileIsBlock = feat > MapFeatureType::NoRender;

    unsigned tileindex = GetTileIndex(x, y);
    int viewid = GetViewId(viewZ);
    FeatureType oldfeat = GetFeature(tileindex, viewid);
    bool oldTileIsBlock = oldfeat > MapFeatureType::NoRender;

    if (feat != MapFeatureType::NoMapFeature || oldfeat > MapFeatureType::Door)
    {
        if (!oldTileIsBlock && !newTileIsBlock && !permutesametiles)
            return false;

        if (oldTileIsBlock == newTileIsBlock)
            return false;
    }

    return true;
}

void MapBase::SetTile(FeatureType feat, int x, int y, int viewZ, Tile** removedtile)
{
    unsigned tileindex = GetTileIndex(x, y);
    int viewid = GetViewId(viewZ);

    // Get old feature
    FeatureType& featref = GetFeatureRef(tileindex, viewid);
    FeatureType oldfeat = featref;
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d z=%d viewid=%s(%d) oldfeat=%s(%u) newfeat=%s(%u) ...",
                    GetMapPoint().ToString().CString(), x, y, viewZ, ViewManager::GetViewName(viewid).CString(), viewid, MapFeatureType::GetName(oldfeat), oldfeat, MapFeatureType::GetName(feat), feat);
#endif
    bool newTileIsBlock = feat > MapFeatureType::NoRender;

    // Check if it's on a TileEntityNode
    if (!newTileIsBlock && oldfeat < MapFeatureType::NoRender)
    {
        if (SetTileEntity(feat, tileindex, viewZ))
        {
#ifdef DUMP_MAPDEBUG_SETTILE
            URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d z=%d remove a tile box node !", GetMapPoint().ToString().CString(), x, y, viewZ);
#endif
            return;
        }
    }

    bool plateformRemoved = oldfeat == MapFeatureType::RoomPlateForm;
    bool plateformAdded = false;

    PODVector<FeatureType> savedfeatures;
    featuredMap_->GetAllViewFeatures(tileindex, savedfeatures);

    if (viewZ == INNERVIEW)
    {
        // If Remove in Innerview, update features in BackView and OuterView too (that disable unwanted effects with ObjectFeatured::ApplyFeatures (REPLACEFEATUREBACK))
        if (feat == MapFeatureType::NoMapFeature)
        {
            int tempviewid = GetViewId(BACKVIEW);
            if (tempviewid != -1)
                GetFeatureRef(tileindex, tempviewid) = MapFeatureType::NoMapFeature;
            tempviewid = GetViewId(OUTERVIEW);
            if (tempviewid != -1)
                GetFeatureRef(tileindex, tempviewid) = MapFeatureType::NoMapFeature;
        }

        // if Add Tile and a plateform is in left or right of the tile => set the feature to plateform
        if (newTileIsBlock && ((x > 0 && GetFeatureType(tileindex-1, viewid) == MapFeatureType::RoomPlateForm) || (x < GetWidth()-1 && GetFeatureType(tileindex+1, viewid) == MapFeatureType::RoomPlateForm)))
        {
            plateformAdded = true;
            feat = MapFeatureType::RoomPlateForm;
        }
    }

    // Change to new feature
    featref = feat;

    // Update TileModifier
    TileModifier modifier(x, y, viewZ, featref, oldfeat);
    List<TileModifier >::Iterator mt = cacheTileModifiers_.Find(modifier);
    if (mt != cacheTileModifiers_.End())
    {
        if (mt->oFeat_ == featref)
        {
            cacheTileModifiers_.Erase(mt);
//                URHO3D_LOGINFOF("MapBase() - SetTile : TileModifier Removed (original feat restored) => TileModifiers Size=%u !", cacheTileModifiers_.Size());
        }
        else
        {
            mt->feat_ = featref;
//                URHO3D_LOGINFOF("MapBase() - SetTile : TileModifier Updated !");
        }
    }
    else
    {
        cacheTileModifiers_.Push(modifier);
//            URHO3D_LOGINFOF("MapBase() - SetTile : TileModifier Added => TileModifiers (%s(%u) %d %d %d) Size=%u !", MapFeatureType::GetName(featref), featref, x, y, viewZ, cacheTileModifiers_.Size());
    }
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - SetTile ... before filters feat=%s(%u)", MapFeatureType::GetName(featref), featref);
#endif
    bool impact = featuredMap_->ApplyFeatureFilters(tileindex);
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - SetTile ... after filters feat=%s(%u)", MapFeatureType::GetName(featref), featref);
#endif
    // Get Impacted Views
    Vector<IntVector2> viewsToUpdate;
    Vector<IntVector2> renderviewsToUpdate;
    PODVector<FeatureType> features;
    featuredMap_->GetAllViewFeatures(tileindex, features);

    const Vector<int>& viewids = GetViewIDs(FRONTVIEW);
    bool viewAboveChanged = false;
    for (int i=viewids.Size()-1; i >= 0; i--)
    {
        int cviewid = viewids[i];
        bool updateView = savedfeatures[cviewid] != features[cviewid] || (features[cviewid] < MapFeatureType::NoRender && viewAboveChanged);
        bool updateRender = savedfeatures[cviewid] != features[cviewid] || viewAboveChanged;

        if (updateView || updateRender)
        {
            int switchview = GetViewZ(cviewid);
            switchview = switchview > BACKGROUND && switchview < OUTERVIEW ? INNERVIEW : FRONTVIEW;
            if (updateView)
                viewsToUpdate.Push(IntVector2(cviewid, switchview));

            if (GameContext::Get().gameConfig_.renderShapes_)
                if (updateRender)
                    renderviewsToUpdate.Push(IntVector2(cviewid, switchview));

            viewAboveChanged = true;
        }
        else
        {
            viewAboveChanged = false;
        }
    }

    // Update fluid cell
    FluidCell* fluidcell = GetFluidCellPtr(tileindex, ViewManager::viewZIndexes_[viewZ]);
    fluidcell->Set(featref);
    fluidcell->UnsettleNeighbors();
    fluidcell->ResetDirections();

//    URHO3D_LOGINFOF("MapBase() - SetTile : Update ObjectSkinned : x=%d y=%d ... ", x, y);

    if (removedtile && viewsToUpdate.Size())
        *removedtile = GetTile(tileindex, viewsToUpdate.Front().x_);

    // Set Tile Views
    for (unsigned i=0; i < viewsToUpdate.Size(); i++)
        skinnedMap_->SetTile(x, y, viewsToUpdate[i].x_);

    // Specific Child Class Update
    OnTileModified(x, y);

    // Update Physic Colliders
    PODVector<MapCollider*> colliders;
    for (unsigned i=0; i < viewsToUpdate.Size(); i++)
    {
        int indv = GetColliderIndex(viewsToUpdate[i].y_, viewsToUpdate[i].x_);
        if (indv != -1)
        {
            GetColliders(PHYSICCOLLIDERTYPE, indv, colliders);
#ifdef DUMP_MAPDEBUG_SETTILE
            URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d viewid=%s(%d) UpdateColliders num=%u ...", GetMapPoint().ToString().CString(), x, y, ViewManager::GetViewName(viewsToUpdate[i].x_).CString(), viewsToUpdate[i].x_, colliders.Size());
#endif
            for (unsigned j=0; j < colliders.Size(); j++)
            {
                if (plateformRemoved || plateformAdded)
                {
                    UpdatePlateformBoxes(*static_cast<PhysicCollider*>(colliders[j]), GetTileIndex(x, y), plateformAdded);
                }
                else
                {
                    bool result = UpdatePhysicCollider(*static_cast<PhysicCollider*>(colliders[j]), x, y);
                }
            }
#ifdef DUMP_MAPDEBUG_SETTILE
            URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d viewid=%s(%d) UpdateColliders ... OK !", GetMapPoint().ToString().CString(), x, y, ViewManager::GetViewName(viewsToUpdate[i].x_).CString(), viewsToUpdate[i].x_);
#endif
        }
#ifdef DUMP_MAPDEBUG_SETTILE
        else
        {
            URHO3D_LOGWARNINGF("MapBase() - SetTile : map=%s x=%d y=%d viewid=%s(%d) UpdateColliders ... No collider", GetMapPoint().ToString().CString(), x, y, ViewManager::GetViewName(viewsToUpdate[i].x_).CString(), viewsToUpdate[i].x_);
        }
#endif
    }

    // Update Render Colliders
    if (GameContext::Get().gameConfig_.renderShapes_)
    {
        for (unsigned i=0; i < renderviewsToUpdate.Size(); i++)
        {
            int indv = GetColliderIndex(renderviewsToUpdate[i].y_, renderviewsToUpdate[i].x_);
            if (indv != -1)
            {
                GetColliders(RENDERCOLLIDERTYPE, indv, colliders);
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d viewid=%s(%d) UpdateRenderCollider ...", GetMapPoint().ToString().CString(), x, y, ViewManager::GetViewName(renderviewsToUpdate[i].x_).CString(), renderviewsToUpdate[i].x_);
#endif
                for (unsigned j=0; j < colliders.Size(); j++)
                    UpdateRenderCollider(*((RenderCollider*)colliders[j]));
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d viewid=%s(%d) UpdateRenderCollider ... OK !", GetMapPoint().ToString().CString(), x, y, ViewManager::GetViewName(renderviewsToUpdate[i].x_).CString(), renderviewsToUpdate[i].x_);
#endif
            }
#ifdef DUMP_MAPDEBUG_SETTILE
            else
            {
                URHO3D_LOGWARNINGF("MapBase() - SetTile : map=%s x=%d y=%d viewid=%s(%d) UpdateRenderCollider ... No collider", GetMapPoint().ToString().CString(), x, y, ViewManager::GetViewName(renderviewsToUpdate[i].x_).CString(), renderviewsToUpdate[i].x_);
            }
#endif
        }
    }
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - SetTile : map=%s x=%d y=%d z=%d viewid=%s(%d) oldfeat=%s(%u) newfeat=%s(%u) ... OK !",
                    GetMapPoint().ToString().CString(), x, y, viewZ, ViewManager::GetViewName(viewid).CString(), viewid, MapFeatureType::GetName(oldfeat), oldfeat, MapFeatureType::GetName(featref), featref);
#endif
}

void MapBase::SetTiles(FeatureType feat, int viewZ, const Vector<unsigned>& tileIndexes)
{
    int viewid = GetViewId(viewZ);

    // Impacted Views
    HashSet<IntVector2> viewsToUpdate;
    HashSet<IntVector2> renderviewsToUpdate;

    static Vector<unsigned> modifiedTiles, addedPlateformTiles, removedPlateformTiles;
    modifiedTiles.Clear();
    addedPlateformTiles.Clear();
    removedPlateformTiles.Clear();

    for (unsigned i=0; i < tileIndexes.Size(); i++)
    {
        unsigned tileindex = tileIndexes[i];
        int x = GetTileCoordX(tileindex);
        int y = GetTileCoordY(tileindex);

        // Get old feature
        FeatureType& featref = GetFeatureRef(tileindex, viewid);
        FeatureType oldfeat = featref;
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - SetTiles : map=%s x=%d y=%d z=%d viewid=%s(%d) oldfeat=%s(%u) newfeat=%s(%u) ...",
                        GetMapPoint().ToString().CString(), x, y, viewZ, ViewManager::GetViewName(viewid).CString(), viewid, MapFeatureType::GetName(oldfeat), oldfeat, MapFeatureType::GetName(feat), feat);
#endif
        bool newTileIsBlock = feat > MapFeatureType::NoRender;

        // Check if it's on a TileEntityNode
        if (!newTileIsBlock && oldfeat < MapFeatureType::NoRender)
        {
            if (SetTileEntity(feat, tileindex, viewZ))
            {
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - SetTiles : map=%s x=%d y=%d z=%d remove a tile box node !", GetMapPoint().ToString().CString(), x, y, viewZ);
#endif
                continue;
            }
        }

        if (oldfeat == MapFeatureType::RoomPlateForm)
        {
            removedPlateformTiles.Push(tileindex);
        }

        PODVector<FeatureType> savedfeatures;
        featuredMap_->GetAllViewFeatures(tileindex, savedfeatures);

        if (viewZ == INNERVIEW)
        {
            // If Remove in Innerview, update features in BackView and OuterView too (that disable unwanted effects with ObjectFeatured::ApplyFeatures (REPLACEFEATUREBACK))
            if (feat == MapFeatureType::NoMapFeature)
            {
                int tempviewid = GetViewId(BACKVIEW);
                if (tempviewid != -1)
                    GetFeatureRef(tileindex, tempviewid) = MapFeatureType::NoMapFeature;
                tempviewid = GetViewId(OUTERVIEW);
                if (tempviewid != -1)
                    GetFeatureRef(tileindex, tempviewid) = MapFeatureType::NoMapFeature;
            }

            // if Add Tile and a plateform is in left or right of the tile => set the feature to plateform
            if (newTileIsBlock && ((x > 0 && GetFeatureType(tileindex-1, viewid) == MapFeatureType::RoomPlateForm) || (x < GetWidth()-1 && GetFeatureType(tileindex+1, viewid) == MapFeatureType::RoomPlateForm)))
            {
                addedPlateformTiles.Push(tileindex);
                feat = MapFeatureType::RoomPlateForm;
            }
        }

        // Change to new feature
        featref = feat;

        if (feat != MapFeatureType::RoomPlateForm)
            modifiedTiles.Push(tileindex);

        // Update TileModifier
        TileModifier modifier(x, y, viewZ, featref, oldfeat);
        List<TileModifier >::Iterator mt = cacheTileModifiers_.Find(modifier);
        if (mt != cacheTileModifiers_.End())
        {
            if (mt->oFeat_ == featref)
            {
                cacheTileModifiers_.Erase(mt);
//                URHO3D_LOGINFOF("MapBase() - SetTile : TileModifier Removed (original feat restored) => TileModifiers Size=%u !", cacheTileModifiers_.Size());
            }
            else
            {
                mt->feat_ = featref;
//                URHO3D_LOGINFOF("MapBase() - SetTile : TileModifier Updated !");
            }
        }
        else
        {
            cacheTileModifiers_.Push(modifier);
//            URHO3D_LOGINFOF("MapBase() - SetTile : TileModifier Added => TileModifiers (%s(%u) %d %d %d) Size=%u !", MapFeatureType::GetName(featref), featref, x, y, viewZ, cacheTileModifiers_.Size());
        }
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - SetTiles ... before filters feat=%s(%u)", MapFeatureType::GetName(featref), featref);
#endif
        bool impact = featuredMap_->ApplyFeatureFilters(tileindex);
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - SetTiles ... after filters feat=%s(%u)", MapFeatureType::GetName(featref), featref);
#endif
        PODVector<FeatureType> features;
        featuredMap_->GetAllViewFeatures(tileindex, features);

        const Vector<int>& viewids = GetViewIDs(FRONTVIEW);
        bool viewAboveChanged = false;
        for (int i=viewids.Size()-1; i >= 0; i--)
        {
            int cviewid = viewids[i];
            bool updateView = savedfeatures[cviewid] != features[cviewid] || (features[cviewid] < MapFeatureType::NoRender && viewAboveChanged);
            bool updateRender = savedfeatures[cviewid] != features[cviewid] || viewAboveChanged;

            if (updateView || updateRender)
            {
                IntVector2 viewinfo(cviewid, GetViewZ(cviewid));
                viewinfo.y_ = viewinfo.y_ > BACKGROUND && viewinfo.y_ < OUTERVIEW ? INNERVIEW : FRONTVIEW;

                if (updateView)
                    viewsToUpdate.Insert(viewinfo);

                if (updateRender && GameContext::Get().gameConfig_.renderShapes_)
                    renderviewsToUpdate.Insert(viewinfo);

                viewAboveChanged = true;
            }
            else
            {
                viewAboveChanged = false;
            }
        }

        // Update fluid cell
        FluidCell* fluidcell = GetFluidCellPtr(tileindex, ViewManager::viewZIndexes_[viewZ]);
        fluidcell->Set(featref);
        fluidcell->UnsettleNeighbors();
        fluidcell->ResetDirections();

        // Set Tile Views
        for(HashSet<IntVector2>::Iterator it = viewsToUpdate.Begin(); it != viewsToUpdate.End(); ++it)
            skinnedMap_->SetTile(x, y, it->x_);

        // Specific Child Class Update
        OnTileModified(x, y);
    }

    // Update Physic Colliders
    PODVector<MapCollider*> colliders;
    for(HashSet<IntVector2>::Iterator it = viewsToUpdate.Begin(); it != viewsToUpdate.End(); ++it)
    {
        int indv = GetColliderIndex(it->y_, it->x_);
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - SetTiles ... update physiccollider viewid=%d viewz=%d ...", it->x_, it->y_);
#endif
        if (indv != -1)
        {
            GetColliders(PHYSICCOLLIDERTYPE, indv, colliders);

            for (unsigned j=0; j < colliders.Size(); j++)
            {
                if (addedPlateformTiles.Size() || removedPlateformTiles.Size())
                    UpdatePlateformBoxes(*static_cast<PhysicCollider*>(colliders[j]), addedPlateformTiles, removedPlateformTiles);

                if (modifiedTiles.Size())
                    bool result = UpdatePhysicCollider(*static_cast<PhysicCollider*>(colliders[j]), modifiedTiles);
            }
        }
    }

    // Update Render Colliders
    if (GameContext::Get().gameConfig_.renderShapes_)
    {
        for(HashSet<IntVector2>::Iterator it = renderviewsToUpdate.Begin(); it != renderviewsToUpdate.End(); ++it)
        {
            int indv = GetColliderIndex(it->y_, it->x_);
            if (indv != -1)
            {
                GetColliders(RENDERCOLLIDERTYPE, indv, colliders);

                for (unsigned j=0; j < colliders.Size(); j++)
                    UpdateRenderCollider(*((RenderCollider*)colliders[j]));
            }
        }
    }
}

bool MapBase::SetTileEntity(FeatureType feature, unsigned tileindex, int viewZ, bool dynamic)
{
    bool result = false;
    const bool addtile = feature > MapFeatureType::NoRender;
    const int viewId = GetViewId(viewZ);
    PhysicCollider* collider;

    // Update Physic Collider
    {
        PODVector<MapCollider*> colliders;
        GetColliders(PHYSICCOLLIDERTYPE, GetColliderIndex(viewZ, viewId), colliders);
        collider = static_cast<PhysicCollider*>(colliders.Back());
        if (!collider)
        {
            URHO3D_LOGERRORF("MapBase() - SetTileEntity : physiccollider viewid=%d viewz=%d Error !", viewId, viewZ);
            return false;
        }
        result = UpdateCollisionBox(*collider, tileindex, addtile, true, dynamic);
    }

    if (!result)
        return false;
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - SetTileEntity : feat=%s(%u) at tileindex=%u viewid=%d viewz=%d !", MapFeatureType::GetName(feature), feature, tileindex, viewId, viewZ);
#endif
    // Update fluid cell
    if (!dynamic)
    {
        FluidCell* fluidcell = GetFluidCellPtr(tileindex, ViewManager::viewZIndexes_[viewZ]);
        fluidcell->Set(feature);
        fluidcell->UnsettleNeighbors();
        fluidcell->ResetDirections();
    }

    // Add Tile Node
    if (addtile)
    {
        HashMap<unsigned, CollisionBox2D* >::Iterator it = collider->blocks_.Find(tileindex);
        if (it != collider->blocks_.End())
        {
            TerrainAtlas* atlas = World2D::GetWorld()->GetWorldInfo()->atlas_;
            const MapTerrain& terrain = atlas->GetTerrain(GetTerrain(tileindex, viewId));

            Node* node = it->second_->GetNode();

            // Add the Tile Drawable And Borders
            {
                StaticSprite2D* drawable = node->CreateComponent<StaticSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
                drawable->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERGROUNDS]);
#endif
                drawable->SetSprite(atlas->GetTileSprite(terrain.GetRandomTileGidForDimension(TILE_1X1)));
                drawable->SetColor(terrain.GetColor());
                drawable->SetViewMask(ViewManager::GetLayerMask(viewZ));
                drawable->SetLayer2(IntVector2(viewZ + LAYER_ACTOR + 1 + LAYER_DECALS, BACKACTORVIEW));
                drawable->SetOrderInLayer(DRAWORDER_TILEENTITY);
                drawable->SetHotSpot(Vector2(0.5f, 0.5f));
                drawable->SetUseHotSpot(true);
            }

            // Add Borders (decal)
            {
                const int DecalSide[4] = { TopSide, RightSide, BottomSide, LeftSide };
                const Vector2 DecalOffset[4] = { Vector2(0.f, MapInfo::info.mTileHalfSize_.y_), Vector2(MapInfo::info.mTileHalfSize_.x_, 0.f),
                                                 Vector2(0.f, -MapInfo::info.mTileHalfSize_.y_), Vector2(-MapInfo::info.mTileHalfSize_.x_, 0.f)
                                               };
                const bool DecalFlip[8] = { false, false, false, false, false, true, true, false };

                StaticSprite2D* drawable;
                Sprite2D* sprite;
                Rect rect;

                for (int i=0; i < 4; i++)
                {
                    const bool& flipX = DecalFlip[i*2];
                    const bool& flipY = DecalFlip[i*2+1];

                    drawable = node->CreateComponent<StaticSprite2D>();
#ifdef ACTIVE_LAYERMATERIALS
                    drawable->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERGROUNDS]);
#endif
                    sprite = atlas->GetDecalSprite(terrain.GetDecalGidTableForConnectIndex(DecalSide[i])[0]);
                    sprite->GetDrawRectangle(rect, flipX, flipY);
                    rect.min_ += DecalOffset[i];
                    rect.max_ += DecalOffset[i];
                    drawable->SetSprite(sprite);
                    if (flipX)
                        drawable->SetFlipX(true);
                    if (flipY)
                        drawable->SetFlipY(true);
                    drawable->SetColor(terrain.GetColor());
                    drawable->SetViewMask(ViewManager::GetLayerMask(viewZ));
                    drawable->SetLayer2(IntVector2(viewZ + LAYER_ACTOR + 1 + LAYER_DECALS, BACKACTORVIEW));
                    drawable->SetOrderInLayer(DRAWORDER_TILEENTITY+i+1);
                    drawable->SetDrawRect(rect);
                    drawable->SetUseDrawRect(true);
                }
            }
        }
    }
    // the remove is done by UpdateCollisionBox (that deletes simply the node)

    return true;
}

bool MapBase::SetTileModifiers(HiresTimer* timer, const long long& delay)
{
    // skip if in unavailable states
    if (GetStatus() > Available)
        return false;

    if (!mapData_)
        return true;

    int& mcount1 = GetMapCounter(MAP_FUNC1);

    if (mcount1 == 0)
    {
        if (!CopyTileModifiersToCache(timer))
            return false;

        if (!cacheTileModifiers_.Size())
        {
            URHO3D_LOGWARNINGF("MapBase() - SetTileModifiers : map=%s NoModifiers Setted !", GetMapPoint().ToString().CString());
            return true;
        }

        mcount1++;
    }

    if (mcount1 == 1)
    {
        int viewid;

        // update features
        for (List<TileModifier >::ConstIterator it=cacheTileModifiers_.Begin(); it!=cacheTileModifiers_.End(); ++it)
        {
            const TileModifier& m = *it;

            viewid = GetViewId(m.z_);

            if (viewid != NOVIEW)
            {
                unsigned tileindex = GetTileIndex(m.x_, m.y_);

                // Always change feature in BackView and OuterView to disable unwanted effects with ObjectFeatured::ApplyFeatures (REPLACEFEATUREBACK)
                if (viewid == InnerView_ViewId && m.feat_ == MapFeatureType::NoMapFeature)
                {
                    GetFeatureRef(tileindex, BackView_ViewId) = MapFeatureType::NoMapFeature;
                    GetFeatureRef(tileindex, OuterView_ViewId) = MapFeatureType::NoMapFeature;
                }

                GetFeatureRef(tileindex, viewid) = m.feat_;

                URHO3D_LOGINFOF("MapBase() - SetTileModifiers : TileModifier Added => TileModifiers (%s(%u) %d %d %d) !",
                                MapFeatureType::GetName(m.feat_), m.feat_, m.x_, m.y_, m.z_);
            }
            else
            {
                URHO3D_LOGERRORF("MapBase() - SetTileModifiers : TileModifiers (%s(%u) %d %d viewZ=%d => viewid=NOVIEW (MAP VERSION ERROR)!",
                                 MapFeatureType::GetName(m.feat_), m.feat_, m.x_, m.y_, m.z_);
                break;
            }
        }

        mcount1++;
        featuredMap_->indexToSet_ = 0;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount1 == 2)
    {
        // Apply features Filters
        if (!featuredMap_->ApplyFeatureFilters(cacheTileModifiers_, timer, delay))
            return false;

        URHO3D_LOGINFOF("MapBase() - SetTileModifiers : map=%s numModifiers=%u", GetMapPoint().ToString().CString(), cacheTileModifiers_.Size());
        return true;
    }

    return false;
}



/// Mapdata Updaters

// copy from MapData to Cached TileModifiers
bool MapBase::CopyTileModifiersToCache(HiresTimer* timer)
{
    if (!mapData_)
        return true;

    cacheTileModifiers_.Clear();

    const Vector<TileModifier>& modifiers = mapData_->tilesModifiers_;
    for (Vector<TileModifier>::ConstIterator it=modifiers.Begin(); it!=modifiers.End(); ++it)
        cacheTileModifiers_.Push(*it);

    return true;
}

// copy from Cached TileModifiers to MapData
bool MapBase::CopyTileModifiersToMapData(HiresTimer* timer)
{
    if (!mapData_)
        return true;

    Vector<TileModifier>& tileModifiers = mapData_->tilesModifiers_;
    tileModifiers.Clear();

    if (tileModifiers.Capacity() < cacheTileModifiers_.Size())
        tileModifiers.Reserve(cacheTileModifiers_.Size());

    for (List<TileModifier>::ConstIterator it=cacheTileModifiers_.Begin(); it != cacheTileModifiers_.End(); ++it)
        tileModifiers.Push(*it);

    return true;
}

bool MapBase::UpdateMapData(HiresTimer* timer)
{
    if (!GetMapData())
        return true;

    int& mcount = GetMapCounter(MAP_FUNC1);

    if (mcount == 0)
    {
        GetMapCounter(MAP_FUNC2) = 0;
        mcount++;
    }
    if (mcount == 1)
    {
        if (!CopyTileModifiersToMapData(timer))
            return false;

        GetMapCounter(MAP_FUNC2) = 0;
        mcount++;
    }
    if (mcount == 2)
    {
        int& mcount2 = GetMapCounter(MAP_FUNC2);
        while (mcount2 < featuredMap_->fluidView_.Size())
        {
            if (!featuredMap_->fluidView_[mcount2].UpdateMapData(timer))
                return false;

            mcount2++;
        }

        mcount2 = 0;
        mcount++;
    }
    if (mcount == 3)
    {
        if (!OnUpdateMapData(timer))
            return false;

        GetMapCounter(MAP_FUNC2) = mcount = 0;

        GetMapData()->RemoveEffectActions();

        return true;
    }

    return false;
}

bool MapBase::OnUpdateMapData(HiresTimer* timer)
{
    return true;
}



/// Colliders Generators

bool MapBase::CreateColliders(HiresTimer* timer, bool setPhysic)
{
    int& mcount0 = GetMapCounter(MAP_GENERAL);

    if (mcount0 == 0)
    {
        URHO3D_LOGINFOF("MapBase() - CreateColliders : ... ");

        unsigned numcolliders;

        if (setPhysic)
        {
            // PhysicColliders
            numcolliders = Min(colliderNumParams_[PHYSICCOLLIDERTYPE], physicColliders_.Size());
            for (unsigned i = 0; i < numcolliders; i++)
                physicColliders_[i].SetParams(&colliderParams_[PHYSICCOLLIDERTYPE][i]);
        }

        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            // RenderColliders
            numcolliders = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
            for (unsigned i = 0; i < numcolliders; i++)
                renderColliders_[i].SetParams(&colliderParams_[RENDERCOLLIDERTYPE][i]);
        }

        mcount0++;
        GetMapCounter(MAP_FUNC1) = 0;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount0 == 1)
    {
        if (!GenerateColliders(timer, setPhysic))
            return false;

        mcount0 = 0;

        URHO3D_LOGINFOF("MapBase() - CreateColliders : ... OK !");
    }

    return true;
}

bool MapBase::GenerateColliders(HiresTimer* timer, bool setPhysic)
{
    int& mcount1 = GetMapCounter(MAP_FUNC1);
    int& mcount2 = GetMapCounter(MAP_FUNC2);

    if (mcount1 == 0)
    {
        MapColliderGenerator::Get()->SetParameters(true, true, 2, 0);
        mcount2 = 0;
        GetMapCounter(MAP_FUNC3) = 0;
        mcount1++;

        if (!setPhysic)
            mcount1++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount1 == 1)
    {
        if (setPhysic)
        {
            MapColliderGenerator::Get()->SetParameters(true, true, 2, 0);
            unsigned numcolliders = Min(colliderNumParams_[PHYSICCOLLIDERTYPE], physicColliders_.Size());

            if (mcount2 < numcolliders)
            {
                for (unsigned i = mcount2; i < numcolliders; i++)
                {
                    int width = 0;
                    int height = 0;

                    if (physicColliders_[i].params_->mode_ == TopBorderBackMode)
                    {
                        if (skipInitialTopBorder_)
                        {
                            mcount2++;
                            continue;
                        }

                        width = GetWidth();
                        height = 1;
                    }

                    if (!MapColliderGenerator::Get()->GeneratePhysicCollider(this, timer, physicColliders_[i], width, height, center_))
                        return false;

                    mcount2++;
                    GetMapCounter(MAP_FUNC3) = 0;
                }
            }

            mcount2 = 0;
            GetMapCounter(MAP_FUNC3) = 0;
            mcount1++;

            if (TimeOverMaximized(timer))
                return false;
        }
        else
        {
            mcount2 = 0;
            GetMapCounter(MAP_FUNC3) = 0;
            mcount1++;
        }
    }

    if (mcount1 == 2)
    {
        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            MapColliderGenerator::Get()->SetParameters(true, true, 2, 0);
            unsigned numcolliders = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());

            if (mcount2 < numcolliders)
            {
                for (unsigned i = mcount2; i < numcolliders; i++)
                {
                    if (!MapColliderGenerator::Get()->GenerateRenderCollider(this, timer, renderColliders_[i], center_))
                        return false;

                    mcount2++;
                    GetMapCounter(MAP_FUNC3) = 0;
                }
            }
        }

        mcount1 = mcount2 = GetMapCounter(MAP_FUNC3) = 0;
        MapColliderGenerator::Get()->SetParameters(false);
    }

    return true;
}

static unsigned char sLastContourId_;

bool MapBase::UpdatePhysicColliders(HiresTimer* timer)
{
    /// TODO Async
    unsigned numcolliders = Min(colliderNumParams_[PHYSICCOLLIDERTYPE], physicColliders_.Size());

    for (unsigned i = 0; i < numcolliders; i++)
        bool result = UpdatePhysicCollider(physicColliders_[i]);

    return true;
}

bool MapBase::UpdatePhysicCollider(PhysicCollider& collider, int x, int y, bool box)
{
    bool result = false;

    if (!box && collider.params_->shapetype_ == SHT_CHAIN)
    {
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - UpdatePhysicCollider : update colliderid=%d x=%d y=%d Chains ...", collider.id_, x, y);
#endif
        unsigned tileindex = 0;

        // Save LastContourId at x y just before Generate new contour
        if (x != -1)
        {
            tileindex = GetTileIndex(x, y);
            if (tileindex >= collider.contourIds_.Size())
                return false;

            sLastContourId_ = collider.contourIds_[tileindex];

//            URHO3D_LOGINFOF("MapBase() - UpdatePhysicCollider : Dump Last Contours IDs :");
//            GameHelpers::DumpData(&collider.contourIds_[0], -1, 2, GetWidth(), GetHeight());
        }

        // Trace new contours
        MapColliderGenerator::Get()->SetParameters(true, true);
        MapColliderGenerator::Get()->GeneratePhysicCollider(this, 0, collider, 0, 0, center_, false);
        MapColliderGenerator::Get()->SetParameters(false);

//        URHO3D_LOGINFOF("MapBase() - UpdatePhysicCollider : Dump New Contours IDs :");
//        GameHelpers::DumpData(&collider.contourIds_[0], -1, 2, GetWidth(), GetHeight());

        // Update Colliders
        if (x != -1)
            result |= UpdateCollisionChain(collider, tileindex);
    }
    else if (box || collider.params_->shapetype_ == SHT_BOX)
    {
        PODVector<unsigned> blocksAdded;
        PODVector<unsigned> blocksRemoved;
        MapColliderGenerator::Get()->GetUpdatedBlockBoxes(collider, x, y, blocksAdded, blocksRemoved);
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - UpdatePhysicCollider : update colliderid=%d x=%d y=%d Boxes ... boxesAdded=%u boxesRemoved=%u ...", collider.id_, x, y, blocksAdded.Size(), blocksRemoved.Size());
#endif
        for (unsigned i=0; i < blocksAdded.Size(); i++)
            result |= UpdateCollisionBox(collider, blocksAdded[i], true);
        for (unsigned i=0; i < blocksRemoved.Size(); i++)
            result |= UpdateCollisionBox(collider, blocksRemoved[i], false);
    }

    return result;
}

bool MapBase::UpdatePhysicCollider(PhysicCollider& collider, const Vector<unsigned>& tileIndexes, bool box)
{
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - UpdatePhysicCollider : update colliderid=%d numtileindexes=%u ...", collider.id_, tileIndexes.Size());
#endif
    bool result = false;

    if (!box && collider.params_->shapetype_ == SHT_CHAIN)
    {
        // Trace new contours
        MapColliderGenerator::Get()->SetParameters(true, true);
        MapColliderGenerator::Get()->GeneratePhysicCollider(this, 0, collider, 0, 0, center_, false);
        MapColliderGenerator::Get()->SetParameters(false);

        // Update CollisionShapes
        for (unsigned i=0; i < tileIndexes.Size(); i++)
            result |= UpdateCollisionChain(collider, tileIndexes[i]);
    }
    else if (box || collider.params_->shapetype_ == SHT_BOX)
    {
        PODVector<unsigned> blocksAdded;
        PODVector<unsigned> blocksRemoved;

        for (unsigned i=0; i < tileIndexes.Size(); i++)
            MapColliderGenerator::Get()->GetUpdatedBlockBoxes(collider, GetTileCoordX(tileIndexes[i]), GetTileCoordY(tileIndexes[i]), blocksAdded, blocksRemoved);

        for (unsigned i=0; i < blocksAdded.Size(); i++)
            result |= UpdateCollisionBox(collider, blocksAdded[i], true);
        for (unsigned i=0; i < blocksRemoved.Size(); i++)
            result |= UpdateCollisionBox(collider, blocksRemoved[i], false);
    }
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - UpdatePhysicCollider : update collider ... OK !");
#endif
    return result;
}

bool MapBase::UpdateRenderColliders(HiresTimer* timer)
{
    /// TODO Async
    unsigned numcolliders = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
    for (unsigned i = 0; i < numcolliders; i++)
    {
        URHO3D_LOGINFOF("MapBase() - UpdateRenderColliders : update collider i=%u ...", i);
        UpdateRenderCollider(renderColliders_[i]);
    }

    return true;
}

void MapBase::UpdateRenderCollider(RenderCollider& collider)
{
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - UpdateRenderCollider : update collider ...");
#endif
    if (!collider.renderShape_)
        return;

    MapColliderGenerator::Get()->SetParameters(true, true);
    MapColliderGenerator::Get()->GenerateRenderCollider(this, 0, collider, center_);
    MapColliderGenerator::Get()->SetParameters(false);

    collider.UpdateRenderShape();

    /// TODO : do just one update by indZ (not at each rendercollider update)
    /// create dirty state for minimaplayer
//    UpdateMiniMap(collider.params_->indz_);
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - UpdateRenderCollider : update collider ... OK !");
#endif
}

bool MapBase::UpdateRenderShapes(HiresTimer* timer)
{
    int& mcount0 = GetMapCounter(MAP_GENERAL);

    unsigned numcolliders = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());

    if (mcount0 >= numcolliders)
        return true;

    while (mcount0 < numcolliders)
    {
        renderColliders_[mcount0].UpdateRenderShape();

        mcount0++;

        if (timer)
            return false;
    }

    return mcount0 >= numcolliders;
}

// Update All Colliders (all views)
bool MapBase::UpdateAllColliders(HiresTimer* timer)
{
    /// TODO Async
    if (!UpdatePhysicColliders(timer))
        return false;

    if (GameContext::Get().gameConfig_.renderShapes_)
        if (!UpdateRenderColliders(timer))
            return false;

    return true;
}



/// Shape Setters

bool MapBase::SetCollisionShapes(HiresTimer* timer)
{
    int& mcount0 = GetMapCounter(MAP_GENERAL);
    int& mcount1 = GetMapCounter(MAP_FUNC1);

    if (mcount0 == 0)
    {
//        URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ...");

        GetMapCounter(MAP_FUNC2) = GetMapCounter(MAP_FUNC3) = GetMapCounter(MAP_FUNC4) = GetMapCounter(MAP_FUNC5) = 0;

        RigidBody2D* body = nodeStatic_->GetComponent<RigidBody2D>(LOCAL);
        if (!body)
            body = nodeStatic_->CreateComponent<RigidBody2D>(LOCAL);

        body->SetBodyType(colliderBodyType_);
        body->SetUseFixtureMass(false);
        body->SetMass(colliderBodyMass_);
        body->SetMassCenter(Vector2::ZERO);

        if (colliderBodyRotate_)
            body->SetInertia(100000.f);
        body->SetFixedRotation(!colliderBodyRotate_);

        // in dynamic, walls need better simulation otherwise bodies can pass through a wall => so, set bullet to active Box2D CCD (continuous collision detection)
        if (colliderBodyType_ == BT_DYNAMIC)
        {
//            body->SetAllowSleep(false);
            body->SetBullet(true);
        }

        OnPhysicsSetted();

        mcount1 = 0;
        mcount0++;
    }
    if (mcount0 == 1)
    {
//        URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... AddCollisionBox2D ...");
        if (AddCollisionBox2D(timer))
        {
            mcount1 = 0;
            mcount0++;
            URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... AddCollisionBox2D ... OK !");
        }
        if (timer)
            return false;
    }
    if (mcount0 == 2)
    {
//        URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... AddCollisionChain2D ...");
        if (AddCollisionChain2D(timer))
        {
            mcount1 = 0;
            mcount0++;
            URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... AddCollisionChain2D ... OK !");
        }
        if (timer)
            return false;
    }
    if (mcount0 == 3)
    {
//        URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... AddCollisionEdge2D ...");
        if (AddCollisionEdge2D(timer))
        {
            mcount1 = 0;
            mcount0++;
            URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... AddCollisionEdge2D ... OK !");
        }
        if (timer)
            return false;
    }
    if (mcount0 == 4)
    {
        URHO3D_LOGINFOF("MapBase() - SetCollisionShapes ... OK !");
        return true;
    }

    return false;
}

bool MapBase::AddCollisionBox2D(HiresTimer* timer)
{
    int& mcount1 = GetMapCounter(MAP_FUNC1);
    int& mcount2 = GetMapCounter(MAP_FUNC2);
    int& mcount3 = GetMapCounter(MAP_FUNC3);

    if (mcount1 == 0)
    {
        mcount2 = 0;

        int numViewColliders = colliderNumParams_[PHYSICCOLLIDERTYPE];

        nodeBoxes_.Resize(numViewColliders);
#ifdef GENERATE_PLATEFORMCOLLIDERS
        nodePlateforms_.Resize(numViewColliders);
#endif
        for (;;)
        {
            if (mcount2 >= numViewColliders)
                break;

            nodeBoxes_[mcount2] = 0;

            PhysicCollider& collider = physicColliders_[mcount2];

            if (collider.numShapes_[SHT_BOX])
            {
                nodeBoxes_[mcount2] = GetStaticNode()->CreateChild("Boxes", LOCAL);
            }
#ifdef GENERATE_PLATEFORMCOLLIDERS
            if (collider.plateforms_.Size())
            {
                nodePlateforms_[mcount2] = GetStaticNode()->CreateChild("Plateforms", LOCAL);
            }
#endif
            mcount2++;
        }

        mcount3 = 0;
        mcount2 = 0;
        mcount1++;

//        URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : ... timer=%d msec ... Init OK ...", timer ? timer->GetUSec(false)/1000 : 0);

#ifdef DUMP_ERROR_ON_TIMEOVER
        if (timer)
            LogTimeOver(ToString("MapBase() - AddCollisionBox2D : map=%s", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount1 > 0)
    {
        int numViewColliders = colliderNumParams_[PHYSICCOLLIDERTYPE];

        if (mcount2 == 0)
        {
            for (;;)
            {
                if (mcount3 >= numViewColliders)
                    break;

                // add collision boxes
                if (!nodeBoxes_[mcount3])
                {
                    mcount1 = 1;
                    mcount3++;
                    continue;
                }

                PhysicCollider& collider = physicColliders_[mcount3];

                HashMap<unsigned, CollisionBox2D* >::Iterator it = collider.blocks_.Begin();
                unsigned i = 0;
                for (;;)
                {
                    // seek to mcount1-1
                    if (i < mcount1-1)
                    {
                        i++;
                        it++;
                        continue;
                    }

                    if (it == collider.blocks_.End())
                    {
//                        URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : indexView=%d => %u CollisionBoxes created ... timer=%d msec ... OK !",
//                                        mcount3, i, timer ? timer->GetUSec(false)/1000 : 0);
                        mcount1 = 1;
                        mcount3++;
                        break;
                    }

                    CollisionBox2D* collisionBox = nodeBoxes_[mcount3]->CreateComponent<CollisionBox2D>(LOCAL);
                    it->second_ = collisionBox;
                    collisionBox->SetCenter(Vector2(2.f * GetTileCoordX(it->first_) + 1.f, 2.f * (GetHeight() - GetTileCoordY(it->first_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
                    collisionBox->SetSize(1.85f * MapInfo::info.tileHalfSize_);
                    collisionBox->SetFriction(0.3f);
                    collisionBox->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
                    collisionBox->SetColliderInfo(&collider);
                    collisionBox->SetViewZ(collider.params_->colliderz_);
                    // No collision if other shape has the same negative Group index
                    collisionBox->SetGroupIndex(colliderGroupIndex_);
//                    collider.shapesNormals_ += Pair< const void * , const PODVector<Vector2> * >((const void *)collisionBox,
//                                                    &(collider.contourNormals_[collider.normalIndex_[SHT_BOX] + i]));
//
                    it++;
                    i++;

                    collisionBox->SetEnabled(false);

#ifdef DUMP_ERROR_ON_TIMEOVER
                    if (timer)
                        LogTimeOver(ToString("MapBase() - AddCollisionBox2D : map=%s", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

                    if (TimeOver(timer))
                    {
                        mcount1 = i + 1;
                        return false;
                    }
                }
            }
            mcount2++;
            mcount3 = 0;
            mcount1 = 1;
//            URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : ... timer=%d msec ... Step 1 OK ...", timer ? timer->GetUSec(false)/1000 : 0);
        }
        if (mcount2 == 1)
        {
#ifdef GENERATE_PLATEFORMCOLLIDERS
            for (;;)
            {
                if (mcount3 >= numViewColliders)
                    break;

                // add collision plateforms
                if (!nodePlateforms_[mcount3])
                {
                    mcount1 = 1;
                    mcount3++;
                    continue;
                }

                PhysicCollider& collider = physicColliders_[mcount3];

                HashMap<unsigned, Plateform* >& plateforms = collider.plateforms_;
                HashMap<unsigned, Plateform* >::Iterator it = plateforms.Begin();
                unsigned i = 0;
                for (;;)
                {
                    // seek to mcount1-1
                    if (i < mcount1-1)
                    {
                        i++;
                        it++;
                        continue;
                    }

                    if (it == plateforms.End())
                    {
//                        URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : indexView=%d => %u CollisionBoxes created ... timer=%d msec ... OK !",
//                                        mcount3, i, timer ? timer->GetUSec(false)/1000 : 0);
                        mcount1 = 1;
                        mcount3++;
                        break;
                    }

                    if (it->second_->box_ == 0)
                    {
                        Plateform* plateform = it->second_;
                        CollisionBox2D* collisionBox = plateform->box_ = nodePlateforms_[mcount3]->CreateComponent<CollisionBox2D>(LOCAL);
                        collisionBox->SetCenter(Vector2(2.f * GetTileCoordX(plateform->tileleft_) + (float)plateform->size_, 2.f * (GetHeight() - GetTileCoordY(plateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
                        collisionBox->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * plateform->size_, 2.f * MapInfo::info.tileHalfSize_.y_));
                        collisionBox->SetFriction(0.3f);
                        collisionBox->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
                        collisionBox->SetColliderInfo(&collider);
                        collisionBox->SetViewZ(collider.params_->colliderz_-1);
                        collisionBox->SetGroupIndex(colliderGroupIndex_);
                        collisionBox->SetEnabled(false);
//                        URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : Adding a Plateform collisionbox at tileindex=%u plateformtileorigin=%u", it->first_, plateform->tileleft_);
                    }

                    it++;
                    i++;

#ifdef DUMP_ERROR_ON_TIMEOVER
                    if (timer)
                        LogTimeOver(ToString("MapBase() - AddCollisionBox2D : map=%s", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

                    if (TimeOver(timer))
                    {
                        mcount1 = i + 1;
                        return false;
                    }
                }
            }
#endif
//            URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : ... timer=%d msec ... Step 2 OK ...", timer ? timer->GetUSec(false)/1000 : 0);
        }

        mcount3 = 0;

        URHO3D_LOGINFOF("MapBase() - AddCollisionBox2D : ... timer=%d msec ... OK !", timer ? timer->GetUSec(false)/1000 : 0);

        return true;
    }

    return false;
}

static unsigned colliderNumVertices_;

bool MapBase::AddCollisionChain2D(HiresTimer* timer)
{
    int& mcount1 = GetMapCounter(MAP_FUNC1);
    int& mcount2 = GetMapCounter(MAP_FUNC2);
    int& mcount3 = GetMapCounter(MAP_FUNC3);
    int& mcount4 = GetMapCounter(MAP_FUNC4);
    int& mcount5 = GetMapCounter(MAP_FUNC5);

    if (mcount1 == 0)
    {
//        URHO3D_PROFILE(Map_SetCShape1);
        int numViewColliders = colliderNumParams_[PHYSICCOLLIDERTYPE];

        nodeChains_.Resize(numViewColliders);

        // Set Chain Nodes & Components
        for (unsigned i = 0; i < numViewColliders; i++)
        {
            Node*& chainnode = nodeChains_[i];
            chainnode = 0;

            PhysicCollider& physicCollider = physicColliders_[i];
            physicCollider.chains_.Clear();
            physicCollider.holes_.Clear();

            if (physicCollider.numShapes_[SHT_CHAIN])
            {
                chainnode = GetStaticNode()->CreateChild("Chains", LOCAL);
            }
        }

        colliderNumVertices_ = 0;
        mcount2 = mcount3 = mcount4 = mcount5 = 0;
        mcount1++;

//        URHO3D_LOGINFOF("MapBase() - AddCollisionChain2D : ... timer=%d msec ... Init OK ...", timer ? timer->GetUSec(false)/1000 : 0);

#ifdef DUMP_ERROR_ON_TIMEOVER
        if (timer)
            LogTimeOver(ToString("MapBase() - AddCollisionChain2D : map=%s", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount1 == 1)
    {
//        URHO3D_PROFILE(Map_SetCShape2);

        int numViewColliders = colliderNumParams_[PHYSICCOLLIDERTYPE];

        for (;;)
        {
            if (mcount2 >= numViewColliders)
                break;

            /// Test Only innerview & frontview
//            if (mcount2 != 2)
//            {
//                mcount2++;
//                continue;
//            }

            if (!nodeChains_[mcount2])
            {
                mcount3 = mcount4 = mcount5 = 0;
                mcount2++;
                continue;
            }

            Node*& chainnode = nodeChains_[mcount2];
            PhysicCollider& physicCollider = physicColliders_[mcount2];

            // Add each contour as CollisionChain
            const Vector< PODVector<Vector2> >& contourVertices = physicCollider.contourVertices_;
            if (mcount3 < contourVertices.Size())
            {
                unsigned i = mcount3;

                Vector<PODVector<Vector2> >::ConstIterator it = contourVertices.Begin() + i;
                for (;;)
                {
                    if (it == contourVertices.End())
                    {
//                        URHO3D_LOGINFOF("MapBase() - AddCollisionChain2D : index=%d => %u CollisionChains created ! %u vertices ... timer=%d msec ... OK !",
//                                        mcount2, i, colliderNumVertices_, timer ? timer->GetUSec(false) /1000 : 0);
                        break;
                    }

                    colliderNumVertices_ += it->Size();
                    CollisionChain2D* collisionChain = chainnode->CreateComponent<CollisionChain2D>(LOCAL);
                    collisionChain->SetLoop(true);

//                    GameHelpers::DumpVertices(*it);

                    collisionChain->SetVertices(*it);
                    collisionChain->SetFriction(0.3f);
                    collisionChain->SetFilterBits(physicCollider.params_->bits1_, physicCollider.params_->bits2_);
                    collisionChain->SetColliderInfo(&physicCollider);
                    collisionChain->SetViewZ(physicCollider.params_->colliderz_);
                    // No collision if other shape has the same negative Group index
                    collisionChain->SetGroupIndex(colliderGroupIndex_);

//                    URHO3D_LOGINFOF("MapBase() - AddCollisionChain2D : indz=%d indv=%d contourID=%c => create CollisionChain (id=%u ptr=%u numvertices=%u) with %u vertices on nodechain=%u catbits=%u maskbits=%u",
//                                     physicCollider.params_->indz_, physicCollider.params_->indv_, char(65+i), collisionChain->GetID(), collisionChain, it->Size(), collisionChain->GetVertexCount(),
//                                     chainnode->GetID(), collisionChain->GetCategoryBits(), collisionChain->GetMaskBits());

                    physicCollider.chains_.Push(collisionChain);

//                    physicCollider.shapesNormals_ += Pair< const void * , const PODVector<Vector2> * >((const void *)collisionChain,
//                                                    &(physicCollider.contourNormals_[physicCollider.normalIndex_[SHT_CHAIN]+i]));
                    it++;
                    i++;

                    collisionChain->SetEnabled(false);

#ifdef DUMP_ERROR_ON_TIMEOVER
                    if (timer)
                        LogTimeOver(ToString("MapBase() - AddCollisionChain2D : map=%s", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

                    if (TimeOver(timer))
                    {
                        mcount3 = i;
                        return false;
                    }
                }
            }

#ifdef MAP_ADDHOLECOLLISIONCHAINS
            // For each contour, Add the Holes as CollisionChains
            const Vector<Vector<PODVector<Vector2> > >& contourHoleVertices = physicCollider.holeVertices_;
            if (mcount4 < contourHoleVertices.Size())
            {
                unsigned i = mcount4;

                Vector<Vector<PODVector<Vector2> > >::ConstIterator it = contourHoleVertices.Begin() + i;
                for (;;)
                {
                    if (it == contourHoleVertices.End())
                        break;

                    if (!it->Size())
                    {
                        it++;
                        i++;
                        mcount5 = 0;
                        continue;
                    }

                    unsigned j = mcount5;
                    const Vector<PODVector<Vector2> >& holeVertices = (*it);
                    Vector<PODVector<Vector2> >::ConstIterator jt = holeVertices.Begin() + j;
                    for (;;)
                    {
                        if (jt == holeVertices.End())
                        {
                            it++;
                            i++;
                            break;
                        }

                        colliderNumVertices_ += jt->Size();
                        CollisionChain2D* collisionChain = chainnode->CreateComponent<CollisionChain2D>(LOCAL);
                        collisionChain->SetLoop(true);
                        collisionChain->SetVertices(*jt);
                        collisionChain->SetFriction(0.3f);
                        collisionChain->SetFilterBits(physicCollider.params_->bits1_, physicCollider.params_->bits2_);
                        collisionChain->SetColliderInfo(&physicCollider);
                        collisionChain->SetViewZ(physicCollider.params_->colliderz_);
                        // No collision if other shape has the same negative Group index
                        collisionChain->SetGroupIndex(colliderGroupIndex_);

//                        URHO3D_LOGINFOF("MapBase() - AddCollisionChain2D : indz=%d indv=%d contourID=%c HoleID=%u=> create CollisionChain (id=%u ptr=%u numvertices=%u) with %u vertices on nodechain=%u catbits=%u maskbits=%u",
//                                        physicCollider.params_->indz_, physicCollider.params_->indv_, char(65+i), j, collisionChain->GetID(), collisionChain, it->Size(), collisionChain->GetVertexCount(),
//                                        chainnode->GetID(), collisionChain->GetCategoryBits(), collisionChain->GetMaskBits());

                        physicCollider.holes_.Push(collisionChain);

                        jt++;
                        j++;

                        collisionChain->SetEnabled(false);

#ifdef DUMP_ERROR_ON_TIMEOVER
                        if (timer)
                            LogTimeOver(ToString("MapBase() - AddCollisionChain2D : map=%s", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

                        if (TimeOver(timer))
                        {
                            mcount4 = i;
                            mcount5 = j;
                            return false;
                        }
                    }

                    mcount4 = i;
                    mcount5 = 0;
                }
            }
#endif

            mcount3 = mcount4 = mcount5 = 0;
            mcount2++;
        }

        URHO3D_LOGINFOF("MapBase() - AddCollisionChain2D : ... timer=%d msec ... OK !", timer ? timer->GetUSec(false)/1000 : 0);
        mcount2 = 0;
        return true;
    }

    return false;
}

bool MapBase::SetCollisionChain2D(PhysicCollider& physicCollider, HiresTimer* timer)
{
    if (!physicCollider.numShapes_[SHT_CHAIN])
        return true;

    const Vector< PODVector<Vector2> >& contourVertices = physicCollider.contourVertices_;

    if (!contourVertices.Size())
        return true;

    Node*& chainnode = nodeChains_[physicCollider.id_];
    int& mcount2 = GetMapCounter(MAP_FUNC2);
    int& mcount3 = GetMapCounter(MAP_FUNC3);
#ifdef MAP_ADDHOLECOLLISIONCHAINS
    int& mcount4 = GetMapCounter(MAP_FUNC4);
    int& mcount5 = GetMapCounter(MAP_FUNC5);
#endif

    if (mcount2 == 0)
    {
        if (!chainnode)
        {
            chainnode = GetStaticNode()->CreateChild("Chains", LOCAL);
            if (!chainnode)
            {
                URHO3D_LOGERRORF("MapBase() - SetCollisionChain2D : colliderid=%d => Can't create Chain Node !", physicCollider.id_);
                return true;
            }
        }

        physicCollider.chains_.Clear();
        physicCollider.holes_.Clear();

        mcount2 = 1;
        mcount3 = 0;

#ifdef MAP_ADDHOLECOLLISIONCHAINS
        mcount4 = 0;
        mcount5 = 0;
#endif
    }

    // Add each contour as CollisionChain
    if (mcount3 < contourVertices.Size())
    {
        unsigned i = mcount3;

        Vector<PODVector<Vector2> >::ConstIterator it = contourVertices.Begin() + i;
        for (;;)
        {
            if (it == contourVertices.End())
            {
//                URHO3D_LOGINFOF("MapBase() - SetCollisionChain2D : colliderid=%d => %u CollisionChains created ! %u vertices ... timer=%d msec ... OK !",
//                                physicCollider.id_, i, colliderNumVertices_, timer ? timer->GetUSec(false) /1000 : 0);
                break;
            }

            colliderNumVertices_ += it->Size();
            CollisionChain2D* collisionChain = chainnode->CreateComponent<CollisionChain2D>(LOCAL);
            collisionChain->SetLoop(true);
            collisionChain->SetVertices(*it);
            collisionChain->SetFriction(0.3f);
            collisionChain->SetFilterBits(physicCollider.params_->bits1_, physicCollider.params_->bits2_);
            collisionChain->SetColliderInfo(&physicCollider);
            collisionChain->SetViewZ(physicCollider.params_->colliderz_);
            // No collision if other shape has the same negative Group index
            collisionChain->SetGroupIndex(colliderGroupIndex_);

//            URHO3D_LOGINFOF("MapBase() - SetCollisionChain2D : indz=%d indv=%d contourID=%c => create CollisionChain (id=%u ptr=%u numvertices=%u) with %u vertices on nodechain=%u catbits=%u maskbits=%u",
//                            physicCollider.params_->indz_, physicCollider.params_->indv_, char(65+i), collisionChain->GetID(), collisionChain, it->Size(), collisionChain->GetVertexCount(),
//                            chainnode->GetID(), collisionChain->GetCategoryBits(), collisionChain->GetMaskBits());

            physicCollider.chains_.Push(collisionChain);

//            physicCollider.shapesNormals_ += Pair< const void * , const PODVector<Vector2> * >((const void *)collisionChain,
//                                                &(physicCollider.contourNormals_[physicCollider.normalIndex_[SHT_CHAIN]+i]));
            it++;
            i++;

//            collisionChain->SetEnabled(false);

#ifdef DUMP_ERROR_ON_TIMEOVER
            if (timer)
                LogTimeOver(ToString("MapBase() - SetCollisionChain2D : map=%s ... colliderid=%d", GetMapPoint().ToString().CString(), physicCollider.id_), timer, delayUpdateUsec_);
#endif

            if (TimeOver(timer))
            {
                mcount3 = i;
                return false;
            }
        }
    }

#ifdef MAP_ADDHOLECOLLISIONCHAINS
    // For each contour, Add the Holes as CollisionChains
    const Vector<Vector<PODVector<Vector2> > >& contourHoleVertices = physicCollider.holeVertices_;
    if (mcount4 < contourHoleVertices.Size())
    {
        unsigned i = mcount4;

        Vector<Vector<PODVector<Vector2> > >::ConstIterator it = contourHoleVertices.Begin() + i;
        for (;;)
        {
            if (it == contourHoleVertices.End())
                break;

            if (!it->Size())
            {
                it++;
                i++;
                mcount5 = 0;
                continue;
            }

            unsigned j = mcount5;
            const Vector<PODVector<Vector2> >& holeVertices = (*it);
            Vector<PODVector<Vector2> >::ConstIterator jt = holeVertices.Begin() + j;
            for (;;)
            {
                if (jt == holeVertices.End())
                {
                    it++;
                    i++;
                    break;
                }

                colliderNumVertices_ += jt->Size();
                CollisionChain2D* collisionChain = chainnode->CreateComponent<CollisionChain2D>(LOCAL);
                collisionChain->SetLoop(true);
                collisionChain->SetVertices(*jt);
                collisionChain->SetFriction(0.3f);
                collisionChain->SetFilterBits(physicCollider.params_->bits1_, physicCollider.params_->bits2_);
                collisionChain->SetColliderInfo(&physicCollider);
                collisionChain->SetViewZ(physicCollider.params_->colliderz_);
                // No collision if other shape has the same negative Group index
                collisionChain->SetGroupIndex(colliderGroupIndex_);

//                URHO3D_LOGINFOF("MapBase() - SetCollisionChain2D : indz=%d indv=%d contourID=%c HoleID=%u=> create CollisionChain (id=%u ptr=%u numvertices=%u) with %u vertices on nodechain=%u catbits=%u maskbits=%u",
//                                physicCollider.params_->indz_, physicCollider.params_->indv_, char(65+i), j, collisionChain->GetID(), collisionChain, it->Size(), collisionChain->GetVertexCount(),
//                                chainnode->GetID(), collisionChain->GetCategoryBits(), collisionChain->GetMaskBits());

                physicCollider.holes_.Push(collisionChain);

                jt++;
                j++;

//                collisionChain->SetEnabled(false);

#ifdef DUMP_ERROR_ON_TIMEOVER
                if (timer)
                    LogTimeOver(ToString("MapBase() - SetCollisionChain2D : map=%s ... colliderid=%d", GetMapPoint().ToString().CString(), physicCollider.id_), timer, delayUpdateUsec_);
#endif

                if (TimeOver(timer))
                {
                    mcount4 = i;
                    mcount5 = j;
                    return false;
                }
            }

            mcount4 = i;
            mcount5 = 0;
        }
    }

    mcount4 = 0;
    mcount5 = 0;
#endif

    mcount2 = 0;
    mcount3 = 0;

    return true;
}

bool MapBase::AddCollisionEdge2D(HiresTimer* timer)
{
//    URHO3D_LOGINFOF("MapBase() - AddCollisionEdge2D ... NOT IMPLEMENTED");
    return true;
}

#define UPDATECOLLIDERCHAIN 1

// method1 : Update All Chains
#if !defined(UPDATECOLLIDERCHAIN) || (UPDATECOLLIDERCHAIN == 1)
static PODVector<unsigned char> sUpdatedContourIds_;

bool MapBase::UpdateCollisionChain(PhysicCollider& collider, unsigned tileindex)
{
//    URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain ...");

    unsigned char newcontourid = collider.contourIds_[tileindex];

    if (newcontourid > 0 && sLastContourId_ == newcontourid)
    {
//        URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : colliderindex=%d sLastContourId_=%u already updated at tileindex=%u !", icollider, sLastContourId_, tileindex);
        return false;
    }

    sUpdatedContourIds_.Clear();

    // Remove a tile
    if (newcontourid == 0 && sLastContourId_ > 0)
    {
        if (sLastContourId_-1 >= collider.chains_.Size())
        {
            URHO3D_LOGERRORF("MapBase() - UpdateCollisionChain : mPoint=%s at %s tileindex=%u sLastContourId_=%c > chains size(%u)",
                             GetMapGeneratorStatus().mappoint_.ToString().CString(), GetTileCoords(tileindex).ToString().CString(), tileindex, (char)(65+sLastContourId_-1), collider.chains_.Size());
        }

        List<void*>::Iterator it = collider.chains_.GetIteratorAt(sLastContourId_-1);
        if (it != collider.chains_.End() && *it != 0)
        {
            CollisionChain2D* collisionChain = (CollisionChain2D*)(*it);

            // Send Event (for node hanging on the tile)
#ifdef DUMP_MAPDEBUG_SETTILE
            URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : mPoint=%s sLastContourId_=%c cs=%u SendEvent MAPTILEREMOVED at %u ...",
                            GetMapGeneratorStatus().mappoint_.ToString().CString(), (char)(65+sLastContourId_-1), collisionChain, tileindex);
#endif
            VariantMap& eventData = collisionChain->GetContext()->GetEventDataMap();
            eventData[MapTileRemoved::MAPPOINT] = GetMapPoint().ToHash();
            eventData[MapTileRemoved::MAPTILEINDEX] = tileindex;
            collisionChain->SendEvent(MAPTILEREMOVED, eventData);
        }
    }

    // Get the collisionChains in the viewCollider node "nodeChains_[colliderid]"
    Node*& nodeChains = nodeChains_[collider.id_];

    int deltaContours = (int)collider.numShapes_[SHT_CHAIN] - (int)collider.chains_.Size();

    // Add chains
    if (deltaContours > 0)
    {
        // no node, create it
        if (!nodeChains)
        {
            nodeChains = GetStaticNode()->CreateChild("Chains", LOCAL);
        }

        // add collisionchains
        if (nodeChains)
        {
            for (int i=0; i < deltaContours; i++)
            {
                unsigned char contourid = collider.chains_.Size();

                CollisionChain2D* collisionChain = nodeChains->CreateComponent<CollisionChain2D>(LOCAL);
                collisionChain->SetLoop(true);
                collisionChain->SetFriction(0.3f);
                collisionChain->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
                collisionChain->SetGroupIndex(colliderGroupIndex_);
                collisionChain->SetVertices(collider.contourVertices_[contourid]);
                collisionChain->SetColliderInfo(&collider);
                collisionChain->SetViewZ(collider.params_->colliderz_);
                collider.chains_.Push(collisionChain);
                sUpdatedContourIds_.Push(contourid);

//                URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid=%c add chain on Node=%s(%u) csID=%u ptr=%u numvertices=%u at viewZ=%d !",
//                                char(65+contourid), nodeChains->GetName().CString(), nodeChains->GetID(),
//                                collisionChain->GetID(), collisionChain, collisionChain->GetVertexCount(), collider.params_->colliderz_);
            }
        }
    }

    // Modify chain vertices
    {
        // adjust newcontourid (if null => UPDATE all vertices)
        if (newcontourid > 0) newcontourid--;

        // the update begins at the newcontourid
        for (unsigned contourid = newcontourid; contourid < collider.numShapes_[SHT_CHAIN]; ++contourid)
        {
            if (sUpdatedContourIds_.Contains(contourid))
                continue;

            // modify contour vertices
            List<void*>::Iterator it = collider.chains_.GetIteratorAt(contourid);
            CollisionChain2D* collisionChain = 0;
            if (it != collider.chains_.End() && *it != 0)
            {
                collisionChain = (CollisionChain2D*)(*it);
            }
            else
            {
                URHO3D_LOGERRORF("MapBase() - UpdateCollisionChain : contourid=%c Empty CollisionChain Ptr !", char(65+contourid));
                continue;
            }

            collisionChain->SetVertices(collider.contourVertices_[contourid]);

//            URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid=%c modify vertices csID=%u ptr=%u numvertices=%u !",
//                            char(65+contourid), collisionChain->GetID(), collisionChain, collisionChain->GetVertexCount());
        }
    }

    // Remove chains
    if (deltaContours < 0)
    {
        //deltaContours = -deltaContours;
        for (int i=0; i < -deltaContours; i++)
        {
            CollisionChain2D* collisionChain = (CollisionChain2D*)(collider.chains_.Back());

//            URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid=%c remove chain csID=%u ptr=%u !",
//                            char(65+collider.chains_.Size()-1), collisionChain->GetID(), collisionChain);

            if (collisionChain)
                collisionChain->Remove();

            collider.chains_.Pop();
        }
    }

//    URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : Contours Updated ... OK !");

    // Update Holes

#ifdef MAP_ADDHOLECOLLISIONCHAINS
    // calculate num holes
    unsigned numholes = 0;
    for (Vector<Vector<PODVector<Vector2> > >::ConstIterator it = collider.holeVertices_.Begin(); it != collider.holeVertices_.End(); ++it)
        numholes += it->Size();

    int deltaHoles = (int)numholes - (int)collider.holes_.Size();

    // Add Holes Collisionchains
    if (deltaHoles > 0)
    {
        for (int i = 0; i < deltaHoles; i++)
        {
            CollisionChain2D* collisionChain = nodeChains->CreateComponent<CollisionChain2D>(LOCAL);
            collisionChain->SetLoop(true);
            collisionChain->SetFriction(0.3f);
            collisionChain->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
            collisionChain->SetGroupIndex(colliderGroupIndex_);
            collisionChain->SetColliderInfo(&collider);
            collisionChain->SetViewZ(collider.params_->colliderz_);
            collider.holes_.Push(collisionChain);
        }
    }

    // Remove Holes Collisionchains
    else if (deltaHoles < 0)
    {
        deltaHoles = -deltaHoles;
        for (int i = 0; i < deltaHoles; i++)
        {
            CollisionChain2D* collisionChain = (CollisionChain2D*)(collider.holes_.Back());
            if (collisionChain)
                collisionChain->Remove();

            collider.holes_.Pop();
        }
    }

    // Modify All Holes Collisionchains Vertices
    if (collider.holes_.Size())
    {
        unsigned index = 0;
        for (Vector<Vector<PODVector<Vector2> > >::ConstIterator it = collider.holeVertices_.Begin(); it != collider.holeVertices_.End(); ++it)
        {
            const Vector<PODVector<Vector2> >& holevertices = *it;

            if (!holevertices.Size())
                continue;

            for (Vector<PODVector<Vector2> >::ConstIterator jt = holevertices.Begin(); jt != holevertices.End(); ++jt, ++index)
            {
                List<void*>::Iterator kt = collider.holes_.GetIteratorAt(index);
                CollisionChain2D* collisionChain = 0;
                if (kt != collider.chains_.End() && *kt != 0)
                    collisionChain = (CollisionChain2D*)(*kt);

                if (collisionChain)
                    collisionChain->SetVertices(*jt);
            }
        }
    }
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : Holes Updated ... OK !");
#endif
#endif

    return true;
}
#endif

// method2 : Reorder chain list to keep same collisionshapes ptr after change
#if !defined(UPDATECOLLIDERCHAIN) || (UPDATECOLLIDERCHAIN == 2)
static unsigned sLastNumChains_;
static PODVector<unsigned char> sLastContourIds_;
static PODVector<unsigned> sNghindexes_;
static PODVector<unsigned char> sUpdatedContourIds_;

void MapBase::UpdateCollider(MapCollider* mapCollider, int x, int y)
{
    if (!mapCollider || !mapCollider->contourIds_.Size() || mapCollider->viewZ_ == -1)
        return;

    // Save last chain infos before new trace (used by UpdateCollisionChain)
    sLastNumChains_ = mapCollider->numShapes_[SHT_CHAIN];
    sLastContourIds_ = mapCollider->contourIds_;

    // Trace new contours
    mapColliderGenerator_->UpdateCollider(this, mapCollider);

    UpdateCollisionChain(mapCollider, x, y);

    // Update RenderShape
    //mapCollider->UpdateRenderShapes();
}

bool MapBase::UpdateCollisionChain(MapCollider* mapCollider, int x, int y)
{
    unsigned tileindex = GetTileIndex(x, y);
    assert(tileindex < mapCollider->contourIds_.Size());

    unsigned char lastcontourid = sLastContourIds_[tileindex];
    unsigned char newcontourid  = mapCollider->contourIds_[tileindex];

    if (lastcontourid == newcontourid)
        return false;

    // Remove a tile
    if (newcontourid == 0 && lastcontourid > 0)
    {
        if (lastcontourid >= mapCollider->chains_.Size())
            URHO3D_LOGERRORF("MapBase() - UpdateCollisionChain : mPoint=%s lastcontourid=%c > chains size(%u)", GetMapPoint().ToString().CString(), (char)(65+lastcontourid-1), mapCollider->chains_.Size());

        Iterator it = collider.chains_.GetIteratorAt(lastcontourid-1);
        if (it != collider.chains_.End() && *it != 0)
        {
            CollisionChain2D* collisionChain = (CollisionChain2D*)(*it);

            // Send Event (for node hanging on the tile)
            URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : mPoint=%s lastcontourid=%c cs=%u SendEvent MAPTILEREMOVED at %u ...", GetMapPoint().ToString().CString(), (char)(65+lastcontourid-1), collisionChain, tileindex);

            VariantMap& eventData = context_->GetEventDataMap();
            eventData[MapTileRemoved::MAPPOINT] = GetMapPoint().ToHash();
            eventData[MapTileRemoved::MAPTILEINDEX] = tileindex;
            collisionChain->SendEvent(MAPTILEREMOVED, eventData);
        }
    }

    // Get the collisionChains in the viewCollider node "nodeChains_[colliderid]"
    const Vector<int>& viewIds = GetViewIDs(mapCollider->params_->colliderz_);
    Node*& nodeChains = nodeChains_[mapCollider->indz_*MAP_NUMMAXCOLLIDERS + viewIds[mapCollider->indv_]];

    int deltaNumChains = (int)mapCollider->numShapes_[SHT_CHAIN] - (int)mapCollider->chains_.Size();

    // Add chains
    if (deltaNumChains > 0)
    {
        // Create node Chains
        if (!nodeChains)
        {
            nodeChains = GetStaticNode()->CreateChild("Chains", LOCAL);
        }

        if (nodeChains)
            // Add Chains
        {
            for (int i=0; i < deltaNumChains; i++)
            {
                CollisionChain2D* collisionChain = nodeChains->CreateComponent<CollisionChain2D>(LOCAL);
                collisionChain->SetLoop(true);
                collisionChain->SetFriction(0.3f);
                if (mapCollider->params_->colliderz_ == INNERVIEW || mapCollider->params_->colliderz_ == BACKVIEW)
                    collisionChain->SetFilterBits(CC_INSIDEWALL, CM_INSIDEWALL);
                else
                    collisionChain->SetFilterBits(CC_OUTSIDEWALL, CM_OUTSIDEWALL);
                collisionChain->SetColliderInfo(mapCollider);
                collisionChain->SetViewZ(mapCollider->params_->colliderz_);
                // No collision if other shape has the same negative Group index
                collisionChain->SetGroupIndex(colliderGroupIndex_);

                mapCollider->chains_.Push(collisionChain);

                URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : add chain !");
            }
        }
    }
    // Reorder Chains List
    {
        sNghindexes_.Clear();
        sUpdatedContourIds_.Clear();

        // for the updated tile
        sNghindexes_.Push(tileindex);
        // and for its cardinal neighbors
        if (x > 0) sNghindexes_.Push(tileindex - 1); // ngh(x-1, y)
        if (x+1 < MapInfo::info.width_) sNghindexes_.Push(tileindex + 1); // ngh(x+1, y)
        if (y+1 < MapInfo::info.height_) sNghindexes_.Push(tileindex + MapInfo::info.width_); // ngh(x, y+1)
        if (y > 0) sNghindexes_.Push(tileindex - MapInfo::info.width_); // ngh(x, y-1)

        // move the collisionchain from lastnghcontourid to newnghcontourid
        for (int i=0; i < sNghindexes_.Size(); i++)
        {
            unsigned char lastnghcontourid = sLastContourIds_[sNghindexes_[i]];
            unsigned char newnghcontourid = mapCollider->contourIds_[sNghindexes_[i]];

            if (newnghcontourid && lastnghcontourid && newnghcontourid != lastnghcontourid && !sUpdatedContourIds_.Contains(newnghcontourid))
            {
                if (lastnghcontourid >= mapCollider->chains_.Size())
                    URHO3D_LOGWARNINGF("MapBase() - UpdateCollisionChain : mPoint=%s reordering lastnghcontourid=%c > chains size(%u)", GetMapPoint().ToString().CString(), (char)(65+lastnghcontourid-1), mapCollider->chains_.Size());

                void* collisionChain = *(mapCollider->chains_.GetIteratorAt(lastnghcontourid-1));
                mapCollider->chains_.Erase(lastnghcontourid-1);
                mapCollider->chains_.Insert(newnghcontourid-1, collisionChain);
                sUpdatedContourIds_.Push(newnghcontourid);

                URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : reordering cs=%u contourid=%c->%c !", collisionChain, (char)(65+lastnghcontourid-1), (char)(65+newnghcontourid-1));
            }
        }
    }
    // Modify only the chain vertices in contact with x,y and the new created chains
    {
        // adjust newcontourid (if null => UPDATE all vertices)
        if (newcontourid > 0) newcontourid--;

        // the update begins at the newcontourid
        List<void*>::ConstIterator it = mapCollider->chains_.Begin();
        for (int i=0; i<newcontourid; i++)
            it++;
        for (unsigned contourid=newcontourid; contourid < mapCollider->numShapes_[SHT_CHAIN]; ++contourid)
        {
            // modify vertices
            CollisionChain2D* collisionChain = (CollisionChain2D*)(*it);
            it++;

            if (collisionChain)
            {
                collisionChain->SetVertices(mapCollider->contourVertices_[contourid]);
                URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid=%u modify vertices !", contourid+1);
            }
            else
            {
                URHO3D_LOGERRORF("MapBase() - UpdateCollisionChain : contourid=%u Empty CollisionChain Ptr !", contourid+1);
                continue;
            }
        }
        // set vertices for the new chains
        if (deltaNumChains > 0)
        {
            for (int i=1; i <= deltaNumChains; i++)
            {
                unsigned char contourid = mapCollider->chains_.Size()-i;
                CollisionChain2D* collisionChain = (CollisionChain2D*)(mapCollider->chains_[contourid]);
                collisionChain->SetVertices(mapCollider->contourVertices_[contourid]);
            }
        }
    }
    // Remove chains
    if (deltaNumChains < 0)
    {
        unsigned oldNumChains = mapCollider->chains_.Size();

        for (int contourid=oldNumChains; contourid > oldNumChains+deltaNumChains; contourid--)
        {
            CollisionChain2D* collisionChain = (CollisionChain2D*)(mapCollider->chains_.Back());
            if (collisionChain)
                collisionChain->Remove();
            mapCollider->chains_.Pop();
            URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid=%d remove chain !", contourid);
        }
    }

    return true;
}
#endif

// method3 : Modify just the chain at the point (x,y) and update if need the other chains (manage add/remove chains when splitting ...)
#if defined(UPDATECOLLIDERCHAIN) && (UPDATECOLLIDERCHAIN == 3)
static unsigned sLastNumChains_;
static PODVector<unsigned char> sLastContourIds_;
static PODVector<unsigned char> sUpdatedContourIds_;

void MapBase::UpdateCollider(int x, int y, MapCollider* mapCollider)
{
    if (!mapCollider || !mapCollider->contourIds_.Size() || mapCollider->params_->colliderz_ == -1)
        return;

    // Save last chain infos before new trace (used by UpdateCollisionChain)
    sLastNumChains_ = mapCollider->numShapes_[SHT_CHAIN];
    sLastContourIds_ = mapCollider->contourIds_;

    // Trace new contours
    /// TODO : only at (x,y) or in a range (x1,y1,x2,y2)
    mapColliderGenerator_->UpdateCollider(this, mapCollider);

    // Update Collision Chains
    sUpdatedContourIds_.Clear();
    UpdateCollisionChain(x, y, mapCollider);

    // Update Collision Chains In Neighborhood
    if (x+1 < MapInfo::info.width_)
    {
        UpdateCollisionChain(x+1, y, mapCollider);

        if (y+1 < MapInfo::info.height_)
        {
            UpdateCollisionChain(x,   y+1, mapCollider);
            UpdateCollisionChain(x+1, y+1, mapCollider);
        }
        if (y > 0)
        {
            UpdateCollisionChain(x,   y-1, mapCollider);
            UpdateCollisionChain(x+1, y-1, mapCollider);
        }
    }

    if (x > 0)
    {
        UpdateCollisionChain(x-1, y, mapCollider);

        if (y+1 < MapInfo::info.height_)
            UpdateCollisionChain(x-1, y+1, mapCollider);
        if (y > 0)
            UpdateCollisionChain(x-1, y-1, mapCollider);
    }

    // Update RenderShape
    //mapCollider->UpdateRenderShapes();
}

bool MapBase::UpdateCollisionChain(int x, int y, MapCollider* mapCollider)
{
    /// TODO : find the right way to track splitting and updating the good collisionchains
    unsigned tileindex = GetTileIndex(x, y);

    assert(tileindex < mapCollider->contourIds_.Size());

    unsigned char newcontourid  = mapCollider->contourIds_[tileindex];
    unsigned char lastcontourid = sLastContourIds_[tileindex];

    // already updated
    if (sUpdatedContourIds_.Contains(newcontourid))
        return false;

    // Get the collisionChains in the viewCollider node "nodeChains_[colliderid]"
    const Vector<int>& viewIds = GetViewIDs(mapCollider->params_->colliderz_);
    Node*& nodeChains = nodeChains_[mapCollider->indz_*MAP_NUMMAXCOLLIDERS + viewIds[mapCollider->indv_]];

    // collisionChains Adding/Removing
    int deltaNumChains = mapCollider->numShapes_[SHT_CHAIN] - mapCollider->chains_.Size();
    if (deltaNumChains != 0)
    {
        // no node, create it and body
        if (!nodeChains)
        {
            nodeChains = GetStaticNode()->CreateChild("Chains", LOCAL);
        }

        if (nodeChains)
        {
            if (deltaNumChains > 0)
            {
                // add new collisionchains
                for (int i=0; i < )
                {
                    CollisionChain2D* collisionChain = nodeChains->CreateComponent<CollisionChain2D>(LOCAL);
                    collisionChain->SetLoop(true);
                    collisionChain->SetFriction(0.3f);
                    if (mapCollider->params_->colliderz_ == INNERVIEW || mapCollider->params_->colliderz_ == BACKVIEW)
                        collisionChain->SetFilterBits(CC_INSIDEWALL, CM_INSIDEWALL);
                    else
                        collisionChain->SetFilterBits(CC_OUTSIDEWALL, CM_OUTSIDEWALL);
                    collisionChain->SetColliderInfo(mapCollider);
                    collisionChain->SetViewZ(mapCollider->params_->colliderz_);
                    // No collision if other shape has the same negative Group index
                    collisionChain->SetGroupIndex(colliderGroupIndex_);

                    mapCollider->chains_.Push(collisionChain);

                    URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : add chain !");
                }
            }
            else
            {
                // remove collisionchains

            }
        }
    }

    // update if difference of contourid
    if (newcontourid != lastcontourid)
    {
        if (newcontourid)
        {
            // modify vertices
            List<void*>::Iterator it = mapCollider->chains_.Begin();
            for (int i=1; i < newcontourid; i++) it++;
            CollisionChain2D* collisionChain = (CollisionChain2D*)(*it);
            if (collisionChain)
            {
                collisionChain->SetVertices(mapCollider->contourVertices_[newcontourid-1]);
                URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid last=%u new=%u at x=%d y=%d modify vertices !", lastcontourid, newcontourid, x, y);
            }
            else
            {
                URHO3D_LOGERROR("MapBase() - UpdateCollisionChain : Empty CollisionChain Ptr !");
                return;
            }

            // keep track of the updated chain
            sUpdatedContourIds_.Push(newcontourid);
        }
        // remove vertices
        else
        {
            // check if the old numvertices == 4
            List<void*>::Iterator it = mapCollider->chains_.Begin();
            for (int i=1; i < lastcontourid; i++) it++;
            CollisionChain2D* collisionChain = (CollisionChain2D*)(*it);
            if (collisionChain->GetVertexCount() == 4)
            {
                // no more vertices, just remove the chain
                collisionChain->Remove();
                mapCollider->chains_.Erase(it);

                URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid last=%u at x=%d y=%d remove chain !", lastcontourid, x, y);
            }
            else
            {
                if (collisionChain)
                {
                    // modify the existing chain
                    URHO3D_LOGINFOF("MapBase() - UpdateCollisionChain : contourid last=%u at x=%d y=%d decrease vertices !", lastcontourid, x, y);
                    collisionChain->SetVertices(mapCollider->contourVertices_[lastcontourid-1]);

                    // keep track of the updated chain
                    sUpdatedContourIds_.Push(lastcontourid);
                }
                else
                {
                    URHO3D_LOGERROR("MapBase() - UpdateCollisionChain : Empty CollisionChain Ptr !");
                    return false;
                }
            }
        }
    }

    return true;
}
#endif

bool MapBase::UpdateCollisionBox(PhysicCollider& collider, unsigned tileindex, bool add, bool addnode, bool dynamic)
{
    HashMap<unsigned, CollisionBox2D* >::Iterator it = collider.blocks_.Find(tileindex);

    // Add Block
    if (add && it == collider.blocks_.End())
    {
//        URHO3D_LOGINFOF("MapBase() - UpdateCollisionBoxes : at x=%d y=%d tileindex=%u ... adding wall ...",
//                        GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex);

        Vector2 center = Vector2(2.f * GetTileCoordX(tileindex) + 1.f, 2.f * (GetHeight() - GetTileCoordY(tileindex) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_;

        CollisionBox2D*& collisionBox = collider.blocks_[tileindex];

        if (addnode)
        {
            Node* node = GetRootNode()->CreateChild("Box", LOCAL);
            node->SetWorldScale2D(Vector2::ONE);
            node->SetPosition2D(center);
            RigidBody2D* body = node->CreateComponent<RigidBody2D>();
            if (dynamic)
                body->SetBodyType(BT_DYNAMIC);
            collisionBox = node->CreateComponent<CollisionBox2D>(LOCAL);
            collisionBox->SetSize(2.f * MapInfo::info.mTileHalfSize_);
        }
        else
        {
            Node*& boxesNode = nodeBoxes_[collider.id_];
            if (!boxesNode)
                boxesNode = GetStaticNode()->CreateChild("Boxes", LOCAL);
            collisionBox = boxesNode->CreateComponent<CollisionBox2D>(LOCAL);
            collisionBox->SetCenter(center);
            collisionBox->SetSize(2.f * MapInfo::info.tileHalfSize_);
        }

        collisionBox->SetFriction(0.3f);
        collisionBox->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
        collisionBox->SetColliderInfo(&collider);
        collisionBox->SetViewZ(collider.params_->colliderz_);
        collisionBox->SetGroupIndex(colliderGroupIndex_); // No collision if other shape has the same negative Group index

        URHO3D_LOGINFOF("MapBase() - UpdateCollisionBoxes : colliderid=%d ptr=%u ... add a block collider.blocks_[%u]=%u !",
                        collider.id_, &collider, tileindex, collider.blocks_[tileindex]);

        return true;
    }

    // Remove Block
    else if (!add && it->second_ && it != collider.blocks_.End())
    {
        URHO3D_LOGINFOF("MapBase() - UpdateCollisionBoxes : colliderid=%d ptr=%u ... removing a block at x=%d y=%d tileindex=%u ...",
                        collider.id_, &collider, GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex);

        if (it->second_->GetNode()->GetName() == "Box")
            it->second_->GetNode()->Remove();
        else
            it->second_->Remove();

        collider.blocks_.Erase(it);

        URHO3D_LOGINFOF("MapBase() - UpdateCollisionBoxes : ... erase the block !");

        return true;
    }

    return false;
}

bool MapBase::UpdatePlateformBoxes(PhysicCollider& collider, unsigned tileindex, bool add)
{
    HashMap<unsigned, Plateform* >& plateformsMap = collider.plateforms_;
    if (!plateformsMap.Size())
        return false;

    HashMap<unsigned, Plateform* >::Iterator it = plateformsMap.Find(tileindex);
    if (!add && it != plateformsMap.End())
    {
        // Remove a Plateform's block
#ifdef DUMP_MAPDEBUG_SETTILE
        URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... removing plateform ...", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex);
#endif
        Plateform* plateform = it->second_;

        // remove the first tile of the plateform
        if (plateform->tileleft_ == tileindex)
        {
            // Send Event before remove collisionbox
            VariantMap& eventData = GameContext::Get().context_->GetEventDataMap();
            eventData[MapTileRemoved::MAPPOINT] = GetMapPoint().ToHash();
            eventData[MapTileRemoved::MAPTILEINDEX] = tileindex;
            plateform->box_->SendEvent(MAPTILEREMOVED, eventData);

            // if just one tile remove the plateform
            if (plateform->size_ == 1)
            {
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... remove the plateform !", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex);
#endif
                plateform->box_->Remove();
                delete plateform;
            }
            else
            {
                plateform->size_--;
                plateform->tileleft_++;
                plateform->box_->SetCenter(Vector2(2.f * GetTileCoordX(plateform->tileleft_) + (float)plateform->size_, 2.f * (GetHeight() - GetTileCoordY(plateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
                plateform->box_->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * plateform->size_, 2.f* MapInfo::info.tileHalfSize_.y_));
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... remove the first tile of the plateform => tileleft1=%u size1=%u", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex, plateform->tileleft_, plateform->size_);
#endif
            }

            it = plateformsMap.Erase(it);

            return true;
        }
        else if (tileindex < plateform->tileleft_ + plateform->size_)
        {
            unsigned lasttileright = plateform->tileleft_ + plateform->size_ - 1;

            // remove the last tile of the plateform
            if (tileindex == lasttileright)
            {
                plateform->size_--;
                plateform->box_->SetCenter(Vector2(2.f * GetTileCoordX(plateform->tileleft_) + (float)plateform->size_, 2.f * (GetHeight() - GetTileCoordY(plateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
                plateform->box_->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * plateform->size_, 2.f * MapInfo::info.tileHalfSize_.y_));
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... remove the last tile of the plateform => tileleft1=%u size1=%u", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex, plateform->tileleft_, plateform->size_);
#endif
            }
            // break the plateform in 2 plateforms
            else
            {
                // create a new plateform on the right
                CollisionBox2D* collisionBox = plateform->box_->GetNode()->CreateComponent<CollisionBox2D>(LOCAL);
                Plateform* rPlateform = new Plateform(tileindex+1);
                rPlateform->size_ = lasttileright - tileindex;
                rPlateform->box_ = collisionBox;
                collisionBox->SetCenter(Vector2(2.f * GetTileCoordX(rPlateform->tileleft_) + (float)rPlateform->size_, 2.f * (GetHeight() - GetTileCoordY(rPlateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
                collisionBox->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * rPlateform->size_, 2.f * MapInfo::info.tileHalfSize_.y_));
                collisionBox->SetFriction(0.3f);
                collisionBox->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
                collisionBox->SetColliderInfo(&collider);
                collisionBox->SetViewZ(collider.params_->colliderz_-1);
                collisionBox->SetGroupIndex(colliderGroupIndex_);
                for (unsigned i=tileindex+1; i <= lasttileright; i++)
                    plateformsMap[i] = rPlateform;

                // resize the keeping left plateform
                plateform->size_ = tileindex - plateform->tileleft_;
                plateform->box_->SetCenter(Vector2(2.f * GetTileCoordX(plateform->tileleft_) + (float)plateform->size_, 2.f * (GetHeight() - GetTileCoordY(plateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
                plateform->box_->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * plateform->size_, 2.f * MapInfo::info.tileHalfSize_.y_));
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... break the plateform => tileleft1=%u size1=%u tileleft2=%u size2=%u", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex, plateform->tileleft_, plateform->size_, rPlateform->tileleft_, rPlateform->size_);
#endif
            }

            VariantMap& eventData = GameContext::Get().context_->GetEventDataMap();
            eventData[MapTileRemoved::MAPPOINT] = GetMapPoint().ToHash();
            eventData[MapTileRemoved::MAPTILEINDEX] = tileindex;
            plateform->box_->SendEvent(MAPTILEREMOVED, eventData);

            it = plateformsMap.Erase(it);

            return true;
        }
    }
    else if (add)
    {
        // Add a Plateform's block if it's near a plateform

        Plateform* plateform = 0;
        {
            HashMap<unsigned, Plateform* >::Iterator jt = plateformsMap.Find(tileindex-1);
            if (jt != plateformsMap.End()) plateform = jt->second_;
        }

        Plateform* rplateform = 0;
        {
            HashMap<unsigned, Plateform* >::Iterator jt = plateformsMap.Find(tileindex+1);
            if (jt != plateformsMap.End()) rplateform = jt->second_;
        }

        if (plateform)
        {
            if (rplateform)
            {
                plateform->size_++;
                plateform->size_ += rplateform->size_;
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... fuses plateforms ... => tileleft1=%u size1=%u", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex, plateform->tileleft_, plateform->size_);
#endif
                unsigned endtile = tileindex + rplateform->size_;
                for (unsigned i=tileindex; i <= endtile; i++)
                    plateformsMap[i] = plateform;

                rplateform->box_->Remove();
                delete rplateform;
            }
            else
            {
                plateform->size_++;
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... adding block on the plateform at left ... => tileleft1=%u size1=%u", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex, plateform->tileleft_, plateform->size_);
#endif
                plateformsMap[tileindex] = plateform;
            }

            plateform->box_->SetCenter(Vector2(2.f * GetTileCoordX(plateform->tileleft_) + (float)plateform->size_, 2.f * (GetHeight() - GetTileCoordY(plateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
            plateform->box_->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * plateform->size_, 2.f * MapInfo::info.tileHalfSize_.y_));
            return true;
        }

        if (rplateform)
        {
            rplateform->size_++;
            rplateform->tileleft_ = tileindex;
#ifdef DUMP_MAPDEBUG_SETTILE
            URHO3D_LOGINFOF("MapBase() - UpdatePlateformBoxes : at x=%d y=%d tileindex=%u ... adding block on the plateform at right ... => tileleft1=%u size1=%u", GetTileCoordX(tileindex), GetTileCoordY(tileindex), tileindex, rplateform->tileleft_, rplateform->size_);
#endif
            plateformsMap[tileindex] = rplateform;

            rplateform->box_->SetCenter(Vector2(2.f * GetTileCoordX(rplateform->tileleft_) + (float)rplateform->size_, 2.f * (GetHeight() - GetTileCoordY(rplateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
            rplateform->box_->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * rplateform->size_, 2.f * MapInfo::info.tileHalfSize_.y_));
            return true;
        }
    }

    return false;
}

bool MapBase::UpdatePlateformBoxes(PhysicCollider& collider, const Vector<unsigned>& addedtiles, const Vector<unsigned>& removedtiles)
{
    HashMap<unsigned, Plateform* >& plateformsMap = collider.plateforms_;
    if (!plateformsMap.Size())
        return false;

    // Find All the plateforms modified by tiles.
    HashSet<Plateform* > plateformsToUpdate;

    for (unsigned i=0; i < removedtiles.Size(); i++)
    {
        unsigned tileindex = removedtiles[i];

        HashMap<unsigned, Plateform* >::Iterator it = plateformsMap.Find(tileindex);
        if (it == plateformsMap.End())
            continue;

        Plateform* plateform = it->second_;

        if (plateform->tileleft_ == tileindex)
        {
            // remove the first tile of the plateform

            if (plateform->size_ == 1)
            {
                // if just one tile remove the plateform
                plateform->box_->Remove();
                delete plateform;
            }
            else
            {
                plateform->size_--;
                plateform->tileleft_++;
                plateformsToUpdate.Insert(plateform);
            }

            it = plateformsMap.Erase(it);
        }
        else if (tileindex < plateform->tileleft_ + plateform->size_)
        {
            unsigned lasttileright = plateform->tileleft_ + plateform->size_ - 1;

            // remove the last tile of the plateform
            if (tileindex == lasttileright)
            {
                plateform->size_--;
                plateformsToUpdate.Insert(plateform);
            }
            // break the plateform in 2 plateforms
            else
            {
                // create a new plateform on the right
                Plateform* rPlateform = new Plateform(tileindex+1);
                rPlateform->size_ = lasttileright - tileindex;
                rPlateform->box_ = plateform->box_->GetNode()->CreateComponent<CollisionBox2D>(LOCAL);
                rPlateform->box_->SetFriction(0.3f);
                rPlateform->box_->SetFilterBits(collider.params_->bits1_, collider.params_->bits2_);
                rPlateform->box_->SetColliderInfo(&collider);
                rPlateform->box_->SetViewZ(collider.params_->colliderz_-1);
                rPlateform->box_->SetGroupIndex(colliderGroupIndex_);

                for (unsigned i=tileindex+1; i <= lasttileright; i++)
                    plateformsMap[i] = rPlateform;

                // resize the keeping left plateform
                plateform->size_ = tileindex - plateform->tileleft_;

                plateformsToUpdate.Insert(rPlateform);
                plateformsToUpdate.Insert(plateform);
            }

            it = plateformsMap.Erase(it);
        }
    }

    for (unsigned i=0; i < addedtiles.Size(); i++)
    {
        unsigned tileindex = addedtiles[i];

        // Add a Plateform's block if it's near a plateform

        Plateform* plateform = 0;
        {
            HashMap<unsigned, Plateform* >::Iterator jt = plateformsMap.Find(tileindex-1);
            if (jt != plateformsMap.End()) plateform = jt->second_;
        }

        Plateform* rplateform = 0;
        {
            HashMap<unsigned, Plateform* >::Iterator jt = plateformsMap.Find(tileindex+1);
            if (jt != plateformsMap.End()) rplateform = jt->second_;
        }

        if (plateform)
        {
            if (rplateform)
            {
                plateform->size_++;
                plateform->size_ += rplateform->size_;

                unsigned endtile = tileindex + rplateform->size_;
                for (unsigned i=tileindex; i <= endtile; i++)
                    plateformsMap[i] = plateform;

                rplateform->box_->Remove();
                delete rplateform;
            }
            else
            {
                plateform->size_++;
                plateformsMap[tileindex] = plateform;
            }

            plateformsToUpdate.Insert(plateform);
            continue;
        }

        if (rplateform)
        {
            rplateform->size_++;
            rplateform->tileleft_ = tileindex;
            plateformsMap[tileindex] = rplateform;

            plateformsToUpdate.Insert(rplateform);
        }
    }

    // Update the Physics
    for (HashSet<Plateform* >:: Iterator it = plateformsToUpdate.Begin(); it != plateformsToUpdate.End(); ++it)
    {
        Plateform* plateform = *it;
        plateform->box_->SetCenter(Vector2(2.f * GetTileCoordX(plateform->tileleft_) + (float)plateform->size_, 2.f * (GetHeight() - GetTileCoordY(plateform->tileleft_) - 1) + 1.f) * MapInfo::info.tileHalfSize_ - center_);
        plateform->box_->SetSize(Vector2(2.f * MapInfo::info.tileHalfSize_.x_ * plateform->size_, 2.f* MapInfo::info.tileHalfSize_.y_));
    }

    return plateformsToUpdate.Size();
}


/// GETTERS

void MapBase::GetFurnitures(const IntRect& rect, PODVector<EntityData>& furnitures, bool newOrigin)
{
    if (!mapData_)
        return;

    int width = rect.Width()+1;
    IntVector2 offset(rect.left_, rect.top_);

    for (unsigned i=0; i < mapData_->furnitures_.Size(); i++)
    {
        const EntityData& furniture = mapData_->furnitures_[i];
        IntVector2 coord = GetTileCoords(furniture.tileindex_);

        if (rect.IsInside(coord) == INSIDE)
        {
            furnitures.Push(furniture);

            if (newOrigin)
                furnitures.Back().tileindex_ = (coord.y_-offset.y_) * width + coord.x_-offset.x_;

            URHO3D_LOGINFOF("MapBase() - GetFurnitures : furniture[%d] tileindex=%u(%s) => %u(%d %d)... ",
                            i, furniture.tileindex_, coord.ToString().CString(), furnitures.Back().tileindex_, coord.x_-offset.x_, coord.y_-offset.y_);
        }
    }
}

const TileModifier* MapBase::GetTileModifier(int x, int y, int viewZ) const
{
    if (!cacheTileModifiers_.Size())
        return 0;

    for (List<TileModifier>::ConstIterator it=cacheTileModifiers_.Begin(); it != cacheTileModifiers_.End(); ++it)
    {
        const TileModifier& m = *it;
        if (m.x_ == x && m.y_ == y && m.z_ == viewZ)
            return &m;
    }

    return 0;
}

Vector2 MapBase::GetWorldTilePosition(const IntVector2& coord, const Vector2& positionInTile) const
{
    Vector2 localposition;
    localposition.x_ = ((float)(coord.x_) + 0.5f + positionInTile.x_) * Map::info_->tileWidth_ - center_.x_;
    localposition.y_ = ((float)(GetHeight() - 1 - coord.y_) + 0.5f + positionInTile.y_) * Map::info_->tileHeight_ - center_.y_;
    return GetRootNode()->GetWorldTransform2D() * localposition;
}

Vector2 MapBase::GetPositionInTile(const Vector2& worldposition, unsigned tileindex)
{
    Vector2 localposition = GetRootNode()->GetWorldTransform2D().Inverse() * worldposition;
    return Vector2(((localposition.x_ + center_.x_) / Map::info_->tileWidth_) - 0.5f - (float)GetTileCoordX(tileindex),
                   ((localposition.y_ + center_.y_) / Map::info_->tileHeight_) - 0.5f - (float)(GetHeight() - GetTileCoordY(tileindex) - 1));
}

unsigned MapBase::GetTileIndexAt(const Vector2& worldposition) const
{
    Vector2 localposition = GetRootNode()->GetWorldTransform2D().Inverse() * worldposition;
    int x = Clamp((int)Floor((localposition.x_ + center_.x_) / Map::info_->tileWidth_), 0, GetWidth()-1);
    int y = Clamp(GetHeight() - (int)Floor((localposition.y_ + center_.y_) / Map::info_->tileHeight_) - 1, 0, GetHeight()-1);
    return GetTileIndex(x, y);
}

void MapBase::GetBlockPositionAt(int direction, int viewid, IntVector2& position, int bound)
{
    const FeaturedMap& features = GetFeaturedMap(viewid);

    if (features[GetTileIndex(position.x_, position.y_)] > MapFeatureType::Threshold)
        return;

    int incx, incy, startx, starty, endx, endy;

    incx = incy = 0;
    startx = position.x_;
    starty = position.y_;

    if (direction & UpDir)
    {
        endy = bound > 0 && bound < starty ? bound : 0;
        incy = -1;
    }
    else if (direction & DownDir)
    {
        endy = bound > starty ? Min(bound, GetHeight()-1) : GetHeight()-1;
        incy = 1;
    }

    if (direction & LeftDir)
    {
        endx = bound > 0 && bound < startx ? bound : 0;
        incx = -1;
    }
    else if (direction & RightDir)
    {
        endx = bound > startx ? Min(bound, GetWidth()-1) : GetWidth()-1;
        incx = 1;
    }

    if (incy != 0 && incx != 0)
    {
        for (int y = starty; y != endy; y += incy)
        {
            for (int x = startx; x != endx; x += incx)
            {
                if (features[GetTileIndex(x, y)] > MapFeatureType::Threshold)
                {
                    position.x_ = x;
                    position.y_ = y;
                    return;
                }
            }
        }

        position.x_ = endx;
        position.y_ = endy;
    }
    else
    {
        if (incy != 0)
        {
            const int x = position.x_;

            for (int y = starty; y != endy; y += incy)
            {
                if (features[GetTileIndex(x, y)] > MapFeatureType::Threshold)
                {
                    position.y_ = y;
                    return;
                }
            }

            position.y_ = endy;
        }
        else if (incx != 0)
        {
            const int y = position.y_;

            for (int x = startx; x != endx; x += incx)
            {
                if (features[GetTileIndex(x, y)] > MapFeatureType::Threshold)
                {
                    position.x_ = x;
                    return;
                }
            }

            position.x_ = endx;
        }
    }
}

void MapBase::GetColliders(MapColliderType type, int indv, PODVector<MapCollider*>& mapcolliders)
{
    mapcolliders.Clear();

    if (type == PHYSICCOLLIDERTYPE)
    {
        unsigned numviews = Min(colliderNumParams_[type], physicColliders_.Size());
        for (unsigned i=0; i < numviews; i++)
        {
            PhysicCollider& collider = physicColliders_[i];
            if (collider.params_ && collider.params_->indv_ == indv)
                mapcolliders.Push(&collider);
        }
    }
    else if (type == RENDERCOLLIDERTYPE && GameContext::Get().gameConfig_.renderShapes_)
    {
        unsigned numviews = Min(colliderNumParams_[type], renderColliders_.Size());
        for (unsigned i=0; i < numviews; i++)
        {
            RenderCollider& collider = renderColliders_[i];
            if (collider.params_ && collider.params_->indv_ == indv)
                mapcolliders.Push(&collider);
        }
    }
}

void MapBase::GetCollidersAtZIndex(MapColliderType type, int indz, PODVector<MapCollider*>& mapcolliders)
{
    mapcolliders.Clear();

    if (type == PHYSICCOLLIDERTYPE)
    {
        unsigned numviews = Min(colliderNumParams_[type], physicColliders_.Size());
        for (unsigned i=0; i < numviews; i++)
        {
            PhysicCollider& collider = physicColliders_[i];
            if (collider.params_ && collider.params_->indz_ == indz)
                mapcolliders.Push(&collider);
        }
    }
    else if (type == RENDERCOLLIDERTYPE && GameContext::Get().gameConfig_.renderShapes_)
    {
        unsigned numviews = Min(colliderNumParams_[type], renderColliders_.Size());
        for (unsigned i=0; i < numviews; i++)
        {
            RenderCollider& collider = renderColliders_[i];
            if (collider.params_ && collider.params_->indz_ == indz)
                mapcolliders.Push(&collider);
        }
    }
}

BoundingBox MapBase::GetWorldBoundingBox2D()
{
#ifdef USE_TILERENDERING
    return objectTiled_->GetWorldBoundingBox2D();
#else
    BoundingBox boundingbox;
    if (renderColliders_.Size() > 0)
        boundingbox = renderColliders_[0].renderShape_->GetWorldBoundingBox2D();
    if (renderColliders_.Size() > 1)
        for (unsigned i = 1; i < renderColliders_.Size(); i++)
            boundingbox.Merge(renderColliders_[i].renderShape_->GetWorldBoundingBox2D());
    return boundingbox;
#endif
}

FeatureType* MapBase::GetTerrainMap() const
{
    return &featuredMap_->GetTerrainMap()[0];
}

FeatureType* MapBase::GetBiomeMap() const
{
    return &featuredMap_->GetBiomeMap()[0];
}

// Check if the rect in this MapBase is masked by another map
bool MapBase::IsMaskedByOtherMap(int viewZ) const
{
    IntRect rect(0, 0, GetWidth()-1, GetHeight()-1);

    const FeaturedMap& features = GetFeatureView(GetViewId(INNERVIEW));

    int width = GetWidth();
    int wrect = rect.Width();
    int hrect = rect.Height();

//    GameHelpers::DumpData(&features[0], 1, 2, width, height);

    int nummaskedtiles = 0;
    IntVector2 coord;
    for (coord.y_=0; coord.y_ < hrect; coord.y_++)
    {
        for (coord.x_=0; coord.x_ < wrect; coord.x_++)
        {
            if (features[coord.y_ * width + coord.x_] > MapFeatureType::NoRender)
            {
                // check if the tile is masked by a map
                Vector2 wtileposition = GetWorldTilePosition(coord);
                Map* map = World2D::GetMapAt(wtileposition);
                if (!map)
                    continue;

                unsigned tileindexInMap;
                const FeaturedMap& mapmask = map->GetMaskedView(ViewManager::GetViewZIndex(viewZ), map->GetViewId(INNERVIEW));
                World2D::GetWorldInfo()->Convert2WorldTileIndex(map->GetMapPoint(), wtileposition, tileindexInMap);
                if (mapmask[tileindexInMap] == MapFeatureType::NoRender)
                    nummaskedtiles++;
            }
        }
    }

    return nummaskedtiles > 0;
}

//// Check if the rect in this MapBase is masked by another map
//bool MapBase::IsMaskedBy(MapBase* map, int viewZ) const
//{
//    IntRect rect(0, 0, GetWidth()-1, GetHeight()-1);
//    IntRect rectInMap = GetRectIn(map);
//    if (rectInMap.top_ < 0)
//    {
//        rect.top_ -= rectInMap.top_;
//        rectInMap.top_ = 0;
//    }
//    if (rectInMap.left_ < 0)
//    {
//        rect.left_ -= rectInMap.left_;
//        rectInMap.left_ = 0;
//    }
//    if (rectInMap.bottom_ >= map->GetHeight())
//    {
//        rect.bottom_ -= (rectInMap.bottom_ - map->GetHeight() + 1);
//        rectInMap.bottom_ = map->GetHeight()-1;
//    }
//    if (rectInMap.right_ >= map->GetWidth())
//    {
//        rect.right_ -= (rectInMap.right_ - map->GetWidth() + 1);
//        rectInMap.right_ = map->GetWidth()-1;
//    }
//
//    if (!rectInMap.Width() || !rectInMap.Height() || rect.left_ >= rect.right_ || rect.top_ >= rect.bottom_)
//        return false;
//
//    const FeaturedMap& mapmask = map->GetMaskedView(ViewManager::GetViewZIndex(viewZ), map->GetViewId(INNERVIEW));
//    const FeaturedMap& features = GetFeatureView(GetViewId(INNERVIEW));
//
//    int width = GetWidth();
//    int mwidth = map->GetWidth();
//    int wrect = rect.Width();
//    int hrect = rect.Height();
//
////    GameHelpers::DumpData(&features[0], 1, 2, width, height);
//
//    int nummaskedtiles = 0;
//    for (int y=0; y < hrect; y++)
//    {
//        for (int x=0; x < wrect; x++)
//        {
//            if (features[(y + rect.top_) * width + x + rect.left_] > MapFeatureType::NoRender && mapmask[(y + rectInMap.top_) * mwidth + x + rectInMap.left_] == MapFeatureType::NoRender)
//                nummaskedtiles++;
//        }
//    }
//
//    return nummaskedtiles > 0;
//}

//IntRect MapBase::GetRectIn(MapBase* map) const
//{
//    Vector2 positioninmap = map->GetRootNode()->GetWorldTransform2D().Inverse() * GetRootNode()->GetWorldPosition2D();
//    IntVector2 coordinmap(floor((positioninmap.x_+map->center_.x_) / Map::info_->tileWidth_), map->GetHeight() - (int)floor((positioninmap.y_+map->center_.y_) / Map::info_->tileHeight_) - 1);
//
//    /// considering the center of this map too !
//    return IntRect(coordinmap.x_ - center_.x_ / Map::info_->tileWidth_, coordinmap.y_ - center_.y_ / Map::info_->tileHeight_, coordinmap.x_ + center_.x_ / Map::info_->tileWidth_, coordinmap.y_ + center_.y_ / Map::info_->tileHeight_);
//}

FeatureType MapBase::GetFeatureAtZ(unsigned index, int layerZ) const
{
    int viewid = featuredMap_->GetViewId(layerZ);
    if (viewid == -1)
    {
        URHO3D_LOGERRORF("MapBase() - GetFeatureAtZ : ref=(%d %d) index=%u layerZ=%d => ERROR viewid=-1 !", mapStatus_.x_, mapStatus_.y_, index, layerZ);
        return 0;
    }

    return GetFeature(index, viewid);
}

FluidCell* MapBase::GetFluidCellPtr(unsigned index, int indexZ) const
{
    return &featuredMap_->GetFluidView(ViewManager::viewZIndex2fluidZIndex_[indexZ]).fluidmap_[index];
}


// Tile Getters
bool MapBase::HasTileProperties(unsigned index, int viewId, unsigned properties) const
{
    Tile* tile = InGetTile(index, viewId);
    return tile != 0 ? (tile->GetPropertyFlags() & properties) != 0 : false;
}

bool MapBase::HasTileProperties(int x, int y, int viewId, unsigned properties) const
{
    Tile* tile = InGetTile(x, y, viewId);
    return tile != 0 ? (tile->GetPropertyFlags() & properties) != 0 : false;
}

unsigned MapBase::GetTileProperties(unsigned index, int viewId) const
{
    Tile* tile = InGetTile(index, viewId);
    return tile != 0 ? tile->GetPropertyFlags() : 0;
}

unsigned MapBase::GetTilePropertiesAtZ(unsigned index, int viewZ) const
{
    Tile* tile = InGetTile(index, GetViewId(viewZ));
    return tile != 0 ? tile->GetPropertyFlags() : 0;
}

unsigned MapBase::GetTileProperties(int x, int y, int viewId) const
{
    Tile* tile = InGetTile(x, y, viewId);
    return tile != 0 ? tile->GetPropertyFlags() : 0;
}

unsigned char MapBase::GetTerrain(unsigned index, int viewId) const
{
//    return objectTiled_->GetTerrainId(skinnedMap_->GetTiledView(viewId), index);

    Tile* tile = InGetTile(index, viewId);
    return tile != 0 ? tile->GetTerrain() : 0;
}

Tile* MapBase::GetTile(unsigned index, int viewId) const
{
    return InGetTile(index, viewId);
}

// Area Properties
unsigned MapBase::GetAreaProps(unsigned index, int indexZ, int jumpheight) const
{
    FluidCell* cell = GetFluidCellPtr(index, indexZ);

    if (!cell || (cell->type_ == BLOCK))
        return nomoveFlag;

    unsigned flag = swimmableFlag;
    if (cell->type_ == AIR)
        flag = flyableFlag;

    // Walk : check ground
    if (cell->Bottom && cell->Bottom->type_ == BLOCK)
        flag |= walkableFlag;

    // Jump : check sides
    if (cell->Right && cell->Right->type_ == BLOCK)
        flag |= jumpableRightFlag;
    if (cell->Left && cell->Left->type_ == BLOCK)
        flag |= jumpableLeftFlag;

    // Jump : check ground max jumpheight
    cell = cell->Bottom;
    while (cell && cell->type_ != BLOCK && jumpheight-- > 0)
        cell = cell->Bottom;
    if (jumpheight > 0)
        flag |= (jumpableRightFlag|jumpableLeftFlag);

    return flag;
}


/// DUMP

void MapBase::Dump() const
{
    URHO3D_LOGINFOF("MapBase() - Dump : mPoint=%s status=%s(%d) numviews=%d", GetMapPoint().ToString().CString(),
                    mapStatusNames[mapStatus_.status_], mapStatus_.status_, skinnedMap_ ? skinnedMap_->GetNumViews() : 0);

    if (skinnedMap_ && skinnedMap_->GetNumViews())
        skinnedMap_->Dump();
}



/// MAP

/// VARIABLES

World2DInfo* Map::info_;
ViewManager* Map::viewManager_= 0;
CreateMode Map::replicationMode_ = REPLICATED;



/// STATIC FUNCTIONS

void Map::Initialize(World2DInfo* winfo, ViewManager* viewManager, int chunknumx, int chunknumy)
{
    Reset();

    info_ = winfo;

    viewManager_ = viewManager;

    MapInfo::Initialize(info_->mapWidth_, info_->mapHeight_, chunknumx, chunknumy, info_->tileWidth_, info_->tileHeight_, info_->mWidth_, info_->mHeight_);

    MapColliderGenerator::Initialize(info_);

    delayUpdateUsec_ = World2DInfo::delayUpdateUsec_;

    URHO3D_LOGINFOF("Map() - Initialize : width=%d height=%d numchunks=%d %d", MapInfo::info.width_, MapInfo::info.height_, chunknumx, chunknumy);
}

void Map::Reset()
{
    MapInfo::Reset();
    MapColliderGenerator::Reset();
}

Rect Map::GetMapRect(const ShortIntVector2& mpoint)
{
    Rect rect;
    rect.min_ = World2D::GetWorld()->GetNode()->GetWorldPosition2D() + Vector2(mpoint.x_ * MapInfo::info.mWidth_, mpoint.y_ * MapInfo::info.mHeight_);
    rect.max_ = rect.min_ + Vector2(MapInfo::info.mWidth_, MapInfo::info.mHeight_);
    return rect;
}


/// INITIALIZERS / STATUS SETTERS

Map::Map(Context* context) :
    Object(context),
    localEntitiesNode_(0),
    replicatedEntitiesNode_(0)
{ }

Map::Map() :
    Object(GameContext::Get().context_),
    localEntitiesNode_(0),
    replicatedEntitiesNode_(0)
{
//    URHO3D_LOGINFOF("Map() ptr=%u ... OK !", this);

    Resize();
    Init();
}

Map::~Map()
{
    RemoveNodes();

    physicColliders_.Clear();
    renderColliders_.Clear();

#ifdef USE_TILERENDERING
    objectTiled_.Reset();
#else
    skinnedMap_.Reset();
#endif

    mapStatus_.map_ = 0;
    mapTopography_.Clear();

    for (int i=0; i < 2; i++)
        miniMap_[i].Reset();

    miniMapLayers_.Clear();
    miniMapLayersByViewZIndex_.Clear();

//    URHO3D_LOGINFOF("~Map() ptr=%u ... OK !", this);
}

void Map::Resize()
{
    mapStatus_.map_ = this;

    SetSize(MapInfo::info.width_, MapInfo::info.height_);

#ifdef USE_TILERENDERING
    if (!objectTiled_)
        objectTiled_ = SharedPtr<ObjectTiled>(new ObjectTiled(context_));

    objectTiled_->Resize(MapInfo::info.width_, MapInfo::info.height_, MAP_NUMMAXVIEWS, MapInfo::info.chinfo_);
    objectTiled_->map_ = this;
    skinnedMap_ = objectTiled_->GetObjectSkinned();
    featuredMap_ = objectTiled_->GetObjectFeatured();
#else
    if (!skinnedMap_)
        skinnedMap_ = SharedPtr<ObjectSkinned>(new ObjectSkinned());

    skinnedMap_->Resize(MapInfo::info.width_, MapInfo::info.height_, MAP_NUMMAXVIEWS);
    featuredMap_ = skinnedMap_->GetFeatureData();
#endif

    skinnedMap_->map_ = this;
    featuredMap_->map_ = this;

    los_.Resize(MapInfo::info.mapsize_);

    // Minimaps
    {
        if (!miniMap_[0])
        {
            for (int i=0; i < 2; i++)
            {
                miniMap_[i] = SharedPtr<Image>(new Image(context_));
                miniMap_[i]->SetSize(MapInfo::info.width_ * MINIMAP_PIXBYTILE, MapInfo::info.height_ * MINIMAP_PIXBYTILE, 4);
            }
        }

        int numViewZ = ViewManager::Get()->GetNumViewZ();
        if (miniMapLayersByViewZIndex_.Size() != numViewZ)
        {
            // MINIMAP_NUMLAYERS layers by viewZ (ex : FRONT+OUTERVIEW+BACKGROUND & INNER+BACKVIEW+BACKGROUND)
            int numLayers = numViewZ * MINIMAP_NUMLAYERS;
            miniMapLayers_.Resize(numLayers);
            for (int l=0; l < numLayers; l++)
                miniMapLayers_[l] = SharedPtr<Image>(new Image(context_));

            for (int l=0; l < miniMapLayers_.Size(); l++)
                miniMapLayers_[l]->SetSize(MapInfo::info.width_ * MINIMAP_PIXBYTILE, MapInfo::info.height_ * MINIMAP_PIXBYTILE, 4);

            miniMapLayersByViewZIndex_.Resize(numViewZ);
            for (int z=0; z < numViewZ; z++)
            {
                miniMapLayersByViewZIndex_[z].Resize(MINIMAP_NUMLAYERS);
                for (int l=0; l < MINIMAP_NUMLAYERS; l++)
                    miniMapLayersByViewZIndex_[z][l] = miniMapLayers_[MINIMAP_NUMLAYERS * z + l].Get();
            }
        }
    }

    // MapColliders Resize
    {
        physicColliders_.Resize(MAP_NUMMAXCOLLIDERS);
        for (unsigned i=0; i < physicColliders_.Size(); i++)
        {
            physicColliders_[i].id_ = i;
            physicColliders_[i].ReservePhysic(MapInfo::info.mapsize_);
            physicColliders_[i].map_ = this;
        }

        renderColliders_.Resize(MAP_NUMMAXRENDERCOLLIDERS);
        for (unsigned i=0; i < renderColliders_.Size(); i++)
        {
            renderColliders_[i].renderShape_ = 0;
            renderColliders_[i].map_ = this;
        }
    }

    // Allocation of PODVector Shape Pointers for SetVisibleStatic
    sboxes_.Reserve(MapInfo::info.mapsize_ / 2);
    splateforms_.Reserve(MapInfo::info.mapsize_ / 10);
    schains_.Reserve(MapInfo::info.mapsize_ / 10);

//    URHO3D_LOGINFOF("Map() - Resize: this=%u numViews=%u numChunks=%u", this, MAP_NUMMAXVIEWS, MapInfo::info.chinfo_->chunks_.Size());
}

void Map::Init()
{
    node_.Reset();
    nodeStatic_.Reset();

    if (nodeTag_)
    {
        nodeTag_->Remove();
        nodeTag_.Reset();
    }

    localEntitiesNode_ = 0;
    replicatedEntitiesNode_ = 0;

    visible_ = false;
    skipInitialTopBorder_ = true;

    for (int dir=0; dir < 4; dir++)
        connectedMaps_[dir] = 0;

    nodeChains_.Clear();
    nodeBoxes_.Clear();
    nodePlateforms_.Clear();
    nodeImages_.Clear();

    mapStatus_.Clear();
    mapTopography_.Clear();

    if (mapData_)
        mapData_->SetMap(0);

    mapData_ = 0;

    cacheTileModifiers_.Clear();

    SetSerializable(false);
}

bool Map::Clear(HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_Clear);
#endif

    int& mcount = GetMapCounter(MAP_FUNC1);

    if (mcount == 0)
    {
#ifdef USE_TILERENDERING
        if (objectTiled_)
        {
            if (node_)
                node_->RemoveComponent(objectTiled_);

            objectTiled_->Clear();

            URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... objectTiled Cleared ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);
        }
#else
        if (skinnedMap_)
        {
            skinnedMap_->Clear();
            URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... skinnedMap Cleared ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);
        }
#endif
        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 1)
    {
        for (unsigned i=0; i < physicColliders_.Size(); i++)
        {
            physicColliders_[i].Clear();
            physicColliders_[i].ClearBlocks();
        }

        URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... PhysicColliders Cleared ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 2)
    {
        ClearConnectedMaps();

        URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... Border Maps Cleared ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 3)
    {
        for (unsigned i=0; i < renderColliders_.Size(); i++)
        {
            renderColliders_[i].Clear();
            renderColliders_[i].map_ = 0;
        }

//        DrawableScroller::RemoveAllObjectsOnMap(GetMapPoint());

        URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... RenderColliders Cleared ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mcount++;
        GetMapCounter(MAP_FUNC2) = 0;

        if (TimeOver(timer))
            return false;
    }

    if (mcount == 4)
    {
        if (node_)
        {
            if (!RemoveNodes(true, timer))
                return false;
        }

        URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... Nodes Removed ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mcount++;
    }

    if (mcount == 5)
    {
        Init();

        URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... Map Reinitialized ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);
    }

    URHO3D_LOGINFOF("Map() - Clear mPoint=%s ... timer=%d/%d msec OK !", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

    return true;
}

void Map::PullFluids()
{
    if (featuredMap_)
        featuredMap_->PullFluids();

    if (WaterLayer::Get())
        WaterLayer::Get()->Clear();

    URHO3D_LOGINFOF("Map() - PullFluids mPoint=%s ... OK !", GetMapPoint().ToString().CString());
}



void Map::Initialize(const ShortIntVector2& mPoint, unsigned wseed)
{
//    URHO3D_LOGINFOF("Map() - Initialize : mPoint=%s ...", mPoint.ToString().CString());

    SetMapPoint(mPoint);

    mapStatus_.x_ = mPoint.x_;
    mapStatus_.y_ = mPoint.y_;
    mapStatus_.width_ = MapInfo::info.width_;
    mapStatus_.height_ = MapInfo::info.height_;
    mapStatus_.wseed_ = wseed;
    mapStatus_.cseed_ = mPoint.ToHash();
    mapStatus_.rseed_ = mapStatus_.wseed_+mapStatus_.cseed_;

    SetStatus(Initializing);

//	URHO3D_LOGINFOF("Map() - Initialize : mPoint=%s map=%u ... OK !", mPoint.ToString().CString(), this);
}

void Map::SetTagNode(Node* node)
{
    if (!nodeTag_)
    {
        Vector2 position = node->GetWorldPosition2D() + Vector2(GetMapPoint().x_ * MapInfo::info.mWidth_, GetMapPoint().y_ * MapInfo::info.mHeight_);
        nodeTag_ = node->CreateChild("TagInfo", LOCAL);
        nodeTag_->SetWorldPosition2D(position);
        nodeTag_->SetWorldScale2D(node->GetWorldScale2D());
        nodeTag_->SetTemporary(true);
    }
}


bool Map::Set(Node* node, HiresTimer* timer)
{
    switch (mapStatus_.status_)
    {
    case Setting_Map:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_Set);
#endif

        assert(node);

        String nodeName = ToString("Map_%d_%d", GetMapPoint().x_, GetMapPoint().y_);
        node_ = WeakPtr<Node>(node->CreateChild(nodeName, LOCAL));
        nodeRoot_ = node_;
        node_->SetTemporary(true);

        SetTagNode(node);

        Vector2 position = node->GetWorldPosition2D() + Vector2(GetMapPoint().x_ * MapInfo::info.mWidth_, GetMapPoint().y_ * MapInfo::info.mHeight_);
        node_->SetWorldPosition2D(position);
        node_->SetWorldScale2D(node->GetWorldScale2D());

//            URHO3D_LOGINFOF("Map() - Set mPoint = %s ... status=%d", GetMapPoint().ToString().CString(), mapStatus_.status_);
        nodeStatic_ = node_->CreateChild("Statics", LOCAL);

#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
        localEntitiesNode_ = World2D::GetEntitiesNode(GetMapPoint(), LOCAL);
        replicatedEntitiesNode_ = World2D::GetEntitiesNode(GetMapPoint(), REPLICATED);
#endif

        mapTopography_.GenerateWorldEllipseInfos(mapStatus_);

        // Create RenderShape
        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            unsigned numrenderviews = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
            for (unsigned i=0; i< numrenderviews; i++)
                renderColliders_[i].CreateRenderShape(node_, true, false);
        }

        SetStatus(Setting_Layers);

        if (TimeOverMaximized(timer))
            break;
    }
    case Setting_Layers:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_SetLayers);
#endif

//            URHO3D_LOGINFOF("Map() - Set mPoint = %s ... status=%d", GetMapPoint().ToString().CString(), mapStatus_.status_);
#ifdef USE_TILERENDERING
        if (objectTiled_)
        {
            node_->SetEnabled(false);
            node_->AddComponent(objectTiled_, 0, LOCAL);
        }
        else
            URHO3D_LOGERROR("Map() - Set : No ObjectTiled !");
#endif

        SetStatus(Setting_Colliders);

        GetMapCounter(MAP_GENERAL) = 0;

#ifdef DUMP_ERROR_ON_TIMEOVER
        LogTimeOver(ToString("Map() - Set : map=%s ... Setting Layers", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif

        if (TimeOverMaximized(timer))
            break;
    }
    case Setting_Colliders:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_SetColliders);
#endif

#ifdef MAP_CREATECOLLISIONSHAPES
        if (!SetCollisionShapes(timer))
            break;
#endif

        SetStatus(Updating_ViewBatches);

        GetMapCounter(MAP_GENERAL) = 0;

#ifdef USE_TILERENDERING
        objectTiled_->indexToSet_ = 0;
#endif

        if (TimeOverMaximized(timer))
            break;
    }
    case Updating_ViewBatches:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_RenderViewBatches);
#endif

#ifdef RENDER_VIEWBATCHES
        if (viewManager_)
        {
#ifdef USE_TILERENDERING
            if (!objectTiled_->UpdateViewBatches(viewManager_->GetNumViewZ(), timer, delayUpdateUsec_))
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("Map() - Set : map=%s ... Updating ViewBatches", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif
                break;
            }
#endif
        }
#endif

        SetStatus(Updating_RenderShapes);

        GetMapCounter(MAP_GENERAL) = 0;

        if (TimeOverMaximized(timer))
            break;
    }
    case Updating_RenderShapes:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_RenderShapes);
#endif

        if (GameContext::Get().gameConfig_.renderShapes_)
            if (!UpdateRenderShapes(timer))
            {
#ifdef DUMP_ERROR_ON_TIMEOVER
                LogTimeOver(ToString("Map() - Set : map=%s ... Updating RenderShapes", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif
                break;
            }

        SetStatus(Setting_Areas);

        GetMapCounter(MAP_GENERAL) = 0;

        if (TimeOverMaximized(timer))
            break;
    }
    case Setting_Areas:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
        SetStatus(Setting_BackGround);

        GetMapCounter(MAP_GENERAL) = 0;

        if (TimeOverMaximized(timer))
            break;
    }
    case Setting_BackGround:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_SetBackGd);
#endif

        if (!AddBackGroundLayers(timer))
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("Map() - Set : map=%s ... Setting BackGround", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif
            break;
        }

        SetStatus(Setting_MiniMap);

        GetMapCounter(MAP_GENERAL) = 0;

        if (TimeOverMaximized(timer))
            break;
    }
    case Setting_MiniMap:
    {
#ifdef DUMP_MAPCREATOR_LOGS
        URHO3D_LOGERRORF("Map() - Set : status=%d timer=%d msec", mapStatus_.status_, timer ? timer->GetUSec(false)/1000 : 0);
#endif
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_MiniMap);
#endif

#ifdef HANDLE_MINIMAP
        if (!SetMiniMap(timer))
        {
#ifdef DUMP_ERROR_ON_TIMEOVER
            LogTimeOver(ToString("Map() - Set : map=%s ... MiniMap", GetMapPoint().ToString().CString()), timer, delayUpdateUsec_);
#endif
            break;
        }

        GetMapCounter(MAP_GENERAL) = 0;
#endif
        SetStatus(Setting_Furnitures);

        SendEvent(MAP_UPDATE);
        return true;
    }
    }

    return false;
}


void Map::MarkDirty()
{
#ifdef USE_TILERENDERING
#ifdef RENDER_VIEWBATCHES
    if (!objectTiled_)
        return;
    objectTiled_->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetDefaultChunkGroup(MapDirection::All));
#endif
#endif // USE_TILERENDERING
}



/// PHYSIC COLLIDERS

void Map::CreateTopBorderCollisionShapes()
{
    unsigned numphysicviews = colliderNumParams_[PHYSICCOLLIDERTYPE];

    MapColliderGenerator::Get()->SetParameters(true, true);

    for (unsigned i = 0; i < numphysicviews; i++)
    {
        if (physicColliders_[i].params_->mode_ != TopBorderBackMode)
            continue;

        // Generate Colliders
        MapColliderGenerator::Get()->GeneratePhysicCollider(this, 0, physicColliders_[i], MapInfo::info.width_, 1, center_);

        // Set CollisionShapes
        GetMapCounter(MAP_FUNC2) = 0;

        SetCollisionChain2D(physicColliders_[i]);
    }

    MapColliderGenerator::Get()->SetParameters(false);
}



/// MAPDATA UPDATER

bool Map::OnUpdateMapData(HiresTimer* timer)
{
    if (!GetMapData())
    {
        URHO3D_LOGERRORF("Map() - OnUpdateMapData : Map=%s ... No MapData !", GetMapPoint().ToString().CString());
        return true;
    }

    const List<unsigned>& entities = World2D::GetEntities(GetMapPoint());

    MapData* mapdata = GetMapData();
    Vector<NodeAttributes >& entitiesAttr = mapdata->entitiesAttributes_;

    int& mcount = GetMapCounter(MAP_FUNC2);

    if (mcount == 0)
    {
        URHO3D_LOGINFOF("Map() - OnUpdateMapData map=%s ... entities=%u ...", GetMapPoint().ToString().CString(), entities.Size());

        entitiesAttr.Clear();
        if (!entities.Size())
        {
//			entitiesAttr.Clear();
            return true;
        }

        if (entitiesAttr.Capacity() < entities.Size())
            entitiesAttr.Reserve(entities.Size());

        mcount++;

        if (TimeOver(timer))
            return false;
    }

    if (mcount > 0)
    {
        unsigned nodeId, gotprops;
        Node* node;
        Scene* scene = node_->GetScene();

        List<unsigned>::ConstIterator startt = entities.Begin();
        {
            unsigned i = 1;
            while (startt != entities.End() && i < mcount)
            {
                startt++;
                i++;
            }
        }

        if (startt != entities.End())
        {
            for (List<unsigned>::ConstIterator it=startt; it != entities.End(); ++it)
            {
                nodeId = *it;

                mcount++;

                // Skip Save Player or Ally
                if (GOManager::IsA(nodeId, GO_Player | GO_AI_Ally))
                    continue;

                node = scene->GetNode(nodeId);
                if (!node)
                    continue;

                // Furnitures
#ifndef MAPDATA_SAVEFURNITURELIKEENTITIES
                gotprops = GOT::GetTypeProperties(node->GetVar(GOA::GOT).GetStringHash());
                if (gotprops & GOT_Furniture)
                {
                    // Update Entity Datas (Furnitures)
                    EntityData* entitydata = GetMapData()->GetEntityData(node);
                    if (entitydata)
                        GetMapData()->SetEntityData(node, entitydata, true);

                    // if usable furniture set GOA_EntityDataID
                    if (gotprops & GOT_Usable)
                        node->SetVar(GOA::ENTITYDATAID, GetMapData()->GetEntityDataID(entitydata));
                    // if not usable skip the entity attributes serialize
                    else
                        continue;
                }
#endif

                // Don't Update and Save Dead Entity
                if (node->GetVars().Contains(GOA::ISDEAD) && node->GetVar(GOA::ISDEAD).GetBool())
                {
                    URHO3D_LOGERRORF("Map() - OnUpdateMapData : Map=%s name=%s(%u) is dead => skip !",
                                     GetMapPoint().ToString().CString(), node->GetName().CString(), node->GetID());

//                    if (entitydata)
//                        GetMapData()->RemoveEntityData(node);

                    continue;
                }

                // Update Entity Attributes
                entitiesAttr.Resize(entitiesAttr.Size()+1);
                GameHelpers::SaveNodeAttributes(node, entitiesAttr.Back());

                if (TimeOver(timer))
                    return false;
            }

            mapdata->SetSection(MAPDATASECTION_ENTITYATTR, entitiesAttr.Size() > 0);
        }
    }

    mcount = 0;

    URHO3D_LOGINFOF("Map() - OnUpdateMapData : Map=%s ... entitiesAttr=%u ... OK !", GetMapPoint().ToString().CString(), entitiesAttr.Size());

    return true;
}



/// MAPS BORDER CONNECTERS

void Map::UpdateRenderShapeBorders()
{
    if (GameContext::Get().gameConfig_.renderShapes_)
    {
        const int emboseOrBorderBatchid = 1;
        unsigned numrenderviews = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
        for (unsigned i = 0; i < numrenderviews; i++)
        {
            if (renderColliders_[i].renderShape_)
                renderColliders_[i].renderShape_->SetMapBorderDirty(emboseOrBorderBatchid);
        }
    }
}

// TODO ASYNC
void Map::SetConnectedMap(int direction, Map* map)
{
    if (connectedMaps_[direction] != map)
    {
        connectedMaps_[direction] = map;

        Vector<FluidDatas>& fluidViews = featuredMap_->fluidView_;

        // Link the border FluidCells for this map
        if (map)
            for (unsigned i=0; i < fluidViews.Size(); i++)
                fluidViews[i].LinkBorderCells(direction, map->GetFluidDatas(i));
        else
            // Unlink the border FluidCells
            for (unsigned i=0; i < fluidViews.Size(); i++)
                fluidViews[i].UnLinkBorderCells(direction);

        // Update Map Borders
        if (map)
        {
#ifdef USE_TILERENDERING
            objectTiled_->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetDefaultChunkGroup(direction));
#endif
            UpdateRenderShapeBorders();
        }

        URHO3D_LOGINFOF("Map() - SetConnectedMap : mpoint=%s othermap=%s ... OK !", GetMapPoint().ToString().CString(), map ? map->GetMapPoint().ToString().CString() : "None");
    }
}

void Map::ClearConnectedMaps()
{
    URHO3D_LOGINFOF("Map() - ClearConnectedMaps : mpoint=%s ...", GetMapPoint().ToString().CString());

    if (connectedMaps_[MapDirection::North])
        connectedMaps_[MapDirection::North]->SetConnectedMap(MapDirection::South, 0);
    if (connectedMaps_[MapDirection::South])
        connectedMaps_[MapDirection::South]->SetConnectedMap(MapDirection::North, 0);
    if (connectedMaps_[MapDirection::East])
        connectedMaps_[MapDirection::East]->SetConnectedMap(MapDirection::West, 0);
    if (connectedMaps_[MapDirection::West])
        connectedMaps_[MapDirection::West]->SetConnectedMap(MapDirection::East, 0);

    for (int dir=0; dir < 4; dir++)
        connectedMaps_[dir] = 0;

    URHO3D_LOGINFOF("Map() - ClearConnectedMaps : mpoint=%s ... OK !", GetMapPoint().ToString().CString());
}

// Connects Borders on 2 maps
void Map::ConnectHorizontalMaps(Map* mapleft, Map* mapright)
{
    if (!mapleft || !mapright)
        return;

    // Connect Tiles
    if (!mapleft->GetConnectedMap(MapDirection::East) || !mapright->GetConnectedMap(MapDirection::West))
    {
        URHO3D_LOGINFOF("Map() - ConnectHorizontalMaps : mapleft=%s mapright=%s ...", mapleft->GetMapPoint().ToString().CString(), mapright->GetMapPoint().ToString().CString());

        bool updateObjects = false;

        // Update All Views in each Map matching the Zviews
        {
            TerrainAtlas* atlas = info_->atlas_;
            const Vector<int>& viewIds1 = mapleft->featuredMap_->GetViewIDs(FRONTVIEW);
            const Vector<int>& viewIds2 = mapright->featuredMap_->GetViewIDs(FRONTVIEW);
            int i1 = viewIds1.Size()-1;
            int i2 = viewIds2.Size()-1;

            // some infos
//            {
//                String s1, s2;
//                for (int i=i1; i >= 0; i--)
//                    s1 = s1 + " " + String(viewIds1[i]) + "(Z=" + String(mapleft->featuredMap_->GetViewZ(viewIds1[i])) + ")";
//                for (int i=i2; i >= 0; i--)
//                    s2 = s2 + " " + String(viewIds2[i]) + "(Z=" + String(mapright->featuredMap_->GetViewZ(viewIds2[i])) + ")";
//                URHO3D_LOGINFOF("Map() - ConnectHorizontalMaps : mapleft viewids=%s - mapright viewids=%s", s1.CString(), s2.CString());
//            }

            int viewZ1, viewZ2;
            unsigned addr1, addr2;

            // for each views
            for (;;)
            {
                if (i1 < 0 || i2 < 0)
                    break;

                viewZ1 = mapleft->featuredMap_->GetViewZ(viewIds1[i1]);
                viewZ2 = mapright->featuredMap_->GetViewZ(viewIds2[i2]);
                TiledMap& tiles1 = mapleft->GetTiledView(viewIds1[i1]);
                TiledMap& tiles2 = mapright->GetTiledView(viewIds2[i2]);
                ConnectedMap& connections1 = mapleft->GetConnectedView(viewIds1[i1]);
                ConnectedMap& connections2 = mapright->GetConnectedView(viewIds2[i2]);
                const FeaturedMap& features1 = mapleft->GetFeatureView(viewIds1[i1]);
                const FeaturedMap& features2 = mapright->GetFeatureView(viewIds2[i2]);

//                URHO3D_LOGINFOF("Map() - ConnectHorizontalMaps : connect (viewid1=%d viewZ1=%d) with (viewid2=%d viewZ2=%d)",
//                                viewIds1[i1], viewZ1, viewIds2[i2], viewZ2);

                for (int i=0; i < MapInfo::info.height_; i++)
                {
                    addr1 = addr2 = i * MapInfo::info.width_;
                    addr1 += MapInfo::info.width_-1;

                    const FeatureType& feature1 = features1[addr1];
                    const FeatureType& feature2 = features2[addr2];
                    ConnectIndex& connect1 = connections1[addr1];
                    ConnectIndex& connect2 = connections2[addr2];

                    updateObjects = feature1 > MapFeatureType::InnerSpace || feature2 > MapFeatureType::InnerSpace;

                    // Set Tile1 on Left Connected to right
                    if (feature1 > MapFeatureType::InnerSpace)
                    {
                        if (feature2 > MapFeatureType::InnerSpace)
                            connect1 = Tile::GetConnectIndex(Tile::GetConnectValue(connect1) | RightSide);
                        else
                            connect1 = Tile::GetConnectIndex(Tile::GetConnectValue(connect1) & ~RightSide);

                        //connect2 = (connect1 & RightSide) != 0 ? connect2 | LeftSide : connect2 & ~LeftSide;

                        Tile*& tile1 = tiles1[addr1];
                        if (tile1 && tile1->GetDimensions() == TILE_0)
                        {
                            tile1 = atlas->GetTile(tile1->GetTerrain(), connect1);
                        }

                        if (!tile1)
                        {
                            URHO3D_LOGERRORF("Map() - ConnectHorizontalMaps : map=%s leftmap tile error at addr=%u y=%d connectindex=%d !",
                                             mapleft->GetMapPoint().ToString().CString(), addr1, i, connect1);
                        }
                    }

                    // Set Tile2 on Right Connected to left
                    if (feature2 > MapFeatureType::InnerSpace)
                    {
                        if (feature1 > MapFeatureType::InnerSpace)
                            connect2 = Tile::GetConnectIndex(Tile::GetConnectValue(connect2) | LeftSide);
                        else
                            connect2 = Tile::GetConnectIndex(Tile::GetConnectValue(connect2) & ~LeftSide);

                        //connect1 = (connect2 & LeftSide) != 0 ? connect1 | RightSide : connect1 & ~RightSide;

                        Tile*& tile2 = tiles2[addr2];
                        if (tile2 && tile2->GetDimensions() == TILE_0)
                        {
                            tile2 = atlas->GetTile(tile2->GetTerrain(), connect2);
                        }

                        if (!tile2)
                        {
                            URHO3D_LOGERRORF("Map() - ConnectHorizontalMaps : map=%s rightmap tile error at addr=%u y=%d connectindex=%d !",
                                             mapright->GetMapPoint().ToString().CString(), addr2, i, connect2);
                        }
                    }
                }

                // get the next correct viewIds for matching with the zview
                if (viewZ1 == viewZ2)
                {
                    i1--;
                    i2--;
                }
                else if (viewZ1 > viewZ2)
                    i1--;
                else
                    i2--;
            }
        }
    }

    mapleft->SetConnectedMap(MapDirection::East, mapright);
    mapright->SetConnectedMap(MapDirection::West, mapleft);

    URHO3D_LOGINFOF("Map() - ConnectHorizontalMaps : ... mapleft=%s mapright=%s OK !", mapleft->GetMapPoint().ToString().CString(), mapright->GetMapPoint().ToString().CString());
}

void Map::ConnectVerticalMaps(Map* maptop, Map* mapbottom)
{
    if (!maptop || !mapbottom)
        return;

    // Connect Tiles
    if (!maptop->GetConnectedMap(MapDirection::South) || !mapbottom->GetConnectedMap(MapDirection::North))
    {
        URHO3D_LOGINFOF("Map() - ConnectVerticalMaps : maptop=%s mapbottom=%s ...", maptop->GetMapPoint().ToString().CString(), mapbottom->GetMapPoint().ToString().CString());

        bool updateObjects = false;

        // Update All Views in each Map matching the Zviews
        {
            TerrainAtlas* atlas = info_->atlas_;
            const Vector<int>& viewIds1 = maptop->featuredMap_->GetViewIDs(FRONTVIEW);
            const Vector<int>& viewIds2 = mapbottom->featuredMap_->GetViewIDs(FRONTVIEW);
            int i1 = viewIds1.Size()-1;
            int i2 = viewIds2.Size()-1;

            // some infos
//            {
//                String s1, s2;
//                for (int i=i1; i >= 0; i--)
//                    s1 = s1 + " " + String(viewIds1[i]) + "(Z=" + String(maptop->featuredMap_->GetViewZ(viewIds1[i])) + ")";
//                for (int i=i2; i >= 0; i--)
//                    s2 = s2 + " " + String(viewIds2[i]) + "(Z=" + String(mapbottom->featuredMap_->GetViewZ(viewIds2[i])) + ")";
//                URHO3D_LOGINFOF("Map() - ConnectVerticalMaps : maptop viewids=%s - mapbottom viewids=%s", s1.CString(), s2.CString());
//            }

            int viewZ1, viewZ2;
            unsigned addr;

            // for each views
            for (;;)
            {
                if (i1 < 0 || i2 < 0)
                    break;

                viewZ1 = maptop->featuredMap_->GetViewZ(viewIds1[i1]);
                viewZ2 = mapbottom->featuredMap_->GetViewZ(viewIds2[i2]);
                TiledMap& tiles1 = maptop->GetTiledView(viewIds1[i1]);
                TiledMap& tiles2 = mapbottom->GetTiledView(viewIds2[i2]);
                ConnectedMap& connections1 = maptop->GetConnectedView(viewIds1[i1]);
                ConnectedMap& connections2 = mapbottom->GetConnectedView(viewIds2[i2]);
                const FeaturedMap& features1 = maptop->GetFeatureView(viewIds1[i1]);
                const FeaturedMap& features2 = mapbottom->GetFeatureView(viewIds2[i2]);

//                URHO3D_LOGINFOF("Map() - ConnectVerticalMaps : connect (viewid1=%d viewZ1=%d) with (viewid2=%d viewZ2=%d)",
//                                viewIds1[i1], viewZ1, viewIds2[i2], viewZ2);

                addr = (MapInfo::info.height_-1) * MapInfo::info.width_;
                for (int i=0; i < MapInfo::info.width_; i++)
                {
                    const FeatureType& feature1 = features1[addr+i];
                    const FeatureType& feature2 = features2[i];
                    ConnectIndex& connect1 = connections1[addr+i];
                    ConnectIndex& connect2 = connections2[i];

                    updateObjects = feature1 > MapFeatureType::InnerSpace || feature2 > MapFeatureType::InnerSpace;

                    // Set Tile1 on Top Connected to bottom
                    if (feature2 > MapFeatureType::InnerSpace)
                    {
                        if (feature1 > MapFeatureType::InnerSpace)
                            connect1 = connect1 | BottomSide;

                        Tile*& tile1 = tiles1[addr+i];
                        if (tile1 && tile1->GetDimensions() == TILE_0)
                            tile1 = atlas->GetTile(tile1->GetTerrain(), connect1);

                        connect2 = (connect1 & BottomSide) != 0 ? connect2 | TopSide : connect2 & ~TopSide;

//                        URHO3D_LOGINFOF("maptop: i=%d connectIndex=%u gid=%d", i, connect1, tile1 ? tile1->GetGid() : 0);
                    }

                    // Set Tile2 on Bottom Connected to top
                    if (feature1 > MapFeatureType::InnerSpace)
                    {
                        if (feature2 > MapFeatureType::InnerSpace)
                            connect2 = connect2 | TopSide;

                        Tile*& tile2 = tiles2[i];
                        if (tile2 && tile2->GetDimensions() == TILE_0)
                            tile2 = atlas->GetTile(tile2->GetTerrain(), connect2);

                        connect1 = (connect2 & TopSide) != 0 ? connect1 | BottomSide : connect1 & ~BottomSide;
//                        URHO3D_LOGINFOF("mapbottom: i=%d connectIndex=%u gid=%d", i, connect2, tile2 ? tile2->GetGid() : 0);
                    }
                }

                // get the next correct viewIds for matching with the zview
                if (viewZ1 == viewZ2)
                {
                    i1--;
                    i2--;
                }
                else if (viewZ1 > viewZ2)
                    i1--;
                else
                    i2--;
            }
        }
    }

    maptop->SetConnectedMap(MapDirection::South, mapbottom);
    mapbottom->SetConnectedMap(MapDirection::North, maptop);

    // Specific : Generate and Set Collision Shapes for Top Borders
    mapbottom->CreateTopBorderCollisionShapes();

    URHO3D_LOGINFOF("Map() - ConnectVerticalMaps : ... maptop=%s mapbottom=%s OK !", maptop->GetMapPoint().ToString().CString(), mapbottom->GetMapPoint().ToString().CString());
}



/// MAP VISIBILITY HANDLER

bool Map::SetVisibleStatic(bool visible, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_VisibleStatic);
#endif

    int numViews = colliderNumParams_[PHYSICCOLLIDERTYPE];

    Node* staticNode;

    int& mcount4 = GetMapCounter(MAP_FUNC4);

    for (;;)
    {
        if (mcount4 >= numViews)
            break;

        int& mcount5 = GetMapCounter(MAP_FUNC5);
        bool state;

//        if (GameContext::Get().createModeOn_ && (visible_ == MAP_VISIBLE || visible_ == MAP_CHANGETOVISIBLE))
//        {
//            /// In CreateMode : enable just the collisionShapes of the currentviewZ
//            const PhysicCollider& physicCollider = physicColliders_[mcount4];
//            state = physicCollider.params_->indz_ == viewManager_->GetCurrentViewZIndex() ? visible : !visible;
//        }
//        else
//        {
//            /// always set visibility in all viewz (for entities in differents viewz)
        state = visible;
//        }

        // initialize static nodes
        if (mcount5 == 0)
        {
            staticNode = nodeBoxes_[mcount4];
            if (staticNode)
            {
                staticNode->SetEnabledNodeOnly(state);
                sboxes_.Resize(0);
                staticNode->GetComponents<CollisionBox2D>(sboxes_);
            }

            staticNode = nodePlateforms_[mcount4];
            if (staticNode)
            {
                staticNode->SetEnabledNodeOnly(state);
                splateforms_.Resize(0);
                staticNode->GetComponents<CollisionBox2D>(splateforms_);
            }

            staticNode = nodeChains_[mcount4];
            if (staticNode)
            {
                staticNode->SetEnabledNodeOnly(state);
                schains_.Resize(0);
                staticNode->GetComponents<CollisionChain2D>(schains_);
            }

            GetMapCounter(MAP_FUNC6) = 0;
            mcount5++;
        }

        // boxes nodes
        if (mcount5 == 1)
        {
            staticNode = nodeBoxes_[mcount4];
            if (staticNode)
            {
#ifdef ACTIVE_WORLD2D_PROFILING
                URHO3D_PROFILE(Map_VisibleStatic_Boxes);
#endif

                int& mcount6 = GetMapCounter(MAP_FUNC6);
                while (mcount6 < sboxes_.Size())
                {
                    sboxes_[mcount6]->SetEnabled(state);
                    mcount6++;

                    if (TimeOver(timer))
                        return false;
                }
                mcount6 = 0;
            }

            mcount5++;
        }

        // plateforms nodes
        if (mcount5 == 2)
        {
            staticNode = staticNode = nodePlateforms_[mcount4];
            if (staticNode)
            {
#ifdef ACTIVE_WORLD2D_PROFILING
                URHO3D_PROFILE(Map_VisibleStatic_Plateforms);
#endif

                int& mcount6 = GetMapCounter(MAP_FUNC6);
                while (mcount6 < splateforms_.Size())
                {
                    splateforms_[mcount6]->SetEnabled(state);
                    mcount6++;

                    if (TimeOver(timer))
                        return false;
                }
                mcount6 = 0;
            }

            mcount5++;
        }

        // chains nodes
        if (mcount5 == 3)
        {
            staticNode = nodeChains_[mcount4];
            if (staticNode)
            {
#ifdef ACTIVE_WORLD2D_PROFILING
                URHO3D_PROFILE(Map_VisibleStatic_Chains);
#endif

                int& mcount6 = GetMapCounter(MAP_FUNC6);
                while (mcount6 < schains_.Size())
                {
                    schains_[mcount6]->SetEnabled(state);
                    mcount6++;

                    if (TimeOver(timer))
                        return false;
                }
                mcount6 = 0;
            }

            mcount5 = 0;
        }

        mcount4++;
    }

    mcount4 = 0;

    return true;
}

const int MAXNODESSETTEDVISIBLE = 3;

bool Map::SetVisibleEntities(bool visible, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_VisibleEntities);
#endif

#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
    int& mcount2 = GetMapCounter(MAP_FUNC2);
    const ShortIntVector2& mPoint = GetMapPoint();
    int status = GetStatus();

//    URHO3D_LOGINFOF("Map() - SetVisibleEntities : mPoint=%s visible=%s ...", mPoint.ToString().CString(), visible ? "show":"hide");

    MapEntityInfo& mapEntityInfo = World2D::GetWorld()->GetEntitiesInfo(mPoint);

    static unsigned numentitiessetted;

    if (mcount2 == 0)
    {
        if (mapEntityInfo.entitiesNode_[LOCAL])
            mapEntityInfo.entitiesNode_[LOCAL]->SetEnabled(visible);

        if (!GameContext::Get().ClientMode_ && mapEntityInfo.entitiesNode_[REPLICATED])
            mapEntityInfo.entitiesNode_[REPLICATED]->SetEnabled(visible);

        mcount2++;

        numentitiessetted=0;
    }

    if (mcount2 > 0)
    {
        Node* node;
        Scene* scene = node_->GetScene();
//        int countStart = mcount2;

        List<unsigned>& entities = mapEntityInfo.entities_;
        List<unsigned>::Iterator it = entities.Begin();
        {
            unsigned i = 0;
            while (it != entities.End() && i < mcount2-1)
            {
                it++;
                i++;
            }
        }

//        bool dynamicObjectFromServer;

        while (it != entities.End())
        {
            node = scene->GetNode(*it);

//            dynamicObjectFromServer = false;
//
//            if (GameContext::Get().ClientMode_ && visible && node)
//            {
//                // in ClientMode, erase servernodes that have dynamic body and inactive or idle ObjectControl
//                if (!GameNetwork::Get()->GetClientObjectControl(*it))
//                {
//                    RigidBody2D* body = node->GetComponent<RigidBody2D>();
//                    if (body && body->GetBodyType() == BT_DYNAMIC)
//                    {
//                        ObjectControlInfo* cserverinfo = GameNetwork::Get()->GetServerObjectControl(*it);
//
//                        dynamicObjectFromServer = !cserverinfo || (!cserverinfo->active_);// || !cserverinfo->IsEnable());
//                    }
//                }
//            }

            if (node)
            {
                // disable constraints of entity with rigidbody
//                {
//                    RigidBody2D* nodebody = node->GetComponent<RigidBody2D>();
//                    if (nodebody && nodebody->GetConstraints().Size())
//                    {
//                        const Vector<WeakPtr<Constraint2D> >& constraints = nodebody->GetConstraints();
//
//                        for (unsigned i=0; i < constraints.Size(); i++)
//                            constraints[i]->SetEnabled(false);
//
//                        URHO3D_LOGWARNINGF("Map() - SetVisibleEntities : mPoint=%s ... node=%s(%u) has %u constraints !",
//                                            mPoint.ToString().CString(), node->GetName().CString(), node->GetID(), constraints.Size());
//                    }
//                }

                numentitiessetted++;

                if (visible && node->GetVar(GOA::ISDEAD).GetBool())
                {
                    URHO3D_LOGERRORF("Map() - SetVisibleEntities : mPoint=%s visible=%s node=%s(%u) is already dead ... skip !", GetMapPoint().ToString().CString(), visible ? "show":"hide", node->GetName().CString(), node->GetID());
                }
                else
                {
                    if (node->GetVar(GOA::KEEPVISIBLE).GetBool())
                        visible = true;
                    node->SetEnabledRecursive(visible);
//                node->SetEnabledRecursive(visible && !dynamicObjectFromServer);

                    GameHelpers::UpdateLayering(node);
                }
            }

            if (!node)
//            if (!node || dynamicObjectFromServer)
            {
                URHO3D_LOGWARNINGF("Map() - SetVisibleEntities : mPoint=%s ... node=%s(%u) NOT IN SCENE OR SERVER => ERASE in World2D entities !",
                                   mPoint.ToString().CString(), node ? node->GetName().CString(): "none", *it, mPoint.ToString().CString());

                // Erase the nodeid before using Destroy, else "it" will be illformed and crash after next reading in the list!
                // In fact, GOC_Destroyer::Destroy send GO_DESTROY event to World2D then World2D remove "it" too.
                it = entities.Erase(it);

//                if (node && dynamicObjectFromServer)
//                    node->GetComponent<GOC_Destroyer>()->Destroy(0.f);

                continue;
            }

            it++;
            mcount2++;

            if (TimeOver(timer))
                return false;
        }

        mcount2 = 0;
    }

//    URHO3D_LOGINFOF("Map() - SetVisibleEntities : mPoint=%s visible=%s numentities=%u... OK !", GetMapPoint().ToString().CString(), visible ? "show":"hide", numentitiessetted);
#endif

    return true;
}

bool Map::SetVisibleTiles(bool visible, bool forced, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_SetVisibleTiles);
#endif

    if (!node_)
        return true;

    int& mcount2 = GetMapCounter(MAP_FUNC2);

    if (mcount2 == 0)
    {
//    #ifdef USE_TILERENDERING
//    #ifdef RENDER_OBJECTTILED
//        if (objectTiled_ && viewManager_)
//        {
//            URHO3D_LOGINFOF("Map() - SetVisibleTiles : map=%s visible=%u ...", GetMapPoint().ToString().CString(), visible);
//
//            int numviewports = ViewManager::Get()->GetNumViewports();
//            for (int viewport=0; viewport < numviewports; viewport++)
//            {
////                if (GetBounds().IsInside(World2D::GetExtendedVisibleRect(viewport)) != OUTSIDE)
//                {
//                    int viewZindex = viewManager_->GetCurrentViewZIndex(viewport);
//                    URHO3D_LOGINFOF("Map() - SetVisibleTiles : map=%s visible=%u ... viewport=%d viewZindex=%d viewZ=%d ...", GetMapPoint().ToString().CString(), visible, viewport, viewZindex, viewZindex != -1 ? viewManager_->GetViewZ(viewZindex) : -1);
//
//                    if (viewZindex == -1)
//                    {
//                        URHO3D_LOGERRORF("Map() - SetVisibleTiles : map=%s visible=%u ... viewport=%d viewZindex=-1 error !", GetMapPoint().ToString().CString(), visible, viewport);
//                    }
//
//                    objectTiled_->SetCurrentViewZ(viewport, viewZindex, forced);
//                }
//            }
//        }
//    #endif
//    #endif
        GetMapCounter(MAP_FUNC3) = 0;
        GetMapCounter(MAP_FUNC4) = 0;
        GetMapCounter(MAP_FUNC5) = 0;
        GetMapCounter(MAP_FUNC6) = 0;
        mcount2++;
    }

    if (mcount2 == 1)
    {
//        URHO3D_LOGINFOF("Map() - SetVisibleTiles : map=%s visible=%u ...", GetMapPoint().ToString().CString(), visible);

        if (!SetVisibleStatic(visible, timer))
            return false;

        GetMapCounter(MAP_FUNC3) = 0;
        mcount2++;
    }

    if (mcount2 == 2)
    {
//    #ifdef RENDER_OBJECTTILED
        node_->SetEnabled(visible);
//    #endif
        nodeStatic_->SetEnabled(visible);

#ifdef HANDLE_BACKGROUND_IMAGE
        {
            if (nodeImages_.Size() && viewManager_)
                nodeImages_.Back()->GetComponent<StaticSprite2D>()->SetColor(viewManager_->GetCurrentViewZ() == INNERVIEW ? Color::GRAY : Color::WHITE);

            int bgImageSize = nodeImages_.Size();
            for (int i=0; i < bgImageSize; i++)
                nodeImages_[i]->SetEnabled(visible);
        }
#endif

        if (GameContext::Get().gameConfig_.renderShapes_)
        {
            unsigned numrenderviews = Min(colliderNumParams_[RENDERCOLLIDERTYPE], renderColliders_.Size());
            for (unsigned i=0; i < numrenderviews; i++)
            {
                if (renderColliders_[i].node_)
                    renderColliders_[i].node_->SetEnabled(visible);
            }
        }

        if (!visible)
            nodeTag_->SetEnabledRecursive(visible);

        if (visible)
            SubscribeToEvent(viewManager_, VIEWMANAGER_SWITCHVIEW, URHO3D_HANDLER(Map, HandleChangeViewIndex));
        else
            UnsubscribeFromEvent(viewManager_, VIEWMANAGER_SWITCHVIEW);

        mcount2 = 0;

//        URHO3D_LOGINFOF("Map() - SetVisibleTiles : map=%s nodePtr=%u visible = %u OK !", GetMapPoint().ToString().CString(), node_.Get(), visible);
    }

    return true;
}

bool Map::UpdateVisibility(HiresTimer* timer)
{
    if (visible_ < MAP_CHANGETOVISIBLE)
        return true;

    int& mcount = GetMapCounter(MAP_VISIBILITY);

    if (visible_ == MAP_CHANGETOVISIBLE)
    {
        if (mcount == 0)
        {
            GetMapCounter(MAP_FUNC2) = 0;
            mcount++;
        }

        if (mcount == 1)
        {
            if (!SetVisibleTiles(true, false, timer))
                return false;

            mcount++;
            GetMapCounter(MAP_FUNC2) = 0;

            if (timer)
                return false;
        }

        if (mcount == 2)
        {
            if (!SetVisibleEntities(true, timer))
                return false;

            mcount = 0;
            GetMapCounter(MAP_FUNC2) = 0;

            visible_ = MAP_VISIBLE;
            World2D::GetWorld()->OnMapVisibleChanged(this);

//			URHO3D_LOGINFOF("Map() - UpdateVisibility : map=%s visible=true OK !", GetMapPoint().ToString().CString());
        }
    }
    else
    {
        if (mcount == 0)
        {
            World2D::GetWorld()->OnMapVisibleChanged(this);

            GetMapCounter(MAP_FUNC2) = 0;
            mcount++;
        }

        if (mcount == 1)
        {
            if (!SetVisibleEntities(false, timer))
                return false;

            mcount++;
            GetMapCounter(MAP_FUNC2) = 0;

            if (timer)
                return false;
        }

        if (mcount == 2)
        {
            if (!SetVisibleTiles(false, false, timer))
                return false;

            mcount = 0;
            GetMapCounter(MAP_FUNC2) = 0;

            visible_ = MAP_NOVISIBLE;

//			URHO3D_LOGINFOF("Map() - UpdateVisibility : map=%s visible=false OK !", GetMapPoint().ToString().CString());
        }
    }

    return true;
}


bool Map::Update(HiresTimer* timer)
{
    if (GetStatus() != Available)
        return true;

#ifdef USE_TILERENDERING
    if (!objectTiled_)
        return true;
#else
    if (!skinnedMap_)
        return true;
#endif

#ifdef USE_TILERENDERING
#ifdef RENDER_VIEWBATCHES
    if (!objectTiled_->UpdateDirtyChunks(timer, delayUpdateUsec_))
        return false;
#endif
#endif

    if (!UpdateVisibility(timer))
        return false;

    //
    if (IsEffectiveVisible())
    {
        ;
    }

    return true;
}

void Map::ShowMap(HiresTimer* timer)
{
    if (visible_ == MAP_VISIBLE || visible_ == MAP_CHANGETOVISIBLE)
        return;

//    URHO3D_LOGINFOF("Map() - ShowMap : map=%s ...", GetMapPoint().ToString().CString());

    visible_ = MAP_CHANGETOVISIBLE;
    GetMapCounter(MAP_VISIBILITY) = 0;
    GetMapCounter(MAP_FUNC2) = 0;

    if (!timer)
        Update(0);

    URHO3D_LOGINFOF("Map() - ShowMap : map=%s ... OK !", GetMapPoint().ToString().CString());
}

void Map::HideMap(HiresTimer* timer)
{
    if (visible_ == MAP_NOVISIBLE || visible_ == MAP_CHANGETONOVISIBLE)
        return;

    visible_ = MAP_CHANGETONOVISIBLE;
    GetMapCounter(MAP_VISIBILITY) = 0;
    GetMapCounter(MAP_FUNC2) = 0;

    if (!timer)
        UpdateVisibility();
}


void Map::HandleChangeViewIndex(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("Map() - HandleChangeViewIndex on mPoint=%s !", GetMapPoint().ToString().CString());

#ifdef USE_TILERENDERING
    if (!objectTiled_)
        return;

    objectTiled_->SetCurrentViewZ(eventData[ViewManager_SwitchView::VIEWPORT].GetInt(),
                                  eventData[ViewManager_SwitchView::VIEWZINDEX].GetInt(),
                                  eventData[ViewManager_SwitchView::FORCEDMODE].GetBool());
#endif // USE_TILERENDERING

//    /// In CreateMode : enable just the collisionShapes of the currentviewZ
//    if (GameContext::Get().createModeOn_ && (visible_ == MAP_VISIBLE || visible_ == MAP_CHANGETOVISIBLE))
//        SetVisibleStatic(true);

#ifdef HANDLE_BACKGROUND_IMAGE
    if (nodeImages_.Size())
        nodeImages_.Back()->GetComponent<StaticSprite2D>()->SetColor(eventData[ViewManager_SwitchView::VIEWZ].GetInt() == INNERVIEW ? Color::GRAY : Color::WHITE);
#endif
}



/// ENTITIES SETTERS

Node* Map::AddEntity(const StringHash& got, int entityid, int id, unsigned holderid, int viewZ, const PhysicEntityInfo& physicInfo, const SceneEntityInfo& sceneInfo, VariantMap* slotData, bool outsidePool)
{
#ifdef HANDLE_ENTITIES

//    URHO3D_LOGINFOF("Map() - AddEntity : mPoint=%s type=%s(%u) at %f %f on viewZ=%d mapNode=%s(%u) slotData=%u ...",
//                    GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value(), physicInfo.positionx_, physicInfo.positiony_, viewZ,
//                    attachNode->GetName().CString(), attachNode->GetID(), slotData);

    ObjectPoolCategory* category = 0;
    Node* attachNode = sceneInfo.attachNode_ ? sceneInfo.attachNode_ : World2D::GetEntitiesRootNode(GameContext::Get().rootScene_, GameNetwork::Get() ? REPLICATED : LOCAL);

    ObjectPool::SetForceLocalMode(false);

    Node* node = ObjectPool::Get() ? ObjectPool::CreateChildIn(got, entityid, attachNode, id, viewZ, 0, false, &category, outsidePool) : 0;
    if (!node)
    {
        URHO3D_LOGERRORF("Map() - AddEntity : mPoint=%s type=%s(%u) ObjectPool Error !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
        return 0;
    }

    // AdjustPosition Test : RockGolem OK - Lance NOK
    // Use AdjustPosition in SetEntities_Add,
    // AjustPosition is not for spawning Objects like Fire or Lance that need to be destroyed if in wall and not to be adjusted in position
    /*
        Vector2 position(physicInfo.positionx_, physicInfo.positiony_);

    	GOC_Destroyer* gocDestroyer = node->GetComponent<GOC_Destroyer>();
    	if (gocDestroyer)
    	{
    		gocDestroyer->AdjustPosition(viewZ, position);

    		// Never Spawn in a Wall
    		if (gocDestroyer->IsInWalls(this, viewZ))
    		{
    			URHO3D_LOGERRORF("Map() - AddEntity : mPoint=%s type=%s(%u) spawn in a wall ! destroy !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
    			gocDestroyer->Destroy(0.1f);
    			return 0;
    		}
        }
    */

    bool physicOk = GameHelpers::SetPhysicProperties(node, physicInfo, true);
    GOC_Destroyer* gocDestroyer = node->GetComponent<GOC_Destroyer>();

//    URHO3D_LOGINFOF("Map() - AddEntity : mPoint=%s type=%s(%u) spawn at %s slotdata=%u ...",
//             GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value(), node->GetWorldPosition2D().ToString().CString(), slotData);

    // Never Spawn in a Wall
    if (gocDestroyer)
    {
        if (!gocDestroyer->AllowWallSpawning() && gocDestroyer->IsInWalls(this, viewZ))
        {
            if (gocDestroyer->GetCheckUnstuck())
            {
                gocDestroyer->AdjustPositionInTile(viewZ);
                if (gocDestroyer->IsInWalls(this, viewZ))
                {
                    URHO3D_LOGERRORF("Map() - AddEntity : mPoint=%s type=%s(%u) spawn in a wall ! destroy !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
                    gocDestroyer->Destroy(0.1f);
                    return 0;
                }
            }
            else
            {
                URHO3D_LOGERRORF("Map() - AddEntity : mPoint=%s type=%s(%u) spawn in a wall ! destroy !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
                gocDestroyer->Destroy(0.1f);
                return 0;
            }
        }
    }
    GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
    if (animator)
        animator->SetOwner(sceneInfo.ownerNode_);

    GameHelpers::SetCollectableProperties(node->GetComponent<GOC_Collectable>(), got, slotData);
    GameHelpers::SetEquipmentList(node->GetComponent<AnimatedSprite2D>(), sceneInfo.equipment_);

    GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
    if (controller)
    {
        unsigned localclientid = GameNetwork::Get() ? GameNetwork::Get()->GetClientID() : 0;
        controller->SetMainController(GameContext::Get().LocalMode_ ||
                                      (GameContext::Get().ClientMode_ && sceneInfo.faction_ == ((localclientid << 8) + GO_Player)) ||
                                      (GameContext::Get().ServerMode_ && ((sceneInfo.faction_ >> 8) == 0)));

//        URHO3D_LOGINFOF("Map() - AddEntity : mPoint=%s modeid=%d nodeid=%u type=%s(%u) entityid=%d at %s viewZ=%d zindex=%d maincontrol=%s ... OK !",
//                    GetMapPoint().ToString().CString(), id, node->GetID(), GOT::GetType(got).CString(), got.Value(), entityid, node->GetWorldPosition2D().ToString().CString(),
//                    viewZ, sceneInfo.zindex_, controller->IsMainController() ? "true":"false");
    }

    node->SetEnabledRecursive(IsVisible());

    if (gocDestroyer)
    {
        if (viewZ != NOVIEW)
            gocDestroyer->SetViewZ(viewZ, viewManager_->layerMask_[viewZ], sceneInfo.zindex_);

        if (!sceneInfo.deferredAdd_)
        {
            gocDestroyer->Reset(true);
            node->SendEvent(WORLD_ENTITYCREATE);
        }
    }
    else
    {
//        if (viewZ != NOVIEW && node->GetDerivedComponent<Drawable2D>())
//        {
//            Drawable2D* drawable = node->GetDerivedComponent<Drawable2D>();
//            drawable->SetLayer2(IntVector2(viewZ + LAYER_ACTOR, BACKACTORVIEW));
//            drawable->SetOrderInLayer(sceneInfo.zindex_);
////            drawable->SetViewMask(viewManager_->viewMask_[viewZ]);
//            drawable->SetViewMask(viewManager_->layerMask_[viewZ]);
////            URHO3D_LOGINFOF("Map() - AddEntity : mPoint=%s type=%s(%u) at %s viewZ=%d zindex=%d dlayer=%d, dorder=%d dvmask=%u scale=%s... OK !",
////                                GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value(), node->GetWorldPosition2D().ToString().CString(),
////                                viewZ, sceneInfo.zindex_, drawable->GetLayer(), drawable->GetOrderInLayer(), drawable->GetViewMask(), node->GetWorldScale2D().ToString().CString());
//        }
        if (!sceneInfo.deferredAdd_)
        {
            unsigned mpointhashed = GetMapPoint().ToHash();
            if ((GOT::GetTypeProperties(got) & GOT_Effect) != 0)  // BUG patch ObjectPool 11/10/2017
            {
                VariantMap& eventData = context_->GetEventDataMap();
                eventData[Go_Appear::GO_ID] = node->GetID();
                eventData[Go_Appear::GO_TYPE] = node->GetVar(GOA::TYPECONTROLLER) != Variant::EMPTY ? node->GetVar(GOA::TYPECONTROLLER).GetInt() : GO_None;
                eventData[Go_Appear::GO_MAP] = mpointhashed;
                eventData[Go_Appear::GO_MAINCONTROL] = controller ? controller->IsMainController() : false;
                node->SendEvent(GO_APPEAR, eventData);
            }
            node->SetVar(GOA::ONMAP, mpointhashed);
        }
    }

    GameHelpers::UpdateLayering(node, viewZ);

    node->SetVar(GOA::FACTION, sceneInfo.faction_);

    bool netusage = category && category->HasReplicatedMode() && !GameContext::Get().LocalMode_;
    if (netusage)
    {
        if (id == LOCAL && !sceneInfo.objectControlInfo_)
        {
            Node* holder = GameContext::Get().rootScene_->GetNode(holderid);
            ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node, holder, true, true, !sceneInfo.skipNetSpawn_);
        }
    }

    // URHO3D_LOGINFOF("Map() - AddEntity : mPoint=%s modeid=%d nodeid=%u type=%s(%u) entityid=%d at %s viewZ=%d zindex=%d netusage=%s... OK !",
    //                 GetMapPoint().ToString().CString(), id, node->GetID(), GOT::GetType(got).CString(), got.Value(), entityid, node->GetWorldPosition2D().ToString().CString(),
    //                 viewZ, sceneInfo.zindex_, netusage ? "true":"false");

//    mapData_->AddEntityData(node);

    ObjectPool::SetForceLocalMode(true);

    return node;
#else
    return 0;
#endif // HANDLE_ENTITIES
}

bool Map::SetEntities_Load(HiresTimer* timer)
{
#ifdef HANDLE_ENTITIES
    int& mcount = GetMapCounter(MAP_GENERAL);

    if (!mapData_ || !mapData_->entitiesAttributes_.Size())
    {
//        mcount = 0;
//        SetStatus(mapStatus_.status_+1);
        return true;
    }

    URHO3D_LOGINFOF("Map() - SetEntities Load : Map=%s ... From Memory on nodeEntities = %s", GetMapPoint().ToString().CString(),
                    replicationMode_ ? replicatedEntitiesNode_->GetName().CString() : localEntitiesNode_->GetName().CString());

    unsigned i = mcount;
    unsigned gotprops;
    int entityDataId;
    StringHash got;

    Node* node;
    bool mapvisible = IsVisible();

    ObjectPool::SetForceLocalMode(false);

    for (;;)
    {
        if (i >= mapData_->entitiesAttributes_.Size())
            break;

        NodeAttributes& nodeAttributes = mapData_->entitiesAttributes_.At(i);
        if (!nodeAttributes.Size())
        {
            i++;
            continue;
        }

        const Variant& varNodeName = GameHelpers::GetNodeAttributeValue(StringHash("Name"), nodeAttributes);
        if (varNodeName == Variant::EMPTY)
        {
            URHO3D_LOGERRORF("Map() - SetEntities Load : Map=%s Entities[%d] : has no name ... skip !", GetMapPoint().ToString().CString(), i);
            i++;
            continue;
        }

//        URHO3D_LOGINFOF(" ... Load %d/%d name=%s", i+1, mapData_->entitiesAttributes_->Size(), varNodeName.GetString().CString());

        got = StringHash(varNodeName.GetString());
        gotprops = GOT::GetTypeProperties(got);
        if ((gotprops & GOT_UsableFurniture) == GOT_UsableFurniture)
        {
            const VariantMap& vars = GameHelpers::GetNodeAttributeValue(StringHash("Variables"), nodeAttributes, 0).GetVariantMap();
            VariantMap::ConstIterator it = vars.Find(GOA::ENTITYDATAID);
            if (it == vars.End())
            {
                URHO3D_LOGERRORF("Map() - SetEntities Load : Map=%s Entities[%d] : usable furniture=%s ... no furnitureDataID => skip !",
                                 GetMapPoint().ToString().CString(), i, GOT::GetType(got).CString());
                i++;
                continue;
            }

            entityDataId = it->second_.GetInt();
            if (entityDataId >= mapData_->furnitures_.Size())
            {
                URHO3D_LOGERRORF("Map() - SetEntities Load : Map=%s Entities[%d] : usable furniture=%s ... entityDataId=%d > furnitures_.Size()=%u ... break !",
                                 GetMapPoint().ToString().CString(), i, GOT::GetType(got).CString(), entityDataId, mapData_->furnitures_.Size());
                break;
            }
        }

        ObjectPoolCategory* category = 0;
        Node* attachNode = World2D::GetEntitiesRootNode(GameContext::Get().rootScene_, GameNetwork::Get() ? REPLICATED : LOCAL);

        /// TODO : 28/06/2022 21/01/2023 - need to correct the problem of serialization in AnimatedSprite2D with charactermapping
        /// so RandomMapping again only after the ApplyAttributes (because of AnimatedSprite2D::SetAppliedCharacterMapsAttr())
        /// remove this only when corrected !
        int entityid = RandomMappingFlag;
        Node* node = ObjectPool::Get() ? ObjectPool::CreateChildIn(got, entityid, attachNode, 0, NOVIEW, &nodeAttributes, false, &category) : 0;
        if (node)
        {
            URHO3D_LOGINFOF("Map() - SetEntities Load : Map=%s Entities[%d] : name=%s(%u) position=%s enabled=%s... OK !",
                            GetMapPoint().ToString().CString(), i, node->GetName().CString(), node->GetID(), node->GetWorldPosition2D().ToString().CString(), node->IsEnabled() ? "true":"false");

            GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
            if (controller)
                controller->SetMainController(GameContext::Get().ClientMode_ ? false : true);

            {
                const Variant& bosszone = node->GetVar(GOA::BOSSZONEID);
                if (bosszone != Variant::EMPTY)
                {
                    const IntVector3& zone = bosszone.GetIntVector3();
                    if (zone.x_ == GetMapPoint().x_ && zone.y_ == GetMapPoint().y_)
                    {
                        unsigned& zonenodeid = mapData_->zones_[zone.z_].nodeid_;

                        // Check if has already a boss
                        if (zonenodeid != 0 && zonenodeid != node->GetID())
                        {
                            Node* checknode = GameContext::Get().rootScene_->GetNode(zonenodeid);
                            if (checknode && checknode->GetVar(GOA::BOSSZONEID) != Variant::EMPTY)
                            {
                                URHO3D_LOGERRORF("Map() - SetEntities Load : Map=%s Entities[%d] : name=%s(%u) is a boss and have already a boss=%s(%u) in the zone=%s ... Remove it !",
                                                 GetMapPoint().ToString().CString(), i, node->GetName().CString(), node->GetID(), checknode->GetName().CString(), checknode->GetID(), zone.ToString().CString());

                                nodeAttributes.Clear();
                                node->GetComponent<GOC_Destroyer>()->Destroy(0.1f);
                                i++;
                                continue;
                            }
                        }

                        zonenodeid = node->GetID();
                    }
                }
            }

            if ((gotprops & GOT_UsableFurniture) == GOT_UsableFurniture)
            {
                EntityData& entitydata = mapData_->furnitures_[entityDataId];
                mapData_->AddEntityData(node, &entitydata, false);

                if (node->GetComponent<GOC_Destroyer>())
                {
                    if (gotprops & GOT_Static)
                    {
                        VariantMap& eventData = node->GetContext()->GetEventDataMap();
                        eventData[Go_Appear::GO_MAP] = GetMapPoint().ToHash();
                        eventData[Go_Appear::GO_TILE] = entitydata.tileindex_;
                        node->SendEvent(WORLD_ENTITYCREATE, eventData);
                    }
                    else
                    {
                        node->SendEvent(WORLD_ENTITYCREATE);
                    }
                }

                if (gotprops & GOT_Static)
                    World2D::AddStaticFurniture(GetMapPoint(), node, entitydata);

//                node->ApplyAttributes();

//                URHO3D_LOGERRORF("Map() - SetEntities Load : Map=%s Entities[%d] : name=%s(%u) is an usable furniture !",
//                                 GetMapPoint().ToString().CString(), i, node->GetName().CString(), node->GetID());
            }
            else
            {
                GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
                if (destroyer)
                {
                    node->SendEvent(WORLD_ENTITYCREATE);
                }
                else
                {
                    World2D::AddEntity(GetMapPoint(), node->GetID());
                }

//                mapData_->AddEntityData(node, &mapData_->entities_[i]);
            }

            if (category && category->HasReplicatedMode())
                ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node, 0, true);

            node->ApplyAttributes();
            node->SetEnabledRecursive(mapvisible);

            GameHelpers::UpdateLayering(node);

            // force update batch for staticsprite
            StaticSprite2D* sprite = node->GetComponent<StaticSprite2D>();
            if (sprite)
                sprite->ForceUpdateBatches();
        }
        else
        {
            URHO3D_LOGERRORF("Map() - SetEntities Load : Map=%s type=%s(%u) ObjectPool Error !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
//            mapData_->RemoveEntityData(&mapData_->entities_[i], false);
        }

        i++;

        if (TimeOver(timer))
        {
//            URHO3D_LOGINFOF("Map() - SetEntities Load ... timer =%d ms ...", timer->GetUSec(false)/1000);
            mcount = i;
            ObjectPool::SetForceLocalMode(true);
            return false;
        }
    }

    if (timer)
        URHO3D_LOGINFOF("Map() - SetEntities Load : Map=%s ... timer =%d ms ... OK !", GetMapPoint().ToString().CString(), timer->GetUSec(false)/1000);
    else
        URHO3D_LOGINFOF("Map() - SetEntities Load : Map=%s ... OK !", GetMapPoint().ToString().CString());

    ObjectPool::SetForceLocalMode(true);
    mcount = 0;
#endif
    return true;
}

bool Map::SetEntities_Add(HiresTimer* timer)
{
#ifdef HANDLE_ENTITIES
    if (!mapData_)
        return true;

//    URHO3D_LOGINFOF("Map() - SetEntities Add ... ");

    int& mcount = GetMapCounter(MAP_GENERAL);

    // Add Initial Entities
    Vector2 position;

    unsigned i = mcount;
    int viewZ;
    bool mapvisible = IsVisible();
    Node* node;

    ObjectPool::SetForceLocalMode(false);

    for (;;)
    {
        if (i >= mapData_->entities_.Size())
            break;

        EntityData& entitydata = mapData_->entities_[i];
        if (entitydata.gotindex_ == 0 || entitydata.gotindex_ >= GOT::GetSize())
        {
            i++;
            continue;
        }

        StringHash got(GOT::Get(entitydata.gotindex_));

        if (entitydata.IsBoss())
        {
            URHO3D_LOGINFOF("Map() - SetEntities Add : mPoint=%s Entities[%d] : type=%s(%u) skip boss entityid=%u ...",
                            GetMapPoint().ToString().CString(), i, GOT::GetType(got).CString(), got.Value(), entitydata.sstype_);
            i++;
            continue;
        }

        viewZ = ViewManager::GetLayerZ(entitydata.drawableprops_ & FlagLayerZIndex_);

//        URHO3D_LOGINFOF("Map() - SetEntities Add : mPoint=%s Entities[%d] : type=%s(%u)",
//                GetMapPoint().ToString().CString(), i, GOT::GetType(got).CString(), got.Value());

        ObjectPoolCategory* category = 0;
        Node* attachNode = World2D::GetEntitiesRootNode(GameContext::Get().rootScene_, GameNetwork::Get() ? REPLICATED : LOCAL);

        int entityid = entitydata.sstype_;
        Node* node = ObjectPool::Get() ? ObjectPool::CreateChildIn(got, entityid, attachNode, 0, viewZ, 0, false, &category) : 0;
        if (!node)
        {
            URHO3D_LOGERRORF("Map() - SetEntities_Add : mPoint=%s type=%s(%u) ObjectPool Error !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
//            mapData_->RemoveEntityData(&entitydata, false);
        }
        else
        {
            GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();

            position = GetWorldTilePosition(GetTileCoords(entitydata.tileindex_), entitydata.GetNormalizedPositionInTile());

//            if (destroyer)
//                destroyer->AdjustPosition(GetMapPoint(), viewZ, position);

            bool physicOk = GameHelpers::SetPhysicProperties(node, PhysicEntityInfo(position.x_, position.y_), true);

            // Never Spawn in a Wall
            if (destroyer)
            {
                if (destroyer->IsInWalls(this, viewZ))
                {
                    URHO3D_LOGERRORF("Map() - SetEntities_Add : mPoint=%s type=%s(%u) spawn in a wall ! destroy !", GetMapPoint().ToString().CString(), GOT::GetType(got).CString(), got.Value());
                    destroyer->Destroy(0.1f);
                    i++;
                    continue;
                }
            }

            GameHelpers::SetCollectableProperties(node->GetComponent<GOC_Collectable>(), got, 0);

            URHO3D_LOGINFOF("Map() - SetEntities Add : Map=%s ... Entities[%d] : id=%u type=%s(%u) enabled=%s position=%s viewZ=%d parentNodePool=%s(%u)",
                            GetMapPoint().ToString().CString(), i, node->GetID(), GOT::GetType(got).CString(), got.Value(), node->IsEnabled() ? "true":"false",
                            position.ToString().CString(), viewZ, node->GetParent()->GetName().CString(), node->GetParent()->GetID());

            GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
            if (controller)
                controller->SetMainController(GameContext::Get().ClientMode_ ? false : true);

            if (category && category->HasReplicatedMode())
                ObjectControlInfo* cinfo = GameNetwork::Get()->AddSpawnControl(node, 0, true);

            node->SetEnabledRecursive(mapvisible);

            GameHelpers::UpdateLayering(node);

//            mapData_->AddEntityData(node, &entitydata, false);

            if (destroyer)
                node->SendEvent(WORLD_ENTITYCREATE);
            else
                World2D::AddEntity(GetMapPoint(), node->GetID());
        }

        i++;

        if (TimeOver(timer))
        {
//            URHO3D_LOGINFOF("Map() - SetEntities Add ... ! timer =%d", timer->GetUSec(false)/1000);
            mcount = i;
            ObjectPool::SetForceLocalMode(true);
            return false;
        }
    }

//    URHO3D_LOGINFOF("Map() - SetEntities Add ... OK !");

    ObjectPool::SetForceLocalMode(true);
    mcount = 0;
#endif
    return true;
}

bool Map::RemoveNodes(bool removeentities, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_RemoveNodes);
#endif

    int& mcount = GetMapCounter(MAP_FUNC2);

    if (mcount == 0)
    {
#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
        if (localEntitiesNode_ && removeentities)
        {
            /// TODO ASYNC
            Node* node;
            PODVector<Node*> children;
            localEntitiesNode_->GetChildren(children);
            URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... Remove Local Entities ... numEntities=%u ...", GetMapPoint().ToString().CString(), children.Size());

            for (PODVector<Node*>::ConstIterator it=children.Begin(); it != children.End(); ++it)
            {
                node = *it;
                if (GOManager::IsA(node->GetID(), GO_Player | GO_AI_Ally))
                    continue;

                if (!node)
                    continue;

                URHO3D_LOGINFOF("  ... To Clear node=%s(%u)", node->GetName().CString(), node->GetID());
                if (node->GetComponent<GOC_Destroyer>())
                    node->GetComponent<GOC_Destroyer>()->Destroy();
                else
                    ObjectPool::Free(node, true);
            }

            if (TimeOver(timer))
                URHO3D_LOGERRORF("Map() - RemoveNodes mPoint=%s ... localEntitiesNodesCleared ... timer=%d/%d msec",
                                 GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);
        }
#endif

        GetMapCounter(MAP_FUNC3) = 0;
        mcount++;

//        URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... localEntitiesNodesCleared ...OK !", GetMapPoint().ToString().CString());

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount == 1)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_PreRemoveEntities);
#endif

#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
        /// TODO : Il faut etre sur ici que World2D ne modifie pas la liste d'entité apres l'extraction avec GetFilteredEntities !
        if (removeentities)
            World2D::GetFilteredEntities(GetMapPoint(), removableEntities_, GO_Player | GO_AI_Ally);
#endif

        mcount++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount == 2)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_RemoveEntities);
#endif

#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
        if (removeentities)
        {
            URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... Remove Entities ... numRemainEntities=%u ...", GetMapPoint().ToString().CString(), removableEntities_.Size());

            while (removableEntities_.Size())
            {
                Node*& node = removableEntities_.Back();

                URHO3D_LOGINFOF("  ... To Clear node=%s(%u)", node->GetName().CString(), node->GetID());
                if (node->GetComponent<GOC_Destroyer>())
                    node->GetComponent<GOC_Destroyer>()->Destroy();
                else
                    ObjectPool::Free(node, true);

                removableEntities_.Pop();

                if (TimeOver(timer))
                    return false;
            }

            if (removableEntities_.Size())
                URHO3D_LOGERRORF("Map() - RemoveNodes mPoint=%s ... WorldEntitiesNodesCleared numRemainEntities=%u ... timer=%d/%d msec",
                                 GetMapPoint().ToString().CString(), removableEntities_.Size(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

            World2D::PurgeEntities(GetMapPoint());
        }
#endif

        mcount++;

//        URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... WorldEntitiesNodesCleared ...OK !", GetMapPoint().ToString().CString());

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount == 3)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_RemoveSafe);
#endif

        if (node_)
            GameHelpers::RemoveNodeSafe(node_);

        URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... RemoveNodeSafe OK ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mcount++;

        if (TimeOverMaximized(timer))
            return false;
    }

    if (mcount == 4)
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(Map_RemoveStaticSafe);
#endif

        if (nodeStatic_)
            GameHelpers::RemoveNodeSafe(nodeStatic_);

        URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... NodeStatic Removed ... timer=%d/%d msec", GetMapPoint().ToString().CString(), timer ? timer->GetUSec(false) / 1000 : 0, delayUpdateUsec_/1000);

        mcount = 0;
    }

    URHO3D_LOGINFOF("Map() - RemoveNodes mPoint=%s ... OK !", GetMapPoint().ToString().CString());
    return true;
}



/// TILE MODIFIERS

void Map::OnTileModified(int x, int y)
{
    const ChunkGroup& chunkgroup = MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x, y));

#ifdef USE_TILERENDERING
    // Update chunk batch
    if (objectTiled_)
    {
        objectTiled_->MarkChunkGroupDirty(chunkgroup);

        // Update Chunks on border in connected maps
        if (x == 0 || x == MapInfo::info.width_-1 || y == 0 || y == MapInfo::info.height_-1)
        {
            // On Left Border
            if (x == 0 && GetConnectedMap(MapDirection::West))
            {
                GetConnectedMap(MapDirection::West)->GetObjectTiled()->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(MapInfo::info.width_-1, y)));
                GetConnectedMap(MapDirection::West)->UpdateRenderShapeBorders();
            }
            // On Right Border
            else if (x == MapInfo::info.width_-1 && GetConnectedMap(MapDirection::East))
            {
                GetConnectedMap(MapDirection::East)->GetObjectTiled()->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(0, y)));
                GetConnectedMap(MapDirection::East)->UpdateRenderShapeBorders();
            }
            // On Top Border
            if (y == 0 && GetConnectedMap(MapDirection::North))
            {
                GetConnectedMap(MapDirection::North)->GetObjectTiled()->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x, MapInfo::info.height_-1)));
                GetConnectedMap(MapDirection::North)->UpdateRenderShapeBorders();
            }
            // On Bottom Border
            else if (y == MapInfo::info.height_-1 && GetConnectedMap(MapDirection::South))
            {
                GetConnectedMap(MapDirection::South)->GetObjectTiled()->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x, 0)));
                GetConnectedMap(MapDirection::South)->UpdateRenderShapeBorders();
            }
        }

        // Update Chunks in neighborhood in the same map
        if (x+1 < MapInfo::info.width_)
        {
            if (y+1 < MapInfo::info.height_)
                objectTiled_->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x+1, y+1)));
            if (y > 0)
                objectTiled_->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x+1, y-1)));
        }
        if (x > 0)
        {
            if (y+1 < MapInfo::info.height_)
                objectTiled_->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x-1, y+1)));
            if (y > 0)
                objectTiled_->MarkChunkGroupDirty(MapInfo::info.chinfo_->GetUniqueChunkGroup(MapInfo::info.chinfo_->GetChunk(x-1, y-1)));
        }
    }
#endif

    // Update chunk maskViews
    /// already made by objectTiled_->MarkChunkGroupDirty->UpdateChunkGroup but need in instant here for updatecollider
    /// TODO : bypass objectTiled_->MarkChunkGroupDirty->UpdateChunkGroup->UpdateMaskViews
    featuredMap_->UpdateMaskViews(chunkgroup.GetTileGroup(), 0, 0, skinnedMap_->GetSkin() ? skinnedMap_->GetSkin()->neighborMode_ : Connected0);

    SetMiniMapAt(x, y);

    SendEvent(MAP_UPDATE);
}



/// BACKGROUND SETTERS

bool Map::AddBackGroundLayers(HiresTimer* timer)
{
    if (!info_->imageLayerResources_.Size())
    {
        URHO3D_LOGERRORF("Map() - AddBackGroundLayers : mPoint=%s mode=%s no imageLayerResources_ !", GetMapPoint().ToString().CString(), mapTopography_.flatmode_ ? "Flat":"Ellipsoid");
        return true;
    }

#ifdef HANDLE_BACKGROUND_IMAGE
    Node* node = node_->CreateChild("ImageLayer", LOCAL);
    if (!node)
        return true;

    nodeImages_.Push(node);

    const ResourceRef& resourceref = info_->imageLayerResources_[Random((int)info_->imageLayerResources_.Size())];
    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(context_, resourceref);
    if (!sprite)
        return true;

    Rect drawrect;
    if (!sprite->GetDrawRectangle(drawrect))
        return true;

    Vector2 size = drawrect.Size();

    node->SetWorldScale(Vector3(MapInfo::info.mWidth_/size.x_, MapInfo::info.mHeight_/size.y_, 0.f));
    node->SetPosition(Vector3::ZERO);
    node->SetEnabled(false);

    StaticSprite2D* background = node->CreateComponent<StaticSprite2D>(LOCAL);
    background->SetSprite(sprite);
    background->SetLayer2(IntVector2(BACKSCROLL_2,-1));
    background->SetHotSpot(Vector2(0.f, 0.f));
    background->SetUseHotSpot(true);
#endif

#ifdef HANDLE_BACKGROUND_LAYERS
    URHO3D_LOGINFOF("Map() - AddBackGroundLayers : mPoint=%s mode=%s", GetMapPoint().ToString().CString(), mapTopography_.flatmode_ ? "Flat":"Ellipsoid");

    // Flat World
    if (mapTopography_.flatmode_ && mapTopography_.HasNoFreeSides())
    {
        URHO3D_LOGINFOF("Map() - AddBackGroundLayers : mPoint=%s Flat world and Nofreesides", GetMapPoint().ToString().CString());
        return true;
    }

    // Ellipsoid World
    if (!mapTopography_.flatmode_ && mapTopography_.IsFullGround())
    {
        URHO3D_LOGINFOF("Map() - AddBackGroundLayers : mPoint=%s Ellipsoid world with FullGround", GetMapPoint().ToString().CString());
        return true;
    }

    ResourceCache* cache = GetSubsystem<ResourceCache>();

    DrawableScroller* scroller;
    Sprite2D *sprite;

    for (int viewport=0; viewport < MAX_VIEWPORTS; viewport++)
    {
        unsigned numscrollers = DrawableScroller::GetNumScrollers(viewport);
        for (int iscroller=0; iscroller < numscrollers; iscroller++)
        {
            scroller = DrawableScroller::GetScroller(viewport, iscroller);

            if (!scroller)
                continue;

            /// TODO : why this ?
            int numdrawables = 1;
            Vector<DrawableObjectInfo>* drawableptr = scroller->GetWorldScrollerObjectInfos(GetMapPoint());

            URHO3D_LOGINFOF("Map() - AddBackGroundLayers : map=%s viewport=%d drawablescroller=%d adding objects ... (%u ? %u)",
                            GetMapPoint().ToString().CString(), viewport, iscroller, numdrawables, drawableptr ? drawableptr->Size() : 0);

            if (!drawableptr || drawableptr->Size() != numdrawables)
            {
                int imageindex = Urho3D::Clamp((int)iscroller, 0, (int)info_->imageLayerResources_.Size()-1);

                Sprite2D* sprite = Sprite2D::LoadFromResourceRef(context_, ResourceRef(info_->imageLayerResources_[imageindex]));
                if (!sprite)
                    continue;

                sprite->SetHotSpot(info_->imageLayerHotSpots_[imageindex]);

                // use edge offset to prevent edge bleeding with texture atlas
                sprite->SetTextureEdgeOffset(Min(1.f, 1.f/GameContext::Get().dpiScale_));

                scroller->ResizeWorldScrollerObjectInfos(GetMapPoint(), numdrawables, sprite);
            }
        }
    }
#endif
    return true;
}



/// MINIMAP

inline void SetSquaredPixel(Image* image, int x, int y, unsigned color)
{
    for (int j=y*MINIMAP_PIXBYTILE; j <(y+1)*MINIMAP_PIXBYTILE; j++)
        for (int i=x*MINIMAP_PIXBYTILE; i <(x+1)*MINIMAP_PIXBYTILE; i++)
            image->SetPixelInt(i, j, color);
}

bool Map::SetMiniMapLayer(int indexViewZ, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_MiniMap_Lay);
#endif

    int& mcount2 = GetMapCounter(MAP_FUNC2);

    const Vector<int>& viewids = featuredMap_->GetViewIDs(ViewManager::Get()->GetViewZ(indexViewZ));
    int viewid;

    // SPECIAL viewZ : INNERVIEW
    if (indexViewZ == 0)
    {
        for (;;)
        {
            viewid = viewids[Min(MiniMapSelectedViewIndex[indexViewZ][mcount2],viewids.Size()-1)];
            if (viewid == -1)
            {
                URHO3D_LOGERRORF("Map() - SetMiniMapLayer : map=%s ERROR ViewId = -1 !", GetMapPoint().ToString().CString());
            }
            else
            {
                FeatureType* features = GetFeatures(viewid);

                // clear last layer if the view is the same on the previous layer
                if (mcount2 == MINIMAP_NUMLAYERS-1)
                {
                    SharedPtr<Image>& image = miniMapLayers_[MINIMAP_NUMLAYERS * indexViewZ + mcount2];
                    for (int y = 0; y < MapInfo::info.height_; ++y)
                    {
                        for (int x = 0; x < MapInfo::info.width_; ++x)
                        {
                            SetSquaredPixel(image, x, y, MiniMapColorEmpty);
                        }
                    }

                    mcount2++;

                    if (TimeOver(timer))
                        return false;

                    continue;
                }

                // link background with backview (patch for dungeon in cave)
                // => don't set transparent
                else if (MiniMapSelectedViewIndex[indexViewZ][mcount2] == 0)
                {
                    SharedPtr<Image>& image = miniMapLayers_[MINIMAP_NUMLAYERS * indexViewZ + mcount2-1];

                    unsigned addr = 0;
                    for (int y = 0; y < MapInfo::info.height_; ++y)
                    {
                        for (int x = 0; x < MapInfo::info.width_; ++x, ++addr)
                        {
                            if (features[addr] > MapFeatureType::NoRender)
                                SetSquaredPixel(image, x, y, MiniMapColorsByLayer[mcount2]);
                        }
                    }
                }

                // simple case
                {
                    SharedPtr<Image>& image = miniMapLayers_[MINIMAP_NUMLAYERS * indexViewZ + mcount2];

                    unsigned addr = 0;
                    for (int y = 0; y < MapInfo::info.height_; ++y)
                    {
                        for (int x = 0; x < MapInfo::info.width_; ++x, ++addr)
                        {
                            SetSquaredPixel(image, x, y, (features[addr] > MapFeatureType::NoRender) ? MiniMapColorsByLayer[mcount2] : MiniMapColorEmpty);
                        }
                    }
                }
            }

            mcount2++;

            if (mcount2 >= MINIMAP_NUMLAYERS)
            {
                mcount2 = 0;
                return true;
            }

            if (TimeOver(timer))
                return false;
        }
    }
    // Other ViewZ (FRONTVIEW) : simple case
    else
    {
        for (;;)
        {
            SharedPtr<Image>& image = miniMapLayers_[MINIMAP_NUMLAYERS * indexViewZ + mcount2];
            viewid = viewids[Min(MiniMapSelectedViewIndex[indexViewZ][mcount2],viewids.Size()-1)];

            if (viewid > -1)
            {
                FeatureType* features = GetFeatures(viewid);

                unsigned addr = 0;
                for (int y = 0; y < MapInfo::info.height_; ++y)
                {
                    for (int x = 0; x < MapInfo::info.width_; ++x, ++addr)
                    {
                        SetSquaredPixel(image, x, y, (features[addr] > MapFeatureType::NoRender) ? MiniMapColorsByLayer[mcount2] : MiniMapColorEmpty);
                    }
                }
            }

            mcount2++;

            if (mcount2 >= MINIMAP_NUMLAYERS)
            {
                mcount2 = 0;
                return true;
            }

            if (TimeOver(timer))
                return false;
        }
    }

    return false;
}

bool Map::SetMiniMapImage(int indexViewZ, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_MiniMap_Img);
#endif

//    URHO3D_LOGINFOF("Map() - SetMiniMapImage ... ");

    int& l = GetMapCounter(MAP_FUNC2);
    int& y = GetMapCounter(MAP_FUNC3);

    Color color;

    Image& mapImage = *miniMap_[indexViewZ].Get();

    if (l == MINIMAP_NUMLAYERS && y == 0)
        mapImage.Clear(Color::TRANSPARENT);

    const Vector<Image* >& layerImgs = miniMapLayersByViewZIndex_[indexViewZ];

    while (l > 0)
    {
        const Image& layerimage = *(layerImgs[l-1]);
        while (y < mapImage.GetHeight())
        {
            for (int x = 0; x < mapImage.GetWidth(); ++x)
            {
                color = layerimage.GetPixel(x, y);
                if (color.a_ > 0.f)
                    mapImage.SetPixel(x, y, color);
            }
            ++y;

            if (TimeOver(timer))
                return false;
        }

        y = 0;
        l--;
    }

    l = MINIMAP_NUMLAYERS;

    return true;
}

bool Map::SetMiniMap(HiresTimer* timer)
{
//    URHO3D_LOGINFOF("Map() - SetMiniMap ...");
    if (!miniMapLayers_.Size() || !viewManager_)
        return true;

//    if (timer && timer->GetUSec(false) >= delayUpdateUsec_)
//        return false;

    int& mcount = GetMapCounter(MAP_GENERAL);

    if (mcount == 0)
    {
        GetMapCounter(MAP_FUNC1) = 0;
        GetMapCounter(MAP_FUNC2) = 0;
        mcount++;
    }

    if (mcount == 1)
    {
        int& mcount1 = GetMapCounter(MAP_FUNC1);

        while (mcount1 < viewManager_->GetNumViewZ())
        {
            if (!SetMiniMapLayer(mcount1, timer))
                return false;

            mcount1++;
        }

        mcount = 2;
        GetMapCounter(MAP_FUNC1) = 0;
        GetMapCounter(MAP_FUNC2) = MINIMAP_NUMLAYERS;
        GetMapCounter(MAP_FUNC3) = 0;
    }

    if (mcount == 2)
    {
        int& mcount1 = GetMapCounter(MAP_FUNC1);

        while (mcount1 < viewManager_->GetNumViewZ())
        {
            if (!SetMiniMapImage(mcount1, timer))
                return false;

            mcount1++;
        }

        GetMapCounter(MAP_FUNC1) = 0;
        GetMapCounter(MAP_FUNC2) = 0;
        GetMapCounter(MAP_FUNC3) = 0;
        mcount = 0;
    }

//    URHO3D_LOGINFOF("Map() - SetMiniMap ... OK !");

    return true;
}

void Map::SetMiniMapAt(int x, int y)
{
#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("Map() - SetMiniMapAt : map=%s x=%d y=%d ...", GetMapPoint().ToString().CString(), x, y);
#endif
    unsigned tileindex = GetTileIndex(x, y);
    FeatureType feature;

    // Update INNERVIEW layers
    {
        int viewid, selectedviewindex;
        const Vector<int>& viewids = featuredMap_->GetViewIDs(INNERVIEW);
        for (int ilayer=0; ilayer < MINIMAP_NUMLAYERS; ilayer++)
        {
            selectedviewindex = Min(MiniMapSelectedViewIndex[ViewManager::INNERVIEW_Index][ilayer], viewids.Size()-1);
            viewid = viewids[selectedviewindex];
            if (viewid == -1)
                continue;

            feature = GetFeature(tileindex, viewid);

            // clear last layer if the view is the same on the previous layer
            if (ilayer == MINIMAP_NUMLAYERS-1)
                SetSquaredPixel(miniMapLayers_[ilayer], x, y, MiniMapColorEmpty);
            // link background with backview (patch for dungeon in cave)
            // => don't set transpŔarent
            else if (selectedviewindex == 0)
            {
                if (feature > MapFeatureType::NoRender)
                    SetSquaredPixel(miniMapLayers_[ilayer-1], x, y, MiniMapColorsByLayer[ilayer]);
            }
            // simple case
            {
                SetSquaredPixel(miniMapLayers_[ilayer], x, y, feature > MapFeatureType::NoRender ? MiniMapColorsByLayer[ilayer] : MiniMapColorEmpty);
            }
        }
    }

    // Update FRONTVIEW layers
    {
        int viewid;
        const Vector<int>& viewids = featuredMap_->GetViewIDs(FRONTVIEW);
        for (int ilayer=0; ilayer < MINIMAP_NUMLAYERS; ilayer++)
        {
            int selectedviewindex = Min(MiniMapSelectedViewIndex[ViewManager::FRONTVIEW_Index][ilayer], viewids.Size()-1);

            viewid = viewids[selectedviewindex];

            if (viewid == -1)
                continue;

            feature = GetFeature(tileindex, viewid);

//            URHO3D_LOGWARNINGF("Map() - SetMiniMapAt : map=%s x=%d y=%d ... ilayer=%d viewid=%d feature=%s(%u)", GetMapPoint().ToString().CString(), x, y, ilayer, viewid, MapFeatureType::GetName(feature), feature);

            SetSquaredPixel(miniMapLayers_[MINIMAP_NUMLAYERS + ilayer], x, y, feature > MapFeatureType::NoRender ? MiniMapColorsByLayer[ilayer] : MiniMapColorEmpty);
        }
    }

    // Update Map Image
    for (int iviewz = 0; iviewz < 2 ; iviewz++)
    {
        const Vector<Image* >& layerImgs = miniMapLayersByViewZIndex_[iviewz];
        Image& mapImage = *miniMap_[iviewz].Get();
        Color color;
        bool emptyPixel = true;

        for (int ilayer = 0; ilayer < MINIMAP_NUMLAYERS; ilayer++)
        {
            const Image& layerimage = *(layerImgs[ilayer]);

            color = layerimage.GetPixel(x*MINIMAP_PIXBYTILE, y*MINIMAP_PIXBYTILE);
            if (color.a_ > 0.f)
            {
                for (int j = y*MINIMAP_PIXBYTILE; j < (y+1)*MINIMAP_PIXBYTILE; j++)
                    for (int i = x*MINIMAP_PIXBYTILE; i < (x+1)*MINIMAP_PIXBYTILE; i++)
                        mapImage.SetPixel(i, j, color);

                // find color, stop to fill image
                emptyPixel = false;
                break;
            }
        }

        // No Color, Clean the Pixels
        if (emptyPixel)
        {
            for (int j = y*MINIMAP_PIXBYTILE; j < (y+1)*MINIMAP_PIXBYTILE; j++)
                for (int i = x*MINIMAP_PIXBYTILE; i < (x+1)*MINIMAP_PIXBYTILE; i++)
                    mapImage.SetPixel(i, j, Color::TRANSPARENT);
        }
    }

#ifdef DUMP_MAPDEBUG_SETTILE
    URHO3D_LOGINFOF("Map() - SetMiniMapAt : map=%s x=%d y=%d ... OK !", GetMapPoint().ToString().CString(), x, y);
#endif
}

void Map::UpdateMiniMap(int indexViewZ)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(Map_MiniMap_Static);
#endif

    URHO3D_LOGERRORF("Map() - UpdateMiniMap : map=%s indexViewZ=%d !", GetMapPoint().ToString().CString(), indexViewZ);

    GetMapCounter(MAP_FUNC2) = 0;
    SetMiniMapLayer(indexViewZ);

    GetMapCounter(MAP_FUNC2) = MINIMAP_NUMLAYERS;
    GetMapCounter(MAP_FUNC3) = 0;
    SetMiniMapImage(indexViewZ);

    SendEvent(WORLD_MAPUPDATE);
}



/// GETTERS

const char* Map::GetVisibleState() const
{
    return MapVisibleStateNames_[visible_];
}

int Map::GetMaterialType(unsigned index, int indexZ) const
{
    return featuredMap_->GetFluidView(ViewManager::viewZIndex2fluidZIndex_[indexZ]).fluidmap_[index].type_;
}

bool Map::AllowEntities(int viewZ) const
{
    if (viewZ == 0)
        return info_ && info_->addObject_;
    else if (viewZ == FRONTVIEW)
        return !mapTopography_.IsFullGround();
    else
        return !mapTopography_.IsFullSky();
}

bool Map::AllowFurnitures(int viewZ) const
{
    return info_ && info_->addFurniture_;
}

int Map::GetGid(unsigned index, int viewId) const
{
    return InGetGid(index, viewId);
}

int Map::GetGid(int x, int y, int viewId) const
{
    return InGetGid(x, y, viewId);
}

int Map::GetGidAtZ(unsigned index, int viewZ) const
{
    return InGetGid(index, GetViewId(viewZ));
}

unsigned Map::GetNumNeighborTiles(int x, int y, int viewId, unsigned properties)
{
    unsigned numNeighbors = 0;

    int xi, yi;
    short int mapdx, mapdy;
    Map* map;

    for (int imoore=0; imoore<8; imoore++)
    {
        // init position
        map = this;
        xi = x + MapInfo::neighborOffX[imoore];
        yi = y + MapInfo::neighborOffY[imoore];

        // bound Test
        mapdx = 0;
        mapdy = 0;
        if      (xi < 0)
        {
            mapdx = -1;
            xi = MapInfo::info.width_-1;
        }
        else if (xi >= MapInfo::info.width_)
        {
            mapdx = 1;
            xi = 0;
        }
        if      (yi < 0)
        {
            mapdy = 1;
            yi = MapInfo::info.height_-1;
        }
        else if (yi >= MapInfo::info.height_)
        {
            mapdy = -1;
            yi = 0;
        }

        // get map if out of bound map

        if (mapdx != 0 || mapdy != 0)
        {
#ifdef USE_WORLD2D
            map = World2D::GetMapAt(map->GetWorldPoint() + ShortIntVector2(mapdx, mapdy));
#else
            map = 0;
#endif
        }

        if (!map)
            continue;

        if (map->HasTileProperties(xi, yi, viewId, properties))
            numNeighbors++;
    }

    return numNeighbors;
}

Rect Map::GetTileRect(const IntVector2& coords) const
{
    assert(node_);

    Rect rect;
//    rect.min_ = node_->GetWorldPosition2D() + Vector2(x * MapInfo::info.tileWidth_, (MapInfo::info.height_ - y - 1)  * MapInfo::info.tileHeight_) * node_->GetWorldScale2D();
//    rect.max_ = rect.min_ + 2.f * MapInfo::info.tileHalfSize_ * node_->GetWorldScale2D();
    Vector2 pos = GetWorldTilePosition(coords);
    Vector2 halfsize(World2D::GetWorldInfo()->mTileWidth_/2, World2D::GetWorldInfo()->mTileHeight_/2);
    rect.min_ = pos + halfsize;
    rect.max_ = pos - halfsize;
    return rect;
}

Rect Map::GetTileRect(unsigned index) const
{
    return GetTileRect(GetTileCoords(index));
}


