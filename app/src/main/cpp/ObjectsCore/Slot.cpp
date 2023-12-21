#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Core/Timer.h>

#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>

#include "GameAttributes.h"
#include "GameEvents.h"
#include "GameHelpers.h"
#include "GameContext.h"

#include "Slot.h"


/// Slot

const SlotEntityInfo SlotEntityInfo::EMPTY = SlotEntityInfo();

void Slot::Clear()
{
    partfromtype_ = StringHash::ZERO;
    type_ = StringHash::ZERO;
    quantity_ = 0U;
    effect_ = -1;

    sprite_.Reset();
    uisprite_.Reset();
}

void Slot::Set(StringHash type)
{
    partfromtype_ = StringHash::ZERO;
    type_ = type;
    quantity_ = 0;
    sprite_ = 0;
    effect_ = -1;

    Node* templatenode = GOT::GetObject(type);
    if (templatenode)
    {
        StaticSprite2D* drawable = templatenode->GetDerivedComponent<StaticSprite2D>();
        if (drawable)
            sprite_ = drawable->GetSprite();

        effect_ = templatenode->GetVar(GOA::EFFECTID1).GetInt();
    }
}

void Slot::Set(const StringHash& type, unsigned int quantity, const StringHash& fromtype, const ResourceRef& spriteRef)
{
    partfromtype_ = fromtype;
    type_ = type;
    quantity_ = quantity;

//    URHO3D_LOGERRORF("Slot() - Set : type=%s(%u) quantity=%u", GOT::GetType(type_).CString(), type_.Value(), quantity);

    if (type == GOT::MONEY)
    {
        SetSprite(Sprite2D::LoadFromResourceRef(GameContext::Get().context_, ResourceRef(SpriteSheet2D::GetTypeStatic(), "UI/game_equipment.xml@or_pepite04")));
    }
    else if (spriteRef != Variant::emptyResourceRef)
    {
        Sprite2D* sprite = Sprite2D::LoadFromResourceRef(GameContext::Get().context_, spriteRef);
        if (sprite)
        {
            SetSprite(sprite);
//                URHO3D_LOGINFOF("Slot() - Set : type=%s(%u) spriteRef=%s;%s",
//                         GOT::GetType(type_).CString(), type_.Value(),
//                         GameContext::Get().context_->GetTypeName(spriteRef.type_).CString(), spriteRef.name_.CString());
        }
        else
        {
            URHO3D_LOGINFOF("Slot() - Set1 : type=%s(%u) spriteRef=%s;%s NOK !",
                            GOT::GetType(type_).CString(), type_.Value(),
                            GameContext::Get().context_->GetTypeName(spriteRef.type_).CString(), spriteRef.name_.CString());
        }
    }
//    else
//        URHO3D_LOGINFO("Slot() - Set : !spriteRef");

    if (!sprite_)
        sprite_ = GameHelpers::GetSpriteForType(type_, partfromtype_, -1);

    if (!sprite_)
        URHO3D_LOGWARNINGF("Slot() - Set : type=%s(%u) NOK !!!", GOT::GetType(type_).CString(), type_.Value());

    UpdateUISprite();
}

void Slot::SetAttributesFrom(const Slot& slot)
{
    partfromtype_ = slot.partfromtype_;
    type_ = slot.type_;
    effect_ = slot.effect_;

    SetSprite(slot.sprite_ ? slot.sprite_.Get() : GameHelpers::GetSpriteForType(type_, partfromtype_), slot.scale_, slot.color_);
}

void Slot::SetAttributes(const StringHash& type, unsigned int quantity, const StringHash& fromtype, Sprite2D* sprite, const Vector3& scale, const Color& color)
{
    partfromtype_ = fromtype;
    type_ = type;
    quantity_ = quantity;

    sprite_ = sprite;
    scale_ = scale;
    color_ = color;

    UpdateUISprite();
}

void Slot::SetSprite(Sprite2D* sprite, const Vector3& scale, const Color& color)
{
    sprite_.Reset();
    sprite_ = sprite;
    scale_ = scale;
    color_ = color;

    UpdateUISprite();
}

void Slot::UpdateUISprite()
{
    if (sprite_ && (!uisprite_ || sprite_->GetNameHash() != uisprite_->GetNameHash()))
        uisprite_ = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(sprite_->GetName());

    if (!uisprite_)
        uisprite_ = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(GOT::GetType(type_));

    if (!uisprite_)
        uisprite_ = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(GOT::GetType(type_).ToLower());
    // update sprite_ with uisprite_ for reduce batches
//    if (uisprite_)
//        sprite_ = uisprite_;
//    else
    if (!uisprite_)
        URHO3D_LOGERRORF("Slot() - UpdateUISprite : type=%s(%u) no uisprite !!!", GOT::GetType(type_).CString(), type_.Value());
}

unsigned int Slot::TransferTo(Slot* slot, unsigned int qtyToTransfer, unsigned int freeSpace)
{
//    URHO3D_LOGINFOF("Slot() - TransferTo : this=%u to slot=%u type=%s(%u) ... qty=%u qtytoTransfer=%u freeSpace=%u ... ", this, slot, GOT::GetType(type_).CString(), type_.Value(), quantity_, qtyToTransfer, freeSpace);

    if (quantity_ == 0U || qtyToTransfer == 0U)
        return 0U;

    if (qtyToTransfer > quantity_)
        qtyToTransfer = quantity_;

    if (qtyToTransfer > freeSpace)
        qtyToTransfer = freeSpace;

//    URHO3D_LOGINFOF("Slot() - TransferTo : this=%u to slot=%u type=%s(%u) ... slot qty before transfer this=%u slot=%u ", this, slot, GOT::GetType(type_).CString(), type_.Value(), quantity_, slot->quantity_);

    if (slot)
    {
        slot->quantity_ += qtyToTransfer;
        slot->SetAttributesFrom(*this);
    }

    quantity_ -= qtyToTransfer;

//    URHO3D_LOGINFOF("Slot() - TransferTo : this=%u to slot=%u type=%s(%u) ... slot qty after transfer this=%u slot=%u ... qtyToTransfer=%u", this, slot, GOT::GetType(type_).CString(), type_.Value(), quantity_, slot->quantity_, qtyToTransfer);

    if (!quantity_)
        Clear();

    return qtyToTransfer;
}

void Slot::Dump() const
{
    URHO3D_LOGINFOF("Slot() - Dump : type_=%s(%u) qty_=%u sprite=%s(%u) uisprite=%s(%u)",
                    GOT::GetType(type_).CString(), type_.Value(), quantity_, sprite_ ? sprite_->GetName().CString() : "", sprite_.Get(), uisprite_ ? uisprite_->GetName().CString() : "", uisprite_.Get());
}

bool Slot::SetSlotAttr(const String& value, Slot& slot)
{
//    URHO3D_LOGINFOF("Slot() - SetSlotAttr (%s) ...", value.CString());

    String quantityStr = String::EMPTY;
    String effectStr = String::EMPTY;
    String typeStr = String::EMPTY;
    String categoryStr = String::EMPTY;
    String fromtypeStr = String::EMPTY;
    String spriteStr = String::EMPTY;
    String scaleStr = String::EMPTY;
    String colorStr = String::EMPTY;

    Vector<String> vString = value.Split('|');
    if (vString.Size() == 0)
        return false;

    for (Vector<String>::Iterator it = vString.Begin(); it != vString.End(); ++it)
    {
        Vector<String> s = it->Split(':');
        if (s.Size() < 2)
            continue;

        String str = s[0].Trimmed();

        if (str == "category")
            categoryStr = s[1].Trimmed();
        if (str == "type")
            typeStr = s[1].Trimmed();
        else if (str == "quantity")
            quantityStr = s[1].Trimmed();
        else if (str == "effect")
            effectStr = s[1].Trimmed();
        else if (str == "fromtype")
            fromtypeStr = s[1].Trimmed();
        else if (str == "sprite")
            spriteStr = s[1].Trimmed();
        else if (str == "scale")
            scaleStr = s[1].Trimmed();
        else if (str == "color")
            colorStr = s[1].Trimmed();
    }

    if (quantityStr.Empty())
    {
        URHO3D_LOGERROR("Slot() - SetSlotAttr No Qty !");
        return false;
    }

    if (typeStr.Empty() && categoryStr.Empty() && fromtypeStr.Empty())
    {
        URHO3D_LOGERROR("Slot() - SetSlotAttr No Type !");
        return false;
    }

    StringHash type;

    if (typeStr.Empty())
    {
        if (!categoryStr.Empty())
        {
            bool setfromcategory = false;

            if (categoryStr.StartsWith("RANDOM(") && categoryStr.EndsWith(")"))
            {
                Vector<String> categoriesString = categoryStr.Substring(7, categoryStr.Length()-8).Split(',');
                Vector<StringHash> categories;
                for (unsigned i=0; i < categoriesString.Size(); i++)
                {
                    StringHash category(categoriesString[i]);
                    if (COT::IsACategory(category))
                        categories.Push(category);
                }

                if (categories.Size())
                {
                    type = COT::GetRandomTypeFrom(categories[Random((int)categories.Size())]);
//					URHO3D_LOGERRORF("Slot() - SetSlotAttr value=%s => random type = %s !", value.CString(), GOT::GetType(type).CString());

                    setfromcategory = true;
                }
                else
                {
                    URHO3D_LOGERRORF("Slot() - SetSlotAttr value=%s => category type error !", value.CString());
                }
            }

            if (!setfromcategory)
            {
                type = categoryStr.Contains("RANDOM") ? COT::GetRandomTypeFrom(COT::ITEMS) : COT::GetRandomTypeFrom(StringHash(categoryStr));
            }
        }
        // TODO : atm fix Elsarion Part that have with no type
        else if (!fromtypeStr.Empty())
        {
            type = GOT::COLLECTABLEPART;
        }
    }

    if (type == StringHash::ZERO && !typeStr.Empty())
        type = StringHash(typeStr);

    if (type == StringHash::ZERO)
        return false;

//    URHO3D_LOGINFOF("Slot() - SetSlotAttr type=%s(%u) ...", GOT::GetType(type).CString(),type.Value());

    unsigned int quantity;

    if (quantityStr.StartsWith("RANDOM"))
    {
        if (quantityStr.EndsWith(")"))
        {
            Vector<String> qtyString = quantityStr.Substring(7, quantityStr.Length()-8).Split(',');
            int valuemin = qtyString.Size() > 1 ? ToInt(qtyString.Front()) : 1;
            int valuemax = ToInt(qtyString.Back());

            quantity = (unsigned int)Random(valuemin, valuemax);
//            URHO3D_LOGERRORF("Slot() - SetSlotAttr type=%s(%u) value=%s => RANDOM(%d,%d) => random Qty = %u !", GOT::GetType(type).CString(), type.Value(), value.CString(), valuemin, valuemax, quantity);
        }
        else
        {
            quantity = (unsigned int)Random(1, GOT::GetMaxDropQuantityFor(type));
//            URHO3D_LOGERRORF("Slot() - SetSlotAttr type=%s(%u) value=%s => random Qty = %u !", GOT::GetType(type).CString(), type.Value(), value.CString(), quantity);
        }
    }
    else if (quantityStr.Contains("MAX"))
    {
        quantity = (unsigned int)GOT::GetMaxDropQuantityFor(type);
    }
    else
    {
        quantity = ToUInt(quantityStr);
    }

    StringHash fromtype;
    if (!fromtypeStr.Empty())
    {
        fromtype = StringHash(ToUInt(fromtypeStr));
    }
    else if (GOT::GetTypeProperties(type) & GOT_Part)
    {
        fromtype = GOT::GetBuildableType(type);
//        URHO3D_LOGINFOF("Slot() - SetSlotAttr : is a PART of fromtype=%s(%u)", GOT::GetType(fromtype).CString(), fromtype.Value());
    }
    if (!spriteStr.Empty())
    {
        Vector<String> s = spriteStr.Split(';');
        ResourceRef ref = ResourceRef(StringHash(s[0].Trimmed()), s[1].Trimmed());
//        URHO3D_LOGINFOF("Slot() - SetSlotAttr : ResourceRef=%s;%s",
//                 GameContext::Get().context_->GetTypeName(ref.type_).CString(), ref.name_.CString());
        slot.Set(type, quantity, fromtype, ref);
    }
    else
    {
        slot.Set(type, quantity, fromtype);
    }
    if (!scaleStr.Empty())
    {
        Variant var;
        var.FromString(VAR_VECTOR3, scaleStr);
        slot.scale_ = var.GetVector3();
    }
    if (!colorStr.Empty())
    {
        Variant var;
        var.FromString(VAR_COLOR, colorStr);
        slot.color_ = var.GetColor();
    }
    if (!effectStr.Empty())
    {
        slot.effect_ = ToInt(effectStr);
//        URHO3D_LOGINFOF("Slot() - SetSlotAttr : type=%s(%u) has effect=%d", GOT::GetType(type).CString(), type.Value(), slot.effect_);
    }

    return true;
}

String Slot::GetSlotAttr(const Slot& slot)
{
    String value;

    if (!slot.quantity_) return String::EMPTY;

//    if (!typeStr.Contains("Collectable_")) return String::EMPTY;
//    value = "type:" + typeStr.Substring(typeStr.Find("_")+1) + "|quantity:" + String(slot.quantity_);

    value = "type:" + GOT::GetType(slot.type_) + "|quantity:" + String(slot.quantity_);

    if (slot.effect_ > -1)
    {
        value += "|effect:" + String(slot.effect_);
    }
    if (slot.partfromtype_)
    {
        value += "|fromtype:" + String(slot.partfromtype_.Value());
    }
    if (slot.sprite_)
    {
        ResourceRef ref = Sprite2D::SaveToResourceRef(slot.sprite_.Get());
        value += "|sprite:" + GameContext::Get().context_->GetTypeName(ref.type_) + ";" + ref.name_;
    }
    if (slot.scale_ != Vector3::ONE)
    {
        value += "|scale:" + slot.scale_.ToString();
    }
    if (slot.color_ != Color::WHITE)
    {
        value += "|color:" + slot.color_.ToString();
    }

//    URHO3D_LOGINFOF("Slot() - GetSlotAttr %s", value.CString());

    return value;
}

void Slot::SetSlotData(Slot& slot, VariantMap& slotData)
{
    StringHash got(slotData[Net_ObjectCommand::P_CLIENTOBJECTTYPE].GetUInt());
    const unsigned int qty = slotData[Net_ObjectCommand::P_SLOTQUANTITY].GetUInt();
    if (got.Value() && qty)
    {
        slot.type_ = got;
        slot.partfromtype_ = StringHash(slotData[Net_ObjectCommand::P_SLOTPARTFROMTYPE].GetUInt());
        slot.quantity_ = qty;
        slot.effect_ = slotData[Net_ObjectCommand::P_SLOTEFFECT].GetInt();
        const ResourceRef& rref = slotData[Net_ObjectCommand::P_SLOTSPRITE].GetResourceRef();
        if (rref != Variant::emptyResourceRef)
            slot.sprite_ = Sprite2D::LoadFromResourceRef(GameContext::Get().context_, rref);
        slot.scale_ = slotData[Net_ObjectCommand::P_SLOTSCALE].GetVector3();
        slot.color_ = slotData[Net_ObjectCommand::P_SLOTCOLOR].GetColor();
        slot.UpdateUISprite();
    }
    else
        slot.Clear();
}

void Slot::GetSlotData(const Slot& slot, VariantMap& slotData, unsigned qty)
{
    slotData[Net_ObjectCommand::P_CLIENTOBJECTTYPE] = slot.type_.Value();
    slotData[Net_ObjectCommand::P_SLOTPARTFROMTYPE] = slot.partfromtype_.Value();
    slotData[Net_ObjectCommand::P_SLOTQUANTITY] = qty == QTY_MAX ? slot.quantity_ : qty;
    slotData[Net_ObjectCommand::P_SLOTEFFECT] = slot.effect_;
//    slotData[Net_ObjectCommand::P_SLOTSPRITE] = slot.sprite_ ? slot.sprite_->GetNameHash() : StringHash::ZERO;
    if (slot.sprite_)
        slotData[Net_ObjectCommand::P_SLOTSPRITE] = Sprite2D::SaveToResourceRef(slot.sprite_.Get());
    slotData[Net_ObjectCommand::P_SLOTSCALE] = slot.scale_;
    slotData[Net_ObjectCommand::P_SLOTCOLOR] = slot.color_;
}

VariantVector Slot::GetSlotDatas(const Vector<Slot>& slots, bool skipEmpty)
{
    VariantVector datas;

    if (slots.Size())
    {
        datas.Reserve(slots.Size());
        for (unsigned i=0; i < slots.Size(); ++i)
        {
            const Slot& slot = slots[i];
            if (skipEmpty && !slot.quantity_)
                continue;

            datas.Push(Slot::GetSlotAttr(slot));
        }
    }
    return datas;
}


