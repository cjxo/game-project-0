#if !defined (MY_MATH_H)
#define MY_MATH_H

static inline f32 absf32(f32 x);

typedef union
{
  struct
  {
    f32 x, y;
  };
  f32 v[2];
} v2f;

static inline u32 v2f_smallest_abs_comp_idx(v2f a);

typedef union
{
  struct
  {
    f32 x, y, z;
  };
  
  struct
  {
    v2f xy;
    f32 __unused_a;
  };
  
  struct
  {
    f32 __unused_b;
    v2f yz;
  };
  
  f32 v[3];
} v3f;

static inline void  v3f_add_eq(v3f *a, v3f b);

typedef union
{
  struct
  {
    f32 x, y, z, w;
  };
  f32 v[4];
} v4f;

typedef union
{
  v4f r[4];
  f32 m[4][4];
} m44;

static m44 m44_make_ortho_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane);

typedef struct
{
  v3f p;
  v3f dims;
} AABB;

#endif