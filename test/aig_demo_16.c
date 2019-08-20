#include <pcode_definitions.h>

void gcd_sym(machine_state *ms);
void gcd_sym_b0(machine_state *ms);
void gcd_sym_bn0(machine_state *ms);

void gcd_sym(machine_state *ms) {
  Vector *b = cMemory_load_be(ms, 0x8, 2);
  
  Vector *c_zero = vec_getConstant(ms, 0, 2*BITS_IN_BYTE);
  
  //if(b == 0)
  Gia_Lit_t cond = vec_equal(ms, b, c_zero);
  vec_printSimple(ms, b);
  
  conditional_branch(ms, cond, gcd_sym_b0, pNULL, gcd_sym_bn0, pNULL, 0);
  
  vec_release(ms, c_zero);
  vec_release(ms, b);
}

//b==0 return a;
void gcd_sym_b0(machine_state *ms) {
  Vector *a = cMemory_load_be(ms, 0x4, 2);
  cMemory_store_be(ms, 0x0, a, 2);
  vec_release(ms, a);
}

//b!=0, call gcd(b, a mod b)
void gcd_sym_bn0(machine_state *ms) {
  Vector *a = cMemory_load_be(ms, 0x4, 2);
  Vector *b = cMemory_load_be(ms, 0x8, 2);
  
  Vector *a_mod_b = vec_quot_rem(ms, a, b, 0);
  
  cMemory_store_be(ms, 0x4, b, 2);
  cMemory_store_be(ms, 0x8, a_mod_b, 2);

  vec_release(ms, a_mod_b);
  vec_release(ms, b);
  vec_release(ms, a);
  
  gcd_sym(ms);
}

int main() {
  
  machine_state *ms = machine_state_init("aig_demo.c", 0, 12, 0x20000000, 32);
  
  Vector *a = vec_getInput(ms, 2*BITS_IN_BYTE, "a");
  cMemory_store_be(ms, 0x4, a, 2);

  Vector *b = vec_getInput(ms, 2*BITS_IN_BYTE, "b");
  cMemory_store_be(ms, 0x8, b, 2);
  
  cMemory_print(ms, ms->memory.cMem, 0);
  
  gcd_sym(ms);
  
  Vector *ret = cMemory_load_be(ms, 0x0, 2);
  
  cMemory_print(ms, ms->memory.cMem, 0);
  
  vec_setAsOutput(ms, ret, "output");
  
  vec_print(ms, ret);
  
  vec_release(ms, ret);
  vec_release(ms, b);
  vec_release(ms, a);

  print_AIG(ms, "aig_demo.aig", 2); //Clean = Cut Sweep

  machine_state_free(ms);
  
  return 0;
}
