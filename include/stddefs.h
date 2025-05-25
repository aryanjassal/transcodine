#ifndef __STDDEFS_H__
#define __STDDEFS_H__

/* For size_t */
#include <stdio.h>

/* Boolean logic */
#define false 0
#define true 1
typedef char bool;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

/**
 * MacOS automatically includes these types with a different signature (it uses
 * unsigned long long), so we need a guard to prevent redefinition in case it
 * already exists. Other types align with standard C behaviour so we don't need
 * to guard them.
 */

#if defined(__APPLE__)

typedef long long int64_t;
typedef unsigned long long uint64_t;

#else

typedef long int64_t;
typedef unsigned long uint64_t;

#endif

#endif
