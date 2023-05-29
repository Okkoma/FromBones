#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../wren.h"
#include "wren_vm.h"

#include "WrenBinder.h"



// Looks for a built-in module with [name].
//
// Returns the BuildInModule for it or NULL if not found.
static ModuleRegistry* findModule(const char* name)
{
    for (int i = 0; wrenModules[i].name != NULL; i++)
    {
        if (strcmp(name, wrenModules[i].name) == 0) return &wrenModules[i];
    }

    return NULL;
}

// Looks for a class with [name] in [module].
static ClassRegistry* findClass(ModuleRegistry* module, const char* name)
{
    for (int i = 0; module->classes[i].name != NULL; i++)
    {
        if (strcmp(name, module->classes[i].name) == 0) return &module->classes[i];
    }

    return NULL;
}

// Looks for a method with [signature] in [clas].
static WrenForeignMethodFn findMethod(ClassRegistry* clas,
                                      bool isStatic, const char* signature)
{
    for (int i = 0; clas->methods[i].signature != NULL; i++)
    {
        MethodRegistry* method = &clas->methods[i];
        if (isStatic == method->isStatic &&
                strcmp(signature, method->signature) == 0)
        {
            return method->method;
        }
    }

    return NULL;
}

char* readBuiltInModule(const char* name)
{
    ModuleRegistry* module = findModule(name);

    if (module == NULL)
    {
        printf("readBuiltInModule can't findmodule \"%s\".\n", name);
        return NULL;
    }

    printf("readBuiltInModule \"%s\" ok !\n", name);

    size_t length = strlen(*module->source);
    char* copy = (char*)malloc(length + 1);
    strncpy(copy, *module->source, length + 1);

    return copy;
}

WrenForeignMethodFn bindBuiltInForeignMethod(
    WrenVM* vm, const char* moduleName, const char* className, bool isStatic,
    const char* signature)
{
    // TODO: Assert instead of return NULL?
    ModuleRegistry* module = findModule(moduleName);
    if (module == NULL) return NULL;

    ClassRegistry* clas = findClass(module, className);
    if (clas == NULL) return NULL;

    return findMethod(clas, isStatic, signature);
}

WrenForeignClassMethods bindBuiltInForeignClass(
    WrenVM* vm, const char* moduleName, const char* className)
{
    WrenForeignClassMethods methods = { NULL, NULL };

    ModuleRegistry* module = findModule(moduleName);
    if (module == NULL) return methods;

    ClassRegistry* clas = findClass(module, className);
    if (clas == NULL) return methods;

    methods.allocate = findMethod(clas, true, "<allocate>");
    methods.finalize = (WrenFinalizerFn)findMethod(clas, true, "<finalize>");

    return methods;
}


static char const* rootDirectory = NULL;

// The exit code to use unless some other error overrides it.
int defaultExitCode = 0;

// Reads the contents of the file at [path] and returns it as a heap allocated
// string.
//
// Returns `NULL` if the path could not be found. Exits if it was found but
// could not be read.
static char* readFile(const char* path)
{
    FILE* file = fopen(path, "rb");
    if (file == NULL) return NULL;

    // Find out how big the file is.
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    // Allocate a buffer for it.
    char* buffer = (char*)malloc(fileSize + 1);
    if (buffer == NULL)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    // Read the entire file.
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    if (bytesRead < fileSize)
    {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    // Terminate the string.
    buffer[bytesRead] = '\0';

    fclose(file);
    return buffer;
}

// Converts the module [name] to a file path.
static char* wrenFilePath(const char* name)
{
    // The module path is relative to the root directory and with ".wren".
    size_t rootLength = rootDirectory == NULL ? 0 : strlen(rootDirectory);
    size_t nameLength = strlen(name);
    size_t pathLength = rootLength + nameLength + 5;
    char* path = (char*)malloc(pathLength + 1);

    if (rootDirectory != NULL)
    {
        memcpy(path, rootDirectory, rootLength);
    }

    memcpy(path + rootLength, name, nameLength);
    memcpy(path + rootLength + nameLength, ".wren", 5);
    path[pathLength] = '\0';

    return path;
}

// Attempts to read the source for [module] relative to the current root
// directory.
//
// Returns it if found, or NULL if the module could not be found. Exits if the
// module was found but could not be read.
static char* readModule(WrenVM* vm, const char* module)
{
    char* source = readBuiltInModule(module);
    if (source != NULL) return source;

    printf("readModule \"%s\".\n", module);
    // First try to load the module with a ".wren" extension.
    char* modulePath = wrenFilePath(module);
    char* moduleContents = readFile(modulePath);
    free(modulePath);

    if (moduleContents != NULL) return moduleContents;

    // If no contents could be loaded treat the module name as specifying a
    // directory and try to load the "module.wren" file in the directory.
    size_t moduleLength = strlen(module);
    size_t moduleDirLength = moduleLength + 7;
    char* moduleDir = (char*)malloc(moduleDirLength + 1);
    memcpy(moduleDir, module, moduleLength);
    memcpy(moduleDir + moduleLength, "/module", 7);
    moduleDir[moduleDirLength] = '\0';

    char* moduleDirPath = wrenFilePath(moduleDir);
    free(moduleDir);

    moduleContents = readFile(moduleDirPath);
    free(moduleDirPath);

    return moduleContents;
}

// Binds foreign methods declared in either built in modules
static WrenForeignMethodFn bindForeignMethod(WrenVM* vm, const char* module,
        const char* className, bool isStatic, const char* signature)
{
    WrenForeignMethodFn method = bindBuiltInForeignMethod(vm, module, className,
                                 isStatic, signature);
    return method;
}

// Binds foreign classes declared in either built in modules
static WrenForeignClassMethods bindForeignClass(
    WrenVM* vm, const char* module, const char* className)
{
    WrenForeignClassMethods methods = bindBuiltInForeignClass(vm, module,
                                      className);
    return methods;
}

static void write(WrenVM* vm, const char* text)
{
    printf("%s", text);
}

static void reportError(WrenVM* vm, WrenErrorType type,
                        const char* module, int line, const char* message)
{
    switch (type)
    {
    case WREN_ERROR_COMPILE:
        fprintf(stderr, "[%s line %d] %s\n", module, line, message);
        break;

    case WREN_ERROR_RUNTIME:
        fprintf(stderr, "%s\n", message);
        break;

    case WREN_ERROR_STACK_TRACE:
        fprintf(stderr, "[%s line %d] in %s\n", module, line, message);
        break;
    }
}

WrenVM* wrenInitializeVM()
{
    printf("wrenInitializeVM ...\n");

    WrenConfiguration config;
    wrenInitConfiguration(&config);

    config.bindForeignMethodFn = bindForeignMethod;
    config.bindForeignClassFn = bindForeignClass;
    config.loadModuleFn = readModule;
    config.writeFn = write;
    config.errorFn = reportError;

    WrenVM* vm = wrenNewVM(&config);
    /*
      // Import BuiltIn Modules
      for (int i = 0; modules[i].name != NULL; i++)
      {
        if (wrenImportModuleWithName(vm, modules[i].name))
            printf(" ... import module \"%s\" ... OK !\n", modules[i].name);
        else
            printf(" ... import module \"%s\" ... NOK !\n", modules[i].name);
      }
    */
    printf("wrenInitializeVM ... ok!\n");

    return vm;
}
