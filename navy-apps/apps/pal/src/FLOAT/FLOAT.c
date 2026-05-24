#include "FLOAT.h"
#include <stdint.h>
#include <assert.h>

FLOAT F_mul_F(FLOAT a, FLOAT b) {
  return (FLOAT)(((int64_t)a * b) >> 16);
}

FLOAT F_div_F(FLOAT a, FLOAT b) {
  assert(b != 0);

  uint64_t dividend = ((uint64_t)((int64_t)a < 0 ? -(int64_t)a : (int64_t)a)) << 16;
  uint64_t divisor = (uint64_t)((int64_t)b < 0 ? -(int64_t)b : (int64_t)b);
  uint64_t quotient = 0;
  uint64_t remainder = 0;

  for (int i = 63; i >= 0; i--) {
    remainder = (remainder << 1) | ((dividend >> i) & 1ULL);
    if (remainder >= divisor) {
      remainder -= divisor;
      quotient |= 1ULL << i;
    }
  }

  if ((a < 0) ^ (b < 0)) {
    return (FLOAT)(-(int64_t)quotient);
  }

  return (FLOAT)quotient;
}

FLOAT f2F(float a) {
  /* You should figure out how to convert `a' into FLOAT without
   * introducing x87 floating point instructions. Else you can
   * not run this code in NEMU before implementing x87 floating
   * point instructions, which is contrary to our expectation.
   *
   * Hint: The bit representation of `a' is already on the
   * stack. How do you retrieve it to another variable without
   * performing arithmetic operations on it directly?
   */

  union {
    float f;
    uint32_t u;
  } v = { .f = a };

  uint32_t sign = v.u >> 31;
  uint32_t exp = (v.u >> 23) & 0xff;
  uint32_t frac = v.u & 0x7fffff;

  if (exp == 0xff) {
    assert(0);
    return 0;
  }

  if (exp == 0) {
    return 0;
  }

  int64_t mant = (1LL << 23) | frac;
  int shift = (int)exp - 134;
  int64_t abs_value;

  if (shift >= 0) {
    abs_value = mant << shift;
  }
  else {
    abs_value = mant >> (-shift);
  }

  if (sign) {
    return (FLOAT)(-(int64_t)abs_value);
  }

  return (FLOAT)abs_value;
}

FLOAT Fabs(FLOAT a) {
  return a < 0 ? -a : a;
}

/* Functions below are already implemented */

FLOAT Fsqrt(FLOAT x) {
  FLOAT dt, t = int2F(2);

  do {
    dt = F_div_int((F_div_F(x, t) - t), 2);
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}

FLOAT Fpow(FLOAT x, FLOAT y) {
  /* we only compute x^0.333 */
  FLOAT t2, dt, t = int2F(2);

  do {
    t2 = F_mul_F(t, t);
    dt = (F_div_F(x, t2) - t) / 3;
    t += dt;
  } while(Fabs(dt) > f2F(1e-4));

  return t;
}
