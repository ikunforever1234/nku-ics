#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    return mmio_read(addr, len, map_NO);
  }
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    mmio_write(addr, len, data, map_NO);
    return;
  }
  memcpy(guest_to_host(addr), &data, len);
}

static paddr_t page_translate(vaddr_t addr, bool is_write) {
  assert(cpu.cr0.protect_enable && cpu.cr0.paging);

  uint32_t pde_index = (addr >> 22) & 0x3ff;
  uint32_t pte_index = (addr >> 12) & 0x3ff;
  uint32_t offset = addr & PAGE_MASK;

  paddr_t pde_addr = (cpu.cr3.page_directory_base << 12) + pde_index * sizeof(PDE);
  PDE pde;
  pde.val = paddr_read(pde_addr, 4);
  assert(pde.present);
  if (!pde.accessed) {
    pde.accessed = 1;
    paddr_write(pde_addr, 4, pde.val);
  }

  paddr_t pte_addr = (pde.page_frame << 12) + pte_index * sizeof(PTE);
  PTE pte;
  pte.val = paddr_read(pte_addr, 4);
  assert(pte.present);
  if (!pte.accessed || (is_write && !pte.dirty)) {
    pte.accessed = 1;
    if (is_write) {
      pte.dirty = 1;
    }
    paddr_write(pte_addr, 4, pte.val);
  }

  return (pte.page_frame << 12) | offset;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if ((addr & PAGE_MASK) + len > PAGE_SIZE) {
    assert(0);
  }
  else {
    paddr_t paddr = addr;
    if (cpu.cr0.protect_enable && cpu.cr0.paging) {
      paddr = page_translate(addr, false);
    }
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if ((addr & PAGE_MASK) + len > PAGE_SIZE) {
    int low_len = PAGE_SIZE - (addr & PAGE_MASK);
    vaddr_write(addr, low_len, data & (~0u >> ((4 - low_len) << 3)));
    vaddr_write(addr + low_len, len - low_len, data >> (low_len * 8));
  }
  else {
    paddr_t paddr = addr;
    if (cpu.cr0.protect_enable && cpu.cr0.paging) {
      paddr = page_translate(addr, true);
    }
    paddr_write(paddr, len, data);
  }
}
