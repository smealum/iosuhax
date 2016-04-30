OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = 0x05116000, LENGTH = 0x00006000
	RAMRW (rw!i) : ORIGIN = 0x050BE000, LENGTH = 0x00002000
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

