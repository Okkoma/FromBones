#pragma once

#include <Urho3D/Container/Vector.h>
#include <Urho3D/Math/Rect.h>

using namespace Urho3D;


struct BlitInfo
{
    void SetNumBytes(char num)
    {
        numbytes = num;
    }
    void SetDestination(char* dest, unsigned dw, unsigned dh)
    {
        dst_w = dw;
        dst_h = dh;
        dst = dest;

    }
    void SetSource(char* source, unsigned sw, unsigned sh, char* mask=0)
    {
        src_w = sw;
        src_h = sh;
        src_skip = sw;
        src = source;
        msk = mask;
    }
    void Clip(const int& dst_x, const int& dst_y)
    {
        // Dst : Out Right Border
        // cutting src_w
        if (src_w > dst_w - dst_x)
        {
            src_w = dst_w - dst_x;
        }

        // cutting border Y
        if (src_h > dst_h - dst_y)
        {
            src_h = dst_h - dst_y;
        }

        dst_start = dst + (dst_y * dst_w + dst_x) * numbytes;
    }

    unsigned dst_w;
    unsigned dst_h;
    char* dst;
    char* dst_start;

    unsigned src_w;
    unsigned src_h;
    unsigned src_skip;
    char* src;
    char* msk;

    char numbytes;
};

inline void CopyFast(char* dst, char* src, unsigned dst_dw, unsigned src_dw, unsigned src_skip, unsigned h)
{
    while (h--)
    {
        memcpy(dst, src, src_dw);

        src += src_skip;
        dst += dst_dw;
    }
}

template< typename T>
inline void AddMask(T* dst, T* src, int size, bool keepDst=true)
{
    if (keepDst)
        while (--size >= 0)
        {
            if (src[size])
                dst[size] = 0;
        }
    else
        while (--size >= 0)
        {
            dst[size] = (src[size] ? 0 : 1);
        }
}

template< typename T, typename U >
inline void Mask(T* dst, U* msk, unsigned dst_dw, unsigned msk_dw, unsigned msk_skip, unsigned h)
{
    unsigned i;

    while (h--)
    {
        for (i=0; i < msk_dw; i++)
            if (!msk[i]) dst[i] &= ~dst[i];

        msk += msk_skip;
        dst += dst_dw;
    }
}

template< typename T >
inline void CopyOR(T* dst, T* src, unsigned dst_dw, unsigned src_dw, unsigned src_skip, unsigned h)
{
    unsigned i;

    while (h--)
    {
        for (i=0; i < src_dw; i++)
            dst[i] |= src[i];

        src += src_skip;
        dst += dst_dw;
    }
}

void BlitCopy(BlitInfo& i, const IntRect& rect);


template< typename T > class Stack
{
public:
    Stack() { ; }
    Stack(const size_t size) : storage_(size), pointer_(0) { ; }
    void Reserve(const size_t size)
    {
        storage_.Reserve(size);
    }
    void Resize(const size_t size)
    {
        storage_.Resize(size);
    }
    void Clear()
    {
        pointer_ = 0;
    }
    bool Pop(T& data)
    {
        if (pointer_ > 0)
        {
            data = storage_[pointer_];
            pointer_--;
            return true;
        }
        else
        {
            return false;
        }
    }
    bool Push(const T& data)
    {
        if (pointer_ < Size() - 1)
        {
            pointer_++;
            storage_[pointer_] = data;
            return true;
        }
        else
        {
            return false;
        }
    }

    bool IsEmpty() const { return pointer_ == 0; }

    const size_t Size() const
    {
        return storage_.Size();
    }

private:
    Vector< T > storage_;
    unsigned pointer_;
};

/// if using SetBufferValue, T must be a type POD (due to memset)
template< typename T > class Matrix2D
{
public:
    Matrix2D() : width_(0), height_(0), buffer_(0) { }
    Matrix2D(unsigned width, unsigned height)
    {
        Resize(width, height);
    }
    Matrix2D(unsigned width, unsigned height, T* buffer) : width_(width), height_(height), buffer_(buffer) { }

    void Reserve(const size_t size)
    {
        storage_.Reserve(size);
    }

    void Resize(unsigned width, unsigned height)
    {
        width_ = width;
        height_ = height;
        storage_.Resize(width*height);
        buffer_ = &storage_[0];
    }

    void Clear()
    {
        width_ = height_ = 0;
        storage_.Clear();
        buffer_ = 0;
    }

    T& operator()( unsigned x, unsigned y )
    {
        return buffer_[y * width_ + x];
    }

    const T& operator()( unsigned x, unsigned y ) const
    {
        return buffer_[y * width_ + x];
    }

    T& operator[]( unsigned addr )
    {
        return buffer_[addr];
    }

    const T& operator[]( unsigned addr ) const
    {
        return buffer_[addr];
    }

    void SetBufferFrom(const T* buffer)
    {
        memcpy(Buffer(), buffer, sizeof(T) * Size());
    }
    void SetBufferValue(unsigned value)
    {
        memset(Buffer(), value, sizeof(T) * Size());
    }
    void CopyValueFrom(const T& value, const Matrix2D<T>& m, bool cleanOtherValues=false)
    {
        unsigned size = Size();

        if (cleanOtherValues)
        {
            for (unsigned i=0; i < size; i++)
                buffer_[i] = m[i] == value ? value : (T)0;
        }
        else
        {
            for (unsigned i=0; i < size; i++)
                if (m[i] == value)
                    buffer_[i] = value;
        }
    }

    const size_t Height() const
    {
        return height_;
    }
    const size_t Width() const
    {
        return width_;
    }
    const size_t Size() const
    {
        return width_ * height_;
    }

    T* Buffer()
    {
        return buffer_;
    }

private:
    // use for static buffer
    T* buffer_;
    // use for dynamic buffer
    Vector< T > storage_;
    size_t width_;
    size_t height_;
};

class PoolObject
{
public:
    PoolObject() : refcount_(0) { }

    void AddRef() { refcount_++; }
    void RemoveRef() { if (refcount_) { refcount_--; } }
    unsigned GetRefCount() const { return refcount_; }

private:
    unsigned refcount_;
};

template <class T> class Pool
{
public:
    Pool() { }
    Pool(int size) { Resize(size); }
    void Resize(unsigned size)
    {
        pool_.Resize(size);
        freeObjects_.Resize(size);
        for (unsigned i=0; i < size; i++)
            freeObjects_[i] = &pool_[i];
    }
    T* Get(bool addref=true)
    {
        T* obj = 0;
        if (freeObjects_.Size())
        {
            obj = freeObjects_.Back();
            if (addref)
                obj->AddRef();
            freeObjects_.Pop();
        }
        return obj;
    }
    void AddRef(T* obj) { obj->AddRef(); }
    void RemoveRef(T* obj)
    {
        if (obj->GetRefCount())
        {
            obj->RemoveRef();
            if (!obj->GetRefCount())
            {
                freeObjects_.Push(obj);
                obj->OnRestore();
            }
        }
    }

    unsigned Size() const { return pool_.Size(); }
    unsigned FreeSize() const { return freeObjects_.Size(); }

private:
    Vector<T > pool_;
    PODVector<T* > freeObjects_;
};


