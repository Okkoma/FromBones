#pragma once

#include "DefsGame.h"


namespace Urho3D
{
class StaticSprite2D;
}

using namespace Urho3D;


class GOC_Collectable : public Component
{
    URHO3D_OBJECT(GOC_Collectable, Component);

public :
    GOC_Collectable(Context* context);
    virtual ~GOC_Collectable();

    static void RegisterObject(Context* context);

    void SetCatcherAttr(const String& value);
    String GetCatcherAttr() const;

    void AddToSlotAttr(const String& value);

    void SetSlot(const StringHash& type, unsigned int quantity, const StringHash& fromtype);
    void SetSpriteProps(const StaticSprite2D* staticSprite);
    void Set();

    static void TransferSlotTo(Slot& slotToGet, Node* nodeGiver, Node* nodeGetter, const Variant& position, unsigned int quantity=QTY_MAX);
    static Node* DropSlotFrom(Node* owner, Slot* slot, int dropmode=-1, int fromSlotId=0, unsigned int qty=QTY_MAX, VariantMap* slotdata=0);

    bool Empty() const;
    bool CheckEmpty();
    String GetSlotAttr() const;
    const Slot& Get() const
    {
        return slot_;
    }
    Slot& Get()
    {
        return slot_;
    }
    Slot* GetSlot()
    {
        return &slot_;
    }

    virtual void CleanDependences();
    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void Init();

    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);
    void HandleEmpty(StringHash eventType, VariantMap& eventData);
    void HandleContact(StringHash eventType, VariantMap& eventData);

    Slot slot_;
    Vector<StringHash> catchers_;
    bool selfDestroy_, wallCollide_;

    static VariantMap tempSlotData_;
};


