#pragma once

#include <Urho3D/Core/Profiler.h>

#include <Urho3D/Scene/Component.h>

#include <Urho3D/Urho2D/Drawable2D.h>

//#define USE_WATERLINE_COLLISIONCHAIN


#ifdef USE_WATERLINE_COLLISIONCHAIN
#include <Urho3D/Urho2D/CollisionChain2D.h>
#else
#include <Urho3D/Urho2D/CollisionBox2D.h>
#endif

namespace Urho3D
{
class Node;
class Drawable2D;
#ifdef USE_WATERLINE_COLLISIONCHAIN
class CollisionChain2D;
#else
class CollisionBox2D;
#endif
}

using namespace Urho3D;

#include "GameOptionsTest.h"

#include "DefsChunks.h"
#include "DefsFluids.h"

struct FluidCell;

struct WaterSurface
{
    void Set(MapBase* map, FluidCell* cell, unsigned addr, float bottomy, Vertex2D& vertex0, Vertex2D& vertex1, bool keepVertexRef);

    MapBase* map_;
    unsigned addr_;
    FluidCell* cell_;
    WaterSurface* next_;
    Vertex2D* vertex0_;
    Vertex2D* vertex1_;

    float bottomy_;
    float x0_, y0_;
    float x1_, y1_;

#ifdef FLUID_RENDER_COLORDEBUG
    unsigned debugColor_;
#endif
};

struct WaterDeformation
{
    float yDelta_;
    float acceleration_;
    float velocity_;
};

struct WaterDeformationPoint
{
    WaterDeformationPoint();

    void Reset();
    void AddDeformation(const float x, const float y, const float velocity);
    void Update();

    Vector3 point_;

    // Physic Wave Simulation Datas
    PODVector<WaterDeformation> deformations_;

    // Deformation constants
    static const unsigned smoothPasses_;
    static const unsigned divisions_;
    static const float spring_;
    static const float damping_;
    static const float spread_;

    // Deltas
    static PODVector<float> leftDeltas_, rightDeltas_;
};

struct WaterLine
{
    WaterLine();

    void Clear();

    void ExpandFrom(WaterSurface* surface, Vector<WaterSurface>& surfaces);
    void Update(SourceBatch2D& batch, SourceBatch2D& batch2, const float zf);

    bool expanded_;

    // Linked surfaces from Left to Right
    WaterSurface* surface_;
    WaterSurface* surfaceEnd_;
    unsigned numSurfaces_;

    Rect boundingRect_;

#ifdef USE_WATERLINE_COLLISIONCHAIN
    WeakPtr<CollisionChain2D> shape_;
#else
    WeakPtr<CollisionBox2D> shape_;
#endif

    PODVector<float> deformations_;

    static const float lineThickness_;
    static const float lineThreshold_;
    static const float deformationThreshold_;
};

struct WaterLayerData
{
    WaterLayerData();

    void Clear();

    bool UpdateSimulation();

    void UpdateTiledBatch(const ViewportRenderData& viewportdata, FluidDatas& fluiddata);
    void UpdateBatches();

    int viewport_;

    Vector<ViewportRenderData* > viewportdatas_;
    Vector<WaterSurface> waterSurfaces_;
    Vector<WaterLine> waterLines_;

    SourceBatch2D watertilesBackBatch_, watertilesFrontBatch_, waterlinesBatch_;
};

class WaterLayer : public Drawable2D
{
    friend struct WaterLine;

public:
    static void RegisterObject(Context* context);
    static WaterLayer* Get()
    {
        return waterLayer_;
    }

    WaterLayer(Context* context);
    ~WaterLayer();

    void Clear();

    void SetViewportData(bool enable, ViewportRenderData* viewportdata);

    void AddSplashAt(const float x, const float y, const float velocity);

    /// Return all source batches To Renderer (called by Renderer2D).
    virtual const Vector<SourceBatch2D* >& GetSourceBatchesToRender(Camera* camera);

    Vector<WaterSurface>& GetWaterSurfaces(int viewport)
    {
        return layerDatas_[viewport].waterSurfaces_;
    }
    Vector<WaterLine>& GetWaterLines(int viewport)
    {
        return layerDatas_[viewport].waterLines_;
    }

    virtual void DrawDebugGeometry(DebugRenderer* debug, bool depthTest);

    virtual void OnSetEnabled();

protected:
    virtual void OnWorldBoundingBoxUpdate();

    virtual void OnDrawOrderChanged() { }
    virtual void UpdateSourceBatches() { }

private:
    void Init();

    virtual void OnNodeSet(Node* node);

    void UpdateBatches(int viewport);

    void HandleUpdateFluids(StringHash eventType, VariantMap& eventData);
    void HandleBeginFluidContact(StringHash eventType, VariantMap& eventData);
    void HandleEndFluidContact(StringHash eventType, VariantMap& eventData);

    void DrawDebugWaterQuads(DebugRenderer* debug, const FluidDatas& fluiddata, const ViewportRenderData& viewportdata);

    // for optimization, think to abstract the viewport because it can be render for multiple viewports if there cameras are closed.
    // Waterlines with deformation waves
    int numMapBatchesUpdates_[MAX_VIEWPORTS];
    Vector<WaterLayerData> layerDatas_;

    Vector<WaterDeformationPoint> waterDeformationPoints_;

    bool fluidUpdated_;
    float fluidTime_;

    static WaterLayer* waterLayer_;
};
