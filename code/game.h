#if !defined (GAME_H)
#define GAME_H

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

typedef struct
{
  Input_InteractFlag key[Input_KeyType_Count];
  Input_InteractFlag button[Input_ButtonType_Count];
} Game_Input;

//

#define Game_MaxQuadInstances 512
typedef struct
{
  v3f p;
  v3f dims;
  
  v4f colour;
} Game_QuadInstance;

typedef struct
{
  Game_QuadInstance game_quad_instances[Game_MaxQuadInstances];
  u64               game_quad_instance_count;
} Game_State;

static void game_update_and_render(Game_State *state, Game_Input *input, f32 game_update_secs);

#endif