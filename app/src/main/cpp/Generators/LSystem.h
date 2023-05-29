#pragma once

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>

using namespace Urho3D;

enum LS_PartType
{
    Branch = 0,
    Leaf,
    NumPartTypes
};

//struct LS_Parameter
//{
//    LS_Parameter() : value_(0.f) { }
//    LS_Parameter(const LS_Parameter& p) : value_(p.value_) { }

    union LS_Parameter
    {
        float value_;
        float angle_;
        float length_;
    };
//};

typedef PODVector<LS_Parameter> LS_Parameters;

struct LS_PartSpriteInfo
{
    LS_PartSpriteInfo() { }
    LS_PartSpriteInfo(Sprite2D* sprite, const Vector2& pivot, const Color& color) : sprite_(sprite), pivot_(pivot), color_(color.ToUInt()) { }
    LS_PartSpriteInfo(const LS_PartSpriteInfo& p) : sprite_(p.sprite_), pivot_(p.pivot_), color_(p.color_) { }

    Sprite2D* sprite_;
    Vector2 pivot_;
    unsigned color_;
};

struct LS_Rule
{
    LS_Rule() : chance_(0.f) { }
    LS_Rule(const String& s) { Parse(s); }
    LS_Rule(const LS_Rule& r) : symbol_(r.symbol_), output_(r.output_), chance_(r.chance_) { }

    bool IsDefined() const;
    void Parse(const String& s);

    String symbol_;
    String output_;
    int chance_;
};

struct LS_Geometry
{
    LS_Geometry() { }
    LS_Geometry(const String& s) { Parse(s); }

    void Parse(const String& s);

    float branchWidth_, branchLength_;
    float branchAngle_;
    float leafLength_;
    float branchFallOut_;
    int lengthVariation_, angleVariation_;
};

struct LS_Model;

struct LS_Sequence
{
    void Initialize(const LS_Model& model, float maturity);
    void SetSequence(bool replace, const String& seq, unsigned index, const LS_Geometry& geometry);
    void Dump() const;

    unsigned iteration_;
    float maturity_;
    String sequence_;
    LS_Parameters parameters_;
};

struct LS_Model
{
    LS_Model(const String& s) { Parse(s); }

    inline int GetSymbolIndex(const String& symbol) const;
    void Parse(const String& s);

    void Evolve(const LS_Geometry& geometry, LS_Sequence& sequence) const;
    void Update(const LS_Geometry& geometry, LS_Sequence& sequence, unsigned toiteration) const;
    void Dump() const;

    String axiom_;

    int minIteration_, maxIteration_;

    LS_Geometry geometry_;
    Vector<String > symbols_;
    Vector<Vector<LS_Rule> > rules_;
};


enum LS_ModelPreset
{
    LSModel0 = 0,
    LSModel1,
    LSModel2,
    LSModel3,
    LSModel4,
    LSModel5,
    LSModel6,
    LSModel7,
    LSModel8,
    MaxLSModelPresets,
    LSRandomModel
};

class LSystem2D : public StaticSprite2D
{
    URHO3D_OBJECT(LSystem2D, StaticSprite2D);

public:
    LSystem2D(Context* context);
    ~LSystem2D();

    static void RegisterObject(Context* context);

    void SetBranchAttr(const ResourceRef& value);
    ResourceRef GetBranchAttr() const;
    void SetLeafAttr(const ResourceRef& value);
    ResourceRef GetLeafAttr() const;
    void SetModelAttr(LS_ModelPreset model);
    LS_ModelPreset GetModelAttr() const;
    void SetPart(int part, int age, LS_PartSpriteInfo spriteInfo);

    void SetCurrentIteration(int iteration);
    int GetCurrentIteration() const;

    void Update(float timestep);

    virtual void ApplyAttributes();
    virtual void OnSetEnabled();

    void Dump() const;

private:
    virtual void OnWorldBoundingBoxUpdate();
    virtual void UpdateSourceBatches();

    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);

    void AddVertices(int type, unsigned age, float width, float width2, float length, const Matrix2x3& transform);

    bool animate_, grow_, evolve_;
    float evolveTime_, growBranchWidthRatio_;
    float animationTime_, angleAnimation_;
    int iteration_;
    LS_ModelPreset modeltype_;
    float branchWidth_;

#ifdef URHO3D_VULKAN
    unsigned texmode_;
#else
    Vector4 texmode_;
#endif

    Vector<Vector<LS_PartSpriteInfo > > partSprites_;

    LS_Sequence sequence_;

    static const LS_Model lsPresets_[];
};


