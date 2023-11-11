#include <Urho3D/Urho3D.h>

#include <Urho3D/Core/Timer.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Graphics/Graphics.h>
#include <Urho3D/Graphics/Texture2D.h>
#include <Urho3D/Graphics/Renderer.h>

#include <Urho3D/Resource/ResourceCache.h>

#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>

#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/SpriteSheet2D.h>
#include <Urho3D/Urho2D/SpriterInstance2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>

#include <Urho3D/Input/Input.h>

#include <Urho3D/UI/BorderImage.h>
#include <Urho3D/UI/Button.h>
#include <Urho3D/UI/Sprite.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>
#include <Urho3D/UI/UIEvents.h>
#include <Urho3D/UI/UIElement.h>

#include "GameAttributes.h"
#include "GameContext.h"
#include "GameHelpers.h"
#include "GameEvents.h"

#include "GOC_Life.h"
#include "GOC_Inventory.h"

#include "Player.h"

#include "UISlotPanel.h"
#include "UIC_AbilityPanel.h"
#include "UIC_StatusPanel.h"



#define NBLIFE_MAX 6

const unsigned updateDelays_[] =
{
    50, 100, 150, 200, 250
};


UIC_StatusPanel::UIC_StatusPanel(Context* context) :
    UIPanel(context),
    selectedElement_(0),
    focusedElement_(0),
    uifactor_(1.f),
    selector_(0),
    selectordirection_(1),
    lastSelectedElement_(0),
    lastSelector_(0) { }

void UIC_StatusPanel::RegisterObject(Context* context)
{
    context->RegisterFactory<UIC_StatusPanel>();
}

void UIC_StatusPanel::Start(Object* user, Object* feeder)
{
    user_ = user;
    feeder_ = feeder;
    Player* player = static_cast<Player*>(user_);

    healthBar = panel_->GetChildStaticCast<BorderImage>(String("HealthBar"), true);
    manaBar = panel_->GetChildStaticCast<BorderImage>(String("ManaBar"), true);
    characterButton = panel_->GetChildStaticCast<Button>(String("CharacterButton"), true);
    lifeIconZone = panel_->GetChild(String("LifeZone"), true);
    moneyZone = panel_->GetChild(String("BagButton"), true);
    moneyText = player->GetPanel(BAGPANEL)->GetElement()->GetChildStaticCast<Text>(String("MoneyText"), true);

    // Create the Characters List Panel
    if (!characterList && panels_.Size())
    {
        SharedPtr<UIElement> uielement = GameContext::Get().ui_->LoadLayout(GetSubsystem<ResourceCache>()->GetResource<XMLFile>("UI/PlayerCharactersList.xml"));
        if (!uielement)
        {
            URHO3D_LOGERRORF("UIC_StatusPanel() - Start : Can't create PlayerCharacterList Panel !");
            return;
        }

        characterList = uielement.Get();
        GameContext::Get().ui_->GetRoot()->AddChild(characterList);
        characterList->SetVisible(false);
        characterArrowTimer_ = new Timer();
    }
    
    // Get Ability Panel if it's a popup (multiplayers mode)
    abilityPanel_ = player->GetPanel(ABILITYPANEL);

    UIElement* firstbutton = panel_->GetChild("QuestButton", true);
    UIElement* lastbutton = panel_->GetChild("MapButton", true);
    statusChildRange_.x_ = panel_->FindChild(firstbutton);
    statusChildRange_.y_ = panel_->FindChild(lastbutton);
    // MapButton is disable for the second players so reduce the range
    if (lastbutton && player->GetID() > 1)
    {
        lastbutton->SetVisible(false);        
        statusChildRange_.y_--;
    }

    characterlistChildRange_.x_ = characterList->FindChild(characterList->GetChild("Slot_1", true));
    characterlistChildRange_.y_ = characterList->FindChild(characterList->GetChild("Slot_3", true));
}

void UIC_StatusPanel::GainFocus()
{
    URHO3D_LOGERRORF("UIC_StatusPanel() - GainFocus !");

    UIPanel::GainFocus();
    GameContext::Get().ui_->SetFocusElement(GetElement());
}

void UIC_StatusPanel::LoseFocus()
{
    URHO3D_LOGERRORF("UIC_StatusPanel() - LoseFocus !");

    UIPanel::LoseFocus();
    CloseCharacterSelection();
    GameContext::Get().ui_->SetFocusElement(0);
}

void UIC_StatusPanel::SetUIFactor(float factor)
{
    if (factor != uifactor_)
    {
        uifactor_ = factor;
        {
            const float defaultsizex = 130.f;
            const IntVector2& currentsize = panel_->GetSize();
            float currentscale = (float)currentsize.x_ / defaultsizex;
            if (currentscale != uifactor_)
                GameHelpers::SetScaleChildRecursive(panel_, uifactor_);
        }
        {
            const float defaultsizex = 230.f;
            const IntVector2& currentsize = characterList->GetSize();
            float currentscale = (float)currentsize.x_ / defaultsizex;
            if (currentscale != uifactor_)
                GameHelpers::SetScaleChildRecursive(characterList, uifactor_);
        }
    }
}

void UIC_StatusPanel::OnSetVisible()
{
    UIPanel::OnSetVisible();

    if (panel_->IsVisible())
    {
        if (characterButton)
            SubscribeToEvent(characterButton, E_RELEASED, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterSelectionStart));

        SubscribeToEvent(UI_ESCAPEPANEL, URHO3D_HANDLER(UIC_StatusPanel, OnEscapePanel));

        Player* player = static_cast<Player*>(user_);
        avatar_ = player->GetAvatar();
        if (avatar_)
        {
            SubscribeToEvent(avatar_, GOC_LIFEUPDATE, URHO3D_HANDLER(UIC_StatusPanel, OnLifeUpdate));
            SubscribeToEvent(avatar_, UI_MONEYUPDATED, URHO3D_HANDLER(UIC_StatusPanel, OnMoneyUpdate));
            SubscribeToEvent(avatar_, CHARACTERUPDATED, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterUpdated));
        }

        int controltype = GameContext::Get().playerState_[player->GetControlID()].controltype;
        if (controltype == CT_KEYBOARD || controltype == CT_JOYSTICK)
        {
            if (panel_->HasFocus())
            {
                URHO3D_LOGINFOF("UIC_StatusPanel() - OnSetVisible : is visible and has the focus ...");

                if (lastSelectedElement_ != panel_)
                    lastSelectedElement_ = panel_;
                
                lastSelector_ = Clamp(lastSelector_, statusChildRange_.x_, statusChildRange_.y_);
                
                SelectElement(lastSelectedElement_, panel_->GetChild(lastSelector_), lastSelector_);

                // Allow Selection Access to other UIC with the keyboard Arrows
                if (controltype == CT_KEYBOARD)
                {
                    SubscribeToEvent(E_KEYDOWN, URHO3D_HANDLER(UIC_StatusPanel, OnKey));
                }
                else
                {
                    SubscribeToEvent(E_JOYSTICKBUTTONDOWN, URHO3D_HANDLER(UIC_StatusPanel, OnKey));
                    SubscribeToEvent(E_JOYSTICKHATMOVE, URHO3D_HANDLER(UIC_StatusPanel, OnKey));
                #ifdef ALLOW_JOYSTICK_AXIS
                    SubscribeToEvent(E_JOYSTICKAXISMOVE, URHO3D_HANDLER(UIC_StatusPanel, OnKey));
                #endif
                }
            }
            else
            {
                URHO3D_LOGINFOF("UIC_StatusPanel() - OnSetVisible : is visible and has not the focus ...");

                SelectElement(0, 0);
                if (HasSubscribedToEvent(E_KEYDOWN))
                {
                    UnsubscribeFromEvent(E_KEYDOWN);
                }
                else
                {
                    UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
                    UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
                #ifdef ALLOW_JOYSTICK_AXIS
                    UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
                #endif                    
                }
            }
        }
    }
    else
    {
        if (characterButton)
            UnsubscribeFromEvent(characterButton, E_RELEASED);

        UnsubscribeFromEvent(UI_ESCAPEPANEL);

        if (avatar_)
        {
            UnsubscribeFromEvent(avatar_, GOC_LIFEUPDATE);
            UnsubscribeFromEvent(avatar_, UI_MONEYUPDATED);
            UnsubscribeFromEvent(avatar_, CHARACTERUPDATED);
        }

        if (HasSubscribedToEvent(E_KEYDOWN))
            UnsubscribeFromEvent(E_KEYDOWN);

        if (HasSubscribedToEvent(E_JOYSTICKBUTTONDOWN))
        {
            UnsubscribeFromEvent(E_JOYSTICKBUTTONDOWN);
            UnsubscribeFromEvent(E_JOYSTICKHATMOVE);
        #ifdef ALLOW_JOYSTICK_AXIS
            UnsubscribeFromEvent(E_JOYSTICKAXISMOVE);
        #endif            
        }

        SelectElement(0, 0);
        panel_->SetFocus(false);
    }

    if (characterList && characterList->IsVisible())
        CloseCharacterSelection();
}

void UIC_StatusPanel::OnRelease()
{
    URHO3D_LOGINFOF("UIC_StatusPanel() - OnRelease : ...");

    if (characterList && characterList->IsVisible())
        CloseCharacterSelection();

    Player* user = (Player*)static_cast<Player*>(user_);
    if (user)
    {
        UIPanel* panel = UIPanel::GetPanel(String("PlayerBag")+String(user->GetID()));
        if (panel)
            panel->SetVisible(false, true);
        panel = UIPanel::GetPanel(String("PlayerEqp")+String(user->GetID()));
        if (panel) panel->SetVisible(false, true);
        panel = UIPanel::GetPanel(String("PlayerCraft")+String(user->GetID()));
        if (panel) panel->SetVisible(false, true);
    }

    if (avatar_)
    {
        UnsubscribeFromEvent(avatar_, GOC_LIFEUPDATE);
        UnsubscribeFromEvent(avatar_, GOC_LIFEUPDATE);
    }

    UnsubscribeFromEvent(E_KEYDOWN);

    URHO3D_LOGINFOF("UIC_StatusPanel() - OnRelease : ... OK !");
}


void UIC_StatusPanel::Update()
{
    avatar_ = ((Player*)static_cast<Player*>(user_))->GetAvatar();

    UpdateCharacter();
    UpdateMoney();
    UpdateLife();
}

void UIC_StatusPanel::UpdateLife()
{
    if (avatar_)
    {
        // goclife change with network replication ... even if execute UpdateComponents
        GOC_Life* goclife = avatar_->GetComponent<GOC_Life>();
        if (!goclife)
            return;

        const LifeProps& lifeProps = goclife->Get();
        GOC_Life_Template* lifeTemplate = goclife->GetCurrentTemplate();

        URHO3D_LOGINFOF("UIC_StatusPanel() - UpdateLifeUI : avatar=%s(%u) lifeTemplate=%u life=%d energy=%f",
                        avatar_->GetName().CString(), avatar_->GetID(), lifeTemplate, lifeProps.life, lifeProps.energy);

//        URHO3D_LOGINFOF("UIC_StatusPanel() - UpdateLifeUI : playerID=%u avatar=%s life=%d/%d energy=%f/%f",
//                                    ID, avatar_->GetName().CString(), lifeProps.life, lifeTemplate->lifeMax, lifeProps.energy, lifeTemplate->energyMax);

        // Set Energy Bar
        if (healthBar)
            healthBar->SetSize(lifeTemplate ? int(lifeProps.energy / Max(lifeProps.energy, lifeTemplate->energyMax) * healthBar->GetMaxSize().x_) : healthBar->GetMaxSize().x_, healthBar->GetMaxSize().y_);

        // Set Num Visible Life Icons
        if (lifeIconZone)
        {
            if (lifeProps.life > 1)
                for (int i = 1; i < Min(lifeProps.life, NBLIFE_MAX); i++)
                    lifeIconZone->GetChild(i-1)->SetVisible(true);

            if (lifeProps.life < NBLIFE_MAX)
                for (int i = Max(lifeProps.life, 1) ; i < NBLIFE_MAX; i++)
                    lifeIconZone->GetChild(i-1)->SetVisible(false);
        }
    }
    else if (healthBar)
    {
        healthBar->SetSize(0, 18);
        for (int i = 0; i < NBLIFE_MAX; i++)
            lifeIconZone->GetChild(i)->SetVisible(false);
    }
}

void UIC_StatusPanel::UpdateMoney()
{
//    URHO3D_LOGINFOF("UIC_StatusPanel() - UpdateMoneyUI : player %u", GetID();
    if (avatar_ && moneyText)
    {
        unsigned int money = avatar_->GetComponent<GOC_Inventory>()->GetQuantityfor(COT::MONEY);
        moneyText->SetText(String(money));
    }
}

void UIC_StatusPanel::UpdateCharacter()
{
    if (!characterButton)
        return;

    Player* player = (Player*)static_cast<Player*>(user_);
    int avatarindex = player->GetAvatarIndex();
    int entityid = player->GetAvatar() && player->GetAvatar()->GetComponent<AnimatedSprite2D>()->GetSpriterInstance()
                   ? player->GetAvatar()->GetComponent<AnimatedSprite2D>()->GetSpriterInstance()->GetEntity()->id_ + 1 : 1;

    String name = GOT::GetType(GOT::GetControllableType(avatarindex)).Substring(String("GOT_Avatar_").Length());

    Sprite2D* uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(String("GOT_") + name);
    if (!uisprite)
        uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(String("GOT_") + name + String("_") + String(entityid));
    if (!uisprite)
        uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(name);
    if (!uisprite)
        uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite("slot40");

    URHO3D_LOGERRORF("UIC_StatusPanel() - UpdateCharacter : player character name = %s", name.CString());

    if (uisprite)
    {
        characterButton->SetTexture(uisprite->GetTexture());
//        characterButton->SetEnableDpiScale(false);
        characterButton->SetImageRect(uisprite->GetRectangle());
    }
}

void UIC_StatusPanel::OpenCharacterSelection()
{
    if (!characterList)
        return;

//    panel_->SetSize(130 * uifactor_, 130 * uifactor_);
//	characterList->SetSize(230 * uifactor_, 230 * uifactor_);

    panel_->AddChild(characterList);

    if (lastPosition_.x_ > GameContext::Get().screenwidth_ - characterList->GetSize().x_)
        characterList->SetPosition(IntVector2(40 * uifactor_ - characterList->GetSize().x_, 40 * uifactor_));
    else
        characterList->SetPosition(IntVector2(40 * uifactor_, 40 * uifactor_));

    URHO3D_LOGINFOF("UIC_StatusPanel() - OpenCharacterSelection : position=%s !", lastPosition_.ToString().CString());

    characterList->SetOpacity(0.99f);
    characterList->SetVisible(false);

    characterHovering_ = 0;
    characterHoveringIndex_ = -1;
    characterArrowDirection_ = 0;

    Player* player = static_cast<Player*>(user_);
    avatarIndex_ = player ? player->GetAvatarIndex() : 0;

    SubscribeToEvent(E_SCENEPOSTUPDATE, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterArrowHovering));
    SubscribeToEvent(characterList, E_HOVERBEGIN, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterListHoverStart));
    SubscribeToEvent(characterList, E_HOVEREND, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterListHoverEnd));

    UpdateCharacterList(avatarIndex_);
    UpdateCharacterSelection();
}

void UIC_StatusPanel::CloseCharacterSelection()
{
    if (!characterList || !characterList->IsVisible())
        return;

    // Hide
    characterList->SetVisible(false);
    GetSubsystem<UI>()->GetRoot()->AddChild(characterList);
//    panel_->SetSize(130 * uifactor_, 130 * uifactor_);

    UnsubscribeFromEvent(characterList, E_RELEASED);
    UnsubscribeFromEvent(characterList, E_HOVERBEGIN);
    UnsubscribeFromEvent(characterList, E_HOVEREND);
    UnsubscribeFromEvent(E_SCENEPOSTUPDATE);

    // Unsubscribe from events
    const Vector<SharedPtr<UIElement> >& children = characterList->GetChildren();
    for (Vector<SharedPtr<UIElement> >::ConstIterator it=children.Begin(); it!=children.End(); ++it)
    {
        UnsubscribeFromEvent(it->Get(), E_RELEASED);
        UnsubscribeFromEvent(it->Get(), E_HOVERBEGIN);
        UnsubscribeFromEvent(it->Get(), E_HOVEREND);
    }

    characterHovering_ = 0;

    URHO3D_LOGINFOF("UIC_StatusPanel() - CloseCharacterSelection : close characterlist !");
}


void UIC_StatusPanel::UpdateCharacterSelection(UIElement* selection)
{
    if (!characterList)
        return;

    const Vector<SharedPtr<UIElement> >& children = characterList->GetChildren();
    int selectid = 1;

    // Get the selected slot id
    if (selection)
    {
        for (int i=0; i < children.Size(); i++)
        {
            UIElement* elt = children[i];
            if (elt->GetChild(0) == selection)
            {
                selectid = ToInt(elt->GetName().Substring(5));
                break;
            }
        }
    }

//    URHO3D_LOGINFOF("UIC_StatusPanel() - UpdateCharacterSelection : selectid=%d", selectid);

    // Show the Hovering Button over the others
    for (int i=0; i < children.Size(); i++)
    {
        UIElement* elt = children[i];

        if (elt->GetName().StartsWith("I_"))
            continue;

        int slotid = ToInt(elt->GetName().Substring(5));
        int priority = characterList->GetPriority() + 100 - Abs(slotid-selectid) * 10;

        elt->SetPriority(priority);
        elt->GetChild(0)->SetPriority(priority+1);
//        URHO3D_LOGINFOF("UIC_StatusPanel() - UpdateCharacterSelection : slotid=%d name=%s(%s)... priority=%d", slotid, elt->GetName().CString(), elt->GetChild(0)->GetName().CString(), priority);
    }

    // Move the Selector
    UIElement* selector = characterList->GetChild(String("I_Select"));
    selector->SetPosition((30 + (selectid-1)*50 - 6) * uifactor_, 0);
    if (selection)
        selector->SetPriority(selection->GetPriority()+1);

    // Change the opacity => allow to Mark Dirty the UIElements for applying the priorities
    characterList->SetOpacity(0.99f);
}

void UIC_StatusPanel::UpdateCharacterList(int firstindex)
{
    if (!characterList || characterHoveringIndex_ == firstindex)
        return;

    characterHoveringIndex_ = firstindex;

    // Find The Activable Characters
    activableCharacters_.Clear();

    GOC_Life* goclife = avatar_ ? avatar_->GetComponent<GOC_Life>() : 0;
    if (!goclife)
        return;

    Player* player = static_cast<Player*>(user_);
    if (!player)
        return;

    const Vector<int>& controllables = player->GetAvatars();
    Vector<int> tempActivableAvatarIndexes;
    for (int i=0; i < controllables.Size(); i++)
    {
        unsigned index = controllables[i];

        // skip index
        if (index == avatarIndex_)
            continue;

        if (goclife->CheckChangeTemplate(GOT::GetControllableTemplate(index)->GetComponent<GOC_Life>()->GetTemplateName()))
            tempActivableAvatarIndexes.Push(index);
    }

    if (!tempActivableAvatarIndexes.Size())
        return;

    // Find the Avatar Index that is the nearest of firstindex
    int nearestIndex = 0;
    while (nearestIndex < tempActivableAvatarIndexes.Size() && tempActivableAvatarIndexes[nearestIndex] < firstindex)
        nearestIndex++;
    nearestIndex--;

    if (nearestIndex == -1)
        nearestIndex = 0;

    // Set The Activable Characters List
    Vector<int> activableAvatarIndexes;
    for (int i=nearestIndex; i < nearestIndex+3; i++)
    {
        if (i >= tempActivableAvatarIndexes.Size())
            break;

        activableAvatarIndexes.Push(tempActivableAvatarIndexes[i]);
        activableCharacters_.Push(GOT::GetControllableType(tempActivableAvatarIndexes[i]));
    }

    // Set the Characters List Panel
    for (int i=0; i < 3; i++)
    {
        UIElement* slot = characterList->GetChild(String("Slot_")+String(i+1));
        Button* slotbutton = (Button*)dynamic_cast<Button*>(slot->GetChild(0));

        slot->SetVisible(i < activableCharacters_.Size());

        if (slot->IsVisible())
        {
            slot->SetPosition((30 + i*50) * uifactor_, 0);
            slot->SetVar(GOA::GOT, activableCharacters_[i].Value());
            if (slotbutton)
            {
                const String& name = GOT::GetType(activableCharacters_[i]);
                String spritename = String("GOT_") + name.Substring(String("GOT_Avatar_").Length());
                Sprite2D* uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(spritename);
                if (!uisprite)
                {
                    spritename +=  String("_1");
                    uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(spritename);
                }
                if (!uisprite)
                {
                    spritename = name.Substring(String("GOT_Avatar_").Length());
                    uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite(spritename);
                    if (!uisprite)
                        uisprite = GameContext::Get().spriteSheets_[UIEQUIPMENT]->GetSprite("slot40");
                }

                slotbutton->SetName(name);
                if (uisprite)
                {
                    slotbutton->SetTexture(uisprite->GetTexture());
                    slotbutton->SetImageRect(uisprite->GetRectangle());
                }

                // Subscribe To Buttons Events
                SubscribeToEvent(slotbutton, E_RELEASED, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterSelectionEnd));
                SubscribeToEvent(slotbutton, E_HOVERBEGIN, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterHoverStart));
                SubscribeToEvent(slotbutton, E_HOVEREND, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterHoverEnd));

//                URHO3D_LOGINFOF("UIC_StatusPanel() - UpdateCharacterList : ... activablecharacter(%d)=%s subscribe to HOVERING", i, GOT::GetType(activableCharacters_[i]).CString());
            }
        }
        else
        {
            slot->SetVar(GOA::GOT, 0U);
            if (slotbutton)
            {
                // Unsubscribe from Buttons Events
                UnsubscribeFromEvent(slotbutton, E_RELEASED);
                UnsubscribeFromEvent(slotbutton, E_HOVERBEGIN);
                UnsubscribeFromEvent(slotbutton, E_HOVEREND);
            }
        }
    }

    // Set Arrows
    UIElement* slotarrowleft = characterList->GetChild(String("I_ArrowL"));
    if (slotarrowleft)
    {
        if (tempActivableAvatarIndexes.Size() > 3 && activableAvatarIndexes[0] >= tempActivableAvatarIndexes[0])
        {
//            slotarrowleft->SetPosition(4 * uifactor_, 0);
            slotarrowleft->SetVisible(true);
            slotarrowleft->SetPosition((50 + 25) * uifactor_, 60*uifactor_);
            SubscribeToEvent(slotarrowleft, E_HOVERBEGIN, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterArrowHoverStart));
            SubscribeToEvent(slotarrowleft, E_HOVEREND, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterArrowHoverEnd));
        }
        else
        {
            slotarrowleft->SetVisible(false);
            UnsubscribeFromEvent(slotarrowleft, E_HOVERBEGIN);
            UnsubscribeFromEvent(slotarrowleft, E_HOVEREND);
        }
    }
    UIElement* slotarrowright = characterList->GetChild(String("I_ArrowR"));
    if (slotarrowright)
    {
        if (tempActivableAvatarIndexes.Size() > 3 && activableAvatarIndexes.Back() <= tempActivableAvatarIndexes.Back())
        {
//            slotarrowright->SetPosition((30 + 2*50 + 70 - 3) * uifactor_, 0);
            slotarrowright->SetVisible(true);
            slotarrowright->SetPosition((30 + 50 + 25) * uifactor_, 60*uifactor_);
            SubscribeToEvent(slotarrowright, E_HOVERBEGIN, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterArrowHoverStart));
            SubscribeToEvent(slotarrowright, E_HOVEREND, URHO3D_HANDLER(UIC_StatusPanel, OnCharacterArrowHoverEnd));
        }
        else
        {
            slotarrowright->SetVisible(false);
            UnsubscribeFromEvent(slotarrowright, E_HOVERBEGIN);
            UnsubscribeFromEvent(slotarrowright, E_HOVEREND);
        }
    }

    // Show or / Hide if no activableCharacters
    characterList->SetPriority(characterList->GetParent()->GetPriority()+10);
    characterList->SetVisible(activableCharacters_.Size());
}

void UIC_StatusPanel::SelectElement(UIElement* element, UIElement* focuselement, int focusindex)
{
    if (focusedElement_)
        focusedElement_->SetKeepHovering(false);

    lastSelectedElement_ = selectedElement_;
    lastSelector_ = selector_;

    selectedElement_ = element;
    selector_ = focusindex;
    focusedElement_ = focuselement;

    if (focusedElement_)
        focusedElement_->SetKeepHovering(true);

    URHO3D_LOGINFOF("UIC_StatusPanel() - SelectElement : element=%s selector=%d elementfocused=%s!", selectedElement_ ? selectedElement_->GetName().CString() : "none", selector_, focusedElement_ ? focusedElement_->GetName().CString() : "none");
}


void UIC_StatusPanel::OnLifeUpdate(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("UIC_StatusPanel() - OnLifeUpdate : Node=%u", avatar_ ? avatar_->GetID() : 0);
    UpdateLife();
}

void UIC_StatusPanel::OnMoneyUpdate(StringHash eventType, VariantMap& eventData)
{
    UpdateMoney();
}

void UIC_StatusPanel::OnCharacterUpdated(StringHash eventType, VariantMap& eventData)
{
    Update();
}

void UIC_StatusPanel::OnCharacterSelectionStart(StringHash eventType, VariantMap& eventData)
{
    // Cancel on Reclick
    if (characterList && characterList->IsVisible())
    {
        CloseCharacterSelection();
        return;
    }

    OpenCharacterSelection();
}

void UIC_StatusPanel::OnCharacterSelectionEnd(StringHash eventType, VariantMap& eventData)
{
    if (!characterList)
        return;

    URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterSelectionEnd : ...");
    CloseCharacterSelection();

    Button* button = dynamic_cast<Button*>(context_->GetEventSender());
    if (!button)
        return;

    // Apply Avatar Selection
    ((Player*)static_cast<Player*>(user_))->ChangeAvatar(GOT::GetControllableIndex(StringHash(button->GetName())));

    Update();
}

void UIC_StatusPanel::OnCharacterListHoverStart(StringHash eventType, VariantMap& eventData)
{
//    URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterListHoverStart : ... characterHovering_=%d", characterHovering_);
}

void UIC_StatusPanel::OnCharacterListHoverEnd(StringHash eventType, VariantMap& eventData)
{
    if (!characterList)
        return;

    if (!characterHovering_)
    {
        URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterListHoverEnd : ... characterHovering_=%d", characterHovering_);
        CloseCharacterSelection();
    }
}

void UIC_StatusPanel::OnCharacterHoverStart(StringHash eventType, VariantMap& eventData)
{
    if (!characterList)
        return;

    Button* selection = dynamic_cast<Button*>(context_->GetEventSender());

    UpdateCharacterSelection(selection);

    characterHovering_++;

//    URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterHoverStart ... characterHovering_=%d", characterHovering_);
}

void UIC_StatusPanel::OnCharacterHoverEnd(StringHash eventType, VariantMap& eventData)
{
    if (!characterList)
        return;

    if (!characterHovering_)
        return;

    Button* button = dynamic_cast<Button*>(context_->GetEventSender());
    if (!button)
        return;

    // Show the Hovering Button over the others
    int index = 1;

    const Vector<SharedPtr<UIElement> >& children = characterList->GetChildren();
    for (int i=0; i < children.Size(); i++)
    {
        UIElement* elt = children[i];
        if (elt->GetChild(0) == button)
        {
            index = ToInt(elt->GetName().Substring(5));
            break;
        }
    }

    characterHovering_--;

//    URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterHoverEnd : index=%d characterHovering_=%d", index, characterHovering_);
}

void UIC_StatusPanel::OnCharacterArrowHoverStart(StringHash eventType, VariantMap& eventData)
{
    if (!characterList)
        return;

    //characterArrowDirection_ = 0;

    char c = dynamic_cast<Button*>(context_->GetEventSender())->GetName().At(7);
    if (c == 'R' && characterArrowDirection_ != 1)
        characterArrowDirection_ = 1;
    else if (c == 'L' && characterArrowDirection_ != -1)
        characterArrowDirection_ = -1;

    characterHovering_++;

//    URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterArrowHoverStart : ... arrow=%c dir=%d characterHovering_=%d", c, characterArrowDirection_, characterHovering_);
}

void UIC_StatusPanel::OnCharacterArrowHoverEnd(StringHash eventType, VariantMap& eventData)
{
    if (!characterList)
        return;

    char c = dynamic_cast<Button*>(context_->GetEventSender())->GetName().At(7);
    if ((c == 'R' && characterArrowDirection_ == 1) || (c == 'L' && characterArrowDirection_ == -1))
        characterArrowDirection_ = 0;

    characterHovering_--;

//    URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterArrowHoverEnd : ... arrow=%c dir=%d characterHovering_=%d", c, characterArrowDirection_, characterHovering_);
}

void UIC_StatusPanel::OnCharacterArrowHovering(StringHash eventType, VariantMap& eventData)
{
    if (!characterList || !characterArrowDirection_ || !characterHovering_)
        return;

    if (!activableCharacters_.Size())
        return;

    if (characterArrowTimer_->GetMSec(false) < updateDelays_[GameContext::Get().gameConfig_.uiUpdateDelay_])
        return;

    characterArrowTimer_->Reset();

    int slotindex = characterArrowDirection_ < 0 ? 1 : Min(activableCharacters_.Size(), 3);
    UIElement* slot = characterList->GetChild(String("Slot_")+String(slotindex));
    StringHash type(slot->GetVar(GOA::GOT).GetUInt());
    if (type)
    {
        int index = GOT::GetControllableIndex(type);

//        URHO3D_LOGINFOF("UIC_StatusPanel() - OnCharacterArrowHovering : characterArrowDirection_=%d characterHoveringIndex_=%d index=%d ...", characterArrowDirection_, characterHoveringIndex_, index);

        if (index != characterHoveringIndex_ && ((index > 0 && characterArrowDirection_ < 0) || (index < GOT::GetControllables().Size()-1 && characterArrowDirection_ > 0)))
        {
            UpdateCharacterList(index);
            UpdateCharacterSelection(slot->GetChild(0));

            if (!characterHoveringIndex_)
                characterArrowDirection_ = 0;
        }
        else
        {
            characterArrowDirection_ = 0;
        }
    }
    else
    {
        characterArrowDirection_ = 0;
    }
}

void UIC_StatusPanel::OnDrag(StringHash eventType, VariantMap& eventData)
{
    UIPanel::OnDrag(eventType, eventData);

    if (eventType == E_DRAGBEGIN)
    {
        // Cancel on Reclick
        if (characterList && characterList->IsVisible())
        {
            URHO3D_LOGINFOF("UIC_StatusPanel() - OnDrag : ... close selection !");
            CloseCharacterSelection();
            SelectElement(GetElement(), 0);
        }
    }
}

void UIC_StatusPanel::OnEscapePanel(StringHash eventType, VariantMap& eventData)
{
    if (characterList && characterList->IsVisible())
    {
        URHO3D_LOGINFOF("UIC_StatusPanel() - OnEscapePanel : ... close selection !");
        CloseCharacterSelection();
        SelectElement(GetElement(), GetElement()->GetChild(statusChildRange_.x_), statusChildRange_.x_);
    }
    else if (selectedElement_ == GetElement())
    {
        GetElement()->SetFocus(false);

        if (IsVisible())
            ToggleVisible();
    }
    else
    {
        GetSubsystem<UI>()->SetFocusElement(0);
    }
}

void UIC_StatusPanel::OnKey(StringHash eventType, VariantMap& eventData)
{
    Player* player = static_cast<Player*>(user_);
    if (player->GetFocusPanel() != this)
        return;

    int scancode = GetKeyFromEvent(player->GetControlID(), eventType, eventData);
    if (!scancode)
        return;

    const PODVector<int>& keymap = GameContext::Get().keysMap_[player->GetControlID()];

    if (selectedElement_ == GetElement())
    {
        if (scancode == keymap[ACTION_DOWN])
            selectordirection_ = -1;
        else if (scancode == keymap[ACTION_UP])
            selectordirection_ = 1;
        else 
        {
            if (scancode == keymap[ACTION_INTERACT] && focusedElement_)
                focusedElement_->SendEvent(E_RELEASED);
            return;
        }

        int newselection = selector_ + selectordirection_;
        if (newselection >= statusChildRange_.x_ && newselection <= statusChildRange_.y_)
        {
            SelectElement(GetElement(), GetElement()->GetChild(newselection), newselection);
            URHO3D_LOGINFOF("UIC_StatusPanel() - OnKey : selector=%d in statuspanel ...", selector_);
        }
        else
        {
            characterArrowDirection_ = selectordirection_;
            OpenCharacterSelection();
            SelectElement(characterList, characterList->GetChild(String("Slot_1")), 0);

            URHO3D_LOGINFOF("UIC_StatusPanel() - OnKey : selector=%d open characterlist ... characterHoveringIndex_%d", selector_, characterHoveringIndex_);
        }
    }
    else if (selectedElement_ == characterList)
    {
        if (scancode == keymap[ACTION_LEFT])
            selectordirection_ = -1;
        else if (scancode == keymap[ACTION_RIGHT])
            selectordirection_ = 1;
        else
        {       
            if (scancode == keymap[ACTION_DOWN] && abilityPanel_ && abilityPanel_->CanFocus()) // Close and Open AbilityPanel
            {
                CloseCharacterSelection();
                SelectElement(abilityPanel_->GetElement(), abilityPanel_->GetElement(), 0);             
                abilityPanel_->GainFocus();  
                player->SetFocusPanel(ABILITYPANEL);
            }            
            else if (scancode == keymap[ACTION_UP] || scancode == keymap[ACTION_DOWN]) // Close and Focus on StatusPanel
            {
                CloseCharacterSelection();
                SelectElement(GetElement(), GetElement()->GetChild(statusChildRange_.x_), statusChildRange_.x_);
            }
            else if (scancode == keymap[ACTION_INTERACT]) // Apply Avatar Selection And Close/UnFocus
            {
                player->ChangeAvatar(GOT::GetControllableIndex(StringHash(focusedElement_->GetChild(0)->GetName())));
                player->SetFocusPanel(-1);
                LoseFocus();                
            }
            return;
        }

        characterArrowDirection_ = selectordirection_;

        URHO3D_LOGINFOF("UIC_StatusPanel() - OnKey : in characterlist ... selector=%d dir=%d ...", selector_, selectordirection_);

        if ((selector_ == 0 && characterArrowDirection_ == -1) || (selector_ == 2 && characterArrowDirection_ == 1))
        {
            URHO3D_LOGINFOF("UIC_StatusPanel() - OnKey : in characterlist ... arrow (selector=%d dir=%d)", selector_, selectordirection_);
            characterHovering_++;
            OnCharacterArrowHovering(eventType, eventData);
            characterHovering_--;      
        }
        else
        {
            selector_ = Clamp(selector_+selectordirection_, 0, 2);
            focusedElement_ = characterList->GetChild(String("Slot_")+String(selector_+1));
            URHO3D_LOGINFOF("UIC_StatusPanel() - OnKey : in characterlist ... selector=%d dir=%d", selector_, selectordirection_);
            UpdateCharacterSelection(focusedElement_->GetChild(0));
        }

        characterArrowDirection_ = selectordirection_ = 0;
    }
}
