static m44
m44_make_ortho_z01(f32 left, f32 right, f32 top, f32 bottom, f32 near_plane, f32 far_plane)
{
  f32 rml       = right - left;
  f32 tmb       = top - bottom;
  
  m44 result =
  {
    2.0f / rml,        0.0f,      0.0f,       0.0f,
    0.0f,        2.0f / tmb,      0.0f,       0.0f,
    0.0f,              0.0f,      1.0f,       0.0f,
    -(right + left) / rml, -(top + bottom) / tmb,  -near_plane / (far_plane - near_plane), 1.0f,
  };
  
  return (result);
}