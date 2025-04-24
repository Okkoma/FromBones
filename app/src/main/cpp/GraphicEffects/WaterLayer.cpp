#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/DebugRenderer.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Camera.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Urho2D/Renderer2D.h>
#include <Urho3D/Urho2D/Drawable2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>

#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameOptions.h"


#include "MapWorld.h"
#include "ViewManager.h"
#include "Map.h"
#include "MapSimulatorLiquid.h"

#include "DefsFluids.h"

#include "WaterLayer.h"


//#define WATERLAYER_DEBUG


const unsigned MAX_DEFORMATIONPOINTS = 50;
const unsigned MAX_NUMTILEBYDEFORMATIONPOINT = 9;

// WaterSurface

void WaterSurface::Set(MapBase* map, FluidCell* cell, unsigned addr, float bottomy, Vertex2D& vertex0, Vertex2D& vertex1, bool keepVertexRef)
{
    map_ = map;
    cell_ = cell;
    addr_ = addr;
    next_ = 0;

    if (keepVertexRef)
    {
        vertex0_ = &vertex0;
        vertex1_ = &vertex1;
    }
    else
    {
        vertex0_ = 0;
    }

    bottomy_ = bottomy;
    x0_ = vertex0.position_.x_;
    y0_ = vertex0.position_.y_;
    x1_ = vertex1.position_.x_;
    y1_ = vertex1.position_.y_;
}


// WaterLine

const float WaterLine::lineThickness_ = 2.5f * PIXEL_SIZE;
const float WaterLine::lineThreshold_ = 0.35f;
const float WaterLine::deformationThreshold_ = 1.f;

WaterLine::WaterLine()
{
    Clear();
    deformations_.Reserve(FluidDatas::width_ * WaterDeformationPoint::divisions_ + 2);
}

void WaterLine::Clear()
{
    expanded_ = false;
    surface_ = 0;
    numSurfaces_ = 0;
    deformations_.Clear();
}

bool CheckConnectedNeighborsAt(WaterSurface*& surface, Vector<WaterSurface>& surfaces, int offsetx, int offsety)
{
    /// TODO : resolve cell addr with neighbor maps
    unsigned neighboraddr = surface->addr_ + offsety * FluidDatas::width_;
    if (!offsety)
        neighboraddr += offsetx;

    WaterSurface* neighbor = 0;

    // find the neighbor in surfaces bank
    for (int i = 0; i < surfaces.Size(); i++)
        if (surfaces[i].addr_ == neighboraddr)
            neighbor = &surfaces[i];

    if (!neighbor || neighbor->cell_->drawn_)
    {
#ifdef WATERLAYER_DEBUG
        if (neighbor)
            URHO3D_LOGDEBUGF("... CheckConnectedNeighborsAt offset %d %d ... neighbor already drawn !", offsetx, offsety);
#endif
        return false;
    }

    if (offsety && (neighbor->next_ == surface || surface->next_ == neighbor))
        return false;

    // To the Left
    if (offsetx < 0)
    {
        bool connected = Abs(surface->y1_ - neighbor->y0_) < WaterLine::lineThreshold_;

        // set the next surface (to the left)
        neighbor->next_ = connected ? surface : 0;

        // travel to the neighbor surface
        if (connected)
            surface = neighbor;

#ifdef WATERLAYER_DEBUG
        URHO3D_LOGDEBUGF("... CheckConnectedNeighborsAt offset %d %d ... neighbor connected=%u dx=%F dy=%F !",
                         offsetx, offsety, connected, surface->x1_ - neighbor->x1_, surface->y1_ - neighbor->y0_);
#endif
        return connected;
    }
    // To the Right
    else
    {
        bool connected = Abs(neighbor->x0_ - surface->x1_) < WaterLine::lineThreshold_ &&
                         Abs(neighbor->y0_ - surface->y1_) < WaterLine::lineThreshold_;

        // set the next surface (to the right)
        surface->next_ = connected ? neighbor : 0;

        // travel to the neighbor surface
        if (connected)
        {
#ifdef WATERLAYER_DEBUG
            URHO3D_LOGDEBUGF("... CheckConnectedNeighborsAt offset %d %d ... neighbor connected !", offsetx, offsety);
#endif
            surface = neighbor;
        }

        return connected;
    }
#ifdef WATERLAYER_DEBUG
    URHO3D_LOGDEBUGF("... CheckConnectedNeighborsAt offset %d %d ... neighbor not closed !", offsetx, offsety);
#endif
    return false;
}

void WaterLine::ExpandFrom(WaterSurface* surface, Vector<WaterSurface>& surfaces)
{
    if (!surface)
        return;

    surface->next_ = 0;
    surface_ = surface;
#ifdef WATERLAYER_DEBUG
    URHO3D_LOGDEBUGF("WaterLine() - ExpandFrom : this=%u at watersurface=%u ...", this, surface->addr_);
#endif

    // First : Expansion to Right
    for (;;)
    {
        // Check Top
        if (CheckConnectedNeighborsAt(surface, surfaces, 1, -1))
            continue;
        // Check Bottom
        if (CheckConnectedNeighborsAt(surface, surfaces, 1, 1))
            continue;
        // Check Right
        if (CheckConnectedNeighborsAt(surface, surfaces, 1, 0))
            continue;
        break;
    }

    // Second : Expansion To Left
    surface = surface_;
    for (;;)
    {
        // Check Top
        if (CheckConnectedNeighborsAt(surface, surfaces, -1, -1))
            continue;
        // Check Bottom
        if (CheckConnectedNeighborsAt(surface, surfaces, -1, 1))
            continue;
        // Check Left
        if (CheckConnectedNeighborsAt(surface, surfaces, -1, 0))
            continue;
        break;
    }

    expanded_ = true;

    // Update the start Surface
    surface_ = surface;

    // Calculate the num of surfaces
    numSurfaces_ = 0;
    for (;;)
    {
        numSurfaces_++;

        // mark traveled
        surface->cell_->drawn_ = true;

        if (!surface->next_)
            break;

        surface = surface->next_;
    }
#ifdef WATERLAYER_DEBUG
    URHO3D_LOGDEBUGF("WaterLine() - ExpandFrom : this=%u at watersurface=%u ... WaterSurfaces Connected = %u !", this, surface->addr_, numSurfaces_);
#endif
    // Update the end Surface
    surfaceEnd_ = surface;
}

void WaterLine::Update(SourceBatch2D& batch, SourceBatch2D& batch2, const float zf)
{
    const Vector2 tileSize = MapInfo::info.mTileHalfSize_ * 2.f;
    const float divisionWidth = tileSize.x_ / WaterDeformationPoint::divisions_;
    const float deformationHalfWidth = tileSize.x_ * MAX_NUMTILEBYDEFORMATIONPOINT * 0.5f;
    const int deformationLength = MAX_NUMTILEBYDEFORMATIONPOINT * WaterDeformationPoint::divisions_;

    Rect lastBoundingRect = boundingRect_;

    unsigned numSurfacesWithTile = 0;

    // Link WaterSurfaces Vertices
    {
        WaterSurface* surface = surface_;
        while (surface != surfaceEnd_)
        {
            surface->x1_ = surface->next_->x0_;
            surface->y1_ = surface->next_->y0_;
            if (!surface->vertex0_)
                numSurfacesWithTile++;
            surface = surface->next_;
        }
        if (!surfaceEnd_->vertex0_)
            numSurfacesWithTile++;

        // Smooth a little
        surface = surface_;
        while (surface != surfaceEnd_)
        {
            //surface->x1_ = surface->next_->x0_ = (surface->x0_ + surface->next_->x1_ + surface->x1_ + surface->next_->x0_) * 0.25f;
            //surface->y1_ = surface->next_->y0_ = (surface->y0_ + surface->next_->y1_ + surface->y1_ + surface->next_->y0_) * 0.25f;
            if (surface->vertex0_)
            {
                surface->vertex1_->position_.x_ = surface->x1_;
                surface->vertex1_->position_.y_ = surface->y1_;
            }
            surface = surface->next_;
            if (surface->vertex0_)
            {
                surface->vertex0_->position_.x_ = surface->x0_;
                surface->vertex0_->position_.y_ = surface->y0_;
            }
        }
    }

    // Add Point Deformations
    if (WaterLayer::Get()->waterDeformationPoints_.Size())
    {
        Vector<WaterDeformationPoint>& points = WaterLayer::Get()->waterDeformationPoints_;

        const Vector2 vmin(Min(surface_->x0_, surfaceEnd_->x0_), Min(surface_->y0_, surfaceEnd_->y0_));
        const Vector2 vmax(Max(surface_->x1_, surfaceEnd_->x1_), Max(surface_->y1_, surfaceEnd_->y1_));
        const int numDivisions = Round((vmax.x_ - vmin.x_) / divisionWidth);
        const float centeredY = (vmin.y_ + vmax.y_) * 0.5f;

        // Reset Deformations
        deformations_.Resize(numSurfaces_ * WaterDeformationPoint::divisions_ + 2);
        memset(&deformations_[0], 0, sizeof(float) * deformations_.Size());

        float offset;
        int start1, start2, length;

        for (unsigned i = 0; i < points.Size(); i++)
        {
            Vector3& point = points[i].point_;

            // skip the point if no more deformation
            if (point.z_ == 0.f)
                continue;

            // check if the point is near of the waterline in vertical
//            if (point.y_ - deformationThreshold_ < vmin.y_ && point.y_ + deformationThreshold_ > vmax.y_)
            if (point.y_ + deformationThreshold_*0.5f >= vmin.y_ && point.y_ - deformationThreshold_*0.5f <= vmax.y_)
            {
                // check if the deformation generated by this point is inside or intersect the waterline in X
                if (point.x_ + deformationHalfWidth >= vmin.x_ && point.x_ - deformationHalfWidth <= vmax.x_)
                {
                    // update the y of the deformation point : this allows that the deformation points will always be on the waterline
                    point.y_ = centeredY;

                    offset = (point.x_ - vmin.x_ - deformationHalfWidth) / divisionWidth;
                    start1 = (offset >= 0.f ? Min(offset, deformations_.Size()) : 0);
                    start2 = (offset >= 0.f ? 0 : Min(-offset, deformationLength));
                    length = Min(deformationLength - start2, deformations_.Size() - start1);
#ifdef WATERLAYER_DEBUG
                    URHO3D_LOGDEBUGF("WaterLine() - Update : this=%u add deformation point=%F,%F,%F tileWidth=%F (offset=%F start1=%u length=%u start2=%u) !",
                                     this, point.x_, point.y_, point.z_, tileSize.x_, offset, start1, length, start2);
#endif
                    if (length < 1)
                        continue;

                    // add the deformations
                    const PODVector<WaterDeformation>& deformations = points[i].deformations_;
                    for (unsigned j=0; j < length; j++)
                        deformations_[start1 + j] += deformations[start2 + j].yDelta_;
                }
            }
        }
    }

    // Define BoundingRect
    boundingRect_.Define(surface_->x0_, surface_->y0_, surfaceEnd_->x1_, surfaceEnd_->y1_);

    // Add WaterLine to Batch
    {
        // Resize the batch vertices
        Vector<Vertex2D>& vertices = batch.vertices_;
        unsigned addr = vertices.Size();
        vertices.Resize(addr + numSurfaces_ * WaterDeformationPoint::divisions_ * 4);

        Vector<Vertex2D>& vertices2 = batch2.vertices_;
        unsigned addr2 = vertices2.Size();
//        vertices2.Resize(addr2 + numSurfaces_ * WaterDeformationPoint::divisions_ * 4);
        vertices2.Resize(addr2 + numSurfacesWithTile * WaterDeformationPoint::divisions_ * 4);

        const unsigned color = Color::WHITE.ToUInt();
        const unsigned tilecolor = Color(0.5f, 0.75f, 1.f, 0.1f).ToUInt();
        const Rect fullWaterRect(0.05f, 1.f, 0.95f, 0.2f);

        const Vector2& mapPosition = surface_->map_->GetRootNode()->GetWorldPosition2D();

        // Travel the surfaces of the waterline
        WaterSurface* surface = surface_;
        unsigned offset = 1;

        for (;;)
        {
            if (addr >= vertices.Size() || !surface)
                break;

            const float xStart = surface->x0_;
            const float yStart = surface->y0_;

            const float surfaceWidth = surface->x1_ - xStart;
            const unsigned numDivisions = Round(surfaceWidth / divisionWidth);
            const float deltay = (surface->y1_ - yStart) / numDivisions;

            const bool bottomclamp = !surface->cell_->Bottom || surface->cell_->Bottom->type_ == BLOCK;
            const bool topclamp = !surface->cell_->Top || surface->cell_->Top->type_ == BLOCK;

            const float clampedBottomY = bottomclamp ? mapPosition.y_ + (MapInfo::info.height_ - surface->addr_ / FluidDatas::width_ - 1) * tileSize.y_ : 0.f;
            const float clampedTopY = topclamp ? mapPosition.y_ + (MapInfo::info.height_ - surface->addr_ / FluidDatas::width_) * tileSize.y_  : 0.f;

            // Set the vertices for each division of the surface
            for (unsigned d = 0; d < numDivisions && addr < vertices.Size(); d++, addr+=4)
            {
                /*
                    V1---------V2
                    |         / |
                    |       /   |
                    |     /     |
                    |   /       |
                    | /         |
                    V0---------V3
                */

                Vertex2D& vertex0 = vertices[addr];
                Vertex2D& vertex1 = vertices[addr+1];
                Vertex2D& vertex2 = vertices[addr+2];
                Vertex2D& vertex3 = vertices[addr+3];

                vertex0.position_.x_ = vertex1.position_.x_ = xStart + d * divisionWidth;
                vertex2.position_.x_ = vertex3.position_.x_ = d+1 == numDivisions ? surface->x1_ : xStart + (d+1) * divisionWidth;
                vertex1.position_.y_ = yStart + d * deltay;
                vertex2.position_.y_ = yStart + (d + 1) * deltay;

                const float& deform1 = deformations_[offset + d];
                const float& deform2 = deformations_[offset + d + 1];

                bool clamped = false;

                // vertice 1 : check if bottom clamped
                if (bottomclamp && vertex1.position_.y_ + deform1 < clampedBottomY)
                {
                    vertex1.position_.y_ = clampedBottomY;
                    clamped = true;
                }
                // vertice 1 : check if top clamped
                if (!clamped && topclamp && vertex1.position_.y_ > clampedTopY)
                {
                    vertex1.position_.y_ = clampedTopY;
                    clamped = true;
                }
                // vertice 1 : check if clamp by maximal deformation
                if (!clamped && Abs(deform1) > tileSize.y_)
                {
                    vertex1.position_.y_ += Sign(deform1) * tileSize.y_;
                    clamped = true;
                }
                // vertice 1 : no clamp
                if (!clamped)
                {
                    vertex1.position_.y_ += deform1;
                }

                clamped = false;

                // vertice 2 : check if bottom clamped
                if (bottomclamp && vertex2.position_.y_ + deform2 < clampedBottomY)
                {
                    vertex2.position_.y_ = clampedBottomY;
                    clamped = true;
                }
                // vertice 2 : check if top clamped
                if (!clamped && topclamp && vertex2.position_.y_ > clampedTopY)
                {
                    vertex2.position_.y_ = clampedTopY;
                    clamped = true;
                }
                // vertice 2 : check if clamp by maximal deformation
                if (!clamped && Abs(deform2) > tileSize.y_)
                {
                    vertex2.position_.y_ += Sign(deform2) * tileSize.y_;
                    clamped = true;
                }
                if (!clamped)
                {
                    vertex2.position_.y_ += deform2;
                }

                vertex0.position_.y_ = vertex1.position_.y_ - lineThickness_;
                vertex3.position_.y_ = vertex2.position_.y_ - lineThickness_;

#ifdef URHO3D_VULKAN
                vertex1.z_ = vertex2.z_ = vertex3.z_ = vertex0.z_ = zf;
#else
                vertex1.position_.z_ = vertex2.position_.z_ = vertex3.position_.z_ = vertex0.position_.z_ = zf;
#endif

                vertex1.color_ = vertex2.color_ = vertex3.color_ = vertex0.color_ = color;
            }

            // If need, draw the tile under the surface
            if (surface->vertex0_ == 0)
            {
                /*
                    | /         |
                    V0---------V3
                    V21---------V22
                    |         / |
                    |       /   |
                    |     /     |
                    |   /       |
                    | /         |
                    V20---------V23
                */

                unsigned prevaddr = addr - numDivisions*4;

                for (unsigned d=0; d < numDivisions && prevaddr < vertices.Size() && addr2 < vertices2.Size(); d++, prevaddr+=4, addr2+=4)
                {
                    const Vertex2D& vertex0 = vertices[prevaddr];
                    const Vertex2D& vertex3 = vertices[prevaddr+3];

                    Vertex2D& vertex20 = vertices2[addr2];
                    Vertex2D& vertex21 = vertices2[addr2+1];
                    Vertex2D& vertex22 = vertices2[addr2+2];
                    Vertex2D& vertex23 = vertices2[addr2+3];

                    vertex20.position_.x_ = vertex21.position_.x_ = vertex0.position_.x_;
                    vertex22.position_.x_ = vertex23.position_.x_ = vertex3.position_.x_;

                    vertex20.position_.y_ = vertex23.position_.y_ = surface->bottomy_;
                    vertex21.position_.y_ = vertex0.position_.y_;
                    vertex22.position_.y_ = vertex3.position_.y_;

#ifdef URHO3D_VULKAN
                    vertex21.z_ = vertex22.z_ = vertex23.z_ = vertex20.z_ = zf;
#else
                    vertex21.position_.z_ = vertex22.position_.z_ = vertex23.position_.z_ = vertex20.position_.z_ = zf;
#endif

                    vertex20.uv_  = fullWaterRect.min_;
                    vertex21.uv_  = Vector2(fullWaterRect.min_.x_, fullWaterRect.max_.y_);
                    vertex22.uv_  = fullWaterRect.max_;
                    vertex23.uv_  = Vector2(fullWaterRect.max_.x_, fullWaterRect.min_.y_);

#ifdef FLUID_RENDER_COLORDEBUG
                    vertex21.color_ = vertex22.color_ = vertex23.color_ = vertex20.color_ = surface->debugColor_;
#else
                    vertex21.color_ = vertex22.color_ = vertex23.color_ = vertex20.color_ = tilecolor;
#endif
                }
            }

            // next surface
            offset += numDivisions;
            surface = surface->next_;
        }

        vertices.Resize(addr);
        vertices2.Resize(addr2);
    }

    // Update the CollisionBox
    if (shape_ && boundingRect_ != lastBoundingRect)
    {
        // TODO : proper scaling
        const Vector2 wscale(0.25f, 0.25f);

        const Vector2 wposition = shape_->GetNode()->GetWorldPosition2D();

#ifdef USE_WATERLINE_COLLISIONCHAIN
        PODVector<Vector2> vertices(4);
        vertices[0] = (boundingRect_.min_ - wposition) * wscale;
        vertices[1] = (Vector2(boundingRect_.min_.x_, boundingRect_.max_.y_) - wposition) * wscale;
        vertices[2] = (boundingRect_.max_ - wposition) * wscale;
        vertices[3] = (Vector2(boundingRect_.max_.x_, boundingRect_.min_.y_) - wposition) * wscale;
        shape_->SetVertices(vertices);
#else
        const Vector2 wsize((boundingRect_.max_.x_ - boundingRect_.min_.x_) * wscale.x_, (boundingRect_.max_.y_ - boundingRect_.min_.y_) * wscale.y_);
        const float length = wsize.Length();

        Vector2 wcenter((boundingRect_.Center() - wposition) * wscale);

        // the following method doesn't destroy fixture, just modify the shape.
        // set the angle of the box with the cosinus and the sinus
        shape_->UpdateBox(wcenter, Vector2(wsize.x_, WaterLine::lineThickness_ * wscale.y_), wsize.x_ / length, wsize.y_ / length);
#endif
    }
}

// WaterDeformationPoint
const unsigned WaterDeformationPoint::smoothPasses_ = 6;
const unsigned WaterDeformationPoint::divisions_    = 10;

const float WaterDeformationPoint::spring_          = 0.1f;
const float WaterDeformationPoint::damping_         = 0.2f;
const float WaterDeformationPoint::spread_          = 0.1f;

PODVector<float> WaterDeformationPoint::leftDeltas_;
PODVector<float> WaterDeformationPoint::rightDeltas_;


WaterDeformationPoint::WaterDeformationPoint()
{
    const unsigned size = MAX_NUMTILEBYDEFORMATIONPOINT * divisions_;

    if (!leftDeltas_.Size())
    {
        leftDeltas_.Resize(size);
        rightDeltas_.Resize(size);
    }

    deformations_.Resize(size);

    Reset();
}

void WaterDeformationPoint::Reset()
{
    point_.z_ = 0.f;
    memset(&deformations_[0], 0, sizeof(WaterDeformation) * deformations_.Size());
}

void WaterDeformationPoint::AddDeformation(const float x, const float y, const float velocity)
{
    point_.x_ = x;
    point_.y_ = y;
    point_.z_ = velocity;

    deformations_[deformations_.Size()/2].velocity_ += velocity;
}

void WaterDeformationPoint::Update()
{
    if (point_.z_ == 0.f)
        return;

    // Skip the extremities
    const unsigned start = 1;
    const unsigned end = deformations_.Size() - 1;

    // Update physics
    for (unsigned i = start; i < end; i++)
    {
        WaterDeformation& deformation = deformations_[i];
        deformation.acceleration_ = -(spring_ * deformation.yDelta_ + deformation.velocity_ * damping_);
        deformation.yDelta_   += deformation.velocity_;
        deformation.velocity_ += deformation.acceleration_;
    }

    // Smooth the heights
    for (unsigned i = 0; i < smoothPasses_; i++)
    {
        for (int j = start; j < end; j++)
        {
            // Spring Euler Method
            leftDeltas_[j]  = spread_ * (deformations_[j].yDelta_ - deformations_[j - 1].yDelta_);
            rightDeltas_[j] = spread_ * (deformations_[j].yDelta_ - deformations_[j + 1].yDelta_);
            deformations_[j-1].velocity_ += leftDeltas_[j];
            deformations_[j+1].velocity_ += rightDeltas_[j];
        }

        if (end - start > 2)
        {
            for (int j=start+1; j < end-1; j++)
            {
                deformations_[j-1].yDelta_ += leftDeltas_[j];
                deformations_[j+1].yDelta_ += rightDeltas_[j];
            }
        }
    }

    point_.z_ -= point_.z_ * 0.05f;

    if (Abs(point_.z_) <= PIXEL_SIZE)
    {
//        URHO3D_LOGDEBUGF("WaterDeformationPoint() - Update : point=%F,%F Reset !", point_.x_, point_.y_);
        Reset();
    }
}


// WaterLayerData

WaterLayerData::WaterLayerData()
{
    waterSurfaces_.Reserve(MapInfo::info.width_ * MapInfo::info.height_ / 4);
    waterLines_.Reserve(MapInfo::info.height_);

    // Set Water Batch Materials
    watertilesFrontBatch_.material_ = World2DInfo::WATERMATERIAL_REFRACT;
    watertilesBackBatch_.material_  = World2DInfo::WATERMATERIAL_TILE;
    waterlinesBatch_.material_      = World2DInfo::WATERMATERIAL_LINE;
}

void WaterLayerData::Clear(int viewZ)
{
    waterSurfaces_.Clear();
    for (int i = 0; i < waterLines_.Size(); i++)
        waterLines_[i].Clear();

    if (viewZ)
    {
        int draworder = (viewZ + LAYER_FLUID) << 20;
        if (watertilesFrontBatch_.drawOrder_ != draworder)
        {
            watertilesFrontBatch_.drawOrder_ = (viewZ + LAYER_FLUID) << 20;
            watertilesBackBatch_.drawOrder_  = BACKWATER_2 << 20;//((viewZ == FRONTVIEW ? INNERVIEW : BACKGROUND) + LAYER_FLUID) << 20;
            waterlinesBatch_.drawOrder_      = watertilesFrontBatch_.drawOrder_ - 1;
        }
    }

    watertilesFrontBatch_.vertices_.Clear();
    watertilesBackBatch_.vertices_.Clear();
    waterlinesBatch_.vertices_.Clear();
}

bool WaterLayerData::UpdateSimulation()
{
    bool updated = false;

    for (unsigned i = 0; i < mapdatas_.Size(); i++)
    {
        ViewportRenderData* mapdata = mapdatas_[i];
        ObjectFeatured* featureData = mapdata->objectTiled_->GetObjectFeatured();
        int numFluidViews = featureData->fluidView_.Size();
        if (!numFluidViews)
            continue;

        bool mapSimulUpdated = false;
        for (int i = 0; i < numFluidViews; i++)
        {
            // Update Fluid Simulation
            if (MapSimulatorLiquid::Get()->Update(featureData->fluidView_[i]))
                mapSimulUpdated = true;
        }

        updated |= mapSimulUpdated;
    }
//    URHO3D_LOGINFOF("WaterLayerData() - UpdateSimulation ... viewport=%d updated=%s ... OK !", viewport_, updated ? "true" : "false");

    if (updated)
        batchesDirty_ = true;

    return updated;
}

// Patterned Fluid Batches
void WaterLayerData::UpdateTiledBatch(int viewZ, const ViewportRenderData& mapdata, FluidDatas& fluiddata)
{
    const Matrix2x3& worldTransform2D = mapdata.objectTiled_->GetNode()->GetWorldTransform2D();

    const IntRect& rect = mapdata.chunkGroup_.rect_;

    const Rect fullWaterRect(0.05f, 1.f, 0.95f, 0.2f);

    const unsigned width = MapInfo::info.width_;
    const unsigned height = MapInfo::info.height_;
    const float twidth  = MapInfo::info.tileWidth_;
    const float theight = MapInfo::info.tileHeight_;
    const float halfdx = twidth/2;
    const float halfdy = theight/2;

    FluidMap& fluidmap = fluiddata.fluidmap_;

    const float zf = viewZ == fluiddata.viewZ_ || fluiddata.viewZ_ == INNERVIEW ? 1.f - PIXEL_SIZE * (fluiddata.viewZ_ + LAYER_FLUID) : 1.f;

//    const float zf = viewZ != fluiddata.viewZ_ && viewZ == INNERVIEW ? 1.f : 1.f - PIXEL_SIZE * (fluiddata.viewZ_ + LAYER_FLUID);

    bool checkDepth = viewZ == FRONTVIEW && viewZ != fluiddata.viewZ_;

    /*
    V1---------V2
    |         / |
    |       /   |
    |     /     |
    |   /       |
    | /         |
    V0---------V3
    */

    SourceBatch2D& batch = viewZ == fluiddata.viewZ_ ? watertilesFrontBatch_ : watertilesBackBatch_;
    const float alpha = 1.f;

    Vector<Vertex2D>& vertices = batch.vertices_;
    Vertex2D vertex[16];

    float xv[16];
    float yv[16];
    int numQuads;
    unsigned surfaceVertexIndex[2];
/*
#ifdef URHO3D_VULKAN
    if (viewZ != fluiddata.viewZ_)
    {
        // initialize Texture Unit=1 for WaterTiles (ShaderGrounds without BackShape)
        unsigned texmode = 0;
        Drawable2D::SetTextureMode(TXM_UNIT, 0, texmode);
        for (unsigned i=0; i < 16; i++)
            vertex[i].texmode_ = texmode;
    }
#endif
*/
    const unsigned coloruint = Color(1.f, 1.f, 1.f, alpha).ToUInt();

    for (int i = 0; i < 4; i++)
    {
        vertex[i].uv_    = fullWaterRect.min_;
        vertex[i+1].uv_  = Vector2(fullWaterRect.min_.x_, fullWaterRect.max_.y_);
        vertex[i+2].uv_  = fullWaterRect.max_;
        vertex[i+3].uv_  = Vector2(fullWaterRect.max_.x_, fullWaterRect.min_.y_);
    }
    for (int i = 0; i < 16; i++)
    {
        vertex[i].color_ = coloruint;
    }

#ifdef FLUID_RENDER_COLORDEBUG
    Color leftColor, rightColor;
    leftColor.a_ = rightColor.a_ = alpha;
#endif

    float centerx, centery, bottomy, topy;
    float val, valr, vall, valtmp, flip;

//    URHO3D_LOGINFOF("WaterLayerData() - UpdateTiledBatch : node=%s(%u) rect=%s viewZ=%d zf=%f...",
//                    node_->GetName().CString(), node_->GetID(), rect.ToString().CString(), fluiddata.viewZ_, zf);

    // Find Pattern Type Pass 1 : identify general cases for FPT_FullWater and FPT_Surface; WaterFalls are FPT_Wave at this stage
    for (int y = rect.bottom_-1; y >= rect.top_; y--)
    {
#ifdef VALIDATE_BASSIN
        // assume to start with a bassin
        int startblock = rect.right_;
        int startfullwaterx = -1;
        int endfullwaterx = -1;
        bool closedBassin = false;
#endif

        for (int x = rect.right_-1; x >= rect.left_; x--)
        {
            FluidCell& c = fluidmap[y * width + x];

#ifdef VALIDATE_BASSIN
            if (c.type_ < WATER)
            {
                if (c.type_ == AIR)
                {
                    // invalidate the bassin
                    startblock = -1;
                    startfullwaterx = endfullwaterx = -1;
                    closedBassin = false;
                }
                else if (c.type_ == BLOCK)
                {
                    // close the bassin.
                    closedBassin = true;
                    // start a new empty bassin.
                    startblock = x;
                }
            }

            if (closedBassin)
            {
                // validate the bassin of fullwater
                if (startfullwaterx != -1 && endfullwaterx != -1)
                {
                    for (int f = endfullwaterx; f <= startfullwaterx; f++)
                        fluidmap[y * width + f].patternType_ = FPT_FullWater;
                }
                // reset the bassin
                startfullwaterx = endfullwaterx = -1;
                closedBassin = false;
            }
#endif

            if (c.type_ < WATER)
                continue;

            c.drawn_ = false;

            // always simulate the bottom as FullWater
            const FluidCell& bottom = y == rect.bottom_-1 || !c.Bottom ? FluidCell::FullWaterCell : *c.Bottom;

            if ((bottom.type_ == BLOCK || bottom.patternType_ == FPT_FullWater) &&
                    (!c.Right  || (c.Right  && c.Right->type_ != AIR)) &&
                    (!c.Left   || (c.Left   && c.Left->type_ != AIR)) &&
                    (!c.Top    || (c.Top    && c.Top->type_ != AIR)))
            {
#ifdef VALIDATE_BASSIN
                if (bottom.type_ != BLOCK && c.Top && c.Top->patternType_ >= WATER)
                {
                    if (startblock != -1)
                    {
                        if (startfullwaterx == -1)
                        {
                            // start the water bassin
                            startfullwaterx = x;
                        }
                        endfullwaterx = x;
                    }

                    c.patternType_ = FPT_Wave;
                }
                else
                {
                    c.patternType_ = (c.Top && c.Top->patternType_ >= WATER) || c.GetMass() >= FLUID_MAXDRAWF ? FPT_FullWater : FPT_Surface;
                }
#else
                c.patternType_ = (c.Top && c.Top->patternType_ >= WATER) || c.GetMass() >= FLUID_MAXDRAWF ? FPT_FullWater : FPT_Surface;
#endif
            }
            else if ((bottom.type_ == BLOCK || bottom.patternType_ == FPT_FullWater) &&
                     (c.Left && c.Left->type_ != AIR) && (c.Right && c.Right->type_ != AIR))
            {
#ifdef VALIDATE_BASSIN
                // FPT_Surface can close a water Bassin if exists.
                closedBassin = true;
#endif
                c.patternType_ = FPT_Surface;

            }
            else if (bottom.type_ != BLOCK && c.Top && c.Top->type_ < WATER &&
                     c.Left && c.Left->type_ == AIR && c.Right && c.Right->type_ == AIR)
            {
                c.patternType_ = FPT_IsolatedWater;
            }
            else if (bottom.type_ != BLOCK)
            {
                c.patternType_ = FPT_Wave;
            }
            else
            {
                c.patternType_ = FPT_Empty;
            }
        }

#ifdef VALIDATE_BASSIN
        // validate the last bassin of fullwater
        if (startfullwaterx != -1 && endfullwaterx != -1)
        {
            for (int f = endfullwaterx; f <= startfullwaterx; f++)
                fluidmap[y * width + f].patternType_ = FPT_FullWater;
        }
#endif
    }

    // Find Pattern Type Pass 2 : transform FPT_Wave to FPT_WaterFall and patch for FPT_Surface cases
    for (int y = rect.top_; y < rect.bottom_; y++)
        for (int x = rect.left_; x < rect.right_; x++)
        {
            FluidCell& c = fluidmap[y * width + x];
            if (c.type_ < WATER)
                continue;

            if (c.patternType_ == FPT_Wave)
            {
                if (c.Top && (c.Top->patternType_ == FPT_WaterFall || c.Top->patternType_ == FPT_Wave))
                    c.patternType_ = FPT_WaterFall;
            }
            else if (c.patternType_ == FPT_FullWater)
            {
                if (c.Top && (c.Top->patternType_ == FPT_WaterFall || c.Top->patternType_ == FPT_Wave || c.Top->patternType_ == FPT_IsolatedWater))
                    c.patternType_ = FPT_Surface;
            }
            else if (c.patternType_ == FPT_Empty)
            {
                if (c.Bottom->type_ == BLOCK)
                    c.patternType_ = FPT_Surface;
            }
        }

    // Update and Draw Patterns
    unsigned addr;
    for (int y = rect.top_; y < rect.bottom_; y++)
        for (int x = rect.left_; x < rect.right_; x++)
        {
            addr = y * width + x;
            FluidCell& cell = fluidmap[addr];

            if (!cell.UpdatePattern())
                continue;

            if (cell.drawn_)
                continue;

            if (checkDepth && cell.DepthZ->type_ == BLOCK)
                continue;

            val = Clamp(cell.GetMass(), 0.f, FLUID_MAXDRAWF);
            valr = cell.Right && cell.Right->type_ != BLOCK ? Clamp((cell.Right->GetMass() + cell.GetMass()) * 0.5f, 0.f, FLUID_MAXDRAWF) : val;
            vall = cell.Left && cell.Left->type_ != BLOCK ? Clamp((cell.Left->GetMass() + cell.GetMass()) * 0.5f, 0.f, FLUID_MAXDRAWF) : val;

            centerx = x * twidth + halfdx;
            centery = (height - y - 1) * theight + halfdy;
            bottomy = centery - halfdy;
            topy = centery + halfdy;

            // Find the bottom of the column
            if (cell.patternType_ == FPT_FullWater)
            {
                FluidCell* bottom = &cell;
                int h = y;
                do
                {
                    bottom = bottom->Bottom;
                    if (!bottom || bottom->type_ < WATER || bottom->patternType_ < FPT_Surface)
                        break;

                    bottom->drawn_ = true;
                }
                while (++h < rect.bottom_-1);

                bottomy = Max(bottomy - (h-y) * theight, 0.f);
            }

            bool allowWaterline = WaterLineAllowed_[cell.pattern_];

            // Draw pattern of the surface
            switch (cell.pattern_)
            {
            case PTRN_Droplet:
            {
                if (val < FLUID_MINDRAWF)
                    continue;
                numQuads = 1;
                flip = y%3 ? 1.f : -1.f;
                yv[0] = bottomy - val*theight*0.5f;
                yv[1] = yv[3] = centery - val*theight*0.5f;
                yv[2] = topy + valr*theight*0.5f;
                xv[0] = xv[2] = centerx;
                xv[1] = centerx - val*twidth*0.1f*flip;
                xv[3] = centerx + val*twidth*0.35f*flip;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::BLUE;
#endif
                break;
            }
            // Swirl
            case PTRN_Swirl:
            {
                numQuads = 2;
                yv[0] = yv[3] = yv[4] = yv[7] = bottomy;
                yv[1] = yv[2] = yv[5] = yv[6] = topy;
                xv[0] = xv[1] = centerx - halfdx;
                xv[6] = xv[7] = centerx + halfdx;
                xv[2] = xv[0] + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[5] = xv[6] - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[3] = xv[0] + val*twidth*0.5f;
                xv[4] = xv[6] - val*twidth*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::CYAN;
#endif
                break;
            }
            // DepthZ
            case PTRN_RightDepthZ:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx + halfdx - val*twidth;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::BLUE;
#endif
                break;
            }
            case PTRN_LeftDepthZ:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx - halfdx + val*twidth;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::BLUE;
#endif
                break;
            }
            case PTRN_CenteredDepthZ:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::BLUE;
#endif
                break;
            }
            case PTRN_CenterFallDepthZ:
            {
                numQuads = 2;
                valtmp = Clamp(cell.Top->DepthZ->GetMass(), 0.f, FLUID_MAXDRAWF);
                xv[0] = centerx - val*twidth*0.5f;
                xv[3] = centerx + val*twidth*0.5f;
                xv[1] = xv[4] = centerx - valtmp*twidth*0.5f;
                xv[2] = xv[7] = centerx + valtmp*twidth*0.5f;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = yv[4] = yv[7] = centery - val*theight*0.5f;
                xv[5] = centerx - valtmp*twidth*0.5f;
                xv[6] = centerx + valtmp*twidth*0.5f;
                yv[5] = yv[6] = topy + valtmp*theight*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::MAGENTA;
#endif
                break;
            }
            // Fall
            case PTRN_RightFall:
            {
                numQuads = 1;
                xv[2] = xv[3] = centerx + halfdx;
                xv[1] = xv[2] - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                xv[0] = xv[3] - val*twidth;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                break;
            }
            case PTRN_LeftFall:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[1] + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                xv[3] = xv[0] + val*twidth;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                break;
            }
            case PTRN_CenterLeftFall:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[1] + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                xv[3] = xv[0] + val*twidth;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::YELLOW;
#endif
                break;
            }
            case PTRN_CenterRightFall:
            {
                numQuads = 1;
                xv[2] = xv[3] = centerx + halfdx;
                xv[1] = xv[2] - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                xv[0] = xv[3] - val*twidth;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::YELLOW;
#endif
                break;
            }
            case PTRN_LeftRightFall:
            {
                numQuads = 2;
                yv[0] = yv[3] = yv[4] = yv[7] = bottomy;
                yv[1] = yv[2] = yv[5] = yv[6] = topy;
                xv[0] = xv[1] = centerx - halfdx;
                xv[6] = xv[7] = centerx + halfdx;
                xv[2] = xv[0] + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[5] = xv[6] - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[3] = xv[0] + val*twidth*0.5f;
                xv[4] = xv[6] - val*twidth*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::YELLOW;
#endif
                break;
            }
            case PTRN_CenterFall1:
            {
                numQuads = 1;
                xv[0] = centerx - val*twidth*0.5f;
                xv[1] = centerx - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[2] = centerx + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[3] = centerx + val*twidth*0.5f;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                break;
            }
            // Fall Head
            case PTRN_RightFallHead:
            {
                numQuads = 1;
                yv[0] = yv[3] = bottomy;
                yv[1] = centery - halfdy + valr*theight*0.5f;
                yv[2] = centery - halfdy + valr*theight;

                xv[0] = centerx + halfdx - val*twidth;
                xv[1] = centerx + halfdx - val*twidth*0.5f;
                xv[2] = xv[3] = centerx + halfdx;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_LeftFallHead:
            {
                numQuads = 1;
                yv[0] = yv[3] = bottomy;
                yv[1] = centery - halfdy + vall*theight;
                yv[2] = centery - halfdy + vall*theight*0.5f;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = centerx - halfdx + val*twidth*0.5f;
                xv[3] = centerx - halfdx + val*twidth;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_CenterLeftFallHead:
            {
                numQuads = 1;
                yv[0] = yv[3] = bottomy;
                yv[1] = centery - halfdy + Clamp(cell.Left->GetMass(), 0.f, FLUID_MAXDRAWF)*theight;
                yv[2] = centery - halfdy + val*theight;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = centerx - halfdx + val*twidth*0.5f;
                xv[3] = centerx - halfdx + val*twidth;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_CenterRightFallHead:
            {
                numQuads = 1;
                yv[0] = yv[3] = bottomy;
                yv[1] = centery - halfdy + val*theight*0.5f;
                yv[2] = centery - halfdy + val*theight;
                xv[0] = centerx + halfdx - val*twidth;
                xv[1] = centerx + halfdx - val*twidth*0.5f;
                xv[2] = xv[3] = centerx + halfdx;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_LeftRightFallHead:
            {
                numQuads = 2;
                yv[0] = yv[3] = yv[4] = yv[7] = bottomy;
                yv[1] = centery - halfdy + vall*theight;
                yv[6] = centery - halfdy + valr*theight;
                yv[2] = centery - halfdy + vall*theight*0.5f;
                yv[5] = centery - halfdy + valr*theight*0.5f;
                xv[0] = xv[1] = centerx - halfdx;
                xv[6] = xv[7] = centerx + halfdx;
                xv[2] = xv[3] = xv[0] + val*twidth*0.5f;
                xv[4] = xv[5] = xv[6] - val*twidth*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_CenterFallHead:
            {
                numQuads = 1;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = centery - halfdy + val*theight;
                xv[0] = xv[1] = centerx - val*twidth*0.5f;
                xv[2] = xv[3] = centerx + val*twidth*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_CenterFallHead2:
            {
                numQuads = 1;
                xv[0] = centerx - val*twidth*0.5f;
                xv[1] = centerx - halfdx;
                xv[2] = centerx + halfdx;
                xv[3] = centerx + val*twidth*0.5f;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::RED;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_SwirlHead:
            {
                numQuads = 2;
                yv[0] = yv[3] = yv[4] = yv[7] = bottomy;
                yv[1] = centery - halfdy + vall*theight;
                yv[2] = centery - halfdy + vall*theight*0.75f;
                yv[6] = centery - halfdy + valr*theight;
                yv[5] = centery - halfdy + valr*theight*0.75f;
                xv[0] = xv[1] = centerx - halfdx;
                xv[6] = xv[7] = centerx + halfdx;
                xv[3] = xv[0] + val*twidth*0.5f;
                xv[2] = (xv[0]+xv[3])*0.5f;
                xv[4] = xv[7] - val*twidth*0.5f;
                xv[5] = (xv[4]+xv[7])*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::CYAN;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            // Fall Foot
            case PTRN_RightFallFoot:
            {
                numQuads = 1;
                xv[2] = xv[3] = centerx + halfdx;
                xv[0] = xv[1] = xv[2] - val*twidth*0.5f;
                yv[0] = yv[3] = topy-val*theight;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GREEN;
#endif
                break;
            }
            case PTRN_LeftFallFoot:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = xv[0] + val*twidth*0.5f;
                yv[0] = yv[3] = topy-val*theight;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GREEN;
#endif
                break;
            }
            // Fall Part && Surface Part
            case PTRN_CenterFallRightSurface:
            {
                numQuads = 2;
                valtmp = Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF);
                xv[0] = xv[1] = xv[4] = centerx - valtmp*twidth*0.5f;
                xv[2] = xv[3] = xv[5] = centerx + valtmp*twidth*0.5f;
                xv[6] = xv[7] = centerx + halfdx;
                yv[0] = yv[4] = yv[7] = bottomy;
                yv[1] = yv[2] = topy;
                yv[6] = bottomy + valr*theight;
                yv[3] = yv[5] = Clamp(bottomy + 1.25f*valr*theight, yv[6], topy);
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 0;
                surfaceVertexIndex[1] = 6;
                break;
            }
            case PTRN_CenterFallLeftSurface:
            {
                numQuads = 2;
                valtmp = Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF);
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[4] = xv[5] = centerx - valtmp*twidth*0.5f;
                xv[3] = xv[6] = xv[7] = centerx + valtmp*twidth*0.5f;
                yv[0] = yv[3] = yv[7] = bottomy;
                yv[5] = yv[6] = topy;
                yv[1] = bottomy + vall*theight;
                yv[2] = yv[4] = Clamp(bottomy + 1.25f*vall*theight, yv[1], topy);
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 7;
                break;
            }
            case PTRN_LeftFallAndRightSurface:
            {
                numQuads = 2;
                yv[0] = yv[3] = yv[4] = yv[7] = bottomy;
                yv[1] = yv[2] = topy;
                yv[5] = bottomy + val*theight*0.5f;
                yv[6] = bottomy + val*theight;

                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[0] + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                xv[3] = xv[0] + val*twidth*0.5f;
                xv[6] = xv[7] = centerx + halfdx;
                xv[4] = xv[5] = xv[6] - val*twidth*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_RightFallAndLeftSurface:
            {
                numQuads = 2;
                yv[0] = yv[3] = yv[4] = yv[7] = bottomy;
                yv[1] = bottomy + Clamp(cell.Left->GetMass(), 0.f, FLUID_MAXDRAWF)*theight;
                yv[2] = bottomy + Clamp(cell.Left->GetMass(), 0.f, FLUID_MAXDRAWF)*theight*0.5f;
                yv[5] = yv[6] = topy;

                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = xv[0] + val*twidth*0.5f;
                xv[6] = xv[7] = centerx + halfdx;
                xv[4] = xv[6] - val*twidth*0.5f;
                xv[5] = xv[6] - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            // Surface
            case PTRN_CenterFallSurface:
            {
                numQuads = 4;
                yv[0] = yv[3] = yv[4] = yv[8] = yv[12] = yv[15] = bottomy;
                yv[6] = yv[7] = yv[9] = yv[10] = topy;
                yv[2] = yv[5] = yv[11] = yv[13] = centery - halfdy + vall*theight + 0.1f*halfdy;
                xv[0] = centerx - halfdx;
                xv[3] = xv[4] = xv[7] = xv[8] = xv[9] = xv[12] = centerx;
                xv[15] = centerx + halfdx;
                valtmp = Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF);
                xv[6] = centerx - valtmp*twidth*0.5f;
                xv[10] = centerx + valtmp*twidth*0.5f;
                xv[2] = xv[5] = Clamp(centerx - valtmp*twidth*0.6f, xv[0], xv[6]);
                xv[11] = xv[13] = Clamp(centerx + valtmp*twidth*0.6f, xv[10], xv[15]);

//            if (cell.Right && cell.Right->patternType_ == FPT_WaterFall && cell.Right->pattern_ <= PTRN_CenterFallSurface)
//            {
//                xv[14] = (xv[13] + xv[15]) * 0.5f;
//                yv[14] = (yv[13] + yv[15]) * 0.5f;
//            }
//            else
//            {
                xv[14] = centerx + halfdx;
                yv[14] = centery - halfdy + valr*theight;
//            }
//            if (cell.Left && cell.Left->patternType_ == FPT_WaterFall && cell.Left->pattern_ <= PTRN_CenterFallSurface)
//            {
//                xv[1] = (xv[0] + xv[2]) * 0.5f;
//                yv[1] = (yv[0] + yv[2]) * 0.5f;
//            }
//            else
//            {
                xv[1] = centerx - halfdx;
                yv[1] = centery - halfdy + vall*theight;
//            }
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 14;
                break;
            }
            case PTRN_LeftFallSurface:
            {
                numQuads = 2;
                xv[0] = xv[1] = centerx - halfdx;
                xv[3] = xv[6] = xv[7] = centerx + halfdx;
                if (cell.Top->pattern_ == PTRN_RightFall || cell.Top->pattern_ == PTRN_RightFallHead)
                    xv[5] = centerx + halfdx - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                else
                    xv[5] = centerx - Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                xv[2] = xv[4] = Clamp(xv[5] + val*twidth*0.15f, xv[5], xv[6]);

                yv[0] = yv[3] = yv[7] = bottomy;
                yv[1] = centery - halfdy + vall*theight;
                yv[5] = yv[6] = topy;
                yv[2] = yv[4] = yv[1] + 0.15f * (topy - yv[1]);//(yv[1] + yv[5]) * 0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 4;
                break;
            }
            case PTRN_RightFallSurface:
            {
                numQuads = 2;
                xv[0] = xv[1] = xv[4] = centerx - halfdx;
                if (cell.Top->pattern_ == PTRN_LeftFall || cell.Top->pattern_ == PTRN_LeftFallHead)
                    xv[2] = centerx - halfdx + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth;
                else
                    xv[2] = centerx + Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[6] = xv[7] = centerx + halfdx;
                xv[3] = xv[5] = Clamp(xv[2] + val*twidth*0.15f, xv[2], xv[6]);

                yv[0] = yv[4] = yv[7] = bottomy;
                yv[1] = yv[2] = topy;
                yv[6] = centery - halfdy + valr*theight;
                yv[3] = yv[5] = yv[6] + 0.15f * (topy - yv[6]);//(yv[1] + yv[6]) * 0.5f;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 3;
                surfaceVertexIndex[1] = 6;
                break;
            }
            case PTRN_LeftRightFallSurface:
            {
                numQuads=4;
                xv[0] = xv[1] = xv[4] = centerx - halfdx;
                xv[11] = xv[14] = xv[15] = centerx + halfdx;
                xv[6] = xv[7] = xv[8] = xv[9] = centerx;
                valtmp = Clamp(cell.Top->GetMass(), 0.f, FLUID_MAXDRAWF)*twidth*0.5f;
                xv[2] = centerx - halfdx + valtmp;
                xv[3] = xv[5] = Clamp(xv[2] + val*twidth*0.25f, xv[2], centerx);
                xv[13] = centerx + halfdx - valtmp;
                xv[10] = xv[12] = Clamp(xv[13] - val*twidth*0.25f, centerx, xv[13]);

                yv[0] = yv[4] = yv[7] = yv[8] = yv[11] = yv[15] = bottomy;
                yv[1] = yv[2] = yv[13] = yv[14] = topy;
                yv[6] = yv[9] = Clamp(centery - halfdy + (valr+vall)*theight*0.5f, 0.f, topy);
                yv[3] = yv[5] = Clamp(yv[6] + vall*theight*0.25f, 0.f, topy);
                yv[10] = yv[12] = Clamp(yv[6] + valr*theight*0.25f, 0.f, topy);
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 3;
                surfaceVertexIndex[1] = 10;
                break;
            }
            case PTRN_LeftSurface:
            {
                numQuads = 2;
                xv[0] = xv[1] = xv[4] = xv[7] = centerx - halfdx;
                xv[2] = centerx;
                xv[3] = xv[5] = xv[6] = centerx + halfdx;

                yv[0] = yv[3] = yv[4] = yv[5] = centery - halfdy;
                yv[1] = centery - halfdy + vall*theight;
                yv[2] = centery - halfdy + vall*theight*0.5f;
                yv[6] = yv[7] = bottomy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_RightSurface:
            {
                numQuads = 2;
                xv[0] = xv[4] = xv[7] = centerx - halfdx;
                xv[1] = centerx;
                xv[2] = xv[3] = xv[5] = xv[6] = centerx + halfdx;

                yv[0] = yv[3] = yv[4] = yv[5] = centery - halfdy;
                yv[1] = centery - halfdy + valr*theight*0.5f;
                yv[2] = centery - halfdy + valr*theight;
                yv[6] = yv[7] = bottomy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_TopLeftSurface:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = topy;
                yv[2] = centery - halfdy + valr*theight;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_TopRightSurface:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = centery - halfdy + vall*theight;
                yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            case PTRN_CenterSurface:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = centery - halfdy + vall*theight;
                yv[2] = centery - halfdy + valr*theight;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                surfaceVertexIndex[0] = 1;
                surfaceVertexIndex[1] = 2;
                break;
            }
            // Full
            case PTRN_Full:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::GRAY;
#endif
                allowWaterline = (!cell.Top || cell.Top->type_ == AIR);
                if (allowWaterline)
                {
                    surfaceVertexIndex[0] = 1;
                    surfaceVertexIndex[1] = 2;
                }
                break;
            }
            case PTRN_FullTest:
            {
                numQuads = 1;
                xv[0] = xv[1] = centerx - halfdx;
                xv[2] = xv[3] = centerx + halfdx;
                yv[0] = yv[3] = bottomy;
                yv[1] = yv[2] = topy;
#ifdef FLUID_RENDER_COLORDEBUG
                leftColor = rightColor = Color::BLACK;
#endif
                break;
            }
            default:
                numQuads = 0;
            }

            for (int i=0; i < 4*numQuads; i++)
            {
                worldTransform2D.Multiply(xv[i], yv[i], vertex[i].position_);
#ifdef URHO3D_VULKAN
                vertex[i].z_ = zf;
#else
                vertex[i].position_.z_ = zf;
#endif
            }

#ifdef FLUID_RENDER_COLORDEBUG
            rightColor.a_ = alpha;
            unsigned coloruint = rightColor.ToUInt();
            for (int i=0; i < 4*numQuads; i++)
                vertex[i].color_ = coloruint;
#endif

            bool drawSurface = viewZ == fluiddata.viewZ_ && allowWaterline;
            bool drawQuads = !drawSurface || cell.pattern_ < PTRN_LeftSurface;

            if (drawQuads)
            {
                // Add WaterTile
                for (int i = 0; i < 4 * numQuads; i++)
                    vertices.Push(vertex[i]);
            }

            if (drawSurface)
            {
                // Add WaterSurfaces for generating WaterLines (include the WaterTile under the waterline)
                waterSurfaces_.Resize(waterSurfaces_.Size()+1);

                bottomy = worldTransform2D.m11_ * bottomy + worldTransform2D.m12_;

                if (drawQuads)
                {
                    unsigned baseindex = vertices.Size() - 4 * numQuads;
                    Vertex2D& vertice0 = vertices[baseindex + surfaceVertexIndex[0]];
                    Vertex2D& vertice1 = vertices[baseindex + surfaceVertexIndex[1]];
                    waterSurfaces_.Back().Set(mapdata.objectTiled_->map_, &cell, addr, bottomy, vertice0, vertice1, numQuads == 1);
                }
                else
                {
                    waterSurfaces_.Back().Set(mapdata.objectTiled_->map_, &cell, addr, bottomy, vertex[surfaceVertexIndex[0]], vertex[surfaceVertexIndex[1]], false);
                }

#ifdef FLUID_RENDER_COLORDEBUG
                waterSurfaces_.Back().debugColor_ = coloruint;
#endif
            }

            cell.drawn_ = true;
        }
}

void WaterLayerData::UpdateBatches()
{
    int viewZ = ViewManager::Get()->GetCurrentViewZ(viewport_);

    Clear(viewZ);

    // WaterTiles
    {
        for (unsigned i = 0; i < mapdatas_.Size(); i++)
        {
            ViewportRenderData& mapdata = *mapdatas_[i];

            ObjectFeatured* featureData = mapdata.objectTiled_->GetObjectFeatured();

            const Vector<int>& fluidViewIds = featureData->GetFluidViewIDs(viewZ);
            for (int j = 0; j < fluidViewIds.Size(); j++)
                UpdateTiledBatch(viewZ, mapdata, featureData->GetFluidView(fluidViewIds[j]));
        }
    }

    // WaterLines
    {
        // Reset FluidCell drawn status before Expand the Waterlines
        for (int i = 0; i < waterSurfaces_.Size(); i++)
            waterSurfaces_[i].cell_->drawn_ = false;

        // Find WaterLines
        // Get WaterSurfaces not again used, and create new waterlines.
        unsigned waterlineIndex = 0;

        for (int i = 0; i < waterSurfaces_.Size(); i++)
        {
            WaterSurface* surface = &waterSurfaces_[i];
            if (surface->cell_->drawn_)
                continue;

            // Create a new Waterline if needed
            if (waterlineIndex >= waterLines_.Size())
            {
                waterLines_.Resize(waterLines_.Size()+1);
                WaterLine& waterline = waterLines_.Back();

                // Create Physics
#ifdef USE_WATERLINE_COLLISIONCHAIN
                CollisionChain2D* shape = WaterLayer::Get()->GetNode()->CreateComponent<CollisionChain2D>(LOCAL);
                shape->SetLoop(true);
#else
                CollisionBox2D* shape = WaterLayer::Get()->GetNode()->CreateComponent<CollisionBox2D>(LOCAL);
#endif

                shape->SetColliderInfo(WATERCOLLIDER);

                // No Collision with Map Colliders
                shape->SetGroupIndex(-1);

                // Add it to the Waterline
                waterline.shape_ = shape;
            }

            // Get the next cleared WaterLine
            WaterLine& waterline = waterLines_[waterlineIndex++];

            // Update Physic if need
            if (waterline.shape_->GetViewZ() != viewZ - LAYER_FLUID)
            {
                waterline.shape_->SetViewZ(viewZ - LAYER_FLUID);
                if (viewZ == INNERVIEW)
                    waterline.shape_->SetFilterBits(CC_INSIDEWALL, CM_INSIDEWALL);
                else
                    waterline.shape_->SetFilterBits(CC_OUTSIDEWALL, CM_OUTSIDEWALL);
            }

            // Expand the waterline from the watersurface
            waterline.ExpandFrom(surface, waterSurfaces_);
        }

        // Apply Deformation to WaterLines
        unsigned numwaterlines = 0;
        const float zf = 1.f - (viewZ + LAYER_FLUID) * PIXEL_SIZE;
        for (Vector<WaterLine>::Iterator it = waterLines_.Begin(); it != waterLines_.End(); ++it)
        {
            WaterLine& waterline = *it;

            if (waterline.expanded_)
            {
                // Update Line Deformation and Batch
                waterline.Update(waterlinesBatch_, watertilesFrontBatch_, zf);

                numwaterlines++;
            }

            // Enabled/Disabled physics box
            if (waterline.shape_)
                waterline.shape_->SetEnabled(waterline.expanded_);
        }

#ifdef WATERLAYER_DEBUG
        URHO3D_LOGDEBUGF("WaterLayerData() - UpdateBatches : numwatersurfaces=%u numwaterlines=%u(%u)", waterSurfaces_.Size(), numwaterlines, waterLines_.Size());
#endif
    }

    batchesDirty_ = false;
}



// WaterLayer Component Effect (one only in the world)

WaterLayer* WaterLayer::waterLayer_ = 0;

void WaterLayer::RegisterObject(Context* context)
{
    context->RegisterFactory<WaterLayer>();
}

WaterLayer::WaterLayer(Context* context) :
    Drawable2D(context)
{
    WaterLayer::waterLayer_ = this;

    Init();

    // for WaterLayer being batched after ObjectTiled
    isSourceBatchedAtEnd_ = true;
}

WaterLayer::~WaterLayer()
{
    WaterLayer::waterLayer_ = 0;
}

void WaterLayer::Init()
{
    waterDeformationPoints_.Resize(MAX_DEFORMATIONPOINTS);
    for (unsigned i=0; i < waterDeformationPoints_.Size(); i++)
        waterDeformationPoints_[i].point_ = Vector3::ZERO;

    layerDatas_.Resize(MAX_VIEWPORTS);
    for (unsigned i = 0; i < layerDatas_.Size(); i++)
    {
        layerDatas_[i].viewport_ = i;
        layerDatas_[i].Clear();
    }

    fluidUpdateTime_ = FLUID_SIMULATION_UPDATEINTERVAL+1;
}

void WaterLayer::Clear()
{
    Init();

    OnSetEnabled();
}

void WaterLayer::SetViewportData(bool enable, ViewportRenderData* mapdata)
{
    WaterLayerData& layerData = layerDatas_[mapdata->viewport_];

    if (enable)
    {
        if (!layerData.mapdatas_.Contains(mapdata))
            layerData.mapdatas_.Push(mapdata);
    }
    else
    {
        if (layerData.mapdatas_.Contains(mapdata))
            layerData.mapdatas_.Remove(mapdata);
    }
}

void WaterLayer::AddSplashAt(const float x, const float y, const float velocity)
{
    for (unsigned i = 0; i < waterDeformationPoints_.Size(); i++)
    {
        if (waterDeformationPoints_[i].point_.z_ == 0.f)
        {
            waterDeformationPoints_[i].AddDeformation(x, y, velocity);
#ifdef WATERLAYER_DEBUG
            URHO3D_LOGDEBUGF("WaterLayer - AddSplashAt() : add point = %F,%F!", x, y);
#endif
            return;
        }
    }

//    URHO3D_LOGERRORF("WaterLayer - AddSplashAt() : no more Free Deformation Points !");
}


void WaterLayer::OnWorldBoundingBoxUpdate()
{
    // Set a large dummy bounding box to ensure the layer is rendered
    boundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
    worldBoundingBox_ = boundingBox_;
}

void WaterLayer::OnNodeSet(Node* node)
{
    if (node)
    {
        RigidBody2D* fluidbody = node->GetOrCreateComponent<RigidBody2D>(LOCAL);
        fluidbody->SetBodyType(BT_STATIC);
        fluidbody->SetUseFixtureMass(false);
        fluidbody->SetMass(MAPMASS);
        fluidbody->SetMassCenter(Vector2::ZERO);
        fluidbody->SetFixedRotation(true);
    }
}

void WaterLayer::OnSetEnabled()
{
    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective() && GameContext::Get().gameConfig_.fluidEnabled_)
        {
            fluidUpdateTime_ = FLUID_SIMULATION_UPDATEINTERVAL+1;

            SubscribeToEvent(GetScene(), E_SCENEPOSTUPDATE, URHO3D_HANDLER(WaterLayer, HandleUpdateFluids));
            SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(WaterLayer, HandleBeginFluidContact));

            URHO3D_LOGINFOF("WaterLayer() - OnSetEnabled = true OK !");
        }
        else
        {
            UnsubscribeFromAllEvents();

            URHO3D_LOGINFOF("WaterLayer() - OnSetEnabled = false OK !");
        }
    }
}

void WaterLayer::UpdateBatches(WaterLayerData& layerData)
{
//    URHO3D_LOGDEBUGF("WaterLayer() - Update : viewport=%d numFluidBatchesUpdated=%d...", viewport, numMapBatchesUpdates_[viewport]);

    // Update Deformation velocity values once a time (first viewport only)
    if (layerData.viewport_ == 0)
    {
        for (unsigned i = 0; i < waterDeformationPoints_.Size(); i++)
            waterDeformationPoints_[i].Update();
    }

    layerData.UpdateBatches();

//    URHO3D_LOGDEBUGF("WaterLayer() - Update : viewport=%d ... numMapsDrawn=%u OK !", viewport, numMapsDrawn);
}

const Vector<SourceBatch2D*>& WaterLayer::GetSourceBatchesToRender(Camera* camera)
{
    WaterLayerData& layerData = layerDatas_[camera->GetViewport()];

    if (layerData.batchesDirty_)
    {
        //URHO3D_LOGINFOF("WaterLayer - GetSourceBatchesToRender() : viewport=%d ...", layerData.viewport_);
        UpdateBatches(layerData);
    }

    // Add Batches
    sourceBatchesToRender_[0].Clear();
    sourceBatchesToRender_[0].Push(&layerData.watertilesBackBatch_);
    sourceBatchesToRender_[0].Push(&layerData.watertilesFrontBatch_);
    sourceBatchesToRender_[0].Push(&layerData.waterlinesBatch_);

    return sourceBatchesToRender_[0];
}


void WaterLayer::HandleUpdateFluids(StringHash eventType, VariantMap& eventData)
{
    if (!GameContext::Get().gameConfig_.fluidEnabled_ || !MapSimulatorLiquid::Get())
        return;

    // if paused no update
    if (!layerDatas_.Front().mapdatas_.Size() ||
        layerDatas_.Front().mapdatas_.Front()->isPaused_)
        return;

//	URHO3D_LOGDEBUGF("WaterLayer - HandleUpdateFluids() : ...");

    URHO3D_PROFILE(WaterLayer_UpdateFluids);

    fluidUpdateTime_ += eventData[SceneUpdate::P_TIMESTEP].GetFloat();

    if (fluidUpdateTime_ > FLUID_SIMULATION_UPDATEINTERVAL)
    {
        unsigned numviewports = ViewManager::Get()->GetNumViewports();
        for (unsigned viewport = 0; viewport < numviewports; viewport++)
            bool update = layerDatas_[viewport].UpdateSimulation();

        fluidUpdateTime_ = 0.f;
    }
}

const float MAX_SPLASHVEL = 10.f;

void WaterLayer::HandleBeginFluidContact(StringHash eventType, VariantMap& eventData)
{
    using namespace PhysicsBeginContact2D;

    const Urho3D::ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[P_CONTACTINFO].GetUInt());

    Vector2	entityPosition;
    float entityHalfSize;

    if (cinfo.shapeA_->IsInstanceOf<CollisionCircle2D>())
    {
        CollisionCircle2D* entityShape = static_cast<CollisionCircle2D*>(cinfo.shapeA_.Get());
        entityPosition = cinfo.shapeA_->GetNode()->GetWorldTransform2D() * entityShape->GetCenter();
        entityHalfSize = entityShape->GetRadius() * entityShape->GetNode()->GetWorldScale2D().y_;
    }
    else
    {
        CollisionBox2D* entityShape = static_cast<CollisionBox2D*>(cinfo.shapeA_.Get());
        entityPosition = cinfo.shapeA_->GetNode()->GetWorldTransform2D() * entityShape->GetCenter();
        entityHalfSize = entityShape->GetSize().y_ * 0.5f * entityShape->GetNode()->GetWorldScale2D().y_;
    }

    CollisionBox2D* fluidShape = static_cast<CollisionBox2D*>(cinfo.shapeB_.Get());
    Vector2 fluidPosition = fluidShape->GetNode()->GetWorldTransform2D() * fluidShape->GetCenter();
    if (fluidShape->GetTangent())
        fluidPosition.y_ += (fluidPosition.x_ - entityPosition.x_) * fluidShape->GetTangent();

    // inside fluid in x
    if (Abs(fluidPosition.x_ - entityPosition.x_) < fluidShape->GetWorldHalfSizeX())
    {
//		URHO3D_LOGINFOF("WaterLayer() - HandleBeginFluidContact : %s => %s(%u) begin fluid contact on viewZ=%d !", node_->GetName().CString(), cinfo.bodyA_->GetNode()->GetName().CString(), cinfo.bodyA_->GetNode()->GetID(), cinfo.shapeB_->GetViewZ());

        AddSplashAt(entityPosition.x_, fluidPosition.y_, Sign(cinfo.bodyA_->GetLinearVelocity().y_) * Clamp(Abs(cinfo.bodyA_->GetLinearVelocity().y_) / MAX_SPLASHVEL * cinfo.bodyA_->GetMass(), 0.1f, 0.3f));

        // inside fluid in y ? send event if need
        if (cinfo.shapeA_->IsTrigger() && cinfo.shapeA_->GetNode()->GetVar(GOA::FIRE).GetBool() && !cinfo.bodyA_->GetNode()->GetVar(GOA::INFLUID).GetBool())
        {
            bool isinsidefluid = (fluidPosition.y_ >= entityPosition.y_ - entityHalfSize) || (fluidShape->GetCenter().y_ - fluidShape->GetPivot().y_ >= 0.f) || (cinfo.bodyA_->GetLinearVelocity().y_ < -1.f);

            if (isinsidefluid)
            {
//				URHO3D_LOGERRORF("WaterLayer() - HandleBeginFluidContact : %s => %s(%u) begin fluid contact on viewZ=%d at velocity=%F mass=%F entityY=%F fluidY=%F fluidCenterY=%F fluidPivotY=%F fluidAngle=%F => insidefluid=%s",
//								node_->GetName().CString(), cinfo.bodyA_->GetNode()->GetName().CString(), cinfo.bodyA_->GetNode()->GetID(),
//								cinfo.shapeB_->GetViewZ(), cinfo.bodyA_->GetLinearVelocity().y_, cinfo.bodyA_->GetMass(), entityPosition.y_, fluidPosition.y_,
//								fluidShape->GetCenter().y_, fluidShape->GetPivot().y_, Atan(fluidShape->GetTangent()),
//								isinsidefluid ? "true":"false");

                eventData[Go_CollideFluid::GO_WETTED] = true;
                cinfo.bodyA_->GetNode()->SendEvent(GO_COLLIDEFLUID, eventData);
            }
        }
    }
}


void AddDebugWaterSquare(DebugRenderer* debug, const Vector3& center, float width, float height, const Color& color, bool depthTest)
{
    Vector3 v1(center.x_ - width/2, center.y_ - height/2);
    Vector3 v2(center.x_ - width/2, center.y_ + height/2);
    Vector3 v3(center.x_ + width/2, center.y_ + height/2);
    Vector3 v4(center.x_ + width/2, center.y_ - height/2);
    debug->AddTriangle(v1, v2, v3, color, depthTest);
    debug->AddTriangle(v3, v4, v1, color, depthTest);
}

void WaterLayer::DrawDebugWaterQuads(DebugRenderer* debug, const FluidDatas& fluiddata, const ViewportRenderData& viewportdata)
{
    const Matrix3x4& worldTransform = viewportdata.objectTiled_->GetNode()->GetWorldTransform();
    const IntRect& rect = viewportdata.chunkGroup_.GetRect();

    const unsigned width = viewportdata.objectTiled_->GetWidth();
    const unsigned height = viewportdata.objectTiled_->GetHeight();
    const float twidth  = MapInfo::info.tileWidth_;
    const float theight = MapInfo::info.tileHeight_;

//    URHO3D_LOGINFOF("ObjectTiled() - DrawDebugWaterQuads : rect=%s minval=%f!", rect.ToString().CString(), PIXEL_SIZE);

#ifdef FLUID_RENDER_DRAWDEBUG_SIMULATION
    if (fluiddata.fluidmap_.Size())
    {
        const FluidMap& fluidmap = fluiddata.fluidmap_;
        float centerx, centery, r, mass;
        for (int y = rect.top_; y < rect.bottom_; y++)
            for (int x = rect.left_; x < rect.right_; x++)
            {
                const FluidCell& cell = fluidmap[y * width + x];

                if (cell.type_ != WATER)
                    continue;

                centerx = x * twidth + twidth*0.5f;
                centery = (height - y - 1) * theight + theight*0.5f;
                mass = cell.GetMass();
                r = Clamp(mass, 0.f, FLUID_MAXVALUE) * theight;

                if (mass < FLUID_MINVALUE)
                    debug->AddCircle(worldTransform * Vector3(centerx, centery, 0.f), Vector3::FORWARD, 2*PIXEL_SIZE, Color::RED, 8, false, false);
                else if (mass < FLUID_MAXVALUE)
                    debug->AddCircle(worldTransform * Vector3(centerx, centery, 0.f), Vector3::FORWARD, r, Color::BLUE, 8, false, false);
                else
                    debug->AddCircle(worldTransform * Vector3(centerx, centery, 0.f), Vector3::FORWARD, r, Color::CYAN, 8, false, false);

                if (cell.state_ & FluidCell::SETTLED)
                    AddDebugWaterSquare(debug, worldTransform * Vector3(centerx-twidth/2+4*PIXEL_SIZE, centery+theight/2-4*PIXEL_SIZE, 0.f), 12*PIXEL_SIZE, 12*PIXEL_SIZE, Color::RED, false);

#if (FLUID_RENDER_USEMESH == 4)
                if (cell.patternType_ > FPT_Empty)
                {
                    Color color;
                    switch (cell.patternType_)
                    {
                    case FPT_IsolatedWater:
                        color = Color::BLUE;
                        break;
                    case FPT_NoRender:
                        color = Color::WHITE;
                        break;
                    case FPT_WaterFall:
                        color = Color::RED;
                        break;
                    case FPT_Wave:
                        color = Color::MAGENTA;
                        break;
                    case FPT_Swirl:
                        color = Color::CYAN;
                        break;
                    case FPT_Surface:
                        color = Color::GRAY;
                        break;
                    case FPT_FullWater:
                        color = Color::YELLOW;
                        break;
                    }

                    if (cell.align_ & FA_Left)
                        AddDebugWaterSquare(debug, worldTransform * Vector3(centerx-4*PIXEL_SIZE, centery+theight/2-10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, color, false);
                    if (cell.align_ & FA_Right)
                        AddDebugWaterSquare(debug, worldTransform * Vector3(centerx+4*PIXEL_SIZE, centery+theight/2-10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, color, false);
                    if (cell.align_ & FA_Center)
                        AddDebugWaterSquare(debug, worldTransform * Vector3(centerx, centery+theight/2-10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, color, false);
                    if (cell.align_ == FA_None)
                        AddDebugWaterSquare(debug, worldTransform * Vector3(centerx - twidth/2 + 10*PIXEL_SIZE, centery+theight/2-10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, color, false);
                }
#endif
#if defined(FLUID_RENDER_DRAWDEBUG_SIMULATION5_MORE) || defined(FLUID_RENDER_DRAWDEBUG_SIMULATION6_MORE)
                // Draw Flow Directions
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx, centery+2*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowdir_ & FD_Top ? Color::WHITE : Color::GRAY, false);
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx, centery-2*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowdir_ & FD_Bottom ? Color::WHITE : Color::GRAY, false);
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx-2*PIXEL_SIZE, centery, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowdir_ & FD_Left ? Color::WHITE : Color::GRAY, false);
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx+2*PIXEL_SIZE, centery, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowdir_ & FD_Right ? Color::WHITE : Color::GRAY, false);
#endif
#if (FLUID_SIMULATION == 6) && defined(FLUID_RENDER_DRAWDEBUG_SIMULATION6_MORE)
                // Draw 3Flows
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx, centery-theight/2+10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowC_ < FLUID_MINVALUE ? Color::GRAY : cell.flowC_ < FLUID_MAXVALUE*.5f ? Color::YELLOW : Color::GREEN, false);
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx-2*PIXEL_SIZE, centery-theight/2+10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowL_ < FLUID_MINVALUE ? Color::GRAY : cell.flowL_ < FLUID_MAXVALUE*.5f ? Color::YELLOW : Color::GREEN, false);
                AddDebugWaterSquare(debug, worldTransform * Vector3(centerx+2*PIXEL_SIZE, centery-theight/2+10*PIXEL_SIZE, 0.f), 8*PIXEL_SIZE, 8*PIXEL_SIZE, cell.flowR_ < FLUID_MINVALUE ? Color::GRAY : cell.flowR_ < FLUID_MAXVALUE*.5f ? Color::YELLOW : Color::GREEN, false);
#endif
            }
    }
#endif
#ifdef FLUID_RENDER_DRAWDEBUG_SOURCE
    {
        const PODVector<FluidSource>& sources = fluiddata.sources_;

        Vector3 v0, v1, v2, v3;
        float dx, dy;
        Color color = Color::BLUE;
        color.a_ = 0.75f;

        for (unsigned isource = 0; isource < sources.Size(); isource++)
        {
            const FluidSource& source = sources[isource];

            if (source.quantity_ < 0)
                continue;

            dx = source.startx_ * twidth;
            dy = (height - source.starty_ - 1) * theight;

            v0 = worldTransform * Vector3(dx, dy, 0.0f);
            v1 = worldTransform * Vector3(dx, dy + theight, 0.0f);
            v2 = worldTransform * Vector3(dx + twidth, dy + theight, 0.0f);
            v3 = worldTransform * Vector3(dx + twidth, dy, 0.0f);

            debug->AddTriangle(v0, v1, v2, color, false);
            debug->AddTriangle(v0, v2, v3, color, false);
        }
    }
#endif
}

void WaterLayer::DrawDebugWaterMesh(DebugRenderer* debug, const Vector<Vertex2D>& vertices, unsigned color)
{
    if (vertices.Size())
    {
        Vector3 v0, v1, v2, v3;

        unsigned count = 0;
        unsigned size = vertices.Size();
        while (count < size)
        {
            v0 = vertices[count].position_;
            v1 = vertices[count + 1].position_;
            v2 = vertices[count + 2].position_;
            v3 = vertices[count + 3].position_;

            debug->AddLine(v0, v1, color, false);
            debug->AddLine(v1, v2, color, false);
            debug->AddLine(v2, v0, color, false);
            debug->AddLine(v0, v3, color, false);
            debug->AddLine(v3, v2, color, false);

            count += 4;
        }
    }
}

void WaterLayer::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
{
    unsigned numviewports = ViewManager::Get()->GetNumViewports();
    for (unsigned viewport = 0; viewport < numviewports; viewport++)
    {
        const WaterLayerData& layerData = layerDatas_[viewport];
        int viewZ = ViewManager::Get()->GetCurrentViewZ(viewport);

        for (unsigned i = 0; i < layerData.mapdatas_.Size(); i++)
        {
            ObjectTiled* objectTiled = layerData.mapdatas_[i]->objectTiled_;

            if (objectTiled->IsEnabledEffective())
            {
                ObjectFeatured* featureData = objectTiled->GetObjectFeatured();
                const Vector<int>& fluidViewIds = featureData->GetFluidViewIDs(viewZ);
                if (fluidViewIds.Size())
                {
                    for (Vector<int>::ConstIterator it = fluidViewIds.Begin(); it != fluidViewIds.End(); ++it)
                        if (*it != NOVIEW)
                            DrawDebugWaterQuads(debug, featureData->GetFluidView(*it), *layerData.mapdatas_[i]);
                }
            }
        }
    #ifdef FLUID_RENDER_DRAWDEBUG_MESH
        DrawDebugWaterMesh(debug, layerData.watertilesBackBatch_.vertices_, Color::GRAY.ToUInt());
        DrawDebugWaterMesh(debug, layerData.watertilesFrontBatch_.vertices_, Color::BLUE.ToUInt());
    #endif
    #ifdef FLUID_RENDER_WATERLINE_RECTS
        for (unsigned i = 0; i < GetWaterLines(0).Size(); i++)
        {
            const WaterLine& waterline = GetWaterLines(0)[i];
            if (waterline.expanded_)
                GameHelpers::DrawDebugRect(waterline.boundingRect_, debug, false, Color::RED);
        }
    #endif
    }
}

