#include <pcode_definitions.h>

void gcd_symlist(machine_state *ms);
void gcd_sym(machine_state *ms);
void gcd_sym_b0(machine_state *ms);
void gcd_sym_bn0(machine_state *ms);

void gcd_sym(machine_state *ms) {
  Vector *b = cMemory_load_le(ms, 8, 1);
  
  Vector *c_zero = vec_getConstant(ms, 0, 1*BITS_IN_BYTE);
  
  //if(b == 0)
  Gia_Lit_t cond = vec_equal(ms, b, c_zero);
  
  vec_release(ms, c_zero);
  vec_release(ms, b);
  
  conditional_branch(ms, cond, gcd_sym_b0, pNULL, gcd_sym_bn0, pNULL, 1);
}

//b==0 return a;
void gcd_sym_b0(machine_state *ms) {
  Vector *a = cMemory_load_le(ms, 4, 1);
  cMemory_store_le(ms, 0, a, 1);
  vec_release(ms, a);
}

//b!=0, call gcd(b, a mod b)
void gcd_sym_bn0(machine_state *ms) {
  Vector *a = cMemory_load_le(ms, 4, 1);
  Vector *b = cMemory_load_le(ms, 8, 1);
  
  Vector *a_mod_b = vec_quot_rem(ms, a, b, 0);
  
  cMemory_store_le(ms, 4, b, 1);
  cMemory_store_le(ms, 8, a_mod_b, 1);
  
  vec_release(ms, a_mod_b);
  vec_release(ms, b);
  vec_release(ms, a);
  
  gcd_sym(ms);
}

//size == 0
void gcd_symlist_size0(machine_state *ms) {
  
  //   Vector *size = vec_getConstant(ms, 0, 1*BITS_IN_BYTE);
  //   cMemory_store_le(ms, 20, size, 1);
  //   vec_release(ms, size);
  
}

//size != 0
void gcd_symlist_sizen0(machine_state *ms) {
  
  Vector *size1 = cMemory_load_le(ms, 20, 1);
  Vector *size = vec_zextend(ms, size1, 4*BITS_IN_BYTE);
  
  Vector *a_address = cMemory_load_le(ms, 12, ms->memory.sMem->address_size / BITS_IN_BYTE);
  Vector *a_address_psize = vec_add(ms, a_address, size);
  Vector *a = sMemory_load_le(ms, a_address_psize, 1);
  cMemory_store_le(ms, 4, a, 1);
  vec_release(ms, a);
  //   vec_release(ms, a_address_psize);
  vec_release(ms, a_address);
  
  Vector *b_address = cMemory_load_le(ms, 16, ms->memory.sMem->address_size / BITS_IN_BYTE);
  Vector *b_address_psize = vec_add(ms, b_address, size);
  Vector *b = sMemory_load_le(ms, b_address_psize, 1);
  cMemory_store_le(ms, 8, b, 1);
  vec_release(ms, b);
  vec_release(ms, b_address_psize);
  vec_release(ms, b_address);
  vec_release(ms, size);
  
  Vector *one = vec_getConstant(ms, 1, BITS_IN_BYTE);
  Vector *sizem1 = vec_sub(ms, size1, one);
  //vec_concretize_with_SAT(ms, sizem1); SEAN!!!
  cMemory_store_le(ms, 20, sizem1, 1);
  vec_release(ms, sizem1);
  vec_release(ms, one);
  vec_release(ms, size1);
  
  gcd_sym(ms);
  
  Vector *ret = cMemory_load_le(ms, 0, 1);
  sMemory_store_le(ms, a_address_psize, ret, 1);
  vec_release(ms, ret);
  vec_release(ms, a_address_psize);
  
  gcd_symlist(ms);
  
}

//Does size == 0?
void gcd_symlist(machine_state *ms) {
  Vector *size = cMemory_load_le(ms, 20, 1);
  //vec_concretize_with_SAT(ms, size); SEAN!!!
  cMemory_store_le(ms, 20, size, 1);
  
  Vector *c_zero = vec_getConstant(ms, 0, 1*BITS_IN_BYTE);
  
  //if(b == 0)
  Gia_Lit_t cond = vec_equal(ms, size, c_zero);
  
  vec_release(ms, c_zero);
  vec_release(ms, size);
  
  conditional_branch(ms, cond, gcd_symlist_size0, pNULL, gcd_symlist_sizen0, pNULL, 1);
}

int main() {
  machine_state *ms = machine_state_init("sym_demo.c", 0, 24, 0x20000000, 32);
  
  Vector **a = vec_getInputArray(ms, 8, 1*BITS_IN_BYTE, "a");
  Vector *a_address = vec_getConstant(ms, 0x1234, ms->memory.sMem->address_size);
  cMemory_store_le(ms, 12, a_address, ms->memory.sMem->address_size / BITS_IN_BYTE);
  sMemory_storeArray_le(ms, a_address, a, 8, 1);
  vec_release(ms, a_address);
  vec_releaseArray(ms, a, 8);

  Vector **b = vec_getInputArray(ms, 8, 1*BITS_IN_BYTE, "b");
  Vector *b_address = vec_getConstant(ms, 0x5678, ms->memory.sMem->address_size);
  cMemory_store_le(ms, 16, b_address, ms->memory.sMem->address_size / BITS_IN_BYTE);
  sMemory_storeArray_le(ms, b_address, b, 8, 1);
  vec_release(ms, b_address);
  vec_releaseArray(ms, b, 8);
  
  Vector *size = vec_get(ms, 1*BITS_IN_BYTE);
  vec_setAsInput(ms, size, "size");
  Vector *eight = vec_getConstant(ms, 8, 1*BITS_IN_BYTE);
  Vector *size_mod = vec_quot_rem(ms, size, eight, 0);
  cMemory_store_le(ms, 20, size_mod, 1);
  vec_release(ms, eight);
  vec_release(ms, size_mod);
  vec_release(ms, size);
  
  gcd_symlist(ms);
  
  Vector *ret = cMemory_load_le(ms, 20, 1);
  vec_setAsOutput(ms, ret, "output size");
  //   vec_print(ms, ret);
  vec_release(ms, ret);
  
  a_address = cMemory_load_le(ms, 12, ms->memory.sMem->address_size / BITS_IN_BYTE);
  vec_print(ms, a_address);
  a = sMemory_loadArray_le(ms, a_address, 8, 1);
  vec_release(ms, a_address);
  vec_setAsOutputArray(ms, a, 8, "a_o");
  //   vec_printArray(ms, a, 8);
  vec_releaseArray(ms, a, 8);
  
  b_address = cMemory_load_le(ms, 16, ms->memory.sMem->address_size / BITS_IN_BYTE);
  vec_print(ms, b_address);
  b = sMemory_loadArray_le(ms, b_address, 8, 1);
  vec_release(ms, b_address);
  vec_setAsOutputArray(ms, b, 8, "b_o");
  //   vec_printArray(ms, b, 8);
  vec_releaseArray(ms, b, 8);
  
  print_AIG(ms, "sym_demo.aig", 2); //Clean = Cut Sweep

  machine_state_free(ms);

  return 0;   
}

