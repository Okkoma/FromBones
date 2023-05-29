#pragma once

#include <Urho3D/Scene/Component.h>

using namespace Urho3D;

struct LifeProps
{
    LifeProps(float e, int l, float i);

    float energy;
    int life;
    float totalDpsReceived;

    HashMap<StringHash, float > defBonuses_;
    float invulnerability;
};

struct GOC_Life_Template
{
    GOC_Life_Template();
//	GOC_Life_Template(const GOC_Life_Template& t);
    GOC_Life_Template(const String& s, GOC_Life_Template* base=0, float e = 1.f, int l = 1);

    static void RegisterTemplate(const String& s, const GOC_Life_Template& t);
    static void DumpAll();
    void Dump() const;

    String name;
    StringHash hashName;
    StringHash baseTemplate;

    float energyMax;
    int lifeMax;

    HashMap<StringHash, float > immunities_;

    static const GOC_Life_Template DEFAULT;
    static HashMap<StringHash, GOC_Life_Template> templates_;

    // temporary immunities table for template registration process
    static HashMap<StringHash, float > sImmunities_;
};

class GOC_Life : public Component
{
    URHO3D_OBJECT(GOC_Life, Component);

public :
    GOC_Life(Context* context);

    virtual ~GOC_Life();

    static void RegisterObject(Context* context);

    void RegisterTemplate(const String& s);

    void SetTemplate(const String& s);
    void SetTotalEnergyLost(float e);
    void SetLife(int l);
    void SetEnergy(float e);
    void AddTemplateImmunity(const String& name);
    void SetInvulnerability(float i);
    void SetBonusDefense(const StringHash& type, float d);
    void Set(float e, float inv);
    void Reset();
    bool ReceiveEffectFrom(Node* sender, StringHash effectElt, float value, const Vector2& point, bool applyinvulnerability);

    void SetLifeBar(bool enable, bool follow=true, const IntVector2& position=IntVector2::ZERO, HorizontalAlignment hAlign=HA_CENTER, VerticalAlignment vAlign=VA_CENTER);

    bool CheckChangeTemplate(const String& newTemplateName);

    const String& GetTemplateName() const
    {
        return currentTemplate ? currentTemplate->name : String::EMPTY;
    }
    const String& GetEmptyAttr() const
    {
        return String::EMPTY;
    }
    GOC_Life_Template* GetCurrentTemplate()
    {
        return currentTemplate;
    }
    float GetTemplateMaxEnergy() const
    {
        return currentTemplate ? currentTemplate->lifeMax * currentTemplate->energyMax : 0.f;
    }

    const LifeProps& Get() const
    {
        return props;
    }
    int GetLife() const
    {
        return props.life;
    }
    float GetEnergy() const
    {
        return props.energy;
    }
    float GetTotalEnergyLost() const
    {
        return props.totalDpsReceived;
    }
    float GetInvulnerability() const
    {
        return props.invulnerability;
    }
    float GetBonusDefense(const StringHash& type) const;

    Node* GetLastAttacker() const
    {
        return killerPtr;
    }

    bool IsImmune(const StringHash& effectelt, float value);

    virtual void ApplyAttributes();
    virtual void OnSetEnabled();

    void Dump() const;

private :
    void SetEnergyLost(float e);
    void ValidateMarkNetworkUpdate();

    bool ApplyTemplateProperties(const StringHash& h);
    void ApplyAmountEnergy(float amount, bool hurtevent=true);
    void ApplyForceEffect(const Vector2& impactPoint, float strength);

    void UpdateProperties();
    void UpdateLifeBar();

    void HandleInvulnerabilityUpdate(StringHash eventType, VariantMap& eventData);
    void HandleLifeUpdate(StringHash eventType, VariantMap& eventData);
    void HandleUpdateLifeBar(StringHash eventType, VariantMap& eventData);
//    void HandleReceiveEffect(StringHash eventType, VariantMap& eventData);

//        bool customTemplate;

    GOC_Life_Template* currentTemplate;
    StringHash template_;
    LifeProps props;
    bool updateProps_, waitServerReset_;
    unsigned lastGainInvulnerabilityTime;
    unsigned killer;
    Node* haloInvulnerability_;
    Vector2 lastDpsContactPoint_;
    WeakPtr<Node> killerPtr;
    WeakPtr<UIElement> lifebarui_;
    WeakPtr<Node> lifebarnode_;
};


