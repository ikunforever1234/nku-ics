#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  return 0;
}

static char dispinfo[128] __attribute__((used));
size_t dispinfo_size;

void dispinfo_read(void *buf, off_t offset, size_t len) {
  if (offset >= (off_t)dispinfo_size) {
    return;
  }

  if (offset + len > dispinfo_size) {
    len = dispinfo_size - offset;
  }

  memcpy(buf, dispinfo + offset, len);

}

void fb_write(const void *buf, off_t offset, size_t len) {
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
}
