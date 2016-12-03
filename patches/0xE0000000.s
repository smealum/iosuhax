.arm.big

.open "sections/0xE0000000.bin","patched_sections/0xE0000000.bin",0xE0000000

; allow custom bootLogoTex and bootMovie.h264
.org 0xE0030D68
	mov r0, #0
.org 0xE0030D34
	mov r0, #0

; allow any region title launch
.org 0xE0030498
	mov r0, #0

.Close
