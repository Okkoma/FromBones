#pragma once

#include <Urho3D/Scene/Component.h>

using namespace Urho3D;


struct EntitiesInfos
{
    StringHash type_;
    Vector2 position_;
};

class GOC_EntityAdder : public Component
{
    URHO3D_OBJECT(GOC_EntityAdder, Component);

public :
    GOC_EntityAdder(Context* context);
    virtual ~GOC_EntityAdder();

    static void RegisterObject(Context* context);

    void SetEntityAttr(const String& typesStr);
    const String& GetEntityAttr() const;

    void SetEnabledEntities(bool enable);

    void Dump();

    virtual void CleanDependences();

    virtual void OnSetEnabled();

protected :
    virtual void OnNodeSet(Node* node);

private :
    void UpdateAttributes();

    String entry_;
    bool autoEnableEntities_;

    Vector<EntitiesInfos> entitiesInfos_;
    Vector<WeakPtr<Node> > entities_;

    WeakPtr<Node> root_;
};


