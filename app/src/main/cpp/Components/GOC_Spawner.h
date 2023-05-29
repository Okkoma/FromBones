#pragma once


using namespace Urho3D;


class GOC_Spawner : public Component
{
    URHO3D_OBJECT(GOC_Spawner, Component);

public :
    GOC_Spawner(Context* context);
    virtual ~GOC_Spawner();

    static void RegisterObject(Context* context);

    void SetActived(bool actived);
    void SetObjectTypesAttr(const String& typesStr);

    bool GetActived() const
    {
        return actived_;
    }
    String GetObjectTypesAttr() const;

    void SpawnObjects();

    void Dump();

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void UpdateAttributes();
    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    bool actived_;
    int maxLivingSpawnables_;
    Vector2 spawnInterval_;
    float spawnDelay_, delay_;
    Vector<StringHash> spawnableObjectTypes_;
    Vector<WeakPtr<Node> > spawnables_;
};


