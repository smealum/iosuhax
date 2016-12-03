#define _WIN32_WINNT    0x0601
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>

#define SETTINGS_FILE                       "./config.ini"
#define BYTES_PER_SECTOR                    512
#define BLOCKSIZE                           0x40000

#define SLC_BASE_SECTORS                    (0x000500)
#define SLCCMPT_BASE_SECTORS                (0x100500)
#define MLC_BASE_SECTORS                    (0x200500)
#define NAND_DUMP_SIGNATURE                 0x4841585844554d50ULL // HAXXDUMP

#define NAND_DESC_TYPE_SLC                  0x534c4320 // 'SLC '
#define NAND_DESC_TYPE_SLCCMPT              0x534c4332 // 'SLC2'
#define NAND_DESC_TYPE_MLC                  0x4d4c4320 // 'MLC '

#define be16(i)                             ((((uint16_t) ((i) & 0xFF)) << 8) | ((uint16_t) (((i) & 0xFF00) >> 8)))
#define be32(i)                             ((((uint32_t)be16((i) & 0xFFFF)) << 16) | ((uint32_t)be16(((i) & 0xFFFF0000) >> 16)))
#define be64(i)                             ((((uint64_t)be32((i) & 0xFFFFFFFFLL)) << 32) | ((uint64_t)be32(((i) & 0xFFFFFFFF00000000LL) >> 32)))

typedef struct _stdio_nand_desc_t
{
    uint32_t nand_type;                          // nand type
    uint32_t base_sector;                        // base sector of dump
    uint32_t sector_count;                       // sector count in SDIO sectors
} __attribute__((packed)) stdio_nand_desc_t;

typedef struct _sdio_nand_signature_sector_t
{
    uint64_t signature;              // HAXXDUMP
    stdio_nand_desc_t nand_descriptions[3];
} __attribute__((packed)) sdio_nand_signature_sector_t;

static HWND mainHwnd;               /* This is the handle for our window */
static HWND textBox1;
static HWND textBox2;
static HWND textBox3;
static HWND editBox1;
static HWND editBox2;
static HWND buttonDump;
static HWND buttonEject;
static HWND open1;
static HWND open2;
static HWND combobox;
static HWND combobox2;
static HANDLE hThread = INVALID_HANDLE_VALUE;
static int iRunning = 0;

int getSettingFromFile(const char *setting, char *value) {
    FILE *pFile = fopen(SETTINGS_FILE, "rb");
    if(!pFile) {
        return 0;
    }
    int iResult = 0;
    char line[1024];
    while(fgets(line, sizeof(line), pFile) != NULL) {
        if(strstr(line, setting) == NULL) {
            continue;
        }
        char *ptr = strchr(line, '\n');
        if(ptr != NULL) {
            *ptr = 0;
        }
        ptr = strchr(line, '\r');
        if(ptr != NULL) {
            *ptr = 0;
        }
        ptr = strchr(line, '=');
        if(ptr == NULL) {
            continue;
        }
        strcpy(value, ptr + 1);
        iResult = 1;
        break;
    }
    fclose(pFile);
    return iResult;
}

int saveSettingToFile(const char *setting, const char *value) {
    int found = 0;
    char line[1024];
    char *newFile = malloc(10240);
    memset(newFile, 0, 10240);

    FILE *pFile = fopen(SETTINGS_FILE, "rb");
    if(pFile) {
        while(fgets(line, sizeof(line), pFile) != NULL) {
            if(strstr(line, setting) == NULL) {
                strncat(newFile, line, 10239);
                continue;
            }
            found = 1;
            snprintf(line, sizeof(line), "%s=%s\r\n", setting, value);
            strncat(newFile, line, 10239);
        }
        fclose(pFile);
    }
    if(!found) {
        snprintf(line, sizeof(line), "%s=%s\r\n", setting, value);
        strncat(newFile, line, 10239);
    }

    pFile = fopen(SETTINGS_FILE, "wb");
    if(!pFile) {
        free(newFile);
        return 0;
    }
    fwrite(newFile, 1, strlen(newFile), pFile);
    fclose(pFile);
    free(newFile);
    return 1;
}

void print_output(const char *format, ...)
{
	char acBuffer[4096];
	va_list args;
	va_start (args, format);
	int iResult = vsnprintf(acBuffer, sizeof(acBuffer), format, args);
	if((iResult > 0)) {
        printf("%s", acBuffer);
        SetWindowText(textBox1, acBuffer);
	}
	va_end(args);
}

int resolvePhysicalPath(char *acFile, int dismount)
{
    if(acFile[strlen(acFile) - 1] != ':') {
        return 1;
    }
    char tempPath[strlen(acFile) + strlen("\\\\.\\") + 2];
    sprintf(tempPath, "\\\\.\\%s", acFile);

    HANDLE handler = CreateFile (tempPath, GENERIC_WRITE,
                    FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                    FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_RANDOM_ACCESS,
                    NULL);

    if(handler == INVALID_HANDLE_VALUE) {
        print_output("Can not open handle %s  error %i\n", tempPath, GetLastError());
        return 0;
    }

    DWORD lpBytesReturned;
    if(dismount && !DeviceIoControl(handler, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &lpBytesReturned, 0)) {
        print_output("Can not unmount %s  error %i\n", tempPath, GetLastError());
        CloseHandle(handler);
        return 0;
    }
    VOLUME_DISK_EXTENTS vExtents;
    if(!DeviceIoControl(handler, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &vExtents, sizeof(vExtents), &lpBytesReturned, 0)) {
        print_output("Can not get device name for %s! Error %i\n", tempPath, GetLastError());
        CloseHandle(handler);
        return 0;
    }
    if(vExtents.NumberOfDiskExtents > 0) {
        sprintf(acFile, "\\\\.\\PhysicalDrive%lu", vExtents.Extents[0].DiskNumber);
    }
    CloseHandle(handler);
    return 1;
}

void writeToSD(int nandType, int driveLetter, const char *acFromFilepath)
{
    uint64_t ullDone = 0;
    DWORD sRead = 1;
    DWORD writtenBytes = 0;
    HANDLE writeHandler;
    HANDLE readHandler;
    char toFile[100];
    snprintf(toFile, sizeof(toFile), "%c:", driveLetter + 'D');

    unsigned char *dataBuffer = (unsigned char *) malloc(BLOCKSIZE);
    if(!dataBuffer) {
        print_output("Not enough memory\n");
        return;
    }

    if(!resolvePhysicalPath(toFile, 1)) {
        return;
    }

    printf("Dumping %s -> %s\n", acFromFilepath, toFile);


    readHandler = CreateFile (toFile, GENERIC_READ,
                                FILE_SHARE_READ, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if(readHandler == INVALID_HANDLE_VALUE) {
        print_output("Failed to read NAND dump signature sector\n");
        free(dataBuffer);
        return;
    }

    LONG seek_low = BYTES_PER_SECTOR;
    LONG seek_high = 0;

    SetFilePointer(readHandler, seek_low, &seek_high, FILE_BEGIN);

    memset(dataBuffer, 0, sizeof(dataBuffer));
    if(ReadFile(readHandler, dataBuffer, BYTES_PER_SECTOR, &sRead, 0) == 0)
    {
        print_output("Failed to read NAND dump signature sector\n");
        CloseHandle(writeHandler);
        CloseHandle(readHandler);
        free(dataBuffer);
        return;
    }

    CloseHandle(readHandler);

    // open device now
    writeHandler = CreateFile (toFile, GENERIC_WRITE,
                                FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if(writeHandler == INVALID_HANDLE_VALUE) {
        print_output("Can not open path %s for write. Error %i\n", toFile, GetLastError());
        return;
    }

    readHandler = CreateFile (acFromFilepath, GENERIC_READ,
                                FILE_SHARE_READ, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if(readHandler == INVALID_HANDLE_VALUE) {
        print_output("Source file can not be opened %s! Error %i\n", acFromFilepath, GetLastError());
        CloseHandle(writeHandler);
        return;
    }

    uint64_t nand_offset = 0;
    sdio_nand_signature_sector_t sign_sector;
    memcpy(&sign_sector, dataBuffer, sizeof(sign_sector));

    if(be64(sign_sector.signature) != NAND_DUMP_SIGNATURE)
    {
        if(nandType == 0) {
            nand_offset = (uint64_t)SLC_BASE_SECTORS * (uint64_t)BYTES_PER_SECTOR;
        }
        else if(nandType == 1) {
            nand_offset = (uint64_t)SLCCMPT_BASE_SECTORS * (uint64_t)BYTES_PER_SECTOR;
        }
        else {
            nand_offset = (uint64_t)MLC_BASE_SECTORS * (uint64_t)BYTES_PER_SECTOR;
        }
    }
    else
    {
        int found = 0;
        int nand_idx = 0;
        while(be32(sign_sector.nand_descriptions[nand_idx].nand_type) != 0)
        {
            if((nandType == 0) && be32(sign_sector.nand_descriptions[nand_idx].nand_type) == NAND_DESC_TYPE_SLC)
            {
                nand_offset = (uint64_t)be32(sign_sector.nand_descriptions[nand_idx].base_sector) * (uint64_t)BYTES_PER_SECTOR;
                found = 1;
                break;
            }
            else if((nandType == 1) && be32(sign_sector.nand_descriptions[nand_idx].nand_type) == NAND_DESC_TYPE_SLCCMPT)
            {
                nand_offset = (uint64_t)be32(sign_sector.nand_descriptions[nand_idx].base_sector) * (uint64_t)BYTES_PER_SECTOR;
                found = 1;
                break;
            }
            else if((nandType == 2) && be32(sign_sector.nand_descriptions[nand_idx].nand_type) == NAND_DESC_TYPE_MLC)
            {
                nand_offset = (uint64_t)be32(sign_sector.nand_descriptions[nand_idx].base_sector) * (uint64_t)BYTES_PER_SECTOR;
                found = 1;
                break;
            }

            nand_idx++;
        }

        if(!found)
        {
            if(nandType == 0) {
                nand_offset = (uint64_t)SLC_BASE_SECTORS * (uint64_t)BYTES_PER_SECTOR;
            }
            else if(nandType == 1) {
                nand_offset = (uint64_t)SLCCMPT_BASE_SECTORS * (uint64_t)BYTES_PER_SECTOR;
            }
            else {
                nand_offset = (uint64_t)MLC_BASE_SECTORS * (uint64_t)BYTES_PER_SECTOR;
            }
        }
    }

    seek_low = nand_offset & 0xFFFFFFFF;
    seek_high = (nand_offset >> 32) & 0xFFFFFFFF;

    SetFilePointer(writeHandler, seek_low, &seek_high, FILE_BEGIN);

    int error = 0;
    sRead = 1;
    size_t tickStart = GetTickCount();
    while(sRead > 0) {
        memset(dataBuffer, 0xff, BLOCKSIZE);
        if(ReadFile(readHandler, dataBuffer, BLOCKSIZE, &sRead, 0) == 0) {
            print_output("%0.2f MB/s done: %llu MBs\n", (double)ullDone / (double)(GetTickCount() - tickStart) / 1024.0f, ullDone / (1024 * 1024));
            break;
        }
        if(WriteFile(writeHandler, dataBuffer, sRead, &writtenBytes, 0) == 0) {
            print_output("Write failure on  %u %llu %u\n", sRead, ullDone, GetLastError());
            error = 1;
            break;
        }
        ullDone += writtenBytes;
        print_output("%0.2f MB/s done: %llu MBs\r", (double)ullDone / (double)(GetTickCount() - tickStart) / 1024.0f, ullDone / (1024 * 1024));
        UpdateWindow(mainHwnd);
    }

    if(error == 0)
    {
        sign_sector.signature = be64(NAND_DUMP_SIGNATURE);
        if(nandType == 0)  {
            sign_sector.nand_descriptions[nandType].nand_type = be32(NAND_DESC_TYPE_SLC);
        }
        else if(nandType == 1)  {
            sign_sector.nand_descriptions[nandType].nand_type = be32(NAND_DESC_TYPE_SLCCMPT);
        }
        else  {
            sign_sector.nand_descriptions[nandType].nand_type = be32(NAND_DESC_TYPE_MLC);
        }
        sign_sector.nand_descriptions[nandType].base_sector = be32((uint32_t)(nand_offset / BYTES_PER_SECTOR));
        sign_sector.nand_descriptions[nandType].sector_count = be32((uint32_t)(ullDone / BYTES_PER_SECTOR));

        memset(dataBuffer, 0, BYTES_PER_SECTOR);
        memcpy(dataBuffer, &sign_sector, sizeof(sign_sector));

        seek_low = BYTES_PER_SECTOR;
        seek_high = 0;

        SetFilePointer(writeHandler, seek_low, &seek_high, FILE_BEGIN);
        WriteFile(writeHandler, dataBuffer, BYTES_PER_SECTOR, &writtenBytes, 0);
        print_output("Inject process finished\n");
    }

    CloseHandle(writeHandler);
    CloseHandle(readHandler);
    free(dataBuffer);
}

void writeToFile(int nandType, int driveLetter, const char *toFile)
{
    unsigned long long ullDone = 0;
    DWORD sRead = 1;
    DWORD writtenBytes = 0;
    HANDLE writeHandler;
    HANDLE readHandler;
    char acFromFilepath[100];
    snprintf(acFromFilepath, sizeof(acFromFilepath), "%c:", driveLetter + 'D');

    unsigned char *dataBuffer = (unsigned char *) malloc(BLOCKSIZE);
    if(!dataBuffer) {
        print_output("Not enough memory\n");
        return;
    }

    if(!resolvePhysicalPath(acFromFilepath, 0)) {
        return;
    }

    printf("Dumping %s -> %s\n", acFromFilepath, toFile);
    // open device now
    writeHandler = CreateFile (toFile, GENERIC_WRITE,
                                FILE_SHARE_WRITE, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if(writeHandler == INVALID_HANDLE_VALUE) {
        writeHandler = CreateFile (toFile, GENERIC_WRITE,
                                    FILE_SHARE_WRITE, NULL,
                                    CREATE_NEW,
                                    FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH | FILE_FLAG_RANDOM_ACCESS,
                                    NULL);
    }
    if(writeHandler == INVALID_HANDLE_VALUE) {
        print_output("Can not open path %s for write. Error %i\n", toFile, GetLastError());
        return;
    }

    readHandler = CreateFile (acFromFilepath, GENERIC_READ,
                                FILE_SHARE_READ, NULL,
                                OPEN_EXISTING,
                                FILE_FLAG_NO_BUFFERING | FILE_FLAG_RANDOM_ACCESS,
                                NULL);

    if(readHandler == INVALID_HANDLE_VALUE) {
        print_output("Source file can not be opened %s! Error %i\n", acFromFilepath, GetLastError());
        CloseHandle(writeHandler);
        return;
    }

    LONG seek_low = BYTES_PER_SECTOR;
    LONG seek_high = 0;

    SetFilePointer(readHandler, seek_low, &seek_high, FILE_BEGIN);

    if(ReadFile(readHandler, dataBuffer, BYTES_PER_SECTOR, &sRead, 0) == 0)
    {
        print_output("Failed to read NAND dump signature sector\n");
        CloseHandle(writeHandler);
        CloseHandle(readHandler);
        free(dataBuffer);
        return;
    }

    sdio_nand_signature_sector_t * sign_sector = (sdio_nand_signature_sector_t *)dataBuffer;

    if(be64(sign_sector->signature) != NAND_DUMP_SIGNATURE)
    {
        print_output("NAND dump signature is not matching\n");
        CloseHandle(writeHandler);
        CloseHandle(readHandler);
        free(dataBuffer);
        return;
    }

    uint64_t nand_offset = 0;
    uint64_t nand_size = 0;

    int found = 0;
    int nand_idx = 0;
    while(be32(sign_sector->nand_descriptions[nand_idx].nand_type) != 0)
    {
        if((nandType == 0) && be32(sign_sector->nand_descriptions[nand_idx].nand_type) == NAND_DESC_TYPE_SLC)
        {
            nand_offset = (uint64_t)be32(sign_sector->nand_descriptions[nand_idx].base_sector) * (uint64_t)BYTES_PER_SECTOR;
            nand_size = (uint64_t)be32(sign_sector->nand_descriptions[nand_idx].sector_count) * (uint64_t)BYTES_PER_SECTOR;
            found = 1;
            break;
        }
        else if((nandType == 1) && be32(sign_sector->nand_descriptions[nand_idx].nand_type) == NAND_DESC_TYPE_SLCCMPT)
        {
            nand_offset = (uint64_t)be32(sign_sector->nand_descriptions[nand_idx].base_sector) * (uint64_t)BYTES_PER_SECTOR;
            nand_size = (uint64_t)be32(sign_sector->nand_descriptions[nand_idx].sector_count) * (uint64_t)BYTES_PER_SECTOR;
            found = 1;
            break;
        }
        else if((nandType == 2) && be32(sign_sector->nand_descriptions[nand_idx].nand_type) == NAND_DESC_TYPE_MLC)
        {
            nand_offset = (uint64_t)be32(sign_sector->nand_descriptions[nand_idx].base_sector) * (uint64_t)BYTES_PER_SECTOR;
            nand_size = (uint64_t)be32(sign_sector->nand_descriptions[nand_idx].sector_count) * (uint64_t)BYTES_PER_SECTOR;
            found = 1;
            break;
        }

        nand_idx++;
    }

    if(!found)
    {
        printf("NAND type not found on SD card\n");
        CloseHandle(writeHandler);
        CloseHandle(readHandler);
        free(dataBuffer);
        return;
    }

    seek_low = nand_offset & 0xFFFFFFFF;
    seek_high = (nand_offset >> 32) & 0xFFFFFFFF;

    SetFilePointer(readHandler, seek_low, &seek_high, FILE_BEGIN);

    int error = 0;
    size_t tickStart = GetTickCount();
    while(ullDone < nand_size) {
        memset(dataBuffer, 0xff, BLOCKSIZE);
        if(ReadFile(readHandler, dataBuffer, BLOCKSIZE, &sRead, 0) == 0) {
            print_output("Read failure on  %u %llu %u\n", sRead, ullDone, GetLastError());
            error = 1;
            break;
        }
        if(WriteFile(writeHandler, dataBuffer, sRead, &writtenBytes, 0) == 0) {
            print_output("Write failure on  %u %llu %u\n", sRead, ullDone, GetLastError());
            error = 1;
            break;
        }
        ullDone += writtenBytes;
        print_output("%0.2f MB/s done: %llu MBs\r", (double)ullDone / (double)(GetTickCount() - tickStart) / 1024.0f, ullDone / (1024 * 1024));
        UpdateWindow(mainHwnd);
    }

    if(!error) {
        print_output("%0.2f MB/s done: %llu MBs\n", (double)ullDone / (double)(GetTickCount() - tickStart) / 1024.0f, ullDone / (1024 * 1024));
        print_output("Dump process finished\n");
    }
    CloseHandle(writeHandler);
    CloseHandle(readHandler);
    free(dataBuffer);
}

DWORD WINAPI threadedCopy( LPVOID lpParam )
{
    iRunning = 1;

    int process_type = (int)lpParam;

    const int iMaxPathLen = 4096;
    char *acToFilepath = (char*)malloc(iMaxPathLen);

    GetWindowText(editBox2, acToFilepath, iMaxPathLen);

    int nandType = SendMessage(combobox, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
    int driveLetter = SendMessage(combobox2, (UINT) CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);

    char driveLetterText[4];
    snprintf(driveLetterText, sizeof(driveLetterText), "%c", driveLetter + 'D');
    saveSettingToFile("Drive", driveLetterText);
    saveSettingToFile("OutPath", acToFilepath);

    if(process_type == 2)
    {
        writeToSD(nandType, driveLetter, acToFilepath);
    }
    else
    {
        writeToFile(nandType, driveLetter, acToFilepath);
    }

    free(acToFilepath);

    iRunning = 0;
    return 0;
}

/*  Declare Windows procedure  */
LRESULT CALLBACK WindowProcedure (HWND, UINT, WPARAM, LPARAM);

/*  Make the class name into a global variable  */
static char szClassName[ ] = "sdio nand extractor";

int WINAPI WinMain (HINSTANCE hThisInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR lpszArgument,
                     int nCmdShow)
{
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl;        /* Data structure for the windowclass */

    /* The Window structure */
    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;      /* This function is called by windows */
    wincl.style = CS_DBLCLKS;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default colour as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */
    if (!RegisterClassEx (&wincl))
        return 0;

    /* The class is registered, let's create the program*/
    mainHwnd = CreateWindowEx (
           0,                   /* Extended possibilites for variation */
           szClassName,         /* Classname */
           "sdio nand extractor v1.1 by Dimok",       /* Title Text */
           WS_OVERLAPPEDWINDOW, /* default window */
           CW_USEDEFAULT,       /* Windows decides the position */
           CW_USEDEFAULT,       /* where the window ends up on the screen */
           450,                 /* The programs width */
           150,                 /* and height in pixels */
           HWND_DESKTOP,        /* The window is a child-window to desktop */
           NULL,                /* No menu */
           hThisInstance,       /* Program Instance handler */
           NULL                 /* No Window Creation data */
           );


    /* Make the window visible on the screen */
    ShowWindow (mainHwnd, nCmdShow);

    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }

    if(hThread != INVALID_HANDLE_VALUE) {
        WaitForSingleObject(hThread, 500);
        CloseHandle(hThread);
        hThread = INVALID_HANDLE_VALUE;
    }

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}

static const TCHAR nandTypes[3][16] =
{
    TEXT("SLC"),
    TEXT("SLCCMPT"),
    TEXT("MLC")
};

/*  This function is called by the Windows function DispatchMessage()  */

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)                  /* handle the messages */
    {
        case WM_CREATE: {
            char driveLetter[50];
            char acToFilepath[1024];
            snprintf(acToFilepath, sizeof(acToFilepath), "C:\\slc.full.img");
            getSettingFromFile("OutPath", acToFilepath);
            getSettingFromFile("Drive", driveLetter);
            textBox2 = CreateWindow("STATIC", "Select a NAND type:", WS_VISIBLE | WS_CHILD, 10, 10, 130, 25, hwnd, 0, 0, 0);
            combobox = CreateWindow("COMBOBOX", "",  CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,  150, 10, 90, 100, hwnd, 0, 0, 0);
            textBox3 = CreateWindow("STATIC", "SD drive:", WS_VISIBLE | WS_CHILD, 315, 10, 60, 25, hwnd, 0, 0, 0);
            combobox2 = CreateWindow("COMBOBOX", "", CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE, 380, 10, 50, 500, hwnd, 0, 0, 0);

            editBox2 = CreateWindow("EDIT", acToFilepath, WS_VISIBLE | WS_CHILD, 10, 40, 390, 25, hwnd, 0, 0, 0);
            open2 = CreateWindow("BUTTON", "...", WS_VISIBLE | WS_CHILD | WS_BORDER, 405, 40, 25, 25, hwnd, (HMENU)3, 0, 0);
            buttonDump = CreateWindow("BUTTON", "Dump", WS_VISIBLE | WS_CHILD | WS_BORDER, 220, 70, 100, 25, hwnd, (HMENU)1, 0, 0);
            buttonEject = CreateWindow("BUTTON", "Inject", WS_VISIBLE | WS_CHILD | WS_BORDER, 330, 70, 100, 25, hwnd, (HMENU)2, 0, 0);
            textBox1 = CreateWindow("STATIC", "Speed: 0.00 MB/s", WS_VISIBLE | WS_CHILD, 10, 70, 200, 25, hwnd, 0, 0, 0);

            SendMessage(combobox,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM)nandTypes[0]);
            SendMessage(combobox,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM)nandTypes[1]);
            SendMessage(combobox,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM)nandTypes[2]);
            SendMessage(combobox, CB_SETCURSEL, (WPARAM)0, (LPARAM)0);

            int i;
            TCHAR letter[16];
            memset(letter, 0, sizeof(letter));
            for(i = 'D'; i <= 'Z'; i++)
            {
                letter[0] = i;
                letter[1] = ':';
                SendMessage(combobox2,(UINT) CB_ADDSTRING,(WPARAM) 0,(LPARAM)letter);
            }
            int selDrive = driveLetter[0];
            if(selDrive < 'D' || selDrive > 'Z')
                selDrive = 'D';

            SendMessage(combobox2, CB_SETCURSEL, (WPARAM)(selDrive - 'D'), (LPARAM)0);

            break;
        }
        case WM_COMMAND:
            if(LOWORD(wParam) == 1 || LOWORD(wParam) == 2) {
                if(iRunning) {
                    print_output("Wait for copy to finish\n");
                    break;
                }
                if(hThread != INVALID_HANDLE_VALUE) {
                    WaitForSingleObject(hThread, 500);
                    CloseHandle(hThread);
                    hThread = INVALID_HANDLE_VALUE;
                }
                DWORD   dwThreadId;
                hThread = CreateThread(NULL, 0, threadedCopy, (LPVOID)LOWORD(wParam), 1, &dwThreadId);
            }
            else if(LOWORD(wParam) == 3) {
                char cwdPath[1024];
                _getcwd(cwdPath, sizeof(cwdPath));
                OPENFILENAME ofn;
                TCHAR szFile[MAX_PATH];
                ZeroMemory(&ofn, sizeof(ofn));
                ofn.lStructSize = sizeof(ofn);
                ofn.lpstrFile = szFile;
                ofn.lpstrFile[0] = '\0';
                ofn.hwndOwner = hwnd;
                ofn.nMaxFile = sizeof(szFile);
                ofn.lpstrFilter = TEXT("All files(*.*)\0*.*\0");
                ofn.nFilterIndex = 1;
                ofn.lpstrInitialDir = NULL;
                ofn.lpstrFileTitle = NULL;
                ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
                if(GetOpenFileName(&ofn)) {
                    if(LOWORD(wParam) == 2) {
                        SetWindowText(editBox1, szFile);
                    }
                    else if(LOWORD(wParam) == 3) {
                        SetWindowText(editBox2, szFile);
                    }
                }
                _chdir(cwdPath);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage (0);       /* send a WM_QUIT to the message queue */
            break;
        default:                      /* for messages that we don't deal with */
            return DefWindowProc (hwnd, message, wParam, lParam);
    }

    return 0;
}
