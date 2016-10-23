OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = (0x10700000 + 0x000F8200 + 0x1000), LENGTH = (0x10800000 - (0x10700000 + 0x000F8200 + 0x1000))
	RAMRW (rw!i) : ORIGIN = (0x10835000 + 0x1406554 + 0x1000), LENGTH = (0x11F00000 - (0x10835000 + 0x1406554 + 0x1000))
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init)
		*(.text)
		*(.rodata)
	}

	.bss : {
		_bss_start = .;
		*(.bss);
	}
	_bss_end = .;
}

