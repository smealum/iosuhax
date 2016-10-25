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

static inline int write_data_offset(void *data_ptr, u32 size, u32 offset_blocks)
{
    //! TODO: FIX ME -> only works with a size multiple of 0x200
    u32 sectors = size >> 9; // size / SDIO_BYTES_PER_SECTOR but faster ;)
    if(sectors == 0)
        sectors = 1;

    return sdcard_readwrite(SDIO_WRITE, data_ptr, sectors, SDIO_BYTES_PER_SECTOR, offset_blocks, NULL, DEVICE_ID_SDCARD_PATCHED);
}

//! rawRead1PhysicalDevice_(int physical_device_handle, unsigned int offset_high, unsigned int offset_low, unsigned int size, void *outptr, int (__fastcall *callback)(unsigned int, int), int callback_arg)
static inline int readSlc(void *data_ptr, u32 offset, u32 sectors, int deviceId)
{
    return FS_RAW_READ1(getPhysicalDeviceHandle(deviceId), 0, offset, sectors, data_ptr, 0, 0);
}

void slc_dump(int deviceId, const char* format, u32 base_sectors)
{
    u32 offset = 0;
    int readResult = 0;
    int writeResult = 0;
    int retry = 0;

    FS_SLEEP(1000);

    do
    {
        _printf(20, 50, format, offset, readResult, writeResult, retry);

        //! pre set a defined memory buffer
        FS_MEMSET(io_buffer, 0xff, sizeof(io_buffer));
        readResult = readSlc(io_buffer, offset, (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR), deviceId);

        //! retry 5 times as there are read failures in several places
        if((readResult != 0) && (retry < 5))
        {
            FS_SLEEP(300);
            retry++;
        }
        else
        {
            FS_SLEEP(10);

            writeResult = write_data_offset(io_buffer, sizeof(io_buffer), base_sectors);
            if((writeResult == 0) || (retry >= 5))
            {
                retry = 0;
                base_sectors += (sizeof(io_buffer) / SDIO_BYTES_PER_SECTOR);
                offset += (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR);
            }
            else
            {
                retry++;
            }
        }
    }
    while (offset < SLC_SECTOR_COUNT);
}

static inline int read_mlc(void *data_ptr, u32 num_sectors, u32 offset_sectors)
{
    return sdcard_readwrite(SDIO_READ, data_ptr, num_sectors, MLC_BYTES_PER_SECTOR, offset_sectors, NULL, DEVICE_ID_MLC);
}

void mlc_dump(u32 base_sector, u32 mlc_end)
{
    u32 offset = 0;

    int retry = 0;
    int mlc_result = 0;
    int write_result = 0;

    do
    {
        _printf(20, 50, " %08X / %08X, mlc res %08X, sd res %08X, retry %d", offset, mlc_end, mlc_result, write_result, retry);

        mlc_result = read_mlc(io_buffer, (sizeof(io_buffer) / MLC_BYTES_PER_SECTOR), offset);
        //! retry 5 times as there are read failures in several places
        if((mlc_result != 0) && (retry < 5))
        {
            FS_SLEEP(100);
            retry++;
        }
        else
        {
            write_result = write_data_offset(io_buffer, sizeof(io_buffer), base_sector + offset);
            if((write_result == 0) || (retry >= 5))
            {
                retry = 0;
                offset += (sizeof(io_buffer) / MLC_BYTES_PER_SECTOR);
            }
            else
            {
                retry++;
            }
        }
    }
    while(offset < mlc_end); //! TODO: make define MLC32_SECTOR_COUNT
}

void dumper_main()
{
    mlc_init();

    slc_dump(0x0E, "slc     = %08X", SLC_BASE_SECTORS);
    slc_dump(0x0E, "slc2    = %08X", SLC_BASE_SECTORS);         //! wtf? why is this dumped again
    slc_dump(0x0F, "slccmpt = %08X", SLCCMPT_BASE_SECTORS);
    mlc_dump(MLC_BASE_SECTORS, MLC_32GB_SECTOR_COUNT);

    FS_IOS_SHUTDOWN(1);
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
