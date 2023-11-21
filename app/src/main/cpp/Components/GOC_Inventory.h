#pragma once

#include <climits>

#include <Urho3D/Scene/Component.h>

#include "Slot.h"

namespace Urho3D
{
class AnimatedSprite2D;
}


using namespace Urho3D;


struct GOC_Inventory_Template
{
    GOC_Inventory_Template() { ; }
    GOC_Inventory_Template(const String& n, const GOC_Inventory_Template& c)
        :  name(n),
           hashName(StringHash(n)),
           baseTemplate(c.hashName),
           slotEquipmentMaxIndex_(0),
           slotHashNames_(c.slotHashNames_),
           slotNames_(c.slotNames_),
           slotCapacityByCategories_(c.slotCapacityByCategories_) { ; }

    static void RegisterTemplate(const String& s, const GOC_Inventory_Template& t);
    static void DumpAll();
    static void DumpBoneKeywords();
    static const HashMap<StringHash, Vector<String> >& GetBoneKeywordsBySlot()
    {
        return boneKeywordsBySlot_;
    }
    static const GOC_Inventory_Template* Get(unsigned templateHashValue)
    {
        HashMap<unsigned, GOC_Inventory_Template>::ConstIterator it = templates_.Find(templateHashValue);
        return it != templates_.End() ? &it->second_ : 0;
    }
    static const String& GetTemplateName(unsigned templateHashValue)
    {
        HashMap<unsigned, GOC_Inventory_Template>::ConstIterator it = templates_.Find(templateHashValue);
        return it != templates_.End() ? it->second_.name : String::EMPTY;
    }
    static const StringHash& GetSlotHashName(unsigned templateHashValue, unsigned index)
    {
        HashMap<unsigned, GOC_Inventory_Template>::ConstIterator it = templates_.Find(templateHashValue);
        return it != templates_.End() ? (index < it->second_.slotHashNames_.Size() ? it->second_.slotHashNames_[index] : StringHash::ZERO) : StringHash::ZERO;
    }
    static const String& GetSlotName(unsigned templateHashValue, unsigned index)
    {
        HashMap<unsigned, GOC_Inventory_Template>::ConstIterator it = templates_.Find(templateHashValue);
        return it != templates_.End() ? (index < it->second_.slotNames_.Size() ? it->second_.slotNames_[index] : String::EMPTY) : String::EMPTY;
    }

    void Dump() const;

    void AddSlot(const String& slotName, const String& acceptedCategories, unsigned int capacity, const String& boneKeys=String::EMPTY);

    const StringHash& GetHashName() const
    {
        return hashName;
    }
    unsigned GetNumSlots() const
    {
        return slotHashNames_.Size();
    }
    bool CanSlotCollectCategory(unsigned index, const StringHash& type) const;
    unsigned int GetSlotCapacityFor(unsigned index, const StringHash& type, bool strictmode=false) const;

    void GetAcceptedSlotsFor(const StringHash& type, HashMap<unsigned, StringHash>& acceptedSlots) const;
    const String& GetSlotName(unsigned index) const
    {
        return slotNames_[index];
    }

    const String& GetSectionForSlot(unsigned index) const
    {
        for (HashMap<String, Pair<unsigned, unsigned> >::ConstIterator it=sections_.Begin(); it!=sections_.End(); ++it)
        {
            if (index >= it->second_.first_ && index <= it->second_.second_)
                return it->first_;
        }
        return String::EMPTY;
    }
    Pair<unsigned, unsigned> GetSectionIndexes(const String& section) const
    {
        HashMap<String, Pair<unsigned, unsigned> >::ConstIterator it = sections_.Find(section);
        return it != sections_.End() ? it->second_ : Pair<unsigned,unsigned>(1,0);
    }

    String name;
    StringHash hashName;
    StringHash baseTemplate;
    unsigned slotEquipmentMaxIndex_;

    Vector<StringHash> slotHashNames_;
    Vector<String> slotNames_;
    Vector<HashMap<StringHash, unsigned int> > slotCapacityByCategories_;

    /// Sections : key = name ; value = (startIndex, EndIndex)
    HashMap<String, Pair<unsigned, unsigned> > sections_;

    /// For EquipmentPanel
    static HashMap<StringHash, Vector<String> > boneKeywordsBySlot_;

    static HashMap<unsigned, GOC_Inventory_Template> templates_;
};

class GOC_Inventory : public Component
{
    URHO3D_OBJECT(GOC_Inventory, Component);

public :
    GOC_Inventory(Context* context);
    virtual ~GOC_Inventory();

    static void RegisterObject(Context* context);

    void RegisterTemplate(const String& s);
    void SetTemplate(const String& s);
    void ResetTemplate(GOC_Inventory_Template* t=0);
    void SetTemplateSlotAttr(const String& value);
    void SetTemplateSectionAttr(const String& value);
    void AddToSlotAttr(const String& value);
    void AddToSlot(const String& value);
    void SetInventoryAttr(const VariantVector& value = Variant::emptyVariantVector);

    void SetReceiveTriggerEvent(const String& s);
    void SetGiveTriggerEvent(const String& s);

    int AddCollectableOfType(const StringHash& type, unsigned int quantity, unsigned startIndex=0);
    int AddCollectableFromSlot(const Slot& slotSrc, unsigned int& quantity, unsigned startIndex=0);
    void AddCollectableFromSlot(const Slot& slotSrc, unsigned idSlot, unsigned int& quantity, bool strictmode, bool recursive=true, unsigned startIndex=0);

    int RemoveCollectableOfType(const StringHash& type, unsigned int quantity, unsigned startIndex=0);
    int RemoveCollectableFromSlot(const Slot& slotSrc, unsigned int& quantity, unsigned startIndex=0);
    void RemoveCollectableFromSlot(unsigned idSlot, unsigned int quantity=QTY_MAX);

    void TransferSlotTo(unsigned idSlot, Node* node, unsigned int quantity=QTY_MAX, const String& section=String::EMPTY);
    void TransferSlotTo(unsigned idSlot, GOC_Inventory* toInventory, unsigned int quantity=QTY_MAX, const String& section=String::EMPTY);
    void TransferAllSlotsTo(Node *node);

    void Set(const Vector<Slot>& inventory);
    void ResetSlots();

    void Dump() const;

    bool HasTemplate() const;
    GOC_Inventory_Template* GetTemplate() const
    {
        return currentTemplate;
    }
    GOC_Inventory_Template* GetTemplate(const StringHash& key) const;
    const String& GetTemplateName() const
    {
        return currentTemplate ? currentTemplate->name : String::EMPTY;
    }
    const StringHash& GetTemplateHashName() const
    {
        return currentTemplate ? currentTemplate->hashName : StringHash::ZERO;
    }
    const String& GetEmptyAttr() const
    {
        return String::EMPTY;
    }
    VariantVector GetInventoryAttr() const;
    bool GetSectionSlots(const String& section, VariantVector& set);
    const String& GetReceiveTriggerEvent() const;
    const String& GetGiveTriggerEvent() const;

    unsigned GetNumSlots() const
    {
        return slots_.Size();
    }
    const Vector<Slot>& Get() const
    {
        return slots_;
    }
    const Slot& GetSlot(unsigned id) const
    {
        return slots_[id];
    }
    Slot& GetSlot(unsigned id)
    {
        return slots_[id];
    }
    const StringHash& GetSlotHashName(unsigned i) const
    {
        return currentTemplate->slotHashNames_[i];
    }
    const String& GetSlotName(unsigned i) const;
    int GetSlotIndex(const StringHash& hashname) const;
    int GetSlotIndex(const String& name) const;

    bool Empty() const;
    bool CheckEmpty();
    unsigned int GetQuantityfor(const StringHash& collectableType) const;
    void GetSlotfor(Slot*& slot, const Slot& slotSrc, unsigned int& freeSpace, unsigned* idslot=0, unsigned startIndex=0);
    int GetSlotIndexFor(const StringHash& category, bool freeSlot=false, unsigned startIndex=0);
    unsigned GetSlotEquipmentMaxIndex() const
    {
        return currentTemplate->slotEquipmentMaxIndex_-1;
    }
    unsigned GetSectionStartIndex(const String& section) const
    {
        return currentTemplate->GetSectionIndexes(section).first_;
    }
    unsigned GetSectionEndIndex(const String& section) const
    {
        return currentTemplate->GetSectionIndexes(section).second_;
    }

    virtual void OnSetEnabled();

    static void ClearCache();
    static void RegisterClientNode(Node* node);
    static bool IsNetworkInventoryAvailable(Node* node) { return clientInventories_.Contains(node->GetID()); }
    static bool IsNetworkEquipmentSetAvailable(Node* node) { return clientEquipmentSets_.Contains(node->GetID()); }
    static void LoadInventory(Node* node, bool forceinitialstuff);
    static bool SetEquipmentSlot(AnimatedSprite2D* animatedSprite, unsigned idslot, const String& slotname, StringHash slotType, GOC_Inventory* inventory=0);
    static void LocalEquipSlotOn(GOC_Inventory* inventory, unsigned idslot, AnimatedSprite2D* animatedSprite, bool netSendMessage=false);
    static void NetClientSetEquipmentSlot(Node* node, VariantMap& eventData);
    static void NetClientSetInventory(unsigned nodeid, VariantMap& eventData);
    static void NetClientSetEquipment(unsigned nodeid, VariantMap& eventData);
    static void NetClientSetEquipment(Node* node);
    static void NetServerSetEquipmentSlot(Node* node, VariantMap& eventData);
    static void NetServerRemoveItem(Node* node, VariantMap& eventData);

protected :
    virtual void OnNodeSet(Node* node);

private :
    void ClearCustomTemplate();
    void ApplyTemplateProperties(GOC_Inventory_Template* t);
    void ApplyTemplateToCurrentSlots(bool dropWastedCollectables);

    bool CanSlotAccept(unsigned index, const Slot& slotSrc, unsigned int& freeSpace, bool strictmode=true);

    void OnEnableGive(StringHash eventType, VariantMap& eventData);
    void HandleGive(StringHash eventType, VariantMap& eventData);
    void HandleReceive(StringHash eventType, VariantMap& eventData);
    void HandleDrop(StringHash eventType, VariantMap& eventData);

    GOC_Inventory_Template* currentTemplate;
    bool customTemplate;
    bool autoSlotsPopulate_;

    WeakPtr<Node> nodeGetZone_;
    Vector<Slot> slots_;

    // only template node use this
    VariantVector valuesToPopulate_;

    bool empty_;
    bool enableGive_;
    StringHash enableGiveEvent_, receiveEvent_;

    static String tempSectionName_;
    // for GameNetwork client : saved equipment slots by nodeid;
    static HashMap<unsigned int, Node* > clientNodes_;
    static HashMap<unsigned int, VariantVector > clientInventories_;
    static HashMap<unsigned int, VariantVector > clientEquipmentSets_;
};
