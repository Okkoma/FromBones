#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/StringUtils.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Scene/Scene.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Graphics/Material.h>

#include <Urho3D/Urho2D/CollisionShape2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/PhysicsEvents2D.h>
#include <Urho3D/Urho2D/PhysicsWorld2D.h>
#include <Urho3D/Urho2D/SpriterData2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GOC_Animator2D.h"
#include "GOC_ZoneEffect.h"
#include "GOC_Collectable.h"
#include "GOC_Controller.h"
#include "GOC_ControllerPlayer.h"
#include "GOC_Destroyer.h"
#include "Player.h"
#include "Equipment.h"

#include "GOC_Inventory.h"


HashMap<StringHash, Vector<String> > GOC_Inventory_Template::boneKeywordsBySlot_;
HashMap<unsigned, GOC_Inventory_Template> GOC_Inventory_Template::templates_;

const StringHash INVENTORY_TEMPLATE_EMPTY    = StringHash("InventoryTemplate_Empty");

String GOC_Inventory::tempSectionName_;

/// GOC_Inventory_Template

void GOC_Inventory_Template::RegisterTemplate(const String& s, const GOC_Inventory_Template& t)
{
    URHO3D_LOGINFOF("GOC_Inventory_Template() - RegisterTemplate %s ...",s.CString());

    unsigned key = StringHash(s).Value();

    if (templates_.Contains(key)) return;

    templates_ += Pair<unsigned, GOC_Inventory_Template>(key, t);
    templates_[key].name = s;
    templates_[key].hashName = StringHash(key);

    URHO3D_LOGINFOF("GOC_Inventory_Template() - RegisterTemplate ... templates_ size=%u !", templates_.Size());
}

void GOC_Inventory_Template::DumpAll()
{
    unsigned index = 0;
    for (HashMap<unsigned, GOC_Inventory_Template>::ConstIterator it=templates_.Begin(); it!=templates_.End(); ++it, ++index)
    {
        URHO3D_LOGINFOF("GOC_Inventory_Template() - DumpAll : templates_[%u]", index);
        it->second_.Dump();
    }
}

void GOC_Inventory_Template::DumpBoneKeywords()
{
//    URHO3D_LOGINFOF("GOC_Inventory_Template() - DumpBoneKeywords : numKeywords=%u", boneKeywordsBySlot_.Size());

    for (HashMap<StringHash, Vector<String> >::ConstIterator it=boneKeywordsBySlot_.Begin(); it!=boneKeywordsBySlot_.End(); ++it)
    {
        String keywords;
        for (Vector<String>::ConstIterator it2=it->second_.Begin(); it2!=it->second_.End(); ++it2)
            keywords += (*it2);
//        URHO3D_LOGINFOF("  => keywords[%u] = name:%s", it->first_.Value(), keywords.CString());
    }
}

void GOC_Inventory_Template::Dump() const
{
    URHO3D_LOGINFOF("GOC_Inventory_Template() - Dump : name=%s hash=%u base=%u - numSlots=%u",
                    name.CString(), hashName.Value(), baseTemplate.Value(), GetNumSlots());

    for (unsigned i=0; i<GetNumSlots(); ++i)
    {
        URHO3D_LOGINFOF("  => slot[%u] = name:%s (hash:%u)", i, slotNames_[i].CString(), slotHashNames_[i].Value());
        const HashMap<StringHash, unsigned int> categoriesQt = slotCapacityByCategories_[i];
        for (HashMap<StringHash, unsigned int>::ConstIterator it=categoriesQt.Begin(); it!=categoriesQt.End(); ++it)
        {
            URHO3D_LOGINFOF("    => Category = %s(%u) - Capacity = %u", COT::GetName(it->first_).CString(), it->first_.Value(), it->second_);
        }
    }
}

void GOC_Inventory_Template::AddSlot(const String& slotName, const String& acceptedCategories, unsigned int capacity, const String& boneKeys)
{
    if (acceptedCategories.Empty())
        return;

//    URHO3D_LOGINFOF("GOC_Inventory_Template() - AddSlot (%s,%s,%d)", slotName.CString(), acceptedCategories.CString(), capacity);

    slotNames_.Push(slotName);
    StringHash hashName(slotName);
    slotHashNames_.Push(hashName);

    /// Set range for Equipment slots from index 1 to slotEquipmentMaxIndex : 0 for money and the remain for the bag Slots
    /// Slot with no Name = bag Slot
    if (slotName != String::EMPTY)
        slotEquipmentMaxIndex_++;

    HashMap<StringHash, unsigned int> capacityByCategory;
    capacityByCategory.Clear();
    Vector<String> vCategories = acceptedCategories.Split(';');
    for (unsigned i=0; i < vCategories.Size(); ++i)
    {
        String CategoryStr = vCategories[i].Trimmed();
        if (CategoryStr == "ALL")
        {
//            URHO3D_LOGINFOF("GOC_Inventory_Template() - Add to slot Collectable_All with capacity=%u", capacity);
            capacityByCategory += Pair<StringHash, unsigned int>(COT::ITEMS,capacity);
            break;
        }
        else
        {
//            URHO3D_LOGINFOF("GOC_Inventory_Template() - Add to slot %s with capacity=%u", CategoryStr.CString(), capacity);
            capacityByCategory += Pair<StringHash, unsigned int>(StringHash(CategoryStr), capacity);
        }
    }
//    URHO3D_LOGINFOF("GOC_Inventory_Template() - push %s in slotCapacityByCategories_",acceptedCategories.CString());
    slotCapacityByCategories_.Push(capacityByCategory);

    if (boneKeys != String::EMPTY)
    {
        Vector<String>& boneKeywords = boneKeywordsBySlot_[hashName];
        Vector<String> vboneKeys = boneKeys.Split(';');
        for (unsigned i=0; i<vboneKeys.Size(); i++)
        {
            if (!boneKeywords.Contains(vboneKeys[i]))
                boneKeywords.Push(vboneKeys[i]);
        }
    }
}

bool GOC_Inventory_Template::CanSlotCollectCategory(unsigned index, const StringHash& type) const
{
    const HashMap<StringHash, unsigned int>& categoryQ = slotCapacityByCategories_[index];
    HashMap<StringHash, unsigned int>::ConstIterator it = categoryQ.Find(GOT::GetCategory(type));

    if (it != categoryQ.End())
    {
        return true;
    }
    else
    {
        it = categoryQ.Find(COT::ITEMS);
        if (it != categoryQ.End())
            return true;
    }
    return false;
}

unsigned int GOC_Inventory_Template::GetSlotCapacityFor(unsigned index, const StringHash& type, bool strictmode) const
{
    if (index >= GetNumSlots()) return 0;

    const HashMap<StringHash, unsigned int>& categoryQ = slotCapacityByCategories_[index];
    HashMap<StringHash, unsigned int>::ConstIterator it = categoryQ.Find(GOT::GetCategory(type));

    if (it != categoryQ.End())
        return it->second_;

    if (!strictmode)
    {
        it = categoryQ.Find(COT::ITEMS);
        if (it != categoryQ.End())
            return it->second_;
    }

    return 0;
}

void GOC_Inventory_Template::GetAcceptedSlotsFor(const StringHash& type, HashMap<unsigned, StringHash>& acceptedSlots) const
{
    for (unsigned i = 0; i < slotCapacityByCategories_.Size(); i++)
    {
        const HashMap<StringHash, unsigned int>& categoryQ = slotCapacityByCategories_[i];
        HashMap<StringHash, unsigned int>::ConstIterator it = categoryQ.Find(GOT::GetCategory(type));

        if (it != categoryQ.End() && !acceptedSlots.Contains(i))
            acceptedSlots[i] = type;
    }
}


/// GOC_Inventory

void GOC_Inventory::RegisterObject(Context* context)
{
    context->RegisterFactory<GOC_Inventory>();

    URHO3D_ACCESSOR_ATTRIBUTE("Register Template", GetEmptyAttr, RegisterTemplate, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Slot", GetEmptyAttr, SetTemplateSlotAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Section", GetEmptyAttr, SetTemplateSectionAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Template", GetTemplateName, SetTemplate, String, String::EMPTY, AM_FILE);
    URHO3D_MIXED_ACCESSOR_ATTRIBUTE("Inventory", GetInventoryAttr, SetInventoryAttr, VariantVector, Variant::emptyVariantVector, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Add To Slot", GetEmptyAttr, AddToSlotAttr, String, String::EMPTY, AM_FILE);
    URHO3D_ATTRIBUTE("Enable Give", bool, enableGive_, false, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Give Trigger Event", GetGiveTriggerEvent, SetGiveTriggerEvent, String, String::EMPTY, AM_FILE);
    URHO3D_ACCESSOR_ATTRIBUTE("Receive Trigger Event", GetReceiveTriggerEvent, SetReceiveTriggerEvent, String, String::EMPTY, AM_FILE);

    GOC_Inventory_Template::templates_.Clear();

    GOC_Inventory_Template defaultProps = GOC_Inventory_Template();
    defaultProps.baseTemplate = 0;

    GOC_Inventory_Template::RegisterTemplate("InventoryTemplate_Empty", defaultProps);
}

GOC_Inventory::GOC_Inventory(Context* context) :
    Component(context),
    customTemplate(false),
    currentTemplate(0),
    enableGive_(false),
    empty_(true),
    enableGiveEvent_(StringHash::ZERO),
    receiveEvent_(StringHash::ZERO)
{ }

GOC_Inventory::~GOC_Inventory()
{
    UnsubscribeFromAllEvents();
    ClearCustomTemplate();
}

void GOC_Inventory::RegisterTemplate(const String& s)
{
    if (s.Empty())
        return;

    URHO3D_LOGINFOF("GOC_Inventory() - RegisterTemplate : %s(%u)", s.CString(), StringHash(s).Value());

    GOC_Inventory_Template::RegisterTemplate(s, *currentTemplate);
}

bool GOC_Inventory::HasTemplate() const
{
    if (!currentTemplate)
        return false;

    return currentTemplate->hashName != INVENTORY_TEMPLATE_EMPTY;
}

GOC_Inventory_Template* GOC_Inventory::GetTemplate(const StringHash& key) const
{
    return GOC_Inventory_Template::templates_.Contains(key.Value()) ? &(GOC_Inventory_Template::templates_.Find(key.Value())->second_) : 0;
}

void GOC_Inventory::SetTemplate(const String& s)
{
    StringHash key = StringHash(s);
    GOC_Inventory_Template* t = GetTemplate(key);

//    URHO3D_LOGINFOF("GOC_Inventory() - SetTemplate : %s(%u) newTemplatePtr=%u currentTemplatePtr=%u", s.CString(), key.Value(), t, currentTemplate);

    if (t != currentTemplate || !currentTemplate)
        ApplyTemplateProperties(t);
}

void GOC_Inventory::ResetTemplate(GOC_Inventory_Template* t)
{
    currentTemplate = t;

//    URHO3D_LOGINFOF("GOC_Inventory() - ResetTemplate : templatePtr=%u", currentTemplate);

    if (currentTemplate)
        ApplyTemplateProperties(currentTemplate);
}

void GOC_Inventory::ApplyTemplateProperties(GOC_Inventory_Template* t)
{
    ClearCustomTemplate();

    if (t == 0)
    {
//        URHO3D_LOGINFO("GOC_Inventory() - ApplyTemplateProperties : template=0 => apply default template !");
        t = &(GOC_Inventory_Template::templates_.Find(INVENTORY_TEMPLATE_EMPTY)->second_);
    }

//    URHO3D_LOGINFOF("GOC_Inventory() - ApplyTemplateProperties : template=%s(%u) !", t->name.CString(), t);

    currentTemplate = t;
    ApplyTemplateToCurrentSlots(false);
}

void GOC_Inventory::ApplyTemplateToCurrentSlots(bool dropWastedCollectables)
{
    Vector<Slot> temporarySlots;

    unsigned numslots = currentTemplate->GetNumSlots();

    if (slots_.Size() > numslots)
    {
        // put waste slots in temporary
        for (unsigned i = numslots; i < slots_.Size(); ++i)
        {
            if (slots_[i].type_.Value() != 0)
                temporarySlots.Push(slots_[i]);
        }
    }

    // apply the template number of slots
    slots_.Resize(numslots);

    if (temporarySlots.Size() != 0)
    {
        // put collectables compatible with the new current template in temporary slots
        for (unsigned i=0; i<numslots; ++i)
        {
            if (slots_[i].type_.Value() == 0 || slots_[i].quantity_ == 0)
            {
                slots_[i].Clear();
                continue;
            }
            if (!currentTemplate->CanSlotCollectCategory(i, slots_[i].type_))
            {
                temporarySlots.Push(slots_[i]);
                slots_[i] = Slot();
            }
        }

        // reinject temporary slots in the inventory
        const unsigned size = temporarySlots.Size();
        PODVector<bool> newpass;
        newpass.Resize(size);
        unsigned int remainCapacity;
        Slot* slot;
        for (unsigned i=0; i<size; ++i)
        {
            GetSlotfor(slot, temporarySlots[i], remainCapacity);
            if (slot)
            {
                newpass[i] = remainCapacity < temporarySlots[i].quantity_;
                remainCapacity = temporarySlots[i].TransferTo(slot, temporarySlots[i].quantity_, remainCapacity);
            }
            else
                newpass[i] = false;
        }
        // new passes if necessary
        for (unsigned i=0; i<size; ++i)
        {
            while (newpass[i])
            {
                GetSlotfor(slot, temporarySlots[i], remainCapacity);
                if (slot)
                {
                    newpass[i] = remainCapacity < temporarySlots[i].quantity_;
                    remainCapacity = temporarySlots[i].TransferTo(slot, temporarySlots[i].quantity_, remainCapacity);
                }
                else
                    newpass[i] = false;
            }
        }
        // drop the waste temporary slots
        if (dropWastedCollectables && node_)
        {
//            URHO3D_LOGINFOF("GOC_Inventory() - ApplyTemplateToCurrentSlots :");
            for (unsigned i=0; i<size; ++i)
            {
                if (temporarySlots[i].quantity_ > 0)
                    GOC_Collectable::DropSlotFrom(node_, temporarySlots[i]);
            }
        }
    }
}

void GOC_Inventory::ClearCustomTemplate()
{
    if (!customTemplate)
        return;

    if (currentTemplate)
    {
//        URHO3D_LOGINFOF("GOC_Inventory() - ClearCustomTemplate : Delete Template Name=%s", currentTemplate->name.CString());
        delete currentTemplate;
        currentTemplate = 0;
    }

    customTemplate = false;
}

void GOC_Inventory::SetTemplateSlotAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_Inventory() - SetTemplateSlotAttr (%s)",value.CString());

    if (value.Empty()) return;

    unsigned numslots = 1;
    unsigned int capacity = 1;
//    String capacityStr = String::EMPTY;
//    String acceptedCategories = String::EMPTY;
//    String numberStr = String::EMPTY;
//    String nameSlot = String::EMPTY;
//    String keywords = String::EMPTY;
    String capacityStr, acceptedCategories, numberStr, nameSlot, keywords;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0) return;
    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0) continue;

        String str = s[0].Trimmed();
        if (str == "name")
            nameSlot = s[1].Trimmed();
        else if (str == "capacity")
            capacityStr = s[1].Trimmed();
        else if (str == "number")
            numberStr = s[1].Trimmed();
        else if (str == "acceptedCategories")
            acceptedCategories = s[1].Trimmed();
        else if (str == "bonekeys")
            keywords = s[1].Trimmed();
    }

    if (capacityStr.Empty() && acceptedCategories.Empty())
    {
        URHO3D_LOGERRORF("GOC_Inventory() : SetTemplateSlotAttr NOK! capacity=%s | acceptedCategories=%s", capacityStr.CString(), acceptedCategories.CString());
        return;
    }

    if (!customTemplate)
    {
//        URHO3D_LOGINFO("GOC_Inventory() - SetTemplateSlotAttr : create new custom Template (don't forget to register it) !");
        GOC_Inventory_Template* newTemplate = new GOC_Inventory_Template("InventoryTemplate_Custom", *currentTemplate);
        customTemplate = true;
        currentTemplate = newTemplate;
    }

    numslots = (numberStr.Empty() ? 1 : ToUInt(numberStr));
    capacity = (capacityStr == "INFINITE" ? QTY_MAX : ToUInt(capacityStr));

    for (unsigned i=0; i<numslots; ++i)
    {
        currentTemplate->AddSlot(nameSlot, acceptedCategories, capacity, keywords);
    }

//    URHO3D_LOGINFOF("GOC_Inventory() - SetTemplateSlotAttr OK!");
}

void GOC_Inventory::SetTemplateSectionAttr(const String& value)
{
//    URHO3D_LOGINFOF("GOC_Inventory() - SetTemplateSectionAttr (%s)",value.CString());

    if (value == "") return;

    unsigned start = 0;
    unsigned end = 0;

    String nameStr, startStr;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0) return;
    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() == 0) continue;

        String str = s[0].Trimmed();
        if (str == "name")
            nameStr = s[1].Trimmed();
        else if (str == "start")
            startStr = s[1].Trimmed();
    }

    if (!customTemplate)
    {
//        URHO3D_LOGINFO("GOC_Inventory() - SetTemplateSectionAttr : create new custom Template (dont't forget to register it) !");
        GOC_Inventory_Template* newTemplate = new GOC_Inventory_Template("InventoryTemplate_Custom",*currentTemplate);
        customTemplate = true;
        currentTemplate = newTemplate;
    }

    if (!startStr.Empty() && currentTemplate->slotNames_.Size())
    {
        start = ToUInt(startStr);
        end = currentTemplate->slotNames_.Size()-1;
    }

    currentTemplate->sections_[nameStr] = Pair<unsigned,unsigned>(start, end);

//    URHO3D_LOGINFOF("GOC_Inventory() - SetTemplateSlotAttr OK!");
}

// only template node use this
void GOC_Inventory::AddToSlotAttr(const String& value)
{
    if (value.Empty())
        return;

//    GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, true);

//    URHO3D_LOGERRORF("GOC_Inventory() - AddToSlotAttr : Node=%s(%u) value=%s", node_->GetName().CString(), node_->GetID(), value.CString());

    valuesToPopulate_.Push(value);

//    Dump();

//    GAME_SETGAMELOGENABLE(GAMELOG_PRELOAD, false);
}

void GOC_Inventory::AddToSlot(const String& value)
{
    if (value.Empty())
        return;

//    URHO3D_LOGINFOF("GOC_Inventory() - AddToSlot : Node=%s(%u) value=%s", node_->GetName().CString(), node_->GetID(), value.CString());

    Slot slot;

    if (Slot::SetSlotAttr(value, slot))
    {
        AddCollectableFromSlot(slot, slot.quantity_);
//        URHO3D_LOGINFOF("GOC_Inventory() - AddToSlot OK !");
    }
    else
        URHO3D_LOGERRORF("GOC_Inventory() - AddToSlot : Node=%s(%u) ... NOK !", node_->GetName().CString(), node_->GetID());
}

void GOC_Inventory::ResetSlots()
{
//    URHO3D_LOGINFOF("GOC_Inventory() - ResetSlots :  Node=%s(%u) !", node_->GetName().CString(), node_->GetID());

    for (unsigned i=0; i<slots_.Size(); ++i)
        slots_[i].Clear();

    empty_ = true;
}


bool GOC_Inventory::Empty() const
{
    return empty_;

//    for (unsigned i=0;i<slots_.Size();++i)
//    {
//        if (slots_[i].quantity_ != 0)
//            return false;
//    }
//    return true;
}

bool GOC_Inventory::CheckEmpty()
{
    // Check for Quantities
    {
        unsigned i = 0;
        empty_ = true;
        while (empty_ && i < slots_.Size())
        {
            if (slots_[i].quantity_ != 0)
                empty_ = false;
            i++;
        }
    }

    if (empty_)
    {
        UnsubscribeFromEvent(node_, enableGiveEvent_);
//        URHO3D_LOGINFOF("GOC_Inventory() - CheckEmpty : %u is Empty !", GetID());
        return true;
    }

//    URHO3D_LOGINFOF("GOC_Inventory() - CheckEmpty : %u is Not Empty !", GetID());
    return false;
}

void GOC_Inventory::SetInventoryAttr(const VariantVector& value)
{
    if (!currentTemplate)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - SetInventoryAttr ... nocurrenttemplate empty=%s ... OK !", empty_ ? "true" : "false");
        return;
    }

    ResetSlots();

//    URHO3D_LOGINFOF("GOC_Inventory() - SetInventoryAttr : Node=%s(%u) empty=%s values.Size=%u",
//                        node_->GetName().CString(), node_->GetID(), empty_ ? "true" : "false", value.Size());

    if (value.Size())
    {
        for (unsigned i=0; i < value.Size(); ++i)
            AddToSlot(value[i].GetString());

        CheckEmpty();

//        Dump();
    }
//    else
//    {
//        URHO3D_LOGINFOF("GOC_Inventory() - SetInventoryAttr : Node=%s(%u) empty=%s valuesToPopulate.Size=%u",
//                        node_->GetName().CString(), node_->GetID(), empty_ ? "true" : "false", value.Size());
//
//        for (unsigned i=0; i < valuesToPopulate_.Size(); ++i)
//            AddToSlot(valuesToPopulate_[i].GetString());
//    }

//    URHO3D_LOGINFOF("GOC_Inventory() - SetInventoryAttr ... empty=%s ... OK !", empty_ ? "true" : "false");
}

VariantVector GOC_Inventory::GetInventoryAttr() const
{
    if (valuesToPopulate_.Size())
        return valuesToPopulate_;

    VariantVector value;

    if (slots_.Size())
    {
        value.Reserve(slots_.Size());
        for (unsigned i=0; i < slots_.Size(); ++i)
        {
            const Slot& slot = slots_[i];

            if (!slot.quantity_)
                continue;

//            String typeStr = GOT::GetType(slot.type_);
            //if (!typeStr.Contains("Collectable_")) continue;

            value.Push(Slot::GetSlotAttr(slot));
        }
    }

    return value;
}


bool GOC_Inventory::GetSectionSlots(const String& section, VariantVector& set)
{
    if (slots_.Size())
    {
        Pair<unsigned, unsigned> sectionIndexes = currentTemplate->GetSectionIndexes(section);
        if (sectionIndexes.first_ <= sectionIndexes.second_)
        {
            set.Clear();
            for (unsigned i = sectionIndexes.first_; i <= sectionIndexes.second_; i++)
                set.Push(Variant(slots_[i].type_.Value()));

            return true;
        }
    }

    return false;
}

void GOC_Inventory::SetGiveTriggerEvent(const String& s)
{
    enableGiveEvent_ = StringHash(s);
}

const String& GOC_Inventory::GetGiveTriggerEvent() const
{
    return GOE::GetEventName(enableGiveEvent_);
}

void GOC_Inventory::SetReceiveTriggerEvent(const String& s)
{
    receiveEvent_ = StringHash(s);
}

const String& GOC_Inventory::GetReceiveTriggerEvent() const
{
    return GOE::GetEventName(receiveEvent_);
}

const String& GOC_Inventory::GetSlotName(unsigned i) const
{
    if (i < currentTemplate->slotNames_.Size())
        return currentTemplate->slotNames_[i];
    else
    {
        tempSectionName_ = currentTemplate->GetSectionForSlot(i);
        tempSectionName_.AppendWithFormat("_%u", i - currentTemplate->sections_[tempSectionName_].first_ + 1);
        return tempSectionName_;
    }
}

int GOC_Inventory::GetSlotIndex(const StringHash& hashname) const
{
    Vector<StringHash>::ConstIterator it = currentTemplate->slotHashNames_.Find(hashname);
    return it!=currentTemplate->slotHashNames_.End() ? it - currentTemplate->slotHashNames_.Begin() : -1;
}

int GOC_Inventory::GetSlotIndex(const String& name) const
{
    if (name.Empty())
        return -1;

    Vector<String> splitnames = name.Split('_');
    if (splitnames.Size() == 1)
    {
        Vector<String>::ConstIterator it = currentTemplate->slotNames_.Find(name);
        return it!=currentTemplate->slotNames_.End() ? it - currentTemplate->slotNames_.Begin() : -1;
    }
    else
    {
        return currentTemplate->sections_.Contains(splitnames[0]) ? currentTemplate->sections_[splitnames[0]].first_ + ToUInt(splitnames[1]) - 1 : -1;
    }
}

unsigned int GOC_Inventory::GetQuantityfor(const StringHash& collectableType) const
{
    unsigned int quantity = 0U;

    for (unsigned i=0; i<slots_.Size(); ++i)
    {
        if (slots_[i].type_ == collectableType)
        {
            quantity += slots_[i].quantity_;
        }
    }
    return quantity;
}

// SLOT Version of GetSlotfor, AddCollectable & AddCollectableToSlot
// consider sprite into the GetSlotfor and CanAccept test

void GOC_Inventory::GetSlotfor(Slot*& slot, const Slot& slotSrc, unsigned int& freeSpace, unsigned* idSlot, unsigned startIndex)
{
    assert(currentTemplate);

    StringHash collectableType = slotSrc.type_;
    const WeakPtr<Sprite2D>& sprite = slotSrc.sprite_;

    for (unsigned i=startIndex; i<slots_.Size(); ++i)
    {
        Slot& s = slots_[i];

//        URHO3D_LOGINFOF("GETSLOT index %u Qt%d sprite=%u ssprite=%u", i, s.quantity_, s.sprite_.Get(), sprite.Get());

        if (!s.quantity_ || (s.type_ == collectableType && (s.sprite_ == sprite || !sprite || !s.sprite_)))
        {
            // Find Capacity for this slot
            unsigned int capacity = currentTemplate->GetSlotCapacityFor(i, collectableType, false);

//            URHO3D_LOGINFOF("GOC_Inventory() - GetSlotfor : Capacity = %u", capacity);

            if (s.quantity_ < capacity)
            {
                freeSpace = capacity - s.quantity_;
                slot = &s;

//                URHO3D_LOGINFOF("GOC_Inventory() - GetSlotfor : Find slot index=%u freeSpace=%u", i, freeSpace);

                if (idSlot)
                    *idSlot = i;

                return;
            }
        }
    }

//    URHO3D_LOGWARNINGF("GOC_Inventory() - GetSlotfor : %s(%u) - No more free slot for %s(%u) remain=%u startindex=%u",
//                       node_->GetName().CString(), node_->GetID(), GOT::GetType(collectableType).CString(), collectableType.Value(), freeSpace, startIndex);
    slot = 0;
}

/// return -1 if not found
int GOC_Inventory::GetSlotIndexFor(const StringHash& type, bool freeSlot, unsigned startIndex)
{
    assert(currentTemplate);

    // if need a free slot, try this
    if (freeSlot)
    {
        for (unsigned i=startIndex; i<slots_.Size(); ++i)
        {
            if (slots_[i].Empty() && currentTemplate->CanSlotCollectCategory(i, type))
                return i;
        }
    }
    else
    {
        // by default return the first compatible slot
        for (unsigned i=startIndex; i<slots_.Size(); ++i)
        {
            if (slots_[i].type_ == type || (slots_[i].Empty() && currentTemplate->CanSlotCollectCategory(i, type)))
                return i;
        }
    }

    return -1;
}

bool GOC_Inventory::CanSlotAccept(unsigned index, const Slot& slotSrc, unsigned int& freeSpace, bool strictmode)
{
    const StringHash& collectableType = slotSrc.type_;
    const WeakPtr<Sprite2D>& sprite = slotSrc.sprite_;
    const Slot& s = slots_[index];

    //if (!s.quantity_ || (s.type_ == collectableType && (s.sprite_ == sprite || !sprite || !s.sprite_)))
    if (!s.quantity_ || s.sprite_ == sprite || !sprite || !s.sprite_)
    {
        // Find Capacity for this slot
        unsigned int capacity = currentTemplate->GetSlotCapacityFor(index, collectableType, s.quantity_ == 0 && strictmode);

        if (s.quantity_ < capacity)
        {
            freeSpace = capacity < s.quantity_ ? 0 : capacity - s.quantity_;
            return (freeSpace > 0);
        }
    }
    return false;
}

/// return the last slot id if freeslot
/// or return -1
int GOC_Inventory::AddCollectableOfType(const StringHash& type, unsigned int quantity, unsigned startIndex)
{
    return AddCollectableFromSlot(Slot(type, quantity), quantity, startIndex);
}

int GOC_Inventory::AddCollectableFromSlot(const Slot& slotSrc, unsigned int& quantity, unsigned startIndex)
{
//    URHO3D_LOGINFOF("GOC_Inventory() - AddCollectableFromSlot : Node=%s(%u) type=%s(%u), qty=%d",
//                     node_->GetName().CString(), node_->GetID(), GOT::GetType(slotSrc.type_).CString(), slotSrc.type_.Value(), slotSrc.quantity_);

    unsigned int remainCapacity;
    unsigned int remainQuantity = quantity;
    Slot* slot = 0;

    GetSlotfor(slot, slotSrc, remainCapacity, &startIndex, startIndex);

    if (!slot)
        return -1;

    while (slot && remainQuantity > 0)
    {
        slot->SetAttributesFrom(slotSrc);

        if (remainCapacity - remainQuantity < 0)
        {
            slot->quantity_ += remainCapacity;
            remainQuantity = remainQuantity-remainCapacity < 0 ? 0 : remainQuantity-remainCapacity;
        }
        else
        {
            slot->quantity_ += remainQuantity;
            remainQuantity = 0;
        }

        if (remainQuantity)
            GetSlotfor(slot, slotSrc, remainCapacity, &startIndex, startIndex);
    }

    empty_ = false;

    // return the waste quantity
    quantity = remainQuantity;

//    URHO3D_LOGINFOF("GOC_Inventory() - AddCollectableFromSlot : OK, remainQuantity=%u", quantity);
//    Dump();

    return startIndex;
}

void GOC_Inventory::AddCollectableFromSlot(const Slot& slotSrc, unsigned idSlot, unsigned int& quantity, bool strictmode, bool recursive, unsigned startIndex)
{
//    URHO3D_LOGINFOF("GOC_Inventory() - AddCollectableFromSlot");
    if (idSlot >= slots_.Size())
        return;

    unsigned int remainCapacity;
    unsigned int remainQuantity = Min(quantity, slotSrc.quantity_);

    if (CanSlotAccept(idSlot, slotSrc, remainCapacity, strictmode))
    {
        slots_[idSlot].SetAttributesFrom(slotSrc);

        if (remainCapacity - remainQuantity < 0)
        {
            slots_[idSlot].quantity_ += remainCapacity;
            remainQuantity -= remainCapacity;
        }
        else
        {
            slots_[idSlot].quantity_ += remainQuantity;
            remainQuantity = 0;
        }

        empty_ = false;
    }
    else
    {
        URHO3D_LOGWARNINGF("GOC_Inventory() - AddCollectableFromSlot : slot %u Can't accept slot type=%s(%u) remainQuantity=%u !",
                           idSlot, GOT::GetType(slotSrc.type_).CString(), slotSrc.type_.Value(), remainQuantity);
    }

    if (recursive && remainQuantity > 0)
    {
//        URHO3D_LOGINFOF("GOC_Inventory() - AddCollectableFromSlot : RemainQuantity = %u => add to inventory", remainQuantity);
        AddCollectableFromSlot(slotSrc, remainQuantity, startIndex);
    }
    // return the waste quantity
    quantity = remainQuantity;
}


/// return the last slot id if freeslot
/// or return -1
int GOC_Inventory::RemoveCollectableOfType(const StringHash& type, unsigned int quantity, unsigned startIndex)
{
    return RemoveCollectableFromSlot(Slot(type, quantity), quantity, startIndex);
}

int GOC_Inventory::RemoveCollectableFromSlot(const Slot& slotSrc, unsigned int& quantity, unsigned startIndex)
{
//    URHO3D_LOGINFOF("GOC_Inventory() - RemoveCollectableFromSlot : Node=%s(%u) type=%s(%u), qty=%d",
//                     node_->GetName().CString(), node_->GetID(), GOT::GetType(slotSrc.type_).CString(), slotSrc.type_.Value(), slotSrc.quantity_);

    unsigned int remainQuantity = quantity;
    int lastslotindex = -1;
    int slotindex = -1;
    do
    {
        slotindex = GetSlotIndexFor(slotSrc.type_, false, startIndex);
        if (slotindex != -1)
        {
            lastslotindex = slotindex;
            Slot& slot = GetSlot(slotindex);
            if (slot.quantity_ <= remainQuantity)
            {
                remainQuantity -= slot.quantity_;
                slot.Clear();
            }
            else
            {
                slot.quantity_ -= remainQuantity;
                remainQuantity = 0;
            }

            if (remainQuantity)
            {
                if (slotindex+1 < slots_.Size())
                    startIndex = slotindex+1;
                else
                    remainQuantity = 0;
            }
        }
    }
    while (slotindex != -1 && remainQuantity > 0);

    CheckEmpty();

    // return the waste quantity
    quantity = remainQuantity;

//    URHO3D_LOGINFOF("GOC_Inventory() - RemoveCollectableFromSlot : OK, remainQuantity=%d", quantity);
//    Dump();

    return lastslotindex;
}

void GOC_Inventory::RemoveCollectableFromSlot(unsigned idSlot, unsigned int quantity)
{
    if (idSlot >= slots_.Size())
        return;

    unsigned int remainQuantity = slots_[idSlot].quantity_ <= quantity ? 0U : slots_[idSlot].quantity_ - quantity;

    if (remainQuantity == 0U)
    {
        slots_[idSlot].Clear();
        CheckEmpty();
    }
    else
    {
        slots_[idSlot].quantity_ -= quantity;
    }

//    URHO3D_LOGINFOF("GOC_Inventory() - RemoveCollectableFromSlot : inventory=%u slotindex=%u remainQuantity=%u", this, idSlot, remainQuantity);
}


void GOC_Inventory::TransferSlotTo(unsigned idSlot, GOC_Inventory* toInventory, unsigned int quantity, const String& section)
{
    if (quantity == QTY_MAX)
        quantity = slots_[idSlot].quantity_;

    bool give = false;
    unsigned int qty;
    Slot *slot = 0;

    Slot& slotsrc = slots_[idSlot];
    Sprite2D* sprite = slotsrc.sprite_;

//    ResourceRef ref = Sprite2D::SaveToResourceRef(sprite);

    if (!slotsrc.type_.Value() && !idSlot)
        slotsrc.type_ = GOT::MONEY;

    ResourceRef ref;
    if (slotsrc.type_ == GOT::MONEY || (!sprite && !idSlot))
    {
        ref.type_ = SpriteSheet2D::GetTypeStatic();
        ref.name_ = "UI/game_equipment.xml@or_pepite04";
    }
    else
    {
        ref = Sprite2D::SaveToResourceRef(sprite);
    }

    if (node_)
        URHO3D_LOGINFOF("GOC_Inventory() - TransferSlotTo : Node=%s(%u) idSlot=%u type=%s(%u) qty=%u sprite=%s resref=%s ...",
                                node_->GetName().CString(), node_->GetID(), idSlot, GOT::GetType(slotsrc.type_).CString(), slotsrc.type_.Value(),
                                quantity, sprite ? sprite->GetName().CString() : "", ref.name_.CString());

    Node* nodeGetter = toInventory->GetNode();
    unsigned idSlotGetter = 0;

    unsigned startIndex = 0;
    if (!section.Empty())
        startIndex = toInventory->GetSectionStartIndex(section);

    do
    {
        toInventory->GetSlotfor(slot, slotsrc, qty, &idSlotGetter, startIndex);

        if (slot)
        {
            qty = slotsrc.TransferTo(slot, quantity, qty);

            if (!qty)
                break;

            if (node_)
            {
                URHO3D_LOGINFOF("GOC_Inventory() - TransferSlotTo : Node=%s(%u) toInventory idSlot=%u, type=%s(%u), qtyTransfered=%u(toTransfer=%u) sprite=%s resref=%s",
                                node_->GetName().CString(), node_->GetID(), idSlot, GOT::GetType(slotsrc.type_).CString(), slotsrc.type_.Value(),
                                qty, quantity, sprite ? sprite->GetName().CString() : "", ref.name_.CString());
            }

            give = true;

            VariantMap& eventData = context_->GetEventDataMap();
            eventData[Go_InventoryGet::GO_GIVER] = node_ ? node_->GetID() : 0;
            eventData[Go_InventoryGet::GO_GETTER] = nodeGetter->GetID();
            eventData[Go_InventoryGet::GO_POSITION] = nodeGetZone_ ? nodeGetZone_->GetWorldPosition2D() : nodeGetter->GetWorldPosition2D();
            eventData[Go_InventoryGet::GO_RESOURCEREF] = ref;
            eventData[Go_InventoryGet::GO_IDSLOTSRC] = idSlot;
            eventData[Go_InventoryGet::GO_IDSLOT] = idSlotGetter;
            eventData[Go_InventoryGet::GO_QUANTITY] = qty;
            nodeGetter->SendEvent(GO_INVENTORYGET, eventData);

            quantity = quantity <= qty ? 0U : quantity - qty;
        }
    }
    while (slot && quantity);

    CheckEmpty();

    if (node_ && give)
        node_->SendEvent(GO_INVENTORYGIVE);
}

void GOC_Inventory::TransferSlotTo(unsigned idSlot, Node* node, unsigned int quantity, const String& section)
{
    if (node)
    {
        if (idSlot >= slots_.Size()) return;

        URHO3D_LOGINFOF("GOC_Inventory() - TransferSlotToNode : node=%s(%u) to node=%s(%u)", node_->GetName().CString(), node_->GetID(), node->GetName().CString(), node->GetID());

        GOC_Inventory* toInventory = node->GetComponent<GOC_Inventory>();
        if (toInventory)
        {
            TransferSlotTo(idSlot, toInventory, quantity, section);
        }
        else
        {
            RemoveCollectableFromSlot(idSlot, quantity);
            node_->SendEvent(GO_INVENTORYGIVE);
        }
    }
    else
    {
        URHO3D_LOGINFOF("GOC_Inventory() - TransferSlotToNode : node=%s(%u) to node=0", node_->GetName().CString(), node_->GetID());

        GOC_Collectable::DropSlotFrom(node_, slots_[idSlot]);
        CheckEmpty();
    }
}

void GOC_Inventory::TransferAllSlotsTo(Node* node)
{
    GOC_Inventory* toInventory = node->GetComponent<GOC_Inventory>();

    if (!toInventory)
    {
        URHO3D_LOGINFOF("GOC_Inventory() - TransferAllSlotsToNode : node=%s(%u) No Inventory !", node->GetName().CString(), node->GetID());
        return;
    }

    unsigned numslots = GetNumSlots();

//    URHO3D_LOGINFOF("GOC_Inventory() - TransferAllSlotsToNode : numslots = %u - Dump Before Transfer", numslots);
//    Dump();
//    toInventory->Dump();

    for (unsigned i=0; i<numslots; ++i)
    {
        if (!slots_[i].Empty())
            TransferSlotTo(i, toInventory);
    }

    CheckEmpty();

//    URHO3D_LOGINFOF("GOC_Inventory() - TransferAllSlotsToNode : Dump After Transfer");
//    Dump();
//    toInventory->Dump();
}

void GOC_Inventory::Set(const Vector<Slot>& inventory)
{
    URHO3D_LOGINFOF("GOC_Inventory() - Set : %s(%u)", node_->GetName().CString(), node_->GetID());

    slots_ = inventory;

    for (unsigned i = 0; i < slots_.Size(); i++)
    {
        Slot& slot = slots_[i];

        slot.uisprite_.Reset();
        slot.UpdateUISprite();

        if (slot.type_ == GOT::MONEY)
            slot.sprite_ = slot.uisprite_;
        else if (slot.type_)
            slot.sprite_ = GameHelpers::GetSpriteForType(slot.type_, slot.partfromtype_, slot.uisprite_ ? slot.uisprite_->GetName() : String::EMPTY);
    }

    CheckEmpty();

    Dump();
}

void GOC_Inventory::Dump() const
{
    URHO3D_LOGINFOF("GOC_Inventory() - Dump : %s(%u) - Inventory : Ptr=%u Template=%s numSlots=%u empty_=%s",
                    node_ ? node_->GetName().CString() : "", node_ ? node_->GetID() : 0, this, currentTemplate ? currentTemplate->name.CString() : "", slots_.Size(), empty_ ? "true":"false");

    for (unsigned i=0; i<valuesToPopulate_.Size(); ++i)
        URHO3D_LOGINFOF(" => valuesToPopulate_[%u] = %s", i, valuesToPopulate_[i].GetString().CString());

    for (unsigned i=0; i<slots_.Size(); ++i)
    {
        const Slot& s = slots_[i];
        URHO3D_LOGINFOF(" => slots_[%u] = (collectable=%s(%u);quantity=%d;fromtype=%s;sprite=%u;uisprite=%u)",i,
                        GOT::GetType(s.type_).CString(), s.type_.Value(), s.quantity_, GOT::GetType(s.partfromtype_).CString(), s.sprite_.Get(), s.uisprite_.Get());
        if (s.sprite_ || s.uisprite_)
            URHO3D_LOGINFOF("   => sprite_ = (scale=%f,%f;name=%s) uisprite_ = (name=%s)",
                            s.scale_.x_, s.scale_.y_, s.sprite_ ? s.sprite_->GetName().CString() : "", s.uisprite_ ? s.uisprite_->GetName().CString() : "");
    }
}



void GOC_Inventory::OnSetEnabled()
{
//    URHO3D_LOGINFO("GOC_Inventory() - OnSetEnabled ...");

    Scene* scene = GetScene();
    if (scene)
    {
        if (IsEnabledEffective())
        {
//            URHO3D_LOGINFO("GOC_Inventory() - OnSetEnabled");

            if (!nodeGetZone_)
            {
                Node* spriterGetZoneNode = node_->GetChild("RootNode") ? (node_->GetChild("RootNode")->GetChild("GetZone") ? node_->GetChild("RootNode")->GetChild("GetZone") : 0) : 0;
                nodeGetZone_ = spriterGetZoneNode ? spriterGetZoneNode : node_;
            }

            if (receiveEvent_)
                SubscribeToEvent(node_, receiveEvent_, URHO3D_HANDLER(GOC_Inventory, HandleReceive));

            if (GameContext::Get().ClientMode_)
                return;

            if (enableGive_ && enableGiveEvent_)
                SubscribeToEvent(node_, enableGiveEvent_, URHO3D_HANDLER(GOC_Inventory, OnEnableGive));

            // Players don't drop
            if (GameContext::Get().LocalMode_)
            {
                if (!node_->GetComponent<GOC_PlayerController>())
                    SubscribeToEvent(node_, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_Inventory, HandleDrop));
            }
            else if (GameContext::Get().ServerMode_)
            {
                GOC_Controller* controller = node_->GetDerivedComponent<GOC_Controller>();
                if (controller && (controller->GetControllerType() & (GO_Player & GO_NetPlayer)) == 0)
                    SubscribeToEvent(node_, GOC_LIFEDEAD, URHO3D_HANDLER(GOC_Inventory, HandleDrop));
            }

            if (CheckEmpty())
                node_->SendEvent(GO_INVENTORYEMPTY);
        }
        else
        {
            if (enableGiveEvent_)
                UnsubscribeFromEvent(node_, enableGiveEvent_);

            if (receiveEvent_)
                UnsubscribeFromEvent(node_, receiveEvent_);

            UnsubscribeFromEvent(node_, GOC_LIFEDEAD);
        }
    }
    else
    {
        UnsubscribeFromAllEvents();
    }

//    URHO3D_LOGINFO("GOC_Inventory() - OnSetEnabled ... OK !");
}

void GOC_Inventory::OnNodeSet(Node* node)
{
    if (node)
    {
//        URHO3D_LOGINFO("GOC_Inventory() - OnNodeSet");
        OnSetEnabled();
    }
    else
    {
        UnsubscribeFromAllEvents();
    }
}

void GOC_Inventory::OnEnableGive(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFO("GOC_Inventory() - OnEnableGive");

    SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Inventory, HandleGive));
}

void GOC_Inventory::HandleGive(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("GOC_Inventory() - HandleGive : %s(%u), ... enableGive_=%s", node_->GetName().CString(), node_->GetID(), enableGive_ ? "true":"false");

    if (!enableGive_)
        return;

    if (eventType != E_PHYSICSBEGINCONTACT2D)
        return;

    using namespace PhysicsBeginContact2D;

    const ContactInfo& cinfo = GameContext::Get().physicsWorld_->GetBeginContactInfo(eventData[P_CONTACTINFO].GetUInt());

    RigidBody2D* body = GetComponent<RigidBody2D>();
    RigidBody2D* b1 = cinfo.bodyA_;
    RigidBody2D* b2 = cinfo.bodyB_;
    RigidBody2D* other = 0;

    if (b1 == body)
    {
        if (cinfo.shapeA_ && cinfo.shapeA_->IsTrigger())
            return;

        other = b2;
    }
    else if (b2 == body)
    {
        if (cinfo.shapeB_ && cinfo.shapeB_->IsTrigger())
            return;

        other = b1;
    }
    else
        return;

    if (!other)
        return;

    if (other->GetNode() == 0)
        return;

//    int typeBody = body->GetNode()->GetVar(GOA::TYPECONTROLLER).GetInt();
    int typeOther = other->GetNode()->GetVar(GOA::TYPECONTROLLER).GetInt();

//    if (GOT::GetTypeProperties(other->GetNode()->GetVar(GOA::GOT).GetStringHash()) &
    if (typeOther & GO_Player || typeOther == GO_AI_Ally)// && GOT::GetTypeProperties(typeBody) & GO_Collectable)
    {
        URHO3D_LOGINFOF("GOC_Inventory() - HandleGive : %s(%u) is in contact with %s(%u)",
                        other->GetNode()->GetName().CString(), other->GetNode()->GetID(), body->GetNode()->GetName().CString(), body->GetNode()->GetID());

        // lock the inventory
        UnsubscribeFromEvent(E_PHYSICSBEGINCONTACT2D);

        TransferAllSlotsTo(other->GetNode());

        // Unlocked if not Empty
        if (!CheckEmpty())
            SubscribeToEvent(node_, E_PHYSICSBEGINCONTACT2D, URHO3D_HANDLER(GOC_Inventory, HandleGive));
    }
//    else if (typeOther == GO_Collectable && typeBody & GO_Player)
//    {
//
//    }
}

void GOC_Inventory::HandleReceive(StringHash eventType, VariantMap& eventData)
{
    unsigned idslot = eventData[Go_InventoryGet::GO_IDSLOT].GetUInt();
    AnimatedSprite2D* animatedSprite = node_->GetComponent<AnimatedSprite2D>();

//    URHO3D_LOGINFOF("GOC_Inventory() - HandleReceive : Node=%s(%u) receive %s(%u) !",
//                    node_->GetName().CString(), node_->GetID(), GOT::GetType(slots_[idslot].type_).CString(), slots_[idslot].type_.Value());

    LocalEquipSlotOn(this, idslot, animatedSprite);
}

void GOC_Inventory::HandleDrop(StringHash eventType, VariantMap& eventData)
{
    if (node_->GetVar(GOA::DESTROYING).GetBool())
        return;

    if (Empty())
        return;

//    URHO3D_LOGINFOF("GOC_Inventory() - HandleDrop : Dump Before Drop");
//    Dump();

    enableGive_ = false;

    AnimatedSprite2D* animatedSprite = node_->GetComponent<AnimatedSprite2D>();

    if (!animatedSprite)
        return;

    URHO3D_LOGINFOF("GOC_Inventory() - HandleDrop :  Node=%s(%u) drops all slots !", node_->GetName().CString(), node_->GetID());

    for (unsigned i=0; i<slots_.Size(); ++i)
    {
        GOC_Collectable::DropSlotFrom(node_, slots_[i]);

        LocalEquipSlotOn(this, i, animatedSprite);
    }

    if (node_->GetVar(GOA::ISDEAD).GetBool() == true)
        UnsubscribeFromAllEvents();

    node_->SendEvent(GO_INVENTORYEMPTY);

//    URHO3D_LOGINFOF("GOC_Inventory() - HandleDrop : Dump After Drop");
//    Dump();
}

HashMap<unsigned int, Node* > GOC_Inventory::clientNodes_;
HashMap<unsigned int, VariantVector > GOC_Inventory::clientInventories_;
HashMap<unsigned int, VariantVector > GOC_Inventory::clientEquipmentSets_;

void GOC_Inventory::ClearCache()
{
    clientNodes_.Clear();
    clientInventories_.Clear();
    clientEquipmentSets_.Clear();
}

void GOC_Inventory::RegisterClientNode(Node* node)
{
    if (!clientNodes_.Contains(node->GetID()))
    {
        URHO3D_LOGINFOF("GOC_Inventory() - RegisterClientNode ... %s(%u) ... cached node !", node->GetName().CString(), node->GetID());
        clientNodes_[node->GetID()] = node;
    }
}

void GOC_Inventory::LoadInventory(Node* node, bool forceInitialStuff)
{
    if (!node)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - LoadInventory ... no node !");
        return;
    }

    URHO3D_LOGERRORF("GOC_Inventory() - LoadInventory ... %s(%u) ... ", node->GetName().CString(), node->GetID());

    GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
    if (!controller)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - LoadInventory ... %s(%u) no controller ...", node->GetName().CString(), node->GetID());
        return;
    }

    bool process = false;

    Player* player = static_cast<Player*>(controller->GetThinker());
    GOC_Inventory* inventory = node->GetComponent<GOC_Inventory>();

    if (GameContext::Get().ClientMode_)
    {
        if (player && inventory && IsNetworkInventoryAvailable(node))
        {
            // Get Full Inventory
            URHO3D_LOGINFOF("GOC_Inventory() - LoadInventory ... %s(%u) load from clientInventories_ !", node->GetName().CString(), node->GetID());
            inventory->SetInventoryAttr(clientInventories_[node->GetID()]);
            clientInventories_.Erase(node->GetID());
            process = true;
        }
        else if (IsNetworkEquipmentSetAvailable(node))
        {
            // Get Full EquipmentSet : will be done in the process block below with NetClientSetEquipment
            URHO3D_LOGINFOF("GOC_Inventory() - LoadInventory ... %s(%u) load from clientEquipmentSets_ !", node->GetName().CString(), node->GetID());
            process = true;
        }
    }
    else
    {
        // For ServerMode or LocalMode : Get the inventoryFile for the entity
        String inventoryFilename;

        if (player)
        {
            int id = player->IsMainController() ? player->GetControlID() : 0;

            if (GameContext::Get().ServerMode_)
            {
                // TODO : Get the account for the player
                //int accountid = GameNetwork::Get()->GetServerClientInfo(player->GetConnection())->GetAccountID();
    //            int playerid  = player->GetID();
    //            id = accountid << 3 + playerid;
                id = 0;
            }
            inventoryFilename = String(GameContext::Get().gameConfig_.saveDir_ + GameContext::Get().savedPlayerFile_[id]);
        }
        else if (static_cast<Actor*>(controller->GetThinker()))
        {
            // TODO : for actor get an unique actorid ...
            inventoryFilename = String::EMPTY;
        }

        // Get the inventory from the inventory file
        if (!inventoryFilename.Empty())
        {
            URHO3D_LOGINFOF("GOC_Inventory() - LoadInventory ... %s(%u) load from file=%s !", node->GetName().CString(), node->GetID(), inventoryFilename.CString());

            // Create a temporary node to load inventory to
            WeakPtr<Node> nodeSrc(GameContext::Get().rootScene_->CreateChild(String::EMPTY, LOCAL));
            nodeSrc->SetTemporary(true);

            if (!forceInitialStuff)
            {
                if (!GameHelpers::LoadNodeXML(node->GetContext(), nodeSrc, inventoryFilename.CString(), LOCAL))
                    bool loadok = GameHelpers::LoadNodeXML(node->GetContext(), nodeSrc, "Data/Save/initialstuff.xml", LOCAL);
            }
            else
                bool loadok = GameHelpers::LoadNodeXML(node->GetContext(), nodeSrc, "Data/Save/initialstuff.xml", LOCAL);

            GOC_Inventory* inventorySrc = nodeSrc->GetComponent<GOC_Inventory>();
            if (inventorySrc)
            {
                inventorySrc->UnsubscribeFromAllEvents();

                // Copy inventory
                if (inventory)
                {
                    inventory->Set(inventorySrc->Get());
                    process = true;
                }

                // Remove the temporary node
                VariantMap& eventData = node->GetContext()->GetEventDataMap();
                eventData[GOC_Life_Events::GO_ID] = nodeSrc->GetID();
                eventData[GOC_Life_Events::GO_KILLER] = 0;
                eventData[GOC_Life_Events::GO_TYPE] = GO_Player;
                nodeSrc->SendEvent(GOC_LIFEDEAD, eventData);
                GOC_Destroyer* destroyer= nodeSrc->GetComponent<GOC_Destroyer>();
                if (destroyer)
                {
                    destroyer->SetDestroyMode(FREEMEMORY);
                    destroyer->Destroy(0.f);
                }
            }
            else
                URHO3D_LOGERRORF("GOC_Inventory() - LoadInventory ... %s(%u) no source ...", node->GetName().CString(), node->GetID());

            if (nodeSrc)
            {
                nodeSrc->Remove();
                nodeSrc.Reset();
            }
        }
    }

    if (process)
    {
        // Apply Equipment

        // TODO : entityid ?
        int entityid = 0;
        AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
        GameHelpers::SetEntityVariation(animatedSprite, entityid);

        if (player)
        {
            // Add to Equipement
            player->equipment_->Clear();
            player->equipment_->Update(false);
            player->UpdateUI();
        }
        else if (GameContext::Get().ClientMode_)
        {
            NetClientSetEquipment(node);
        }

        URHO3D_LOGERRORF("GOC_Inventory() - LoadInventory ... %s(%u) OK !", node->GetName().CString(), node->GetID());
    }
    else
    {
        URHO3D_LOGINFOF("GOC_Inventory() - LoadInventory ... %s(%u) not process ...", node->GetName().CString(), node->GetID());
        RegisterClientNode(node);
    }
}

static const String EffectFlame("effects_flame");
bool GOC_Inventory::SetEquipmentSlot(AnimatedSprite2D* animatedSprite, unsigned idslot, const String& slotname, StringHash slotType, GOC_Inventory* inventory)
{
    URHO3D_LOGINFOF("GOC_Inventory() - SetEquipmentSlot : idslot=%u slotname=%s ...", idslot, slotname.CString());

    Node* node = animatedSprite->GetNode();
    bool animatorToUpdate = false;
    bool special = slotname.StartsWith("Special");

    if (slotType != StringHash::ZERO)
    {
        if (GOT::GetTypeProperties(slotType) & GOT_Animation)
        {
            if (animatedSprite->HasCharacterMap(slotname))
            {
                animatedSprite->SwapSprite(slotname, 0);
                animatedSprite->ApplyCharacterMap(slotname);
            }

            animatedSprite->ResetAnimation();
            animatorToUpdate |= animatedSprite->RemoveRenderedAnimation(slotname);

            Node* templateNode = GOT::GetObject(slotType);
            if (templateNode)
            {
                AnimatedSprite2D* templateAnimation = templateNode->GetComponent<AnimatedSprite2D>();
                if (templateAnimation)
                {
                    AnimatedSprite2D* renderedAnimation = animatedSprite->AddRenderedAnimation(special ? String("Special1") : slotname, templateAnimation->GetAnimationSet(), templateAnimation->GetTextureFX());
                    if (renderedAnimation)
                    {
                        animatorToUpdate = true;
                        renderedAnimation->GetNode()->SetEnabled(true);

                        /// si l'equipment a un effet, ajouter à renderedAnimation une renderedAnimationEffect
                        if (templateNode->GetVar(GOA::EFFECTID1) != Variant::EMPTY)
                        {
                            ResourceCache* cache = node->GetContext()->GetSubsystem<ResourceCache>();

                            AnimationSet2D* animationEffectSet = cache->GetResource<AnimationSet2D>("2D/" + EffectFlame + ".scml");
                            if (animationEffectSet)
                            {
                                AnimatedSprite2D* renderedAnimationEffect = renderedAnimation->AddRenderedAnimation("Effect", animationEffectSet, templateAnimation->GetTextureFX());
                                if (renderedAnimationEffect)
                                {
//                                    Material* effectMaterial = cache->GetResource<Material>("Materials/effects.xml");
//                                    if (effectMaterial)
//                                        renderedAnimationEffect->SetCustomMaterial(effectMaterial);

                                    renderedAnimationEffect->GetNode()->SetEnabled(true);

                                    if (animatedSprite->GetNode()->GetVar(GOA::INFLUID).GetBool())
                                        if (renderedAnimationEffect->HasCharacterMap(CMAP_NOFIRE))
                                            renderedAnimationEffect->ApplyCharacterMap(CMAP_NOFIRE);
                                }
                                else
                                {
                                    URHO3D_LOGERRORF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) no effect for cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());
                                }
                            }
                            else
                            {
                                URHO3D_LOGERRORF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) no effect for cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());
                            }
                        }
                    }
                    else
                    {
                        URHO3D_LOGERRORF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) no renderedanimation for cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());
                    }
                }
                else
                {
                    URHO3D_LOGERRORF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) no animation for cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());
                }
            }
            else
            {
//                URHO3D_LOGERRORF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) no templateNode for cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());
            }

//            URHO3D_LOGINFOF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) Add rendered animation for cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());
        }
        else
        {
            if (!special)
            {
                animatorToUpdate |= animatedSprite->RemoveRenderedAnimation(slotname);
            }

//            URHO3D_LOGINFOF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) Apply cmap=%s", node->GetName().CString(), node->GetID(), slotname.CString());

            if (animatedSprite->HasCharacterMap(slotname))
            {
                PODVector<Sprite2D*> spritesList;
                Sprite2D::LoadFromResourceRefList(node->GetContext(), GOT::GetResourceList(slotType), spritesList);
                // apply a scaled ratio for weapon if the node scale is quite different than the node scale of Petit (petit scale=0.2) (it's the case for chapanze who has a node scale of 1)
                bool weapon = slotname.StartsWith("Weapon");
//                if (weapon)
//                    animatedSprite->SetMappingScaleRatio(68.f/240.f);
//                if (weapon)
//                    animatedSprite->SetMappingScaleRatio(animatedSprite->GetNode()->GetWorldScale().x_ > 0.4f ? 0.2f / animatedSprite->GetNode()->GetWorldScale().x_: 1.f);
                animatedSprite->SwapSprites(slotname, spritesList, weapon);
                animatedSprite->ResetAnimation();
            }
        }
    }
    else
    {
//        URHO3D_LOGINFOF("GOC_Inventory() - SetEquipmentSlot : Node=%s(%u) Apply cmap=No_%s", node->GetName().CString(), node->GetID(), slotname.CString());

//        GameHelpers::SetRenderedAnimation(animatedSprite, rmap, StringHash::ZERO);

        if (special && inventory)
        {
            if (((slotname.At(7) == '1') && !(GOT::GetTypeProperties(inventory->Get()[idslot+1].type_) & GOT_Animation)) ||
                    ((slotname.At(7) == '2') && !(GOT::GetTypeProperties(inventory->Get()[idslot-1].type_) & GOT_Animation)))
                animatorToUpdate |= animatedSprite->RemoveRenderedAnimation(String("Special1"));
        }
        else
        {
            animatorToUpdate |= animatedSprite->RemoveRenderedAnimation(slotname);
        }

        animatedSprite->ApplyCharacterMap(String("No_") + slotname);
    }

    return animatorToUpdate;
}

void GOC_Inventory::LocalEquipSlotOn(GOC_Inventory* inventory, unsigned idslot, AnimatedSprite2D* animatedSprite, bool netSendMessage)
{
    const String& slotname = inventory->GetSlotName(idslot);
    if (slotname.Empty())
        return;

    Node* node = inventory->GetNode();
    const Slot& slot = inventory->Get()[idslot];
    StringHash slotType = !slot.Empty() ? slot.type_ : StringHash::ZERO;
//
//    URHO3D_LOGINFOF("GOC_Inventory() - LocalEquipSlotOn ... Node=%s(%u) slotname=%s slot=%s empty=%s idslot=%u sendnetmess=%s",
//                    node->GetName().CString(), node->GetID(), slotname.CString(), GOT::GetType(slot.type_).CString(),
//                    !hasslot ? "true":"false", idslot, netSendMessage?"true":"false");

    if (SetEquipmentSlot(animatedSprite, idslot, slotname, slotType, inventory))
    {
        GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
        if (animator)
            animator->PlugDrawables();
    }

    if (netSendMessage)
    {
        URHO3D_LOGINFOF("GOC_Inventory() - LocalEquipSlotOn ... Node=%s(%u) send GO_INVENTORYSLOTEQUIP cmap=%s (inventoryTemplate=%s(%u)) !",
                        node->GetName().CString(), node->GetID(), slotname.CString(), inventory->GetTemplateName().CString(), inventory->GetTemplateHashName().Value());

        VariantMap& eventData = inventory->GetContext()->GetEventDataMap();
        eventData[Net_ObjectCommand::P_NODEID] = inventory->GetNode()->GetID();
        eventData[Net_ObjectCommand::P_INVENTORYTEMPLATE] = inventory->GetTemplateHashName().Value();
        eventData[Net_ObjectCommand::P_INVENTORYITEMTYPE] = slotType.Value();
        eventData[Net_ObjectCommand::P_INVENTORYIDSLOT] = idslot;
        inventory->GetNode()->SendEvent(GO_INVENTORYSLOTEQUIP, eventData);
    }
}

void GOC_Inventory::NetClientSetInventory(unsigned nodeid, VariantMap& eventData)
{
    URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetInventory nodeid=%u ...", nodeid);

    if (eventData.Contains(Net_ObjectCommand::P_INVENTORYSLOTS))
    {
        // in case no node exists, we save the inventory for later use in GOC_Inventory::LoadInventory
        URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetInventory nodeid=%u ... save temporary inventory ", nodeid);
        VariantVector& inventoryslots = clientInventories_[nodeid];
        inventoryslots = eventData[Net_ObjectCommand::P_INVENTORYSLOTS].GetVariantVector();
    }

    if (clientNodes_.Contains(nodeid))
        LoadInventory(clientNodes_[nodeid], false);
    else
        URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetInventory nodeid=%u ... no node exists ... inventory saved for later use !", nodeid);
}

void GOC_Inventory::NetClientSetEquipment(unsigned nodeid, VariantMap& eventData)
{
    URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetEquipment nodeid=%u ...", nodeid);

    if (eventData.Contains(Net_ObjectCommand::P_INVENTORYSLOTS))
    {
        // in case no node exists, we save the equipment for later use in GOC_Inventory::LoadInventory
        URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetEquipment nodeid=%u ... save equipmentset ", nodeid);
        VariantVector& equipmentDataSet = clientEquipmentSets_[nodeid];
        equipmentDataSet = eventData[Net_ObjectCommand::P_INVENTORYSLOTS].GetVariantVector();
        // add the template inventory hash
        equipmentDataSet.Push(eventData[Net_ObjectCommand::P_INVENTORYTEMPLATE].GetUInt());
    }

    if (clientNodes_.Contains(nodeid))
        NetClientSetEquipment(clientNodes_[nodeid]);
    else
        URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetEquipment nodeid=%u ... no node exists ... equipmentset saved for later use !", nodeid);
}

void GOC_Inventory::NetClientSetEquipment(Node* node)
{
    GOC_Inventory::RegisterClientNode(node);

    HashMap<unsigned int, VariantVector>::Iterator it = clientEquipmentSets_.Find(node->GetID());
    if (it == clientEquipmentSets_.End())
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipment : %s(%u) No EquipmentSet exists !", node->GetName().CString(), node->GetID());
        return;
    }

    // get temporary equipment
    VariantVector& equipmentDataSet = it->second_;

    AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
    if (!animatedSprite)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipment : %s(%u) No AnimatedSprite", node->GetName().CString(), node->GetID());
        return;
    }
    const GOC_Inventory_Template* inventoryTemplate = GOC_Inventory_Template::Get(equipmentDataSet.Back().GetUInt());
    if (!inventoryTemplate)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipment : %s(%u) No inventory template %u found !", node->GetName().CString(), node->GetID(), equipmentDataSet.Back().GetUInt());
        return;
    }

    // get inventory if exists (remote host avatar on client has no inventory)
    GOC_Inventory* inventory = node->GetComponent<GOC_Inventory>();

    // set the Equipement Slots
    bool animatorToUpdate = false;
    Pair<unsigned, unsigned> sectionIndexes = inventoryTemplate->GetSectionIndexes("Equipment");
    if (sectionIndexes.first_ <= sectionIndexes.second_)
    {
        for (unsigned i=0; i < equipmentDataSet.Size()-1; i++)
        {
            unsigned idslot = sectionIndexes.first_ + i;
            if (idslot > sectionIndexes.second_)
                break;

            const String& slotname = inventoryTemplate->GetSlotName(idslot);
            if (slotname.Empty())
            {
                URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipment : %s(%u) idslot=%u not find !", node->GetName().CString(), node->GetID(), idslot);
                return;
            }

            animatorToUpdate |= GOC_Inventory::SetEquipmentSlot(animatedSprite, idslot, slotname, StringHash(equipmentDataSet[i].GetUInt()), inventory);
        }
    }

    if (animatorToUpdate)
    {
        GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
        if (animator)
            animator->PlugDrawables();

        URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetEquipment : %s(%u) equipmentset applied ... OK !");
    }
}

void GOC_Inventory::NetClientSetEquipmentSlot(Node* node, VariantMap& eventData)
{
    URHO3D_LOGINFOF("GOC_Inventory() - NetClientSetEquipmentSlot ...");

    AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
    if (!animatedSprite)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipmentSlot : No AnimatedSprite in Node=%u", node->GetID());
        return;
    }

    const unsigned inventorytemplatehash = eventData[Net_ObjectCommand::P_INVENTORYTEMPLATE].GetUInt();
    const GOC_Inventory_Template* inventoryTemplate = GOC_Inventory_Template::Get(inventorytemplatehash);
    if (!inventoryTemplate)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipmentSlot : %s(%u) No inventory template %u found !", node->GetName().CString(), node->GetID(), inventorytemplatehash);
        return;
    }

    const unsigned idslot = eventData[Net_ObjectCommand::P_INVENTORYIDSLOT].GetUInt();
    Pair<unsigned, unsigned> equipmentIndexes = inventoryTemplate->GetSectionIndexes("Equipment");
    if (idslot < equipmentIndexes.first_ || idslot > equipmentIndexes.second_)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipmentSlot : %s(%u) idslot=%u not inside the equipment !", node->GetName().CString(), node->GetID(), idslot);
        return;
    }

    const String& slotname = GOC_Inventory_Template::GetSlotName(inventorytemplatehash, idslot);
    if (slotname.Empty())
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetClientSetEquipmentSlot : %s(%u) idslot=%u inventorytemplatehash=%u not find !", node->GetName().CString(), node->GetID(), idslot, inventorytemplatehash);
        return;
    }

    StringHash slotType(eventData[Net_ObjectCommand::P_INVENTORYITEMTYPE].GetUInt());

    // Update the clientEquipmentSets_ if exists
    HashMap<unsigned int, VariantVector>::Iterator it = clientEquipmentSets_.Find(node->GetID());
    if (it != clientEquipmentSets_.End())
    {
        VariantVector& equipmentDataSet = it->second_;
        unsigned slotindex = idslot - equipmentIndexes.first_;
        // resize if needed
        if (equipmentDataSet.Size() <= slotindex)
            equipmentDataSet.Resize(slotindex+1);
        equipmentDataSet[slotindex] = slotType.Value();
    }

    // Update the animation
    if (SetEquipmentSlot(animatedSprite, idslot, slotname, slotType, 0))
    {
        GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
        if (animator)
            animator->PlugDrawables();
    }
}

void GOC_Inventory::NetServerSetEquipmentSlot(Node* node, VariantMap& eventData)
{
    GOC_Inventory* inventory = node->GetComponent<GOC_Inventory>();
    if (!inventory)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetServerSetEquipmentSlot : No inventory in Node=%u", node->GetID());
        return;
    }
    AnimatedSprite2D* animatedSprite = node->GetComponent<AnimatedSprite2D>();
    if (!animatedSprite)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetServerSetEquipmentSlot : No AnimatedSprite in Node=%u", node->GetID());
        return;
    }

    unsigned idslot = eventData[Net_ObjectCommand::P_INVENTORYIDSLOT].GetUInt();
    StringHash got(eventData[Net_ObjectCommand::P_INVENTORYITEMTYPE].GetUInt());

    // slot inventory update
    Slot& slot = inventory->GetSlot(idslot);
    if (!got)
        slot.Clear();
    else
        slot.Set(got, 1);

    slot.uisprite_.Reset();
    slot.UpdateUISprite();

    // animation update with the inventory on the server
    LocalEquipSlotOn(inventory, idslot, animatedSprite);

    // update player attributes
    GOC_Controller* controller = node->GetDerivedComponent<GOC_Controller>();
    if (controller && controller->GetThinker())
    {
        Player* player = static_cast<Player*>(controller->GetThinker());
        if (player && player->equipment_)
            player->equipment_->UpdateAttributes(idslot);
    }
}

void GOC_Inventory::NetServerDropItem(Node* holder, VariantMap& eventData)
{
    GOC_Inventory* inventory = holder->GetComponent<GOC_Inventory>();
    if (!inventory)
    {
        URHO3D_LOGERRORF("GOC_Inventory() - NetServerDropItem : No inventory in Node=%u", holder->GetID());
        return;
    }

    int dropmode = eventData[Net_ObjectCommand::P_INVENTORYDROPMODE].GetInt();
    unsigned int qty = eventData[Net_ObjectCommand::P_SLOTQUANTITY].GetUInt();

    URHO3D_LOGINFOF("GOC_Inventory() - NetServerDropItem ... holder=%s(%u) dropmode=%d qty=%u ...",
                    holder->GetName().CString(), holder->GetID(), dropmode, qty);

    if (qty == 0)
        return;

    Slot& slot = inventory->GetSlot(eventData[Net_ObjectCommand::P_INVENTORYIDSLOT].GetUInt());

    /// Just drop
    if (dropmode == 0)
    {
        /// Drop items
        Node* node = GOC_Collectable::DropSlotFrom(holder, slot, qty);
        if (node)
        {
            dropmode = 0;
            URHO3D_LOGINFOF("GOC_Inventory() - NetServerDropItem : drop node=%s(%u) item=%s qty=%u !", 
                            node->GetName().CString(), node->GetID(), GOT::GetType(slot.type_).CString(), qty);
        }
        else
            URHO3D_LOGERRORF("GOC_Inventory() - NetServerDropItem : no node drop item=%s qty=%u", GOT::GetType(slot.type_).CString(), qty);
    }
    /// Use the item on holder (DropOn Mode)
    else if (dropmode == 1)
    {
        Node* node = GOC_Collectable::DropSlotFrom(holder, slot, 1U);
        if (node)
        {
            // Apply Effect
            GOC_ZoneEffect* itemEffect = node->GetComponent<GOC_ZoneEffect>();
            if (itemEffect)
                itemEffect->UseEffectOn(holder->GetID());

            // Item don't fall/move
            RigidBody2D* itemBody = node->GetComponent<RigidBody2D>();
            if (itemBody)
                itemBody->SetEnabled(false);

            URHO3D_LOGINFOF("GOC_Inventory() - NetServerDropItem : drop and use item=%s on holder !", GOT::GetType(slot.type_).CString());
        }
        else
            URHO3D_LOGERRORF("GOC_Inventory() - NetServerDropItem : no node drop item=%s qty=1", GOT::GetType(slot.type_).CString());        
    }
    /// Use item (DropOut Mode)
    else if (dropmode == 2)
    {
        Node* node = GOC_Collectable::DropSlotFrom(holder, slot, 1U);
        if (node)
        {
            GOC_Animator2D* animator = node->GetComponent<GOC_Animator2D>();
            if (animator)
            {
                if (!animator->GetTemplateName().StartsWith("AnimatorTemplate_Usable"))
                    animator->SetTemplateName("AnimatorTemplate_Usable");
            }

            node->SendEvent(GO_USEITEM);

            URHO3D_LOGINFOF("GOC_Inventory() - NetServerDropItem : drop and use item=%s !", GOT::GetType(slot.type_).CString());
        }
        else
            URHO3D_LOGERRORF("GOC_Inventory() - NetServerDropItem : no node drop item=%s qty=1", GOT::GetType(slot.type_).CString());        
    }
}
