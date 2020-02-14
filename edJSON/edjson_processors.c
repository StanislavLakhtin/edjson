//
// Created by Stanislav Lakhtin on 14/02/2020.
//

#include "edJSON.h"

static const char * NUMBERS_ONLY_SYMBOLS = "0123456789";
static const char * NUMBER_SYMBOLS = "0123456789eE.-";
static const char * ESCAPED_SYMBOLS = "\"\\/bfnrtu";
static const char * NEWLINE_CHARS = "\n\r";
static const char * HEX_SYMBOLS = "0123456789aAbBcCdDeEfF";
static const char * SPACE_SYMBOLS = DEFAULT_SPACE_SYMBOLS;
static const char * TRUE_STRING = "true";
static const char * FALSE_STRING = "false";
static const char * NULL_STRING = "null";



int parse_boolean(json_parser_t *parser) {
  if (strstr(TRUE_STRING, parser->string_buffer) ) {
    if (strlen(parser->string_buffer) == strlen(TRUE_STRING))
      return EDJSON_FINISH;
    else
      push_to_buffer(parser, parser->current_symbol);
    return EDJSON_OK;
  } else if  ( strstr(FALSE_STRING, parser->string_buffer)) {
    if (strlen(parser->string_buffer) == strlen(FALSE_STRING))
      return EDJSON_FINISH;
    else
      push_to_buffer(parser, parser->current_symbol);
    return EDJSON_OK;
  }
  return EDJSON_ERR_WRONG_SYMBOL;
}

int parse_string(json_parser_t *parser) {
  if (strchr(NEWLINE_CHARS, parser->current_symbol)) {
    return EDJSON_ERR_WRONG_SYMBOL;
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
      return EDJSON_OK;
    case reverse_solidus:
      if (strchr(ESCAPED_SYMBOLS, parser->current_symbol)) {
        push_to_buffer(parser, parser->current_symbol);
      } else
        return EDJSON_ERR_WRONG_SYMBOL;
      parser->string_fsm_state = (parser->current_symbol == 'u') ? hex_digits : str_body;
      parser->string_hex_fsm_state = 0x00;
      return EDJSON_OK;
    case hex_digits:
      if (!strchr(HEX_SYMBOLS, parser->current_symbol))
        return EDJSON_ERR_WRONG_SYMBOL;
      push_to_buffer(parser, parser->current_symbol);
      parser->string_hex_fsm_state += 1;
      if (parser->string_hex_fsm_state > 3)
        parser->string_fsm_state = str_body;
      return EDJSON_OK;
    case str_end:
      return 0x01;
  }
  return EDJSON_OK;
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

// number_begin, number_sign, number_zero, number_digit, number_dot, number_digit_after_dot, number_e, number_e_sign, number_e_digit,  // number FSM

static const char * NUMBER_ZERO_E_STATE = ".eE";

int parse_number( json_parser_t * parser ) {
  switch (parser->number_fsm_state) {
    case number_begin:
      if ( parser->current_symbol == '-') {
        parser->number_fsm_state = number_sign;
        push_to_buffer(parser, parser->current_symbol);
        return EDJSON_OK;
      } else if (!strchr(NUMBERS_ONLY_SYMBOLS, parser->current_symbol)) {
        return EDJSON_ERR_WRONG_SYMBOL;
      }
      parser->number_fsm_state = (parser->current_symbol == '0') ? number_zero : number_digit;
      push_to_buffer(parser, parser->current_symbol);
      return EDJSON_OK;
    case number_digit:
      if (strchr(NUMBERS_ONLY_SYMBOLS, parser->current_symbol)) {
        push_to_buffer(parser, parser->current_symbol);
        return EDJSON_OK;
      } // do NOT return nothing here!
    case number_zero:
      if (strchr(SPACE_SYMBOLS, parser->current_symbol) || parser->current_symbol == ',')
        return EDJSON_FINISH;
      if (!strchr(NUMBER_ZERO_E_STATE, parser->current_symbol))
        return EDJSON_ERR_WRONG_SYMBOL;
      parser->number_fsm_state = (parser->current_symbol == '.') ? number_dot : number_e;
      push_to_buffer(parser, parser->current_symbol);
      return EDJSON_OK;
    case  number_dot:
      if (strchr(NUMBERS_ONLY_SYMBOLS, parser->current_symbol)) {
        push_to_buffer(parser, parser->current_symbol);
        return EDJSON_OK;
      }
      if (strchr(SPACE_SYMBOLS, parser->current_symbol) || parser->current_symbol == ',')
        return EDJSON_FINISH;
      if (parser->current_symbol == 'e' || parser->current_symbol == 'E') {
        parser->number_fsm_state = number_e_sign;
        push_to_buffer(parser, parser->current_symbol);
        return EDJSON_OK;
      }
      return EDJSON_ERR_WRONG_SYMBOL;
    case number_e_sign:
      if (parser->current_symbol=='-' || parser->current_symbol=='+') {
        parser->number_fsm_state = number_e_digit;
        push_to_buffer(parser, parser->current_symbol);
      } // do NOT return nothing here!
    case number_e_digit:
      if (strchr(NUMBERS_ONLY_SYMBOLS, parser->current_symbol)) {
        push_to_buffer(parser, parser->current_symbol);
        return EDJSON_OK;
      }
      if (strchr(SPACE_SYMBOLS, parser->current_symbol) || parser->current_symbol == ',')
        return EDJSON_FINISH;
      return EDJSON_ERR_WRONG_SYMBOL;
  }
  return EDJSON_ERR_WRONG_SYMBOL;
}