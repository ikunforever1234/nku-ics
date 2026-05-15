#include "fs.h"

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;  // 文件被打开之后的读写指针
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */
static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, size_t len);
extern void dispinfo_read(void *buf, off_t offset, size_t len);
extern void fb_write(const void *buf, off_t offset, size_t len);
extern size_t dispinfo_size;

void init_fs() {
  file_table[FD_FB].size = (size_t)_screen.width * _screen.height * sizeof(uint32_t);
  file_table[FD_DISPINFO].size = dispinfo_size;
}

int fs_open(const char *pathname, int flags, int mode) {
  (void)flags;
  (void)mode;

  for (int fd = 0; fd < (int)NR_FILES; fd ++) {
    if (strcmp(pathname, file_table[fd].name) == 0) {
      file_table[fd].open_offset = 0;
      return fd;
    }
  }

  return -1;
}

size_t fs_filesz(int fd) {
  if (fd < 0 || fd >= (int)NR_FILES) {
    return 0;
  }
  return file_table[fd].size;
}

size_t fs_read(int fd, void *buf, size_t len) {
  if (fd < 0 || fd >= (int)NR_FILES) {
    return 0;
  }

  Finfo *f = &file_table[fd];
  size_t remain = f->size > (size_t)f->open_offset ? f->size - (size_t)f->open_offset : 0;
  size_t nread = len < remain ? len : remain;

  if (fd == FD_EVENTS) {
    return events_read(buf, nread);
  }

  if (fd == FD_DISPINFO) {
    dispinfo_read(buf, f->open_offset, nread);
    f->open_offset += nread;
    return nread;
  }

  ramdisk_read(buf, f->disk_offset + f->open_offset, nread);
  f->open_offset += nread;
  return nread;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd < 0 || fd >= (int)NR_FILES) {
    return 0;
  }

  Finfo *f = &file_table[fd];

  if (fd == FD_STDIN || fd == FD_EVENTS) {
    return 0;
  }

  if (fd == FD_STDOUT || fd == FD_STDERR) {
    const char *ch = buf;
    for (size_t i = 0; i < len; i ++) {
      _putc(ch[i]);
    }
    f->open_offset += len;
    return len;
  }

  if (fd == FD_FB) {
    fb_write(buf, f->open_offset, len);
    f->open_offset += len;
    return len;
  }

  size_t remain = f->size > (size_t)f->open_offset ? f->size - (size_t)f->open_offset : 0;
  size_t nwrite = len < remain ? len : remain;
  ramdisk_write(buf, f->disk_offset + f->open_offset, nwrite);
  f->open_offset += nwrite;
  return nwrite;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  if (fd < 0 || fd >= (int)NR_FILES) {
    return -1;
  }

  if (fd == FD_STDIN || fd == FD_STDOUT || fd == FD_STDERR || fd == FD_EVENTS) {
    return -1;
  }

  Finfo *f = &file_table[fd];
  off_t new_offset;

  switch (whence) {
    case SEEK_SET: new_offset = offset; break;
    case SEEK_CUR: new_offset = f->open_offset + offset; break;
    case SEEK_END: new_offset = f->size + offset; break;
    default: return -1;
  }

  if (new_offset < 0) {
    return -1;
  }

  f->open_offset = new_offset;
  return new_offset;
}

int fs_close(int fd) {
  if (fd < 0 || fd >= (int)NR_FILES) {
    return -1;
  }

  file_table[fd].open_offset = 0;
  return 0;
}
