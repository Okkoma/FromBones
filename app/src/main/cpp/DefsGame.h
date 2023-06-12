#pragma once

#include <climits>

#include <Urho3D/Core/Context.h>
#include <Urho3D/Scene/Node.h>
#include <Urho3D/Scene/Scene.h>

using namespace Urho3D;


typedef Vector<VariantMap > NodeAttributes;


enum GameStatus
{
    MENUSTATE = 0,
    PLAYSTATE_INITIALIZING = 1,
    PLAYSTATE_LOADING,
    PLAYSTATE_FINISHLOADING,
    PLAYSTATE_CLIENT_LOADING_SERVEROBJECTS,
    PLAYSTATE_CLIENT_LOADING_REPLICATEDNODES,
    PLAYSTATE_SYNCHRONIZING,
    PLAYSTATE_READY,
    PLAYSTATE_STARTGAME,
    PLAYSTATE_RUNNING,
    PLAYSTATE_ENDGAME,
    PLAYSTATE_WINGAME,

    NOGAMESTATE = -1,
    KILLCLIENTS = -2,
};

enum
{
    GAMELOG_PRELOAD = 1 << 0,
    GAMELOG_MAPPRELOAD = 1 << 1,
    GAMELOG_MAPCREATE = 1 << 2,
    GAMELOG_MAPUNLOAD = 1 << 3,
    GAMELOG_WORLDUPDATE = 1 << 4,
    GAMELOG_WORLDVISIBLE = 1 << 5,
    GAMELOG_PLAYER = 1 << 6,
};

enum GOTPoolType
{
    GOT_GOPool = 1,
    GOT_ObjectPool = 2,
};

struct GOTInfo
{
    GOTInfo();

    StringHash got_;
    String typename_;
    String filename_;
    StringHash category_;
    unsigned properties_;
    // allow replicated node CreateMode or not
    bool replicatedMode_;
    // pool quantity
    int pooltype_;
    unsigned poolqty_;
    int scaleVariation_;
    bool entityVariation_;
    bool mappingVariation_;
    // collectable infos
    int maxdropqty_;
    int defaultvalue_;
    IntVector2 entitySize_;

    static const GOTInfo EMPTY;
};

const float SWITCHSCREENTIME = 0.4f;
//const float SWITCHSCREENTIME = 1.f;

const String GAMEDATADIR = "Data/";
const String GAMESAVEDIR = "Save/";
const String DATALEVELSDIR = "Data/Levels/";
const String SAVELEVELSDIR = "Save/Levels/";
