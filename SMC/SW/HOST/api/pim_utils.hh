#ifndef _PIM_UTILS_H_
#define _PIM_UTILS_H_

// Headers
#include <stdio.h>
#include <stdint.h>
#include "_params.hh"

#define ulong_t unsigned long       // 32b on ARMv7 and 64b on ARMv8

/*****************************************************/
// Messages and Debugging facilities
#define API_ALERT(fmt,args...)		{printf("[PIM.API]: " fmt "\n", ## args);}
#ifdef DEBUG_API
#define API_INFO(fmt,args...)		{printf("[PIM.API]: " fmt "\n", ## args);}
#define ASSERT_DBG(cond)            {if (!(cond)) {printf("[PIM.API] Assertion failed in %s LINE: %d", __FILE__, __LINE__);exit(1);}}
#else
#define API_INFO(fmt,args...)		// Do nothing
#define ASSERT_DBG(cond)            // Do nothing
#endif

// Hard Assertion
// #define ASSERT(cond)				{if (!(cond)) {printf("[PIM.API] Assertion failed in %s LINE: %d", __FILE__, __LINE__);exit(1);}}

#endif // _PIM_UTILS_H_
