//
// Created by Stanislav Lakhtin on 11/02/2020.
//

#include "edJSON_stack.h"

edjson_err_t pop(parse_object_state_t *data, edjson_stack * stack) {
  if (is_stack_empty(stack))
    return EDJSON_ERR_JSON_TO_COMPLICATED;
  *data = peek(stack);
  stack->head -= 1;
  return EDJSON_OK;
}

edjson_err_t push(parse_object_state_t data, edjson_stack * stack) {
  if (if_stack_full(stack))
    return EDJSON_ERR_MEMORY_OVERFLOW;
  stack->head += 1;
  stack->data[stack->head] = data;
  return EDJSON_OK;
}

inline parse_object_state_t peek(edjson_stack * stack) {
  if (stack->head >= 0)
    return stack->data[stack->head];
  return stack->data[0];
}

edjson_err_t flush_until(parse_object_state_t data, edjson_stack * stack) {
  int _h = stack->head;
  while (_h > 0 ) {
    if (stack->data[_h] == data) {
      stack->head = _h;
      memset(stack->data + stack->head, unknown, STACK_MAX_DEPTH - stack->head);
      return EDJSON_OK;
    }
    _h -= 1;
  }
  return EDJSON_ERR_STACK_ERROR;
}