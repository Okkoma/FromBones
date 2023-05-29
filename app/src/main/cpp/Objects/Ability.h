#pragma once

#include "DefsNetwork.h"
#include "DefsEntityInfo.h"

using namespace Urho3D;


class GO_Pool;


class Ability : public Object
{
    URHO3D_OBJECT(Ability, Object);

public :
//    static void RegisterObject(Context* context);
    static void RegisterLibraryAbility(Context* context);
    static void Clear();

    static Ability* Get(const StringHash& abi);

    Ability(Context* context);
    virtual ~Ability();

    bool UsePool() const
    {
        return usePool_;
    }
    bool UseNetworkReplication() const
    {
        return useNetwork_;
    }

    void SetHolder(Node* node)
    {
        holder_ = node;
    }
    void SetObjetType(const StringHash& type);

    void UseAtPoint(const Vector2& wpoint);
    bool UseAtPoint(const Vector2& wpoint, Node* holder, const StringHash& GOT2Spawn=StringHash::ZERO);

    static Node* Use(const StringHash& got, Node* holder, const PhysicEntityInfo& physics, bool replicatemode, int viewZ=0, ObjectControlInfo** oinfo=0);
    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0)
    {
        return (Node*)1;
    }

    void Update();

    virtual void OnActive(bool active) { }
    const IntRect& GetIconRect() const
    {
        return icon_;
    }

    StringHash GOT_;

    WeakPtr<Node> node_;
    WeakPtr<Node> holder_;
    GO_Pool* pool_;

    bool isInUse_;
    bool usePool_, useNetwork_;
    IntRect icon_;

private :
    static HashMap<StringHash, SharedPtr<Ability> > staticAbilities_;
};

class ABI_Grapin : public Ability
{
    URHO3D_OBJECT(ABI_Grapin, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_Grapin(Context* context);
    virtual ~ABI_Grapin();

    void Release(float time=10.f);

    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0);

private :
    void HandleRelease(StringHash eventType, VariantMap& eventData);
};

class ABI_CrossWalls : public Ability
{
    URHO3D_OBJECT(ABI_CrossWalls, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_CrossWalls(Context* context);

    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0);
};

class ABI_WallBreaker : public Ability
{
    URHO3D_OBJECT(ABI_WallBreaker, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_WallBreaker(Context* context);

    virtual Node* Use(const Vector2& direction, ObjectControlInfo** oinfo=0);

    static bool OnSameHolderLayerOnly_;
};

class ABI_WallBuilder : public Ability
{
    URHO3D_OBJECT(ABI_WallBuilder, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_WallBuilder(Context* context);

    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0);
};

class ABI_Shooter : public Ability
{
    URHO3D_OBJECT(ABI_Shooter, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_Shooter(Context* context);

    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0);
};

class ABIBomb : public Ability
{
    URHO3D_OBJECT(ABIBomb, Ability);

public :
    static void RegisterObject(Context* context);

    ABIBomb(Context* context);

    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0);
};

class ABI_AnimShooter : public Ability
{
    URHO3D_OBJECT(ABI_AnimShooter, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_AnimShooter(Context* context);

    virtual Node* Use(const Vector2& wpoint, ObjectControlInfo** oinfo=0);
};

class ABI_Fly : public Ability
{
    URHO3D_OBJECT(ABI_Fly, Ability);

public :
    static void RegisterObject(Context* context);

    ABI_Fly(Context* context);

    virtual void OnActive(bool active);
};

