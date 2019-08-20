#ifndef AIG_PCODEDEF_H
#define AIG_PCODEDEF_H

#include "vectype.h"

machine_state *machine_state_init(char *network_name, uintmax_t cmem_base_address, uintmax_t cmem_size, uintmax_t ho, uint8_t address_size);
void machine_state_free(machine_state *ms);

//pcode function definitions

Vector *pCOPY(machine_state *ms, Vector *input0);
Vector *pINT_ADD(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SUB(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_MULT(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_DIV(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_REM(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SDIV(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SREM(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_OR(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_XOR(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_AND(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_LEFT(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_RIGHT(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SRIGHT(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_EQUAL(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_NOTEQUAL(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_LESS(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_LESSEQUAL(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_CARRY(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SLESS(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SLESSEQUAL(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SCARRY(machine_state *ms, Vector *input0, Vector *input1);
Vector *pINT_SBORROW(machine_state *ms, Vector *input0, Vector *input1);
Vector *pBOOL_OR(machine_state *ms, Vector *input0, Vector *input1);
Vector *pBOOL_XOR(machine_state *ms, Vector *input0, Vector *input1);
Vector *pBOOL_AND(machine_state *ms, Vector *input0, Vector *input1);
Vector *pBOOL_NEGATE(machine_state *ms, Vector *input0);
Vector *pPIECE(machine_state *ms, Vector *input0, Vector *input1);
Vector *pSUBPIECE(machine_state *ms, Vector *input0, uintmax_t input1, uintmax_t output_size);
Vector *pINT_ZEXT(machine_state *ms, Vector *input0, uintmax_t output_size);
Vector *pINT_SEXT(machine_state *ms, Vector *input0, uintmax_t output_size);
Vector *pINT_2COMP(machine_state *ms, Vector *input0);
Vector *pINT_NEGATE(machine_state *ms, Vector *input0);

//pcode helper functions

void pRETURN(machine_state *ms);
void pCUT(machine_state *ms);
void pNULL(machine_state *ms);

typedef void (*cbranch_type)(machine_state *ms);
void conditional_branch(machine_state *ms, Gia_Lit_t condition,
			cbranch_type t_branch, cbranch_type t_cut,
			cbranch_type e_branch, cbranch_type e_cut,
			uint8_t call_SAT_solver);
//Library functions

void plib_malloc_32_x86_le(machine_state *ms);
void plib_free_32_x86_le(machine_state *ms);
void plib___assert_fail_32_x86_le(machine_state *ms);
void plib_unknownFunction_32_x86_le(machine_state *ms, uint32_t argLen);

#endif
