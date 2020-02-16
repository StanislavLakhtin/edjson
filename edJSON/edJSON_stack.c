//
// Created by Stanislav Lakhtin on 11/02/2020.
//

#include "edJSON_stack.h"

json_ret_codes_t pop(parse_object_state_t *data, edjson_stack *stack) {
  if (is_stack_empty(stack))
    return EDJSON_ERR_STACK_ERROR;
  *data = peek(stack);
  stack->head -= 1;
  return EDJSON_OK;
}

json_ret_codes_t push(parse_object_state_t data, edjson_stack *stack) {
  if (if_stack_full(stack))
    return EDJSON_ERR_JSON_STACK_OVERFLOW;
  stack->head += 1;
  stack->data[stack->head] = data;
  return EDJSON_OK;
}

inline parse_object_state_t peek(edjson_stack *stack) {
  if (stack->head >= 0)
    return stack->data[stack->head];
  return stack->data[0];
}

json_ret_codes_t flush_until(parse_object_state_t data, edjson_stack *stack) {
  int _h = stack->head;
  while (_h > 0) {
    if (stack->data[_h] == data) {
      memset(stack->data + _h, unknown, STACK_MAX_DEPTH - _h);
      stack->head = _h - 1;
      return EDJSON_OK;
    }
    _h -= 1;
  }
  return EDJSON_ERR_STACK_ERROR;
}