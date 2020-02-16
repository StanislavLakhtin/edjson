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

// ----------------- edjson_ret_t codes ---------------
typedef enum {
  EDJSON_OK = 0x00,
  EDJSON_FINISH = 1,
  EDJSON_ERR_RESOURCE_NOT_FOUND = -1,
  EDJSON_ERR_JSON_STACK_OVERFLOW = -2,
  EDJSON_ERR_WRONG_SYMBOL = -3,
  EDJSON_EOF = -4,
  EDJSON_ERR_MEMORY_OVERFLOW = -5,
  EDJSON_ERR_STACK_ERROR = -6,
  EDJSON_ERR_PARSER = -7,
  EDJSON_OBJECT_DETECTED = 2,
  EDJSON_REPEAT = 3,
  EDJSON_REPEAT_WITH_SKIP_READ = 4,
  EDJSON_ARRAY_DETECTED = 5,
  EDJSON_ATTRIBUTE_DETECTED = 6,
  EDJSON_VALUE_DETECTED = 7,
  EDJSON_DETECT_NEXT = 8,
} json_ret_codes_t;

typedef enum {
  JSON_ROOT,
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_ATTRIBUTE
} json_element_type_t;

typedef enum {
  parse_err = 0,
  parse_obj = 1,
  parse_arr = 2,
  parse_attr = 3,
  parse_val = 4,
} json_states_t;


#define ENTRY_PARSER_STATE  parse_obj
#define UNKNOWN_STATE       parse_err

typedef enum {
  unknown, obj_begin, attr_name, obj_end, // Object FSM
  array_begin, array_end,   // Array FSM
  value_begin, value_body, value_end,   // Value FSM
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
  number_begin,
  number_sign,
  number_zero,
  number_digit,
  number_dot,
  number_e,
  number_e_sign,
  number_e_digit,  // number FSM
} parse_number_state_t;

#ifndef RETURN_STATE
#define RETURN_STATE(SIGNAL, SKIP) return {.signal = SIGNAL, .skip_next_read = SKIP};
#define RETURN(SIGNAL) RETURN_STATE(SIGNAL, false)
#endif


struct json_transition {
  json_states_t src;
  json_ret_codes_t ret_codes;
  json_states_t dst;
};


typedef struct {
  json_element_type_t kind;
  char *name;
  char *value;
} json_element_t;

#define DEFAULT_VALUE_NODE(data) {\
  .kind = JSON_VALUE, \
  .name = parser->last_element, \
  .value = data \
};

#endif //EDJSON_EDJSON_DEF_H
