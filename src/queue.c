#include "queue.h"

#define QUEUE_INCREASE_SIZE 1000
#define STACK_INCREASE_SIZE 1000

//-------------Pointer-based Queue Manipulations---------------//

void increase_queue(void_queue *queue) {
  cell_queue *tmp = queue->tail;
  uintmax_t i;
  for(i = 0; i < QUEUE_INCREASE_SIZE; i++) {
    tmp->next = (cell_queue *)malloc(sizeof(cell_queue));
    tmp = tmp->next;
  }
  tmp->next = queue->head;
}

void_queue *queue_init() {
  void_queue *queue = (void_queue *)malloc(sizeof(void_queue));
  queue->head = (cell_queue *)malloc(sizeof(cell_queue));
  cell_queue *tmp = queue->head;
  uintmax_t i;
  for(i = 0; i < QUEUE_INCREASE_SIZE; i++) {
    tmp->next = (cell_queue *)malloc(sizeof(cell_queue));
    tmp = tmp->next;
  }
  tmp->next = queue->head;
  queue->tail = queue->head;
  return queue;
}

void queue_free(void_queue *queue) {
  assert(queue != NULL);
  if(queue->head!=queue->tail)
    fprintf(stderr, "Warning: queue being free'd while objects are still in the queue.\n");
  
  cell_queue *tmp = queue->head;
  queue->head = queue->head->next;
  tmp->next = NULL;
  
  while(queue->head != NULL) {
    tmp = queue->head;
    queue->head = queue->head->next;
    free(tmp);
  }
  free(queue);
}

void enqueue_x(void_queue *queue, void *x) {
  assert(x != NULL);
  if(queue->tail->next == queue->head)
    increase_queue(queue);
  queue->tail = queue->tail->next;
  queue->tail->x = x;
}

void *dequeue(void_queue *queue) {
  if(queue->head == queue->tail) return NULL;
  queue->head = queue->head->next;
  void *x = queue->head->x;
  return x;	
}

//-------------Pointer-based Stack Manipulations---------------//

void increase_stack(void_stack *stack) {
  cell_stack *tmp = stack->head;
  uintmax_t i;
  for(i = 0; i < STACK_INCREASE_SIZE; i++) {
    tmp->push = (cell_stack *)malloc(sizeof(cell_stack));
    tmp->push->pop = tmp;
    tmp = tmp->push;
  }
  tmp->push = NULL;
}

void_stack *stack_init() {
  void_stack *stack = (void_stack *)malloc(sizeof(void_stack));
  stack->head = (cell_stack *)malloc(sizeof(cell_stack));
  increase_stack(stack);
  return stack;
}

void stack_free(void_stack *stack) {
  assert(stack != NULL);
  if(stack->head->pop!=NULL)
    fprintf(stderr, "Warning: stack being free'd while objects are still in the stack.\n");
  
  while(stack->head != NULL) {
    cell_stack *tmp = stack->head;
    stack->head = stack->head->push;
    free(tmp);
  }
  free(stack);
}

void stack_push(void_stack *stack, void *x) {
  assert(x != NULL);
  if(stack->head->push==NULL)
    increase_stack(stack);
  stack->head = stack->head->push;
  stack->head->x = x;
}

void *stack_pop(void_stack *stack) {
  if(stack->head->pop == NULL) return NULL;
  void *x = stack->head->x;
  stack->head = stack->head->pop;
  return x;	
}

//-------------Array-based Stack Manipulations---------------//

void increase_arr_stack(void_arr_stack *stack) {
  stack->mem = (void **)realloc(stack->mem, (stack->size+STACK_INCREASE_SIZE) * sizeof(void *));
  stack->size += STACK_INCREASE_SIZE;
}

void_arr_stack *arr_stack_init() {
  void_arr_stack *stack = (void_arr_stack *)malloc(sizeof(void_arr_stack));
  stack->mem = (void **)malloc(STACK_INCREASE_SIZE * sizeof(void *));
  stack->head = 0;
  stack->size = STACK_INCREASE_SIZE;
  return stack;
}

void arr_stack_free(void_arr_stack *stack) {
  assert(stack != NULL);
  if(stack->head!=0)
    fprintf(stderr, "Warning: stack being free'd while objects are still in the stack.\n");
  
  free(stack->mem);
  free(stack);
}

void arr_stack_push(void_arr_stack *stack, void *x) {
  if(stack->head>=stack->size-2)
    increase_arr_stack(stack);
  stack->mem[++stack->head] = x;
}

void arr_stack_push_uintmax(void_arr_stack *stack, uintmax_t x) {
  if(stack->head>=stack->size-2)
    increase_arr_stack(stack);
  stack->mem[++stack->head] = (void *)x;
}

void *arr_stack_pop(void_arr_stack *stack) {
  if(stack->head == 0) return NULL;
  void *x = stack->mem[stack->head];
  stack->head--;
  return x;
}

uintmax_t arr_stack_pop_uintmax(void_arr_stack *stack) {
  if(stack->head == 0) return 0;
  uintmax_t x = (uintmax_t)stack->mem[stack->head];
  stack->head--;
  return x;
}

uint8_t arr_stack_empty(void_arr_stack *stack) {
  if(stack->head == 0) return 1;
  return 0;
}
