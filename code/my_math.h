#if !defined (MY_MATH_H)
#define MY_MATH_H

typedef union
{
  struct
  {
    f32 x, y, z;
  };
  f32 v[3];
} v3f;

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

#endif