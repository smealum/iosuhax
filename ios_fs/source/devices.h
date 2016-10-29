#ifndef DEVICES_H_
#define DEVICES_H_

#define DEVICE_TYPE_SDCARD                  0x06

#define DEVICE_ID_SDCARD_REAL               0x43
#define DEVICE_ID_SDCARD_PATCHED            0xDA

#define DEVICE_ID_MLC                       0xAB

#define SDIO_BYTES_PER_SECTOR               512
#define MLC_BYTES_PER_SECTOR                512
#define SLC_BYTES_PER_SECTOR                2048

#define SLC_BASE_SECTORS                    (0x000500)
#define SLCCMPT_BASE_SECTORS                (0x100500)
#define MLC_BASE_SECTORS                    (0x200500)

#define USB_BASE_SECTORS                    (0x2720000)
#define SYSLOG_BASE_SECTORS                 (0x6D00000)
#define DUMPDATA_BASE_SECTORS               (SYSLOG_BASE_SECTORS + (0x40000 / SDIO_BYTES_PER_SECTOR))

#define SLC_SECTOR_COUNT                    0x40000
#define MLC_8GB_SECTOR_COUNT                0xE90000
#define MLC_32GB_SECTOR_COUNT               0x3A3E000  //0x3A20000

#define MLC_NAND_TYPE_32GB                  0
#define MLC_NAND_TYPE_8GB                   1

#define NAND_DUMP_SIGNATURE_SECTOR          0x01
#define NAND_DUMP_SIGNATURE                 0x4841585844554d50ULL // HAXXDUMP

typedef _sdio_nand_signature_sector_t
{
    u64 signature;              // HAXXDUMP
    u32 slc_base_sector;        // base sector of slc dump
    u32 slc_sector_count;       // sector count of slc in SDIO sectors
    u32 slccmpt_base_sector;    // base sector of slccmpt dump
    u32 slccmpt_sector_count;   // sector count of slc in SDIO sectors
    u32 mlc_base_sector;        // base sector of slccmpt dump
    u32 mlc_sector_count;       // sector count of mlc in SDIO sectors
} sdio_nand_signature_sector_t


typedef void (*read_write_callback_t)(int, int);

int getPhysicalDeviceHandle(u32 device);

int slcRead1_original(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter);
int sdcardRead_original(void *physical_device_info, u32 offset_high, u32 offset_low, u32 cnt, u32 block_size, void *data_outptr, read_write_callback_t callback, int callback_parameter);

#endif // DEVICES_H_
