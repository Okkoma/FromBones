#pragma once

#ifdef FROMBONES_EXPORTS
#  if defined(__WIN32__) || defined(__WINRT__)
#    define FROMBONES_API __declspec(dllexport)
#  elif defined(__GNUC__) && __GNUC__ >= 4
#    define FROMBONES_API __attribute__ ((visibility("default")))
#  endif
#else
#   define FROMBONES_API
#endif

#include "ShortIntVector2.h"
#include "MemoryObjects.h"
#include "Slot.h"

typedef unsigned char FeatureType;
