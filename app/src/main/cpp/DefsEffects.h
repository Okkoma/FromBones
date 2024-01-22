#pragma once

namespace Urho3D
{
class Node;
}

using namespace Urho3D;

#include "DefsEntityInfo.h"

class GOC_ZoneEffect;
class MapBase;
struct ZoneData;

class EffectType
{
public :
    EffectType() : id(-1), name(String::EMPTY), effectElt(StringHash()), effectRessource(String::EMPTY), numTicks(1), qtyByTicks(0.f), qtyFactorOutZone(1.f) { }

    EffectType(const StringHash& elt, float qty, const String& n = String::EMPTY, const String& r = String::EMPTY, unsigned numticks=1, float delay=0.f, bool persistent=false, float factor=1.f)
        :  id(-1), name(n), effectElt(elt), effectRessource(r), numTicks(numticks), qtyByTicks(qty), delayBetweenTicks(delay),
           persistentOutZone(persistent), qtyFactorOutZone(factor) { }

    EffectType(const EffectType& e)
        :  id(e.id), name(e.name), effectElt(e.effectElt), effectRessource(e.effectRessource), numTicks(e.numTicks), qtyByTicks(e.qtyByTicks),
           delayBetweenTicks(e.delayBetweenTicks), persistentOutZone(e.persistentOutZone), qtyFactorOutZone(e.qtyFactorOutZone) { }

    void Set(const String& name, const StringHash& elt, float qty, unsigned numticks=1, float delay=0.f, bool persistent=false, float factor=1.f, const String& res = String::EMPTY);

    void Register();

    static void InitTable();

    static unsigned GetNumRegisteredEffects()
    {
        return effecTypes_.Size();
    }
    static const EffectType& Get(unsigned id);
    static EffectType* GetPtr(unsigned id);
    static int GetID(const String& name);
    static void DumpAll();

    void Dump() const;

    int id;
    String name;
    StringHash effectElt;
    String effectRessource;

    int numTicks;
    float qtyByTicks;
    float delayBetweenTicks;
    bool persistentOutZone;
    float qtyFactorOutZone;

    static const EffectType EMPTY;

private :
    static Vector<EffectType> effecTypes_;
};

struct EffectInstance
{
    EffectInstance();
    EffectInstance(GOC_ZoneEffect* zone, Node* node, EffectType* effect, const Vector2& localImpact);
    EffectInstance(const EffectInstance& e);

    static const EffectInstance EMPTY;

    WeakPtr<GOC_ZoneEffect> zone_;
    WeakPtr<Node> node_;
    EffectType* effect_;

    bool firstCount;
    int tickCount;
    float chrono;
    bool activeForHolder;
    bool outZone;

    Vector2 localImpact_;
};

class EffectAction : public Object
{
    URHO3D_OBJECT(EffectAction, Object);

public:
    EffectAction(Context* context) : Object(context) { }
    virtual ~EffectAction() { }

    static void RegisterEffectActionLibrary(Context* context);
    static void Clear();
    static void Clear(const IntVector3& zone);
    static void Update(Node* node, IntVector3& zone, int viewport);
    static EffectAction* Get(const IntVector3& zone);
    static EffectAction* GetOrCreate(unsigned zoneid, unsigned actiontype, MapBase* map);
    static void PurgeFinishedActions();

    void Set(MapBase* map, unsigned zone);
    void AddViewportUser(int viewport, Object* user);
    void RemoveViewportUser(int viewport, Object* user);

    bool IsZoneValid();

    virtual bool Apply();
    virtual void OnFinished();
    virtual void OnEnterZone(int viewport) = 0;
    virtual void OnExitZone(int viewport) = 0;

    void MarkFinished();

    HashMap<int, PODVector<Object*> > viewportUsers_;

protected:

    MapBase* map_;
    IntVector3 zone_;
    ZoneData* zonedata_;

    bool finished_;

private:
    static HashMap<IntVector3, EffectAction* > runningEffectActions_;
};

class BossZone : public EffectAction
{
    URHO3D_OBJECT(BossZone, EffectAction);

public :
    static void RegisterObject(Context* context);

    BossZone(Context* context) : EffectAction(context), time_(0.f) { }
    virtual ~BossZone() { }

    virtual bool Apply();
    virtual void OnFinished();
    virtual void OnEnterZone(int viewport);
    virtual void OnExitZone(int viewport);

    static void SetEnableLifeBar(Node* boss, bool enable, int viewport=-1);

    static void RemoveBoss(const IntVector3& zone, Node* boss);

private :
    void SpawnBoss();
    void SetBossNode();

    static void GetCloseBlocks(MapBase* map, ZoneData* zonedata, Vector<unsigned>& blocks);

    void HandleUpdate(StringHash eventType, VariantMap& eventData);

    float time_;

    WeakPtr<Node> boss_;
};

const unsigned BOSSZONE = BossZone::GetTypeStatic().Value();
