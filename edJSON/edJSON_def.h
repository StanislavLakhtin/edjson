//
// Created by Stanislav Lakhtin on 11/02/2020.
//

#ifndef EDJSON_EDJSON_DEF_H
#define EDJSON_EDJSON_DEF_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>

typedef int8_t edjson_err_t;     //  user errors bigger than 0, 0 -- OK, system error lower than 0

#define EDJSON_OK                             0x00
#define EDJSON_FINISH                          1
#define EDJSON_ERR_RESOURCE_NOT_FOUND         -1
#define EDJSON_ERR_JSON_TO_COMPLICATED        -2
#define EDJSON_ERR_WRONG_SYMBOL               -3
#define EDJSON_EOF                            -4
#define EDJSON_ERR_MEMORY_OVERFLOW            -5
#define EDJSON_ERR_STACK_ERROR                -6

typedef enum {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_VALUE
} json_element_type_t;

typedef enum  {
  parser_error = 0,
  detect_obj= 1,
  parse_obj = 2,
  parse_arr = 3,
  parse_val = 4,
} json_states_t;

typedef enum {
  unknown, obj_begin, obj_name, obj_colon_wait, obj_colon, obj_comma, obj_end, // Object FSM
  array_begin, array_next, array_end,   // Array FSM
  value_begin, value_end,   // Value FSM
} parse_object_state_t;

typedef enum {
  str_begin, str_body, reverse_solidus, hex_digits, str_end, // String FSM
} parse_string_state_t;

typedef enum {
  object_value,
  string_value,
  number_value,
  array_value,
  string_constant_value,
  unknown_value
} parse_value_state_t;

typedef enum {
  number_begin, number_sign, number_zero, number_digit, number_dot, number_e, number_e_sign, number_e_digit,  // number FSM
} parse_number_state_t;

typedef enum {
  OBJECT_DETECTED, PARSER_FAIL, REPEAT_PLEASE, ARRAY_DETECTED, VALUE_DETECTED, VALUE_ENDED
} json_ret_codes_t;

#ifndef PUSH_OR_FAIL
#define PUSH_OR_FAIL()  (push_to_buffer(parser, parser->current_symbol)) ? PARSER_FAIL : REPEAT_PLEASE
#endif

struct json_transition {
  json_states_t src;
  json_ret_codes_t ret_codes;
  json_states_t dst;
};

#define ENTRY_PARSER_STATE  detect_obj
#define UNKNOWN_STATE       parser_error

typedef struct {
  json_element_type_t kind;
  char * name;
  char * value;
} json_element_t;

#define DEFAULT_VALUE_NODE(data) {\
  .kind = JSON_VALUE, \
  .name = parser->last_element, \
  .value = data \
};

#endif //EDJSON_EDJSON_DEF_H
