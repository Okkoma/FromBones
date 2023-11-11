#pragma once

namespace Urho3D
{
class CollisionShape2D;
}

using namespace Urho3D;


class MapBase;
class World2DInfo;
class ViewManager;
class TerrainAtlas;
struct MapCollider;


class MapColliderGenerator
{
public:
    MapColliderGenerator(World2DInfo* info);

    void SetParameters(bool findholes, bool shrink=false, int debugphysic=-1, int debugrender=-1);
    void SetSize(int width, int height, int maxheight);

    void ActiveDebugTraceOnce() { debugTraceOn_ = true; }

    void ResetMapGeneration(MapBase* map)
    {
        if (map_ == map) map_ = 0;
    }

    bool GeneratePhysicCollider(MapBase* map, HiresTimer* timer, PhysicCollider& collider, int specificwidth=0, int specificheight=0, const Vector2& center=Vector2::ZERO, bool createPlateform=true);
    bool GenerateRenderCollider(MapBase* map, HiresTimer* timer, RenderCollider& collider, const Vector2& center=Vector2::ZERO);

    void GetUpdatedBlockBoxes(const PhysicCollider& collider, const int x, const int y, PODVector<unsigned>& addedBlocks, PODVector<unsigned>& removedBlocks);

    static void Initialize(World2DInfo* info)
    {
        if (!generator_) generator_ = new MapColliderGenerator(info);
    }
    static void Reset()
    {
        if (generator_)
        {
            delete generator_;
            generator_ = 0;
        }
    }

    static MapColliderGenerator* Get()
    {
        return generator_;
    }

private:
    unsigned GetTileIndex(unsigned BoundaryTileIndex) const;
    bool GenerateWorkMatrix(MapCollider* collider, HiresTimer* timer, const FeaturedMap& view, MapColliderMode mode, bool createPlateform);
    bool GenerateContours(MapCollider* collider, HiresTimer* timer, unsigned blockID);
    bool GenerateBlockInfos(PhysicCollider& collider, HiresTimer* timer, unsigned blockID);
    bool GenerateBlockInfos(RenderCollider& collider, HiresTimer* timer);

    void GetStartBlockPoint(unsigned* refmap, unsigned* mapblock, const unsigned& refMark, const unsigned& patternMark, const unsigned& index, unsigned& startPoint, unsigned& startBackTrackPoint);
    void TraceContours_MooreNeighbor(Matrix2D<unsigned>& refmap, const unsigned& refMark, const unsigned& patternMark, Vector<PODVector<Vector2> >& contourVertices, Vector<PODVector<InfoVertex> >* infoVertices);

    void AddPointsToSegments (PODVector<Vector2>& segments, PODVector<InfoVertex>* infoVertex, unsigned boundaryTile, int fromNeighbor, int toNeighbor, unsigned blockID);
    void CloseContour        (PODVector<Vector2>& segments, PODVector<InfoVertex>* infoVertex);

    bool FindPatternInClosedContour(Matrix2D<unsigned>& matrix, const IntRect& border, const unsigned& pattern, const unsigned& borderPattern, int& x, int& y);
    bool FillClosedContourInArea(Matrix2D<unsigned>& matrix, const IntRect& border, const unsigned& patternToFill, const unsigned& borderPattern, const unsigned& fillPattern);

    bool MarkHolesInArea(Matrix2D<unsigned>& matrix, const IntRect& border, const unsigned& borderPattern, const unsigned& markpattern);

    bool Has4NeighBors(const FeaturedMap& featureMap, int x, int y) const;

    // VAR
    MapBase* map_;
    World2DInfo* info_;
    ViewManager* viewManager_;
    TerrainAtlas* atlas_;

    unsigned width_, height_, maxheight_;
    float tileWidth_, tileHeight_;
    Vector2 center_;
    int indexToSet_;

    // BlockMap Stuff
    int neighborOffsets[8];
    unsigned blockMapWidth_, blockMapHeight_, numBlocks_;

    static const unsigned blockMapBlockedTileID;
    static const unsigned BlockBoxID;
    static const unsigned BlockChainID;
    static const unsigned BlockEdgeID;
    static const unsigned BlockHoleID;
    static const unsigned BlockCheckID;
    static const unsigned VectorCapacity;

    Matrix2D<unsigned> checkBuffer_;
    Matrix2D<unsigned> blockMap_;
    Matrix2D<unsigned> holeMap_;
    Stack<unsigned> collStack_;
    Vector<IntRect> contourBorder_;
    PODVector<Vector2> contour_;

    bool& forcedShapeType_;
    ColliderShapeTypeMode& shapeType_;
    long long& delayUpdateUsec_;
    bool findHoles_, holesfounded_, shrinkContours_;
    int debuglevel_, debuglevelphysic_, debuglevelrender_;

    bool debugTraceOn_;
    static MapColliderGenerator* generator_;
};

