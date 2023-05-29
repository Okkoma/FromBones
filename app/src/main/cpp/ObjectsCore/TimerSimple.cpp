#include <Urho3D/Urho3D.h>

#include "TimerSimple.h"



TimerSimple::TimerSimple(unsigned expirationTime, bool startActive) :
    expirationTime_(expirationTime)
{
    timer_.Reset();
}

TimerSimple::~TimerSimple()
{
}

bool TimerSimple::Expired()
{
    return (timer_.GetMSec(false) > expirationTime_);
}

bool TimerSimple::Active()
{
    return (expirationTime_ > timer_.GetMSec(false));
}

void TimerSimple::Reset()
{
    timer_.Reset();
}

void TimerSimple::SetExpirationTime(unsigned expirationTime)
{
    expirationTime_ = expirationTime;
    timer_.Reset();
}


unsigned TimerSimple::GetCurrentTime()
{
    return timer_.GetMSec(false);
}





