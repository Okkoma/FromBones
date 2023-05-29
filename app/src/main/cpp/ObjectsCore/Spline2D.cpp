#include <Urho3D/Urho3D.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include "Spline2D.h"


Spline2D::Spline2D() :
    interpolationMode_(BEZIER_CURVE)
{
}

Spline2D::Spline2D(InterpolationMode mode) :
    interpolationMode_(mode)
{
}

Spline2D::Spline2D(const Vector<Vector2>& knots, InterpolationMode mode) :
    interpolationMode_(mode),
    knots_(knots)
{
}

Spline2D::Spline2D(const Spline2D& rhs) :
    interpolationMode_(rhs.interpolationMode_),
    knots_(rhs.knots_)
{
}

Vector2 Spline2D::GetPoint(float f) const
{
    if (knots_.Size() < 2)
        return knots_.Size() == 1 ? knots_[0] : Vector2::ZERO;

    if (f > 1.f)
        f = 1.f;
    else if (f < 0.f)
        f = 0.f;

    switch (interpolationMode_)
    {
    case BEZIER_CURVE:
        return BezierInterpolation(knots_, f);
    case CATMULL_ROM_CURVE:
        return CatmullRomInterpolation(knots_, f);
    case LINEAR_CURVE:
        return LinearInterpolation(knots_, f);
    case CATMULL_ROM_FULL_CURVE:
    {
        /// \todo Do not allocate a new vector each time
        Vector<Vector2> fullKnots;
        if (knots_.Size() > 1)
        {
            // Non-cyclic case: duplicate start and end
            if (knots_.Front() != knots_.Back())
            {
                fullKnots.Push(knots_.Front());
                fullKnots.Push(knots_);
                fullKnots.Push(knots_.Back());
            }
            // Cyclic case: smooth the tangents
            else
            {
                fullKnots.Push(knots_[knots_.Size() - 2]);
                fullKnots.Push(knots_);
                fullKnots.Push(knots_[1]);
            }
        }
        return CatmullRomInterpolation(fullKnots, f);
    }

    default:
        URHO3D_LOGERROR("Spline2D Unsupported interpolation mode");
        return Vector2::ZERO;
    }
}

void Spline2D::SetKnot(const Vector2& knot, unsigned index)
{
    if (index < knots_.Size())
        knots_[index] = knot;
}

void Spline2D::AddKnot(const Vector2& knot)
{
    knots_.Push(knot);
}

void Spline2D::AddKnot(const Vector2& knot, unsigned index)
{
    if (index < knots_.Size())
        knots_.Insert(index, knot);
    else
        knots_.Push(knot);
}

Vector2 Spline2D::BezierInterpolation(const Vector<Vector2>& knots, float t) const
{
    if (knots.Size() == 2)
    {
        return knots[0].Lerp(knots[1], t);
    }
    else
    {
        /// \todo Do not allocate a new vector each time
        Vector<Vector2> interpolatedKnots;
        for (unsigned i = 1; i < knots.Size(); i++)
            interpolatedKnots.Push(knots[i - 1].Lerp(knots[i], t));

        return BezierInterpolation(interpolatedKnots, t);
    }
}

Vector2 CalculateCatmullRom(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float t, float t2, float t3)
{
    return 0.5f * ((2.0f * p1) + (-p0 + p2) * t +
                   (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                   (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
}

Vector2 Spline2D::CatmullRomInterpolation(const Vector<Vector2>& knots, float t) const
{
    if (knots.Size() < 4)
        return Vector2::ZERO;
    else
    {
        if (t >= 1.f)
            return knots[knots.Size() - 2];

        int originIndex = static_cast<int>(t * (knots.Size() - 3));
        t = fmodf(t * (knots.Size() - 3), 1.f);
        float t2 = t * t;
        float t3 = t2 * t;

        return CalculateCatmullRom(knots[originIndex], knots[originIndex + 1],
                                   knots[originIndex + 2], knots[originIndex + 3], t, t2, t3);
    }
}

Vector2 Spline2D::LinearInterpolation(const Vector<Vector2>& knots, float t) const
{
    if (knots.Size() < 2)
        return Vector2::ZERO;
    else
    {
        if (t >= 1.f)
            return knots.Back();

        int originIndex = Clamp((int)(t * (knots.Size() - 1)), 0, (int)(knots.Size() - 2));
        t = fmodf(t * (knots.Size() - 1), 1.f);
        return knots[originIndex].Lerp(knots[originIndex + 1], t);
    }
}

void Spline2D::DrawDebugGeometry(DebugRenderer* debug, const Vector2& origin, const Color& color, bool depthTest) const
{
    for (unsigned i=0; i < knots_.Size(); i++)
        debug->AddCross(Vector3(origin + knots_[i]), 0.5f, color, false);

    Vector3 v0 = origin + GetPoint(0.f);
    Vector3 v1;
    for (unsigned i=1; i < 64; i++)
    {
        v1 = origin + GetPoint((float)i/63);
        debug->AddLine(v0, v1, color, false);
        v0 = v1;
    }
}
