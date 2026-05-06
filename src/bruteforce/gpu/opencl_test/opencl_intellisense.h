// Stub definitions to suppress IntelliSense errors in .cl files.
// Not included in any real build — forced-include for VS only.
#pragma once

// Qualifiers
#define __kernel
#define __global
#define __local
#define __constant
#define __private
#define __read_only
#define __write_only
#define __read_write

// Scalar types
typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

// Vector types
struct uchar2  { unsigned char  s0, s1; };
struct uchar4  { unsigned char  s0, s1, s2, s3; };
struct ushort2 { unsigned short s0, s1; };
struct ushort4 { unsigned short s0, s1, s2, s3; };
struct uint2   { unsigned int   s0, s1; };
struct uint4   { unsigned int   s0, s1, s2, s3; };
struct int2    { int s0, s1; };
struct int4    { int s0, s1, s2, s3; };
struct float2  { float s0, s1; };
struct float4  { float s0, s1, s2, s3; };
typedef uchar4  uchar4;
typedef ushort2 ushort2;
typedef uint2   uint2;
typedef uint4   uint4;

// Memory fence flags
#define CLK_LOCAL_MEM_FENCE  0
#define CLK_GLOBAL_MEM_FENCE 1

// Built-in functions
inline size_t get_global_id(unsigned int)  { return 0; }
inline size_t get_local_id(unsigned int)   { return 0; }
inline size_t get_group_id(unsigned int)   { return 0; }
inline size_t get_global_size(unsigned int){ return 0; }
inline size_t get_local_size(unsigned int) { return 0; }
inline void   barrier(int) {}

// Attributes
#define __attribute__(x)
