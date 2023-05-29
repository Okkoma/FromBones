#include <cstdio>

#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Serializer.h>
#include <Urho3D/IO/Deserializer.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include "GameOptions.h"
#include "GameHelpers.h"

#include "Actor.h"

#include <PugiXml/pugixml.hpp>

#ifdef USE_WREN
#include "wren.hpp"
#endif // USE_WREN

#include "DialogueData.h"


String testVMSource("System.print(\"I am running in a VM!\")");


/// Dialogue Resource

const char* MessageCategoryStr[] =
{
    "",
    0
};

Actor* DialogueData::dialogueSpeaker_ = 0;
Actor* DialogueData::dialogueContributor_ = 0;

DialogueData::DialogueData(Context* context) :
    Resource(context)
{
    URHO3D_LOGINFO("DialogueData()");
#ifdef USE_WREN
    vm_ = (void*)wrenInitializeVM();
#else

#endif
}

DialogueData::~DialogueData()
{
    URHO3D_LOGINFO("~DialogueData()");
#ifdef USE_WREN
    for (unsigned i=0; i < bytecodes_.Size(); ++i)
        wrenReleaseHandle((WrenVM*)vm_, (WrenHandle*)bytecodes_[i]);
    wrenFreeVM((WrenVM*)vm_);
#else

#endif
}

void DialogueData::RegisterObject(Context* context)
{
    context->RegisterFactory<DialogueData>();
}

bool DialogueData::BeginLoad(Deserializer& source)
{
    if (GetName().Empty())
        SetName(source.GetName());

    String extension = GetExtension(source.GetName());
    if (extension == ".xml")
        return BeginLoadFromXMLFile(source);

    URHO3D_LOGERROR("DialogueData() - BeginLoad : Unsupported file type !");
    return false;
}

bool DialogueData::EndLoad()
{
    if (loadXMLFile_)
        return EndLoadFromXMLFile();

    return false;
}

bool DialogueData::BeginLoadFromXMLFile(Deserializer& source)
{
    loadXMLFile_ = new XMLFile(context_);
    if (!loadXMLFile_->Load(source))
    {
        URHO3D_LOGERROR("DialogueData() - BeginLoad : could not load file !");
        loadXMLFile_.Reset();
        return false;
    }

    SetMemoryUse(source.GetSize());

    if (!loadXMLFile_->GetRoot("dialoguedata"))
    {
        URHO3D_LOGERROR("DialogueData() - BeginLoadFromXMLFile : Invalid Dialogue File !");
        loadXMLFile_.Reset();
        return false;
    }

    return true;
}

bool DialogueData::EndLoadFromXMLFile()
{
    XMLElement rootElem = loadXMLFile_->GetRoot("dialoguedata");
    XMLElement dialogueElem = rootElem.GetChild("dialogue");
    while (dialogueElem)
    {
        String name = dialogueElem.GetAttribute("name");
        StringHash hashname(name);
        if (DialogueData::Get(hashname))
        {
            URHO3D_LOGWARNINGF("DialogueData() : EndLoadFromXMLFile - dialogue=%s already registered !", name.CString());
            continue;
        }

        Dialogue& dialogue = dialogs_[hashname];
        dialogue.name_ = name;

        int msgcount = -1;

        const pugi::xml_node& node = pugi::xml_node(dialogueElem.GetNode());
        pugi::xml_object_range<pugi::xml_node_iterator> children = node.children();
        for (pugi::xml_node_iterator it = children.begin(); it != children.end(); ++it)
        {
            XMLElement elem(loadXMLFile_.Get(), (*it).internal_object());

            if (elem.GetName() == "message")
            {
                dialogue.messages_.Resize(dialogue.messages_.Size()+1);
                DialogueMessage& message = dialogue.messages_.Back();
                message.text_ = elem.GetValue();
                message.id_ = hashname;
                message.category_ = GetEnumValue(elem.GetAttribute("category"), MessageCategoryStr);
                msgcount++;
            }
            else if (elem.GetName() == "response")
            {
                dialogue.responses_.Resize(dialogue.responses_.Size()+1);
                DialogueResponse& response = dialogue.responses_.Back();
                response.name_ = StringHash(elem.GetAttribute("name"));
                response.next_ = StringHash(elem.GetAttribute("next"));
                response.msgindex_ = msgcount;
                if (elem.HasAttribute("command"))
                    response.cmdid_ = Compile(elem.GetAttribute("command"));
                if (elem.HasAttribute("condition"))
                    response.condid_ = Compile(elem.GetAttribute("condition"));
            }
            else if (elem.GetName() == "command")
            {
                dialogue.commands_.Resize(dialogue.commands_.Size()+1);
                DialogueCommand& command = dialogue.commands_.Back();
                command.sourcecode_ = elem.GetValue();
                command.msgindex_ = msgcount;
                command.cmdid_ = Compile(command.sourcecode_);
            }
        }

        dialogueElem = dialogueElem.GetNext("dialogue");
    }

    loadXMLFile_.Reset();
    return true;
}

int DialogueData::Compile(const String& source)
{
    /// TEST : no compile => just interpret in execution
    //return -1;

    if (source.Empty())
        return -1;

#ifdef USE_WREN
    // compile and get a handle
    WrenHandle* handle = wrenCompileToHandle((WrenVM*)vm_, source.CString());
    if (handle == NULL)
    {
        URHO3D_LOGERRORF("DialogueData() - Compile : source=%s ... compile error !", source.CString());
        return -1;
    }

    bytecodes_.Push((void*)handle);
#else

#endif

//    printf("compile source=\"%s\" => cmdid=%d \n", source.CString(), (int)(bytecodes_.Size()-1));
//    URHO3D_LOGINFOF("DialogueData() - Compile : source=%s => cmdid = %d ... compile ok !", source.CString(), (int)(bytecodes_.Size()-1));
    return (int)(bytecodes_.Size()-1);
}

bool DialogueData::ExecuteCode(const StringHash& hashname, int icmd, Actor* speaker, Actor* contributor)
{
    const Vector<DialogueCommand>& commands = Get(hashname)->commands_;
    if (commands.Size())
    {
        if (icmd < commands.Size())
        {
            URHO3D_LOGINFOF("DialogueData() - ExecuteCode : cmd=%d for %u", icmd, hashname.Value());

            dialogueSpeaker_ = speaker;
            dialogueContributor_ = contributor;

#ifdef USE_WREN
            wrenRunHandle((WrenVM*)vm_, (WrenHandle*)bytecodes_[commands[icmd].cmdid_]);
#else

#endif
            return true;
        }
        else
        {
//            URHO3D_LOGERRORF("DialogueData() - ExecuteCode : no cmd=%d for %u", icmd, hashname.Value());
            return false;
        }
    }
    return false;
}

bool DialogueData::ExecuteSource(const StringHash& hashname, int icmd, Actor* speaker, Actor* contributor)
{
    const Vector<DialogueCommand>& commands = Get(hashname)->commands_;
    if (commands.Size())
    {
        if (icmd < commands.Size())
        {
            URHO3D_LOGINFOF("DialogueData() - ExecuteSource : cmd=%d for %u", icmd, hashname.Value());

            dialogueSpeaker_ = speaker;
            dialogueContributor_ = contributor;
#ifdef USE_WREN
            WrenInterpretResult result = wrenInterpret((WrenVM*)vm_, commands[icmd].sourcecode_.CString());
#else

#endif
            return true;
        }
        else
        {
//            URHO3D_LOGERRORF("DialogueData() - ExecuteSource : no cmd=%d for %u", icmd, hashname.Value());
            return false;
        }
    }
    return false;
}

const Dialogue* DialogueData::Get(const StringHash& hashname) const
{
    HashMap<StringHash, Dialogue>::ConstIterator it = dialogs_.Find(hashname);
    return it != dialogs_.End() ? &(it->second_) : 0;
}

const DialogueMessage* DialogueData::GetMessage(const StringHash& hashname, Actor* speaker) const
{
    const Dialogue* dialogue = Get(hashname);
    if (!dialogue || !dialogue->messages_.Size())
        return 0;

    return &dialogue->messages_[0];
}

const Vector<DialogueMessage>* DialogueData::GetMessages(const StringHash& hashname, Actor* speaker) const
{
    const Dialogue* dialogue = Get(hashname);
    if (!dialogue || !dialogue->messages_.Size())
        return 0;

    return &dialogue->messages_;
}

Vector<const DialogueResponse*> DialogueData::GetReponses(const StringHash& hashname, int msgindex, Actor* contributor) const
{
    Vector<const DialogueResponse*> responses;

    const Dialogue* dialogue = Get(hashname);
    if (!dialogue || !dialogue->responses_.Size())
        return responses;

    for (Vector<DialogueResponse>::ConstIterator it=dialogue->responses_.Begin(); it!=dialogue->responses_.End(); ++it)
    {
        const DialogueResponse& dresponse = *it;
        if (msgindex == dresponse.msgindex_)
        {
            const DialogueMessage* responsemessage = GetMessage(dresponse.name_, contributor);
            if (!responsemessage->text_.Empty())
                responses.Push(&dresponse);
        }
    }

    return responses;
}

void DialogueData::Dump() const
{
    URHO3D_LOGINFO("DialogueData() - Dump() ...");

    unsigned i=0;
    for (HashMap<StringHash, Dialogue>::ConstIterator dt=dialogs_.Begin(); dt!=dialogs_.End(); ++dt,++i)
    {
        const Dialogue& dialogue = dt->second_;
        URHO3D_LOGINFOF("Dialogue[%u] name=%s hash=%u =>", i, dialogue.name_.CString(), dt->first_.Value());

        unsigned j=0;
        for (Vector<DialogueMessage>::ConstIterator jt=dialogue.messages_.Begin(); jt!=dialogue.messages_.End(); ++jt,++j)
            URHO3D_LOGINFOF(" Message[%u] text=%s id=%u category=%s(%u)", j, jt->text_.CString(), jt->id_.Value(), MessageCategoryStr[jt->category_], jt->category_);
        j=0;
        for (Vector<DialogueResponse>::ConstIterator jt=dialogue.responses_.Begin(); jt!=dialogue.responses_.End(); ++jt,++j)
            URHO3D_LOGINFOF(" Response[%u] id=%u next=%u condid=%d cmdid=%d", j, jt->name_.Value(), jt->next_.Value(), jt->condid_, jt->cmdid_);
        j=0;
        for (Vector<DialogueCommand>::ConstIterator jt=dialogue.commands_.Begin(); jt!=dialogue.commands_.End(); ++jt,++j)
            URHO3D_LOGINFOF(" Command[%u] cmdid=%d", j, jt->cmdid_);
    }

    URHO3D_LOGINFO("DialogueData() - Dump() ... OK !");
}
