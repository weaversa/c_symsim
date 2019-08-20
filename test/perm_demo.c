#include <pcode_definitions.h>

unsigned char translate[] =
  {
    153, 214, 158, 155, 57, 138, 52, 101, 192, 157, 118, 251, 139, 33,
    160, 210, 0, 255, 229, 182, 143, 159, 250, 179, 166, 128, 134, 114,
    6, 30, 156, 206, 144, 113, 146, 227, 211, 121, 38, 215, 167, 147,
    183, 100, 39, 252, 105, 31, 246, 205, 161, 109, 126, 58, 122, 188,
    247, 207, 104, 50, 150, 203, 193, 106, 28, 224, 124, 51, 83, 222,
    189, 231, 204, 66, 29, 7, 9, 54, 56, 27, 88, 175, 72, 226, 116,
    173, 110, 103, 24, 55, 169, 35, 49, 59, 3, 16, 135, 178, 67, 25,
    194, 80, 15, 5, 217, 84, 181, 95, 102, 137, 172, 81, 131, 89, 82,
    136, 151, 208, 90, 32, 86, 45, 91, 125, 41, 127, 112, 220, 191,
    253, 168, 68, 149, 47, 190, 245, 123, 20, 162, 209, 92, 199, 170,
    93, 198, 171, 76, 111, 23, 48, 235, 142, 85, 145, 42, 238, 1, 71,
    232, 21, 163, 154, 10, 254, 4, 22, 164, 197, 129, 174, 242, 223,
    73, 248, 241, 53, 130, 40, 61, 63, 11, 97, 218, 17, 152, 196, 8,
    26, 165, 187, 74, 219, 18, 148, 65, 202, 216, 176, 64, 239, 243,
    79, 249, 115, 119, 19, 233, 120, 230, 184, 14, 75, 60, 94, 117,
    132, 200, 78, 185, 62, 96, 13, 221, 180, 213, 34, 70, 2, 44, 12,
    43, 244, 236, 133, 87, 141, 98, 99, 186, 37, 46, 234, 140, 69, 177,
    201, 195, 225, 107, 77, 237, 212, 240, 36, 228, 108
  };

int main() {
  machine_state *ms = machine_state_init("aig_demo.c", 0, 12, 0x20000000, 32);

  Vector *table_address = vec_getConstant(ms, 0x1234abcd, 32);

  uint16_t i;
  //Copy concrete table into memory.
  for(i = 0; i < 256; i++) {
    Vector *table_element = vec_getConstant(ms, translate[i], 1*BITS_IN_BYTE);
    Vector *i_vec = vec_getConstant(ms, i, 32);
    Vector *table_element_address = vec_add(ms, i_vec, table_address);

    sMemory_store_le(ms, table_element_address, table_element, 1);

    vec_release(ms, table_element_address);
    vec_release(ms, i_vec);
    vec_release(ms, table_element);
  }

  Vector *input_address = vec_getConstant(ms, 0xabcd1234, 4*BITS_IN_BYTE);
  
  uint16_t input_size = 100;
  Vector **input = vec_getInputArray(ms, input_size, 1*BITS_IN_BYTE, "input");

  //Copy input into memory.
  for(i = 0; i < input_size; i++) {
    Vector *i_vec = vec_getConstant(ms, i, 4*BITS_IN_BYTE);
    Vector *input_element_address = vec_add(ms, i_vec, input_address);

    sMemory_store_le(ms, input_element_address, input[i], 1);

    vec_release(ms, input_element_address);
    vec_release(ms, i_vec);
  }

  for(i = 0; i < input_size; i++) {
    Vector *i_vec = vec_getConstant(ms, i, 4*BITS_IN_BYTE);    
    Vector *input_element_address = vec_add(ms, i_vec, input_address);
    Vector *input_element = sMemory_load_le(ms, input_element_address, 1);

    Vector *input_element_resized = vec_zextend(ms, input_element, 4*BITS_IN_BYTE);

    Vector *table_element_address = vec_add(ms, table_address, input_element_resized);

    Vector *table_element = sMemory_load_le(ms, table_element_address, 1);
    
    sMemory_store_le(ms, input_element_address, table_element, 1);
    //local[i] = translate[local[i]];
    
    vec_release(ms, table_element);
    vec_release(ms, table_element_address);
    vec_release(ms, input_element_address);
    vec_release(ms, input_element_resized);
    vec_release(ms, input_element);
    vec_release(ms, i_vec);
  }

  Vector **output = vec_getArray(ms, input_size, 1*BITS_IN_BYTE);

  //Copy input from memory to output.
  for(i = 0; i < input_size; i++) {
    Vector *i_vec = vec_getConstant(ms, i, 4*BITS_IN_BYTE);
    Vector *input_element_address = vec_add(ms, i_vec, input_address);

    Vector *output_i = sMemory_load_le(ms, input_element_address, 1);
    vec_copy(ms, output[i], output_i);

    vec_release(ms, output_i);
    vec_release(ms, input_element_address);
    vec_release(ms, i_vec);
  }
  
  vec_setAsOutputArray(ms, output, input_size, "output");

  //  sMemory_print(ms, ms->memory.sMem, 1);

  vec_release(ms, table_address);
  vec_release(ms, input_address);
  vec_releaseArray(ms, input, input_size);
  vec_releaseArray(ms, output, input_size);

  print_AIG(ms, "perm_demo.aig", 2); //Clean = Cut Sweep

  machine_state_free(ms);

  return 0;   
}
