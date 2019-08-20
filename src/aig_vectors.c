#include "vectype.h"

//Symbolic Vector Routines

Vector *alloc_vector(uintmax_t num_bits) {
  Vector *new_vec = (Vector *)malloc(sizeof(Vector));
  new_vec->symWord = (Gia_Lit_t *)calloc(num_bits, sizeof(Gia_Lit_t ));
  new_vec->conWord = 0;
  new_vec->size = num_bits;
  new_vec->isSymbolic = 0;
  new_vec->inuse = 0;
  return new_vec;
}

void vec_free(machine_state *ms, Vector *vec) {
  free(vec->symWord);
  free(vec);   
}

void vec_verify(machine_state *ms, Vector *vec) {
  uintmax_t i;
  if(vec == NULL) return;
  assert(vec->size > 0);
  assert(vec->size <= WORD_BITS || vec->isSymbolic == 1);
  assert(vec->inuse == 1);
  if(vec->isSymbolic == 1) {
    assert(vec->symWord != NULL);
    for(i = 0; i < vec->size; i++) {
      Gia_Obj_t *node = Gia_ObjFromLit(ms->ntk, vec->symWord[i]);
      Gia_Lit_t lit = Gia_ObjToLit(ms->ntk, node);
      assert(lit == vec->symWord[i]); (void)lit;
    }
  }
}

void vec_setValue(machine_state *ms, Vector *vec, uintmax_t value) {
  uintmax_t i;
  vec->conWord = value;
  for(i = 0; i < vec->size; i++) {
    if((value&1) == 0)
      vec->symWord[i] = Gia_ManConst0Lit();
    else
      vec->symWord[i] = Gia_ManConst1Lit();
    value>>=1;
  }
  if(vec->size > WORD_BITS)
    vec->isSymbolic = 1;
  else vec->isSymbolic = 0;
}

void vec_calc_sym(machine_state *ms, Vector *vec) {
  assert(!vec->isSymbolic);
  vec_setValue(ms, vec, vec->conWord);
}

Vector *vec_get(machine_state *ms, uintmax_t num_bits) {
  uintmax_t i;
  if(num_bits >= ms->vectorStack_size) {
    ms->vectorStack = (void_arr_stack **)realloc((void *)ms->vectorStack, (num_bits + REALLOC_DELTA) * sizeof(void_arr_stack *));
    for(i = ms->vectorStack_size; i < num_bits+REALLOC_DELTA; i++)
      ms->vectorStack[i] = arr_stack_init();
    ms->vectorStack_size = num_bits+REALLOC_DELTA;
  }
  if(ms->vectorStack[num_bits]->head == 0) {
    //Stack is empty, populate it with new vectors
    for(i = 0; i < NUM_VECS_TO_CREATE; i++)
      arr_stack_push(ms->vectorStack[num_bits], (void *)alloc_vector(num_bits));
    ms->vecs_allocated+=NUM_VECS_TO_CREATE;
    ms->vecs_in_stack+=NUM_VECS_TO_CREATE;
  }
  Vector *new_vec = (Vector *)arr_stack_pop(ms->vectorStack[num_bits]);
  ms->vecs_in_stack--;
  assert(new_vec->inuse == 0);
  new_vec->inuse = 1;
  return new_vec;
}

Vector *vec_getConstant(machine_state *ms, uintmax_t value, uintmax_t num_bits) {
  assert(num_bits <= WORD_BITS);
  Vector *new_vec = vec_get(ms, num_bits);
  vec_setValue(ms, new_vec, value);
  return new_vec;
}

Vector **vec_getArray(machine_state *ms, uintmax_t num_arr_elements, uintmax_t num_element_bits) {
  uintmax_t i;
  Vector **vec_array = (Vector **)malloc(num_arr_elements * sizeof(Vector *));
  for(i = 0; i < num_arr_elements; i++)
    vec_array[i] = vec_get(ms, num_element_bits);
  return vec_array;   
}

Vector **vec_getConstantArray(machine_state *ms, uintmax_t value, uintmax_t num_arr_elements, uintmax_t num_element_bits) {
  assert(num_element_bits <= WORD_BITS);
  uintmax_t i;
  Vector **vec_array = (Vector **)malloc(num_arr_elements * sizeof(Vector *));
  for(i = 0; i < num_arr_elements; i++)
    vec_array[i] = vec_getConstant(ms, value, num_element_bits);
  return vec_array;
}

void vec_release(machine_state *ms, Vector *vec) {
  assert(vec->inuse == 1);
  vec->inuse = 0;
  arr_stack_push(ms->vectorStack[vec->size], (void *)vec);
  ms->vecs_in_stack++;   
}

void vec_releaseArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements) {
  uintmax_t i;
  assert(vec_array != NULL);
  for(i = 0; i < num_arr_elements; i++)
    vec_release(ms, vec_array[i]);
  free(vec_array);
}

inline
void vec_copy(machine_state *ms, Vector *dst, Vector *src) {
  assert(src->size == dst->size);
  memcpy(dst->symWord, src->symWord, src->size * sizeof(Gia_Lit_t ));
  dst->conWord    = src->conWord;
  dst->isSymbolic = src->isSymbolic;
  dst->inuse      = src->inuse;
}

inline
Vector *vec_dup(machine_state *ms, Vector *src) {
  Vector *dst = vec_get(ms, src->size);
  vec_copy(ms, dst, src);
  return dst;
}

void vec_setAsInput(machine_state *ms, Vector *vec, char name[1024]) {
  char *buf;
  uintmax_t i;

  if(ms->ntk->vNamesIn == NULL) {
    ms->ntk->vNamesIn = Vec_PtrAlloc(100);
  }

  vec->isSymbolic = 1;
  for(i = 0; i < vec->size; i++) {
    buf = (char *)malloc(1024 * sizeof(char));    
    snprintf(buf, 1024, "%s_%ju", name, i);
    vec->symWord[i] = Gia_ManAppendCi(ms->ntk);
    Vec_PtrPush(ms->ntk->vNamesIn, buf);
  }
}

Vector *vec_getInput(machine_state *ms, uintmax_t num_element_bits, char name[1024]) {
  Vector *vec = vec_get(ms, num_element_bits);
  vec_setAsInput(ms, vec, name);
  return vec;
}

void vec_setAsInputArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements, char name[1024]) {
  char buf[1024];
  uintmax_t i;
  for(i = 0; i < num_arr_elements; i++) {
    snprintf(buf, 1024, "%s_%ju", name, i);
    vec_setAsInput(ms, vec_array[i], buf);
  }
}

Vector **vec_getInputArray(machine_state *ms, uintmax_t num_arr_elements, uintmax_t num_element_bits, char name[1024]) {
  uintmax_t i;
  char buf[1024];
  Vector **vec_array = (Vector **)malloc(num_arr_elements * sizeof(Vector *));
  for(i = 0; i < num_arr_elements; i++) {
    snprintf(buf, 1024, "%s_%ju", name, i);
    vec_array[i] = vec_getInput(ms, num_element_bits, buf);
  }
  return vec_array;   
}

void vec_setAsOutput(machine_state *ms, Vector *vec, char name[1024]) {
  char *buf;
  uintmax_t i;

  if(!vec->isSymbolic)
    vec_calc_sym(ms, vec);
  
  for(i = 0; i < vec->size; i++) {
    buf = (char *)malloc(1024 * sizeof(char));
    snprintf(buf, 1024, "%s_%ju", name, i);
    Gia_Probe_t tmp_output_probe = Gia_SweeperProbeCreate(ms->ntk, vec->symWord[i]);
    Vec_IntPush(ms->pOutputProbes, tmp_output_probe);
    Vec_PtrPush(ms->pOutputNames, buf);
  }
}

void vec_setAsOutputArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements, char name[1024]) {
  char buf[1024];
  uintmax_t i;
  for(i = 0; i < num_arr_elements; i++) {
    snprintf(buf, 1024, "%s_%ju", name, i);
    vec_setAsOutput(ms, vec_array[i], buf);
  }
}

void vec_removeLastNOutputs(machine_state *ms, uintmax_t n) {
  uintmax_t i;
  assert(Vec_IntSize(ms->pOutputProbes) == Vec_PtrSize(ms->pOutputNames));
  assert(n <= Vec_IntSize(ms->pOutputProbes));
  
  for(i = 0; i < n; i++) {
    Gia_Probe_t tmp_output_probe = Vec_IntPop(ms->pOutputProbes);
    Gia_SweeperProbeDelete(ms->ntk, tmp_output_probe);
    char *buf = Vec_PtrPop(ms->pOutputNames);
    free(buf);
  }
}

// Transformations between probes and vectors

inline
Gia_Probe_t get_probe_from_lit(machine_state *ms, Gia_Lit_t lit) {
  return Gia_SweeperProbeCreate(ms->ntk, lit);
}

inline
void update_probe_from_lit(machine_state *ms, Gia_Probe_t probe, Gia_Lit_t lit) {
  Gia_SweeperProbeUpdate(ms->ntk, probe, lit);
};

inline
Gia_Lit_t get_lit_from_probe(machine_state *ms, Gia_Probe_t probe) {
  return Gia_SweeperProbeLit(ms->ntk, probe);
}

inline
void probe_free(machine_state *ms, Gia_Probe_t probe) {
  Gia_SweeperProbeDelete(ms->ntk, probe);
}

inline
Gia_Probe_t *get_probes_from_vec(machine_state *ms, Vector *vec) {
  uintmax_t i;
  if(!vec->isSymbolic) vec_calc_sym(ms, vec);
  Gia_Probe_t *probes = (Gia_Probe_t *)malloc(vec->size * sizeof(Gia_Probe_t));
  for(i = 0; i < vec->size; i++) {
    probes[i] = get_probe_from_lit(ms, vec->symWord[i]);
  }
  return probes;
}

inline
void update_probes_from_vec(machine_state *ms, Gia_Probe_t *probes, Vector *vec) {
  uintmax_t i;
  if(!vec->isSymbolic) vec_calc_sym(ms, vec);
  for(i = 0; i < vec->size; i++) {
    update_probe_from_lit(ms, probes[i], vec->symWord[i]);
  }
}

inline
void update_vec_from_probes(machine_state *ms, Gia_Probe_t *probes, Vector *vec) {
  uintmax_t i;
  for(i = 0; i < vec->size; i++) {
    vec->symWord[i] = get_lit_from_probe(ms, probes[i]);
  }
  vec->isSymbolic = 1;
  vec_sym_to_con_attempt(ms, vec);
}

inline
Vector *get_vec_from_probes(machine_state *ms, Gia_Probe_t *probes, uintmax_t size) {
  Vector *vec = vec_get(ms, size);
  update_vec_from_probes(ms, probes, vec);
  return vec;  
}

inline
void probes_free(machine_state *ms, Gia_Probe_t *probes, uintmax_t size) {
  uintmax_t i;
  for(i = 0; i < size; i++)
    probe_free(ms, probes[i]);
  free(probes);
}

inline
void collect_probe(machine_state *ms, Gia_Probe_t probe) {
  Vec_IntPush(ms->gc_probes, probe);
  //fprintf(stdout, "[%d -> %d]", probe, get_lit_from_probe(ms, probe));
}

inline
void collect_probes(machine_state *ms, Gia_Probe_t *probes, uintmax_t size) {
  uintmax_t i;
  for(i = 0; i < size; i++) {
    collect_probe(ms, probes[i]);
  }
}

// Transformations between symbolic and concrete vectors

void vec_sym_to_con(machine_state *ms, Vector *vec) {
  intmax_t i; //Must be a signed integer
  assert(vec->size <= WORD_BITS);
  assert(Gia_ManIsConstLit(vec->symWord[vec->size-1]));
  uint8_t torf = Gia_ManIsConst1Lit(vec->symWord[vec->size-1]);
  uintmax_t conWord = torf;
  for(i = (vec->size)-2; i >= 0; i--) {
    assert(Gia_ManIsConstLit(vec->symWord[i]));
    conWord<<=1;
    torf = Gia_ManIsConst1Lit(vec->symWord[i]);
    conWord+=torf;      
  }
  vec->conWord = conWord;
  vec->isSymbolic = 0;   
}

uint8_t vec_sym_to_con_attempt(machine_state *ms, Vector *vec) {
  intmax_t i; //Must be a signed integer
  if(vec->size > WORD_BITS) return 0;
  if(!Gia_ManIsConstLit(vec->symWord[vec->size-1])) return 0;
  uint8_t torf = Gia_ManIsConst1Lit(vec->symWord[vec->size-1]);
  uintmax_t conWord = torf;
  for(i = (vec->size)-2; i >= 0; i--) {
    if(!Gia_ManIsConstLit(vec->symWord[i])) return 0;
    conWord<<=1;
    torf = Gia_ManIsConst1Lit(vec->symWord[i]);
    conWord+=torf;      
  }
  vec->conWord = conWord;
  vec->isSymbolic = 0;   
  return 1;
}

uint8_t vec_concretize_with_SAT(machine_state *ms, Vector *vec) {
  uintmax_t i;
  if(vec->isSymbolic == 0) return 1;
  for(i = 0; i < vec->size; i++) {
    if(!Gia_ManIsConstLit(vec->symWord[i])) {
      if(is_node_constant(ms, vec->symWord[i], 0)) {
	vec->symWord[i] = Gia_ManConst0Lit();
	fprintf(stdout, "forcing %ju=False\n", i);
      } else if(is_node_constant(ms, vec->symWord[i], 1)) {
	vec->symWord[i] = Gia_ManConst1Lit();
	fprintf(stdout, "forcing %ju=True\n", i);
      }
    }
  }
  return vec_sym_to_con_attempt(ms, vec);
}


//Functions for printing vectors

void vec_printSimple(machine_state *ms, Vector *vec) {
  uintmax_t i;
  if(vec->isSymbolic) {
    for(i = 0; i < ((vec->size-1) / (BITS_IN_BYTE / 2))+1; i++) {
      fprintf(stdout, "#");
    }
  } else {
    fprintf(stdout, "%2.2jx", int_zextend(vec->conWord, vec->size));
  }
}

void vec_print(machine_state *ms, Vector *vec) {
  uintmax_t i;
  if(vec->isSymbolic) {
    for(i = 0; i < vec->size; i++) {
      fprintf(stdout, "\nsymWord[%ju] = ", i);
      Gia_ObjPrint(ms->ntk, Gia_ObjFromLit(ms->ntk, vec->symWord[i]));
    }
  }
  fprintf(stdout, "conWord = %ju(0x%jx)\n", int_zextend(vec->conWord, vec->size), int_zextend(vec->conWord, vec->size));
  fprintf(stdout, "size    = %ju\n", vec->size);
  fprintf(stdout, "inuse   = %u\n", vec->inuse);
}

void vec_printArraySimple(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements) {
  uintmax_t i;
  for(i = 0; i < num_arr_elements; i++) {
    vec_printSimple(ms, vec_array[i]);
  }
}

void vec_printArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements) {
  uintmax_t i;
  for(i = 0; i < num_arr_elements; i++) {
    vec_print(ms, vec_array[i]);
  }
}

Vector *vec_joinArray(machine_state *ms, Vector **vec_array, uintmax_t num_arr_elements) {
  uintmax_t i, j, k;
  Vector *ret = vec_get(ms, num_arr_elements*vec_array[0]->size); //All vectors in an array are assumed to be the same size.
  
  j = 0;
  for(i = 0; i < num_arr_elements; i++) {
    if(!vec_array[i]->isSymbolic)
      vec_calc_sym(ms, vec_array[i]);
    for(k = 0; k < vec_array[i]->size; k++) {
      ret->symWord[j++] = vec_array[i]->symWord[k];
    }
  }
  
  if(!vec_sym_to_con_attempt(ms, ret))
    ret->isSymbolic = 1;
  
  return ret;
}

Vector **vec_splitIntoNewArray(machine_state *ms, Vector *vec, uintmax_t num_arr_elements) {
  uintmax_t i, j, k;
  assert(num_arr_elements > 0);
  assert(vec->size % num_arr_elements == 0);
  Vector **ret_array = vec_getArray(ms, num_arr_elements, vec->size / num_arr_elements);
  if(!vec->isSymbolic)
    vec_calc_sym(ms, vec);
  
  j = 0;
  for(i = 0; i < num_arr_elements; i++) {
    for(k = 0; k < ret_array[i]->size; k++) {
      ret_array[i]->symWord[k] = vec->symWord[j++];
    }
    
    if(!vec_sym_to_con_attempt(ms, ret_array[i]))
      ret_array[i]->isSymbolic = 1;
  }
  
  return ret_array;
}

void vec_splitIntoArray(machine_state *ms, Vector *vec, Vector **vec_array, uintmax_t num_arr_elements) {
  uintmax_t i, j, k;
  assert(num_arr_elements > 0);
  assert(vec->size % num_arr_elements == 0);
  if(!vec->isSymbolic)
    vec_calc_sym(ms, vec);
  
  j = 0;
  for(i = 0; i < num_arr_elements; i++) {
    for(k = 0; k < vec_array[i]->size; k++) {
      vec_array[i]->symWord[k] = vec->symWord[j++];
    }
    
    if(!vec_sym_to_con_attempt(ms, vec_array[i]))
      vec_array[i]->isSymbolic = 1;
  }
}

//Vector manipulation routines

uintmax_t int_zextend(uintmax_t x, uintmax_t x_size) {
  assert(x_size <= WORD_BITS);
  assert(x_size > 0);
  if(x_size == WORD_BITS) return x;
  x &= ((uintmax_t)~0)>>(WORD_BITS - x_size);   
  return x;
}

//BV-ZeroExtend
Vector *vec_zextend(machine_state *ms, Vector *x, uintmax_t result_size) {
  uintmax_t i;
  assert(result_size >= x->size);
  assert(x->size > 0);
  Vector *ret = vec_get(ms, result_size);
  
  //Concrete case
  if(!x->isSymbolic) {
    if(result_size <= WORD_BITS) {
      //Zero out upper sign bits
      ret->conWord = int_zextend(x->conWord, x->size);
      ret->isSymbolic = 0;
      return ret;
    }
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  
  memcpy(ret->symWord, x->symWord, x->size * sizeof(Gia_Lit_t ));
  
  for(i = x->size; i < result_size; i++)
    ret->symWord[i] = Gia_ManConst0Lit();
  
  ret->isSymbolic = 1;
  
  return ret;
}

uintmax_t int_sextend(uintmax_t x, uintmax_t x_size, uintmax_t result_size) {
  //get sign bit
  if(x_size == WORD_BITS) return x;
  uint8_t sign = (x >> (x_size-1))&1;
  if(sign != 0) {
    //Fill in upper sign bits
    x |= ((uintmax_t)~0)<<x_size;
  }
  //Zero out upper sign bits
  x = int_zextend(x, result_size);
  return x;
}

//BV-SignExtend
Vector *vec_sextend(machine_state *ms, Vector *x, uintmax_t result_size) {
  uintmax_t i;
  assert(result_size >= x->size);
  assert(x->size > 0);
  Vector *ret = vec_get(ms, result_size);
  
  //Concrete case
  if(!x->isSymbolic) {
    if(result_size <= WORD_BITS) {
      ret->conWord = int_sextend(x->conWord, x->size, result_size);
      ret->isSymbolic = 0;
      return ret;
    }
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  memcpy(ret->symWord, x->symWord, x->size * sizeof(Gia_Lit_t ));
  
  for(i = x->size; i < result_size; i++)
    ret->symWord[i] = ret->symWord[x->size-1];
  
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-Concatenate
Vector *vec_cat(machine_state *ms, Vector *x, Vector *y) {
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size + y->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      if(ret->size <= WORD_BITS) {
	//Concrete case
	uintmax_t y_ze = int_zextend(y->conWord, y->size);
	ret->conWord = (x->conWord << y->size) | y_ze;
	ret->isSymbolic = 0;
	return ret;
      } else {
	vec_calc_sym(ms, x);
	vec_calc_sym(ms, y);
      }
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  for(i = 0; i < y->size; i++)
    ret->symWord[i] = y->symWord[i];
  for(i = 0; i < x->size; i++)
    ret->symWord[i+y->size] = x->symWord[i];
  
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-Truncate
Vector *vec_trunc(machine_state *ms, Vector *x, uintmax_t amount) {
  assert(x->size >= amount);
  uintmax_t i;

  Vector *ret = vec_get(ms, amount);

  if(!x->isSymbolic) {
    uintmax_t mask = ((uintmax_t) ~0)>>(WORD_BITS - amount);
    ret->conWord = x->conWord & mask;
    ret->isSymbolic = 0;
    return ret;
  }
  
  uint8_t isSymbolic = 0;
  for(i = 0; i < amount; i++) {
    ret->symWord[i] = x->symWord[i];
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;
}

//BV-Extract
Vector *vec_extract(machine_state *ms, Vector *x, uintmax_t start, uintmax_t amount) {
  assert(start < x->size);
  assert(amount < x->size);
  assert(start + amount < x->size);
  uintmax_t i;

  Vector *ret = vec_get(ms, amount);
  
  if(!x->isSymbolic) {
    uintmax_t mask = ((uintmax_t)~0)>>(WORD_BITS - amount);
    ret->conWord = (x->conWord>>start) & mask;
    ret->isSymbolic = 0;
    return ret;
  }
  
  uint8_t isSymbolic = 0;
  for(i = 0; i < amount; i++) {
    ret->symWord[i] = x->symWord[start+i];
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;
}

//BV-AND
Vector *vec_and(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      ret->conWord = x->conWord & y->conWord;
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);      
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  uint8_t isSymbolic = (ret->size > WORD_BITS);
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Gia_ManHashAnd(ms->ntk, x->symWord[i], y->symWord[i]);
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;
}

//BV-OR
Vector *vec_or(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      ret->conWord = x->conWord | y->conWord;
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);      
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  uint8_t isSymbolic = (ret->size > WORD_BITS);
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Gia_ManHashOr(ms->ntk, x->symWord[i], y->symWord[i]);
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;
}

//BV-XOR
Vector *vec_xor(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      ret->conWord = x->conWord ^ y->conWord;
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);      
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  uint8_t isSymbolic = (ret->size > WORD_BITS);
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Gia_ManHashXor(ms->ntk, x->symWord[i], y->symWord[i]);
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;
}

//BV-Equality Test - returns 0 if x and y are not equivalent, 1 if they are equivalent, and 2 if unknown
uint8_t vec_sym_equal(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t y_ze = int_zextend(y->conWord, y->size);
      if(x_ze == y_ze) return 1;
      else return 0;
    } else {
      vec_calc_sym(ms, x);      
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  uint8_t are_equal = 1;
  for(i = 0; i < x->size; i++) {
    if(Abc_LitNot(x->symWord[i]) == y->symWord[i]) return 0;
    if(x->symWord[i] == y->symWord[i]) continue;
    else are_equal = 2; //not concrete
  }
  
  return are_equal;	  
}

//BV-Equality Test - returns a single AIG node
Gia_Lit_t vec_equal(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  intmax_t i; //Must be a signed integer
  
  //Concrete case
  uint8_t equal = vec_sym_equal(ms, x, y);
  if(equal == 0) return Gia_ManConst0Lit();
  else if(equal == 1) return Gia_ManConst1Lit();
  else assert(equal == 2);
  
  //Symbolic case
  Gia_Lit_t ret = Gia_ManConst1Lit();
  
  for(i = x->size-1; i >=0; i--) {
    ret = Gia_ManHashAnd(ms->ntk, ret, Abc_LitNot(Gia_ManHashXor(ms->ntk, x->symWord[i], y->symWord[i])));
  }
  
  return ret;
}

//BV-Equality Test - returns a single AIG node, potentially strengthened by a SAT solver
Gia_Lit_t vec_equal_SAT(machine_state *ms, Vector *x, Vector *y, uint8_t call_SAT_solver) {
  assert(x->size == y->size);
  intmax_t i; //Must be a signed integer
  
  //Concrete case
  uint8_t equal = vec_sym_equal(ms, x, y);
  if(equal == 0) return Gia_ManConst0Lit();
  else if(equal == 1) return Gia_ManConst1Lit();
  else assert(equal == 2);

  //Symbolic case
  Gia_Lit_t ret = Gia_ManConst1Lit();
  
  for(i = x->size-1; i >=0; i--) {
    ret = Gia_ManHashAnd(ms->ntk, ret, Abc_LitNot(Gia_ManHashXor(ms->ntk, x->symWord[i], y->symWord[i])));
  }
  
  if(!Gia_ManIsConstLit(ret)) {
    if(call_SAT_solver) {
      if(is_node_constant(ms, ret, 0)) {
	ret = Gia_ManConst0Lit();
      } else if(is_node_constant(ms, ret, 1)) {
	ret = Gia_ManConst1Lit();
      }
    }
  }
  
  return ret;
}

//BV-Negate / two's complement
Vector *vec_negate(machine_state *ms, Vector *x) {
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    //Concrete case
    ret->conWord = -(int_sextend(x->conWord, x->size, WORD_BITS));
    ret->isSymbolic = 0;
    return ret;
  }
  
  Gia_Lit_t invert = Gia_ManConst0Lit();
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Gia_ManHashMux(ms->ntk, invert,
				 Abc_LitNot(x->symWord[i]),
				 x->symWord[i]);
    invert = Gia_ManHashOr(ms->ntk, invert, x->symWord[i]);      
  }
  
  ret->isSymbolic = 1;
  return ret;
}

//BV-Invert
Vector *vec_invert(machine_state *ms, Vector *x) {
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    //Concrete case
    ret->conWord = int_zextend(~x->conWord, x->size);
    ret->isSymbolic = 0;
    return ret;
  }
  
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Abc_LitNot(x->symWord[i]);
  }
  
  ret->isSymbolic = 1;
  return ret;
}

//BV-ITE
Vector *vec_ite(machine_state *ms, Gia_Lit_t c, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(Gia_ManIsConstLit(c)) {
    //Concrete condition case
    if(Gia_ManIsConst0Lit(c))
      vec_copy(ms, ret, y);
    else vec_copy(ms, ret, x);
    return ret;
  }
  
  //Concrete vector case
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      if(x->conWord == y->conWord) {
	vec_copy(ms, ret, x);
	return ret;
      }
      vec_calc_sym(ms, y);
    }
    vec_calc_sym(ms, x);
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Gia_ManHashMux(ms->ntk, c, x->symWord[i], y->symWord[i]);
  }
  
  ret->isSymbolic = 1;
  
  return ret;   
}

//BV-AbsoluteValue
Vector *vec_abs(machine_state *ms, Vector *x) {
  Vector *ret;
  
  if(!x->isSymbolic) {
    //Concrete case
    uint8_t sign = (x->conWord >> (x->size-1))&1;
    if(sign == 0) {
      ret = vec_get(ms, x->size);
      vec_copy(ms, ret, x);
    } else {
      ret = vec_negate(ms, x);
    }
    return ret;              
  }
  
  //Symbolic case
  Gia_Lit_t sign = x->symWord[x->size-1];
  Vector *x_inv = vec_negate(ms, x);
  
  ret = vec_ite(ms, sign, x_inv, x);
  vec_release(ms, x_inv);
  
  return ret;   
}

//BV-GreaterThan
Gia_Lit_t vec_greaterthan(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  intmax_t i; //Must be a signed integer
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t y_ze = int_zextend(y->conWord, y->size);
      if(x_ze > y_ze) return Gia_ManConst1Lit();
      else return Gia_ManConst0Lit();
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  Gia_Lit_t known = Gia_ManConst0Lit();
  Gia_Lit_t ret = Gia_ManConst0Lit();
  
  for(i = (x->size-1); i >= 0; i--) {
    ret = Gia_ManHashMux(ms->ntk, known, ret,
		     Gia_ManHashAnd(ms->ntk, x->symWord[i], Abc_LitNot(y->symWord[i])));
    known = Gia_ManHashOr(ms->ntk, known, Gia_ManHashXor(ms->ntk, x->symWord[i], y->symWord[i]));
    if(Gia_ManIsConst1Lit(known))
      break;
  }
  
  return ret;
}

//BV-SignedGreaterThan
Gia_Lit_t vec_signed_greaterthan(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      intmax_t x_ze = (intmax_t) int_sextend(x->conWord, x->size, WORD_BITS);
      intmax_t y_ze = (intmax_t) int_sextend(y->conWord, y->size, WORD_BITS);
      if(x_ze > y_ze) return Gia_ManConst1Lit();
      else return Gia_ManConst0Lit();
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Concrete sign case
  Gia_Lit_t sign_x = x->symWord[x->size-1];
  Gia_Lit_t sign_y = y->symWord[y->size-1];
  
  if(Gia_ManIsConstLit(sign_x) && Gia_ManIsConstLit(sign_y)) {
    if(Gia_ManIsConst1Lit(sign_x)) {
      if(Gia_ManIsConst1Lit(sign_y)) {
	//Both x and y are negative
	Vector *x_abs = vec_abs(ms, x);
	Vector *y_abs = vec_abs(ms, y);
	Gia_Lit_t ret = vec_greaterthan(ms, y_abs, x_abs);
	vec_release(ms, x_abs);
	vec_release(ms, y_abs);
	return ret;
      } else {
	//x is negative and y is positive
	return Gia_ManConst0Lit();
      }         
    } else if(Gia_ManIsConst1Lit(sign_y)) {
      //x is positive and y is negative
      return Gia_ManConst1Lit();
    } else {      
      //Both x and y are positive
      return vec_greaterthan(ms, x, y);
    }
  }
  
  //Symbolic case where signs are concretely either equal or not equal
  Gia_Lit_t signs_opposite = Gia_ManHashXor(ms->ntk, sign_x, sign_y);
  if(Gia_ManIsConstLit(signs_opposite)) {
    if(Gia_ManIsConst0Lit(signs_opposite)) {
      //Both positive
      Gia_Lit_t both_pos = vec_greaterthan(ms, x, y);
      //Both negative
      Vector *x_abs = vec_abs(ms, x);
      Vector *y_abs = vec_abs(ms, y);
      Gia_Lit_t both_neg = vec_greaterthan(ms, y_abs, x_abs);
      vec_release(ms, x_abs);
      vec_release(ms, y_abs);
      return Gia_ManHashMux(ms->ntk, sign_x, both_neg, both_pos);
    } else {
      return Abc_LitNot(sign_x);
    }
  }
  
  //Fully symbolic case
  Gia_Lit_t both_pos = vec_greaterthan(ms, x, y);
  
  Vector *x_abs = vec_abs(ms, x);
  Vector *y_abs = vec_abs(ms, y);
  Gia_Lit_t both_neg = vec_greaterthan(ms, y_abs, x_abs);
  vec_release(ms, x_abs);
  vec_release(ms, y_abs);
  
  return Gia_ManHashMux(ms->ntk, signs_opposite,
		    Abc_LitNot(sign_x),
		    Gia_ManHashMux(ms->ntk, sign_x, both_neg, both_pos));
}

//BV-LessThan
Gia_Lit_t vec_lessthan(machine_state *ms, Vector *x, Vector *y) {
  return vec_greaterthan(ms, y, x);
}

//BV-SignedLessThan
Gia_Lit_t vec_signed_lessthan(machine_state *ms, Vector *x, Vector *y) {
  return vec_signed_greaterthan(ms, y, x);
}

//BV-ShiftRight
Vector *vec_shiftright(machine_state *ms, Vector *x, Vector *amount) {
  uintmax_t i, j;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!amount->isSymbolic) {
    if(!x->isSymbolic) {
      //Concrete case
      vec_copy(ms, ret, x);
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t amount_ze = int_zextend(amount->conWord, amount->size);
      if(amount_ze >= x->size) {
	ret->conWord = 0;
      } else {
	ret->conWord = (x_ze >> amount_ze);
      }
      return ret;         
    } else {
      //Case with concrete amount, but symbolic value
      uint8_t isSymbolic = (ret->size > WORD_BITS);
      j = 0;
      uintmax_t amount_ze = int_zextend(amount->conWord, amount->size);
      for(i = amount_ze; i < x->size; i++) {
	ret->symWord[j] = x->symWord[i];
	if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[j])) {
	  isSymbolic = 1;
	}
	j++;
      }
      for( ;j < x->size; j++) {
	ret->symWord[j] = Gia_ManConst0Lit();
      }
      
      if(!isSymbolic) 
	vec_sym_to_con(ms, ret);
      else ret->isSymbolic = 1;
      
      return ret;
    }
  } else if(!x->isSymbolic) {
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  vec_copy(ms, ret, x);
  uint8_t short_circuit = 0;
  for(i = 0; i < amount->size; i++) {
    for(j = 0; j < x->size; j++) {
      if(short_circuit || (j + (1<<i) >= x->size)) {
	ret->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i], Gia_ManConst0Lit(), ret->symWord[j]);
	if((1<<i) > x->size) short_circuit = 1;
      } else {
	ret->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i], ret->symWord[j+(1<<i)], ret->symWord[j]);
      }
    }
  }
  
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-SignedShiftRight
Vector *vec_signedshiftright(machine_state *ms, Vector *x, Vector *amount) {
  uintmax_t i, j;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!amount->isSymbolic) {
    if(!x->isSymbolic) {
      //Concrete case
      vec_copy(ms, ret, x);
      intmax_t x_se = (intmax_t) int_sextend(x->conWord, x->size, WORD_BITS);
      uintmax_t amount_ze = int_zextend(amount->conWord, amount->size);
      if(amount_ze >= x->size) {
	ret->conWord = (uintmax_t) (x_se >> (WORD_BITS-1));
      } else {
	ret->conWord = (uintmax_t) (x_se >> amount_ze);
      }
      return ret;         
    } else {
      //Case with concrete amount, but symbolic value
      uint8_t isSymbolic = (ret->size > WORD_BITS);
      j = 0;
      uintmax_t amount_ze = int_zextend(amount->conWord, amount->size);
      for(i = amount_ze; i < x->size; i++) {
	ret->symWord[j] = x->symWord[i];
	if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[j])) {
	  isSymbolic = 1;
	}
	j++;
      }
      for( ;j < x->size; j++) {
	ret->symWord[j] = x->symWord[x->size-1];
      }
      
      if(!isSymbolic) 
	vec_sym_to_con(ms, ret);
      else ret->isSymbolic = 1;
      
      return ret;
    }
  } else if(!x->isSymbolic) {
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  vec_copy(ms, ret, x);
  uint8_t short_circuit = 0;
  for(i = 0; i < amount->size; i++) {
    for(j = 0; j < x->size-1; j++) {
      if(short_circuit || (j + (1<<i) >= x->size)) {
	ret->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i], x->symWord[x->size-1], ret->symWord[j]);
	if((1<<i) > x->size) short_circuit = 1;
      } else {
	ret->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i], ret->symWord[j+(1<<i)], ret->symWord[j]);
      }
    }
  }
  
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-ShiftLeft
Vector *vec_shiftleft(machine_state *ms, Vector *x, Vector *amount) {
  uintmax_t i, j;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!amount->isSymbolic) {
    if(!x->isSymbolic) {
      //Concrete case
      vec_copy(ms, ret, x);
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t amount_ze = int_zextend(amount->conWord, amount->size);
      if(amount_ze >= x->size) {
	ret->conWord = 0;
      } else {
	ret->conWord = (x_ze << amount_ze);
      }
      return ret;         
    } else {
      //Case with concrete amount, but symbolic value
      uint8_t isSymbolic = (ret->size > WORD_BITS);
      j = 0;
      uintmax_t amount_ze = int_zextend(amount->conWord, amount->size);
      for(j = 0; (j < amount_ze) && (j < x->size); j++) {
	ret->symWord[j] = Gia_ManConst0Lit();
      }
      for(i = 0; j < x->size; i++) {
	ret->symWord[j] = x->symWord[i];
	if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[j])) {
	  isSymbolic = 1;
	}
	j++;            
      }
      
      if(!isSymbolic)
	vec_sym_to_con(ms, ret);
      else ret->isSymbolic = 1;
      
      return ret;
    }
  } else if(!x->isSymbolic) {
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  vec_copy(ms, ret, x);
  uint8_t short_circuit = 0;
  for(i = 0; i < amount->size; i++) {
    for(j = (x->size-1); j != ~0; j--) {
      if(short_circuit || ((1<<i) > j)) {
	ret->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i], Gia_ManConst0Lit(), ret->symWord[j]);
	if((1<<i) > x->size) short_circuit = 1;
      } else {
	ret->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i], ret->symWord[j-(1<<i)], ret->symWord[j]);
      }
    }
  }
  
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-RotateRight
Vector *vec_rotateright(machine_state *ms, Vector *x, Vector *amount) {
  uintmax_t i, j;
  
  //large word bit-vector rotates is currently not available.
  assert(x->size <= WORD_BITS);
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!amount->isSymbolic) {
    uintmax_t amount_mod = amount->conWord % x->size;
    if(!x->isSymbolic) {
      //Concrete case
      vec_copy(ms, ret, x);
      if(amount_mod != 0) {
	uintmax_t mask = ((uintmax_t) ~0)>>(WORD_BITS - x->size);
	uintmax_t low = (mask & x->conWord) >> amount_mod; //new low order bits
	ret->conWord = ((x->conWord << (x->size - amount_mod)) | low) & mask;
      }
      return ret;
    } else {
      //Case with concrete amount, but symbolic value
      j = 0;
      for(i = amount_mod; i < x->size; i++) {
	ret->symWord[j] = x->symWord[i];
	j++;            
      }
      for(i = 0; i < amount_mod; i++) {
	ret->symWord[j] = x->symWord[i];
	j++;
      }
      ret->isSymbolic = 1;
      return ret;         
    }
  } else if(!x->isSymbolic) {
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  Vector *tmp_vec = vec_get(ms, x->size);
  vec_copy(ms, ret, x);
  for(i = 0; i < amount->size; i++) {
    for(j = 0; j < x->size; j++) {
      tmp_vec->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i],
				       ret->symWord[(j+(1<<i))%((uintmax_t)x->size)],
				       ret->symWord[j]);
    }
    vec_copy(ms, ret, tmp_vec);
  }
  vec_release(ms, tmp_vec);
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-RotateLeft
Vector *vec_rotateleft(machine_state *ms, Vector *x, Vector *amount) {
  uintmax_t i, j;
  
  //large word bit-vector rotates is currently not available.
  assert(x->size <= WORD_BITS);
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!amount->isSymbolic) {
    uintmax_t amount_mod = amount->conWord % x->size;
    if(!x->isSymbolic) {
      //Concrete case
      vec_copy(ms, ret, x);
      if(amount_mod != 0) {
	uintmax_t mask = ((uintmax_t) ~0)>>(WORD_BITS - x->size);
	uintmax_t low = (mask & x->conWord) >> (x->size - amount_mod); //new low order bits
	ret->conWord = ((x->conWord << amount_mod) | low) & mask;
      }
      return ret;
    } else {
      //Case with concrete amount, but symbolic value
      j = 0;
      for(i = (x->size - amount_mod); i < x->size; i++) {
	ret->symWord[j] = x->symWord[i];
	j++;            
      }
      for(i = amount_mod; j < x->size; i++) {
	ret->symWord[j] = x->symWord[i];
	j++;
      }
      ret->isSymbolic = 1;
      return ret;         
    }
  } else if(!x->isSymbolic) {
    vec_calc_sym(ms, x);
  }
  
  //Symbolic case
  Vector *tmp_vec = vec_get(ms, x->size);
  vec_copy(ms, ret, x);
  for(i = 0; i < amount->size; i++) {
    for(j = 0; j < x->size; j++) {
      tmp_vec->symWord[j] = Gia_ManHashMux(ms->ntk, amount->symWord[i],
				       ret->symWord[(((uintmax_t) x->size) - (1<<i) + j)%((uintmax_t) x->size)],
				       ret->symWord[j]);
    }
    vec_copy(ms, ret, tmp_vec);
  }
  vec_release(ms, tmp_vec);
  ret->isSymbolic = 1;
  
  return ret;
}

//BV-SelectBits
Vector *vec_selectBits(machine_state *ms, Vector *x, uint16_t num_bits, uint16_t bit_offset) {
  uint16_t i;
  assert((num_bits + bit_offset) <= x->size);
  
  Vector *ret = vec_get(ms, num_bits);
  
  if(!x->isSymbolic) {
    //Concrete case
    uintmax_t x_ze = int_zextend(x->conWord, x->size);
    ret->conWord = x_ze >> bit_offset;
    ret->isSymbolic = 0;
    return ret;      
  }
  
  //Symbolic case
  uint8_t isSymbolic = (ret->size > WORD_BITS);
  for(i = 0; i < num_bits; i++) {
    ret->symWord[i] = x->symWord[i + bit_offset];
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;
}

//BV-Reverse
Vector *vec_reverse(machine_state *ms, Vector *x) {
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  ret->isSymbolic = x->isSymbolic;
  
  if(!x->isSymbolic) {
    //Concrete case
    uintmax_t v = x->conWord;
    uintmax_t s = sizeof(v) * BITS_IN_BYTE;
    uintmax_t mask = ~0;
    while ((s >>= 1) > 0) {
      mask ^= (mask << s);
      v = ((v >> s) & mask) | ((v << s) & ~mask);
    }
    ret->conWord = v>>(WORD_BITS - x->size);
    ret->isSymbolic = 0;
    return ret; //SEAN!!! test this code
  }
  
  //Symbolic case
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = x->symWord[(x->size-1) - i];
  }
  
  ret->isSymbolic = 1;
  return ret;
}

//BV-Add
Vector *vec_add(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t y_ze = int_zextend(y->conWord, y->size);
      ret->conWord = x_ze + y_ze;
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  ret->isSymbolic = 1;
  Gia_Lit_t carry = Gia_ManConst0Lit(); //Carry flag is initially false
  uint8_t isSymbolic = (ret->size > WORD_BITS);
  for(i = 0; i < x->size; i++) {
    ret->symWord[i] = Gia_ManHashXor(ms->ntk, carry, Gia_ManHashXor(ms->ntk, x->symWord[i], y->symWord[i]));
    if(i < x->size-1) {
      carry = Gia_ManHashMux(ms->ntk, carry,
		       Gia_ManHashOr(ms->ntk, x->symWord[i], y->symWord[i]),
		       Gia_ManHashAnd(ms->ntk, x->symWord[i], y->symWord[i]));
    }
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;   
}

//BV-Carry
Gia_Lit_t vec_carry(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t mask = ((uintmax_t)~0)>>(WORD_BITS - x->size);
      uintmax_t sum = (x->conWord + y->conWord) & mask;
      return (sum < (x->conWord & mask) || sum < (y->conWord & mask)) ? Gia_ManConst1Lit() : Gia_ManConst0Lit();
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  Gia_Lit_t carry = Gia_ManConst0Lit(); //Carry flag is initially false
  for(i = 0; i < x->size; i++) {
    //Majority vote
    carry = Gia_ManHashMux(ms->ntk, carry,
		       Gia_ManHashOr(ms->ntk, x->symWord[i], y->symWord[i]),
		       Gia_ManHashAnd(ms->ntk, x->symWord[i], y->symWord[i]));
  }
  
  return carry;
}

//BV-Subtract
Vector *vec_sub(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t y_ze = int_zextend(y->conWord, y->size);
      ret->conWord = x_ze - y_ze;
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  Gia_Lit_t borrow = Gia_ManConst0Lit(); //borrow flag is initially false
  uint8_t isSymbolic = (ret->size > WORD_BITS);
  for(i = 0; i < x->size; i++) {
    Gia_Lit_t top_bit = Gia_ManHashMux(ms->ntk, borrow,
				    Abc_LitNot(x->symWord[i]),
				    x->symWord[i]);
    borrow = Gia_ManHashMux(ms->ntk, x->symWord[i],
			Gia_ManHashAnd(ms->ntk, borrow, y->symWord[i]),
			Gia_ManHashOr(ms->ntk, borrow, y->symWord[i]));
    ret->symWord[i] = Gia_ManHashXor(ms->ntk, top_bit, y->symWord[i]);
    if(!isSymbolic && !Gia_ManIsConstLit(ret->symWord[i])) {
      isSymbolic = 1;
    }
  }
  
  if(!isSymbolic)
    vec_sym_to_con(ms, ret);
  else ret->isSymbolic = 1;
  
  return ret;   
}

//BV-Multiply
Vector *vec_mult(machine_state *ms, Vector *x, Vector *y) {
  assert(x->size == y->size);
  uintmax_t i, j;
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t y_ze = int_zextend(y->conWord, y->size);
      ret->conWord = x_ze * y_ze;
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  //Symbolic case
  vec_setValue(ms, ret, 0);
  ret->isSymbolic = 1;
  
  Vector *tmp_vec = vec_get(ms, ret->size);
  vec_copy(ms, tmp_vec, ret);
  
  for(j = 0; j < x->size; j++) {
    if(Gia_ManIsConst0Lit(y->symWord[j]))
      continue;
    
    //mini BV-Add
    Gia_Lit_t carry = Gia_ManConst0Lit(); //Carry flag is initially false
    for(i = 0; i < (x->size - j); i++) {
      tmp_vec->symWord[i+j] = Gia_ManHashXor(ms->ntk, carry, Gia_ManHashXor(ms->ntk, x->symWord[i], ret->symWord[i+j]));
      //Majority vote
      carry = Gia_ManHashMux(ms->ntk, carry,
			 Gia_ManHashOr(ms->ntk, x->symWord[i], ret->symWord[i+j]),
			 Gia_ManHashAnd(ms->ntk, x->symWord[i], ret->symWord[i+j]));
    }
    
    if(Gia_ManIsConst1Lit(y->symWord[j])) {
      vec_copy(ms, ret, tmp_vec);
    } else {
      for(i = 0; i < (x->size - j); i++) {
	ret->symWord[i+j] = Gia_ManHashMux(ms->ntk, y->symWord[j],
				       tmp_vec->symWord[i+j],
				       ret->symWord[i+j]);
      }
    }
  }
  
  if(!vec_sym_to_con_attempt(ms, ret))
    ret->isSymbolic = 1;
  
  vec_release(ms, tmp_vec);
  
  return ret;
}

//BV-Quotient|Remainder
Vector *vec_quot_rem(machine_state *ms, Vector *x, Vector *y, uint8_t quot_rem) {
  assert(x->size == y->size);
  intmax_t i, j; //Must be signed integers
  
  Vector *ret = vec_get(ms, x->size);
  
  if(!x->isSymbolic) {
    if(!y->isSymbolic) {
      //Concrete case
      uintmax_t x_ze = int_zextend(x->conWord, x->size);
      uintmax_t y_ze = int_zextend(y->conWord, y->size);
      if(y_ze == 0) ret->conWord = quot_rem ? -1 : x_ze;
      else ret->conWord = quot_rem ? (x_ze/y_ze) : (x_ze%y_ze);
      ret->isSymbolic = 0;
      return ret;
    } else {
      vec_calc_sym(ms, x);
    }
  } else if(!y->isSymbolic) {
    vec_calc_sym(ms, y);
  }
  
  vec_copy(ms, ret, x);
  ret->isSymbolic = 1;
  
  Vector *quot_vec = vec_get(ms, x->size);
  quot_vec->isSymbolic = 1;
  
  Vector *tmp_vec = vec_get(ms, x->size);
  tmp_vec->isSymbolic = 1;
  
  for(j = (x->size-1); j >= 0; j--) {
    Gia_Lit_t known = Gia_ManConst0Lit();
    for(i = (x->size-1); i > ((x->size-1) - j); i--) {
      known = Gia_ManHashOr(ms->ntk, known, y->symWord[i]);
      if(Gia_ManIsConst1Lit(known))
	break;
    }
    quot_vec->symWord[j] = known;
    
    for(i = (x->size-1); i >= 0; i--) {
      if(Gia_ManIsConst1Lit(known))
	break;
      Gia_Lit_t y_bit = (i >= j) ? y->symWord[i-j] : Gia_ManConst0Lit();
      quot_vec->symWord[j] = Gia_ManHashMux(ms->ntk, known,
					quot_vec->symWord[j],
					Gia_ManHashAnd(ms->ntk, y_bit, Abc_LitNot(ret->symWord[i])));
      known = Gia_ManHashOr(ms->ntk, known, Gia_ManHashXor(ms->ntk, y_bit, ret->symWord[i]));
    }
    
    quot_vec->symWord[j] = Abc_LitNot(quot_vec->symWord[j]);
    
    if(Gia_ManIsConst0Lit(quot_vec->symWord[j]))
      continue;
    
    Gia_Lit_t borrow = Gia_ManConst0Lit(); //borrow flag is initially false
    for(i = 0; i < x->size; i++) {
      Gia_Lit_t top_bit = Gia_ManHashMux(ms->ntk, borrow,
				      Abc_LitNot(ret->symWord[i]),
				      ret->symWord[i]);
      Gia_Lit_t y_bit = (i >= j) ? y->symWord[i-j] : Gia_ManConst0Lit();
      borrow = Gia_ManHashMux(ms->ntk, ret->symWord[i],
			  Gia_ManHashAnd(ms->ntk, borrow, y_bit),
			  Gia_ManHashOr(ms->ntk, borrow, y_bit));
      tmp_vec->symWord[i] = Gia_ManHashXor(ms->ntk, top_bit, y_bit);
    }
    
    if(Gia_ManIsConst1Lit(quot_vec->symWord[j])) {
      vec_copy(ms, ret, tmp_vec);
    } else {
      for(i = 0; i < x->size; i++) {
	ret->symWord[i] = Gia_ManHashMux(ms->ntk, quot_vec->symWord[j],
				     tmp_vec->symWord[i],
				     ret->symWord[i]);
      }
    }
  }
  
  vec_release(ms, tmp_vec);
  
  if(quot_rem) {
    vec_release(ms, ret);
    vec_sym_to_con_attempt(ms, quot_vec);
    return quot_vec;
  }
  
  vec_release(ms, quot_vec);
  vec_sym_to_con_attempt(ms, ret);
  
  return ret;   
}

void print_AIG(machine_state *ms, char *filename, uint8_t clean_first) {
  assert(filename != NULL);
  //Cleaning can help make a smaller AIG, but can destroy AIG nodes
  //needed by the machine_state. So, only clean if the machine_state
  //is no longer needed.
  //if(clean_first==1) Abc_AigCleanup(ms->ntk->pManFunc);
  //if(clean_first==2) { Abc_AigCleanup(ms->ntk->pManFunc); cut_sweep(ms); }
  //Io_WriteAiger(ms->ntk, filename, 0, 0, 0);

  /*
  int i;
  fprintf(stdout, "Printing Probes\n");
  for(i = 0; i < Vec_IntSize(ms->pOutputProbes); i++) {
    fprintf(stdout, "%d %d\n", Vec_IntEntry(ms->pOutputProbes, i), Gia_SweeperProbeLit(ms->ntk, Vec_IntEntry(ms->pOutputProbes, i)));
  }
  Gia_ManPrint(ms->ntk);
  fprintf(stdout, "\n=======\n");
  */

  Gia_Man_t *tmp_gia_ntk = Gia_SweeperExtractUserLogic(ms->ntk, ms->pOutputProbes, ms->ntk->vNamesIn, ms->pOutputNames);

  //Gia_SweeperFraig(ms->ntk, ms->pOutputProbes, "&syn4 -v", 1, 100, 1);

  //Gia_ManPrint(tmp_gia_ntk);
  Gia_AigerWrite(tmp_gia_ntk, filename, 0, 0, 0);
  Gia_ManStop(tmp_gia_ntk);
  //fprintf(stdout, "\n------*****\n\n");
}

void machine_state_update_probes(machine_state *ms) {
  //Update probes for all literals in memory
  arr_stack_push(ms->memories_stack, (void *)&ms->memory);
  ms->CurrsMemFlag++;
  uintmax_t head = ms->memories_stack->head;
  while(head!=0) {
    memTuple *memories = (memTuple *)ms->memories_stack->mem[head];
    cMemory_update_probes(ms, memories->cMem);
    sMemory_compress(ms, memories->sMem);
    sMemory_update_probes(ms, memories->sMem);
    if(head == ms->memories_stack->head) {
      cMemory_collect_probes(ms, memories->cMem); 
      sMemory_collect_probes(ms, memories->sMem);
    }
    head--;
  }
}

void machine_state_update_from_probes(machine_state *ms) {
  //Update literals in memory from probes
  uintmax_t head = ms->memories_stack->head;
  memTuple *memories = (memTuple *)ms->memories_stack->mem[head];
  cMemory_update_from_probes(ms, memories->cMem);
  sMemory_update_from_probes(ms, memories->sMem);
  
  arr_stack_pop(ms->memories_stack);
}

//AIG cleanup / garbage collection routines
Gia_Lit_t garbage_collect_ntk(machine_state *ms, Gia_Lit_t cond, uint8_t force_gc) {
  uintmax_t nNumMaxObjects = Gia_ManObjNum(ms->ntk);
  //uintmax_t nNumMaxObjects = Abc_NtkObjNumMax(ms->ntk);

  //Don't garbage collect all of the time.
  if(force_gc || nNumMaxObjects > ms->nNodes_last) {
    fprintf(stdout, "GC...");

    /*
    //remove conds - temporarily.
    uintmax_t i;
    for(i = 1; i <= ms->conditions_stack->head; i++) {
      Gia_SweeperCondPop(ms->ntk);
    }
    */

    assert(Vec_IntSize(ms->gc_probes) == 0);
    machine_state_update_probes(ms);
    Gia_Probe_t condProbe = get_probe_from_lit(ms, cond);
    //collect_probe(ms, condProbe);
    fprintf(stdout, "Condition = %d\n", cond);

    //Gia_SweeperSweep(ms->ntk, ms->pOutputProbes, 1, 1000, 1);
    //ms->ntk = Gia_SweeperSweep(ms->ntk, ms->gc_probes, 1, 1000, 1);
    //Gia_SweeperFraig(ms->ntk, ms->gc_probes, "&dc2 -v; &dc2 -v; &dc2 -v", 1, 1000, 1);
    //Gia_SweeperFraig(ms->ntk, ms->gc_probes, NULL, 1, 1000, 1);
    //Gia_SweeperFraig(ms->ntk, ms->pOutputProbes, NULL, 1, 1000, 1);
    //Vec_Int_t *new_probes = Gia_SweeperFraig(ms->ntk, ms->gc_probes, NULL, 1, 1000, 1);
    //int success = Gia_SweeperFraig(ms->ntk, ms->gc_probes, "&dc2 -v; &dc2 -v", 1, 100, 1);

    //Verify
    //int success = Gia_SweeperFraig(ms->ntk, ms->gc_probes, "&syn4 -v", 1, 1000, 1, 1);

    //No Verify
    int success = Gia_SweeperFraig(ms->ntk, ms->gc_probes, "&syn4 -v", 1, 1000, 0, 1);

    //To Dump errors
    //void Gia_SweeperLogicDump(ms->ntk, ms->gc_probes, 1, "sweeper_cond_dump_error.aig");

    //int success = Gia_SweeperFraig(ms->ntk, ms->gc_probes, NULL, 1, 100, 1);

    int nodes_before = Gia_ManAndNum(ms->ntk);

    //int success = Gia_SweeperFraig(ms->ntk, ms->gc_probes, NULL, 1, 0, 1);
    //int success = Gia_SweeperRun(ms->ntk, ms->gc_probes, "&dc2 -v", 1); //For Debugging
    if(success != 1) {
      fprintf(stderr, "Sweeper failed...exiting\n");
      exit(0);
    }

    int nodes_after = Gia_ManAndNum(ms->ntk);

    fprintf(stdout, "Sweeper Node reduction: %d\n", nodes_before - nodes_after);
    if(nodes_before < nodes_after) {
      fprintf(stderr, "Sweeper Adding Nodes (before: %d vs after: %d \n", nodes_before, nodes_after);
      //assert(0);
    }

    while(Vec_IntSize(ms->gc_probes) > 0) {
      //Gia_Probe_t p = 
	Vec_IntPop(ms->gc_probes);
      //fprintf(stdout, "[%d -> %d]", p, get_lit_from_probe(ms, p));
    }

    machine_state_update_from_probes(ms);
    cond = get_lit_from_probe(ms, condProbe);
    probe_free(ms, condProbe);
    
    /*
    //add conditions back
    for(i = 1; i <= ms->conditions_stack->head; i++) {
      condition *c = (condition *)ms->conditions_stack->mem[i];
      Gia_Probe_t pProbeId = Gia_SweeperProbeFind(ms->ntk, Abc_LitNotCond(c->node, c->value==0));
      Gia_SweeperProbeDeref(ms->ntk, pProbeId);
      Gia_SweeperCondPush(ms->ntk, pProbeId);
    }
    */
    ms->nNodes_last = Gia_ManObjNum(ms->ntk) + ms->nNodes_increment;
    //ms->nNodes_last = Abc_NtkObjNumMax(ms->ntk) + ms->nNodes_increment;
  }
  
  return cond;
}

