//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#include "edJSON.h"

static const char *HEX_SYMBOLS = "0123456789aAbBcCdDeEfF";
static const char *SPACES = " \n\r\t";
static const char *NEWLINE_CHARS = "\n\r";
static const char *ESCAPED_SYMBOLS = "\"\\/bfnrtu";

// ----------------- string buffer ------------------

static void flush_string_buffer(json_parser_t *parser) {
  memset(parser->string_buffer, 0x00, EDJSON_BUFFER_DEPTH);
  parser->buffer_head = 0x00;
  parser->string_fsm_state = str_begin;
}

static edjson_err_t push_to_buffer(json_parser_t *parser, char symbol) {
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

#define TRANSITION_COUNT 3    // всего N правил. Следует синхронизировать число с нижеследующей инициализацие правил
static const struct json_transition state_transitions[TRANSITION_COUNT] = {
    {detect_obj, OBJECT_DETECTED, parse_obj},
    {parse_obj,  OBJECT_DETECTED, parse_obj},
    {parse_obj,  ARRAY_DETECTED,  parse_arr},
};

// Внимание. Это должно быть полностью синхронизировано с json_states_t
static const json_state_fptr_t states_fn[] = {handle_error,
                                              detect_object,
                                              parse_object,
                                              parse_array,
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
  if (parser->current_symbol == '{') {
    parser->on_parse_event(OBJECT_START);
    push(obj_begin, &parser->stack);
    return OBJECT_DETECTED;
  } else if (strchr(SPACES, parser->current_symbol))
    return REPEAT_PLEASE;
  else
    return PARSER_FAIL;
}

// return
// -1 if error,
// 0  is ok
// 1  is finished

static int parse_string(json_parser_t *parser) {
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

json_ret_codes_t parse_object(json_parser_t *parser) {
  parse_object_state_t current_obj_parser_state = peek(&parser->stack);
  switch (current_obj_parser_state) {
    case obj_begin:
      if (strchr(SPACES, parser->current_symbol)) {
        return REPEAT_PLEASE;
      } else if (parser->current_symbol == '}') {
        FAIL_IF (flush_until(obj_begin, &parser->stack));
        parser->on_parse_event(OBJECT_END);
        return REPEAT_PLEASE; // todo check after comma
      } else if (parser->current_symbol == '"') {
        FAIL_IF (push(obj_name, &parser->stack));
        flush_string_buffer(parser);
        return REPEAT_PLEASE;
      }
      return PARSER_FAIL;
    case obj_name:
      switch (parse_string(parser)) {
        case -1:
          return PARSER_FAIL;
        case 0:
          return REPEAT_PLEASE;
        case 1:
          memcpy(parser->last_element, parser->string_buffer, EDJSON_BUFFER_DEPTH);
          FAIL_IF (push(obj_colon, &parser->stack));
          parser->on_parse_event(ELEMENT_START);
          return REPEAT_PLEASE;
      }
    case obj_colon:
      // detect value kind
      if (strchr(SPACES, parser->current_symbol)) {
        return REPEAT_PLEASE;
      }

  }
}


static parse_value_state_t value_recognition(json_parser_t *parser) {
  switch (parser->current_symbol) {
    case '"':
      return string_value;
    case '{':
      return object_value;
    case '[':
      return array_value;
    case 't':
      return true_value;
    case 'f':
      return false_value;
    case 'n':
      return null_value;
    default:
      return number_value;
  }
}

json_ret_codes_t parse_array(json_parser_t *parser) {
  parse_object_state_t current_arr_parser_state = peek(&parser->stack);
  switch (current_arr_parser_state) {
    case array_begin:
      if (strchr(SPACES, parser->current_symbol))
        return REPEAT_PLEASE;
      if (parser->current_symbol == ']') {
        FAIL_IF (push(array_end, &parser->stack));
        parser->on_parse_event(ARRAY_END);
        return REPEAT_PLEASE;
      }
      return REPEAT_PLEASE;
    case
  }
}
