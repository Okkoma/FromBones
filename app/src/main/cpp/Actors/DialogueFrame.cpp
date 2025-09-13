#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Object.h>
#include <Urho3D/Core/Context.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/Localization.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>
#include <Urho3D/Resource/ResourceEvents.h>

#include <Urho3D/Graphics/Drawable.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionShape2D.h>

#include <Urho3D/Input/Input.h>
#include <Urho3D/Input/InputEvents.h>

#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UIEvents.h>

#include "DefsColliders.h"

#include "GameContext.h"
#include "GameHelpers.h"
#include "GameOptions.h"
#include "GameEvents.h"
#include "GameAttributes.h"
#include "GameData.h"

#include "ViewManager.h"

#include "TimerInformer.h"
#include "TimerRemover.h"

#include "Actor.h"

#include "DialogueFrame.h"

//const int NumMaxCharByLine = 28;
//const int NumMaxRowByLine = 3;

const String INTERACTIVEKEYWORD("Interactive_");


/// Dialogue Frame

DialogueFrame::DialogueFrame(Context* context) :
    Component(context)
{
    URHO3D_LOGDEBUG("DialogueFrame() ... OK !");
    enabled_ = false;
}

DialogueFrame::~DialogueFrame()
{
    URHO3D_LOGDEBUG("~DialogueFrame() ... OK !");
}


void DialogueFrame::RegisterObject(Context* context)
{
    context->RegisterFactory<DialogueFrame>();

    URHO3D_ACCESSOR_ATTRIBUTE("Layout", GetLayoutName, SetLayoutName, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Frame Position", GetFramePosition, SetFramePosition, Vector2, Vector2::ZERO, AM_FILE);
}

void SetLayering(PODVector<Node*> nodes, int layer, int viewport)
{
    unsigned viewmask = (VIEWPORTSCROLLER_OUTSIDE_MASK | VIEWPORTSCROLLER_INSIDE_MASK) << viewport;

    for (unsigned i=0; i < nodes.Size(); ++i)
    {
        Drawable* drawable = nodes[i]->GetDerivedComponent<Drawable>();
        if (drawable)
        {
            drawable->SetViewMask(viewmask);
            Drawable2D* drawable2d = nodes[i]->GetDerivedComponent<Drawable2D>();
            if (drawable2d)
            {
                drawable2d->SetLayer2(IntVector2(layer, -1));
                drawable2d->SetOrderInLayer(i+1);
            }
        }
    }
}

void DialogueFrame::SetLayoutName(const String& layoutfile)
{
    if (layoutname_ != layoutfile && !layoutfile.Empty())
    {
        URHO3D_LOGINFOF("DialogueFrame() - SetLayoutName : %s", layoutfile.CString());

        layoutname_ = layoutfile;
        if (frameNode_)
            frameNode_->Remove();
        frameNode_.Reset();
    }

    if (!frameNode_ && !layoutname_.Empty())
    {
        frameNode_ = node_->CreateChild("DialogueFrame", LOCAL);

        // Load frame node
        if (!GameHelpers::LoadNodeXML(GetContext(), frameNode_, layoutname_, LOCAL))
        {
            URHO3D_LOGINFOF("DialogueFrame() - SetLayoutName : Can't load layout=%s ... NOK !", layoutname_.CString());
            return;
        }

        PODVector<Node*> nodes;
        frameNode_->GetChildren(nodes);

        // Add "interactive" nodes only
        for (int i=0; i< nodes.Size(); i++)
        {
            if (nodes[i]->GetName().StartsWith(INTERACTIVEKEYWORD))
            {
                URHO3D_LOGINFOF("DialogueFrame() - SetLayoutName : Node[%d]=%s interactive", i, nodes[i]->GetName().CString());
                interactiveItems_.Push(nodes[i]);
            }
        }

#ifdef ACTIVE_LAYERMATERIALS
        PODVector<StaticSprite2D*> staticsprites;
        frameNode_->GetComponents<StaticSprite2D>(staticsprites, true);
        for (PODVector<StaticSprite2D*>::Iterator it = staticsprites.Begin(); it != staticsprites.End(); ++it)
            (*it)->SetCustomMaterial(GameContext::Get().layerMaterials_[LAYERDIALOG]);

        PODVector<Text3D*> text3ds;
        frameNode_->GetComponents<Text3D>(text3ds, true);
        for (PODVector<Text3D*>::Iterator it = text3ds.Begin(); it != text3ds.End(); ++it)
            (*it)->SetMaterial(GameContext::Get().layerMaterials_[LAYERDIALOGTEXT]);
#else
        Material* material = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/LayerDialog.xml");
        PODVector<StaticSprite2D*> staticsprites;
        frameNode_->GetComponents<StaticSprite2D>(staticsprites, true);
        for (PODVector<StaticSprite2D*>::Iterator it = staticsprites.Begin(); it != staticsprites.End(); ++it)
            (*it)->SetCustomMaterial(material);

        material = GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/LayerDialogText.xml");
        PODVector<Text3D*> text3ds;
        frameNode_->GetComponents<Text3D>(text3ds, true);
        for (PODVector<Text3D*>::Iterator it = text3ds.Begin(); it != text3ds.End(); ++it)
            (*it)->SetMaterial(material);
#endif
        // Set frame attributes
        frameNode_->SetWorldScale(Vector3::ONE);
        frameNode_->SetPosition(Vector3(framePosition_, 1.f));
        nodes.Insert(0, frameNode_);
        SetLayering(nodes, frameLayer_, 0);

        Text3D* text = frameNode_->GetChild("Message")->GetComponent<Text3D>();
        int fontsize = text->GetFontSize();
        IntVector2 framesize = DialogueFrame::GetFrameSize();
        numMaxCharByLine_ = framesize.x_ / fontsize;
        numMaxRowByLine_ = framesize.y_ / fontsize - 1;

        URHO3D_LOGINFOF("DialogueFrame() - SetLayoutName : Create from %s ... framesize=%d,%d fontsize=%d maxcharbyrow=%d maxrow=%d OK !", layoutname_.CString(), framesize.x_, framesize.y_, fontsize, numMaxCharByLine_, numMaxRowByLine_);
    }
}

void DialogueFrame::SetSpeaker(Actor* speaker)
{
    speaker_ = speaker;
}

void DialogueFrame::SetContributor(Actor* contributor)
{
    URHO3D_LOGINFOF("DialogueFrame() - SetContributor = %u", contributor);

    contributor_ = contributor;
}

void DialogueFrame::SetDialogue(const StringHash& dialoguekey, Actor* speaker, bool force)
{
    if (dialoguekey_ != dialoguekey || force)
    {
        dialoguekey_ = dialoguekey;

        if (speaker)
            SetSpeaker(speaker);

        if (dialoguekey_.Value() == 0)
            return;

        msgindex_ = 0;

        const Vector<DialogueMessage>* msgs = GameData::GetDialogueData()->GetMessages(dialoguekey_, speaker_);
        if (msgs)
        {
            GameHelpers::ParseText(msgs->At(msgindex_).text_, dialoguelines_, numMaxCharByLine_ * numMaxRowByLine_);
            dialoguelocked_ = true;
        }
    }

    msgline_ = -1;

    if (frameNode_ && frameNode_->IsEnabled())
        ShowNextLine();
}

void DialogueFrame::SetFramePosition(const Vector2& position)
{
    if (framePosition_ != position)
    {
        framePosition_ = position;

        if (frameNode_)
            frameNode_->SetPosition(Vector3(framePosition_, 1.f));
    }
}

void DialogueFrame::SetFrameLayer(int viewZ, int viewport)
{
//    if (frameLayer_ != layer)
    {
        if (viewZ == INNERVIEW)
            viewZ = FRONTINNERBIOME;
        else
            viewZ = FRONTBIOME;

        frameLayer_ = viewZ + LAYERADDER_DIALOGS;

        if (frameNode_)
        {
            PODVector<Node*> nodes;
            frameNode_->GetChildren(nodes);
            nodes.Insert(0, frameNode_);
            SetLayering(nodes, frameLayer_, viewport);
        }
    }
}

IntVector2 DialogueFrame::GetFrameSize() const
{
    Node* frame = frameNode_->GetChild("Frame");
    Sprite2D* sprite = frame->GetComponent<StaticSprite2D>()->GetSprite();
    if (!sprite)
    {
        URHO3D_LOGERRORF("DialogueFrame::GetFrameSize() : No Sprite !");
        return IntVector2::ZERO;
    }
    return sprite->GetRectangle().Size();
}

void DialogueFrame::OnSetEnabled()
{
    URHO3D_LOGINFOF("DialogueFrame() - OnSetEnabled : %s framenode=%u", enabled_ ? "true":"false", frameNode_.Get());

    if (GetScene() && IsEnabledEffective())
    {
        ToggleFrame(true);
        ShowNextLine();
    }
    else
    {
        ToggleFrame(false);
    }
}

void DialogueFrame::ToggleFrame(bool enable, float delay)
{
    if (enable)
    {
        if (!frameNode_)
            SetLayoutName();

        if (!frameNode_)
            return;

        lastSelectedNode_ = 0;
        selectedNode_ = 0;
        selectedId_ = 0;
        
        SubscribeToInputEvents();
    }
    else
    {
        UnsubscribeFromInputEvents();
    }

    if (frameNode_)
    {
        if (delay == 0.f)
        {
            frameNode_->SetEnabledRecursive(enable);
        }
        else
        {
            TimerRemover* remover = TimerRemover::Get();
            if (remover)
                remover->Start(frameNode_, delay, enable ? ENABLERECURSIVE : DISABLERECURSIVE);
            else
                frameNode_->SetEnabledRecursive(enable);
        }
    }

    URHO3D_LOGINFOF("DialogueFrame() - ToggleFrame %s !", enable ? "true" : "false");
}

void DialogueFrame::OnNodeSet(Node* node)
{
    if (node)
    {
        SubscribeToEvent(E_CHANGELANGUAGE, URHO3D_HANDLER(DialogueFrame, HandleChangeLanguage));
//        URHO3D_LOGINFOF("DialogueFrame() - OnNodeSet : layoutname=%s", layoutname_.CString());
    }
    else
    {
        UnsubscribeFromAllEvents();
    }
}

void DialogueFrame::SubscribeToInputEvents()
{
    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(DialogueFrame, HandleKeyDown));
    SubscribeToEvent(E_KEYUP, URHO3D_HANDLER(DialogueFrame, HandleInput));

    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(DialogueFrame, HandleInput));
    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(DialogueFrame, HandleInput));
#ifndef __ANDROID__
    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(DialogueFrame, HandleInput));
#endif
    SubscribeToEvent(E_TOUCHEND, URHO3D_HANDLER(DialogueFrame, HandleInput));
    SubscribeToEvent(E_TOUCHBEGIN, URHO3D_HANDLER(DialogueFrame, HandleInput));
    SubscribeToEvent(E_TOUCHMOVE, URHO3D_HANDLER(DialogueFrame, HandleInput));

    SubscribeToEvent(E_MOUSEMOVE, URHO3D_HANDLER(DialogueFrame, HandleInput));
    SubscribeToEvent(E_MOUSEBUTTONDOWN, URHO3D_HANDLER(DialogueFrame, HandleInput));

    URHO3D_LOGINFOF("DialogueFrame() - SubscribeToInputEvents !");
}

void DialogueFrame::UnsubscribeFromInputEvents()
{
    UnsubscribeFromEvent(E_KEYDOWN);
    UnsubscribeFromEvent(E_KEYUP);

    UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
#ifndef __ANDROID__
    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
#endif
    UnsubscribeFromEvent(E_TOUCHEND);
    UnsubscribeFromEvent(E_TOUCHBEGIN);
    UnsubscribeFromEvent(E_TOUCHMOVE);

    UnsubscribeFromEvent(E_MOUSEMOVE);
    UnsubscribeFromEvent(E_MOUSEBUTTONDOWN);

    URHO3D_LOGINFOF("DialogueFrame() - UnsubscribeFromInputEvents !");
}

void DialogueFrame::ShowNextLine()
{
    // Next Message in the dialogue
    if (msgline_+1 >= dialoguelines_.Size())
    {
        msgindex_++;

        const Vector<DialogueMessage>* msgs = GameData::GetDialogueData()->GetMessages(dialoguekey_, speaker_);
        if (msgs && msgindex_ < msgs->Size())
        {
            msgline_ = -1;

            GameHelpers::ParseText(msgs->At(msgindex_).text_, dialoguelines_, numMaxCharByLine_ * numMaxRowByLine_);

            URHO3D_LOGINFOF("DialogueFrame() - ShowNextLine : nextmessage in the dialog dialogkey=%u msgindex=%d", dialoguekey_.Value(), msgindex_);
        }
        else
        {
            URHO3D_LOGERRORF("DialogueFrame() - ShowNextLine : can't find nextmessage in the dialog dialogkey=%u msgindex=%d msgsPtr=%u", 
                            dialoguekey_.Value(), msgindex_, msgs);
        }
    }

    // Next Line of the current message
    if (dialoguelines_.Size() && msgline_+1 < dialoguelines_.Size())
    {
        msgline_++;

        const String& nextline = dialoguelines_[msgline_];
        if (nextline.Length())
        {
            Text3D* text = frameNode_->GetChild("Message")->GetComponent<Text3D>();
            text->SetText(nextline);
            text->SetWidth(GetFrameSize().x_);
            text->SetWordwrap(true);

            bool isQuestion = nextline[nextline.Length()-1] == '?';
            bool endLine = msgline_+1 >= dialoguelines_.Size();
            bool hasNextMsg = false;

            // Check for Next Message
            if (endLine)
            {
                const Vector<DialogueMessage>* msgs = GameData::GetDialogueData()->GetMessages(dialoguekey_, speaker_);
                hasNextMsg = msgs && msgindex_+1 < msgs->Size();
            }

            bool hasNextLine = (!endLine && !isQuestion) || hasNextMsg;

            URHO3D_LOGINFOF("DialogueFrame() - ShowNextLine : dialogkey=%u msgindex=%d isQuestion=%s hasNextLine=%s to contributor=%u",
                            dialoguekey_.Value(), msgindex_, isQuestion ? "true":"false", hasNextLine ? "true":"false", contributor_.Get());

            if (isQuestion && contributor_)
            {
                // Load Player UI Dialog
                VariantMap& eventData = context_->GetEventDataMap();
                eventData[Dialog_Response::ACTOR_ID] = speaker_->GetID();
                eventData[Dialog_Response::DIALOGUENAME] = dialoguekey_;
                eventData[Dialog_Response::DIALOGUEMSGID] = msgindex_;
                contributor_->SendEvent(DIALOG_RESPONSE, eventData);
            }

            // question = desactive nextline button
            // other line & not endline = active nextline button

            frameNode_->GetChild("Interactive_NextLine")->SetEnabled(hasNextLine);

            // execute command if exist
            if (GameData::GetDialogueData()->ExecuteSource(dialoguekey_, msgindex_, speaker_, contributor_))
                dialoguelocked_ = true;
//            GameData::GetDialogueData()->ExecuteCode(dialoguekey_, speaker_, contributor_);
        }
    }
}

void DialogueFrame::HandleChangeLanguage(StringHash eventType, VariantMap& eventData)
{
    Localization* l10n = GetSubsystem<Localization>();
    if (l10n)
    {
        GameData::Get()->SetLanguage(l10n->GetLanguage());
        SetDialogue(dialoguekey_, 0, true);
        URHO3D_LOGINFOF("DialogueFrame() - HandleChangeLanguage : lang=%s", l10n->GetLanguage().CString());
    }
}

void DialogueFrame::HandleUpdate(StringHash eventType, VariantMap& eventData)
{

}

void DialogueFrame::HandleKeyDown(StringHash eventType, VariantMap& eventData)
{
    if (eventData[KeyDown::P_KEY].GetInt() == KEY_ESCAPE)
    {
        SetEnabled(false);
    }
}

void DialogueFrame::HandleInput(StringHash eventType, VariantMap& eventData)
{
    static bool inside = false;
    bool move = false;
    bool launch = false;

    /// Set Mouse Moves in Menu Items
    if (eventType == E_MOUSEMOVE || eventType == E_TOUCHEND || eventType == E_TOUCHMOVE || eventType == E_TOUCHBEGIN)
    {
        IntVector2 position;

        if (eventType == E_MOUSEMOVE)
        {
            GameHelpers::GetInputPosition(position.x_, position.y_);
            //URHO3D_LOGINFOF("DialogueFrame() - HandleInput : mousemove=%s !", position.ToString().CString());
        }
        else if (eventType == E_TOUCHEND)
        {
            position.x_ = eventData[TouchEnd::P_X].GetInt();
            position.y_ = eventData[TouchEnd::P_Y].GetInt();
        }
        else if (eventType == E_TOUCHMOVE)
        {
            position.x_ = eventData[TouchMove::P_X].GetInt();
            position.y_ = eventData[TouchMove::P_Y].GetInt();
        }
        else // E_TOUCH_BEGIN
        {
            position.x_ = eventData[TouchBegin::P_X].GetInt();
            position.y_ = eventData[TouchBegin::P_Y].GetInt();
//            URHO3D_LOGINFOF("DialogueFrame() - HandleInput : E_TOUCHBEGIN=%s !", position.ToString().CString());
        }

        RigidBody2D* body = GameContext::Get().rootScene_->GetComponent<PhysicsWorld2D>()->GetRigidBody(GameHelpers::ScreenToWorld2D(position.x_, position.y_), CM_SCENEUI);
        if (!body || !body->GetNode())
        {
            lastSelectedNode_ = selectedNode_;
            selectedNode_ = 0;
            inside = false;
            move = true;

//            URHO3D_LOGINFOF("DialogueFrame() - HandleInput : no body at %s !", position.ToString().CString());
        }
        else
        {
            if (interactiveItems_.Contains(body->GetNode()))
            {
                if (lastSelectedNode_ != body->GetNode())
                {
                    lastSelectedNode_ = selectedNode_;
                    selectedNode_ = body->GetNode();
                    move = true;
                }
                inside = true;

//                URHO3D_LOGINFOF("DialogueFrame() - HandleInput : interactive body at %s !", position.ToString().CString());
            }
            else
            {
                lastSelectedNode_ = selectedNode_;
                selectedNode_ = 0;
                inside = false;
                move = true;

//                URHO3D_LOGINFOF("DialogueFrame() - HandleInput : NO interactive body at %s !", position.ToString().CString());
            }
        }
    }

    /// Set KeyBoard/JoyPad Moves in Menu Items
    else if (eventType == E_KEYUP || eventType == E_JOYSTICKAXISMOVE || eventType == E_JOYSTICKHATMOVE)
    {
        int direction=0;

        if (eventType == E_KEYUP)
        {
            int key = eventData[KeyUp::P_KEY].GetInt();
            if (key == KEY_UP || key == KEY_LEFT)
                direction = 1;
            else if (key == KEY_DOWN || key == KEY_RIGHT)
                direction = -1;
            move = (direction != 0);
            launch = (key == KEY_SPACE || key == KEY_RETURN);
        }

        if (eventType == E_JOYSTICKAXISMOVE)
        {
            if (abs(eventData[JoystickAxisMove::P_POSITION].GetFloat()) >= 0.9f)
            {
                direction = eventData[JoystickAxisMove::P_POSITION].GetFloat() > 0.f ? -1 : 1;
                move = true;
            }
        }

        if (eventType == E_JOYSTICKHATMOVE && eventData[JoystickHatMove::P_POSITION].GetInt())
        {
            direction = eventData[JoystickHatMove::P_POSITION].GetInt() & (HAT_DOWN | HAT_RIGHT) ? -1 : 1;
            move = true;
        }

        if (move)
        {
            inside = true;
            lastSelectedId_ = selectedId_;

            if (direction == 1 && selectedId_ < interactiveItems_.Size()-1)
                selectedId_++;
            else if (direction == -1 && selectedId_ > 0)
                selectedId_--;
            else
                move = false;
        }

        if (move)
        {
            selectedNode_ = interactiveItems_[selectedId_];
            lastSelectedNode_ = interactiveItems_[lastSelectedId_];
        }
    }

    /// Change Animations
    if (move)
    {
        if (lastSelectedNode_ && lastSelectedNode_->GetComponent<AnimatedSprite2D>())
            lastSelectedNode_->GetComponent<AnimatedSprite2D>()->SetAnimation("unselected");

        if (selectedNode_ && selectedNode_->GetComponent<AnimatedSprite2D>())
            selectedNode_->GetComponent<AnimatedSprite2D>()->SetAnimation("selected");
    }

    /// Check control input for launch
    if ((eventType == E_MOUSEBUTTONDOWN && eventData[MouseButtonDown::P_BUTTON] == MOUSEB_LEFT) ||
            eventType == E_TOUCHEND ||
            (eventType == E_JOYSTICKBUTTONDOWN && eventData[JoystickButtonDown::P_BUTTON] == 0)) // PS4controller = X
    {
//        URHO3D_LOGINFOF("DialogueFrame() - HandleInput : clic inside=%s !", inside ? "true":"false");
        launch = inside;
    }

    /// Launch Selected Item or quit
    if (launch && selectedNode_)
    {
        Vector<String> action = selectedNode_->GetName().Substring(INTERACTIVEKEYWORD.Length()).Split('_');

        if (action[0] == "NextLine")
            ShowNextLine();

        if (!dialoguelocked_)
        {
            URHO3D_LOGINFOF("DialogueFrame() - No more line to read !");

            eventData[Dialog_Close::ACTOR_ID] = speaker_ ? speaker_->GetID() : 0;
            eventData[Dialog_Close::PLAYER_ID] = contributor_ ? contributor_->GetID() : 0;
            if (speaker_)
                speaker_->SendEvent(DIALOG_CLOSE, eventData);
            if (contributor_)
                contributor_->SendEvent(DIALOG_CLOSE, eventData);
        }
    }
}



