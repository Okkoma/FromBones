#pragma once


#include <Urho3D/Core/Object.h>

#include "DefsViews.h"

namespace Urho3D
{
class Node;
class Scene;
class Camera;
}

/// ViewManager Event
URHO3D_EVENT(VIEWMANAGER_SWITCHVIEW, ViewManager_SwitchView)
{
    URHO3D_PARAM(VIEWPORT, Viewport);  // int
    URHO3D_PARAM(VIEWZINDEX, ViewZindex);  // int
    URHO3D_PARAM(VIEWZ, ViewZ);            // int
    URHO3D_PARAM(FORCEDMODE, ForcedMode); // bool
}

using namespace Urho3D;


struct ViewportInfo
{
    ViewportInfo() : focusEnabled_(false), cameraFocusZoom_(1.f), cameraYaw_(0.f), cameraPitch_(0.f), camMotionSpeed_(0.f), currentCamMotionSpeed_(0.f), viewZindex_(-1), lastViewZindex_(-1), layerZindex_(2) { }

    void Clear();

    bool focusEnabled_;
    float cameraFocusZoom_;
    float cameraYaw_;
    float cameraPitch_;
    float camMotionSpeed_;
    float currentCamMotionSpeed_;
    Vector2 cameraFocus_;
    Vector3 cameraMotion_;

    int viewZindex_, lastViewZindex_, layerZindex_;

    SharedPtr<Viewport> viewport_;
    WeakPtr<Camera> camera_;
    WeakPtr<Node> cameraFocusNode_;
    List<WeakPtr<Node> > nodesOnFocus_;
};

class Actor;

class FROMBONES_API ViewManager : public Object
{
    URHO3D_OBJECT(ViewManager, Object);

public:
    ViewManager(Context* context);
    ~ViewManager();

    static void RegisterObject(Context* context);
    static void RegisterLayer(int Z, unsigned layermask);
    static void RegisterViewId(int viewid, const String& name);
    static void RegisterSwitchView(int Z, const String& name, unsigned viewmask);
    static void RegisterFluidView(int Z);
    static void RegisterEffectView(int Z, unsigned viewmask);
    static void ResetDefault();
    static void ResizeViewportRects(int numviewports);

    static ViewManager* Get()
    {
        return viewManager_;
    }
    static unsigned GetNumLayersZ()
    {
        return layerZ_.Size();
    }
    static int GetNumViewZ()
    {
        return (int)viewZ_.Size();
    }
    static unsigned GetNumFluidViewZ()
    {
        return fluidZ_.Size();
    }
    static int GetViewZ(int viewZIndex)
    {
        return viewZ_[viewZIndex];
    }
    static int GetLayerZ(int index)
    {
        return layerZ_[index];
    }
    static int GetFluidZ(int index)
    {
        return fluidZ_[index];
    }
    static unsigned GetLayerMask(int viewZ)
    {
        HashMap<int, unsigned>::ConstIterator it = layerMask_.Find(viewZ);
        return it != layerMask_.End() ? it->second_ : DEFAULT_VIEWMASK;
    }
    static unsigned GetViewMask(int viewZ)
    {
        HashMap<int, unsigned>::ConstIterator it = viewMask_.Find(viewZ);
        return it != effectMask_.End() ? it->second_ : DEFAULT_VIEWMASK;
    }
    static unsigned GetEffectMask(int viewZ)
    {
        HashMap<int, unsigned>::ConstIterator it = effectMask_.Find(viewZ);
        return it != effectMask_.End() ? it->second_ : DEFAULT_VIEWMASK;
    }
    static int GetViewZIndex(int viewZ)
    {
        HashMap<int, unsigned>::ConstIterator it = viewZIndexes_.Find(viewZ);
        return it != viewZIndexes_.End() ? it->second_ : - 1;
    }
    static int GetLayerZIndex(int viewZ)
    {
        HashMap<int, unsigned>::ConstIterator it = layerZIndexes_.Find(viewZ);
        return it != layerZIndexes_.End() ? it->second_ : - 1;
    }
    static unsigned GetFluidZIndex(int viewZIndex)
    {
        return viewZIndex2fluidZIndex_[viewZIndex];
    }
    static const Vector<int>& GetFluidZ()
    {
        return fluidZ_;
    }
    static int GetNearLayerZ(int z, int dir);
    static int GetNearViewZ(int z, int dir=0);
    static const String& GetViewZName(unsigned viewZIndex)
    {
        return viewZNames_[viewZIndex];
    }
    static const String& GetViewName(int viewid)
    {
        return viewIdNames_[viewid%viewIdNames_.Size()];
    }

    /// Setters
    // Initializer
    void SetScene(Scene* scene, const Vector2& focus=Vector2::ZERO);
    void SetMiniMapEnable(bool enable);

    // Viewport
    void SetViewportLayout(int numviewports, bool reset=false);
    void ResizeViewports(int numviewports=-1);

    // Camera
    void SetCamera(const Vector2& focus = Vector2::ZERO, int viewport=-1);
    void AddFocus(Node* nodeToFocus=0, bool center=true, int viewport=0);
    void SetFocusEnable(bool enable, int viewport=-1);
    void ResetFocus(int viewport=-1);
    void MoveCamera(int viewport, const float& timeStep);
    void UpdateFocus(int viewport=-1, const float& timeStep=0.f);

    // ViewZ
    int SwitchToViewIndex(int viewZindex, Node* node, int viewport=0);
    void SwitchToViewZ(int viewZ, Node* node=0, int viewport=0);
    int SwitchNextZ(Node* node=0);
    int SwitchPreviousZ(Node* node=0);
    void HandleSwitchViewZ(StringHash eventType, VariantMap& eventData);

    /// Getters
    // Viewport
    unsigned GetNumViewports() const { return numViewports_; }
    int GetViewportAt(int screenx, int screeny);
    int GetViewport(Camera* camera) const;
    Rect GetViewRect(int viewport=0) const;
    const IntRect& GetViewportRect(int viewport=0) const;
    int GetControllerViewport(Actor* actor) const;
    unsigned GetControllerLightMask(Actor* actor) const;

    // Camera
    Node* GetCameraNode(int viewport=0) const;
    Camera* GetCamera(int viewport=0) const
    {
        return viewportInfos_[viewport].camera_;
    }
    Node* GetCameraFocus(int viewport=0) const
    {
        return viewportInfos_[viewport].cameraFocusNode_;
    }
    bool GetFocusEnable(int viewport=0) const
    {
        return viewportInfos_[viewport].focusEnabled_;
    }
    const List<WeakPtr<Node> >& GetNodesOnFocus(int viewport=0) const
    {
        return viewportInfos_[viewport].nodesOnFocus_;
    }

    // ViewZ
    int GetCurrentLayerZIndex(int viewport=0) const
    {
        return viewportInfos_[viewport].layerZindex_;
    }
    int GetCurrentViewZIndex(int viewport=0) const
    {
        return viewportInfos_[viewport].viewZindex_;
    }
    int GetLastViewZIndex(int viewport=0) const
    {
        return viewportInfos_[viewport].lastViewZindex_;
    }
    int GetCurrentViewZ(int viewport=0) const
    {
        return viewportInfos_[viewport].viewZindex_ == -1 ? -1 : viewZ_[viewportInfos_[viewport].viewZindex_];
    }

    void Dump() const;

    static unsigned INNERVIEW_Index;
    static unsigned INNERLAYER_Index;
    static unsigned FRONTVIEW_Index;
    static unsigned MAX_PURELAYERS;
    static unsigned FLUIDINNERVIEW_Index;
    static unsigned FLUIDFRONTVIEW_Index;

    static IntRect viewportRects_[MAX_VIEWPORTS][MAX_VIEWPORTS];

private:
    Scene* scene_;

    unsigned numViewports_;
    ViewportInfo viewportInfos_[MAX_VIEWPORTS];

    static Vector<int> layerZ_;
    static Vector<int> viewZ_;
    static Vector<int> fluidZ_;
    static Vector<unsigned> viewZIndex2fluidZIndex_;
    static HashMap<int, unsigned> layerZIndexes_;
    static HashMap<int, unsigned> viewZIndexes_;
    static Vector<String> viewZNames_;
    static Vector<String> viewIdNames_;

    static HashMap<int, unsigned> layerMask_;
    static HashMap<int, unsigned> viewMask_;
    static HashMap<int, unsigned> effectMask_;

    static ViewManager* viewManager_;
};
