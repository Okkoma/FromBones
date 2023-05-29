#ifndef ANLVM_H
#define ANLVM_H

// Define ANLVM_IMPLEMENTATION in exactly 1 source (.cpp) file before including this header.

#include "instruction.h"
#include "kernel.h"
#include "vm.h"

#ifdef ANLVM_IMPLEMENTATION
#include "kernel.inl"
#include "vm.inl"
#endif

#endif
