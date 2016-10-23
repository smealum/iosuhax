#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"

#define SYSLOG_BASE_SECTORS     (0x6D00000)

static unsigned char syslog_buffer[0x40000] __attribute__((aligned(0x40)));

void dump_syslog()
{
    FS_MEMCPY(syslog_buffer, *(void**)0x05095ECC, 0x40000);
    sdcard_readwrite(SDIO_WRITE, syslog_buffer, 0x200, 0x200, SYSLOG_BASE_SECTORS, NULL, DEVICE_ID_SDCARD_PATCHED);
}

int write_data_offset(void *data_ptr, u32 size, u32 offset_blocks)
{
    //! TODO: FIX ME -> only works with a size multiple of 0x200
    u32 sectors = size >> 9;
    if(sectors == 0)
    {
        sectors = 1;
    }

    return sdcard_readwrite(SDIO_WRITE, data_ptr, sectors, 0x200, offset_blocks, NULL, DEVICE_ID_SDCARD_PATCHED);
}
