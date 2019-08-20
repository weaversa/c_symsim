#include <pcode_definitions.h>

void less(machine_state *ms) {
  fprintf(stderr, "\n\nLESS\n\n");

  Vector *a = cMemory_load_le(ms, 0x4, 1);
  Vector *b = cMemory_load_le(ms, 0x8, 1);
  Vector *c = pINT_LESS(ms, a, b);
  cMemory_store_le(ms, 0x0, c, 1);
  vec_release(ms, a);
  vec_release(ms, b);
  vec_release(ms, c);
}

void mult(machine_state *ms) {
  fprintf(stderr, "\n\nMULT\n\n");

  Vector *a = cMemory_load_le(ms, 0x4, 1);
  Vector *b = cMemory_load_le(ms, 0x8, 1);
  Vector *c = pINT_MULT(ms, a, b);
  cMemory_store_le(ms, 0x0, c, 1);
  vec_release(ms, a);
  vec_release(ms, b);
  vec_release(ms, c);
}

void branch1(machine_state *ms) {
  Vector *choice = cMemory_load_le(ms, 0x1, 1);
  Gia_Lit_t choice_lit = vec_equal(ms, choice, ms->vec_zero_byte);
  vec_release(ms, choice);
  conditional_branch(ms, choice_lit, mult, pCUT, less, pCUT, 0);
}

void branch2(machine_state *ms) {
  Vector *choice = cMemory_load_le(ms, 0x1, 1);
  Gia_Lit_t choice_lit = vec_equal(ms, choice, ms->vec_zero_byte);
  vec_release(ms, choice);
  conditional_branch(ms, choice_lit, less, pCUT, mult, pCUT, 0);
}

int main() {
  
  Gia_Man_t *p;
  //Gia_Probe_t probe1, probe2;
  //Gia_Lit_t lit1, lit2;
  //Gia_Lit_t input1, input2;
  //Gia_Lit_t t0, t1, t2;
  //Gia_Probe_t p2;
  //Gia_Lit_t t2_swept;
  //Vec_Int_t *probes;

  p = Gia_SweeperStart(NULL);

  /*  
  probe1 = Gia_SweeperProbeCreate(p, Gia_ManConst1Lit());
  probe2 = Gia_SweeperProbeCreate(p, Gia_ManConst1Lit());
  fprintf(stdout, "probe1=%d, probe2=%d\n", probe1, probe2);
  lit1 = Gia_SweeperProbeLit(p, probe1);
  Gia_SweeperProbeDelete(p, probe1);
  lit2 = Gia_SweeperProbeLit(p, probe2);
  Gia_SweeperProbeDelete(p, probe2);
  fprintf(stdout, "lit1=%d, lit2=%d\n", lit1, lit2);

  input1 = Gia_ManAppendCi(p);
  input2 = Gia_ManAppendCi(p);

  t0 = Gia_ManHashAnd(p, input1, input2);
  t1 = Gia_ManHashOr(p, input1, input2);
  t2 = Gia_ManHashMux(p, t0, t1, Gia_ManConst1Lit());

  fprintf(stdout, "t2 = %d\n", t2);

  p2 = Gia_SweeperProbeCreate(p, t2);
  probes = Vec_IntAlloc(0);
  Vec_IntPush(probes, p2);
  
  int success = Gia_SweeperFraig(p, probes, NULL, 1, 1000, 1);
  if(success != 1) { fprintf(stderr, "Sweeper Failed...\n"); exit(0); }

  t2_swept = Gia_SweeperProbeLit(p, p2);
  fprintf(stdout, "t2_swept = %d\n", t2_swept);

  Gia_SweeperProbeDelete(p, p2);

  assert(t2 != t2_swept);
  assert(t2_swept == Gia_ManConst1Lit());

  */

  machine_state *ms = machine_state_init("sweeper_test.c", 0, 12, 0x20000000, 32);
  
  Vector *a = vec_getInput(ms, 1*BITS_IN_BYTE, "a");
  cMemory_store_le(ms, 0x4, a, 1);
  vec_release(ms, a);
  
  Vector *b = vec_getInput(ms, 1*BITS_IN_BYTE, "b");
  cMemory_store_le(ms, 0x8, b, 1);
  vec_release(ms, b);

  /*
  cMemory_print(ms, ms->memory.cMem, 1);
  
  garbage_collect_ntk(ms, 0, 1);

  cMemory_print(ms, ms->memory.cMem, 1);
  
  a = cMemory_load_le(ms, 0x4, 1);
  b = cMemory_load_le(ms, 0x8, 1);
  Vector *c = pINT_MULT(ms, a, b);
  cMemory_store_le(ms, 0x0, c, 1);
  vec_release(ms, c);
  vec_release(ms, b);
  vec_release(ms, a);

  cMemory_print(ms, ms->memory.cMem, 1);

  garbage_collect_ntk(ms, 0, 1);

  cMemory_print(ms, ms->memory.cMem, 1);
  */

  Vector *choice = vec_getInput(ms, 1*BITS_IN_BYTE, "choice");
  cMemory_store_le(ms, 0x1, choice, 1);
  Gia_Lit_t choice_lit = vec_equal(ms, choice, ms->vec_zero_byte);
  vec_release(ms, choice);
  conditional_branch(ms, choice_lit, branch1, pCUT, branch2, pCUT, 0);
  
  Vector *ret = cMemory_load_le(ms, 0x0, 1);

  vec_setAsOutput(ms, ret, "output");
  
  vec_print(ms, ret);
  
  vec_release(ms, ret);

  print_AIG(ms, "sweeper_test.aig", 2);

  machine_state_free(ms);
  
  return 0;   
}
