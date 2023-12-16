#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/IO/FileSystem.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Octree.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/Renderer2D.h>

#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionChain2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"
#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameNetwork.h"

#include "GOC_Animator2D.h"
#include "GOC_PhysicRope.h"
#include "GOC_Destroyer.h"
#include "GOC_Controller.h"
#include "GOC_Move2D.h"
#include "GEF_Scrolling.h"
#include "WaterLayer.h"

#include "ObjectPool.h"
#include "TimerRemover.h"
#include "Actor.h"
#include "MAN_Go.h"

#include "Map.h"
#include "ObjectMaped.h"
#include "MapPool.h"
#include "MapStorage.h"
#include "MapEditor.h"
#include "MapCreator.h"
#include "MapGenerator.h"
#include "MapSimulatorLiquid.h"
#include "ViewManager.h"

#include "MapWorld.h"


#define MAP_MEMORYBUFFERSPREAD_DEFAULT 1


#define MAP_GENERATORSEED_DEFAULT 1049564U

#define NET_MAXTILEGAP 10.f

//#define MAP_NOUPDATE_VISIBILITY

static const char* colliderShapeTypeModes[] =
{
    "All",
    "Boxes",
    "Chains",
    "Edges",
    0
};


WorldViewInfo::WorldViewInfo() : camera_(0), currentMap_(0), visibleCollideBorder_(0), cameraFocusEnabled_(true), visibleMapArea_(-1000, -1000, -1000, -1000), mPoint_(-1000, -1000), needUpdateCurrentMap_(false) { }

void WorldViewInfo::Clear()
{
    camera_ = 0;
    currentMap_ = 0;
    visibleCollideBorder_ = 0;

    cameraFocusEnabled_ = true;
    dMapPoint_ = ShortIntVector2::ZERO;
    extVisibleRect_ = Rect::ZERO;
    visibleMapArea_ = IntRect(-1000, -1000, -1000, -1000);
    tiledVisibleRect_.Clear();

    needUpdateCurrentMap_ = false;

    visibleAreaMaps_.Reserve(36);
    mapsToShow_.Reserve(36);
    mapsToHide_.Reserve(36);
}

float World2D::mWidth_ = 0.f;
float World2D::mHeight_ = 0.f;
float World2D::mTileWidth_ = 0.f;
float World2D::mTileHeight_ = 0.f;

World2D* World2D::world_ = 0;
World2DInfo* World2D::info_ = 0;
MapStorage* World2D::mapStorage_ = 0;

Text* World2D::world2DDebugPoolText_ = 0;

IntVector2 World2D::worldPoint_ = IntVector2::ZERO;
IntRect World2D::worldBounds_;
Rect World2D::worldFloatBounds_;
IntVector2 World2D::worldSize_;
bool World2D::worldMapUpdate_ = true;
bool World2D::noBounds_ = true;
Vector2 World2D::focusPosition_ = Vector2::ZERO;
Rect World2D::extVisibleRectCached_;

Vector<Map*> World2D::effectiveVisibleMaps_[MAX_VIEWPORTS];
Vector<ShortIntVector2> World2D::keepedVisibleMaps_;
HashMap<ShortIntVector2, MapEntityInfo> World2D::mapEntities_;
HashMap<ShortIntVector2, MapFurnitureLocation> World2D::mapFurnitures_;
WeakPtr<Node> World2D::entitiesRootNodes_[2];


void World2D::RegisterObject(Context* context)
{
    context->RegisterFactory<World2D>();
    URHO3D_ACCESSOR_ATTRIBUTE("WorldPoint", GetWorldPoint, SetWorldPoint, IntVector2, IntVector2(-1,-1), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("WorldSize", GetWorldSize, SetWorldSize, IntVector2, IntVector2(-1,-1), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("ChunkNum", GetChunkNum, SetChunkNum, IntVector2, IntVector2(4,4), AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Default Map Point", GetDefaultMapPoint, SetDefaultMapPoint, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Collider - Force Shape Type", GetColliderForceShapeTypeAttr, SetColliderForceShapeTypeAttr, bool, false, AM_FILE);
    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Collider - Shape Type", GetColliderShapeTypeAttr, SetColliderShapeTypeAttr, ColliderShapeTypeMode, colliderShapeTypeModes, SHT_ALL, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Model", GetAnlWorldModelAttr, SetAnlWorldModelAttr, String, String::EMPTY, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Map - TileSize", GetWorldMapSize, SetWorldMapSize, IntVector2, IntVector2(-1,-1), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Seed", GetGeneratorSeed, SetGeneratorSeed, unsigned, 0, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Type", GetMapGeneratorAttr, SetMapGeneratorAttr, String, String("MapGeneratorRandom"), AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Params", GetMapGeneratorParams, SetMapGeneratorParams, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Num Entities", GetGeneratorNumEntities, SetGeneratorNumEntities, IntVector2, IntVector2::ZERO, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Authorized Categories", GetGeneratorAuthorizedCategories, SetGeneratorAuthorizedCategories, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Ground Level", GetMapGroundLevelAttr, SetMapGroundLevelAttr, int, 50, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Add Objects", GetMapAddObjectAttr, SetMapAddObjectAttr, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Add Furnitures", GetMapAddFurnitureAttr, SetMapAddFurnitureAttr, bool, true, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Add Border", GetMapAddBorderAttr, SetMapAddBorderAttr, bool, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Add ImageLayer", GetMapAddImageLayerAttr, SetMapAddImageLayerAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Map - Add ImageLayers", GetMapAddImageLayersAttr, SetMapAddImageLayersAttr, StringVector, Variant::emptyStringVector, AM_FILE);
}


World2D::World2D(Context* context) :
    Component(context),
    generatorSeed_(MAP_GENERATORSEED_DEFAULT),
    addBorder_(false),
    allowClearMaps_(true),
    isSet_(false)
{
    URHO3D_LOGINFOF("World2D() - ...");

    world_ = this;
    mapStorage_ = 0;
    mapLiquid_ = 0;
    viewManager_ = 0;

    if (!world2DDebugPoolText_)
    {
        UI* ui = GetSubsystem<UI>();
        if (ui)
        {
            UIElement* uiRoot = ui->GetRoot();
            world2DDebugPoolText_  = new Text(context_);
            world2DDebugPoolText_->SetAlignment(HA_LEFT, VA_BOTTOM);
            world2DDebugPoolText_->SetPriority(100);
            world2DDebugPoolText_->SetStyle("DebugHudText");
            world2DDebugPoolText_->SetVisible(false);
            uiRoot->AddChild(world2DDebugPoolText_);
        }
    }

    chunkNum_ = IntVector2(4, 4);

    SetWorldPoint(IntVector2::ZERO);
    SetWorldSize(IntVector2::ZERO);

    for (unsigned i=0; i < MAX_VIEWPORTS; i ++)
    {
        viewinfos_[i].viewport_ = i;
        viewinfos_[i].Clear();
    }

    URHO3D_LOGINFOF("World2D() - ... OK !");
}

World2D::~World2D()
{
    URHO3D_LOGINFOF("~World2D() - ...");

//	  Stop();

    if (mapStorage_)
    {
        delete mapStorage_;
        mapStorage_ = 0;
    }

    Map::Reset();
#ifdef HANDLE_BACKGROUND_LAYERS
    DrawableScroller::Reset();
#endif

    if (mapLiquid_)
    {
        delete mapLiquid_;
        mapLiquid_ = 0;
    }

    info_ = 0;
    world_ = 0;

    keepedVisibleMaps_.Clear();

    for (unsigned viewport=0; viewport<MAX_VIEWPORTS; viewport++)
        effectiveVisibleMaps_[viewport].Clear();
    mapEntities_.Clear();
    mapFurnitures_.Clear();
    entitiesRootNodes_[0].Reset();
    entitiesRootNodes_[1].Reset();

    Actor::RemoveActors();

    URHO3D_LOGINFOF("~World2D() - ... OK !");
}


/// Attributes Setters

void World2D::SetWorldPoint(const IntVector2& worldpoint)
{
    URHO3D_LOGINFOF("World2D() - SetWorldPoint ...");
//	  if (worldpoint != worldPoint_)
    {
        worldPoint_ = worldpoint;
        int index = MapStorage::GetWorldIndex(worldpoint);
        info_ = index != -1 ? &MapStorage::GetWorld2DInfo(index) : 0;

        // Set static Atlas : for MapGenerator
        if (info_)
        {
            World2DInfo::currentAtlas_ = info_->atlas_;
            info_->ClearImageLayer();
        }
    }

    URHO3D_LOGINFOF("World2D() - SetWorldPoint ... OK ! worldPoint=%s name=%s radius=%F",
                    worldPoint_.ToString().CString(), MapStorage::GetWorldName(worldPoint_).CString(), info_ && info_->worldModel_ ? info_->worldModel_->GetRadius() : 0.f);
}

void World2D::SetWorldSize(const IntVector2& size)
{
    URHO3D_LOGINFOF("World2D() - SetWorldSize size=%s ...", size.ToString().CString());

    if (size == IntVector2::ZERO)
    {
        // infinite world size
        worldSize_ = IntVector2::ZERO;
        worldFloatBounds_.Clear();
        noBounds_ = true;
        URHO3D_LOGINFOF("World2D() - SetWorldSize : infinite world !");
        return;
    }

    worldSize_ = size;
    noBounds_ = false;
}

void World2D::SetWorldMapSize(const IntVector2& size)
{
    URHO3D_LOGINFOF("World2D() - SetWorldMapSize ...");
    if (info_->mapWidth_ != size.x_ || info_->mapHeight_ != size.y_)
    {
        info_->SetMapSize(size.x_, size.y_);
    }
    URHO3D_LOGINFOF("World2D() - SetWorldMapSize ... OK ! mapsize=%d %d", info_->mapWidth_, info_->mapHeight_);
}

void World2D::SetChunkNum(const IntVector2& chunkNum)
{
    URHO3D_LOGINFOF("World2D() - SetChunkNum ...");
    if (chunkNum != chunkNum_)
    {
        chunkNum_ = chunkNum;
    }
    URHO3D_LOGINFOF("World2D() - SetChunkNum ... OK ! chunkNum=%s", chunkNum_.ToString().CString());
}

void World2D::SetDefaultMapPoint(const ShortIntVector2& mPoint, bool checkInsideWorld) // TODO : viewport
{
    if (mPoint != viewinfos_[0].dMapPoint_)
    {
        if (!checkInsideWorld || (checkInsideWorld && IsInsideWorldBounds(mPoint)))
        {
            viewinfos_[0].dMapPoint_ = mPoint;
            URHO3D_LOGINFOF("World2D() - SetDefaultMapPoint ... OK ! dMapPoint_=%s", viewinfos_[0].dMapPoint_.ToString().CString());
        }
    }
}

void World2D::SetDefaultMapPoint(const IntVector2& mPoint) // TODO : viewport
{
    if (GameContext::Get().testZoneCustomDefaultMap_.x_ != 65535)
        SetDefaultMapPoint(ShortIntVector2(GameContext::Get().testZoneCustomDefaultMap_.x_, GameContext::Get().testZoneCustomDefaultMap_.y_), false);
    else
        SetDefaultMapPoint(ShortIntVector2(mPoint.x_, mPoint.y_), false);
}

void World2D::SetDefaultMapPoint(const Vector2& point) // TODO : viewport
{
    ShortIntVector2 mPoint;
    info_->Convert2WorldMapPoint(point, mPoint);
    SetDefaultMapPoint(mPoint);
}

void World2D::SetMapGroundLevelAttr(int level)
{
    if (info_->simpleGroundLevel_ != level)
    {
        info_->simpleGroundLevel_ = level;
        URHO3D_LOGINFOF("World2D() - SetMapGroundLevelAttr : grlevel = %d", level);
    }
}

void World2D::SetColliderForceShapeTypeAttr(bool force)
{
    if (info_->forcedShapeType_ != force)
        info_->forcedShapeType_ = force;
}

void World2D::SetColliderShapeTypeAttr(ColliderShapeTypeMode stype)
{
    if (info_->shapeType_ != stype)
        info_->shapeType_ = stype;
}

void World2D::SetMapAddObjectAttr(bool addObject)
{
    if (info_->addObject_ != addObject)
        info_->addObject_ = addObject;
}

void World2D::SetMapAddFurnitureAttr(bool addFurniture)
{
    if (info_->addFurniture_ != addFurniture)
        info_->addFurniture_ = addFurniture;
}

void World2D::SetMapAddBorderAttr(bool addBorder)
{
    if (addBorder_ != addBorder)
        addBorder_ = addBorder;
}

void World2D::SetMapAddImageLayerAttr(const String& textureinfo)
{
    imageLayersInfos_.Push(textureinfo);

    Vector<String> vars = textureinfo.Split(';');
    info_->imageLayerResources_.Push(ResourceRef(vars[0], vars[1]));
    info_->imageLayerHotSpots_.Push(vars.Size() > 2 ? ToVector2(vars[2]) : Vector2(0.5f, 0.5f));
}

void World2D::SetMapAddImageLayersAttr(const StringVector& textureinfos)
{
    for (unsigned i=0; i < textureinfos.Size(); i ++)
    {
        Vector<String> vars = textureinfos[i].Split(';');
        info_->imageLayerResources_.Push(ResourceRef(vars[0], vars[1]));
        info_->imageLayerHotSpots_.Push(vars.Size() > 2 ? ToVector2(vars[2]) : Vector2(0.5f, 0.5f));
    }

    imageLayersInfos_ = textureinfos;
}

void World2D::SetAnlWorldModelAttr(const String& modelfile)
{
    if (info_->worldModelFile_ != modelfile || !info_->worldModel_)
    {
        if (!modelfile.Empty())
        {
            URHO3D_LOGINFO("World2D() - SetAnlWorldModelAttr : file = " + modelfile);

            if (modelfile == "CUSTOM")
            {
                SetWorldPoint(IntVector2(0, 6));
                SetGeneratorSeed(GameContext::Get().testZoneCustomSeed_);
                SetDefaultMapPoint(GameContext::Get().testZoneCustomDefaultMap_);
//				info_->worldModel_ = context_->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(GameContext::Get().testZoneCustomModel_);
//				info_->worldModel_->SetRadius(GameContext::Get().testZoneCustomSize_);
//				info_->worldModel_->SetSeedAllModules(GameContext::Get().testZoneCustomSeed_);
            }
            else
            {
                info_->worldModelFile_ = modelfile;
                info_->worldModel_ = context_->GetSubsystem<ResourceCache>()->GetResource<AnlWorldModel>(World2DInfo::ATLASSETDIR + modelfile);
                SetDefaultMapPoint(GameContext::Get().testZoneCustomDefaultMap_);
            }

            URHO3D_LOGINFOF("World2D() - SetAnlWorldModelAttr : radius=%F ", info_->worldModel_ ? info_->worldModel_->GetRadius() : 0.f);
        }
        else
        {
            URHO3D_LOGINFO("World2D() - SetAnlWorldModelAttr : clear model !");

            info_->worldModelFile_ = String::EMPTY;
            info_->worldModel_.Reset();
        }
    }
}

void World2D::SetMapGeneratorAttr(const String& gen)
{
    unsigned type = StringHash(gen).Value();

    if (info_->defaultGenerator_ != type)
        info_->defaultGenerator_ = type;
}

void World2D::SetMapGeneratorParams(const String& params)
{
    genParams_ = params;

    if (info_)
        info_->ParseParams(params);
}

void World2D::SetGeneratorNumEntities(const IntVector2& numEntities)
{
    generatorNumEntities_ = numEntities;
    if (MapStorage::GetCreator())
        MapStorage::GetCreator()->SetNumEntities(numEntities);
}

void World2D::SetGeneratorAuthorizedCategories(const String& value)
{
    generatorAuthorizedCategories_ = value;
    if (MapStorage::GetCreator())
        MapStorage::GetCreator()->SetAuthorizedCategories(value);
}

void World2D::SetGeneratorSeed(unsigned seed)
{
    generatorSeed_ = seed;

    if (mapStorage_)
    {
        mapStorage_->SetMapSeed(seed);
        generatorSeed_ = mapStorage_->GetMapSeed();
    }

    URHO3D_LOGINFOF("World2D() - SetGeneratorSeed : seed = %u (%u) !", generatorSeed_, seed);
}

void World2D::SetGeneratorRandomSeed()
{
    SetGeneratorSeed(0);
}


/// Setters

void World2D::SetUpdateLoadingDelay(int delay)
{
    World2DInfo::delayUpdateUsec_ = delay;
    URHO3D_LOGINFOF("World2D() - SetUpdateLoadingDelay : delay = %d", World2DInfo::delayUpdateUsec_);
}

void World2D::SetInfo(World2DInfo* info)
{
    info_ = info;
    GameContext::Get().NetMaxDistance_ = NET_MAXTILEGAP * info_->mTileWidth_;
}

float viewSpanFactor = 1.5f;

void World2D::SetCamera(float zoom, const Vector2& focus)
{
    if (!viewManager_)
    {
        URHO3D_LOGERRORF("World2D() - SetCamera ... NOK !");
        return;
    }

    URHO3D_LOGINFOF("World2D() - SetCamera ...");

    viewManager_->SetScene(GetNode()->GetScene(), focus);
    viewManager_->GetCamera(0)->SetZoom(zoom);

    OnViewportUpdated();

    WorldViewInfo& vinfo = viewinfos_[0];

    URHO3D_LOGINFOF("World2D() - SetCamera ... viewport=%d ortho=%f aspectratio=%f ratio=%f ... OK !", 0, vinfo.camOrtho_, vinfo.camera_->GetAspectRatio(), vinfo.camRatio_);
}

void World2D::ResetPositionFocus()
{
    focusPosition_ = Vector2::ZERO;
}

void World2D::SavePositionFocus()
{
    focusPosition_ = ViewManager::Get()->GetCameraNode()->GetWorldPosition2D();
}

void World2D::ResetPosition(const Vector2& focus)
{
    ShortIntVector2 newmPoint;
    int viewport = 0;

    WorldViewInfo& vinfo = viewinfos_[viewport];

    // Reset before first world update (for setting visible areas)
    vinfo.extVisibleRect_ = Rect::ZERO;

    if (focus == Vector2::ZERO)
    {
        newmPoint = vinfo.mPoint_ = vinfo.dMapPoint_;
        vinfo.mPoint_.x_++;
        info_->Convert2WorldPosition(newmPoint, IntVector2(info_->mapWidth_/2, info_->mapHeight_/2), vinfo.dMapPosition_);
        URHO3D_LOGINFOF("World2D() - ResetPosition on Default Map Point = %s !", vinfo.dMapPoint_.ToString().CString());
    }
    else
    {
        vinfo.dMapPosition_ = focus;

        info_->Convert2WorldMapPoint(focus, newmPoint);
        vinfo.dMapPoint_ = vinfo.mPoint_ = newmPoint;
        vinfo.mPoint_.x_++;

        ResetPositionFocus();

        URHO3D_LOGINFOF("World2D() - ResetPosition on Map Point = %s !", newmPoint.ToString().CString());
    }

    mapStorage_->SetBufferedArea(viewport, newmPoint);

    allowClearMaps_ = true;
}

void World2D::ReinitWorld(const IntVector2& wPoint)
{
    MapStorage::DeleteWorldFiles(wPoint);
    MapStorage::CopyInitialWorldFiles(wPoint);
}

void World2D::ReinitAllWorlds()
{
    const Vector<IntVector2>& world2DPoints = MapStorage::GetAllWorldPoints();
    for (Vector<IntVector2>::ConstIterator it=world2DPoints.Begin(); it!=world2DPoints.End(); ++it)
    {
        ReinitWorld(*it);
    }
}

void World2D::LoadActors()
{
    String filename;
    String json = "/actors" + String(GetGeneratorSeed()) + ".json";

    const String& worldname = MapStorage::GetWorldName(GetWorldPoint());

#ifdef ACTIVE_CREATEMODE
    if (MapEditor::Get() && GameContext::Get().createModeOn_)
        filename = "Levels/" + worldname + json;
#endif

    if (filename.Empty())
        filename = MapStorage::GetWorldDirName(worldname) + json;

    Actor::LoadJSONFile(filename);
}

void World2D::SaveActors()
{
    String filename;
    String json = "/actors" + String(GetGeneratorSeed()) + ".json";

    const String& worldname = MapStorage::GetWorldName(GetWorldPoint());

#ifdef ACTIVE_CREATEMODE
    if (MapEditor::Get() && GameContext::Get().createModeOn_)
        filename = "Levels/" + worldname + json;
#endif

    if (filename.Empty())
        filename = MapStorage::GetWorldDirName(worldname) + json;

    if (!Actor::SaveJSONFile(filename))
    {
        bool del = GameContext::Get().context_->GetSubsystem<FileSystem>()->Delete(filename);
        URHO3D_LOGERRORF("World2D() - SaveActors : JsonFile=%s deleted !", filename.CString());
    }
}

void World2D::SaveWorld(bool saveEntities)
{
    if (!world_ || !world_->mapStorage_)
        return;

    URHO3D_LOGINFOF("World2D() - SaveWorld : ... %s entities !", saveEntities ? "with": "without");

    // Save Maps
    world_->mapStorage_->SaveMaps(saveEntities, false);

    // Save Actors
    if (saveEntities)
        world_->SaveActors();

    // Transfer Files To Local Save Directory
    MapStorage::SaveWorldFiles(world_->GetWorldPoint());

//	  world_->DumpEntitiesInMemory();

    URHO3D_LOGINFO("World2D() - SaveWorld : ... OK !");
}

//const Vector2 WorldEllipseParallax(0.095f, 0.f);
const Vector2 WorldEllipseParallax(0.f, 0.f);

void World2D::Set(bool load)
{
    URHO3D_LOGINFOF("World2D() - Set ...");

    worldObjectState_.stamp_ = 0U;

    // Sanitate wPoint, Register info_ if need or get new worldInfo
    worldPoint_ = MapStorage::CheckWorld2DPoint(context_, worldPoint_, info_);

    // Get the corresponding registered World2DInfo
    int index = MapStorage::GetWorldIndex(worldPoint_);
    info_ = index != -1 ? &MapStorage::GetWorld2DInfo(index) : 0;
    if (!info_)
        URHO3D_LOGWARNINGF("World2D() - Set : no World2DInfo for worldPoint_=%s", worldPoint_.ToString().CString());

//	MapStorage::DumpRegisterWorldPath();

    // Reglage des bounds avec prise en compte du scale du node !!!
    const Vector3& scale = GetNode()->GetWorldScale();

    info_->Update(scale);

    mWidth_ = info_->mWidth_;
    mHeight_ = info_->mHeight_;
    mTileWidth_ = info_->mTileWidth_;
    mTileHeight_ = info_->mTileHeight_;

    info_->ConvertMapRect2WorldRect(IntRect::ZERO, originMapRect_);

    UpdateWorldBounds();

    if (noBounds_)
    {
        worldMapUpdate_ = true;
    }
    else
    {
        if (addBorder_)
        {
            URHO3D_LOGINFOF("World2D() - Set ... Add Border ...");
//            RigidBody2D* borderbody = GetScene()->GetChild("StaticScene")->CreateComponent<RigidBody2D>();
//            CollisionChain2D* border = GetScene()->GetChild("StaticScene")->CreateComponent<CollisionChain2D>();
            RigidBody2D* borderbody = node_->GetOrCreateComponent<RigidBody2D>(LOCAL);
            borderbody->SetTemporary(true);
            CollisionChain2D* border = node_->GetOrCreateComponent<CollisionChain2D>(LOCAL);
            border->SetLoop(true);
            border->SetVertexCount(4);
            border->SetVertex(0, Vector2(worldFloatBounds_.min_.x_/scale.x_, (worldFloatBounds_.min_.y_)/scale.y_));
            border->SetVertex(1, Vector2(worldFloatBounds_.min_.x_/scale.x_, (worldFloatBounds_.max_.y_)/scale.y_));
            border->SetVertex(2, Vector2(worldFloatBounds_.max_.x_/scale.x_, (worldFloatBounds_.max_.y_)/scale.y_));
            border->SetVertex(3, Vector2(worldFloatBounds_.max_.x_/scale.x_, (worldFloatBounds_.min_.y_)/scale.y_));
            border->SetFilterBits(CC_MAPBORDER, CM_MAPBORDER);
            border->SetViewZ(0);
            border->SetColliderInfo(WALLCOLLIDER);
            border->SetTemporary(true);
            URHO3D_LOGINFOF("World2D() - Set ... Add Border ... OK !");
        }
        // correct Width and Height in IntRect => +1
        worldMapUpdate_ = (worldBounds_.Width()+1 > 1 || worldBounds_.Height()+1 > 1);

        URHO3D_LOGINFOF("World2D() - Set : worldsize=%s WorldBounds=%s WorldFloatBounds=%s",
                        worldSize_.ToString().CString(), worldBounds_.ToString().CString(), worldFloatBounds_.ToString().CString());
    }

    // For the Editor (octree Raycast)
    if (noBounds_)
        GameContext::Get().octree_->SetSize(BoundingBox(Vector3(-info_->worldGround_.radius_.x_, -info_->worldGround_.radius_.y_, -5.f), Vector3(info_->worldGround_.radius_.x_, info_->worldGround_.radius_.y_, 5.f)), 16);
    else
        GameContext::Get().octree_->SetSize(BoundingBox(Vector3(worldFloatBounds_.min_.x_, worldFloatBounds_.min_.y_, -5.f), Vector3(worldFloatBounds_.max_.x_, worldFloatBounds_.max_.y_, 5.f)), 16);

    URHO3D_LOGINFOF("World2D() - Set : scale=%s worldMapUpdate_=%u mWidth_=%F mHeight_=%F mTileWidth_=%F mTileHeight_=%F",
                    scale.ToString().CString(), worldMapUpdate_, mWidth_, mHeight_, mTileWidth_, mTileHeight_);

    viewManager_ = ViewManager::Get();
    if (!viewManager_)
    {
        URHO3D_LOGERRORF("World2D() - Set : ViewManager Error !");
        assert(viewManager_);
    }

    ObjectMaped::Reset();

    Map::Initialize(info_, viewManager_, chunkNum_.x_, chunkNum_.y_);

    if (!mapStorage_)
        mapStorage_ = new MapStorage(context_, worldPoint_);

    mapStorage_->SetNode(GetNode());
    mapStorage_->SetMapBufferOffset(MAP_MEMORYBUFFERSPREAD_DEFAULT);

    if (GameContext::Get().gameConfig_.fluidEnabled_)
    {
        if (!mapLiquid_)
            mapLiquid_ = new MapSimulatorLiquid();
        MapSimulatorLiquid::SetSimulationMode(FLUID_SIMULATION, FLUID_ITERATIONS);
    }

    URHO3D_LOGINFOF("World2D() - Set : Defined ActiveFluid mapLiquid_=%u fluidenable_=%s debugFluid=%s (use fluidenable in cmdline to enable/disable)",
                    mapLiquid_, GameContext::Get().gameConfig_.fluidEnabled_ ? "true" :"false", GameContext::Get().gameConfig_.debugFluid_ ? "true" :"false");

#ifdef ACTIVE_WORLDELLIPSES
    // Add textured world Ellipse
    if (info_->worldModel_ && info_->worldModel_->GetRadius() != 0.f)
    {
//        Node* scrollShapeNode1 = GameContext::Get().rootScene_->GetChild("LocalScene")->CreateChild("WorldGround", LOCAL);
//        scrollShapeNode1->SetEnabled(false);
//        ScrollingShape* scrollshape1 = scrollShapeNode1->CreateComponent<ScrollingShape>();
//        scrollshape1->SetLayer2(IntVector2(BACKSCROLL_1, -1));
//        scrollshape1->SetOrderInLayer(0);
//        scrollshape1->SetViewMask(ViewManager::layerMask_[BACKGROUND]);
//        scrollshape1->SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/backrock.xml"));
//        scrollshape1->SetAlpha(1.f);
//        scrollshape1->SetParallax(Vector2(0.f, 0.f));
//        scrollshape1->SetColor(Color(0.35f, 0.35f, 0.35f, 1.f));
//        scrollshape1->SetNodeToFollow(GameContext::Get().cameraNode_);
//        scrollshape1->AddShape(info_->worldGround_.ellipseShape_, 0);
//        scrollShapeNode1->SetEnabled(true);

        Node* scrollShapeNode2 = GameContext::Get().rootScene_->GetChild("LocalScene")->CreateChild("WorldHillTop", LOCAL);
        scrollShapeNode2->SetEnabled(false);
        ScrollingShape* scrollshape2 = scrollShapeNode2->CreateComponent<ScrollingShape>();
        scrollshape2->SetLayer2(IntVector2(BACKSCROLL_2,-1));
        scrollshape2->SetOrderInLayer(8);
        scrollshape2->SetViewMask(BACKGROUND_MASK);
#ifdef ACTIVE_LAYERMATERIALS
//        scrollshape2->SetMaterial(GameContext::Get().layerMaterials_[LAYERRENDERSHAPES], 0);
        scrollshape2->SetMaterial(GameContext::Get().layerMaterials_[LAYERBACKGROUNDS], 0);
        // tileindex x=1 y=1 (backrock)
        scrollshape2->SetTextureFX(BACKROCK);
#else
        scrollshape2->SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/backrock.xml"));
#endif
        scrollshape2->SetAlpha(1.f);
        scrollshape2->SetParallax(WorldEllipseParallax);
        scrollshape2->SetColor(Color(0.55f, 0.55f, 0.45f, 1.f));
//        scrollshape2->SetColor(0.5f * Color(0.314f, 0.176f, 0.086f, 2.f));
        scrollshape2->SetNodeToFollow(GameContext::Get().cameraNode_);
        scrollshape2->AddShape(info_->worldHillTop_.ellipseShape_, 0);
        scrollshape2->SetClipping(true);
        scrollShapeNode2->SetEnabled(true);
    }
#endif

#ifdef HANDLE_BACKGROUND_LAYERS
    DrawableScroller::Reset();
    for (int viewport=0; viewport < MAX_VIEWPORTS; viewport++)
        AddScrollers(viewport);
#endif

    SetGeneratorSeed(generatorSeed_);
    SetGeneratorNumEntities(generatorNumEntities_);
    SetGeneratorAuthorizedCategories(generatorAuthorizedCategories_);

    URHO3D_LOGINFOF("World2D() - Set ... OK !");

    UpdateTextureLevels();

    isSet_ = true;

    // always purge world file before load if need
    MapStorage::DeleteWorldFiles(GetWorldPoint());

    // transfer the saved map files in the world directory
    if (load)
        MapStorage::LoadWorldFiles(GetWorldPoint());
    else
        MapStorage::CopyInitialWorldFiles(GetWorldPoint());

    // Load Actors
    if (info_->addObject_)
        LoadActors();
}

void World2D::AddScrollers(int viewport)
{
    Material* material = 0;

//    URHO3D_LOGINFOF("World2D() - AddScrollers on viewport=%d !", viewport);

#ifdef ACTIVE_LAYERMATERIALS
    material = GameContext::Get().layerMaterials_[LAYERBACKGROUNDS];
#endif

    EllipseW* boundcurve = 0;

#ifdef ACTIVE_WORLDELLIPSES
    boundcurve = &info_->worldHillTop_;
#endif

    // ForeGround 1
    DrawableScroller::AddScroller(viewport, material, LIT, BACKSCROLL_2, 11, 2.f*Vector2::ONE, Vector2(0.f, 0.f), Vector2(0.0f, 0.0f), 0, false, false, Vector2::ZERO, Color(0.75f, 0.65f, 0.45f, 1.f), false);
    // ForeGround 2
    DrawableScroller::AddScroller(viewport, material, LIT, BACKSCROLL_2, 9, 2.f*Vector2::ONE, Vector2(0.f, 0.f), WorldEllipseParallax, boundcurve, true, true, Vector2(0.f, 2.f), Color(0.55f, 0.55f, 0.45f, 1.f), false);
    // MiddleGround 1
    DrawableScroller::AddScroller(viewport, material, UNLIT, BACKSCROLL_2, 7, 2.f*Vector2::ONE, Vector2(0.f, 1.f*info_->mTileHeight_), Vector2(0.8f, 0.6f), boundcurve, false, true, Vector2(0.f, -2.f), Color::WHITE);
    // MiddleGround 2
    DrawableScroller::AddScroller(viewport, material, UNLIT, BACKSCROLL_2, 5, 2.f*Vector2::ONE, Vector2(0.f, 1.5f*info_->mTileHeight_), Vector2(0.9f, 0.8f), boundcurve, false, true, Vector2(0.f, -1.f), Color(0.85f, 0.85f, 0.85f, 1.f));
    // MiddleGround 3
    DrawableScroller::AddScroller(viewport, material, UNLIT, BACKSCROLL_2, 3, 2.f*Vector2(0.9f, 0.9f), Vector2(0.f, 1.5f*info_->mTileHeight_), Vector2(0.95f, 0.9f), boundcurve, false, true, Vector2(0.f, 0.f), Color(0.65f, 0.65f, 0.65f, 1.f));
    // BackGround
    DrawableScroller::AddScroller(viewport, material, UNLIT, BACKSCROLL_2, 1, 2.f*Vector2(0.85f, 0.85f), Vector2(0.f, 2.5f*info_->mTileHeight_), Vector2(0.99f, 0.95f), boundcurve, false, true, Vector2(0.f, 0.f), Color(0.5f, 0.5f, 0.5f, 1.f));
}

void World2D::GoToMap(const ShortIntVector2& mpoint, const IntVector2& mposition, int viewZ, int viewport)
{
    WorldViewInfo& vinfo = viewinfos_[viewport];
    vinfo.dMapPoint_ = mpoint;

    URHO3D_LOGINFOF("World2D() - GoToMap : Viewport=%d Map=%s mposition=%s viewZ=%d ...", viewport, mpoint.ToString().CString(), mposition.ToString().CString(), viewZ);

    // Set New World Position
    info_->Convert2WorldPosition(mpoint, mposition, vinfo.dMapPosition_);

    SetKeepedVisibleMaps(true);
    viewManager_->SetFocusEnable(false, viewport);
    vinfo.cameraFocusEnabled_ = false;

    if (viewZ != -1)
        viewManager_->SwitchToViewZ(viewZ, 0, viewport);

    URHO3D_LOGINFOF("World2D() - GoToMap : Viewport=%d Map=%s mposition=%s ... OK !", viewport, mpoint.ToString().CString(), mposition.ToString().CString());
}

void World2D::GoToMap(const ShortIntVector2& mpoint, int viewport)
{
    // set the Camera position to the center of the map
    GoToMap(mpoint, IntVector2(info_->mapWidth_/2, info_->mapHeight_/2), -1, viewport);
}

void World2D::GoCameraToDestinationMap(int viewport, bool updateinstant)
{
    WorldViewInfo& vinfo = viewinfos_[viewport];

    // hide if not in a visiblearea
    for (Vector<ShortIntVector2>::ConstIterator it = keepedVisibleMaps_.Begin(); it != keepedVisibleMaps_.End(); ++it)
    {
        if (!IsInsideVisibleAreasMinimized(*it))
            vinfo.mapsToHide_.Push(*it);
    }

    SetKeepedVisibleMaps(false);

    // update for Unloading map
//    mapStorage_->UpdateBufferedArea(viewport, vinfo.mPoint_);
    mapStorage_->UpdateBufferedArea(viewport, vinfo.dMapPoint_);

    vinfo.cameraFocusEnabled_ = true;
    viewManager_->GetCameraNode(viewport)->SetPosition2D(vinfo.dMapPosition_);
    viewManager_->SetFocusEnable(true, viewport);

    URHO3D_LOGINFOF("World2D - GoCameraToDestinationMap : viewport=%d map=%s destinationmap=%s destination=%s !",
                    viewport, vinfo.mPoint_.ToString().CString(), vinfo.dMapPoint_.ToString().CString(), vinfo.dMapPosition_.ToString().CString());

    // update instant with setting map visibles
    if (updateinstant)
        UpdateInstant(viewport);
}

void World2D::AttachEntityToMapNode(Node* entity, const ShortIntVector2& mPoint, CreateMode mode)
{
    Node* mapNode = GetEntitiesNode(mPoint, mode);
    if (mapNode && mapNode != entity->GetParent())
    {
        Matrix2x3 transform = entity->GetWorldTransform2D();

        // don't use setparent (hang program), prefer addchild
        mapNode->AddChild(entity);

        // restore world positions after adding to mapNode
        entity->SetWorldTransform2D(transform);

//        URHO3D_LOGWARNINGF("World2D() - AttachEntityToMapNode : entity=%s(%u) parentMap=%s(%u) ti=%s tf=%s",
//                            entity->GetName().CString(), entity->GetID(), mapNode->GetName().CString(), mapNode->GetID(), transform.ToString().CString(), entity->GetWorldTransform2D().ToString().CString());

//        GameContext::Get().DumpNode(mapNode, 0, 1);
    }
    else if (!mapNode)
    {
        URHO3D_LOGWARNINGF("World2D() - AttachEntityToMapNode : node=%s(%u) noparent !", entity->GetName().CString(), entity->GetID());
    }
}

void World2D::AddEntity(const ShortIntVector2& mPoint, unsigned id)
{
    List<unsigned>& entities = mapEntities_[mPoint].entities_;
    List<unsigned>::ConstIterator it = entities.Find(id);
    if (it == entities.End())
        entities += id;
}

void World2D::RemoveEntity(const ShortIntVector2& mPoint, unsigned id)
{
    List<unsigned>& entities = GetEntities(mPoint);

    URHO3D_LOGINFOF("World2D() - RemoveEntity : nodeid=%u mpoint=%s ... OK !", id, mPoint.ToString().CString());
//    world_->DumpNodeList(entities, "map");
    List<unsigned>::Iterator it = entities.Find(id);
    if (it != entities.End())
    {
        entities.Erase(it);
//        world_->DumpNodeList(entities, "map");
    }
}

void World2D::DestroyEntity(MapBase* map, Node* node)
{
    if (node->GetComponent<GOC_Destroyer>())
    {
        URHO3D_LOGINFOF("World2D() - DestroyEntity : %s(%u) ... OK !", node->GetName().CString(), node->GetID());
        node->SendEvent(WORLD_ENTITYDESTROY);
        return;
    }

//    map->GetMapData()->RemoveEntityData(node);

    // if no destroyer
    VariantMap& eventData = node->GetContext()->GetEventDataMap();
    eventData[Go_Destroy::GO_ID] = node->GetID();
    eventData[Go_Destroy::GO_TYPE] = node->GetVar(GOA::TYPECONTROLLER).GetInt();
    eventData[Go_Destroy::GO_MAP] = map->GetMapPoint().ToHash();
    eventData[Go_Destroy::GO_PTR] = 0;

    // for static furniture
//    if ((GOT::GetTypeProperties(node->GetVar(GOA::GOT).GetStringHash()) & (GOT_Furniture | GOT_Anchored)) == (GOT_Furniture | GOT_Anchored))
    if (node->GetVar(GOA::ONTILE) != Variant::EMPTY)
        eventData[Go_Destroy::GO_TILE] = node->GetVar(GOA::ONTILE).GetUInt();

    node->SendEvent(GO_DESTROY, eventData);

    URHO3D_LOGINFOF("World2D() - DestroyEntity : %s(%u) ... OK !", node->GetName().CString(), node->GetID());

    TimerRemover::Get()->Start(node, 0.f, POOLRESTORE);
}

void World2D::AddStaticFurniture(const ShortIntVector2& mPoint, Node* node, EntityData& furniture)
{
    List<MapFurnitureRef>& furnitures = mapFurnitures_[mPoint][furniture.tileindex_];

    bool furnitureExists = false;
    for (List<MapFurnitureRef>::ConstIterator it=furnitures.Begin(); it != furnitures.End(); ++it)
    {
        if (it->nodeid_ == node->GetID())
        {
            furnitureExists = true;
            break;
        }
    }

    if (!furnitureExists)
    {
        if (mPoint.y_ == fixedObjeMapedPointY)
            URHO3D_LOGINFOF("World2D() - AddStaticFurniture : node=%s(%u) at mPoint=%s tileindex=%u !", node->GetName().CString(), node->GetID(), mPoint.ToString().CString(), furniture.tileindex_);

        furnitures += MapFurnitureRef(node->GetID(), &furniture);

        if (!IsInsideVisibleAreasMinimized(mPoint))
            node->SetEnabled(false);

        node->SetVar(GOA::ONMAP, mPoint.ToHash());
        node->SetVar(GOA::ONTILE, furniture.tileindex_);

        // furniture is also entity (especially for map visibility process)
        AddEntity(mPoint, node->GetID());
    }
}

static Vector2 debugTemporaryPoint_;
static Rect debugTemporaryDrawRect_;
static float delayTemporaryDrawRect_ = 0.f;

void World2D::DestroyFurnituresAt(MapBase* map, unsigned tileindex)
{
    bool inside = ViewManager::Get()->GetCurrentViewZ() == INNERVIEW;

    HashMap<ShortIntVector2, MapFurnitureLocation >::Iterator it = mapFurnitures_.Find(map->GetMapPoint());
    if (it != mapFurnitures_.End())
    {
        MapFurnitureLocation& location = it->second_;

        // Remove Furnitures at tileindex
        MapFurnitureLocation::Iterator jt = location.Find(tileindex);
        if (jt != location.End())
        {
//            URHO3D_LOGINFOF("World2D() - DestroyFurnituresAt : destroy furnitures at mPoint=%s tileindex=%u !", map->GetMapPoint().ToString().CString(), tileindex);

            List<MapFurnitureRef >& furnitures = jt->second_;
            for (List<MapFurnitureRef >::Iterator kt = furnitures.Begin(); kt != furnitures.End(); ++kt)
            {
                MapFurnitureRef& furnitureref = *kt;

                Node* node = world_->GetScene()->GetNode(furnitureref.nodeid_);
                if (!node)
                    continue;

                // TODO
                // if (inside && node->GetVar(GOA::ONVIEWZ).GetInt() ... )

                map->GetMapData()->RemoveEntityData(node);

                if (node->GetComponent<GOC_PhysicRope>())
                    continue;

                DestroyEntity(map, node);
            }

            location.Erase(jt);
        }

        // Physic Cast : Remove Static Furnitures that will be in collision with the tile
        Vector2 wposition = map->GetWorldTilePosition(map->GetTileCoords(tileindex));
        Vector2 halfTileSize(info_->mTileWidth_ * 0.6f, info_->mTileHeight_ * 0.6f);
        Rect worldTileRect(wposition - halfTileSize, wposition + halfTileSize + Vector2(0.f, info_->mTileHeight_));
        debugTemporaryPoint_ = wposition;
        debugTemporaryDrawRect_ = Rect(wposition - halfTileSize, wposition + halfTileSize);
        delayTemporaryDrawRect_ = 2.f;

        PODVector<RigidBody2D*> bodies;
        world_->GetScene()->GetComponent<PhysicsWorld2D>()->GetRigidBodies(bodies, worldTileRect, inside ? CC_INSIDESTATICFURNITURE | CC_INSIDEOBJECT : CC_OUTSIDESTATICFURNITURE | CC_OUTSIDEOBJECT);
//        URHO3D_LOGINFOF("World2D() - DestroyFurnituresAt : Find %u bodies in rect=%s ... ", bodies.Size(), worldTileRect.ToString().CString());

        for (unsigned i=0; i < bodies.Size(); i++)
        {
            Node* node = bodies[i]->GetNode();

//            URHO3D_LOGINFOF("World2D() - DestroyFurnituresAt : Body[%u] = %s(%u) ...", i, node->GetName().CString(), node->GetID());

            // skip if not a furniture
            if ((GOT::GetTypeProperties(node->GetVar(GOA::GOT).GetStringHash()) & GOT_Furniture) == 0)
                continue;

            // if it's a static furniture, find the MapFurnitureRef for the tileindex and nodeid
            if (node->GetVar(GOA::ONTILE) != Variant::EMPTY)
            {
                jt = location.Find(node->GetVar(GOA::ONTILE).GetUInt());
                if (jt != location.End())
                {
                    List<MapFurnitureRef >& furnitures = jt->second_;
                    List<MapFurnitureRef >::Iterator kt;
                    for (kt = furnitures.Begin(); kt != furnitures.End(); ++kt)
                        if (kt->nodeid_ == node->GetID())
                            break;

                    if (kt == furnitures.End())
                        continue;

                    // compact
                    furnitures.Erase(kt);
                    if (!furnitures.Size())
                        location.Erase(jt);
                }
            }

            // destroyer the node
            map->GetMapData()->RemoveEntityData(node);

            // skip if has PhysicRope component
            if (node->GetComponent<GOC_PhysicRope>())
                continue;

            DestroyEntity(map, node);

            URHO3D_LOGINFOF("World2D() - DestroyFurnituresAt : %s(%u) remove furniture node ... OK !", node->GetName().CString(), node->GetID());
        }
    }
}

void World2D::PurgeEntities(const ShortIntVector2& mPoint)
{
    // clear furnitures
    mapFurnitures_[mPoint].Clear();

    // clear entities : keep Players
    List<unsigned>& ids = GetEntities(mPoint);
    List<unsigned>::Iterator it=ids.Begin();
    while (it != ids.End())
    {
        if (GOManager::IsA(*it, GO_Player | GO_AI_Ally))
            it++;
        else
            it = ids.Erase(it);
    }
}

// Editor Spawn Furniture
Node* World2D::SpawnFurniture(const StringHash& got, int layerZ, bool isabiome)
{
    IntVector2 position;
    GameHelpers::GetInputPosition(position.x_, position.y_);
    Vector2 pos = GameHelpers::ScreenToWorld2D(position);

    Node* node = SpawnFurniture(got, pos, layerZ, isabiome, true, false);

    if (node && !GameContext::Get().LocalMode_)
    {
        // Send a NetCommand
        VariantMap eventData;
        eventData[Net_ObjectCommand::P_NODEID] = node->GetID();
        eventData[Net_ObjectCommand::P_NODEIDFROM] = 0;
        eventData[Net_ObjectCommand::P_CLIENTOBJECTTYPE] = got.Value();
        eventData[Net_ObjectCommand::P_CLIENTOBJECTVIEWZ] = layerZ;
        eventData[Net_ObjectCommand::P_CLIENTOBJECTPOSITION] = pos;
        eventData[Net_ObjectCommand::P_DATAS] = isabiome;
        SendEvent(MAP_ADDFURNITURE, eventData);
    }

    return node;
}

Node* World2D::SpawnFurniture(VariantMap& eventData)
{
    return SpawnFurniture(StringHash(eventData[Net_ObjectCommand::P_CLIENTOBJECTTYPE].GetUInt()),
                          eventData[Net_ObjectCommand::P_CLIENTOBJECTPOSITION].GetVector2(),
                          eventData[Net_ObjectCommand::P_CLIENTOBJECTVIEWZ].GetInt(),
                          eventData[Net_ObjectCommand::P_DATAS].GetBool(), true, false);
}

Node* World2D::SpawnFurniture(const StringHash& got, Vector2 position, int layerZ, bool isabiome, bool checkpositionintile, bool findfloor)
{
    ShortIntVector2 mpoint;

    info_->Convert2WorldMapPoint(position, mpoint);
    Map* map = GetMapAt(mpoint);
    if (!map)
        return 0;

    if (findfloor)
    {
        IntVector2 coord = map->GetTileCoords(position);
        IntVector2 coordatfloor = coord;

        // Get the floor positions
        int viewid = map->GetNearestViewId(layerZ);

        map->GetBlockPositionAt(DownDir, viewid, coordatfloor);
        coordatfloor.y_--;
        position = map->GetWorldTilePosition(coordatfloor);

        // TODO : check for change the map

//        URHO3D_LOGINFOF("World2D() - SpawnFurniture : at mPoint=%s layerZ=%d coord=%s coordatfloor=%s => new position=%s!",
//                        mpoint.ToString().CString(), layerZ, coord.ToString().CString(), coordatfloor.ToString().CString(), position.ToString().CString());
    }

    EntityData entitydata(got, position, layerZ);

    if (GOT::GetTypeProperties(got) & GOT_Anchored)
    {
        if (!map->AnchorEntityOnTileAt(entitydata, 0, isabiome, checkpositionintile))
        {
            URHO3D_LOGERRORF("World2D() - SpawnFurniture : Can't anchor on tile pos=%s layerZ=%d", position.ToString().CString(), layerZ);
            return 0;
        }

        URHO3D_LOGINFOF("World2D() - SpawnFurniture : got=%s(%u) anchored at mPoint=%s tileindex=%u layerZ=%d !",
                        GOT::GetType(got).CString(), got.Value(), mpoint.ToString().CString(), entitydata.tileindex_, layerZ);
    }

    Node* node = map->AddFurniture(entitydata);
    if (!node)
        URHO3D_LOGERRORF("World2D() - SpawnFurniture : at mPoint=%s layerZ=%d can't spawn got=%s(%u) !", mpoint.ToString().CString(), layerZ, GOT::GetType(got).CString(), got.Value());

    return node;
}

Node* World2D::NetSpawnFurniture(ObjectControlInfo& info)
{
    const ObjectControl& control = info.GetReceivedControl();

    Node* node = SpawnFurniture(StringHash(control.states_.type_), Vector2(control.physics_.positionx_, control.physics_.positiony_),
                                (int)control.states_.viewZ_, false, false, false);

    GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
    if (animator)
    {
        animator->SetDirection(Vector2(control.physics_.direction_, 0.f));
    }
    else
    {
        StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
        if (sprite)
            sprite->SetFlip(control.physics_.direction_ < 0.f, false);
    }

    node->SetEnabled(true);

    return node;
}

// Editor Spawn Collectable
Node* World2D::SpawnCollectable(const StringHash& got)
{
    URHO3D_LOGINFOF("World2D() - SpawnCollectable : got=%s editor ...", GOT::GetType(got).CString());

    IntVector2 position;
    GameHelpers::GetInputPosition(position.x_, position.y_);
    Vector2 pos = GameHelpers::ScreenToWorld2D(position);
    ShortIntVector2 mpoint;
    info_->Convert2WorldMapPoint(pos, mpoint);

    Map* map = GetMapAt(mpoint);
    if (!map)
        return 0;

    SceneEntityInfo sceneInfo;
    sceneInfo.skipNetSpawn_ = true;
    sceneInfo.zindex_ = 1000;

    StringHash fromtype;
    if (GOT::GetTypeProperties(got) & GOT_Part)
        fromtype = GOT::GetBuildableType(got);

    VariantMap eventData;
    Slot slot;
    slot.Set(got, 1U, fromtype);
    Slot::GetSlotData(slot, eventData);

    Node* node = map->AddEntity(got, 0, LOCAL, 0, ViewManager::Get()->GetCurrentViewZ(), PhysicEntityInfo(pos.x_, pos.y_), sceneInfo, &eventData);
    if (node && !GameContext::Get().LocalMode_)
    {
        // Send a NetCommand
        eventData[Net_ObjectCommand::P_NODEID] = node->GetID();
        eventData[Net_ObjectCommand::P_NODEIDFROM] = 0;
        eventData[Net_ObjectCommand::P_CLIENTOBJECTVIEWZ] = ViewManager::Get()->GetCurrentViewZ();
        eventData[Net_ObjectCommand::P_CLIENTOBJECTPOSITION] = pos;
        SendEvent(MAP_ADDCOLLECTABLE, eventData);
    }

    return node;
}

Node* World2D::SpawnCollectable(VariantMap& eventData)
{
    unsigned nodeid = eventData[Net_ObjectCommand::P_NODEID].GetUInt();
    StringHash got(eventData[Net_ObjectCommand::P_CLIENTOBJECTTYPE].GetUInt());

    URHO3D_LOGINFOF("World2D() - SpawnCollectable : got=%s nodeid=%u  ...", GOT::GetType(got).CString(), nodeid);

    int viewZ = eventData[Net_ObjectCommand::P_CLIENTOBJECTVIEWZ].GetUInt();

    PhysicEntityInfo physicInfo;
    physicInfo.positionx_ = eventData[Net_ObjectCommand::P_CLIENTOBJECTPOSITION].GetVector2().x_;
    physicInfo.positiony_ = eventData[Net_ObjectCommand::P_CLIENTOBJECTPOSITION].GetVector2().y_;

    SceneEntityInfo sceneinfo;
    sceneinfo.zindex_ = 1000;
    sceneinfo.clientId_ = 0;
    sceneinfo.skipNetSpawn_ = true;

    Node* node = World2D::SpawnEntity(got, 0, LOCAL, 0, viewZ, physicInfo, sceneinfo, &eventData);
    if (node)
        URHO3D_LOGINFOF("GameNetwork() - SpawnCollectable : Node=%s(%u) spawned ... OK !", node->GetName().CString(), node->GetID());
    else
        URHO3D_LOGERRORF("GameNetwork() - SpawnCollectable : can't Spawn %s(%u) !", GOT::GetType(got).CString(), got.Value());

    return node;
}

// Editor Spawn Entity
Node* World2D::SpawnEntity(const StringHash& got)
{
    URHO3D_LOGINFOF("World2D() - SpawnEntity : got=%s editor ...", GOT::GetType(got).CString());

    IntVector2 position;
    GameHelpers::GetInputPosition(position.x_, position.y_);
    Vector2 pos = GameHelpers::ScreenToWorld2D(position);
    ShortIntVector2 mpoint;
    info_->Convert2WorldMapPoint(pos, mpoint);

    Map* map = GetMapAt(mpoint);

    return map ? map->AddEntity(got, RandomEntityFlag|RandomMappingFlag, LOCAL, 0, ViewManager::Get()->GetCurrentViewZ(), PhysicEntityInfo(pos.x_, pos.y_)) : 0;
}

Node* World2D::SpawnEntity(const StringHash& got, int entityid, int id, unsigned holderid, int viewZ, const PhysicEntityInfo& physicInfo, const SceneEntityInfo& sceneInfo, VariantMap* slotData, bool outsidePool)
{
#ifdef HANDLE_ENTITIES
//    const StringHash& type = GOT::HasObject(got) ? got : GOT::COLLECTABLEPART;

    ShortIntVector2 mpoint;
    info_->Convert2WorldMapPoint(physicInfo.positionx_, physicInfo.positiony_, mpoint);

    viewZ = ViewManager::Get()->GetNearViewZ(viewZ);

    Map* map = GetMapAt(mpoint);
    if (!map)
    {
        URHO3D_LOGERRORF("World2D() - SpawnEntity : id=%u got=%s(%u) effectiveType=%s(%u) position=%F %F mpoint=%s viewZ=%d deferredAdd=%s No MAP !",
                         id, GOT::GetType(got).CString(), got.Value(),GOT::GetType(got).CString(), got.Value(),
                         physicInfo.positionx_, physicInfo.positiony_,
                         mpoint.ToString().CString(), viewZ, sceneInfo.deferredAdd_ ? "true":"false");
        return 0;
    }

    if (map->GetStatus() > Available)
    {
        URHO3D_LOGERRORF("World2D() - SpawnEntity : id=%u got=%s(%u) effectiveType=%s(%u) position=%F %F mpoint=%s the map is purging !",
                         id, GOT::GetType(got).CString(), got.Value(),GOT::GetType(got).CString(), got.Value(),
                         physicInfo.positionx_, physicInfo.positiony_, mpoint.ToString().CString());
        return 0;
    }

    URHO3D_LOGINFOF("World2D() - SpawnEntity : id=%u got=%s(%u) effectiveType=%s(%u) position=%F %F mpoint=%s viewZ=%d deferredAdd=%s",
                    id, GOT::GetType(got).CString(), got.Value(),GOT::GetType(got).CString(), got.Value(),
                    physicInfo.positionx_, physicInfo.positiony_,
                    mpoint.ToString().CString(), viewZ, sceneInfo.deferredAdd_ ? "true":"false");

//    return map->AddEntity(got, entityid, (!id && GameContext::Get().ClientMode_) ? LOCAL : id, holderid, viewZ, physicInfo, sceneInfo, slotData);
    return map->AddEntity(got, entityid, !id ? LOCAL : id, holderid, viewZ, physicInfo, sceneInfo, slotData, outsidePool);
#else
    return 0;
#endif // HANDLE_ENTITIES
}

Node* World2D::NetSpawnEntity(ObjectControlInfo& info, Node* holder, VariantMap* slotData)
{
    const ObjectControl& control = info.GetReceivedControl();

    Node* node = 0;
    StringHash got(control.states_.type_);
    const GOTInfo& gotinfo = GOT::GetConstInfo(got);

    if (gotinfo.pooltype_ == GOT_GOPool)
    {
        ObjectControlInfo* oinfo = &info;
        node = Ability::Use(got, holder, *((PhysicEntityInfo*) &control.physics_), false, (int)control.states_.viewZ_, &oinfo);

//        URHO3D_LOGINFOF("World2D() - NetSpawnEntity : Node=%s(%u) Ability spawned dir=%f rot=%f velx=%f... OK !",
//                        node->GetName().CString(), node->GetID(), control.physics_.direction_, control.physics_.rotation_, control.physics_.velx_);
    }
    else
    {
        SceneEntityInfo scinfo;
        scinfo.faction_ = holder ? (holder->GetVar(GOA::NOCHILDFACTION).GetBool() ? 0 : holder->GetVar(GOA::FACTION).GetUInt()) : 0;
//        scinfo.zindex_ = ;
        scinfo.clientId_ = info.clientId_;
        scinfo.skipNetSpawn_ = control.HasNetSpawnMode() == 0;
        scinfo.objectControlInfo_ = &info;
        node = SpawnEntity(got, (int)control.states_.entityid_, info.serverNodeID_, info.serverNodeID_, control.states_.viewZ_, *((PhysicEntityInfo*) &control.physics_), scinfo, slotData);

        URHO3D_LOGINFOF("World2D() - NetSpawnEntity : Node=%s(%u) spawned dir=%f rot=%f velx=%f... OK !",
                        node->GetName().CString(), node->GetID(), control.physics_.direction_, control.physics_.rotation_, control.physics_.velx_);
    }

    if (!node)
        return 0;

    // BT_STATIC for all netspawnentities
//   if (node)
//   {
//       RigidBody2D* body = node->GetComponent<RigidBody2D>();
//        if (body)
//            body->SetBodyType(BT_STATIC);
//   }

    GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
    if (controller)
    {
        controller->SetMainController(false);

#ifdef ACTIVE_NETWORK_LOCALPHYSICSIMULATION_BUTTON_MAINONLY
        GOC_Move2D* move2d = node->GetComponent<GOC_Move2D>();
        if (move2d)
            move2d->SetPhysicEnable(false, controller);
#endif
    }

//    GOC_Destroyer* destroyer = node->GetComponent<GOC_Destroyer>();
//    if (destroyer)
//        destroyer->SetEnableLifeTimer(false);

    GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
    if (animator)
    {
        animator->SetDirection(Vector2(control.physics_.direction_, 0.f));
    }
    else
    {
        StaticSprite2D* sprite = node->GetDerivedComponent<StaticSprite2D>();
        if (sprite)
            sprite->SetFlip(control.physics_.direction_ < 0.f, false);
    }

    // Set Velocity only here and in GameNetwork::Server_CommandCreateRequestObject
    GameHelpers::SetPhysicVelocity(node, control.physics_.velx_, control.physics_.vely_);

    node->SetEnabled(true);

    return node;
}

// Editor Spawn Actor
Node* World2D::SpawnActor()
{
    IntVector2 position;
    GameHelpers::GetInputPosition(position.x_, position.y_);
    Vector2 pos = GameHelpers::ScreenToWorld2D(position);

    ShortIntVector2 mpoint;
    info_->Convert2WorldMapPoint(pos, mpoint);

    Map* map = GetMapAt(mpoint);
    if (!map)
        return 0;

    Actor* actor = SpawnActor(String("Eredot"), StringHash("GOT_Merchant"), 0, StringHash("merchandise1"), ViewManager::Get()->GetCurrentViewZ(), pos);

    return actor ? actor->GetAvatar() : 0;
}

Actor* World2D::SpawnActor(const String& name, const StringHash& got, unsigned char entityid, const StringHash& dialogue, int viewZ, const Vector2& position)
{
    Actor* actor = Actor::Get(got);
    if (!actor)
    {
        actor = new Actor(GameContext::Get().context_, GameContext::Get().MAX_NUMPLAYERS+1);
    }

    URHO3D_LOGINFOF("World2D() - SpawnActor : got=%s(%u) => actorid=%u ...", GOT::GetType(got).CString(), got.Value(), actor->GetID());

    actor->SetName(name);
    actor->SetObjectType(got, entityid);
    actor->SetController(true);
    actor->SetScene(GameContext::Get().rootScene_, position, viewZ);

    if (actor->GetAvatar())
    {
        info_->Convert2WorldMapPosition(position, actor->GetInfo().position_);
        actor->GetInfo().position_.viewZ_ = viewZ;
        bool resurrection = actor->GetInfo().state_ == ActorState::Dead;
        actor->GetInfo().state_ = ActorState::Activated;
        actor->GetInfo().dialog_ = dialogue;
        actor->SetDialogue(dialogue);
        actor->Start(resurrection);

        URHO3D_LOGINFOF("World2D() - SpawnActor : actorid=%u ... avatar=%s(%u) ... OK !", actor->GetID(), actor->GetAvatar()->GetName().CString(), actor->GetAvatar()->GetID());
    }
    else
    {
        URHO3D_LOGINFOF("World2D() - SpawnActor : can't create more actor %s", GOT::GetType(got).CString());
        Actor::RemoveActor(actor->GetID());
    }

    return actor;
}

// Respawn an Actor in another position (like a teleportation)
Actor* World2D::SpawnActor(unsigned actorid, const Vector2& position, int viewZ)
{
    Actor* actor = Actor::Get(actorid);
    if (actor)
    {
        if (actor->GetAvatar())
        {
            URHO3D_LOGINFOF("World2D() - SpawnActor : actorid=%u ...", actorid);

            WorldMapPosition& pos = actor->GetInfo().position_;
            world_->GetWorldInfo()->Convert2WorldMapPosition(position, pos);
            pos.SetViewZ(viewZ);
            actor->GetInfo().state_ = Activated;
            actor->GetAvatar()->SetEnabled(true);
            actor->GetAvatar()->GetComponent<GOC_Destroyer>()->SetWorldMapPosition(pos);
            actor->SetDialogue(actor->GetInfo().dialog_);

            URHO3D_LOGINFOF("World2D() - SpawnActor : actorid=%u ... OK !", actorid);
        }
    }
    else
        URHO3D_LOGINFOF("World2D() - SpawnActor : no actor with actorid=%u !", actorid);

    return actor;
}


/// Network World Objects

const unsigned NumNetObjectsByMap = 2U;

void World2D::SetNetWorldObjects(const VariantVector& objects)
{
    if (!objects.Size())
    {
        URHO3D_LOGERRORF("World2D() - SetNetWorldObjects ... no objects received !");
        return;
    }

    VariantVector::ConstIterator it = objects.Begin();

    // Get the stamp and check if new
    unsigned short stamp = it->GetUInt();

    if (!GameNetwork::CheckNewStamp<unsigned short>(stamp, worldObjectState_.stamp_, STAMP_MAXDELTASHORT))
    {
        URHO3D_LOGERRORF("World2D() - SetNetWorldObjects ... stamp received=%u (current=%u) not new !", stamp, worldObjectState_.stamp_);
        return;
    }

    worldObjectState_.stamp_ = stamp;
    it++;

    while (it != objects.End())
    {
        // MapPoint
        ShortIntVector2 mpoint(it->GetUInt());
        it++;

        Map* map = GetMapAt(mpoint);

        // TileModifiers : use PODVector placement
        map->SetTiles(PODVector<TileModifier>(it->GetBuffer(), true));
        it++;

        // Entities
//        {
//            const PODVector<unsigned char>& buf = varvector[2].GetBuffer();
//            numentities = buf.Size() / sizeof(EntityData);
//        }

        URHO3D_LOGINFOF("World2D() - SetNetWorldObjects ... map=%s ... OK !", mpoint.ToString().CString());
    }
}

const VariantVector& World2D::GetNetWorldObjects(ClientInfo& clientinfo)
{
    PrepareNetWorldObjects(clientinfo);

    return worldObjectState_.datas_;
}

void World2D::PrepareNetWorldObjects(ClientInfo& clientinfo)
{
    // TODO : Get the Maps Set of the world to be prepared
    // in ClientInfo, we must find the mapposition in the world and get the MapBufferRect

    const HashMap<ShortIntVector2, Map* >& maps = mapStorage_->GetMapsInMemory();
    if (!maps.Size())
    {
        URHO3D_LOGERRORF("World2D() - PrepareNetWorldObjects ... no map in memory !");
        return;
    }

    bool change = false;

    // Check for a change
    for (HashMap<ShortIntVector2, Map* >::ConstIterator it = maps.Begin(); it != maps.End(); ++it)
    {
        if (it->second_->IsTileModifiersDirty() || it->second_->IsEntitiesDirty())
        {
            change = true;
            break;
        }
    }

    // if any change occurs, do a new snapshot of the world objects
    if (change)
    {
        worldObjectState_.datas_.Resize(NumNetObjectsByMap*maps.Size() + 1);

        // Push the stamp
        worldObjectState_.stamp_++;
        worldObjectState_.datas_[0] = Variant((unsigned)worldObjectState_.stamp_);

        URHO3D_LOGINFOF("World2D() - PrepareNetWorldObjects ... stamp=%u", worldObjectState_.stamp_);

        PODVector<TileModifier> tileModifiers;
        unsigned i = 1;
        for (HashMap<ShortIntVector2, Map* >::ConstIterator it = maps.Begin(); it != maps.End(); ++it, i+=NumNetObjectsByMap)
        {
            Map* map = it->second_;

            // set MapPoint
            worldObjectState_.datas_[i] = Variant(it->first_.ToHash());

            // TileModifiers
            map->GetCachedTileModifiers(tileModifiers);
            worldObjectState_.datas_[i+1].SetBuffer(tileModifiers.Buffer(), tileModifiers.Size() * sizeof(TileModifier));
            map->SetTileModifiersDirty(false);

            // TODO : Entities
            map->SetEntitiesDirty(false);
            // Push All Modified EntityData
            //static HashMap<ShortIntVector2, MapEntityInfo > mapEntities_;
            //static HashMap<ShortIntVector2, MapFurnitureLocation> mapFurnitures_;
        }
    }
    else
    {
        URHO3D_LOGINFOF("World2D() - PrepareNetWorldObjects ... no change !");
    }
}


/// Attributes Getters

const String& World2D::GetAnlWorldModelAttr() const
{
    return String::EMPTY;
//    return info_ ? info_->worldModelFile_ : String::EMPTY;
}

const String& World2D::GetMapGeneratorAttr() const
{
    return MapCreator::GetTypeName(info_->defaultGenerator_);
}

const String& World2D::GetMapGeneratorParams() const
{
    return genParams_;
}

const IntVector2& World2D::GetGeneratorNumEntities() const
{
    return generatorNumEntities_;
}

Rect World2D::GetMapBounds(const Vector2& worldPosition)
{
    ShortIntVector2 mPoint;
    info_->Convert2WorldMapPoint(worldPosition, mPoint);

    Map* map = GetMapAt(mPoint, false);
    if (map)
    {
        return map->GetBounds();
    }
    return Rect::ZERO;
}


/// Getters

bool World2D::IsInsideWorldBounds(const Vector2& worldPosition)
{
    return noBounds_ ? true : worldFloatBounds_.IsInside(worldPosition) == INSIDE;
}

bool World2D::IsInsideWorldBounds(const ShortIntVector2& mPoint)
{
    return noBounds_ ? true : worldBounds_.IsInside(IntVector2(mPoint.x_, mPoint.y_)) == INSIDE;
}

bool World2D::IsInsideVisibleAreas(const ShortIntVector2& mPoint)
{
    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    IntVector2 point(mPoint.x_, mPoint.y_);
    for (unsigned i = 0; i < numviewports; i++)
    {
        if (world_->viewinfos_[i].visibleMapArea_.IsInside(point) == INSIDE)
            return true;
    }

    return false;
}

bool World2D::IsInsideVisibleAreasMinimized(const ShortIntVector2& mPoint)
{
    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    IntVector2 point(mPoint.x_, mPoint.y_);
    for (unsigned i = 0; i < numviewports; i++)
    {
        if (world_->viewinfos_[i].visibleMapAreaMinimized_.IsInside(point) == INSIDE)
            return true;
    }

    return false;
}

const Rect& World2D::GetExtendedVisibleRect(int viewport)
{
    if (!world_ || !world_->IsEnabled())
    {
        extVisibleRectCached_ = ViewManager::Get()->GetViewRect(viewport);
        return extVisibleRectCached_;
    }
//    {
//        const Frustum& frustum = ViewManager::Get()->GetCamera(viewport)->GetFrustum();
//        extVisibleRectCached_ = Rect(Vector2(frustum.vertices_[2].x_, frustum.vertices_[2].y_) - VisibleRectOverscan * Vector2::ONE, Vector2(frustum.vertices_[0].x_, frustum.vertices_[0].y_) + VisibleRectOverscan * Vector2::ONE);
//        return extVisibleRectCached_;
//    }

    return world_->viewinfos_[viewport].extVisibleRect_;
}

const Rect& World2D::GetTiledVisibleRect(int viewport)
{
    return world_->viewinfos_[viewport].tiledVisibleRect_;
}

Rect World2D::GetVisibleRect(int viewport)
{
    if (!world_ || !world_->IsEnabled())
        return ViewManager::Get()->GetViewRect(viewport);

//    return world_->viewinfos_[viewport].extVisibleRect_.Adjusted(-VisibleRectOverscan);
    return world_->viewinfos_[viewport].visibleRect_;
}

IntRect World2D::GetVisibleAreas(const Vector2& wposition)
{
    return IntRect((int)floor(wposition.x_ / mWidth_), (int)floor(wposition.y_ / mHeight_),
                   (int)floor(wposition.x_ / mWidth_+ 0.5f), (int)floor(wposition.y_ / mHeight_ + 0.5f));
}

const Vector2& World2D::GetCurrentMapOrigin(int viewport)
{
    return world_ && world_->viewinfos_[viewport].currentMap_ ? world_->viewinfos_[viewport].currentMap_->GetTopography().maporigin_ : Vector2::ZERO;
}

Map* World2D::GetMapAt(const ShortIntVector2& mPoint, bool createIfMissing)
{
    Map* map = mapStorage_->GetMapAt(mPoint);

    if (map && map->GetStatus() > Available)
    {
        URHO3D_LOGERRORF("World2D() - GetMapAt : point=%s map=%u is > Available !", mPoint.ToString().CString(), map);
    }

    if ((!map || map->GetStatus() < Available) && createIfMissing && IsInsideWorldBounds(mPoint))
    {
        map = mapStorage_->InitializeMap(mPoint);
//        URHO3D_LOGINFOF("World2D() - GetMapAt : point=%s map=%u loadorcreate", mPoint.ToString().CString(), map);
    }

    return map;
}

Map* World2D::GetMapAt(const Vector2& wPosition)
{
    if (!mapStorage_)
        return 0;

    ShortIntVector2 mPoint;
    info_->Convert2WorldMapPoint(wPosition, mPoint);
    return mapStorage_->GetMapAt(mPoint);
}

Map* World2D::GetAvailableMapAt(const ShortIntVector2& mPoint)
{
    return mapStorage_ ? mapStorage_->GetAvailableMapAt(mPoint) : 0;
}

unsigned World2D::GetNumMapsDrawn(int viewport)
{
    unsigned numdrawn = 0;
    for (unsigned i=0; i < effectiveVisibleMaps_[viewport].Size(); i++)
    {
        Map* map = effectiveVisibleMaps_[viewport][i];
        if (map->GetObjectTiled()->IsInView(world_->viewinfos_[viewport].camera_))
            numdrawn++;
    }
    return numdrawn;
}

bool FindBlock(const Vector2& posInTile, const IntVector2& direction, int viewZ, IntVector2& mposition, ShortIntVector2& mpoint, unsigned& tileindex)
{
    IntVector2 position = mposition;
    ShortIntVector2 mp = mpoint;

    // Get TileIndex On Horizontal Border
    if (direction.x_ > 0)
    {
        position.x_ += posInTile.x_ > 0.f ? 1 : -1;

        // x overflow : change map point x
        if (position.x_ < 0)
        {
            mp.x_--;
            position.x_ = World2D::GetWorldInfo()->mapWidth_-1;
        }
        else if (position.x_ >= World2D::GetWorldInfo()->mapWidth_)
        {
            mp.x_++;
            position.x_ = 0;
        }
    }

    // Get TileIndex On Vertical Border
    if (direction.y_ > 0)
    {
        position.y_ += posInTile.y_ > 0.f ? -1 : 1;

        // y overflow : change map point y
        if (position.y_  < 0)
        {
            mp.y_++;
            position.y_  = World2D::GetWorldInfo()->mapHeight_-1;
        }
        else if (position.y_ >= World2D::GetWorldInfo()->mapHeight_)
        {
            mp.y_--;
            position.y_  = 0;
        }
    }

    // check for a block
    Map* map = World2D::GetMapAt(mp);
    int viewid = map->GetViewId(viewZ);
    if (viewid == -1)
    {
        viewid = map->GetNearestViewId(viewZ);
        int newViewZ = map->GetViewZ(viewid);
        URHO3D_LOGERRORF("FindBlock no viewid for viewZ=%d ... get a nearest viewid=%d (viewz=%d)", viewZ, viewid, newViewZ);
    }

    if (map && map->HasTileProperties(map->GetTileIndex(position.x_, position.y_), viewid, TilePropertiesFlag::Blocked))
    {
        mposition = position;
        mpoint = mp;
        tileindex = map->GetTileIndex(position.x_, position.y_);
        return true;
    }

    return false;
}

bool World2D::GetNearestBlockTile(const Vector2& wposition, int viewZ, ShortIntVector2& mpoint, unsigned& tileindex)
{
    mpoint = info_->GetMapPoint(wposition);
    IntVector2 mposition;
    Vector2 posInTile;
    info_->Convert2WorldMapPosition(mpoint, wposition, mposition, posInTile);

    // Centered Tile Position
//    URHO3D_LOGINFOF("tileindex=%u", info_->GetTileIndex(mposition));
//    URHO3D_LOGINFOF("PosInTile=%F,%F", posInTile.x_, posInTile.y_);
    posInTile.x_ -= 0.5f*info_->mTileWidth_;
    posInTile.y_ -= 0.5f*info_->mTileHeight_;
//    URHO3D_LOGINFOF("Centered PosInTile=%F,%F", posInTile.x_, posInTile.y_);

    bool result = false;

    if (Abs(posInTile.x_) > Abs(posInTile.y_))
    {
        result = FindBlock(posInTile, IntVector2(1,0), viewZ, mposition, mpoint, tileindex);
        if (!result)
        {
            result = FindBlock(posInTile, IntVector2(1,1), viewZ, mposition, mpoint, tileindex);
            if (!result)
                result = FindBlock(posInTile, IntVector2(0,1), viewZ, mposition, mpoint, tileindex);
        }
    }
    else
    {
        result = FindBlock(posInTile, IntVector2(0,1), viewZ, mposition, mpoint, tileindex);
        if (!result)
        {
            result = FindBlock(posInTile, IntVector2(1,1), viewZ, mposition, mpoint, tileindex);
            if (!result)
                result = FindBlock(posInTile, IntVector2(1,0), viewZ, mposition, mpoint, tileindex);
        }
    }

    return result;
}

void World2D::GetCollisionShapesAt(const ShortIntVector2& mpoint, unsigned tileindex, int viewZ, Vector<CollisionShape2D*>& cshapes)
{
    if (!world_)
        return;

    Map* map = world_->GetMapAt(mpoint);
    if (!map)
        return;

    cshapes.Clear();

    const Vector<int>& viewids = map->GetViewIDs(viewZ);
    PODVector<MapCollider*> colliders;

    for (int i=viewids.Size()-1; i >= 0; --i)
    {
        int viewid = viewids[i];

        if (viewid == NOVIEW)
            continue;

        int indc = map->GetColliderIndex(viewZ, viewid);
        if (indc == -1)
            continue;

        map->GetColliders(PHYSICCOLLIDERTYPE, indc, colliders);

        for (unsigned j=0; j < colliders.Size(); j++)
        {
            PhysicCollider* collider = (PhysicCollider*)colliders[j];
            if (collider)
            {
                // Add Chains
                if (collider->contourIds_.Size() > tileindex)
                {
                    List<void*>::Iterator it = collider->chains_.GetIteratorAt(collider->contourIds_[tileindex]-1);
                    if (it != collider->chains_.End() && *it != 0)
                        cshapes.Push((CollisionShape2D*)(*it));
                }

                // Add Box (TileLeft) from Plateforms
                {
                    HashMap<unsigned, Plateform* >::Iterator it = collider->plateforms_.Find(tileindex);
                    if (it != collider->plateforms_.End())
                        cshapes.Push(it->second_->box_);
                }

                // Add Box from blocks
                {
                    HashMap<unsigned, CollisionBox2D* >::Iterator it = collider->blocks_.Find(tileindex);
                    if (it != collider->blocks_.End())
                        cshapes.Push(it->second_);
                }
            }
        }
    }
}

int World2D::GetTileGid(const WorldMapPosition& mapPosition)
{
    Map* map = mapStorage_->GetAvailableMapAt(mapPosition.mPoint_);
    if (map)
    {
        return map->GetGidAtZ(mapPosition.tileIndex_, mapPosition.viewZ_);
    }
    return 0;
}

unsigned World2D::GetTileProperty(const WorldMapPosition& mapPosition)
{
    Map* map = mapStorage_->GetAvailableMapAt(mapPosition.mPoint_);
    if (map)
        return map->GetTilePropertiesAtZ(mapPosition.tileIndex_, mapPosition.viewZ_);

    return 0;
}

//FeatureType World2D::GetTileFeature(const ShortIntVector2& mpoint, unsigned tileIndex, int viewZ)
//{
//	Map* map = mapStorage_->GetAvailableMapAt(mpoint);
//	if (map)
//		return map->GetFeatureAtZ(tileIndex, viewZ);
//
//	return 0;
//}

unsigned World2D::GetLineOfSight(const Vector2& worldPosition)
{
    ShortIntVector2 mPoint;
    info_->Convert2WorldMapPoint(worldPosition, mPoint);

    Map* map = mapStorage_->GetAvailableMapAt(mPoint);
    if (map)
    {
        unsigned index;
        info_->Convert2WorldTileIndex(mPoint, worldPosition, index);
        return map->LineOfSightValue(index);
    }
    return 0;
}

unsigned World2D::GetLineOfSight(const WorldMapPosition& position)
{
    Map* map = mapStorage_->GetAvailableMapAt(position.mPoint_);
    if (map)
        return map->LineOfSightValue(position.tileIndex_);
    return 0;
}

PODVector<unsigned>* World2D::GetLosTable(const ShortIntVector2& mPoint)
{
    Map* map = mapStorage_->GetAvailableMapAt(mPoint);
    if (map)
        return &(map->GetLosTable());
    return 0;
}

void World2D::GetFilteredEntities(const ShortIntVector2& mPoint, PODVector<Node*>& entities, int skipControllerType)
{
    if (!world_)
        return;

    entities.Clear();

    List<unsigned>& ids = mapEntities_[mPoint].entities_;
    if (!ids.Size())
    {
        URHO3D_LOGWARNINGF("World2D() - GetEntities : mPoint=%s noentities !", mPoint.ToString().CString());
        return;
    }

    Scene* scene = world_->GetScene();
    Node* node;

    for (List<unsigned>::ConstIterator it=ids.Begin(); it!=ids.End(); ++it)
    {
        node = scene->GetNode(*it);

        if (!node || GOManager::IsA(*it, skipControllerType))
            continue;

        entities.Push(node);
    }
}

void World2D::GetEntities(const ShortIntVector2& mPoint, PODVector<Node*>& entities, const StringHash& type)
{
    if (!world_)
        return;

    entities.Clear();

    List<unsigned>& ids = mapEntities_[mPoint].entities_;
    if (!ids.Size())
    {
        URHO3D_LOGWARNINGF("World2D() - GetEntities : mPoint=%s noentities !", mPoint.ToString().CString());
        return;
    }

    Scene* scene = world_->GetScene();
    Node* node;

    for (List<unsigned>::ConstIterator it=ids.Begin(); it!=ids.End(); ++it)
    {
        node = scene->GetNode(*it);
        if (node && node->GetVar(GOA::GOT).GetUInt() == type.Value())
            entities.Push(node);
    }
}

void World2D::GetEntities(const ShortIntVector2& mPoint, PODVector<Node*>& entities, const char* name)
{
    if (!world_)
        return;

    entities.Clear();

    List<unsigned>& ids = mapEntities_[mPoint].entities_;
    if (!ids.Size())
    {
        URHO3D_LOGWARNINGF("World2D() - GetEntities : mPoint=%s noentities !", mPoint.ToString().CString());
        return;
    }

    Scene* scene = world_->GetScene();
    Node* node;

    for (List<unsigned>::ConstIterator it=ids.Begin(); it!=ids.End(); ++it)
    {
        node = scene->GetNode(*it);
        if (node)
        {
            if (node->GetName() == name)
                entities.Push(node);
        }
    }
}

void World2D::GetVisibleEntities(PODVector<Node*>& entities)
{
    if (!world_)
        return;

    Scene* scene = world_->GetScene();
    Node* node;
    ShortIntVector2 mpoint;
    int xmin, xmax, ymin, ymax;

    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (unsigned i = 0; i < numviewports; i++)
    {
        IntRect& visibleMapArea = world_->viewinfos_[i].visibleMapAreaMinimized_;

        xmin = visibleMapArea.left_;
        xmax = visibleMapArea.right_;
        ymin = visibleMapArea.top_;
        ymax = visibleMapArea.bottom_;

        for (int x = xmin; x <= xmax; x++)
        {
            mpoint.x_ = x;
            for (int y = ymin; y <= ymax; y++)
            {
                mpoint.y_ = y;

                const List<unsigned>& ids = GetEntities(mpoint);
                for (List<unsigned>::ConstIterator it=ids.Begin(); it!=ids.End(); ++it)
                {
                    node = scene->GetNode(*it);

                    if (!node || !node->IsEnabled())
                        continue;

                    if (entities.Contains(node))
                        continue;

                    entities.Push(node);
                }
            }
        }
    }
}

Node* World2D::GetEntitiesRootNode(Scene* scene, CreateMode mode)
{
    WeakPtr<Node>& node = entitiesRootNodes_[mode];
    if (!node)
    {
        Node* nodeScene = scene->GetChild(mode == LOCAL ? "LocalScene" : "ReplicatedScene");
        if (!nodeScene)
            nodeScene = scene->CreateChild(mode == LOCAL ? "LocalScene" : "ReplicatedScene", mode);

        node = nodeScene->GetChild("Entities");
        if (!node)
            node = nodeScene->CreateChild("Entities", mode);
        node->SetTemporary(true);
    }
    return node.Get();
}

Node* World2D::GetEntitiesNode(const ShortIntVector2& mPoint, CreateMode mode)
{
    WeakPtr<Node>& node = mapEntities_[mPoint].entitiesNode_[mode];
    if (!node)
    {
        String nodeName = ToString("Map_%d_%d", mPoint.x_, mPoint.y_);
        node = entitiesRootNodes_[mode]->GetChild(nodeName);
        if (!node)
            node = entitiesRootNodes_[mode]->CreateChild(nodeName, mode);
    }
    return node.Get();
}

PODVector<Node*> World2D::FindEntitiesAt(const String& entityName, const ShortIntVector2& mPoint, int viewZ)
{
    PODVector<Node*> entities;
    if (!world_)
        return entities;

    world_->GetEntities(mPoint, entities, entityName);

    if (!entities.Size())
    {
        URHO3D_LOGWARNINGF("World2D() - FindEntityAt : No %s nodes Found on mPoint=%s !", entityName.CString(), mPoint.ToString().CString());
        return entities;
    }

    int tryViewZ[2] =  { viewZ != FRONTVIEW ? INNERVIEW : FRONTVIEW, viewZ != FRONTVIEW ? FRONTVIEW : INNERVIEW };

    PODVector<Node*> filterEntities;
    for (unsigned i=0; i < 2; i++)
    {
        for (unsigned j=0; j < entities.Size(); j++)
        {
            if (entities[j]->GetVar(GOA::ONVIEWZ).GetInt() == tryViewZ[i])
            {
                URHO3D_LOGINFOF("World2D() - FindEntityAt : find a %s nodeID=%u on mPoint=%s viewZ=%d ...",
                                entityName.CString(), entities[j]->GetID(), mPoint.ToString().CString(), tryViewZ[i]);

                filterEntities.Push(entities[j]);
            }
        }
        if (filterEntities.Size())
            return filterEntities;
    }

    URHO3D_LOGWARNINGF("World2D() - FindEntityAt : No %s nodes Found on mPoint=%s viewZ=%d!", entityName.CString(), mPoint.ToString().CString(), viewZ);

    entities.Clear();
    return entities;
}

List<MapFurnitureRef> World2D::FindFurnituresAt(const ShortIntVector2& mPoint, unsigned tileindex)
{
    List<MapFurnitureRef> furnitures;
    HashMap<ShortIntVector2, MapFurnitureLocation>::ConstIterator it = mapFurnitures_.Find(mPoint);
    if (it != mapFurnitures_.End())
    {
        const MapFurnitureLocation& location = it->second_;
        MapFurnitureLocation::ConstIterator jt = location.Find(tileindex);
        if (jt != location.End())
        {
            furnitures = jt->second_;
            URHO3D_LOGINFOF("World2D() - FindFurnituresAt : find furnitures at mPoint=%s tileindex=%u!", mPoint.ToString().CString(), tileindex);
        }
    }
    else
    {
        URHO3D_LOGWARNINGF("World2D() - FindFurnituresAt : No furnitures at mPoint=%s !", mPoint.ToString().CString());
    }

    return furnitures;
}


/// Handlers

void World2D::Start()
{
    URHO3D_LOGINFOF("World2D() - Start !");

    SubscribeToEvent(WORLD_DIRTY, URHO3D_HANDLER(World2D, HandleChunksDirty));

#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
    SubscribeToEvent(GO_APPEAR, URHO3D_HANDLER(World2D, HandleObjectAppear));
    SubscribeToEvent(GO_CHANGEMAP, URHO3D_HANDLER(World2D,HandleObjectChangeMap));
    SubscribeToEvent(GO_DESTROY, URHO3D_HANDLER(World2D, HandleObjectDestroy));
#endif

    if (GameContext::Get().gameConfig_.fluidEnabled_)
        node_->GetChild("Fluid")->SetEnabled(true);
}

void World2D::Stop()
{
    UnsubscribeFromEvent(WORLD_DIRTY);
#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
    UnsubscribeFromEvent(GO_APPEAR);
    UnsubscribeFromEvent(GO_CHANGEMAP);
    UnsubscribeFromEvent(GO_DESTROY);
#endif

#ifdef HANDLE_BACKGROUND_LAYERS
    DrawableScroller::Stop();
#endif

    if (GameContext::Get().gameConfig_.fluidEnabled_)
    {
        Node* nodeFluid = node_->GetChild("Fluid");
        if (nodeFluid)
            nodeFluid->SetEnabled(false);
    }

//    SaveWorld(true);

    if (isSet_)
    {
//		  TimerRemover::Reset();
//		  ObjectPool::Reset();
        if (ObjectPool::Get())
            ObjectPool::Get()->RestoreCategories();

        Actor::RemoveActors();
    }

    URHO3D_LOGINFOF("World2D() - Stop !");
}


void World2D::OnSetEnabled()
{
    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
        {
            URHO3D_LOGINFOF("World2D() - OnSetEnabled WorldPoint=%s ... ", worldPoint_.ToString().CString());

            // Start here before loading map for Handle Appear Entities
            Start();

            ResetPosition(focusPosition_);

            URHO3D_LOGINFOF("World2D() - OnSetEnabled = true OK !");
        }
        else
        {
            Stop();

            URHO3D_LOGINFOF("World2D() - OnSetEnabled = false OK !");
        }
    }
}

void World2D::OnNodeSet(Node* node)
{
    if (!node)
        return;

    URHO3D_LOGINFOF("World2D() - OnNodeSet ...");

    Scene* scene = GetScene();
    if (!scene)
    {
        URHO3D_LOGERROR("World2D() - OnNodeSet ... NOK => No Scene!");
        return;
    }

    if (GameContext::Get().gameConfig_.fluidEnabled_)
    {
        Node* nodeFluid = node->CreateChild("Fluid", LOCAL);
        nodeFluid->SetTemporary(true);
        nodeFluid->CreateComponent<WaterLayer>(LOCAL);
    }

    // STATIC NODES
//    {
//        Node* nodeStatic = scene->GetChild("StaticScene");
//        if (!nodeStatic)
//            nodeStatic = scene->CreateChild("StaticScene", LOCAL);
//
//        if (nodeStatic)
//        {
//            nodeStatic->SetWorldPosition(node->GetWorldPosition());
//            nodeStatic->SetWorldScale(node->GetWorldScale());
//        }
//    }
    // LOCAL ENTITIES NODE
    {
        node = GetEntitiesRootNode(scene, LOCAL);
    }
    // REPLICATED ENTITIES NODE
    if (!GameContext::Get().gameNetwork_)
        entitiesRootNodes_[REPLICATED] = entitiesRootNodes_[LOCAL];
    else
        node = GetEntitiesRootNode(scene, REPLICATED);

    isSet_ = false;
    SetEnabled(false);

    URHO3D_LOGINFOF("World2D() - OnNodeSet OK !");
}

void World2D::OnViewportUpdated()
{
    if (!viewManager_)
        return;

    int viewport = 0;

    Camera* camera = viewManager_->GetCamera(viewport);
    WorldViewInfo& vinfo0 = viewinfos_[viewport];
    vinfo0.camera_ = camera;
    vinfo0.camOrtho_ = viewSpanFactor * camera->GetOrthoSize() / camera->GetZoom();
    vinfo0.camRatio_ = vinfo0.camOrtho_ / camera->GetAspectRatio();

    unsigned numviewports = viewManager_->GetNumViewports();
    if (numviewports > 1)
    {
        for (viewport=1; viewport < numviewports; viewport++)
        {
            WorldViewInfo& vinfo = viewinfos_[viewport];
            vinfo.camera_ = viewManager_->GetCamera(viewport);
            vinfo.camOrtho_ = vinfo0.camOrtho_;
            vinfo.camRatio_ = vinfo0.camRatio_;

            URHO3D_LOGINFOF("World2D() - OnViewportUpdated ... viewport=%d ortho=%f aspectratio=%f ratio=%f ... OK !", viewport, vinfo.camOrtho_, vinfo.camera_->GetAspectRatio(), vinfo.camRatio_);
        }
    }
}


void World2D::HandleChunksDirty(StringHash eventType, VariantMap& eventData)
{
    UpdateTextureLevels();
}


void World2D::HandleObjectAppear(StringHash eventType, VariantMap& eventData)
{
#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
    unsigned nodeId = eventData[Go_Appear::GO_ID].GetUInt();
    Node* node = GetScene()->GetNode(nodeId);
    if (!node)
        return;

    ShortIntVector2 mpoint(eventData[Go_Appear::GO_MAP].GetUInt());

    /// entities (furnitures is an entity too)
    {
        List<unsigned>& entities = GetEntities(mpoint);

//        DumpNodeList(entities, "list before adding entity :");

        if (!entities.Contains(nodeId))
        {
            entities.Push(nodeId);

//            URHO3D_LOGINFOF("World2D() - HandleObjectAppear : GO APPEAR node=%s(%u) type=%d mpoint=%s viewZ=%d entitiesInMap=%u",
//                            node->GetName().CString(), nodeId, eventData[Go_Appear::GO_TYPE].GetInt(), mpoint.ToString().CString(),
//                            node->GetVar(GOA::ONVIEWZ).GetInt(), entities.Size());

//            DumpNodeList(entities, "list after adding entity :");

            if (!IsInsideVisibleAreasMinimized(mpoint))
            {
//                URHO3D_LOGINFOF("World2D() - HandleObjectAppear : GO APPEAR node=%s(%u) on map=%d %d is OUT VISIBLE AREA => HIDE",
//    					  node->GetName().CString(), nodeId, mpoint.x_, mpoint.y_);

                node->SetEnabled(false);
            }
        }
        if (node->GetComponent<GOC_Destroyer>())
            node->GetComponent<GOC_Destroyer>()->WorldAppearCallBack();
        else
            node->SetVar(GOA::ONMAP, mpoint.ToHash());
    }
#endif
}

void World2D::HandleObjectChangeMap(StringHash eventType, VariantMap& eventData)
{
#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)
    unsigned nodeId = eventData[Go_ChangeMap::GO_ID].GetUInt();
    int goType = eventData[Go_ChangeMap::GO_TYPE].GetInt();
    ShortIntVector2 mapFrom(eventData[Go_ChangeMap::GO_MAPFROM].GetUInt());
    ShortIntVector2 mapTo(eventData[Go_ChangeMap::GO_MAPTO].GetUInt());

    Node* node = GetScene()->GetNode(nodeId);
    if (!node)
    {
        URHO3D_LOGERRORF("World2D() - HandleObjectChangeMap : nodeId=%u => node=0 !", nodeId);
        return;
    }

    if (mapFrom != mapTo)
    {
        List<unsigned>& entitiesFrom = GetEntities(mapFrom);
        List<unsigned>::Iterator it = entitiesFrom.Find(nodeId);
        if (it != entitiesFrom.End())
        {
            entitiesFrom.Erase(it);

            if (node->GetVar(GOA::ISDEAD).GetBool())
            {
                URHO3D_LOGERRORF("World2D() - HandleObjectChangeMap : nodeId=%u => is Dead remove from map=%s!", nodeId, mapFrom.ToString().CString());
                return;
            }

            GetEntities(mapTo).Push(nodeId);
        }
    }

//    URHO3D_LOGINFOF("World2D() - HandleObjectChangeMap : node=%s(%u) type=%d mapFrom=%s mapTo=%s ...",
//					  node->GetName().CString(), nodeId, goType, mapFrom.ToString().CString(), mapTo.ToString().CString());

#ifdef REPARENT_ENTITIES_ONMAP
//	if (!node->GetVar(GOA::ISMOUNTEDON).GetUInt())
    {
        AttachEntityToMapNode(node, mapTo, GameContext::Get().LocalMode_ || node->GetID() > LAST_REPLICATED_ID ? LOCAL : REPLICATED);
    }
#endif
    // if outside visible area, disable node
    // todo : transfer node info to a AI World update, for update entities in no visible areas
    if (!IsInsideVisibleAreasMinimized(mapTo))
    {
        if (node->GetVar(GOA::ISDEAD).GetBool())
        {
            URHO3D_LOGERRORF("World2D() - HandleObjectChangeMap : nodeId=%u => is Dead !", nodeId);
            node->SendEvent(WORLD_ENTITYDESTROY);
            return;
        }
        node->SetEnabled(false);
//        URHO3D_LOGINFOF("World2D() - HandleObjectChangeMap : node=%s(%u) position=%s enabled_=%s OUTSIDE visibleArea ... OK !", node->GetName().CString(), nodeId,
//                        node->GetWorldPosition().ToString().CString(), node->IsEnabled() ? "true" : "false");
    }
    else
    {
        node->SetEnabled(true);
//        URHO3D_LOGINFOF("World2D() - HandleObjectChangeMap : node=%s(%u) position=%s enabled_=%s INSIDE visibleArea ... OK !", node->GetName().CString(), nodeId,
//                        node->GetWorldPosition().ToString().CString(), node->IsEnabled() ? "true" : "false");
    }

//    DumpNodeList(GetEntities(mapFrom), "mapFrom");
//    DumpNodeList(GetEntities(mapTo), "mapTo="+mapTo.ToString()+" :");
#endif
}

void World2D::HandleObjectDestroy(StringHash eventType, VariantMap& eventData)
{
#if defined(HANDLE_ENTITIES) || defined(HANDLE_FURNITURES)

    Node* node = (Node*) eventData[Go_Destroy::GO_PTR].GetPtr();
    unsigned nodeId = !node ? eventData[Go_Destroy::GO_ID].GetUInt() : node->GetID();
    ShortIntVector2 mpoint(!node ? eventData[Go_Destroy::GO_MAP].GetUInt() : node->GetVar(GOA::ONMAP).GetUInt());

    /// entities (furnitures is an entity too)
    List<unsigned>& entities = GetEntities(mpoint);
    {
        List<unsigned>::Iterator it = entities.Find(nodeId);
        if (it == entities.End())
        {
            URHO3D_LOGWARNINGF("World2D() - HandleObjectDestroy : GO DESTROY node=%s(%u) no entity in mpoint=%s !!!",
                               node ? node->GetName().CString() : "", nodeId, mpoint.ToString().CString());
            return;
        }
        it = entities.Erase(it);
    }

    /// static furnitures (just for static)
    if (eventData[Go_Destroy::GO_TILE] != Variant::EMPTY)
    {
        List<MapFurnitureRef >& furnitures = mapFurnitures_[mpoint][eventData[Go_Destroy::GO_TILE].GetUInt()];
        List<MapFurnitureRef>::Iterator it;
        bool furnitureExists = false;
        for (it=furnitures.Begin(); it != furnitures.End(); ++it)
        {
            if (it->nodeid_ == nodeId)
            {
                furnitureExists = true;
                break;
            }
        }
        if (furnitureExists)
            furnitures.Erase(it);
    }

    if (node)
    {
        TimerRemover::Get()->Start(node, 0.1f, POOLRESTORE);
    }

//    URHO3D_LOGINFOF("World2D() - HandleObjectDestroy : GO DESTROY (ver GO_PTR) node=%s(%u) mpoint=%s",
//                    node ? node->GetName().CString() : "", nodeId, mpoint.ToString().CString());

//    DumpNodeList(entities, "map");
#endif
}


/// Updaters

// TODO multiviews ?
void World2D::SetKeepedVisibleMaps(bool state)
{
    URHO3D_LOGINFOF("World2D() - SetKeepedVisibleMaps ...");

    keepedVisibleMaps_.Clear();

    const HashMap<ShortIntVector2, Map* >& mapsInMemory = mapStorage_->GetMapsInMemory();
    for (HashMap<ShortIntVector2, Map*>::ConstIterator it=mapsInMemory.Begin(); it != mapsInMemory.End(); ++it)
    {
        if (state && it->second_->IsVisible())
        {
            URHO3D_LOGINFOF(" map=%s visible", it->first_.ToString().CString());
            keepedVisibleMaps_.Push(it->first_);
#ifdef USE_TILERENDERING
            it->second_->GetObjectTiled()->SetRenderPaused(0, state);
#endif
        }
#ifdef USE_TILERENDERING
        else
        {
            it->second_->GetObjectTiled()->SetRenderPaused(0, false);
        }
#endif
    }

    URHO3D_LOGINFOF("World2D() - SetKeepedVisibleMaps ... OK !");
}

void World2D::UpdateWorldBounds()
{
    noBounds_ = (worldSize_ == IntVector2::ZERO);

    if (noBounds_ == false)
    {
        info_->wBounds_.left_   = viewinfos_[0].dMapPoint_.x_ - (worldSize_.x_-1)/2;
        info_->wBounds_.right_  = viewinfos_[0].dMapPoint_.x_ + (worldSize_.x_)/2;
        info_->wBounds_.top_    = viewinfos_[0].dMapPoint_.y_ - (worldSize_.y_-1)/2;
        info_->wBounds_.bottom_ = viewinfos_[0].dMapPoint_.y_ + (worldSize_.y_)/2;

        worldBounds_ = info_->wBounds_;

        worldFloatBounds_.min_.x_ = ((float)worldBounds_.left_ - 0.0001f) * mWidth_;
        worldFloatBounds_.max_.x_ = ((float)worldBounds_.right_ + 1.0001f) * mWidth_;
        worldFloatBounds_.min_.y_ = ((float)worldBounds_.top_ - 0.0001f) * mHeight_ - mTileHeight_;
        worldFloatBounds_.max_.y_ = ((float)worldBounds_.bottom_ + 1.0001f) * mHeight_ + mTileHeight_;

        URHO3D_LOGINFOF("World2D() - UpdateWorldBounds : worldBounds=%s worldFloatBounds=%s", worldBounds_.ToString().CString(), worldFloatBounds_.ToString().CString());
    }
    else
    {
        worldBounds_ = info_->wBounds_ = IntRect::ZERO;

        worldFloatBounds_.Clear();

        URHO3D_LOGINFOF("World2D() - UpdateWorldBounds : Infinite World !");
    }
}

void World2D::UpdateTextureLevels()
{
    if (info_ && info_->atlas_)
        info_->atlas_->SetWorldInfo(info_);

    if (mapStorage_)
        mapStorage_->MarkMapDirty();
}


/// Map Visibility

static Rect world2DVisibleCollideRect_[MAX_VIEWPORTS];
static bool world2DVisibleCollideRectDirty_ = false;

void World2D::OnMapVisibleChanged(Map* updatedMap)
{
    unsigned numviewports = viewManager_->GetNumViewports();
    for (unsigned viewport=0; viewport < numviewports; viewport++)
    {
        // need to use effectiveVisibleMaps_ (can't use visibleAreaMaps_ because it is not updated when it's necessary <==> in World2D::UpdateVisibleLists the map to show is not always create or available)
        if (updatedMap->IsEffectiveVisible())
        {
            if (!effectiveVisibleMaps_[viewport].Contains(updatedMap))
            {
                effectiveVisibleMaps_[viewport].Push(updatedMap);
                world2DVisibleCollideRectDirty_ = true;
            }
        }
        else
        {
            if (effectiveVisibleMaps_[viewport].Contains(updatedMap))
            {
                effectiveVisibleMaps_[viewport].Remove(updatedMap);
                world2DVisibleCollideRectDirty_ = true;
            }
        }
    }
}

// TODO Multiviews
void World2D::UpdateVisibleCollideBorders()
{
    if (!world2DVisibleCollideRectDirty_)
        return;

    static PODVector<Vector2> sVisibleCollideBorderShape;
    sVisibleCollideBorderShape.Resize(4);

    // TODO : check intersection between viewrect
    // if overlaped areas, make a shape regrouping the rects

    unsigned numviewports = viewManager_->GetNumViewports();
    for (unsigned viewport=0; viewport < numviewports; viewport++)
    {
        WorldViewInfo& viewInfo = viewinfos_[viewport];

        // prepare the collide border for the viewport
        if (!viewInfo.visibleCollideBorder_)
        {
            String bordername = "WorldVisibleBorder" + String(viewport);

            Node* node = node_->GetChild(bordername, LOCAL);
            if (!node)
                node = node_->CreateChild(bordername, LOCAL);

            node->SetWorldPosition2D(node_->GetWorldPosition2D());
            node->SetWorldScale2D(node_->GetWorldScale2D());
            node->SetTemporary(true);

            RigidBody2D* borderbody = node->GetOrCreateComponent<RigidBody2D>(LOCAL);
            borderbody->SetBodyType(BT_STATIC);

            viewInfo.visibleCollideBorder_ = node->GetOrCreateComponent<CollisionChain2D>(LOCAL);
            // Collision with all other entities
            viewInfo.visibleCollideBorder_->SetFilterBits(CC_ALLWALLS, CM_ALLWALL);
            // No collision with mapcollider collisionchains
            viewInfo.visibleCollideBorder_->SetGroupIndex(-1);
            viewInfo.visibleCollideBorder_->SetLoop(true);

            URHO3D_LOGERRORF("World2D() - UpdateVisibleCollideBorders : Create Border Node and CollisionChain !");
        }

        // get the world rect really shown for the viewport
        Vector<Map*>& effectiveVisibleMaps = effectiveVisibleMaps_[viewport];
        Rect visiblerect;

        for (unsigned i=0; i < effectiveVisibleMaps.Size(); i++)
        {
            Map* map = effectiveVisibleMaps[i];

            if (visiblerect.Defined())
                visiblerect.Merge(map->GetBounds());
            else
                visiblerect.Define(map->GetBounds());
        }

        if (visiblerect.Defined())
        {
            Vector2 minscaled = visiblerect.min_ / node_->GetWorldScale2D();
            Vector2 maxscaled = visiblerect.max_ / node_->GetWorldScale2D();
            sVisibleCollideBorderShape[0] = Vector2(minscaled.x_, minscaled.y_);
            sVisibleCollideBorderShape[1] = Vector2(minscaled.x_, maxscaled.y_);
            sVisibleCollideBorderShape[2] = Vector2(maxscaled.x_, maxscaled.y_);
            sVisibleCollideBorderShape[3] = Vector2(maxscaled.x_, minscaled.y_);
            viewInfo.visibleCollideBorder_->SetVertices(sVisibleCollideBorderShape);
            viewInfo.visibleCollideBorder_->SetEnabled(true);

            world2DVisibleCollideRect_[viewport] = visiblerect;
        }
        else
        {
            viewInfo.visibleCollideBorder_->SetEnabled(false);

            world2DVisibleCollideRect_[viewport].Clear();
        }
    }

    world2DVisibleCollideRectDirty_ = false;

//    URHO3D_LOGERRORF("World2D() - UpdateVisibleCollideBorders : update box p[0]=%s p[2]=%s !", sVisibleCollideBorderShape[0].ToString().CString(), sVisibleCollideBorderShape[2].ToString().CString());
}

// Update Visible Maps List

void World2D::UpdateVisibleLists(int viewport)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(World2D_SetVisibleArea);
#endif

    WorldViewInfo& viewInfo = viewinfos_[viewport];

    // Set Hide maps List
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(World2D_SetMapListToHide);
#endif
        const HashMap<ShortIntVector2, Map* >& mapsInMemory = mapStorage_->GetMapsInMemory();
        for (HashMap<ShortIntVector2, Map*>::ConstIterator it=mapsInMemory.Begin(); it != mapsInMemory.End(); ++it)
        {
            Map* map = it->second_;

            if (IsInsideVisibleAreasMinimized(map->GetMapPoint()))
                continue;

            if (map->GetStatus() < Available)
                continue;

            viewInfo.visibleAreaMaps_.RemoveSwap(map);

            // if in mapToShow, remove it
            Vector<ShortIntVector2>::Iterator jt = viewInfo.mapsToShow_.Find(map->GetMapPoint());
            if (jt != viewInfo.mapsToShow_.End())
                viewInfo.mapsToShow_.EraseSwap(jt-viewInfo.mapsToShow_.Begin());

            jt = viewInfo.mapsToHide_.Find(map->GetMapPoint());
            if (jt == viewInfo.mapsToHide_.End())
            {
                URHO3D_LOGINFOF("World2D() - UpdateVisibleLists : viewport=%d numVisibleMaps=%u Push To Hide %s ...", viewport, viewInfo.visibleAreaMaps_.Size(), map->GetMapPoint().ToString().CString());

                viewInfo.mapsToHide_.Push(map->GetMapPoint());
            }
        }
    }

    // Set Show maps List
    {
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(World2D_SetMapListToShow);
#endif
        ShortIntVector2 mpoint;

//		URHO3D_LOGINFOF("World2D() - UpdateVisibleLists : viewport=%d numVisibleMaps=%u mpoint=%s mposition=%s ...",
//							viewport, viewInfo.visibleAreaMaps_.Size(), viewInfo.mPoint_.ToString().CString(), viewInfo.mPosition_.ToString().CString());

        const IntRect& visibleMapArea = viewInfo.visibleMapAreaMinimized_;
        for (int x = visibleMapArea.left_; x <= visibleMapArea.right_; x++)
        {
            mpoint.x_ = x;
            for (int y = visibleMapArea.top_; y <= visibleMapArea.bottom_; y++)
            {
                mpoint.y_ = y;

                // if in mapsToHide, remove it
                Vector<ShortIntVector2>::Iterator it;

                it = viewInfo.mapsToHide_.Find(mpoint);
                if (it != viewInfo.mapsToHide_.End())
                    viewInfo.mapsToHide_.EraseSwap(it-viewInfo.mapsToHide_.Begin());

                // Add map to visible area maps list even if not available
                // Allow to priorize this map in MapCreator Update
                Map* map = mapStorage_->GetMapAt(mpoint);
                if (map && map->GetStatus() <= Available && !viewInfo.visibleAreaMaps_.Contains(map))
                    viewInfo.visibleAreaMaps_.Push(map);

                it = viewInfo.mapsToShow_.Find(mpoint);
                if (it == viewInfo.mapsToShow_.End())
                {
                    URHO3D_LOGINFOF("World2D() - UpdateVisibleLists : viewport=%d numVisibleMaps=%u Push To Show %s ...", viewInfo.viewport_, viewInfo.visibleAreaMaps_.Size(), mpoint.ToString().CString());
                    viewInfo.mapsToShow_.Push(mpoint);
                }
            }
        }
    }
}

void World2D::UpdateVisibleArea(int viewport, HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(World2D_UpdateVisibleArea);
#endif

    WorldViewInfo& viewInfo = viewinfos_[viewport];

    if (!viewInfo.cameraFocusEnabled_)
        return;

    // Hide Maps
    if (viewInfo.mapsToHide_.Size())
    {
//        URHO3D_LOGINFOF("World2D() - UpdateVisibleArea ... HideMaps ... %u", mapsToHide.Size());
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(World2D_HideMaps);
#endif
        Map* map;

        for (Vector<ShortIntVector2>::ConstIterator it = viewInfo.mapsToHide_.Begin(); it != viewInfo.mapsToHide_.End(); ++it)
        {
            /// Never Hide the Maps To Keep
            if (GetKeepedVisibleMaps().Contains(*it))
            {
//                URHO3D_LOGINFOF("World2D() - UpdateVisibleArea : Keep Visible %s !", it->ToString().CString());
                continue;
            }

            map = mapStorage_->GetAvailableMapAt(*it);
            if (!map)
                continue;

            /// Skip if the map is in visible areas
            if (IsInsideVisibleAreasMinimized(*it))
                continue;

            map->HideMap(timer);
//            URHO3D_LOGINFOF("World2D() - UpdateVisibleArea : map=%s IsVisible_Map=false !", it->ToString().CString());
        }
//        URHO3D_LOGINFOF("World2D() - UpdateVisibleArea ... toHide ... timer=%d mSec !", timer ? timer->GetUSec(false)/1000 : 0);
    }

    // Show Maps
    // todo : ordering list mapsToShow

    if (viewInfo.mapsToShow_.Size())
    {
//        URHO3D_LOGINFOF("World2D() - UpdateVisibleArea ... ShowMaps ... %u", mapsToShow.Size());
#ifdef ACTIVE_WORLD2D_PROFILING
        URHO3D_PROFILE(World2D_ShowMaps);
#endif
        Map* map;

        for (Vector<ShortIntVector2>::ConstIterator it = viewInfo.mapsToShow_.Begin(); it != viewInfo.mapsToShow_.End(); ++it)
        {
            map = mapStorage_->GetAvailableMapAt(*it);
            if (!map)
                continue;

            map->ShowMap(timer);
        }
//        URHO3D_LOGINFOF("World2D() - UpdateVisibleArea ... toShow ... timer=%d mSec !", timer ? timer->GetUSec(false)/1000 : 0);
    }
}

void World2D::UpdateVisibleRectInfos(int viewport)
{
    WorldViewInfo& viewInfo = viewinfos_[viewport];

    if (!viewInfo.cameraFocusEnabled_)
        return;

    unsigned numMapsVisible = 0;
    unsigned numMapsInFullBackground = 0;

    const unsigned numMaps = viewInfo.visibleAreaMaps_.Size();
    for (unsigned i=0; i < numMaps; i++)
    {
        Map* map = viewInfo.visibleAreaMaps_[i];

        if (map && map->GetStatus() > Creating_Map_Layers)
        {
            numMapsVisible++;
            if (map->GetTopography().IsFullGround())
                numMapsInFullBackground++;
        }
    }

    // Update FullBackGround
    viewInfo.visibleRectInFullBackground_ = (numMapsVisible == numMapsInFullBackground);

//    URHO3D_LOGINFOF("World2D() - UpdateVisibleRectInfos : viewport=%d numVisibleMaps=%u numMapsInFullBackground=%u !", viewport, numMapsVisible, numMapsInFullBackground);
}

void World2D::UpdateActors(HiresTimer* timer)
{
    const Vector<WeakPtr<Actor> >& actors = Actor::GetActors();

    for (unsigned i=GameContext::Get().MAX_NUMPLAYERS; i < actors.Size(); i++)
    {
        Actor* actor = actors[i];
        if (!actor)
            continue;

        ActorInfo& actorinfo = actor->GetInfo();

        if (actorinfo.state_ == Dead)
            continue;

        Node* avatar = actor->GetAvatar();

        // check for activating avatar
        if (actorinfo.state_ == Desactivated)
        {
            Map* map = mapStorage_->GetAvailableMapAt(actorinfo.position_.mPoint_);
            if (map && map->IsEffectiveVisible())
            {
                if (!avatar)
                {
                    info_->UpdateWorldPosition(actorinfo.position_);
                    actor->SetScene(GameContext::Get().rootScene_, actorinfo.position_.position_, actorinfo.position_.viewZ_);
                    if (actor->GetAvatar())
                        actor->SetDialogue(actorinfo.dialog_);
                    else
                        actorinfo.state_ = Dead;
                }
                if (actor->GetAvatar())
                {
                    actor->Start(!avatar);
                    actor->ResetDialogue();
                    actorinfo.state_ = Activated;
                    URHO3D_LOGINFOF("World2D() - UpdateActors : actor name=%s ID=%u avatar activated !", actor->GetAvatar()->GetName().CString(), actor->GetID());
                }
            }
        }
        // check for desactivating avatar
        else if (actorinfo.state_ == Activated && avatar && !avatar->IsEnabled())
        {
            Map* map = mapStorage_->GetAvailableMapAt(actorinfo.position_.mPoint_);
            if (!map || !map->IsEffectiveVisible())
            {
                GOC_Destroyer* destroyer = avatar->GetComponent<GOC_Destroyer>();
                // update actorinfo position
                actorinfo.position_ = destroyer->GetWorldMapPosition();
                // stop actor
                actor->StopSubscribers();
                actorinfo.state_ = Desactivated;

                URHO3D_LOGINFOF("World2D() - UpdateActors : actor name=%s ID=%u avatar desactivated !", avatar->GetName().CString(), actor->GetID());
            }
        }
    }
}

void World2D::UpdateMaps(HiresTimer* timer)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(World2D_UpdateVisibleMaps);
#endif

    const HashMap<ShortIntVector2, Map* >& mapsInMemory = mapStorage_->GetMapsInMemory();
    for (HashMap<ShortIntVector2, Map* >::ConstIterator it=mapsInMemory.Begin(); it!=mapsInMemory.End(); ++it)
        if (!it->second_->Update(timer))
            break;
}

static HiresTimer timer_;

bool World2D::UpdateLoading()
{
    timer_.Reset();
    return mapStorage_->UpdateMapsInMemory(&timer_);
}

static unsigned lastframe_ = 1000U;

void World2D::UpdateStep(float timestep)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(World2D_UpdateStep);
#endif

    if (!viewManager_)
        return;

//    if (lastframe_ == GameContext::Get().renderer2d_->GetFrameInfo().frameNumber_)
//    {
//        URHO3D_LOGERRORF("World2D() - UpdateStep : another update in the same frame => SKIP !");
//        return;
//    }

    lastframe_ = GameContext::Get().renderer2d_->GetFrameInfo().frameNumber_;
    timer_.Reset();

    unsigned numviewports = viewManager_->GetNumViewports();

    for (unsigned viewport=0; viewport < numviewports; viewport++)
    {
        WorldViewInfo& viewInfo = viewinfos_[viewport];

        if (timestep && viewInfo.cameraFocusEnabled_)
            viewManager_->MoveCamera(viewport, timestep);

        // if not cameraFocusEnabled_, use dMapPosition_ (used by UpdateInstant)
        Vector3 posCam;
        if (viewInfo.camera_ && viewInfo.cameraFocusEnabled_)
            posCam = viewInfo.camera_->GetNode()->GetPosition();
        else
            posCam = Vector3(viewInfo.dMapPosition_);

//#ifdef ACTIVE_WORLD2D_DYNAMICZOOM
//        viewInfo.camOrtho_ = viewSpanFactor * viewInfo.camera_->GetOrthoSize() / viewInfo.camera_->GetZoom();
//        viewInfo.camRatio_ = viewInfo.camOrtho_ / viewInfo.camera_->GetAspectRatio();
//#endif

        Rect newvisibleRect;
        if (viewInfo.cameraFocusEnabled_)
        {
#if defined(ACTIVE_WORLD2D_DYNAMICZOOM)
            const Frustum& frustum = viewInfo.camera_->GetFrustum();
            newvisibleRect.Define(frustum.vertices_[2].x_ - VisibleRectOverscan, frustum.vertices_[2].y_ - VisibleRectOverscan,
                                  frustum.vertices_[0].x_ + VisibleRectOverscan, frustum.vertices_[0].y_ + VisibleRectOverscan);
#else
            newvisibleRect.Define(posCam.x_ - viewInfo.camOrtho_ - VisibleRectOverscan, posCam.y_ - viewInfo.camRatio_ - VisibleRectOverscan,
                                  posCam.x_ + viewInfo.camOrtho_ + VisibleRectOverscan, posCam.y_ + viewInfo.camRatio_ + VisibleRectOverscan);
#endif
        }
        else
        {
            newvisibleRect.Define(posCam.x_ - viewInfo.camOrtho_ - VisibleRectOverscan, posCam.y_ - viewInfo.camRatio_ - VisibleRectOverscan,
                                  posCam.x_ + viewInfo.camOrtho_ + VisibleRectOverscan, posCam.y_ + viewInfo.camRatio_ + VisibleRectOverscan);
        }

        Rect& visibleRect = viewInfo.extVisibleRect_;

        if (newvisibleRect != visibleRect)
        {
            visibleRect = newvisibleRect;
#if defined(ACTIVE_WORLD2D_DYNAMICZOOM)
            const Frustum& frustum = viewInfo.camera_->GetFrustum();
            viewInfo.visibleRect_.Define(frustum.vertices_[2].x_, frustum.vertices_[2].y_, frustum.vertices_[0].x_, frustum.vertices_[0].y_);
#else
            viewInfo.visibleRect_.Define(posCam.x_ - viewInfo.camOrtho_, posCam.y_ - viewInfo.camRatio_, posCam.x_ + viewInfo.camOrtho_, posCam.y_ + viewInfo.camRatio_);
#endif
            // Set map point
            ShortIntVector2 newmPoint;
            info_->Convert2WorldMapPoint(posCam.x_, posCam.y_, newmPoint);

            // Set position in the map
            IntVector2 newmPosition;
            info_->Convert2WorldMapPosition(newmPoint, posCam.x_, posCam.y_, newmPosition);
            viewInfo.mPosition_ = newmPosition;

            if (newmPoint != viewInfo.mPoint_)
            {
                viewInfo.mPoint_ = newmPoint;

                URHO3D_LOGINFOF("World2D() - UpdateStep : viewport=%u onMap=%s posCam=%s", viewport, viewInfo.mPoint_.ToString().CString(), posCam.ToString().CString());

                viewInfo.currentMap_ = GetMapAt(viewInfo.mPoint_, true);
                viewInfo.needUpdateCurrentMap_ = true;

                GAME_SETGAMELOGENABLE(GAMELOG_WORLDUPDATE, false);

                // Update Buffer Area
                mapStorage_->UpdateBufferedArea(viewport, viewInfo.mPoint_);

                GAME_SETGAMELOGENABLE(GAMELOG_WORLDUPDATE, true);
            }

            IntRect newvisibleMapArea;

            // Set visible Area
            if (noBounds_)
                newvisibleMapArea = IntRect((int)floor(newvisibleRect.min_.x_ / mWidth_), (int)floor(newvisibleRect.min_.y_ / mHeight_),
                                            (int)floor(newvisibleRect.max_.x_ / mWidth_), (int)floor(newvisibleRect.max_.y_ / mHeight_));
            else
                newvisibleMapArea = IntRect(Max((int)floor(newvisibleRect.min_.x_ / mWidth_), worldBounds_.left_),  Max((int)floor(newvisibleRect.min_.y_ / mHeight_), worldBounds_.top_),
                                            Min((int)floor(newvisibleRect.max_.x_ / mWidth_), worldBounds_.right_), Min((int)floor(newvisibleRect.max_.y_ / mHeight_), worldBounds_.bottom_));

            viewInfo.tiledVisibleRect_.min_.x_ = (floor(newvisibleRect.min_.x_ / mTileWidth_)  + 0.01f) * mTileWidth_;
            viewInfo.tiledVisibleRect_.min_.y_ = (floor(newvisibleRect.min_.y_ / mTileHeight_) + 0.01f) * mTileHeight_;
            viewInfo.tiledVisibleRect_.max_.x_ = (floor(newvisibleRect.max_.x_ / mTileWidth_)  - 0.01f) * mTileWidth_;
            viewInfo.tiledVisibleRect_.max_.y_ = (floor(newvisibleRect.max_.y_ / mTileHeight_) - 0.01f) * mTileHeight_;

            if (newvisibleMapArea != viewInfo.visibleMapArea_)
            {
                viewInfo.visibleMapArea_ = newvisibleMapArea;
                IntVector2 size = viewInfo.visibleMapArea_.Size();
                if (size.x_ * size.y_ > viewInfo.visibleAreaMaps_.Capacity())
                {
                    const unsigned w = Sqrt(viewInfo.visibleAreaMaps_.Capacity());
                    const unsigned hw = w / 2;
                    const IntVector2 center = viewInfo.visibleMapArea_.Center();
                    viewInfo.visibleMapAreaMinimized_.left_ = center.x_ - hw;
                    viewInfo.visibleMapAreaMinimized_.top_ = center.y_ - hw;
                    viewInfo.visibleMapAreaMinimized_.right_ = viewInfo.visibleMapAreaMinimized_.left_ + w -1;
                    viewInfo.visibleMapAreaMinimized_.bottom_ = viewInfo.visibleMapAreaMinimized_.top_ + w -1;
                }
                else
                {
                    viewInfo.visibleMapAreaMinimized_ = viewInfo.visibleMapArea_;
                }

                URHO3D_LOGINFOF("World2D() - UpdateStep : viewport=%u mpoint=%s visibleArea=%s center=%s minimized=%s center=%s",
                                viewport, viewInfo.mPoint_.ToString().CString(),
                                viewInfo.visibleMapArea_.ToString().CString(), viewInfo.visibleMapArea_.Center().ToString().CString(),
                                viewInfo.visibleMapAreaMinimized_.ToString().CString(), viewInfo.visibleMapAreaMinimized_.Center().ToString().CString());
            }
        }

        if (!viewInfo.currentMap_ || viewInfo.needUpdateCurrentMap_)
        {
            if (!viewInfo.currentMap_)
            {
                viewInfo.currentMap_ = GetMapAt(viewInfo.mPoint_, true);
            }

            if (viewInfo.currentMap_)
            {
                viewInfo.needUpdateCurrentMap_ = false;
                Vector2 vcenter = visibleRect.Center();
                const MapTopography& topo = viewInfo.currentMap_->GetTopography();
                viewInfo.isUnderground_ = vcenter.y_ < topo.maporigin_.y_ + topo.GetFloorY(vcenter.x_ - topo.maporigin_.x_);
            }
        }
    }

    GAME_SETGAMELOGENABLE(GAMELOG_WORLDUPDATE, false);

    for (unsigned viewport=0; viewport < numviewports; viewport++)
        UpdateVisibleLists(viewport);

    for (unsigned viewport=0; viewport < numviewports; viewport++)
        UpdateVisibleArea(viewport, &timer_);

    UpdateVisibleCollideBorders();

    GAME_SETGAMELOGENABLE(GAMELOG_WORLDUPDATE, true);

    // if no timestep skip initialize maps
    // => using UpdateStep(0.f) in UpdateAll to set buffer only (Before Updating All Maps, in a second step)
    if (timestep)
        mapStorage_->UpdateMapsInMemory(&timer_);

    for (unsigned viewport=0; viewport < numviewports; viewport++)
        UpdateVisibleRectInfos(viewport);

    UpdateActors(&timer_);

//    if ((int)(timer_.GetUSec(false) / 1000) > (int)(World2DInfo::delayUpdateUsec_/1000))
//        URHO3D_LOGWARNINGF("World2D() - UpdateStep : timer=%d/%d", (int)(timer_.GetUSec(false) / 1000), (int)(World2DInfo::delayUpdateUsec_/1000));
}

bool World2D::IsVisible() const
{
    unsigned numviewports = viewManager_->GetNumViewports();

    for (unsigned viewport=0; viewport < numviewports; viewport++)
    {
        const IntRect& visibleMapArea = viewinfos_[viewport].visibleMapAreaMinimized_;

        for (int x = visibleMapArea.left_; x <= visibleMapArea.right_; x++)
        {
            for (int y = visibleMapArea.top_; y <= visibleMapArea.bottom_; y++)
            {
                Map* map = mapStorage_->GetMapAt(ShortIntVector2(x, y));
                if (!map || !map->IsEffectiveVisible())
                {
                    return false;
                    break;
                }
            }
        }
    }

    return true;
}

bool World2D::UpdateVisibility(float timestep)
{
    UpdateStep(timestep);

    return IsVisible();
}

void World2D::UpdateInstant(int viewport, const Vector2& position, float timestep, bool sendevent)
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(World2D_UpdateInstant);
#endif

    WorldViewInfo& viewInfo = viewinfos_[viewport];

    Vector<ShortIntVector2>& mapsToShow = viewInfo.mapsToShow_;

    viewInfo.cameraFocusEnabled_ = false;

    if (position != Vector2::ZERO)
        viewInfo.dMapPosition_ = position;

    ShortIntVector2 point;
    info_->Convert2WorldMapPoint(viewInfo.dMapPosition_, point);

    URHO3D_LOGINFOF("--------------------------------------------------------------");
    URHO3D_LOGINFOF("- World2D() - UpdateInstant : viewport=%d position=%s point=%s ... -", viewport, viewInfo.dMapPosition_.ToString().CString(), point.ToString().CString());
    URHO3D_LOGINFOF("--------------------------------------------------------------");

    // check if workers unfinished ?
    World2DInfo* info = World2D::GetWorldInfo();
    if (info && info->worldModel_)
    {
        URHO3D_LOGERRORF("World2D() - UpdateInstant : viewport=%d position=%s point=%s ... waiting Threads ... ", viewport, viewInfo.dMapPosition_.ToString().CString(), point.ToString().CString());

        info->worldModel_->StopUnfinishedWorks();

        while (!info->worldModel_->AreThreadFinished()) { ; }
    }

    UpdateStep(timestep);

    if (mapsToShow.Size())
    {
        URHO3D_LOGINFOF("World2D() - UpdateInstant : viewport=%d position=%s point=%s ... Maps to Show = %u ...", viewport, viewInfo.dMapPosition_.ToString().CString(), point.ToString().CString(), mapsToShow.Size());

        Map* map;
        // Maps must be in Available State (to have borders setted) before set visibles

        // First : Update map loading
        mapStorage_->SetCreatingMode(MCM_INSTANT);
        MapCreator* creator = MapStorage::GetCreator();
        for (Vector<ShortIntVector2>::Iterator it = mapsToShow.Begin(); it != mapsToShow.End(); ++it)
        {
            map = GetMapAt(*it, true);
            if (!map)
                continue;

            creator->AddMapToCreate(*it);

            URHO3D_LOGINFOF("World2D() - UpdateInstant : viewport=%d position=%s point=%s ... update map=%s ...", viewport, viewInfo.dMapPosition_.ToString().CString(), point.ToString().CString(), it->ToString().CString());

            while (map->GetStatus() != Available && creator->IsRunning())
            {
                mapStorage_->UpdateMapsInMemory();
            }
        }
        mapStorage_->SetCreatingMode(MCM_ASYNCHRONOUS);

        // Second : Show maps
//        Vector<ShortIntVector2>::Iterator it = mapsToShow.Begin();
//        while (it != mapsToShow.End())
//        {
//            map = GetMapAt(*it);
//
//            URHO3D_LOGINFOF("World2D() - UpdateInstant : viewport=%d position=%s point=%s ... show map=%s %u ...", viewport, viewInfo.dMapPosition_.ToString().CString(), point.ToString().CString(), it->ToString().CString(), map);
//
//            if (map)
//            {
//                map->ShowMap(0);
//                mapsToShow.EraseSwap(it-mapsToShow.Begin());
//            }
//            else
//                it++;
//        }
    }

    viewInfo.cameraFocusEnabled_ = true;

    UpdateVisibleArea(viewport, 0);

    if (sendevent)
        SendEvent(WORLD_CAMERACHANGED);

    URHO3D_LOGINFOF("---------------------------------------------------");
    URHO3D_LOGINFOF("- World2D() - UpdateInstant : viewport=%d point=%s ... OK ! -", viewport, point.ToString().CString());
    URHO3D_LOGINFOF("---------------------------------------------------");
}

void World2D::UpdateAll()
{
#ifdef ACTIVE_WORLD2D_PROFILING
    URHO3D_PROFILE(World2D_UpdateAll);
#endif

    URHO3D_LOGINFOF("World2D() ---------------");
    URHO3D_LOGINFOF("World2D() - UpdateAll ...");
    URHO3D_LOGINFOF("World2D() ---------------");

    timer_.Reset();

    // First Step :: set buffer and visible lists
    UpdateStep(0.f);

//	mapStorage_->DumpMapsMemory();

    URHO3D_LOGINFOF("World2D() - UpdateAll : ... UpdateStep OK !");

    mapStorage_->UpdateAllMaps();

//	mapStorage_->DumpMapsMemory();

    URHO3D_LOGINFOF("World2D() - UpdateAll : ... UpdateAllMaps OK !");

    GAME_SETGAMELOGENABLE(GAMELOG_WORLDUPDATE, false);

    unsigned numviewports = viewManager_->GetNumViewports();
    for (unsigned viewport=0; viewport < numviewports; viewport++)
        UpdateVisibleArea(viewport, 0);

    GAME_SETGAMELOGENABLE(GAMELOG_WORLDUPDATE, true);

    URHO3D_LOGINFOF("World2D() - UpdateAll : ... UpdateVisibleArea OK !");

    URHO3D_LOGINFOF("World2D() ----------------");
    URHO3D_LOGINFOF("World2D() - UpdateAll OK ! timer_ = %u msec", timer_.GetUSec(false)/1000);
    URHO3D_LOGINFOF("World2D() ----------------");

#ifdef ACTIVE_SCROLLINGSHAPE_FRUSTUMCAMERA
    // Force Update Camera Dirty -> Recalculate Frustum
    GameContext::Get().camera_->SetOrthographic(true);
    GameContext::Get().renderer2d_->UpdateFrustumBoundingBox(GameContext::Get().camera_);
#endif

    SendEvent(WORLD_MAPUPDATE);
    SendEvent(WORLD_CAMERACHANGED);
}


/// Debug

void World2D::SetEnableDrawDebug(bool enable)
{
#ifdef DRAWDEBUG_MAPPOOL
    world2DDebugPoolText_->SetVisible(enable);
#endif // DRAWDEBUG_MAPPOOL
}

void World2D::DrawDebugGeometry(DebugRenderer* debug, bool activeTagText)
{
//    URHO3D_LOGINFOF("World2D::DrawDebugGeometry : debug=%u activeTagText=%s ...", debug, activeTagText?"true":"false");

    if (debug)
    {
        unsigned numviewports = viewManager_ ? viewManager_->GetNumViewports() : 1;
        for (unsigned viewport=0; viewport < numviewports; viewport++)
        {
            const WorldViewInfo& viewInfo = viewinfos_[viewport];
            GameHelpers::DrawDebugRect(mapStorage_->GetBufferedAreaRect(viewport).Adjusted(5.f), debug, false, Color::GRAY);
#ifdef ACTIVE_WORLD2D_DYNAMICZOOM
            if (viewInfo.visibleMapAreaMinimized_ != viewInfo.visibleMapArea_)
            {
                Rect rect;
                rect.min_.x_ = viewInfo.visibleMapAreaMinimized_.left_ * info_->mWidth_;
                rect.min_.y_ = viewInfo.visibleMapAreaMinimized_.top_ * info_->mHeight_;
                rect.max_.x_ = (viewInfo.visibleMapAreaMinimized_.right_+1) * info_->mWidth_;
                rect.max_.y_ = (viewInfo.visibleMapAreaMinimized_.bottom_+1) * info_->mHeight_;
                GameHelpers::DrawDebugRect(rect, debug, false, Color::BLUE);
            }
            else
            {
                GameHelpers::DrawDebugRect(viewInfo.visibleRect_.Adjusted(-0.25f), debug, false, Color::BLUE);
            }
#else
            GameHelpers::DrawDebugRect(viewinfos_[viewport].extVisibleRect_, debug, false, Color::BLUE);
            GameHelpers::DrawDebugRect(viewInfo.visibleRect_.Adjusted(-0.25f), debug, false, Color::BLUE);

//            GameHelpers::DrawDebugRect(Rect(GameContext::Get().renderer2d_->GetFrustumBoundingBox().min_.ToVector2(), GameContext::Get().renderer2d_->GetFrustumBoundingBox().max_.ToVector2()).Adjusted(-0.25f), debug, false, Color::RED);
#endif
        }

        if (delayTemporaryDrawRect_ > 0.f)
        {
            debug->AddCross(Vector3(debugTemporaryPoint_), 2.f, Color::YELLOW, false);
            GameHelpers::DrawDebugRect(debugTemporaryDrawRect_, debug, false, Color::YELLOW);
            delayTemporaryDrawRect_ -= 0.1f;
        }

        if (!noBounds_)
            GameHelpers::DrawDebugRect(worldFloatBounds_, debug, false, Color::RED);
        else
        {
            for (unsigned viewport=0; viewport < numviewports; viewport++)
                GameHelpers::DrawDebugRect(world2DVisibleCollideRect_[viewport].Adjusted(-1.f), debug, false, Color::RED);
        }

#ifdef DRAWDEBUG_MAPBORDER
        const HashMap<ShortIntVector2, Map* >& maps = mapStorage_->GetMapsInMemory();
        for (HashMap<ShortIntVector2, Map* >::ConstIterator it=maps.Begin(); it != maps.End(); ++it)
        {
            Map* map = it->second_;
            if (map)
            {
                int status = map->GetStatus();

                Color color;
                if (status < Generating_Map)
                    color = Color::BLUE;
                else if (status < Available)
                    color = Color::MAGENTA;
                else if (status == Available)
                    color = map->IsVisible() ? Color::GREEN : Color::YELLOW;
                else if (status > Available)
                    color = Color::RED;

                GameHelpers::DrawDebugRect(map->GetBounds(), debug, false, color);
            }
        }

        if (viewinfos_[0].currentMap_ && viewinfos_[0].currentMap_->GetRootNode())
            GameHelpers::DrawDebugRect(viewinfos_[0].currentMap_->GetBounds().Adjusted(-1.f), debug, false, Color::CYAN);

//        GameHelpers::DrawDebugRect(originMapRect_, debug, false, Color::WHITE);
#endif

#ifdef DRAWDEBUG_MAPPOOL
        {
            MapPool* mappool = mapStorage_->GetPool();
            String text;
            text.AppendWithFormat("Free Game Objects : \n Maps(%u/%u)\n%s\n\n", mappool->GetFreeSize(), mappool->GetSize(), ObjectPool::GetDebugData().CString());
            world2DDebugPoolText_->SetText(text);
        }
#endif

//        /// For Test : Draw BackImage BoundingBox
//        for (HashMap<ShortIntVector2, Map* >::ConstIterator it=maps.Begin(); it != maps.End(); ++it)
//        {
//            Map* map = it->second_;
//            if (!map) continue;
//
//            const Vector<Node*>& imagelayers = map->GetImageNodes();
//            for (int i=0; i < imagelayers.Size(); i++)
//            {
//                Drawable* drawable = imagelayers[i]->GetDerivedComponent<Drawable>();
//                if (drawable)
//                    debug->AddBoundingBox(drawable->GetWorldBoundingBox(), Color::BLUE, false);
//            }
//        }

#ifdef DRAWDEBUG_MAPTOPOGRAPHY
        if (viewinfos_[0].currentMap_ && viewinfos_[0].currentMap_->GetRootNode())
        {
            const MapTopography& topo = viewinfos_[0].currentMap_->GetTopography();

            /// For Test : Draw Top Free Tiles
//            const PODVector<unsigned>& topfreeSideTiles = topo.freeSideTiles_[0];
//            for (PODVector<unsigned>::ConstIterator it = topfreeSideTiles.Begin(); it != topfreeSideTiles.End(); ++it)
//                DrawDebugTile(viewinfos_[0].currentMap_, *it, debug, false, Color::CYAN);

            /// For Test : Draw Topography FloorY points
            Vector3 point;
            for (unsigned xi=0; xi < info_->mapWidth_; xi++)
            {
                point.x_ = info_->mTileWidth_ * (0.5f+(float)xi);
                point.y_ = topo.GetFloorY(point.x_);
                point.x_ += topo.maporigin_.x_;
                point.y_ += topo.maporigin_.y_;
                debug->AddCross(point, 0.5f, Color::GREEN, false);
            }

            /// For Test : Draw Topography Back Spline Curve
#ifdef USE_TOPOGRAPHY_BACKCURVEFLOOR
            topo.backFloorCurve_.DrawDebugGeometry(debug, topo.maporigin_, Color::YELLOW, false);
#endif
        }
#endif

#if defined(ACTIVE_WORLDELLIPSES) && defined(DRAWDEBUG_WORLDELLIPSES)
        /// For Test : AnlWorld Ellipse
        if (info_ && info_->worldModel_)
        {
            // draw world ellipses
            info_->worldGroundRef_.DrawDebugGeometry(debug, Color::WHITE, false);

            info_->worldAtmosphere_.DrawDebugGeometry(debug, Color::BLUE, false);
            info_->worldHillTop_.DrawDebugGeometry(debug, Color::GREEN, false);
            info_->worldGround_.DrawDebugGeometry(debug, Color::BLACK, false);
            info_->worldCenter_.DrawDebugGeometry(debug, Color::RED, false);

//            // for currentMap_ : draw world ellipse Tiles for the currentMap
//            if (viewinfos_[0].currentMap_ && viewinfos_[0].currentMap_->GetNode())
//            {
//                const MapTopography& topo = viewinfos_[0].currentMap_->GetTopography();
//
//                if (topo.IsWorldEllipseVisible())
//                {
//                    for (unsigned xi=0; xi < info_->mapWidth_; xi++)
//                    {
//                        int yi = topo.GetEllipseTileY(xi);
//                        if (viewinfos_[0].currentMap_->IsInYBound(yi))
//                            DrawDebugTile(viewinfos_[0].currentMap_, info_->GetTileIndex(xi, yi), debug, false, Color::RED);
//                    }
//                }
//            }
        }
#endif

#ifdef DRAWDEBUG_SCROLLERS
        PODVector<DrawableScroller*> drawables;
        GetScene()->GetComponents<DrawableScroller>(drawables, true);
        for (unsigned i = 0; i < drawables.Size(); ++i)
            drawables[i]->DrawDebugGeometry(debug, false);
#endif

#ifdef DRAWDEBUG_BIOMEMAP
        for (Vector<Map*>::ConstIterator it=viewinfos_[0].visibleAreaMaps_.Begin(); it!=viewinfos_[0].visibleAreaMaps_.End(); ++it)
        {
            if ((*it)->IsVisible())
                DrawDebugBiomeTiles(*it, debug, false);
        }
#endif

//        if (info_ && info_->worldModel_)
//        {
//            Vector2 mapbound1, mapbound2;
//            mapbound1.x_ = GetCurrentMap()->GetBounds().min_.x_;
//            mapbound2.x_ = GetCurrentMap()->GetBounds().max_.x_;
//            mapbound1.y_ = info_->worldGround_.GetYFromShape(mapbound1.x_);
//            mapbound2.y_ = info_->worldGround_.GetYFromShape(mapbound2.x_);
//            debug->AddLine(mapbound1, mapbound2, Color::RED.ToUInt(), false);
//        }

//		  PODVector<GOC_BodyExploder2D*> drawables;
//		  GetScene()->GetComponents<GOC_BodyExploder2D>(drawables, true);
//		  for (unsigned i = 0; i < drawables.Size(); ++i)
//			  drawables[i]->DrawDebugGeometry(debug, false);

#ifdef DRAWDEBUG_EFFECTZONE
        for (Vector<Map*>::ConstIterator it=viewinfos_[0].visibleAreaMaps_.Begin(); it!=viewinfos_[0].visibleAreaMaps_.End(); ++it)
        {
            Map* map = *it;
            if (map->IsVisible())
            {
                Rect rect;
                IntRect irect;

                const PODVector<ZoneData>& zones = map->GetMapData()->zones_;
                for (unsigned i=0; i < zones.Size(); i++)
                {
                    const ZoneData& zone = zones[i];
                    int w = zone.GetWidth();
                    int h = zone.GetHeight();
                    irect.left_ = zone.centerx_ - w/2;
                    irect.top_ = zone.centery_ - h/2;
                    irect.right_ = irect.left_ + w;
                    irect.bottom_ = irect.top_ + h;
                    info_->Convert2WorldRect(irect, map->GetRootNode()->GetWorldPosition2D(), rect);
                    GameHelpers::DrawDebugRect(rect, debug, false, Color::MAGENTA);
                }
            }
        }
#endif

    }

    DrawDebugTaggedInfos(debug, activeTagText);
}

extern const char* mapStatusNames[];

void World2D::DrawDebugTaggedInfos(DebugRenderer* debug, bool activeTagText)
{
    bool enable = debug && activeTagText;

    const HashMap<ShortIntVector2, Map* >& mapsInMemory = mapStorage_->GetMapsInMemory();
    for (HashMap<ShortIntVector2, Map* >::ConstIterator it=mapsInMemory.Begin(); it!=mapsInMemory.End(); ++it)
    {
        Map* map = it->second_;
        if (!map)
            continue;

        int mapstatus = map->GetStatus();
//        if (mapstatus <= Setting_Map || mapstatus > Available)
//            continue;

        Node* mapTagNode = map->GetTagNode();
        if (!mapTagNode)
        {
            map->SetTagNode(node_);
            mapTagNode = map->GetTagNode();
        }

        if (!enable)
        {
            mapTagNode->SetEnabledRecursive(false);
            continue;
        }

        const ShortIntVector2& mPoint = map->GetMapPoint();

#ifdef DRAWDEBUG_MAPSTATUS
        {
            String tag = "Map_" + mPoint.ToString();
            Text3D* text3D;
            Vector2 pos;
            String text;
            text.AppendWithFormat("%s (%s)\n", tag.CString(), map->IsEffectiveVisible() ? "Visible":"NoVisible");
            text.AppendWithFormat("%s (Entities=%u)\n", mapStatusNames[map->GetStatus()], World2D::GetEntities(mPoint).Size());
            if (viewinfos_[0].isUnderground_)
                text.AppendWithFormat("IsUnderground\n");

            Node* tagNode = mapTagNode->GetChild(tag);
            if (tagNode)
            {
                text3D = tagNode->GetComponent<Text3D>(LOCAL);
                text3D->SetText(text);
            }
            else
            {
                info_->Convert2WorldPosition(mPoint, IntVector2::ONE, pos);
                Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");
                tagNode = mapTagNode->CreateChild(tag, LOCAL);
                text3D = tagNode->CreateComponent<Text3D>(LOCAL);
                text3D->SetText(text);
                text3D->SetColor(Color::WHITE);
                text3D->SetFont(font, 150);
                text3D->SetOccludee(false);
                text3D->SetAlignment(HA_LEFT, VA_TOP);
                text3D->SetOnTop(true);

                tagNode->SetWorldPosition(Vector3(pos.x_, pos.y_, 0.f));
            }

            tagNode->SetEnabled(true);
        }
#endif

        if (!map->IsEffectiveVisible())
            continue;

#ifdef DRAWDEBUG_MAPSPOTS
        {
            // DrawDebug Spots
            Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>("Fonts/Anonymous Pro.ttf");
            const PODVector<MapSpot>& spots = map->GetSpots();
            int spotSize = spots.Size();
//            URHO3D_LOGINFOF("DrawDebug %u Spots for Map=%s!", spotSize, map->GetMapPoint().ToString().CString());
            Vector2 pos;
            Color color;
            for (int i=0; i < spotSize; i++)
            {
                const MapSpot& spot = spots[i];

                switch (spot.type_)
                {
                case SPOT_START :
                    color = Color::MAGENTA;
                    break;
                case SPOT_EXIT :
                    color = Color::YELLOW;
                    break;
                case SPOT_ROOM :
                    color = Color::BLUE;
                    break;
                case SPOT_LIQUID :
                    color = Color::CYAN;
                    break;
                default :
                    color = Color::GRAY;
                }

                info_->Convert2WorldPosition(mPoint, spot.position_, pos);

                // Add Cross Mark
                if (debug)
                {
                    debug->AddLine(Vector3(pos.x_ - 0.1f, pos.y_, 0.f), Vector3(pos.x_ + 0.1f, pos.y_, 0.f), color, false);
                    debug->AddLine(Vector3(pos.x_, pos.y_ - 0.1f, 0.f), Vector3(pos.x_, pos.y_ + 0.1f, 0.f), color, false);
                }

                // Add Spot Name (Create if not exist)
                String tag = spot.GetName(i);
                Node* tagNode = mapTagNode->GetChild(tag);
                if (!tagNode)
                {
//                    URHO3D_LOGINFOF("CreateTag : spot=%s mpoint=%s x=%d y=%d", tag.CString(), map->GetMapPoint().ToString().CString(), spot.x_, spot.y_);
                    tagNode = mapTagNode->CreateChild(tag, LOCAL);

                    Text3D* text3D = tagNode->CreateComponent<Text3D>(LOCAL);
                    text3D->SetText(tag);
                    text3D->SetColor(color);
                    text3D->SetFont(font, 12);
                    text3D->SetOccludee(false);
                    text3D->SetAlignment(HA_CENTER, VA_CENTER);

                    tagNode->SetWorldPosition(Vector3(pos.x_, pos.y_, 0.f));
                }

                tagNode->SetEnabled(enable);
            }
        }
#endif
    }
}

void World2D::DrawDebugTile(Map* map, unsigned tileindex, DebugRenderer* debugRenderer, bool depthTest, const Color& color)
{
    Vector2 halftile(info_->mTileWidth_/2, info_->mTileHeight_/2);
    Vector3 center(map->GetWorldTilePosition(map->GetTileCoords(tileindex)));
    debugRenderer->AddCross(center, 0.5f, color, false);

    Color colora(color);
    colora.a_ = 0.5f;
    debugRenderer->AddPolygon(Vector3(center.x_-halftile.x_, center.y_-halftile.y_, 0.f), Vector3(center.x_-halftile.x_, center.y_+halftile.y_, 0.f),
                              Vector3(center.x_+halftile.x_, center.y_+halftile.y_, 0.f), Vector3(center.x_+halftile.x_, center.y_-halftile.y_, 0.f), colora, depthTest);
//    debugRenderer->AddQuadXY(center, info_->mTileWidth_, info_->mTileHeight_, color, false);
}

void World2D::DrawDebugBiomeTiles(Map* map, DebugRenderer* debugRenderer, bool depthTest)
{
    if (!map || map->GetStatus() < Available)
        return;

    unsigned mapsize = info_->mapWidth_*info_->mapHeight_;

    Color color;
    color.r_ = color.b_ = 0.f;
    color.g_ = 1.f;
    color.a_ = 0.5f;

    int value;
    Rect rect;
    for (unsigned addr=0; addr < mapsize; addr++)
    {
        value = map->GetBiomeMap()[addr];

        if (!value)
            continue;

        rect = map->GetTileRect(addr);
        rect = rect.Adjusted(-rect.Size().x_ * 0.25f * (float)(11 - value) / 5.f);

        debugRenderer->AddPolygon(Vector3(rect.min_.x_, rect.min_.y_, 0.f), Vector3(rect.min_.x_, rect.max_.y_, 0.f),
                                  Vector3(rect.max_.x_, rect.max_.y_, 0.f), Vector3(rect.max_.x_, rect.min_.y_, 0.f), color, depthTest);
    }
}


/// Dump

void World2D::DumpNodeList(const List<unsigned>& list, String title) const
{
    URHO3D_LOGINFOF("%s", title.CString());
    unsigned i=0;
    if (!list.Size())
    {
        URHO3D_LOGINFOF("  No Entity");
        return;
    }

    for (List<unsigned>::ConstIterator it=list.Begin(); it!=list.End(); ++it,++i)
    {
        Node* node = GetScene()->GetNode(*it);
        if (node)
            URHO3D_LOGINFOF("  node[%u] name=%s id=%u enable=%s ptr=%u", i, node->GetName().CString(), node->GetID(), node->IsEnabled() ? "true" : "false", node);
    }
}

void World2D::DumpEntitiesInMemory() const
{
    const HashMap<ShortIntVector2, Map* >& maps = mapStorage_->GetMapsInMemory();

    URHO3D_LOGINFOF("World2D() - DumpEntitiesInMemory : NumMapsInMemory=%u", maps.Size());

    for (HashMap<ShortIntVector2, Map* >::ConstIterator it=maps.Begin(); it!=maps.End(); ++it)
    {
        DumpNodeList(GetEntities(it->first_), String("Map " + it->first_.ToString()) + " " + (it->second_->IsEffectiveVisible() ? "shown":"hidden"));
        it->second_->GetMapData()->Dump();
    }
}

void World2D::DumpMapsInMemory() const
{
    const HashMap<ShortIntVector2, Map* >& maps = mapStorage_->GetMapsInMemory();

    URHO3D_LOGINFOF("World2D() - DumpMapInMemory : NumMapsInMemory=%u", maps.Size());

    for (HashMap<ShortIntVector2, Map* >::ConstIterator it=maps.Begin(); it!=maps.End(); ++it)
    {
        URHO3D_LOGINFOF("%s", String("map " + it->first_.ToString()).CString());
    }

    MapCreator::Get()->Dump();
}

void World2D::DumpMapVisibilityProgress() const
{
    URHO3D_LOGINFO("World2D() - DumpMapVisibilityProgress ...");

    int numviewports = ViewManager::Get()->GetNumViewports();

    for (int i=0; i < numviewports; i++)
    {
        const WorldViewInfo& viewInfo = viewinfos_[i];

        URHO3D_LOGINFOF(" Viewport=%d => currentmap=%s ortho=%f ratio=%f focus=%s visiblearea=%s visiblerect=%s ...",
                        i, viewInfo.mPoint_.ToString().CString(), viewInfo.camOrtho_, viewInfo.camRatio_, viewInfo.cameraFocusEnabled_ ? "true":"false",
                        viewInfo.visibleMapArea_.ToString().CString(), viewInfo.extVisibleRect_.ToString().CString());

        for (Vector<ShortIntVector2>::ConstIterator it=viewInfo.mapsToShow_.Begin(); it!=viewInfo.mapsToShow_.End(); ++it)
        {
            URHO3D_LOGINFOF("  => MapToShow : %s", it->ToString().CString());
        }

        for (Vector<ShortIntVector2>::ConstIterator it=viewInfo.mapsToHide_.Begin(); it!=viewInfo.mapsToHide_.End(); ++it)
        {
            URHO3D_LOGINFOF("  => MapToHide : %s", it->ToString().CString());
        }
    }

    for (Vector<ShortIntVector2>::ConstIterator it=keepedVisibleMaps_.Begin(); it!=keepedVisibleMaps_.End(); ++it)
    {
        URHO3D_LOGINFOF("  => MapToKeepVisible : %s", it->ToString().CString());
    }

    URHO3D_LOGINFO("World2D() - DumpMapVisibilityProgress ... OK !");
}


