//
// Created by Stanislav Lakhtin on 11/02/2020.
//

#ifndef EDJSON_EDJSON_STACK_H
#define EDJSON_EDJSON_STACK_H

#include "edJSON_def.h"

#define STACK_MAX_DEPTH  8

typedef struct {
  int head;
  json_states_t data[STACK_MAX_DEPTH];
} edjson_stack;

#define flush_stack(stack) stack.head = -1
#define is_stack_empty(stack) ( stack->head == -1 )
#define if_stack_full(stack) ( stack->head == STACK_MAX_DEPTH )
#define peek(stack) stack->data[stack->head]

#ifdef __cplusplus
extern "C"
{
#endif

edjson_err_t  pop(json_states_t * data, edjson_stack * stack);
edjson_err_t push(json_states_t data, edjson_stack * stack);

#ifdef __cplusplus
}
#endif

#endif //EDJSON_EDJSON_STACK_H
