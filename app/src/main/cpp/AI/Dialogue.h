#pragma once

using namespace Urho3D;


class Actor;


const unsigned DIALOGUE_MAXRESPONSES = 5;


struct DialogueMessage
{
    StringHash id_;
    String text_;
    int category_;
};

struct DialogueResponse
{
    StringHash name_;
    StringHash next_;
    int msgindex_;
    int condid_;
    int cmdid_;
};

struct DialogueCommand
{
    int msgindex_;
    int cmdid_;
    String sourcecode_;
};

struct Dialogue
{
    String name_;
    Vector<DialogueMessage> messages_;
    Vector<DialogueResponse> responses_;
    Vector<DialogueCommand> commands_;
};

enum DialogueInstruction
{
    DIALINST_WAIT           = 0x00,
    DIALINST_OPENSHOP       = 0x01,
    DIALINST_CLOSEDIALOGUE  = 0x02,
};

struct DialogueInstance
{
    DialogueInstance(const Dialogue& dialogue) : dialogue_(dialogue), icmd_(0) { }
    ~DialogueInstance() { }

    void Update();

private :
    const Dialogue& dialogue_;

    unsigned icmd_;
};
