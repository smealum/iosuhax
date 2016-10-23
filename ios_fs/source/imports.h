#ifndef IMPORTS_H_
#define IMPORTS_H_

#define FS_SVC_CREATEMUTEX                  ((int (*)(int, int))0x107F6BBC)
#define FS_SVC_ACQUIREMUTEX                 ((int (*)(int, int))0x107F6BC4)
#define FS_SVC_RELEASEMUTEX                 ((int (*)(int))0x107F6BCC)
#define FS_SVC_DESTROYMUTEX                 ((int (*)(int))0x107F6BD4)

#define FS_SLEEP                            ((void (*)(int))0x1071D668)

#define FS_SDIO_DOREADWRITECOMMAND          ((int (*)(int, void*, u32, void*, void*))0x10718A8C)

#define FS_REGISTERMDPHYSICALDEVICE         ((int (*)(void*, int, int))0x10718860)

#define FS_MMC_SDCARD_STRUCT                ((vu32*)0x1089B9F8)
#define FS_MMC_MLC_STRUCT                   ((vu32*)0x1089B948)


#endif // IMPORTS_H_
