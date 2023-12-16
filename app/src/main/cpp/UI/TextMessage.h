#pragma once

namespace Urho3D
{
class Object;
class Node;
class Text;
class Text3D;
}

using namespace Urho3D;


class TextMessage : public Object
{
    URHO3D_OBJECT(TextMessage, Object);

public :
    TextMessage(Context* context);
    ~TextMessage();

    void Remove();

    void Set(Node* node, const String& message, const char *fontName, int fontSize,
             float duration, float fadescale=1.f, bool follow=true, bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);
    void Set(const String& message, const char *fontName, int fontSize,
             float duration, IntVector2 position=IntVector2::ZERO, float fadescale=1.f, bool autoRemove=true, float delayedStart=0.f, float delayedRemove=0.f);
    void SetDuration(float duration);
    void SetColor(const Color& colorTL=Color::WHITE, const Color& colorTR=Color::WHITE, const Color& colorBL=Color::WHITE, const Color& colorBR=Color::WHITE);
    void SetExpireEvent(const StringHash& event)
    {
        expireEvent_ = event;
    }
    bool IsExpired()
    {
        return (timer_ > expirationTime3_);
    }

private :
    void OnRemove(StringHash eventType, VariantMap& eventData);

    void UpdatePosition();

    void handleUpdate1(StringHash eventType, VariantMap& eventData);
    void handleUpdate2(StringHash eventType, VariantMap& eventData);
    void handleUpdate1_3D(StringHash eventType, VariantMap& eventData);
    void handleUpdate2_3D(StringHash eventType, VariantMap& eventData);
    void handleUpdate3(StringHash eventType, VariantMap& eventData);

    WeakPtr<Text> text_;
    WeakPtr<Node> node_;
    Text3D* text3D_;
    IntVector2 position_;
    StringHash expireEvent_;
    int type_;
    bool autoRemove_;
    float fadescale_;
    float timer_;
    float expirationTime1_, expirationTime2_, expirationTime3_;
};
