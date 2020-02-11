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
  detect_obj= 0,
  parse_obj = 1,
} json_states_t;

typedef enum {
  obj_begin, obj_name, obj_colon, obj_comma, obj_end, // Object FSM
} parse_object_state_t;

typedef enum {
  str_begin, rev_solidus, hex_digits, str_end, // String FSM
} parse_string_state_t;

typedef enum {
  JSON_OK, JSON_FAIL, JSON_REPEAT, JSON_ARR, JSON_OBJ
} json_ret_codes_t;

struct json_transition {
  json_states_t src;
  json_ret_codes_t ret_codes;
  json_states_t dst;
};

#define ENTRY_PARSER_STATE  detect_object
#define UNKNOWN_STATE       fail

typedef struct {
  json_element_type_t kind;
  char * name;
} json_element_t;

typedef enum {
  unknown_value,
  as_string_value,
  as_raw_value
} value_kind_t;

#endif //EDJSON_EDJSON_DEF_H
