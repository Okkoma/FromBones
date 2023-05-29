#pragma once

#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Urho2D/Drawable2D.h>

#include "DefsMap.h"
#include "DefsChunks.h"
#include "DefsFluids.h"


const int REPLACEFEATURE        = 1;
const int REPLACEFEATUREBACK    = 2;
const int REPLACEFRONTIER       = 3;
const int COPYTILEMODIFIER      = 4;
const int TRANSFERFRONTIER      = 5;
const int REPLACEFEATUREBACK2   = 6;

#define ConnectedMapID  0
#define FeaturedMapID   1

class World2DInfo;
struct DungeonInfo;


struct ObjectFeatured : public RefCounted
{
    ObjectFeatured();
    ObjectFeatured(const ObjectFeatured& t) { }
    ObjectFeatured(unsigned width, unsigned height, unsigned numviews=0);
    ~ObjectFeatured();

    static void SetViewZs(const Vector<int>& views);
    static int GetIndexViewZ(int viewZ);

    void Clear();
    void PullFluids();
    void Resize(unsigned width, unsigned height, unsigned numviews=0);
    void Copy(int left, int top, ObjectFeatured& object);

    /// Setters
//    void SetViewConfiguration(bool worldground, bool dungeon, DungeonInfo* dinfo=0);
    void SetViewConfiguration(int modeltype);
    void LinkViewIdToViewZ(unsigned viewid, unsigned zValue);
    void CopyFeatureView(unsigned viewidfrom, unsigned viewid);
    unsigned AddFeatureView(unsigned zValue, const FeatureType* featureView, bool hasFluidView);
    void SetFluidView(unsigned featureViewId, int checkViewId, unsigned zValue);
    int LinkFluidViewToZView(unsigned zValue1, unsigned zValue2);

    /// Set Feature Filters
    void AddFeatureFilter(unsigned filterId, unsigned viewId, FeatureType feature);
    void AddFeatureFilter(unsigned filterId, unsigned viewId, FeatureType feature1, FeatureType feature2);
    void AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2);
    void AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2, FeatureType featureBack, Vector<unsigned>* storeIndexes=0);
    void AddFeatureFilter(unsigned filterId, unsigned viewId1, unsigned viewId2, FeatureType feature1, FeatureType feature2, FeatureType featureBack, FeatureType featureReplace, Vector<unsigned>* storeIndexes=0);

    /// Apply Feature Filters
    void ApplyFeatureFilter(unsigned id);
    void ApplyFeatureFilter(const List<TileModifier >& modifiers, unsigned id);
    bool ApplyFeatureFilters(unsigned tileindex);
    bool ApplyFeatureFilters(const List<TileModifier >& modifiers, HiresTimer* timer=0, const long long& delay=0);
    bool ApplyFeatureFilters(HiresTimer* timer=0, const long long& delay=0);

    bool SetFluidCells(HiresTimer* timer=0, const long long& delay=0);

    /// MaskView Updaters
    bool IsTotallyMasked(const FeaturedMap& mask, unsigned char dimension, unsigned addr) const;
    bool UpdateMaskViews(const TileGroup& tileGroup, HiresTimer* timer, const long long& delay, NeighborMode nghmode);
    bool UpdateMaskViews(HiresTimer* timer, const long long& delay, NeighborMode nghmode);

    /// Getters
    unsigned GetNumViews() const;
    FeaturedMap& GetTerrainMap()
    {
        return terrainMap_;
    }
    FeatureType GetTerrainValue(unsigned addr) const
    {
        return terrainMap_[addr];
    }
    FeaturedMap& GetBiomeMap()
    {
        return biomeMap_;
    }
    FeaturedMap& GetFeatureView(unsigned id);
    FluidDatas& GetFluidViewByZValue(int viewZ);
    FluidDatas& GetFluidView(unsigned indexFluidZ);
    Vector<FeaturedMap>& GetMaskedViews(unsigned indexZ);
    FeaturedMap& GetMaskedView(unsigned indexZ, unsigned indexView);
    void GetAllViewFeatures(unsigned tileindex, PODVector<FeatureType>& savedfeatures);
    bool HasNeighBors4(const FeaturedMap& featureMap, unsigned addr, FeatureType feature) const;

    /// ViewIds Getters
    void SortViews();
    void SortViewsIds_Last(unsigned viewZ);
    void SortViewsIds_All(unsigned viewZ);

    const Vector<int>& GetViewIDs(unsigned viewZ);
    int GetViewId(unsigned viewZ) const;
    int GetViewZ(unsigned viewid) const;
    int GetRealViewZAt(unsigned viewZ) const;
    unsigned GetNearestViewZ(unsigned layerZ) const;
    unsigned GetNearestViewId(unsigned layerZ) const;
    int GetColliderIndex(unsigned viewZ) const;
    int GetColliderIndex(unsigned viewZ, int viewid) const;
    inline unsigned GetTileIndex(int x, int y) const
    {
        return y * width_ + x;
    }

    const Vector<int>& GetFluidViewIDs(unsigned viewZ);
    int GetFluidViewId(unsigned viewZ);

    String GetDumpViewZs() const;
    String GetDumpViewId(unsigned viewZ) const;
    String GetDumpViewIds() const;

    MapBase* map_;

    unsigned width_;
    unsigned height_;
    unsigned numviews_;
    unsigned indexToSet_;

    short int nghTable4_[4];
    short int nghCorner_[4];
    short int nghTable8_[8];

    /// correspondance des viewId et des viewZ
    HashMap<unsigned, unsigned> viewZ_;
    /// table des id feature views (tries suivant Z ascendant) pour chaque zview
    HashMap<unsigned, Vector<int> > viewIds_;
    /// table des feature filters pour chaque view (key1=viewID key2=filterID data=FeatureInfos)
//    HashMap<unsigned, HashMap<unsigned, Vector<FeatureFilterInfo> > > featurefilters_;
    Vector<FeatureFilterInfo > featurefilters_;

    /// container des featureView par viewId
    Vector<FeaturedMap> featuredView_;
    /// containers des maskView par index Zview et viewId
    Vector<Vector<FeaturedMap> > maskedView_;

    FeaturedMap terrainMap_;
    FeaturedMap biomeMap_;

    /// container des fluidView par fluidViewId
    Vector<FluidDatas> fluidView_;
    /// table des id fluid views pour chaque zview standard
    HashMap<unsigned, Vector<int> > fluidViewIds_;

    /// table des viewZ standard
    static Vector<int> viewZs_;
};
