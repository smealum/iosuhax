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
.org CODE_BASE

createDevThread_hook:
	push {r0}
	; check if we were initializing /dev/mmc
	ldr r0, [r4, #0x8]
	cmp r0, #0xD
	bne createDevThread_hook_skip1

	bl sdcard_init

    mov r0, 0xFF
	bl clearScreen

	mov r0, #20
	mov r1, #20
	adr r2, createDevThread_hook_welcome
	ldr r3, =0x050BD000 - 4
	ldr r3, [r3]
	bl _printf

	createDevThread_hook_skip1:

	; check if we were initializing /dev/fla
	ldr r0, [r4, #0x8]
	cmp r0, #0x7
	bne createDevThread_hook_skip2

	; b dumper_main

	createDevThread_hook_skip2:
	pop {r0,r4-r8,pc}
	createDevThread_hook_welcome:
	.ascii "welcome to redNAND %08X"
	.byte 0x00
	.align 0x4

; ; ; ; ; ; ; ; ; ;
;   DEBUG STUFF   ;
; ; ; ; ; ; ; ; ; ;
;mlcRead1_dbg:
;	mlcRead1_dbg_stackframe equ (4*6)
;	mov r12, r0
;	push {r0-r3,r12,lr}
;	adr r0, mlcRead1_dbg_format
;	ldr r1, [sp, #mlcRead1_dbg_stackframe+9*4]
;	bl FS_SYSLOG_OUTPUT
;	pop {r0-r3,lr,pc} ; replaces mov lr, r0
;	mlcRead1_dbg_format:
;		.ascii "mlcRead1 : %08X %08X %08X"
;		.byte 0x0a
;		.byte 0x00
;		.align 0x4
;
;mlcRead1_end_hook:
;	mlcRead1_end_hook_stackframe equ (4*10)
;	push {r0}
;	mov r0, #50
;	bl FS_SLEEP
;	ldr r0, =sdcard_read_buffer
;	ldr r1, [sp, #mlcRead1_end_hook_stackframe+4*1]
;	mov r2, #0x200
;	bl FS_MEMCPY
;	ldr r0, =sdcard_read_buffer
;	str r6, [r0]
;	mov r1, #0x200
;	bl dump_data
;	pop {r0,r4-r11,pc}

; r0 : data ptr
; r1 : size
dump_data:
	push {r1,r2,r3,r4,lr}
	mov r3, r1, lsr 9 ; size /= 0x200
	cmp r3, #0
	moveq r3, #1
	mov r1, r0 ; data_ptr
	ldr r0, =DUMPDATA_BASE_SECTORS ; offset_sectors
	ldr r4, =dumpdata_offset
	ldr r2, [r4]
	add r0, r2
	str r0, [sp]
	add r2, r3
	str r2, [r4]
	mov r0, #0
	str r0, [sp, #0x4] ; out_callback_arg2
	mov r0, #0xDA
	str r0, [sp, #0x8] ; device id
	mov r0, #0 ; write
	mov r2, r3 ; num_sectors
	mov r3, #0x200 ; block_size
	bl sdcard_readwrite
	add sp, #0xC
	pop {r4,pc}

dump_lots_data:
	push {r4-r6,lr}
	mov r4, r0 ; addr
	mov r5, r1 ; size
	dump_lots_data_loop:
		mov r6, #0x40000 ; cur_size
		cmp r6, r5
		movgt r6, r5
		ldr r0, =sdcard_read_buffer
		mov r1, r4
		mov r2, r6
		bl FS_MEMCPY
		ldr r0, =sdcard_read_buffer ; data_ptr
		mov r1, r6 ; size
		bl dump_data
		add r4, r6
		sub r5, r6
		cmp r6, #0
		bne dump_lots_data_loop
	pop {r4-r6,pc}

syslogOutput_hook:
	push {r0}
	; bl dump_syslog
	pop {r0,r4-r8,r10,pc}


	retval_format:
	.ascii "retval = %08X"
	.byte 0x00
	.align 4

	slc_format:
	.ascii "slc     = %08X"
	.byte 0x00
	.align 4

	slc2_format:
	.ascii "slc2    = %08X"
	.byte 0x00
	.align 4

	slccmpt_format:
	.ascii "slccmpt = %08X"
	.byte 0x00
	.align 4

	mlc_format:
	.ascii "mlc     = %08X %08X %08X"
	.byte 0x00
	.align 4

dumper_main:
	push {lr}
	bl mlc_init
	mov r0, #0xE ; slc
	adr r1, slc_format
	ldr r2, =SLC_BASE_SECTORS
	bl slc_dump
	mov r0, #0xE ; slc
	adr r1, slc2_format
	ldr r2, =SLC_BASE_SECTORS
	bl slc_dump
	mov r0, #0xF ; slccmpt
	adr r1, slccmpt_format
	ldr r2, =SLCCMPT_BASE_SECTORS
	bl slc_dump
	bl mlc_dump
	; intentional crash
	mov r0, #0
	ldr r0, [r0]
	; pop {pc}

.pool
.pool

; attach our C code
.org 0x107F9200
	.incbin "ios_fs/ios_fs.text.bin"
	.align 0x100

.Close



.create "patched_sections/0x10835000.bin",0x10835000

.org FS_BSS_START
	sdcard_access_mutex:
		.word 0x00000000
	dumpdata_offset:
		.word 0x00000000
	mlc_out_callback_arg2:
		.word 0x00000000
	.align 0x40
	sdcard_read_buffer:
		.fill ((sdcard_read_buffer + 0x100000) - .), 0x00

; attach our C code bss and data
.org 0x11C3C554
	.incbin "ios_fs/ios_fs.data.bin"
	.align 0x04

.Close
