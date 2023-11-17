#pragma once

#include <Urho3D/Scene/Component.h>
#include <Urho3D/Input/Controls.h>

#include "GameAttributes.h"
#include "DefsNetwork.h"

namespace Urho3D
{
class CollisionShape2D;
class RigidBody2D;
}

using namespace Urho3D;

class Actor;

static const unsigned ButtonHoldThreshold = 20U;
struct ObjectControlLocal
{
    ObjectControlLocal() : direction_(0.f), totalDpsReceived_(0.f), type_(0), buttons_(0), animation_(0), entityid_(0), flag_(0)  { }

    void Clear()
    {
        memset(this, 0, sizeof(ObjectControlLocal));
    }
    void Reset()
    {
        direction_ = 0.f;
        buttons_ = 0;
    }
    void SetButtons(unsigned buttons, bool down = true)
    {
        if (down)
            buttons_ |= buttons;
        else
            buttons_ &= ~buttons;
    }
    bool IsButtonDown(unsigned button) const
    {
        return (buttons_ & button) != 0;
    }

    float direction_;
    float totalDpsReceived_;
    unsigned type_, buttons_, animation_;         // 3uint + 2float + 2uchar = 5*4 + 2 = 22 bytes
    unsigned char entityid_, flag_;
};


// manage Input GoComponent
class GOC_Controller : public Component
{
    URHO3D_OBJECT(GOC_Controller, Component);

public :
    GOC_Controller(Context* context);
    GOC_Controller(Context* context, int type);

    virtual ~GOC_Controller();
    static void RegisterObject(Context* context);

    virtual void Start();
    virtual void Stop();

    virtual void SetThinker(void* thinker)
    {
        thinker_ = thinker;
    }
    void ResetButtons();
    void ResetDirection();
    void SetControllerType(int type, bool force = false);
    void SetMainController(bool state);

    bool ChangeAvatar(unsigned type, unsigned char entityid);

    bool Update(unsigned buttons, bool forceUpdate);

    void FindAndFollowPath(const Vector2& destination);
    void StopFollowPath();
    void UpdateFollowPath();
    Actor* GetThinker() const
    {
        return (Actor*)thinker_;
    }
    int GetControllerType() const
    {
        return controlType_;
    }
    bool IsFollowingPath() const
    {
        return followPath_;
    }
    bool IsMainController() const
    {
        return mainController_;
    }
    bool IsButtonHold() const
    {
        return buttonholdtime_ == ButtonHoldThreshold;
    }
    unsigned GetButtonHoldTime() const
    {
        return buttonholdtime_;
    }

    virtual void OnSetEnabled();

    ObjectControlLocal control_;
    unsigned prevbuttons_;
    unsigned buttonholdtime_;
    bool forceControlUpdate_;
    bool lastdesync_;

protected :
    virtual void OnNodeSet(Node* node);

    bool mainController_;
    bool followPath_;
    bool lastimpulse_;
    bool noreverse_;

    short unsigned lastcount_;
    int currentIdPath_;
    int currentIdUserPath_;
    void* currentPath_;

private :
    void HandleNetUpdate(StringHash eventType, VariantMap& eventData);

    int controlType_;
    void* thinker_;

    unsigned short laststamp_;
};
