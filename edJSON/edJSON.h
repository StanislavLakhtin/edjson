//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#ifndef edJSON_H
#define edJSON_H

#include "edJSON_def.h"

#include "edJSON_stack.h"

#define DEFAULT_SPACE_SYMBOLS " \n\r\t"

typedef enum {
  ROOT_START,           // Root Object
  ROOT_END,
  OBJECT_START,         // Any other (except root) object
  OBJECT_END,
  ARRAY_START,          // array
  ARRAY_END,
  FIELD_START,          // Field : value
  FIELD_END,
  VALUE_START,
  VALUE_END
} edjson_event_kind_t;

typedef json_ret_codes_t ( *edjson_to_begin )(void);

typedef json_ret_codes_t ( *edjson_read_next )(char *buffer);

typedef json_ret_codes_t ( *edjson_error_handler )(json_ret_codes_t code, uint32_t position);

typedef json_ret_codes_t ( *on_object_event_fn )(edjson_event_kind_t event_kind, void *);

typedef json_ret_codes_t ( *on_element_name_fn )(const char *node_name);

typedef json_ret_codes_t ( *on_element_value_fn )(const json_element_t *node);

#ifndef EDJSON_BUFFER_DEPTH
#define EDJSON_BUFFER_DEPTH 32      // length of most long name or string value
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

typedef json_ret_codes_t ( *json_state_fptr_t )(json_parser_t *);

typedef int (*parser_fptr_t)(json_parser_t *);

#define EDJSON_CHECK(VAL) do { \
  json_ret_codes_t _err = VAL; \
  if (_err) \
    return _err; \
} while(0)\

#ifdef __cplusplus
extern "C"
{
#endif

json_ret_codes_t handle_error(json_parser_t *parser);

json_ret_codes_t parse_object(json_parser_t *parser);

json_ret_codes_t parse_array(json_parser_t *parser);

json_ret_codes_t parse_attribute(json_parser_t *parser);

json_ret_codes_t parse_value(json_parser_t *parser);


json_ret_codes_t parse(json_parser_t *parser);

json_states_t lookup_transitions(json_states_t state, json_ret_codes_t code);

json_ret_codes_t read_next(json_parser_t *parser, bool skip_spaces);

int parse_string_constant(json_parser_t *parser);

int parse_string(json_parser_t *parser);

int parse_number(json_parser_t *parser);

parse_value_state_t value_recognition(json_parser_t *parser);

void flush_string_buffer(json_parser_t *parser);

json_ret_codes_t push_to_buffer(json_parser_t *parser, char symbol);

json_ret_codes_t push_state(parse_object_state_t state, json_parser_t *parser);

#ifdef __cplusplus
}
#endif

#endif //edJSON_H
