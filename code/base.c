#define PCG_DefaultMultiplier64 6364136223846793005ULL
#define PCG_DefaultIncrement64  1442695040888963407ULL

static inline void
pcg32_seed(PRNG_PCG32 *pcg, u64 seed)
{
  pcg->state    = 0;
  pcg32_u32(pcg);
  pcg->state   += seed;
  pcg32_u32(pcg);
}

static inline u32
pcg32_u32(PRNG_PCG32 *pcg)
{
  u64 state      = pcg->state;
  pcg->state     = state * PCG_DefaultMultiplier64 + PCG_DefaultIncrement64;
  
  u32 value      = (u32)((state ^ (state >> 18)) >> 27);
  s32 rot        = state >> 59;
  return rot ? (value >> rot) | (value << (32 - rot)) : value;
}

static inline u32
pcg32_range_u32(PRNG_PCG32 *pcg, u32 low, u32 high)
{
  u32 bound     = high - low;
  u32 threshold = (u32)(-(s32)bound % bound);
  
  for (;;)
  {
    u32 r = pcg32_u32(pcg);
    if (r >= threshold)
    {
      return low + (r % bound);
    }
  }
}

// [0, 1)
static inline f32
pcg32_f32(PRNG_PCG32 *pcg)
{
  u32 x = pcg32_u32(pcg);
  return (f32)(s32)(x >> 8) * 0x1.0p-24f;
}