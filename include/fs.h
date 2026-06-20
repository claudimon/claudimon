#ifndef FS_H
#define FS_H

#include "stdint.h"

/* ============================================
   CLAUDIMON FILE SYSTEM (CMFS)

   A simple on-disk filesystem stored on the
   virtual hard disk via the ATA driver.

   Layout (in 512-byte sectors):
     Sector 0        : Superblock (magic number, version)
     Sector 1-64     : Directory table  (fixed-size entries)
     Sector 65-...   : File data blocks (1 file = N sectors)
   ============================================ */

#define FS_MAGIC         0xC1A0D1A0
#define FS_MAX_FILES     64
#define FS_MAX_DIRS      32
#define FS_NAME_LEN      32
#define FS_MAX_FILE_SIZE 4096          /* 8 sectors per file */
#define FS_SECTORS_PER_FILE (FS_MAX_FILE_SIZE / 512)

#define FS_DIR_TABLE_LBA   1
#define FS_FILE_TABLE_LBA  9            /* after dir table */
#define FS_DATA_START_LBA  73           /* after file table */

typedef struct {
    char     name[FS_NAME_LEN];
    uint32_t size;
    int      used;
    int      dir_index;
    uint32_t data_lba;   /* starting sector of file's data on disk */
} fs_file_t;

typedef struct {
    char name[FS_NAME_LEN];
    int  used;
    int  parent;
} fs_dir_t;

void fs_init(void);

int  fs_mkdir(const char* name);
int  fs_ls(void);
int  fs_cd(const char* name);
void fs_pwd(void);

int  fs_create(const char* name, const char* data);
int  fs_cat(const char* name);
int  fs_load(const char* name, char* out, int max);

extern int fs_current_dir;

#endif
