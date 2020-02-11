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

#define EDJSON_VALUE_STRING 0x00
#define EDJSON_VALUE_SPEC   0x01

typedef enum {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_VALUE
} json_element_type_t;

typedef enum  {
  idle = 0,
  object = 1,
  fail = 2,
  node = 3,
  definition = 4,
  value = 5,
  close = 6,
} json_states_t;

typedef enum {
  JSON_OK, JSON_FAIL, JSON_REPEAT, JSON_ARR, JSON_OBJ
} json_ret_codes_t;

struct json_transition {
  json_states_t src;
  json_ret_codes_t ret_codes;
  json_states_t dst;
};

#define ENTRY_PARSER_STATE  idle
#define UNKNOWN_STATE       fail

typedef struct {
  json_element_type_t kind;
  char * name;
} json_element_t;

#endif //EDJSON_EDJSON_DEF_H
