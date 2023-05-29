#include <Urho3D/Urho3D.h>

#include <Urho3D/IO/Log.h>

#include "GameOptions.h"

#include <ctime>

#if defined(CPP2011)
#include <chrono>
#endif

#include "GameRand.h"

#include "AccidentalNoise/VCommon/random_gen.h"

Vector<GameRand> GameRand::registeredRandomizer_;


void GameRand::InitTable()
{
    URHO3D_LOGINFOF("GameRand() - InitTable : %u Randomizers", NUM_GAMERANDRNG);

    for (unsigned i=0; i < NUM_GAMERANDRNG; ++i)
        registeredRandomizer_.Push(GameRand());
}

unsigned GameRand::GetTimeSeed()
{
    unsigned seed;
#if defined(CPP2011)
    seed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    URHO3D_LOGINFOF("GameRand() - SetSeed : ... set time random seed (C++11) = %u", seed);
#else
    seed = (unsigned)time(NULL);
    URHO3D_LOGINFOF("GameRand() - SetSeed : ... set time random seed = %u", seed);
#endif

    return seed;
}

void GameRand::Dump10Value()
{
    unsigned seed = GetSeedRand(MAPRAND);
    String numbers;
    for (unsigned i=0; i < 10; ++i)
        numbers += String(GetRand(MAPRAND, 100)) + " ";
    URHO3D_LOGERRORF("MAPRAND(seed=%u) => (%s)", seed, numbers.CString());

    anl::KISS kiss;
    kiss.setSeed(seed);
    numbers = "";
    for (unsigned i=0; i < 10; ++i)
        numbers += String(kiss.get01()) + " ";
    URHO3D_LOGERRORF("KISS(seed=%u) => (%s)", seed, numbers.CString());
}

void GameRand::SetSeedRand(GameRandRng r, unsigned seed)
{
    if (r == ALLRAND)
    {
#if !defined(CPP2011)
        srand(seed);
#endif
        for (unsigned i=1; i < NUM_GAMERANDRNG; ++i)
            registeredRandomizer_[i].SetSeed(seed);
    }
    else
        registeredRandomizer_[r].SetSeed(seed);
}

