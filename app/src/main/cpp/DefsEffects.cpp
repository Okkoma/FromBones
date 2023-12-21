#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Scene/SceneEvents.h>

#include "GameEvents.h"
#include "GameAttributes.h"
#include "GameHelpers.h"

#include "GOC_Animator2D.h"
#include "GOC_Life.h"
#include "GOC_Destroyer.h"
#include "GOC_ZoneEffect.h"
#include "GOC_ControllerAI.h"

#include "Map.h"
#include "ViewManager.h"
#include "MapWorld.h"

#include "DefsEffects.h"


/// EffectType

const EffectType EffectType::EMPTY;
Vector<EffectType> EffectType::effecTypes_;

void EffectType::Register()
{
    if (EffectType::GetID(name) == -1)
    {
        effecTypes_.Push(*this);
        effecTypes_.Back().id = effecTypes_.Size()-1;
    }
}

void EffectType::InitTable()
{
    effecTypes_.Clear();
}

void EffectType::Set(const String& n, const StringHash& elt, float qty, unsigned numticks, float delay, bool persistent, float factor, const String& res)
{
    name = n;
    effectElt = elt;
    effectRessource = res;
    numTicks = numticks;
    qtyByTicks = qty;
    delayBetweenTicks = delay;
    persistentOutZone = persistent;
    qtyFactorOutZone = factor;
}

const EffectType& EffectType::Get(unsigned t)
{
    if (t >= effecTypes_.Size())
        return EMPTY;

    return effecTypes_[t];
}

EffectType* EffectType::GetPtr(unsigned t)
{
    if (t >= effecTypes_.Size())
        return 0;

    return &effecTypes_[t];
}

int EffectType::GetID(const String& name)
{
    for (Vector<EffectType>::Iterator it=effecTypes_.Begin(); it!=effecTypes_.End(); ++it)
    {
        if (it->name == name)
            return it->id;
    }

    return -1;
}

void EffectType::Dump() const
{
    URHO3D_LOGINFOF("        effect = %s (id:%d - effectElt:%s(%u) - res:%s ticks:%d qtybytick:%f outzonefactor:%f)",
                    name.CString(), id, GOA::GetAttributeName(effectElt).CString(), effectElt.Value(), effectRessource.CString(),
                    numTicks, qtyByTicks, qtyFactorOutZone);
}

void EffectType::DumpAll()
{
    URHO3D_LOGINFOF("EffectType() - DumpAll : types Size =%u", effecTypes_.Size());
    unsigned index = 0;
    for (Vector<EffectType>::Iterator it=effecTypes_.Begin(); it!=effecTypes_.End(); ++it)
    {
        it->Dump();
        ++index;
    }
}


/// EffectInstance

const EffectInstance EffectInstance::EMPTY;

EffectInstance::EffectInstance() :
    effect_(0),
    firstCount(true),
    tickCount(0),
    chrono(0.f),
    activeForHolder(false),
    outZone(false) { }

EffectInstance::EffectInstance(GOC_ZoneEffect* zone, Node* node, EffectType* effect, const Vector2& point) :
    node_(node),
    effect_(effect),
    zone_(zone),
    firstCount(true),
    tickCount(0),
    chrono(0.f),
    activeForHolder(zone ? zone->GetHolder() == node->GetID() && zone->GetApplyToHolder() : false),
    outZone(false),
    localImpact_(point) { }

EffectInstance::EffectInstance(const EffectInstance& e) :
    node_(e.node_),
    effect_(e.effect_),
    zone_(e.zone_),
    firstCount(e.firstCount),
    tickCount(e.tickCount),
    chrono(e.chrono),
    activeForHolder(e.activeForHolder),
    outZone(e.outZone),
    localImpact_(e.localImpact_) { }


/// EffectAction

HashMap<IntVector3, SharedPtr<EffectAction> > EffectAction::runningEffectActions_;

void EffectAction::RegisterEffectActionLibrary(Context* context)
{
    BossZone::RegisterObject(context);
}

void EffectAction::Clear()
{
    runningEffectActions_.Clear();
}

void EffectAction::Clear(const IntVector3& zone)
{
    runningEffectActions_.Erase(zone);
}

void EffectAction::Update(int viewport, MapBase* map, const WorldMapPosition& position, IntVector3& zone)
{
    IntVector3 newzone(map->GetMapPoint().x_, map->GetMapPoint().y_, -1);

    if (map->IsEffectiveVisible())
    {
        const PODVector<ZoneData>& zones = map->GetMapData()->zones_;
        if (zones.Size())
        {
            for (unsigned i=0; i < zones.Size(); i++)
            {
                if (zones[i].IsInside(position.mPosition_, position.viewZIndex_))
                {
                    newzone.z_ = i;
                    break;
                }
            }
        }
    }

    if (newzone != zone)
    {
        URHO3D_LOGINFOF("EffectAction() - Update : inside newzone=%s lastzone=%s...", newzone.ToString().CString(), zone.ToString().CString());

        // Apply Effects when Go outside the zone
        if (zone.z_ != -1)
        {
            MapBase* oldmap = zone.x_ == map->GetMapPoint().x_ && zone.y_ == map->GetMapPoint().y_ ? map : World2D::GetWorld()->GetMapAt(ShortIntVector2(zone.x_, zone.y_), false);

            if (oldmap && oldmap->IsEffectiveVisible() && zone.z_ < oldmap->GetMapData()->zones_.Size())
            {
                ZoneData& zonedata = oldmap->GetMapData()->zones_[zone.z_];

                int numPlayersInside = zonedata.GetNumPlayersInside();

                if (zonedata.goOutAction_ && numPlayersInside == 0)
                    EffectAction::ApplyActionOn(zonedata.goOutAction_, map, zone.z_, viewport);

                zonedata.lastVisit_ = Time::GetSystemTime();

                // Always reset the camera focus for the viewport.
                ViewManager::Get()->SetFocusEnable(true, viewport);
            }
        }

        // Apply Effects when Go inside the zone
        if (newzone.z_ != -1)
        {
            ZoneData& zonedata = map->GetMapData()->zones_[newzone.z_];

            int numPlayersInside = zonedata.GetNumPlayersInside();

            URHO3D_LOGINFOF("EffectAction() - Update : inside newzone=%s apply EffectAction=%u ... numPlayersInside=%d", newzone.ToString().CString(), zonedata.goInAction_, numPlayersInside);

            // Add Effects for newzone once
            if (zonedata.goInAction_ && numPlayersInside == 1)
                EffectAction::ApplyActionOn(zonedata.goInAction_, map, newzone.z_, viewport);

            zonedata.lastVisit_ = Time::GetSystemTime();
        }

        zone = newzone;
    }
}

void EffectAction::ApplyActionOn(unsigned actiontype, MapBase* map, unsigned zoneid, int viewport)
{
    IntVector3 zone(map->GetMapPoint().x_, map->GetMapPoint().y_, zoneid);
    HashMap<IntVector3, SharedPtr<EffectAction> >::Iterator it = runningEffectActions_.Find(zone);

    EffectAction* effectAction = 0;

    if (it == runningEffectActions_.End())
    {
        SharedPtr<Object> object(GameContext::Get().context_->CreateObject(StringHash(actiontype)));
        if (object)
        {
            effectAction = static_cast<EffectAction*>(object.Get());
            effectAction->Set(viewport, map, zoneid);
            runningEffectActions_[zone] = SharedPtr<EffectAction>(effectAction);
        }
        else
        {
            URHO3D_LOGERRORF("EffectAction - ApplyActionOn() : no object for actiontype=%u !", actiontype);
        }
    }
    else
    {
        effectAction = it->second_.Get();
    }

    if (effectAction)
        effectAction->Apply();
}

void EffectAction::PurgeFinishedActions()
{
    if (!runningEffectActions_.Size())
        return;

    HashMap<IntVector3, SharedPtr<EffectAction> >::Iterator it = runningEffectActions_.Begin();
    while (it != runningEffectActions_.End())
    {
        if (it->second_.Get()->finished_)
            it = runningEffectActions_.Erase(it);
        else
            it++;
    }
}

void EffectAction::Set(int viewport, MapBase* map, unsigned zone)
{
    viewport_ = viewport;
    map_ = map;
    zone_.x_ = map->GetMapPoint().x_;
    zone_.y_ = map->GetMapPoint().y_;
    zone_.z_ = zone;
}

bool EffectAction::IsZoneValid()
{
    if (!map_->IsAvailable()  || zone_.x_ != map_->GetMapPoint().x_ ||
            zone_.y_ != map_->GetMapPoint().y_ || zone_.z_ >= map_->GetMapData()->zones_.Size())
    {
        zonedata_ = 0;
    }
    else
    {
        zonedata_ = &map_->GetMapData()->zones_[zone_.z_];
    }

    return zonedata_ != 0;
}

bool EffectAction::Apply()
{
    if (!IsZoneValid())
    {
        MarkFinished();
        return false;
    }

    finished_ = false;
    return true;
}

void EffectAction::MarkFinished()
{
    finished_ = true;

    if (zonedata_)
        OnFinished();
}

void EffectAction::OnFinished()
{
    URHO3D_LOGINFOF("EffectAction - OnFinished() !");
    UnsubscribeFromAllEvents();
    zonedata_->state_ = 0;
    zonedata_->nodeid_ = 0;
}

void BossZone::RegisterObject(Context* context)
{
    context->RegisterFactory<BossZone>();
}

const unsigned BossZoneActivationDelay_ = 60000 * 5; // 5min
const IntVector2 LIFEBARBOSSPOSITION = IntVector2(0, 200);

enum BossZoneState
{
    BOSSZONE_START = 0,
    BOSSZONE_BOSSAPPEARS,
    BOSSZONE_FIGHT,
    BOSSZONE_BOSSDEFEATED,
    BOSSZONE_ADDCHEST,
    BOSSZONE_END
};

void BossZone::OnFinished()
{
    URHO3D_LOGINFOF("BossZone - OnFinished() !");

    UnsubscribeFromAllEvents();

    if (boss_)
    {
        // Remove the boss life bar
        GOC_Life* goclife = boss_->GetComponent<GOC_Life>();
        if (goclife)
            goclife->SetLifeBar(false);
    }

//    if (zonedata_->state_ == BOSSZONE_FIGHT && boss_)
//    {
//        URHO3D_LOGINFOF("BossZone - HandleUpdate() : Keep boss dead event activated ...");
//        SubscribeToEvent(boss_, GOC_LIFEDEAD, URHO3D_HANDLER(BossZone, HandleBossDead));
//    }

    if (zonedata_->state_ == BOSSZONE_FIGHT && boss_)
    {
        zonedata_->nodeid_ = boss_->GetID();
    }
    else
    {
        zonedata_->state_ = BOSSZONE_END;
        zonedata_->nodeid_ = 0;
    }
    zonedata_->lastEnabled_ = Time::GetSystemTime();

    ViewManager::Get()->SetFocusEnable(true, viewport_);
}

bool BossZone::Apply()
{
    if (!EffectAction::Apply())
    {
        return false;
    }

    URHO3D_LOGINFOF("BossZone - Apply() : state=%d ...", zonedata_->state_);

    if (zonedata_->state_ >= BOSSZONE_END)
    {
        // Check for Reactivation
        unsigned time = Time::GetSystemTime();

        float delay = (BossZoneActivationDelay_ - (time - zonedata_->lastEnabled_)) / 60000.f;
        int minutes = delay;
        int secondes = (delay - minutes) * 60;
        URHO3D_LOGINFOF("BossZone - Apply() : Check Reactivation ... in %d min %d sec", minutes, secondes);

        if (time > zonedata_->lastEnabled_ + BossZoneActivationDelay_)
        {
            URHO3D_LOGINFOF("BossZone - Apply() : Reactivation ... OK !");

            zonedata_->lastEnabled_ = time;
            zonedata_->nodeid_ = 0;
            zonedata_->state_ = 0;

            boss_.Reset();
        }
    }

    if (zonedata_->state_ < BOSSZONE_END)
    {
        // Initiate from save data
        {
            // Patch state if <0
            zonedata_->state_ = Max(0, zonedata_->state_);

            if (zonedata_->state_ == BOSSZONE_FIGHT)
            {
                if (!boss_)
                {
                    if (zonedata_->nodeid_)
                        boss_ = GameContext::Get().rootScene_->GetNode(zonedata_->nodeid_);

                    if (!boss_)
                    {
                        zonedata_->nodeid_ = 0;

                        SpawnBoss();
                    }
                    else if (!boss_->IsEnabled())
                    {
                        if (!boss_->isPoolNode_)
                        {
                            URHO3D_LOGINFOF("BossZone - Apply() : Boss recovery ... nodeid=%u not a pool node ... reset !", zonedata_->nodeid_);
                            zonedata_->nodeid_ = 0;
                        }
                        else
                        {
                            URHO3D_LOGINFOF("BossZone - Apply() : Boss recovery ... nodeid=%u get the node in the pool !", zonedata_->nodeid_);
                        }

                        SpawnBoss();
                    }

                    // No Boss => End
                    if (!boss_)
                    {
                        MarkFinished();

                        URHO3D_LOGERRORF("BossZone - Apply() : Boss recovery ... NOK !");

                        return false;
                    }

                    URHO3D_LOGINFOF("BossZone - Apply() : Boss recovery ... nodeid=%u ... %s !", zonedata_->nodeid_, boss_ ? "OK":"NOK");

                    SetBossNode();
                    zonedata_->nodeid_ = boss_->GetID();

                    time_ = 0.f;
                }
            }
        }

        // Focus on Zone with move camera and zoom
        {
            ViewManager::Get()->SetFocusEnable(false, viewport_);

            Vector2 focus;
            World2D::GetWorld()->GetWorldInfo()->Convert2WorldPosition(map_->GetMapPoint(), IntVector2(zonedata_->centerx_, zonedata_->centery_), focus);
            GameHelpers::MoveCameraTo(focus, viewport_, 2.f);
            GameHelpers::ZoomCameraTo(GameHelpers::GetZoomToFitTo(zonedata_->GetWidth(), zonedata_->GetHeight(), viewport_), viewport_, 2.f);
        }

        SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(BossZone, HandleUpdate));
    }

    return true;
}

void BossZone::SpawnBoss()
{
    int viewZ = ViewManager::Get()->GetViewZ(zonedata_->zindex_);
    int viewid = map_->GetViewId(viewZ);

    // Get the floor coord
    IntVector2 floorcoords(zonedata_->centerx_, zonedata_->centery_);
    map_->GetBlockPositionAt(DownDir, viewid, floorcoords, zonedata_->centery_ + zonedata_->GetHeight()/2);
    if (floorcoords.y_ > zonedata_->centery_+3)
        floorcoords.y_ -= 4;

    URHO3D_LOGINFOF("BossZone - SpawnBoss() : zone=%d %d viewZ=%d viewId=%d(InnerView_ViewId=%d) ... try to spawn boss at %s ...",
                    zonedata_->centerx_, zonedata_->centery_, viewZ, viewid, InnerView_ViewId, floorcoords.ToString().CString());

    // Set the world position for the boss And Spawn
    Vector2 focus;
    World2D::GetWorld()->GetWorldInfo()->Convert2WorldPosition(map_->GetMapPoint(), floorcoords, focus);

    static const StringHash Bosses[4] =
    {
        StringHash("EliegorGolem"),
        StringHash("GOT_RedLord"),
        StringHash("GOT_Darkren"),
        StringHash("GOT_Mirubil")
    };

    boss_ = World2D::SpawnEntity(Bosses[zonedata_->entityIndex_ % 4], zonedata_->sstype_, zonedata_->nodeid_, 0, viewZ, PhysicEntityInfo(focus.x_, focus.y_), SceneEntityInfo(), 0, true);

    URHO3D_LOGINFOF("BossZone - SpawnBoss() : Boss=%s(%u) !", boss_ ? boss_->GetName().CString() : "none", boss_ ? boss_->GetID() : 0);
}

void BossZone::SetBossNode()
{
    if (!boss_)
        return;

    // Huge Boss
    Node* templateNode = GOT::GetObject(boss_->GetVar(GOA::GOT).GetStringHash());
    boss_->SetWorldScale2D(templateNode->GetWorldScale2D() * 1.5f);

    // Aggressive
    GOC_AIController* aicontroller = boss_->GetComponent<GOC_AIController>();
    aicontroller->SetDetectTarget(true, 8.f);
    aicontroller->SetBehaviorTargetAttr("FollowAttack");

    // Lots Of Energy
    GOC_Life* goclife = boss_->GetComponent<GOC_Life>();
    goclife->SetTemplate("BossTest");
    goclife->SetInvulnerability(3.f);

    // Add LifeBar
    boss_->SetVar(GOA::BOSSZONEID, zone_);
    goclife->SetLifeBar(true, false, LIFEBARBOSSPOSITION, HA_CENTER, VA_TOP);

    URHO3D_LOGINFOF("BossZone - SetBossNode() : Boss Scale=%F Life=%d Energy=%F !", boss_->GetWorldScale2D().x_, goclife->GetLife(), goclife->GetEnergy());
}

void BossZone::GetCloseBlocks(MapBase* map, ZoneData* zonedata, Vector<unsigned>& blocks)
{
    blocks.Clear();

    // Find the Empty Tiles On the Zone perimeter
    map->FindTileIndexesWithPixelShapeAt(false, zonedata->centerx_, zonedata->centery_, map->GetViewId(ViewManager::Get()->GetViewZ(zonedata->zindex_)), zonedata->shapeinfo_, blocks);
}

void BossZone::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    // check map for desactivation
    if (!IsZoneValid())
    {
        MarkFinished();
        return;
    }

    // No Player In the Zone
    if (zonedata_ && !zonedata_->GetNumPlayersInside())
    {
        MarkFinished();
        return;
    }

    time_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    if (time_ > 2.f && zonedata_->state_ == BOSSZONE_START)
    {
        GameHelpers::ShakeNode(ViewManager::Get()->GetCameraNode(viewport_), 10, 3.f, Vector2(0.f, 1.5f));

        // Close the zone
        Vector<unsigned> blocks;
        GetCloseBlocks(map_, zonedata_, blocks);
        if (blocks.Size())
            GameHelpers::SetTiles(map_, MapFeatureType::RoomWall, blocks, ViewManager::Get()->GetViewZ(zonedata_->zindex_), true);

        zonedata_->state_++;
    }

    if (zonedata_->state_ == BOSSZONE_BOSSAPPEARS)
    {
        if (time_ > 4.f)
        {
            if (!boss_)
            {
                URHO3D_LOGINFOF("BossZone - HandleUpdate() : Spawn Boss ...");

                zonedata_->nodeid_ = 0;
                SpawnBoss();

                if (boss_)
                {
                    SetBossNode();
                    zonedata_->nodeid_ = boss_->GetID();

                    // Add Particule Effects
                    Drawable2D* drawable = boss_->GetDerivedComponent<Drawable2D>();
                    Node* particuleNode = GameHelpers::SpawnParticleEffectInNode(GetContext(), boss_, ParticuleEffect_[PE_SUN], drawable->GetLayer()+1, drawable->GetViewMask(), boss_->GetWorldPosition2D(), 0.f, 3.f, true, 3.f, Color(0.2f, 0.0f, 0.0f, 1.0f), LOCAL);
                }
            }

            zonedata_->state_ = boss_ ? BOSSZONE_FIGHT : BOSSZONE_END;
        }
    }

    if (zonedata_->state_ == BOSSZONE_FIGHT)
    {
        return;
    }

    if (zonedata_->state_ == BOSSZONE_BOSSDEFEATED)
    {
        // Open the zone
        Vector<unsigned> blocks;
        GetCloseBlocks(map_, zonedata_, blocks);
        if (blocks.Size())
            GameHelpers::SetTiles(map_, MapFeatureType::NoMapFeature, blocks, ViewManager::Get()->GetViewZ(zonedata_->zindex_), true);

        zonedata_->state_ = BOSSZONE_ADDCHEST;
        time_ = 0.f;
    }

    if (zonedata_->state_ == BOSSZONE_ADDCHEST)
    {
        // Add Chest
        if (time_ > 2.f)
        {
            int viewZ = ViewManager::Get()->GetViewZ(zonedata_->zindex_);
            int viewid = map_->GetViewId(viewZ);

            // Get the floor coord
            IntVector2 floorcoords(zonedata_->centerx_, zonedata_->centery_);
            map_->GetBlockPositionAt(DownDir, viewid, floorcoords, zonedata_->centery_ + zonedata_->GetHeight()/2);
            floorcoords.y_--;

            // Set the world position for the chest And Spawn
            Vector2 focus;
            World2D::GetWorld()->GetWorldInfo()->Convert2WorldPosition(map_->GetMapPoint(), floorcoords, focus);
            Node* chest = World2D::SpawnEntity(StringHash("GOT_TreasureChestBoss"), 0, 0, 0, viewZ, PhysicEntityInfo(focus.x_, focus.y_), SceneEntityInfo(), 0, true);

            // Add Particule Effects
            Drawable2D* drawable = chest->GetDerivedComponent<Drawable2D>();
            GameHelpers::SpawnParticleEffect(GetContext(), ParticuleEffect_[PE_SUN], drawable->GetLayer()+1, drawable->GetViewMask(), chest->GetWorldPosition2D(), 0.f, 3.f, true, 3.f, Color(0.9f, 0.9f, 0.5f, 0.5f), LOCAL);

            // Move Camera
            GameHelpers::MoveCameraTo(focus, viewport_, 1.f);
            GameHelpers::ZoomCameraTo(GameContext::Get().CameraZoomDefault_, viewport_, 1.f);

            zonedata_->state_++;
            time_ = 0.f;
        }
    }

    if (zonedata_->state_ == BOSSZONE_END)
    {
        if (time_ > 3.f)
        {
            MarkFinished();

            URHO3D_LOGINFOF("BossZone - HandleUpdate() : Finished ... time=%u", zonedata_->lastEnabled_);
        }
    }
}

void BossZone::RemoveBoss(const IntVector3& zone, Node* boss)
{
    URHO3D_LOGINFOF("BossZone - RemoveBoss() : ...");

    GOC_Animator2D* animator = boss->GetComponent<GOC_Animator2D>();
    if (animator)
        animator->MoveLayer(-2);

    GOC_Life* goclife = boss->GetComponent<GOC_Life>();
    if (goclife)
        goclife->SetLifeBar(false);

    // Find zonedata
    ZoneData* zonedata = 0;
    MapBase* map = World2D::GetWorld()->GetMapAt(ShortIntVector2(zone.x_, zone.y_), false);
    if (map && zone.z_ < map->GetMapData()->zones_.Size())
        zonedata = &map->GetMapData()->zones_[zone.z_];

    if (zonedata)
    {
        // Set the zone state to
        if (zonedata->state_ == BOSSZONE_FIGHT)
        {
            // Check if the boss is in the zone : if not => no chest
            const WorldMapPosition& bossposition = boss->GetComponent<GOC_Destroyer>()->GetWorldMapPosition();
            zonedata->state_ = zonedata->IsInside(bossposition.mPosition_, bossposition.viewZIndex_) ? BOSSZONE_BOSSDEFEATED : BOSSZONE_END;

            URHO3D_LOGINFOF("BossZone - RemoveBoss() : ... Boss Defeated %s the Zone !", zonedata->state_ == BOSSZONE_BOSSDEFEATED ? "Inside" : "Outside");
        }
        // if the zone is ended, always finish properly the zone : open it and reset data
        if (zonedata->state_ == BOSSZONE_END)
        {
            URHO3D_LOGINFOF("BossZone - RemoveBoss() : ... BOSSZONE_END !");
            zonedata->nodeid_ = 0;
            zonedata->lastEnabled_ = Time::GetSystemTime();

            Vector<unsigned> blocks;
            GetCloseBlocks(map, zonedata, blocks);
            if (blocks.Size())
                GameHelpers::SetTiles(map, MapFeatureType::NoMapFeature, blocks, ViewManager::Get()->GetViewZ(zonedata->zindex_), true);
        }
    }

    boss->RemoveVar(GOA::BOSSZONEID);
}

