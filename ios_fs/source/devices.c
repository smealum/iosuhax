#include <stdio.h>
#include "types.h"
#include "devices.h"
#include "imports.h"
#include "sdio.h"

void * getMdDeviceById(int deviceId)
{
    if(deviceId == DEVICE_ID_SDCARD_PATCHED)
    {
        return (void*)FS_MMC_SDCARD_STRUCT;
    }
    else if(deviceId == DEVICE_ID_MLC)
    {
        return (void*)FS_MMC_MLC_STRUCT;
    }
    return NULL;
}

int registerMdDevice_hook(void * md, int arg2, int arg3)
{
    u32 *mdStruct = (u32*)md;

    if((md != 0) && (mdStruct[2] == (u32)FS_MMC_SDCARD_STRUCT))
    {
        sdcard_lock_mutex();
        FS_MMC_SDCARD_STRUCT[0x24/4] = FS_MMC_SDCARD_STRUCT[0x24/4] & (~0x20);

        int result = FS_REGISTERMDPHYSICALDEVICE(md, arg2, arg3);

        sdcard_unlock_mutex();

        return result;
    }

    return FS_REGISTERMDPHYSICALDEVICE(md, arg2, arg3);
}

/*

; read1(void *physical_device_info, int offset_high, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
; readWriteCallback_patch(bool read, int offset_offset, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
readWriteCallback_patch:
	readWriteCallback_patch_stackframe_size equ (7*4)
	push {r0,r1,r2,r3,r4,r5,lr}
	mov r5, #0xDA
	str r5, [sp, #0x8] ; device id (sdcard)
	add r5, sp, #0xC ; out_callback_arg2 dst
	add r2, r1
	str r2, [sp] ; offset
	str r5, [sp, #4] ; out_callback_arg2
	ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0x4] ; data_ptr
	mov r2, r3 ; cnt
	ldr r3, [sp, #readWriteCallback_patch_stackframe_size] ; block_size
	bl sdcard_readwrite
	mov r4, r0
	cmp r0, #0
	bne readWriteCallback_patch_skip_callback

	ldr r12, [sp, #readWriteCallback_patch_stackframe_size+0x8] ; callback
	ldr r0, [r5]
	ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0xC] ; callback_parameter
	cmp r12, #0
	blxne r12

	readWriteCallback_patch_skip_callback:
	mov r0, r4
	add sp, #4
	pop {r1,r2,r3,r4,r5,pc}

*/

typedef void (*read_write_callback_t)(int, int);

//! read1(void *physical_device_info, int offset_high, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
int readWriteCallback_patch(int is_read, int offset_offset, int offset_low, int cnt, int block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter)
{
    int result_arg;
    int result = sdcard_readwrite(is_read, data_outptr, cnt, block_size, offset_offset + offset_low, &result_arg, DEVICE_ID_SDCARD_PATCHED);

    if(result == 0)
    {
        if(callback != 0)
        {
            callback(result_arg, callback_parameter);
        }
    }
    return result;
}
