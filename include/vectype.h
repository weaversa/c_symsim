#ifndef AIG_VECTYPE_H
#define AIG_VECTYPE_H

#include <stdint.h>
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "queue.h"

//ABC Includes
#include "aig/gia/gia.h"

#define WORD_BITS 64
#define BITS_IN_BYTE 8
#define NUM_VECS_TO_CREATE 100
#define SYMBOLIC_MEMORY_SIZE 20
#define REALLOC_DELTA 100 //Used for resizing (realloc'ing) arrays

typedef uint32_t Gia_Lit_t;
typedef uint32_t Gia_Probe_t;

struct timeval tv1;
struct timezone tzp1;

typedef struct {
  Gia_Lit_t *symWord;     //Of size WORD_BITS, indexed from 0..(.size-1)
  uintmax_t conWord;     //If .isSymbolic is false then this is the vector's value
  uintmax_t size;        //Number of bits of the vector
  unsigned isSymbolic:1; //True if the vector is symbolic, false if the vector is concrete
  unsigned inuse:1;      //False if the vector is in the stack, true if it is in use.
} Vector;

//cMemory
typedef struct {
  //Symbolic Values
  Vector *value;
  Gia_Lit_t writtenTo; //Read-before-write error
  //Probes
  Gia_Probe_t *valueProbes;
  Gia_Probe_t writtenToProbe;
} cMemoryCell;

typedef struct {
  cMemoryCell *cByte;
  uintmax_t size;
  uintmax_t base_address;
} cMemory;

//sMemory

typedef struct {
  //Symbolic Values
  Vector *value;
  Vector *address;
  //Probes
  Gia_Probe_t *valueProbes;
  Gia_Probe_t *addressProbes;
} sMemoryCell;

typedef struct sMemoryStruct {
  uintmax_t memoized_flag;
  Vector *memoized_value;
  Gia_Probe_t *memoized_value_probes;

  Gia_Lit_t writtenTo; //Read-before-write error
  Gia_Probe_t writtenToProbe;
  
  uintmax_t head;
  uint8_t address_size;
  uintmax_t size;
  sMemoryCell *sByteArray;
  
  Gia_Lit_t c;
  Gia_Probe_t cProbe;

  struct sMemoryStruct *sMemT;
  struct sMemoryStruct *sMemF;
} sMemory;

typedef struct {
  cMemory *cMem;
  sMemory *sMem;
} memTuple;

//SEAN!!! Unneeded?
typedef struct {
  Gia_Lit_t node;
  uint8_t value;
} condition;   

typedef struct {
  Gia_Man_t *ntk;
  
  uintmax_t vecs_allocated;
  uintmax_t vecs_in_stack;
  uintmax_t vectorStack_size;
  void_arr_stack **vectorStack;

  Vector *vec_zero_byte;
  Gia_Probe_t *vec_zero_byte_probes;

  void_arr_stack *conditions_stack;

  //Controls for the frequency of garbage collection
  uintmax_t nNodes_last;
  uintmax_t nNodes_increment;
  Vec_Int_t *gc_probes;
  uintmax_t next_gc_probe;

  uint8_t branch_error;
  uintmax_t stackframe_depth;

  Vector *heap_offset;
  Gia_Probe_t *heap_offset_probes;

  uintmax_t CurrsMemFlag;
  void_arr_stack *sMemStack;       //Needed for sMemory reads
  void_arr_stack *sMemInitStack;   //Needed for sMemory init and deletion
  void_arr_stack *sMemDeleteStack; //Needed for sMemory deletion
  void_arr_stack *memories_stack;
  uint8_t sMemory_auto_compress;

  Vec_Int_t *pOutputProbes;
  Vec_Ptr_t *pOutputNames;

  memTuple memory;
} machine_state;


//Symbolic Vector Routines

void vec_free(machine_state *ms, Vector *vec);
void vec_verify(machine_state *ms, Vector *vec);
Vector *vec_get(machine_state *ms, uintmax_t num_bits);
void vec_setValue(machine_state *ms, Vector *vec, uintmax_t value);
void vec_calc_sym(machine_state *ms, Vector *vec);
Vector *vec_getConstant(machine_state *ms, uintmax_t value, uintmax_t num_bits);
Vector **vec_getArray(machine_state *ms, uintmax_t num_arr_elements, uintmax_t num_element_bits);
Vector **vec_getConstantArray(machine_state *ms, uintmax_t value, uintmax_t num_arr_elements, uintmax_t num_element_bits);
void vec_release(machine_state *ms, Vector *vec);
void vec_releaseArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements);
void vec_copy(machine_state *ms, Vector *dst, Vector *src);
Vector *vec_dup(machine_state *ms, Vector *src);
void vec_setAsInput(machine_state *ms, Vector *vec, char name[1024]);
Vector *vec_getInput(machine_state *ms, uintmax_t num_element_bits, char name[1024]);
void vec_setAsInputArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements, char name[1024]);
Vector **vec_getInputArray(machine_state *ms, uintmax_t num_arr_elements, uintmax_t num_element_bits, char name[1024]);
void vec_setAsOutput(machine_state *ms, Vector *vec, char name[1024]);
void vec_setAsOutputArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements, char name[1024]);
void vec_removeLastNOutputs(machine_state *ms, uintmax_t n);
Vector *vec_joinArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements);
Vector **vec_splitIntoNewArray(machine_state *ms, Vector *vec, uintmax_t num_arr_elements);
void vec_splitIntoArray(machine_state *ms, Vector *vec, Vector **vec_array, uintmax_t num_arr_elements);

// Transformations between probes and vectors

Gia_Probe_t get_probe_from_lit(machine_state *ms, Gia_Lit_t lit);
void update_probe_from_lit(machine_state *ms, Gia_Probe_t probe, Gia_Lit_t lit);
Gia_Lit_t get_lit_from_probe(machine_state *ms, Gia_Probe_t probe);
void probe_free(machine_state *ms, Gia_Probe_t probe);
Gia_Probe_t *get_probes_from_vec(machine_state *ms, Vector *vec);
void update_probes_from_vec(machine_state *ms, Gia_Probe_t *probes, Vector *vec);
void update_vec_from_probes(machine_state *ms, Gia_Probe_t *probes, Vector *vec);
Vector *get_vec_from_probes(machine_state *ms, Gia_Probe_t *probes, uintmax_t size);
void probes_free(machine_state *ms, Gia_Probe_t *probes, uintmax_t size);
void collect_probe(machine_state *ms, Gia_Probe_t probe);
void collect_probes(machine_state *ms, Gia_Probe_t *probes, uintmax_t size);

// Transformations between symbolic and concrete vectors

void vec_sym_to_con(machine_state *ms, Vector *vec);
uint8_t vec_sym_to_con_attempt(machine_state *ms, Vector *vec);
uint8_t vec_concretize_with_SAT(machine_state *ms, Vector *vec);

//Functions for printing vectors

void vec_printSimple(machine_state *ms, Vector *vec);
void vec_print(machine_state *ms, Vector *vec);
void vec_printArraySimple(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements);
void vec_printArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements);

//Vector manipulation routines

uintmax_t int_zextend(uintmax_t, uintmax_t x_size);
Vector *vec_zextend(machine_state *ms, Vector *x, uintmax_t result_size);
uintmax_t int_sextend(uintmax_t x, uintmax_t x_size, uintmax_t result_size);
Vector *vec_sextend(machine_state *ms, Vector *x, uintmax_t result_size);
Vector *vec_cat(machine_state *ms, Vector *x, Vector *y);
Vector *vec_trunc(machine_state *ms, Vector *x, uintmax_t amount);
Vector *vec_extract(machine_state *ms, Vector *x, uintmax_t start, uintmax_t amount);
Vector *vec_and(machine_state *ms, Vector *x, Vector *y);
Vector *vec_or(machine_state *ms, Vector *x, Vector *y);
Vector *vec_xor(machine_state *ms, Vector *x, Vector *y);
uint8_t vec_sym_equal(machine_state *ms, Vector *x, Vector *y);
Gia_Lit_t vec_equal(machine_state *ms, Vector *x, Vector *y);
Gia_Lit_t vec_equal_SAT(machine_state *ms, Vector *x, Vector *y, uint8_t call_SAT_solver);
Vector *vec_negate(machine_state *ms, Vector *x);
Vector *vec_invert(machine_state *ms, Vector *x);
Vector *vec_ite(machine_state *ms, Gia_Lit_t c, Vector *x, Vector *y);
Vector *vec_abs(machine_state *ms, Vector *x);
Gia_Lit_t vec_greaterthan(machine_state *ms, Vector *x, Vector *y);
Gia_Lit_t vec_signed_greaterthan(machine_state *ms, Vector *x, Vector *y);
Gia_Lit_t vec_lessthan(machine_state *ms, Vector *x, Vector *y);
Gia_Lit_t vec_signed_lessthan(machine_state *ms, Vector *x, Vector *y);
Vector *vec_shiftright(machine_state *ms, Vector *x, Vector *amount);
Vector *vec_signedshiftright(machine_state *ms, Vector *x, Vector *amount);
Vector *vec_shiftleft(machine_state *ms, Vector *x, Vector *amount);
Vector *vec_rotateright(machine_state *ms, Vector *x, Vector *amount);
Vector *vec_rotateleft(machine_state *ms, Vector *x, Vector *amount);
Vector *vec_selectBits(machine_state *ms, Vector *x, uint16_t num_bits, uint16_t bit_offset);
Vector *vec_reverse(machine_state *ms, Vector *x);
Vector *vec_add(machine_state *ms, Vector *x, Vector *y);
Gia_Lit_t vec_carry(machine_state *ms, Vector *x, Vector *y);
Vector *vec_sub(machine_state *ms, Vector *x, Vector *y);
Vector *vec_mult(machine_state *ms, Vector *x, Vector *y);
Vector *vec_quot_rem(machine_state *ms, Vector *x, Vector *y, uint8_t quot_rem);

//Routines for handling concretely addressed memory

cMemory *cMemory_init(machine_state *ms, uintmax_t base_address, uintmax_t size);
void cMemory_free(machine_state *ms, cMemory *cMem);
void cMemory_print(machine_state *ms, cMemory *cMem, uint8_t full);
void cMemory_store_le(machine_state *ms, uintmax_t address, Vector *value, uintmax_t size);
void cMemory_store_be(machine_state *ms, uintmax_t address, Vector *value, uintmax_t size);
Vector *cMemory_load_le(machine_state *ms, uintmax_t address, uintmax_t size);
Vector *cMemory_load_be(machine_state *ms, uintmax_t address, uintmax_t size);
Gia_Lit_t cMemory_load_rbw(machine_state *ms, uintmax_t address, uintmax_t size);
cMemory *cMemory_ite(machine_state *ms, Gia_Lit_t c, cMemory *cMemT, cMemory *cMemF);
cMemory *cMemory_copy(machine_state *ms, cMemory *cMem);
void cMemory_update_probes(machine_state *ms, cMemory *cMem);
void cMemory_update_from_probes(machine_state *ms, cMemory *cMem);
void cMemory_collect_probes(machine_state *ms, cMemory *cMem);

//Routined for handling symbolically addressed memory

sMemory *sMemory_init(machine_state *ms, uint8_t address_size);
void sMemory_free(machine_state *ms, sMemory *sMem);
void sMemory_print(machine_state *ms, sMemory *sMem, uint8_t full);
void sMemory_store_le(machine_state *ms, Vector *address_start, Vector *value, uintmax_t size);
void sMemory_store_be(machine_state *ms, Vector *address, Vector *value, uintmax_t size);
void sMemory_storeInt_le(machine_state *ms, uintmax_t address, uintmax_t value, uintmax_t size);
void sMemory_storeInt_be(machine_state *ms, uintmax_t address, uintmax_t value, uintmax_t size);
void sMemory_storeArray_le(machine_state *ms, Vector *address, Vector **vec_array, uintmax_t numArrayElements, uintmax_t numElementBytes);
void sMemory_storeArray_be(machine_state *ms, Vector *address, Vector **vec_array, uintmax_t numArrayElements, uintmax_t numElementBytes);
Vector *sMemory_load_le(machine_state *ms, Vector *address, uintmax_t size);
Vector *sMemory_load_be(machine_state *ms, Vector *address, uintmax_t size);
Gia_Lit_t sMemory_load_rbw(machine_state *ms, Vector *address, uintmax_t size);
Vector **sMemory_loadArray_le(machine_state *ms, Vector *address, uintmax_t numArrayElements, uintmax_t numElementBytes);
Vector **sMemory_loadArray_be(machine_state *ms, Vector *address, uintmax_t numArrayElements, uintmax_t numElementBytes);
sMemory *sMemory_ite(machine_state *ms, Gia_Lit_t c, sMemory *sMemT, sMemory *sMemF);
sMemory *sMemory_copy(machine_state *ms, sMemory *sMem);
void sMemory_compress(machine_state *ms, sMemory *sMem);
void sMemory_update_probes(machine_state *ms, sMemory *sMem);
void sMemory_update_from_probes(machine_state *ms, sMemory *sMem);
void sMemory_collect_probes(machine_state *ms, sMemory *sMem);

//Constant check functions (calls to the SAT solver)

void pop_condition(machine_state *ms);
void push_condition(machine_state *ms, Gia_Lit_t node, uint8_t value);
uint8_t is_node_constant(machine_state *ms, Gia_Lit_t node, uint8_t value);
int8_t are_conditions_unsat(machine_state *ms, uint8_t print_result);
void print_sat_solver_result_on_inputs(machine_state *ms);

//Cleaning up and printing the AIG
//uint8_t cut_sweep(machine_state *ms);
void print_AIG(machine_state *ms, char *filename, uint8_t clean_first);
Gia_Lit_t garbage_collect_ntk(machine_state *ms, Gia_Lit_t cond, uint8_t force_gc);
#endif
