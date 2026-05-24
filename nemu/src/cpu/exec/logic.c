#include "cpu/exec.h"

make_EHelper(test) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(test);
}

make_EHelper(and) {
  rtl_and(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(and);
}

make_EHelper(xor) {
  rtl_xor(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(xor);
}

make_EHelper(or) {
  rtl_or(&t2, &id_dest->val, &id_src->val);
  operand_write(id_dest, &t2);

  rtl_update_ZFSF(&t2, id_dest->width);
  rtl_set_CF(&tzero);
  rtl_set_OF(&tzero);

  print_asm_template2(or);
}

make_EHelper(sar) {
  rtl_andi(&t0, &id_src->val, 0x1f);
  if (t0 == 0) {
    print_asm_template2(sar);
    return;
  }
  rtl_sar(&t2, &id_dest->val, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(sar);
}

make_EHelper(rol) {
  rtl_andi(&t0, &id_src->val, 0x1f);

  uint32_t bits = id_dest->width * 8;
  uint32_t n = t0 % bits;
  if (n == 0) {
    print_asm_template2(rol);
    return;
  }

  uint32_t mask = (bits == 32 ? 0xffffffffu : ((1u << bits) - 1));
  uint32_t val = id_dest->val & mask;
  uint32_t res = ((val << n) | (val >> (bits - n))) & mask;

  rtl_li(&t2, res);
  operand_write(id_dest, &t2);

  print_asm_template2(rol);
}

make_EHelper(shl) {
  rtl_andi(&t0, &id_src->val, 0x1f);
  if (t0 == 0) {
    print_asm_template2(shl);
    return;
  }
  rtl_shl(&t2, &id_dest->val, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shl);
}

make_EHelper(shr) {
  rtl_andi(&t0, &id_src->val, 0x1f);
  if (t0 == 0) {
    print_asm_template2(shr);
    return;
  }
  rtl_shr(&t2, &id_dest->val, &t0);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);
  // unnecessary to update CF and OF in NEMU

  print_asm_template2(shr);
}

make_EHelper(shrd) {
  rtl_andi(&t0, &id_src->val, 0x1f);
  if (t0 == 0) {
    print_asm_template2(shrd);
    return;
  }

  uint32_t bits = id_dest->width * 8;
  uint32_t mask = (bits == 32 ? 0xffffffffu : ((1u << bits) - 1));
  uint32_t dest = id_dest->val & mask;
  uint32_t src = id_src2->val & mask;
  uint32_t n = t0;
  uint64_t merged = ((uint64_t)src << bits) | dest;
  uint32_t res = (uint32_t)((merged >> n) & mask);

  rtl_li(&t2, res);
  operand_write(id_dest, &t2);
  rtl_update_ZFSF(&t2, id_dest->width);

  print_asm_template2(shrd);
}

make_EHelper(setcc) {
  uint8_t subcode = decoding.opcode & 0xf;
  rtl_setcc(&t2, subcode);
  operand_write(id_dest, &t2);

  print_asm("set%s %s", get_cc_name(subcode), id_dest->str);
}

make_EHelper(not) {
  rtl_mv(&t2, &id_dest->val);
  rtl_not(&t2);
  operand_write(id_dest, &t2);

  print_asm_template1(not);
}
