#include "text.h"
#include "sdio.h"
#include "mlcio.h"
#include "dumper.h"
#include "imports.h"
#include "devices.h"
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
        FormatSDCard(MLC_BASE_SECTORS + MLC_32GB_SECTOR_COUNT);
        dump_nand_complete();
    }
}
