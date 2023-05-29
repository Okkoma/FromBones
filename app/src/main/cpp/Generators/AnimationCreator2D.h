#pragma once

#include <Urho3D/Container/Ptr.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Resource/Resource.h>
#include <Urho3D/Resource/XMLFile.h>


namespace Urho3D
{
class AnimationSet2D;
}

using namespace Urho3D;

const unsigned MAX_PARTTYPE = 6;
enum PartType
{
    noPartType = 0,
    legType,
    armType,
    bodyType,
    headType,
    wingType
};

enum AnimationType
{
    noAnimationType = 0,
    idleType,
    walkType,
    flyType,
    fallType,
    deadType,
    explodeType,
};



enum PatternFinderMode
{
    returnFirstPattern = 0,
    returnMinNumPartsPattern,
    returnMaxNumPartsPattern,
    returnRandomPattern,
    returnBestPattern
};

/// Decrit les proprietes de placement entre les type de parts
/// ecartement, alignement, proportionalité etc...
struct MorphPattern
{

};

/// Decrit la cinematic d'animation d'un State
struct AnimationStatePattern
{
    AnimationStatePattern() { ; }

    ///
    void SetType(AnimationType type);
    void SetLength(float length);
    void AddPart(PartType type, const String& partName);
    void AddLink(const String& part1, const String& part2);

    void LoadFrom(XMLFile* scmlFile);

    void ApplyMorphology(const MorphPattern& morpho);
    void CreateSkeleton();
    void CreateCinematic();


};


/// AnimationSetPattern ///

/// doit spécifier les differentes animations State à créer pour générer un animationSet complet (Idle, Attack, Dead ... )
/// voire pour le relier à un GOC_Animator2D_Template.
/// un AnimationSetPattern décrit un ensemble d'animation State

struct AnimationSetPattern// : public Resource
{
    AnimationSetPattern() : id_(0) { ; }

    bool HasPartType(PartType type) const;
    unsigned GetNumParts() const;

    bool operator == (const AnimationSetPattern& ap) const
    {
        return id_ == ap.id_;
    }
    bool operator != (const AnimationSetPattern& ap) const
    {
        return id_ != ap.id_;
    }

    unsigned id_;

    unsigned numPartByTypes_[MAX_PARTTYPE];

    Vector<AnimationStatePattern*> animationStates_;

    static const AnimationSetPattern EMPTY;
};

class AnimationCreator2D
{
public :
    AnimationCreator2D() : patternFinderMode_(returnFirstPattern) { ; }
    ~AnimationCreator2D();
    static void Register(const AnimationSetPattern& animationPattern);

    AnimationSet2D* Make(const PODVector<Node*>& parts, const AnimationSetPattern& animationPattern = AnimationSetPattern::EMPTY);

private :
    PartType GetPartType(const String& nodeName);
    bool CheckParts(const PODVector<Node*>& parts, const AnimationSetPattern& animationPattern);
    AnimationSet2D* Assembly(const PODVector<Node*>& parts, const AnimationSetPattern& animationPattern);

    int patternFinderMode_;

    /// bank of Pattern
    static Vector<AnimationStatePattern> statePatterns_;
    static Vector<AnimationSetPattern> setPatterns_;
};

