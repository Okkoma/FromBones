#pragma once

#include "Slot.h"

namespace Urho3D
{
class RigidBody2D;
class CollisionCircle2D;
}

using namespace Urho3D;


class GOC_DropZone : public Component
{
    URHO3D_OBJECT(GOC_DropZone, Component);

public :
    GOC_DropZone(Context* context);
    virtual ~GOC_DropZone();

    static void RegisterObject(Context* context);

    void SetActived(bool actived);
    void SetActiveStorage(bool activeStorage);
    void SetActiveThrowItems(bool activeThrow);
    void SetBuildObjects(bool buildobjects);
    void SetRemoveParts(bool removeparts);
    void SetRadius(float radius);
    void SetNumHitBeforeTrig(int numHits);
    void SetThrowLinearVelocity(float vel);
    void SetThrowAngularVelocity(float ang);
    void SetThrowImpulse(float imp);
    void SetThrowDelay(float delay);

    bool GetActived() const;
    bool GetActiveStorage() const;
    bool GetActiveThrowItems() const;
    bool GetBuildObjects() const;
    bool GetRemoveParts() const;
    float GetRadius() const;
    int GetNumHitBeforeTrig() const;
    float GetThrowLinearVelocity() const;
    float GetThrowAngularVelocity() const;
    float GetThrowImpulse() const;
    float GetThrowDelay() const;

    bool AddItem(const Slot& slot);
    void AddItemAttr(const String& value);
    void SetStorageAttr(const VariantVector& value);
    VariantVector GetStorageAttr() const;

    void ThrowOutItems();

    void Dump();

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void Stop();
    void Clear();

    void SearchForBuildableObjects(const Vector<Slot>& slots, Vector<StringHash>& buildableObjectTypes);
    int SelectBuildableEntity(const StringHash& got, const Vector<Slot>& slots, EquipmentList& equipment);
    void RemovePartsOfBuildableObject(const StringHash& buildableObjectType);

    void HandleScenePostUpdate(StringHash eventType, VariantMap& eventData);
    void HandleContact(StringHash eventType, VariantMap& eventData);
    void HandleThrowOutItems(StringHash eventType, VariantMap& eventData);

    RigidBody2D* body_;
    CollisionCircle2D* dropZone_;
    bool actived_;
    bool activeStorage_;
    bool activeThrow_;
    bool buildObjects_;
    bool removeParts_;

    float radius_;
    int numHitsToTrig_;
    short unsigned hitCount_;
    unsigned lastHitTime_;
    unsigned throwItemsBeginIndex_;
    unsigned throwItemsEndIndex_;
    float throwItemsTimer_;
    float linearVelocity_;
    float angularVelocity_;
    float impulse_;
    float throwDelay_;

    Vector<Slot> items_;
    Vector<String> itemsAttr_;
    Vector<WeakPtr<Node> > itemsNodes_;
};


