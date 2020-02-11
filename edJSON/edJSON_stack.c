//
// Created by Stanislav Lakhtin on 11/02/2020.
//

#include "edJSON_stack.h"

edjson_err_t pop(json_states_t *data, edjson_stack * stack) {
  if (is_stack_empty(stack))
    return EDJSON_ERR_JSON_TO_COMPLICATED;
  *data = peek(stack);
  stack->head -= 1;
  return EDJSON_OK;
}

edjson_err_t push(json_states_t data, edjson_stack * stack) {
  if (if_stack_full(stack))
    return EDJSON_ERR_MEMORY_OVERFLOW;
  stack->data[stack->head] = data;
  stack->head += 1;
  return EDJSON_OK;
}