#pragma once

#include <Urho3D/Core/Object.h>

//#define URHO3D_EVENT(eventID, eventName) static const Urho3D::StringHash eventID(#eventName); namespace eventName
//#define URHO3D_PARAM(paramID, paramName) static const Urho3D::StringHash paramID(#paramName)
//#define URHO3D_HANDLER(className, function) (new Urho3D::EventHandlerImpl<className>(this, &className::function))
//#define URHO3D_HANDLER_USERDATA(className, function, userData) (new Urho3D::EventHandlerImpl<className>(this, &className::function, userData))

namespace Urho3D
{
/// Game
URHO3D_EVENT(GAME_SCREENRESIZED, Game_ScreenResized) { }
URHO3D_EVENT(GAME_REMOVESCENE, Game_RemoveScene) { }
URHO3D_EVENT(GAME_PRELOADINGFINISHED, Game_PreloadingFinished) { }
URHO3D_EVENT(GAME_SOUNDVOLUMECHANGED,  Game_SoundVolumeEnabled) { }
URHO3D_EVENT(GAME_START, Game_Start) { }
URHO3D_EVENT(GAME_STOP, Game_Stop) { }
URHO3D_EVENT(GAME_WIN, Game_Win) { }
URHO3D_EVENT(GAME_OVER, Game_Over) { }
URHO3D_EVENT(GAME_EXIT, Game_Exit) { }
URHO3D_EVENT(GAME_PLAYERDIED, Game_PlayerDied) { }
URHO3D_EVENT(GAME_WORLDSNAPSHOTSAVED, Game_WorldSnapShotSaved) { }

/// PlayState, TextMessage
URHO3D_EVENT(TEXTMESSAGE_CLEAN, TextMessage_Clean) { }

URHO3D_EVENT(OBJECT_SELFDESTROY, Object_SelfDestroy) { }

/// OptionState
URHO3D_EVENT(GAME_OPTIONSFRAME_OPEN,  Game_OptionsFrameOpen) { }
URHO3D_EVENT(GAME_OPTIONSFRAME_CLOSED,  Game_OptionsFrameClosed) { }

/// PlayState, Player
URHO3D_EVENT(GO_SELECTED, Go_Selected)
{
    URHO3D_PARAM(GO_ID, GoID);             // ID of the GO
    URHO3D_PARAM(GO_TYPE, GoType);         // Type of controller (player,AI,none)
    URHO3D_PARAM(GO_ACTION, GoAction);     // Type of Action to Release by AI
}
URHO3D_EVENT(GO_TRIGCLICKED, Go_TrigClicked) { }
URHO3D_EVENT(NET_TRIGCLICKED, Net_TrigClicked) { }

/// World2D

/// => Sender : ObjectTiled
/// => Subscribers : World2D => reload all chunks
URHO3D_EVENT(WORLD_DIRTY, World_Dirty) { }
/// => Sender : sPlay
/// => Subscribers : ??? => TODO
URHO3D_EVENT(WORLD_FINISHLOADING, World_FinishLoading) { }
/// => Sender : Map, BodyExploder, SimpleActor, Collectable, Animator2D
/// => Subscribers : GOC_Destroyer
URHO3D_EVENT(WORLD_ENTITYCREATE, World_EntityCreate) { }
/// => Sender : Collectable, Animator2D
/// => Subscribers : GOC_Destroyer
URHO3D_EVENT(WORLD_ENTITYDESTROY, World_EntityDestroy) { }
/// => Sender : GOC_Destroyer
/// => Subscribers : UIC_MiniMap
URHO3D_EVENT(WORLD_ENTITYUPDATE, World_EntityUpdate)
{
    URHO3D_PARAM(GO_STATE, GoState);  // 0 = create ; 1 = destroy
}
/// => Sender : Map, MapWorld, MapStorage, ViewManager
/// => Subscribers : UIC_MiniMap
URHO3D_EVENT(WORLD_MAPUPDATE, World_MapUpdate) { }
/// => Sender : World2D
/// => Subscribers : DrawableScroller, ScrollingShape
URHO3D_EVENT(WORLD_CAMERACHANGED, World_CameraChanged) { }
/// => Sender : Map
/// => Subscribers : UIC_MiniMap
URHO3D_EVENT(MAP_UPDATE, Map_Update) { }

URHO3D_EVENT(MAP_ADDFURNITURE, Map_AddFurniture) { }
URHO3D_EVENT(MAP_ADDCOLLECTABLE, Map_AddCollectable) { }

/// Weather
URHO3D_EVENT(WEATHER_DAWN, Weather_Dawn) { }
URHO3D_EVENT(WEATHER_TWILIGHT, Weather_Twilight) { }

/// Network
URHO3D_EVENT(NET_MODECHANGED, Net_ModeChanged) { }
URHO3D_EVENT(NET_SERVERSEEDTIME, Net_ServerSeedTime)
{
    URHO3D_PARAM(P_SEEDTIME, SeedTime);        // Server Seed Time (Unsigned)
}
URHO3D_EVENT(NET_GAMESTATUSCHANGED, Net_GameStatusChanged)
{
    URHO3D_PARAM(P_TIMESTAMP, TimeStamp);           // TimeStamp
    URHO3D_PARAM(P_STATUS, PlayStatus);              // PlayStatus (Int)
    URHO3D_PARAM(P_GAMEMODE, GameMode);              // GameMode (Int) (arena, world ...)
    URHO3D_PARAM(P_CLIENTID, ClientID);             // Client ID of this client (Int) (for setting GameNetwork::clientID_
    URHO3D_PARAM(P_NUMPLAYERS, NumPlayers);         // Num Players (Int)
}
URHO3D_EVENT(NET_OBJECTCOMMAND, Net_ObjectCommand)
{
    URHO3D_PARAM(P_COMMAND, NetCommand);                                    // ServerCommand (Int)
    URHO3D_PARAM(P_NODEID, NetNodeID);                                      // ServerNodeID (Unsigned)
    URHO3D_PARAM(P_NODEIDFROM, NetNodeIDFrom);                              // ServerNodeFromID (Unsigned)
    URHO3D_PARAM(P_NODEPTRFROM, NetNodePtrFrom);                            // Node Ptr From (Pointer)
    URHO3D_PARAM(P_NODEISENABLED, NetNodeIsEnabled);                        // Bool
    URHO3D_PARAM(P_CLIENTID, ClientID);                                     // Int
    URHO3D_PARAM(P_CLIENTOBJECTTYPE, ClientObjectType);                     // Unsigned
    URHO3D_PARAM(P_CLIENTOBJECTENTITYID, ClientObjectEntityID);             // Unsigned char
    URHO3D_PARAM(P_CLIENTOBJECTVIEWZ, ClientObjectViewZ);                   // Unsigned
    URHO3D_PARAM(P_CLIENTOBJECTFACTION, ClientObjectFaction);               // Unsigned
    URHO3D_PARAM(P_CLIENTOBJECTPOSITION, ClientObjectPosition);             // Vector2
    URHO3D_PARAM(P_CLIENTOBJECTVELOCITY, ClientObjectVelocity);             // Vector2
    URHO3D_PARAM(P_CLIENTOBJECTDIRECTION, ClientObjectDirection);           // Float
    URHO3D_PARAM(P_SLOTPARTFROMTYPE, ClientSlotPartFromType);               // uint
    URHO3D_PARAM(P_SLOTQUANTITY, ClientSlotQuantity);                       // unsigned : Slot Quantity
    URHO3D_PARAM(P_SLOTEFFECT, ClientSlotEffect);                           // int : Slot Effect
    URHO3D_PARAM(P_SLOTSPRITE, ClientSlotSprite);                           // uint : Slot Hash of the sprite
    URHO3D_PARAM(P_SLOTSCALE, ClientSlotScale);                             // float : Slot scale x
    URHO3D_PARAM(P_SLOTCOLOR, ClientSlotColor);                             // uint : Slot color
    URHO3D_PARAM(P_INVENTORYTEMPLATE, ClientInventoryTemplate);             // uint : Inventory Template HashName
    URHO3D_PARAM(P_INVENTORYSLOTS, ClientInventorySlots);                   // VariantVector : All the Equipements each entry of the VariantVector is a uint (got)
    URHO3D_PARAM(P_INVENTORYITEMTYPE, ClientItemType);                      // uint : TYPE of the Item
    URHO3D_PARAM(P_INVENTORYIDSLOT, ClientInventoryIdSlot);                 // uint : ID of the slot
    URHO3D_PARAM(P_INVENTORYDROPMODE, ClientInventoryDropMode);             // int : Drop Mode
    URHO3D_PARAM(P_TILEOP, NetTileOp);                                      // uint : Remove(0) or Add(1)
    URHO3D_PARAM(P_TILEINDEX, NetTileIndex);                                // uint : TileIndex
    URHO3D_PARAM(P_TILEMAP, NetTileMap);                                    // uint : hash of shortIntVector2
    URHO3D_PARAM(P_TILEVIEW, NetTileView);                                  // uint : viewid
    URHO3D_PARAM(P_DATAS, NetDatas);                                        // VariantVector
    URHO3D_PARAM(P_SERVEROBJECTS, NetServerObjects);                        // Buffer
}
/// GOC_Animator, GOC_Destroyer
/// CHANGE STATE Event
URHO3D_EVENT(GO_CHANGESTATE, Go_ChangeState)
{
    URHO3D_PARAM(GO_ID, GoID);         // ID of the GO that animState change
    URHO3D_PARAM(GO_STATE, GoState);   // unsigned : new State
}
/// APPEAR Event
/// => Sender : GOC_Destroyer
/// => Subscribers : GOManager, World2D
URHO3D_EVENT(GO_APPEAR, Go_Appear)
{
    URHO3D_PARAM(GO_ID, GoID);         // ID of the node GO appeared
    URHO3D_PARAM(GO_TYPE, GoType);     // Type of controller (player,AI,none)
    URHO3D_PARAM(GO_MAINCONTROL, GoMainControl);     // AI mainControl
    URHO3D_PARAM(GO_MAP, GoMap);
    URHO3D_PARAM(GO_TILE, GoTile);
}
/// CHANGEMAP Event
/// => Sender : GOC_Destroyer
/// => Subscribers : World2D
URHO3D_EVENT(GO_CHANGEMAP, Go_ChangeMap)
{
    URHO3D_PARAM(GO_ID, GoID);         // ID of the node GO appeared
    URHO3D_PARAM(GO_TYPE, GoType);     // Type of controller (player,AI,none)
    URHO3D_PARAM(GO_MAPFROM, MapFrom);
    URHO3D_PARAM(GO_MAPTO, MapTo);
}
/// DESTROY Event
/// => Sender : GOC_Animator2D, GOC_Destroyer, GOC_Collectable,
/// => Subscribers : GOManager, World2D, GOC_Destroyer
URHO3D_EVENT(GO_DESTROY, Go_Destroy)
{
    URHO3D_PARAM(GO_ID, GoID);         // ID of the GO To Destroy
    URHO3D_PARAM(GO_TYPE, GoType);     // Type of controller (player,AI,none)
    URHO3D_PARAM(GO_MAP, GoMap);
    URHO3D_PARAM(GO_TILE, GoTile);
    URHO3D_PARAM(GO_PTR, GoPtr);       // Node*
}
/// MOUNT Event
/// => Sender : Player, GOB_MountOn
/// => Subscribers : GameNetwork
URHO3D_EVENT(GO_MOUNTEDON, Go_MountedOn) { } // the other Params see NET_OBJECTCOMMAND
/// ITEMS STORAGE Event
/// => Sender : GOC_DropZone
/// => Subscribers : GameNetwork
URHO3D_EVENT(GO_STORAGECHANGED, Go_StorageChanged)
{
    URHO3D_PARAM(GO_ACTIONTYPE, GoActionType);                  // 1=AddItem, 2=ThrowItems, 3=SetStorage
    // the other Params see NET_OBJECTCOMMAND
}

/// KILLER Entity
/// is sent to the killer node
/// => Sender : GOC_Life
/// => Subscribers : class Actor, Player
URHO3D_EVENT(GO_KILLER, Go_Killer)
{
    URHO3D_PARAM(GO_ID, GoID);         // ID of the GO Killer
    URHO3D_PARAM(GO_DEAD, GoDead);     // ID of the GO Dead
}

/// GOC_Controller
/// Control Update Event
/// => Subscribers : GOC_Move2D
URHO3D_EVENT(GOC_CONTROLUPDATE, ControlUpdate) { }
/// Move Event
URHO3D_EVENT(GOC_CONTROLMOVE, ControlMove) { }
/// Action0 Press Event : Jump
URHO3D_EVENT(GOC_CONTROLACTION0, ControlAction0) { }
/// Action1 Press Event : Fire
/// => Subscribers : GOC_Attack
URHO3D_EVENT(GOC_CONTROLACTION1, ControlAction1) { }
URHO3D_EVENT(GOC_CONTROLACTION1HOLD, ControlAction1Hold) { }
/// Action2 Press Event : Fire2
URHO3D_EVENT(GOC_CONTROLACTION2, ControlAction2) { }
URHO3D_EVENT(GOC_CONTROLACTION2HOLD, ControlAction2Hold) { }
URHO3D_EVENT(GOC_CONTROLACTIONSTOP, ControlActionStop) { }
/// Action3 Press Event : Fire3
URHO3D_EVENT(GOC_CONTROLACTION3, ControlAction3) { }
/// Action Status Press Event
URHO3D_EVENT(GOC_CONTROLACTION_STATUS, ControlActionStatus) { }
URHO3D_EVENT(GOC_CONTROLACTION_PREVFOCUSPANEL, ControlActionPrevFocusPanel) { }
URHO3D_EVENT(GOC_CONTROLACTION_NEXTFOCUSPANEL, ControlActionNextFocusPanel) { }

/// Change Controller Event
URHO3D_EVENT(GOC_CONTROLLERCHANGE, ControllerChange)
{
    URHO3D_PARAM(NODE_ID, Node_ID);             // Node ID
    URHO3D_PARAM(GO_TYPE, GoType);              // Type of controller (player,AI,none)
    URHO3D_PARAM(GO_MAINCONTROL, GoMainControl);     // AI mainControl
    URHO3D_PARAM(NEWCTRL_PTR, NewControl);      // Pointer of new GOC_Controller
}
/// GOC_Life
/// LIFE RESTORE Event
URHO3D_EVENT(GOC_LIFERESTORE, GOC_Life_Restore) { }
/// LIFE DECREASE Event
URHO3D_EVENT(GOC_LIFEDEC, GOC_Life_Decrease) { }
/// LIFE UPDATE Event
URHO3D_EVENT(GOC_LIFEUPDATE, GOC_Life_Update) { }
/// DEAD Event
/// => Sender : GOC_Life, GOC_Destroyer
/// => Subscribers : GOManager, GOC_Inventory(Drop), GOC_Collectable(Drop), Player(updateUI),
/// ==> sPlay(updateScore), GOC_PlayerController(StopUpdateInput), GOC_AIController(StopUpdateAI),
/// ==> GOC_BodyExploder(Explode), GOC_Animator2D(ChangeAnimation)
URHO3D_EVENT(GOC_LIFEDEAD, GOC_Life_Dead) { }
URHO3D_EVENT(GOC_LIFEEVENTS, GOC_Life_Events)
{
    URHO3D_PARAM(GO_ID, GoID);                  // ID of the Node
    URHO3D_PARAM(GO_TYPE, GoType);         // Type of controller (player,AI,none)
    URHO3D_PARAM(GO_KILLER, GoKiller);      // ID of the GO killer
    URHO3D_PARAM(GO_MAINCONTROL, GoMainControl);
    URHO3D_PARAM(GO_WORLDCONTACTPOINT, GoWorldContactPoint);   // Vector2 (Last World contact point)
}

/// GOC_Collide2D
/// GO Touch Walls Event
URHO3D_EVENT(GO_COLLIDEGROUND, Go_CollideGround) { }
URHO3D_EVENT(COLLIDEWALLBEGIN, CollideWallBegin)
{
    URHO3D_PARAM(WALLCONTACTTYPE, WallContactType);         // int WallType (Ground,Border,Roof)
    URHO3D_PARAM(WALLSIDE, WallSide);                       // when Climb if > 0 it's a Wall on Right Side
}
URHO3D_EVENT(COLLIDEWALLEND, CollideWallEnd)
{
    URHO3D_PARAM(ENDCONTACTWALLTYPE, EndContactWallType);         // int WallType (Ground,Border,Roof)
    URHO3D_PARAM(WALLCONTACTNUM, WallContactNum);           // qty of contacts (if <= 0 no more contact)
}
/// GO Touch Enemy Event
URHO3D_EVENT(GO_COLLIDEATTACK, Go_CollideAttack)
{
    URHO3D_PARAM(GO_ID, GoID);         // ID of the node GO
    URHO3D_PARAM(GO_ENEMY, GoEnemy);   // ID of the node GO Enemy touched
    URHO3D_PARAM(GO_WORLDCONTACTPOINT, GoWorldContactPoint);   // Vector2 (World contact point)
}
/// WaterLayer
URHO3D_EVENT(GO_COLLIDEFLUID, Go_CollideFluid)
{
    URHO3D_PARAM(GO_WETTED, GoWetted);
}
/// GOC_Collide2D, GOC_ZoneEffect
/// GO Receive Effect Event
URHO3D_EVENT(GO_RECEIVEEFFECT, Go_ReceiveEffect)
{
    URHO3D_PARAM(GO_RECEIVERID, GoReceiverID);         // UInt : ID of the node GO Receiver
    URHO3D_PARAM(GO_SENDERID, GoSenderID);             // UInt : ID of the node GO Sender
    URHO3D_PARAM(GO_SENDERFACTION, GoSenderFaction);   // UInt : Faction of the node GO Sender
    URHO3D_PARAM(GO_EFFECTTYPE1, GO_EFFECTTYPE1);
    URHO3D_PARAM(GO_EFFECTQTY1, GO_EFFECTQTY1);
    URHO3D_PARAM(GO_EFFECTTYPE2, GO_EFFECTTYPE2);
    URHO3D_PARAM(GO_EFFECTQTY2, GO_EFFECTQTY2);
    URHO3D_PARAM(GO_EFFECTTYPE3, GO_EFFECTTYPE3);
    URHO3D_PARAM(GO_EFFECTQTY3, GO_EFFECTQTY3);
    URHO3D_PARAM(GO_WORLDCONTACTPOINT, GoWorldContactPoint);   // Vector2 (World contact point)
}
/// MAP CollisionShape, GOC_Collide2D subscribe to it when in contact
URHO3D_EVENT(MAPTILEREMOVED, MapTileRemoved)                // Sender is a CollisionShape
{
    URHO3D_PARAM(MAPPOINT, MapPoint);                       // map point
    URHO3D_PARAM(MAPTILEINDEX, MapTileIndex);               // index tile
}
/// GOC_Move2D
/// GO Change Direction
URHO3D_EVENT(GO_CHANGEDIRECTION, Go_ChangeDirection) { }
URHO3D_EVENT(GO_UPDATEDIRECTION, Go_UpdateDirection) { }

/// GOC_Detector
URHO3D_EVENT(GO_DETECTOR, Go_Detector)
{
    URHO3D_PARAM(GO_GETTER, GoGetter);
    URHO3D_PARAM(GO_IMPACTPOINT, GoImpactPoint);

}
URHO3D_EVENT(GO_DETECTORPLAYERIN, Go_DetectorPlayerIn) { }
URHO3D_EVENT(GO_DETECTORPLAYEROFF, Go_DetectorPlayerOff) { }
URHO3D_EVENT(GO_DETECTORSWITCHVIEWIN, Go_DetectorSwitchViewIn) { }
URHO3D_EVENT(GO_DETECTORSWITCHVIEWOUT, Go_DetectorSwitchViewOut) { }

/// GOC_Inventory
/// GO_INVENTORYGET
/// => Sender : GOC_Inventory, GOC_Collectable
/// => Subscribers : Player
URHO3D_EVENT(GO_INVENTORYGET, Go_InventoryGet)
{
    URHO3D_PARAM(GO_GIVER, GoGiver);                                // ID of the node GO Giver (which has a GOC_Collectable or GOC_Inventory component)
    URHO3D_PARAM(GO_GETTER, GoGetter);                              // ID of the node GO Getter (which has a GOC_Inventory component)
    URHO3D_PARAM(GO_POSITION, GoPosition);                          // Variant (Vector3 or IntVector2) world position of item
    URHO3D_PARAM(GO_RESOURCEREF, GoResourceRef);                    // sprite resource ref
    URHO3D_PARAM(GO_IDSLOTSRC, Go_IdSlotSrc);                       // ID of the slot source (use in network AddItem)
    URHO3D_PARAM(GO_IDSLOT, Go_IdSlot);                             // ID of the slot destination (use in player for transfertoUI)
    URHO3D_PARAM(GO_QUANTITY, GoQuantity);                          // unsigned quantity of collectable
}
URHO3D_EVENT(GO_INVENTORYEMPTY, Go_InventoryEmpty) { }
URHO3D_EVENT(GO_INVENTORYFULL, Go_InventoryFull) { }
URHO3D_EVENT(GO_INVENTORYRECEIVE, Go_InventoryReceive) { }
URHO3D_EVENT(GO_INVENTORYGIVE, Go_InventoryGive) { }
URHO3D_EVENT(GO_INVENTORYSLOTEQUIP, Go_InventorySlotEquip) { }
URHO3D_EVENT(GO_INVENTORYSLOTSET, Go_InventorySlotSet) { }

/// GOC_Collectable
/// GO_COLLECTABLEDROP
/// => Sender : GOC_Collectable (DropSlotFrom)
/// => Subscribers : Player
URHO3D_EVENT(GO_COLLECTABLEDROP, Go_CollectableDrop)
{
    URHO3D_PARAM(GO_TYPE, GoType);           // type of the node Collectable, (use in drop Collectable)
    URHO3D_PARAM(GO_QUANTITY, GoQuantity);   // unsigned quantity of collectable, (use in drop Collectable)
}
URHO3D_EVENT(GO_DROPITEM, Go_DropItem) { }
URHO3D_EVENT(GO_USEITEM, Go_UseItem) { }
URHO3D_EVENT(GO_USEITEMEND, Go_UseItemEnd) { }

/// Player, Equipment
URHO3D_EVENT(GO_NODESCHANGED, Go_NodesChanged) { }
URHO3D_EVENT(GO_EQUIPMENTUPDATED, Go_EquipmentUpdated) { }
URHO3D_EVENT(GO_ABILITYADDED, Go_AbilityAdded) { }
URHO3D_EVENT(GO_ABILITYREMOVED, Go_AbilityRemoved) { }
URHO3D_EVENT(GO_PLAYERMISSIONMANAGERSTART, Go_PlayerMissionManagerStart) { }
URHO3D_EVENT(GO_PLAYERTRANSFERCOLLECTABLESTART, Go_PlayerTransferCollectableStart) { }
URHO3D_EVENT(GO_PLAYERTRANSFERCOLLECTABLEFINISH, Go_PlayerTransferCollectableFinish) { }
/// Mission Manager, Mission
URHO3D_EVENT(GO_MISSIONADDED, Go_MissionAdded)
{
    URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
    URHO3D_PARAM(GO_RECEIVERID, GoReceiverID);         // UInt : ID Actor Receiver
    URHO3D_PARAM(GO_SENDERID, GoSenderID);             // UInt : ID Actor Sender
}
URHO3D_EVENT(GO_MISSIONUPDATED, Go_MissionUpdated)
{
    URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
    URHO3D_PARAM(GO_STATE, GoState);                   // Int : Mission State
}
URHO3D_EVENT(GO_OBJECTIVEUPDATED, Go_ObjectiveUpdated)
{
    URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
    URHO3D_PARAM(GO_OBJECTIVEID, GoObjectiveID);       // UInt : ID Objective
    URHO3D_PARAM(GO_STATE, GoState);                   // Int :  Objective State
}
URHO3D_EVENT(GO_NAMEDMISSIONADDED, Go_NamedMissionAdded)
{
    URHO3D_PARAM(GO_MISSIONID, GoMissionID);           // UInt : ID Mission
}
URHO3D_EVENT(GO_NAMEDMISSIONUPDATED, Go_NamedMissionUpdated) { }
URHO3D_EVENT(GO_NAMEDOBJECTIVEUPDATED, Go_NamedObjectiveUpdated) { }

/// UISlotPanel
URHO3D_EVENT(PANEL_SWITCHVISIBLESUBSCRIBERS, Panel_SwitchVisibleSubscribers) { }
URHO3D_EVENT(PANEL_SLOTUPDATED, Panel_SlotUpdated) { }
/// UIC_StatusPanel
URHO3D_EVENT(UI_MONEYUPDATED, UI_MoneyUpdated) { }
//    {
//        URHO3D_PARAM(GO_IDSLOT, Go_IdSlot);
//    }
URHO3D_EVENT(CHARACTERUPDATED, CharacterUpdated) { }

URHO3D_EVENT(UI_OPENSTATUSPANEL, UI_OpenStatusPanel) { }
URHO3D_EVENT(UI_ESCAPEPANEL, UI_EscapePanel) { }

/// GOC_AIController
URHO3D_EVENT(AI_CHANGEORDER, Ai_ChangeOrder)
{
    URHO3D_PARAM(AI_ORDER, Ai_Order);           // StringHash, UInt : order, state
}

//    URHO3D_EVENT(AI_CALLBACKORDER, Ai_CallBackOrder)
//    {
//        URHO3D_PARAM(NODE_ID, Node_ID);      // Node ID : UInt
//        URHO3D_PARAM(ORDER, Order);          // UInt
//    }
/// Dialog
URHO3D_EVENT(DIALOG_DETECTED, Dialog_Detected)
{
    URHO3D_PARAM(ACTOR_ID, ActorId);    // UInt : ID of the actor who talk to Player
    URHO3D_PARAM(PLAYER_ID, PlayerId);  // UInt : ID of the player who talk to AI
}
URHO3D_EVENT(DIALOG_OPEN, Dialog_Open)
{
    URHO3D_PARAM(ACTOR_ID, ActorId);    // UInt : ID of the actor who talk to Player
    URHO3D_PARAM(PLAYER_ID, PlayerId);  // UInt : ID of the player who talk to AI
}
URHO3D_EVENT(DIALOG_NEXT, Dialog_Next)
{
    URHO3D_PARAM(ACTOR_ID, ActorId);    // UInt : ID of the actor who talk to Player
    URHO3D_PARAM(PLAYER_ID, PlayerId);  // UInt : ID of the player who talk to AI
    URHO3D_PARAM(DIALOGUENAME, DialogueName);   // StringHash : ID of the dialogue
    URHO3D_PARAM(DIALOGUEMSGID, DialogueMsgId);  // Int : index of the message in the dialogue
}
URHO3D_EVENT(DIALOG_QUESTION, Dialog_Question)
{
    URHO3D_PARAM(ACTOR_ID, ActorId);    // UInt : ID of the actor who talk to Player
    URHO3D_PARAM(PLAYER_ID, PlayerId);  // UInt : ID of the player who talk to AI
    URHO3D_PARAM(DIALOGUENAME, DialogueName);   // StringHash : ID of the dialogue
    URHO3D_PARAM(DIALOGUEMSGID, DialogueMsgId);  // Int : index of the message question in the dialogue
}
URHO3D_EVENT(DIALOG_RESPONSE, Dialog_Response)
{
    URHO3D_PARAM(ACTOR_ID, ActorId);    // UInt : ID of the actor who talk to Player
    URHO3D_PARAM(PLAYER_ID, PlayerId);  // UInt : ID of the player who talk to AI
    URHO3D_PARAM(DIALOGUENAME, DialogueName);   // StringHash : ID of the dialogue
    URHO3D_PARAM(DIALOGUEMSGID, DialogueMsgId);  // Int : index of the message question for this response in the dialogue
}
URHO3D_EVENT(DIALOG_CLOSE, Dialog_Close)
{
    URHO3D_PARAM(ACTOR_ID, ActorId);    // UInt : ID of the actor who talk to Player
    URHO3D_PARAM(PLAYER_ID, PlayerId);  // UInt : ID of the player who talk to AI
}
URHO3D_EVENT(DIALOG_BARGAIN, Dialog_Bargain)
{
}
/// Ability
URHO3D_EVENT(ABI_RELEASE, Ability_Release) { }

/// Generic Animation Event
URHO3D_EVENT(ANIM_EVENT, ANIM_Event)
{
    URHO3D_PARAM(DATAS, datas);
}
URHO3D_EVENT(EVENT_DEFAULT, Event_Default) { }
URHO3D_EVENT(EVENT_DEFAULT_GROUND, Event_Default_Ground) { }
URHO3D_EVENT(EVENT_DEFAULT_AIR, Event_Default_Air) { }
URHO3D_EVENT(EVENT_DEFAULT_CLIMB, Event_Default_Climb) { }
URHO3D_EVENT(EVENT_DEFAULT_FLUID, Event_Default_Fluid) { }
URHO3D_EVENT(EVENT_MOVE, Event_OnWalk) { }
URHO3D_EVENT(EVENT_MOVE_GROUND, Event_OnWalk) { }
URHO3D_EVENT(EVENT_MOVE_AIR, Event_OnFlyUp) { }
URHO3D_EVENT(EVENT_MOVE_FLUID, Event_OnSwim) { }
URHO3D_EVENT(EVENT_FALL, Event_OnFall) { }
URHO3D_EVENT(EVENT_JUMP, Event_OnJump) { }
URHO3D_EVENT(EVENT_FLYUP, Event_OnFlyUp) { }
URHO3D_EVENT(EVENT_FLYDOWN, Event_OnFlyDown) { }
URHO3D_EVENT(EVENT_CLIMB, Event_OnClimb) { }
URHO3D_EVENT(EVENT_SWIM, Event_OnSwim) { }
URHO3D_EVENT(EVENT_CHANGEAREA, Event_ChangeArea) { }
URHO3D_EVENT(EVENT_CHANGEGRAVITY, Event_ChangeGravity) { }

/// SPRITER Animation Event
URHO3D_EVENT(SPRITER_PARTICULE, SPRITER_Particule) { }
URHO3D_EVENT(SPRITER_PARTFEET, SPRITER_PartFeet) { }
URHO3D_EVENT(SPRITER_PART, SPRITER_Part) { }
URHO3D_EVENT(SPRITER_LIGHT, SPRITER_Light) { }
URHO3D_EVENT(SPRITER_SOUND, SPRITER_Sound) { }
URHO3D_EVENT(SPRITER_SOUNDFEET, SPRITER_SoundFeet) { }
URHO3D_EVENT(SPRITER_SOUNDATTACK, SPRITER_SoundAttack) { }
URHO3D_EVENT(SPRITER_ENTITY, SPRITER_Entity) { }
URHO3D_EVENT(SPRITER_ANIMATION, SPRITER_Animation) { }
URHO3D_EVENT(SPRITER_ANIMATIONINSIDE, SPRITER_AnimationInside) { }
}

using namespace Urho3D;


/// GO Events
struct GOE
{
    static void InitEventTable();
    static StringHash Register(const String& eventName);
    static const String& GetEventName(const StringHash& hashevent);
    static void DumpAll();

private :
    static HashMap<StringHash, String> events_;
};
