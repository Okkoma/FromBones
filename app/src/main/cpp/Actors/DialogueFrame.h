#pragma once

#include <Urho3D/Scene/Component.h>

namespace Urho3D
{
class Object;
class Node;
}

using namespace Urho3D;

class Actor;

class DialogueFrame : public Component
{
    URHO3D_OBJECT(DialogueFrame, Component);

public :
    DialogueFrame(Context* context);
    virtual ~DialogueFrame();

    static void RegisterObject(Context* context);

    void SetLayoutName(const String& layoutfile=String::EMPTY);
    void SetFramePosition(const Vector2& position);
    void SetFrameLayer(int layer, int viewport);

    void SetSpeaker(Actor* speaker);
    void SetContributor(Actor* contributor);
    void SetDialogue(const StringHash& dialogueKey, Actor* actor=0, bool force=false);

    const String& GetLayoutName() const
    {
        return layoutname_;
    }
    Node* GetFrameNode() const
    {
        return frameNode_;
    }
    const Vector2& GetFramePosition() const
    {
        return framePosition_;
    }
    int GetFrameLayer() const
    {
        return frameLayer_;
    }
    IntVector2 GetFrameSize() const;

    void ToggleFrame(bool enable, float delay=0.f);

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void SubscribeToInputEvents();

    void ShowNextLine();

    void HandleChangeLanguage(StringHash eventType, VariantMap& eventData);
    void HandleUpdate(StringHash eventType, VariantMap& eventData);
    void HandleKeyDown(StringHash eventType, VariantMap& eventData);
    void HandleInput(StringHash eventType, VariantMap& eventData);

    /// Attributes
    String layoutname_;
    Vector2 framePosition_;
    int frameLayer_;
    StringHash dialoguekey_;
    WeakPtr<Actor> speaker_;
    WeakPtr<Actor> contributor_;
    int numMaxCharByLine_;
    int numMaxRowByLine_;

    /// Locals
    WeakPtr<Node> frameNode_;
    Node *selectedNode_;
    Node *lastSelectedNode_;
    int lastSelectedId_, selectedId_, msgline_, msgindex_;
    Vector<Node*> interactiveItems_;
    Vector<String> dialoguelines_;
    bool dialoguelocked_;
};
