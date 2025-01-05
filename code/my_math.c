#include <math.h>

static inline f32
absf32(f32 x)
{
  union
  {
    u32 n;
    f32 f;
  } a;
  
  a.f = x;
  a.n &= ~0x80000000;
  return(a.f);
}

static inline v2f
v2f_sub(v2f a, v2f b)
{
  v2f result =
  {
    .x = a.x - b.x,
    .y = a.y - b.y,
  };
  
  return(result);
}

static inline f32
v2f_dot(v2f a, v2f b)
{
  f32 result = (a.x*b.x + a.y*b.y);
  return(result);
}

static inline v2f
v2f_add(v2f a, v2f b)
{
  v2f result =
  {
    a.x + b.x,
    a.y + b.y,
  };
  
  return(result);
}

static inline v2f
v2f_scale(f32 a, v2f v)
{
  v2f result =
  {
    a * v.x,
    a * v.y,
  };
  
  return(result);
}

static inline u32
v2f_smallest_abs_comp_idx(v2f a)
{
  a.x = absf32(a.x);
  a.y = absf32(a.y);
  if ((a.x <= a.y))
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

static inline void
v3f_add_eq(v3f *a, v3f b)
{
  a->x += b.x;
  a->y += b.y;
  a->z += b.z;
}

static m44
m44_make_ortho_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane)
{
  f32 rml       = right - left;
  f32 tmb       = top - bottom;
  
  m44 result =
  {
    2.0f / rml,        0.0f,      0.0f,       0.0f,
    0.0f,        2.0f / tmb,      0.0f,       0.0f,
    0.0f,              0.0f,      1.0f / (far_plane - near_plane),       0.0f,
    -(right + left) / rml, -(top + bottom) / tmb,  -near_plane / (far_plane - near_plane), 1.0f,
  };
  
  return (result);
}