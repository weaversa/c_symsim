#ifndef QUEUE_H
#define QUEUE_H

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

//-------------Pointer-based Queue Manipulations---------------//

typedef struct cell_queue {
	void *x;
	struct cell_queue *next;
} cell_queue;

typedef struct void_queue {
	cell_queue *head;
	cell_queue *tail;
} void_queue;

//-------------Pointer-based Stack Manipulations---------------//

typedef struct cell_stack {
	void *x;
	struct cell_stack *push;
	struct cell_stack *pop;
} cell_stack;

typedef struct void_stack {
	cell_stack *head;
} void_stack;

//-------------Array-based Stack Manipulations---------------//

typedef struct void_arr_stack {
	uintmax_t head;
	uintmax_t size;
	void **mem;
} void_arr_stack;

//-------------Pointer-based Queue Manipulations---------------//

void_queue *queue_init();
void queue_free(void_queue *queue);
void enqueue_x(void_queue *queue, void *x);
void *dequeue(void_queue *queue);

//-------------Pointer-based Stack Manipulations---------------//

void_stack *stack_init();
void stack_free(void_stack *stack);
void stack_push(void_stack *stack, void *x);
void *stack_pop(void_stack *stack);

//-------------Array-based Stack Manipulations---------------//

void_arr_stack *arr_stack_init();
void arr_stack_free(void_arr_stack *stack);
void arr_stack_push(void_arr_stack *stack, void *x);
void arr_stack_push_uintmax(void_arr_stack *stack, uintmax_t x);
void *arr_stack_pop(void_arr_stack *stack);
uintmax_t arr_stack_pop_uintmax(void_arr_stack *stack);
uint8_t arr_stack_empty(void_arr_stack *stack);

#endif
