#pragma once

#include <Urho3D/Scene/Node.h>


using namespace Urho3D;

// enum for waitCallBackOrderOfType
enum TypeForOrder
{
    None = 0,
    StateTypeForOrder = 1,
    EventTypeForOrder = 2
};


class GOC_AIController;


struct Behavior : public Object
{
    URHO3D_OBJECT(Behavior, Object);

public :
    Behavior(Context* context) : Object(context) { }
    virtual ~Behavior() { }

    virtual void Start(GOC_AIController& controller) { }
    virtual void Stop(GOC_AIController& controller) { }
    virtual void Update(GOC_AIController& controller) = 0;
};


// Static class for storing behaviors in heterogen collection
struct Behaviors
{
    static void Register(Behavior* behavior);
    static void Unregister(Behavior* behavior);
    static void Clear();

    static Behavior* Get(unsigned hashValue);
    static unsigned GetNumBehaviors()
    {
        return behaviors.Size();
    }

    static void Dump();

private :
    static Vector<unsigned> behaviorIndexes;
    static Vector<Behavior*> behaviors;
};

