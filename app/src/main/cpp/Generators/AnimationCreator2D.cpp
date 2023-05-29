#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>

#include "AnimationCreator2D.h"

const AnimationSetPattern AnimationSetPattern::EMPTY = AnimationSetPattern();

Vector<AnimationStatePattern> AnimationCreator2D::statePatterns_;
Vector<AnimationSetPattern> AnimationCreator2D::setPatterns_;

bool AnimationSetPattern::HasPartType(PartType type) const
{
    return false;
}

unsigned AnimationSetPattern::GetNumParts() const
{
    return 0;
}

AnimationCreator2D::~AnimationCreator2D()
{

}

void AnimationCreator2D::Register(const AnimationSetPattern& animationPattern)
{

}

AnimationSet2D* AnimationCreator2D::Make(const PODVector<Node*>& parts, const AnimationSetPattern& animationPattern)
{
    if (animationPattern != AnimationSetPattern::EMPTY)
    {
        if (CheckParts(parts, animationPattern))
        {
            return Assembly(parts, animationPattern);
        }
    }
    if (patternFinderMode_ == returnFirstPattern)
        for (Vector<AnimationSetPattern>::ConstIterator it=setPatterns_.Begin(); it!=setPatterns_.End(); ++it)
        {
            if (CheckParts(parts, *it))
                return Assembly(parts, animationPattern);
        }
    else
    {
        switch (patternFinderMode_)
        {
        case returnMinNumPartsPattern:

            break;

        case returnMaxNumPartsPattern:

            break;

        case returnRandomPattern:

            break;

        case returnBestPattern:

            break;
        }
    }
    return 0;
}

PartType AnimationCreator2D::GetPartType(const String& nodeName)
{
    return noPartType;
}

bool AnimationCreator2D::CheckParts(const PODVector<Node*>& parts, const AnimationSetPattern& animationPattern)
{
    unsigned quantitiesInPattern = animationPattern.GetNumParts();
    unsigned numParts = parts.Size();

    // First Check on number of parts
    if (numParts < quantitiesInPattern)
        return false;

    // Check for each type, that the quantity in parts  > quantity in animationPattern
    unsigned numPartByTypes[MAX_PARTTYPE];
    PartType partType;
    bool result = true;
    for (unsigned i=0; i<MAX_PARTTYPE; ++i)
    {
        for (unsigned j=0; j<numParts; ++j)
        {
            if (GetPartType(parts[j]->GetName()) == i)
                numPartByTypes[i]++;
        }
        result &= (numPartByTypes[i] < animationPattern.numPartByTypes_[i]);
//        if (numPartByTypes[i] < animationPattern.numPartByTypes_[i])
//            return false;
    }
    // Dump
    for (unsigned i=0; i<MAX_PARTTYPE; ++i)
    {
        URHO3D_LOGINFOF("Type %i : numPartByTypes=%u & animationPattern.numPartByTypes=%u : result=%s",
                        i, numPartByTypes[i], animationPattern.numPartByTypes_[i],
                        numPartByTypes[i] < animationPattern.numPartByTypes_[i] ? "FALSE" : "TRUE");
    }
    URHO3D_LOGINFOF("Result = %s", result ? "TRUE" : "FALSE");

    return result;
}

AnimationSet2D* AnimationCreator2D::Assembly(const PODVector<Node*>& parts, const AnimationSetPattern& animationPattern)
{

    return 0;
}
