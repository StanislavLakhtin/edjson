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

edjson_err_t push_to_buffer(json_parser_t *parser, char symbol) {
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

#define TRANSITION_COUNT 9    // всего N правил. Следует синхронизировать число с нижеследующей инициализацие правил
static const struct json_transition state_transitions[TRANSITION_COUNT] = {
    {detect_obj, OBJECT_DETECTED, parse_obj},
    {parse_obj,  OBJECT_DETECTED, parse_obj},
    {parse_obj,  ARRAY_DETECTED,  parse_arr},
    {parse_obj,  VALUE_DETECTED,  parse_val},
    {parse_arr,  VALUE_DETECTED,  parse_val},
    {parse_arr,  OBJECT_DETECTED, parse_obj},
    {parse_arr,  ARRAY_DETECTED,  parse_arr},
    {parse_val,  VALUE_ENDED,     detect_obj},
    {parse_val,  OBJECT_DETECTED, parse_obj},
};

// Внимание. Это должно быть полностью синхронизировано с json_states_t
static const json_state_fptr_t states_fn[] = {handle_error,
                                              detect_object,
                                              parse_object,
                                              parse_array,
                                              parse_value,
};

json_states_t lookup_transitions(json_states_t state, json_ret_codes_t code) {
  for (int i = 0; i < TRANSITION_COUNT; i++) {
    if (state_transitions[i].src == state && state_transitions[i].ret_codes == code)
      return state_transitions[i].dst;
  }
  return UNKNOWN_STATE;
}

edjson_err_t parse(json_parser_t *parser) {
  edjson_err_t reading_state = parser->init();
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
    cur_state = (parser->last_rc == REPEAT_PLEASE) ? cur_state : lookup_transitions(cur_state, parser->last_rc);
    reading_state = parser->read(&parser->current_symbol);
    parser->position += 1;
  }
  return EDJSON_OK;
}

json_ret_codes_t detect_object(json_parser_t *parser) {
  SKIP_SPACES();
  if (parser->current_symbol == '{') {
    parser->emit_event(OBJECT_START, parser);
    push(obj_begin, &parser->stack);
    return OBJECT_DETECTED;
  } else if (strchr(SPACES, parser->current_symbol))
    return REPEAT_PLEASE;
  else
    return PARSER_FAIL;
}

json_ret_codes_t parse_object(json_parser_t *parser) {
  parse_object_state_t current_obj_parser_state = peek(&parser->stack);
  switch (current_obj_parser_state) {
    case obj_begin:
      SKIP_SPACES();
      if (parser->current_symbol == '}') {
        FAIL_IF (flush_until(obj_begin, &parser->stack));
        parser->emit_event(OBJECT_END, parser);
        return REPEAT_PLEASE; // todo check after comma
      } else if (parser->current_symbol == '"') {
        FAIL_IF (push(obj_name, &parser->stack));
        flush_string_buffer(parser);
        return REPEAT_PLEASE;
      }
      return PARSER_FAIL;
    case obj_name:
      switch (parse_string(parser)) {
        case EDJSON_ERR_WRONG_SYMBOL:
          return PARSER_FAIL;
        case EDJSON_OK:
          return REPEAT_PLEASE;
        case EDJSON_FINISH:
          memcpy(parser->last_element, parser->string_buffer, EDJSON_BUFFER_DEPTH);
          FAIL_IF (push(obj_colon_wait, &parser->stack));
          parser->emit_event(ELEMENT_START, parser);
          return REPEAT_PLEASE;
      }
    case obj_colon_wait:
      SKIP_SPACES();
      if (parser->current_symbol == ':') {
        parse_object_state_t _w;
        pop(&_w, &parser->stack);
        FAIL_IF (push(obj_colon, &parser->stack));
        parser->emit_event(ATTRIBUTE_START, parser);
        return REPEAT_PLEASE;
      }
    case obj_colon:
      SKIP_SPACES();
      flush_string_buffer(parser);
      parse_value_state_t _val_detect = value_recognition(parser);
      GENERATE_VALUE_SIGNAL(_val_detect, parser);
  }
  return PARSER_FAIL;
}

json_ret_codes_t parse_value(json_parser_t *parser) {
  parse_object_state_t current_value_parser_state = peek(&parser->stack);
  parser_fptr_t _fptr = NULL;
  int ret_code = EDJSON_ERR_WRONG_SYMBOL;
  switch (current_value_parser_state) {
    case value_begin:
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
      }
      if (_fptr != NULL) {
        ret_code = _fptr(parser);
        switch (ret_code) {
          case EDJSON_ERR_WRONG_SYMBOL:
            return PARSER_FAIL;
          case EDJSON_OK:
            return REPEAT_PLEASE;
          case EDJSON_FINISH:
            FAIL_IF (push(value_end, &parser->stack));
            json_element_t node = DEFAULT_VALUE_NODE(parser->string_buffer);
            parser->on_element_value(&node);
            parser->emit_event(VALUE_END, parser);
            return REPEAT_PLEASE;
          default:
            return PARSER_FAIL;
        }
      } else {
        return PARSER_FAIL;
      }

    case value_end:
      SKIP_SPACES();
      if (parser->current_symbol == ',') {
        FAIL_IF (flush_until(obj_begin, &parser->stack));
        push(obj_begin, &parser->stack);
        return OBJECT_DETECTED;
      } else if (parser->current_symbol == '}') {
        FAIL_IF (flush_until(obj_begin, &parser->stack));
        parser->emit_event(OBJECT_END, parser);
        return VALUE_ENDED;
      }
      return PARSER_FAIL;
  }
}


json_ret_codes_t parse_array(json_parser_t *parser) {
  SKIP_SPACES();
  parse_object_state_t current_arr_parser_state = peek(&parser->stack);
  switch (current_arr_parser_state) {
    case array_begin:
      SKIP_SPACES();
      if (parser->current_symbol == ']') {
        FAIL_IF (flush_until(array_begin, &parser->stack));
        parser->emit_event(ARRAY_END, parser);
        return REPEAT_PLEASE;
      }
      parse_value_state_t _val_detect = value_recognition(parser);
      GENERATE_VALUE_SIGNAL(_val_detect, parser);
  }
  return PARSER_FAIL;
}
