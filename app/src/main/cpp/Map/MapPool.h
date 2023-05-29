#pragma once

#include "Map.h"

namespace Urho3D
{
class Context;
}

using namespace Urho3D;


template <class T> class DataPool
{
public:
    DataPool(unsigned numObjects=0, unsigned sizeObject=0) :
        allocator_(0)
    {
        if (numObjects && sizeObject)
            allocator_ = AllocatorInitialize((unsigned)sizeof(T)*sizeObject, numObjects);
    }

    /// Destruct.
    ~DataPool()
    {
        AllocatorUninitialize(allocator_);
    }

private:
    /// Node allocator.
    AllocatorBlock* allocator_;
};


class MapPool : public Object
{
    URHO3D_OBJECT(MapPool, Object);

public :

    static void RegisterObject(Context* context);

    MapPool(Context* context);
    virtual ~MapPool();

    void Clear();

    void Resize(unsigned size);
    bool Free(Map* map, HiresTimer* timer=0);

    Map* Get();
    unsigned GetSize() const
    {
        return maps_.Size();
    }
    unsigned GetFreeSize() const
    {
        return freemaps_.Size();
    }

    void Dump() const;

private :

    Vector<SharedPtr<Map> > maps_;
    PODVector<Map*> freemaps_;
};
