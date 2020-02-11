//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#ifndef edJSON_H
#define edJSON_H

#include "edJSON_def.h"

#include "edJSON_stack.h"

typedef enum {
  OBJECT_START,
  OBJECT_END,
  ARRAY_START,
  ARRAY_END,
  ELEMENT_START,
  ELEMENT_END
} edjson_event_kind_t;

typedef edjson_err_t ( * edjson_to_begin )( void );
typedef edjson_err_t ( * edjson_read_next )( char * buffer );
typedef edjson_err_t ( * edjson_error_handler )( edjson_err_t code, uint32_t position );
typedef edjson_err_t ( * on_object_event_fn )( edjson_event_kind_t event_kind );
typedef edjson_err_t ( * on_element_name_fn )( const char * node_name );
typedef edjson_err_t ( * on_element_value_fn )( const json_element_t * node );

#ifndef EDJSON_BUFFER_DEPTH
#define EDJSON_BUFFER_DEPTH 32      // length of most long name or string value
#endif

#ifndef FAIL_IF
#define FAIL_IF(expression) do {\
  if (expression) \
    return PARSER_FAIL; \
} while (0x00)
#endif

typedef struct {
  edjson_stack stack;
  char string_buffer[EDJSON_BUFFER_DEPTH];
  parse_string_state_t string_fsm_state;
  parse_value_state_t value_fsm_state;
  uint8_t string_hex_fsm_state;
  char last_element[EDJSON_BUFFER_DEPTH];
  uint16_t buffer_head;
  char current_symbol;
  uint32_t position;

  json_ret_codes_t last_rc;

  edjson_to_begin init;
  edjson_read_next read;
  edjson_error_handler on_error;
  // ------------- events handlers ----------------
  on_object_event_fn on_parse_event;
  on_element_name_fn on_element_name;
  on_element_value_fn on_element_value;
} json_parser_t;

typedef json_ret_codes_t ( * json_state_fptr_t ) ( json_parser_t * );

#ifdef __cplusplus
extern "C"
{
#endif

json_ret_codes_t handle_error( json_parser_t * parser );
json_ret_codes_t detect_object( json_parser_t * parser );
json_ret_codes_t parse_object( json_parser_t * parser );
json_ret_codes_t parse_array( json_parser_t * parser );
json_ret_codes_t parse_value( json_parser_t * parser );


edjson_err_t parse( json_parser_t * parser );
json_states_t lookup_transitions(json_states_t state, json_ret_codes_t code);

edjson_err_t as_int( const char * data, int * value );
edjson_err_t as_string( const char * data, char * buffer );

#ifdef __cplusplus
}
#endif

#endif //edJSON_H
