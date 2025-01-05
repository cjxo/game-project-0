#if !defined (BASE_H)
#define BASE_H

#include <stdint.h>
#include <float.h>

typedef int8_t    s8;
typedef uint8_t   u8;
typedef int16_t  s16;
typedef uint16_t u16;
typedef int32_t  s32;
typedef uint32_t u32;
typedef int64_t  s64;
typedef uint64_t u64;
typedef float   f32;
typedef s32 b32;

#define AssertBreak() __debugbreak()
#define Assert(cond) do{if(!(cond)){AssertBreak();}}while(0)
#define AssertFalse(cond) Assert((cond)==false)
//#define AssertHR(hr) Assert(SUCCEEDED(hr))
#define InvalidCodePath() AssertBreak()
#define ArrayCount(a) (sizeof(a)/sizeof((a)[0]))
#define true 1
#define false 0

#define Clamp(x,s,e) ((x)<(s)?(s):(x)>(e)?(e):(x))
#define Minimum(a,b) ((a)<(b))?(a):(b)
#define Maximum(a,b) ((a)>(b))?(a):(b)
#define AlignAToB(a,b) (((a)+((b)-1))&(~((b)-1)))
#define KB(v) (1024llu*((u64)v))
#define MB(v) (1024llu*KB(v))
#define GB(v) (1024llu*MB(v))

typedef struct
{
  u64 state;
} PRNG_PCG32;

static inline void  pcg32_seed(PRNG_PCG32 *pcg, u64 seed);
static inline u32   pcg32_u32(PRNG_PCG32 *pcg);
static inline u32   pcg32_range_u32(PRNG_PCG32 *pcg, u32 low, u32 high);
static inline f32   pcg32_f32(PRNG_PCG32 *pcg); // [0, 1)

#endif