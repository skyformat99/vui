#pragma once

#define VUI_DEBUG

#ifdef VUI_DEBUG

#include <stdio.h>
#include <stdlib.h>

#define VUI_DEBUG_TEST
#define VUI_DEBUG_VUI
#define VUI_DEBUG_STATEMACHINE
//#define VUI_DEBUG_GC
//#define VUI_DEBUG_STACK
//#define VUI_DEBUG_UTF8

extern void vui_debug(char* msg);  // user must declare this

#endif