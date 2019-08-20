#include "pcode_definitions.h"

uintmax_t abc_started = 0;
uint32_t random_seed = 0;

//address_size is the number of bits needed to represent an address, i.e. 32, 64.
machine_state *machine_state_init(char *network_name, uintmax_t cmem_base_address, uintmax_t cmem_size, uintmax_t ho, uint8_t address_size) {
  uintmax_t i;
  machine_state *ms = malloc(sizeof(*ms));

  if(abc_started == 0) {
    ms->ntk = Gia_SweeperStart(NULL);

    //Start random number generator
    if(random_seed == 0) {
      gettimeofday(&tv1, &tzp1);
      random_seed = ((tv1.tv_sec & 0177) * 1000000) + tv1.tv_usec;
    }
    //fprintf(stdout, "Random seed = %u\n", random_seed);
    srandom(random_seed);
    srand(random_seed);

    Gia_SweeperSetConflictLimit(ms->ntk, 0); //No conflict limit
    Gia_SweeperSetRuntimeLimit(ms->ntk, 0);  //No time limit
    
    ms->pOutputProbes = Vec_IntAlloc(0);
    ms->pOutputNames = Vec_PtrAlloc(0);
    ms->gc_probes = Vec_IntAlloc(0);
  }
  abc_started++;
  ms->ntk->pName = Abc_UtilStrsav(network_name);
  
  //Initialize the vector stack
  fprintf(stdout, "Initializing Stacks\n");
  ms->vecs_allocated = 0;
  ms->vecs_in_stack = 0;
  ms->vectorStack_size = 64; //Start w/ a stack of vectors of size 1..64 bits
  ms->vectorStack = (void_arr_stack **)malloc(ms->vectorStack_size * sizeof(void_arr_stack *));
  for(i = 0; i < ms->vectorStack_size; i++)
    ms->vectorStack[i] = arr_stack_init();
 
  ms->vec_zero_byte = vec_get(ms, BITS_IN_BYTE);
  ms->vec_zero_byte->conWord = 0;
  ms->vec_zero_byte->isSymbolic = 0;
  ms->vec_zero_byte_probes = get_probes_from_vec(ms, ms->vec_zero_byte);

  ms->conditions_stack = arr_stack_init();
 
  ms->nNodes_last = 1000;
  ms->nNodes_increment = 1000;

  ms->branch_error = 0;
  ms->stackframe_depth = 20;

  ms->heap_offset = vec_getConstant(ms, ho, address_size);
  ms->heap_offset_probes = get_probes_from_vec(ms, ms->heap_offset);

  ms->CurrsMemFlag = 0;
  ms->sMemStack = arr_stack_init();
  ms->sMemInitStack = arr_stack_init();
  ms->sMemDeleteStack = arr_stack_init();
  ms->memories_stack = arr_stack_init();
  ms->sMemory_auto_compress = 1;

  ms->memory.cMem = cMemory_init(ms, cmem_base_address, cmem_size);
  ms->memory.sMem = sMemory_init(ms, address_size);

  return ms;
}

void machine_state_free(machine_state *ms) {
  uintmax_t i;

  cMemory_free(ms, ms->memory.cMem);
  sMemory_free(ms, ms->memory.sMem);

  assert(ms->memories_stack->head == 0);
  arr_stack_free(ms->memories_stack);

  while(ms->sMemStack->head != 0)
    vec_free(ms, (Vector *)arr_stack_pop(ms->sMemStack));
  arr_stack_free(ms->sMemStack);
  
  if(ms->sMemInitStack->head!=0)
    fprintf(stderr, "Warning: %ju sMemories not free'd\n", ms->sMemInitStack->head);
  arr_stack_free(ms->sMemInitStack);
  
  if(ms->sMemDeleteStack->head != 0)
    fprintf(stderr, "Warning: %ju sMemories still in delete stack\n", ms->sMemDeleteStack->head);
  arr_stack_free(ms->sMemDeleteStack);

  probes_free(ms, ms->heap_offset_probes, ms->heap_offset->size);
  vec_release(ms, ms->heap_offset);

  if(ms->conditions_stack->head != 0)
    fprintf(stderr, "Warning: %ju conditions sill in conditions stack\n", ms->conditions_stack->head);
  arr_stack_free(ms->conditions_stack);

  probes_free(ms, ms->vec_zero_byte_probes, ms->vec_zero_byte->size);
  vec_release(ms, ms->vec_zero_byte);
  
  fprintf(stdout, "Freeing Stacks\n");
  
  if(ms->vecs_allocated != ms->vecs_in_stack)
    fprintf(stderr, "Warning: %ju Vectors were not released\n", ms->vecs_allocated - ms->vecs_in_stack);
  
  for(i = 0; i < ms->vectorStack_size; i++) {
    while(ms->vectorStack[i]->head!=0)
      vec_free(ms, (Vector *)arr_stack_pop(ms->vectorStack[i]));
    arr_stack_free(ms->vectorStack[i]);
  }
  free(ms->vectorStack);
  
  assert(ms->stackframe_depth == 20);

  abc_started--;
  if(abc_started == 0) {
    vec_removeLastNOutputs(ms, Vec_IntSize(ms->pOutputProbes));
    assert(Vec_IntSize(ms->pOutputProbes) == 0);
    assert(Vec_PtrSize(ms->pOutputNames) == 0);
    Vec_IntFree(ms->pOutputProbes);
    Vec_PtrFree(ms->pOutputNames);

    Vec_IntFree(ms->gc_probes);

    Vec_Int_t *probeIds = Gia_SweeperCollectValidProbeIds(ms->ntk);
    assert(Vec_IntSize(probeIds) == 0); //All probe ids created have been deleted
    Vec_IntFree(probeIds);

    Gia_SweeperPrintStats(ms->ntk);
    Gia_SweeperStop(ms->ntk);
    Gia_ManStop(ms->ntk);
  }
  
  free(ms);
}

//pcode function definitions

Vector *pCOPY(machine_state *ms, Vector *input0) {
  return vec_dup(ms, input0);
}

Vector *pINT_ADD(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_add(ms, input0, input1);
}

Vector *pINT_SUB(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_sub(ms, input0, input1);
}

Vector *pINT_MULT(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_mult(ms, input0, input1);
}

Vector *pINT_DIV(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_quot_rem(ms, input0, input1, 1);
}

Vector *pINT_REM(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_quot_rem(ms, input0, input1, 0);
}

Vector *pINT_SDIV(machine_state *ms, Vector *input0, Vector *input1) {
  assert(input0->size == input1->size);
  Vector *output;
  
  if(!input0->isSymbolic) {
    if(!input1->isSymbolic) {
      //Concrete case
      output = vec_get(ms, input0->size);
      intmax_t input0_se = int_sextend(input0->conWord, input0->size, WORD_BITS);
      intmax_t input1_se = int_sextend(input1->conWord, input1->size, WORD_BITS);
      if(input1_se == 0) output->conWord = 0;
      else output->conWord = input0_se / input1_se;
      output->isSymbolic = 0;
      return output;
    } else {
      vec_calc_sym(ms, input0);
    }
  } else if(!input1->isSymbolic) {
    if(input1->conWord == 0) {
      output = vec_getConstant(ms, 0, input0->size);
      return output;
    }
    vec_calc_sym(ms, input1);
  }
  
  //Symbolic case
  Vector *vec_zero = vec_getConstant(ms, 0, input0->size);
  Gia_Lit_t zerotest, sign, sign0, sign1;
  
  zerotest = vec_equal(ms, input1, vec_zero);
  if(Gia_ManIsConst1Lit(zerotest)) return vec_zero;
  
  sign0 = input0->symWord[input0->size-1];
  sign1 = input1->symWord[input1->size-1];
  sign = Gia_ManHashXor(ms->ntk, sign0, sign1);
  
  Vector *input0_abs = vec_abs(ms, input0);
  Vector *input1_abs = vec_abs(ms, input1);
  Vector *quotient = vec_quot_rem(ms, input0_abs, input1_abs, 1);
  Vector *negated = vec_negate(ms, quotient);
  Vector *intermediate = vec_ite(ms, sign, negated, quotient);
  output = vec_ite(ms, zerotest, vec_zero, intermediate);
  
  vec_release(ms, intermediate);
  vec_release(ms, negated);
  vec_release(ms, quotient);
  vec_release(ms, input1_abs);
  vec_release(ms, input0_abs);
  vec_release(ms, vec_zero);
  
  return output;   
}

Vector *pINT_SREM(machine_state *ms, Vector *input0, Vector *input1) {
  assert(input0->size == input1->size);
  Vector *output;
  
  if(!input0->isSymbolic) {
    if(!input1->isSymbolic) {
      //Concrete case
      output = vec_get(ms, input0->size);
      intmax_t input0_se = int_sextend(input0->conWord, input0->size, WORD_BITS);
      intmax_t input1_se = int_sextend(input1->conWord, input1->size, WORD_BITS);
      if(input1_se == 0) output->conWord = input0->conWord;
      else output->conWord = input0_se % input1_se;
      output->isSymbolic = 0;
      return output;
    } else {
      vec_calc_sym(ms, input0);
    }
  } else if(!input1->isSymbolic) {
    if(input1->conWord == 0) {
      output = vec_getConstant(ms, 0, input0->size);
      return output;
    }      
    vec_calc_sym(ms, input1);
  }
  
  //Symbolic case
  Gia_Lit_t sign = input0->symWord[input0->size-1];
  
  Vector *input0_abs = vec_abs(ms, input0);
  Vector *input1_abs = vec_abs(ms, input1);
  
  Vector *temp = vec_quot_rem(ms, input0_abs, input1_abs, 0);
  Vector *negated = vec_negate(ms, temp);
  
  output = vec_ite(ms, sign, negated, temp);
  
  vec_release(ms, negated);
  vec_release(ms, temp);
  vec_release(ms, input1_abs);
  vec_release(ms, input0_abs);
  
  return output;
}

Vector *pINT_OR(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_or(ms, input0, input1);
}

Vector *pINT_XOR(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_xor(ms, input0, input1);
}

Vector *pINT_AND(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_and(ms, input0, input1);
}

Vector *pINT_LEFT(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_shiftleft(ms, input0, input1);
}

Vector *pINT_RIGHT(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_shiftright(ms, input0, input1);
}

Vector *pINT_SRIGHT(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_signedshiftright(ms, input0, input1);
}

Vector *pINT_EQUAL(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *output = vec_getConstant(ms, 0, BITS_IN_BYTE);
  output->symWord[0] = vec_equal(ms, input0, input1);
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;
}

Vector *pINT_NOTEQUAL(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *output = vec_getConstant(ms, 0, BITS_IN_BYTE);
  output->symWord[0] = Abc_LitNot(vec_equal(ms, input0, input1));
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;
}

Vector *pINT_LESS(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *output = vec_getConstant(ms, 0, BITS_IN_BYTE);
  output->symWord[0] = vec_lessthan(ms, input0, input1);

  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;
}

Vector *pINT_LESSEQUAL(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *output = vec_getConstant(ms, 0, BITS_IN_BYTE);
  output->symWord[0] = Abc_LitNot(vec_greaterthan(ms, input0, input1));
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;
}

Vector *pINT_CARRY(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *output = vec_getConstant(ms, 0, BITS_IN_BYTE);
  output->symWord[0] = vec_carry(ms, input0, input1);
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;
}

Vector *pINT_SLESS(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *output = vec_getConstant(ms, 0, BITS_IN_BYTE);
  output->symWord[0] = vec_signed_lessthan(ms, input0, input1);
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;
}

Vector *pINT_SLESSEQUAL(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *sless, *equal;
  
  sless = pINT_SLESS(ms, input0, input1);
  if(!sless->isSymbolic) {
    if(sless->conWord == 1) return sless;
    vec_release(ms, sless);
    return pINT_EQUAL(ms, input0, input1);
  }
  
  equal = pINT_EQUAL(ms, input0, input1);
  
  if(!equal->isSymbolic) {
    if(equal->conWord == 1) {
      vec_release(ms, sless);
      return equal;           
    }
    
    vec_release(ms, equal);
    return sless;
  }
  
  sless->symWord[0] = Gia_ManHashOr(ms->ntk, sless->symWord[0], equal->symWord[0]);
  vec_release(ms, equal);
  
  if(Gia_ManIsConstLit(sless->symWord[0])) {
    //Concrete case
    if(Gia_ManIsConst1Lit(sless->symWord[0])) {
      sless->conWord = 1;
    }
  } else {
    //Symbolic case
    sless->isSymbolic = 1;
  }
  
  return sless;
}

Vector *pINT_SCARRY(machine_state *ms, Vector *input0, Vector *input1) {
  uintmax_t i;
  Vector *output = vec_get(ms, BITS_IN_BYTE);
  assert(input0->size == input1->size);
  
  if(!input0->isSymbolic) {
    if(!input1->isSymbolic) {
      //Concrete case
      intmax_t input0_se = int_sextend(input0->conWord, input0->size, WORD_BITS);
      intmax_t input1_se = int_sextend(input1->conWord, input1->size, WORD_BITS);
      intmax_t sum = int_sextend(input0->conWord + input1->conWord, input0->size, WORD_BITS);
      if(input0_se > 0 && input1_se > 0 && sum <= 0)
	vec_setValue(ms, output, 1);
      else if(input0_se < 0 && input1_se < 0 && sum >= 0)
	vec_setValue(ms, output, 1);
      else 
	vec_setValue(ms, output, 0);
      return output;
    } else {
      vec_calc_sym(ms, input0);
    }
  } else if(!input1->isSymbolic) {
    vec_calc_sym(ms, input1);
  }
  
  //Symbolic case
  Gia_Lit_t carry = Gia_ManConst0Lit(); //Carry flag is initially false
  Gia_Lit_t carryin = carry;
  for(i = 0; i < input0->size; i++) {
    if(i == input0->size-1) carryin = carry;
    //Majority vote
    carry = Gia_ManHashMux(ms->ntk, carry,
		       Gia_ManHashOr(ms->ntk, input0->symWord[i], input1->symWord[i]),
		       Gia_ManHashAnd(ms->ntk, input0->symWord[i], input1->symWord[i]));
  }
 
  for(i = 1; i < BITS_IN_BYTE; i++) {
    output->symWord[i] = Gia_ManConst0Lit();
  }

  output->symWord[0] = Gia_ManHashXor(ms->ntk, carryin, carry);
 
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    output->isSymbolic = 0;
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    } else {
      output->conWord = 0;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;  
}

Vector *pINT_SBORROW(machine_state *ms, Vector *input0, Vector *input1) {
  Vector *input1c = pINT_2COMP(ms, input1);
  Vector *output = pINT_SCARRY(ms, input0, input1c);
  vec_release(ms, input1c);
  return output;
}

Vector *pBOOL_OR(machine_state *ms, Vector *input0, Vector *input1) {
  uintmax_t i;
  assert(input0->size == BITS_IN_BYTE);
  assert(input1->size == BITS_IN_BYTE);
  
  Vector *output = vec_get(ms, BITS_IN_BYTE);
  
  if(!input0->isSymbolic) {
    if(!input1->isSymbolic) {
      //Concrete case
      output->conWord = (input0->conWord | input1->conWord) & 1;
      output->isSymbolic = 0;
      return output;
    } else {
      vec_calc_sym(ms, input0);
    }
  } else if(!input1->isSymbolic) {
    vec_calc_sym(ms, input1);
  }
  
  //Symbolic case
  for(i = 1; i < output->size; i++) {
    output->symWord[i] = Gia_ManConst0Lit();
  }
  output->symWord[0] = Gia_ManHashOr(ms->ntk, input0->symWord[0], input1->symWord[0]);
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    output->isSymbolic = 0;
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    } else {
      output->conWord = 0;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;      
}

Vector *pBOOL_XOR(machine_state *ms, Vector *input0, Vector *input1) {
  uintmax_t i;
  assert(input0->size == BITS_IN_BYTE);
  assert(input1->size == BITS_IN_BYTE);
  
  Vector *output = vec_get(ms, BITS_IN_BYTE);
  
  if(!input0->isSymbolic) {
    if(!input1->isSymbolic) {
      //Concrete case
      output->conWord = (input0->conWord ^ input1->conWord) & 1;
      output->isSymbolic = 0;
      return output;
    } else {
      vec_calc_sym(ms, input0);
    }
  } else if(!input1->isSymbolic) {
    vec_calc_sym(ms, input1);
  }
  
  //Symbolic case
  for(i = 1; i < output->size; i++) {
    output->symWord[i] = Gia_ManConst0Lit();
  }
  output->symWord[0] = Gia_ManHashXor(ms->ntk, input0->symWord[0], input1->symWord[0]);
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    output->isSymbolic = 0;
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    } else {
      output->conWord = 0;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;      
}

Vector *pBOOL_AND(machine_state *ms, Vector *input0, Vector *input1) {
  uintmax_t i;
  assert(input0->size == BITS_IN_BYTE);
  assert(input1->size == BITS_IN_BYTE);
  
  Vector *output = vec_get(ms, BITS_IN_BYTE);
  
  if(!input0->isSymbolic) {
    if(!input1->isSymbolic) {
      //Concrete case
      output->conWord = (input0->conWord & input1->conWord) & 1;
      output->isSymbolic = 0;
      return output;
    } else {
      vec_calc_sym(ms, input0);
    }
  } else if(!input1->isSymbolic) {
    vec_calc_sym(ms, input1);
  }
  
  //Symbolic case
  for(i = 1; i < output->size; i++) {
    output->symWord[i] = Gia_ManConst0Lit();
  }
  output->symWord[0] = Gia_ManHashAnd(ms->ntk, input0->symWord[0], input1->symWord[0]);
  
  if(Gia_ManIsConstLit(output->symWord[0])) {
    //Concrete case
    output->isSymbolic = 0;
    if(Gia_ManIsConst1Lit(output->symWord[0])) {
      output->conWord = 1;
    } else {
      output->conWord = 0;
    }
  } else {
    //Symbolic case
    output->isSymbolic = 1;
  }
  
  return output;      
}

Vector *pBOOL_NEGATE(machine_state *ms, Vector *input0) {
  uintmax_t i;
  assert(input0->size == BITS_IN_BYTE);
  
  Vector *output = vec_get(ms, BITS_IN_BYTE);
  
  if(!input0->isSymbolic) {
    //Concrete case
    output->conWord = (~input0->conWord) & 1;
    output->isSymbolic = 0;
    return output;
  }
  
  //Symbolic case
  output->symWord[0] = Abc_LitNot(input0->symWord[0]);
  for(i = 1; i < BITS_IN_BYTE; i++)
    output->symWord[i] = Gia_ManConst0Lit();
  output->isSymbolic = 1;
  
  return output;
}

Vector *pPIECE(machine_state *ms, Vector *input0, Vector *input1) {
  return vec_cat(ms, input0, input1);
}

//trunc and output_size are given in bytes
Vector *pSUBPIECE(machine_state *ms, Vector *input0, uintmax_t input1, uintmax_t output_size) {
  uintmax_t i;
  assert(output_size > 0);
  uintmax_t output_size_bits = output_size * BITS_IN_BYTE;
  uintmax_t input1_bits = input1 * BITS_IN_BYTE;
  Vector *output = vec_get(ms, output_size_bits);
  
  if(input1_bits >= input0->size) {
    vec_setValue(ms, output, 0);
    return output;
  }
  
  if(!input0->isSymbolic) {
    if(output_size_bits <= WORD_BITS) {
      output->conWord = int_zextend(input0->conWord, input0->size);
      output->conWord >>= input1_bits;
      output->isSymbolic = 0;
      return output;
    }
    vec_calc_sym(ms, input0);
  }
  
  uint8_t isSymbolic = (output->size > WORD_BITS);
  for(i = 0; ((i+input1_bits) < input0->size) && (i < output_size_bits); i++) {
    output->symWord[i] = input0->symWord[i+input1_bits];
    if(!Gia_ManIsConstLit(output->symWord[i]))
      isSymbolic = 1;
  }
  
  for(; i < output_size_bits; i++)
    output->symWord[i] = Gia_ManConst0Lit();
  
  if(isSymbolic)
    output->isSymbolic = 1;
  else vec_sym_to_con(ms, output);
  
  return output;	
}

Vector *pINT_ZEXT(machine_state *ms, Vector *input0, uintmax_t output_size) {
  return vec_zextend(ms, input0, output_size*BITS_IN_BYTE);
}

Vector *pINT_SEXT(machine_state *ms, Vector *input0, uintmax_t output_size) {
  return vec_sextend(ms, input0, output_size*BITS_IN_BYTE);
}

Vector *pINT_2COMP(machine_state *ms, Vector *input0) {
  return vec_negate(ms, input0);
}

Vector *pINT_NEGATE(machine_state *ms, Vector *input0) {
  return vec_invert(ms, input0);
}

//pcode helper functions

void pRETURN(machine_state *ms) {
}

void pCUT(machine_state *ms) {
}

void pNULL(machine_state *ms) {
}


void conditional_branch(machine_state *ms, Gia_Lit_t condition,
			cbranch_type t_branch, cbranch_type t_cut,
			cbranch_type f_branch, cbranch_type f_cut,
			uint8_t call_SAT_solver) {
  memTuple mem_ret;
  memTuple mem_cut;
  memTuple mem_copy_t;
  memTuple mem_copy_f;
  
  memTuple memories = ms->memory;

  //	assert(branch_error == 0);
  if(ms->branch_error == 1) return;
  
  call_SAT_solver = 1;
  
  //fprintf(stdout, "%ju\n", stackframe_depth); fflush(stdout);
  
  condition = garbage_collect_ntk(ms, condition, 0); //SEAN!!!
  
  //If the condition is constant, just follow one path.
  if(Gia_ManIsConstLit(condition)) {
    if(Gia_ManIsConst0Lit(condition)) {
      /******* Follow the false path *******/
      ms->memory = memories;
      f_branch(ms);
      mem_cut = ms->memory;
      if(ms->branch_error == 1) { ms->memory = mem_cut; return; }
      ms->memory = mem_cut;
      f_cut(ms);
      mem_ret = ms->memory;
    } else {
      /******* Follow the true path *******/
      ms->memory = memories;
      t_branch(ms);
      mem_cut = ms->memory;
      if(ms->branch_error == 1) { ms->memory = mem_cut; return; }
      ms->memory = mem_cut;
      t_cut(ms);
      mem_ret = ms->memory;
    }
  } else if(call_SAT_solver && is_node_constant(ms, condition, 0)) {
    push_condition(ms, condition, 0);
    if(are_conditions_unsat(ms, 0)) {
      ms->branch_error = 1;
      pop_condition(ms);
      ms->memory = memories;
      return;
    }
    
    /******* Follow the false path *******/
    ms->memory = memories;
    f_branch(ms);
    mem_cut = ms->memory;
    if(ms->branch_error == 1) {
      pop_condition(ms);
      ms->memory = mem_cut;
      return;
    }
    ms->memory = mem_cut;
    f_cut(ms);

    //condition = garbage_collect_ntk(ms, condition, 1); //SEAN!!!

    mem_ret = ms->memory;
    pop_condition(ms);
  } else if(call_SAT_solver && is_node_constant(ms, condition, 1)) {
    push_condition(ms, condition, 1);
    if(are_conditions_unsat(ms, 0)) {
      ms->branch_error = 1;
      pop_condition(ms);
      return;
    }
    
    /******* Follow the true path *******/
    ms->memory = memories;
    t_branch(ms);
    mem_cut = ms->memory;
    if(ms->branch_error == 1) {
      pop_condition(ms);
      ms->memory = mem_cut;
      return;
    }
    ms->memory = mem_cut;
    t_cut(ms);

    //condition = garbage_collect_ntk(ms, condition, 1); //SEAN!!!

    mem_ret = ms->memory;
    pop_condition(ms);
  } else {
    //The condition is symbolic, follow both paths.
    if(ms->stackframe_depth--==0) {
      if(!call_SAT_solver) {
	fprintf(stdout, "Warning: conditional branch recursion depth reached for this path...cutting it short\n");
	fflush(stdout);
	ms->branch_error = 1;
	ms->stackframe_depth++;
	ms->memory = memories;
	return;
      }
    }
    
    //Make copies of the memories, one for each path
    mem_copy_t.cMem = cMemory_copy(ms, memories.cMem);
    mem_copy_t.sMem = sMemory_copy(ms, memories.sMem);
    mem_copy_f.cMem = cMemory_copy(ms, memories.cMem);
    mem_copy_f.sMem = sMemory_copy(ms, memories.sMem);
    
    uintmax_t m_head = ms->memories_stack->head;
    
    /******* Follow the true path *******/
    arr_stack_push(ms->memories_stack, (void *)&mem_copy_f);
    push_condition(ms, condition, 1);
    assert(ms->branch_error == 0);
    Vector *orig_heap_offset = vec_dup(ms, ms->heap_offset);

    ms->memory = mem_copy_t;
    t_branch(ms);
    t_cut(ms);

    //condition = garbage_collect_ntk(ms, condition, 1); //SEAN!!!

    memTuple t_branch_result = ms->memory;
    
    Vector *t_branch_heap_offset = vec_dup(ms, ms->heap_offset);
    
    uint8_t t_branch_error = ms->branch_error;
    
    pop_condition(ms);
    arr_stack_pop(ms->memories_stack);			
    
    /******* Follow the false path *******/
    arr_stack_push(ms->memories_stack, (void *)&mem_copy_t);
    push_condition(ms, condition, 0);
    
    ms->branch_error = 0;
    
    vec_copy(ms, ms->heap_offset, orig_heap_offset);

    ms->memory = mem_copy_f;
    f_branch(ms);
    f_cut(ms);

    //condition = garbage_collect_ntk(ms, condition, 1); //SEAN!!!

    memTuple f_branch_result = ms->memory;
    
    Vector *f_branch_heap_offset = vec_dup(ms, ms->heap_offset);
    
    uint8_t f_branch_error = ms->branch_error;
    
    pop_condition(ms);
    arr_stack_pop(ms->memories_stack);			
    
    //Done recursing
    
    ms->stackframe_depth++;
    
    if(m_head != ms->memories_stack->head) assert(0);
    
    //Decide which memory to keep (or merge both).
    if(f_branch_error && t_branch_error) {
      mem_ret.cMem = memories.cMem;
      mem_ret.sMem = memories.sMem;
      cMemory_free(ms, t_branch_result.cMem);
      sMemory_free(ms, t_branch_result.sMem);
      cMemory_free(ms, f_branch_result.cMem);
      sMemory_free(ms, f_branch_result.sMem);
      vec_copy(ms, ms->heap_offset, orig_heap_offset);
      assert(ms->branch_error == 1);
    } else if(t_branch_error) {
      cMemory_free(ms, memories.cMem);
      sMemory_free(ms, memories.sMem);
      cMemory_free(ms, t_branch_result.cMem);
      sMemory_free(ms, t_branch_result.sMem);
      mem_ret.cMem = f_branch_result.cMem;
      mem_ret.sMem = f_branch_result.sMem;
      vec_copy(ms, ms->heap_offset, f_branch_heap_offset);
      assert(ms->branch_error == 0);
    } else if(f_branch_error) {
      cMemory_free(ms, memories.cMem);
      sMemory_free(ms, memories.sMem);
      cMemory_free(ms, f_branch_result.cMem);
      sMemory_free(ms, f_branch_result.sMem);
      mem_ret.cMem = t_branch_result.cMem;
      mem_ret.sMem = t_branch_result.sMem;
      vec_copy(ms, ms->heap_offset, t_branch_heap_offset);
      assert(ms->branch_error == 1);
      ms->branch_error = 0;
    } else {
      cMemory_free(ms, memories.cMem);
      sMemory_free(ms, memories.sMem);
      mem_ret.cMem = cMemory_ite(ms, condition, t_branch_result.cMem, f_branch_result.cMem);
      mem_ret.sMem = sMemory_ite(ms, condition, t_branch_result.sMem, f_branch_result.sMem);
      //ms->heap_offset = (t_branch_heap_offset > f_branch_heap_offset) ? t_branch_heap_offset : f_branch_heap_offset;
      Vector *c_branch_heap_offset = vec_ite(ms, condition, t_branch_heap_offset, f_branch_heap_offset);
      vec_release(ms, ms->heap_offset);
      ms->heap_offset = c_branch_heap_offset;
      assert(ms->branch_error == 0);
    }
    vec_release(ms, orig_heap_offset);
    vec_release(ms, t_branch_heap_offset);
    vec_release(ms, f_branch_heap_offset);
  }
  ms->memory = mem_ret;

  return;
}

//Library functions

void plib_malloc_32_x86_le(machine_state *ms) {
  Vector *r_ESP_4_0 = cMemory_load_le(ms, 0x10, 4);
  Vector *c_0x4_4 = vec_getConstant(ms, 0x4, 4*BITS_IN_BYTE);
  Vector *r_ESP_4_1 = pINT_ADD(ms, r_ESP_4_0, c_0x4_4);
  cMemory_store_le(ms, 0x10, r_ESP_4_1, 4);
  
  //Malloc
  cMemory_store_le(ms, 0x0, ms->heap_offset, 4);
  Vector *mallocSize = sMemory_load_le(ms, r_ESP_4_1, 4);
  Vector *new_heap_offset = vec_add(ms, ms->heap_offset, mallocSize);
  vec_release(ms, ms->heap_offset);
  vec_release(ms, mallocSize);
  ms->heap_offset = new_heap_offset;
  
  vec_release(ms, r_ESP_4_1);
  vec_release(ms, c_0x4_4);
  vec_release(ms, r_ESP_4_0);
}

void plib_free_32_x86_le(machine_state *ms) {
  Vector *r_ESP_4_0 = cMemory_load_le(ms, 0x10, 4);
  Vector *c_0x4_4 = vec_getConstant(ms, 0x4, 4*BITS_IN_BYTE);
  Vector *r_ESP_4_1 = pINT_ADD(ms, r_ESP_4_0, c_0x4_4);
  cMemory_store_le(ms, 0x10, r_ESP_4_1, 4);
  
  //Free
  
  vec_release(ms, r_ESP_4_1);
  vec_release(ms, c_0x4_4);
  vec_release(ms, r_ESP_4_0);
}

void plib___assert_fail_32_x86_le(machine_state *ms) {
  Vector *r_ESP_4_0 = cMemory_load_le(ms, 0x10, 4);
  Vector *c_0x4_4 = vec_getConstant(ms, 0x4, 4*BITS_IN_BYTE);
  Vector *r_ESP_4_1 = pINT_ADD(ms, r_ESP_4_0, c_0x4_4);
  cMemory_store_le(ms, 0x10, r_ESP_4_1, 4);
  
  int8_t result = are_conditions_unsat(ms, 0);
  
  if(!result) {
    fprintf(stdout, "\nassert fails for inputs: ");
    //print_recent_sat_solver_result_on_inputs(ms);
    assert(0);
  } else {
    fprintf(stdout, "\nassert reached symbolically, however, no concrete inputs cause this assert\n");
  }
  
  ms->branch_error = 1;   
  
  vec_release(ms, r_ESP_4_1);
  vec_release(ms, c_0x4_4);
  vec_release(ms, r_ESP_4_0);
}

void plib_unknownFunction_32_x86_le(machine_state *ms, uint32_t argLen) {
  Vector *r_ESP_4_0 = cMemory_load_le(ms, 0x10, 4);
  Vector *c_argLen_4 = vec_getConstant(ms, argLen, 4*BITS_IN_BYTE);
  Vector *r_ESP_4_1 = pINT_ADD(ms, r_ESP_4_0, c_argLen_4);
  cMemory_store_le(ms, 0x10, r_ESP_4_1, 4);
  
  //Free
  
  vec_release(ms, r_ESP_4_1);
  vec_release(ms, c_argLen_4);
  vec_release(ms, r_ESP_4_0);
}
