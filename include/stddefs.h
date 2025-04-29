#ifndef __STDDEFS_H__
#define __STDDEFS_H__

/* Setup boolean logic */
#define false 0
#define true 1
typedef char bool;

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#ifndef _UINT64_T
typedef unsigned long uint64_t;
#define _UINT64_T
#endif
#ifndef _SIZE_T
typedef uint64_t size_t;
#define _SIZE_T
#endif

#endif