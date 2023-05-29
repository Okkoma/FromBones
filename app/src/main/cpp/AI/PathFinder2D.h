#pragma once

#include "DefsMove.h"
#include "Map.h"

#define DEBUG_PATH


class World2DInfo;

using namespace Urho3D;


enum PathStatus
{
    NoPathFound = -1,
    PathFound = 1,
    PathPartiallyFound = 2,
    PathReady = 3,
    PathInUse = 4,
    PathFinished = 5
};

class PathNode
{
public :
    PathNode() {}

    PathNode(short unsigned x, short unsigned y, short unsigned v, Map* map)
        : map_(map), x_(x), y_(y), v_(v), g_(0), parentmap_(0) { }

    PathNode(const PathNode& p)
        : map_(p.map_), x_(p.x_), y_(p.y_), v_(p.v_), g_(p.g_), f_(p.f_), parentx_(p.parentx_), parenty_(p.parenty_), parentv_(p.parentv_), parentmap_(p.parentmap_) { }

    /// Assign from another vector.
    PathNode& operator = (const PathNode& rhs)
    {
        map_ = rhs.map_;
        x_ = rhs.x_;
        y_ = rhs.y_;
        v_ = rhs.v_;
        g_ = rhs.g_;
        f_ = rhs.f_;
        parentx_ = rhs.parentx_;
        parenty_ = rhs.parenty_;
        parentv_ = rhs.parentv_;
        parentmap_= rhs.parentmap_;
        return *this;
    }

    bool operator == (const PathNode& rhs) const
    {
        return map_ == rhs.map_ && x_ == rhs.x_ && y_ == rhs.y_ && v_ == rhs.v_;
    }
    bool operator != (const PathNode& rhs) const
    {
        return map_ != rhs.map_ || x_ != rhs.x_ || y_ != rhs.y_ || v_ != rhs.v_;
    }

    Map* map_;
    short unsigned x_;
    short unsigned y_;
    short unsigned v_;
    short unsigned g_;
    short unsigned f_; // f_ = g_ + heuristic
    short unsigned parentx_;
    short unsigned parenty_;
    short unsigned parentv_;
    Map* parentmap_;

    static const PathNode EMPTY;
    static short unsigned width;
};

struct Path2D
{
    short unsigned id_;
    short unsigned numNodes_;
    short unsigned numActiveUsers_;
    short int status_;

    Vector<Node*> users_;
    Vector<short unsigned> indexes_;

    PODVector<Vector2> points_;
    PODVector<Vector2> segments_;
};

typedef short unsigned (*PathFunc)(const PathNode& a, const PathNode& b);



class PathFinder2D : public Object
{
    URHO3D_OBJECT(PathFinder2D, Object);

public :
    static void Init(Context* context, World2DInfo* info);
    static void Stop();

    static int FindPath(const Vector2& startpos, int v1, const Vector2& endpos, int v2, unsigned moveTypeFlags, bool partially=false, Map* map = 0);

    static Path2D* GetPath(int idpath);
    static int SetNodeOnPath(int idpath, Node* node);
    static int GetCurrentPathIndex(Path2D* path, int iuser);
    static int GetPathIndex(Path2D* path, int iuser, const Vector2& currentPosition);
    static int GetLastPathIndex(Path2D * path);
    static const Vector2& GetPathPosition(Path2D* path, int index);
    static const Vector2& GetPathSegment(Path2D* path, int index);
    static bool IsOnLastPathSegment(Path2D* path, int index);
    static bool IsPathFinished(int idpath);
    static void ClearPaths()
    {
        if (pathfinder_) pathfinder_->Free();
    }

    PathFinder2D(Context* context, World2DInfo* info);
    virtual ~PathFinder2D();

/// Search a Path : return an idpath if found or NoPathFound(-1)
    int SearchPath(const Vector2& startpos, int v1, const Vector2& endpos, int v2, unsigned moveTypeFlags, bool partially=false, Map* map = 0);

/// Handle Path Functions
    Path2D* GetStoredPath(int idpath)
    {
        return storedPaths_[idpath];
    }
    int AffectNodeOnPath(int idpath, Node* node);
//    const Vector2& GetNextPathNode(int idpath, Node* node, const Vector2& currentPosition, float velocity=0.f);
    void FreeUserSlotInPath(Path2D* path, int iuser);
    int FindFirstActiveUserInPath(Path2D* path);
    int FindUserSlotInPath(Path2D* path, Node* node);
    int GetUserSlotInPath(Path2D* path, Node* node);

    short unsigned GetLastFcost()
    {
        return lastPathNode_.map_ ? lastPathNode_.f_ : 0;
    }
    short unsigned GetHeuristic(const PathNode& sStart, const PathNode& sEnd, unsigned moveTypeFlags);

    void DumpPath(int idpath) const;

    static void DrawDebugGeometry(DebugRenderer* debugRenderer);
#ifdef DEBUG_PATH
    void DrawDebug(DebugRenderer* debugRenderer);
#endif // DEBUG_PATH

private :
    void Init();
    void Free();

    // a* functions for FindPath
    int Search_Astar(PathNode& sStart, PathNode& sEnd, unsigned moveTypeFlags, bool partially);
    void ComputeCost_Astar(PathNode* a, PathNode* b, PathNode* sEnd, PathFunc heuristic, PathFunc nghbDistance);
    void GetNeighbors(const PathNode& a);
    inline bool isPointOnSameLine(int px, int py, int startx, int starty, int endx, int endy);
    int ConstructPath();   // use lastPathNode_ and closed_ table

    short unsigned layerWidth_;
    short unsigned layerHeight_;
    short unsigned layerSize_;
    Vector2 tileCenter_;
    unsigned areaflag_;

    // table for a* search
    Vector<PathNode> open_;      // sorted via QuickSort (with compare PathNode function)
    Vector<PathNode> closed_;
    PathNode lastPathNode_;

    // local data for GetNeighbors
    short unsigned numNeighbors_;
    PathNode neighbors_[8];

    // handle path var
    unsigned ibuff_[1024];       // buffer for ConstructPath
    HashMap<Node*, int> nodeUsingPath_;
    Vector<Path2D*> storedPaths_;
    List<Path2D*> freePaths_;

    static World2DInfo* info_;
    static PathFinder2D* pathfinder_;
};

// Compare for Descending Sort higher cost to lower cost
// see Sort.h (QuickSort)
// get the last element for lower cost : Back then Pop the table
static inline bool ComparePathNode(const PathNode& i, const PathNode& j)
{
    return (i.f_ > j.f_);
}


#define COST_NOWAY          65535
#define COST_X	            10
#define COST_XY	            14
#define COST_DXY            4

static inline short unsigned Heuristic_fly(const PathNode& a, const PathNode& b)
{
//    // Manhattan : num block horizontal and vertical between a and b
//    return 10 * (Abs(a.x_ - b.x_) + Abs(a.y_ - b.y_));

    // Octile
    short unsigned dx = (a.x_ > b.x_)?(a.x_ - b.x_):(b.x_ - a.x_);
    short unsigned dy = (a.y_ > b.y_)?(a.y_ - b.y_):(b.y_ - a.y_);
    return (dx > dy) ? (dx*COST_X + dy*COST_DXY) : (dy*COST_X + dx*COST_DXY);
}

static inline short unsigned NghbDist_fly(const PathNode& a, const PathNode& b)
{
    int dx = Abs(a.x_ - b.x_);
    int dy = Abs(a.y_ - b.y_);

    if (dx + dy == 1)
    {
        return COST_X;
    }
    else if (dx + dy == 2)
    {
        return COST_XY;
    }

    return 0;
}

#define COST_WALK_YMID	45
#define COST_WALK_DXMID	35
#define COST_WALK_YUP	60
#define COST_WALK_YDOWN	9
#define COST_WALK_XYUP	80
#define COST_WALK_XYDOWN	13

//#define COST_WALK_YMID	    26
//#define COST_WALK_DXMID	    16
//#define COST_WALK_YUP	    40
//#define COST_WALK_YDOWN	    8
//#define COST_WALK_XYUP	    50
//#define COST_WALK_XYDOWN	12

static inline short unsigned Heuristic_walker(const PathNode& a, const PathNode& b)
{
//    // Manhattan : num block horizontal and vertical between a and b
//    return 10 * Abs(a.x_ - b.x_) + (a.y_ - b.y_ > 0 ? 20 : 10) * Abs(a.y_ - b.y_);

    // Octile
    short unsigned dx = (a.x_ > b.x_)?(a.x_ - b.x_):(b.x_ - a.x_);
    short unsigned dy;
    short unsigned jCost = 0;
    if (a.y_ > b.y_)
    {
        // mainly jumping
        dy = (a.y_ - b.y_);
        jCost = dy * COST_WALK_YUP;
    }
    else
    {
        // mainly falling
        dy = (b.y_ - a.y_);
        jCost = dy * COST_WALK_YDOWN;
    }

    return (dx > dy) ? (dx*COST_X + dy*COST_WALK_DXMID + jCost) : (dy*COST_WALK_YMID + dx*COST_WALK_DXMID + jCost);
}

// good behavior with jump and fall
static inline short unsigned NghbDist_walker(const PathNode& a, const PathNode& b)
{
    int dx = a.x_ - b.x_;
    unsigned area = b.map_->GetAreaProps(b.map_->GetTileIndex(b.x_, b.y_), b.v_, 3);

    // jump
    if (a.y_ > b.y_)
    {
        if (area & (jumpableFlag | walkableFlag))
        {
            if (dx != 0)
                return COST_WALK_XYUP;
            else
                return COST_WALK_YUP;
        }

        return COST_NOWAY;
    }

    // fall
    if (a.y_ < b.y_)
    {
        if (dx != 0)
            return COST_WALK_XYDOWN;
        else
            return COST_WALK_YDOWN;
    }

    // horizontal move
    if (area & jumpableFlag)
        return COST_WALK_YMID;

    if (area & walkableFlag)
        return COST_X;

    return COST_NOWAY;
}
