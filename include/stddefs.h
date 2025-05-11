#ifndef __STDDEFS_H__
#define __STDDEFS_H__

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

#ifndef _INT64_T
typedef long int64_t;
#endif

#ifndef _UINT64_T
typedef unsigned long uint64_t;
#endif

#ifndef _SIZE_T
typedef uint64_t size_t;
#endif

#endif