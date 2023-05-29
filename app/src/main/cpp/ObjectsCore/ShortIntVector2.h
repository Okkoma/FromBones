#pragma once

#include <Urho3D/Math/MathDefs.h>
#include <Urho3D/Container/Str.h>

using namespace Urho3D;

/// Two-dimensional vector with short integer values.
class ShortIntVector2
{
public:
    /// Construct a zero vector.
    ShortIntVector2() :
        x_(0),
        y_(0)
    {
    }

    /// Construct from coordinates.
    ShortIntVector2(short int x, short int y) :
        x_(x),
        y_(y)
    {
    }

    /// Construct from unsigned hash.
    ShortIntVector2(unsigned hash) :
        x_(hash >> 16),
        y_(hash & 0xffff)
    {
    }

    /// Construct from an int array.
    ShortIntVector2(const short int* data) :
        x_(data[0]),
        y_(data[1])
    {
    }

    /// Copy-construct from another vector.
    ShortIntVector2(const ShortIntVector2& rhs) :
        x_(rhs.x_),
        y_(rhs.y_)
    {
    }

    /// Test for equality with another vector.
    bool operator == (const ShortIntVector2& rhs) const
    {
        return x_ == rhs.x_ && y_ == rhs.y_;
    }
    /// Test for inequality with another vector.
    bool operator != (const ShortIntVector2& rhs) const
    {
        return x_ != rhs.x_ || y_ != rhs.y_;
    }
    /// Add a vector.
    ShortIntVector2 operator + (const ShortIntVector2& rhs) const
    {
        return ShortIntVector2(x_ + rhs.x_, y_ + rhs.y_);
    }
    /// Return negation.
    ShortIntVector2 operator - () const
    {
        return ShortIntVector2(-x_, -y_);
    }
    /// Subtract a vector.
    ShortIntVector2 operator - (const ShortIntVector2& rhs) const
    {
        return ShortIntVector2(x_ - rhs.x_, y_ - rhs.y_);
    }
    /// Multiply with a scalar.
    ShortIntVector2 operator * (short int rhs) const
    {
        return ShortIntVector2(x_ * rhs, y_ * rhs);
    }
    /// Divide by a scalar.
    ShortIntVector2 operator / (short int rhs) const
    {
        return ShortIntVector2(x_ / rhs, y_ / rhs);
    }

    /// Add-assign a vector.
    ShortIntVector2& operator += (const ShortIntVector2& rhs)
    {
        x_ += rhs.x_;
        y_ += rhs.y_;
        return *this;
    }

    /// Subtract-assign a vector.
    ShortIntVector2& operator -= (const ShortIntVector2& rhs)
    {
        x_ -= rhs.x_;
        y_ -= rhs.y_;
        return *this;
    }

    /// Multiply-assign a scalar.
    ShortIntVector2& operator *= (short int rhs)
    {
        x_ *= rhs;
        y_ *= rhs;
        return *this;
    }

    /// Divide-assign a scalar.
    ShortIntVector2& operator /= (short int rhs)
    {
        x_ /= rhs;
        y_ /= rhs;
        return *this;
    }

    /// Return integer data.
    const short int* Data() const
    {
        return &x_;
    }
    /// Return as string.
    String ToString() const;
    unsigned ToHash() const;

    /// X coordinate.
    short int x_;
    /// Y coordinate.
    short int y_;

    /// Zero vector.
    static const ShortIntVector2 ZERO;
};

///// Multiply IntVector2 with a scalar.
//inline ShortIntVector2 operator * (short int lhs, const ShortIntVector2& rhs) { return rhs * lhs; }
