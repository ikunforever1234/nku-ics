#include "cpu/exec.h"

void diff_test_skip_qemu();
void diff_test_skip_nemu();
extern void raise_intr(uint8_t NO, vaddr_t ret_addr);

make_EHelper(lidt) {
  rtlreg_t base = id_dest->addr;
  rtlreg_t limit = vaddr_read(base, 2);
  rtlreg_t ptr = vaddr_read(base + 2, 4);

  cpu.idtr.limit = limit;
  cpu.idtr.base = ptr;

  print_asm_template1(lidt);
}

make_EHelper(mov_r2cr) {
  switch (id_dest->reg) {
    case 0: cpu.cr0.val = id_src->val; break;
    case 3: cpu.cr3.val = id_src->val; break;
    default: panic("unsupported cr%d", id_dest->reg);
  }

  print_asm("movl %%%s,%%cr%d", reg_name(id_src->reg, 4), id_dest->reg);
}

make_EHelper(mov_cr2r) {
  switch (id_src->reg) {
    case 0: id_dest->val = cpu.cr0.val; break;
    case 3: id_dest->val = cpu.cr3.val; break;
    default: panic("unsupported cr%d", id_src->reg);
  }

  operand_write(id_dest, &id_dest->val);

  print_asm("movl %%cr%d,%%%s", id_src->reg, reg_name(id_dest->reg, 4));

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(int) {
  raise_intr(id_dest->val & 0xff, *eip);
  decoding.is_jmp = 1;
  decoding.jmp_eip = cpu.eip;

  print_asm("int %s", id_dest->str);

#ifdef DIFF_TEST
  diff_test_skip_nemu();
#endif
}

make_EHelper(iret) {
  rtl_pop(&t0);
  rtl_pop(&t1);
  rtl_pop(&t2);
  decoding.is_jmp = 1;
  decoding.jmp_eip = t0;
  cpu.eflags = t2;

  print_asm("iret");
}

uint32_t pio_read(ioaddr_t, int);
void pio_write(ioaddr_t, int, uint32_t);

make_EHelper(in) {
  rtl_li(&t0, pio_read(id_src->val & 0xffff, id_dest->width));
  operand_write(id_dest, &t0);

  print_asm_template2(in);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}

make_EHelper(out) {
  pio_write(id_dest->val & 0xffff, id_src->width, id_src->val);

  print_asm_template2(out);

#ifdef DIFF_TEST
  diff_test_skip_qemu();
#endif
}
