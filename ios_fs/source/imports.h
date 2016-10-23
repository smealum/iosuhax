#ifndef IMPORTS_H_
#define IMPORTS_H_

#define FS_SVC_CREATEMUTEX                  ((int (*)(int, int))0x107F6BBC)
#define FS_SVC_ACQUIREMUTEX                 ((int (*)(int, int))0x107F6BC4)
#define FS_SVC_RELEASEMUTEX                 ((int (*)(int))0x107F6BCC)
#define FS_SVC_DESTROYMUTEX                 ((int (*)(int))0x107F6BD4)

#define FS_SLEEP                            ((void (*)(int))0x1071D668)
#define FS_MEMCPY                           ((void* (*)(void*, void*, u32))0x107F4F7C)
#define FS_VSNPRINTF                        ((void* (*)(char * s, size_t n, const char * format, va_list arg))0x107F5F68)
#define FS_SNPRINTF                         ((void* (*)(char * s, size_t n, const char * format, ...))0x107F5FB4)

#define FS_RAW_READ1                        ((int (*)(int handle, u32 offset_high, u32 offset_low, u32 size, void* buf, void *callback, int callback_arg))0x10732BC0)
#define FS_SDIO_DOREADWRITECOMMAND          ((int (*)(int, void*, u32, void*, void*))0x10718A8C)

#define FS_REGISTERMDPHYSICALDEVICE         ((int (*)(void*, int, int))0x10718860)

#define FS_MMC_SDCARD_STRUCT                ((vu32*)0x1089B9F8)
#define FS_MMC_MLC_STRUCT                   ((vu32*)0x1089B948)


#endif // IMPORTS_H_
