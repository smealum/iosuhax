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
sdcard_init:
	; this should run *after* /dev/mmc thread is created
	push {lr}

	; first we create our synchronization stuff
	mov r0, #1
	mov r1, #1
	bl FS_SVC_CREATEMUTEX
	ldr r1, =sdcard_access_mutex
	str r0, [r1]

	ldr r1, =dumpdata_offset
	mov r0, #0
	str r0, [r1]

	; then we sleep until /dev/mmc is done initializing sdcard (TODO : better synchronization here)
	mov r0, #1000
	bl FS_SLEEP

	; finally we set some flags to indicate sdcard is ready for use
	ldr r0, =FS_MMC_SDCARD_STRUCT
	ldr r1, [r0, #0x24]
	orr r1, #0x20
	str r1, [r0, #0x24]
	ldr r1, [r0, #0x28]
	bic r1, #0x4
	str r1, [r0, #0x28]

	pop {pc}

mlc_init:
	; this should run *after* /dev/mmc thread is created (and after sdcard_init)
	; this should also only be run when you want to dump mlc; this will cause the physical mlc device to be inaccessible to regular FS code
	push {lr}

	; finally we set some flags to indicate sdcard is ready for use
	ldr r0, =FS_MMC_MLC_STRUCT
	ldr r1, [r0, #0x24]
	orr r1, #0x20
	str r1, [r0, #0x24]
	ldr r1, [r0, #0x28]
	bic r1, #0x4
	str r1, [r0, #0x28]

	pop {pc}

; r0 : bool read (0 = read, not 0 = write), r1 : data_ptr, r2 : cnt, r3 : block_size, sparg0 : offset_blocks, sparg1 : out_callback_arg2, sparg2 : device_id
sdcard_readwrite:
	sdcard_readwrite_stackframe_size equ (4*4+12*4)
	push {r4,r5,r6,lr}
	sub sp, #12*4

	; pointer for command paramstruct
	add r4, sp, #0xC
	; pointer for mutex (sp + 0x8 will be callback's arg2)
	add r5, sp, #0x4
	; offset_blocks
	ldr r6, [sp, #sdcard_readwrite_stackframe_size]

	; first of all, grab sdcard mutex
	push {r0,r1,r2,r3}
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	; also create a mutex for synchronization with end of operation...
	mov r0, #1
	mov r1, #1
	bl FS_SVC_CREATEMUTEX
	str r0, [r5]
	; ...and acquire it
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	pop {r0,r1,r2,r3}

	; block_size needs to be equal to sector_size (0x200)
	sdcard_readwrite_block_size_adjust:
		cmp r3, #0x200
		movgt r3, r3, lsr 1 ; block_size >>= 1;
		movgt r2, r2, lsl 1 ; cnt <<= 1;
		movgt r6, r6, lsl 1 ; offset_blocks <<= 1;
		bgt sdcard_readwrite_block_size_adjust

	; build rw command paramstruct 
	str r2, [r4, #0x00] ; cnt
	str r3, [r4, #0x04] ; block_size
	cmp r0, #0
	movne r0, #0x3 ; read operation
	str r0, [r4, #0x08] ; command type
	str r1, [r4, #0x0C] ; data_ptr
	mov r0, #0
	str r0, [r4, #0x10] ; offset_high
	str r6, [r4, #0x14] ; offset_low
	str r0, [r4, #0x18] ; callback
	str r0, [r4, #0x1C] ; callback_arg
	mvn r0, #0
	str r0, [r4, #0x20] ; -1

	; setup parameters
	ldr r0, [sp, #sdcard_readwrite_stackframe_size+0x8] ; device_identifier : sdcard (real one is 0x43, but patch makes 0xDA valid)
	mov r1, r4 ; paramstruct
	mov r2, r6 ; offset_blocks
	adr r3, sdcard_readwrite_callback ; callback
	str r5, [sp] ; callback_arg (mutex ptr)

	; call readwrite function
	bl FS_SDIO_DOREADWRITECOMMAND
	mov r4, r0
	cmp r0, #0
	bne sdcard_readwrite_skip_wait

	; wait for callback to give the go-ahead
	ldr r0, [r5]
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	ldr r0, [r5, #0x4]
	ldr r1, [sp, #sdcard_readwrite_stackframe_size+0x4]
	cmp r1, #0
	strne r0, [r1]
	sdcard_readwrite_skip_wait:

	; finally, release sdcard mutexes
	ldr r0, [r5]
	bl FS_SVC_DESTROYMUTEX
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	bl FS_SVC_RELEASEMUTEX

	; return
	mov r0, r4
	add sp, #12*4
	pop {r4,r5,r6,pc}

	; release mutex to let everyone know we're done
	sdcard_readwrite_callback:
		str r1, [r0, #4]
		ldr r0, [r0]
		b FS_SVC_RELEASEMUTEX

createDevThread_hook:
	push {r0}
	; check if we were initializing /dev/mmc
	ldr r0, [r4, #0x8]
	cmp r0, #0xD
	bne createDevThread_hook_skip1

	bl sdcard_init

	bl clear_screen

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

getMdDeviceById_hook:
	mov r4, r0
	cmp r0, #0xDA ; magic id (sdcard)
	ldreq r0, =FS_MMC_SDCARD_STRUCT
	popeq {r4,r5,pc}
	cmp r0, #0xAB ; magic id (mlc)
	ldreq r0, =FS_MMC_MLC_STRUCT
	popeq {r4,r5,pc}
	bx lr ; return if different

registerMdDevice_hook:
	push {r4,lr}

	cmp r0, #0
	beq registerMdDevice_hook_skip

	ldr r3, [r0, #0x8] ; mmc device struct ptr
	ldr r4, =FS_MMC_SDCARD_STRUCT
	cmp r3, r4
	bne registerMdDevice_hook_skip

	; sdcard ! fix stuff up so that registration can happen ok

	push {r0-r3}
	
	; first lock that mutex
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	mov r1, #0
	bl FS_SVC_ACQUIREMUTEX
	
	; then, clear the flag we set earlier (that FS_REGISTERMDPHYSICALDEVICE will set back anyway)
	ldr r0, =FS_MMC_SDCARD_STRUCT
	ldr r1, [r0, #0x24]
	bic r1, #0x20
	str r1, [r0, #0x24]

	pop {r0-r3}

	; register it !
	bl FS_REGISTERMDPHYSICALDEVICE
	mov r4, r0

	; finally, release the mutex
	ldr r0, =sdcard_access_mutex
	ldr r0, [r0]
	bl FS_SVC_RELEASEMUTEX

	mov r0, r4
	pop {r4,pc}

	registerMdDevice_hook_skip:
	; not sdcard
	bl FS_REGISTERMDPHYSICALDEVICE
	pop {r4,pc}

; read1(void *physical_device_info, int offset_high, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
; readWriteCallback_patch(bool read, int offset_offset, int offset_low, int cnt, int block_size, void *data_outptr, void *callback, int callback_parameter)
readWriteCallback_patch:
	readWriteCallback_patch_stackframe_size equ (7*4)
	push {r0,r1,r2,r3,r4,r5,lr}
	mov r5, #0xDA
	str r5, [sp, #0x8] ; device id (sdcard)
	add r5, sp, #0xC ; out_callback_arg2 dst
	add r2, r1
	str r2, [sp] ; offset
	str r5, [sp, #4] ; out_callback_arg2
	ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0x4] ; data_ptr
	mov r2, r3 ; cnt
	ldr r3, [sp, #readWriteCallback_patch_stackframe_size] ; block_size
	bl sdcard_readwrite
	mov r4, r0
	cmp r0, #0
	bne readWriteCallback_patch_skip_callback

	ldr r12, [sp, #readWriteCallback_patch_stackframe_size+0x8] ; callback
	ldr r0, [r5]
	ldr r1, [sp, #readWriteCallback_patch_stackframe_size+0xC] ; callback_parameter
	cmp r12, #0
	blxne r12

	readWriteCallback_patch_skip_callback:
	mov r0, r4
	add sp, #4
	pop {r1,r2,r3,r4,r5,pc}

; ; ; ; ; ; ; ; ; ;
; USB REDIRECTION ;
; ; ; ; ; ; ; ; ; ;

usbReadWrite_patch:
	ldr r1, =USB_BASE_SECTORS ; offset_offset
	b readWriteCallback_patch

usbRead_patch:
	mov r0, #1 ; read
	b usbReadWrite_patch

usbWrite_patch:
	mov r0, #0 ; write
	b usbReadWrite_patch

; ; ; ; ; ; ; ; ; ; ;
; SDIO REDIRECTION  ;
; ; ; ; ; ; ; ; ; ; ;

sdcardReadWrite_patch:
	push {r2}
	ldr r2, [r0, #0x14]
	mov r0, r1
	cmp r2, #6 ; DEVICETYPE_SDCARD
	ldrne r1, =MLC_BASE_SECTORS ; mlc
	ldreq r1, =0x00000000 ; sdcard
	pop {r2}
	b readWriteCallback_patch

sdcardRead_patch:
	mov r1, #1 ; read
	b sdcardReadWrite_patch

sdcardWrite_patch:
	mov r1, #0 ; write
	b sdcardReadWrite_patch

; ; ; ; ; ; ; ; ; ;
; SLC REDIRECTION ;
; ; ; ; ; ; ; ; ; ;

slcReadWrite_patch:
	push {r2}
	ldr r2, [r0, #4]
	mov r0, r1
	cmp r2, #0
	ldrne r1, =((SLC_BASE_SECTORS * 0x200) / 0x800)
	ldreq r1, =((SLCCMPT_BASE_SECTORS * 0x200) / 0x800)
	pop {r2}
	b readWriteCallback_patch

slcRead1_patch:
	mov r1, #1 ; read
	b slcReadWrite_patch

slcWrite1_patch:
	mov r1, #0 ; write
	b slcReadWrite_patch

slcRead2_patch:
	slcRead2_patch_stackframe_size equ (0x10+7*4)
	push {r0-r5,lr}
	sub sp, #0x10
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x00]
	str r4, [sp, #0x0] ; block_size
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x08]
	str r4, [sp, #0x4] ; data_outptr
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x10]
	str r4, [sp, #0x8] ; callback
	ldr r4, [sp, #slcRead2_patch_stackframe_size+0x14]
	str r4, [sp, #0xC] ; callback_parameter
	bl slcRead1_patch
	add sp, #0x14
	pop {r1-r5,pc}

slcWrite2_patch:
	slcWrite2_patch_stackframe_size equ (0x10+6*4)
	push {r0-r4,lr}
	sub sp, #0x10
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x00]
	str r4, [sp, #0x0] ; block_size
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x08]
	str r4, [sp, #0x4] ; data_outptr
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x10]
	str r4, [sp, #0x8] ; callback
	ldr r4, [sp, #slcWrite2_patch_stackframe_size+0x14]
	str r4, [sp, #0xC] ; callback_parameter
	bl slcWrite1_patch
	add sp, #0x14
	pop {r1-r4,pc}

; ; ; ; ; ; ; ; ; ;
;   DEBUG STUFF   ;
; ; ; ; ; ; ; ; ; ;

mlcRead1_dbg:
	mlcRead1_dbg_stackframe equ (4*6)
	mov r12, r0
	push {r0-r3,r12,lr}
	adr r0, mlcRead1_dbg_format
	ldr r1, [sp, #mlcRead1_dbg_stackframe+9*4]
	bl FS_SYSLOG_OUTPUT
	pop {r0-r3,lr,pc} ; replaces mov lr, r0
	mlcRead1_dbg_format:
		.ascii "mlcRead1 : %08X %08X %08X"
		.byte 0x0a
		.byte 0x00
		.align 0x4

mlcRead1_end_hook:
	mlcRead1_end_hook_stackframe equ (4*10)
	push {r0}
	mov r0, #50
	bl FS_SLEEP
	ldr r0, =sdcard_read_buffer
	ldr r1, [sp, #mlcRead1_end_hook_stackframe+4*1]
	mov r2, #0x200
	bl FS_MEMCPY
	ldr r0, =sdcard_read_buffer
	str r6, [r0]
	mov r1, #0x200
	bl dump_data
	pop {r0,r4-r11,pc}

; r0 : data ptr
; r1 : size
; r2 : offset_blocks
write_data_offset:
	push {r1,r2,r3,r4,lr}
	mov r3, r1, lsr 9 ; size /= 0x200
	cmp r3, #0
	moveq r3, #1
	mov r1, r0 ; data_ptr
	str r2, [sp] ; offset
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

dump_syslog:
	push {r1,r2,r3,lr}
	ldr r0, =syslog_buffer
	ldr r1, =0x05095ECC ; data_ptr (syslog ptr)
	ldr r1, [r1]
	mov r2, #0x40000
	bl FS_MEMCPY
	ldr r0, =SYSLOG_BASE_SECTORS ; offset_sectors
	str r0, [sp]
	mov r0, #0
	str r0, [sp, #0x4] ; out_callback_arg2
	mov r0, #0xDA
	str r0, [sp, #0x8] ; device id
	mov r0, #0 ; write
	ldr r1, =syslog_buffer
	mov r2, #0x200 ; num_sectors (0x40000 bytes)
	mov r3, #0x200 ; block_size
	bl sdcard_readwrite
	add sp, #0xC
	pop {pc}

syslogOutput_hook:
	push {r0}
	; bl dump_syslog
	pop {r0,r4-r8,r10,pc}

; r0 : device id
getPhysicalDeviceHandle:
	ldr r1, =0x1091C2EC
	mov r2, #0x204
	mla r1, r2, r0, r1
	ldrh r1, [r1, #6]
	orr r0, r1, r0, lsl 16
	bx lr

; r0 : dst, r1 : offset, r2 : sectors, r3 : device id
; rawRead1PhysicalDevice_(int physical_device_handle, unsigned int offset_high, unsigned int offset_low, unsigned int size, void *outptr, int (__fastcall *callback)(unsigned int, int), int callback_arg)
readSlc:
	push {lr}
	sub sp, #0xC
	str r0, [sp] ; outptr
	mov r0, #0
	str r0, [sp, #4] ; callback
	str r0, [sp, #8] ; callback_arg
	push {r1-r3}
	mov r0, r3
	bl getPhysicalDeviceHandle
	pop {r1-r3}
	mov r3, r2 ; cnt
	mov r2, r1 ; offset_low
	mov r1, #0 ; offset_high
	BL FS_RAW_READ1
	add sp, #0xC
	pop {pc}

slc_dump:
	push {r4-r7,lr}
	mov r4, #0
	mov r5, r0
	mov r6, r1
	mov r7, r2

	mov r0, #1000
	bl FS_SLEEP

	slc_dump_loop:
		mov r3, r4
		mov r0, #20
		mov r1, #0
		mov r2, r6
		bl _printf

		ldr r0, =sdcard_read_buffer
		mov r1, r4
		mov r2, #0x80
		add r4, r2
		mov r3, r5
		bl readSlc

		mov r0, #10
		bl FS_SLEEP

		ldr r0, =sdcard_read_buffer
		mov r1, #0x40000
		mov r2, r7
		add r7, r7, r1, lsr 9
		bl write_data_offset

		cmp r4, #0x40000
		blt slc_dump_loop

	pop {r4-r7,pc}

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

mlc_dump:
	push {r4,r7,lr}
	sub sp, #8
	mov r4, #0
	ldr r7, =MLC_BASE_SECTORS

	mlc_dump_loop:
		bl dump_syslog
		mov r5, #0
		mlc_dump_loop2:
			mov r3, r4
			mov r0, #20
			mov r1, #0
			adr r2, mlc_format
			bl _printf

			ldr r0, =sdcard_read_buffer
			mov r1, #0x800
			mov r2, r4
			add r4, r1
			bl read_mlc
			str r0, [sp]

			ldr r0, =sdcard_read_buffer
			mov r1, #0x100000
			mov r2, r7
			add r7, r7, r1, lsr 9
			bl write_data_offset
			str r0, [sp, #4]

			add r5, #1
			cmp r5, #0x40
			blt mlc_dump_loop2

		ldr r0, =0x3A20000
		cmp r4, r0
		blt mlc_dump_loop

	add sp, #8
	pop {r4,r7,pc}

; r0 : data ptr
; r1 : num_sectors
; r2 : offset_sectors
read_mlc:
	push {r1,r2,r3,r4,lr}
	str r2, [sp]
	mov r2, r1 ; num_sectors
	mov r1, r0 ; data_ptr
	ldr r0, =mlc_out_callback_arg2
	str r0, [sp, #0x4] ; out_callback_arg2
	mov r0, #0xAB
	str r0, [sp, #0x8] ; device id
	mov r0, #1 ; read
	mov r3, #0x200 ; block_size
	bl sdcard_readwrite
	add sp, #0xC
	pop {r4,pc}

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

clear_screen:
	push {lr}
	ldr r0, =FRAMEBUFFER_ADDRESS ; data_ptr
	ldr r1, =0x00
	ldr r2, =FRAMEBUFFER_STRIDE*504
	bl FS_MEMSET
	pop {pc}

; r0 : x, r1 : y, r2 : format, ...
; NOT threadsafe so dont even try you idiot
_printf:
	ldr r12, =_printf_xylr
	str r0, [r12]
	str r1, [r12, #4]
	str lr, [r12, #8]
	ldr r0, =_printf_string
	mov r1, #_printf_string_end-_printf_string
	bl FS_SNPRINTF
	ldr r12, =_printf_xylr
	ldr r1, [r12]
	ldr r2, [r12, #4]
	ldr lr, [r12, #8]
	push {lr}
	ldr r0, =_printf_string
	bl drawString
	pop {pc}


; r0 : str, r1 : x, r2 : y
drawString:
	push {r4-r6,lr}
	mov r4, r0
	mov r5, r1
	mov r6, r2
	drawString_loop:
		ldrb r0, [r4], #1
		cmp r0, #0x00
		beq drawString_end
		mov r1, r5
		mov r2, r6
		bl drawCharacter
		add r5, #CHARACTER_SIZE
		b drawString_loop
	drawString_end:
	pop {r4-r6,pc}

; r0 : char, r1 : x, r2 : y
drawCharacter:
	subs r0, #32
	; bxlt lr
	push {r4-r7,lr}
	ldr r4, =FRAMEBUFFER_ADDRESS ; r4 : framebuffer address
	add r4, r1, lsl 2 ; add x * 4
	mov r3, #FRAMEBUFFER_STRIDE
	mla r4, r2, r3, r4
	adr r5, font_data ; r5 : character data
	add r5, r0, lsl 3 ; font is 1bpp, 8x8 => 8 bytes represents one character
	mov r1, #0xFFFFFFFF ; color
	mov r2, #0x0 ; empty color
	mov r6, #8 ; i
	drawCharacter_loop1:
		mov r3, #CHARACTER_MULT
		drawCharacter_loop3:
			mov r7, #8 ; j
			ldrb r0, [r5]
			drawCharacter_loop2:
				tst r0, #1
				; as many STRs as CHARACTER_MULT (would be nice to do this in preproc...)
				streq r1, [r4], #4
				streq r1, [r4], #4
				strne r2, [r4], #4
				strne r2, [r4], #4
				mov r0, r0, lsr #1
				subs r7, #1
				bne drawCharacter_loop2
			add r4, #FRAMEBUFFER_STRIDE-CHARACTER_SIZE*4
			subs r3, #1
			bne drawCharacter_loop3
		add r5, #1
		subs r6, #1
		bne drawCharacter_loop1
	pop {r4-r7,pc}

.pool

font_data:
	.incbin "patches/font.bin"

.Close

.create "patched_sections/0x10835000.bin",0x10835000

.org FS_BSS_START
	sdcard_access_mutex:
		.word 0x00000000
	dumpdata_offset:
		.word 0x00000000
	mlc_out_callback_arg2:
		.word 0x00000000
	_printf_xylr:
		.word 0x00000000
		.word 0x00000000
		.word 0x00000000
	_printf_string:
		.fill ((_printf_string + 0x100) - .), 0x00
	_printf_string_end:
	.align 0x40
	syslog_buffer:
		.fill ((syslog_buffer + 0x40000) - .), 0x00
	.align 0x40
	sdcard_read_buffer:
		.fill ((sdcard_read_buffer + 0x100000) - .), 0x00

.Close
