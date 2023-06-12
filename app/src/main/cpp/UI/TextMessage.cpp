#include <Urho3D/Urho3D.h>

//#include <Urho3D/Core/CoreEvents.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>
#include <Urho3D/Scene/ObjectAnimation.h>
#include <Urho3D/Scene/ValueAnimation.h>

#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/Text3D.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIElement.h>

#include "GameEvents.h"

#include "TextMessage.h"

static const Color defaultTxtMsgColor_[4] =
{
    Color(1.f, 0.5f, 0.f, 1.f),
    Color(1.f, 0.25f, 0.f, 1.f),
    Color(0.5f, 0.25f, 0.f, 0.6f),
    Color(0.3f, 0.15f, 0.f, 0.6f)
};

//static const Color defaultTxtMsgColor_[4] =
//{
//    Color(1.f, 1.f, 1.f, 1.f),
//    Color(1.f, 1.f, 1.f, 1.f),
//    Color(0.75f, 0.75f, 0.75f, 0.6f),
//    Color(0.75f, 0.75f, 0.75f, 0.8f),
//};

//#define TextMessageUpdateNameSpace AttributeAnimationUpdate
//const StringHash TextMessageUpdateEvent = E_ATTRIBUTEANIMATIONUPDATE;
#define TextMessageUpdateNameSpace SceneUpdate
const StringHash TextMessageUpdateEvent = E_SCENEUPDATE;


TextMessage::TextMessage(Context* context) :
    Object(context),
    expireEvent_(StringHash::ZERO),
    fadescale_(1.f)
{
    SubscribeToEvent(TEXTMESSAGE_CLEAN, URHO3D_HANDLER(TextMessage, OnRemove));
    SubscribeToEvent(GAME_EXIT, URHO3D_HANDLER(TextMessage, OnRemove));
}

TextMessage::~TextMessage()
{
    URHO3D_LOGINFOF("~TextMessage() - %u", this);

    UnsubscribeFromAllEvents();

    if (type_ == 0)
    {
        if (text_)
            GetSubsystem<UI>()->GetRoot()->RemoveChild(text_);
        text_.Reset();
    }
    else if (type_ == 1)
    {
        if (node_)
            node_->Remove();
        node_.Reset();
    }
}

void TextMessage::Remove()
{
    this->~TextMessage();
}

void TextMessage::Set(Node* node, const String& message, const char *fontName, int fontSize,
                      float duration, float fadescale, bool follow, bool autoRemove, float delayedStart, float delayedRemove)
{
    type_ = 1;
    fadescale_ = fadescale;

    Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>(fontName);

    node_ = follow ? node->CreateChild("Text3D") : node->GetScene()->CreateChild("Text3D");
    if (!node_)
    {
        this->~TextMessage();
        return;
    }

    Vector3 position = node->GetWorldPosition();
    node_->SetWorldPosition(Vector3(position.x_, position.y_, 0.1f));
    node_->SetEnabled(false);

    text3D_ = node_->CreateComponent<Text3D>();
    if (!text3D_)
    {
        this->~TextMessage();
        return;
    }

    text3D_->SetEnabled(false);
#ifdef ACTIVE_LAYERMATERIALS
    text3D_->SetMaterial(GameContext::Get().layerMaterials_[LAYERDIALOGTEXT]);
#else
    text3D_->SetMaterial(GetSubsystem<ResourceCache>()->GetResource<Material>("Materials/LayerDialogText.xml"));
#endif
    text3D_->SetText(message);
    text3D_->SetColor(C_TOPLEFT, defaultTxtMsgColor_[0]);
    text3D_->SetColor(C_TOPRIGHT, defaultTxtMsgColor_[1]);
    text3D_->SetColor(C_BOTTOMLEFT, defaultTxtMsgColor_[2]);
    text3D_->SetColor(C_BOTTOMRIGHT, defaultTxtMsgColor_[3]);
    text3D_->SetFont(font, fontSize);
    text3D_->SetAlignment(HA_CENTER, VA_CENTER);

    autoRemove_ = autoRemove;
    expirationTime1_ = delayedStart;
    expirationTime2_ = delayedStart + duration;
    expirationTime3_ = expirationTime2_ + delayedRemove;
    timer_ = 0.f;

    SubscribeToEvent(TextMessageUpdateEvent, URHO3D_HANDLER(TextMessage, handleUpdate1_3D));

//    URHO3D_LOGINFOF("TextMessage() - Set : %u type=ui text=%s", this, message.CString());
}

void TextMessage::Set(const String& message, const char *fontName, int fontSize,
                      float duration, IntVector2 position, float fadescale,
                      bool autoRemove, float delayedStart, float delayedRemove)
{
    type_ = 0;
    fadescale_ = fadescale;

    Font* font = GetSubsystem<ResourceCache>()->GetResource<Font>(fontName);
    Graphics* graphics = GetSubsystem<Graphics>();

    text_ = GetSubsystem<UI>()->GetRoot()->CreateChild<Text>();
    if (!text_)
    {
        this->~TextMessage();
        return;
    }

    text_->SetText(message);
    text_->SetFont(font, fontSize);

    if (position.x_ == 0)
        text_->SetHorizontalAlignment(HA_CENTER);
    if (position.y_ == 0)
        text_->SetVerticalAlignment(VA_CENTER);
    if (position.x_ < 0)
        position.x_ += graphics->GetWidth() - fontSize;
    if (position.y_ < 0)
        position.y_ = graphics->GetHeight() - fontSize + position.y_;

    text_->SetPosition(position);

    SetColor(defaultTxtMsgColor_[0], defaultTxtMsgColor_[1], defaultTxtMsgColor_[2], defaultTxtMsgColor_[3]);
//
//    text_->SetTextEffect(TE_STROKE);
//    text_->SetEffectStrokeThickness(2);
//    text_->SetEffectRoundStroke(false);
//    text_->SetEffectColor(Color(0.f, 0.f, 0.f, 0.25f));

    text_->SetVisible(false);

    autoRemove_ = autoRemove;
    expirationTime1_ = delayedStart;
    expirationTime2_ = delayedStart + duration;
    expirationTime3_ = expirationTime2_ + delayedRemove;
    timer_ = 0.f;

    SubscribeToEvent(TextMessageUpdateEvent, URHO3D_HANDLER(TextMessage, handleUpdate1));

//    URHO3D_LOGINFOF("TextMessage() - Set : %u type=node text=%s start=%f", this, message.CString(), delayedStart);
}

void TextMessage::SetDuration(float duration)
{
    expirationTime2_ = duration;
    expirationTime3_ = expirationTime2_;
    timer_ = 0.f;
    text_->SetVisible(true);

    SubscribeToEvent(E_SCENEUPDATE, URHO3D_HANDLER(TextMessage, handleUpdate2));
}

void TextMessage::SetColor(const Color& colorTL, const Color& colorTR, const Color& colorBL, const Color& colorBR)
{
    if (type_ == 0)
    {
        text_->SetColor(C_TOPLEFT, colorTL);
        text_->SetColor(C_TOPRIGHT, colorTR);
        text_->SetColor(C_BOTTOMLEFT, colorBL);
        text_->SetColor(C_BOTTOMRIGHT, colorBR);
    }
    else
    {
        text3D_->SetColor(C_TOPLEFT, colorTL);
        text3D_->SetColor(C_TOPRIGHT, colorTR);
        text3D_->SetColor(C_BOTTOMLEFT, colorBL);
        text3D_->SetColor(C_BOTTOMRIGHT, colorBR);
    }
}

void TextMessage::OnRemove(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("TextMessage() - OnRemove : %u", this);
    Remove();
}

void TextMessage::handleUpdate1(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[TextMessageUpdateNameSpace::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime1_)
    {
        UnsubscribeFromEvent(TextMessageUpdateEvent);

        if (!text_)
        {
            this->~TextMessage();
            URHO3D_LOGERRORF("TextMessage() - handleUpdate1 : no text !");
            return;
        }

        text_->SetVisible(true);
        // put over other UI else than tooltip
        // bug in BringFront()
        // instead use SetPriority
//        text_->SetPriority(M_MAX_INT-1);

        URHO3D_LOGERRORF("TextMessage() - handleUpdate1 : %s", text_->GetText().CString());

        /// Bizarre in this version Tex2D, we need to use a less duration otherwise the animation is incorrect
        float duration = expirationTime2_-expirationTime1_;
        SharedPtr<ObjectAnimation> effectAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.f);
        alphaAnimation->SetKeyFrame(0.1f*duration, 1.f);
        alphaAnimation->SetKeyFrame(0.35f*duration, 1.f);
        alphaAnimation->SetKeyFrame(0.5f*duration, 0.f);
        alphaAnimation->SetKeyFrame(0.75f*duration, 0.f);
        alphaAnimation->SetKeyFrame(duration, 0.f);
        effectAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        if (fadescale_ != 1.f)
        {
            int finalfontsize = (int)(text_->GetFontSize() * fadescale_);
            SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
            scaleAnimation->SetKeyFrame(0.f, text_->GetFontSize());
            scaleAnimation->SetKeyFrame(0.4f*duration, text_->GetFontSize());
            scaleAnimation->SetKeyFrame(0.5f*duration, finalfontsize);
            scaleAnimation->SetKeyFrame(duration, finalfontsize);
            effectAnimation->AddAttributeAnimation("Font Size", scaleAnimation, WM_ONCE);
        }
        text_->SetObjectAnimation(effectAnimation);

//        text_->SetAnimationTime(0.f);

        SubscribeToEvent(TextMessageUpdateEvent, URHO3D_HANDLER(TextMessage, handleUpdate2));
    }
}

void TextMessage::handleUpdate1_3D(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[TextMessageUpdateNameSpace::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime1_)
    {
        UnsubscribeFromEvent(TextMessageUpdateEvent);

        if (!node_)
        {
            this->~TextMessage();
            URHO3D_LOGERRORF("TextMessage() - handleUpdate1_3D : no text !");
            return;
        }

        text3D_->SetEnabled(true);
        node_->SetEnabled(true);

        URHO3D_LOGERRORF("TextMessage() - handleUpdate1_3D : %s text3denable=%s", text3D_->GetText().CString(), text3D_->IsEnabled() ? "true":"false");

        /// Bizarre in this version Tex2D, we need to use a less duration otherwise the animation is incorrect

        float duration = expirationTime2_-expirationTime1_;
        SharedPtr<ObjectAnimation> textAnimation(new ObjectAnimation(context_));
        SharedPtr<ValueAnimation> alphaAnimation(new ValueAnimation(context_));
        alphaAnimation->SetKeyFrame(0.f, 0.f);
        alphaAnimation->SetKeyFrame(0.1f*duration, 1.f);
        alphaAnimation->SetKeyFrame(0.35f*duration, 1.f);
        alphaAnimation->SetKeyFrame(0.5f*duration, 0.f);
        alphaAnimation->SetKeyFrame(0.75f*duration, 0.f);
        alphaAnimation->SetKeyFrame(duration, 0.f);
        textAnimation->AddAttributeAnimation("Opacity", alphaAnimation, WM_ONCE);
        text3D_->SetObjectAnimation(textAnimation);

        bool translate = true;
        if (translate || fadescale_ != 1.f)
        {
            SharedPtr<ObjectAnimation> nodeAnimation(new ObjectAnimation(context_));
            if (translate)
            {
                Vector3 position = node_->GetPosition();
                Vector3 finalposition = position + Vector3(0.f, 3.f, 0.f);
                SharedPtr<ValueAnimation> positionAnimation(new ValueAnimation(context_));
                positionAnimation->SetKeyFrame(0.f, position);
                positionAnimation->SetKeyFrame(0.4f*duration, position);
                positionAnimation->SetKeyFrame(0.5f*duration, finalposition);
                positionAnimation->SetKeyFrame(duration, finalposition);
                nodeAnimation->AddAttributeAnimation("Position", positionAnimation, WM_ONCE);
            }
            if (fadescale_ != 1.f)
            {
                Vector3 scale = node_->GetScale();
                SharedPtr<ValueAnimation> scaleAnimation(new ValueAnimation(context_));
                scaleAnimation->SetKeyFrame(0.f, scale);
                scaleAnimation->SetKeyFrame(0.4f*duration, scale);
                scaleAnimation->SetKeyFrame(0.5f*duration, fadescale_ * scale);
                scaleAnimation->SetKeyFrame(duration, fadescale_ * scale);
                nodeAnimation->AddAttributeAnimation("Scale", scaleAnimation, WM_ONCE);
            }
            node_->SetObjectAnimation(nodeAnimation);
        }

//        node_->SetAnimationTime(0.f);
//        text3D_->SetAnimationTime(0.f);

        SubscribeToEvent(TextMessageUpdateEvent, URHO3D_HANDLER(TextMessage, handleUpdate2_3D));
    }
}

void TextMessage::handleUpdate2(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[TextMessageUpdateNameSpace::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime2_)
    {
        UnsubscribeFromEvent(TextMessageUpdateEvent);

        if (!text_)
        {
            this->~TextMessage();
            return;
        }

        text_->SetVisible(false);

        if (expirationTime3_ > expirationTime2_)
            SubscribeToEvent(TextMessageUpdateEvent, URHO3D_HANDLER(TextMessage, handleUpdate3));
        else
        {
            if (expireEvent_)
                SendEvent(expireEvent_);

            if (autoRemove_)
            {
//                URHO3D_LOGERRORF("TextMessage() - handleUpdate2 ... autoremove !");
                Remove();
            }
        }
    }
}

void TextMessage::handleUpdate2_3D(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[TextMessageUpdateNameSpace::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime2_)
    {
        UnsubscribeFromEvent(TextMessageUpdateEvent);

        if (!node_)
        {
            this->~TextMessage();
            return;
        }

        node_->SetEnabled(false);

        if (expirationTime3_ > expirationTime2_)
            SubscribeToEvent(TextMessageUpdateEvent, URHO3D_HANDLER(TextMessage, handleUpdate3));
        else
        {
            if (expireEvent_)
                SendEvent(expireEvent_);

            if (autoRemove_)
            {
                URHO3D_LOGERRORF("TextMessage() - handleUpdate2_3D ... autoremove !");
                Remove();
            }
        }
    }
}

void TextMessage::handleUpdate3(StringHash eventType, VariantMap& eventData)
{
    timer_ += eventData[TextMessageUpdateNameSpace::P_TIMESTEP].GetFloat();
    if (timer_ > expirationTime3_)
    {
        UnsubscribeFromEvent(TextMessageUpdateEvent);

        if (expireEvent_)
            SendEvent(expireEvent_);

        if (autoRemove_ && (text_ || node_))
        {
//            URHO3D_LOGERRORF("TextMessage() - handleUpdate3 ... autoremove !");
            Remove();
        }
    }
}
