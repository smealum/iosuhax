#ifndef FSA_H
#define FSA_H

typedef struct
{
	u32 unk[0x19];
	char name[0x100];
}directoryEntry_s;

typedef struct
{
	u32 unk1[0x4];
	u32 size; // size in bytes
	u32 physsize; // physical size on disk in bytes
	u32 unk2[0x13];
}fileStat_s;

#define FSA_MOUNTFLAGS_BINDMOUNT (1 << 0)
#define FSA_MOUNTFLAGS_GLOBAL (1 << 1)

int FSA_Open();

int FSA_Mount(int fd, char* device_path, char* volume_path, u32 flags, char* arg_string, int arg_string_len);
int FSA_Unmount(int fd, char* path, u32 flags);

int FSA_GetDeviceInfo(int fd, char* device_path, int type, u32* out_data);

int FSA_MakeDir(int fd, char* path, u32 flags);
int FSA_OpenDir(int fd, char* path, int* outHandle);
int FSA_ReadDir(int fd, int handle, directoryEntry_s* out_data);
int FSA_CloseDir(int fd, int handle);

int FSA_OpenFile(int fd, char* path, char* mode, int* outHandle);
int FSA_ReadFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags);
int FSA_WriteFile(int fd, void* data, u32 size, u32 cnt, int fileHandle, u32 flags);
int FSA_StatFile(int fd, int handle, fileStat_s* out_data);
int FSA_CloseFile(int fd, int fileHandle);

int FSA_RawOpen(int fd, char* device_path, int* outHandle);
int FSA_RawRead(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
int FSA_RawWrite(int fd, void* data, u32 size_bytes, u32 cnt, u64 sector_offset, int device_handle);
int FSA_RawClose(int fd, int device_handle);

#endif
