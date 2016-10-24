#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "mlcio.h"
#include "text.h"

#define SLC_SECTOR_COUNT        0x40000 // * SLC_BYTES_PER_SECTOR to get bytes

// the IO buffer is put behind everything else because there is no access to this region from IOS-FS it seems
unsigned char io_buffer[0x40000]  __attribute__((aligned(0x40))) __attribute__((section(".io_buffer")));

void dump_syslog()
{
    FS_MEMCPY(io_buffer, *(void**)0x05095ECC, sizeof(io_buffer));
    sdcard_readwrite(SDIO_WRITE, io_buffer, sizeof(io_buffer) / SDIO_BYTES_PER_SECTOR, SDIO_BYTES_PER_SECTOR, SYSLOG_BASE_SECTORS, NULL, DEVICE_ID_SDCARD_PATCHED);
}

int write_data_offset(void *data_ptr, u32 size, u32 offset_blocks)
{
    //! TODO: FIX ME -> only works with a size multiple of 0x200
    u32 sectors = size >> 9; // size / SDIO_BYTES_PER_SECTOR but faster ;)
    if(sectors == 0)
        sectors = 1;

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

        readSlc(io_buffer, offset, (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR), deviceId);
        offset += (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR);

        FS_SLEEP(10);

        write_data_offset(io_buffer, sizeof(io_buffer), base_sectors);
        base_sectors += (sizeof(io_buffer) / SDIO_BYTES_PER_SECTOR);
    }
    while (offset <SLC_SECTOR_COUNT); //! TODO: make define SLC_SECTOR_COUNT
}

int read_mlc(void *data_ptr, u32 num_sectors, u32 offset_sectors)
{
    return sdcard_readwrite(SDIO_READ, data_ptr, num_sectors, MLC_BYTES_PER_SECTOR, offset_sectors, NULL, DEVICE_ID_MLC);
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
            mlc_result = read_mlc(io_buffer, (sizeof(io_buffer) / MLC_BYTES_PER_SECTOR), offset);
            write_result = write_data_offset(io_buffer, sizeof(io_buffer), MLC_BASE_SECTORS + offset);
            offset += (sizeof(io_buffer) / MLC_BYTES_PER_SECTOR);
            syslog_counter++;
        }
        while (syslog_counter < 0x40);
    }
    while(offset < 0x3A20000); //! TODO: make define MLC32_SECTOR_COUNT
}


void dumper_main()
{
    mlc_init();

    slc_dump(0x0E, "slc     = %08X", SLC_BASE_SECTORS);
    slc_dump(0x0E, "slc2    = %08X", SLC_BASE_SECTORS);         //! wtf? why is this dumped again
    slc_dump(0x0F, "slccmpt = %08X", SLCCMPT_BASE_SECTORS);
    mlc_dump();

    //! intentional crash
    u32 * nullPtr = (u32 *)0;
    nullPtr[0] = 0;
}


void dump_data(void* data_ptr, u32 size)
{
    static u32 dumpdata_offset = 0;

    u32 num_sectors = size >> 9; // size / SDIO_BYTES_PER_SECTOR but faster ;)
    if (num_sectors == 0)
        num_sectors = 1;

    sdcard_readwrite(SDIO_WRITE, data_ptr, num_sectors, SDIO_BYTES_PER_SECTOR, DUMPDATA_BASE_SECTORS + dumpdata_offset, NULL, DEVICE_ID_SDCARD_PATCHED);
    dumpdata_offset += num_sectors;
}

void dump_lots_data(u8* addr, u32 size)
{
    u32 cur_size;
    u32 size_remaining = size;
    u8* cur_addr = addr;
    do
    {
        cur_size = sizeof(io_buffer);
        if (cur_size > size_remaining)
            cur_size = size_remaining;

        FS_MEMCPY(io_buffer, cur_addr, cur_size);
        dump_data(io_buffer, cur_size);

        cur_addr += cur_size;
        size_remaining -= cur_size;
    }
    while (cur_size != 0);
}
