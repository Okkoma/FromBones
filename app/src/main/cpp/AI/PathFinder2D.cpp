#include <climits>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/Core/Context.h>
#include <Urho3D/Core/Profiler.h>
#include <Urho3D/Core/Timer.h>

#include <Urho3D/Container/Sort.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#define USE_WORLD2D
#define REDUCE_PATHPOINTS

#ifdef USE_WORLD2D
#include "MapWorld.h"
#endif

#include "PathFinder2D.h"

#define MAX_SEARCHTIME 10       // if elapsedTime > MAX_SEARCHTIME => Stop
#define MAX_SEARCHCOSTFACTOR 5 // if cost > MAX_SEARCHCOSTFACTOR x heuristic => stop

const float CHANGENODE_THRESHOLD = 0.5f;

short unsigned PathNode::width = 0;
const PathNode PathNode::EMPTY = PathNode();

World2DInfo* PathFinder2D::info_ = 0;
PathFinder2D* PathFinder2D::pathfinder_ = 0;


/// Static Functions

void PathFinder2D::Init(Context* context, World2DInfo* info)
{
    Stop();

    pathfinder_ = new PathFinder2D(context, info);
}

void PathFinder2D::Stop()
{
    if (pathfinder_)
        delete pathfinder_;

    pathfinder_ = 0;
}

int PathFinder2D::FindPath(const Vector2& startpos, int v1, const Vector2& endpos, int v2, unsigned moveTypeFlags, bool partially, Map* map)
{
    return pathfinder_->SearchPath(startpos, v1, endpos, v2, moveTypeFlags, partially, map);
}

Path2D* PathFinder2D::GetPath(int idpath)
{
    return pathfinder_->GetStoredPath(idpath);
}

int PathFinder2D::SetNodeOnPath(int idpath, Node* node)
{
    return pathfinder_->AffectNodeOnPath(idpath, node);
}

int PathFinder2D::GetCurrentPathIndex(Path2D* path, int iuser)
{
    return path->indexes_[iuser];
}

int PathFinder2D::GetLastPathIndex(Path2D * path)
{
    return path->numNodes_-1;
}

int PathFinder2D::GetPathIndex(Path2D* path, int iuser, const Vector2& position)
{
    short unsigned& index = path->indexes_[iuser];

    // if index==0 it's a free iuser slot (see AffectNodeOnPath=> GetUserSlotInPath)
    if (index == 0)
        return -1;

    // currentPosition at proximity of points_[index], go to next index
    if (Abs(position.x_ - path->points_[index].x_) < CHANGENODE_THRESHOLD &&
            Abs(position.y_ - path->points_[index].y_) < CHANGENODE_THRESHOLD * 2)
    {
        URHO3D_LOGINFOF("PathFinder2D() - GetPathIndex : path=%u iuser=%d pass to nextindex=%u (lastindex=%u)",
                        path->id_, iuser, index, path->numNodes_-1);
        index = index + 1;
    }

    // path finish, free user
    if (index == path->numNodes_)
    {
        pathfinder_->FreeUserSlotInPath(path, iuser);
        return -1;
    }

    return index;
}

const Vector2& PathFinder2D::GetPathPosition(Path2D* path, int index)
{
    return (path->points_[index]);
}

const Vector2& PathFinder2D::GetPathSegment(Path2D* path, int index)
{
    return (path->segments_[index]);
}

bool PathFinder2D::IsOnLastPathSegment(Path2D* path, int index)
{
    return (index == path->numNodes_-1);
}

bool PathFinder2D::IsPathFinished(int idpath)
{
    Path2D* path = pathfinder_->GetPath(idpath);
    if (!path)
        return false;

    return (path->status_ == PathFinished);
}


/// Instantiate Functions

PathFinder2D::PathFinder2D(Context* context, World2DInfo* info)
    : Object(context)
{
    info_ = info;

    Init();
}

PathFinder2D::~PathFinder2D()
{
    Free();

    URHO3D_LOGDEBUG("~PathFinder2D()");
}

void PathFinder2D::Init()
{
    PathNode::width = info_->mapWidth_;

    layerWidth_ = info_->mapWidth_;
    layerHeight_ = info_->mapHeight_;

    layerSize_ = layerWidth_ * layerHeight_;

    tileCenter_ = Vector2(info_->mWidth_/info_->mapWidth_/2, info_->mHeight_/info_->mapHeight_/2);

    URHO3D_LOGINFOF("PathFinder() - Init : width=%u height=%u", layerWidth_, layerHeight_);
}

void PathFinder2D::Free()
{
    URHO3D_LOGINFO("PathFinder2D() - Free ... ");

    for (int i=0; i<storedPaths_.Size(); i++)
        delete storedPaths_[i];
    storedPaths_.Clear();

    URHO3D_LOGINFO("PathFinder2D() - Free ... OK !");
}

int PathFinder2D::SearchPath(const Vector2& startpos, int v1, const Vector2& endpos, int v2, unsigned moveTypeFlags, bool partially, Map* map)
{
    IntVector2 pStart, pEnd;
    Map* mapstart = 0;
    Map* mapend = 0;

#ifdef USE_WORLD2D
    ShortIntVector2 mpointstart, mpointend;
    info_->Convert2WorldMapPoint(startpos, mpointstart);
    info_->Convert2WorldMapPoint(endpos, mpointend);

    info_->Convert2WorldMapPosition(mpointstart, startpos, pStart);
    info_->Convert2WorldMapPosition(mpointend, endpos, pEnd);

    mapstart = World2D::GetMapAt(mpointstart);
    mapend = World2D::GetMapAt(mpointend);

//    URHO3D_LOGINFOF("PathFinder() - FindPath : World2D, mpointstart=%s mpointend=%s mapstart=%u mapend=%u startpos=%s(%s) endpos=%s(%s) ...",
//             mpointstart.ToString().CString(),  mpointend.ToString().CString(),
//             mapstart, mapend, startpos.ToString().CString(), pStart.ToString().CString(),
//             endpos.ToString().CString(), pEnd.ToString().CString());
#else
    Map::LocalPositionToTileIndex(pStart.x_, pStart.y_, startpos);
    Map::LocalPositionToTileIndex(pEnd.x_, pEnd.y_, endpos);

    mapstart = map;
    mapend = map;

//    URHO3D_LOGINFOF("PathFinder() - FindPath : noWorld2D, map=%u startpos=%s(%s) endpos=%s(%s) ...",
//             map, startpos.ToString().CString(), pStart.ToString().CString(),
//             endpos.ToString().CString(), pEnd.ToString().CString());
#endif

    if (!mapstart || !mapend)
    {
        URHO3D_LOGERRORF("PathFinder() - FindPath : ... NOK! mapstart=%u, mapend=%u", mapstart, mapend);
        return -1;
    }

    if (pStart == pEnd)
    {
        URHO3D_LOGWARNINGF("PathFinder() - FindPath : ... NOK! pStart=pEnd=%s", pStart.ToString().CString());
        return -1;
    }

    URHO3D_LOGINFOF("PathFinder() - FindPath : ... moveType=%s", (moveTypeFlags & MV_FLY) ? "Flyer":"Walker");

    PathNode sStart(pStart.x_, pStart.y_, v1, mapstart);
    PathNode sEnd(pEnd.x_, pEnd.y_, v2, mapend);

    URHO3D_PROFILE(SearchPathFinder2D);

    HiresTimer* timerUsec_ = 0;
    if (HiresTimer::IsSupported())
        timerUsec_ = new HiresTimer();

    Timer* timerMsec_ = new Timer();

    int idpath = -1;

    int result = Search_Astar(sStart, sEnd, moveTypeFlags, partially);

    if (result == PathFound || result == PathPartiallyFound)
        idpath = ConstructPath();

    if (idpath == -1)
        URHO3D_LOGINFOF("PathFinder() - FindPath : ... moveType=%s - PathNoFound heuristic=%u !",
                        (moveTypeFlags & MV_FLY) ? "Flyer" : "Walker", GetHeuristic(sStart, sEnd, moveTypeFlags));
    else
        URHO3D_LOGINFOF("PathFinder() - FindPath : ... moveType=%s - %s => idpath=%d , heuristic=%u , fcost=%u !",
                        (moveTypeFlags & MV_FLY) ? "Flyer" : "Walker", result == PathFound ? "PathFound" : "PathPartiallyFound",
                        idpath, GetHeuristic(sStart, sEnd, moveTypeFlags), GetLastFcost());

    if (HiresTimer::IsSupported())
    {
        URHO3D_LOGINFOF("PathFinder() - FindPath : ... (elapsedTime=%d Msec (%d Usec) )", timerMsec_->GetMSec(false), timerUsec_->GetUSec(false));
        delete timerUsec_;
    }
    else
        URHO3D_LOGINFOF("PathFinder() - FindPath : ... (elapsedTime=%d Msec)", timerMsec_->GetMSec(false));

    delete timerMsec_;

    return idpath;
}

short unsigned PathFinder2D::GetHeuristic(const PathNode& sStart, const PathNode& sEnd, unsigned moveTypeFlags)
{
    return (moveTypeFlags & MV_FLY) ? Heuristic_fly(sStart, sEnd)
           : (moveTypeFlags & MV_WALK) ? Heuristic_walker(sStart, sEnd)
           : 0;
}

int PathFinder2D::Search_Astar(PathNode& sStart, PathNode& sEnd, unsigned moveTypeFlags, bool partially)
{
    if ((moveTypeFlags & (MV_FLY|MV_WALK)) == 0)
        return NoPathFound;

    PathFunc heuristic;
    PathFunc nghbDistance;
    int status;

    unsigned areaflag = (moveTypeFlags & MV_FLY) ? flyableFlag : (walkableFlag | jumpableRightFlag | jumpableLeftFlag);

    /// INIT : Set Heuristic functions and permission on areas (areaflag)
    // end checking point
    if (!(sEnd.map_->GetAreaProps(sEnd.map_->GetTileIndex(sEnd.x_, sEnd.y_), sEnd.v_, 3) & areaflag))
        return NoPathFound;

    heuristic = (moveTypeFlags & MV_FLY) ? &Heuristic_fly : &Heuristic_walker;
    nghbDistance = (moveTypeFlags & MV_FLY) ? &NghbDist_fly : &NghbDist_walker;
    areaflag_ = (moveTypeFlags & MV_FLY) ? areaflag : areaflag | flyableFlag; // MV_WALK : flyableFlag for falling

    /// SEARCH
//    LOGINFOF("PathFinder() - Search_Astar : ...");
    open_.Clear();
    closed_.Clear();

    sStart.f_ = sStart.g_ + (*heuristic)(sStart, sEnd);
    open_.Push(sStart);

    Timer* timerMsec_ = new Timer();

    PathNode a;

    while (open_.Size())
    {
        a = open_.Back();

        open_.Pop();

        closed_.Push(a);

        if (a == sEnd)
        {
            status = PathFound;
            break;
        }

        if (a.f_ > MAX_SEARCHCOSTFACTOR * sStart.f_ || timerMsec_->GetMSec(false) > MAX_SEARCHTIME)
        {
            status = NoPathFound;
            break;
        }

        GetNeighbors(a);

        for (int i=0; i < numNeighbors_; ++i)
        {
            // Get Neighbor mooreIndex i verify the props
            PathNode& b = neighbors_[i];

            if (!closed_.Contains(b))
            {
//                URHO3D_LOGINFOF("PathFinder() - Search_Astar : GetNeighbor(%u(x%u,y%u),m%d) = %u(x%u,y%u)",
//                         a, a->x_, a->y_, i, b, b->x_, b->y_);
                if (!open_.Contains(b))
                    b.g_= USHRT_MAX;

                ComputeCost_Astar(&a, &b, &sEnd, heuristic, nghbDistance);
            }
//            else
//                URHO3D_LOGINFOF("PathFinder() - Search_Astar : GetNeighbor(%u(x%u,y%u),m%d) = %u(x%u,y%u) in closed_", a, a->x_,a->y_, i, b, b->x_, b->y_);
        }
    }

    if (status == NoPathFound && partially && a.g_ < sStart.f_)
        status = PathPartiallyFound;

    if (status == PathFound || status == PathPartiallyFound)
        lastPathNode_ = a;

    delete timerMsec_;
    return status;
}

void PathFinder2D::ComputeCost_Astar(PathNode* a, PathNode* b, PathNode* sEnd, PathFunc heuristic, PathFunc nghbDistance)
{
//    URHO3D_LOGINFOF("PathFinder() - Search_Astar : ComputeCost_Astar(%u,%u) ...", a, b);
    short unsigned gtemp = b->g_;
    short unsigned nghbCost = (*nghbDistance)(*a, *b);

    if (a->g_ + nghbCost < b->g_)
    {
        b->parentx_ = a->x_;
        b->parenty_ = a->y_;
        b->parentv_ = a->v_;
        b->parentmap_ = a->map_;
        b->g_ = a->g_ + nghbCost;
    }

    if (b->g_ < gtemp)
    {
        b->f_ = b->g_ + (*heuristic)(*b, *sEnd);

        if (!open_.Contains(*b))
            open_.Push(*b);

//        URHO3D_LOGINFOF("PathFinder() - Search_Astar : ComputeCost_Astar(%u,%u) lower cost %u(%u) => Sort open_(%u)", a, b, b->g_, gtemp, open_.Size());

        Sort(open_.Begin(), open_.End(), ComparePathNode);
    }
}

void PathFinder2D::GetNeighbors(const PathNode& a)
{
    short int x;
    short int y;
    short int v;
    short int mapdx;
    short int mapdy;
    Map* map;
    numNeighbors_ = 0;

    for (int imoore=0; imoore<8; imoore++)
    {
        // init position
        x = a.x_ + MapInfo::neighborOffX[imoore];
        y = a.y_ + MapInfo::neighborOffY[imoore];
        v = a.v_;
        map = a.map_;

        // bound Test
        mapdx = 0;
        mapdy = 0;
        if      (x < 0)
        {
            mapdx = -1;
            x = layerWidth_-1;
        }
        else if (x >= layerWidth_)
        {
            mapdx = 1;
            x = 0;
        }
        if      (y < 0)
        {
            mapdy = 1;
            y = layerHeight_-1;
        }
        else if (y >= layerHeight_)
        {
            mapdy = -1;
            y = 0;
        }

        // get map if out of bound map

        if (mapdx != 0 || mapdy != 0)
        {
#ifdef USE_WORLD2D
            map = World2D::GetMapAt(map->GetMapPoint() + ShortIntVector2(mapdx, mapdy));
#else
            map = 0;
#endif
        }

        if (!map)
            continue;

        if (map->GetAreaProps(map->GetTileIndex(x, y), v, 3) & areaflag_)
        {
//            URHO3D_LOGINFOF("PathFinder2D() - GetNeighbor(%u,%d) : PathNode x=%d y=%d", a, imoore, x, y);
            neighbors_[numNeighbors_].x_ = x;
            neighbors_[numNeighbors_].y_ = y;
            neighbors_[numNeighbors_].v_ = v;
            neighbors_[numNeighbors_].f_ = USHRT_MAX;
            neighbors_[numNeighbors_].map_ = map;
            neighbors_[numNeighbors_].parentx_ = a.x_;
            neighbors_[numNeighbors_].parenty_ = a.y_;
            neighbors_[numNeighbors_].parentmap_ = a.map_;

            numNeighbors_++;
        }
    }
}

inline bool PathFinder2D::isPointOnSameLine(int px, int py, int startx, int starty, int endx, int endy)
{
    return (startx == endx && startx == px) || (starty == endy && starty == py);
}

int PathFinder2D::ConstructPath()
{
    Vector<PathNode>::ConstIterator it = closed_.Find(lastPathNode_);
    if (it == closed_.End())
        return -1;

    Path2D* path;

    if (freePaths_.Size())
    {
        path = freePaths_.Back();
        freePaths_.Pop();
        path->points_.Clear();
//        URHO3D_LOGINFOF("PathFinder2D() - ConstructPath : Reuse Path %u !", path->id_);
    }
    else
    {
        path = new Path2D();
        // store path
        path->id_ = storedPaths_.Size();
        storedPaths_.Push(path);
//        URHO3D_LOGINFOF("PathFinder2D() - ConstructPath : Add new Path %u !", path->id_);
    }

    URHO3D_LOGINFOF("PathFinder2D() - ConstructPath : ... id=%u", path->id_);

    // Calculate num nodes
    // init with last point
    ibuff_[0] = it - closed_.Begin();
    short unsigned childx = (*it).x_;
    short unsigned childy = (*it).y_;
    unsigned index = 1;
    for (;;)
    {
        it = closed_.Find(PathNode((*it).parentx_, (*it).parenty_, (*it).parentv_, (*it).parentmap_));
        const PathNode& p = (*it);
        // if no parent, it's the first point, add its index and stop
        if (!p.parentmap_)
        {
            ibuff_[index] = it - closed_.Begin();
            index++;
            break;
        }
#ifdef REDUCE_PATHPOINTS
        // if p is not on the line compound by child and parent vectors, add its index
        if (!isPointOnSameLine(p.x_, p.y_, childx, childy, p.parentx_, p.parenty_))
        {
            ibuff_[index] = it - closed_.Begin();
            childx = p.x_;
            childy = p.y_;
            index++;
        }
#else
        ibuff_[index] = it - closed_.Begin();
        index++;
#endif
        if (index > 1023)
        {
            index = 1023;
            break;
        }
    }

    path->numNodes_ = index;
    path->points_.Resize(path->numNodes_);

    URHO3D_LOGINFOF("PathFinder2D() - ConstructPath : ... Resize Points %u closed_ = %u!", path->points_.Size(), closed_.Size());

    // store each node in points_
    while (index > 0)
    {
//        URHO3D_LOGINFOF("ibuff_[%u]=%u", index-1, ibuff_[index-1]);
        const PathNode& p = closed_[ibuff_[index-1]];
        // last point in ibuff is the first point
        info_->Convert2WorldPosition(p.map_->GetMapPoint(), IntVector2(p.x_, p.y_), path->points_[path->numNodes_-index]);
//        path->points_[path->numNodes_-index] = p.map_->GetGroundLayer()->GetTileNode(p.x_, p.y_)->GetWorldPosition2D() + tileCenter_;
//        path->points_[path->numNodes_-index] += tileCenter_;
        index--;
    }

    path->status_ = PathReady;
    path->numActiveUsers_ = 0;

    // add segments
    path->segments_.Resize(path->numNodes_);
    for (int i=1; i<path->numNodes_; i++)
    {
        path->segments_[i] = path->points_[i] - path->points_[i-1];
    }
//    DumpPath(path->id_);

    URHO3D_LOGINFO("PathFinder2D() - ConstructPath : ... OK !");

    return path->id_;
}


/// Handle Path Functions

void PathFinder2D::FreeUserSlotInPath(Path2D* path, int iuser)
{
    if (iuser == -1)
        return;

    if (iuser >= path->users_.Size())
    {
        URHO3D_LOGERRORF("PathFinder2D() - FreeUserSlotInPath : path=%u iuser=%d > users_.Size=%u",
                         path->id_, iuser, path->users_.Size());
        return;
    }

    path->numActiveUsers_--;
    path->users_[iuser] = 0;
    path->indexes_[iuser] = 0;

//    URHO3D_LOGINFOF("PathFinder2D() - FreeUserSlotInPath : path=%u usernode=%d has finished the Path! numActiveUsers_ = %u",
//        path->id_, iuser, path->numActiveUsers_);

    // update status
    if (path->numActiveUsers_ == 0)
    {
        path->status_ = PathFinished;
        freePaths_.Push(path);
        URHO3D_LOGINFOF("PathFinder2D() - FreeUserSlotInPath : path=%u Finished ! put it in freelist !", path->id_);
    }
}

int PathFinder2D::FindFirstActiveUserInPath(Path2D* path)
{
    if (!path->numActiveUsers_)
        return -1;

    for (Vector<Node*>::Iterator it=path->users_.Begin(); it != path->users_.End(); ++it)
    {
        if (*it != 0)
            return it - path->users_.Begin();
    }

    return -1;
}

int PathFinder2D::FindUserSlotInPath(Path2D* path, Node* node)
{
    int iuser = -1;

    // try to get iuser for this node
    Vector<Node*>::Iterator it = path->users_.Find(node);
    if (it != path->users_.End())
    {
        iuser = it - path->users_.Begin();
//        URHO3D_LOGINFOF("PathFinder2D() - FindUserSlotInPath : ... Find iuser=%d continue path", iuser);
    }

    return iuser;
}

int PathFinder2D::GetUserSlotInPath(Path2D* path, Node* node)
{
    int iuser = -1;

    // try to get free slot
    Vector<Node*>::Iterator it = path->users_.Find(0);
    if (it != path->users_.End())
    {
        iuser = it - path->users_.Begin();
        path->users_[iuser] = node;
//        URHO3D_LOGINFOF("PathFinder2D() - GetUserSlotInPath : ... get free slot iuser=%d begin path", iuser);
    }

    // no free slot, add new one
    if (iuser == -1)
    {
        iuser = path->users_.Size();
        path->users_.Push(node);
        path->indexes_.Push(0);
//        URHO3D_LOGINFOF("PathFinder2D() - GetUserSlotInPath : ... add new slot iuser=%d (%u) begin path", iuser, path->users_[iuser]);
    }

    // set the new user node index on this path
    path->indexes_[iuser] = 1;
    path->numActiveUsers_++;
    path->status_ = PathInUse;

    return iuser;
}

int PathFinder2D::AffectNodeOnPath(int idpath, Node* node)
{
    Path2D* path = storedPaths_[idpath];

    if (path->status_ < PathReady || path->status_ == PathFinished)
        return -1;

    int iuser = FindUserSlotInPath(path, node);

    // if node don't use already this path, register it
    if (iuser == -1)
    {
        // check if the node used an other path and free it
        HashMap<Node*, int>::Iterator it = nodeUsingPath_.Find(node);
        if (it != nodeUsingPath_.End())
            if (it->second_ != idpath)
                FreeUserSlotInPath(storedPaths_[it->second_], FindUserSlotInPath(storedPaths_[it->second_], node));

        // Get a user slot (a free slot or allocate a new slot) in path
        iuser = GetUserSlotInPath(path, node);

        // register that the node is using the path idpath
        nodeUsingPath_[node] = idpath;
    }

    return iuser;
}

//const Vector2& PathFinder2D::GetNextPathNode(int idpath, Node* node, const Vector2& currentPosition, float velocity)
//{
//    if (idpath < storedPaths_.Size() && node)
//    {
//        Path2D* path = storedPaths_[idpath];
//
//        if (path->status_ < PathReady || path->status_ == PathFinished)
//            return currentPosition;
//
//        int iuser = FindUserSlotInPath(path, node);
//
//        // if node don't use already this path, register it
//        if (iuser == -1)
//        {
//            // check if the node used an other path and free it
//            HashMap<Node*, int>::Iterator it = nodeUsingPath_.Find(node);
//            if (it != nodeUsingPath_.End())
//                if (it->second_ != idpath)
//                    FreeUserSlotInPath(storedPaths_[it->second_], FindUserSlotInPath(storedPaths_[it->second_], node));
//
//            // Get a user slot (a free slot or allocate a new slot) in path
//            iuser = GetUserSlotInPath(path, node);
//
//            // register that the node is using the path idpath
//            nodeUsingPath_[node] = idpath;
//        }
//
//        short unsigned& index = path->indexes_[iuser];
//
//        // currentPosition at proximity of points_[index], go to next index
//        if (Abs(currentPosition.x_ - path->points_[index].x_) < /*velocity + */CHANGENODE_THRESHOLD &&
//            Abs(currentPosition.y_ - path->points_[index].y_) < /*velocity + */CHANGENODE_THRESHOLD * 2 )
//        {
//            LOGINFOF("PathFinder2D() - GetNextPathNode : path=%u usernode=%d(num=%u) pass to nextpoint=%u (num=%u)",
//                     path->id_, iuser, path->numActiveUsers_, index, path->numNodes_);
//            index = index + 1;
//        }
//
//        // path finish, free user
//        if (index == path->numNodes_)
//        {
//            FreeUserSlotInPath(path, iuser);
//            return currentPosition;
//        }
//
//        // return the point
//        return path->points_[index];
//    }
//
//    return Vector2::ZERO;
//}

void PathFinder2D::DumpPath(int idpath) const
{
    Path2D* path = storedPaths_[idpath];

    short int status = path->status_;
    URHO3D_LOGINFOF("PathFinder2D() - Dump : idPath=%d status=%s numNodes=%u numactiveuser=%u", idpath,
                    status == NoPathFound ? "NoPathFound" : status == PathFound ? "PathFound" :
                    status == PathPartiallyFound ? "PathPartiallyFound" :
                    status == PathReady ? "PathReady" : status == PathInUse ? "PathInUse" : "PathFinished",
                    path->numNodes_, path->numActiveUsers_);

    unsigned i=0;
    for (PODVector<Vector2>::ConstIterator it=path->points_.Begin(); it!=path->points_.End(); ++it,++i)
        URHO3D_LOGINFOF(" => Points[%u] = %s", i, (*it).ToString().CString());
}

void PathFinder2D::DrawDebugGeometry(DebugRenderer* debugRenderer)
{
#ifdef DEBUG_PATH
    if (!pathfinder_)
        return;

    pathfinder_->DrawDebug(debugRenderer);
#endif
}

#ifdef DEBUG_PATH
void PathFinder2D::DrawDebug(DebugRenderer* debugRenderer)
{
    for (Vector<Path2D*>::ConstIterator it=storedPaths_.Begin(); it!=storedPaths_.End(); ++it)
    {
        const Path2D& path = *(*it);
//        if (path.status_ != PathFinished)
        {
//            int iuser = FindFirstActiveUserInPath(*it);
//            if (iuser == -1) continue;

            Color c1(0.1f * path.id_, 1.f, 0.1f * path.id_, 0.5f);
            Color c2(1.f, 0.1f * path.id_, 0.1f * path.id_, 0.5f);

            for (int i=0; i<path.numNodes_; i++)
            {
                Vector3 p(path.points_[i].x_, path.points_[i].y_, 0.f);
//                if (i < path.indexes_[iuser])
//                {
//                    debugRenderer->AddTriangle(p + Vector3(-0.2f, -0.1f, 0.f)/* - Vector3(tileCenter_)*/, p + Vector3(0.2f, -0.1f, 0.f)/* - Vector3(tileCenter_)*/, p + Vector3(0.f, 0.1f, 0.f)/* - Vector3(tileCenter_)*/, c1, false);
//                }
//                else
                {
                    debugRenderer->AddTriangle(p + Vector3(-0.2f, -0.1f, 0.f)/* - Vector3(tileCenter_)*/, p + Vector3(0.2f, -0.1f, 0.f)/* - Vector3(tileCenter_)*/, p + Vector3(0.f, 0.1f, 0.f)/* - Vector3(tileCenter_)*/, c2, false);
                }
            }
        }
    }
}
#endif
