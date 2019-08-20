#include "vectype.h"

//Routines for handling concretely addressed memory

cMemory *cMemory_init(machine_state *ms, uintmax_t base_address, uintmax_t size) {
  uintmax_t i;
  cMemory *cMem = (cMemory *)malloc(1 * sizeof(cMemory));
  cMem->cByte   = (cMemoryCell *)malloc(size * sizeof(cMemoryCell));
  for(i = 0; i < size; i++) {
    cMem->cByte[i].value = vec_getConstant(ms, 0, BITS_IN_BYTE);
    cMem->cByte[i].valueProbes = get_probes_from_vec(ms, cMem->cByte[i].value);    
    cMem->cByte[i].writtenTo = Gia_ManConst0Lit();
    cMem->cByte[i].writtenToProbe = get_probe_from_lit(ms, cMem->cByte[i].writtenTo);
  }
  
  cMem->size = size;
  cMem->base_address = base_address;
  return cMem;
}

void cMemory_free(machine_state *ms, cMemory *cMem) {
  uintmax_t i;
  for(i = 0; i < cMem->size; i++) {
    probe_free(ms, cMem->cByte[i].writtenToProbe);
    cMem->cByte[i].writtenToProbe = 0;
    cMem->cByte[i].writtenTo = Gia_ManConst0Lit();  
    probes_free(ms, cMem->cByte[i].valueProbes, cMem->cByte[i].value->size);
    cMem->cByte[i].valueProbes = NULL;
    vec_release(ms, cMem->cByte[i].value);
    cMem->cByte[i].value = NULL;
  }
  free(cMem->cByte);
  free(cMem);
}

void cMemory_print(machine_state *ms, cMemory *cMem, uint8_t full) {
  uintmax_t i;
  fprintf(stdout, "cMemory(%p): base_address=0x%jx, size=%ju\n", cMem, cMem->base_address, cMem->size);
  if(full == 1) {
    for(i = 0; i < cMem->size; i++) {
      if(cMem->cByte[i].writtenTo == Gia_ManConst0Lit()) continue;
      fprintf(stdout, "0x%jx (%p) ", i, cMem->cByte[i].value);
      fprintf(stdout, "writtenTo=");
      Gia_ObjPrint(ms->ntk, Gia_ObjFromLit(ms->ntk, cMem->cByte[i].writtenTo));
      vec_print(ms, cMem->cByte[i].value);
    }
  } else if(full == 0) {
    for(i = 0; i < cMem->size; i++) {
      if(cMem->cByte[i].writtenTo == Gia_ManConst0Lit()) {
	fprintf(stdout, "..");
      } else {
	vec_printSimple(ms, cMem->cByte[i].value);
      }
      fprintf(stdout, " ");
      if(i%32 == 31) fprintf(stdout, "\n");
    }
  }
  fprintf(stdout, "\n");
  fflush(stdout);
}

//Store 'value' of 'size' bytes into 'cMem' at address 'address' (little endian)
void cMemory_store_le(machine_state *ms, uintmax_t address, Vector *value, uintmax_t size) {
  uintmax_t i;
  uintmax_t size_bits = size*BITS_IN_BYTE;
  cMemory *cMem = ms->memory.cMem;
  assert(value->size >= size_bits); (void)size_bits;
  assert(address >= cMem->base_address);
  
  if(((address - cMem->base_address)+size) > cMem->size) {
    fprintf(stdout, "Error: Storing cMem outside the bounds allocated...exiting\n");
    assert(0);
    exit(0);
  }

  Vector **vec_split = vec_splitIntoNewArray(ms, value, size);
  for(i = 0; i < size; i++) {
    vec_copy(ms, cMem->cByte[(address - cMem->base_address) + i].value, vec_split[i]);
    cMem->cByte[(address - cMem->base_address) + i].writtenTo = Gia_ManConst1Lit();
  }
  vec_releaseArray(ms, vec_split, size);
}

//Store 'value' of 'size' bytes into 'cMem' at address 'address' (big endian)
void cMemory_store_be(machine_state *ms, uintmax_t address, Vector *value, uintmax_t size) {
  uintmax_t i;
  uintmax_t size_bits = size*BITS_IN_BYTE;
  cMemory *cMem = ms->memory.cMem;
  assert(value->size >= size_bits); (void)size_bits;
  assert(address >= cMem->base_address);
  
  if(((address - cMem->base_address)+size) > cMem->size) {
    fprintf(stdout, "Error: Storing cMem outside the bounds allocated...exiting\n");
    assert(0);
    exit(0);
  }

  Vector **vec_split = vec_splitIntoNewArray(ms, value, size);
  for(i = 0; i < size; i++) {
    vec_copy(ms, cMem->cByte[(address - cMem->base_address) + i].value, vec_split[(size-1)-i]);
    cMem->cByte[(address - cMem->base_address) + i].writtenTo = Gia_ManConst1Lit();
  }
  vec_releaseArray(ms, vec_split, size);
}

//Load 'size' bytes from 'cMem' at address 'address' into ret (little endian)
Vector *cMemory_load_le(machine_state *ms, uintmax_t address, uintmax_t size) {
  uintmax_t i;
  cMemory *cMem = ms->memory.cMem;
  assert(address >= cMem->base_address);
  
  if(((address - cMem->base_address)+size) > cMem->size) {
    fprintf(stdout, "Error: Loading cMem outside the bounds allocated...exiting\n");
    assert(0);
    exit(0);
  }

  Vector **vec_split = vec_getArray(ms, size, BITS_IN_BYTE);
  for(i = 0; i < size; i++) {
    if(cMem->cByte[(address - cMem->base_address) + i].writtenTo == Gia_ManConst0Lit()) {
      fprintf(stdout, "Error: cMemory Read-Before-Write error at address 0x%jx (assuming [0x%jx] = 0)\n", (address - cMem->base_address)+i, (address - cMem->base_address)+i);
    }
    vec_copy(ms, vec_split[i], cMem->cByte[(address - cMem->base_address) + i].value);
  }
  Vector *ret = vec_joinArray(ms, vec_split, size);
  vec_releaseArray(ms, vec_split, size);

  return ret;
}

//Load 'size' bytes from 'cMem' at address 'address' into ret (big endian)
Vector *cMemory_load_be(machine_state *ms, uintmax_t address, uintmax_t size) {
  uintmax_t i;
  cMemory *cMem = ms->memory.cMem;
  assert(address >= cMem->base_address);
  
  if(((address - cMem->base_address)+size) > cMem->size) {
    fprintf(stdout, "Error: Loading cMem outside the bounds allocated...exiting\n");
    assert(0);
    exit(0);
  }
  
  Vector **vec_split = vec_getArray(ms, size, BITS_IN_BYTE);
  for(i = 0; i < size; i++) {
    if(cMem->cByte[(address - cMem->base_address) + i].writtenTo == Gia_ManConst0Lit()) {
      fprintf(stdout, "Error: cMemory Read-Before-Write error at address 0x%jx (assuming [0x%jx] = 0)\n", (address - cMem->base_address)+i, (address - cMem->base_address)+i);
    }
    vec_copy(ms, vec_split[(size-1)-i], cMem->cByte[(address - cMem->base_address) + i].value);
  }
  Vector *ret = vec_joinArray(ms, vec_split, size);
  vec_releaseArray(ms, vec_split, size);
  return ret;
}

//Attempt to load 'size' bytes from 'cMem' at address 'address' and check
//for a 'read-before-write' error
Gia_Lit_t cMemory_load_rbw(machine_state *ms, uintmax_t address, uintmax_t size) {
  uintmax_t j, k;
  cMemory *cMem = ms->memory.cMem;
  assert(address >= cMem->base_address);
  
  if(((address - cMem->base_address)+size) > cMem->size) {
    fprintf(stdout, "Error: Loading cMem outside the bounds allocated...exiting\n");
    assert(0);
    exit(0);
  }
  
  Gia_Lit_t rbw = Gia_ManConst0Lit();
  k = address - cMem->base_address;
  j = k+size;
  for(; k < j; k++) {
    rbw = Gia_ManHashOr(ms->ntk, rbw, Abc_LitNot(cMem->cByte[k].writtenTo));
    if(rbw == Gia_ManConst1Lit()) break;
  }
  
  return rbw;
}

//Merge cMemT and cMemF. Normally used after returning from a conditional.
//The cMemory not returned will be free'd.
cMemory *cMemory_ite(machine_state *ms, Gia_Lit_t c, cMemory *cMemT, cMemory *cMemF) {
  uintmax_t k;
  assert(cMemT->size == cMemF->size);
  assert(cMemT->base_address == cMemF->base_address);
  if(Gia_ManIsConstLit(c)) {
    //Concrete conditional case
    if(Gia_ManIsConst0Lit(c)) {
      cMemory_free(ms, cMemT);
      return cMemF;
    } else {
      cMemory_free(ms, cMemF);
      return cMemT;
    }
  }
  
  for(k = 0; k < cMemT->size; k++) {
    Vector *TByte = cMemT->cByte[k].value;
    Vector *FByte = cMemF->cByte[k].value;
    
    if(Gia_ManIsConst0Lit(cMemF->cByte[k].writtenTo)) {
      //keep TByte, ignore FByte
    } else if(Gia_ManIsConst0Lit(cMemT->cByte[k].writtenTo)) {
      //keep FByte, ignore TByte
      vec_copy(ms, TByte, FByte);
      cMemT->cByte[k].writtenTo = cMemF->cByte[k].writtenTo;
    } else {
      Vector *ITEByte = vec_ite(ms, c, TByte, FByte);
      vec_copy(ms, TByte, ITEByte);
      vec_release(ms, ITEByte);
      cMemT->cByte[k].writtenTo = Gia_ManHashMux(ms->ntk, c, cMemT->cByte[k].writtenTo, cMemF->cByte[k].writtenTo);
    }
  }
  
  cMemory_free(ms, cMemF);
  return cMemT;
}

cMemory *cMemory_copy(machine_state *ms, cMemory *cMem) {
  uintmax_t i;
  cMemory *cMemRet = cMemory_init(ms, cMem->base_address, cMem->size);
  for(i = 0; i < cMem->size; i++) {
    vec_copy(ms, cMemRet->cByte[i].value, cMem->cByte[i].value);
    cMemRet->cByte[i].writtenTo = cMem->cByte[i].writtenTo;
  }
  return cMemRet;
}

void cMemory_update_probes(machine_state *ms, cMemory *cMem) {
  uintmax_t i;
  for(i = 0; i < cMem->size; i++) {
    if(cMem->cByte[i].writtenTo != Gia_ManConst0Lit())
      update_probes_from_vec(ms, cMem->cByte[i].valueProbes, cMem->cByte[i].value);
    update_probe_from_lit(ms, cMem->cByte[i].writtenToProbe, cMem->cByte[i].writtenTo);
  }
}

void cMemory_update_from_probes(machine_state *ms, cMemory *cMem) {
  uintmax_t i;
  for(i = 0; i < cMem->size; i++) {
    if(cMem->cByte[i].writtenTo != Gia_ManConst0Lit())
      update_vec_from_probes(ms, cMem->cByte[i].valueProbes, cMem->cByte[i].value);
    cMem->cByte[i].writtenTo = get_lit_from_probe(ms, cMem->cByte[i].writtenToProbe);
  }
}

void cMemory_collect_probes(machine_state *ms, cMemory *cMem) {
  uintmax_t i;
  for(i = 0; i < cMem->size; i++) {
    if(cMem->cByte[i].writtenTo != Gia_ManConst0Lit())
      collect_probes(ms, cMem->cByte[i].valueProbes, cMem->cByte[i].value->size);
    collect_probe(ms, cMem->cByte[i].writtenToProbe);
  }
}

//Routined for handling symbolically addressed memory

sMemory *sMemory_init(machine_state *ms, uint8_t address_size) {
  sMemory *sMem = (sMemory *)malloc(1 * sizeof(sMemory));
  sMem->memoized_flag = 0;
  sMem->memoized_value = vec_getConstant(ms, 0, BITS_IN_BYTE);
  sMem->memoized_value_probes = get_probes_from_vec(ms, sMem->memoized_value);
  sMem->writtenTo = Gia_ManConst0Lit();
  sMem->writtenToProbe = get_probe_from_lit(ms, sMem->writtenTo);
  sMem->head = 0;
  sMem->size = SYMBOLIC_MEMORY_SIZE;
  sMem->address_size = address_size;
  sMem->sByteArray = (sMemoryCell *)malloc(sMem->size * sizeof(sMemoryCell));
  sMem->c = Gia_ManConst1Lit();
  sMem->cProbe = get_probe_from_lit(ms, sMem->c);
  sMem->sMemT = NULL;
  sMem->sMemF = NULL;
  arr_stack_push(ms->sMemInitStack, (void *)sMem);
  return sMem;
}

void sMemory_flag(machine_state *ms, sMemory *sMem) {
  if(sMem == NULL) return;
  
  if(sMem->memoized_flag == ms->CurrsMemFlag) return;
  sMem->memoized_flag = ms->CurrsMemFlag;
  
  sMemory_flag(ms, sMem->sMemT);
  sMemory_flag(ms, sMem->sMemF);
}

void sMemory_collectNotFlaggedForDeletion(machine_state *ms, sMemory *sMem) {
  if(sMem == NULL) return;
  
  if(sMem->memoized_flag == ms->CurrsMemFlag) return;
  sMem->memoized_flag = ms->CurrsMemFlag;
  
  sMemory_collectNotFlaggedForDeletion(ms, sMem->sMemT);
  sMemory_collectNotFlaggedForDeletion(ms, sMem->sMemF);
  
  arr_stack_push(ms->sMemDeleteStack, (void *)sMem);        
}

void sMemory_free(machine_state *ms, sMemory *sMem) {
  sMemory *sMem_tmp;
  uintmax_t i, head;
  
  ms->CurrsMemFlag++;
  
  for(head = ms->sMemInitStack->head; head!=0; head--) {
    sMem_tmp = (sMemory *)ms->sMemInitStack->mem[head];
    if(sMem_tmp!=sMem) sMemory_flag(ms, sMem_tmp);
    else {
      //Remove sMem from stack
      ms->sMemInitStack->mem[head] = ms->sMemInitStack->mem[ms->sMemInitStack->head];
      ms->sMemInitStack->head--;
    }
  }
  
  sMemory_collectNotFlaggedForDeletion(ms, sMem);
  
  while((sMem_tmp = (sMemory *)arr_stack_pop(ms->sMemDeleteStack))!=NULL) {
    assert(sMem_tmp->sByteArray != NULL);

    for(i = 0; i < sMem_tmp->head; i++) {
      probes_free(ms, sMem_tmp->sByteArray[i].addressProbes, sMem_tmp->sByteArray[i].address->size);
      vec_release(ms, sMem_tmp->sByteArray[i].address);
      probes_free(ms, sMem_tmp->sByteArray[i].valueProbes, sMem_tmp->sByteArray[i].value->size);
      vec_release(ms, sMem_tmp->sByteArray[i].value);      
    }
    free(sMem_tmp->sByteArray);
    sMem_tmp->sByteArray = NULL;

    probe_free(ms, sMem_tmp->writtenToProbe);
    
    probes_free(ms, sMem_tmp->memoized_value_probes, sMem_tmp->memoized_value->size);
    vec_release(ms, sMem_tmp->memoized_value);

    probe_free(ms, sMem_tmp->cProbe);
        
    free(sMem_tmp);
  }
}

void _sMemory_print(machine_state *ms, sMemory *sMem, uint8_t full) {
  uintmax_t i;
  if(sMem == NULL) return;
  
  if(sMem->memoized_flag == ms->CurrsMemFlag) return;
  sMem->memoized_flag = ms->CurrsMemFlag;
  
  _sMemory_print(ms, sMem->sMemT, full);
  _sMemory_print(ms, sMem->sMemF, full);
  
  fprintf(stdout, "sMemory(%p): head=%ju, size=%ju, address_size=%du, memoized_flag=%ju, memoized_value=%p, sMemT=%p, sMemF=%p\n",
	  sMem, sMem->head, sMem->size, sMem->address_size, sMem->memoized_flag, sMem->memoized_value, sMem->sMemT, sMem->sMemF);
  fprintf(stdout, "condition=");
  Gia_ObjPrint(ms->ntk, Gia_ObjFromLit(ms->ntk, sMem->c));
  
  if(full) {
    for(i = 0; i < sMem->head; i++) {
      fprintf(stdout, "%ju address(%p)\n", i, sMem->sByteArray[i].address);
      vec_print(ms, sMem->sByteArray[i].address);
      fprintf(stdout, "%ju value(%p)\n", i, sMem->sByteArray[i].value);
      vec_print(ms, sMem->sByteArray[i].value);
    }      
  } else {
    for(i = 0; i < sMem->head; i++) {
      vec_printSimple(ms, sMem->sByteArray[i].address);
      fprintf(stdout, " = ");
      vec_printSimple(ms, sMem->sByteArray[i].value);
      fprintf(stdout, "\n");
    }
  }
  fprintf(stdout, "\n");
}

void sMemory_print(machine_state *ms, sMemory *sMem, uint8_t full) {
  ms->CurrsMemFlag++;
  _sMemory_print(ms, sMem, full);
}

void _sMemory_check(machine_state *ms, sMemory *sMem) {
  uintmax_t i;
  if(sMem == NULL) return;
  
  if(sMem->memoized_flag == ms->CurrsMemFlag) return;
  sMem->memoized_flag = ms->CurrsMemFlag;
  
  _sMemory_check(ms, sMem->sMemT);
  _sMemory_check(ms, sMem->sMemF);
  
  vec_verify(ms, sMem->memoized_value);
  
  assert(sMem->sByteArray);
  for(i = 0; i < sMem->head; i++) {
    assert(sMem->sByteArray[i].address);
    vec_verify(ms, sMem->sByteArray[i].address);
    assert(sMem->sByteArray[i].value);
    vec_verify(ms, sMem->sByteArray[i].value);
  }   
}

void sMemory_check(machine_state *ms, sMemory *sMem) {
  ms->CurrsMemFlag++;
  _sMemory_check(ms, sMem);   
}

void sMemory_increaseSize(sMemory *sMem) {
  sMem->sByteArray = (sMemoryCell *)realloc(sMem->sByteArray, (sMem->size + SYMBOLIC_MEMORY_SIZE) * sizeof(sMemoryCell));
  sMem->size += SYMBOLIC_MEMORY_SIZE;
}

uint8_t sMemory_removeByte(machine_state *ms, Vector *address) {
  intmax_t i; //Must be a signed integer
  uint8_t ret = 0;
  sMemory *sMem = ms->memory.sMem;
  if(sMem->head == 0) return ret;
  
  for(i = sMem->head-1; i >= 0; i--) {
    if(vec_sym_equal(ms, address, sMem->sByteArray[i].address)==1) {
      probes_free(ms, sMem->sByteArray[i].addressProbes, sMem->sByteArray[i].address->size);
      sMem->sByteArray[i].addressProbes = NULL;
      vec_release(ms, sMem->sByteArray[i].address);
      sMem->sByteArray[i].address = NULL;
      probes_free(ms, sMem->sByteArray[i].valueProbes, sMem->sByteArray[i].value->size);
      sMem->sByteArray[i].valueProbes = NULL;
      vec_release(ms, sMem->sByteArray[i].value);
      sMem->sByteArray[i].value = NULL;
      break;
    }
  }
  
  if(i != -1) {
    //Found the address
    //Compress the table
    ret = 1;
    for(; i < sMem->head-1; i++) {
      sMem->sByteArray[i] = sMem->sByteArray[i+1];
    }
    sMem->head--;
  }
  
  //Recursing here can destroy the integrity of child memories if
  //they are referenced from other memories (caused by conditional
  //branches).
  
  return ret;
}

void sMemory_storeByte(machine_state *ms, Vector *address, Vector *value) {
  sMemory *sMem = ms->memory.sMem;
  assert(address->size == sMem->address_size);
  if(ms->sMemory_auto_compress)
    sMemory_removeByte(ms, address);
  
  if(sMem->head >= (sMem->size - 2)) //Check for out of memory
    sMemory_increaseSize(sMem);
  sMem->sByteArray[sMem->head].address = address;
  sMem->sByteArray[sMem->head].value = value;
  sMem->sByteArray[sMem->head].addressProbes = get_probes_from_vec(ms, address);
  sMem->sByteArray[sMem->head].valueProbes = get_probes_from_vec(ms, value);
  sMem->head++;
}

//Symbolically addressed store (little endian)
void sMemory_store_le(machine_state *ms, Vector *address_start, Vector *value, uintmax_t size) {
  uintmax_t i;
  assert(value->size >= size*BITS_IN_BYTE);
  
  Vector *address = vec_get(ms, address_start->size);
  vec_copy(ms, address, address_start);
  
  Vector *vec_one = vec_getConstant(ms, 1, address->size);
  
  for(i = 0; i < size; i++) {
    Vector *value_byte = vec_selectBits(ms, value, BITS_IN_BYTE, (i*BITS_IN_BYTE));
    sMemory_storeByte(ms, address, value_byte);
    
    if(i < (size-1)) { //No need to add 1 on the last loop
      address = vec_add(ms, vec_one, address);
      //'address' will be pushed onto the sMem stack, so no need to release it.
    }
  }
  vec_release(ms, vec_one);   
}

//Symbolically addressed store (big endian)
void sMemory_store_be(machine_state *ms, Vector *address, Vector *value, uintmax_t size) {
  intmax_t i; //Must be a signed integer
  assert(value->size >= size*BITS_IN_BYTE);
  
  Vector *address_i = vec_get(ms, address->size);
  vec_copy(ms, address_i, address);
  
  Vector *vec_one = vec_getConstant(ms, 1, address_i->size);
  
  for(i = size-1; i >= 0; i--) {
    Vector *value_byte = vec_selectBits(ms, value, BITS_IN_BYTE, (i*BITS_IN_BYTE));
    sMemory_storeByte(ms, address_i, value_byte);
    
    if(i > 0) { //No need to add 1 on the last loop
      address_i = vec_add(ms, vec_one, address_i);
      //The old 'address_i' will be pushed onto the sMem stack, so no need to release it.
    }
  }
  vec_release(ms, vec_one);   
}

void sMemory_storeInt_le(machine_state *ms, uintmax_t address, uintmax_t value, uintmax_t size) {
  sMemory *sMem = ms->memory.sMem;
  Vector *address_vec = vec_getConstant(ms, address, sMem->address_size);
  Vector *value_vec = vec_getConstant(ms, value, size*BITS_IN_BYTE);
  sMemory_store_le(ms, address_vec, value_vec, size);
  vec_release(ms, value_vec);
  vec_release(ms, address_vec);
}

void sMemory_storeInt_be(machine_state *ms, uintmax_t address, uintmax_t value, uintmax_t size) {
  sMemory *sMem = ms->memory.sMem;
  Vector *address_vec = vec_getConstant(ms, address, sMem->address_size);
  Vector *value_vec = vec_getConstant(ms, value, size*BITS_IN_BYTE);
  sMemory_store_be(ms, address_vec, value_vec, size);
  vec_release(ms, value_vec);
  vec_release(ms, address_vec);
}

void sMemory_storeArray_le(machine_state *ms, Vector *address, Vector **vec_array, uintmax_t numArrayElements, uintmax_t numElementBytes) {
  uintmax_t i;
  Vector *address_i = vec_get(ms, address->size);
  vec_copy(ms, address_i, address);
  
  Vector *vec_elementSize = vec_getConstant(ms, numElementBytes, address->size);
  
  for(i = 0; i < numArrayElements; i++) {
    sMemory_store_le(ms, address_i, vec_array[i], numElementBytes);
    if(i < (numArrayElements - 1)) {
      //No need to add 1 on the last loop
      Vector *tmp = vec_add(ms, address_i, vec_elementSize);
      vec_release(ms, address_i);
      address_i = tmp;
    }
  }
  vec_release(ms, address_i);
  vec_release(ms, vec_elementSize);
}

void sMemory_storeArray_be(machine_state *ms, Vector *address, Vector **vec_array, uintmax_t numArrayElements, uintmax_t numElementBytes) {
  uintmax_t i;
  Vector *address_i = vec_get(ms, address->size);
  vec_copy(ms, address_i, address);
  
  Vector *vec_elementSize = vec_getConstant(ms, numElementBytes, address->size);
  
  for(i = 0; i < numArrayElements; i++) {
    sMemory_store_be(ms, address_i, vec_array[i], numElementBytes);
    if(i < (numArrayElements - 1)) {
      //No need to add 1 on the last loop
      Vector *tmp = vec_add(ms, address_i, vec_elementSize);
      vec_release(ms, address_i);
      address_i = tmp;
    }
  }
  vec_release(ms, address_i);
  vec_release(ms, vec_elementSize);
}

//return value denotes whether or not any perfectly matching value was found
uint8_t sMemory_loadLocal(machine_state *ms, sMemory *sMem, Vector *address, uint8_t call_SAT_solver) {
  intmax_t i; //Must be a signed integer
  
  for(i = (sMem->head-1); i >=0; i--) {
    Gia_Lit_t equal = vec_equal_SAT(ms, address, sMem->sByteArray[i].address, call_SAT_solver);
    if(Gia_ManIsConstLit(equal)) {
      if(Gia_ManIsConst0Lit(equal)) {
	continue;
      } else {
	arr_stack_push(ms->sMemStack, (void *)sMem->sByteArray[i].value);
	return 1;
      }
    } else {
      arr_stack_push_uintmax(ms->sMemStack, (uintmax_t)equal);
      arr_stack_push(ms->sMemStack, (void *)sMem->sByteArray[i].value);
    }
  }
  
  return 0;
}

Vector *sMem_stackMerge(machine_state *ms, sMemory *sMem, uintmax_t pop_to_level) {
  Vector *ret = vec_get(ms, BITS_IN_BYTE);

  assert(pop_to_level < ms->sMemStack->head);
  Vector *first = (Vector *)arr_stack_pop(ms->sMemStack);
  vec_copy(ms, ret, first);
  
  while(ms->sMemStack->head > pop_to_level) {
    Vector *tmp_vec = (Vector *)arr_stack_pop(ms->sMemStack);
    Gia_Lit_t equal = (Gia_Lit_t )arr_stack_pop_uintmax(ms->sMemStack);
   
    Vector *result = vec_ite(ms, equal, tmp_vec, ret);
    vec_release(ms, ret);
    ret = result;

    sMem->writtenTo = Gia_ManHashOr(ms->ntk, sMem->writtenTo, equal);
  }

  return ret;
}

Vector *sMemory_loadByte(machine_state *ms, sMemory *sMem, Vector *address) {
  Vector *ret;
  
  uint8_t call_SAT_solver = 0;
  
  if(sMem->memoized_flag == ms->CurrsMemFlag) {
    ret = vec_get(ms, sMem->memoized_value->size);
    vec_copy(ms, ret, sMem->memoized_value);
    return ret;      
  }
  sMem->memoized_flag = ms->CurrsMemFlag;

  uintmax_t pop_to_level = ms->sMemStack->head;
  sMem->writtenTo = Gia_ManConst0Lit();
  uint8_t perfect_match = sMemory_loadLocal(ms, sMem, address, call_SAT_solver);
  if(pop_to_level == ms->sMemStack->head)
    assert(sMem->writtenTo == Gia_ManConst0Lit());

  if(perfect_match) {
    sMem->writtenTo = Gia_ManConst1Lit();
    ret = sMem_stackMerge(ms, sMem, pop_to_level);
  } else if(sMem->sMemF == NULL) {
    if(sMem->sMemT == NULL) {
      //leaf node
      arr_stack_push(ms->sMemStack, (void *)ms->vec_zero_byte); //a potential read-before-write error, will return the 'zero' vector
      ret = sMem_stackMerge(ms, sMem, pop_to_level);
    } else {
      Vector *ret1 = sMemory_loadByte(ms, sMem->sMemT, address);
      sMem->writtenTo = Gia_ManHashOr(ms->ntk, sMem->writtenTo, sMem->sMemT->writtenTo);
      arr_stack_push(ms->sMemStack, (void *)ret1);
      ret = sMem_stackMerge(ms, sMem, pop_to_level);
      vec_release(ms, ret1);
    }
  } else {
    assert(sMem->sMemT != NULL);
    assert(!Gia_ManIsConstLit(sMem->c));
    /******* Follow the true path *******/
    //Push condition=true onto the stack
    push_condition(ms, sMem->c, 1);
    Vector *ret1 = sMemory_loadByte(ms, sMem->sMemT, address);
    pop_condition(ms);
    
    /******* Follow the false path *******/
    //Push condition=false onto the stack
    push_condition(ms, sMem->c, 0);
    Vector *ret2 = sMemory_loadByte(ms, sMem->sMemF, address);
    pop_condition(ms);

    sMem->writtenTo = Gia_ManHashMux(ms->ntk, sMem->c,
      Gia_ManHashOr(ms->ntk, sMem->writtenTo, sMem->sMemT->writtenTo),
      Gia_ManHashOr(ms->ntk, sMem->writtenTo, sMem->sMemF->writtenTo));

    arr_stack_push_uintmax(ms->sMemStack, (uintmax_t)sMem->c);
    arr_stack_push(ms->sMemStack, (void *)ret1);
    arr_stack_push(ms->sMemStack, (void *)ret2);
    ret = sMem_stackMerge(ms, sMem, pop_to_level);
    vec_release(ms, ret1);
    vec_release(ms, ret2);
  }
  
  vec_copy(ms, sMem->memoized_value, ret);
  return ret;
}

//Load 'size' bytes from 'sMem' at address 'address' into ret (little endian)
Vector *sMemory_load_le(machine_state *ms, Vector *address, uintmax_t size) {
  uintmax_t i;
  intmax_t k; //Must be a signed integer
  uintmax_t size_bits = size*BITS_IN_BYTE;
  sMemory *sMem = ms->memory.sMem;

  assert(address->size == sMem->address_size);
  
  Vector *ret = NULL;
  
  Vector *address_i = vec_get(ms, address->size);
  vec_copy(ms, address_i, address);
  
  //vec_concretize_with_SAT(ms, address_i);
  
  Vector *vec_one = vec_getConstant(ms, 1, address->size);
  
  uint8_t isSymbolic = ((size*BITS_IN_BYTE) > WORD_BITS);
  for(i = 0; i < size; i++) {
    ms->CurrsMemFlag++;
    ret = sMemory_loadByte(ms, sMem, address_i);

    if(sMem->writtenTo == Gia_ManConst0Lit()) {
      fprintf(stdout, "Error: sMemory Read-Before-Write error at address "); vec_printSimple(ms, address_i);
      fprintf(stdout, "\nAssuming the value = 0\n");
    }

    //if(sMem->writtenTo != Gia_ManConst1Lit()) {
    // potential read-before-write error
    //}

    if(ret->isSymbolic == 1) {
      isSymbolic = 1;
    }
    arr_stack_push(ms->sMemStack, (void *)ret);
    
    if(i < (size - 1)) {
      Vector *tmp = vec_add(ms, address_i, vec_one);
      vec_release(ms, address_i);
      address_i = tmp;         
    }
  }
  
  vec_release(ms, address_i);
  vec_release(ms, vec_one);
  
  ret = vec_getConstant(ms, 0, size_bits);
  
  if(!isSymbolic) {
    //Concrete case
    for(i = 0; i < size; i++) {
      Vector *vec_byte = (Vector *)arr_stack_pop(ms->sMemStack);
      assert(vec_byte->size == BITS_IN_BYTE);
      assert(!vec_byte->isSymbolic);
      ret->conWord <<= BITS_IN_BYTE;
      ret->conWord |= vec_byte->conWord & (((uintmax_t) ~0)>>(WORD_BITS - BITS_IN_BYTE)); //0xFF when BITS_IN_BYTE == 8
      vec_release(ms, vec_byte); //release the pushed vectors
    }
    ret->isSymbolic = 0;      
  } else {
    //Symbolic case
    for(k = size-1; k >= 0; k--) {
      Vector *vec_byte = (Vector *)arr_stack_pop(ms->sMemStack);
      if(!vec_byte->isSymbolic) vec_calc_sym(ms, vec_byte);
      assert(vec_byte->symWord!=NULL);
      for(i = 0; i < BITS_IN_BYTE; i++) {
	ret->symWord[(k*BITS_IN_BYTE)+i] = vec_byte->symWord[i];
      }
      vec_release(ms, vec_byte);
    }
    ret->isSymbolic = 1;      
  }

  assert(ms->sMemStack->head == 0);  
  return ret;
}

//Load 'size' bytes from 'sMem' at address 'address' into ret (big endian)
Vector *sMemory_load_be(machine_state *ms, Vector *address, uintmax_t size) {
  uintmax_t i;
  intmax_t k; //Must be a signed integer
  uintmax_t size_bits = size*BITS_IN_BYTE;
  sMemory *sMem = ms->memory.sMem;

  assert(address->size == sMem->address_size);
  
  Vector *ret = NULL;
  
  Vector *vec_sizem1 = vec_getConstant(ms, size-1, address->size);
  
  Vector *address_i = vec_add(ms, address, vec_sizem1);
  
  vec_release(ms, vec_sizem1);
  
  //vec_concretize_with_SAT(ms, address_i);
  
  Vector *vec_one = vec_getConstant(ms, 1, address->size);
  
  uint8_t isSymbolic = ((size*BITS_IN_BYTE) > WORD_BITS);
  for(i = 0; i < size; i++) {
    ms->CurrsMemFlag++;
    ret = sMemory_loadByte(ms, sMem, address_i);

    if(sMem->writtenTo == Gia_ManConst0Lit()) {
      fprintf(stdout, "Error: sMemory Read-Before-Write error at address "); vec_printSimple(ms, address_i);
      fprintf(stdout, "\nAssuming the value = 0\n");
    }

    //if(sMem->writtenTo != Gia_ManConst1Lit()) {
    // potential read-before-write error
    //}

    if(ret->isSymbolic == 1) {
      isSymbolic = 1;
    }
    arr_stack_push(ms->sMemStack, (void *)ret);
    
    if(i < (size - 1)) {
      Vector *tmp = vec_sub(ms, address_i, vec_one);
      vec_release(ms, address_i);
      address_i = tmp;         
    }
  }
  
  vec_release(ms, address_i);
  vec_release(ms, vec_one);
  
  ret = vec_getConstant(ms, 0, size_bits);
  
  if(!isSymbolic) {
    //Concrete case
    for(i = 0; i < size; i++) {
      Vector *vec_byte = (Vector *)arr_stack_pop(ms->sMemStack);
      assert(vec_byte->size == BITS_IN_BYTE);
      assert(!vec_byte->isSymbolic);
      ret->conWord <<= BITS_IN_BYTE;
      ret->conWord |= vec_byte->conWord & (((uintmax_t) ~0)>>(WORD_BITS - BITS_IN_BYTE)); //0xFF when BITS_IN_BYTE == 8
      vec_release(ms, vec_byte); //release the pushed vectors
    }
    ret->isSymbolic = 0;      
  } else {
    //Symbolic case
    for(k = size-1; k >= 0; k--) {
      Vector *vec_byte = (Vector *)arr_stack_pop(ms->sMemStack);
      if(!vec_byte->isSymbolic) vec_calc_sym(ms, vec_byte);
	assert(vec_byte->symWord!=NULL);
      for(i = 0; i < BITS_IN_BYTE; i++) {
	ret->symWord[(k*BITS_IN_BYTE)+i] = vec_byte->symWord[i];
      }
      vec_release(ms, vec_byte);
    }
    ret->isSymbolic = 1;      
  }
  
  assert(ms->sMemStack->head == 0);  
  return ret;
}

Gia_Lit_t sMemory_load_rbw(machine_state *ms, Vector *address, uintmax_t size) {
  
  //???
  
  return 0;
}

Vector **sMemory_loadArray_le(machine_state *ms, Vector *address, uintmax_t numArrayElements, uintmax_t numElementBytes) {
  uintmax_t i;
  Vector **vec_array = (Vector **)malloc(numArrayElements * sizeof(Vector *));
  
  Vector *address_i = vec_get(ms, address->size);
  vec_copy(ms, address_i, address);
  
  Vector *vec_elementSize = vec_getConstant(ms, numElementBytes, address->size);
  
  for(i = 0; i < numArrayElements; i++) {
    vec_array[i] = sMemory_load_le(ms, address_i, numElementBytes);
    if(i < (numArrayElements - 1)) {
      Vector *tmp = vec_add(ms, address_i, vec_elementSize);
      vec_release(ms, address_i);
      address_i = tmp;
    }
  }
  vec_release(ms, address_i);
  vec_release(ms, vec_elementSize);
  return vec_array;   
}

Vector **sMemory_loadArray_be(machine_state *ms, Vector *address, uintmax_t numArrayElements, uintmax_t numElementBytes) {
  uintmax_t i;
  Vector **vec_array = (Vector **)malloc(numArrayElements * sizeof(Vector *));
  
  Vector *address_i = vec_get(ms, address->size);
  vec_copy(ms, address_i, address);
  
  Vector *vec_elementSize = vec_getConstant(ms, numElementBytes, address->size);
  
  for(i = 0; i < numArrayElements; i++) {
    vec_array[i] = sMemory_load_be(ms, address_i, numElementBytes);
    if(i < (numArrayElements - 1)) {
      Vector *tmp = vec_add(ms, address_i, vec_elementSize);
      vec_release(ms, address_i);
      address_i = tmp;
    }
  }
  vec_release(ms, address_i);
  vec_release(ms, vec_elementSize);
  return vec_array;   
}

//A fast sMemory_ite function (pushes computation off until later)
sMemory *sMemory_ite(machine_state *ms, Gia_Lit_t c, sMemory *sMemT, sMemory *sMemF) {
  assert(sMemT!=NULL);
  assert(sMemF!=NULL);
  assert(sMemT->address_size == sMemF->address_size);
  if(Gia_ManIsConstLit(c)) {
    if(Gia_ManIsConst0Lit(c)) {
      sMemory_free(ms, sMemT);
      return sMemF;
    } else {
      sMemory_free(ms, sMemF);
      return sMemT;           
    }
  }
  
  sMemory *sMemITE = sMemory_init(ms, sMemT->address_size);
  sMemITE->c = c;
  sMemITE->sMemT = sMemT;
  sMemITE->sMemF = sMemF;
  
  sMemory_free(ms, sMemT); //It is important to call sMemory_free here
  sMemory_free(ms, sMemF);
  
  return sMemITE;
}

sMemory *sMemory_copy(machine_state *ms, sMemory *sMem) {
  sMemory *sMemRet = sMemory_init(ms, sMem->address_size);
  sMemRet->c = Gia_ManConst1Lit();
  sMemRet->sMemT = sMem;
  sMemRet->sMemF = NULL;
  return sMemRet;
}

//A compression function for symbolic memory
uintmax_t _sMemory_compress(machine_state *ms, sMemory *sMem) {
  intmax_t i, j, values_removed = 0;
  
  if(sMem == NULL) return 0;
  
  if(sMem->memoized_flag == ms->CurrsMemFlag)
    return 0;
  sMem->memoized_flag = ms->CurrsMemFlag;
  
  values_removed+=_sMemory_compress(ms, sMem->sMemT);
  values_removed+=_sMemory_compress(ms, sMem->sMemF);
  
  for(i = sMem->head-1; i > 0; i--) {
    for(j = i-1; j >= 0; j--) {
      if(sMem->sByteArray[i].address == NULL || sMem->sByteArray[j].address == NULL) {
	continue;
      } else if(vec_sym_equal(ms, sMem->sByteArray[i].address, sMem->sByteArray[j].address)==1) {
	probes_free(ms, sMem->sByteArray[j].addressProbes, sMem->sByteArray[j].address->size);
	sMem->sByteArray[j].addressProbes = NULL;
	vec_release(ms, sMem->sByteArray[j].address);
	sMem->sByteArray[j].address = NULL;
	probes_free(ms, sMem->sByteArray[j].valueProbes, sMem->sByteArray[j].value->size);
	sMem->sByteArray[j].valueProbes = NULL;
	vec_release(ms, sMem->sByteArray[j].value);
	sMem->sByteArray[j].value = NULL;
      }
    }
  }
  
  j = 0;
  for(i = 0; i < sMem->head; i++) {
    if(sMem->sByteArray[i].address != NULL) {
      sMem->sByteArray[j] = sMem->sByteArray[i];
      j++;
    } else {
      values_removed++;
    }
  }
  sMem->head = j;
  return values_removed;
}

void sMemory_compress(machine_state *ms, sMemory *sMem) {
  if(ms->sMemory_auto_compress) return;
  fprintf(stdout, "Compressing symbolic memory, ");
  ms->CurrsMemFlag++;
  uintmax_t values_removed = _sMemory_compress(ms, sMem);
  fprintf(stdout, "%ju values removed...\n", values_removed);
  sMemory_check(ms, sMem);   
}

void sMemory_update_probes(machine_state *ms, sMemory *sMem) {   
  uintmax_t i;
  
  if(sMem == NULL) return;
  if(sMem->memoized_flag == ms->CurrsMemFlag) return;
  sMem->memoized_flag = ms->CurrsMemFlag;
  sMemory_update_probes(ms, sMem->sMemT);
  sMemory_update_probes(ms, sMem->sMemF);
  update_probes_from_vec(ms, sMem->memoized_value_probes, sMem->memoized_value);
  update_probe_from_lit(ms, sMem->writtenToProbe, sMem->writtenTo);
  for(i = 0; i < sMem->head; i++) {
    update_probes_from_vec(ms, sMem->sByteArray[i].addressProbes, sMem->sByteArray[i].address);
    update_probes_from_vec(ms, sMem->sByteArray[i].valueProbes, sMem->sByteArray[i].value);
  }
  update_probe_from_lit(ms, sMem->cProbe, sMem->c);
}

void sMemory_update_from_probes(machine_state *ms, sMemory *sMem) {   
  uintmax_t i;

  if(sMem == NULL) return;
  update_vec_from_probes(ms, sMem->memoized_value_probes, sMem->memoized_value);
  sMem->writtenTo = get_lit_from_probe(ms, sMem->writtenToProbe);
  for(i = 0; i < sMem->head; i++) {
    update_vec_from_probes(ms, sMem->sByteArray[i].addressProbes, sMem->sByteArray[i].address);
    update_vec_from_probes(ms, sMem->sByteArray[i].valueProbes, sMem->sByteArray[i].value);
  }
  sMem->c = get_lit_from_probe(ms, sMem->cProbe);
}

void sMemory_collect_probes(machine_state *ms, sMemory *sMem) {
  uintmax_t i = 0;

  if(sMem == NULL) return;
  collect_probes(ms, sMem->memoized_value_probes, sMem->memoized_value->size);
  collect_probe(ms, sMem->writtenToProbe);
  for(i = 0; i < sMem->head; i++) {
    collect_probes(ms, sMem->sByteArray[i].addressProbes, sMem->sByteArray[i].address->size);
    collect_probes(ms, sMem->sByteArray[i].valueProbes, sMem->sByteArray[i].value->size);
  }
  collect_probe(ms, sMem->cProbe);
}
