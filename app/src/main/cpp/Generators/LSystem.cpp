#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>
#include <Urho3D/IO/Log.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Urho2D/Renderer2D.h>

#include "GameContext.h"
#include "GameRand.h"

#include "LSystem.h"


const char* LS_PartTypeNames[] =
{
    "Branch",
    "Leaf",
    0
};

const int maxIteration_ = 5;
const float branchOverSize_ = 1.1f;
const float animationDeformFactor_ = PIXEL_SIZE;


bool LS_Rule::IsDefined() const
{
    return !symbol_.Empty() && !output_.Empty();
}

void LS_Rule::Parse(const String& s)
{
    Vector<String> t = s.Split(';');
    for (unsigned i=0; i < t.Size(); i++)
    {
        Vector<String> u = t[i].Trimmed().Split(':');
        if (u.Size() == 2)
        {
            String v = u[0];
            if (v == "s") // initial symbols
                symbol_ = u[1];
            else if (v == "n") // new symbols
                output_ = u[1];
            else if (v == "c") // parameter
                chance_ = ToInt(u[1]);
        }
    }
}

void LS_Geometry::Parse(const String& s)
{
    Vector<String> t = s.Split(';');

    branchWidth_ = ToFloat(t[0]) * PIXEL_SIZE;
    branchLength_ = ToFloat(t[1]) * PIXEL_SIZE;
    leafLength_ = ToFloat(t[2]) * PIXEL_SIZE;
    branchAngle_ = ToFloat(t[3]);
    lengthVariation_ = ToInt(t[4]);
    angleVariation_ = ToInt(t[5]);
    branchFallOut_ = ToFloat(t[6]);
}

void LS_Sequence::Dump() const
{
    String s;

    for (unsigned i=0; i < sequence_.Length(); i++)
    {
        s += sequence_[i];
        s += "{";
        if (i >= parameters_.Size())
            s += "error";
        else
            s += parameters_[i].value_;
        s += "} ";
    }

    URHO3D_LOGERRORF("LS_Sequence - Dump() : length=%u parameters.Size=%u %s", sequence_.Length(), parameters_.Size(), s.CString());
}

void LS_Sequence::Initialize(const LS_Model& model, float maturity)
{
    iteration_ = 0;
    maturity_ = maturity;
    sequence_.Clear();
    parameters_.Clear();

    SetSequence(true, model.axiom_, 0, model.geometry_);
}

void LS_Sequence::SetSequence(bool replace, const String& seq, unsigned index, const LS_Geometry& geometry)
{
    float variation;

    // replace the parameter if the symbol is different
    if (replace)
    {
        if (!parameters_.Size())
            parameters_.Resize(1);

        char c = seq.Front();

        if (c == 'F')
        {
            variation = geometry.lengthVariation_ > 0 ? (float)GameRand::GetRand(ALLRAND, 100 - geometry.lengthVariation_, 100 + geometry.lengthVariation_) / 100.f : 1.f;
            parameters_[index].length_ = geometry.branchLength_ * variation * maturity_;
        }
        else if (c == 'X' || c == ']')
        {
            variation = geometry.lengthVariation_ > 0 ? (float)GameRand::GetRand(ALLRAND, 100 - geometry.lengthVariation_, 100 + geometry.lengthVariation_) / 100.f : 1.f;
            parameters_[index].length_ = geometry.leafLength_ * variation * maturity_;
        }
        else if (c == '+' || c == '-')
        {
            variation = geometry.angleVariation_ > 0 ? (float)GameRand::GetRand(ALLRAND, 100 - geometry.angleVariation_, 100 + geometry.angleVariation_) / 100.f : 1.f;
            parameters_[index].angle_ = (c == '-' ? -1 : 1) * geometry.branchAngle_ * variation;
        }
        else
            parameters_[index].value_ = 0.f;

//        URHO3D_LOGERRORF("LS_Sequence - InsertParameters() : ... symbol=%c replace parameters[%d]=%F", c, index, parameters_[index].value_);
    }

    // insert new parameters after
    if (seq.Length() > 1)
    {
        LS_Parameters newparameters(seq.Length()-1);

        for (unsigned i=0; i < newparameters.Size(); i++)
        {
            char c = seq[i+1];

            if (c == 'F')
            {
                variation = geometry.lengthVariation_ > 0 ? (float)GameRand::GetRand(ALLRAND, 100 - geometry.lengthVariation_, 100 + geometry.lengthVariation_) / 100.f : 1.f;
                newparameters[i].length_ = geometry.branchLength_ * variation * maturity_;
            }
            else if (c == 'X' || c == ']')
            {
                variation = geometry.lengthVariation_ > 0 ? (float)GameRand::GetRand(ALLRAND, 100 - geometry.lengthVariation_, 100 + geometry.lengthVariation_) / 100.f : 1.f;
                newparameters[i].length_ = geometry.leafLength_ * variation * maturity_;
            }
            else if (c == '+' || c == '-')
            {
                variation = geometry.angleVariation_ > 0 ? (float)GameRand::GetRand(ALLRAND, 100 - geometry.angleVariation_, 100 + geometry.angleVariation_) / 100.f : 1.f;
                newparameters[i].angle_ = (c == '-' ? -1 : 1) * geometry.branchAngle_ * variation;
            }
            else
                newparameters[i].value_ = 0.f;

//            URHO3D_LOGERRORF("LS_Sequence - InsertParameters() : ... symbol=%c insert parameters[%d]=%F", c, index+1+i, newparameters[i].value_);
        }

        parameters_.Insert(index+1, newparameters);
    }

    // add the sequence
    sequence_ += seq;

//    Dump();
}

inline int LS_Model::GetSymbolIndex(const String& symbol) const
{
    for (unsigned i=0; i < symbols_.Size(); i++)
    {
        if (symbol == symbols_[i])
            return i;
    }
    return -1;
}

void LS_Model::Parse(const String& s)
{
    Vector<String> t = s.Split(' ');
    for (unsigned i=0; i < t.Size(); i++)
    {
        String u = t[i].Trimmed();
        if (u.Length() > 3 && u[1] == '(' && u.Back() == ')')
        {
            char type = u.Front();

            if (type == 'a' || type == 'A')
            {
                // set the axiom
                axiom_ = u.Substring(2, u.Length()-3);
            }
            else if (type == 'r' || type == 'R')
            {
                // add a rule
                LS_Rule rule;
                rule.Parse(u.Substring(2, u.Length()-3));
                if (rule.IsDefined())
                {
                    int index = GetSymbolIndex(rule.symbol_);
                    if (index < 0)
                    {
                        index = rules_.Size();
                        rules_.Resize(rules_.Size()+1);
                        symbols_.Push(rule.symbol_);
                    }
                    rules_[index].Push(rule);
                }
            }
            else if (type == 'g' || type == 'G')
            {
                // add geometry
                geometry_.Parse(u.Substring(2, u.Length()-3));
            }
            else if (type == 'i' || type == 'I')
            {
                // set min/max iterations
                Vector<String> v = u.Substring(2, u.Length()-3).Split(';');
                minIteration_ = ToInt(v[0]);
                maxIteration_ = ToInt(v[1]);
            }
        }
    }
}

void LS_Model::Evolve(const LS_Geometry& geometry, LS_Sequence& sequence) const
{
    String seq(sequence.sequence_), s(' ');
    const String* ruleoutput;

    sequence.sequence_.Clear();

    for (unsigned j = 0; j < seq.Length(); j++)
    {
        bool replace = false;

        s[0] = seq[j];
        ruleoutput = &s;

        // Find rules for the symbol
        int isymbol = GetSymbolIndex(s);
        if (isymbol >= 0)
        {
            const Vector<LS_Rule>& rules = rules_[isymbol];
            unsigned irule = 0;

            if (rules.Size() > 1)
            {
                int r = GameRand::GetRand(ALLRAND, 100);
                int chance = 0;
                for (irule = 0; irule < rules.Size(); irule++)
                {
                    chance += rules[irule].chance_;
                    if (r < chance)
                        break;
                }
            }

            ruleoutput = &rules[irule].output_;
            replace = ruleoutput->Front() != s[0];

//            URHO3D_LOGERRORF("LS_Model - Evolve() : ... iteration=%u s[%u]=%c ruleoutput=%s ...", sequence.iteration_, j, s[0], ruleoutput->CString());
        }

        sequence.SetSequence(replace, *ruleoutput, sequence.sequence_.Length(), geometry);
    }

//    sequence.Dump();

    sequence.iteration_++;

//    URHO3D_LOGERRORF("LS_Model - Evolve() : ... iteration=%u sequence = %s", sequence.iteration_, sequence.sequence_.CString());
}

void LS_Model::Update(const LS_Geometry& geometry, LS_Sequence& sequence, unsigned toiteration) const
{
    for (unsigned i = sequence.iteration_+1; i <= toiteration; i++)
        Evolve(geometry, sequence);
}

void LS_Model::Dump() const
{
    String s = "axiom="+axiom_;

    for (unsigned i=0; i < rules_.Size(); i++)
    {
        for (unsigned j=0; j < rules_[i].Size(); j++)
        {
            const LS_Rule& rule = rules_[i][j];
            s.AppendWithFormat(" rule[%u] : %s => %s chance=%d ", j, rule.symbol_.CString(), rule.output_.CString(), rule.chance_);
        }
    }
    URHO3D_LOGERROR(s);
}

const LS_Model LSystem2D::lsPresets_[MaxLSModelPresets] =
{
    // a: axiom
    // i: iteration
    // g: geometry branchwidth branchlength leaflength branchangle lengthvariation anglevariation branchfallout
    // r: rule F=branch X=apex c=chance +/-=rotate [=open ]=close
    LS_Model("a(FX) i(0;3) g(90;162;200;20;5;10;0.93) r(s:F;n:F[+F]F[-F][F];c:33) r(s:F;n:F[+F][F];c:33) r(s:F;n:F[-F][F];c:34)"),
    LS_Model("a(X) i(0;2) g(90;162;200;20;5;10;0.93) r(s:F;n:FF;c:100) r(s:X;n:F+[-F-XF-X][+FF][--XF[+X]][++F-X];c:100)"),
    LS_Model("a(FX) i(0;2) g(70;90;150;20;5;20;0.95) r(s:F;n:FF+[+F-F-F]-[-F+F+F];c:100)"),
    LS_Model("a(X) i(0;2) g(90;162;200;20;5;10;0.93) r(s:F;n:FX[FX[+XF]];c:100) r(s:X;n:FF[+XZ++X-F[+ZX]][-X++F-X];c:100) r(s:Z;n:[+F-X-F][++ZX];c:100)"),
    LS_Model("a(FX) i(0;4) g(70;70;120;30;5;20;0.95) r(s:F;n:F[+F][-F];c:50) r(s:F;n:F[+F]F[-F];c:50)"),
    LS_Model("a(X) i(0;2) g(90;162;200;20;5;10;0.93) r(s:X;n:F[+X]F[-X]+X;c:33) r(s:X;n:F[-X]F[-X]+X;c:33) r(s:X;n:F[-X]F+X;c:34) r(s:F;n:FF;c:100)"),
    LS_Model("a(X) i(0;2) g(90;162;200;20;5;10;0.93) r(s:X;n:F[-[[X]+X]]+F[+FX]-X;c:100) r(s:F;n:FF;c:100)"),

    LS_Model("a(FX) i(0;7) g(73;162;150;20;20;20;0.8) r(s:X;n:F[+[X]--[X]];c:45) r(s:X;n:F[[X]+[X]];c:45) r(s:X;n:F[-[X]+[X]];c:10)"),
    LS_Model("a(FI) i(0;7) g(73;162;150;20;20;20;0.8) r(s:I;n:F[+[X]-[X]]) r(s:X;n:JX;c:50) r(s:X;n:K;c:50) r(s:J;n:F[+X+X]) r(s:K;n:F[-X])"),
};

const char* LS_ModelPresetNames[] =
{
    "LSModel0",
    "LSModel1",
    "LSModel2",
    "LSModel3",
    "LSModel4",
    "LSModel5",
    "LSModel6",
    "LSModel7",
    "LSModel8",
    0,
    "Random"
};



LSystem2D::LSystem2D(Context* context) :
    StaticSprite2D(context),
    modeltype_(LSRandomModel),
    iteration_(0),
    animate_(false),
    grow_(false),
    evolve_(false),
    evolveTime_(0.f),
    animationTime_(0.f),
    angleAnimation_(0.f)
{
    partSprites_.Resize(NumPartTypes);

    SetDrawRect(Rect::FULL);
    SetUseDrawRect(true);

    sequence_.parameters_.Reserve(1000);
    sourceBatchesDirty_ = false;
}

LSystem2D::~LSystem2D()
{

}

void LSystem2D::RegisterObject(Context* context)
{
    context->RegisterFactory<LSystem2D>();

    URHO3D_COPY_BASE_ATTRIBUTES(StaticSprite2D);
    URHO3D_REMOVE_ATTRIBUTE("Sprite");

    URHO3D_ENUM_ACCESSOR_ATTRIBUTE("Model", GetModelAttr, SetModelAttr, LS_ModelPreset, LS_ModelPresetNames, LSRandomModel, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Iteration", GetCurrentIteration, SetCurrentIteration, int, 0, AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Branch", GetBranchAttr, SetBranchAttr, ResourceRef, ResourceRef(Sprite2D::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Leaf", GetLeafAttr, SetLeafAttr, ResourceRef, ResourceRef(Sprite2D::GetTypeStatic(), String::EMPTY), AM_DEFAULT);
}

void LSystem2D::SetBranchAttr(const ResourceRef& value)
{
    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(context_, value);
    if (sprite)
        SetPart(Branch, 0, LS_PartSpriteInfo(sprite, Vector2(0.5f, 0.f), Color::WHITE));
}

void LSystem2D::SetLeafAttr(const ResourceRef& value)
{
    Sprite2D* sprite = Sprite2D::LoadFromResourceRef(context_, value);
    if (sprite)
        SetPart(Leaf, 0, LS_PartSpriteInfo(sprite, Vector2(0.5f, 0.5f), Color::WHITE));
}

ResourceRef LSystem2D::GetBranchAttr() const
{
    return Sprite2D::SaveToResourceRef(partSprites_[Branch][0].sprite_);
}

ResourceRef LSystem2D::GetLeafAttr() const
{
    return Sprite2D::SaveToResourceRef(partSprites_[Leaf][0].sprite_);
}

void LSystem2D::SetModelAttr(LS_ModelPreset preset)
{
    modeltype_ = preset;
}

LS_ModelPreset LSystem2D::GetModelAttr() const
{
    return modeltype_;
}

void LSystem2D::SetCurrentIteration(int iteration)
{
    iteration_ = iteration;
}

int LSystem2D::GetCurrentIteration() const
{
    return iteration_;
}

void LSystem2D::SetPart(int part, int age, LS_PartSpriteInfo spriteInfo)
{
    if (part >= partSprites_.Size())
        partSprites_.Resize(part+1);

    Vector<LS_PartSpriteInfo>& spriteInfos = partSprites_[part];

    LS_PartSpriteInfo& spriteInfo0 = !spriteInfos.Size() ? spriteInfo : spriteInfos.Back();
    while (age >= spriteInfos.Size())
    {
        spriteInfos.Push(spriteInfo0);
    }

    spriteInfos[age] = spriteInfo;

    sprite_.Reset();
}

void LSystem2D::ApplyAttributes()
{
    bool isATemplateNode = GOT::GetObject(node_->GetVar(GOA::GOT).GetStringHash()) == node_;

//    URHO3D_LOGERRORF("LSystem2D - ApplyAttributes() ... got=%u model=%s template=%s", node_->GetVar(GOA::GOT).GetStringHash().Value(), LS_ModelPresetNames[modeltype_], isATemplateNode?"true":"false");

    if (isATemplateNode)
        return;

    bool modelchanged = false;

    // Randomized model
    if (modeltype_ == LSRandomModel)
    {
        modeltype_ = (LS_ModelPreset)GameRand::GetRand(ALLRAND, (int)MaxLSModelPresets);
        modelchanged = true;
    }

    const LS_Model& model = lsPresets_[modeltype_];

    // Set the iteration
    if (iteration_ == -1)
        iteration_ = GameRand::GetRand(ALLRAND, model.minIteration_, model.maxIteration_);
    else
        iteration_ = Clamp(iteration_, model.minIteration_, model.maxIteration_);

    // Initialize the sequence
    if (modelchanged || !iteration_)
    {
        // set the maturity in function of the targeted iteration
        float maturity = (float)iteration_ / model.maxIteration_;
        sequence_.Initialize(model, maturity);
        sourceBatchesDirty_ = true;
    }

    // Update the sequence to the targeted iteration
    if (sequence_.iteration_ != iteration_ || !iteration_)
    {
        model.Update(model.geometry_, sequence_, iteration_);
        sourceBatchesDirty_ = true;
//        Dump();
    }

    // Update Textures
    if (sprite_ != partSprites_[0][0].sprite_)
    {
        SetSprite(partSprites_[0][0].sprite_);

        unsigned textureUnit = sourceBatches_[0][0].material_->GetTextureUnit(sprite_->GetTexture());
    #ifdef URHO3D_VULKAN
        texmode_ = 0;
    #else
        texmode_ = Vector4::ZERO;
    #endif
        SetTextureMode(TXM_FX, textureFX_, texmode_);
        SetTextureMode(TXM_UNIT, textureUnit, texmode_);
    }

//    URHO3D_LOGERRORF("LSystem2D - ApplyAttributes() ... model=%s iteration=%d material=%s texture=%s", LS_ModelPresetNames[modeltype_], iteration_, sourceBatches_[0][0].material_->GetName().CString(), sprite_->GetTexture()->GetName().CString());
}

void LSystem2D::OnWorldBoundingBoxUpdate()
{
    if (!worldBoundingBox_.Defined())
        worldBoundingBox_.Define(-M_LARGE_VALUE, M_LARGE_VALUE);
}

void LSystem2D::OnSetEnabled()
{
    Drawable2D::OnSetEnabled();

    Scene* scene = GetScene();
    if (!scene)
        return;

    if (IsEnabledEffective())
    {
        worldBoundingBox_.Clear();
        animate_ = true;
        grow_ = true;
        evolve_ = true;
        animationTime_ = 0.f;
        evolveTime_ = 0.f;
        growBranchWidthRatio_ = sequence_.maturity_;

        SubscribeToEvent(scene, E_SCENEPOSTUPDATE, URHO3D_HANDLER(LSystem2D, HandleScenePostUpdate));
    }
    else
    {
        UnsubscribeFromEvent(scene, E_SCENEPOSTUPDATE);
        animate_ = false;
    }
}

void LSystem2D::HandleScenePostUpdate(StringHash eventType, VariantMap& eventData)
{
    Update(eventData[ScenePostUpdate::P_TIMESTEP].GetFloat());
}

const float evolveThreshold_ = 5.f;
const float growBranchWidthSpeed_ = 2.5f * PIXEL_SIZE;
const float growBranchLengthSpeed_ = PIXEL_SIZE / 10.f;
const float growLeafSpeed_ = PIXEL_SIZE / 10.f;

void LSystem2D::Update(float timestep)
{
    // Handle the dimensions growing
    if (grow_)
    {
        growBranchWidthRatio_ += growBranchWidthSpeed_ * timestep;
        if (growBranchWidthRatio_ > 1.f)
            grow_ = false;
    }

    // Handle the evolution (model iteration)
    if (evolve_)
    {
        const LS_Model& model = lsPresets_[modeltype_];

        evolveTime_ += timestep;
        if (evolveTime_ > evolveThreshold_)
        {
            evolveTime_ = 0.f;
            iteration_++;

            if (iteration_ > model.maxIteration_)
            {
                iteration_ = model.maxIteration_;
                evolve_ = false;
            }
            else
            {
                model.Update(model.geometry_, sequence_, iteration_);
                sourceBatchesDirty_ = true;
            }
        }
    }

    // Handle Wind Animation
    if (animate_)
    {
        animationTime_ += timestep;
        angleAnimation_ += animationDeformFactor_ * Sin(180.f * animationTime_);
        sourceBatchesDirty_ = true;
    }
}

void LSystem2D::AddVertices(int part, unsigned age, float width1, float width2, float height, const Matrix2x3& transform)
{
    if (part >= partSprites_.Size())
        return;

    if (!partSprites_[part].Size())
        return;

    const Vector<LS_PartSpriteInfo>& spriteInfos = partSprites_[part];
    const LS_PartSpriteInfo& spriteinfo = age >= spriteInfos.Size() ? spriteInfos.Back() : spriteInfos[age];
    if (!spriteinfo.sprite_)
        return;

    Rect textureRect;
    if (!spriteinfo.sprite_->GetTextureRectangle(textureRect, false, false))
        return;

    Vector<Vertex2D>& vertices = sourceBatches_[0][0].vertices_;

    Rect drawRect;
    spriteinfo.sprite_->GetDrawRectangle(drawRect, spriteinfo.pivot_);
    const float scalex1 = width1 / drawRect.Size().x_ ;
    const float scalex2 = width2 / drawRect.Size().x_ ;
    const float scaley = height / drawRect.Size().y_;

    const float z = node_->GetWorldPosition().z_;

    Vertex2D vertex;
    vertex.color_ = spriteinfo.color_;
    vertex.texmode_ = texmode_;

    vertex.position_ = transform * Vector2(drawRect.min_.x_ * scalex1, drawRect.min_.y_ * scaley);
#ifdef URHO3D_VULKAN
    vertex.z_ = z;
#else
    vertex.position_.z_ = z;
#endif
    vertex.uv_ = textureRect.min_;
    vertices.Push(vertex);
    // update boundingbox with min vertex position
    worldBoundingBox_.Merge(vertex.position_);

    vertex.position_ = transform * Vector2(drawRect.min_.x_ * scalex2, drawRect.max_.y_ * scaley);
#ifdef URHO3D_VULKAN
    vertex.z_ = z;
#else
    vertex.position_.z_ = z;
#endif
    vertex.uv_ = Vector2(textureRect.min_.x_, textureRect.max_.y_);
    vertices.Push(vertex);

    vertex.position_ = transform * Vector2(drawRect.max_.x_ * scalex2, drawRect.max_.y_ * scaley);
#ifdef URHO3D_VULKAN
    vertex.z_ = z;
#else
    vertex.position_.z_ = z;
#endif
    vertex.uv_ = textureRect.max_;
    vertices.Push(vertex);
    // update boundingbox with max vertex position
    worldBoundingBox_.Merge(vertex.position_);

    vertex.position_ = transform * Vector2(drawRect.max_.x_ * scalex1, drawRect.min_.y_ * scaley);
#ifdef URHO3D_VULKAN
    vertex.z_ = z;
#else
    vertex.position_.z_ = z;
#endif
    vertex.uv_ = Vector2(textureRect.max_.x_, textureRect.min_.y_);
    vertices.Push(vertex);

//    URHO3D_LOGINFOF("LSystem2D - AddVertices() : part=%d position=%s angle=%F !", part, transform.Translation().ToString().CString(), transform.Rotation());
}

void LSystem2D::UpdateSourceBatches()
{
    if (!sourceBatchesDirty_)
        return;

    sourceBatches_[0][0].vertices_.Clear();

    Matrix2x3 transform = node_->GetWorldTransform2D();
    Vector<Matrix2x3> savedtransforms;
    Vector<float> savedBranchWidths;
    unsigned age = 0;
    unsigned color = color_.ToUInt();

    float angle = 0.f;

    // Update the worldBoundingBox
    worldBoundingBox_.Define(node_->GetWorldPosition());

    const LS_Model& model = lsPresets_[modeltype_];

    float branchWidth = Min(model.geometry_.branchWidth_ * growBranchWidthRatio_, model.geometry_.branchWidth_);

//    URHO3D_LOGERRORF("LSystem2D - UpdateSourceBatches() : model=%u iteration=%u position=%s angle=%F growBranchWidthRatio=%F branchWidth=%F(Max=%F) ...",
//                      modeltype_, iteration_, transform.Translation().ToString().CString(), transform.Rotation(), growBranchWidthRatio_, branchWidth, model.geometry_.branchWidth_);

    for (unsigned i=0; i < sequence_.sequence_.Length(); i++)
    {
        char c = sequence_.sequence_[i];
        float& param = sequence_.parameters_[i].value_;

        if (c == 'F')
        {
            float angleanim = ((2.f*i+1.f) / sequence_.sequence_.Length()) * angleAnimation_;

            transform.SetRotation(angle + angleanim);

            param = Min(model.geometry_.branchLength_, param + growBranchLengthSpeed_);
            float w2 = Max(model.geometry_.branchWidth_ * 0.25f, branchWidth * model.geometry_.branchFallOut_);
            AddVertices(Branch, age, branchWidth, w2, param * branchOverSize_, transform);

            branchWidth = w2;

            transform.SetTranslation(transform * Vector2(0.f, param));
        }
        else if (c == 'X')
        {
            param = Min(model.geometry_.leafLength_, param + growLeafSpeed_);
            AddVertices(Leaf, age, param, param, param, transform);
        }
        else if (c == '+' || c == '-')
        {
            // rotate of brancheAngle_
            angle += param;
            transform.SetRotation(angle);
        }
        else if (c == '[')
        {
            // Save Position
            savedtransforms.Push(transform);
            savedBranchWidths.Push(branchWidth);
        }
        else if (c == ']')
        {
            param = Min(model.geometry_.leafLength_, param + growLeafSpeed_);
            AddVertices(Leaf, age, param, param, param, transform);

            // Restore Position
            transform = savedtransforms.Back();
            angle = transform.Rotation();
            branchWidth = savedBranchWidths.Back();
            savedtransforms.Pop();
            savedBranchWidths.Pop();
        }
    }

    if (layer_.y_ != -1)
    {
        color = color2_.ToUInt();

        Vector<Vertex2D>& vertices = sourceBatches_[1][0].vertices_;
        vertices = sourceBatches_[0][0].vertices_;

        for (unsigned i = 0; i < vertices.Size(); i++)
            vertices[i].color_ = color;
    }

    sourceBatchesDirty_ = false;
}

void LSystem2D::Dump() const
{
    URHO3D_LOGERRORF("LSystem2D - Dump() : model=%u iteration=%u sequence=%s", modeltype_, iteration_, sequence_.sequence_.CString());
}
