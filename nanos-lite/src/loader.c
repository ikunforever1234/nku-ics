#include "common.h"
#include "fs.h"
#include "memory.h"

#define DEFAULT_ENTRY ((void *)0x8048000)

uintptr_t loader(_Protect *as, const char *filename) {
  if (filename == NULL) {
    filename = "/bin/hello";
  }

  int fd = fs_open(filename, 0, 0);
  assert(fd >= 0);

  size_t size = fs_filesz(fd);
  uintptr_t entry = (uintptr_t)DEFAULT_ENTRY;
  uintptr_t start = PGROUNDDOWN(entry);
  uintptr_t end = PGROUNDUP(entry + size);

  for (uintptr_t va = start; va < end; va += PGSIZE) {
    void *pa = new_page();
    memset(pa, 0, PGSIZE);
    _map(as, (void *)va, pa);

    size_t offset = va < entry ? entry - va : 0;
    size_t nread = PGSIZE - offset;
    if (nread > size) {
      nread = size;
    }

    if (nread > 0) {
      fs_read(fd, (uint8_t *)pa + offset, nread);
      size -= nread;
    }
  }

  fs_close(fd);
  return entry;
}
