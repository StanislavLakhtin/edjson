//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#include "edJSON.h"

static const char *SPACES = DEFAULT_SPACE_SYMBOLS;

// ----------------- string buffer ------------------

void flush_string_buffer(json_parser_t *parser) {
  memset(parser->string_buffer, 0x00, EDJSON_BUFFER_DEPTH);
  parser->buffer_head = 0x00;
  parser->string_fsm_state = str_begin;
  parser->number_fsm_state = number_begin;
}

json_ret_codes_t read_next(json_parser_t *parser, bool skip_spaces) {
  json_ret_codes_t err_code = parser->read(&parser->current_symbol);
  parser->position += 1;
  if (err_code)
    return err_code;
  while (skip_spaces && strchr(SPACES, parser->current_symbol)) {
    err_code = parser->read(&parser->current_symbol);
    parser->position += 1;
    if (err_code)
      return err_code;
  }
  return err_code;
}

json_ret_codes_t push_to_buffer(json_parser_t *parser, char symbol) {
  if (parser->buffer_head < EDJSON_BUFFER_DEPTH) {
    parser->string_buffer[parser->buffer_head] = symbol;
    parser->buffer_head += 1;
    return EDJSON_OK;
  }
  return EDJSON_ERR_MEMORY_OVERFLOW;
}

json_ret_codes_t parse(json_parser_t *parser) {
  json_ret_codes_t reading_state = parser->init();
  if (reading_state != EDJSON_OK)
    return reading_state;
  parser->position = 0x00;
  EDJSON_CHECK(read_next(parser, true));
  if (parser->current_symbol != '{') {
    return EDJSON_ERR_WRONG_SYMBOL;
  }
  return parse_object(parser);
}

json_ret_codes_t parse_object(json_parser_t *parser) {
  parser->emit_event(OBJECT_START, parser);
  // В неопределённом состоянии. Мы ожидаем только символа {
  EDJSON_CHECK(read_next(parser, true));
  json_ret_codes_t err_code;
  do {
    if (parser->current_symbol == '"') {
      parser->emit_event(FIELD_START, parser);
      // определяем имя атрибута
      parser->string_fsm_state = str_begin;
      do {
        EDJSON_CHECK(read_next(parser, true));
        err_code = parse_string(parser);
      } while (!err_code);
      if (err_code != EDJSON_FINISH)
        return err_code;
      EDJSON_CHECK(read_next(parser, true));
      if (parser->current_symbol != ':')
        return EDJSON_ERR_WRONG_SYMBOL;
      err_code = parse_value(parser);
      if (err_code)
        return err_code;
      parser->emit_event(FIELD_END, parser);
      // end attribute value
      EDJSON_CHECK(read_next(parser, true));
      if (parser->current_symbol == ',') {
        return parse_value(parser);
      }
    }
  } while (parser->current_symbol != '}');
}

json_ret_codes_t parse_value(json_parser_t *parser) {
  flush_string_buffer(parser);
  parser->emit_event(VALUE_START, parser);
  parser_fptr_t _fptr = NULL;
  int ret_code = EDJSON_ERR_WRONG_SYMBOL;
  EDJSON_CHECK(read_next(parser, true));
  parser->value_fsm_state = value_recognition(parser);
  switch (parser->value_fsm_state) {
    case string_value:
      _fptr = parse_string;
      break;
    case string_constant_value:
      _fptr = parse_string_constant;
      break;
    case number_value:
      _fptr = parse_number;
      break;
    case object_value:
      return parse_object(parser);
    case array_value:
      return parse_array(parser);
    default:
      return EDJSON_ERR_PARSER;
  }
  if (_fptr == NULL)
    return EDJSON_ERR_PARSER;
  do {
    ret_code = _fptr(parser);
    EDJSON_CHECK(read_next(parser, true));
  } while (!ret_code);
  if (ret_code != EDJSON_FINISH)
    return ret_code;
  parser->emit_event(VALUE_END, parser);
  return EDJSON_OK;
}

json_ret_codes_t parse_array(json_parser_t *parser) {
  parser->emit_event(ARRAY_START, parser);
  do {
    parse_value(parser);
    EDJSON_CHECK(read_next(parser, true));
    if (parser->current_symbol == ']') {
      return EDJSON_FINISH;
    }
  } while (parser->current_symbol != ',');
  parser->emit_event(ARRAY_END, parser);
}
