#include "nemu.h"

/* We use the POSIX regex functions to process regular expressions.
 * Type 'man regex' for more information about POSIX regex functions.
 */
#include <sys/types.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

enum {
  TK_NOTYPE = 256,
  TK_EQ,
  TK_NEQ,
  TK_AND,
  TK_OR,
  TK_NOT,
  TK_NEG,         //一元负号
  TK_DEREF,       //一元*解引用
  TK_DEC,
  TK_HEX,
  TK_REG

  /* TODO: Add more token types */

};

static struct rule {
  char *regex;
  int token_type;
} rules[] = {

  /* TODO: Add more rules.
   * Pay attention to the precedence level of different rules.
   */

  {" +", TK_NOTYPE},                 // spaces
  {"==", TK_EQ},                     // equal
  {"!=", TK_NEQ},                    // not equal
  {"&&", TK_AND},                    // and
  {"\\|\\|", TK_OR},                 // or
  {"!", TK_NOT},                     // not
  {"\\+", '+'},                      // plus
  {"-", '-'},                        // minus
  {"\\*", '*'},                      // multiply
  {"/", '/'},                        // divide
  {"\\(", '('},                      // left parenthesis
  {"\\)", ')'},                      // right parenthesis
  {"0[xX][0-9a-fA-F]+", TK_HEX},     // hex number
  {"[0-9]+", TK_DEC},                // decimal number
  {"\\$[a-zA-Z][a-zA-Z0-9]*", TK_REG} // register
};

#define NR_REGEX (sizeof(rules) / sizeof(rules[0]) )

static regex_t re[NR_REGEX];

/* Rules are used for many times.
 * Therefore we compile them only once before any usage.
 */
void init_regex() {
  int i;
  char error_msg[128];
  int ret;

  for (i = 0; i < NR_REGEX; i ++) {
    ret = regcomp(&re[i], rules[i].regex, REG_EXTENDED);
    if (ret != 0) {
      regerror(ret, &re[i], error_msg, 128);
      panic("regex compilation failed: %s\n%s", error_msg, rules[i].regex);
    }
  }
}

typedef struct token {
  int type;
  char str[32];
} Token;

Token tokens[32];
int nr_token;

static bool make_token(char *e) {
  int position = 0;
  int i;
  regmatch_t pmatch;

  nr_token = 0;

  while (e[position] != '\0') {
    /* Try all rules one by one. */
    for (i = 0; i < NR_REGEX; i ++) {
      if (regexec(&re[i], e + position, 1, &pmatch, 0) == 0 && pmatch.rm_so == 0) {
        char *substr_start = e + position;
        int substr_len = pmatch.rm_eo;

        Log("match rules[%d] = \"%s\" at position %d with len %d: %.*s",
            i, rules[i].regex, position, substr_len, substr_len, substr_start);
        position += substr_len;

        /* TODO: Now a new token is recognized with rules[i]. Add codes
         * to record the token in the array `tokens'. For certain types
         * of tokens, some extra actions should be performed.
         */

        switch (rules[i].token_type) {
          case TK_NOTYPE:
            break;

          case TK_DEC:
          case TK_HEX:
          case TK_REG:
            if (nr_token >= (int)(sizeof(tokens) / sizeof(tokens[0]))) {
              printf("too many tokens\n");
              return false;
            }
            if (substr_len >= (int)sizeof(tokens[nr_token].str)) {
              printf("token is too long: %.*s\n", substr_len, substr_start);
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            strncpy(tokens[nr_token].str, substr_start, substr_len);
            tokens[nr_token].str[substr_len] = '\0';
            nr_token++;
            break;

          default:
            if (nr_token >= (int)(sizeof(tokens) / sizeof(tokens[0]))) {
              printf("too many tokens\n");
              return false;
            }
            tokens[nr_token].type = rules[i].token_type;
            tokens[nr_token].str[0] = '\0';
            nr_token++;
            break;
        }

        break;
      }
    }

    if (i == NR_REGEX) {
      printf("no match at position %d\n%s\n%*.s^\n", position, e, position, "");
      return false;
    }
  }

  for (i = 0; i < nr_token; i ++) {
    //对于每一个-，若在表达式开头，或前一个 token 不是“操作数/右括号”，改写为 TK_NEG
    if (tokens[i].type == '-') {
      if (i == 0 ||
          !(tokens[i - 1].type == TK_DEC || tokens[i - 1].type == TK_HEX ||
            tokens[i - 1].type == TK_REG || tokens[i - 1].type == ')')) {
        tokens[i].type = TK_NEG;
      }
    }
    //对每个 *，若在表达式开头，或前一个 token 不是“操作数/右括号”，改写为 TK_DEREF
    else if (tokens[i].type == '*') {
      if (i == 0 ||
          !(tokens[i - 1].type == TK_DEC || tokens[i - 1].type == TK_HEX ||
            tokens[i - 1].type == TK_REG || tokens[i - 1].type == ')')) {
        tokens[i].type = TK_DEREF;
      }
    }
  }

  return true;
}

static bool get_reg_val(const char *s, uint32_t *val) {
  int i;
  if (s == NULL || s[0] != '$') {
    return false;
  }

  const char *name = s + 1;
  if (strcmp(name, "eip") == 0) {
    *val = cpu.eip;
    return true;
  }

  for (i = 0; i < 8; i ++) {
    if (strcmp(name, regsl[i]) == 0) {
      *val = reg_l(i);
      return true;
    }
  }

  for (i = 0; i < 8; i ++) {
    if (strcmp(name, regsw[i]) == 0) {
      *val = reg_w(i);
      return true;
    }
  }

  for (i = 0; i < 8; i ++) {
    if (strcmp(name, regsb[i]) == 0) {
      *val = reg_b(i);
      return true;
    }
  }

  return false;
}

static bool check_parentheses(int p, int q) {
  int i;
  int level = 0;

  if (tokens[p].type != '(' || tokens[q].type != ')') {
    return false;
  }

  for (i = p; i <= q; i ++) {
    if (tokens[i].type == '(') {
      level ++;
    }
    else if (tokens[i].type == ')') {
      level --;
      if (level == 0 && i < q) {
        return false;
      }
      if (level < 0) {
        return false;
      }
    }
  }

  return level == 0;
}

static int precedence(int type) {
  switch (type) {
    case TK_OR: return 1;
    case TK_AND: return 2;
    case TK_EQ:
    case TK_NEQ: return 3;
    case '+':
    case '-': return 4;
    case '*':
    case '/': return 5;
    default: return 100;
  }
}

static uint32_t eval(int p, int q, bool *success) {
  int i;
  int op = -1;
  int op_prec = 100;
  int level = 0;

  if (p > q) {
    *success = false;
    return 0;
  }

  if (p == q) {
    if (tokens[p].type == TK_DEC) {
      return (uint32_t)strtoul(tokens[p].str, NULL, 10);
    }
    else if (tokens[p].type == TK_HEX) {
      return (uint32_t)strtoul(tokens[p].str, NULL, 16);
    }
    else if (tokens[p].type == TK_REG) {
      uint32_t val = 0;
      if (!get_reg_val(tokens[p].str, &val)) {
        *success = false;
        return 0;
      }
      return val;
    }

    *success = false;
    return 0;
  }

  if (check_parentheses(p, q)) {
    return eval(p + 1, q - 1, success);
  }

  for (i = p; i <= q; i ++) {
    if (tokens[i].type == '(') {
      level ++;
      continue;
    }
    if (tokens[i].type == ')') {
      level --;
      if (level < 0) {
        *success = false;
        return 0;
      }
      continue;
    }

    if (level == 0) {
      int cur_prec = precedence(tokens[i].type);
      if (cur_prec <= 5 && cur_prec <= op_prec) {
        op_prec = cur_prec;
        op = i;
      }
    }
  }

  if (level != 0) {
    *success = false;
    return 0;
  }

  if (op == -1) {
    if (tokens[p].type == TK_NEG) {
      uint32_t val = eval(p + 1, q, success);
      if (!*success) {
        return 0;
      }
      return (uint32_t)(-(int32_t)val);
    }
    else if (tokens[p].type == TK_NOT) {
      uint32_t val = eval(p + 1, q, success);
      if (!*success) {
        return 0;
      }
      return !val;
    }
    else if (tokens[p].type == TK_DEREF) {
      uint32_t addr = eval(p + 1, q, success);
      if (!*success) {
        return 0;
      }
      return vaddr_read(addr, 4);
    }

    *success = false;
    return 0;
  }

  uint32_t val1 = eval(p, op - 1, success);
  if (!*success) {
    return 0;
  }
  uint32_t val2 = eval(op + 1, q, success);
  if (!*success) {
    return 0;
  }

  switch (tokens[op].type) {
    case '+': return val1 + val2;
    case '-': return val1 - val2;
    case '*': return val1 * val2;
    case '/':
      if (val2 == 0) {
        *success = false;
        return 0;
      }
      return val1 / val2;
    case TK_EQ: return val1 == val2;
    case TK_NEQ: return val1 != val2;
    case TK_AND: return val1 && val2;
    case TK_OR: return val1 || val2;
    default:
      *success = false;
      return 0;
  }
}

uint32_t expr(char *e, bool *success) {
  uint32_t result;

  // printf("[expr] input: %s\n", e);

  if (!make_token(e)) {
    *success = false;
    return 0;
  }

  // printf("[expr] tokens (%d):\n", nr_token);
  // for (int i = 0; i < nr_token; i ++) {
  //   printf("  [%d] type=%d str='%s'\n", i, tokens[i].type, tokens[i].str);
  // }

  if (nr_token == 0) {
    *success = false;
    return 0;
  }

  *success = true;
  result = eval(0, nr_token - 1, success);
  // printf("[expr] success=%d result=0x%08x (%u)\n", *success, result, result);
  return result;
}
