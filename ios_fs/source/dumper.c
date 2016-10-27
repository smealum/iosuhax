#include <stdio.h>
#include "types.h"
#include "imports.h"
#include "devices.h"
#include "sdio.h"
#include "mlcio.h"
#include "fat32_format.h"
#include "text.h"
#include "hardware_registers.h"
#include "svc.h"

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
static inline int readSlc(void *data_ptr, u32 offset, u32 sectors, int deviceHandle)
{
    return FS_RAW_READ1(deviceHandle, 0, offset, sectors, data_ptr, 0, 0);
}

//! this one is required for the read function
static void slc_read_callback(int result, int priv)
{
    int *private_data = (int*)priv;
    private_data[1] = result;
    FS_SVC_RELEASEMUTEX(private_data[0]);
}

void slc_dump(void *deviceHandle, const char* device, u32 base_sectors, int y_offset)
{
	//also create a mutex for synchronization with end of operation...
    int sync_mutex = FS_SVC_CREATEMUTEX(1, 1);
    FS_SVC_ACQUIREMUTEX(sync_mutex, 0);

    int result_array[2];
    result_array[0] = sync_mutex;

    u32 offset = 0;
    int readResult = 0;
    int writeResult = 0;
    int retry = 0;

    FS_SLEEP(1000);

    do
    {
        _printf(20, y_offset, "%s     = %08X / 40000, read code %08X, write code %08X, retry %d", device, offset, readResult, writeResult, retry);

        //! pre set a defined memory buffer
        FS_MEMSET(io_buffer, 0xff, sizeof(io_buffer));
        //readResult = readSlc(io_buffer, offset, (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR), deviceHandle);
        readResult = slcRead1_original(deviceHandle, 0, offset, (sizeof(io_buffer) / SLC_BYTES_PER_SECTOR), SLC_BYTES_PER_SECTOR, io_buffer, slc_read_callback, (int)result_array);

        //! retry 5 times as there are read failures in several places
        if((readResult != 0) && (retry < 5))
        {
            FS_SLEEP(100);
            retry++;
        }
        else
        {
            //! wait for operation to finish
            FS_SVC_ACQUIREMUTEX(sync_mutex, 0);

            if((result_array[1] != 0) && (retry < 5))
            {
                readResult = result_array[1];
                FS_SLEEP(100);
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
    }
    while (offset < SLC_SECTOR_COUNT);

    FS_SVC_DESTROYMUTEX(sync_mutex);

    // last print to show "done"
    _printf(20, y_offset, "%s     = %08X / 40000, read code %08X, write code %08X, retry %d", device, offset, readResult, writeResult, retry);
}

void mlc_dump(u32 base_sector, u32 mlc_end)
{
    u32 offset = 0;

    int retry = 0;
    int mlc_result = 0;
    int callback_result = 0;
    int write_result = 0;
    int print_counter = 0;

    do
    {
        //! print only every 4th time
        if(print_counter == 0)
        {
            print_counter = 4;
            _printf(20, 70, "mlc     = %08X / %08X, mlc res %08X, sd res %08X, retry %d", offset, mlc_end, mlc_result, write_result, retry);
        }
        else
        {
            --print_counter;
        }

        memset(io_buffer, 0xff, sizeof(io_buffer));
        mlc_result = sdcard_readwrite(SDIO_READ, io_buffer, (sizeof(io_buffer) / MLC_BYTES_PER_SECTOR), MLC_BYTES_PER_SECTOR, offset, &callback_result, DEVICE_ID_MLC);
        //mlc_result = sdcardRead_original(deviceHandle, 0, offset, (sizeof(io_buffer) / MLC_BYTES_PER_SECTOR), MLC_BYTES_PER_SECTOR, io_buffer, read_dummy_callback, 0);

        if((mlc_result == 0) && (callback_result != 0))
        {
            mlc_result = callback_result;
        }

        //! retry 5 times as there are read failures in several places
        if((mlc_result != 0) && (retry < 5))
        {
            FS_SLEEP(100);
            retry++;
            print_counter = 0; // print errors directly
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
                FS_SLEEP(100);
                retry++;
                print_counter = 0; // print errors directly
            }
        }
    }
    while(offset < mlc_end); //! TODO: make define MLC32_SECTOR_COUNT

    // last print to show "done"
    _printf(20, 70, "mlc     = %08X / %08X, mlc res %08X, sd res %08X, retry %d", offset, mlc_end, mlc_result, write_result, retry);
}

int check_nand_type(void)
{
    //! just try reading from offset 16GB
    if( FS_MMC_MLC_STRUCT[0x30/4] == MLC_32GB_SECTOR_COUNT)
    {
        return MLC_NAND_TYPE_32GB;
    }
    else
    {
        return MLC_NAND_TYPE_8GB;
    }
}

int check_nand_dump(void)
{
    u32 mlc_sector_count = FS_MMC_MLC_STRUCT[0x30/4];

    int signature_correct = 0;
    u32 * signature = (u32*)io_buffer;
    memset(signature, 0, SDIO_BYTES_PER_SECTOR);
    sdcard_readwrite(SDIO_READ, signature, 1, SDIO_BYTES_PER_SECTOR, NAND_DUMP_SIGNATURE_SECTOR, NULL, DEVICE_ID_SDCARD_PATCHED);

    signature_correct = (signature[0] == NAND_DUMP_SIGNATURE);

    memset(io_buffer, 0, SDIO_BYTES_PER_SECTOR);
    sdcard_readwrite(SDIO_READ, io_buffer, 1, SDIO_BYTES_PER_SECTOR, 0, NULL, DEVICE_ID_SDCARD_PATCHED);

    return signature_correct && CheckFAT32Partition(io_buffer, MLC_BASE_SECTORS + mlc_sector_count);
}

static void wait_format_confirmation(void)
{
    int timeout = 200;
    //"Press the POWER button SD then , else the console will reboot in %u seconds."
    while(1)
    {
        _printf(20, 30, "No NAND dump detected. SD Format and complete NAND dump required.");
        _printf(20, 40, "Press the POWER button to format SD card otherwise the console will reboot in %d seconds.", timeout/10);

        if(svcRead32(LT_GPIO_IN) & GPIO_IN_POWER_BUTTON)
        {
            break;
        }

        if(--timeout == 0)
        {
            FS_SLEEP(1000);
            svcShutdown(SHUTDOWN_TYPE_REBOOT);
        }

        FS_SLEEP(100);
    }

    // clear the lines
    clearLine(30, 0x000000FF);
    clearLine(40, 0x000000FF);
}

void dump_nand_complete()
{
    wait_format_confirmation();

    mlc_init();
    FS_SLEEP(1000);

    int nand_type = check_nand_type();
    u32 sdio_sector_count = FS_MMC_SDCARD_STRUCT[0x30/4];
    u32 mlc_sector_count = FS_MMC_MLC_STRUCT[0x30/4];
    u32 fat32_partition_offset = (MLC_BASE_SECTORS + mlc_sector_count);

    _printf(20, 30, "Detected %d GB MLC NAND type.", (nand_type == MLC_NAND_TYPE_8GB) ? 8 : 32);

    if(sdio_sector_count < fat32_partition_offset)
    {
        _printf(20, 40, "SD card too small! Required sectors %u > available %u.", fat32_partition_offset, sdio_sector_count);
        FS_SLEEP(3000);
        svcShutdown(SHUTDOWN_TYPE_REBOOT);
    }

    if( FormatSDCard(fat32_partition_offset, sdio_sector_count) < 0 )
    {
        FS_SLEEP(3000);
        svcShutdown(SHUTDOWN_TYPE_REBOOT);
    }

    slc_dump(FS_SLC_PHYS_DEV_STRUCT, "slc", SLC_BASE_SECTORS, 50);
    slc_dump(FS_SLCMPT_PHYS_DEV_STRUCT, "slccmpt", SLCCMPT_BASE_SECTORS, 60);
    mlc_dump(MLC_BASE_SECTORS, mlc_sector_count);

    //! write marker to SD card from which we can auto detect NAND dump
    //! we can actually use that for settings
    u32 * signature = (u32*)io_buffer;
    memset(io_buffer, 0, SDIO_BYTES_PER_SECTOR);
    signature[0] = NAND_DUMP_SIGNATURE;

    sdcard_readwrite(SDIO_WRITE, io_buffer, 1, SDIO_BYTES_PER_SECTOR, NAND_DUMP_SIGNATURE_SECTOR, NULL, DEVICE_ID_SDCARD_PATCHED);

    _printf(20, 80, "Complete! -> rebooting into sysNAND...");

    FS_SLEEP(3000);
    svcShutdown(SHUTDOWN_TYPE_REBOOT);
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
