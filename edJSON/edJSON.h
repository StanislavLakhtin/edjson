//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#ifndef edJSON_H
#define edJSON_H

#include "edJSON_def.h"

#include "edJSON_stack.h"

typedef edjson_err_t ( * edjson_to_begin )( void );
typedef edjson_err_t ( * edjson_read_next )( char * buffer );
typedef edjson_err_t ( * edjson_error_handler )( edjson_err_t code, uint32_t position );
typedef edjson_err_t ( * on_start_object_fn )( void );
typedef edjson_err_t ( * on_element_name_fn )( const char * node_name );
typedef edjson_err_t ( * on_element_value_fn )( const json_element_t * node, const char * value );

typedef enum {
  unknown_value,
  as_string_value,
  as_raw_value
} value_kind_t;

#ifndef EDJSON_BUFFER_DEPTH
#define EDJSON_BUFFER_DEPTH 32      // length of most long name or string value
#endif

typedef struct {
  edjson_stack stack;
  char string_buffer[EDJSON_BUFFER_DEPTH];
  char last_element[EDJSON_BUFFER_DEPTH];
  uint16_t buffer_head;
  char current_symbol;
  uint32_t position;
  value_kind_t value_kind;

  json_ret_codes_t last_rc;

  edjson_to_begin init;
  edjson_read_next read;
  edjson_error_handler on_error;
  // ------------- events handlers ----------------
  on_start_object_fn on_start_object;
  on_element_name_fn on_element_name;
  on_element_value_fn on_element_value;
} json_parser_t;

typedef json_ret_codes_t ( * json_state_fptr_t ) ( json_parser_t * );

#ifdef __cplusplus
extern "C"
{
#endif

json_ret_codes_t idle_state( json_parser_t * parser );
json_ret_codes_t object_state( json_parser_t * parser );
json_ret_codes_t error_state( json_parser_t * parser );
json_ret_codes_t node_state( json_parser_t * parser );
json_ret_codes_t definition_state( json_parser_t * parser );
json_ret_codes_t value_state( json_parser_t * parser );
json_ret_codes_t close_state( json_parser_t * parser );

edjson_err_t parse( json_parser_t * parser );
json_states_t lookup_transitions(json_states_t state, json_ret_codes_t code);

edjson_err_t as_int( const char * data, int * value );
edjson_err_t as_string( const char * data, char * buffer );

#ifdef __cplusplus
}
#endif

#endif //edJSON_H
