#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/DebugRenderer.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/SpriterData2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>

#include "DefsColliders.h"

#include "MapWorld.h"
#include "Map.h"
#include "ViewManager.h"
#include "GameContext.h"

#include "GameOptions.h"
#include "GameOptionsTest.h"

#include "GEF_Rain.h"


#define DROPLET_ANIM_DROP 0
#define DROPLET_ANIM_HIT  1

#define DROPLET_ACTIVE_RAYCAST
#define DROPLET_ACTIVE_FLUID


/// Droplet


PhysicsWorld2D* Droplet::physicsWorld_;
PhysicsRaycastResult2D Droplet::rayresult_;
float Droplet::intensity_[MAX_VIEWPORTS];
Map* Droplet::spawningMap_[MAX_VIEWPORTS];
Spriter::Animation* Droplet::animationHit_;

//Vector2 Droplet::position_;
const unsigned Droplet::NUM_SCREENPARTS = 5;
const unsigned Droplet::NUM_DROPLET_ROW = 10;
const unsigned Droplet::NUM_DROPLET_COL = 25;
const unsigned Droplet::NUM_DROPLETS = NUM_DROPLET_ROW*NUM_DROPLET_COL;
const float Droplet::DROPLET_OFFSETMINY = 1.f;
const float Droplet::DROPLET_ANGLE = 2.f;
const float Droplet::DROPLET_MINTIME = 1.f;
const float Droplet::DROPLET_MAXTIME = 2.f;
const float Droplet::DROPLET_DISPERSION = 0.30f;
Vector<unsigned> Droplet::hittedTiles_;

void Droplet::Set(Node* root, AnimationSet2D* animationSet, int viewport, unsigned x, unsigned y, unsigned layerIndex)
{
    node_ = root->CreateChild("Droplet", LOCAL);

    animatedSprite_ = node_->CreateComponent<AnimatedSprite2D>(LOCAL);
    viewport_ = viewport;
    coordx_ = x;
    coordy_ = y;
    layerIndex_ = layerIndex;

    animatedSprite_->SetAnimationSet(animationSet);
#ifdef ACTIVE_LAYERMATERIALS
    animatedSprite_->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERGROUNDS]);
#endif
    animatedSprite_->SetSpriterAnimation(DROPLET_ANIM_DROP, LM_FORCE_LOOPED);
    float color = Max((float)layerIndex/2.f, 0.4f);
    animatedSprite_->SetColor(Color(color, color, color+0.1f, 1.f));
    animatedSprite_->SetOrderInLayer(10000);
    node_->SetEnabled(false);
}

void Droplet::Reset()
{
    const float& intensity = intensity_[viewport_];

    maxtime_ = Abs(intensity) > 0.5f ? Random(DROPLET_MINTIME, DROPLET_MAXTIME) / Abs(intensity) : Random(DROPLET_MINTIME, DROPLET_MAXTIME);
//    maxtime_ = 3.f;
    time_ = Random(0.f, maxtime_*0.5f);

    if (World2D::IsUnderground(viewport_) && ViewManager::Get()->GetCurrentLayerZIndex(viewport_) == ViewManager::INNERLAYER_Index && layerIndex_ != ViewManager::INNERLAYER_Index)
    {
        node_->SetEnabled(false);
        return;
    }

    const Rect& visibleRect = World2D::GetExtendedVisibleRect(viewport_);

    float dx = Abs(visibleRect.max_.x_ - visibleRect.min_.x_) / (float)NUM_DROPLET_COL;
    float dy = Abs(visibleRect.max_.y_ - visibleRect.min_.y_) / (float)NUM_SCREENPARTS / (float)NUM_DROPLET_ROW;
    float r = Random(0.f, 1.f);

    float dispersion = DROPLET_DISPERSION * r * (intensity+r*0.5f);
    float angle = DROPLET_ANGLE * r * (intensity+r*0.5f);

    iPos_.x_ = visibleRect.min_.x_ + ((float) coordx_ + dispersion) * dx;
    iPos_.y_ = visibleRect.max_.y_ - ((float) coordy_) * dy;

    // 22/02/2023 : if floorY is above the start y of the droplet don't start it.
    // TODO : problems on map borders because we take only the topography of the current map
    // TODO : => we need a WorldTopography of the visibleMaps (inside World2D)
    Map* map = World2D::GetCurrentMap(viewport_);
    if (!map)
        return;

    const MapTopography& topo = map->GetTopography();
    float floory = topo.maporigin_.y_ + topo.GetFloorY(iPos_.x_ - topo.maporigin_.x_);
    if (iPos_.y_ < floory)
    {
        node_->SetEnabled(false);
        return;
    }
    
    bool innercave = World2D::IsUnderground(viewport_) && layerIndex_ == ViewManager::INNERLAYER_Index;
//    if (innercave)
//    {
//        float floory = topo.maporigin_.y_ + topo.GetFloorY(iPos_.x_ - topo.maporigin_.x_);
//        if (iPos_.y_ < floory)
//        {
//            iPos_.y_ = floory + 2.f;
//            time_ = 0.f;
//        }
//    }

    position_ = innercave ? iPos_ : iPos_.Lerp(impact_, Random(0.f, 0.5f));
    if (CheckInFluid(position_))
    {
        node_->SetEnabled(false);
        return;
    }

    node_->SetEnabled(true);

    fPos_.y_ = visibleRect.min_.y_ - DROPLET_OFFSETMINY;
    fPos_.x_ = iPos_.x_ + Tan(angle) * (iPos_.y_ - impact_.y_);
    impact_ = fPos_;

    animatedSprite_->SetSpriterAnimation(DROPLET_ANIM_DROP, LM_FORCE_LOOPED);

#ifdef DROPLET_ACTIVE_RAYCAST
    // Initial RayCast
    if (iPos_ != fPos_)
    {
        physicsWorld_->RaycastSingle(rayresult_, iPos_, fPos_, layerIndex_ == ViewManager::INNERLAYER_Index ? CM_INSIDERAIN | CC_INSIDEPROJECTILE : CM_OUTSIDERAIN | CC_OUTSIDEPROJECTILE);
        if (rayresult_.body_ && rayresult_.position_.y_ > fPos_.y_ && rayresult_.normal_.y_ >= 0.f)
        {         
            impact_ = rayresult_.position_;
        }
    }
#endif

    if (innercave || (!World2D::IsUnderground(viewport_) && layerIndex_ <= ViewManager::INNERLAYER_Index))
    {
        animatedSprite_->SetViewMask(VIEWPORTSCROLLER_INSIDE_MASK << viewport_);//ViewManager::effectMask_[INNERVIEW]);
        animatedSprite_->SetLayer2(IntVector2(layerIndex_ == ViewManager::INNERLAYER_Index ? INNERRAINLAYER : BACKACTORVIEW-1,-1));
    }
    else
    {
        animatedSprite_->SetViewMask(VIEWPORTSCROLLER_OUTSIDE_MASK << viewport_);//ViewManager::effectMask_[FRONTVIEW]);
        animatedSprite_->SetLayer2(IntVector2(ViewManager::Get()->GetLayerZ(layerIndex_)+LAYER_WEATHER,-1));
    }

    node_->SetWorldRotation2D(angle);
    node_->SetWorldPosition2D(position_);
}

bool Droplet::CheckInFluid(const Vector2& position)
{
    // 22/04/2025 : test si la droplet est dans du liquide
    Map* map = spawningMap_[viewport_];
    if (!map || !map->GetBounds().IsInside(position))
        map = World2D::GetCurrentMap(viewport_);
    FluidCell* fcell = map ? map->GetFluidCellPtr(map->GetTileIndexAt(position), ViewManager::FLUIDFRONTVIEW_Index) : nullptr;
    return fcell && (fcell->mass_ > 0.f || (fcell->type_ == BLOCK && fcell->Top && fcell->Top->mass_ > 0.f));
}

void Droplet::SetSpawningMap(int viewport)
{
    const Rect& visibleRect = World2D::GetExtendedVisibleRect(viewport);
    Map* map = World2D::GetMapAt(Vector2(visibleRect.min_.x_ + (visibleRect.max_.x_-visibleRect.min_.x_)*0.5f, visibleRect.max_.y_));
    spawningMap_[viewport] = map;
}

void Droplet::Update(float timeStep)
{
    time_ += timeStep;

    // Out of time => reset droplet
    if (time_ >= maxtime_)
    {
        Reset();
        return;
    }

    if (!node_->IsEnabled())
        return;

    position_ = iPos_.Lerp(fPos_, SmoothStep(0.f, maxtime_, time_));

    // Out of screen => reset droplet
    if (position_.y_ < fPos_.y_ || impact_.y_ > World2D::GetExtendedVisibleRect(viewport_).max_.y_)
    {
        Reset();
        return;
    }
#if defined(DROPLET_ACTIVE_FLUID)    
    if (GameContext::Get().gameConfig_.fluidEnabled_ && CheckInFluid(position_))
    {
        Reset();
        return;
    }
#endif

    // Touch Ground
    if (node_->GetWorldPosition2D().y_ - PIXEL_SIZE <= impact_.y_ && animatedSprite_->GetSpriterAnimation() == animationHit_)
    {
        // Check End of Hit Ground Animation
        if (animatedSprite_->HasFinishedAnimation())
            Reset();
        return;
    }
    
#ifdef DROPLET_ACTIVE_RAYCAST
    // if Impact, set Hit Ground Animation
    if (position_.y_ <= impact_.y_)
    {
        if (CheckInFluid(impact_))
        {
            Reset();
            return;
        }
        node_->SetWorldPosition2D(impact_);
        animatedSprite_->SetSpriterAnimation(DROPLET_ANIM_HIT, LM_DEFAULT);

#if defined(DROPLET_ACTIVE_FLUID)
        if (GameContext::Get().gameConfig_.fluidEnabled_)
        {
            Map* map = World2D::GetCurrentMap(viewport_);
            if (map && map->IsVisible() && map->GetBounds().IsInside(impact_/*+Vector2(0.f, 0.16f)*/))
            {
                unsigned tileindex = map->GetTileIndexAt(impact_/*+Vector2(0.f, 0.16f)*/);
                if (map->GetFeature(tileindex, OuterView_ViewId) <= MapFeatureType::Threshold)
                    hittedTiles_.Push(tileindex);
            }
        }
#endif
        return;
    }
#endif
    // Move droplet
    node_->SetWorldPosition2D(position_);
}

void Droplet::Clear()
{
    if (node_)
    {
        node_->Remove();
        node_ = 0;
    }
}

// milliseconds for rain update pass
const int DROPLET_TIMEUPDATE = 2;

/// GEF_Rain

GEF_Rain::GEF_Rain(Context* context) :
    Component(context),
    viewport_(0),
    intensity_(0.f),
    incIntensity_(1.f),
    started_(false),
    dropletsSetted_(false)
{
    raintimer_.SetExpirationTime(DROPLET_TIMEUPDATE);
    droplets_.Resize(Droplet::NUM_DROPLETS);
    for (PODVector<Droplet>::Iterator it=droplets_.Begin(); it!=droplets_.End(); ++it)
        it->node_ = nullptr;
}

GEF_Rain::~GEF_Rain()
{ }

void GEF_Rain::RegisterObject(Context* context)
{
    context->RegisterFactory<GEF_Rain>();
    URHO3D_ACCESSOR_ATTRIBUTE("AnimationSet", GetAnimationSet, SetAnimationSet, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Intensity", GetIntensity, SetIntensity, float, 0.f, AM_DEFAULT);
}

void GEF_Rain::Start()
{
    if (started_)
        return;

    URHO3D_LOGINFO("GEF_Rain() - Start !");

    if (!dropletsSetted_)
    {
        if (filename_.Empty())
            return;

        AnimationSet2D* animationSet = context_->GetSubsystem<ResourceCache>()->GetResource<AnimationSet2D>(filename_);
        if (!animationSet)
        {
            URHO3D_LOGERRORF("GEF_Rain() - Start : no animation Set %s !", filename_.CString());
            return;
        }

//        SetIntensity(0.f);

        Droplet::physicsWorld_ = GetScene()->GetComponent<PhysicsWorld2D>();
        Droplet::animationHit_ = animationSet->GetSpriterData()->entities_[0]->animations_[DROPLET_ANIM_HIT];
//        URHO3D_LOGINFO("GEF_Rain() - Start : Create Droplets ...");
        for (int i = 0; i < MAX_VIEWPORTS; ++i)
            Droplet::spawningMap_[i] = nullptr;

        unsigned i, j, k=0;
        unsigned layerindex;
        for (j=0; j < Droplet::NUM_DROPLET_ROW; ++j)
        {
            layerindex = j % ViewManager::MAX_PURELAYERS;

            for (i=0; i < Droplet::NUM_DROPLET_COL; ++i, ++k)
                droplets_[k].Set(node_, animationSet, viewport_, i, j, layerindex);
        }

//        URHO3D_LOGINFO("GEF_Rain() - Start : Create Droplets ... OK !");

        dropletsSetted_ = true;
    }

    started_ = true;
    enabled_ = true;
    paused_ = false;

    OnSetEnabled();
}

void GEF_Rain::Stop()
{
    if (!started_)
        return;

    URHO3D_LOGINFO("GEF_Rain() - Stop !");

    started_ = false;
    enabled_ = false;

    OnSetEnabled();
}

void GEF_Rain::OnSetEnabled()
{
    if (!dropletsSetted_)
        return;

    Scene* scene = GetScene();
    bool enabled = IsEnabledEffective();

    if (scene && enabled)
    {
//        URHO3D_LOGINFO("GEF_Rain() - OnSetEnabled = true");
        Droplet::SetSpawningMap(viewport_);
        for (PODVector<Droplet>::Iterator it=droplets_.Begin(); it!=droplets_.End(); ++it)
            it->Reset();

//        URHO3D_LOGINFO("GEF_Rain() - OnSetEnabled : Droplets Reseted !");

        iCount_ = 0;
        raintimer_.Reset();
        SubscribeToEvent(scene, E_SCENEUPDATE, URHO3D_HANDLER(GEF_Rain, HandleUpdate));
    }
    else
    {
//        URHO3D_LOGINFO("GEF_Rain() - OnSetEnabled = false");

        for (PODVector<Droplet>::Iterator it=droplets_.Begin(); it!=droplets_.End(); ++it)
            it->node_->SetEnabled(false);

        UnsubscribeFromAllEvents();
    }
}

void GEF_Rain::SetAnimationSet(const String& filename)
{
//    URHO3D_LOGINFO("GEF_Rain() - SetAnimationSet !");

    filename_ = filename;

    for (PODVector<Droplet>::Iterator it=droplets_.Begin(); it!=droplets_.End(); ++it)
        it->Clear();

//    URHO3D_LOGINFO("GEF_Rain() - SetAnimationSet : Droplets Cleared !");

    dropletsSetted_ = false;
}

void GEF_Rain::SetIntensity(float intensity)
{
    intensity_ = intensity;
    Droplet::intensity_[viewport_] = intensity_*incIntensity_;
}

void GEF_Rain::SetDirection(float dir)
{
    incIntensity_ = dir;
    SetIntensity(intensity_);
}

void GEF_Rain::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    Droplet::spawningMap_[viewport_] = nullptr;

    // Check Background
    if (!paused_ && World2D::IsVisibleRectInFullBackGround())
    {
        URHO3D_LOGERRORF("GEF_Rain() - HandleUpdate() : worldVisibleRect is in Full BackGround => rain paused !");
        
        for (PODVector<Droplet>::Iterator it=droplets_.Begin(); it!=droplets_.End(); ++it)
            it->node_->SetEnabled(false);

        paused_ = true;
        return;
    }
    
    if (paused_ && !World2D::IsVisibleRectInFullBackGround())
    {
        URHO3D_LOGERRORF("GEF_Rain() - HandleUpdate() : worldVisibleRect is not in Full BackGround => rain resumed !");

        Droplet::SetSpawningMap(viewport_);
        for (PODVector<Droplet>::Iterator it=droplets_.Begin(); it!=droplets_.End(); ++it)
            it->Reset();

        paused_ = false;
    }

    // Droplets update
    if (paused_)
        return;
    
    if (!Droplet::spawningMap_[viewport_])
        Droplet::SetSpawningMap(viewport_);

    float timeStep = eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    raintimer_.Reset();

    while (iCount_ < Droplet::NUM_DROPLETS)
    {
        droplets_[iCount_].Update(timeStep);
        iCount_++;

        if (raintimer_.Expired())
        {
//            URHO3D_LOGWARNINGF("GEF_Rain() - HandleUpdate() : Timer Expiration(%u msec) at index =%u", DROPLET_TIMEUPDATE, iCount_);
            break;
        }
    }

    if (iCount_ == Droplet::NUM_DROPLETS)
    {
#if defined(DROPLET_ACTIVE_FLUID)
        if (GameContext::Get().gameConfig_.fluidEnabled_)
        {
#if defined(DEBUG)
            const float ratiointensity = 0.75f;
#else
            const float ratiointensity = 0.01f;
#endif
            Map* map = World2D::GetCurrentMap(viewport_);
            if (map)
            {
                for (unsigned i=0; i < Droplet::hittedTiles_.Size(); i++)
                {
                    FluidCell* fluidcell = map->GetFluidCellPtr(Droplet::hittedTiles_[i], ViewManager::INNERVIEW_Index);
                    if (fluidcell && fluidcell->type_ != BLOCK)
                        fluidcell->AddFluid(WATER, ratiointensity * intensity_ * FLUID_MINVALUE);
                }
            }
            //        URHO3D_LOGERRORF("GEF_Rain() - HandleUpdate() : Updated in %u/%u msec ... iCount=%u/%u hittedTiles=%u", raintimer_.GetCurrentTime(), raintimer_.GetExpirationTime(), iCount_, Droplet::NUM_DROPLETS, Droplet::hittedTiles_.Size());
            Droplet::hittedTiles_.Clear();
        }
#endif

        iCount_ = 0;
    }
}

void GEF_Rain::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    if (!IsEnabledEffective())
        return;

    for (unsigned i = 0; i < Droplet::NUM_DROPLETS; ++i)
    {
        if (droplets_[i].node_->IsEnabled() && droplets_[i].animatedSprite_)
            droplets_[i].animatedSprite_->DrawDebugGeometry(debug, depthTest);
    }
}

void GEF_Rain::Dump() const
{
    URHO3D_LOGINFOF("GEF_Rain() - Dump() : ...");

    for (unsigned i = 0; i < Droplet::NUM_DROPLETS; ++i)
    {
        const Droplet& droplet = droplets_[i];
        URHO3D_LOGINFOF("  Droplet[%u] : enabled=%s position=%F %F layerIndex=%d animatedSprite_=%u drawRect=%s ...", i,
                        droplet.node_ ? (droplet.node_->IsEnabled()?"true":"false") : "NA", droplet.position_.x_, droplet.position_.y_,
                        droplet.layerIndex_, droplet.animatedSprite_, droplet.animatedSprite_ ? droplet.animatedSprite_->GetDrawRect().ToString().CString(): "NA");
    }
}
