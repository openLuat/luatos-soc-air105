
//如果已经包含cs_types.h 则不再重复包含这个文件
#ifdef __CS_TYPE_H__
#define __TYPE_H__
#endif

#ifndef __TYPE_H__
#define __TYPE_H__

#ifndef NULL
#define NULL    ((void *)0)
#endif

#ifndef FALSE
#define FALSE   (0)
#endif

#ifndef TRUE
#define TRUE    (1)
#endif

#ifndef NULL
#define NULL (0)
#endif

typedef unsigned char   BYTE;
typedef unsigned short  WORD;
typedef unsigned int    DWORD;
typedef unsigned char   BOOL;

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short u16;
typedef signed short s16;
typedef unsigned int u32;
typedef signed int s32;
typedef unsigned long long u64;
typedef long long s64;

#ifndef offsetof
#define offsetof(TYPE, MEMBER)          ((u32) &((TYPE *)0)->MEMBER)
#endif


#endif
