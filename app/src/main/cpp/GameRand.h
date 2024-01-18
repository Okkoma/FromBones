#pragma once

#include "GameOptions.h"

#if defined(CPP2011)
#include <random>
#endif

using namespace Urho3D;

const int NUM_GAMERANDRNG = 5;
enum GameRandRng
{
    ALLRAND = 0,
    MAPRAND,
    TILRAND,
    OBJRAND,
    WEATHERRAND,
};

class GameRand
{
public:
#if defined(CPP2011)
    typedef std::mt19937 RngT;
#else
    typedef unsigned RngT;
#endif

    static void InitTable();
    static GameRand& GetRandomizer(GameRandRng r)
    {
        return registeredRandomizer_[r];
    }
    static int GetRand(GameRandRng r)
    {
        return registeredRandomizer_[r].Get();
    }
    static int GetRand(GameRandRng r, int range)
    {
        return registeredRandomizer_[r].Get(range);
    }
    static int GetRand(GameRandRng r, int min, int max)
    {
        return registeredRandomizer_[r].Get(min, max);
    }
    static void SetSeedRand(GameRandRng r, unsigned seed);
    static unsigned GetSeedRand(GameRandRng r)
    {
        return registeredRandomizer_[r].GetSeed();
    }
    static unsigned GetTimeSeed();
    static void Dump10Value();

    GameRand() : seed_(1) { }

    void SetSeed(unsigned seed)
    {
        seed_ = seed;
        rng_ = RngT(seed);
    }
    unsigned GetSeed() const
    {
        return seed_ ;
    }

#if defined(CPP2011)
    inline int Get()
    {
        return std::uniform_int_distribution<int>()(rng_);
    }
    inline int Get(int range)
    {
        return std::uniform_int_distribution<int>(0, range-1)(rng_);
    }
    inline int Get(int min, int max)
    {
        return std::uniform_int_distribution<int>(min, max)(rng_);
    }
    inline int GetDir()
    {
        return std::uniform_int_distribution<int>(0, 3)(rng_);
    }
    inline int GetAllDir()
    {
        return std::uniform_int_distribution<int>(0, 6)(rng_);
    }
#else
    inline int Get()
    {
        seed_ = seed_ * 214013 + 2531011;
        return (seed_ >> 16) & 32767;
    }
    inline int Get(int range)
    {
        return range > 1 ? Get() % range : 0;
    }
    inline int Get(int min, int max)
    {
        return Get(max-min+1) + min;
    }
    inline int GetDir()
    {
        return Get(4);
    }
    inline int GetAllDir()
    {
        return Get(7);
    }
    inline int GetPlusOrMinus()
    {
         return  Get(2) ? 1 : -1;
    }
#endif

private:
    unsigned seed_;
    RngT rng_;

    static Vector<GameRand> registeredRandomizer_;
};
