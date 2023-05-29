#ifndef wren_modules_h
#define wren_modules_h

#include "../wren.h"


// The maximum number of foreign methods a single class defines. Ideally, we
// would use variable-length arrays for each class in the table below, but
// C++98 doesn't have any easy syntax for nested global static data, so we
// just use worst-case fixed-size arrays instead.
//
// If you add a new method to the longest class below, make sure to bump this.
// Note that it also includes an extra slot for the sentinel value indicating
// the end of the list.
#define MAX_METHODS_PER_CLASS 7

// The maximum number of foreign classes a single built-in module defines.
//
// If you add a new class to the largest module below, make sure to bump this.
// Note that it also includes an extra slot for the sentinel value indicating
// the end of the list.
#define MAX_CLASSES_PER_MODULE 3

// Describes one foreign method in a class.
typedef struct
{
    bool isStatic;
    const char* signature;
    WrenForeignMethodFn method;
} MethodRegistry;

// Describes one class in a built-in module.
typedef struct
{
    const char* name;

    MethodRegistry methods[MAX_METHODS_PER_CLASS];
} ClassRegistry;

// Describes one built-in module.
typedef struct
{
    // The name of the module.
    const char* name;

    // Pointer to the string containing the source code of the module. We use a
    // pointer here because the string variable itself is not a constant
    // expression so can't be used in the initializer below.
    const char **source;

    ClassRegistry classes[MAX_CLASSES_PER_MODULE];
} ModuleRegistry;

// To locate foreign classes and modules, we build a big directory for them in
// static data. The nested collection initializer syntax gets pretty noisy, so
// define a couple of macros to make it easier.
#define WREN_SENTINEL_METHOD { false, NULL, NULL }
#define WREN_SENTINEL_CLASS { NULL, { WREN_SENTINEL_METHOD } }
#define WREN_SENTINEL_MODULE {NULL, NULL, { WREN_SENTINEL_CLASS } }

#define WREN_MODULE(name) { #name, &name##ModuleSource, {
#define WREN_END_MODULE WREN_SENTINEL_CLASS } },

#define WREN_CLASS(name) { #name, {
#define WREN_END_CLASS WREN_SENTINEL_METHOD } },

#define WREN_METHOD(signature, fn) { false, signature, fn },
#define WREN_STATIC_METHOD(signature, fn) { true, signature, fn },
#define WREN_FINALIZER(fn) { true, "<finalize>", (WrenForeignMethodFn)fn },


#endif
