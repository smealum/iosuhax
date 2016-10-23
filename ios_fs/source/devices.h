#ifndef DEVICES_H_
#define DEVICES_H_

#define DEVICE_TYPE_SDCARD                  0x06

#define DEVICE_ID_SDCARD_REAL               0x43
#define DEVICE_ID_SDCARD_PATCHED            0xDA

#define DEVICE_ID_MLC                       0xAB

#define SDIO_BYTES_PER_SECTOR               512
#define MLC_BYTES_PER_SECTOR                512
#define SLC_BYTES_PER_SECTOR                2048

#define USB_BASE_SECTORS                    (0x2720000)
#define SLC_BASE_SECTORS                    (0x2F20000)
#define SLCCMPT_BASE_SECTORS                (0x3020000)
#define MLC_BASE_SECTORS                    (0x3200000)
#define SYSLOG_BASE_SECTORS                 (0x6D00000)

int getPhysicalDeviceHandle(int device);

#endif // DEVICES_H_
