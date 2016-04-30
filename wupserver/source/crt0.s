.section ".init"
.arm
.align 4

.extern _main
.type _main, %function

.extern memset
.type memset, %function

_start:

	@ initialize bss
	ldr r0, =_bss_start
	mov r1, #0x00
	ldr r2, =_bss_end
	sub r2, r0
	bl memset

	b _main
