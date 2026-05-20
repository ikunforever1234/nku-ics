#include <x86.h>
#include <string.h>

#define PG_ALIGN __attribute((aligned(PGSIZE)))

static PDE kpdirs[NR_PDE] PG_ALIGN;
static PTE kptabs[PMEM_SIZE / PGSIZE] PG_ALIGN;
static void* (*palloc_f)();
static void (*pfree_f)(void*);

_Area segments[] = {      // Kernel memory mappings
  {.start = (void*)0,          .end = (void*)PMEM_SIZE}
};

#define NR_KSEG_MAP (sizeof(segments) / sizeof(segments[0]))

void _pte_init(void* (*palloc)(), void (*pfree)(void*)) {
  palloc_f = palloc;
  pfree_f = pfree;

  int i;

  // make all PDEs invalid
  for (i = 0; i < NR_PDE; i ++) {
    kpdirs[i] = 0;
  }

  PTE *ptab = kptabs;
  for (i = 0; i < NR_KSEG_MAP; i ++) {
    uint32_t pdir_idx = (uintptr_t)segments[i].start / (PGSIZE * NR_PTE);
    uint32_t pdir_idx_end = (uintptr_t)segments[i].end / (PGSIZE * NR_PTE);
    for (; pdir_idx < pdir_idx_end; pdir_idx ++) {
      // fill PDE
      kpdirs[pdir_idx] = (uintptr_t)ptab | PTE_P;

      // fill PTE
      PTE pte = PGADDR(pdir_idx, 0, 0) | PTE_P;
      PTE pte_end = PGADDR(pdir_idx + 1, 0, 0) | PTE_P;
      for (; pte < pte_end; pte += PGSIZE) {
        *ptab = pte;
        ptab ++;
      }
    }
  }

  set_cr3(kpdirs);
  set_cr0(get_cr0() | CR0_PG);
}

void _protect(_Protect *p) {
  PDE *updir = (PDE*)(palloc_f());
  p->ptr = updir;
  // map kernel space
  for (int i = 0; i < NR_PDE; i ++) {
    updir[i] = kpdirs[i];
  }

  p->area.start = (void*)0x8000000;
  p->area.end = (void*)0xc0000000;
}

void _release(_Protect *p) {
}

void _switch(_Protect *p) {
  set_cr3(p->ptr);
}

void _map(_Protect *p, void *va, void *pa) {
  uint32_t pdir_idx = PDX(va);
  uint32_t ptab_idx = PTX(va);

  PDE *pdir = (PDE *)p->ptr;
  if (!(pdir[pdir_idx] & PTE_P)) {
    PTE *ptab = (PTE *)palloc_f();
    memset(ptab, 0, PGSIZE);
    pdir[pdir_idx] = PTE_ADDR(ptab) | PTE_P | PTE_W | PTE_U;
  }

  PTE *ptab = (PTE *)PTE_ADDR(pdir[pdir_idx]);
  ptab[ptab_idx] = PTE_ADDR(pa) | PTE_P | PTE_W | PTE_U;
}

void _unmap(_Protect *p, void *va) {
}

_RegSet *_umake(_Protect *p, _Area ustack, _Area kstack, void *entry, char *const argv[], char *const envp[]) {
  (void)p;
  (void)kstack;
  size_t argc = 0;
  size_t envc = 0;

  if (argv != NULL) {
    while (argv[argc] != NULL) {
      argc++;
    }
  }
  if (envp != NULL) {
    while (envp[envc] != NULL) {
      envc++;
    }
  }

  uintptr_t argv_ptrs[argc + 1];
  uintptr_t envp_ptrs[envc + 1];
  char *sp = (char *)ustack.end;

  for (size_t i = envc; i > 0; i--) {
    size_t len = strlen(envp[i - 1]) + 1;
    sp -= len;
    memcpy(sp, envp[i - 1], len);
    envp_ptrs[i - 1] = (uintptr_t)sp;
  }
  envp_ptrs[envc] = 0;

  for (size_t i = argc; i > 0; i--) {
    size_t len = strlen(argv[i - 1]) + 1;
    sp -= len;
    memcpy(sp, argv[i - 1], len);
    argv_ptrs[i - 1] = (uintptr_t)sp;
  }
  argv_ptrs[argc] = 0;

  sp = (char *)((uintptr_t)sp & ~(sizeof(uintptr_t) - 1));
  uintptr_t *usp = (uintptr_t *)sp;

  *--usp = 0;
  for (size_t i = envc; i > 0; i--) {
    *--usp = envp_ptrs[i - 1];
  }
  *--usp = 0;
  for (size_t i = argc; i > 0; i--) {
    *--usp = argv_ptrs[i - 1];
  }
  *--usp = argc;
  *--usp = 0;

  _RegSet *tf = (_RegSet *)usp - 1;
  memset(tf, 0, sizeof(*tf));

  tf->eip = (uintptr_t)entry;
  tf->cs = 8;
  tf->eflags = 0x2;
  tf->esp = (uintptr_t)usp;
  tf->irq = 0;
  tf->error_code = 0;

  return tf;
}
