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

// ----------------- edjson_ret_t codes ---------------
typedef enum {
  EDJSON_OK = 0x00,
  EDJSON_FINISH = 1,
  EDJSON_ERR_RESOURCE_NOT_FOUND = -1,
  EDJSON_ERR_WRONG_SYMBOL = -3,
  EDJSON_EOF = -4,
  EDJSON_ERR_MEMORY_OVERFLOW = -5,
  EDJSON_ERR_PARSER = -7,
} json_ret_codes_t;

typedef enum {
  JSON_OBJECT,
  JSON_ARRAY,
  JSON_ATTRIBUTE
} json_element_type_t;


typedef enum {
  str_begin, str_body, reverse_solidus, hex_digits, str_end, // String FSM
} parse_string_state_t;

typedef enum {
  object_value = 0,
  string_value = 1,
  number_value = 2,
  array_value = 3,
  string_constant_value = 4,
  unknown_value = 5,
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

typedef struct {
  json_element_type_t kind;
  char *name;
  char *value;
} json_element_t;

#define DEFAULT_SPACE_SYMBOLS " \n\r\t"

typedef enum {
  OBJECT_START,         // Any object
  OBJECT_END,
  ARRAY_START,          // array
  ARRAY_END,
  FIELD_START,          // Field : value
  FIELD_NAME,
  FIELD_END,
  VALUE_START,
  VALUE_END
} edjson_event_kind_t;


typedef json_ret_codes_t ( *sys_init_deinit_fptr_t )(void);

typedef json_ret_codes_t ( *edjson_read_next )(char *buffer);

typedef json_ret_codes_t ( *edjson_error_handler )(json_ret_codes_t code, uint32_t position);

typedef json_ret_codes_t ( *on_object_event_fn )(int arg_cnt, ...);

typedef json_ret_codes_t ( *on_element_name_fn )(const char *node_name);

typedef json_ret_codes_t ( *on_element_value_fn )(const json_element_t *node);

#ifndef EDJSON_BUFFER_DEPTH
#define EDJSON_BUFFER_DEPTH 32      // length of most long name or string value
#endif

typedef struct {
  char string_buffer[EDJSON_BUFFER_DEPTH];
  parse_string_state_t string_fsm_state;
  parse_number_state_t number_fsm_state;
  uint8_t string_hex_fsm_state;
  uint16_t buffer_head;
  char current_symbol;
  uint32_t position;

  json_ret_codes_t last_rc;

  sys_init_deinit_fptr_t init;
  sys_init_deinit_fptr_t finish;
  edjson_read_next read;
  edjson_error_handler on_error;
  // ------------- events handlers ----------------
  on_object_event_fn emit_event;
  on_element_name_fn on_element_name;
  on_element_value_fn on_element_value;
} json_parser_t;

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

json_ret_codes_t parse_object(json_parser_t *parser);
json_ret_codes_t parse_attribute(json_parser_t *parser);
json_ret_codes_t parse_array(json_parser_t *parser);
json_ret_codes_t parse_value(parser_fptr_t _fptr, parse_value_state_t kind, json_parser_t *parser);
json_ret_codes_t detect_kind_of_value(json_parser_t *parser);
json_ret_codes_t parse(json_parser_t *parser);


json_ret_codes_t read_next(json_parser_t *parser, bool skip_spaces);

json_ret_codes_t parse_string_constant(json_parser_t *parser);

json_ret_codes_t parse_string(json_parser_t *parser);

json_ret_codes_t parse_number(json_parser_t *parser);

parse_value_state_t value_recognition(json_parser_t *parser);

void flush_string_buffer(json_parser_t *parser);

json_ret_codes_t push_to_buffer(json_parser_t *parser, char symbol);

#ifdef __cplusplus
}
#endif

#endif //edJSON_H
