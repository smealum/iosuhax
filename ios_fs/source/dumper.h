#ifndef _DUMPER_H_
#define _DUMPER_H_

//! debug dumps
void dump_syslog();
void dump_data(void* data_ptr, u32 size);
void dump_lots_data(u8* addr, u32 size);

void slc_dump(int deviceId, const char* format, u32 base_sectors);
void mlc_dump(u32 base_sector, u32 mlc_end);
void dumper_main();

#endif // _DUMPER_H_
