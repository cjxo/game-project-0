#if !defined (GAME_H)
#define GAME_H

#include "platform.h"

typedef u16 Input_KeyType;
enum
{
  Input_KeyType_Escape,
  Input_KeyType_Space,
  
  Input_KeyType_0, Input_KeyType_1, Input_KeyType_2, Input_KeyType_3, Input_KeyType_4,
  Input_KeyType_5, Input_KeyType_6, Input_KeyType_7, Input_KeyType_8, Input_KeyType_9,
  
  Input_KeyType_A, Input_KeyType_B, Input_KeyType_C, Input_KeyType_D, Input_KeyType_E,
  Input_KeyType_F, Input_KeyType_G, Input_KeyType_H, Input_KeyType_I, Input_KeyType_J,
  Input_KeyType_K, Input_KeyType_L, Input_KeyType_M, Input_KeyType_N, Input_KeyType_O,
  Input_KeyType_P, Input_KeyType_Q, Input_KeyType_R, Input_KeyType_S, Input_KeyType_T,
  Input_KeyType_U, Input_KeyType_V, Input_KeyType_W, Input_KeyType_X, Input_KeyType_Y,
  Input_KeyType_Z,
  
  Input_KeyType_Count,
};

typedef u16 Input_ButtonType;
enum
{
  Input_ButtonType_Left,
  Input_ButtonType_Right,
  Input_ButtonType_Count,
};

typedef u8 Input_InteractFlag;
enum
{
  Input_InteractFlag_Pressed = 0x1,
  Input_InteractFlag_Released = 0x2,
  Input_InteractFlag_Held = 0x4,
};

#define Game_KeyPressed(inp,keytype) ((inp)->key[keytype]&Input_InteractFlag_Pressed)
#define Game_KeyReleased(inp,keytype) ((inp)->key[keytype]&Input_InteractFlag_Released)
#define Game_KeyHeld(inp,keytype) ((inp)->key[keytype]&Input_InteractFlag_Held)
typedef struct
{
  Input_InteractFlag key[Input_KeyType_Count];
  Input_InteractFlag button[Input_ButtonType_Count];
  s32 mouse_x, mouse_y, prev_mouse_x, prev_mouse_y;
} Game_Input;

//

typedef struct
{
  u8 *base;
  u64 capacity;
  u64 stack_ptr;
  u64 commit_ptr;
  
  Platform_ReserveMemory   rm;
  Platform_CommitMemory    cm;
  Platform_DecommitMemory  dm;
} Memory_Arena;

#define arena_push_array(a,t,c) (t*)arena_push((a),sizeof(t)*(c))
#define arena_push_struct(a,t) arena_push_array(a,t,1)
static void           *arena_push(Memory_Arena *arena, u64 size);
static void           *arena_pop(Memory_Arena *arena, u64 size);
//

#define Game_MaxQuadInstances 512
typedef struct
{
  v3f p;
  v3f dims;  
  v4f colour;
  
  v2f atlas_p;
  v2f atlas_dims;
  u32 tex_on_me;
} Game_QuadInstance;

typedef u16 Game_InteractFlag;
enum
{
  Game_InteractFlag_Interactable = 0x1,
  Game_InteractFlag_Collidable = 0x2,
};

typedef u16 Game_StructureType;
enum
{
  Game_StructureType_Grass_Flat,
  
  Game_StructureType_Grass_Blade,
  Game_StructureType_Tree_Fir,
  Game_StructureType_Rock,
  Game_StructureType_Count,
};

typedef u16 Game_ChunkLayer;
enum
{
  Game_ChunkLayer_NonInteractable,
  Game_ChunkLayer_Interactable,
  Game_ChunkLayer_Count,
};

typedef struct
{
  Game_StructureType type;
  Game_InteractFlag flags;
} Game_Structure;

typedef u16 Game_RenderFlag;
enum
{
  Game_RenderFlag_DrawWire                     = 0x1,
  Game_RenderFlag_DisableDepth                 = 0x2,
};

typedef struct
{
  Game_QuadInstance instances[Game_MaxQuadInstances];
  u64               count;
} Game_QuadInstances;

typedef struct
{
  Game_QuadInstances filled_quads;
  Game_QuadInstances wire_quads;
} Game_RenderState;

#define Game_BlockDimPixels 48
#define Game_ChunkDim 8
typedef struct
{
  Memory_Arena      *main_arena;
  Game_RenderState  *render_state;
  Game_Structure     structure_chunk[Game_ChunkLayer_Count][Game_ChunkDim * Game_ChunkDim];
  PRNG_PCG32         pcg32;
  
  v3f player_p;
  v3f player_dims;
  
  b32 has_init;
} Game_State;

static void game_init(Game_State *state);
static void game_update_and_render(Game_State *state, Platform_Functions platform_functions, Game_Input *input, f32 game_update_secs);

#endif