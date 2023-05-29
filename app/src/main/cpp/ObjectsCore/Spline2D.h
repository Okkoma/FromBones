#pragma once

#include <Urho3D/Core/Spline.h>
#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Math/Vector2.h>
#include <Urho3D/Container/Vector.h>


namespace Urho3D
{
class DebugRenderer;
}

using namespace Urho3D;


/// Spline class to get a point on it based off the interpolation mode.
class Spline2D
{

public:
    /// Default constructor.
    Spline2D();
    /// Constructor setting interpolation mode.
    Spline2D(InterpolationMode mode);
    /// Constructor setting knots and interpolation mode.
    Spline2D(const Vector<Vector2>& knots, InterpolationMode mode = BEZIER_CURVE);
    /// Copy constructor.
    Spline2D(const Spline2D& rhs);

    /// Copy operator.
    void operator =(const Spline2D& rhs)
    {
        knots_ = rhs.knots_;
        interpolationMode_ = rhs.interpolationMode_;
    }

    /// Equality operator.
    bool operator ==(const Spline2D& rhs) const
    {
        return (knots_ == rhs.knots_ && interpolationMode_ == rhs.interpolationMode_);
    }

    /// Inequality operator.
    bool operator !=(const Spline2D& rhs) const
    {
        return !(*this == rhs);
    }

    /// Return the interpolation mode.
    InterpolationMode GetInterpolationMode() const
    {
        return interpolationMode_;
    }

    /// Return the knots of the spline.
    const Vector<Vector2>& GetKnots() const
    {
        return knots_;
    }

    /// Return the knot at the specific index.
    const Vector2& GetKnot(unsigned index) const
    {
        return knots_[index];
    }

    /// Return the T of the point of the spline at f from 0.f - 1.f.
    Vector2 GetPoint(float f) const;

    /// Set the interpolation mode.
    void SetInterpolationMode(InterpolationMode interpolationMode)
    {
        interpolationMode_ = interpolationMode;
    }

    /// Set the knots of the spline.
    void SetKnots(const Vector<Vector2>& knots)
    {
        knots_ = knots;
    }

    /// Set the value of an existing knot.
    void SetKnot(const Vector2& knot, unsigned index);
    /// Add a knot to the end of the spline.
    void AddKnot(const Vector2& knot);
    /// Add a knot to the spline at a specific index.
    void AddKnot(const Vector2& knot, unsigned index);

    /// Remove the last knot on the spline.
    void RemoveKnot()
    {
        knots_.Pop();
    }

    /// Remove the knot at the specific index.
    void RemoveKnot(unsigned index)
    {
        knots_.Erase(index);
    }

    /// Clear the spline.
    void Clear()
    {
        knots_.Clear();
    }

    void DrawDebugGeometry(DebugRenderer* debug, const Vector2& origin, const Color& color, bool depthTest) const;

private:
    /// Perform Bezier interpolation on the spline.
    Vector2 BezierInterpolation(const Vector<Vector2>& knots, float t) const;
    /// Perform Spline interpolation on the spline.
    Vector2 CatmullRomInterpolation(const Vector<Vector2>& knots, float t) const;
    /// Perform linear interpolation on the spline.
    Vector2 LinearInterpolation(const Vector<Vector2>& knots, float t) const;

    /// Interpolation mode.
    InterpolationMode interpolationMode_;
    /// Knots on the spline.
    Vector<Vector2> knots_;
};

