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

// ------------------- fsm -----------------------

json_ret_codes_t handle_error(json_parser_t *parser) {
  parser->on_error(parser->last_rc, parser->position);
  return EDJSON_OK;
}

#define TRANSITION_COUNT 1    // всего N правил. Следует синхронизировать число с нижеследующей инициализацие правил
static const struct json_transition state_transitions[TRANSITION_COUNT] = {
    {parse_object, EDJSON_ROOT_DETECTED, parse_obj},
    {parse_obj,    OBJECT_DETECTED,      parse_obj},
    {parse_obj,    ARRAY_DETECTED,       parse_arr},
    {parse_obj,    VALUE_DETECTED,       parse_val},
    {parse_arr,    VALUE_DETECTED,       parse_val},
    {parse_arr,    OBJECT_DETECTED,      parse_obj},
    {parse_arr,    ARRAY_DETECTED,       parse_arr},
    {parse_val,    VALUE_ENDED,          detect_obj},
    {parse_val,    OBJECT_DETECTED,      parse_obj},
};

// Внимание. Это должно быть полностью синхронизировано с json_states_t
static const json_state_fptr_t states_fn[] = {handle_error,
                                              parse_object,
                                              parse_array,
                                              parse_attribute,
                                              parse_value,
};

json_states_t lookup_transitions(json_states_t state, json_ret_codes_t code) {
  for (int i = 0; i < TRANSITION_COUNT; i++) {
    if (state_transitions[i].src == state && state_transitions[i].ret_codes == code)
      return state_transitions[i].dst;
  }
  return UNKNOWN_STATE;
}

json_ret_codes_t parse(json_parser_t *parser) {
  json_ret_codes_t reading_state = parser->init();
  if (reading_state != EDJSON_OK)
    return reading_state;
  flush_stack(parser->stack);
  parser->position = 0x00;
  json_states_t cur_state = ENTRY_PARSER_STATE;
  json_state_fptr_t fn;
  reading_state = parser->read(&parser->current_symbol);
  while (reading_state == EDJSON_OK) {
    fn = states_fn[cur_state];
    parser->last_rc = fn(parser);
    cur_state = (parser->last_rc == EDJSON_REPEAT) ? cur_state : lookup_transitions(cur_state, parser->last_rc);
    if (parser->last_rc != EDJSON_REPEAT_WITH_SKIP_READ)
      EDJSON_CHECK(read_next(parser,
                             true)); // skip any spaces between operation states. Current_symbol always contains significant symbol
  }
  return EDJSON_OK;
}

json_ret_codes_t push_state(parse_object_state_t state, json_parser_t *parser) {
  json_ret_codes_t err_code = push(state, &parser->stack);
  if (err_code)
    return err_code;
  switch (state) {
    case obj_begin:
      parser->emit_event(OBJECT_START, parser);
  }
  return EDJSON_OK;
}

json_ret_codes_t parse_object(json_parser_t *parser) {
  parse_object_state_t _st = peek(&parser->stack);
  json_ret_codes_t err_code = EDJSON_ERR_PARSER;
  switch (_st) {
    case unknown:
      // В неопределённом состоянии. Мы ожидаем только символа {
      if (parser->current_symbol == '{') {
        EDJSON_CHECK(push_state(obj_begin, parser));
        return EDJSON_REPEAT;
      }
    case obj_begin:
      if (parser->current_symbol != '"' && parser->current_symbol != '}')
        return EDJSON_ERR_WRONG_SYMBOL;
      if (parser->current_symbol == '}') {
        EDJSON_CHECK(push_state(obj_end, parser));
        return EDJSON_FINISH;
      }
      EDJSON_CHECK(push_state(attr_name, parser));
      return EDJSON_REPEAT;
    case attr_name: // определяем имя атрибута
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
      EDJSON_CHECK(push(value_begin, &parser->stack));
      return EDJSON_VALUE_DETECTED;
    case value_end:
      if (parser->current_symbol != '"')
        return EDJSON_ERR_WRONG_SYMBOL;
      EDJSON_CHECK(push_state(attr_name, parser));
      return EDJSON_REPEAT;
    case obj_end:
      // flush stack
      // Очистить стек следует включая имя атрибута (только так это имеет смысл)
      // Кроме имени атрибута, объект может находиться внутри массива (очистка стека на совести массива)
      flush_until(obj_begin, &parser->stack);
      _st = peek(&parser->stack);
      if (_st == attr_name)
        flush_until(attr_name, &parser->stack);
      // дальше может быть:
      //  запятая (следующий элемент массива или следующий аттрибут)
      //  вновь закрывающая скобка (окончание вышестоящего аттрибута )
      //  квадратная закрывающая скобка (окончание массива)
      return EDJSON_DETECT_NEXT;
    default:
      return EDJSON_ERR_PARSER;
  }
}

json_ret_codes_t parse_value(json_parser_t *parser) {
  parse_object_state_t current_value_parser_state = peek(&parser->stack);
  parser_fptr_t _fptr = NULL;
  int ret_code = EDJSON_ERR_WRONG_SYMBOL;
  switch (current_value_parser_state) {
    case value_begin:
      EDJSON_CHECK(read_next(parser, true));
      parser->value_fsm_state = value_recognition(parser);
      EDJSON_CHECK(push_state(value_body, parser));
      switch (parser->value_fsm_state) {
        case string_value:
          parser->string_fsm_state = str_begin;
          break;
        case number_value:
          parser->number_fsm_state = number_begin;
          break;
      }
      return EDJSON_REPEAT;
    case value_body:
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
          EDJSON_CHECK(push_state(obj_begin, parser));
          return EDJSON_OBJECT_DETECTED;
        case array_value:
          EDJSON_CHECK(push_state(array_begin, parser));
          return EDJSON_ARRAY_DETECTED;
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
      EDJSON_CHECK(push_state(value_end, parser));
      return EDJSON_REPEAT_WITH_SKIP_READ;
    case value_end:
      // Поскольку строка заканчивается кавычкой, то здесь либо запятая, либо кавычка
      if (parser->current_symbol != '"')
        EDJSON_CHECK(read_next(parser, true));
      // Запятая после значения может стоять в двух случаях:
      // 1.
      if (parser->current_symbol != ',') {
        return EDJSON_ATTRIBUTE_DETECTED;
      }
      if (parser->current_symbol == '}') {
        EDJSON_CHECK(push_state(obj_end, parser));
        return EDJSON_FINISH;
      }
      return EDJSON_ERR_WRONG_SYMBOL;
  }
}

json_ret_codes_t parse_array(json_parser_t *parser) {
  parse_object_state_t _st = peek(&parser->stack);
  int arr_cnt;
  switch (_st) {
    case array_begin:
      if (parser->current_symbol == ']') {
        EDJSON_CHECK(push_state(array_end, parser));
        return EDJSON_REPEAT;
      }
      EDJSON_CHECK(push_state(value_begin, parser));
      return EDJSON_VALUE_DETECTED;
    case array_end:
      arr_cnt = 0x01;
      // сворачиваем массив, какой бы странный он ни был
      do {
        EDJSON_CHECK(pop(&_st, &parser->stack));
        if (_st == array_end)
          arr_cnt += 1;
      } while (!arr_cnt);
      EDJSON_CHECK((flush_until(attr_name, &parser->stack)));
      return EDJSON_FINISH;
  }
}
