#include <pcode_definitions.h>

int main() {
  machine_state *ms = machine_state_init("simpletest.c", 0, 12, 0x20000000, 32);
  
  Vector *x = vec_getInput(ms, 4*BITS_IN_BYTE, "x");
  Vector *y = vec_getInput(ms, 4*BITS_IN_BYTE, "y");

  Vector *z = pINT_ADD(ms, x, y);
  vec_setAsOutput(ms, z, "z");
  print_AIG(ms, "INT_ADD_32.aig", 1);
  vec_removeLastNOutputs(ms, z->size); //Fancy huh?
  vec_release(ms, z);

  z = pINT_MULT(ms, x, y);
  vec_setAsOutput(ms, z, "z");
  print_AIG(ms, "INT_MULT_32.aig", 1);

  vec_release(ms, z);
  vec_release(ms, y);
  vec_release(ms, x);

  machine_state_free(ms);

  return 0;
}
