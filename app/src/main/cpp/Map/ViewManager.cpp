#include <Urho3D/Urho3D.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundListener.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/RenderPath.h>
#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/UI/UI.h>

#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

//#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameAttributes.h"
#include "GameHelpers.h"

#include "GOC_Controller.h"
#include "GOC_Destroyer.h"
#include "GOC_Move2D.h"

#include "Actor.h"
#include "Player.h"

#include "ObjectMaped.h"
#include "ObjectFeatured.h"
#include "MapWorld.h"

#include "UIC_MiniMap.h"

#include "ViewManager.h"


/// Camera Ground Focus Offset
const float CAMERAFOCUS_WALKOFFSET = 3.f;

/// Camera Velocity Ratio
const float CAMERAFOCUS_VELRATIO = 0.85f;


void ViewportInfo::Clear()
{
    focusEnabled_ = false;
    cameraFocusZoom_ = 1.f;
    cameraYaw_ = 0.f;
    cameraPitch_ = 0.f;
    camMotionSpeed_ = 0.f;
    currentCamMotionSpeed_ = 0.f;
    cameraFocus_ = Vector2::ZERO;
    cameraMotion_ = Vector3::ZERO;

    // never remove the default camera node
    if (cameraNode_ && cameraNode_ != GameContext::Get().cameraNode_)
        cameraNode_->Remove();

    if (cameraFocusNode_)
        cameraFocusNode_->Remove();

    camera_.Reset();

    nodesOnFocus_.Clear();
}

/// ViewManager Implementation

ViewManager* ViewManager::viewManager_ = 0;
Vector<int> ViewManager::layerZ_;
Vector<int> ViewManager::viewZ_;
HashMap<int, unsigned> ViewManager::layerZIndexes_;
HashMap<int, unsigned> ViewManager::viewZIndexes_;
Vector<String> ViewManager::viewZNames_;
Vector<String> ViewManager::viewIdNames_;
Vector<int> ViewManager::fluidZ_;
Vector<unsigned> ViewManager::viewZIndex2fluidZIndex_;

HashMap<int, unsigned> ViewManager::layerMask_;
HashMap<int, unsigned> ViewManager::viewMask_;
HashMap<int, unsigned> ViewManager::effectMask_;
unsigned ViewManager::INNERVIEW_Index = 0U;
unsigned ViewManager::INNERLAYER_Index = 0U;
unsigned ViewManager::FRONTVIEW_Index = 0U;
unsigned ViewManager::MAX_PURELAYERS = 0U;
unsigned ViewManager::FLUIDINNERVIEW_Index = 0U;
unsigned ViewManager::FLUIDFRONTVIEW_Index = 0U;

IntRect ViewManager::viewportRects_[MAX_VIEWPORTS][MAX_VIEWPORTS];

ViewManager::ViewManager(Context* context) :
    Object(context),
    scene_(0),
    numViewports_(0U)
{
    viewManager_ = this;
}

ViewManager::~ViewManager()
{
    viewManager_ = 0;
}

void ViewManager::RegisterObject(Context* context)
{
    context->RegisterFactory<ViewManager>();

    ResetDefault();
}

void ViewManager::ResetDefault()
{
    layerZ_.Clear();
    viewZ_.Clear();
    viewZNames_.Clear();
    layerZIndexes_.Clear();
    viewZIndexes_.Clear();
    fluidZ_.Clear();

    layerMask_.Clear();
    viewMask_.Clear();
    effectMask_.Clear();

    RegisterLayer(BACKGROUND, BACKGROUND_MASK);
    RegisterLayer(BACKVIEW, BACKVIEW_MASK);
    RegisterLayer(INNERVIEW, INNERVIEW_MASK);
    RegisterLayer(OUTERVIEW, OUTERVIEW_MASK);
    RegisterLayer(FRONTVIEW, FRONTVIEW_MASK|BACKGROUND_MASK);

    RegisterLayer(BACKBIOME, BACKGROUND_MASK);
    RegisterLayer(BACKINNERBIOME, INNERBIOME_MASK);
    RegisterLayer(FRONTINNERBIOME, INNERBIOME_MASK);
    RegisterLayer(OUTERBIOME, FRONTBIOME_MASK);
    RegisterLayer(FRONTBIOME, FRONTBIOME_MASK);
    RegisterLayer(BACKACTORVIEW, BACKGROUND_MASK);
    RegisterLayer(THRESHOLDVIEW, FRONTVIEW_MASK|INNERVIEW_MASK);

    RegisterViewId(FrontView_ViewId, "FRONTVIEW");
    RegisterViewId(BackGround_ViewId, "BACKGROUND");
    RegisterViewId(InnerView_ViewId, "INNERVIEW");
    RegisterViewId(BackView_ViewId, "BACKVIEW");
    RegisterViewId(OuterView_ViewId, "OUTERVIEW");

    RegisterSwitchView(INNERVIEW, "INNERVIEW", DEFAULT_INNERVIEW_MASK);
    RegisterSwitchView(FRONTVIEW, "FRONTVIEW", DEFAULT_FRONTVIEW_MASK);

    RegisterFluidView(INNERVIEW);
//    RegisterFluidView(INNERVIEW_2OUTERVIEW);
//    RegisterFluidView(FRONTVIEW_2OUTERVIEW);
    RegisterFluidView(FRONTVIEW);

//    RegisterEffectView(INNERVIEW, WEATHERINSIDEVIEW_MASK);
//    RegisterEffectView(FRONTVIEW, WEATHEROUTSIDEVIEW_MASK);

    ObjectFeatured::SetViewZs(viewZ_);
}

/// Register Layer
void ViewManager::RegisterLayer(int Z, unsigned layermask)
{
    layerZ_.Push(Z);
    layerZIndexes_[Z] = layerZ_.Size()-1;
    layerMask_[Z] = layermask;

    if (Z == INNERVIEW)
        INNERLAYER_Index = layerZ_.Size()-1;

    if (Z == FRONTVIEW)
        MAX_PURELAYERS = layerZ_.Size();
}

void ViewManager::RegisterViewId(int viewid, const String& name)
{
    if (viewIdNames_.Size() <= viewid)
        viewIdNames_.Resize(viewid+1);

    viewIdNames_[viewid] = name;
}

/// Register Switcheable Views (Switcheable view : INNERVIEW, FRONTVIEW)
void ViewManager::RegisterSwitchView(int Z, const String& name, unsigned viewmask)
{
    viewZ_.Push(Z);
    viewZNames_.Push(name);
    viewZIndexes_[Z] = viewZ_.Size()-1;
    viewMask_[Z] = viewmask;
    viewZIndex2fluidZIndex_.Push(0);

    if (Z == INNERVIEW)
        INNERVIEW_Index = viewZ_.Size()-1;

    if (Z == FRONTVIEW)
        FRONTVIEW_Index = viewZ_.Size()-1;
}

/// Register Fluid Views (INNERVIEW, OUTERVIEW, FRONTVIEW)
void ViewManager::RegisterFluidView(int Z)
{
    fluidZ_.Push(Z);

    Vector<int>::ConstIterator it = viewZ_.Find(Z);
    if (it != viewZ_.End())
    {
        unsigned zIndex = it - viewZ_.Begin();
        viewZIndex2fluidZIndex_[zIndex] = fluidZ_.Size()-1;
    }

    if (Z == INNERVIEW)
        FLUIDINNERVIEW_Index = viewZ_.Size()-1;

    if (Z == FRONTVIEW)
        FLUIDFRONTVIEW_Index = viewZ_.Size()-1;
}

/// Register Effect Views (Effect view : Rain, Cloud ...)
void ViewManager::RegisterEffectView(int Z, unsigned viewmask)
{
    effectMask_[Z] = viewmask;
}

void ViewManager::SetViewportLayout(int numviewports, bool force)
{
    URHO3D_LOGINFOF("ViewManager() - SetViewportLayout : numviewports=%d ...", numviewports);

    if (numViewports_ == numviewports && !force)
        return;

    if (numviewports < 1 || numviewports > MAX_VIEWPORTS)
        return;

    Renderer* renderer = GameContext::Get().context_->GetSubsystem<Renderer>();
    if (!renderer)
        return;

    numViewports_ = numviewports;

    renderer->SetNumViewports(numviewports);

    // Reset the Cameras
    for (unsigned i=0; i < MAX_VIEWPORTS; i++)
        viewportInfos_[0].Clear();

    // Clone the default render path so that we do not interfere with the other viewport, then add
    // bloom and FXAA post process effects to the front viewport. Render path commands can be tagged
    // for example with the effect name to allow easy toggling on and off. We start with the effects
    // disabled.
//    ResourceCache* cache = GetSubsystem<ResourceCache>();
//    SharedPtr<RenderPath> effectRenderPath = viewport->GetRenderPath()->Clone();
//    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/Bloom.xml"));
//    effectRenderPath->Append(cache->GetResource<XMLFile>("PostProcess/FXAA2.xml"));
//    // Make the bloom mixing parameter more pronounced
//    effectRenderPath->SetShaderParameter("BloomMix", Vector2(0.9f, 0.6f));
//    effectRenderPath->SetEnabled("Bloom", false);
//    effectRenderPath->SetEnabled("FXAA2", false);
//    viewport->SetRenderPath(effectRenderPath);

    String renderPathFilename_ = GameContext::Get().gameConfig_.fluidEnabled_ ? "RenderPaths/ForwardUrho2D.xml" : "RenderPaths/Forward.xml";

    SharedPtr<RenderPath> renderpath(new RenderPath());
    renderpath->Load(GetSubsystem<ResourceCache>()->GetResource<XMLFile>(renderPathFilename_));

    // create viewports
    for (unsigned i=0; i < numviewports; i++)
    {
        ViewportInfo& vinfo = viewportInfos_[i];

        // create camera focus node
        Node* node = GameContext::Get().rootScene_->CreateChild("CameraFocus", LOCAL);
        vinfo.cameraFocusNode_ = WeakPtr<Node>(node);
        // get or create camera node
        node = (i == 0 && GameContext::Get().cameraNode_) ? GameContext::Get().cameraNode_ : GameContext::Get().rootScene_->CreateChild("Camera", LOCAL);
        vinfo.cameraNode_ = WeakPtr<Node>(node);

        // create camera
        Camera* camera = node->GetOrCreateComponent<Camera>(LOCAL);
        camera->SetNearClip(-100.f);
        camera->SetFarClip(100.f);
        camera->SetFov(60.f);
        camera->SetOrthographic(true);
        camera->SetViewport(i);

        renderer->SetCullMode(CULL_NONE, camera);
        GameContext::Get().renderer2d_->UpdateFrustumBoundingBox(camera);
        vinfo.camera_ = camera;

        // reset the current viewZ
        vinfo.viewZindex_ = -1;

        // create viewport : the rect of the viewport will be setted in ResetLayouts()
        SharedPtr<Viewport> viewport(new Viewport(GameContext::Get().context_, GameContext::Get().rootScene_, camera, renderpath));
        renderer->SetViewport(i, viewport);

        URHO3D_LOGINFOF("ViewManager() - SetViewportLayout : viewport(%u) added with camera=%u ...", i, camera);
    }

    // update gamestatics
    GameContext::Get().camera_ = viewportInfos_[0].camera_;
    GameContext::Get().cameraNode_ = viewportInfos_[0].cameraNode_;

    // update WorldViewInfos
    if (World2D::GetWorld())
        World2D::GetWorld()->OnViewportUpdated();

    URHO3D_LOGINFOF("ViewManager() - SetViewportLayout : numviewports=%d renderPathFilename_=%s ... OK !", numviewports, renderPathFilename_.CString());
}

void ViewManager::ResizeViewportRects(int numviewports)
{
    Graphics* graphics = GameContext::Get().context_->GetSubsystem<Graphics>();

    // Single viewport
    viewportRects_[0][0] = IntRect(0, 0, graphics->GetWidth(), graphics->GetHeight());
    if (numviewports > 1)
    {
        const int border = 2;

        // 2 viewports
        viewportRects_[1][0] = IntRect(0, 0, graphics->GetWidth()/2 -border, graphics->GetHeight());
        viewportRects_[1][1] = IntRect(graphics->GetWidth()/2 +border, 0, graphics->GetWidth(), graphics->GetHeight());
        if (numviewports > 2)
        {
            // 3 viewports
            viewportRects_[2][0] = IntRect(0, 0, graphics->GetWidth()/2 -border, graphics->GetHeight()/2 -border);
            viewportRects_[2][1] = IntRect(graphics->GetWidth()/2 +border, 0, graphics->GetWidth(), graphics->GetHeight()/2 -border);
            viewportRects_[2][2] = IntRect(graphics->GetWidth()/4, graphics->GetHeight()/2 +border, 3 * graphics->GetWidth() / 4, graphics->GetHeight());
            if (numviewports > 3)
            {
                // 4 viewports
                viewportRects_[3][0] = IntRect(0, 0, graphics->GetWidth()/2 -border, graphics->GetHeight()/2 -border);
                viewportRects_[3][1] = IntRect(graphics->GetWidth()/2 +border, 0, graphics->GetWidth(), graphics->GetHeight()/2 -border);
                viewportRects_[3][2] = IntRect(0, graphics->GetHeight()/2 +border, graphics->GetWidth()/2 -border, graphics->GetHeight());
                viewportRects_[3][3] = IntRect(graphics->GetWidth()/2 +border, graphics->GetHeight()/2 +border, graphics->GetWidth(), graphics->GetHeight());
            }
        }
    }
}

void ViewManager::ResizeViewports()
{
    if (numViewports_ < 1 || numViewports_ > MAX_VIEWPORTS)
        return;

    Renderer* renderer = GameContext::Get().context_->GetSubsystem<Renderer>();
    if (!renderer)
        return;

    // Single viewport
    ResizeViewportRects(numViewports_);

    // update viewports rects & cameras orthosizes
    for (unsigned i=0; i < numViewports_; i++)
    {
        Viewport* viewport = renderer->GetViewport(i);
        if (!viewport)
            continue;

        Camera* camera = viewportInfos_[i].camera_;
        if (!camera)
            continue;

        viewport->SetRect(viewportRects_[numViewports_-1][i]);

        float zoom = GameContext::Get().CameraZoomDefault_;
        float height = 1080.f;
        /*
                float height = (float)viewportRects_[numViewports_-1][i].Height();

                // small window : cut scene
                if (height <= 768.f)
                {
                    zoom = GameContext::Get().cameraZoomFactor_ * height / 1080.f;
                    //zoom = 1.f;
                }
                // large window : adjust orthosize to 1080
                else
                {
                    zoom = GameContext::Get().cameraZoomFactor_;
                    height = 1080.f;
                }
        */
#ifdef __ANDROID__
        height = 768.f;
#endif

        camera->SetZoom(zoom);
        camera->SetOrthoSize(height * PIXEL_SIZE);
    }

    // Add Debug viewport for rttscene
    if (GameContext::Get().gameConfig_.debugRttScene_ && GameContext::Get().rttScene_)
    {
        if (renderer->GetNumViewports() < numViewports_+1)
        {
            SharedPtr<Viewport> viewport(new Viewport(GameContext::Get().context_, GameContext::Get().rttScene_, GameContext::Get().rttScene_->GetChild("Camera")->GetComponent<Camera>()));
            renderer->SetNumViewports(numViewports_+1);
            renderer->SetViewport(numViewports_, viewport);
        }

        int border = 5;
        int w = GameContext::Get().screenwidth_/3;
        int h = GameContext::Get().screenheight_/3;

        Viewport* viewport = renderer->GetViewport(numViewports_);
        viewport->SetRect(IntRect(GameContext::Get().screenwidth_ - w - border, GameContext::Get().screenheight_ - h - border,
                                  GameContext::Get().screenwidth_ - border, GameContext::Get().screenheight_ - border));
    }
    else
    {
        if (renderer->GetNumViewports() > numViewports_)
            renderer->SetNumViewports(numViewports_);
    }

    renderer->SetClearBack(numViewports_ > 1 && !GameContext::Get().gameConfig_.debugRttScene_);
}

void ViewManager::SetScene(Scene* scene, const Vector2& focus)
{
    if (!scene)
        return;

    scene_ = scene;

    SetCamera(focus);

    SubscribeToEvent(GO_DETECTORSWITCHVIEWIN, URHO3D_HANDLER(ViewManager, HandleSwitchViewZ));
    SubscribeToEvent(GO_DETECTORSWITCHVIEWOUT, URHO3D_HANDLER(ViewManager, HandleSwitchViewZ));
}

void ViewManager::SetMiniMapEnable(bool enable)
{
#ifdef HANDLE_MINIMAP
    // Set Minimap UIC
    {
        if (enable)
        {
            UIC_MiniMap* minimap = GetCameraNode()->GetOrCreateComponent<UIC_MiniMap>();
        }
        else
        {
            UIC_MiniMap* minimap = GetCameraNode()->GetComponent<UIC_MiniMap>();
            if (minimap)
                minimap->Clear();
        }
    }

    // Set Panel Visibility
    UI* ui = GetSubsystem<UI>();
    UIElement* minimap = ui->GetRoot()->GetChild(String("MiniMap"));
    if (minimap)
        minimap->SetVisible(enable);
#endif // HANDLE_MINIMAP
}

int ViewManager::SwitchToViewIndex(int viewZindex, Node* node, int viewport)
{
    int viewZ = viewZ_[viewZindex];

    /// never switch directly a mounted entity that is in same viewport => made by the parent node in GOC_Destroy::SetViewZ
    if (node && node->GetVar(GOA::ISMOUNTEDON).GetUInt() && ViewManager::Get()->GetNumViewports() == 1)
        return viewZ;

    unsigned layermask = layerMask_[viewZ];

    GOC_Controller* controller = node ? node->GetDerivedComponent<GOC_Controller>() : 0;
    GOC_Destroyer* destroyer = node ? node->GetComponent<GOC_Destroyer>() : 0;
    bool islocalplayer = (controller && controller->IsMainController() && controller->GetThinker() && controller->GetThinker()->IsInstanceOf<Player>());

    if (node)
    {
        /// if not a main player => set viewZ, viewMask for this node and stop here
        if (!islocalplayer)
        {
            if (destroyer)
                destroyer->SetViewZ(viewZ, layermask);

            return viewZ;
        }
        /// multiviewport actived => use the viewport assigned to the player
        else if (controller && ViewManager::Get()->GetNumViewports() > 1)
        {
            viewport = Min(controller->GetThinker() ? controller->GetThinker()->GetControlID() : 0, (int)ViewManager::Get()->GetNumViewports()-1);
        }
    }

    viewportInfos_[viewport].lastViewZindex_ = viewportInfos_[viewport].viewZindex_;
    viewportInfos_[viewport].viewZindex_ = viewZindex;
    viewportInfos_[viewport].layerZindex_ = viewZindex == INNERVIEW_Index ? 2 : 4;

    URHO3D_LOGINFOF("ViewManager() - SwitchToViewIndex : viewport=%d switch to viewZ=%d viewZindex=%d !", viewport, viewZ, viewZindex);

    /// Set the camera view mask
//    unsigned viewportmask = (VIEWPORTSCROLLER_INSIDE_MASK << viewport);
//    if (viewZ != INNERVIEW)
//        viewportmask |= (VIEWPORTSCROLLER_OUTSIDE_MASK << viewport);
//    GetCamera(viewport)->SetViewMask(viewMask_[viewZ] | viewportmask);

    unsigned viewportmask;
    if (viewZ != INNERVIEW)
        viewportmask = (VIEWPORTSCROLLER_OUTSIDE_MASK << viewport);
    else
        viewportmask = (VIEWPORTSCROLLER_INSIDE_MASK << viewport);
    GetCamera(viewport)->SetViewMask(viewMask_[viewZ] | viewportmask);

    if (GameContext::Get().rttScene_)
        GameContext::Get().rttScene_->GetChild("Camera")->GetComponent<Camera>()->SetViewMask(viewMask_[viewZ] | viewportmask);

    /// if a player or no node, set viewZindex_, change TileObjects to ViewZ and update ViewMask Camera
    if (node)
    {
        if (destroyer)
            destroyer->SetViewZ(viewZ, layermask);

        // Update Light
        if (islocalplayer)
            GameHelpers::SetLightActivation((Player*)controller->GetThinker());
        else
            GameHelpers::SetLightActivation(node, viewport);

//        URHO3D_LOGINFOF("ViewManager() - SwitchToViewIndex : node=%s(%u) - player in viewport=%d at ViewZ=%d => Player Switch - Camera Mask = %u !",
//                        node->GetName().CString(), node->GetID(), viewport, viewZ, GetCamera(viewport)->GetViewMask());
    }

    VariantMap& eventData = context_->GetEventDataMap();
    eventData[ViewManager_SwitchView::VIEWPORT] = viewport;
    eventData[ViewManager_SwitchView::VIEWZINDEX] = viewZindex;
    eventData[ViewManager_SwitchView::VIEWZ] = viewZ;
    SendEvent(VIEWMANAGER_SWITCHVIEW, eventData);

    /// Update UIC MiniMap
    SendEvent(WORLD_MAPUPDATE);

    /// Update the layering (backviewactor if needed) of the other visible entities (including objectMaped)
    PODVector<Node*> visiblenodes;
    World2D::GetVisibleEntities(visiblenodes);
    for (unsigned i=0; i < visiblenodes.Size(); i++)
    {
        if (visiblenodes[i] != node)
            GameHelpers::UpdateLayering(visiblenodes[i]);
    }

    return viewZ;
}

void ViewManager::SwitchToViewZ(int viewZ, Node* node, int viewport)
{
    if (!viewZ_.Contains(viewZ))
    {
        int newViewZ = GetNearViewZ(viewZ);
        URHO3D_LOGWARNINGF("ViewManager() - SwitchToViewZ : for node=%u ViewZ = %d Not Register ! get the nearest ViewZ=%d", node ? node->GetID() : 0, viewZ, newViewZ);
        viewZ = newViewZ;
    }

//    if (node)
//        URHO3D_LOGINFOF("ViewManager() - SwitchToViewZ : node=%s(%u) change to ViewZ = %d ", node->GetName().CString(), node->GetID(), viewZ);
//    else
//        URHO3D_LOGINFOF("ViewManager() - SwitchToViewZ : ViewZ = %d", viewZ);

    viewZ = SwitchToViewIndex(viewZ_.Find(viewZ) - viewZ_.Begin(), node, viewport);
}

int ViewManager::SwitchNextZ(Node* node)
{
    if (!viewZ_.Size())
        return -1;

//    if (!node)
//        return -1;

    int nextViewZindex = (viewportInfos_[0].viewZindex_+1) % viewZ_.Size();

    URHO3D_LOGINFOF("ViewManager() - ChangeView To Next View : ViewZ(%d) = %d node=%u", nextViewZindex, viewZ_[nextViewZindex], node);

    return SwitchToViewIndex(nextViewZindex, node);
}

int ViewManager::SwitchPreviousZ(Node* node)
{
    if (!viewZ_.Size())
        return -1;

//    if (!node)
//        return -1;

    int prevViewZindex = (viewportInfos_[0].viewZindex_ == 0 ? viewZ_.Size()-1 : viewportInfos_[0].viewZindex_-1);

    URHO3D_LOGINFOF("ViewManager() - ChangeView To Prev View : ViewZ(%d) = %d node=%u", prevViewZindex, viewZ_[prevViewZindex], node);

    return SwitchToViewIndex(prevViewZindex, node);
}

int ViewManager::GetViewport(Camera* camera) const
{
    if (GetNumViewports() <= 1)
        return 0;

    int numviewports = GetNumViewports();
    for (int viewport=0; viewport < numviewports; viewport++)
        if (viewportInfos_[viewport].camera_ == camera)
            return viewport;

    return 0;
}

int ViewManager::GetViewportAt(int screenx, int screeny)
{
    if (Get()->GetNumViewports() <= 1)
        return 0;

    int numviewports = Get()->GetNumViewports();
    IntVector2 screenpoint(screenx, screeny);
    IntRect* viewportrects = viewportRects_[numviewports-1];

    for (int viewport=0; viewport < numviewports; viewport++)
        if (viewportrects[viewport].IsInside(screenpoint) == INSIDE)
            return viewport;

    return 0;
}

Rect ViewManager::GetViewRect(int viewport) const
{
    const Frustum& frustum = viewportInfos_[viewport].camera_->GetFrustum();
    return Rect(Vector2(frustum.vertices_[2].x_, frustum.vertices_[2].y_), Vector2(frustum.vertices_[0].x_, frustum.vertices_[0].y_));
}

const IntRect& ViewManager::GetViewportRect(int viewport) const
{
    return viewportRects_[numViewports_-1][viewport];
}

int ViewManager::GetNearLayerZ(int z, int dir)
{
    // return near layerZ below
    if (dir == -1)
    {
        if (z < 0)
            return -1;

        int i = (int)layerZ_.Size()-1;
        for(;;)
        {
            if (i < 0)
                break;

            if (layerZ_[i] <= z)
                return layerZ_[i];

            i--;
        }
    }
    else
        // return near layerZ above
    {
        if (z > layerZ_.Back())
            return -1;

        int i = 0;
        for(;;)
        {
            if (i >= (int)layerZ_.Size())
                break;

            if (layerZ_[i] >= z)
                return layerZ_[i];

            i++;
        }
    }
    return -1;
}

int ViewManager::GetNearViewZ(int z, int dir)
{

    if (dir == -1) // return near viewZ below
    {
        if (z < 0)
            return -1;

        int i = (int)viewZ_.Size()-1;
        for(;;)
        {
            if (i < 0)
                break;

            if (viewZ_[i] <= z)
                return viewZ_[i];

            i--;
        }
    }
    else if (dir == 1) // return near viewZ above
    {
        if (z > viewZ_.Back())
            return -1;

        int i = 0;
        for(;;)
        {
            if (i >= (int)viewZ_.Size())
                break;

            if (viewZ_[i] >= z)
                return viewZ_[i];

            i++;
        }
    }
    else
    {
        if (z > BACKVIEW && z < OUTERVIEW)
            return INNERVIEW;
        else
            return FRONTVIEW;
    }
    return -1;
}

void ViewManager::SetCamera(const Vector2& focus, int viewport)
{
    int numviewportstoset = 1;
    if (viewport == -1)
    {
        // Remove th soundListener.
        GetContext()->GetSubsystem<Audio>()->SetListener(0);
        viewport = 0;
        numviewportstoset = numViewports_;
    }

    while (numviewportstoset > 0)
    {
        ViewportInfo& vinfo = viewportInfos_[viewport];

        vinfo.cameraFocusZoom_ = vinfo.camera_->GetZoom();
        vinfo.camMotionSpeed_ = 0.f;
        vinfo.currentCamMotionSpeed_ = 0.f;

        vinfo.cameraFocus_ = focus;
        vinfo.cameraMotion_ = Vector3::ZERO;

        vinfo.cameraNode_->SetPosition2D(focus);
        vinfo.cameraFocusNode_->SetPosition2D(focus);
        vinfo.camera_->SetNearClip(-100.f);
        vinfo.camera_->SetFarClip(100.f);
        vinfo.camera_->SetFov(60.f);

        // SoundListener on camera node
        // TODO : add SoundListener on the main viewport
        if (!GetContext()->GetSubsystem<Audio>()->GetListener())
        {
            SoundListener* soundListener = vinfo.cameraNode_->GetOrCreateComponent<SoundListener>(LOCAL);
            GetContext()->GetSubsystem<Audio>()->SetListener(soundListener);
        }

        numviewportstoset--;
        viewport++;
    }
}

void ViewManager::AddFocus(Node* nodeToFocus, bool center, int viewport)
{
    if (!nodeToFocus)
        return;

    ViewportInfo& vinfo = viewportInfos_[viewport];

    int viewZ = nodeToFocus->GetVar(GOA::ONVIEWZ).GetInt();

    vinfo.cameraFocusZoom_ = vinfo.camera_->GetZoom();

    WeakPtr<Node> node(nodeToFocus);
    if (!vinfo.nodesOnFocus_.Contains(node))
        vinfo.nodesOnFocus_.Push(node);

    SetFocusEnable(true, viewport);

    if (GetCurrentViewZ(viewport) != viewZ)
        SwitchToViewZ(viewZ, 0, viewport);

    if (center)
    {
        vinfo.cameraFocusNode_->SetPosition2D(nodeToFocus->GetWorldPosition2D());
        vinfo.cameraNode_->SetPosition2D(nodeToFocus->GetWorldPosition2D());

        UpdateFocus(viewport);
//        World2D::GetWorld()->UpdateInstant(viewport, nodeToFocus->GetWorldPosition2D(), 1.f);
    }

    URHO3D_LOGINFOF("ViewManager() - AddFocus : %s(%u) - viewport=%d viewZ=%d NodePosition=%s NumNodesOnFocus=%u ... OK !",
                    nodeToFocus->GetName().CString(), nodeToFocus->GetID(), viewport, viewZ, nodeToFocus->GetWorldPosition2D().ToString().CString(),
                    viewportInfos_[0].nodesOnFocus_.Size());
}

void ViewManager::ResetFocus(int viewport)
{
    int numviewportstoset = 1;
    if (viewport == -1)
    {
        viewport = 0;
        numviewportstoset = numViewports_;
    }

    while (numviewportstoset > 0)
    {
        viewportInfos_[viewport].nodesOnFocus_.Clear();
        SetFocusEnable(false, viewport);

        numviewportstoset--;
        viewport++;
    }
}

void ViewManager::SetFocusEnable(bool enable, int viewport)
{
    int numviewportstoset = 1;
    if (viewport == -1)
    {
        viewport = 0;
        numviewportstoset = numViewports_;
    }

    while (numviewportstoset > 0)
    {
        ViewportInfo& vinfo = viewportInfos_[viewport];

#ifdef HANDLE_MINIMAP
        if (viewport == 0 && !vinfo.nodesOnFocus_.Empty() && vinfo.nodesOnFocus_.Front() != 0)
            vinfo.cameraNode_->GetOrCreateComponent<UIC_MiniMap>()->SetFollowedNode(enable ? vinfo.nodesOnFocus_.Front() : vinfo.cameraNode_);
#endif

        if (vinfo.nodesOnFocus_.Empty())
            enable = false;

        vinfo.focusEnabled_ = enable;

        if (enable)
        {
            // Desactive the Camera Animations
            if (vinfo.cameraNode_->GetObjectAnimation())
                vinfo.cameraNode_->SetAnimationEnabled(false);
            if (vinfo.camera_->GetObjectAnimation())
                vinfo.camera_->SetAnimationEnabled(false);

            vinfo.camera_->SetZoom(vinfo.cameraFocusZoom_);
        }

        numviewportstoset--;
        viewport++;
    }
}


//#define CAMERAFOCUS_TEMPO

#if defined(CAMERAFOCUS_TEMPO) && defined(CAMERAFOCUS_ADJUSTWALK)
static const float CAMERAFOCUS_DELAY = 0.25f;
static float sTempoFocus = 0.f;
#endif

void ViewManager::MoveCamera(int viewport, const float& timeStep)
{
    if (!scene_ || !scene_->IsUpdateEnabled())
        return;

    ViewportInfo& vinfo = viewportInfos_[viewport];

    if (!vinfo.focusEnabled_)
        return;

    UpdateFocus(viewport, timeStep);

    vinfo.cameraFocusNode_->SetPosition2D(vinfo.cameraFocus_);

#ifndef CAMERAMOVE_LERP
    /// Simple Move
    {
//        URHO3D_LOGINFOF("ViewManager() - MoveCamera SIMPLE : cameraFocus_=%s cameraFocusNode_=%s(%u) cameraNode_=%u",
//                        vinfo.cameraFocus_.ToString().CString(), vinfo.cameraFocusNode_->GetName().CString(), vinfo.cameraFocusNode_->GetID(), vinfo.cameraNode_);
        vinfo.cameraNode_->SetPosition2D(vinfo.cameraFocus_);
        return;
    }
#else
    /// LERP Move
    {
//        URHO3D_LOGINFOF("ViewManager() - MoveCamera LERP : cameraFocus_=%s cameraFocusNode_=%s(%u) cameraNode_=%u",
//                        vinfo.cameraFocus_.ToString().CString(), vinfo.cameraFocusNode_->GetName().CString(), vinfo.cameraFocusNode_->GetID(), vinfo.cameraNode_);

        vinfo.cameraMotion_ = vinfo.cameraFocus_ - vinfo.cameraNode_->GetPosition2D();

        if (Abs(vinfo.cameraMotion_.x_) > 0.25f || Abs(vinfo.cameraMotion_.y_) > 0.25f)
            vinfo.cameraNode_->Translate(vinfo.cameraMotion_ * timeStep);
    }
#endif
}

void ViewManager::UpdateFocus(int viewport, const float& timeStep)
{
    int numviewportstoset = 1;
    if (viewport == -1)
    {
        viewport = 0;
        numviewportstoset = numViewports_;
    }

    while (numviewportstoset > 0)
    {
        ViewportInfo& vinfo = viewportInfos_[viewport];

        const unsigned initialNumNodes = vinfo.nodesOnFocus_.Size();

        if (initialNumNodes)
        {
            Vector2 position;
            Vector2 velocity;
            Node* node;
            int movestate;

            List<WeakPtr<Node> >::Iterator it=vinfo.nodesOnFocus_.Begin();
            while (it != vinfo.nodesOnFocus_.End())
            {
                node = it->Get();
                if (node != 0)
                {
                    position += node->GetWorldPosition2D();
                    velocity += node->GetVar(GOA::VELOCITY).GetVector2();

#ifdef CAMERAFOCUS_ADJUSTWALK
                    movestate = node->GetVar(GOA::MOVESTATE).GetUInt();
                    if ((movestate & MV_WALK) && !(movestate & MV_INFALL))
                    {
#ifdef CAMERAFOCUS_TEMPO
                        sTempoFocus += timeStep;
                        if (sTempoFocus >= CAMERAFOCUS_DELAY)
#endif
                            position.y_ += CAMERAFOCUS_WALKOFFSET;
                    }
#ifdef CAMERAFOCUS_TEMPO
                    else
                        sTempoFocus = 0.f;
#endif
#endif
                    it++;
                }
                else
                {
                    it = vinfo.nodesOnFocus_.Erase(it);
                }
            }

            if (vinfo.nodesOnFocus_.Size())
                vinfo.cameraFocus_ = (position + velocity*CAMERAFOCUS_VELRATIO) / (float)vinfo.nodesOnFocus_.Size();
        }

        if (vinfo.nodesOnFocus_.Size() == 0)
        {
            if (initialNumNodes > 0)
            {
                URHO3D_LOGERRORF("ViewManager() - UpdateFocus : ... Focus Lost !");
                ResetFocus(viewport);
            }
        }

        numviewportstoset--;
        viewport++;
    }

//	URHO3D_LOGINFOF("ViewManager() - UpdateFocus : ... cameraFocus=%s numNodesInFocus=%u", cameraFocus_.ToString().CString(), numNodes);
}

void ViewManager::HandleSwitchViewZ(StringHash eventType, VariantMap& eventData)
{
    Node* node = scene_->GetNode(eventData[Go_Detector::GO_GETTER].GetUInt());
    if (!node)
    {
        URHO3D_LOGERRORF("ViewManager() - HandleSwitchViewZ : no node for nodeID=%u !", eventData[Go_Detector::GO_GETTER].GetUInt());
        return;
    }

    GOC_Destroyer* gocDestroyer = node->GetComponent<GOC_Destroyer>();
    if (!gocDestroyer)
    {
        URHO3D_LOGERROR("ViewManager() - HandleSwitchViewZ : no gocDestroyer !");
        return;
    }

    const WorldMapPosition& wmposition = gocDestroyer->GetWorldMapPosition();
    int viewZ = wmposition.viewZ_;

    Map* map = World2D::GetMapAt(wmposition.mPoint_);
    if (!map)
    {
        URHO3D_LOGERRORF("ViewManager() - HandleSwitchViewZ : nodeID=%u no map at mPoint=%s!",
                         node->GetID(), wmposition.mPoint_.ToString().CString());
        return;
    }

    if (eventType == GO_DETECTORSWITCHVIEWIN)
    {
        if (viewZ != INNERVIEW)
        {
//            if (map->GetFeatureAtZ(wmposition.tileIndex_, FRONTVIEW-1) == MapFeatureType::NoMapFeature)
            {
                URHO3D_LOGINFOF("ViewManager() - HandleSwitchViewZ node=%s(%u) FrontView To InnerView", node->GetName().CString(), node->GetID());
                SwitchToViewZ(INNERVIEW, node);
            }
        }
    }
    else
    {
        if (viewZ != FRONTVIEW)
        {
//            if (map->GetFeatureAtZ(wmposition.tileIndex_, BACKVIEW) == MapFeatureType::NoMapFeature)
            {
                URHO3D_LOGINFOF("ViewManager() - HandleSwitchViewZ node=%s(%u) InnerView To FrontView", node->GetName().CString(), node->GetID());
                SwitchToViewZ(FRONTVIEW, node);
            }
        }
    }
}

void ViewManager::Dump() const
{
    for (int i=0; i < numViewports_; i++)
    {
        URHO3D_LOGINFOF("ViewManager - Dump() : viewport=%d currentViewZ=%d currentViewZindex=%d", i, GetCurrentViewZ(i), GetCurrentViewZIndex(i));
    }

}
