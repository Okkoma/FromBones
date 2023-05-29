#pragma once

#include "UISlotPanel.h"

namespace Urho3D
{
class Node;
class UIElement;
class Text;
class BorderImage;
class Button;
class Timer;
}

using namespace Urho3D;


class UIC_StatusPanel : public UIPanel
{
    URHO3D_OBJECT(UIC_StatusPanel, UIPanel);

public :

    UIC_StatusPanel(Context* context);
    virtual ~UIC_StatusPanel() { }

    static void RegisterObject(Context* context);

    void SelectElement(UIElement* selectelement, UIElement* focuselement, int focusindex=0);
    UIElement* GetSelectElement() const
    {
        return selectedElement_;
    }
    UIElement* GetCharacterList() const
    {
        return characterList;
    }
    void SetUIFactor(float factor);

    virtual void Start(Object* user, Object* feeder=0);

    virtual void OnSetVisible();

    virtual void Update();

protected :
    virtual void OnDrag(StringHash eventType, VariantMap& eventData);
    virtual void OnRelease();

private :

    void UpdateLife();
    void UpdateMoney();

    void UpdateCharacter();
    void UpdateCharacterSelection(UIElement* selection=0);
    void UpdateCharacterList(int index);
    void OpenCharacterSelection();
    void CloseCharacterSelection();

    void OnLifeUpdate(StringHash eventType, VariantMap& eventData);
    void OnMoneyUpdate(StringHash eventType, VariantMap& eventData);
    void OnCharacterUpdated(StringHash eventType, VariantMap& eventData);

    void OnCharacterSelectionStart(StringHash eventType, VariantMap& eventData);
    void OnCharacterSelectionEnd(StringHash eventType, VariantMap& eventData);
    void OnCharacterListHoverEnd(StringHash eventType, VariantMap& eventData);
    void OnCharacterListHoverStart(StringHash eventType, VariantMap& eventData);
    void OnCharacterHoverStart(StringHash eventType, VariantMap& eventData);
    void OnCharacterHoverEnd(StringHash eventType, VariantMap& eventData);
    void OnCharacterArrowHoverStart(StringHash eventType, VariantMap& eventData);
    void OnCharacterArrowHoverEnd(StringHash eventType, VariantMap& eventData);
    void OnCharacterArrowHovering(StringHash eventType, VariantMap& eventData);

    void OnEscapePanel(StringHash eventType, VariantMap& eventData);
    void OnKeyDown(StringHash eventType, VariantMap& eventData);

    WeakPtr<Node> avatar_;
    WeakPtr<UIElement> lifeIconZone;
    WeakPtr<UIElement> moneyZone;
    WeakPtr<Text> moneyText;
    WeakPtr<BorderImage> healthBar;
    WeakPtr<BorderImage> manaBar;
    WeakPtr<Button> characterButton;
    WeakPtr<UIElement> characterList;
    int avatarIndex_;
    int characterHovering_;
    int characterArrowDirection_;
    int characterSelectIndex_;
    int characterHoveringIndex_;
    Timer* characterArrowTimer_;
    UIElement* selectedElement_;
    UIElement* focusedElement_;

    float uifactor_;

    Vector<StringHash> activableCharacters_;
};
