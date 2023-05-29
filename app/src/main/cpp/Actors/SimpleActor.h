#pragma once


namespace Urho3D
{
class Node;
class Scene;
}

using namespace Urho3D;


class ViewManager;


class SimpleActor : public Object
{
    URHO3D_OBJECT(SimpleActor, Object);

public :
    SimpleActor(Context* context) : Object(context), controlID_(0), viewManager_(0) { ; }
    SimpleActor(Context* context, const String& name) : Object(context), name_(name), controlID_(0), viewManager_(0) { ; }
    SimpleActor(const SimpleActor& actor) : Object(actor.GetContext()), name_(actor.name_), controlID_(0), viewManager_(0) { ; }
    ~SimpleActor();

    static void RegisterObject(Context* context);

    bool operator == (const SimpleActor& rhs) const
    {
        return nodeID_ == rhs.nodeID_;
    }
    bool operator != (const SimpleActor& rhs) const
    {
        return nodeID_ != rhs.nodeID_;
    }

    void SetScene(Scene* scene);
    void SetViewManager(ViewManager* viewManager)
    {
        viewManager_ = viewManager;
    }
    void SetControlID(int ID)
    {
        controlID_ = ID;
    }

    void SetNameFile(const String& filename);
    void SetAnimationSet(const String& nameSet = String::EMPTY);
    void ResetAvatar(const Vector2& position = Vector2::ZERO, const Vector2& dir = Vector2::ZERO);

    virtual void Start();
    virtual void Stop();

    Node* GetAvatar() const
    {
        return avatar_;
    }
    Scene* GetScene() const
    {
        return scene_;
    }

    String name_;
    unsigned nodeID_;
    int controlID_;

protected:
    void ResetMapping();
    void UpdateComponents();

    Scene* scene_;
    WeakPtr<Node> avatar_;
    ViewManager* viewManager_;
};

