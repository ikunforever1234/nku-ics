#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

#define KEYDOWN_MASK 0x8000

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  char event[32];
  int key = _read_key();

  if (key != _KEY_NONE) {
    const char *type = (key & KEYDOWN_MASK) ? "kd" : "ku";
    key &= ~KEYDOWN_MASK;
    sprintf(event, "%s %s\n", type, keyname[key]);
  }
  else {
    sprintf(event, "t %u\n", (unsigned)_uptime());
  }

  size_t event_len = strlen(event);
  if (len > event_len) {
    len = event_len;
  }
  memcpy(buf, event, len);
  return len;
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
  size_t pixels = len / sizeof(uint32_t);
  const uint32_t *p = buf;
  int x = (offset / sizeof(uint32_t)) % _screen.width;
  int y = (offset / sizeof(uint32_t)) / _screen.width;

  while (pixels > 0) {
    int w = _screen.width - x;
    if (w > (int)pixels) {
      w = (int)pixels;
    }

    _draw_rect(p, x, y, w, 1);
    p += w;
    pixels -= w;
    x = 0;
    y ++;
  }
}

void init_device() {
  _ioe_init();

  // TODO: print the string to array `dispinfo` with the format
  // described in the Navy-apps convention
  sprintf(dispinfo, "WIDTH : %d\nHEIGHT: %d\n", _screen.width, _screen.height);
  dispinfo_size = strlen(dispinfo);
}
