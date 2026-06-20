#include "fs.h"
#include "disk.h"
#include "vga.h"

/* ============================================
   CLAUDIMON FILE SYSTEM (CMFS) IMPLEMENTATION

   We keep an in-memory cache (fast to use from
   the shell) and write through to disk on every
   change, so data survives reboots.
   ============================================ */

static fs_file_t files[FS_MAX_FILES];
static fs_dir_t  dirs[FS_MAX_DIRS];
int fs_current_dir = 0;

/* ---- string helpers ---- */
static int fs_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}
static void fs_strcpy(char* dst, const char* src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

/* ============================================
   DISK SERIALISATION

   We pack directory/file metadata into sectors.
   Each fs_dir_t / fs_file_t fits inside a 512B
   sector with room to spare (simplicity > density).
   ============================================ */

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t dir_count;
    uint32_t file_count;
} fs_superblock_t;

static void save_superblock(void) {
    uint8_t sector[512];
    for (int i = 0; i < 512; i++) sector[i] = 0;
    fs_superblock_t* sb = (fs_superblock_t*)sector;
    sb->magic   = FS_MAGIC;
    sb->version = 1;
    sb->dir_count  = FS_MAX_DIRS;
    sb->file_count = FS_MAX_FILES;
    disk_write_sector(0, sector);
}

/* Each dir entry stored in its own sector (simple, wastes space, but easy) */
static void save_dir(int index) {
    uint8_t sector[512];
    for (int i = 0; i < 512; i++) sector[i] = 0;
    fs_dir_t* d = (fs_dir_t*)sector;
    *d = dirs[index];
    disk_write_sector(FS_DIR_TABLE_LBA + index, sector);
}

static void load_dir(int index) {
    uint8_t sector[512];
    disk_read_sector(FS_DIR_TABLE_LBA + index, sector);
    fs_dir_t* d = (fs_dir_t*)sector;
    dirs[index] = *d;
}

/* Each file entry stored in its own sector too */
static void save_file_meta(int index) {
    uint8_t sector[512];
    for (int i = 0; i < 512; i++) sector[i] = 0;
    fs_file_t* f = (fs_file_t*)sector;
    *f = files[index];
    disk_write_sector(FS_FILE_TABLE_LBA + index, sector);
}

static void load_file_meta(int index) {
    uint8_t sector[512];
    disk_read_sector(FS_FILE_TABLE_LBA + index, sector);
    fs_file_t* f = (fs_file_t*)sector;
    files[index] = *f;
}

/* Write file content (up to FS_MAX_FILE_SIZE bytes) across its sectors */
static void save_file_data(int index, const char* data, uint32_t size) {
    uint32_t lba = FS_DATA_START_LBA + (uint32_t)index * FS_SECTORS_PER_FILE;
    uint8_t sector[512];

    for (int s = 0; s < FS_SECTORS_PER_FILE; s++) {
        for (int i = 0; i < 512; i++) sector[i] = 0;
        uint32_t offset = s * 512;
        for (int i = 0; i < 512; i++) {
            if (offset + i < size) sector[i] = data[offset + i];
        }
        disk_write_sector(lba + s, sector);
    }
}

static void load_file_data(int index, char* out, uint32_t size) {
    uint32_t lba = FS_DATA_START_LBA + (uint32_t)index * FS_SECTORS_PER_FILE;
    uint8_t sector[512];

    for (int s = 0; s < FS_SECTORS_PER_FILE; s++) {
        disk_read_sector(lba + s, sector);
        uint32_t offset = s * 512;
        for (int i = 0; i < 512; i++) {
            if (offset + i < size) out[offset + i] = sector[i];
        }
    }
}

/* ============================================
   PUBLIC API
   ============================================ */

void fs_init(void) {
    for (int i = 0; i < FS_MAX_FILES; i++) files[i].used = 0;
    for (int i = 0; i < FS_MAX_DIRS;  i++) dirs[i].used  = 0;

    int disk_ok = (disk_init() == 0);

    if (disk_ok) {
        /* Check for existing filesystem on disk */
        uint8_t sector[512];
        disk_read_sector(0, sector);
        fs_superblock_t* sb = (fs_superblock_t*)sector;

        if (sb->magic == FS_MAGIC) {
            /* Filesystem already exists — load it! */
            for (int i = 0; i < FS_MAX_DIRS;  i++) load_dir(i);
            for (int i = 0; i < FS_MAX_FILES; i++) load_file_meta(i);
            fs_current_dir = 0;
            return;
        }
    }

    /* Fresh disk (or no disk) — create a brand-new filesystem */
    fs_strcpy(dirs[0].name, "/", FS_NAME_LEN);
    dirs[0].used   = 1;
    dirs[0].parent = 0;
    fs_current_dir = 0;

    fs_mkdir("docs");
    fs_mkdir("src");
    fs_create("readme.txt",
        "Welcome to Claudimon!\n"
        "Files now persist across reboots.\n"
        "Use 'help' to see all commands.\n");

    fs_current_dir = 0;

    if (disk_ok) {
        save_superblock();
        for (int i = 0; i < FS_MAX_DIRS;  i++) save_dir(i);
        /* files already saved by fs_create() above */
    }
}

int fs_mkdir(const char* name) {
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (dirs[i].used && dirs[i].parent == fs_current_dir
            && fs_strcmp(dirs[i].name, name) == 0) {
            terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
            terminal_write("mkdir: already exists: ");
            terminal_write(name); terminal_write("\n");
            return -1;
        }
    }
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (!dirs[i].used) {
            fs_strcpy(dirs[i].name, name, FS_NAME_LEN);
            dirs[i].used   = 1;
            dirs[i].parent = fs_current_dir;
            save_dir(i);
            terminal_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
            terminal_write("mkdir: created '"); terminal_write(name); terminal_write("'\n");
            return i;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("mkdir: no space left\n");
    return -1;
}

int fs_cd(const char* name) {
    if (fs_strcmp(name, "..") == 0) {
        if (fs_current_dir != 0) fs_current_dir = dirs[fs_current_dir].parent;
        return 0;
    }
    if (fs_strcmp(name, "/") == 0) { fs_current_dir = 0; return 0; }

    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (dirs[i].used && dirs[i].parent == fs_current_dir
            && fs_strcmp(dirs[i].name, name) == 0) {
            fs_current_dir = i;
            return 0;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("cd: no such directory: "); terminal_write(name); terminal_write("\n");
    return -1;
}

void fs_pwd(void) {
    int parts[FS_MAX_DIRS], depth = 0, cur = fs_current_dir;
    while (cur != 0) { parts[depth++] = cur; cur = dirs[cur].parent; }
    if (depth == 0) { terminal_write("/\n"); return; }
    for (int i = depth - 1; i >= 0; i--) {
        terminal_write("/"); terminal_write(dirs[parts[i]].name);
    }
    terminal_write("\n");
}

int fs_ls(void) {
    int found = 0;
    terminal_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
    for (int i = 0; i < FS_MAX_DIRS; i++) {
        if (dirs[i].used && dirs[i].parent == fs_current_dir && i != 0) {
            terminal_write("[DIR]  "); terminal_write(dirs[i].name); terminal_write("\n");
            found++;
        }
    }
    terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].dir_index == fs_current_dir) {
            terminal_write("[FILE] "); terminal_write(files[i].name); terminal_write("\n");
            found++;
        }
    }
    if (!found) {
        terminal_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        terminal_write("(empty directory)\n");
    }
    return 0;
}

int fs_create(const char* name, const char* data) {
    /* Overwrite if file already exists in this dir */
    int slot = -1;
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].dir_index == fs_current_dir
            && fs_strcmp(files[i].name, name) == 0) {
            slot = i;
            break;
        }
    }
    if (slot == -1) {
        for (int i = 0; i < FS_MAX_FILES; i++) {
            if (!files[i].used) { slot = i; break; }
        }
    }
    if (slot == -1) {
        terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        terminal_write("fs: no space for new file\n");
        return -1;
    }

    fs_strcpy(files[slot].name, name, FS_NAME_LEN);
    uint32_t len = 0;
    while (data[len] && len < FS_MAX_FILE_SIZE - 1) len++;
    files[slot].size      = len;
    files[slot].used      = 1;
    files[slot].dir_index = fs_current_dir;
    files[slot].data_lba  = FS_DATA_START_LBA + (uint32_t)slot * FS_SECTORS_PER_FILE;

    save_file_meta(slot);
    save_file_data(slot, data, len);

    return slot;
}

int fs_cat(const char* name) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].dir_index == fs_current_dir
            && fs_strcmp(files[i].name, name) == 0) {
            char data[FS_MAX_FILE_SIZE];
            load_file_data(i, data, files[i].size);
            data[files[i].size] = '\0';
            terminal_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            terminal_write(data);
            return 0;
        }
    }
    terminal_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    terminal_write("cat: file not found: "); terminal_write(name); terminal_write("\n");
    return -1;
}

int fs_load(const char* name, char* out, int max) {
    for (int i = 0; i < FS_MAX_FILES; i++) {
        if (files[i].used && files[i].dir_index == fs_current_dir
            && fs_strcmp(files[i].name, name) == 0) {
            int len = (int)files[i].size;
            if (len > max - 1) len = max - 1;
            load_file_data(i, out, len);
            out[len] = '\0';
            return len;
        }
    }
    return -1;
}
