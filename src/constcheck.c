#include "vectype.h"

#define DEBUG 2

//Routines for pushing and poping conditions

void push_condition(machine_state *ms, Gia_Lit_t node, uint8_t value) {
  Gia_Probe_t pProbeId = Gia_SweeperProbeCreate(ms->ntk, Abc_LitNotCond(node, value==1));
  Gia_SweeperCondPush(ms->ntk, pProbeId);

  int unsat = Gia_SweeperCondCheckUnsat(ms->ntk);
  if(unsat) {
    fprintf(stderr, "Latest condition pushed causes constraints to be unsat\n");
    //assert(0);
  }
  
  condition *c = (condition *)malloc(1 * sizeof(condition));
  c->node = node;
  c->value = value;

  //Push the current machine state

  arr_stack_push(ms->conditions_stack, (void *)c);
}

void pop_condition(machine_state *ms) {
  Gia_SweeperProbeDelete(ms->ntk, Gia_SweeperCondPop(ms->ntk));

  condition *c = arr_stack_pop(ms->conditions_stack);

  //Pop the current machine state

  free(c);
}

int8_t are_conditions_unsat(machine_state *ms, uint8_t print_result) {
  uint8_t result = Gia_SweeperCondCheckUnsat(ms->ntk);

  if(DEBUG > 0) {
    fprintf(stdout, "SAT result %d\n", result);
    fflush(stdout);
    if(DEBUG > 1 && (result==0)) {
      print_sat_solver_result_on_inputs(ms);
    } else {
      fprintf(stdout, "no model(depth=%ju, num_vars=%d):", ms->conditions_stack->head, Gia_ManAndNum(ms->ntk));
    }
  }
  
  return result;
}

uint8_t is_node_constant(machine_state *ms, Gia_Lit_t node, uint8_t value) {
  push_condition(ms, node, value==0);
  int8_t result = are_conditions_unsat(ms, 0);
  assert(-1 <= result && result <= 1);
  if(result == -1) result = 0;
  pop_condition(ms);
   
  return result;
}

void set_sat_solver_limits(machine_state *ms, int32_t num_conflict, int32_t time_limit) {
  Gia_SweeperSetConflictLimit(ms->ntk, num_conflict);
  Gia_SweeperSetRuntimeLimit(ms->ntk, time_limit);
}

void print_sat_solver_result_on_inputs(machine_state *ms) {
  uintmax_t i;
  Vec_Int_t *pSolution = Gia_SweeperGetCex(ms->ntk);
  if(pSolution == NULL) return;
  
  fprintf(stdout, "model(depth=%ju, num_vars=%d):", ms->conditions_stack->head, Gia_ManAndNum(ms->ntk));
  
  for(i = 0; i < Vec_IntSize(pSolution); i++) {
    int32_t value = Vec_IntEntry(pSolution, i);
    if(value==0) {
      fprintf(stdout, " -%s", (char *)Vec_PtrEntry(ms->ntk->vNamesIn, i));
    } else if(value == 1) {
      fprintf(stdout, " %s", (char *)Vec_PtrEntry(ms->ntk->vNamesIn, i));
    } else {
      assert(value == 2);
      fprintf(stdout, " *%s", (char *)Vec_PtrEntry(ms->ntk->vNamesIn, i));
    }
  }
  
  fprintf(stdout, "\n");
}
