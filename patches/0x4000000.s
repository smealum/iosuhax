.arm.big

.open "sections/0x4000000.bin","patched_sections/0x4000000.bin",0x04000000

SECTION_BASE equ 0x04000000
SECTION_SIZE equ 0x00017020
CODE_BASE equ (SECTION_BASE + SECTION_SIZE)

; nop out memcmp hash checks
.org 0x040017E0
	mov r0, #0
.org 0x040019C4
	mov r0, #0
.org 0x04001BB0
	mov r0, #0
.org 0x04001D40
	mov r0, #0

.Close
