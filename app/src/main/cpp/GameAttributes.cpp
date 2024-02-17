#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Resource/XMLFile.h>

#include <Urho3D/Urho2D/AnimatedSprite2D.h>
#include <Urho3D/Urho2D/AnimationSet2D.h>
#include <Urho3D/Urho2D/StaticSprite2D.h>
#include <Urho3D/Urho2D/ParticleEmitter2D.h>
#include <Urho3D/Urho2D/Sprite2D.h>
#include <Urho3D/Urho2D/RigidBody2D.h>
#include <Urho3D/Urho2D/CollisionBox2D.h>
#include <Urho3D/Urho2D/CollisionCircle2D.h>

#include "DefsWorld.h"
#include "GameContext.h"
#include "GameHelpers.h"

#include "GO_Pool.h"
#include "ObjectPool.h"
#include "TimerRemover.h"

#include "GameAttributes.h"


using namespace Urho3D;

/// GO Attributes
HashMap<StringHash, String> GOA::attributes_;
HashMap<StringHash, String> CharacterMappingNames_;

const FROMBONES_API StringHash GOA::GOT               = StringHash("GOA_GOT");
const StringHash GOA::CLIENTID          = StringHash("GOA_ClientID");
const StringHash GOA::OWNERID           = StringHash("GOA_OwnerId");
const StringHash GOA::ACTORID           = StringHash("GOA_ActorId");
const StringHash GOA::ENTITYDATAID      = StringHash("GOA_EntityDataId");
const StringHash GOA::BOSSZONEID	    = StringHash("GOA_BossZoneId");

const StringHash GOA::ONMAP             = StringHash("GOA_OnMap");
const StringHash GOA::INFLUID           = StringHash("GOA_InFluid");
const StringHash GOA::ONTILE            = StringHash("GOA_OnTile");
const FROMBONES_API StringHash GOA::ONVIEWZ           = StringHash("GOA_OnViewZ");

const StringHash GOA::KEEPVISIBLE       = StringHash("GOA_KeepVisible");
const StringHash GOA::LAYERALIGNMENT    = StringHash("GOA_LayerAlignement");
const StringHash GOA::PLATEFORM         = StringHash("GOA_PlateForm");
const StringHash GOA::BULLET            = StringHash("GOA_Bullet");
const StringHash GOA::DIRECTION         = StringHash("GOA_Direction");
const StringHash GOA::LIGHTSTATE        = StringHash("GOA_LightState");

const StringHash GOA::MOVESTATE         = StringHash("GOA_MoveState");
const StringHash GOA::KEEPDIRECTION     = StringHash("GOA_KeepDirection");
const StringHash GOA::VELOCITY          = StringHash("GOA_Velocity");

const StringHash GOA::ONGROUND          = StringHash("GOA_OnGround");
const StringHash GOA::ISMOUNTEDON       = StringHash("GOA_IsMountedOn");

const StringHash GOA::ISDEAD            = StringHash("GOA_IsDead");
const StringHash GOA::DESTROYING        = StringHash("GOA_Destroying");
const StringHash GOA::LIFE              = StringHash("GOA_Life");
const StringHash GOA::DEATH             = StringHash("GOA_Death");
const StringHash GOA::FIRE              = StringHash("GOA_Fire");
const StringHash GOA::EFFECTID1         = StringHash("GOA_EffectID1");
const StringHash GOA::EFFECTID2         = StringHash("GOA_EffectID2");
const StringHash GOA::EFFECTID3         = StringHash("GOA_EffectID3");

const StringHash GOA::ABILITY           = StringHash("GOA_Ability");

const StringHash GOA::FACTION           = StringHash("GOA_Faction");
const StringHash GOA::NOCHILDFACTION    = StringHash("GOA_NoChildFaction");

const StringHash GOA::TYPECONTROLLER    = StringHash("GOA_TypeCtrl");
const StringHash GOA::BUTTONS           = StringHash("GOA_Buttons");

const StringHash GOA::PANELID           = StringHash("GOA_PanelID");

const StringHash GOA::GGOAT             = StringHash("GOA_GGOAT");
const StringHash GOA::GGOAT_COLLECTOR   = StringHash("GGOAT_Collector");
const StringHash GOA::GGOAT_KILLER      = StringHash("GGOAT_Killer");
const StringHash GOA::GGOAT_EXPLORER    = StringHash("GGOAT_Explorer");
const StringHash GOA::GGOAT_PROTECTOR   = StringHash("GGOAT_Protector");
const StringHash GOA::GGOAT_CREATOR     = StringHash("GGOAT_Creator");
//const StringHash GOA::GGOA_GIVER       = StringHash("GGOAA_Giver");
//const StringHash GOA::GGOAA_RECEIVER    = StringHash("GGOAA_Receiver");
//const StringHash GOA::GGOAA_LOCATION    = StringHash("GGOAA_Location");
//const StringHash GOA::GGOAA_NUMTOREACH  = StringHash("GGOAA_NumToReach");
//const StringHash GOA::GGOAA_TYPETOREACH = StringHash("GGOAA_TypeToReach");
const StringHash GOA::ACS_ENTITIES_KILLED   = StringHash("ACS_EntitiesKilled");
const StringHash GOA::ACS_ITEMS_COLLECTED   = StringHash("ACS_ItemsCollected");
const StringHash GOA::ACS_ITEMS_GIVED       = StringHash("ACS_ItemsGived");
const StringHash GOA::ACS_ACTOR_FOUND        = StringHash("ACS_Actor_Found");
const StringHash GOA::ACS_ACTOR_PROTECTED    = StringHash("ACS_Actor_Protected");
const StringHash GOA::ACS_NODE_CREATED      = StringHash("ACS_Node_Created");

const StringHash GOA::COND_TRUE              = StringHash("true");
const StringHash GOA::COND_FALSE             = StringHash("false");
const StringHash GOA::COND_ACTIVEABILITY     = StringHash("ActiveAbility");
const StringHash GOA::COND_ABILITY           = StringHash("Ability");
const StringHash GOA::COND_APPLIEDMAPPING    = StringHash("AppliedMapping");
const StringHash GOA::COND_ENTITY            = StringHash("Entity");
const StringHash GOA::COND_MAXTICKDELAY      = StringHash("MaxTickDelay");
const StringHash GOA::COND_BUTTONHOLD        = StringHash("ButtonHold");

void GOA::InitAttributeTable()
{
    attributes_.Clear();

    Register("GOA_GOT");

    Register("GOA_ClientId");
    Register("GOA_OwnerId");
    Register("GOA_ActorId");
    Register("GOA_EntityDataId");
    Register("GOA_BossZoneId");

    Register("GOA_OnMap");
    Register("GOA_InFluid");
    Register("GOA_OnTile");
    Register("GOA_OnViewZ");

    Register("GOA_RescaleToWorld");

    Register("GOA_KeepVisible");
    Register("GOA_LayerAlignement");
    Register("GOA_Plateform");
    Register("GOA_Bullet");
    Register("GOA_Direction");
    Register("GOA_LightState");

    Register("GOA_MoveState");
    Register("GOA_KeepDirection");
    Register("GOA_Velocity");

    Register("GOA_IsMountedOn");
    Register("GOA_OnGround");

    Register("GOA_IsDead");
    Register("GOA_Destroying");
    Register("GOA_Life");
    Register("GOA_Death");
    Register("GOA_Fire");
    Register("GOA_EffectID1");
    Register("GOA_EffectID2");
    Register("GOA_EffectID3");

    Register("GOA_Ability");

    Register("GOA_OnControl");

    Register("GOA_Faction");
    Register("GOA_NoChildFaction");

    Register("GOA_TypeCtrl");
    Register("GOA_Buttons");

    Register("GOA_PanelID");

    Register("GOA_GGOAT");
    Register("GGOAT_Collector");
    Register("GGOAT_Killer");
    Register("GGOAT_Explorer");
    Register("GGOAT_Protector");
    Register("GGOAT_Creator");

    Register("ACS_EntitiesKilled");
    Register("ACS_ItemsCollected");
    Register("ACS_ItemsGived");
    Register("ACS_Actor_Found");
    Register("ACS_Actor_Protected");
    Register("ACS_Node_Created");

    Register("true");
    Register("false");
    Register("ActiveAbility");
    Register("Ability");
    Register("AppliedMapping");
    Register("Entity");
    Register("MaxTickDelay");
    Register("ButtonHold");

    CharacterMappingNames_.Clear();
    CharacterMappingNames_[CMAP_HEAD1] = String("Head2");
    CharacterMappingNames_[CMAP_HEAD2] = String("Head1");
    CharacterMappingNames_[CMAP_HEAD3] = String("Head3");
    CharacterMappingNames_[CMAP_NAKED] = String("Naked");
    CharacterMappingNames_[CMAP_ARMOR] = String("Armor");
    CharacterMappingNames_[CMAP_HELMET] = String("Helmet");
    CharacterMappingNames_[CMAP_WEAPON1] = String("Weapon1");
    CharacterMappingNames_[CMAP_WEAPON2] = String("Weapon2");
    CharacterMappingNames_[CMAP_BELT] = String("Belt");
    CharacterMappingNames_[CMAP_CAPE] = String("Cape");
    CharacterMappingNames_[CMAP_HEADBAND] = String("Headband");
    CharacterMappingNames_[CMAP_TAIL] = String("Tail");
    CharacterMappingNames_[CMAP_NOARMOR] = String("No_Armor");
    CharacterMappingNames_[CMAP_NOHELMET] = String("No_Helmet");
    CharacterMappingNames_[CMAP_NOWEAPON1] = String("No_Weapon1");
    CharacterMappingNames_[CMAP_NOWEAPON2] = String("No_Weapon2");
    CharacterMappingNames_[CMAP_NOBELT] = String("No_Belt");
    CharacterMappingNames_[CMAP_NOCAPE] = String("No_Cape");
    CharacterMappingNames_[CMAP_NOHEADBAND] = String("No_Headband");
    CharacterMappingNames_[CMAP_NOTAIL] = String("No_Tail");
}

void GOA::RegisterToContext(Context* context)
{
    if (!context) return;

    URHO3D_LOGINFOF("GOA() - RegisterToContext");

    for (HashMap<StringHash, String>::ConstIterator it=attributes_.Begin(); it!=attributes_.End(); ++it)
    {
        context->RegisterUserAttribute(it->second_);
    }
}

void GOA::RegisterToScene()
{
    if (!GameContext::Get().rootScene_) return;

    for (HashMap<StringHash, String>::ConstIterator it=attributes_.Begin(); it!=attributes_.End(); ++it)
    {
        GameContext::Get().rootScene_->RegisterVar(it->second_);
    }
}

StringHash GOA::Register(const String& AttributeName)
{
    StringHash attrib = StringHash(AttributeName);

    if (attributes_.Contains(attrib))
    {
//        URHO3D_LOGINFOF("GOA() : Register AttributeName=%s(%u) (Warning:attributes_ contains already the key)", AttributeName.CString(), attrib.Value());
        return attrib;
    }

    attributes_[attrib] = AttributeName;
    return attrib;
}

String GOA::GetAttributeName(StringHash hashattrib)
{
    HashMap<StringHash, String>::ConstIterator it = attributes_.Find(hashattrib);
    return it != attributes_.End() ? it->second_ : String::EMPTY;
}

void GOA::DumpAll()
{
    URHO3D_LOGINFOF("GOA() - DumpAll : attributes_ Size =", attributes_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String>::ConstIterator it=attributes_.Begin(); it!=attributes_.End(); ++it)
    {
        URHO3D_LOGINFOF("     => attributes_[%u] = (hash:%u(dec:%d) - name:%s)", index, it->first_.Value(), it->first_.Value(), it->second_.CString());
        ++index;
    }
}


/// GO Types
#include "EnumList.hpp"

#define GOTYPEPROPERTIES \
    GOT_None, GOT_Collectable, GOT_Usable, GOT_Usable_DropOut, \
    GOT_Buildable, GOT_Part, GOT_Droppable, GOT_Wearable, GOT_Ability, \
    GOT_Controllable, GOT_Spawnable, GOT_CanCollect, GOT_Effect, GOT_Furniture, \
    GOT_AnchorOnGround, GOT_AnchorOnBack, GOT_Animation, GOT_Tool, GOT_Flippable

#define GOTYPEPROPERTIES_STRINGS \
    ENUMSTRING( GOT_None ), ENUMSTRING( GOT_Collectable ), ENUMSTRING( GOT_Usable ), ENUMSTRING( GOT_Usable_DropOut ), \
    ENUMSTRING( GOT_Buildable ), ENUMSTRING( GOT_Part ), ENUMSTRING( GOT_Droppable ), ENUMSTRING( GOT_Wearable), ENUMSTRING( GOT_Ability ), \
    ENUMSTRING( GOT_Controllable ), ENUMSTRING( GOT_Spawnable ), ENUMSTRING( GOT_CanCollect ), ENUMSTRING( GOT_Effect ), \
    ENUMSTRING( GOT_Furniture ), ENUMSTRING( GOT_AnchorOnGround ), ENUMSTRING( GOT_AnchorOnBack ), ENUMSTRING( GOT_Animation ) , ENUMSTRING( GOT_Tool ) , ENUMSTRING( GOT_Flippable )

#define GOTYPEPROPERTIES_VALUES \
    0x00000, 0x00001, 0x00002, 0x00004, \
    0x00008, 0x00010, 0x00020, 0x00040, \
    0x00080, 0x00100, 0x00200, 0x00400, \
    0x00800, 0x01000, 0x02000, 0x04000, \
    0x08000, 0x10000, 0x20000

LISTVALUE(GOTypeProps, GOTYPEPROPERTIES, GOTYPEPROPERTIES_STRINGS, GOTYPEPROPERTIES_VALUES);
LISTVALUECONST(GOTypeProps, GOTYPEPROPERTIES_STRINGS, GOTYPEPROPERTIES_VALUES, 19);

//unsigned Convert2TypeProperties(const String& dataStr)
//{
//    int value = 0;
//    int propvalue;
//
//    Vector<String> properties = dataStr.Split('|');
//
//    for (unsigned i=0; i<properties.Size(); i++)
//    {
//        propvalue = GOTypeProps::GetValue(properties[i].CString());
//        if (propvalue != -1)
//            value = value | propvalue;
//    }
//
//    return (unsigned)value;
//}


GOTInfo::GOTInfo() :
    replicatedMode_(false)
{ }

const GOTInfo GOTInfo::EMPTY = GOTInfo();

const StringHash GOT::COLLECTABLE_ALL = StringHash("All");
const StringHash GOT::COLLECTABLEPART = StringHash("CollectablePart");
const StringHash GOT::BUILDABLEPART = StringHash("BuildablePart");
const StringHash GOT::PART = StringHash("Part");
const StringHash GOT::BONE = StringHash("Bone");
const StringHash GOT::MONEY = StringHash("Money");
const StringHash GOT::START = StringHash("GOT_Start");

Vector<StringHash > GOT::gots_;
Vector<WeakPtr<Node> > GOT::controllables_;
Vector<StringHash> GOT::controllableTypes_;
HashMap<StringHash, GOTInfo > GOT::infos_;
HashMap<StringHash, WeakPtr<Node> > GOT::objects_;
HashMap<StringHash, ResourceRefList > GOT::resourceLists_;
HashMap<StringHash, BuildableObjectInfo > GOT::buildablesInfos_;
HashMap<StringHash, Vector<StringHash> > GOT::buildablesForPart_;

// Temporary HashTable for buildable entityselection
HashMap<StringHash, HashMap<StringHash, String > > buildableEntityStrings_;

/// GOT Setters

void GOT::Clear()
{
    infos_.Clear();

//    typeNames_.Clear();
//    objectsFiles_.Clear();
//    maxDropQuantities_.Clear();
//    typeProperties_.Clear();

    objects_.Clear();
    buildablesInfos_.Clear();
    buildablesForPart_.Clear();

    buildableEntityStrings_.Clear();
}

void GOT::InitDefaultTables()
{
    Clear();

    /// Register(const String& typeName, const StringHash& category, const String& fileName, unsigned typeProperty, bool replicatedMode, int pooltype, int qtyInPool, int scaleVariation, bool entityVariation, bool mappingVariation, int defaultValue, int maxDropQty, const IntVector2& entitySize)

    Register("GOT_Default", COT::OBJECTS, String::EMPTY);
    Register("All", COT::OBJECTS, String::EMPTY, GOT_Collectable);
    Register("Part", COT::OBJECTS, "Data/Objects/part.xml", GOT_None, false, GOT_ObjectPool, 600);
    Register("BuildablePart", COT::OBJECTS, "Data/Objects/part_buildable.xml", GOT_None, true, GOT_ObjectPool, 5000);
    Register("CollectablePart", COT::OBJECTS, "Data/Objects/part_collectable.xml", GOT_None, true, GOT_ObjectPool, 7000);

    /// Money
    Register("Money", COT::MONEY, "Data/Objects/money.xml", GOT_Collectable|GOT_Droppable, true, GOT_ObjectPool, 20, 0, false, false, 1, 1000);

    /// Items
    Register("Bone", COT::ITEMS, String::EMPTY, GOT_Collectable|GOT_Part|GOT_Droppable, true, GOT_None, 0, 0, false, false, 5, 5);
    Register("ElsarionMeat", COT::ITEMS, String::EMPTY, GOT_Collectable|GOT_Part|GOT_Droppable, true, GOT_None, 0, 0, false, false, 5, 5);
//    Register("Potion", COT::ITEMS, "Data/Objects/potion.xml", GOT_Collectable|GOT_Usable|GOT_Droppable, true, GOT_ObjectPool, 20, 0, false, 20, 2);
//    Register("BombeCollec", COT::ITEMS, "Data/Objects/bombe_c.xml", GOT_Collectable|GOT_Usable_DropOut|GOT_Droppable, true, GOT_ObjectPool, 20, 50, 5);

    /// Equipments

    /// Specials Weapons
    Register("Grapin", COT::SPECIALS, "Data/Objects/weapons-grapin.xml", GOT_Collectable|GOT_Wearable|GOT_Droppable, true, GOT_ObjectPool, 5, 0, false, false, 25, 1);
    Register("Lame", COT::SPECIALS, "Data/Objects/weapons-lame.xml", GOT_Collectable|GOT_Wearable|GOT_Droppable, true, GOT_ObjectPool, 5, 0, false, false, 50, 1);
    Register("Bomb", COT::OBJECTS, "Data/Objects/weapons-bomb.xml", GOT_Ability, true, GOT_ObjectPool, 20);
    Register("BigBomb", COT::OBJECTS, "Data/Objects/weapons-bigbomb.xml", GOT_Ability, true, GOT_ObjectPool, 20);

    /// Abilities
    Register("ABI_Grapin", COT::OBJECTS, "Data/Objects/abilities-grapin.xml", GOT_Ability, true, GOT_GOPool, 5);
    Register("ABI_Shooter", COT::OBJECTS, "Data/Objects/abilities-shooter.xml", GOT_Ability, true, GOT_GOPool, 15);
    Register("ABIBomb", COT::OBJECTS, "Data/Objects/abilities-bomb.xml", GOT_Ability, true, GOT_GOPool, 5);

    /// Spots
    Register("GOT_Start", COT::OBJECTS, "Data/Objects/start.xml", GOT_None, false, GOT_ObjectPool, 50);

    /// Effect Objects
    Register("GOT_Rain", COT::OBJECTS, "Data/Objects/droplet.xml", GOT_Effect, false);
//    Register("GOT_Flame", COT::OBJECTS, "Data/Objects/flame.xml", GOT_Effect);
//    Register("GOT_LifeFlame", COT::OBJECTS, "Data/Objects/lifeflame.xml", GOT_Effect);
//    Register("GOT_Water", COT::LIQUIDS, "Data/Objects/water.xml", GOT_Effect, false, 10);
//    Register("GOT_Scraps", COT::OBJECTS, "Data/Objects/scraps.xml", GOT_Effect);

    /// Spawner
//    Register("GOT_Spawner", COT::OBJECTS, "Data/Objects/spawner.xml", GOT_None);
}

StringHash GOT::Register(const String& typeName, const StringHash& category, const String& fileName,
                         unsigned typeProperty, bool replicatedMode, int pooltype, int qtyInPool, int scaleVariation, bool entityVariation,
                         bool mappingVariation, int defaultValue, int maxDropQty, const IntVector2& entitySize)
{
    StringHash hashName(typeName);

    if (infos_.Contains(hashName))
    {
//        URHO3D_LOGINFOF("GOT() : Register templateName=%s(%u) (Warning:infos_ contains already the key)", typeName.CString(), hashName.Value());
        return hashName;
    }

    gots_.Push(hashName);

    GOTInfo& info = infos_[hashName];

    info.got_ = hashName;
    info.typename_ = typeName;
    info.filename_ = fileName;
    info.category_ = category;
    info.properties_ = typeProperty;
    info.replicatedMode_ = replicatedMode;
    info.pooltype_ = pooltype;
    info.poolqty_ = qtyInPool;
    info.scaleVariation_ = scaleVariation;
    info.entityVariation_ = entityVariation;
    info.mappingVariation_ = mappingVariation;
    info.maxdropqty_ = maxDropQty;
    info.defaultvalue_ = defaultValue;
    info.entitySize_ = entitySize;

    COT::AddTypeInCategory(category, hashName);

    URHO3D_LOGINFOF("GOT() : Register templateName=%s(%u) : cat=%u file=%s typeProps=%u replicatedMode=%s qtyInPool=%d value=%d maxDropQty=%d",
                    typeName.CString(), hashName.Value(), category.Value(), fileName.CString(), typeProperty,
                    replicatedMode ? "true" : "false", qtyInPool, defaultValue, maxDropQty);

    return hashName;
}

void GOT::RegisterBuildablePartsForType(const String& typeName, const String& typePartName, const String& partNames)
{
    StringHash type(typeName);
    StringHash partType(typePartName);

    URHO3D_LOGINFOF("GOT() : RegisterBuildablePartsForType type=%s(%u) part=%s(%u) partNames=%s",
                    typeName.CString(), type.Value(), typePartName.CString(), partType.Value(), partNames.CString());

    Vector<String>& parts = buildablesInfos_[type].partnames_;
    parts = partNames.Split('|');

    Vector<StringHash>& buildablesForPart = buildablesForPart_[partType];

    if (!buildablesForPart.Contains(type))
        buildablesForPart.Push(type);
}

//"entitySelection":"Simple1=Default|Warrior1=Swords;Helmets|Pirate1=Guns"

void GOT::RegisterBuildableEntitySelection(const String& typeName, const String& entitySelection)
{
    StringHash type(typeName);

    URHO3D_LOGINFOF("GOT() : RegisterBuildableEntitySelection type=%s(%u) entitySelection=%s",
                    typeName.CString(), type.Value(), entitySelection.CString());

    HashMap<StringHash, String >& strings = buildableEntityStrings_[type];
    HashMap<StringHash, unsigned >& selections = buildablesInfos_[type].entitySelection_;

    Vector<String> associations = entitySelection.Split('|');
    for (Vector<String>::ConstIterator it = associations.Begin(); it != associations.End(); ++it)
    {
        Vector<String> names = it->Split('=');
        if (names.Size() <= 1)
            continue;

        const String& entityname = names[0];

        Vector<String> objectnames = names[1].Split(';');
        for (Vector<String>::ConstIterator jt=objectnames.Begin(); jt != objectnames.End(); ++jt)
        {
            StringHash objectHash(*jt);
            strings[objectHash] = entityname;
            // initialize entityid = 0
            // it will be setted after preloading objects
            selections[objectHash] = 0;
        }
    }
}

void GOT::LoadJSONFile(Context* context, const String& name)
{
    JSONFile* jsonFile = context->GetSubsystem<ResourceCache>()->GetResource<JSONFile>(name);
    if (!jsonFile)
    {
        URHO3D_LOGWARNINGF("GOT() - LoadJSONFile : %s not exist !", name.CString());
        return;
    }

    const JSONValue& source = jsonFile->GetRoot();

    URHO3D_LOGINFO("GOT() - LoadJSONFile : " + name);

    for (JSONObject::ConstIterator i = source.Begin(); i != source.End(); ++i)
    {
        const String& typeName = i->first_;
        if (typeName.Empty())
        {
            URHO3D_LOGWARNING("GOT() - LoadJSONFile : typeName is empty !");
            continue;
        }

        String categoryname, sscategoryname, fileName, typeProperty;
        bool replicateMode=false;
        int qtyInPool=0;
        int scaleVariation=0;
        bool entityVariation = false;
        bool mappingVariation = false;
        int defaultValue=0;
        int maxDropQty=0;
        String typePartName;
        String partNames;
        String entitySelection;
        IntVector2 entitySize(3,2);

        const JSONValue& attributes = i->second_;
        for (JSONObject::ConstIterator j = attributes.Begin(); j != attributes.End(); ++j)
        {
            const String& attribute = j->first_;
            if (attribute == "category")
                categoryname = j->second_.GetString();
            else if (attribute == "sscategory")
                sscategoryname = j->second_.GetString();
            else if (attribute == "fileName")
                fileName = j->second_.GetString();
            else if (attribute == "typeProperty")
                typeProperty = j->second_.GetString();
            else if (attribute == "replicateMode")
                replicateMode = j->second_.GetInt() > 0;
            else if (attribute == "qtyInPool")
                qtyInPool = j->second_.GetInt();
            else if (attribute == "scaleVariation")
                scaleVariation = j->second_.GetInt();
            else if (attribute == "entityVariation")
                entityVariation = j->second_.GetInt();
            else if (attribute == "mappingVariation")
                mappingVariation = j->second_.GetInt();
            else if (attribute == "defaultValue")
                defaultValue = j->second_.GetInt();
            else if (attribute == "maxDropQty")
                maxDropQty = j->second_.GetInt();
            else if (attribute == "typePartName")
                typePartName = j->second_.GetString();
            else if (attribute == "partNames")
                partNames = j->second_.GetString();
            else if (attribute == "entitySelection")
                entitySelection = j->second_.GetString();
            else if (attribute == "sizex")
                entitySize.x_ = j->second_.GetInt();
            else if (attribute == "sizey")
                entitySize.y_ = j->second_.GetInt();
            else
            {
                URHO3D_LOGWARNING("GOT() - LoadJSONFile : attribute is unknown, typeName=\"" + typeName + "\"");
                continue;
            }
        }

        Vector<String> categories = categoryname.Split('|');
        StringHash got = Register(typeName, StringHash(categories.Size() ? categories[0] : String::EMPTY), "Data/" + fileName, GOTypeProps::GetValue(typeProperty),
                                  replicateMode, qtyInPool ? GOT_ObjectPool : 0, qtyInPool, scaleVariation,
                                  entityVariation, mappingVariation, defaultValue, maxDropQty, entitySize);

        for (unsigned i=1; i<categories.Size(); ++i)
            COT::AddTypeInCategory(StringHash(categories[i]), got);

        if (!sscategoryname.Empty())
        {
            Vector<String> sscategories = sscategoryname.Split('|');
            for (unsigned i=0; i<sscategories.Size(); ++i)
                COT::AddTypeInCategory(StringHash(sscategories[i]), got);
        }

        if (!typePartName.Empty() && !partNames.Empty())
            RegisterBuildablePartsForType(typeName, typePartName, partNames);

        if (!entitySelection.Empty())
            RegisterBuildableEntitySelection(typeName, entitySelection);
    }
}

bool GOT::PreLoadObjects(int& state, HiresTimer* timer, const long long& delay, Node* preloaderGOT, bool useObjectPool)
{
    static HashMap<StringHash, GOTInfo >::ConstIterator gotinfosIt;
    static HashMap<StringHash, WeakPtr<Node> >::ConstIterator objectsIt;
    static HashMap<StringHash, BuildableObjectInfo >::Iterator buildablesIt;

    URHO3D_LOGINFOF("GOT() - PreLoadObjects : state=%d", state);

    // Reset ObjectPool
    if (state == 3)
    {
        objects_.Clear();
        resourceLists_.Clear();

        TimerRemover::Reset(preloaderGOT->GetContext(), 1000);

        // Reset the object pool
        if (useObjectPool)
            ObjectPool::Reset(preloaderGOT);

        gotinfosIt = infos_.Begin();
        state++;

        if (TimeOver(timer, delay))
            return false;
    }

    // Set Template nodes
    if (state == 4)
    {
        if (gotinfosIt != infos_.End())
        {
            for (; gotinfosIt != infos_.End(); ++gotinfosIt)
            {
                const StringHash& got = gotinfosIt->first_;
                const GOTInfo& info = gotinfosIt->second_;

                if (info.filename_.Empty())
                    continue;

                if (objects_.Find(got) != objects_.End())
                    continue;

                // Set Object Template File
                WeakPtr<Node> templateNode = WeakPtr<Node>(preloaderGOT->CreateChild(String::EMPTY, LOCAL));

                URHO3D_LOGINFOF("GOT() - PreLoadObjects : Object %s(%u) templateNode=%u hasReplicateMode=%s",
                                 info.filename_.CString(), got.Value(), templateNode->GetID(), info.replicatedMode_ ? "true" : "false");

                if (!GameHelpers::LoadNodeXML(preloaderGOT->GetContext(), templateNode, info.filename_.CString(), LOCAL, false))
                    continue;

                objects_[got] = templateNode;
                templateNode->SetVar(GOA::GOT, got.Value());

                templateNode->ApplyAttributes();
                templateNode->SetEnabledRecursive(false);
                //templateNode->SetEnabled(false);

                // Set Controllable list
                if (info.properties_ & GOT_Controllable)
                {
                    controllables_.Push(templateNode);
                    controllableTypes_.Push(got);

                    URHO3D_LOGINFOF("GOT() - PreLoadObjects : Object %s(%u) Controllable", GOT::GetType(got).CString(), got.Value());
                }
#ifdef ACTIVE_LAYERMATERIALS
                Material* layermaterial = GameContext::Get().layerMaterials_[info.properties_ & GOT_Furniture ? LAYERFURNITURES : LAYERACTORS];
                {
                    PODVector<StaticSprite2D*> staticsprites;
                    templateNode->GetDerivedComponents<StaticSprite2D>(staticsprites, true);
                    for (PODVector<StaticSprite2D*>::Iterator it = staticsprites.Begin(); it != staticsprites.End(); ++it)
                    {
                        StaticSprite2D* ssprite = *it;
                        if (ssprite->GetType() != AnimatedSprite2D::GetTypeStatic())
                            ssprite->SetCustomMaterial(layermaterial);
                        else if (static_cast<AnimatedSprite2D*>(ssprite)->GetRenderTargetAttr().Empty())
                            ssprite->SetCustomMaterial(layermaterial);
                    }
                }
                {
                    PODVector<ParticleEmitter2D*> particles;
                    templateNode->GetComponents<ParticleEmitter2D>(particles, true);
                    for (PODVector<ParticleEmitter2D*>::Iterator it = particles.Begin(); it != particles.End(); ++it)
                        (*it)->SetCustomMaterial(layermaterial);
                }
#endif
                if (TimeOver(timer, delay))
                    return false;
            }
        }

        state++;
    }

    // Set ObjectPool Categories
    if (state == 5)
    {
        if (useObjectPool && !ObjectPool::Get()->CreateCategories(GameContext::Get().rootScene_, infos_, objects_, timer, delay))
            return false;

        objectsIt = objects_.Begin();
        state++;
    }

    // Set Sprite RefResourceList for wearable objects
    if (state == 6)
    {
        if (objectsIt != objects_.End())
        {
            Vector<String> list;
            StringHash refType;

            while (objectsIt != objects_.End())
//            for (; objectsIt!=objects_.End(); ++objectsIt)
            {
                const StringHash& got = objectsIt->first_;

                if ((GOT::GetTypeProperties(got) & GOT_Wearable) == 0)
                {
                    ++objectsIt;
                    continue;
                }
                if ((GOT::GetTypeProperties(got) & GOT_Animation) != 0)
                {
                    ++objectsIt;
                    continue;
                }

                list.Clear();
                // RefType == SpriteSheet2D or Sprite2D
                refType = StringHash::ZERO;

                URHO3D_LOGERRORF("GOT() - PreLoadObjects : Object %s(%u) Wearable ...", GOT::GetType(got).CString(), got.Value());

                AnimatedSprite2D* animatedSprite = objectsIt->second_->GetComponent<AnimatedSprite2D>();

                if (animatedSprite)
                {
                    refType = SpriteSheet2D::GetTypeStatic();

//                    const unsigned MAX_MAPPINGSIZE = 30;
//                    AnimationSet2D* set2d = animatedSprite->GetAnimationSet();
//                    SpriteSheet2D* sheet2d = set2d->GetSpriteSheet();
//                    URHO3D_LOGERRORF("GOT() - PreLoadObjects : Object %s(%u) Wearable ... spritesheet=%s ...", GOT::GetType(got).CString(), got.Value(), sheet2d ? sheet2d->GetName().CString() : "null");
//
//                    if (set2d->GetSpriteMapping().Empty() || set2d->GetSpriteMapping().Size() > MAX_MAPPINGSIZE)
//                        URHO3D_LOGERRORF("GOT() - PreLoadObjects : Object %s(%u) Wearable ...  %s reftype=%s(%u) mappingsize=%d ... error !",
//                                         GOT::GetType(got).CString(), got.Value(), set2d->GetName().CString(), GameContext::Get().context_->GetTypeName(refType), refType.Value(), set2d->GetSpriteMapping().Size());
//
//                    if (sheet2d)
//                    {
//                        const HashMap<unsigned, SharedPtr<Sprite2D> >& mapping = set2d->GetSpriteMapping();
//                        list.Resize(Min(mapping.Size(), MAX_MAPPINGSIZE));
//
//                        unsigned mindex=0;
//                        HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator mt = mapping.Begin();
//                        while (mt != mapping.End() && mindex < MAX_MAPPINGSIZE)
//                        {
//                            list[mindex] = mt->second_ ? Sprite2D::SaveToResourceRef(mt->second_).name_ : String::EMPTY;
//                            mindex++;
//                            mt++;
//                        }
//                    }

                    const HashMap<unsigned, SharedPtr<Sprite2D> >& mapping = animatedSprite->GetAnimationSet()->GetSpriteMapping();
                    list.Resize(mapping.Size());
                    unsigned i=0;
                    for (HashMap<unsigned, SharedPtr<Sprite2D> >::ConstIterator it2=mapping.Begin(); it2!=mapping.End(); ++it2,++i)
                        list[i] = Sprite2D::SaveToResourceRef(it2->second_).name_;
                    refType = Sprite2D::SaveToResourceRef(mapping.Begin()->second_).type_;
                }
                else
                {
                    StaticSprite2D* staticSprite = objectsIt->second_->GetComponent<StaticSprite2D>();
                    if (staticSprite)
                    {
                        list.Resize(1);
                        ResourceRef ref = Sprite2D::SaveToResourceRef(staticSprite->GetSprite());
                        list[0] = ref.name_;
                        refType = ref.type_;
                    }
                }

                if (list.Size())
                {
                    resourceLists_[got] = ResourceRefList(refType, list);
                    URHO3D_LOGINFOF("GOT() - PreLoadObjects : Object %s(%u) Wearable => list = %s", GOT::GetType(got).CString(), got.Value(),
                                    resourceLists_[got].ToString().CString());
                }

                ++objectsIt;

                if (TimeOver(timer, delay))
                    return false;
            }
        }

        buildablesIt = buildablesInfos_.Begin();
        state++;
    }

    // Set Spriter Entity Indexes for buildable objects
    if (state == 7)
    {
        if (buildablesIt != buildablesInfos_.End())
        {
            for (; buildablesIt!=buildablesInfos_.End(); ++buildablesIt)
            {
                const StringHash& got = buildablesIt->first_;
                BuildableObjectInfo& bobjectinfo = buildablesIt->second_;
                HashMap<StringHash, unsigned >& selection = bobjectinfo.entitySelection_;

                Node* templatenode = GetObject(got);
                if (!templatenode)
                {
                    bobjectinfo.entitySelection_.Clear();
                    continue;
                }
                AnimatedSprite2D* animatedSprite = templatenode->GetComponent<AnimatedSprite2D>();
                if (!animatedSprite)
                {
                    bobjectinfo.entitySelection_.Clear();
                    continue;
                }
                unsigned numentities = animatedSprite->GetNumSpriterEntities();
                if (numentities <= 1)
                {
                    bobjectinfo.entitySelection_.Clear();
                    continue;
                }

                const HashMap<StringHash, String >& strings = buildableEntityStrings_[got];
                for (HashMap<StringHash, String>::ConstIterator jt = strings.Begin(); jt != strings.End(); ++jt)
                {
                    const String& entityname = jt->second_;
                    for (unsigned i=0; i < numentities; i++)
                    {
                        if (animatedSprite->GetSpriterEntity(i) == entityname)
                        {
                            bobjectinfo.entitySelection_[jt->first_] = i;
                            break;
                        }
                    }
                }

                URHO3D_LOGINFOF("GOT() - PreLoadObjects : Object %s(%u) Buildable", GOT::GetType(got).CString(), got.Value());

                if (TimeOver(timer, delay))
                    return false;
            }
        }

        buildableEntityStrings_.Clear();

        URHO3D_LOGINFOF("GOT() - PreloadObjects ... OK !");

        return true;
    }

    return false;
}

void GOT::UnLoadObjects(Node* root)
{
    URHO3D_LOGINFOF("GOT() - UnloadObjects ...");

    Context* context = root->GetContext();

    objects_.Clear();

    // Erase Pools
    ObjectPool::Reset();
    GO_Pools::Reset();

    root->Remove();

    TimerRemover::Reset(context, 0);
    resourceLists_.Clear();

    URHO3D_LOGINFOF("GOT() - UnloadObjects ... OK !");
}


/// GOT Getters


unsigned GOT::GetSize()
{
    return gots_.Size();
}

const StringHash& GOT::Get(unsigned index)
{
    return index < gots_.Size() ? gots_[index] : StringHash::ZERO;
}

unsigned GOT::GetIndex(const StringHash& type)
{
    Vector<StringHash >::ConstIterator it = gots_.Find(type);
    return it != gots_.End() ? it - gots_.Begin() : 0;
}

bool GOT::IsRegistered(const StringHash& type)
{
    return gots_.Contains(type);
}

const GOTInfo& GOT::GetConstInfo(const StringHash& type)
{
    HashMap<StringHash, GOTInfo >::ConstIterator it = infos_.Find(type);
    return it != infos_.End() ? it->second_ : GOTInfo::EMPTY;
}

const String& GOT::GetType(const StringHash& type)
{
    if (!infos_.Contains(type)) return String::EMPTY;
    return infos_[type].typename_;
}

bool GOT::HasObjectFile(const StringHash& type)
{
    if (!infos_.Contains(type)) return false;
    return !infos_[type].filename_.Empty();
}

const String& GOT::GetObjectFile(const StringHash& type)
{
    if (!infos_.Contains(type)) return String::EMPTY;
    return infos_[type].filename_;
}

const StringHash& GOT::GetCategory(const StringHash& type)
{
    if (!infos_.Contains(type)) return StringHash::ZERO;
    return infos_[type].category_;
}

unsigned GOT::GetTypeProperties(const StringHash& type)
{
    if (!infos_.Contains(type)) return 0U;
    return infos_[type].properties_;
}

bool GOT::HasReplicatedMode(const StringHash& type)
{
    if (!infos_.Contains(type)) return false;
    return infos_[type].replicatedMode_;
}

int GOT::GetMaxDropQuantityFor(const StringHash& type)
{
    if (!infos_.Contains(type)) return 0;
    return infos_[type].maxdropqty_;
}

int GOT::GetDefaultValue(const StringHash& type)
{
    if (!infos_.Contains(type)) return 0;
    return infos_[type].defaultvalue_;
}

bool GOT::HasObject(const StringHash& type)
{
    HashMap<StringHash, WeakPtr<Node> >::ConstIterator it = objects_.Find(type);
    return it != objects_.End();
}

Node* GOT::GetObject(const StringHash& type)
{
    HashMap<StringHash, WeakPtr<Node> >::ConstIterator it = objects_.Find(type);
    return it != objects_.End() ? it->second_.Get() : 0;
//    return it != objects_.End() ? it->second_ : 0;
}

const ResourceRefList& GOT::GetResourceList(const StringHash& type)
{
    return resourceLists_[type];
}

const StringHash& GOT::GetBuildableType(const StringHash& partType)
{
    return buildablesForPart_.Contains(partType) ? buildablesForPart_[partType].Back() : StringHash::ZERO;
}

const HashMap<StringHash, unsigned >* GOT::GetBuildableEntitySelection(const StringHash& buildableType)
{
    HashMap<StringHash, BuildableObjectInfo >::ConstIterator it = buildablesInfos_.Find(buildableType);
    return it != buildablesInfos_.End() ? &it->second_.entitySelection_ : 0;
}

int GOT::GetBuildableEntityIdFor(const StringHash& buildableType, const StringHash& otherObject)
{
    const HashMap<StringHash, unsigned >* entityselection = GOT::GetBuildableEntitySelection(buildableType);
    if (!entityselection)
        return -1;

    HashMap<StringHash, unsigned >::ConstIterator jt = entityselection->Find(otherObject);
    return jt != entityselection->End() ? jt->second_ : -1;
}

Vector<String>* GOT::GetPartsNamesFor(const StringHash& buildableType)
{
    HashMap<StringHash, BuildableObjectInfo >::Iterator it = buildablesInfos_.Find(buildableType);
    return it != buildablesInfos_.End() ? &(it->second_.partnames_) : 0;
}

const String& GOT::GetPartNameFor(const StringHash& buildableType, unsigned partIndex)
{
    HashMap<StringHash, BuildableObjectInfo >::ConstIterator it = buildablesInfos_.Find(buildableType);
    if (it != buildablesInfos_.End())
    {
        const Vector<String>& partnames = it->second_.partnames_;
        return partIndex < partnames.Size() ? partnames[partIndex] : String::EMPTY;
    }
    return String::EMPTY;
}

int GOT::GetPartIndexFor(const StringHash& buildableType, const String& partName)
{
    HashMap<StringHash, BuildableObjectInfo >::ConstIterator it = buildablesInfos_.Find(buildableType);
    if (it != buildablesInfos_.End())
    {
        const Vector<String>& partnames = it->second_.partnames_;

        if (partName == "Random")
            return Random(0, partnames.Size());

        Vector<String>::ConstIterator it2 = partnames.Find(partName);
        return it2 != partnames.End() ? it2 - partnames.Begin() : -1;
    }
    return -1;
}

int GOT::GetControllableIndex(const StringHash& type)
{
    Vector<StringHash>::ConstIterator it = controllableTypes_.Find(type);
    return it != controllableTypes_.End() ? it-controllableTypes_.Begin() : -1;
}

const StringHash& GOT::GetControllableType(int index)
{
    return index < controllableTypes_.Size() ? controllableTypes_[index] : StringHash::ZERO;
}

Node* GOT::GetControllableTemplate(int index)
{
    return (unsigned)index < controllables_.Size() ? controllables_[index].Get() : 0;
}

Node* GOT::GetControllableTemplate(const StringHash& type)
{
    int index = GetControllableIndex(type);
    return index != -1 ? GetControllableTemplate(index) : 0;
}

Node* GOT::GetClonedControllableFrom(Node* templateNode, unsigned nodeid, CreateMode mode)
{
    URHO3D_LOGINFOF("GOT() - GetClonedControllableFrom : ... Clone Template=%u on nodeId=%u ...", templateNode->GetID(), nodeid);

    if (!GameContext::Get().controllablesNode_)
        GameContext::Get().controllablesNode_ = GameContext::Get().rootScene_->CreateChild("Controllables");

    Node* node = templateNode->Clone(mode, false, nodeid, 0, GameContext::Get().controllablesNode_);

    if (!node)
        URHO3D_LOGERRORF("GOT() - GetClonedControllableFrom : ... NOK !");
    else
        URHO3D_LOGINFOF("GOT() - GetClonedControllableFrom : ... Clone Template=%u on nodeId=%u ... OK !", templateNode->GetID(), node->GetID());

    return node;
}

Node* GOT::GetClonedControllable(int index, unsigned nodeid, CreateMode mode)
{
    Node* templateNode = GOT::GetControllableTemplate(index);
    if (!templateNode)
    {
        URHO3D_LOGERRORF("GOT() - GetClonedControllable : ... no TemplateNode NOK !");
        return 0;
    }

    if (!nodeid)
    {
        const StringHash& type = controllableTypes_[index];
        mode = mode==REPLICATED && HasReplicatedMode(type) ? REPLICATED : LOCAL;
        nodeid = GameContext::Get().rootScene_->GetFreeNodeID(mode);
    }
    else
    {
        mode = nodeid < FIRST_LOCAL_ID ? REPLICATED : LOCAL;
    }

    return GetClonedControllableFrom(templateNode, nodeid, mode);
}

Node* GOT::GetClonedControllable(const StringHash& type, unsigned nodeid, CreateMode mode)
{
    Node* templateNode = GOT::GetControllableTemplate(type);
    if (!templateNode)
    {
        URHO3D_LOGERRORF("GOT() - GetClonedControllable : ... no TemplateNode NOK !");
        return 0;
    }

    if (!nodeid)
    {
        mode = mode==REPLICATED && HasReplicatedMode(type) ? REPLICATED : LOCAL;
        nodeid = GameContext::Get().rootScene_->GetFreeNodeID(mode);
    }
    else
    {
        mode = nodeid < FIRST_LOCAL_ID ? REPLICATED : LOCAL;
    }

    return GetClonedControllableFrom(templateNode, nodeid, mode);
}

void GOT::DumpAll()
{
    URHO3D_LOGINFOF("GOT() - DumpAll : infos_ Size =", infos_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, GOTInfo >::ConstIterator it=infos_.Begin(); it!=infos_.End(); ++it, ++index)
    {
        const GOTInfo& info = it->second_;
        URHO3D_LOGINFOF("     => infos_[%u] = (hash:%u - typeName:%s category:%s properties:%u fileName:%s)", index, it->first_.Value(),
                        info.typename_.CString(), COT::GetName(GetCategory(it->first_)).CString(), info.properties_, info.filename_.CString());
    }
}

String GOT::Dump(const GOTInfo& info)
{
    String str;
    str = "got:" + info.typename_ + "(" + String(info.got_.Value()) + ") cot:" + COT::GetName(info.category_) + "(" + String(info.category_.Value()) + ") filename:" + info.filename_;
    return str;
}

/// Category Object Types : COT
HashMap<StringHash, String> COT::categoryNames_;
//HashMap<StringHash, unsigned > COT::categoryAttributes_;
HashMap<StringHash, Vector<StringHash> > COT::typesByCategory_;
HashMap<StringHash, StringHash> COT::categoryBases_;
HashMap<StringHash, Vector<StringHash> > COT::categoriesByBase_;

const StringHash COT::OBJECTS               = StringHash("Objects");
const StringHash COT::LIQUIDS               = StringHash("Liquids");

const StringHash COT::MONEY                 = StringHash("Money");
const StringHash COT::ITEMS                 = StringHash("Items");
const StringHash COT::ABILITY               = StringHash("Ability");
const StringHash COT::AVATAR                = StringHash("Avatar");
const StringHash COT::QUEST                 = StringHash("Quest");

const StringHash COT::DUNGEON               = StringHash("Dungeon");

const StringHash COT::CHARACTERS            = StringHash("Characters");
const StringHash COT::MONSTERS              = StringHash("Monsters");

const StringHash COT::EQUIPMENTS            = StringHash("Equipments");
const StringHash COT::WEAPONS               = StringHash("Weapons");
const StringHash COT::MELEEWEAPONS          = StringHash("MeleeWeapons");
const StringHash COT::RANGEDWEAPONS         = StringHash("RangedWeapons");
const StringHash COT::ARMORS                = StringHash("Armors");
const StringHash COT::HELMETS               = StringHash("Helmets");
const StringHash COT::GLOVES                = StringHash("Gloves");
const StringHash COT::BOOTS                 = StringHash("Boots");
const StringHash COT::BELTS                 = StringHash("Belts");
const StringHash COT::CAPES                 = StringHash("Capes");
const StringHash COT::HEADBANDS             = StringHash("Headbands");

const StringHash COT::SPECIALS              = StringHash("Specials");

const StringHash COT::DOORS                 = StringHash("Doors");
const StringHash COT::CONTAINERS            = StringHash("Containers");
const StringHash COT::PLANTS                = StringHash("Plants");
const StringHash COT::LIGHTS                = StringHash("Lights");
const StringHash COT::DECORATIONS           = StringHash("Decorations");
const StringHash COT::TOOLS                 = StringHash("Tools");

bool COT::IsACategory(const StringHash& hash)
{
    HashMap<StringHash, String>::ConstIterator it = categoryNames_.Find(hash);
    return it != categoryNames_.End();
}

const String& COT::GetName(const StringHash& category)
{
    HashMap<StringHash, String>::ConstIterator it = categoryNames_.Find(category);
    return it != categoryNames_.End() ? it->second_ : String::EMPTY;
}

StringHash COT::Register(const String& category, const StringHash& base)
{
    StringHash hashName = StringHash(category);

    if (categoryNames_.Contains(hashName))
        return hashName;

    categoryNames_[hashName] = category;

    typesByCategory_[hashName] = Vector<StringHash>();
    categoryBases_[hashName] = base;

    if (base == StringHash::ZERO)
        categoriesByBase_[hashName] = Vector<StringHash>();

    else if (!categoriesByBase_[base].Contains(hashName))
        categoriesByBase_[base].Push(hashName);

    return hashName;
}

void COT::AddTypeInCategory(const StringHash& category, const StringHash& type)
{
    if (!typesByCategory_[category].Contains(type))
        typesByCategory_[category].Push(type);
}

bool COT::IsInCategory(const StringHash& type, const StringHash& category)
{
    return typesByCategory_[category].Contains(type);
}

int COT::IsInCategories(const StringHash& type, const Vector<StringHash>& categories)
{
    for (int i=0; i < categories.Size(); i++)
    {
        if (COT::IsInCategory(type, categories[i]))
            return i;
    }

    return -1;
}

void COT::GetAllTypesInCategory(Vector<StringHash>& gots, const StringHash& category)
{
    gots += GetTypesInCategory(category);

    if (categoriesByBase_.Contains(category))
    {
        const Vector<StringHash>& subcategories = categoriesByBase_[category];
        for (unsigned i=0; i < subcategories.Size(); i++)
        {
            GetAllTypesInCategory(gots, subcategories[i]);
        }
    }

}

#define NUMTRIES 4

StringHash COT::GetRandomCategoryFrom(const StringHash& base, int rand)
{
//    URHO3D_LOGINFOF("COT::GetRandomCategoryFrom CategoryBase=%s rand=%d", COT::GetName(base).CString(), rand);

    StringHash category = base;
    int tries = 0;

    if (categoriesByBase_[base].Size())
    {
        while (category == base && tries++ < NUMTRIES)
        {
            int newrand = (rand ? (rand+tries) % (categoriesByBase_[base].Size() + 1) : Random(0, categoriesByBase_[base].Size()+1));
            if (newrand == 0)
            {
                category = base;
                break;
            }

            category = categoriesByBase_[base][(unsigned)(newrand-1)];

            if (tries < NUMTRIES)
            {
                if (categoriesByBase_[category].Size())
                    category = GetRandomCategoryFrom(category, rand ? newrand : 0);
                if (!typesByCategory_[category].Size())
                    category = base;
            }
            else if (!typesByCategory_[category].Size())
                category = base;
        }
    }

    if (typesByCategory_[category].Size())
        return category;
    else
        return base;
}

const StringHash& COT::GetRandomTypeFrom(const StringHash& base, int rand)
{
//    URHO3D_LOGINFOF("COT::GetRandomTypeFrom Category=%s rand=%d", COT::GetName(base).CString(), rand);
    if (!categoryNames_.Contains(base))
        return base;

    StringHash category = GetRandomCategoryFrom(base, rand);

    unsigned size = typesByCategory_[category].Size();

    if (size)
    {
        const StringHash& type = typesByCategory_[category][rand ? rand % size : Random(0, size)];
//        URHO3D_LOGINFOF("COT::GetRandomTypeFrom Category=%s Type=%s", COT::GetName(category).CString(), GOT::GetType(type).CString());

        return type;
    }
    else
        URHO3D_LOGWARNINGF("COT::GetRandomTypeFrom Category=%s : no type in Category !", COT::GetName(category).CString());

    return StringHash::ZERO;
}

const StringHash& COT::GetTypeFrom(const StringHash& category, unsigned index)
{
    unsigned size = typesByCategory_[category].Size();
    return size ? typesByCategory_[category][index%size] : StringHash::ZERO;
}

Node* COT::GetObjectFromCategory(const StringHash& category, unsigned index)
{
    if (index < typesByCategory_[category].Size())
    {
        const StringHash& type = typesByCategory_[category][index];
//        URHO3D_LOGINFOF("COT::GetObjectFromCategory Category=%s Type=%s(%u)", COT::GetName(category).CString(), GOT::GetType(type).CString(), index);

        return GOT::GetObject(type);
    }

    return 0;
}

void COT::InitCategoryTables()
{
    categoryNames_.Clear();
    //categoryAttributes_.Clear();
    typesByCategory_.Clear();

    Register("Objects");

    Register("Money");
    Register("Items");
    Register("Ability");
    Register("Avatar");
    Register("Quest");

    Register("Characters");
    Register("Monsters", COT::OBJECTS);

    Register("Liquids", COT::OBJECTS);

    Register("Dungeon", COT::OBJECTS);

    Register("DungeonBottom", COT::DUNGEON);
    Register("DungeonSides", COT::DUNGEON);

    Register("Corridor", COT::DUNGEON);
    Register("Chamber", COT::DUNGEON);
    Register("Bedroom", COT::DUNGEON);
    Register("Office", COT::DUNGEON);
    Register("Kitchen", COT::DUNGEON);
    Register("Library", COT::DUNGEON);
    Register("Forge", COT::DUNGEON);
    Register("Hall", COT::DUNGEON);
    Register("Refectory", COT::DUNGEON);
    Register("Sorcererroom", COT::DUNGEON);
    Register("Priestroom", COT::DUNGEON);

    Register("BiomeCave", COT::OBJECTS);
    Register("BiomeCaveSides", COT::OBJECTS);
    Register("BiomeCaveBottom", COT::OBJECTS);

    Register("BiomeExternal", COT::OBJECTS);
    Register("BiomeExternalSides", COT::OBJECTS);
    Register("BiomeExternalBottom", COT::OBJECTS);

    Register("BiomeBackGround", COT::OBJECTS);
    Register("BiomeBackGroundSides", COT::OBJECTS);
    Register("BiomeBackGroundBottom", COT::OBJECTS);

    Register("Equipments", COT::ITEMS);

    Register("Weapons", COT::EQUIPMENTS);
    Register("Armors", COT::EQUIPMENTS);
    Register("Specials", COT::EQUIPMENTS);
    Register("Helmets", COT::EQUIPMENTS);
    Register("Belts", COT::EQUIPMENTS);
    Register("Capes", COT::EQUIPMENTS);
    Register("Headbands", COT::EQUIPMENTS);
    Register("Gloves", COT::EQUIPMENTS);
    Register("Boots", COT::EQUIPMENTS);

    Register("MeleeWeapons", COT::WEAPONS);
    Register("RangedWeapons", COT::WEAPONS);

    Register("Doors");
    Register("Containers", COT::OBJECTS);
    Register("Plants");
    Register("Lights");
    Register("Decorations");
    Register("Tools");

    categoryNames_.Sort();
    //categoryAttributes_.Sort();
    typesByCategory_.Sort();
    categoryBases_.Sort();
    categoriesByBase_.Sort();
}

void COT::DumpAll()
{
    URHO3D_LOGINFOF("COT() - DumpAll : Size =", categoryNames_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String >::ConstIterator it=categoryNames_.Begin(); it!=categoryNames_.End(); ++it,++index)
    {
        URHO3D_LOGINFOF("     => categoryNames_[%u] = (hash:%u(dec:%d) - name:%s)", index, it->first_.Value(), it->first_.Value(), it->second_.CString());
        const Vector<StringHash>& gots = GetTypesInCategory(it->first_);
        if (gots.Size())
        {
            for (unsigned i=0; i< gots.Size(); i++)
            {
                URHO3D_LOGINFOF("       => gameObject_[%u] = (hash:%u - file:%s)", i, gots[i].Value(), GOT::GetObjectFile(gots[i]).CString());
            }
        }
    }
}


/// GO States

Vector<StringHash> GOS::states_;
HashMap<StringHash, String> GOS::stateNames_;

void GOS::InitStateTable()
{
    states_.Clear();
    stateNames_.Clear();

    Register("State_Appear");
    Register("State_Default");
    Register("State_Default_Ground");
    Register("State_Default_Air");
    Register("State_Default_Fluid");
    Register("State_Default_Climb");
    Register("State_Walk");
    Register("State_Climb");
    Register("State_FlyUp");
    Register("State_FlyDown");
    Register("State_Swim");
    Register("State_Run");
    Register("State_Attack");
    Register("State_Attack_Ground");
    Register("State_Attack_Air");
    Register("State_Shoot");
    Register("State_Jump");
    Register("State_Fall");
    Register("State_Hurt");
    Register("State_Dead");
    Register("State_Explode");
    Register("State_Talk");
    Register("State_Disappear");
    Register("State_Destroy");
    Register("State_Lighted");
    Register("State_Unlighted");
}

const StringHash& GOS::Register(const String& stateName)
{
    StringHash state = StringHash(stateName);

    Vector<StringHash>::ConstIterator it = states_.Find(state);
    if (it != states_.End())
    {
//        URHO3D_LOGINFOF("GOS() : Register StateName=%s(%u) (Warning:states_ contains already the key)", stateName.CString(), state.Value());
        return *it;
    }

    states_.Push(state);
    stateNames_[state] = stateName;

    return states_.Back();
}

const String& GOS::GetStateName(const StringHash& hashstate)
{
    HashMap<StringHash, String>::ConstIterator it = stateNames_.Find(hashstate);
    return it != stateNames_.End() ? it->second_ : String::EMPTY;
}

const String& GOS::GetStateName(unsigned state)
{
    HashMap<StringHash, String>::ConstIterator it = stateNames_.Find(StringHash(state));
    return it != stateNames_.End() ? it->second_ : String::EMPTY;
}

unsigned GOS::GetStateIndex(const StringHash& state)
{
    Vector<StringHash>::ConstIterator it = states_.Find(state);
    return it != states_.End() ? it - states_.Begin() : 0;
}

const StringHash& GOS::GetStateFromIndex(unsigned index)
{
    return index < states_.Size() ? states_[index] : StringHash::ZERO;
}

void GOS::DumpAll()
{
    URHO3D_LOGINFOF("GOS() - DumpAll : states_ Size=%u", stateNames_.Size());
    unsigned index = 0;
    for (HashMap<StringHash, String>::ConstIterator it=stateNames_.Begin(); it!=stateNames_.End(); ++it)
    {
        URHO3D_LOGINFOF("     => states_[%u] = (hash:%u(dec=%d) - name:%s)", index, it->first_.Value(), it->first_.Value(), it->second_.CString());
        ++index;
    }
}

const char* ParticuleEffectTypeNames_[]
{
    "DUST",
    "TORCHE",
    "FLAME",
    "LIFEFLAME",
    "SUN",
    "GREENSPIRAL",
    "BLACKSPIRAL",
    "BULLEDAIR",
    0
};

const String ParticuleEffect_[] =
{
    String("Particules/dust.pex"),
    String("Particules/torche1.pex"),
    String("Particules/flame.pex"),
    String("Particules/lifeflame.pex"),
    String("Particules/sun.pex"),
    String("Particules/greenspiral.pex"),
    String("Particules/blackspiral.pex"),
    String("Particules/bulledair.pex"),
};
