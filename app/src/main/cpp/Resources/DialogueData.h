#pragma once

#include <Urho3D/Resource/Resource.h>

namespace Urho3D
{
class XMLFile;
}

using namespace Urho3D;

#include "Dialogue.h"

class Actor;

struct DialogueData : public Resource
{
    URHO3D_OBJECT(DialogueData, Resource);

public:
    DialogueData(Context* context);
    virtual ~DialogueData();

    /// Register object factory.
    static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    virtual bool BeginLoad(Deserializer& source);
    /// Finish resource loading. Always called from the main thread. Return true if successful.
    virtual bool EndLoad();

    /// Compile a string command and Return the commandid
    int Compile(const String& value);
    /// Execute Dialogue Command with the bytecode
    bool ExecuteCode(const StringHash& hashname, int icmd, Actor* speaker, Actor* contributor);
    /// Interpret and Execute Dialogue Command
    bool ExecuteSource(const StringHash& hashname, int icmd, Actor* speaker, Actor* contributor);

    /// Return the dialog
    const Dialogue* Get(const StringHash& hashname) const;
    /// Return the first message for the speaker
    const DialogueMessage* GetMessage(const StringHash& hashname, Actor* speaker) const;
    /// Return all the messages for the speaker
    const Vector<DialogueMessage>* GetMessages(const StringHash& hashname, Actor* speaker=0) const;
    /// Return the reponses for the contributor
    Vector<const DialogueResponse*> GetReponses(const StringHash& hashname, int index=0, Actor* contributor=0) const;

    void Dump() const;

    static Actor* GetSpeaker()
    {
        return dialogueSpeaker_;
    }
    static Actor* GetContributor()
    {
        return dialogueContributor_;
    }

private :
    /// Begin load from XML file.
    bool BeginLoadFromXMLFile(Deserializer& source);
    /// End load from XML file.
    bool EndLoadFromXMLFile();

    HashMap<StringHash, Dialogue> dialogs_;
    Vector<void*> bytecodes_;
    void* vm_;

    SharedPtr<XMLFile> loadXMLFile_;

    static Actor* dialogueSpeaker_;
    static Actor* dialogueContributor_;
};


