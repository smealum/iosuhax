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

#define NAND_DUMP_SIGNATURE_SECTOR          0x800
#define NAND_DUMP_SIGNATURE                 0x48415858 // HAXX

int getPhysicalDeviceHandle(u32 device);

#endif // DEVICES_H_
