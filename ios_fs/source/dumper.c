#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "text.h"


static unsigned char syslog_buffer[0x40000] __attribute__((aligned(0x40)));
static unsigned char sdcard_read_buffer[0x100000] __attribute__((aligned(0x40)));

void dump_syslog()
{
    FS_MEMCPY(syslog_buffer, *(void**)0x05095ECC, 0x40000);
    sdcard_readwrite(SDIO_WRITE, syslog_buffer, 0x40000 / SDIO_BYTES_PER_SECTOR, SDIO_BYTES_PER_SECTOR, SYSLOG_BASE_SECTORS, NULL, DEVICE_ID_SDCARD_PATCHED);
}

int write_data_offset(void *data_ptr, u32 size, u32 offset_blocks)
{
    //! TODO: FIX ME -> only works with a size multiple of 0x200
    u32 sectors = size >> 9; // size / SDIO_BYTES_PER_SECTOR but faster ;)
    if(sectors == 0)
    {
        sectors = 1;
    }

    return sdcard_readwrite(SDIO_WRITE, data_ptr, sectors, SDIO_BYTES_PER_SECTOR, offset_blocks, NULL, DEVICE_ID_SDCARD_PATCHED);
}

//! rawRead1PhysicalDevice_(int physical_device_handle, unsigned int offset_high, unsigned int offset_low, unsigned int size, void *outptr, int (__fastcall *callback)(unsigned int, int), int callback_arg)
int readSlc(void *data_ptr, u32 offset, u32 sectors, int deviceId)
{
    return FS_RAW_READ1(getPhysicalDeviceHandle(deviceId), 0, offset, sectors, data_ptr, 0, 0);
}


void slc_dump(int deviceId, const char* format, u32 base_sectors)
{
    FS_SLEEP(1000);
    u32 offset = 0;

    do
    {
        _printf(20, 0, format, offset);

        readSlc(sdcard_read_buffer, offset, (0x40000 / SLC_BYTES_PER_SECTOR), deviceId);
        offset += (0x40000 / SLC_BYTES_PER_SECTOR);

        FS_SLEEP(10);

        write_data_offset(sdcard_read_buffer, 0x40000, base_sectors);
        base_sectors += (0x40000 / SDIO_BYTES_PER_SECTOR);
    }
    while (offset < 0x40000); //! TODO: make define SLC_SECTOR_COUNT
}

int read_mlc(void *data_ptr, u32 num_sectors, u32 offset_sectors)
{
    return sdcard_readwrite(SDIO_READ, data_ptr, num_sectors, MLC_BYTES_PER_SECTOR, offset_sectors, 0, DEVICE_ID_MLC);
}

void mlc_dump(void)
{
    u32 offset = 0;

    int mlc_result = 0;
    int write_result = 0;

    do
    {
        int syslog_counter = 0;

        //! TODO: this i nuts actually -> not required and to be removed
        dump_syslog();

        do
        {
            _printf(20, 0, " %08X %08X %08X", offset, mlc_result, write_result);

            //! TODO: at least check result values .... god...and the buffers are way too huge -> reduce to 0x40000 and use the same for io and syslog
            mlc_result = read_mlc(sdcard_read_buffer, (0x100000 / MLC_BYTES_PER_SECTOR), offset);
            write_result = write_data_offset(sdcard_read_buffer, 0x100000, MLC_BASE_SECTORS + offset);
            offset += (0x100000 / MLC_BYTES_PER_SECTOR);
            syslog_counter++;
        }
        while (syslog_counter < 0x40);
    }
    while(offset < 0x3A20000); //! TODO: make define MLC32_SECTOR_COUNT
}
