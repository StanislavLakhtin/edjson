//
// Created by Stanislav Lakhtin on 14/02/2020.
//

#include "edJSON.h"

static const char *NUMBER_SYMBOLS = "0123456789eE.-";
static const char *ESCAPED_SYMBOLS = "\"\\/bfnrtu";
static const char *NEWLINE_CHARS = "\n\r";
static const char *HEX_SYMBOLS = "0123456789aAbBcCdDeEfF";

// return
// -1 if error,
// 0  is ok
// 1  is finished

static const char * TRUE_STRING = "true";
static const char * FALSE_STRING = "false";

int parse_boolean(json_parser_t *parser) {
  if (strstr(TRUE_STRING, parser->string_buffer) ) {
    if (strlen(parser->string_buffer) == strlen(TRUE_STRING))
      return 1;
    else
      push_to_buffer(parser, parser->current_symbol);
    return 0;
  } else if  ( strstr(FALSE_STRING, parser->string_buffer)) {
    if (strlen(parser->string_buffer) == strlen(FALSE_STRING))
      return 1;
    else
      push_to_buffer(parser, parser->current_symbol);
    return 0;
  }
  return -1;
}

int parse_string(json_parser_t *parser) {
  if (strchr(NEWLINE_CHARS, parser->current_symbol)) {
    return -1;
  }
  switch (parser->string_fsm_state) {
    case str_begin:
    case str_body:
      switch (parser->current_symbol) {
        case '\\':
          parser->string_fsm_state = reverse_solidus;
          break;
        case '"':
          parser->string_fsm_state = str_end;
          return 0x1;
      }
      push_to_buffer(parser, parser->current_symbol);
      return 0x00;
    case reverse_solidus:
      if (strchr(ESCAPED_SYMBOLS, parser->current_symbol)) {
        push_to_buffer(parser, parser->current_symbol);
      } else
        return -1;
      parser->string_fsm_state = (parser->current_symbol == 'u') ? hex_digits : str_body;
      parser->string_hex_fsm_state = 0x00;
      return 0x00;
    case hex_digits:
      if (!strchr(HEX_SYMBOLS, parser->current_symbol))
        return -1;
      push_to_buffer(parser, parser->current_symbol);
      parser->string_hex_fsm_state += 1;
      if (parser->string_hex_fsm_state > 3)
        parser->string_fsm_state = str_body;
      return 0x00;
    case str_end:
      return 0x01;
  }
  return 0;
}

parse_value_state_t value_recognition(json_parser_t *parser) {
  flush_string_buffer(parser);
  switch (parser->current_symbol) {
    case '"':
      return string_value;
    case '{':
      return object_value;
    case '[':
      return array_value;
    case 't':
    case 'f':
      push_to_buffer(parser, parser->current_symbol);
      return boolean_value;
    case 'n':
      push_to_buffer(parser, parser->current_symbol);
      return null_value;
    default:
      if (strchr(NUMBER_SYMBOLS, parser->current_symbol)) {
        push_to_buffer(parser, parser->current_symbol);
        return number_value;
      } else
        return unknown_value;
  }
}

json_ret_codes_t parse_number( json_parser_t * parser ) {
  return -1;
}