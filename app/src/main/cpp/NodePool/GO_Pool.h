#pragma once


using namespace Urho3D;


class GO_Pool : public Object
{
    URHO3D_OBJECT(GO_Pool, Object);

public :
    GO_Pool() : Object(0) { ; }
    GO_Pool(Context* context) : Object(context) { ; }
    GO_Pool(const GO_Pool& pool);
    virtual ~GO_Pool() { ; }

    static void RegisterObject(Context* context);

    void Init(Node* nodePool, const StringHash& GOT, int clientid);
    void Resize(unsigned size);
    void TagNodes(const String& tag);

    unsigned GetSize() const
    {
        return nodes_.Size();
    }
    unsigned GetFirstID() const
    {
        return nodes_.Front()->GetID();
    }
    unsigned GetLastID() const
    {
        return nodes_.Back()->GetID();
    }

    Node* GetGO();
    Node* UseGO(Node* node);
    void FreeGO(Node* node);

    void Restore();

private :
    StringHash GOT_;
    int clientID_;
    Vector<WeakPtr<Node> > nodes_;
    List<Node* > freenodes_;
    Node* nodePool_;
};

class GO_Pools : public Object
{
    URHO3D_OBJECT(GO_Pools, Object);

public :
    GO_Pools(Context* context) : Object(context) { ; }

    static void Start();

    static void AddPool(const StringHash& GOT, unsigned numGOs, const String& tag=String::EMPTY);
    static GO_Pool* GetOrCreatePool(const StringHash& GOT, const String& tag=String::EMPTY, int clientid=-1);
    static GO_Pool* GetPool(const StringHash& GOT, int clientid=-1);
    static Node* Get(const StringHash& GOT, int clientid=-1);
    static Node* Use(Node* node);
    static void Free(Node* node);

    static void Restore();
    static void Reset();

private :
    void SubscribeToEvents();
    void HandleObjectDestroy(StringHash eventType, VariantMap& eventData);

    static WeakPtr<Node> nodePool_;
    static Vector<StringHash> poolHashs_;
    static Vector<Vector<GO_Pool> > pools_;

    static GO_Pools* gopools_;
};
