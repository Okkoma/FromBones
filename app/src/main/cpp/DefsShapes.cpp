#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include "GameOptions.h"
#include "GameOptionsTest.h"
#include "GameHelpers.h"

#include "Map.h"
#include "MapWorld.h"

#include "DefsMap.h"

#include "Libs/clipping.h"
#include "Libs/fastpoly2tri.h"

#define EMBOSE_TRIANGLE


PODVector<unsigned char> PolyShape::sMemory_;
PODVector<Vector2> PolyShape::sTempContour_;
PODVector<unsigned> PolyShape::sSegmentsOnMapBorder_;

PolyShape::PolyShape()
{
    embose_ = 0.f;
}

void PolyShape::Allocate()
{
    sMemory_.Resize(MPE_PolyMemoryRequired(1000));
    sTempContour_.Reserve(1000);
    sSegmentsOnMapBorder_.Reserve(1000);
}

void PolyShape::DeAllocate()
{
    sMemory_.Clear();
    sTempContour_.Clear();
    sSegmentsOnMapBorder_.Clear();
}

void PolyShape::Clear()
{
    embose_ = 0.f;

    clippedRect_.Clear();
    worldBoundingRect_.Clear();

    contours_.Clear();
    holes_.Clear();

    clippedContours_.Clear();
    clippedHoles_.Clear();

    innerContours_.Clear();
    enlargedHoles_.Clear();

    triangles_.Clear();
    emboses_.Clear();
    emboseholes_.Clear();
    mapbordervertices_.Clear();

    shapeDirty_ = false;
}


void PolyShape::AddContour(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> > *holesptr)
{
    contours_ += contour;
    holes_.Resize(contours_.Size());

    if (holesptr)
        holes_.Back() = *holesptr;

    shapeDirty_ = true;

    Sanitate(false);
}

void PolyShape::AdjustHoles(const PODVector<Vector2>& contour, Vector<PODVector<Vector2> >& holes)
{
    URHO3D_LOGDEBUGF("PolyShape() - AdjustHoles : contour ... numpoints=%u numholes=%u !", contour.Size(), holes.Size());

    for (unsigned i=0; i < holes.Size(); i++)
        GameHelpers::OffsetEqualVertices(contour, holes[i], 0.001f, true);
}

void PolyShape::SplitContour(int icontour, Vector<PODVector<Vector2> >& contours, Vector<Vector<PODVector<Vector2> > >& gholes)
{
    const unsigned maxcontours = contours[icontour].Size() / 4;

    URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d start splitting ... numpoints=%u maxcontours=%u !", icontour, contours[icontour].Size(), maxcontours);

    if (maxcontours < 2)
        return;

    Vector<IntVector2 > splitters;
    Vector<int> contourids;
    int contourid = icontour;

    // copy the entry contour
    sTempContour_ = contours[icontour];

    const unsigned numpoints = sTempContour_.Size();
    if (numpoints < 4)
        return;

    // clear the entry contour, keep only the first point
    contours[icontour].Resize(1);

    URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d ... push start point[0]=%F %F ...", contourid, contours[contourid].Back().x_, contours[contourid].Back().y_);

    int i=0, j=1;

    splitters.Clear();
    contourids.Clear();
    contourids.Push(icontour);

    for (;;)
    {
        if (j >= numpoints || (splitters.Size() && j > splitters.Back().x_))
        {
            i++;
            j = splitters.Size() ? splitters.Back().y_ : 1;

            // fill the current contour with the current point
            if (i < numpoints)
            {
                URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d ... end jloop ... go to i=%d j=%d ...", contourid, i, j);

                contours[contourid].Push(sTempContour_[i]);

                URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d ... push index=%d to point[%d]=%F %F !", contourid, i, contours[contourid].Size()-1, contours[contourid].Back().x_, contours[contourid].Back().y_);
            }
            // has finished to split the entry contour
            else
            {
                URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d ... end of the entry contour !", contourid);
                break;
            }
        }

        if (j != i)
        {
            if (AreEquals(sTempContour_[i], sTempContour_[j], 0.001f))
            {
                URHO3D_LOGDEBUGF("PolyShape() - SplitContour : indexi=%d indexj=%d points are close ...", i, j);

                // check if the origin split point is the same but with the swapped indexes
                if (splitters.Size() && splitters.Back().x_ == i && splitters.Back().y_ == j)
                {
                    // modify the coord of splitter point to not have the same point in these 2 contours.
                    PODVector<Vector2>& currentContour = contours[contourid];
                    Vector2& splitterVertex = currentContour.Back();
                    splitterVertex += GameHelpers::GetManhattanVector(currentContour.Size()-1, currentContour) * 0.01f;

                    // previous contour
                    contourids.Pop();

                    URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d ... indexi=%d indexj=%d close the contour and return to the previous contour=%d !", contourid, i, j, contourids.Back());

                    contourid = contourids.Back();

                    // remove the splitter
                    splitters.Resize(splitters.Size()-1);
                }
                // insert a new splitter
                else if (!splitters.Size() || splitters.Back().y_ != i || splitters.Back().x_ != j)
                {
                    if (contourid >= maxcontours)
                    {
                        URHO3D_LOGDEBUGF("PolyShape() - SplitContour : contour=%d ... indexi=%d indexj=%d maxContours=%u reached => break !", contourid, i, j, maxcontours);
                        break;
                    }

                    // insert the split point
                    splitters.Push(IntVector2(j, i));

                    // allocate new contour
                    contours.Resize(contours.Size()+1);

                    // allocate and duplicate the holes for the new contour
                    gholes.Resize(contours.Size());
                    gholes.Back() = gholes.At(contourid);

                    // set the new contour as current
                    contourid = contours.Size()-1;
                    contourids.Push(contourid);

                    // restart index for the new contour
                    j = i;
                }
            }
        }

        j++;
    }
}

void PolyShape::Sanitate(bool clipped)
{
    Vector<PODVector<Vector2> >& contours = clipped ? clippedContours_ : contours_;
    Vector<Vector<PODVector<Vector2> > >& gholes = clipped ? clippedHoles_ : holes_;

    if (gholes.Size() != contours.Size())
        gholes.Resize(contours.Size());

    unsigned initialNumContours = contours.Size();
    for (unsigned icontour=0; icontour < initialNumContours; icontour++)
    {
        const unsigned numpoints = contours[icontour].Size();
        if (!numpoints)
            continue;

        // reserve the max contours to be setted
        const unsigned maxcontours = numpoints / 4;
        contours.Reserve(contours.Size() + maxcontours - 1);
        gholes.Reserve(contours.Size() + maxcontours - 1);

        // remove the intersections in each hole of the contour
        Vector<PODVector<Vector2> >& holes = gholes[icontour];
        for (unsigned i=0; i < holes.Size(); i++)
            GameHelpers::OffsetEqualVertices(holes[i], 0.005f);

        // find the splitter points, split the contour and redistribute the holes
        SplitContour(icontour, contours, gholes);
    }

    UpdateBoundingRects(clipped);
    Vector<Rect>& boundingRects = clipped ? clippedBoundingRects_ : boundingRects_;

    // remove outside holes
    for (unsigned i=0; i < contours.Size(); i++)
    {
        if (!contours[i].Size())
            continue;

        const PODVector<Vector2>& contour = contours[i];
        Vector<PODVector<Vector2> >& holes = gholes[i];

        if (!holes.Size())
            continue;

        unsigned ihole = 0;
        while (ihole < holes.Size())
        {
            PODVector<Vector2>& holepoints = holes[ihole];

            if (holepoints.Size() < 3)
            {
                // Remove
                holes.EraseSwap(ihole);
                continue;
            }

            // check if hole is inside the contour : if not remove it
            if (!GameHelpers::IsInsidePolygon(holepoints.Front(), contour))
            {
                holes.EraseSwap(ihole);
                continue;
            }

            ihole++;
        }
    }
#ifdef ACTIVE_RENDERSHAPE_CLIPPING
    if (!clipped)
    {
        // adjust hole if have adjacent point with contour
        for (unsigned i=0; i < contours.Size(); i++)
            AdjustHoles(contours[i], gholes[i]);
    }
#endif
}


void PolyShape::Shrink(float offset, bool clipped)
{
    // Shrink Contours and Holes

    Vector<PODVector<Vector2> >& contours = clipped ? clippedContours_ : contours_;
    Vector<Vector<PODVector<Vector2> > >& gholes = clipped ? clippedHoles_ : holes_;

    innerContours_.Resize(contours.Size());
    enlargedHoles_.Resize(gholes.Size());

    for (unsigned icontour=0; icontour < contours.Size(); icontour++)
    {
        if (!contours[icontour].Size())
            continue;

        PODVector<Vector2>& contour = contours[icontour];
        PODVector<Vector2>& innerContour = innerContours_[icontour];

        // shrink the contour
        ScaleManhattanContour(contours[icontour], -offset, innerContours_[icontour]);

        // enlarge the holes
        Vector<PODVector<Vector2> >& holes = gholes[icontour];
        Vector<PODVector<Vector2> >& enlargedHoles = enlargedHoles_[icontour];

        enlargedHoles.Resize(holes.Size());
        for (int i=0; i < holes.Size(); i++)
            ScaleManhattanContour(holes[i], offset, enlargedHoles[i]);

        // for each hole, modify the contour if the hole cuts it and remove the hole
        Vector<PODVector<Vector2> >::Iterator it = enlargedHoles.Begin();
        Vector<PODVector<Vector2> >::Iterator jt = holes.Begin();
        while (it != enlargedHoles.End())
        {
            /// TODO : RAF in CutContour => if intersection, fusion the hole with the contour
            if (CutContour(innerContour, *it))
            {
                it = enlargedHoles.Erase(it);
                jt = holes.Erase(jt);
            }
            else
            {
                it++;
                jt++;
            }
        }
    }
}

void PolyShape::SetEmboseParameters(float embose, const MapCollider* mapcollider, const Rect* mapBorder)
{
    embose_ = embose;
    mapCollider_ = mapcollider;
    mapBorder_ = mapBorder;
}

void PolyShape::Embose(bool clipped)
{
    if (!embose_)
        return;

    // Shrink the shape (shrink the contour and enlarge holes)
    Shrink(embose_, clipped);

    // Create the embose border between the hole and the innerhole
    Vector<Vector<PODVector<Vector2> > >& gholes = !clipped ? holes_ : clippedHoles_;

    emboseholes_.Clear();
    unsigned numcontours = Min(gholes.Size(), enlargedHoles_.Size());
    for (unsigned i=0; i < numcontours; i++)
    {
        const Vector<PODVector<Vector2> >& innerholes = gholes[i];
        const Vector<PODVector<Vector2> >& outerholes = enlargedHoles_[i];

        unsigned numholes = Min(innerholes.Size(), outerholes.Size());
        for (unsigned j=0; j < numholes; j++)
            CreateVerticesBetween(innerholes[j], outerholes[j], emboseholes_);
    }

    // Create vertices between the innercoutours and the outercoutours
    CreateEmboseVertices();
}

void PolyShape::Triangulate(int contourid)
{
#ifdef ACTIVE_RENDERSHAPE_EMBOSE
    const bool embosed = embose_ != 0.f;
#else
    embose_ = 0.f;
    const bool embosed = false;
#endif

    const bool clipped = IsClipped();

    // Sanitate the contours and the holes
    Sanitate(clipped);

    const Vector<PODVector<Vector2> >& contours = embosed ? innerContours_ : clipped ? clippedContours_ : contours_;
    Vector<Vector<PODVector<Vector2> > >& holes = embosed ? enlargedHoles_ : clipped ? clippedHoles_ : holes_;

    // Embose if needed
    if (embose_)
    {
        Embose(clipped);
    }
    else
    {
        emboses_.Clear();
        emboseholes_.Clear();
        mapbordervertices_.Clear();
        innerContours_.Clear();
        enlargedHoles_.Clear();
    }

    triangles_.Clear();

    if (holes.Size() != contours.Size())
        holes.Resize(contours.Size());

    if (contourid == -1)
    {
        for (unsigned i=0; i < contours.Size(); i++)
        {
            if (contours[i].Size())
            {
                URHO3D_LOGDEBUGF("PolyShape() - Triangulate : contourid=%d/%u numpoints=%u numholes=%u ...", i, contours.Size(), contours[i].Size(), holes[i].Size());
                GameHelpers::TriangulateShape(contours[i], holes[i], triangles_, sMemory_);
            }
        }
    }
    else
    {
        if (contourid < contours.Size() && contours[contourid].Size())
            GameHelpers::TriangulateShape(contours[contourid], holes[contourid], triangles_, sMemory_);
    }
}

void PolyShape::Clip(const Rect& cliprect, bool useClipper, const Matrix2x3& transform, bool debug)
{
    clippedRect_ = cliprect;

    clippedContours_.Clear();
    clippedHoles_.Clear();

    if (!worldBoundingRect_.Defined() || worldBoundingRect_.IsInside(cliprect) != OUTSIDE)
    {
        Rect localClipRect(transform * cliprect.min_, transform * cliprect.max_);

        PODVector<Vector2> clipPolygon(4);
        clipPolygon[0] = localClipRect.min_;
        clipPolygon[1] = Vector2(localClipRect.min_.x_, localClipRect.max_.y_);
        clipPolygon[2] = localClipRect.max_;
        clipPolygon[3] = Vector2(localClipRect.max_.x_, localClipRect.min_.y_);

        if (boundingRects_.Size() != contours_.Size())
            UpdateBoundingRects(false);

        URHO3D_LOGDEBUGF("PolyShape : %u clip numcontours=%u ...", this, contours_.Size());

        for (unsigned i=0; i < contours_.Size(); i++)
        {
            Intersection inter = localClipRect.IsInside(boundingRects_[i]);
            if (inter == OUTSIDE)
            {
                URHO3D_LOGDEBUGF("PolyShape : %u => contour %d/%u outside cliprect skip !", this, i, contours_.Size());
                continue;
            }
            else
            {
                bool clipok = false;

                const PODVector<Vector2>& contour = contours_[i];
                const Vector<PODVector<Vector2> >& holes = holes_[i];

                int startClippedContour = clippedContours_.Size();
                int numClippedContours = 1;

                if (inter == INTERSECTS)
                {
                    URHO3D_LOGDEBUGF("PolyShape : %u => contour %d/%u intersects cliprect : use clipPolygon numEntryHoles=%u ...", this, i, contours_.Size(), holes.Size());

                    clipok = holes.Size() ? ClipPolygonWithHoles(clipPolygon, contour, holes, clippedContours_, clippedHoles_) : ClipPolygon(clipPolygon, contour, clippedContours_, useClipper);

                    numClippedContours = Max((int)clippedContours_.Size() - startClippedContour, 0);

                    URHO3D_LOGDEBUGF("PolyShape : %u => contour %d/%u intersects the cliprect : clipOk=%s clippedContours id=%d to id=%d ...",
                                     this, i, contours_.Size(), clipok ? "true":"false", startClippedContour, startClippedContour + numClippedContours - 1);
                }

                if (!clipok)
                {
                    clippedContours_.Push(contour);
                    clippedHoles_.Push(holes);

                    URHO3D_LOGDEBUGF("PolyShape : %u => contour %d/%u is inside cliprect : just copy contour => clippedContour id=%u ...",  this, i, contours_.Size(), clippedContours_.Size()-1);
                }

                if (debug && clipok)
                    DumpContours(i, true, startClippedContour, numClippedContours);

            }
        }

        URHO3D_LOGDEBUGF("PolyShape : %u clip ... OK !", this);

        shapeDirty_ = true;

        URHO3D_LOGDEBUGF("PolyShape : %u triangulate numclippedcontours=%u ...", this, clippedContours_.Size());

        Triangulate();

        URHO3D_LOGDEBUGF("PolyShape : %u triangulate numclippedcontours=%u ... OK !", this, clippedContours_.Size());
    }
}

void PolyShape::CreateVerticesBetween(const PODVector<Vector2>& outercontour, const PODVector<Vector2>& innercontour, PODVector<Vector2>& vertices, PODVector<unsigned>* segmentsOnMapBorder)
{
    unsigned numfaces = Min(outercontour.Size(), innercontour.Size());
    unsigned startvertice = vertices.Size();

    if (segmentsOnMapBorder)
    {
        PODVector<Vector2>& emboseverticesonmapborder = mapbordervertices_;
        unsigned startindex = startvertice;
        unsigned startonmapborder = emboseverticesonmapborder.Size();
        const PODVector<unsigned>& onmapborder = *segmentsOnMapBorder;
        int s = 0;

        // create the triangles between the 2 contours
        // 2 triangles by face
        // the first one is the most outside
#ifdef EMBOSE_TRIANGLE
        vertices.Resize(startvertice + numfaces * 2 * 3);
        emboseverticesonmapborder.Resize(startonmapborder + onmapborder.Size() * 6);

        for (int i=0; i < numfaces; i++)
        {
            int j = i+1 < numfaces ? i+1 : 0;

            // if this vertice is on the map border
            if (s < onmapborder.Size() && i == onmapborder[s])
            {
                unsigned start = startonmapborder + s * 6;
                emboseverticesonmapborder[start] = innercontour[i];
                emboseverticesonmapborder[start+1] = outercontour[i];
                emboseverticesonmapborder[start+2] = outercontour[j];
                emboseverticesonmapborder[start+3] = outercontour[j];
                emboseverticesonmapborder[start+4] = innercontour[j];
                emboseverticesonmapborder[start+5] = innercontour[i];
                s++;
                continue;
            }

            vertices[startindex] = innercontour[i];
            vertices[startindex+1] = outercontour[i];
            vertices[startindex+2] = outercontour[j];
            vertices[startindex+3] = outercontour[j];
            vertices[startindex+4] = innercontour[j];
            vertices[startindex+5] = innercontour[i];
            startindex += 6;
        }
        vertices.Resize(vertices.Size() - s * 6);
#else
        // quad version
        vertices.Resize(startvertice + numfaces * 4);
        emboseverticesonmapborder.Resize(startonmapborder + onmapborder.Size() * 4);
        for (int i=0; i < numfaces; i++)
        {
            int j = i+1 < numfaces ? i+1 : 0;

            // if this vertice is on the map border
            if (s < onmapborder.Size() && i == onmapborder[s])
            {
                unsigned start = startonmapborder + s * 4;
                emboseverticesonmapborder[start] = outercontour[i];
                emboseverticesonmapborder[start+1] = outercontour[j];
                emboseverticesonmapborder[start+2] = innercontour[j];
                emboseverticesonmapborder[start+3] = innercontour[i];
                s++;
                continue;
            }

            vertices[startindex] = outercontour[i];
            vertices[startindex+1] = outercontour[j];
            vertices[startindex+2] = innercontour[j];
            vertices[startindex+3] = innercontour[i];
            startindex += 4;
        }
        vertices.Resize(vertices.Size() - s * 4);
#endif
    }
    else
    {
        // create the triangles between the 2 contours
        // 2 triangles by face
        // the first one is the most outside
#ifdef EMBOSE_TRIANGLE
        vertices.Resize(startvertice + numfaces * 2 * 3);
        for (int i=0; i < numfaces; i++)
        {
            int j = i+1 < numfaces ? i+1 : 0;
            unsigned startindex = i * 6;
            vertices[startvertice + startindex] = innercontour[i];
            vertices[startvertice + startindex+1] = outercontour[i];
            vertices[startvertice + startindex+2] = outercontour[j];
            vertices[startvertice + startindex+3] = outercontour[j];
            vertices[startvertice + startindex+4] = innercontour[j];
            vertices[startvertice + startindex+5] = innercontour[i];
        }
#else
        // quad version
        vertices.Resize(startvertice + numfaces * 4);
        for (int i=0; i < numfaces; i++)
        {
            int j = i+1 < numfaces ? i+1 : 0;
            unsigned startindex = i * 4;
            vertices[startvertice + startindex] = outercontour[i];
            vertices[startvertice + startindex+1] = outercontour[j];
            vertices[startvertice + startindex+2] = innercontour[j];
            vertices[startvertice + startindex+3] = innercontour[i];
        }
#endif
    }
}

void PolyShape::CreateEmboseVertices()
{
    const Vector<PODVector<Vector2> >& contours = IsClipped() ? clippedContours_ : contours_;

    emboses_.Clear();
    mapbordervertices_.Clear();
    sSegmentsOnMapBorder_.Clear();

    innerContours_.Resize(contours.Size());

    // Create the embose vertices between the outer and inner contours using segments on map border
    if (mapBorder_)
    {
        for (unsigned i=0; i < contours.Size(); i++)
        {
            // copy outer contour to temp
            sTempContour_ = contours[i];
            // get modified contour with map segments
            GetSegmentsOnMapBorder(mapCollider_, *mapBorder_, sTempContour_, sSegmentsOnMapBorder_);

            // create the innercontour by applying the embose offset
            ScaleManhattanContour(sTempContour_, -embose_, innerContours_[i]);

            // create the vertices
            CreateVerticesBetween(sTempContour_, innerContours_[i], emboses_, sSegmentsOnMapBorder_.Size() ? &sSegmentsOnMapBorder_ : 0);
        }
    }
    // Create the embose vertices between the outer and inner contours
    else
    {
        for (unsigned i=0; i < contours.Size(); i++)
        {
            // create the innercontour by applying the embose offset
            ScaleManhattanContour(contours[i], -embose_, innerContours_[i]);

            // create the vertices
            CreateVerticesBetween(contours[i], innerContours_[i], emboses_, 0);
        }
    }
}


void PolyShape::UpdateBoundingRects(bool clipped)
{
    // calculate the rects for each contour
    const Vector<PODVector<Vector2> >& contours = clipped ? clippedContours_ : contours_;
    Vector<Rect >& boundingRects = clipped ? clippedBoundingRects_ : boundingRects_;

    boundingRects.Resize(contours.Size());
    for (unsigned icontour=0; icontour < contours.Size(); icontour++)
    {
        const PODVector<Vector2>& contour = contours[icontour];

        Rect& boundingRect = boundingRects[icontour];
        boundingRect.Clear();

        if (contour.Size())
        {
            boundingRect.Define(contour.Front(), contour.Back());
            for (unsigned j=1; j < contour.Size()-1; j++)
                boundingRect.Merge(contour[j]);
        }
    }
}

void PolyShape::UpdateWorldBoundingRect(const Matrix2x3& worldTransform)
{
    worldBoundingRect_.Clear();

    for (unsigned i=0; i < boundingRects_.Size(); i++)
        worldBoundingRect_.Merge(boundingRects_[i]);

    worldBoundingRect_.Transform(worldTransform);
}

void PolyShape::DumpContours(unsigned contourid, bool clipped, unsigned startid, unsigned numcontours) const
{
    const Vector<PODVector<Vector2> >& contours = clipped ? clippedContours_ : contours_;

    numcontours = Min(startid + numcontours, contours.Size());

    for (unsigned i=startid; i < numcontours; i++)
    {
        URHO3D_LOGINFOF("PolyShape - DumpContours : this=%u contourid=%u %s=%u ...", this, contourid, clipped ? "clippedContourid":"contourid", i);

        if (startid < contours.Size())
        {
            const PODVector<Vector2>& contour = contours[i];

            for (unsigned j=0; j < contour.Size(); j++)
                URHO3D_LOGINFOF(" point[%u] x=%F y=%F", j, contour[j].x_, contour[j].y_);
        }
    }
}
