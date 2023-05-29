#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Urho2D/SpriterInstance2D.h>

#include <Urho3D/Audio/Audio.h>
#include <Urho3D/Audio/Sound.h>
#include <Urho3D/Audio/SoundSource.h>
#include <Urho3D/Audio/SoundSource3D.h>

#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "GOC_SoundEmitter.h"


Vector<SharedPtr<Sound> > GOC_SoundEmitter_Template::registeredSounds_;
HashMap<unsigned, GOC_SoundEmitter_Template> GOC_SoundEmitter_Template::templates_;

const StringHash SOUND_TEMPLATE_EMPTY    = StringHash("SoundTemplate_Empty");


/// GOC_SoundEmitter_Template

void GOC_SoundEmitter_Template::RegisterSound(int soundid, const SharedPtr<Sound>& sound)
{
    if (soundid >= registeredSounds_.Size())
        registeredSounds_.Resize(soundid+1);
    registeredSounds_[soundid] = sound;

//    URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - RegisterTemplate templates_ size=%u",templates_.Size());
}

void GOC_SoundEmitter_Template::RegisterTemplate(const String& s, const GOC_SoundEmitter_Template& t)
{
    unsigned key = StringHash(s).Value();

    if (templates_.Contains(key)) return;

    templates_ += Pair<unsigned, GOC_SoundEmitter_Template>(key, t);
    templates_[key].name = s;
    templates_[key].hashName = StringHash(key);

//    URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - RegisterTemplate templates_ size=%u",templates_.Size());
}

void GOC_SoundEmitter_Template::DumpAll()
{
    unsigned index = 0;
    for (HashMap<unsigned, GOC_SoundEmitter_Template>::ConstIterator it=templates_.Begin(); it!=templates_.End(); ++it)
    {
        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - DumpAll - templates_[%u]",index);
        it->second_.Dump();
        ++index;
    }
}

void GOC_SoundEmitter_Template::Dump() const
{
    URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - Dump - name=%s hash=%u base=%u - stateSoundTable size=%u - eventSoundTable size=%u",
                    name.CString(), hashName.Value(), baseTemplate.Value(), stateSoundTable.Size(), eventSoundTable.Size());

    unsigned index = 0;
    for (HashMap<unsigned, int>::ConstIterator it=stateSoundTable.Begin(); it!=stateSoundTable.End(); ++it)
    {
        URHO3D_LOGINFOF("  => stateSoundTable[%u] = (state:%u sound:%d)", index, it->first_, it->second_);
        ++index;
    }
    index = 0;
    for (HashMap<unsigned, int>::ConstIterator it=eventSoundTable.Begin(); it!=eventSoundTable.End(); ++it)
    {
        URHO3D_LOGINFOF("  => eventSoundTable[%u] = (event:%s sound:%d)", index, GOE::GetEventName(StringHash(it->first_)).CString(), it->second_);
        ++index;
    }
    index = 0;
    for (HashMap<unsigned, int>::ConstIterator it=spriterEventSoundTable.Begin(); it!=spriterEventSoundTable.End(); ++it)
    {
        URHO3D_LOGINFOF("  => spriterEventSoundTable[%u] = (event:%u sound:%d)", index, it->first_, it->second_);
        ++index;
    }
}

void GOC_SoundEmitter_Template::AddSoundForState(int soundid, const String& stateName)
{
    if (soundid == -1 || stateName.Empty())
        return;

//    URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - AddSoundForState (%s,%d)", stateName.CString(), soundid);

    unsigned state = StringHash(stateName).Value();

    if (stateSoundTable.Contains(state))
    {
        // Modify Sound for this Event
        stateSoundTable[state] = soundid;
//        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - register in Existing state %u, soundid = %d !", state, soundid);
    }
    else
    {
        // Add Sound & Event in the Table
        stateSoundTable += Pair<unsigned, int>(state, soundid);
//        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - register in state %s(%u) with soundid = %d  !", stateName.CString(), state, soundid);
    }
}

void GOC_SoundEmitter_Template::AddSoundForEvent(int soundid, const String& eventName)
{
    if (soundid == -1 || eventName.Empty())
        return;

//    URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - AddSoundForEvent (%s,%d)", eventName.CString(), soundid);

    unsigned event = StringHash(eventName).Value();

    if (eventSoundTable.Contains(event))
    {
        // Modify Sound for this Event
        eventSoundTable[event] = soundid;
//        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - register in Existing Event %s(%u) with soundid = %d !", eventName.CString(), event, soundid);
    }
    else
    {
        // Add Sound & Event in the Table
        eventSoundTable += Pair<unsigned, int>(event, soundid);
//        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - register in Event %s(%u) with soundid = %d !", eventName.CString(), event, soundid);
    }
}

void GOC_SoundEmitter_Template::AddSoundForSpriterEvent(int soundid, const String& eventName)
{
    if (soundid == -1 || eventName.Empty())
        return;

//    URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - AddSoundForSpriterEvent (%s,%d)", eventName.CString(), soundid);

    unsigned event = StringHash(eventName).Value();

    if (spriterEventSoundTable.Contains(event))
    {
        // Modify Sound for this Event
        spriterEventSoundTable[event] = soundid;
//        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - register in Existing SpriterEvent %s(%u) with soundid = %d !", eventName.CString(), event, soundid);
    }
    else
    {
        // Add Sound & Event in the Table
        spriterEventSoundTable += Pair<unsigned, int>(event, soundid);
//        URHO3D_LOGINFOF("GOC_SoundEmitter_Template() - register in SpriterEvent %s(%u) with soundid = %d !", eventName.CString(), event, soundid);
    }
}



/// GOC_SoundEmitter

GOC_SoundEmitter::~GOC_SoundEmitter()
{
//    URHO3D_LOGINFOF("~GOC_SoundEmitter()");
    UnsubscribeFromAllEvents();
    ClearCustomTemplate();
}

void GOC_SoundEmitter::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_SoundEmitter>();

    URHO3D_ACCESSOR_ATTRIBUTE("Is Enabled", IsEnabled, SetEnabled, bool, true, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Template", GetTemplateName, SetTemplate, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Sound", GetSoundAttr, SetSoundAttr, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Register Sound", GetSoundAttr, RegisterSound, String, String::EMPTY, AM_DEFAULT);
    URHO3D_ACCESSOR_ATTRIBUTE("Register Template", GetTemplateAttr, RegisterTemplate, String, String::EMPTY, AM_DEFAULT);

    GOC_SoundEmitter_Template::templates_.Clear();

    GOC_SoundEmitter_Template defaultProps = GOC_SoundEmitter_Template();
    defaultProps.baseTemplate = 0;

    GOC_SoundEmitter_Template::RegisterTemplate("SoundTemplate_Empty", defaultProps);
}

void GOC_SoundEmitter::ClearTemplates()
{
    GOC_SoundEmitter_Template::registeredSounds_.Clear();
    GOC_SoundEmitter_Template::templates_.Clear();
}

void GOC_SoundEmitter::RegisterTemplate(const String& value)
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - RegisterTemplate=%s(%u)", s.CString(), StringHash(s).Value());
    if (value.Empty())
        return;

    GOC_SoundEmitter_Template::RegisterTemplate(value, *currentTemplate);
}

void GOC_SoundEmitter::RegisterSound(const String& value)
{
    // value="id:0 | file:Sounds/016-Jump02.ogg"
    if (value.Empty())
        return;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return;

    int soundid=-1;
    String fileName;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0)
            continue;

        String str = s[0].Trimmed();
        if (str == "id")
            soundid= Urho3D::ToInt(s[1].Trimmed());
        else if (str == "file")
            fileName = s[1].Trimmed();
    }

    if (soundid == -1 || fileName.Empty())
        return;

    SharedPtr<Sound> sound(GetSubsystem<ResourceCache>()->GetResource<Sound>(fileName));
    if (sound)
        GOC_SoundEmitter_Template::RegisterSound(soundid, sound);
    else
        URHO3D_LOGERRORF("GOC_SoundEmitter() - RegisterSound=%s => no sound file !", value.CString());
}

void GOC_SoundEmitter::SetTemplate(const String& value)
{
    if (value.Empty())
        return;

    StringHash key = StringHash(value);

//    URHO3D_LOGINFOF("GOC_SoundEmitter() - SetTemplate=%s(%u)", s.CString(), key.Value());

    ApplyTemplateProperties(key);
}

void GOC_SoundEmitter::SetSoundAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - SetSoundAttr (%s)",value.CString());

    if (value.Empty())
        return;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return;

    int soundid=-1;
    String eventName, stateName, paramName;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0)
            continue;

        String str = s[0].Trimmed();
        if (str == "id")
            soundid= Urho3D::ToInt(s[1].Trimmed());
        else if (str == "event")
            eventName = s[1].Trimmed();
        else if (str == "param")
            paramName = s[1].Trimmed();
        else if (str == "state")
            stateName = s[1].Trimmed();
    }

    if (soundid == -1 && (eventName.Empty() || stateName.Empty()))
    {
        URHO3D_LOGERRORF("GOC_SoundEmitter() - SetSoundAttr : NOK! soundid=%d | eventName=%s | stateName=%s", soundid, eventName.CString(), stateName.CString());
        return;
    }
    if (!customTemplate)
    {
//        URHO3D_LOGINFO("GOC_SoundEmitter() - SetSoundAttr : create new custom Template (dont't forget to register it) !");
        GOC_SoundEmitter_Template* newTemplate = new GOC_SoundEmitter_Template("SoundTemplate_Custom",*currentTemplate);
        customTemplate = true;
        currentTemplate = newTemplate;
    }

    if (!eventName.Empty())
    {
        if (eventName == "SPRITER_Sound")
            currentTemplate->AddSoundForSpriterEvent(soundid, paramName);
        else
            currentTemplate->AddSoundForEvent(soundid, eventName);
    }

    if (!stateName.Empty())
        currentTemplate->AddSoundForState(soundid, stateName);

//    LOGINFOF("GOC_SoundEmitter() - SetSoundAttr : OK!");
}

void GOC_SoundEmitter::ApplyTemplateProperties(StringHash key)
{
    unsigned k = key.Value();

    ClearCustomTemplate();

//    URHO3D_LOGINFOF("GOC_SoundEmitter() - ApplyTemplateProperties : key=%u",k);

    if (GOC_SoundEmitter_Template::templates_.Contains(k))
        currentTemplate = &(GOC_SoundEmitter_Template::templates_.Find(k)->second_);
    else
        currentTemplate = &(GOC_SoundEmitter_Template::templates_.Find(SOUND_TEMPLATE_EMPTY)->second_);

    UpdateSubscriber();
}

void GOC_SoundEmitter::Init()
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - Init");

}

void GOC_SoundEmitter::ClearCustomTemplate()
{
    if (customTemplate)
    {
        if (currentTemplate)
        {
            StringHash event;
            for (HashMap<unsigned, int>::ConstIterator it=currentTemplate->eventSoundTable.Begin(); it!=currentTemplate->eventSoundTable.End(); ++it)
            {
                event.SetValue(it->first_);
                if (HasSubscribedToEvent(GetNode(), event))
                    UnsubscribeFromEvent(GetNode(), event);
            }

//            URHO3D_LOGINFOF("GOC_SoundEmitter() - ClearCustomTemplate : Delete Template Name=%s",currentTemplate->name.CString());
            delete currentTemplate;
            currentTemplate = 0;
        }
        customTemplate = false;
    }
}

void GOC_SoundEmitter::OnSetEnabled()
{
    if (IsEnabledEffective())
    {
        UpdateSubscriber();
    }
    else
    {
        UnsubscribeFromAllEvents();
    }
}

void GOC_SoundEmitter::OnNodeSet(Node* node)
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - OnNodeSet");

    if (node)
    {
        if (!source)
        {
//            URHO3D_LOGINFOF("GOC_SoundEmitter() - OnSetEnabled : %s(%u) create sound source component ...", node_->GetName().CString(), node_->GetID());
            source = node_->GetOrCreateComponent<SoundSource3D>(LOCAL);
            source->SetTemporary(true);
            source->SetSoundType(SOUND_EFFECT);
//            source->SetAttenuation(0.7f);
            source->SetFarDistance(12.f);
        }
    }
}

void GOC_SoundEmitter::UpdateSubscriber()
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - UpdateSubscriber : %s(%u) ...", node_->GetName().CString(), node_->GetID());

    if (GameContext::Get().gameConfig_.soundEnabled_)
    {
        if (currentTemplate)
        {
            if (currentTemplate->stateSoundTable.Size() && !HasSubscribedToEvent(GetNode(), GO_CHANGESTATE))
                SubscribeToEvent(GetNode(), GO_CHANGESTATE, URHO3D_HANDLER(GOC_SoundEmitter, OnChangeState));
            if (currentTemplate->spriterEventSoundTable.Size() && !HasSubscribedToEvent(GetNode(), SPRITER_SOUND))
                SubscribeToEvent(GetNode(), SPRITER_SOUND, URHO3D_HANDLER(GOC_SoundEmitter, OnEvent));

            StringHash event;
            for (HashMap<unsigned, int>::ConstIterator it=currentTemplate->eventSoundTable.Begin(); it!=currentTemplate->eventSoundTable.End(); ++it)
            {
                event.SetValue(it->first_);
                if (!HasSubscribedToEvent(GetNode(), event))
                {
                    SubscribeToEvent(GetNode(), event, URHO3D_HANDLER(GOC_SoundEmitter, OnEvent));
//                    URHO3D_LOGINFOF("GOC_SoundEmitter() - UpdateSubscriber : Subscribe To Event %s", GOE::GetEventName(event).CString());
                }
            }
        }
    }

    if (!HasSubscribedToEvent(GAME_SOUNDVOLUMECHANGED))
        SubscribeToEvent(GAME_SOUNDVOLUMECHANGED, URHO3D_HANDLER(GOC_SoundEmitter, HandleSoundEnabled));
}

void GOC_SoundEmitter::OnEvent(StringHash eventType, VariantMap& eventData)
{
    if (eventType == SPRITER_SOUND)
    {
        HashMap<unsigned, int >::ConstIterator it = currentTemplate->spriterEventSoundTable.Find(eventData[SPRITER_Event::TYPE].GetUInt() );
        if (it != currentTemplate->spriterEventSoundTable.End())
        {
            Sound* sound = currentTemplate->GetSound(it->second_);
            if (!sound)
                return;

            if (source)
            {
                source->Play(sound);
                //URHO3D_LOGINFOF("GOC_SoundEmitter() - OnEvent : Spriter Sound %s Played ... attenuation=%F !", sound->GetName().CString(), source->GetAttenuation());
            }
            else
            {
                URHO3D_LOGERRORF("GOC_SoundEmitter() - OnEvent : Spriter Sound %s Not Played (no source component) !", sound->GetName().CString());
            }
        }
    }
    else
    {
        HashMap<unsigned, int >::ConstIterator it = currentTemplate->eventSoundTable.Find(eventType.Value());
        if (it != currentTemplate->eventSoundTable.End())
        {
            Sound* sound = currentTemplate->GetSound(it->second_);
            if (!sound)
                return;

            if (source)
            {
                source->Play(sound);
                //URHO3D_LOGINFOF("GOC_SoundEmitter() - OnEvent : Sound %s Played ... attenuation=%F !", sound->GetName().CString(), source->GetAttenuation());
            }
            else
            {
                URHO3D_LOGERRORF("GOC_SoundEmitter() - OnEvent : Sound %s Not Played (no source component) !", sound->GetName().CString());
            }
        }
    }
}

void GOC_SoundEmitter::OnChangeState(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - OnChangeState : state=%u", state);
    HashMap<unsigned, int >::ConstIterator it = currentTemplate->stateSoundTable.Find(eventData[Go_ChangeState::GO_STATE].GetUInt());
    if (it != currentTemplate->stateSoundTable.End())
    {
        Sound* sound = currentTemplate->GetSound(it->second_);
        if (!sound)
            return;

        if (source)
        {
            source->Play(sound);
            //URHO3D_LOGINFOF("GOC_SoundEmitter() - OnChangeState : Sound %s Played !", sound->GetName().CString());
        }
        else
        {
            URHO3D_LOGERRORF("GOC_SoundEmitter() - OnChangeState : Sound %s Not Played (no source component) !", sound->GetName().CString());
        }
    }
}

void GOC_SoundEmitter::HandleSoundEnabled(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_SoundEmitter() - HandleSoundEnabled : %s(%u) ... soundenable=%s", node_->GetName().CString(), node_->GetID(), GameContext::Get().gameConfig_.soundEnabled_ ? "true":"false");

    UpdateSubscriber();
}

