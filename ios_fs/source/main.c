#include "text.h"
#include "sdio.h"
#include "mlcio.h"
#include "dumper.h"
#include "imports.h"
#include "fat32_format.h"

#define INITIALIZING_FLA        0x07
#define INITIALIZING_MMC        0x0D

void createDevThread_entry(int initialization_type)
{
    if(initialization_type == INITIALIZING_MMC)
    {
        sdcard_init();
        clearScreen(0x000000FF);
        _printf(20, 20, "welcome to redNAND %08X", *(vu32*)(0x050BD000 - 4));
    }

    if(initialization_type == INITIALIZING_FLA)
    {
        /*
        mlc_init();

        //FormatSDCard(0x100000);

        //slc_dump(0x0E, "slc     = %08X / 40000, read code %08X, write code %08X, retry %d", 2048);
        //slc_dump(0x0E, "slc2    = %08X / 40000, read code %08X, write code %08X, retry %d", 2048);
        mlc_dump(2048, MLC_32GB_SECTOR_COUNT);

        int i;
        for(i = 0; i < 5; i++)
            FS_SLEEP(1000);

        //! This is a cold reboot which always works. If not we have a problem ;-).
        FS_IOS_SHUTDOWN(1);
        */
    }
}
