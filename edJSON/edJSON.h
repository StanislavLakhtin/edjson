//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#ifndef edJSON_H
#define edJSON_H

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

enum json_states_t {
  idle = 0,
  object = 1,
  fail = 2,
  node = 3,
  definition = 4,
  value = 5,
  close = 6,
};

enum json_ret_codes_t {
  JSON_OK, JSON_FAIL, JSON_REPEAT, JSON_ALT
};

struct json_transition {
  enum json_states_t src;
  enum json_ret_codes_t ret_codes;
  enum json_states_t dst;
};

#define ENTRY_PARSER_STATE  idle
#define UNKNOWN_STATE       fail

typedef struct {
  json_element_type_t kind;
  char * name;
} json_element_t;

typedef edjson_err_t ( * edjson_to_begin )( void );
typedef edjson_err_t ( * edjson_read_next )( char * buffer );
typedef edjson_err_t ( * edjson_error_handler )( edjson_err_t code, uint32_t position );
typedef edjson_err_t ( * on_start_object_fn )( void );
typedef edjson_err_t ( * on_element_name_fn )( const char * node_name );
typedef edjson_err_t ( * on_element_value_fn )( const json_element_t * node, const char * value );

enum value_kind_t {
  as_string_value,
  as_raw_value
};

#ifndef EDJSON_BUFFER_DEPTH
#define EDJSON_BUFFER_DEPTH 32      // length of most long name or string value
#endif

typedef struct {
  char string_buffer[EDJSON_BUFFER_DEPTH];
  char last_element[EDJSON_BUFFER_DEPTH];
  uint16_t buffer_head;
  char current_symbol;
  uint32_t position;
  enum value_kind_t value_kind;

  enum json_ret_codes_t last_rc;

  edjson_to_begin init;
  edjson_read_next read;
  edjson_error_handler on_error;
  // ------------- events handlers ----------------
  on_start_object_fn on_start_object;
  on_element_name_fn on_element_name;
  on_element_value_fn on_element_value;
} json_parser_t;

typedef enum json_ret_codes_t ( * json_state_fptr_t ) ( json_parser_t * );

#ifdef __cplusplus
extern "C"
{
#endif

enum json_ret_codes_t idle_state( json_parser_t * parser );
enum json_ret_codes_t object_state( json_parser_t * parser );
enum json_ret_codes_t error_state( json_parser_t * parser );
enum json_ret_codes_t node_state( json_parser_t * parser );
enum json_ret_codes_t definition_state( json_parser_t * parser );
enum json_ret_codes_t value_state( json_parser_t * parser );
enum json_ret_codes_t close_state( json_parser_t * parser );

edjson_err_t parse( json_parser_t * parser );
enum json_states_t lookup_transitions(enum json_states_t state, enum json_ret_codes_t code);

edjson_err_t as_int( const char * data, int * value );
edjson_err_t as_string( const char * data, char * buffer );

#ifdef __cplusplus
}
#endif

#endif //edJSON_H
