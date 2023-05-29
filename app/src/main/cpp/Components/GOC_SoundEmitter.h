#pragma once


namespace Urho3D
{
class Sound;
class SoundSource;
class SoundSource3D;
}

using namespace Urho3D;
/*
#define MAX_EVENTSOUNDS 5
#define MAX_STATESOUNDS 5
*/
struct GOC_SoundEmitter_Template
{
    GOC_SoundEmitter_Template() { }
    GOC_SoundEmitter_Template(String n, const GOC_SoundEmitter_Template& c) :
        name(n),
        hashName(StringHash(n)),
        baseTemplate(c.hashName),
        eventSoundTable(c.eventSoundTable),
        stateSoundTable(c.stateSoundTable) { }

    static void RegisterTemplate(const String& s, const GOC_SoundEmitter_Template& t);
    static void RegisterSound(int soundid, const SharedPtr<Sound>& sound);
    static void DumpAll();

    void AddSoundForState(int soundid, const String& stateName);
    void AddSoundForEvent(int soundid, const String& eventName);
    void AddSoundForSpriterEvent(int soundid, const String& eventName);

    const String& GetSoundFileName(StringHash hashevent) const;
    Sound* GetSound(int id) const
    {
        return registeredSounds_[id];
    }

    void Dump() const;

    String name;
    StringHash hashName;
    StringHash baseTemplate;

    HashMap<unsigned, int > eventSoundTable;
    HashMap<unsigned, int > spriterEventSoundTable;
    HashMap<unsigned, int > stateSoundTable;

    /*
        Vector<unsigned> eventTable_;
        Vector<unsigned> stateTable_;

        int eventSounds_[MAX_EVENTSOUNDS];
        int stateSounds_[MAX_STATESOUNDS];
    */
    static Vector<SharedPtr<Sound> > registeredSounds_;
    static HashMap<unsigned, GOC_SoundEmitter_Template> templates_;
};

class GOC_SoundEmitter : public Component
{
    URHO3D_OBJECT(GOC_SoundEmitter, Component);

public :
    GOC_SoundEmitter(Context* context) : Component(context), customTemplate(false), currentTemplate(0)
    {
        Init();
    }
    virtual ~GOC_SoundEmitter();

    static void RegisterObject(Context* context);
    static void ClearTemplates();

    void RegisterSound(const String& value);
    void RegisterTemplate(const String& s);
    void SetTemplate(const String& s);
    void SetSoundAttr(const String& value);

    const String& GetTemplateName() const
    {
        return currentTemplate->name;
    }
    const String& GetTemplateAttr() const
    {
        return String::EMPTY;
    }
    const String& GetSoundAttr() const
    {
        return String::EMPTY;
    }

    virtual void OnSetEnabled();

    void OnEvent(StringHash eventType, VariantMap& eventData);
    void OnChangeState(StringHash eventType, VariantMap& eventData);

protected :
    virtual void OnNodeSet(Node* node);
    void HandleSoundEnabled(StringHash eventType, VariantMap& eventData);

private :
    void Init();
    void ClearCustomTemplate();
    void ApplyTemplateProperties(StringHash key);

    void UpdateSubscriber();

    GOC_SoundEmitter_Template* currentTemplate;

    WeakPtr<SoundSource3D> source;

    bool customTemplate;
};


