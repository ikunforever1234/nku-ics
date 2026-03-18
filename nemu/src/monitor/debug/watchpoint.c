#include "monitor/watchpoint.h"
#include "monitor/expr.h"
#include <string.h>

#define NR_WP 32

static WP wp_pool[NR_WP];
static WP *head, *free_;

void init_wp_pool() {
  int i;
  for (i = 0; i < NR_WP; i ++) {
    wp_pool[i].NO = i;
    wp_pool[i].next = &wp_pool[i + 1];
  }
  wp_pool[NR_WP - 1].next = NULL;

  head = NULL;
  free_ = wp_pool;
}

/* TODO: Implement the functionality of watchpoint */

WP* new_wp() {
  assert(free_ != NULL);

  WP *wp = free_;
  free_ = free_->next;

  wp->next = head;
  head = wp;

  wp->expr[0] = '\0';
  wp->last_val = 0;

  return wp;
}

void free_wp(WP *wp) {
  WP *prev = NULL;
  WP *cur = head;

  if (wp == NULL) {
    return;
  }

  while (cur != NULL && cur != wp) {
    prev = cur;
    cur = cur->next;
  }

  assert(cur != NULL);

  if (prev == NULL) {
    head = cur->next;
  }
  else {
    prev->next = cur->next;
  }

  cur->expr[0] = '\0';
  cur->last_val = 0;
  cur->next = free_;
  free_ = cur;
}

WP* add_watchpoint(const char *expr_str, bool *success) {
  assert(expr_str != NULL);

  WP *wp = new_wp();
  strncpy(wp->expr, expr_str, sizeof(wp->expr) - 1);
  wp->expr[sizeof(wp->expr) - 1] = '\0';

  wp->last_val = expr(wp->expr, success);
  if (!*success) {
    free_wp(wp);
    return NULL;
  }

  return wp;
}

bool delete_watchpoint(int no) {
  WP *cur = head;

  while (cur != NULL) {
    if (cur->NO == no) {
      free_wp(cur);
      return true;
    }
    cur = cur->next;
  }

  return false;
}

void list_watchpoints(void) {
  WP *cur = head;

  if (cur == NULL) {
    printf("No watchpoints.\n");
    return;
  }

  printf("Num\tWhat\n");
  while (cur != NULL) {
    printf("%d\t%s\n", cur->NO, cur->expr);
    cur = cur->next;
  }
}

bool check_watchpoints(void) {
  WP *cur = head;
  bool triggered = false;

  while (cur != NULL) {
    bool success = true;
    uint32_t new_val = expr(cur->expr, &success);
    if (!success) {
      printf("watchpoint %d evaluation failed: %s\n", cur->NO, cur->expr);
      return true;
    }

    if (new_val != cur->last_val) {
      printf("Watchpoint %d triggered: %s\n", cur->NO, cur->expr);
      printf("Old value = 0x%08x (%u)\n", cur->last_val, cur->last_val);
      printf("New value = 0x%08x (%u)\n", new_val, new_val);
      cur->last_val = new_val;
      triggered = true;
    }

    cur = cur->next;
  }

  return triggered;
}

