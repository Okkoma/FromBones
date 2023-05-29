#pragma once

using namespace Urho3D;

#include "DefsCore.h"

struct FROMBONES_API PolyShape
{
    PolyShape();

    static void Allocate();
    static void DeAllocate();

    void Clear();
    void SetEmboseParameters(float embose, const MapCollider* mapcollider, const Rect* mapBorder=0);
    void AddContour(const PODVector<Vector2>& contour, const Vector<PODVector<Vector2> > *holesptr=0);
    void AdjustHoles(const PODVector<Vector2>& contour, Vector<PODVector<Vector2> >& gholes);
    void SplitContour(int icontour, Vector<PODVector<Vector2> >& contours, Vector<Vector<PODVector<Vector2> > >& gholes);
    void Sanitate(bool clipped=false);
    void Shrink(float offset, bool clipped=false);
    void Embose(bool clipped=false);
    void Clip(const Rect& cliprect, bool useClipper=false, const Matrix2x3& transform=Matrix2x3::IDENTITY, bool debug=false);
    void Triangulate(int contourid=-1);

    bool IsClipped() const
    {
        return clippedRect_.Defined();
    }

    void CreateVerticesBetween(const PODVector<Vector2>& outercontour, const PODVector<Vector2>& innercontour, PODVector<Vector2>& vertices, PODVector<unsigned>* segmentsOnMapBorder=0);
    void CreateEmboseVertices();

    void UpdateBoundingRects(bool clipped);
    void UpdateWorldBoundingRect(const Matrix2x3& worldTransform);

    void DumpContours(unsigned contourid, bool clipped, unsigned startid, unsigned numcontours=100) const;

    Rect worldBoundingRect_, clippedRect_;
    bool shapeDirty_;
    float embose_;
    const Rect* mapBorder_;
    const MapCollider* mapCollider_;

    // Contours
    Vector<PODVector<Vector2> > contours_; // the contours or the outer contours if embose
    Vector<Vector<PODVector<Vector2> > > holes_; // the holes for each contour
    Vector<Rect > boundingRects_; // the boundingRect for each contour

    // Clipped
    Vector<PODVector<Vector2> > clippedContours_; // the clipped contours or the outer contours if embose
    Vector<Vector<PODVector<Vector2> > > clippedHoles_; // the clipped holes
    Vector<Rect > clippedBoundingRects_;

    // Embosed contours and holes (clipped if clip actived)
    Vector<PODVector<Vector2> > innerContours_; // the inner contours if embose
    Vector<Vector<PODVector<Vector2> > > enlargedHoles_; // the outer holes contours for each contour

    // Vertices Points
    PODVector<Vector2> triangles_; // the vertices inside the contours.
    PODVector<Vector2> emboses_; // the vertices for the embosed border
    PODVector<Vector2> emboseholes_; // the vertices for the embosed holes
    PODVector<Vector2> mapbordervertices_; // the vertices for the mapborder no embose

private:
    static PODVector<unsigned char> sMemory_;
    static PODVector<Vector2> sTempContour_;
    static PODVector<unsigned> sSegmentsOnMapBorder_;
};
