#pragma once


namespace Urho3D
{
class UIElement;
class Font;
}

class GOC_Inventory;
class Equipment;


using namespace Urho3D;


class UIPanel : public Object
{
    URHO3D_OBJECT(UIPanel, Object);

public :
    UIPanel(Context* context);
    virtual ~UIPanel();

    static void RegisterObject(Context* context);

    bool Set(const String& name, const String& layout, const IntVector2& position, HorizontalAlignment hAlign, VerticalAlignment vAlign, float alpha);

    virtual void Start(Object* user, Object* feeder=0);
    virtual void Reset();
    virtual void Stop();

    void SetPosition(int x, int y, HorizontalAlignment hAlign, VerticalAlignment vAlign);
    void SetPosition(const IntVector2& position, HorizontalAlignment hAlign, VerticalAlignment vAlign);
    void SetVisible(bool state, bool saveposition=false);
    bool IsVisible();

    void ToggleVisible();

    virtual void OnSetVisible();
    virtual void OnResize() { }

    void OnSwitchVisible(StringHash eventType, VariantMap& eventData);

    Object* GetUser()
    {
        return user_;
    }
    UIElement* GetElement()
    {
        return panel_;
    }
    const String& GetName() const
    {
        return panel_->GetName();
    }

    void DebugDraw();

    virtual void Update() {}

    static void RegisterPanel(UIPanel* panel);
    static void ClearRegisteredPanels();
    static UIPanel* GetPanel(const String& name);

    static SharedPtr<UIElement> draggedElement_;
    static bool onKeyDrag_;

protected :

    void OnFocusChange(StringHash eventType, VariantMap& eventData);
    virtual void OnDrag(StringHash eventType, VariantMap& eventData);
    virtual void OnRelease() { }

    String name_;
    Object* feeder_;
    Object* user_;

    WeakPtr<UIElement> panel_;
    IntVector2 lastPosition_;

    static HashMap<String, SharedPtr<UIPanel> > panels_;
};

class UISlotPanel : public UIPanel
{
    URHO3D_OBJECT(UISlotPanel, UIPanel);

public :
    UISlotPanel(Context* context);
    virtual ~UISlotPanel();

    static void RegisterObject(Context* context);

    virtual void Start(Object* user, Object* feeder=0);
    virtual void Reset();

    void SetInventorySection(const String& section);

    Equipment* GetEquipment() const
    {
        return equipment_;
    }
    GOC_Inventory* GetInventory() const
    {
        return inventory_;
    }

    const String& GetInventorySection() const
    {
        return inventorySection_;
    }
    unsigned GetStartSlotIndex() const
    {
        return startSlotIndex_;
    }

    virtual void UpdateSlot(unsigned index, bool updateButtons=false, bool updateSubscribers=false);

    virtual void Update();

    virtual bool AcceptSlotType(int slotId, const StringHash& type, UISlotPanel* otherPanel);

    virtual void OnSetVisible();

    int UseSlotItem(Slot& slot, int fromSlotId, bool allowdrop=false, const IntVector2& droppedPosition=IntVector2::NEGATIVE_ONE);

protected :
    virtual void SetSlotZone();
    virtual void SelectInventoryFrom(const String& name) {  }
    void UpdateNodes();
    virtual void UpdateSlots(bool updateButtons=false, bool updateSubscribers=false);

    virtual void OnSlotUpdate(StringHash eventType, VariantMap& eventData);
    void OnNodesChange(StringHash eventType, VariantMap& eventData);
    virtual int GetFocusSlotId(UIElement* focus, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel);

    virtual void OnDragSlotIn(int slotId, Slot& fromSlot, int fromSlotId, UISlotPanel* fromPanel);
    virtual void OnDragSlotFinish(int slotId);
    virtual void OnSlotRemain(Slot& slot, int slotid);

    virtual void HandleSlotDragBegin(StringHash eventType, VariantMap& eventData);
    virtual void HandleSlotDragMove(StringHash eventType, VariantMap& eventData);
    virtual void HandleSlotDragEnd(StringHash eventType, VariantMap& eventData);

    void BeginKeyDrag();
    void EndKeyDrag();
    void UpdateSlotSelector();

    void OnKeyDown(StringHash eventType, VariantMap& eventData);

    UIElement* slotZone_;
    int slotSize_, miniSlotSize_;

    WeakPtr<Node> holderNode_;
    Equipment* equipment_;
    GOC_Inventory* inventory_;
    GOC_Inventory* dragFromInventory_;

    String inventorySection_;
    unsigned startSlotIndex_;
    unsigned endSlotIndex_;
    unsigned slotselector_;
    unsigned numSlots_;

    WeakPtr<UIElement> dragParent_;

    int slotIndex_;
    int dragElementIndexFrom_;
    int dragElementHolder_;

//    SharedPtr<Font> font_;

    HashMap<int, int > incselector_;

    static WeakPtr<UIElement> beginDragElementByKey_;
    static WeakPtr<UIElement> selectSlotByKey_;
    static UISlotPanel* fromPanelDrag_;
};
