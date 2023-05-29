#pragma once

#define QTY_MAX 1000000U
//#define QTY_MAX M_MAX_UNSIGNED

namespace Urho3D
{
class Timer;
class Sprite2D;
}

using namespace Urho3D;


struct SlotEntityInfo
{
    SlotEntityInfo()
    {
        Reset();
    }
    void Reset()
    {
        memset(this, 0, sizeof(SlotEntityInfo));
    }

    StringHash type_;
    unsigned int quantity_;
    int effect_;

    WeakPtr<Sprite2D> sprite_;
    Vector3 scale_;
    Color color_;

    static const SlotEntityInfo EMPTY;
};

struct Slot
{
    Slot() : partfromtype_(StringHash::ZERO), type_(StringHash::ZERO), quantity_(0U), effect_(-1), scale_(Vector3::ONE), color_(Color::WHITE) { }
    Slot(const StringHash& t, unsigned int q) : partfromtype_(StringHash::ZERO), type_(t), quantity_(q), effect_(-1) { }
    Slot(const Slot& slot) :
        partfromtype_(slot.partfromtype_),
        type_(slot.type_),
        quantity_(slot.quantity_),
        effect_(slot.effect_),
        sprite_(slot.sprite_),
        uisprite_(slot.sprite_),
        scale_(slot.scale_),
        color_(slot.color_) { }

    bool Empty() const
    {
        return quantity_ == 0L;
    }

    void Clear();

    void Set(StringHash type);
    void Set(const StringHash& type, unsigned int quantity, const StringHash& fromtype = StringHash::ZERO, const ResourceRef& spriteRef = Variant::emptyResourceRef);
    void SetAttributes(const StringHash& type, unsigned int quantity, const StringHash& fromtype, Sprite2D* sprite, const Vector3& scale, const Color& color);
    void SetAttributesFrom(const Slot& slot);
    void SetSprite(Sprite2D* sprite, const Vector3& scale = Vector3::ONE, const Color& color = Color::WHITE);
    void UpdateUISprite();

    unsigned int TransferTo(Slot* slot, unsigned int qtyToTransfer, unsigned int freeSpace=QTY_MAX);

    void Dump() const;

    static bool SetSlotAttr(const String& value, Slot& slot);
    static String GetSlotAttr(const Slot& slot);
    static void GetSlotData(const Slot& slot, VariantMap& slotData, unsigned qty);
    static bool HaveSameTypes(const Slot& slot1, const Slot& slot2)
    {
        return slot1.type_ == slot2.type_ && slot1.partfromtype_ == slot2.partfromtype_;
    }

    StringHash partfromtype_;

    StringHash type_;
    unsigned int quantity_;
    int effect_;

    WeakPtr<Sprite2D> sprite_;
    WeakPtr<Sprite2D> uisprite_;
    Vector3 scale_;
    Color color_;
};
