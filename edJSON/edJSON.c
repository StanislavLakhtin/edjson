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
  json_ret_codes_t err_code = detect_kind_of_value(parser);
  reading_state = parser->finish();
  return (err_code == EDJSON_FINISH) ? EDJSON_OK : err_code;
}

json_ret_codes_t parse_object(json_parser_t *parser) {
  parser->emit_event(2, OBJECT_START, parser);
  // В неопределённом состоянии. Мы ожидаем только символа {
  EDJSON_CHECK(read_next(parser, true));
  json_ret_codes_t err_code;
  if (parser->current_symbol == '"') {
    do {
      err_code = parse_attribute(parser);
    } while (err_code == EDJSON_OK);
    if (err_code != EDJSON_FINISH)
      return err_code;
  }
  if (parser->current_symbol != '}')
    return EDJSON_ERR_WRONG_SYMBOL;
  parser->emit_event(2, OBJECT_END, parser);
  err_code = read_next(parser, true);
  return (err_code == EDJSON_EOF || err_code == EDJSON_OK)? EDJSON_FINISH : err_code;
}

json_ret_codes_t parse_attribute(json_parser_t *parser) {
  json_ret_codes_t err_code;
  parser->emit_event(2, FIELD_START, parser);
  // определяем имя атрибута
  flush_string_buffer(parser);
  do {
    EDJSON_CHECK(read_next(parser, false));
    err_code = parse_string(parser);
  } while (!err_code);
  if (err_code != EDJSON_FINISH)
    return err_code;
  //char * attribute_name = malloc(strlen(parser->string_buffer));
  //memcpy(attribute_name, parser->string_buffer, strlen(parser->string_buffer));
  parser->emit_event(2, FIELD_NAME, parser);
  //free(attribute_name);
  EDJSON_CHECK(read_next(parser, true));
  if (parser->current_symbol != ':')
    return EDJSON_ERR_WRONG_SYMBOL;

  err_code = detect_kind_of_value(parser);
  if (err_code)
    return err_code;
  parser->emit_event(2, FIELD_END, parser);
  if (parser->current_symbol == ',') {
    EDJSON_CHECK(read_next(parser, true));
    return EDJSON_OK;
  }
  return EDJSON_FINISH;
}

json_ret_codes_t detect_kind_of_value(json_parser_t *parser) {
  flush_string_buffer(parser);
  parser_fptr_t _fptr = NULL;
  EDJSON_CHECK(read_next(parser, true));
  parse_value_state_t _kind = value_recognition(parser);
  switch (_kind) {
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
      _fptr = parse_object;
      break;
    case array_value:
      _fptr = parse_array;
      break;
    default:
      return EDJSON_ERR_PARSER;
  }
  if (_fptr == NULL)
    return EDJSON_ERR_PARSER;
  return parse_value(_fptr, _kind, parser);
}

json_ret_codes_t parse_value(parser_fptr_t fptr, parse_value_state_t kind, json_parser_t *parser) {
  //todo разделить на два метода и в подчинённый передавать определённый тип
  parser->emit_event(2, VALUE_START, parser);
  int ret_code;
  do {
    if (kind == string_value
        || kind == string_constant_value
        || kind == number_value)
      EDJSON_CHECK(read_next(parser, false));
    ret_code = fptr(parser);
  } while (!ret_code);
  if (ret_code != EDJSON_FINISH)
    return ret_code;
  if (kind == string_value)
    EDJSON_CHECK(read_next(parser, true));     // compensate symbol '"'
  if (strchr(SPACES, parser->current_symbol))       // if value is the last in line (before })
    EDJSON_CHECK(read_next(parser, true));
  parser->emit_event(3, VALUE_END, parser, kind);
  return EDJSON_OK;
}

json_ret_codes_t parse_array(json_parser_t *parser) {
  parser->emit_event(2, ARRAY_START, parser);
  do {
    json_ret_codes_t err_code = detect_kind_of_value(parser);
    if (err_code)
      return err_code;
  } while (parser->current_symbol == ',');
  if (parser->current_symbol == ']') {
    parser->emit_event(2, ARRAY_END, parser);
    EDJSON_CHECK(read_next(parser, false));
    return EDJSON_FINISH;
  }
  return EDJSON_ERR_WRONG_SYMBOL;
}
