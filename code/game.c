static v3f g_structures_dims[] =
{
  // Game_StructureType_Grass_Flat
  { Game_BlockDimPixels, Game_BlockDimPixels, Game_BlockDimPixels },
  
  // Game_StructureType_Grass_Blade
  { Game_BlockDimPixels, Game_BlockDimPixels, Game_BlockDimPixels },
  
  // Game_StructureType_Tree_Fir
  { Game_BlockDimPixels, Game_BlockDimPixels * 3, Game_BlockDimPixels },
  
  // Game_StructureType_Tree_Rock
  { Game_BlockDimPixels, Game_BlockDimPixels, Game_BlockDimPixels },
};

// assume that the local position is the zero vector
static void
get_structure_aabb(Game_StructureType type, AABB **aabbs, u64 *count)
{
  static AABB default_aabb[] =
  {
    {
      .p    = { 0.0f, 0.0f },
      .dims = { 1.0f, 1.0f, 0.0f },
    },
  };
  
  static AABB fir_aabb[] =
  {
    {
      .p    = { 0.0f, 0.0f },
      .dims = { 1.0f, 1.0f, 0.0f },
    },
    
    {
      .p    = { 0.25f, 1.0f },
      .dims = { 0.5f, 0.75f, 0.0f },
    },
  };
  
  switch (type)
  {
    case Game_StructureType_Tree_Fir:
    {
      *aabbs = fir_aabb;
      *count = ArrayCount(fir_aabb);
    } break;
    
    default:
    {
      *aabbs = default_aabb;
      *count = ArrayCount(default_aabb);
    } break;
  }
}

static Game_Structure
create_structure(Game_StructureType type)
{
  Game_Structure result;
  result.type = type;
  switch (type)
  {
    case Game_StructureType_Grass_Blade:
    {
      result.flags = Game_InteractFlag_Interactable;
    } break;
    
    case Game_StructureType_Tree_Fir:
    case Game_StructureType_Rock:
    {
      result.flags = (Game_InteractFlag_Interactable | Game_InteractFlag_Collidable);
    } break;
    
    default:
    {
      result.flags = 0;
    } break;
  }
  
  return(result);
}

static void
game_init(Game_State *state)
{
  pcg32_seed(&state->pcg32, 48);

  Game_Structure grass_flat    = create_structure(Game_StructureType_Grass_Flat);
  Game_Structure grass_blade   = create_structure(Game_StructureType_Grass_Blade);
  Game_Structure tree_fir      = create_structure(Game_StructureType_Tree_Fir);
  Game_Structure rock          = create_structure(Game_StructureType_Rock);
  Game_Structure none          = create_structure(Game_StructureType_Count);
  for (u32 chunk_y = 0; chunk_y < Game_ChunkDim; ++chunk_y)
  {
    for (u32 chunk_x = 0; chunk_x < Game_ChunkDim; ++chunk_x)
    {
      u32 idx = chunk_x + chunk_y * Game_ChunkDim;
      state->structure_chunk[Game_ChunkLayer_NonInteractable][idx] = grass_flat;
    }
  }
  
  for (u32 chunk_y = 0; chunk_y < Game_ChunkDim; ++chunk_y)
  {
    for (u32 chunk_x = 0; chunk_x < Game_ChunkDim; ++chunk_x)
    {
      f32 weight = pcg32_f32(&state->pcg32);
      u32 idx = chunk_x + chunk_y * Game_ChunkDim;
      
      if (weight < 0.5f)
      {
        state->structure_chunk[Game_ChunkLayer_Interactable][idx] = grass_blade;
      }
      else if (weight < 0.6f)
      {
        state->structure_chunk[Game_ChunkLayer_Interactable][idx] = rock;
      }
      else if (weight < 0.65f)
      {
        state->structure_chunk[Game_ChunkLayer_Interactable][idx] = tree_fir;
      }
      else
      {
        state->structure_chunk[Game_ChunkLayer_Interactable][idx] = none;
      }
    }
  }
  
  state->player_p    = (v3f) { 50, 0, 1.0f };
  state->player_dims  = (v3f) { 32, 32, 32 };
}

static inline Game_QuadInstance *
add_quad(Game_QuadInstances *instances, v3f p, v3f dims, v4f colour)
{
  AssertTrue((instances->count + 1) < Game_MaxQuadInstances);
  Game_QuadInstance *result = instances->instances + instances->count++;
  result->p          = p;
  result->dims       = dims;
  result->colour     = colour;
  result->tex_on_me  = 0;
  return(result);
}

static inline Game_QuadInstance *
add_quad_tex_clipped(Game_QuadInstances *instances, v3f p, v3f dims, v2f atlas_p, v2f atlas_dims)
{
  Game_QuadInstance *result    = add_quad(instances, p, dims, (v4f){ 1.0f, 1.0f, 1.0f, 1.0f });
  result->atlas_p              = atlas_p;
  result->atlas_dims           = atlas_dims;
  result->tex_on_me            = 1;
  return(result);
}

static void
draw_the_game(Game_State *game)
{
  v2f camera_p                       = game->player_p.xy;
  Game_RenderState *render_state     = game->render_state;
  for (Game_ChunkLayer layer = Game_ChunkLayer_NonInteractable;
       layer < Game_ChunkLayer_Count;
       ++layer)
  {
    for (u32 chunk_y = 0; chunk_y < Game_ChunkDim; ++chunk_y)
    {
      for (u32 chunk_x = 0; chunk_x < Game_ChunkDim; ++chunk_x)
      {
        u32 idx                  = chunk_x + chunk_y * Game_ChunkDim;
        Game_Structure structure = game->structure_chunk[layer][idx];
        
        v3f p       = { (f32)chunk_x * Game_BlockDimPixels - camera_p.x, (f32)chunk_y * Game_BlockDimPixels - camera_p.y, Game_ChunkLayer_Count - (f32)layer };
        if (structure.type != Game_StructureType_Count)
        //if (structure.type == Game_StructureType_Tree_Fir)
        {
          v4f structure_colour = {1.0f, 1.0f, 1.0f, 1.0f};
          v2f atlas_p          = {0};
          v2f atlas_dims       = {0};
          b32 tex              = false;
          switch (structure.type)
          {
            case Game_StructureType_Grass_Flat:
            {
              atlas_p    = (v2f) { 0.0f, 0.0f };
              atlas_dims = (v2f) { 32.0f, 32.0f };
              tex        = true;
            } break;
            
            case Game_StructureType_Grass_Blade:
            {
              structure_colour = (v4f) { 74.0f / 255.0f, 206.0f / 255.0f, 71.0f / 255.0f, 1.0f };
            } break;
            
            case Game_StructureType_Tree_Fir:
            {
              structure_colour = (v4f) { 109.0f / 255.0f, 74.0f / 255.0f, 23.0f / 255.0f, 1.0f };
              p.z -= 1.0f;
            } break;
            
            case Game_StructureType_Rock:
            {
              structure_colour = (v4f) { 68.0f / 255.0f, 75.0f / 255.0f, 77.0f / 255.0f, 1.0f };
            } break;
            
            default:
            {
              InvalidCodePath();
            } break;
          }
          
          if (tex)
          {
            add_quad_tex_clipped(&render_state->filled_quads, p, g_structures_dims[structure.type],
                                 atlas_p, atlas_dims);
          }
          else
          {
            add_quad(&render_state->filled_quads, p, g_structures_dims[structure.type], structure_colour);
          }
          
          f32 z_value = Game_ChunkLayer_Count - (f32)layer + 10;
          if ((structure.type != Game_StructureType_Count) && (structure.flags & Game_InteractFlag_Collidable))
          //if (structure.type == Game_StructureType_Tree_Fir)
          {
            v3f start_p           = { (f32)chunk_x * Game_BlockDimPixels - camera_p.x, (f32)chunk_y * Game_BlockDimPixels - camera_p.y, Game_ChunkLayer_Count - (f32)layer };
            structure_colour      = (v4f){1,1,1,1};
            AABB *aabbs;
            u64   aabbs_count;
            get_structure_aabb(structure.type, &aabbs, &aabbs_count);
            for (u64 aabb_idx = 0; aabb_idx < aabbs_count; ++aabb_idx)
            {
              AABB aabb     = aabbs[aabb_idx];
              p             = (v3f) { start_p.x + aabb.p.x * Game_BlockDimPixels, start_p.y + aabb.p.y * Game_BlockDimPixels, z_value };
              v3f dims      = { aabb.dims.x * Game_BlockDimPixels, aabb.dims.y * Game_BlockDimPixels, Game_BlockDimPixels };
              add_quad(&render_state->wire_quads,
                       (v3f) { p.x, p.y, p.z },
                       dims,
                       structure_colour);
            }
          }
        }
      }
    }
  }

  v3f player_p = { game->player_p.x - camera_p.x, game->player_p.y - camera_p.y, game->player_p.z };
  add_quad(&render_state->filled_quads, player_p, game->player_dims, (v4f) { 1.0f, 1.0f, 1.0f, 1.0f });
  add_quad(&render_state->wire_quads, player_p, game->player_dims, (v4f) { 1.0f, 1.0f, 1.0f, 1.0f });
}

static Memory_Arena *
arena_reserve(Platform_Functions functions, u64 size)
{
  AssertTrue(size > sizeof(Memory_Arena));
  
  size                   = AlignAToB(size, 16);
  Memory_Arena *result   = 0;
  u8 *base               = functions.reserve_memory(size);
  
  Assert(base);
  
  u64 initial_commit = sizeof(Memory_Arena);
  functions.commit_memory(base, initial_commit);
  result              = (Memory_Arena *)base;
  result->base        = base;
  result->capacity    = size;
  result->stack_ptr   = initial_commit;
  result->commit_ptr  = initial_commit;
  result->rm          = functions.reserve_memory;
  result->cm          = functions.commit_memory;
  result->dm          = functions.decommit_memory;
  
  return(result);
}

static void *
arena_push(Memory_Arena *arena, u64 size)
{
  void *result_block = 0;
  size               = AlignAToB(size, 16);
  u64 new_stack_ptr  = size + arena->stack_ptr;
  u64 commit_size    = KB(128);
  if (new_stack_ptr <= arena->capacity)
  {
    void *result_block2 = arena->base + arena->stack_ptr;
    if (new_stack_ptr >= arena->commit_ptr)
    {
      u64 new_commit_ptr       = AlignAToB(new_stack_ptr, commit_size);
      u64 desired_commit_ptr   = Minimum(new_commit_ptr, arena->capacity);
      
      if (desired_commit_ptr > arena->commit_ptr)
      {
        arena->cm(arena->base + arena->commit_ptr, desired_commit_ptr - arena->commit_ptr);
        arena->commit_ptr  = desired_commit_ptr;
      }
    }
    
    if (arena->commit_ptr > arena->stack_ptr)
    {
      arena->stack_ptr   = new_stack_ptr;
      result_block       = result_block2;
    }
  }
  
  Assert(result_block);
  return(result_block);
}

static void *
arena_pop(Memory_Arena *arena, u64 size)
{
  u64 commit_size    = KB(128);
  size               = AlignAToB(size, 16);
  AssertTrue(size <= (arena->stack_ptr + sizeof(Memory_Arena)));
  
  arena->stack_ptr        -= size;
  u64 desired_commit_ptr   = AlignAToB(arena->stack_ptr, commit_size);
  
  if (desired_commit_ptr < arena->commit_ptr)
  {
    arena->dm(arena->base + desired_commit_ptr, arena->commit_ptr - desired_commit_ptr);
    arena->commit_ptr = desired_commit_ptr;
  }
}

static void
game_update_and_render(Game_State *game, Platform_Functions platform_functions, Game_Input *input, f32 game_update_secs)
{
  if (!game->has_init)
  {
    game->main_arena      = arena_reserve(platform_functions, MB(32));
    game->render_state    = arena_push_struct(game->main_arena, Game_RenderState);
    game_init(game);
    game->has_init = true;
  }
  
  f32 player_dP_mag = 96.0f * game_update_secs;
  f32 player_dP_normalized_comp = 1.0f / sqrtf(2.0f);
  v3f player_dP = { 0.0f, 0.0f, 0.0f };
  if (Game_KeyHeld(input, Input_KeyType_W))
  {
    player_dP.y = player_dP_normalized_comp;
  }
  
  if (Game_KeyHeld(input, Input_KeyType_A))
  {
    player_dP.x = -player_dP_normalized_comp;
  }
  
  if (Game_KeyHeld(input, Input_KeyType_S))
  {
    player_dP.y = -player_dP_normalized_comp;
  }
  
  if (Game_KeyHeld(input, Input_KeyType_D))
  {
    player_dP.x = player_dP_normalized_comp;
  }

  if (player_dP.x || player_dP.y)
  {
    player_dP.x *= player_dP_mag;
    player_dP.y *= player_dP_mag;
    
    s32 player_block_x = (s32)(game->player_p.x / Game_BlockDimPixels);
    s32 player_block_y = (s32)(game->player_p.y / Game_BlockDimPixels);
    
    s32 collision_check_radius    = 2;
    s32 start_block_x             = player_block_x - collision_check_radius;
    s32 start_block_y             = player_block_y - collision_check_radius;
    s32 end_block_x               = player_block_x + collision_check_radius;
    s32 end_block_y               = player_block_y + collision_check_radius;
    
    start_block_x  = Maximum(start_block_x, 0);
    start_block_y  = Maximum(start_block_y, 0);
    
    v2f player_P        = { game->player_p.x, game->player_p.y };
    for (s32 chunk_y = start_block_y; chunk_y < end_block_y; ++chunk_y)
    {
      for (s32 chunk_x = start_block_x; chunk_x < end_block_x; ++chunk_x)
      {
        s32 idx                    = chunk_y * Game_ChunkDim + chunk_x;
        if (idx < (Game_ChunkDim*Game_ChunkDim))
        {        
          Game_Structure structure   = game->structure_chunk[Game_ChunkLayer_Interactable][idx];
          
          if (structure.flags & Game_InteractFlag_Collidable)
          {
            AABB *aabbs;
            u64   aabbs_count;
            get_structure_aabb(structure.type, &aabbs, &aabbs_count);
            
            for (u64 aabb_idx = 0; aabb_idx < aabbs_count; ++aabb_idx)
            {
              AABB aabb = aabbs[aabb_idx];
              v2f structure_dims     = { aabb.dims.x * Game_BlockDimPixels, aabb.dims.y * Game_BlockDimPixels };
              v2f structure_min      = { aabb.p.x * Game_BlockDimPixels + (f32)chunk_x * Game_BlockDimPixels - game->player_dims.x, aabb.p.y * Game_BlockDimPixels + (f32)chunk_y * Game_BlockDimPixels - game->player_dims.y };
              v2f structure_max      = { structure_min.x + structure_dims.x + game->player_dims.x, structure_min.y + structure_dims.y + game->player_dims.y };
              f32 t_farthest_entry   = 0.0f;
              f32 t_nearest_entry    = FLT_MAX;
              
              b32 collision = true;
              f32 epsilon   = 0.001f;
              for (u32 slab_idx = 0; slab_idx < 2; ++slab_idx)
              {
                if (absf32(player_dP.v[slab_idx]) <= epsilon)
                {
                  if ((player_P.v[slab_idx] < structure_min.v[slab_idx]) || (player_P.v[slab_idx] > structure_max.v[slab_idx]))
                  {
                    collision = false;
                    break;
                  }
                }
                else
                {
                  f32 t0 = (structure_min.v[slab_idx] - player_P.v[slab_idx]) / player_dP.v[slab_idx];
                  f32 t1 = (structure_max.v[slab_idx] - player_P.v[slab_idx]) / player_dP.v[slab_idx];
                  
                  if (t0 > t1)
                  {
                    f32 temp = t0;
                    t0 = t1;
                    t1 = temp;
                  }
                  
                  if (t0 > t_farthest_entry)
                  {
                    t_farthest_entry = t0;
                  }
                  
                  if (t1 < t_nearest_entry)
                  {
                    t_nearest_entry = t1;
                  }
                  
                  if ((t_farthest_entry - t_nearest_entry) >= -epsilon)
                  {
                    collision = false;
                    break;
                  }
                }
              }
              
              if (collision)
              {
                v2f collision_normal = {0};
                f32 t_hit = t_farthest_entry;
                if ((t_hit >= 0.0f) && (t_hit <= 1.0f))
                {
                  v2f tmn = { player_P.x - structure_min.x, player_P.y - structure_min.y };
                  v2f tmx = { structure_max.x - player_P.x, structure_max.y - player_P.y };
                  
                  u32 a = v2f_smallest_abs_comp_idx(tmn);
                  u32 b = v2f_smallest_abs_comp_idx(tmx);
                  
                  if (absf32(tmn.v[a]) < absf32(tmx.v[b]))
                  {
                    collision_normal.v[a] = -1.0f;
                  }
                  else
                  {
                    collision_normal.v[b] = 1.0f;
                  }
                  
                  v2f residue   = v2f_scale(1.0f - t_hit, player_dP.xy);
                  v2f proj      = v2f_scale(v2f_dot(residue, collision_normal), collision_normal);
                  player_dP.xy  = v2f_sub(player_dP.xy, proj);
                }
              }
            }
          }
        }
      }
    }
    
    v3f_add_eq(&game->player_p, player_dP);
  }
  
  draw_the_game(game);
}