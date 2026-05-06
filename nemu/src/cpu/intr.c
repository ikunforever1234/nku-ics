#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr) {
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  rtlreg_t idt_base = cpu.idtr.base;
  rtlreg_t desc_lo = vaddr_read(idt_base + NO * 8, 4);
  rtlreg_t desc_hi = vaddr_read(idt_base + NO * 8 + 4, 4);
  vaddr_t target = (desc_hi & 0xffff0000) | (desc_lo & 0xffff);

  rtl_push(&cpu.eflags);
  rtl_li(&t0, 0x8);
  rtl_push(&t0);
  rtl_li(&t0, ret_addr);
  rtl_push(&t0);

  cpu.eip = target;
}

void dev_raise_intr() {
}
