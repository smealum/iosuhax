//#include "text.h"

#define INITIALIZING_FLA        0x07
#define INITIALIZING_MMC        0x0D

# if 0

extern void sdcard_init(void);

void createDevThread_hookC(int initialization_type)
{
    if(initialization_type == INITIALIZING_MMC)
    {
        sdcard_init();
        clear_screen();
        _printf(20, 20, "welcome to redNAND %08X", *(vu32*)(0x050BD000 - 4));
    }

    if(initialization_type == INITIALIZING_FLA)
    {
        dumper_main();
    }
}
#endif
