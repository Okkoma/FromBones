#pragma once

#include "GameOptions.h"

#define GENWORLD_NUMTHREADS 2

#if defined(ACTIVE_WORLD2D_THREADING)

// MODULETHREAD_VERSION=1 uses std::thread or pthread
// MODULETHREAD_VERSION=2 uses GameContext::Get().gameWorkQueue_

#define MODULETHREAD_VERSION 2

#if MODULETHREAD_VERSION == 1

#if (defined(_WIN32) && defined(_MSC_VER)) || defined(CPP2011)
#define USE_STDTHREAD
#undef USE_PTHREAD
#define IMAGING_USE_STDTHREAD
#undef IMAGING_USE_PTHREAD
#else
#define USE_PTHREAD
#undef USE_STDTHREAD
#define IMAGING_USE_PTHREAD
#undef IMAGING_USE_STDTHREAD
#endif

#if defined(USE_STDTHREAD)
#include <thread>
#elif defined(USE_PTHREAD)
#include <pthread.h>
#endif

#else

#include <Urho3D/Core/WorkQueue.h>
#include "GameContext.h"

#define IMAGING_USE_WORKQUEUE

#endif


#endif

