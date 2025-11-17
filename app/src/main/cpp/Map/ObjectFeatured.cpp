#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/IO/Log.h>

#include "DefsViews.h"
#include "DefsWorld.h"

#include "GameOptionsTest.h"
#include "GameEvents.h"
#include "GameHelpers.h"

#include "ViewManager.h"

#include "MapGeneratorDungeon.h"
#include "Map.h"
#include "MemoryObjects.h"

#include "ObjectFeatured.h"

//#define SORTSINGLE


/// ObjectFeatured Implementation

#define MAXNUMFEATUREFILTERS 6

extern const char* mapStatusNames[];

const char* FeatureFilterStr[] =
{
    "NOFILTER(0)",
    "REPLACEFEATURE(1)",
    "REPLACEFEATUREBACK(2)",
    "REPLACEFRONTIER(3)",
    "COPYTILEMODIFIER(4)",
    "TRANSFERFRONTIER(5)",
    "COPYTILEMODIFIER_KEEPINNERSPACE(6)"
};


Vector<int> ObjectFeatured::viewZs_;
static Vector<unsigned> workMap_;

void ObjectFeatured::SetViewZs(const Vector<int>& viewZs)
{
    viewZs_ = viewZs;
}

int ObjectFeatured::GetIndexViewZ(int viewZ)
{
    Vector<int>::ConstIterator it = viewZs_.Find(viewZ);
    return (it != viewZs_.End() ? it-viewZs_.Begin() : -1);
}


ObjectFeatured::ObjectFeatured() :
    width_(0),
    height_(0),
    numviews_(0)
{

}

ObjectFeatured::ObjectFeatured(unsigned width, unsigned height, unsigned numviews)
{
    Resize(width, height, numviews);
}

ObjectFeatured::~ObjectFeatured()
{
//    URHO3D_LOGDEBUG("~ObjectFeatured() ... OK !");
}

void ObjectFeatured::Clear()
{
//    URHO3D_LOGINFOF("ObjectFeatured() - Clear ...");

    // Clear Filters
    featurefilters_.Clear();

    // Clear Views
    viewZ_.Clear();
    viewIds_.Clear();
    fluidViewIds_.Clear();

    // Clear Fluid Maps
    for (unsigned i=0; i < fluidView_.Size(); ++i)
        fluidView_[i].Clear();

//    URHO3D_LOGINFOF("ObjectFeatured() - Clear : viewIds_=%u, maskedView_=%u featurefilters_=%u",
//            viewIds_.Size(), maskedView_.Size(), featurefilters_.Size());
}

void ObjectFeatured::PullFluids()
{
    // Pull Fluid Maps
    for (unsigned i=0; i < fluidView_.Size(); ++i)
    {
        fluidView_[i].Pull();
    }
}

void ObjectFeatured::Resize(unsigned width, unsigned height, unsigned numviews)
{
    // resize maskViews table
    if (maskedView_.Size() != viewZs_.Size())
        maskedView_.Resize(viewZs_.Size());

    // Allocate if numviews is setting
//    URHO3D_LOGINFOF("ObjectFeatured() - Resize : w=%d h=%d numviews=%d", width_, height_, numviews_);
    if (numviews != numviews_)
    {
        numviews_ = numviews;
        featuredView_.Resize(numviews_);

        // resize maskviews for each viewZ standard
        for (unsigned i=0; i < viewZs_.Size(); ++i)
            maskedView_[i].Resize(numviews_);
    }

    if (fluidView_.Size() != ViewManager::GetNumFluidViewZ())
        fluidView_.Resize(ViewManager::GetNumFluidViewZ());

    unsigned size = width * height;
    if (size != width_ * height_)
    {
        // resize terrainmap
        terrainMap_.Resize(size);
        biomeMap_.Resize(size);

        // resize featuredviews
        for (unsigned i=0; i < numviews_; ++i)
        {
            featuredView_[i].Resize(size);
//            URHO3D_LOGINFOF("ObjectFeatured() - Resize : ... viewID=%d buffer=%u", i, &featureView_[i][0]);
        }

        // resize maskviews to mapsize
        for (unsigned i=0; i < viewZs_.Size(); ++i)
            for (unsigned j=0; j < numviews_; ++j)
                maskedView_[i][j].Resize(size);
    }

    if (width_ != width || height_ != height)
    {
        // resize fluidviews and link fluid cells
        if (fluidView_.Size())
        {
            unsigned numfluidviews = fluidView_.Size();

            for (int i=0; i < numfluidviews; ++i)
                fluidView_[i].Resize(i, width, height);

//            GetFluidViewByZValue(INNERVIEW).LinkInnerCells(&(GetFluidViewByZValue(FRONTVIEW)));
//            GetFluidViewByZValue(FRONTVIEW).LinkInnerCells(&(GetFluidViewByZValue(INNERVIEW)));
            if (numfluidviews > 1)
            {
                for (int i=0; i < numfluidviews-1; i++)
                    fluidView_[i].LinkInnerCells(&(fluidView_[i+1]));
                fluidView_[numfluidviews-1].LinkInnerCells(&(fluidView_[0]));
            }
            else
                fluidView_[0].LinkInnerCells(0);
        }

        if (width_ != width)
        {
            width_ = width;
            nghTable4_[0] = -(int)width;   // Top (N)
            nghTable4_[1] = 1;              // Right (E)
            nghTable4_[2] = width;         // Bottom (S)
            nghTable4_[3] = -1;             // Left (W)
            nghCorner_[0] = nghTable4_[0]-1; // TopLeft
            nghCorner_[1] = nghTable4_[0]+1; // TopRight
            nghCorner_[2] = nghTable4_[2]+1; // BottomRight
            nghCorner_[3] = nghTable4_[2]-1; // BottomLeft
            nghTable8_[0] = -(int)width;   // Top (N)
            nghTable8_[1] = 1;              // Right (E)
            nghTable8_[2] = width;         // Bottom (S)
            nghTable8_[3] = -1;             // Left (W)
            nghTable8_[4] = -(int)width-1; // Top (N) Left (W)
            nghTable8_[5] = -(int)width+1; // Top(N) Right (E)
            nghTable8_[6] = width + 1;     // Bottom (S) Right (E)
            nghTable8_[7] = width - 1;     // Bottom (S) Left (W)
        }

        if (height_ != height)
            height_ = height;
    }
}

void ObjectFeatured::Copy(int left, int top, ObjectFeatured& object)
{
    // copy the views defs and filters
    object.viewZ_ = viewZ_;
    object.viewIds_ = viewIds_;
    object.fluidViewIds_ = fluidViewIds_;
    object.featurefilters_ = featurefilters_;

    // copy features maps
    unsigned dst_dw = object.width_ * sizeof(FeatureType);
    unsigned src_start = (top * width_ + left) * sizeof(FeatureType);
    unsigned src_dw = dst_dw;
    unsigned src_skip = width_ * sizeof(FeatureType);

    for (unsigned i=0; i < featuredView_.Size(); i++)
        CopyFast((char*)object.featuredView_[i].Buffer(), (char*)featuredView_[i].Buffer() + src_start, dst_dw, src_dw, src_skip, object.height_);

    // copy terrain map
    CopyFast((char*)object.terrainMap_.Buffer(), (char*)terrainMap_.Buffer() + src_start, dst_dw, src_dw, src_skip, object.height_);

    // copy biome map
    CopyFast((char*)object.biomeMap_.Buffer(), (char*)biomeMap_.Buffer() + src_start, dst_dw, src_dw, src_skip, object.height_);
}

void ObjectFeatured::SetViewConfiguration(int modeltype)
{
    /// Set Link between ViewIDs and ViewZs
    if (modeltype == GEN_CAVE)
    {
        LinkViewIdToViewZ(FrontView_ViewId, FRONTVIEW);
        LinkViewIdToViewZ(BackGround_ViewId, BACKGROUND);
        LinkViewIdToViewZ(InnerView_ViewId, INNERVIEW);
    }
    else if (modeltype == GEN_DUNGEON)
    {
        LinkViewIdToViewZ(FrontView_ViewId, FRONTVIEW);
        LinkViewIdToViewZ(BackGround_ViewId, BACKGROUND);
        LinkViewIdToViewZ(InnerView_ViewId, INNERVIEW);
        LinkViewIdToViewZ(BackView_ViewId, BACKVIEW);
        LinkViewIdToViewZ(OuterView_ViewId, OUTERVIEW);
    }
    else if (modeltype == GEN_ASTEROID)
    {
        LinkViewIdToViewZ(0, INNERVIEW);
//        LinkViewIdToViewZ(1, OUTERVIEW);
    }
    else if (modeltype == GEN_MOBILECASTLE)
    {
        LinkViewIdToViewZ(0, BACKVIEW);
        LinkViewIdToViewZ(1, INNERVIEW);
        LinkViewIdToViewZ(2, OUTERVIEW);
    }

    /// Set Feature Filters
    if (modeltype == GEN_CAVE)
    {
        AddFeatureFilter(COPYTILEMODIFIER, FrontView_ViewId, BackGround_ViewId, MapFeatureType::OuterFloor, MapFeatureType::OuterFloor);

        AddFeatureFilter(TRANSFERFRONTIER, FrontView_ViewId, InnerView_ViewId, MapFeatureType::OuterFloor, MapFeatureType::Door, MapFeatureType::TunnelInnerSpace, MapFeatureType::NoMapFeature, 0);

        // convert inner features(=NoMapFeature) to InnerSpace
        AddFeatureFilter(REPLACEFEATUREBACK, InnerView_ViewId, BackGround_ViewId, MapFeatureType::NoMapFeature, MapFeatureType::TunnelInnerSpace, MapFeatureType::Threshold);
    }
    else if (modeltype == GEN_DUNGEON)
    {
        AddFeatureFilter(COPYTILEMODIFIER, FrontView_ViewId, BackGround_ViewId, MapFeatureType::OuterFloor, MapFeatureType::OuterFloor);

        /// INNERVIEW
        AddFeatureFilter(COPYTILEMODIFIER, InnerView_ViewId, BackView_ViewId, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);
        AddFeatureFilter(COPYTILEMODIFIER, InnerView_ViewId, OuterView_ViewId, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);
        AddFeatureFilter(COPYTILEMODIFIER, InnerView_ViewId, OuterView_ViewId, MapFeatureType::RoomWall, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, InnerView_ViewId, MapFeatureType::Window, MapFeatureType::RoomInnerSpace);
        AddFeatureFilter(REPLACEFEATURE, InnerView_ViewId, MapFeatureType::InnerSpace, MapFeatureType::RoomInnerSpace);
        AddFeatureFilter(REPLACEFEATURE, InnerView_ViewId, MapFeatureType::CorridorPlateForm, MapFeatureType::RoomWall);

#ifdef ACTIVE_DUNGEONROOFS
        AddFeatureFilter(REPLACEFEATURE, InnerView_ViewId, MapFeatureType::InnerRoof, MapFeatureType::RoofInnerSpace);
        AddFeatureFilter(REPLACEFEATUREBACK, InnerView_ViewId, FrontView_ViewId, MapFeatureType::RoofInnerSpace, MapFeatureType::RoomWall, MapFeatureType::Threshold);
#endif
        // add door entrance => dans innerview remplace RoomInnerSpace par Door si dans Frontview c'est NoMapFeature et qu'au moins un voisin dans innerview est un NoMapFeature
        AddFeatureFilter(REPLACEFRONTIER, InnerView_ViewId, FrontView_ViewId, MapFeatureType::RoomInnerSpace, MapFeatureType::Door, MapFeatureType::NoMapFeature, &(MapGeneratorDungeon::Get()->GetDungeonInfo().doorIndexes_));

        /// BACKVIEW
        // remove window if background is block
        //AddFeatureFilter(REPLACEFEATUREBACK, BackView_ViewId, BackGround_ViewId, MapFeatureType::Window, MapFeatureType::RoomWall, MapFeatureType::Threshold);
        // always remove window in backview
        AddFeatureFilter(REPLACEFEATURE, BackView_ViewId, MapFeatureType::Window, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, BackView_ViewId, MapFeatureType::CorridorPlateForm, MapFeatureType::NoMapFeature);
        AddFeatureFilter(REPLACEFEATURE, BackView_ViewId, MapFeatureType::RoomPlateForm, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, BackView_ViewId, MapFeatureType::RoomFloor, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, BackView_ViewId, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);

#ifdef ACTIVE_DUNGEONROOFS
        AddFeatureFilter(REPLACEFEATUREBACK, BackView_ViewId, FrontView_ViewId, MapFeatureType::InnerRoof, MapFeatureType::RoomWall, MapFeatureType::Threshold);
#endif
        /// OUTERVIEW
        AddFeatureFilter(REPLACEFEATURE, OuterView_ViewId, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);
#ifdef ACTIVE_DUNGEONROOFS
        AddFeatureFilter(REPLACEFEATURE, OuterView_ViewId, MapFeatureType::InnerRoof, MapFeatureType::OuterRoof);
#endif
        AddFeatureFilter(REPLACEFEATURE, OuterView_ViewId, MapFeatureType::RoomPlateForm, MapFeatureType::RoomWall);
        // remove window if frontview is block
        AddFeatureFilter(REPLACEFEATUREBACK, OuterView_ViewId, FrontView_ViewId, MapFeatureType::Window, MapFeatureType::RoomWall, MapFeatureType::Threshold);
        // Remove door entrance
        AddFeatureFilter(REPLACEFRONTIER, OuterView_ViewId, InnerView_ViewId, MapFeatureType::RoomWall, MapFeatureType::NoMapFeature, MapFeatureType::Door);

        AddFeatureFilter(TRANSFERFRONTIER, FrontView_ViewId, InnerView_ViewId, MapFeatureType::OuterFloor, MapFeatureType::Door, MapFeatureType::TunnelInnerSpace, MapFeatureType::NoMapFeature, 0);

        // convert inner features(=NoMapFeature) to InnerSpace
        AddFeatureFilter(REPLACEFEATUREBACK, InnerView_ViewId, BackView_ViewId, MapFeatureType::NoMapFeature, MapFeatureType::RoomInnerSpace, MapFeatureType::Threshold);
#ifdef ACTIVE_DUNGEONROOFS
        /// Test Roof keep state on BackGround
        AddFeatureFilter(REPLACEFEATUREBACK2, BackGround_ViewId, BackView_ViewId, MapFeatureType::NoMapFeature, MapFeatureType::RoofInnerSpace, MapFeatureType::InnerRoof);

//        AddFeatureFilter(REPLACEFEATUREBACK, InnerView_ViewId, BackView_ViewId, MapFeatureType::RoofInnerSpace, MapFeatureType::NoMapFeature, MapFeatureType::Threshold);
#endif
    }
    else if (modeltype == GEN_ASTEROID)
    {

    }
    else if (modeltype == GEN_MOBILECASTLE)
    {
        /// INNERVIEW
        AddFeatureFilter(COPYTILEMODIFIER, 1, 0, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);
        AddFeatureFilter(COPYTILEMODIFIER, 1, 2, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);
        AddFeatureFilter(COPYTILEMODIFIER, 1, 2, MapFeatureType::RoomWall, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, 1, MapFeatureType::Window, MapFeatureType::RoomInnerSpace);
        AddFeatureFilter(REPLACEFEATURE, 1, MapFeatureType::InnerSpace, MapFeatureType::RoomInnerSpace);
        AddFeatureFilter(REPLACEFEATURE, 1, MapFeatureType::CorridorPlateForm, MapFeatureType::RoomWall);

        // add door entrance => dans innerview remplace RoomInnerSpace par Door et qu'au moins un voisin dans innerview est un NoMapFeature
        AddFeatureFilter(REPLACEFRONTIER, 1, 100, MapFeatureType::RoomInnerSpace, MapFeatureType::Door, MapFeatureType::NoMapFeature, &(MapGeneratorDungeon::Get()->GetDungeonInfo().doorIndexes_));

        /// BACKVIEW
        // remove window if background is block
        AddFeatureFilter(REPLACEFEATURE, 0, MapFeatureType::CorridorPlateForm, MapFeatureType::NoMapFeature);
        AddFeatureFilter(REPLACEFEATURE, 0, MapFeatureType::RoomPlateForm, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, 0, MapFeatureType::RoomFloor, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, 0, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);

        /// OUTERVIEW
        AddFeatureFilter(REPLACEFEATURE, 2, MapFeatureType::InnerSpace, MapFeatureType::RoomWall);
        AddFeatureFilter(REPLACEFEATURE, 2, MapFeatureType::RoomPlateForm, MapFeatureType::RoomWall);
        // Remove door entrance
        AddFeatureFilter(REPLACEFRONTIER, 2, 1, MapFeatureType::RoomWall, MapFeatureType::NoMapFeature, MapFeatureType::Door);

        AddFeatureFilter(TRANSFERFRONTIER, 2, 1, MapFeatureType::OuterFloor, MapFeatureType::Door, MapFeatureType::TunnelInnerSpace, MapFeatureType::NoMapFeature, 0);

        // convert inner features(=NoMapFeature) to InnerSpace
        AddFeatureFilter(REPLACEFEATUREBACK, 1, 0, MapFeatureType::NoMapFeature, MapFeatureType::RoomInnerSpace, MapFeatureType::Threshold);
    }

    /// Set Fluid
    if (modeltype == GEN_CAVE)
    {
        SetFluidView(InnerView_ViewId, -1, INNERVIEW);
        LinkFluidViewToZView(FRONTVIEW, INNERVIEW);

        SetFluidView(FrontView_ViewId, -1, FRONTVIEW);
        LinkFluidViewToZView(INNERVIEW, FRONTVIEW);
    }
    else if (modeltype == GEN_DUNGEON)
    {
        // Add Fluid in Innerview with OuterView for Wall DepthZ Checking
        SetFluidView(InnerView_ViewId, OuterView_ViewId, INNERVIEW);
        // Show Fluid from FrontView in INNERVIEW
        LinkFluidViewToZView(FRONTVIEW, INNERVIEW);

        // Add Fluid in FRONTVIEW with OuterView for Wall DepthZ Checking
        SetFluidView(FrontView_ViewId, OuterView_ViewId, FRONTVIEW);
        // Show Fluid from InnerView in FRONTVIEW
        LinkFluidViewToZView(INNERVIEW, FRONTVIEW);
    }
    else if (modeltype == GEN_ASTEROID)
    {
        SetFluidView(0, -1, INNERVIEW);
        /*
                // Show Fluid from FrontView in INNERVIEW
                LinkFluidViewToZView(FRONTVIEW, INNERVIEW);
                SetFluidView(1, -1, FRONTVIEW);
                // Show Fluid from InnerView in FRONTVIEW
                LinkFluidViewToZView(INNERVIEW, FRONTVIEW);
        */
    }
    else if (modeltype == GEN_MOBILECASTLE)
    {
        // Add Fluid in Innerview with OuterView for Wall DepthZ Checking
        SetFluidView(1, 2, INNERVIEW);
        // Show Fluid from OuterView in INNERVIEW
        LinkFluidViewToZView(OUTERVIEW, INNERVIEW);
    }

    /// Sorting views
    SortViews();

    URHO3D_LOGDEBUGF("ObjectFeatured() - SetViewConfiguration : mpoint=%s modeltype=%d ...", map_->GetMapPoint().ToString().CString(), modeltype);
}

void ObjectFeatured::LinkViewIdToViewZ(unsigned viewid, unsigned zValue)
{
//    URHO3D_LOGINFOF("ObjectFeatured() - LinkViewIdToViewZ : viewID=%u zValue=%d", id, zValue);
    viewZ_[viewid] = zValue;
}

void ObjectFeatured::CopyFeatureView(unsigned viewidfrom, unsigned viewid)
{
//    URHO3D_LOGINFOF("ObjectFeatured() - CopyFeatureView : viewIDFrom=%u viewID=%u w=%d h=%d", viewidfrom, viewid, width_, height_);

    FeatureType* addrfrom = &featuredView_[viewidfrom][0];
    FeatureType* addr = &featuredView_[viewid][0];

    if (addr != addrfrom)
        memcpy(addr, addrfrom, width_ * height_ * sizeof(FeatureType));
}

unsigned ObjectFeatured::AddFeatureView(unsigned zValue, const FeatureType* featureView, bool hasFluidView)
{
    unsigned id = featuredView_.Size();
    unsigned mapsize = width_ * height_;
    viewZ_[id] = zValue;

    featuredView_.Push(FeaturedMap(featureView, mapsize));

    if (hasFluidView)
    {
        fluidView_.Resize(fluidView_.Size()+1);
        fluidView_.Back().fluidmap_.Resize(mapsize);
    }

//    URHO3D_LOGINFOF("ObjectFeatured() - AddFeatureView : viewID=%u buffer=%u zValue=%d w=%d h=%d", id, buffer, zValue, width_, height_);

//    GameHelpers::DumpData(buffer, 1, 2, width_, height_);

    return id;
}

void ObjectFeatured::SetFluidView(unsigned featureViewId, int checkViewId, unsigned zValue)
{
    if (featureViewId >= numviews_)
    {
        URHO3D_LOGWARNINGF("ObjectFeatured() - SetFluidView : ... no featured view at featureViewId=%u !", featureViewId);
        return;
    }

    unsigned fluidViewId = LinkFluidViewToZView(zValue, zValue);

    if (fluidViewId >= fluidView_.Size())
    {
        URHO3D_LOGERRORF("ObjectFeatured() - SetFluidView : ... fluidView Size < fluidViewId=%u !", fluidViewId);
        return;
    }

    FeaturedMap* checkMap = checkViewId >= 0 || checkViewId < numviews_ ? &(featuredView_[checkViewId]) : 0;
    fluidView_[fluidViewId].LinkFeatureMaps(this, &(featuredView_[featureViewId]), checkMap);
    fluidView_[fluidViewId].viewZ_ = zValue;
    URHO3D_LOGINFOF("ObjectFeatured() - SetFluidView : featureViewId=%u checkViewId=%d checkMap=%u fluidViewId=%u zValue=%u", featureViewId, checkViewId, checkMap, fluidViewId, zValue);
}

int ObjectFeatured::LinkFluidViewToZView(unsigned zValue1, unsigned zValue2)
{
    const Vector<int>& fluidZ = ViewManager::GetFluidZ();
    Vector<int>::ConstIterator it = fluidZ.Find(zValue1);
    if (it == fluidZ.End())
    {
        URHO3D_LOGWARNINGF("ObjectFeatured() - LinkFluidViews : ... no fluidZ=%u !", zValue1);
        return -1;
    }

    int fluidViewId = it - fluidZ.Begin();

    if (fluidViewId >= fluidView_.Size())
    {
        URHO3D_LOGERRORF("ObjectFeatured() - LinkFluidViews : ... fluidView Size < fluidViewId=%u !", fluidViewId);
        return -1;
    }

    // WARNING : first in fluidViewIds => first to be updated && drawn
    fluidViewIds_[zValue2].Push(fluidViewId);

    return fluidViewId;
}


void ObjectFeatured::AddFeatureFilter(unsigned filterId, unsigned viewId, FeatureType feature)
{
    featurefilters_.Resize(featurefilters_.Size()+1);
    FeatureFilterInfo& info = featurefilters_.Back();
    info.filter_ = filterId;
    info.feature1_ = feature;
    info.viewId1_ = viewId;
    info.storeIndexes_ = 0;
    indexToSet_ = 0;
}

void ObjectFeatured::AddFeatureFilter(unsigned filterId, unsigned viewId, FeatureType feature1, FeatureType feature2)
{
    featurefilters_.Resize(featurefilters_.Size()+1);
    FeatureFilterInfo& info = featurefilters_.Back();
    info.filter_ = filterId;
    info.feature1_ = feature1;
    info.feature2_ = feature2;
    info.viewId1_ = viewId;
    info.storeIndexes_ = 0;
    indexToSet_ = 0;
}

void ObjectFeatured::AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2)
{
    featurefilters_.Resize(featurefilters_.Size()+1);
    FeatureFilterInfo& info = featurefilters_.Back();
    info.filter_ = filterId;
    info.feature1_ = feature1;
    info.feature2_ = feature2;
    info.viewId1_ = viewId1;
    info.viewId2_ = viewId2;
    info.storeIndexes_ = 0;
    indexToSet_ = 0;
}

void ObjectFeatured::AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2, FeatureType featureBack, Vector<unsigned>* storeIndexes)
{
    featurefilters_.Resize(featurefilters_.Size()+1);
    FeatureFilterInfo& info = featurefilters_.Back();
    info.filter_ = filterId;
    info.feature1_ = feature1;
    info.feature2_ = feature2;
    info.featureBack_ = featureBack;
    info.viewId1_ = viewId1;
    info.viewId2_ = viewId2;
    info.storeIndexes_ = storeIndexes;
    indexToSet_ = 0;
}

void ObjectFeatured::AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2, FeatureType featureBack, FeatureType featureReplace, Vector<unsigned>* storeIndexes)
{
    featurefilters_.Resize(featurefilters_.Size()+1);
    FeatureFilterInfo& info = featurefilters_.Back();
    info.filter_ = filterId;
    info.feature1_ = feature1;
    info.feature2_ = feature2;
    info.featureBack_ = featureBack;
    info.featureReplace_ = featureReplace;
    info.viewId1_ = viewId1;
    info.viewId2_ = viewId2;
    info.storeIndexes_ = storeIndexes;
    indexToSet_ = 0;
}

bool ObjectFeatured::HasNeighBors4(const FeaturedMap& featureMap, unsigned addr, FeatureType feature) const
{
    for (unsigned i=0; i<4; ++i)
    {
        // check par index sans utiliser coords (x,y)
        if (addr/width_ == (addr+nghTable4_[i])/width_)
            if (featureMap[addr+nghTable4_[i]] == feature)
                return true;
    }
    return false;
}

void ObjectFeatured::ApplyFeatureFilter(unsigned id)
{
    const FeatureFilterInfo& info = featurefilters_[id];
    FeaturedMap& featMap1 = GetFeatureView(info.viewId1_);

    if (info.filter_ > MAXNUMFEATUREFILTERS)
    {
        URHO3D_LOGERRORF("ObjectFeatured - ApplyFeatureFilter : filterid=%u filter=%d > MAXNUMFEATUREFILTERS(%d)", id, info.filter_, MAXNUMFEATUREFILTERS);
        return;
    }

#ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW
    URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilter - Filter[%u]=%s - Dump Before ApplyFilter ... ViewID=%d ...", id, FeatureFilterStr[info.filter_], info.viewId1_);
    GameHelpers::DumpData(&featMap1[0], 1, 2, width_, height_);
#endif

    if (info.filter_ == REPLACEFEATURE)
    {
        for (FeaturedMap::Iterator it=featMap1.Begin(); it!=featMap1.End(); ++it)
        {
            FeatureType& feature = *it;
            if (feature == info.feature1_)
                feature = info.feature2_;
        }
    }
    else if (info.filter_ == REPLACEFEATUREBACK)
    {
        for (unsigned addr=0; addr < featMap1.Size(); addr++)
        {
            FeatureType& feature = featMap1[addr];
            if (feature == info.feature1_)
                if (GetFeatureView(info.viewId2_)[addr] > info.featureBack_)
                    feature = info.feature2_;
        }
    }
    else if (info.filter_ == REPLACEFEATUREBACK2)
    {
        for (unsigned addr=0; addr < featMap1.Size(); addr++)
        {
            FeatureType& feature = featMap1[addr];
            if (feature == info.feature1_)
                if (GetFeatureView(info.viewId2_)[addr] == info.featureBack_)
                    feature = info.feature2_;
        }
    }
    // Remplace le feat1 par feat2 si le voisinage de la cellule contient au moins un vide et que la seconde map ne contient pas de feat bloquant
    //   ex1 : map->featuredMap_->AddFeatureFilter(REPLACEFRONTIER, InnerView_ViewId, FrontView_ViewId, MapFeatureType::RoomInnerSpace, MapFeatureType::Door, MapFeatureType::NoMapFeature);
    //   => dans innerview remplace RoomInnerSpace par Door si dans Fronview c'est NoMapFeature et qu'au moins un voisin dans innerview est un NoMapFeature
    //   ex2 : map->featuredMap_->AddFeatureFilter(REPLACEFRONTIER, OuterView_ViewId, InnerView_ViewId, MapFeatureType::RoomWall, MapFeatureType::NoMapFeature, MapFeatureType::Door);
    //   => dans outerview remplace RoomWall par NoMapFeature si dans innerview c'est Door et qu'au moins un voisin dans outerview est un NoMapFeature
    else if (info.filter_ == REPLACEFRONTIER)
    {
        workMap_.Clear();
        // parcourir featMap1 et stocker dans workmap les cells a remplacer
        // l'utilisation du tableau temporaire est necessaire (le test des cellules voisines doit etre effectuer avant changement des cells)

        if (info.viewId2_ < GetNumViews())
        {
            FeaturedMap& featMap2 = GetFeatureView(info.viewId2_);
            for (unsigned addr=0; addr < featMap1.Size(); addr++)
            {
                if (featMap1[addr] == info.feature1_)
                    if (featMap2[addr] == info.featureBack_)
                        if (HasNeighBors4(featMap1, addr, MapFeatureType::NoMapFeature))
                            workMap_.Push(addr);
            }
        }
        else
        {
            // pas de seconde map pour tester => assumer que la condition sur featureBack est remplie
            for (unsigned addr=0; addr < featMap1.Size(); addr++)
            {
                if (featMap1[addr] == info.feature1_)
                    if (HasNeighBors4(featMap1, addr, MapFeatureType::NoMapFeature))
                        workMap_.Push(addr);
            }
        }

        // remplacer les cells trouvees
        for (unsigned i=0; i < workMap_.Size(); ++i)
        {
            featMap1[workMap_[i]] = info.feature2_;
        }

        if (info.storeIndexes_)
        {
            for (unsigned i=0; i < workMap_.Size(); ++i)
                info.storeIndexes_->Push(workMap_[i]);
        }
    }
    // si (feat1 sur map1 && map1 voisinage a un vide au moins && feat <= featback sur map2) alors replace le feat1 par featreplace et set en feat2 dans map2
    // AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2, FeatureType featureBack, FeatureType featureReplace)
    // ex1 : map->featuredMap_->AddFeatureFilter(TRANSFERFRONTIER, FrontView_ViewId, InnerView_ViewId, MapFeatureType::OuterFloor, MapFeatureType::Door, MapFeatureType::TunnelInnerSpace, MapFeatureType::NoMapFeature);
    else if (info.filter_ == TRANSFERFRONTIER)
    {
        workMap_.Clear();

        FeaturedMap& featMap2 = GetFeatureView(info.viewId2_);
        for (unsigned addr=0; addr < featMap1.Size(); addr++)
        {
            if (featMap1[addr] == info.feature1_)
                if (featMap2[addr] <= info.featureBack_)
                    if (HasNeighBors4(featMap1, addr, MapFeatureType::NoMapFeature))
                        workMap_.Push(addr);
        }

        // transfer et remplace les cells trouvees
        for (unsigned i=0; i < workMap_.Size(); ++i)
        {
            featMap1[workMap_[i]] = info.featureReplace_;
            featMap2[workMap_[i]] = info.feature2_;
        }

        if (info.storeIndexes_)
        {
            for (unsigned i=0; i < workMap_.Size(); ++i)
                info.storeIndexes_->Push(workMap_[i]);
        }
    }
#ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW
    URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilter - Filter[%u]=%s - Dump After ApplyFilter ... ViewID=%d", id, FeatureFilterStr[info.filter_], info.viewId1_);
    GameHelpers::DumpData(&featMap1[0], 1, 2, width_, height_);
    if (info.filter_ == TRANSFERFRONTIER)
        GameHelpers::DumpData(&GetFeatureView(info.viewId2_)[0], 1, 2, width_, height_);
#endif
}

bool ObjectFeatured::ApplyFeatureFilters(unsigned addr)
{
    bool change = false;

    URHO3D_LOGDEBUGF("ObjectFeatured() - ApplyFeatureFilters ... addr=%u numFeatureFilters=%u ... ", addr, featurefilters_.Size());
    for (Vector<FeatureFilterInfo >::ConstIterator ft=featurefilters_.Begin(); ft != featurefilters_.End(); ++ft)
    {
        const FeatureFilterInfo& info = *ft;
        if (info.viewId1_ >= featuredView_.Size())
            continue;

        FeatureType& feature1 = GetFeatureView(info.viewId1_)[addr];

        if (info.filter_ == COPYTILEMODIFIER)
        {
            if (info.viewId2_ >= featuredView_.Size())
                continue;

            if (feature1 == info.feature1_)
            {
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters : COPYTILEMODIFIER ... addr=%u feat=%u to %u from viewId=%s to viewId=%s ... OK !",
                                addr, info.feature1_, info.feature2_, ViewManager::GetViewName(info.viewId1_).CString(), ViewManager::GetViewName(info.viewId2_).CString());
#endif
                GetFeatureView(info.viewId2_)[addr] = info.feature2_;
                change = true;
            }
        }
        else if (info.filter_ == REPLACEFEATURE)
        {
            if (info.viewId2_ >= featuredView_.Size())
                continue;

            if (feature1 == info.feature1_)
            {
#ifdef DUMP_MAPDEBUG_SETTILE
                URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters : REPLACEFEATURE ... addr=%u feat=%u to %u on viewId=%s ... OK !",
                                addr, info.feature1_, info.feature2_, ViewManager::GetViewName(info.viewId1_).CString());
#endif
                feature1 = info.feature2_;
                change = true;
            }
        }
        else if (info.filter_ == REPLACEFEATUREBACK)
        {
            if (info.viewId2_ >= featuredView_.Size())
                continue;

            if (feature1 == info.feature1_)
            {
                if (GetFeatureView(info.viewId2_)[addr] > info.featureBack_)
            {
#ifdef DUMP_MAPDEBUG_SETTILE
                    URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters : REPLACEFEATUREBACK ... addr=%u feat=%u to %u on viewId=%s (featback>%u on viewId=%s)... OK !",
                                    addr, info.feature1_, info.feature2_, ViewManager::GetViewName(info.viewId1_).CString(), info.featureBack_, ViewManager::GetViewName(info.viewId2_).CString());
#endif
                feature1 = info.feature2_;
                change = true;
            }
        }
        }
        else if (info.filter_ == REPLACEFEATUREBACK2)
        {
            if (info.viewId2_ >= featuredView_.Size())
                continue;

            if (feature1 == info.feature1_)
            {
                if (GetFeatureView(info.viewId2_)[addr] == info.featureBack_)
                {
#ifdef DUMP_MAPDEBUG_SETTILE
                    URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters : REPLACEFEATUREBACK2 ... addr=%u feat=%u to %u on viewId=%s (featback=%u on viewId=%s)... OK !",
                                    addr, info.feature1_, info.feature2_, ViewManager::GetViewName(info.viewId1_).CString(), info.featureBack_, ViewManager::GetViewName(info.viewId2_).CString());
#endif
                    feature1 = info.feature2_;
                    change = true;
                }
            }
        }
        // Remplace le feat1 par feat2 si le voisinage de la cellule contient au moins un vide et que la second map ne contient pas de feat bloquant
        else if (info.filter_ == REPLACEFRONTIER)
        {
            if (feature1 == info.feature1_)
            {
                if (info.viewId2_ >= GetNumViews() || GetFeatureView(info.viewId2_)[addr] == info.featureBack_)
                {
                    if (HasNeighBors4(GetFeatureView(info.viewId1_), addr, MapFeatureType::NoMapFeature))
                    {
#ifdef DUMP_MAPDEBUG_SETTILE
                        URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters : REPLACEFRONTIER ... addr=%u feat=%u to %u on viewId=%s (featback=%u on viewId=%s)... OK !",
                                        addr, info.feature1_, info.feature2_, ViewManager::GetViewName(info.viewId1_).CString(), info.featureBack_, ViewManager::GetViewName(info.viewId2_).CString());
#endif
                        feature1 = info.feature2_;
                        change = true;
                    }
                }
            }
        }
        // si (feat1 sur map1 && map1 voisinage a un vide au moins && feat <= featback sur map2) alors replace le feat1 par featreplace et set en feat2 dans map2
        //      AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2, FeatureType featureBack, FeatureType featureReplace)
        // ex : AddFeatureFilter(TRANSFERFRONTIER, FrontView_ViewId, InnerView_ViewId, MapFeatureType::OuterFloor, MapFeatureType::Door, MapFeatureType::TunnelInnerSpace, MapFeatureType::NoMapFeature);
        else if (info.filter_ == TRANSFERFRONTIER)
        {
            if (feature1 == info.feature1_)
            {
                FeatureType& feature2 = GetFeatureView(info.viewId2_)[addr];
                if (feature2 <= info.featureBack_)
                {
                    if (HasNeighBors4(GetFeatureView(info.viewId1_), addr, MapFeatureType::NoMapFeature))
                    {
#ifdef DUMP_MAPDEBUG_SETTILE
                        URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters : TRANSFERFRONTIER ... addr=%u feat=%u to %u on viewId=%s and feat=%u to feat=%u on viewId=%s... OK !",
                                        addr, feature1, info.featureReplace_, ViewManager::GetViewName(info.viewId1_).CString(), feature2, info.feature2_, ViewManager::GetViewName(info.viewId2_).CString());
#endif
                        feature1 = info.featureReplace_;
                        feature2 = info.feature2_;
                        change = true;
                    }
                }
            }
        }
    }
    URHO3D_LOGDEBUGF("ObjectFeatured() - ApplyFeatureFilters ... addr=%u numFeatureFilters=%u ... change=%u !", addr, featurefilters_.Size(), change);

    return change;
}

void ObjectFeatured::ApplyFeatureFilter(const List<TileModifier >& modifiers, unsigned id)
{
    const FeatureFilterInfo& info = featurefilters_[id];
    FeaturedMap& featMap1 = GetFeatureView(info.viewId1_);

#ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW_MODIFIER
    URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilter - Filter[%u]=%s - Before ApplyFilter ... ViewID=%s(%d) ... numTileModifiers=%u ...", id, FeatureFilterStr[info.filter_],
                    ViewManager::GetViewName(info.viewId1_).CString(), info.viewId1_, modifiers.Size());
#endif
    if (info.filter_ == COPYTILEMODIFIER)
    {
        for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
        {
            unsigned addr = mt->y_*width_ + mt->x_;
            FeatureType& feature = featMap1[addr];
            if (feature == info.feature1_)
            {
            #ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW_MODIFIER
                URHO3D_LOGINFOF("... Modifier at x=%d y=%d view2=%d feature=%u => feature=%u!", mt->x_, mt->y_, info.viewId2_, GetFeatureView(info.viewId2_)[addr], info.feature2_);
            #endif
                GetFeatureView(info.viewId2_)[addr] = info.feature2_;
            }
        }
    }
    else if (info.filter_ == REPLACEFEATURE)
    {
        for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
        {
            FeatureType& feature = featMap1[mt->y_*width_+mt->x_];
            if (feature == info.feature1_)
            {
            #ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW_MODIFIER
                URHO3D_LOGINFOF("... Modifier at x=%d y=%d view1=%d feature=%u => %u !", mt->x_, mt->y_, info.viewId1_, info.feature1_, info.feature2_);
            #endif
                feature = info.feature2_;
            }

        }
    }
    else if (info.filter_ == REPLACEFEATUREBACK)
    {
        for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
        {
            unsigned addr = mt->y_*width_ + mt->x_;
            FeatureType& feature = featMap1[addr];
            if (feature == info.feature1_)
                if (GetFeatureView(info.viewId2_)[addr] > info.featureBack_)
                {
                #ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW_MODIFIER
                    URHO3D_LOGINFOF("... Modifier at x=%d y=%d view1=%d feature=%u => %u !", mt->x_, mt->y_, info.viewId1_, info.feature1_, info.feature2_);
                #endif
                feature = info.feature2_;
        }
    }
    }
    else if (info.filter_ == REPLACEFEATUREBACK2)
    {
        for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
        {
            unsigned addr = mt->y_*width_ + mt->x_;
            FeatureType& feature = featMap1[addr];
            if (feature == info.feature1_)
            {
                if (GetFeatureView(info.viewId2_)[addr] == info.featureBack_)
                {
                #ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW_MODIFIER
                    URHO3D_LOGINFOF("... Modifier at x=%d y=%d view1=%d feature=%u => %u !", mt->x_, mt->y_, info.viewId1_, info.feature1_, info.feature2_);
                #endif
                    feature = info.feature2_;
                }
            }
        }
    }
    // Remplace le feat1 par feat2 si le voisinage de la cellule contient au moins un vide et que la seconde map ne contient pas de feat bloquant
    //   ex1 : map->featuredMap_->AddFeatureFilter(REPLACEFRONTIER, InnerView_ViewId, FrontView_ViewId, MapFeatureType::RoomInnerSpace, MapFeatureType::Door, MapFeatureType::NoMapFeature);
    //   => dans innerview remplace RoomInnerSpace par Door si dans Fronview c'est NoMapFeature et qu'au moins un voisin dans innerview est un NoMapFeature
    //   ex2 : map->featuredMap_->AddFeatureFilter(REPLACEFRONTIER, OuterView_ViewId, InnerView_ViewId, MapFeatureType::RoomWall, MapFeatureType::NoMapFeature, MapFeatureType::Door);
    //   => dans outerview remplace RoomWall par NoMapFeature si dans innerview c'est Door et qu'au moins un voisin dans outerview est un NoMapFeature
    else if (info.filter_ == REPLACEFRONTIER)
    {
        workMap_.Clear();
        // parcourir featMap1 et stocker dans workmap les cells a remplacer
        // l'utilisation du tableau temporaire est necessaire (le test des cellules voisines doit etre effectuer avant changement des cells)

        if (info.viewId2_ < GetNumViews())
        {
            FeaturedMap& featMap2 = GetFeatureView(info.viewId2_);
            for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
            {
                unsigned addr = mt->y_*width_ + mt->x_;
                if (featMap1[addr] == info.feature1_)
                    if (featMap2[addr] == info.featureBack_)
                        if (HasNeighBors4(featMap1, addr, MapFeatureType::NoMapFeature))
                            workMap_.Push(addr);
            }
        }
        else
        {
            // pas de seconde map pour tester => assumer que la condition sur featureBack est remplie
            for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
            {
                unsigned addr = mt->y_*width_ + mt->x_;
                if (featMap1[addr] == info.feature1_)
                    if (HasNeighBors4(featMap1, addr, MapFeatureType::NoMapFeature))
                        workMap_.Push(addr);
            }
        }

        // remplacer les cells trouvees
        for (unsigned i=0; i < workMap_.Size(); ++i)
        {
            featMap1[workMap_[i]] = info.feature2_;
        }

        if (info.storeIndexes_)
        {
            for (unsigned i=0; i < workMap_.Size(); ++i)
                info.storeIndexes_->Push(workMap_[i]);
        }
    }
    // si (feat1 sur map1 && map1 voisinage a un vide au moins && feat <= featback sur map2) alors replace le feat1 par featreplace et set en feat2 dans map2
    //                          AddFeatureFilter(TRANSFERFRONTIER, unsigned viewId1, unsigned viewId2, FeatureType feature1,      FeatureType feature2, FeatureType featureBack,           FeatureType featureReplace)
    // ex1 : map->featuredMap_->AddFeatureFilter(TRANSFERFRONTIER, FrontView_ViewId, InnerView_ViewId, MapFeatureType::OuterFloor, MapFeatureType::Door, MapFeatureType::TunnelInnerSpace, MapFeatureType::NoMapFeature);
    else if (info.filter_ == TRANSFERFRONTIER)
    {
        workMap_.Clear();

        FeaturedMap& featMap2 = GetFeatureView(info.viewId2_);
        for (List<TileModifier >::ConstIterator mt=modifiers.Begin(); mt!=modifiers.End(); ++mt)
        {
            unsigned addr = mt->y_*width_ + mt->x_;
//            URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilter : TRANSFERFRONTIER ... check (x=%d y=%d) addr=%u feat1=%u(need=%u) feat2=%u(need<=%u)",
//                            mt->x_, mt->y_, addr, featMap1[addr], info.feature1_, featMap2[addr], info.featureBack_);
            if (featMap1[addr] == info.feature1_)
                if (featMap2[addr] <= info.featureBack_)
                    if (HasNeighBors4(featMap1, addr, MapFeatureType::NoMapFeature))
                        workMap_.Push(addr);
        }
        // transfer et remplace les cells trouvees
        for (unsigned i=0; i < workMap_.Size(); ++i)
        {
            featMap1[workMap_[i]] = info.featureReplace_;
            featMap2[workMap_[i]] = info.feature2_;
//            URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilter : TRANSFERFRONTIER ... addr=%u feat=%u to %u on viewId=%d and feat<=%u to %u  onviewId=%d ... OK !",
//                                    workMap_[i], info.feature1_, info.featureReplace_, info.feature2_, info.viewId1_, info.featureBack_, info.feature2_, info.viewId2_);
        }

        if (info.storeIndexes_)
        {
            for (unsigned i=0; i < workMap_.Size(); ++i)
                info.storeIndexes_->Push(workMap_[i]);
        }
    }
#ifdef DUMP_MAPDEBUG_APPLYFEATUREVIEW_MODIFIER
    URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilter - Filter[%u]=%s - After ApplyFilter !", id, FeatureFilterStr[info.filter_]);
#endif
}

bool ObjectFeatured::ApplyFeatureFilters(const List<TileModifier >& modifiers, HiresTimer* timer, const long long& delay)
{
    for (;;)
    {
        if (indexToSet_ >= featurefilters_.Size())
        {
//            URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters ... OK !");
            indexToSet_ = 0;
            return true;
        }

        ApplyFeatureFilter(modifiers, indexToSet_);

        indexToSet_++;

//        URHO3D_LOGINFOF("ObjectFeatured() - ApplyFeatureFilters ... filter=%u/%u ... timer=%d msec",
//                        indexToSet_, featurefilters_.Size(), timer ? timer->GetUSec(false) / 1000 : 0);

        if (TimeOver(timer))
            return false;
    }

    return true;
}

bool ObjectFeatured::ApplyFeatureFilters(HiresTimer* timer, const long long& delay)
{
    URHO3D_LOGDEBUGF("ObjectFeatured() - ApplyFeatureFilters ... ");

    for (;;)
    {
        if (indexToSet_ >= featurefilters_.Size())
        {
            URHO3D_LOGDEBUGF("ObjectFeatured() - ApplyFeatureFilters ... OK !");
            indexToSet_ = 0;
            return true;
        }

        ApplyFeatureFilter(indexToSet_);

        indexToSet_++;

//        URHO3D_LOGDEBUGF("ObjectFeatured() - ApplyFeatureFilters ... filter=%u/%u ... timer=%d msec",
//                        indexToSet_, featurefilters_.Size(), timer ? timer->GetUSec(false) / 1000 : 0);

        if (TimeOver(timer))
            return false;
    }

    return true;
}


bool ObjectFeatured::SetFluidCells(HiresTimer* timer, const long long& delay)
{
    URHO3D_LOGDEBUGF("ObjectFeatured() - SetFluidCells ... ");

    for (;;)
    {
        if (indexToSet_ >= fluidView_.Size())
        {
            URHO3D_LOGDEBUGF("ObjectFeatured() - SetFluidCells ... OK !");
            indexToSet_ = 0;
            return true;
        }

        fluidView_[indexToSet_].SetCells();

        indexToSet_++;

//        URHO3D_LOGDEBUGF("ObjectFeatured() - SetFluidCells ... fluidView=%u/%u ... timer=%d msec",
//                        indexToSet_, fluidView_.Size(), timer ? timer->GetUSec(false) / 1000 : 0);

        if (TimeOver(timer))
            return false;
    }

    return true;
}

bool ObjectFeatured::IsTotallyMasked(const FeaturedMap& mask, unsigned char dimension, unsigned addr) const
{
    switch (dimension)
    {
    case TILE_0:
    case TILE_1X1:
        return (mask[addr] > MapFeatureType::Threshold);
    case TILE_2X1:
        return (mask[addr] > MapFeatureType::Threshold && mask[addr+1] > MapFeatureType::Threshold);
    case TILE_1X2:
        return (mask[addr] > MapFeatureType::Threshold && mask[addr+width_] > MapFeatureType::Threshold);
    case TILE_2X2:
        return (mask[addr] > MapFeatureType::Threshold && mask[addr+width_] > MapFeatureType::Threshold &&
                mask[addr+1] > MapFeatureType::Threshold && mask[addr+width_+1] > MapFeatureType::Threshold);
    }

//  Slower version with check border (Prevent Crash but for this case the matter is elsewhere)
//    {
//    case TILE_0:
//    case TILE_1X1:
//        return (mask[addr] >= MapFeatureType::NoDecal);
//    case TILE_2X1:
//        return addr+1 < mask.Size() ? (mask[addr] >= MapFeatureType::NoDecal && mask[addr+1] >= MapFeatureType::NoDecal) : true;
//    case TILE_1X2:
//        return addr+width_ < mask.Size() ? (mask[addr] >= MapFeatureType::NoDecal && mask[addr+width_] >= MapFeatureType::NoDecal) : true;
//    case TILE_2X2:
//        return addr+width_ < mask.Size() ? (mask[addr] >= MapFeatureType::NoDecal && mask[addr+width_] >= MapFeatureType::NoDecal &&
//            mask[addr+1] >= MapFeatureType::NoDecal && mask[addr+width_+1] >= MapFeatureType::NoDecal) : true;
//    }

    return false;
}


bool ObjectFeatured::UpdateMaskViews(const TileGroup& tileGroup, HiresTimer* timer, const long long& delay, NeighborMode nghmode)
{
//    URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews : ... startTimer=%d", timer ? timer->GetUSec(false)/1000 : 0);

    const Vector<ConnectedMap>& connectedViews = map_->GetConnectedViews();
    const Vector<TiledMap>& tiledViews = map_->GetTiledViews();

    for (;;)
    {
        if (indexToSet_ >= ViewManager::GetNumViewZ())
            break;

        int viewZ = viewZs_[indexToSet_];
        const Vector<int>& viewIds = GetViewIDs(viewZ);
        unsigned numviews = viewIds.Size();

        if (!numviews || viewIds[0] == -1)
        {
            URHO3D_LOGERRORF("ObjectFeatured() - UpdateMaskViews : indexToSet=%u ... ERROR at viewZ=%d => DumpViewIds=%s DumpViewZs=%s!",
                             indexToSet_, viewZ, GetDumpViewIds().CString(), GetDumpViewZs().CString());
            indexToSet_++;
            continue;
        }

#ifdef DUMP_MAPDEBUG_MASKVIEW
        {
            String s;
            for (unsigned i=0; i < numviews; i++)
                s = s + " " + String(viewIds[i]);

            URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u numviews=%u : Dump ordered views id=%s ...", viewZ, numviews, s.CString());
        }
#endif

        Vector<FeaturedMap>& masks = GetMaskedViews(indexToSet_);
        unsigned numtiles = tileGroup.Size();
        unsigned addr;

        // copy base featureView to all Mask Views
        for (int i=numviews-1; i >= 0; i--)
        {
            FeaturedMap& maskcells = masks[i];
            FeaturedMap& features = GetFeatureView(viewIds[i]);
            for (unsigned j=0; j < numtiles; j++)
            {
                addr = tileGroup[j];
                maskcells[addr] = features[addr];
            }

#ifdef DUMP_MAPDEBUG_MASKVIEW
            if (i == numviews-1)
            {
                URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Copy of FeaturedMap ... Dump %u/%u - viewID=%d",
                                viewZ, i+1, numviews, viewIds[i]);
                GameHelpers::DumpData(&(maskcells[0]), 1, 2, width_, height_);
            }
#endif
        }

        if (numviews < 2)
        {
            indexToSet_++;
            continue;
        }

        //    URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Compute MaskedViews ...", viewZ);

        // Note : maskCell = MapFeatureType::NoRender => MaskedCell

        // Top Mask View
        if (numviews > 2 && nghmode != Connected0)
        {
            FeaturedMap& mask = masks[numviews-1];
            int viewID = viewIds[numviews-1];

            // if a border tile don't mask it
            for (unsigned j=0; j < numtiles; j++)
            {
                addr = tileGroup[j];
                if (connectedViews[viewID][addr] < MapTilesConnectType::AllConnect)
                    masks[numviews-3][addr] = MapFeatureType::NoRender;
            }
        }

        // Other Mask Views
        if (nghmode == Connected0)
        {
            for (int i=numviews-2; i >= 0 ; i--)
            {
                Tile *tile;
                int viewID = viewIds[i];
                int aboveViewID = viewIds[i+1];
                FeaturedMap& topMask = masks[i+1];
                FeaturedMap& mask = masks[i];

                unsigned numTileErrors = 0;

#ifdef DUMP_MAPDEBUG_MASKVIEW
                URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Compute MaskedView %u/%u - viewID=%d ... ",
                                viewZ, i+1, numviews, viewID);
#endif

                for (unsigned j=0; j < numtiles; j++)
                {
                    addr = tileGroup[j];

                    if (mask[addr] == MapFeatureType::NoRender)
                        continue;

                    if (topMask[addr] > MapFeatureType::Threshold)
                    {
                        tile = tiledViews[viewID][addr];

                        /// TODO : 12/09/2020 remove this code after finding the origin of the crash
                        if (!tile)
                        {
                            numTileErrors++;
                            mask[addr] = MapFeatureType::NoRender;
                            continue;
                        }

                        if (tile->GetDimensions() < TILE_RENDER)
                            // No Render Tile And No Decals
                            mask[addr] = MapFeatureType::NoRender;
                        else
                            // Check if can be totally masked or just No Decals
                            mask[addr] = IsTotallyMasked(topMask, tile->GetDimensions(), addr) ? MapFeatureType::NoRender : MapFeatureType::NoDecal;
                    }
                }

                /// TODO : 12/09/2020 remove this code after finding the origin of the crash
                if (numTileErrors)
                {
                    URHO3D_LOGERRORF("ObjectFeatured() - UpdateMaskViews : map=%s(%s) compute MaskedView %u/%u - viewZ=%u viewID=%d ... Num Tile Errors = %u !!",
                                     map_->GetMapPoint().ToString().CString(), mapStatusNames[map_->GetStatus()], i+1, numviews, viewZ, viewID, numTileErrors);
                }

#ifdef DUMP_MAPDEBUG_MASKVIEW
                GameHelpers::DumpData(&mask[0], 1, 2, width_, height_);
#endif
            }
        }
        else
        {
            for (int i=numviews-2; i >= 0 ; i--)
            {
                int aboveViewID = viewIds[i+1];
                FeaturedMap& topMask = masks[i+1];
                FeaturedMap& mask = masks[i];

#ifdef DUMP_MAPDEBUG_MASKVIEW
                URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Compute MaskedView %u/%u - viewID=%d ... ",
                                viewZ, i+1, numviews, viewIds[i]);
#endif

                for (unsigned j=0; j < numtiles; j++)
                {
                    addr = tileGroup[j];

                    if (mask[addr] == MapFeatureType::NoRender)
                    {
                        if (i != 0)
                            masks[i-1][addr] = MapFeatureType::NoRender;
                        continue;
                    }

                    // if a border tile don't mask it
                    if (connectedViews[aboveViewID][addr] < MapTilesConnectType::AllConnect)
                    {
                        if (i > 1)
                            masks[i-2][addr] = MapFeatureType::NoRender;

                        if (topMask[addr] == MapFeatureType::NoRender)
                            mask[addr] = MapFeatureType::NoRender;

                        continue;
                    }

                    if (topMask[addr] > MapFeatureType::Threshold)
                        mask[addr] = MapFeatureType::NoRender;
                }

#ifdef DUMP_MAPDEBUG_MASKVIEW
                GameHelpers::DumpData(&mask[0], 1, 2, width_, height_);
#endif
            }
        }

        indexToSet_++;

//        URHO3D_LOGDEBUGF("ObjectFeatured() - UpdateMaskViews : ... indexZ=%d/%d ... timer=%d msec", indexToSet_, ViewManager::GetNumViewZ(), timer ? timer->GetUSec(false)/1000 : 0);

        if (HalfTimeOver(timer))
            return false;
    }

//    URHO3D_LOGDEBUGF("ObjectFeatured() - UpdateMaskViews : ... timer=%d OK !", timer ? timer->GetUSec(false)/1000 : 0);
    indexToSet_ = 0;
    return true;
}

bool ObjectFeatured::UpdateMaskViews(HiresTimer* timer, const long long& delay, NeighborMode nghmode)
{
//    URHO3D_LOGDEBUGF("ObjectFeatured() - UpdateMaskViews : ... startTimer=%d", timer ? timer->GetUSec(false)/1000 : 0);

    if (!map_ || !map_->GetObjectSkinned())
        return true;

    const Vector<ConnectedMap>& connectedViews = map_->GetConnectedViews();
    const Vector<TiledMap>& tiledViews = map_->GetTiledViews();

    for (;;)
    {
        if (indexToSet_ >= ViewManager::GetNumViewZ())
            break;

        int viewZ = viewZs_[indexToSet_];
        const Vector<int>& viewIds = GetViewIDs(viewZ);
        unsigned numviews = viewIds.Size();

        if (!numviews || viewIds[0] == -1)
        {
            URHO3D_LOGERRORF("ObjectFeatured() - UpdateMaskViews : indexToSet=%u ... ERROR at viewZ=%d => DumpViewIds=%s DumpViewZs=%s!",
                             indexToSet_, viewZ, GetDumpViewIds().CString(), GetDumpViewZs().CString());
            indexToSet_++;
            continue;
        }

#ifdef DUMP_MAPDEBUG_MASKVIEW
        {
            String s;
            for (unsigned i=0; i < numviews; i++)
                s = s + " " + String(viewIds[i]);

            URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u numviews=%u : Dump ordered views id=%s ...", viewZ, numviews, s.CString());
        }
#endif

        Vector<FeaturedMap>& masks = GetMaskedViews(indexToSet_);
        unsigned numtiles = width_*height_;

        // copy base featureView to all Mask Views
        for (int i=numviews-1; i >= 0; i--)
        {
            FeaturedMap& maskcells = masks[i];
            FeaturedMap& features = GetFeatureView(viewIds[i]);
            for (unsigned j=0; j < numtiles; j++)
            {
                maskcells[j] = features[j];
            }

#ifdef DUMP_MAPDEBUG_MASKVIEW
            if (i == numviews-1)
            {
                URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Copy of FeaturedMap ... Dump %u/%u - viewID=%d",
                                viewZ, i+1, numviews, viewIds[i]);
                GameHelpers::DumpData(&(maskcells[0]), 1, 2, width_, height_);
            }
#endif
        }

        if (numviews < 2)
        {
            indexToSet_++;
            continue;
        }

        //URHO3D_LOGDEBUGF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Compute MaskedViews ...", viewZ);

        // Note : maskCell = MapFeatureType::NoRender => MaskedCell

        // Top Mask View
        if (numviews > 2 && nghmode != Connected0)
        {
            FeaturedMap& mask = masks[numviews-1];
            int viewID = viewIds[numviews-1];

            // if a border tile don't mask it
            for (unsigned j=0; j < numtiles; j++)
            {
                if (connectedViews[viewID][j] < MapTilesConnectType::AllConnect)
                    masks[numviews-3][j] = MapFeatureType::NoRender;
            }
        }

        // Other Mask Views
        if (nghmode == Connected0)
        {
            for (int i=numviews-2; i >= 0 ; i--)
            {
                Tile *tile;
                int viewID = viewIds[i];
                int aboveViewID = viewIds[i+1];
                FeaturedMap& topMask = masks[i+1];
                FeaturedMap& mask = masks[i];

                unsigned numTileErrors = 0;

#ifdef DUMP_MAPDEBUG_MASKVIEW
                URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Compute MaskedView %u/%u - viewID=%d ... ",
                                viewZ, i+1, numviews, viewIds[i]);
#endif

                for (unsigned j=0; j < numtiles; j++)
                {
                    if (mask[j] == MapFeatureType::NoRender)
                        continue;

                    if (topMask[j] > MapFeatureType::Threshold)
                    {
                        tile = tiledViews[viewID][j];
                        /// TODO : 12/09/2020 remove this code after finding the origin of the crash
                        if (!tile)
                        {
                            numTileErrors++;
                            mask[j] = MapFeatureType::NoRender;
                            continue;
                        }
                        if (tile->GetDimensions() < TILE_RENDER)
                            // No Render Tile And No Decals
                            mask[j] = MapFeatureType::NoRender;
                        else
                            // Check if can be totally masked or just No Decals
                            mask[j] = IsTotallyMasked(topMask, tile->GetDimensions(), j) ? MapFeatureType::NoRender : MapFeatureType::NoDecal;
                    }
                }

                /// TODO : 12/09/2020 remove this code after finding the origin of the crash
                if (numTileErrors)
                {
                    URHO3D_LOGERRORF("ObjectFeatured() - UpdateMaskViews : map=%s(%s) compute MaskedView %u/%u - viewZ=%u viewID=%d ... Num Tile Errors = %u !!",
                                     map_->GetMapPoint().ToString().CString(), mapStatusNames[map_->GetStatus()], i+1, numviews, viewZ, viewID, numTileErrors);
                }

#ifdef DUMP_MAPDEBUG_MASKVIEW
                GameHelpers::DumpData(&mask[0], 1, 2, width_, height_);
#endif
            }
        }
        else
        {
            for (int i=numviews-2; i >= 0 ; i--)
            {
                int aboveViewID = viewIds[i+1];
                FeaturedMap& topMask = masks[i+1];
                FeaturedMap& mask = masks[i];

#ifdef DUMP_MAPDEBUG_MASKVIEW
                URHO3D_LOGINFOF("ObjectFeatured() - UpdateMaskViews viewZ=%u : Compute MaskedView %u/%u - viewID=%d ... ",
                                viewZ, i+1, numviews, viewIds[i]);
#endif

                for (unsigned j=0; j < numtiles; j++)
                {
                    if (mask[j] == MapFeatureType::NoRender)
                    {
                        if (i != 0)
                            masks[i-1][j] = MapFeatureType::NoRender;
                        continue;
                    }

                    // if a border tile don't mask it
                    if (connectedViews[aboveViewID][j] < MapTilesConnectType::AllConnect)
                    {
                        if (i > 1)
                            masks[i-2][j] = MapFeatureType::NoRender;

                        if (topMask[j] == MapFeatureType::NoRender)
                            mask[j] = MapFeatureType::NoRender;

                        continue;
                    }

                    if (topMask[j] > MapFeatureType::Threshold)
                        mask[j] = MapFeatureType::NoRender;
                }

#ifdef DUMP_MAPDEBUG_MASKVIEW
                GameHelpers::DumpData(&mask[0], 1, 2, width_, height_);
#endif
            }
        }

        indexToSet_++;

//        URHO3D_LOGDEBUGF("ObjectFeatured() - UpdateMaskViews : ... indexZ=%d/%d ... timer=%d msec", indexToSet_, ViewManager::GetNumViewZ(), timer ? timer->GetUSec(false)/1000 : 0);

        if (HalfTimeOver(timer))
            return false;
    }

//    URHO3D_LOGDEBUGF("ObjectFeatured() - UpdateMaskViews : ... timer=%d OK !", timer ? timer->GetUSec(false)/1000 : 0);
    indexToSet_ = 0;
    return true;
}


unsigned ObjectFeatured::GetNumViews() const
{
    return featuredView_.Size();
}

FeaturedMap& ObjectFeatured::GetFeatureView(unsigned id)
{
    assert(id < featuredView_.Size());

    return featuredView_[id];
}

FluidDatas& ObjectFeatured::GetFluidView(unsigned indexFluidZ)
{
    assert(indexFluidZ < fluidView_.Size());

    return fluidView_[indexFluidZ];
}

FluidDatas& ObjectFeatured::GetFluidViewByZValue(int Z)
{
    Vector<int>::ConstIterator it = ViewManager::GetFluidZ().Find(Z);
    return fluidView_[it - ViewManager::GetFluidZ().Begin()];
}

Vector<FeaturedMap>& ObjectFeatured::GetMaskedViews(unsigned indexZ)
{
    assert(indexZ < maskedView_.Size());

    return maskedView_[indexZ];
}

FeaturedMap& ObjectFeatured::GetMaskedView(unsigned indexZ, unsigned indexView)
{
    assert(indexZ < maskedView_.Size() && indexView < maskedView_[indexZ].Size());

    return maskedView_[indexZ][indexView];
}

void ObjectFeatured::GetAllViewFeatures(unsigned tileindex, PODVector<FeatureType>& savedfeatures)
{
    savedfeatures.Resize(featuredView_.Size());
    for (unsigned i=0; i < featuredView_.Size(); i++)
        savedfeatures[i] = featuredView_[i][tileindex];
}

void ObjectFeatured::SortViews()
{
    viewIds_.Clear();

//    URHO3D_LOGDEBUGF("ObjectFeatured() - SortViews : ... DumpViewZs=%s ...", GetDumpViewZs().CString());

    for (HashMap<unsigned, unsigned>::ConstIterator it = viewZ_.Begin(); it != viewZ_.End(); ++it)
    {
        int viewZ = it->second_;

//        URHO3D_LOGDEBUGF("ObjectFeatured() - SortViews : sorting viewZ=%d ...", viewZ);
#ifdef SORTSINGLE
        SortViewsIds_Last(viewZ);
#else
        SortViewsIds_All(viewZ);
#endif

        const Vector<int>& viewIds = GetViewIDs(viewZ);

        if (!viewIds.Size() || viewIds[0] == -1)
        {
            URHO3D_LOGERRORF("ObjectFeatured() - SortViews : ERROR at viewZ=%d => DumpViewIds=%s !",
                             viewZ, GetDumpViewId(viewZ).CString());

            int nearestViewZ = GetNearestViewZ(viewZ);
            viewIds_[viewZ] = GetViewIDs(nearestViewZ);
        }

        URHO3D_LOGDEBUGF("ObjectFeatured() - SortViews : viewZ=%d => DumpViewIds=%s !", viewZ, GetDumpViewId(viewZ).CString());
    }

//    URHO3D_LOGDEBUGF("ObjectFeatured() - SortViews : ... DumpViewIDs=%s", GetDumpViewIds().CString());
}

void ObjectFeatured::SortViewsIds_Last(unsigned viewZ)
{
    Vector<int>& viewIds = viewIds_[viewZ];

    viewIds.Clear();
    unsigned numIds = viewZ_.Size();

    if (numIds >= 1)
    {
        unsigned lastid = 0;
        if (numIds > 1)
        {
            unsigned lastViewZ = viewZ_[0];
            // Get the id view with the maximal Z <= viewZ
            for (unsigned i = 1; i < numIds; i++)
            {
                if (viewZ_[i] > lastViewZ && viewZ_[i] <= viewZ)
                {
                    lastViewZ = viewZ_[i];
                    lastid = i;
                }
            }
        }
        viewIds.Push(lastid);
    }

    if (numIds == 0)
        viewIds.Push(NOVIEW);
}

void ObjectFeatured::SortViewsIds_All(unsigned viewZ)
{
    Vector<int>& viewIds = viewIds_[viewZ];

    unsigned numIds = viewZ_.Size();
//    int numIds = numviews_;
    viewIds.Clear();
    viewIds.Reserve(numIds);

//    URHO3D_LOGDEBUGF("ObjectFeatured() - SortViewsIds_All : viewZ=%d numMaxViewZ=%d ...", viewZ, numIds);

    unsigned minViewZ = 0, lastViewZ;
    int lastid = -1;
    for (unsigned i=0; i < numIds; i++)
    {
        if (viewZ_[i] > viewZ)
            continue;

        lastViewZ = viewZ;

        // Get the id view with the minimal Z (sort ascending order)
        int id = 0;
        for (unsigned j=0; j < numIds; j++)
        {
            if (viewZ_[j] < minViewZ)
                continue;

            if (viewZ_[j] <= lastViewZ)
            {
                id = j;
                lastViewZ = viewZ_[j];
            }
        }
//        URHO3D_LOGDEBUGF("ObjectFeatured() - SortViewsIds_All : pass=%d id=%d minz=%d lastz=%d...", i, id, minViewZ, lastViewZ);

        minViewZ = lastViewZ+1;

        if (id == lastid)
            break;

        viewIds.Push(id);
        lastid = id;
    }

    if (!viewIds.Size())
        viewIds.Push(NOVIEW);

    viewIds.Compact();

    numIds = viewIds.Size();
    for (int i=0; i < numIds; i++)
        URHO3D_LOGDEBUGF("ObjectFeatured() - SortViewsIds_All : Dump i=%d/%d viewId=%d viewZ=%d", i+1, numIds, viewIds[i], viewZ_[viewIds[i]]);

    URHO3D_LOGDEBUGF("ObjectFeatured() - SortViewsIds_All : ... OK !", viewZ);
}

const Vector<int>& ObjectFeatured::GetViewIDs(unsigned viewZ)
{
    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Find(viewZ);

    if (it == viewIds_.End() || it->second_.At(0) == -1)
    {
        URHO3D_LOGERRORF("ObjectFeatured() - GetViewIDs : ERROR at viewZ=%d => DumpViewIds=%s !",
                         viewZ, GetDumpViewId(viewZ).CString());

        SortViewsIds_All(viewZ);
//        int nearestViewZ = GetNearestViewZ(viewZ);
//
//        Vector<int>& viewids = viewIds_[viewZ];
//
//        if (nearestViewZ > 0)
//        {
//            viewids = GetViewIDs(nearestViewZ);
//        }
//        else
//        {
//            viewids.Push(0);
//        }
//
//        URHO3D_LOGDEBUGF("ObjectFeatured() - GetViewIDs : viewZ=%d => patch use nearestviewz=%d DumpViewIds=%s !", viewZ, nearestViewZ, GetDumpViewId(viewZ).CString());

        return viewIds_[viewZ];
    }

    return it->second_;
}

int ObjectFeatured::GetViewId(unsigned viewZ) const
{
    assert(viewIds_.Size() > 0);
    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Find(viewZ);
    return it != viewIds_.End() ? it->second_.Back() : -1;
//    return GetViewIDs(viewZ).Back();
}

int ObjectFeatured::GetViewZ(unsigned viewid) const
{
    assert(viewZ_.Size() > 0);
    HashMap<unsigned, unsigned>::ConstIterator it = viewZ_.Find(viewid);
    return it != viewZ_.End() ? (int)it->second_ : -1;
}

int ObjectFeatured::GetRealViewZAt(unsigned viewZ) const
{
    assert(viewIds_.Size() > 0);
    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Find(viewZ);
    if (it == viewIds_.End())
        return NOVIEW;

    int viewid = it->second_.Back();

    if (viewid == NOVIEW)
        return NOVIEW;

    return GetViewZ(viewid);
}

unsigned ObjectFeatured::GetNearestViewZ(unsigned layerZ) const
{
    assert(viewIds_.Size() > 0);
    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Begin();
    HashMap<unsigned, Vector<int> >::ConstIterator jt = it;
    for (it = viewIds_.Begin(); it != viewIds_.End(); ++it)
    {
        if (it->first_ <= layerZ && it->first_ > jt->first_)
            jt = it;
    }
    return jt->first_;
}

unsigned ObjectFeatured::GetNearestViewId(unsigned layerZ) const
{
    assert(viewIds_.Size() > 0);
    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Begin();
    HashMap<unsigned, Vector<int> >::ConstIterator jt = it;
    for (it = viewIds_.Begin(); it != viewIds_.End(); ++it)
    {
        if (it->first_ <= layerZ && it->first_ > jt->first_)
            jt = it;
    }
    return jt->second_.Back();
}

int ObjectFeatured::GetColliderIndex(unsigned viewZ) const
{
    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Find(viewZ);
    return it != viewIds_.End() ? it->second_.Size()-1 : -1;
}

int ObjectFeatured::GetColliderIndex(unsigned viewZ, int viewid) const
{
    if (viewid < 0)
        return -1;

    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Find(viewZ);
    if (it == viewIds_.End())
        return -1;

    const Vector<int>& viewids = it->second_;
    Vector<int>::ConstIterator jt = viewids.Find(viewid);
    return jt != viewids.End() ? jt-viewids.Begin() : -1;
}

const Vector<int>& ObjectFeatured::GetFluidViewIDs(unsigned viewZ)
{
    return fluidViewIds_[viewZ];
}

int ObjectFeatured::GetFluidViewId(unsigned viewZ)
{
    return GetFluidViewIDs(viewZ).Front();
}

String ObjectFeatured::GetDumpViewZs() const
{
    String s;
    for (HashMap<unsigned, unsigned>::ConstIterator it=viewZ_.Begin(); it != viewZ_.End(); ++it)
    {
        s += "\n";
        s += "viewid=";
        s += String(it->first_);
        s += " => viewZ = ";
        s += String(it->second_);
    }
    return s;
}

String ObjectFeatured::GetDumpViewId(unsigned viewZ) const
{
    String s;

    HashMap<unsigned, Vector<int> >::ConstIterator it = viewIds_.Find(viewZ);
    if (it != viewIds_.End())
    {
        s += "viewZ=";
        s += String(it->first_);
        s += " => viewids = ";
        const Vector<int>& viewids = it->second_;
        for (Vector<int>::ConstIterator it=viewids.Begin(); it != viewids.End(); ++it)
        {
            s += String(*it);
            s += " ";
        }
    }
    else
    {
        s = "no ViewIDs for viewZ=";
        s += String(viewZ);
    }

    return s;
}

String ObjectFeatured::GetDumpViewIds() const
{
    String s;

    if (viewIds_.Size())
    {
        for (HashMap<unsigned, Vector<int> >::ConstIterator it=viewIds_.Begin(); it != viewIds_.End(); ++it)
        {
            s += "\n";
            s += "viewZ=";
            s += String(it->first_);
            s += " => viewids = ";
            const Vector<int>& viewids = it->second_;
            for (Vector<int>::ConstIterator it=viewids.Begin(); it != viewids.End(); ++it)
            {
                s += String(*it);
                s += " ";
            }
        }
    }
    else
        s = "noViewIds";

    return s;
}


