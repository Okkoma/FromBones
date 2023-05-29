#include <cstdio>
#include <Urho3D/Urho3D.h>

#include "ShortIntVector2.h"


const ShortIntVector2 ShortIntVector2::ZERO;

String ShortIntVector2::ToString() const
{
    char tempBuffer[CONVERSION_BUFFER_LENGTH];
    sprintf(tempBuffer, "%d %d", x_, y_);
    return String(tempBuffer);
}

unsigned ShortIntVector2::ToHash() const
{
//    return (unsigned) (x_ << 16) | y_;
    return (unsigned) (y_ | 0xffff0000) & (x_ << 16 | 0xffff);
}
