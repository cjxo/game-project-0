static void
game_update_and_render(Game_State *game, Game_Input *input, f32 game_update_secs)
{
  game->game_quad_instances[game->game_quad_instance_count++] = (Game_QuadInstance)
  {
    .p       = { 0.0f, 0.0f, 0.0f },
    .dims    = { 50.0f, 50.0f, 50.0f },
    .colour  = { 1.0f, 0.0f, 0.0f, 1.0f }
  };
}