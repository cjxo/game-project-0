#if !defined(PLATFORM_H)
#define PLATFORM_H

typedef struct
{
  u8 *file;
  u64 capacity;
} Read_File_Result;

typedef void *                       (*Platform_ReserveMemory)(u64 size);
typedef void *                       (*Platform_CommitMemory)(void *base, u64 size);
typedef b32                          (*Platform_DecommitMemory)(void *base, u64 size);
typedef Read_File_Result             (*Platform_ReadEntireFile)(char *filename);

typedef struct
{
  Platform_ReserveMemory   reserve_memory;
  Platform_CommitMemory    commit_memory;
  Platform_DecommitMemory  decommit_memory;
  Read_File_Result         read_entire_file;
} Platform_Functions;

#endif