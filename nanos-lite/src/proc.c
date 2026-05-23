#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;
PCB *current_game = NULL;
static int game_budget = 0;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  // _switch(&pcb[i].as);
  // current = &pcb[i];
  // ((void (*)(void))entry)();

  
  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

void switch_game(void) {
  if (current_game == NULL) {
    current_game = &pcb[0];
    return;
  }

  current_game = (current_game == &pcb[0] ? &pcb[2] : &pcb[0]);
  game_budget = 0;
}


// _RegSet* schedule(_RegSet *prev) {
//   if (current != NULL) {
//     current->tf = prev;
//   }
//   // current = (current == &pcb[0] ? &pcb[1] : &pcb[0]);

//   if (current == NULL) {
//     current = &pcb[0];
//     pal_budget = 0;
//   }
//   else if (current == &pcb[0]) {
//     if (nr_proc > 1 && pal_budget + 1 >= 10) {
//       current = &pcb[1];
//       pal_budget = 0;
//     }
//     else {
//       current = &pcb[0];
//       pal_budget ++;
//     }
//   }
//   else {
//     current = &pcb[0];
//   }

//   _switch(&current->as);
//   return current->tf;
// }


_RegSet* schedule(_RegSet *prev) {
  if (current != NULL) {
    current->tf = prev;
  }

  // if (current_game == NULL) {
  //   current_game = &pcb[0];
  // }

  if (current == NULL) {
    current = current_game;
  }
  else if (current == current_game) {
    if (nr_proc > 1 && game_budget + 1 >= 10) {
      current = &pcb[1];
      game_budget = 0;
    }
    else {
      current = current_game;
      game_budget ++;
    }
  }
  else if (current == &pcb[1]) {
    current = current_game;
  }
  else {
    current = current_game;
  }

  _switch(&current->as);
  return current->tf;
}
