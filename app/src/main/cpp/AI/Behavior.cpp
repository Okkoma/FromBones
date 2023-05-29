#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "Behavior.h"

#define MAX_BEHAVIOR 50




Vector<unsigned> Behaviors::behaviorIndexes;
Vector<Behavior*> Behaviors::behaviors;



void Behaviors::Register(Behavior* behavior)
{
    if (behaviors.Size() < MAX_BEHAVIOR)
    {
        behaviors.Push(behavior);
        behaviorIndexes.Push(behavior->GetType().Value());
    }
}

void Behaviors::Unregister(Behavior* behavior)
{
    Vector<unsigned>::ConstIterator it = behaviorIndexes.Find(behavior->GetType().Value());
    if (it != behaviorIndexes.End())
    {
        unsigned index = behaviorIndexes[it-behaviorIndexes.Begin()];
        delete(behaviors[index]);
        behaviors.Erase(index);
        behaviorIndexes.Erase(index);
    }
}

void Behaviors::Clear()
{
    if (!GetNumBehaviors()) return;

    for (Vector<Behavior*>::Iterator it=behaviors.Begin(); it!=behaviors.End(); ++it)
    {
        delete(*it);
        *it = 0;
    }
    behaviors.Clear();
    behaviorIndexes.Clear();

    URHO3D_LOGINFOF("Behaviors() - Clear : Registered Behaviors  = %u", GetNumBehaviors());
}

Behavior* Behaviors::Get(unsigned hashValue)
{
    Vector<unsigned>::ConstIterator it = behaviorIndexes.Find(hashValue);
    if (it != behaviorIndexes.End())
        return behaviors[it-behaviorIndexes.Begin()];
    else
        return 0;
}

void Behaviors::Dump()
{
    if (!GetNumBehaviors()) return;
    unsigned index = 1;
    URHO3D_LOGINFOF("Behaviors() - Dump : Registered Behaviors  = %u", GetNumBehaviors());
    for (Vector<Behavior*>::Iterator it=behaviors.Begin(); it!=behaviors.End(); ++it)
    {
        URHO3D_LOGINFOF(" => id=%u hashValue=%u name=%s", index, (*it)->GetType().Value(), (*it)->GetTypeName().CString());
        index++;
    }
}
