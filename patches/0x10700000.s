.arm.big

.open "sections/0x10700000.bin","patched_sections/0x10700000.bin",0x10700000

SECTION_BASE equ 0x10700000
SECTION_SIZE equ 0x000F8200
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)

FRAMEBUFFER_ADDRESS equ (0x14000000+0x38C0000)
FRAMEBUFFER_STRIDE equ (0xE00)
CHARACTER_MULT equ (2)
CHARACTER_SIZE equ (8*CHARACTER_MULT)

USB_BASE_SECTORS equ (0x2720000)
SLC_BASE_SECTORS equ (0x2F20000)
SLCCMPT_BASE_SECTORS equ (0x3020000)
MLC_BASE_SECTORS equ (0x3200000)
SYSLOG_BASE_SECTORS equ (0x6D00000)
DUMPDATA_BASE_SECTORS equ (SYSLOG_BASE_SECTORS + (0x40000 / 0x200))

FS_BSS_START equ (0x10835000 + 0x1406554)
FS_MMC_SDCARD_STRUCT equ (0x1089B9F8)
FS_MMC_MLC_STRUCT equ (0x1089B948)

FS_SNPRINTF equ 0x107F5FB4
FS_MEMCPY equ 0x107F4F7C
FS_MEMSET equ 0x107F5018
FS_SYSLOG_OUTPUT equ 0x107F0C84
FS_SLEEP equ 0x1071D668
FS_GETMDDEVICEBYID equ 0x107187C4
FS_SDIO_DOREADWRITECOMMAND equ 0x10718A8C
FS_SDIO_DOCOMMAND equ 0x1071C958
FS_MMC_DEVICEINITSOMETHING equ 0x1071992C
FS_MMC_DEVICEINITSOMETHING2 equ 0x1071A4A4
FS_MMC_DEVICEINITSOMETHING3 equ 0x10719F60
FS_SVC_CREATEMUTEX equ 0x107F6BBC
FS_SVC_ACQUIREMUTEX equ 0x107F6BC4
FS_SVC_RELEASEMUTEX equ 0x107F6BCC
FS_SVC_DESTROYMUTEX equ 0x107F6BD4
FS_USB_READ equ 0x1077F1C0
FS_USB_WRITE equ 0x1077F35C
FS_SLC_READ1 equ 0x107B998C
FS_SLC_READ2 equ 0x107B98FC
FS_SLC_WRITE1 equ 0x107B9870
FS_SLC_WRITE2 equ 0x107B97E4
FS_SLC_HANDLEMESSAGE equ 0x107B9DE4
FS_MLC_READ1 equ 0x107DC760
FS_MLC_READ2 equ 0x107DCDE4
FS_MLC_WRITE1 equ 0x107DC0C0
FS_MLC_WRITE2 equ 0x107DC73C
FS_SDCARD_READ1 equ 0x107BDDD0
FS_SDCARD_WRITE1 equ 0x107BDD60
FS_ISFS_READWRITEBLOCKS equ 0x10720324
FS_CRYPTO_HMAC equ 0x107F3798
FS_RAW_READ1 equ 0x10732BC0
FS_REGISTERMDPHYSICALDEVICE equ 0x10718860

.include "ios_fs/ios_fs.syms"

; patches start here

.org 0x107F0B68
	bl syslogOutput_hook

; null out references to slcSomething1 and slcSomething2
; (nulling them out is apparently ok; more importantly, i'm not sure what they do and would rather get a crash than unwanted slc-writing)
.org 0x107B96B8
	.word 0x00000000
	.word 0x00000000

; in createDevThread
	.org 0x10700294
		b createDevThread_hook
; ; usb redirection
; 	.org FS_USB_READ
; 		b usbRead_patch
; 	.org FS_USB_WRITE
; 		b usbWrite_patch
; slc redirection
	.org FS_SLC_READ1
		b slcRead1_patch
	.org FS_SLC_READ2
		b slcRead2_patch
	.org FS_SLC_WRITE1
		b slcWrite1_patch
	.org FS_SLC_WRITE2
		b slcWrite2_patch
	.org 0x107206F0
		mov r0, #0 ; nop out hmac memcmp
; mlc redirection
	.org FS_SDCARD_READ1
		b sdcardRead_patch
	.org FS_SDCARD_WRITE1
		b sdcardWrite_patch
; FS_GETMDDEVICEBYID
	.org FS_GETMDDEVICEBYID + 0x8
		bl getMdDeviceById_hook
; call to FS_REGISTERMDPHYSICALDEVICE in mdMainThreadEntrypoint
	.org 0x107BD81C
		bl registerMdDevice_hook
; mdExit : patch out sdcard deinitialization
	.org 0x107BD374
		bx lr

; ; mlcRead1 logging
; .org FS_MLC_READ1 + 0x4
; 	bl mlcRead1_dbg
; .org 0x107DC7D8
; 	b mlcRead1_end_hook

; some selective logging function : enable all the logging !
.org 0x107F5720
	b FS_SYSLOG_OUTPUT

; our own custom codes starts here
.org _text_start
; attach our C code
	.incbin "ios_fs/ios_fs.text.bin"
.Close

.create "patched_sections/0x10835000.bin",0x10835000
; attach our C code bss
.org _bss_start
    .fill (_bss_end - _bss_start), 0x00
.Close
