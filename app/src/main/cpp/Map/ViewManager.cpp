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
    focusEnabled_          = false;
    cameraFocusZoom_       = 1.f;
    cameraYaw_             = 0.f;
    cameraPitch_           = 0.f;
    camMotionSpeed_        = 0.f;
    currentCamMotionSpeed_ = 0.f;
    cameraFocus_           = Vector2::ZERO;
    cameraMotion_          = Vector3::ZERO;

    // never remove the default camera node
    if (camera_ && camera_ != GameContext::Get().camera_)
        camera_->GetNode()->Remove();

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

unsigned ViewManager::GetLayerMask(int viewZ)
{
    HashMap<int, unsigned>::ConstIterator it = layerMask_.Find(viewZ);
    return it != layerMask_.End() ? it->second_ : DEFAULT_VIEWMASK;
}

unsigned ViewManager::GetViewMask(int viewZ)
{
    HashMap<int, unsigned>::ConstIterator it = viewMask_.Find(viewZ);
    return it != effectMask_.End() ? it->second_ : DEFAULT_VIEWMASK;
}

unsigned ViewManager::GetEffectMask(int viewZ)
{
    HashMap<int, unsigned>::ConstIterator it = effectMask_.Find(viewZ);
    return it != effectMask_.End() ? it->second_ : DEFAULT_VIEWMASK;
}

void ViewManager::SetViewportLayout(int numviewports, bool reset)
{
    if (numViewports_ == numviewports || !GameContext::Get().renderer_)
        return;

    numviewports = Clamp(numviewports, 1, (int)MAX_VIEWPORTS);

    URHO3D_LOGINFOF("ViewManager() - SetViewportLayout : numviewports=%d ...", numviewports);

    // reset the cameras
    if (reset)
    {
        for (unsigned i = 0; i < MAX_VIEWPORTS; i++)
            viewportInfos_[i].Clear();
    }

    RenderPath* renderpath = GameContext::Get().gameConfig_.fluidEnabled_ ? GameContext::Get().renderPaths_[1] : GameContext::Get().renderPaths_[0];

    // create viewports infos
    for (int i = 0; i < numviewports; i++)
    {
        ViewportInfo& vinfo = viewportInfos_[i];

        // create camera focus node
        if (!vinfo.cameraFocusNode_)
            vinfo.cameraFocusNode_ = GameContext::Get().rootScene_->CreateChild("CameraFocus", LOCAL);

        // get or create camera
        if (!vinfo.camera_)
        {
            if (i == 0)
            {
                if (!GameContext::Get().camera_)
                {
                    GameContext::Get().camera_     = GameContext::Get().rootScene_->CreateChild("Camera", LOCAL)->CreateComponent<Camera>(LOCAL);
                    GameContext::Get().cameraNode_ = GameContext::Get().camera_->GetNode();
                }
                vinfo.camera_ = GameContext::Get().camera_;
            }
            else
            {
                vinfo.camera_ = GameContext::Get().rootScene_->CreateChild("Camera", LOCAL)->CreateComponent<Camera>(LOCAL);
            }

            // update camera parameters
            vinfo.camera_->SetNearClip(-100.f);
            vinfo.camera_->SetFarClip(100.f);
            vinfo.camera_->SetFov(60.f);
            vinfo.camera_->SetOrthographic(true);
            vinfo.camera_->SetViewport(i);
        }

        // reset the current viewZ
        vinfo.viewZindex_ = -1;

        // create viewport : the rect of the viewport will be setted in ResetLayouts()
        // TODO : why can we reuse last viewport ? pb in urho ?
//        if (!vinfo.viewport_)
        {
            vinfo.viewport_ = new Viewport(GameContext::Get().context_, GameContext::Get().rootScene_, vinfo.camera_, renderpath);
        }

        URHO3D_LOGINFOF("ViewManager() - SetViewportLayout : viewport=%d use camera=%u ...", i, vinfo.camera_.Get());
    }

    // update renderer
    GameContext::Get().renderer_->SetNumViewports(numviewports);

    for (int i = 0; i < numviewports; i++)
    {
        ViewportInfo& vinfo = viewportInfos_[i];
        GameContext::Get().renderer_->SetCullMode(CULL_NONE, vinfo.camera_);
        GameContext::Get().renderer_->SetViewport(i, vinfo.viewport_);
    }

    ResizeViewports(numviewports);

    numViewports_ = numviewports;

    URHO3D_LOGINFOF("ViewManager() - SetViewportLayout : numviewports=%d ... OK !", numviewports);
}

void ViewManager::ResizeViewports(int numviewports)
{
    if (numviewports == numViewports_)
        return;

    if (numviewports == -1)
        numviewports = numViewports_;

    if (numviewports < 1 || numviewports > MAX_VIEWPORTS)
        return;

    Renderer* renderer = GameContext::Get().context_->GetSubsystem<Renderer>();
    if (!renderer)
        return;

    // Single viewport
    ResizeViewportRects(numviewports);

    // update viewports rects & cameras orthosizes
    for (unsigned i=0; i < numviewports; i++)
    {
        Viewport* viewport = renderer->GetViewport(i);
        if (!viewport)
            continue;

        Camera* camera = viewportInfos_[i].camera_;
        if (!camera)
            continue;

        viewport->SetRect(viewportRects_[numviewports-1][i]);

        float zoom = GameContext::Get().CameraZoomDefault_;
        float height = 1080.f;

#ifdef __ANDROID__
        height = 768.f;
#endif

        camera->SetZoom(zoom);
        camera->SetOrthoSize(height * PIXEL_SIZE);
    }

    // Add Debug viewport for rttscene
    if (GameContext::Get().gameConfig_.debugRttScene_ && GameContext::Get().rttScene_)
    {
        if (renderer->GetNumViewports() < numviewports+1)
        {
            SharedPtr<Viewport> viewport(new Viewport(GameContext::Get().context_, GameContext::Get().rttScene_, GameContext::Get().rttScene_->GetChild("Camera")->GetComponent<Camera>()));
            renderer->SetNumViewports(numviewports+1);
            renderer->SetViewport(numviewports, viewport);
        }

        int border = 5;
        int w = GameContext::Get().screenwidth_/3;
        int h = GameContext::Get().screenheight_/3;

        Viewport* viewport = renderer->GetViewport(numviewports);
        viewport->SetRect(IntRect(GameContext::Get().screenwidth_ - w - border, GameContext::Get().screenheight_ - h - border,
                                  GameContext::Get().screenwidth_ - border, GameContext::Get().screenheight_ - border));
    }
    else
    {
        if (renderer->GetNumViewports() > numviewports)
            renderer->SetNumViewports(numviewports);
    }

    renderer->SetClearBack(numviewports > 1 && !GameContext::Get().gameConfig_.debugRttScene_);
}

void ViewManager::ResizeViewportRects(int numviewports)
{
    Graphics* graphics = GameContext::Get().context_->GetSubsystem<Graphics>();
    const int border = 2;

    switch (numviewports)
    {
    case 2 :
        viewportRects_[1][0] = IntRect(0, 0, graphics->GetWidth()/2 -border, graphics->GetHeight());
        viewportRects_[1][1] = IntRect(graphics->GetWidth()/2 +border, 0, graphics->GetWidth(), graphics->GetHeight());
        break;
    case 3 :
        viewportRects_[2][0] = IntRect(0, 0, graphics->GetWidth()/2 -border, graphics->GetHeight()/2 -border);
        viewportRects_[2][1] = IntRect(graphics->GetWidth()/2 +border, 0, graphics->GetWidth(), graphics->GetHeight()/2 -border);
        viewportRects_[2][2] = IntRect(graphics->GetWidth()/4, graphics->GetHeight()/2 +border, 3 * graphics->GetWidth() / 4, graphics->GetHeight());
        break;
    case 4 :
        viewportRects_[3][0] = IntRect(0, 0, graphics->GetWidth()/2 -border, graphics->GetHeight()/2 -border);
        viewportRects_[3][1] = IntRect(graphics->GetWidth()/2 +border, 0, graphics->GetWidth(), graphics->GetHeight()/2 -border);
        viewportRects_[3][2] = IntRect(0, graphics->GetHeight()/2 +border, graphics->GetWidth()/2 -border, graphics->GetHeight());
        viewportRects_[3][3] = IntRect(graphics->GetWidth()/2 +border, graphics->GetHeight()/2 +border, graphics->GetWidth(), graphics->GetHeight());
        break;
    default:
        viewportRects_[0][0] = IntRect(0, 0, graphics->GetWidth(), graphics->GetHeight());
        break;
    }
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

    if (node && node->GetVar(GOA::ISMOUNTEDON).GetUInt())
    {
        /// if it's a mounted entity use the mount node (the viewZ is setted by the parent node in GOC_Destroy::SetViewZ)
        Node* mountnode = GameContext::Get().rootScene_->GetNode(node->GetVar(GOA::ISMOUNTEDON).GetUInt());
        if (mountnode)
        {
            if (!GameContext::Get().LocalMode_)
            {
                /// don't use mountnode if it's two different clients
                unsigned mountclientid = mountnode->GetVar(GOA::CLIENTID).GetUInt();
                if (mountclientid && mountclientid != node->GetVar(GOA::CLIENTID).GetUInt())
                    mountnode = 0;
            }

            if (mountnode)
            {
                URHO3D_LOGINFOF("ViewManager() - SwitchToViewIndex : viewport=%d node=%s(%u) use mountnode=%s(%u) to switch to viewZ=%d viewZindex=%d !",
                        viewport, node ? node->GetName().CString() : 0, node ? node->GetID() : 0,
                        mountnode ? mountnode->GetName().CString() : 0, mountnode ? mountnode->GetID() : 0, viewZ, viewZindex);

                node = mountnode;
            }
        }
    }

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
        else if (controller)
        {
            viewport = GetControllerViewport(controller->GetThinker());
        }
    }

    viewportInfos_[viewport].lastViewZindex_ = viewportInfos_[viewport].viewZindex_;
    viewportInfos_[viewport].viewZindex_ = viewZindex;
    viewportInfos_[viewport].layerZindex_ = viewZindex == INNERVIEW_Index ? 2 : 4;

    URHO3D_LOGINFOF("ViewManager() - SwitchToViewIndex : viewport=%d node=%s(%u) switch to viewZ=%d viewZindex=%d !",
                    viewport, node ? node->GetName().CString() : 0, node ? node->GetID() : 0, viewZ, viewZindex);

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
    if (numViewports_ <= 1)
        return 0;

    for (int viewport=0; viewport < numViewports_; viewport++)
        if (viewportInfos_[viewport].camera_ == camera)
            return viewport;

    return 0;
}

int ViewManager::GetViewportAt(int screenx, int screeny)
{
    if (numViewports_ <= 1)
        return 0;

    IntVector2 screenpoint(screenx, screeny);
    IntRect* viewportrects = viewportRects_[numViewports_-1];

    for (int viewport=0; viewport < numViewports_; viewport++)
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

int ViewManager::GetControllerViewport(Actor* actor) const
{
    if (actor && numViewports_ > 1)
    {
        for (int viewport = 0; viewport < numViewports_; viewport++)
        {
            List<WeakPtr<Node> >::ConstIterator it = viewportInfos_[viewport].nodesOnFocus_.Find(WeakPtr<Node>(actor->GetAvatar()));
            if (it != viewportInfos_[viewport].nodesOnFocus_.End())
                return viewport;
        }
    }

    return 0;
}

unsigned ViewManager::GetControllerLightMask(Actor* actor) const
{
    if (!actor->IsMainController())
        return NOLIGHT_MASK;

    for (int viewport = 0; viewport < numViewports_; viewport++)
    {
        List<WeakPtr<Node> >::ConstIterator it = viewportInfos_[viewport].nodesOnFocus_.Find(WeakPtr<Node>(actor->GetAvatar()));
        if (it != viewportInfos_[viewport].nodesOnFocus_.End())
        {
            if (it == --viewportInfos_[viewport].nodesOnFocus_.End())
                return ((VIEWPORTSCROLLER_OUTSIDE_MASK << viewport) | (VIEWPORTSCROLLER_INSIDE_MASK << viewport));
            else
                return NOLIGHT_MASK;
        }
    }

    return NOLIGHT_MASK;
}

Node* ViewManager::GetCameraNode(int viewport) const
{
    return viewportInfos_[viewport].camera_ ? viewportInfos_[viewport].camera_->GetNode() : 0;
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

        vinfo.cameraFocusNode_->SetPosition2D(focus);
        vinfo.camera_->GetNode()->SetPosition2D(focus);
        vinfo.camera_->SetNearClip(-100.f);
        vinfo.camera_->SetFarClip(100.f);
        vinfo.camera_->SetFov(60.f);

        // SoundListener on camera node
        // TODO : add SoundListener on the main viewport
        if (!GetContext()->GetSubsystem<Audio>()->GetListener())
        {
            SoundListener* soundListener = vinfo.camera_->GetNode()->GetOrCreateComponent<SoundListener>(LOCAL);
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

    if (vinfo.nodesOnFocus_.Size() == 1 && GetCurrentViewZ(viewport) != viewZ)
        SwitchToViewZ(viewZ, 0, viewport);

    if (center)
    {
        vinfo.cameraFocusNode_->SetPosition2D(nodeToFocus->GetWorldPosition2D());
        vinfo.camera_->GetNode()->SetPosition2D(nodeToFocus->GetWorldPosition2D());

        UpdateFocus(viewport);
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
            vinfo.camera_->GetNode()->GetOrCreateComponent<UIC_MiniMap>()->SetFollowedNode(enable ? vinfo.nodesOnFocus_.Front() : vinfo.camera_->GetNode());
#endif

        if (vinfo.nodesOnFocus_.Empty())
            enable = false;

        vinfo.focusEnabled_ = enable;

        if (enable)
        {
            // Desactive the Camera Animations
            if (vinfo.camera_->GetNode()->GetObjectAnimation())
                vinfo.camera_->GetNode()->SetAnimationEnabled(false);
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
//                        vinfo.cameraFocus_.ToString().CString(), vinfo.cameraFocusNode_->GetName().CString(), vinfo.cameraFocusNode_->GetID(), vinfo.camera_->GetNode());
        vinfo.camera_->GetNode()->SetPosition2D(vinfo.cameraFocus_);
        return;
    }
#else
    /// LERP Move
    {
//        URHO3D_LOGINFOF("ViewManager() - MoveCamera LERP : cameraFocus_=%s cameraFocusNode_=%s(%u) cameraNode_=%u",
//                        vinfo.cameraFocus_.ToString().CString(), vinfo.cameraFocusNode_->GetName().CString(), vinfo.cameraFocusNode_->GetID(), vinfo.camera_->GetNode());

        vinfo.cameraMotion_ = vinfo.cameraFocus_ - vinfo.camera_->GetNode()->GetPosition2D();

        if (Abs(vinfo.cameraMotion_.x_) > 0.25f || Abs(vinfo.cameraMotion_.y_) > 0.25f)
            vinfo.camera_->GetNode()->Translate(vinfo.cameraMotion_ * timeStep);
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
                    // for Mounted node
                    if (node->GetVar(GOA::ISMOUNTEDON).GetUInt())
                        node = node->GetParent()->GetName() == MOUNTNODE ? node->GetParent()->GetParent() : node->GetParent();

                    position += node->GetWorldPosition2D();
                    velocity += node->GetVar(GOA::VELOCITY).GetVector2();

#ifdef CAMERAFOCUS_ADJUSTWALK
                    movestate = node->GetVar(GOA::MOVESTATE).GetUInt();

                    if ((movestate & (MV_WALK|MV_FLY|MV_INFALL) == MV_WALK) || (movestate & MV_TOUCHGROUND))
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
