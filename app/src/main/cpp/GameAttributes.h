#pragma once

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

#include "DefsGame.h"

using namespace Urho3D;

/// GO Attributes
struct GOA
{
    // GameObjectType handle by GOT and GO_Pools
    static const StringHash GOT;
    static const StringHash CLIENTID;
    // handle by Player or AI or GOC_Animator2D for spawned entity
    static const StringHash OWNERID; // was CONTROLLERID
    // handle by Actor
    static const StringHash ACTORID;
    // handle by Map and MapData
    static const StringHash ENTITYDATAID;
    static const StringHash BOSSZONEID;

    // handle by GOC_Destroyer
    static const StringHash ONMAP;
    static const StringHash INFLUID;
    // handle by World2D::AddStaticFurniture
    static const StringHash ONTILE;
    // handle by GOC_Destroyer & Map for node with no GOC_Destroyer
    static const StringHash ONVIEWZ; /// Net Usage from client to Server

    // used by Map visibilty process (for LTree)
    static const StringHash KEEPVISIBLE;
    // handle by GOC_Destroyer for setting layer alignement Drawable2D (centered on LAYER_ACTOR, back=-1,front=+1)
    static const StringHash LAYERALIGNMENT;
    // used by an entity to specify the physic plateform mode used by PhysicWorld2D
    static const StringHash PLATEFORM;
    static const StringHash BULLET;
    // handle by GOC_Animator
    static const StringHash DIRECTION;
    static const StringHash LIGHTSTATE;

    // handle by GOC_Move2D and use by GOC_Controller
    static const StringHash MOVESTATE;
    static const StringHash KEEPDIRECTION;
    static const StringHash VELOCITY;

    // handle by GOC_Collide2D
    static const StringHash ONGROUND;
    // handle by GOC_Move2D and GOB_MountOn
    static const StringHash ISMOUNTEDON;
    // handle by GOC_Life
    static const StringHash ISDEAD;
    // handle by GOC_Destroy
    static const StringHash DESTROYING;

    // handle by GOC_ZoneEffect, GOC_Attack, GOC_Life
    static const StringHash LIFE;
    static const StringHash FIRE;
    static const StringHash DEATH;
    static const StringHash EFFECTID1;
    static const StringHash EFFECTID2;
    static const StringHash EFFECTID3;

    // handle by Equipment
    static const StringHash ABILITY;

    // handle by GOC_Controller
    static const StringHash TYPECONTROLLER;
    static const StringHash BUTTONS;
    static const StringHash FACTION;
    static const StringHash NOCHILDFACTION;

    // handle by UISlotPanel
    static const StringHash PANELID;

    // GameGoalType handle by GameGoal
    static const StringHash GGOAT;
    // handle by GameGoal
    static const StringHash GGOAT_COLLECTOR;
    static const StringHash GGOAT_KILLER;
    static const StringHash GGOAT_EXPLORER;
    static const StringHash GGOAT_PROTECTOR;
    static const StringHash GGOAT_CREATOR;
//    static const StringHash GGOAA_GIVER;
//    static const StringHash GGOAA_RECEIVER;
//    static const StringHash GGOAA_LOCATION;
//    static const StringHash GGOAA_NUMTOREACH;
//    static const StringHash GGOAA_TYPETOREACH;

    /// Actor Stat
    // handle by ActorStat
    static const StringHash ACS_ENTITIES_KILLED;
    static const StringHash ACS_ITEMS_COLLECTED;
    static const StringHash ACS_ITEMS_GIVED;
    static const StringHash ACS_ACTOR_FOUND;
    static const StringHash ACS_ACTOR_PROTECTED;
    static const StringHash ACS_NODE_CREATED;

    /// Condition
    static const StringHash COND_TRUE;
    static const StringHash COND_FALSE;
    static const StringHash COND_ACTIVEABILITY;
    static const StringHash COND_ABILITY;
    static const StringHash COND_APPLIEDMAPPING;
    static const StringHash COND_ENTITY;
    static const StringHash COND_MAXTICKDELAY;
    static const StringHash COND_BUTTONHOLD;
    static void InitAttributeTable();
    static void RegisterToContext(Context* context);
    static void RegisterToScene();
    static StringHash Register(const String& AttributeName);
    static String GetAttributeName(StringHash hashattrib);
    static void DumpAll();

private :
    static HashMap<StringHash, String> attributes_;
};

/// GO Base Types
enum GOControllerType
{
    GO_None = 0,
    GO_Player = 0x01,       // 0000001 => Player
    GO_NetPlayer = 0x03,    // 0000011 => Net Player
    GO_AI = 0x04,           // 0000100 => AI Player
    GO_AI_None = 0x08,      // 0001000 => Ai No Alignement
    GO_AI_Ally = 0x10,      // 0010000 => Ai friendly
    GO_AI_Enemy = 0x20,     // 0100000 => Ai enemy
    GO_Furniture = 0x40,    // 1000000
    GO_Entity = GO_Player | GO_NetPlayer | GO_AI | GO_AI_None | GO_AI_Ally | GO_AI_Enemy,
};

enum ControlType
{
    CT_KEYBOARD = 0,
    CT_JOYSTICK,
    CT_SCREENJOYSTICK,
    CT_TOUCH,
    CT_CPU,
};

enum ControlAction
{
    ACTION_UP = 0,
    ACTION_DOWN,
    ACTION_LEFT,
    ACTION_RIGHT,
    ACTION_JUMP,
    ACTION_FIRE1,
    ACTION_FIRE2,
    ACTION_FIRE3,
    ACTION_STATUS,
    ACTION_SCENE,
    MAX_NUMACTIONS
};

// Input Key
const int CTRL_UP     = 1 << ACTION_UP;
const int CTRL_DOWN   = 1 << ACTION_DOWN;
const int CTRL_LEFT   = 1 << ACTION_LEFT;
const int CTRL_RIGHT  = 1 << ACTION_RIGHT;
const int CTRL_JUMP   = 1 << ACTION_JUMP;
const int CTRL_FIRE   = 1 << ACTION_FIRE1;
const int CTRL_FIRE2  = 1 << ACTION_FIRE2;
const int CTRL_FIRE3  = 1 << ACTION_FIRE3;
const int CTRL_STATUS = 1 << ACTION_STATUS;
const int CTRL_SCENE  = 1 << ACTION_SCENE;

enum GOTypeProperties
{
    GOT_None            = 0x00000,
    GOT_Collectable     = 0x00001,
    GOT_Usable          = 0x00002,
    GOT_Usable_DropOut  = 0x00004,
    GOT_Buildable       = 0x00008,
    GOT_Part        	= 0x00010,
    GOT_Droppable    	= 0x00020,
    GOT_Wearable		= 0x00040,     // equipement
    GOT_Ability         = 0x00080,     // ability getter
    GOT_Controllable    = 0x00100,     // ControllerAI_Ally possible ?
    GOT_Spawnable       = 0x00200,     // via DropZone, Random spawn etc ...
    GOT_CanCollect      = 0x00400,     // can collect collectables
    GOT_Effect          = 0x00800,     // local animation
    GOT_Furniture       = 0x01000,
    GOT_AnchorOnGround  = 0x02000,
    GOT_AnchorOnBack    = 0x04000,
    GOT_Animation       = 0x08000,     // if Wearable && Animation => it renders as AnimationInside
    GOT_Tool            = 0x10000,
    GOT_Flippable       = 0x20000,

    // wearable + usable = slot arme ?
    GOT_CollectablePart = GOT_Collectable | GOT_Part,
    GOT_BuildablePart   = GOT_Buildable | GOT_Part,
    GOT_Static          = GOT_AnchorOnGround | GOT_AnchorOnBack,
    GOT_StaticFurniture = GOT_Furniture | GOT_Static,
    GOT_UsableFurniture = GOT_Furniture | GOT_Usable,
};

struct BuildableObjectInfo
{
    ///
    Vector<String> partnames_;

    /// SpriterEntityId when the Buildable is associated with another GOT or COT
    HashMap<StringHash, unsigned > entitySelection_;
};

/// Table for GOT (Game Object Types)
struct GOT
{
    static StringHash Register(const String& type, const StringHash& category, const String& objectFile,
                               unsigned typeProperty = 0, bool replicateMode=false, int pooltype = 0,int qtyInPool = 0, int scaleVariation=0, bool entityVariation=false,
                               bool mappingVariation=false, int defaultValue = 0, int maxDropQty = 0, const IntVector2& entitySize=IntVector2::ZERO);

    static void RegisterBuildablePartsForType(const String& typeName, const String& typePartName, const String& partNames);
    static void RegisterBuildableEntitySelection(const String& typeName, const String& entitySelection);

    static void Clear();
    static void InitDefaultTables();
    static void LoadJSONFile(Context* context, const String& name);
    static bool PreLoadObjects(int& state, HiresTimer* timer, const long long& delay, Node* preloaderGOT, bool useObjectPool=false);
    static void UnLoadObjects(Node* node);

    static unsigned GetSize();
    static const StringHash& Get(unsigned index);
    static unsigned GetIndex(const StringHash& type);
    static bool IsRegistered(const StringHash& type);
    static const GOTInfo& GetConstInfo(const StringHash& type);
    static bool HasObject(const StringHash& type);
    static Node* GetObject(const StringHash& type);
    static bool HasObjectFile(const StringHash& type);
    static const String& GetObjectFile(const StringHash& type);
    static const String& GetType(const StringHash& type);
    static unsigned GetTypeProperties(const StringHash& type);
    static bool HasReplicatedMode(const StringHash& type);
    static const StringHash& GetCategory(const StringHash& type);
    static int GetMaxDropQuantityFor(const StringHash& type);
    static int GetDefaultValue(const StringHash& type);
    static const ResourceRefList& GetResourceList(const StringHash& type);
    /// Buildable
    static const StringHash& GetBuildableType(const StringHash& partType);
    static Vector<String>* GetPartsNamesFor(const StringHash& type);
    static const String& GetPartNameFor(const StringHash& buildableType, unsigned partIndex);
    static int GetPartIndexFor(const StringHash& buildableType, const String& partName);
    static const HashMap<StringHash, unsigned >* GetBuildableEntitySelection(const StringHash& buildableType);
    static int GetBuildableEntityIdFor(const StringHash& buildableType, const StringHash& otherObject);
    /// Controllable
    static Vector<WeakPtr<Node> >& GetControllables()
    {
        return controllables_;
    }
    static int GetControllableIndex(const StringHash& type);
    static const StringHash& GetControllableType(int index);
    static Node* GetControllableTemplate(int index);
    static Node* GetControllableTemplate(const StringHash& type);
    static Node* GetClonedControllableFrom(Node* templateNode, unsigned nodeid, CreateMode mode);
    static Node* GetClonedControllable(int index, unsigned nodeid, CreateMode mode = LOCAL);
    static Node* GetClonedControllable(const StringHash& type, unsigned nodeid, CreateMode mode = LOCAL);

    static void DumpAll();

    static const StringHash COLLECTABLE_ALL;
    static const StringHash COLLECTABLEPART;
    static const StringHash BUILDABLEPART;
    static const StringHash PART;
    static const StringHash BONE;
    static const StringHash MONEY;
    static const StringHash START;

private :
    static Vector<StringHash > gots_;
    static Vector<WeakPtr<Node> > controllables_;
    static Vector<StringHash > controllableTypes_;
    static HashMap<StringHash, GOTInfo > infos_;
    static HashMap<StringHash, WeakPtr<Node> > objects_;
    static HashMap<StringHash, ResourceRefList > resourceLists_;
    static HashMap<StringHash, BuildableObjectInfo > buildablesInfos_;//partsForBuildable_;
    static HashMap<StringHash, Vector<StringHash> > buildablesForPart_;
};

/// Tables for COT (Category Object Type)
// a COT is a set of GOT
// a GOT have a unique Category register in GOT::typeCategories_
// example : a Leather Cuirass (GOT) is an Armor (COT)
struct COT
{
public :
    static const StringHash OBJECTS;
    static const StringHash LIQUIDS;

    static const StringHash MONEY;
    static const StringHash ITEMS;
    static const StringHash ABILITY;
    static const StringHash AVATAR;
    static const StringHash QUEST;

    static const StringHash DUNGEON;

    static const StringHash CHARACTERS;

    static const StringHash EQUIPMENTS;
    static const StringHash WEAPONS;
    static const StringHash MELEEWEAPONS;
    static const StringHash RANGEDWEAPONS;
    static const StringHash ARMORS;
    static const StringHash HELMETS;
    static const StringHash BELTS;
    static const StringHash CAPES;
    static const StringHash HEADBANDS;
    static const StringHash SPECIALS;

    static const StringHash CONTAINERS;
    static const StringHash MONSTERS;

    static const String& GetName(const StringHash& category);

    static bool IsACategory(const StringHash& hash);

    static StringHash Register(const String& category, const StringHash& base=StringHash::ZERO);
    static void AddTypeInCategory(const StringHash& category, const StringHash& type);
    static bool IsInCategory(const StringHash& type, const StringHash& category);
    static int IsInCategories(const StringHash& type, const Vector<StringHash>& categories);
    static void InitCategoryTables();

    static const Vector<StringHash>& GetTypesInCategory(const StringHash& category)
    {
        return typesByCategory_[category];
    }
    static void GetAllTypesInCategory(Vector<StringHash>& gots, const StringHash& category);
    static StringHash GetRandomCategoryFrom(const StringHash& base, int rand=0);
    static const StringHash& GetTypeFrom(const StringHash& category, unsigned index);
    static const StringHash& GetRandomTypeFrom(const StringHash& base, int rand=0);
    static Node* GetObjectFromCategory(const StringHash& base, unsigned index=0);

    static void DumpAll();

private :
    static HashMap<StringHash, String> categoryNames_;
    static HashMap<StringHash, Vector<StringHash> > typesByCategory_;
    static HashMap<StringHash, StringHash> categoryBases_;
    static HashMap<StringHash, Vector<StringHash> > categoriesByBase_;
};

/// GO States
struct GOS
{
    static void InitStateTable();
    static void RegisterToContext(Context* context);
    static const StringHash& Register(const String& stateName);
    static const String& GetStateName(const StringHash& hashstate);
    static const String& GetStateName(unsigned state);
    static unsigned GetStateIndex(const StringHash& hashstate);
    static const StringHash& GetStateFromIndex(unsigned index);
    static void DumpAll();

private :
    static Vector<StringHash> states_;
    static HashMap<StringHash, String> stateNames_;
};

/// Animation States
//const unsigned STATEFINDER            = StringHash("STATEFINDER").Value();
const unsigned STATE_APPEAR           = StringHash("State_Appear").Value();
const unsigned STATE_DEFAULT          = StringHash("State_Default").Value();
const unsigned STATE_DEFAULT_GROUND   = StringHash("State_Default_Ground").Value();
const unsigned STATE_DEFAULT_AIR      = StringHash("State_Default_Air").Value();
const unsigned STATE_DEFAULT_FLUID    = StringHash("State_Default_Fluid").Value();
const unsigned STATE_DEFAULT_CLIMB    = StringHash("State_Default_Climb").Value();
const unsigned STATE_WALK             = StringHash("State_Walk").Value();
const unsigned STATE_CLIMB            = StringHash("State_Climb").Value();
const unsigned STATE_FLYUP            = StringHash("State_FlyUp").Value();
const unsigned STATE_FLYDOWN          = StringHash("State_FlyDown").Value();
const unsigned STATE_SWIM             = StringHash("State_Swim").Value();
const unsigned STATE_RUN              = StringHash("State_Run").Value();
const unsigned STATE_ATTACK           = StringHash("State_Attack").Value();
const unsigned STATE_ATTACK_GROUND    = StringHash("State_Attack_Ground").Value();
const unsigned STATE_ATTACK_AIR       = StringHash("State_Attack_Air").Value();
const unsigned STATE_SHOOT            = StringHash("State_Shoot").Value();
const unsigned STATE_POWER1           = StringHash("State_Power1").Value();
const unsigned STATE_POWER2           = StringHash("State_Power2").Value();
const unsigned STATE_JUMP             = StringHash("State_Jump").Value();
const unsigned STATE_FALL             = StringHash("State_Fall").Value();
const unsigned STATE_HURT             = StringHash("State_Hurt").Value();
const unsigned STATE_DEAD             = StringHash("State_Dead").Value();
const unsigned STATE_EXPLODE          = StringHash("State_Explode").Value();
const unsigned STATE_TALK             = StringHash("State_Talk").Value();
const unsigned STATE_DISAPPEAR        = StringHash("State_Disappear").Value();
const unsigned STATE_DESTROY          = StringHash("State_Destroy").Value();
const unsigned STATE_LIGHTED          = StringHash("State_Lighted").Value();
const unsigned STATE_UNLIGHTED        = StringHash("State_Unlighted").Value();
const StringHash STATE_IDLE           = StringHash("State_Default");

/// Behaviors
const unsigned GOB_PLAYERCPU          = StringHash("GOB_PlayerCPU").Value();
const unsigned GOB_PATROL             = StringHash("GOB_Patrol").Value();
const unsigned GOB_FOLLOW             = StringHash("GOB_Follow").Value();
const unsigned GOB_FOLLOWATTACK       = StringHash("GOB_FollowAttack").Value();
const unsigned GOB_MOUNTON            = StringHash("GOB_MountOn").Value();

const unsigned BEHAVIOR_DEFAULT       = GOB_PATROL;

/// Equipment
const unsigned EQUIPMENTTEMPLATE      = StringHash("EquipmentTemplate").Value();

/// CharacterMaps
extern HashMap<StringHash, String> CharacterMappingNames_;
const StringHash CMAP_HEAD1 = StringHash("Head1");
const StringHash CMAP_HEAD2 = StringHash("Head2");
const StringHash CMAP_HEAD3 = StringHash("Head3");
const StringHash CMAP_NAKED = StringHash("Naked");
const StringHash CMAP_ARMOR = StringHash("Armor");
const StringHash CMAP_HELMET = StringHash("Helmet");
const StringHash CMAP_WEAPON1 = StringHash("Weapon1");
const StringHash CMAP_WEAPON2 = StringHash("Weapon2");
const StringHash CMAP_BELT = StringHash("Belt");
const StringHash CMAP_CAPE = StringHash("Cape");
const StringHash CMAP_HEADBAND = StringHash("Headband");
const StringHash CMAP_TAIL = StringHash("Tail");
const StringHash CMAP_FIRE = StringHash("Fire");
const StringHash CMAP_NOARMOR = StringHash("No_Armor");
const StringHash CMAP_NOHELMET = StringHash("No_Helmet");
const StringHash CMAP_NOWEAPON1 = StringHash("No_Weapon1");
const StringHash CMAP_NOWEAPON2 = StringHash("No_Weapon2");
const StringHash CMAP_NOBELT = StringHash("No_Belt");
const StringHash CMAP_NOCAPE = StringHash("No_Cape");
const StringHash CMAP_NOHEADBAND = StringHash("No_Headband");
const StringHash CMAP_NOTAIL = StringHash("No_Tail");
const StringHash CMAP_NOFIRE = StringHash("No_Fire");

/// Biome
const StringHash DUNGEON                = StringHash("Dungeon");
const StringHash DUNGEONSIDES           = StringHash("DungeonSides");
const StringHash DUNGEONBOTTOM          = StringHash("DungeonBottom");
const StringHash DUNGEONTHRESHOLD       = StringHash("DungeonThreshold");
const StringHash BIOMECAVE              = StringHash("BiomeCave");
const StringHash BIOMECAVESIDES         = StringHash("BiomeCaveSides");
const StringHash BIOMECAVEBOTTOM        = StringHash("BiomeCaveBottom");
const StringHash BIOMEEXTERNAL          = StringHash("BiomeExternal");
const StringHash BIOMEEXTERNALSIDES     = StringHash("BiomeExternalSides");
const StringHash BIOMEEXTERNALBOTTOM    = StringHash("BiomeExternalBottom");
const StringHash BIOMEBACK              = StringHash("BiomeBackGround");
const StringHash BIOMEBACKSIDES         = StringHash("BiomeBackGroundSides");
const StringHash BIOMEBACKBOTTOM        = StringHash("BiomeBackGroundBottom");

/// Particule Effects Types
enum ParticuleEffectType
{
    PE_DUST,
    PE_TORCHE,
    PE_FLAME,
    PE_LIFEFLAME,
    PE_SUN,
    PE_GREENSPIRAL,
    PE_BLACKSPIRAL,
    PE_BULLEDAIR
};

extern const String ParticuleEffect_[];
