#include "common.h"
#include "fs.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

uintptr_t loader(_Protect *as, const char *filename) {
  (void)as;
  if (filename == NULL) {
    filename = "/bin/hello";
  }

  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0);

  fs_read(fd, DEFAULT_ENTRY, fs_filesz(fd));
  fs_close(fd);
  return (uintptr_t)DEFAULT_ENTRY;
}
