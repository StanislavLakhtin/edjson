//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#ifndef edJSON_H
#define edJSON_H

#include "edJSON_def.h"

#include "edJSON_stack.h"

#define DEFAULT_SPACE_SYMBOLS " \n\r\t"

typedef enum {
  OBJECT_START,
  OBJECT_END,
  ARRAY_START,
  ARRAY_END,
  ELEMENT_START,
  ELEMENT_END,
  VALUE_START,
  VALUE_END
} edjson_event_kind_t;

typedef edjson_err_t ( * edjson_to_begin )( void );
typedef edjson_err_t ( * edjson_read_next )( char * buffer );
typedef edjson_err_t ( * edjson_error_handler )( edjson_err_t code, uint32_t position );
typedef edjson_err_t ( * on_object_event_fn )( edjson_event_kind_t event_kind, void * );
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

#ifndef SKIP_SPACES
#define SKIP_SPACES() do {\
  if (strchr(SPACES, parser->current_symbol)) {\
      return REPEAT_PLEASE;\
  }\
} while (0)
#endif

#ifndef GENERATE_VALUE_SIGNAL
#define GENERATE_VALUE_SIGNAL(value, parser) switch (value) { \
  case array_value: \
    FAIL_IF (push(array_begin, &parser->stack));\
    parser->emit_event(ARRAY_START, parser);\
    return ARRAY_DETECTED; \
  case object_value:\
    FAIL_IF (push(obj_begin, &parser->stack));\
    parser->emit_event(OBJECT_START, parser);\
    return OBJECT_DETECTED;\
  case unknown_value:\
    return PARSER_FAIL;\
  default:\
    FAIL_IF (push(value_begin, &parser->stack));\
    parser->value_fsm_state = _val_detect;\
    parser->emit_event(VALUE_START, parser);\
    return VALUE_DETECTED;\
  }
#endif

typedef struct {
  edjson_stack stack;
  char string_buffer[EDJSON_BUFFER_DEPTH];
  parse_string_state_t string_fsm_state;
  parse_number_state_t number_fsm_state;
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
  on_object_event_fn emit_event;
  on_element_name_fn on_element_name;
  on_element_value_fn on_element_value;
} json_parser_t;

typedef json_ret_codes_t ( * json_state_fptr_t ) ( json_parser_t * );
typedef int (*parser_fptr_t) (json_parser_t * );

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

int parse_string_constant(json_parser_t *parser );
int parse_string( json_parser_t *parser );
int parse_number( json_parser_t * parser );

parse_value_state_t value_recognition(json_parser_t *parser);

void flush_string_buffer(json_parser_t *parser);
edjson_err_t push_to_buffer(json_parser_t *parser, char symbol);

#ifdef __cplusplus
}
#endif

#endif //edJSON_H
