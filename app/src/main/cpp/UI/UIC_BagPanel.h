#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
class UIElement;
}

using namespace Urho3D;


class UIC_BagPanel : public UISlotPanel
{
    URHO3D_OBJECT(UIC_BagPanel, UISlotPanel);

public :

    UIC_BagPanel(Context* context);
    virtual ~UIC_BagPanel()  { ; }

    static void RegisterObject(Context* context);

    virtual void UpdateSlot(unsigned index, bool updateButtons=false, bool updateSubscribers=false, bool updateNet=false);

    virtual void OnResize();

protected :
    virtual void SetSlotZone();

    void HandleDoubleClic(StringHash eventType, VariantMap& eventData);

};
