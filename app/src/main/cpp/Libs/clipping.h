void GetSegmentsOnMapBorder(const MapCollider* mapcollider, const Rect& mapBorder, PODVector<Vector2>& contour, PODVector<unsigned>& segmentsOnMapBorder)
{
    if (!mapcollider->map_)
        return;

    segmentsOnMapBorder.Clear();

    if (mapcollider->map_->GetType() != Map::GetTypeStatic())
        return;

    Map* map = static_cast<Map*>(mapcollider->map_);
    if (!map)
        return;

    const Vector2 tilesize(World2D::GetWorldInfo()->tileWidth_, World2D::GetWorldInfo()->tileHeight_);
    const int tileindexoffset[4] = { 1, -1, MapInfo::info.width_, -MapInfo::info.width_ };
    const Vector2 verticeoffset[4] = { Vector2(tilesize.x_, 0.f), Vector2(-tilesize.x_, 0.f), Vector2(0.f, -tilesize.y_), Vector2(0.f, tilesize.y_) };
    const int colliderZ = mapcollider->params_->colliderz_;
    const unsigned numpoints = contour.Size();

    unsigned layerZ;
    Map* sidemap = 0;
    int lastside = -1;
    int ioffset = 0;
    Vector<Pair<unsigned, Vector2> > breakPoints;

    for (unsigned i = 0; i < numpoints; i++)
    {
        // get a segment
        const Vector2& p1 = contour[i];
        const Vector2& p2 = contour[i+1 == numpoints ? 0 : i+1];
        int side = -1;

        // check if the segment is on top border
        if (p1.y_ > mapBorder.max_.y_ && p2.y_ > mapBorder.max_.y_)
            side = MapDirection::North;
        // check if the segment is on bottom border
        else if (p1.y_ < mapBorder.min_.y_ && p2.y_ < mapBorder.min_.y_)
            side = MapDirection::South;
        // check if the segment is on right border
        else if (p1.x_ > mapBorder.max_.x_ && p2.x_ > mapBorder.max_.x_)
            side = MapDirection::East;
        // check if the segment is on left border
        else if (p1.x_ < mapBorder.min_.x_ && p2.x_ < mapBorder.min_.x_)
            side = MapDirection::West;

        if (side == -1)
            continue;

        // the 2 points are on a side

        // if the side has changed, get the map on this side
        if (!sidemap || side != lastside)
        {
            sidemap = map->GetConnectedMap(side);
            if (!sidemap)
            {
//                URHO3D_LOGERRORF("No Side Map at %s", MapDirection::GetName(side));
                continue;
            }

//            layerZ = sidemap->GetObjectFeatured()->GetNearestViewZ(colliderZ);
//            if (!layerZ)
//                continue;

            layerZ = colliderZ;

//            URHO3D_LOGERRORF("GetSegmentsOnMapBorder : at %s sidemap=%s colliderZ=%d layerZ=%d ...", MapDirection::GetName(side), sidemap->GetMapPoint().ToString().CString(), colliderZ, layerZ);

            lastside = side;
        }

        if (!sidemap)
        {
            URHO3D_LOGERRORF("GetSegmentsOnMapBorder() - Map=%s => No Side Map at %s", map->GetMapPoint().ToString().CString(), MapDirection::GetName(side));
            continue;
        }

        // check the features on the sidemap belong this segment
        /*
                int viewid = sidemap->GetViewId(layerZ);
                if (viewid == -1)
                {
                    URHO3D_LOGERRORF("GetSegmentsOnMapBorder() - Map=%s => SideMap=%s no viewid at Z=%u",
                                     map->GetMapPoint().ToString().CString(), sidemap->GetMapPoint().ToString().CString(), layerZ);
                    continue;
                }
                const FeaturedMap& sidefeatures = sidemap->GetFeatureView(viewid);
        */
        const FeaturedMap& sidefeatures = sidemap->GetFeatureViewAtZ(layerZ);

        unsigned tileindex;
        int tilex, tiley;

        // horizontal
        if (side < 2)
        {
            tilex = Round(p1.x_ / tilesize.x_);
            if (side == MapDirection::South)
            {
                tiley = 0;
                tilex--;
            }
            else
            {
                tiley = MapInfo::info.height_ - 1;
            }
        }
        // vertical
        else
        {
            tiley = Round(p1.y_ / tilesize.y_);
            if (side == MapDirection::East)
            {
                tiley--;
                tilex = 0;
            }
            else
            {
                tilex = MapInfo::info.width_ - 1;
            }
            tiley = MapInfo::info.height_ - 1 - tiley;
        }

        tileindex = tiley * MapInfo::info.width_ + tilex;

//        unsigned tileindex = (sideoffset[side].y_ + MapInfo::info.height_ - 1 - tiley) * MapInfo::info.width_ + (sideoffset[side].x_ + tilex);
        int numtiles = Round(side < 2 ? (Abs(p2.x_ - p1.x_)) / tilesize.x_: (Abs(p2.y_ - p1.y_)) / tilesize.y_);

//        URHO3D_LOGERRORF("GetSegmentsOnMapBorder : i=%d/%d segment(p1=%s to p2=%s) on Border=%s => on SideMap tileCoord=%d,%d (%u) numtiles=%d !",
//                        i, numpoints, p1.ToString().CString(), p2.ToString().CString(), MapDirection::GetName(side), tilex, tiley, tileindex, numtiles);

        if (tileindex >= sidefeatures.Size())
        {
            return;
        }

        bool tileIsSolid = sidefeatures[tileindex] >= MapFeatureType::InnerFloor && sidefeatures[tileindex] <= MapFeatureType::TunnelWall;
        if (tileIsSolid)
        {
//            URHO3D_LOGERRORF("GetSegmentsOnMapBorder : at %s sidemap=%s solidtile at tileindex=%u => add first point for a new segment!", MapDirection::GetName(side), sidemap->GetMapPoint().ToString().CString(), tileindex, numtiles);
            segmentsOnMapBorder.Push(ioffset+i);
        }

        // more tiles => check for break points
        if (numtiles > 1)
        {
            for (int t=1; t < numtiles; t++)
            {
                tileindex += tileindexoffset[side];

                if (tileindex >= sidefeatures.Size())
                {
                    URHO3D_LOGERRORF("GetSegmentsOnMapBorder : at %s sidemap=%s tileindex=%u tile=%d/%u >= sidefeatures.Size(%u)", MapDirection::GetName(side), sidemap->GetMapPoint().ToString().CString(), tileindex, t+1, numtiles, sidefeatures.Size());
                    return;
                }

                bool lastTileIsSolid = tileIsSolid;
                tileIsSolid = sidefeatures[tileindex] >= MapFeatureType::InnerFloor && sidefeatures[tileindex] <= MapFeatureType::TunnelWall;

                // Add Break Point in contour, End of the current Segment
                if (!tileIsSolid && lastTileIsSolid)
                {
                    ioffset++;
                    breakPoints.Push(Pair<unsigned, Vector2>(ioffset+i, p1 + (t * verticeoffset[side])));
//                    URHO3D_LOGERRORF("GetSegmentsOnMapBorder : at %s sidemap=%s at tileindex=%u tile=%d/%u add break point for ending of the current segment!", MapDirection::GetName(side), sidemap->GetMapPoint().ToString().CString(), tileindex, t+1, numtiles);
                }
                // Add this new segment point On Map Border
                else if (tileIsSolid && !lastTileIsSolid)
                {
                    ioffset++;
                    breakPoints.Push(Pair<unsigned, Vector2>(ioffset+i, p1 + (t * verticeoffset[side])));
                    segmentsOnMapBorder.Push(ioffset+i);
//                    URHO3D_LOGERRORF("GetSegmentsOnMapBorder : at %s sidemap=%s solidtile at tileindex=%u tile=%d/%u add break point for a new segment!", MapDirection::GetName(side), sidemap->GetMapPoint().ToString().CString(), tileindex, t+1, numtiles);
                }
            }
        }
    }

    // insert break points in the contour
    if (breakPoints.Size())
    {
        const unsigned numbreakpoints = breakPoints.Size();
        for (unsigned i=0; i < numbreakpoints; i++)
            contour.Insert(breakPoints[i].first_, breakPoints[i].second_);
    }

    // dump segments on mapborder
//    if (segmentsOnMapBorder.Size())
//    {
//        const unsigned numborderpoints = segmentsOnMapBorder.Size();
//        String s;
//        for (unsigned i=0; i < numborderpoints; i++)
//            s += String(segmentsOnMapBorder[i]) + ", ";
//        URHO3D_LOGERRORF("Dump Segments on MapBorder = %s", s.CString());
//    }
}

void ScaleManhattanContour(const PODVector<Vector2>& contour, float offset, Vector<PODVector<Vector2> >& scaledContours)
{
    int numfaces = contour.Size();

    if (numfaces < 3)
        return;

    scaledContours.Resize(scaledContours.Size()+1);
    PODVector<Vector2>& scaledContour = scaledContours.Back();
    scaledContour.Resize(numfaces);

    // Point 0
    int i = 0;
    Vector2 u = contour[i+1] - contour[numfaces-1];
    u.x_ = Sign(u.x_);
    u.y_ = Sign(u.y_);
    scaledContour[i] = contour[i] + Vector2(u.y_ * -offset, u.x_ * offset);

    // Point 1 to numfaces-2
    for (i=1; i < numfaces-1; i++)
    {
        u = contour[i+1] - contour[i-1];
        u.x_ = Sign(u.x_);
        u.y_ = Sign(u.y_);
        scaledContour[i] = contour[i] + Vector2(u.y_ * -offset, u.x_ * offset);
    }

    // Point numface-1
    i = numfaces-1;
    u = contour[0] - contour[i-1];
    u.x_ = Sign(u.x_);
    u.y_ = Sign(u.y_);
    scaledContour[i] = contour[i] + Vector2(u.y_ * -offset, u.x_ * offset);
}

void ScaleManhattanContour(const PODVector<Vector2>& contour, float offset, PODVector<Vector2>& scaledContour)
{
    int numfaces = contour.Size();

    if (numfaces < 3)
        return;

    scaledContour.Resize(numfaces);

    // Point 0
    int i = 0;
    Vector2 u = contour[i+1] - contour[numfaces-1];
    u.x_ = Sign(u.x_);
    u.y_ = Sign(u.y_);
    scaledContour[i] = contour[i] + Vector2(u.y_ * -offset, u.x_ * offset);

    // Point 1 to numfaces-2
    for (i=1; i < numfaces-1; i++)
    {
        u = contour[i+1] - contour[i-1];
        u.x_ = Sign(u.x_);
        u.y_ = Sign(u.y_);
        scaledContour[i] = contour[i] + Vector2(u.y_ * -offset, u.x_ * offset);
    }

    // Point numface-1
    i = numfaces-1;
    u = contour[0] - contour[i-1];
    u.x_ = Sign(u.x_);
    u.y_ = Sign(u.y_);
    scaledContour[i] = contour[i] + Vector2(u.y_ * -offset, u.x_ * offset);
}


//#define EXTERNAL_SEGMENTSINTERSECTION
//#define SEGMENTINTERSECTION1

#ifdef EXTERNAL_SEGMENTSINTERSECTION
#ifdef SEGMENTINTERSECTION1
bool SegmentsIntersect(const Vector2& p, const Vector2& q, const Vector2& p2, const Vector2& q2, Vector2& result)
{
    const float Epsilon = 0.00001f;

    Vector2 r = p2 - p;
    Vector2 s = q2 - q;
    Vector2 qp = q - p;
    float rxs = r.x_*s.y_ - r.y_*s.x_;
    float qpxr = qp.x_*r.y_ - qp.y_*r.x_;

    bool rxsnull = Abs(rxs) < Epsilon;
    bool qpxrnull = Abs(qpxr) < Epsilon;

    // If r x s = 0 and (q - p) x r = 0, then the two lines are collinear.
    if (rxsnull && qpxrnull)
    {
        // 1. If either  0 <= (q - p) * r <= r * r or 0 <= (p - q) * s <= * s
        // then the two lines are overlapping,
        // ...

        // 2. If neither 0 <= (q - p) * r = r * r nor 0 <= (p - q) * s <= s * s
        // then the two lines are collinear but disjoint.
        // No need to implement this expression, as it follows from the expression above.
        return false;
    }

    // 3. If r x s = 0 and (q - p) x r != 0, then the two lines are parallel and non-intersecting.
    if (rxsnull && !qpxrnull)
        return false;

    if (!rxsnull)
    {
        // t = (q - p) x s / (r x s)
        float t = (qp.x_*s.y_ - qp.y_*s.x_) / rxs;
        // u = (q - p) x r / (r x s)
        float u = qpxr / rxs;

        // 4. If r x s != 0 and 0 <= t <= 1 and 0 <= u <= 1
        // the two line segments meet at the point p + t r = q + u s.
        if ((t >= 0.f && t <= 1.f) && (u >= 0.f && u <= 1.f))
        {
            // We can calculate the intersection point using either t or u.
            result = p + t*r;
            return true;
        }
    }

    // 5. Otherwise, the two line segments are not parallel but do not intersect.
    return false;
}
#else
bool SegmentsIntersect(const Vector2& segment1p, const Vector2& segment1q, const Vector2& segment2p, const Vector2& segment2q, Vector2& result)
{
    const float Epsilon = 0.00001f;

    // compute the determinant of the 2x2 matrix
    Vector2 vec1 = segment1q - segment1p;
    Vector2 vec2 = segment2q - segment2p;
    float det = vec1.x_ * vec2.y_ - vec1.y_ * vec2.x_;

    // if det is 0, the directional vectors are colinear
    if (Abs(det) < Epsilon)
        return false;

    // compute directional vector between the two starting points
    Vector2 directionalVector = Vector2(segment2p.x_ - segment1p.x_, segment2p.y_ - segment1p.y_);

    // compute t1
    float t1 = vec2.y_ * directionalVector.x_ - vec2.x_ * directionalVector.y_;
    t1 /= det;

    // if t1 is not between 0 and 1, the segments can't intersect
    if (t1 < 0.f || t1 > 1.f)
        return false;

    // compute t2
    float t2 = -vec1.y_ * directionalVector.x_ + vec1.x_ * directionalVector.y_;
    t2 /= det;

    // if t2 is not between 0 and 1, the segments can't intersect
    if (t2 < 0.f || t2 > 1.f)
        return false;

    // else return true
    result.x_ = t1;
    result.y_ = t2;
    return true;
}
#endif
#endif

struct SplitInfo
{
    int index1_, index2_;
    Vector2 point_;
};

bool FindIntersections(const PODVector<Vector2>& contour1, const PODVector<Vector2>& contour2, Vector<SplitInfo >& splitters)
{
    splitters.Clear();

#ifndef EXTERNAL_SEGMENTSINTERSECTION
    const float Epsilon = 0.01f;
    Vector2 p2p, q2q, qp;
    float p2pxq2q, t, u;
#endif

    // find the intersection points
    SplitInfo si;
    int nextindex1, nextindex2;
    for (si.index1_=0; si.index1_ < contour1.Size(); si.index1_++)
    {
        nextindex1 = si.index1_ == contour1.Size()-1 ? 0 : si.index1_+1;

        for (si.index2_=0; si.index2_ < contour2.Size(); si.index2_++)
        {
            nextindex2 = si.index2_ == contour2.Size()-1 ? 0 : si.index2_+1;
#ifdef EXTERNAL_SEGMENTSINTERSECTION
            if (SegmentsIntersect(contour1[si.index1_], contour1[nextindex1], contour2[si.index2_], contour2[nextindex2], si.point_))
            {
                // save the splitter
                splitters.Push(si);
            }
#else
            // fast edges intersection algorithm
            p2p = contour2[si.index2_] - contour1[si.index1_];
            q2q = contour2[nextindex2] - contour1[nextindex1];
            p2pxq2q = p2p.x_ * q2q.y_ - p2p.y_ * q2q.x_;

            if (Abs(p2pxq2q) > Epsilon)
            {
                qp  = contour1[nextindex1] - contour1[si.index1_];
                t = (qp.x_ * q2q.y_ - qp.y_ * q2q.x_) / p2pxq2q;
                u = (qp.x_ * p2p.y_ - qp.y_ * p2p.x_) / p2pxq2q;

                // the edges intersect
                if ((t >= 0.f && t <= 1.f) && (u >= 0.f && u <= 1.f))
                {
                    si.point_ = contour1[si.index1_] + t * p2p;
                    // save the splitter
                    splitters.Push(si);
                }
            }
#endif
        }
    }

    return splitters.Size();
}

bool FindIntersections_HV(const PODVector<Vector2>& contour1, const PODVector<Vector2>& contour2, Vector<SplitInfo >& splitters)
{
    const float Epsilon = 0.01f;

    splitters.Clear();
    SplitInfo si;

    PODVector<int> hSegments1;
    PODVector<int> vSegments1;
    PODVector<int> hSegments2;
    PODVector<int> vSegments2;
    Vector2 u;

    // Find Horizontal And Vertical Segments
    int numpoints = contour1.Size();
    int i;
    for (i=0; i < numpoints-1; i++)
    {
        u = contour1[i+1] - contour1[i];
        if (Abs(u.x_) < Epsilon) vSegments1.Push(i);
        else if (Abs(u.y_) < Epsilon) hSegments1.Push(i);
    }
    u = contour1[0] - contour1[numpoints-1];
    if (Abs(u.x_) < Epsilon) vSegments1.Push(numpoints-1);
    else if (Abs(u.y_) < Epsilon) hSegments1.Push(numpoints-1);

    numpoints = contour2.Size();
    for (i=0; i < numpoints-1; i++)
    {
        u = contour2[i+1] - contour2[i];
        if (Abs(u.x_) < Epsilon) vSegments2.Push(i);
        else if (Abs(u.y_) < Epsilon) hSegments2.Push(i);
    }
    u = contour2[0] - contour2[numpoints-1];
    if (Abs(u.x_) < Epsilon) vSegments2.Push(numpoints-1);
    else if (Abs(u.y_) < Epsilon) hSegments2.Push(numpoints-1);

    // Find Horizontal/Vertical Intersections

    // check hSegments1 with vSegments2
    numpoints = hSegments1.Size();
    int j, numpoints2 = vSegments2.Size();
    float minx, maxx, y;
    float miny, maxy, x;
    for (i=0; i < numpoints; i++)
    {
        y = contour1[hSegments1[i]].y_;
        minx = contour1[hSegments1[i]].x_;
        maxx = contour1[hSegments1[i+1 < numpoints ? i+1 : 0]].x_;
        if (minx > maxx)
            Swap(minx, maxx);

        for (j=0; j < numpoints2; j++)
        {
            x = contour2[vSegments2[j]].x_;
            miny = contour2[vSegments2[j]].y_;
            maxy = contour2[vSegments2[j+1 < numpoints2 ? j+1 : 0]].y_;
            if (miny > maxy)
                Swap(miny, maxy);

            // intersection
            if (x >= minx && x <= maxx && y >= miny && y <= maxy)
            {
                si.index1_ = i;
                si.index2_ = j;
                si.point_.x_ = x;
                si.point_.y_ = y;
                splitters.Push(si);
            }
        }
    }
    // check vSegments1 with hSegments2
    numpoints = hSegments2.Size();
    numpoints2 = vSegments1.Size();
    for (i=0; i < numpoints; i++)
    {
        y = contour2[hSegments2[i]].y_;
        minx = contour2[hSegments2[i]].x_;
        maxx = contour2[hSegments2[i+1 < numpoints ? i+1 : 0]].x_;
        if (minx > maxx)
            Swap(minx, maxx);

        for (j=0; j < numpoints2; j++)
        {
            x = contour1[vSegments1[j]].x_;
            miny = contour1[vSegments1[j]].y_;
            maxy = contour1[vSegments1[j+1 < numpoints2 ? j+1 : 0]].y_;
            if (miny > maxy)
                Swap(miny, maxy);

            // intersection
            if (x >= minx && x <= maxx && y >= miny && y <= maxy)
            {
                si.index1_ = j;
                si.index2_ = i;
                si.point_.x_ = x;
                si.point_.y_ = y;
                splitters.Push(si);
            }
        }
    }

    return splitters.Size();
}

bool CutContour(PODVector<Vector2>& contour1, PODVector<Vector2>& contour2)
{
    if (!contour1.Size() || !contour2.Size())
        return false;

    Vector<SplitInfo > splitters;

//    if (!FindIntersections(contour1, contour2, splitters))
//        return false;

    if (!FindIntersections_HV(contour1, contour2, splitters))
        return false;

    // can't cut with one intersection
    if (splitters.Size() < 2)
        return false;

    // keep the first and last splitters
    const SplitInfo& split1 = splitters[0];
    const SplitInfo& split2 = splitters.Back();

    // modify the contour1 with contour2 at split points
    /// TODO

//    URHO3D_LOGERRORF("CutContour : Find Intersections=%u", splitters.Size());

    return true;
}

/*

    https://rosettacode.org/wiki/Sutherland-Hodgman_polygon_clipping#C++

*/

// this works only if all of the following are true:
//   1. poly has no colinear edges;
//   2. poly has no duplicate vertices;
//   3. poly has at least three vertices;
//   4. poly is convex (implying 3).
//

void EdgeClip(const PODVector<Vector2>& sub, const Vector2& p0, const Vector2& p1, int dir, PODVector<Vector2>& result)
{
    result.Clear();

    Vector2 v0 = sub.Back();
    Vector2 v1, tmp;

    int side0, side1;

    GetDirection(p0, p1, v0, side0);

    if (side0 != -dir)
        result.Push(v0);

    for (unsigned i = 0; i < sub.Size(); i++)
    {
        v1 = sub[i];

        GetDirection(p0, p1, v1, side1);

        if (side0 + side1 == 0 && side0)
        {
            // last point and current straddle the edge
            if (Intersect(p0, p1, v0, v1, tmp))
                result.Push(tmp);
        }

        if (i == sub.Size()-1)
            break;

        if (side1 != -dir)
            result.Push(v1);

        v0 = v1;
        side0 = side1;
    }
}


#ifdef ACTIVE_RENDERSHAPE_CLIPPING
const float CLIPSCALING = 1000.f;

#define CLIPPERLIB_IMPLEMENTATION

#include "Libs/Clipper/clipper.hpp"

ClipperLib::Clipper clipper_;
ClipperLib::ClipperOffset clipperOffset_;

void Clipper_Upscale(const PODVector<Vector2>& entry, ClipperLib::Path& result)
{
    result.resize(entry.Size());

    for (unsigned i = 0; i < entry.Size(); i++)
    {
        const Vector2& e = entry[i];
        ClipperLib::IntPoint& r = result[i];
        r.X = e.x_ * CLIPSCALING;
        r.Y = e.y_ * CLIPSCALING;
    }
}

void Clipper_Downscale(const ClipperLib::Paths& entries, Vector<PODVector<Vector2> >& results)
{
    unsigned lastSize = results.Size();
    results.Resize(lastSize+entries.size());

    for (unsigned i = 0; i < entries.size(); i++)
        results[lastSize+i].Resize(entries[i].size());

    for (unsigned i = 0; i < entries.size(); i++)
    {
        PODVector<Vector2>& result = results[lastSize+i];
        for (unsigned j = 0; j < entries[i].size(); j++)
        {
            const ClipperLib::IntPoint& e = entries[i][j];
            Vector2& r = result[j];
            r.x_ = (float)e.X / CLIPSCALING;
            r.y_ = (float)e.Y / CLIPSCALING;
        }
    }
}

void Clipper_Downscale(const ClipperLib::Path& entry, PODVector<Vector2>& result)
{
    result.Resize(entry.size());

    for (unsigned i = 0; i < entry.size(); i++)
    {
        const ClipperLib::IntPoint& e = entry[i];
        Vector2& r = result[i];
        r.x_ = (float)e.X / CLIPSCALING;
        r.y_ = (float)e.Y / CLIPSCALING;
    }
}
#endif

bool ClipPolygon(const PODVector<Vector2>& clip, const PODVector<Vector2>& entry, Vector<PODVector<Vector2> >& results, bool useClipper)
{
    if (entry.Size() < 3)
        return false;

#ifdef ACTIVE_RENDERSHAPE_CLIPPING
    if (useClipper)
    {
        clipper_.Clear();

        ClipperLib::Paths clp(1);
        ClipperLib::Paths sub(1);
        ClipperLib::Paths solution;

        // UpScale Float to Int
        Clipper_Upscale(entry, sub[0]);
        Clipper_Upscale(clip, clp[0]);

        clipper_.AddPaths(sub, ClipperLib::ptSubject, true);
        clipper_.AddPaths(clp, ClipperLib::ptClip, true);

        clipper_.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftEvenOdd, ClipperLib::pftEvenOdd);

        if (solution.size() && solution[0].size())
        {
            ClipperLib::ReversePaths(solution);

            // DownScale Int to Float
            Clipper_Downscale(solution, results);

            return true;
        }

        return false;
    }
    else
#endif
        // Clipping for simple polygon
    {
        PODVector<Vector2> sub, buffer;
        PODVector<Vector2>* p0 = &sub;
        PODVector<Vector2>* p1 = &buffer;

        int dir;

        GetDirection(clip[0], clip[1], clip[2], dir);

        EdgeClip(entry, clip.Back(), clip.Front(), dir, *p0);

        for (unsigned i = 0; i < clip.Size()-1; i++)
        {
            Swap<PODVector<Vector2>*>(p0, p1);

            if (!p1->Size())
            {
                p0->Clear();
                break;
            }

            EdgeClip(*p1, clip[i], clip[i+1], dir, *p0);
        }

        results.Resize(results.Size()+1);
        PODVector<Vector2 >& result = results.Back();

        if (p0->Size() < 3)
            return false;

        // Remove duplicate points
        result.Push(p0->Back());
        for (unsigned i = 0; i < p0->Size()-1; i++)
        {
            const Vector2& point = p0->At(i);
            if (!AreEquals(result.Back(), point, 0.001f))
                result.Push(point);
        }

        if (result.Size() < 3)
        {
            result.Clear();
            return false;
        }
    }

    return true;
}

bool ClipPolygonWithHoles(const PODVector<Vector2>& clip, const PODVector<Vector2>& entry, const Vector<PODVector<Vector2> >& entryholes, Vector<PODVector<Vector2> >& contours, Vector<Vector<PODVector<Vector2> > >& resultholes)
{
    if (entry.Size() < 3)
        return false;

#ifdef ACTIVE_RENDERSHAPE_CLIPPING
    URHO3D_LOGDEBUGF("-------------------------------------------------------------");
    URHO3D_LOGDEBUGF("ClipPolygonWithHoles() : contournumpoints=%u numholes=%u ...", entry.Size(), entryholes.Size());

    clipper_.Clear();

    ClipperLib::Paths clp(1);
    ClipperLib::Paths sub(1+entryholes.Size());
    ClipperLib::Paths solution;

    // UpScale Float to Int
    Clipper_Upscale(entry, sub[0]);

    if (!ClipperLib::Orientation(sub[0]))
        ClipperLib::ReversePath(sub[0]);

    for (unsigned i=0; i < entryholes.Size(); i++)
    {
        ClipperLib::Path& hole = sub[i+1];
        Clipper_Upscale(entryholes[i], hole);

        // holes must have inversed orientation
        if (ClipperLib::Orientation(hole))
            ClipperLib::ReversePath(hole);
    }

    Clipper_Upscale(clip, clp[0]);

    clipper_.AddPaths(sub, ClipperLib::ptSubject, true);
    clipper_.AddPaths(clp, ClipperLib::ptClip, true);

    clipper_.Execute(ClipperLib::ctIntersection, solution, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    if (solution.size())
    {
        Vector<unsigned> contourids;
        Vector<unsigned> holeids;

        for (unsigned i=0; i < solution.size(); i++)
        {
            if (ClipperLib::Orientation(solution[i]))
                contourids.Push(i);
            else
                holeids.Push(i);
        }

        URHO3D_LOGDEBUGF("ClipPolygonWithHoles() : results => numcontours=%u numholes=%u...", contourids.Size(), holeids.Size());

        // Downscale the contours
        unsigned numInitialContours = contours.Size();
        contours.Resize(numInitialContours + contourids.Size());
        resultholes.Resize(contours.Size());
        for (unsigned i=0; i < contourids.Size(); i++)
        {
            ClipperLib::Path& contour = solution[contourids[i]];
            if (ClipperLib::Orientation(contour))
                ClipperLib::ReversePath(contour);
            Clipper_Downscale(contour, contours[numInitialContours+i]);
        }

        if (contourids.Size() > 1)
        {
            // Need to Reattribute the holes to the good contour

            // Copy Holes to a temporary downscaled vectors
            Vector<PODVector<Vector2> > holes;
            holes.Resize(holeids.Size());
            for (unsigned i=0; i < holeids.Size(); i++)
            {
                ClipperLib::Path& hole = solution[holeids[i]];
                if (ClipperLib::Orientation(hole))
                    ClipperLib::ReversePath(hole);
                Clipper_Downscale(hole, holes[i]);
            }

            // calculate the boundingrect for each contours
            Vector<Rect> contourRects;
            contourRects.Resize(contourids.Size());
            for (unsigned i=0; i < contourids.Size(); i++)
            {
                const PODVector<Vector2>& contour = contours[numInitialContours+i];
                const unsigned numpoints = contour.Size();
                Rect& rect = contourRects[i];
                rect.Define(contour[0], contour[numpoints-1]);
                for (unsigned j=1; j < numpoints-1; j++)
                    rect.Merge(contour[j]);
            }

            // distribute the holes to the contours
            Rect holerect;
            for (unsigned i=0; i < holes.Size(); i++)
            {
                // calculate the boundingrect of the hole
                const PODVector<Vector2>& holepoints = holes[i];
                const unsigned numpoints = holepoints.Size();
                holerect.Define(holepoints[1], holepoints[numpoints-1]);
                for (unsigned j=1; j < numpoints-1; j++)
                    holerect.Merge(holepoints[j]);

                for (unsigned j=0; j < contourids.Size(); j++)
                {
                    if (contourRects[j].IsInside(holerect) != OUTSIDE)
                    {
                        URHO3D_LOGDEBUGF("ClipPolygonWithHoles() : ... hole=%u (numpoints=%u) is attributed to contour=%u", i, holepoints.Size(), j);
                        Vector<PODVector<Vector2> >& contourholes = resultholes[numInitialContours+j];
                        contourholes.Push(holepoints);
                        continue;
                    }
                }
            }
        }
        else
        {
            // Downscale the holes
            resultholes.Back().Resize(holeids.Size());
            for (unsigned i=0; i < holeids.Size(); i++)
            {
                ClipperLib::Path& hole = solution[holeids[i]];
                if (ClipperLib::Orientation(hole))
                    ClipperLib::ReversePath(hole);
                Clipper_Downscale(hole, resultholes.Back()[i]);
            }
        }

        URHO3D_LOGDEBUGF("-------------------------------------------------------------");

        return true;
    }
#endif // ACTIVE_RENDERSHAPE_CLIPPING
    return false;
}

bool WidenPolygon(const PODVector<Vector2>& entry, float offset, Vector<PODVector<Vector2> >& results, bool useClipper)
{
#ifdef ACTIVE_RENDERSHAPE_CLIPPING
    if (useClipper)
    {
        clipperOffset_.Clear();

        ClipperLib::Paths sub(1);
        ClipperLib::Paths solution;

        // UpScale Float to Int
        Clipper_Upscale(entry, sub[0]);

        clipperOffset_.AddPaths(sub, ClipperLib::jtSquare, ClipperLib::etClosedPolygon);

        clipperOffset_.Execute(solution, offset);

        if (solution.size() && solution[0].size())
        {
            ClipperLib::ReversePaths(solution);

            // DownScale Int to Float
            Clipper_Downscale(solution, results);

            return true;
        }

        return false;
    }
    else
#endif // ACTIVE_RENDERSHAPE_CLIPPING
    {
        ScaleManhattanContour(entry, offset, results);
    }

    return true;
}

