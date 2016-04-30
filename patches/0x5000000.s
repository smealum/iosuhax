.arm.big

.open "sections/0x5000000.bin","patched_sections/0x5000000.bin",0x05000000

SECTION_BASE equ 0x05000000
SECTION_SIZE equ 0x000598f0
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)
MCP_BSS_START equ (0x5074000 + 0x48574)

MCP_FSA_OPEN_T equ 0x05059160
MCP_FSA_MOUNT_T equ 0x05059530
MCP_SYSLOG_OUTPUT_T equ 0x05059140

MCP_SVC_CREATETHREAD equ 0x050567EC
MCP_SVC_STARTTHREAD equ 0x05056824

NEW_TIMEOUT equ (0xFFFFFFFF) ; over an hour

; fix 10 minute timeout that crashes MCP after 10 minutes of booting
.org 0x05022474
	.word NEW_TIMEOUT

; hook main thread to start our thread ASAP
.org 0x05056718
	.arm
	bl mcpMainThread_hook

; patch OS launch sig check
.org 0x0500A818
	.thumb
	mov r0, #0
	mov r0, #0

; patch IOSC_VerifyPubkeySign to always succeed
.org 0x05052C44
	.arm
	mov r0, #0
	bx lr

.org 0x050282AE
	.thumb
	bl launch_os_hook

; patch pointer to fw.img loader path
.org 0x050284D8
	.word fw_img_path

.org CODE_BASE
	.arm
	mcpMainThread_hook:
		mov r11, r0
		push {r0-r11,lr}
		sub sp, #8
		
		mov r0, #0x78
		str r0, [sp] ; prio
		mov r0, #1
		str r0, [sp, #4] ; detached
		ldr r0, =wupserver_entrypoint ; thread entrypoint
		mov r1, #0 ; thread arg
		ldr r2, =wupserver_stacktop ; thread stacktop
		mov r3, #wupserver_stacktop - wupserver_stack ; thread stack size
		bl MCP_SVC_CREATETHREAD

		cmp r0, #0
		blge MCP_SVC_STARTTHREAD

		ldr r1, =0x050BD000 - 4
		str r0, [r1]

		add sp, #8
		pop {r0-r11,pc}

	.thumb
	launch_os_hook:
		bx pc
		.align 0x4
		.arm
		push {r0-r3,lr}
		sub sp, #8

		bl MCP_SYSLOG_OUTPUT_T
		
		mov r0, #0
		bl MCP_FSA_OPEN_T

		ldr r1, =launch_os_hook_devicepath
		ldr r2, =launch_os_hook_mountpath
		mov r3, #0
		str r3, [sp]
		str r3, [sp, #4]
		bl MCP_FSA_MOUNT_T

		add sp, #8
		pop {r0-r3,pc}
		launch_os_hook_devicepath:
			.ascii "/dev/sdcard01"
			.byte 0x00
		launch_os_hook_mountpath:
			.ascii "/vol/sdcard"
			.byte 0x00
			.align 0x4

	fw_img_path:
		.ascii "/vol/sdcard"
		.byte 0x00
		.align 0x4

	.pool

.Close

.open "sections/0x5100000.bin","patched_sections/0x5100000.bin",0x05100000

; append wupserver code
.org 0x5116000
	wupserver_entrypoint:
		.incbin "wupserver/wupserver.bin"
	.align 0x100

.Close

.create "patched_sections/0x5074000.bin",0x5074000

.org MCP_BSS_START

.org 0x050BD000
	wupserver_stack:
		.fill ((wupserver_stack + 0x1000) - .), 0x00
	wupserver_stacktop:
	wupserver_bss:
		.fill ((wupserver_bss + 0x2000) - .), 0x00

.Close
