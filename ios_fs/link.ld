OUTPUT_ARCH(arm)

SECTIONS
{
    . = (0x10700000 + 0x000F8200);
	.text : ALIGN(0x100) {
	    _text_start = .;
		*(.text*);
		*(.rodata*);
	}

    . = (0x10835000 + 0x1406554);
	.bss : {
		_bss_start = .;
		*(.bss*);
		*(COMMON);
	}
	.io_buffer : ALIGN(0x40) {
		*(.io_buffer*);
	}
	_bss_end = .;

	/DISCARD/ : {
		*(*);
	}
}

