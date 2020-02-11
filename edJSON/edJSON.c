//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#include "edJSON.h"

static const char * HEX_SYMBOLS = "0123456789aAbBcCdDeEfF";
static const char * QUOTES = "\"";
static const char SPACES[] = { ' ', '\n', '\r', '\t', 0x00 };
static const char NEWLINE_CHARS[] = {'\n', '\r'};

// ----------------- string buffer ------------------

static void flush_string_buffer( json_parser_t * parser ) {
  memset( parser->string_buffer, 0x00, EDJSON_BUFFER_DEPTH );
  parser->buffer_head = 0x00;
  parser->string_fsm_state = str_begin;
}

static edjson_err_t push_to_buffer( json_parser_t * parser, char symbol ) {
  if ( parser->buffer_head < EDJSON_BUFFER_DEPTH ) {
    parser->string_buffer[ parser->buffer_head ] = symbol;
    parser->buffer_head += 1;
    return EDJSON_OK;
  }
  return EDJSON_ERR_MEMORY_OVERFLOW;
}

// ------------------- fsm -----------------------
#define TRANSITION_COUNT 0    // всего N правил. Следует синхронизировать число с нижеследующей инициализацие правил
static const struct json_transition state_transitions[] = {
    { detect_obj,       JSON_OK,     parse_obj },

};

// Внимание. Это должно быть полностью синхронизировано с json_states_t
static const json_state_fptr_t states_fn[] = { detect_object,
                                               parse_object,

};

json_states_t lookup_transitions( json_states_t state, json_ret_codes_t code ) {
  for ( int i = 0 ; i < TRANSITION_COUNT ; i++ ) {
    if ( state_transitions[ i ].src == state && state_transitions[ i ].ret_codes == code )
      return state_transitions[ i ].dst;
  }
  return UNKNOWN_STATE;
}

edjson_err_t parse( json_parser_t * parser ) {
  edjson_err_t reading_state = parser->init();
  if ( reading_state != EDJSON_OK )
    return reading_state;
  flush_stack(parser->stack);
  parser->position = 0x00;
  json_states_t cur_state = ENTRY_PARSER_STATE;
  json_state_fptr_t fn;
  reading_state = parser->read( &parser->current_symbol );
  while ( reading_state == EDJSON_OK ) {
    fn = states_fn[ cur_state ];
    parser->last_rc = fn( parser );
    cur_state = lookup_transitions( cur_state, parser->last_rc );
    reading_state = parser->read( &parser->current_symbol );
    parser->position += 1;
  }
  return EDJSON_OK;
}

json_ret_codes_t detect_object( json_parser_t * parser ) {
  if ( parser->current_symbol == '{' ) {
    parser->on_parse_event(OBJECT_START);
    push(obj_begin, &parser->stack);
    return JSON_OK;
  } else if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  else
    return JSON_FAIL;
}

#ifndef FAIL_IF
#define FAIL_IF( expression ) do {\
  if (expression) \
    return JSON_FAIL; \
} while (0x00)
#endif

  // return
  // -1 if error,
  // 0  is ok
  // 1  is finished

static int parse_string(json_parser_t * parser) {
  switch (parser->string_fsm_state) {

  }
  return 0;
}

json_ret_codes_t parse_object( json_parser_t * parser ) {
  parse_object_state_t current_obj_parser_state = peek(&parser->stack);
  switch (current_obj_parser_state) {
    case obj_begin:
      if (strchr(SPACES, parser->current_symbol)) {
        return JSON_REPEAT;
      } else if (parser->current_symbol == '}') {
        FAIL_IF (flush_until(obj_begin, &parser->stack));
        parser->on_parse_event(OBJECT_END);
        return JSON_REPEAT; // todo check after comma
      } else if (parser->current_symbol == '"') {
        FAIL_IF (push(obj_name, &parser->stack));
        flush_string_buffer(parser);
        return JSON_REPEAT;
      }
      return JSON_FAIL;
    case obj_name:
      switch (parse_string(parser)) {
        case -1:
          return JSON_FAIL;
        case 0:
          return JSON_REPEAT;
        case 1:
          FAIL_IF (push(obj_colon, &parser->stack));
          return JSON_REPEAT;
      }
  }

}
/*json_ret_codes_t idle_state( json_parser_t * parser ) {
  if ( parser->current_symbol == '{' ) {
    parser->on_parse_event(OBJECT_START);
    return JSON_OK;
  } else if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  else
    return JSON_FAIL;
}

json_ret_codes_t object_state( json_parser_t * parser ) {
  if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  if (parser->current_symbol == '}')
    parser->on_parse_event(OBJECT_END);
  if ( strchr( QUOTES, parser->current_symbol ))
    return JSON_OK;
  return JSON_REPEAT;
}

json_ret_codes_t node_state( json_parser_t * parser ) {
  if ( strchr( QUOTES, parser->current_symbol )) {
    memcpy( parser->last_element, parser->string_buffer, EDJSON_BUFFER_DEPTH );
    parser->on_element_name( parser->last_element );
    return JSON_OK;
  }
  return PUSH_OR_FAIL();
}

json_ret_codes_t definition_state( json_parser_t * parser ) {
  if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  else if ( parser->current_symbol == ':' ) {
    flush_buffer( parser );
    parser->value_kind = unknown_value;
    return JSON_OK;
  }
  return JSON_REPEAT;
}

json_ret_codes_t value_state( json_parser_t * parser ) {
  if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  else if ( parser->current_symbol == '[' || parser->current_symbol == '{' ) {
    flush_buffer( parser );
    json_element_t node = {
        .kind = ( parser->current_symbol == '[' ) ? JSON_ARRAY : JSON_OBJECT,
        .name = parser->last_element
    };
    edjson_event_kind_t evnt_kind = ( parser->current_symbol == '[' ) ? ARRAY_START : OBJECT_START;
    parser->on_parse_event(evnt_kind);
    parser->on_element_value( &node, "" );
    return ( parser->current_symbol == '[' ) ? JSON_ARR : JSON_OBJ;
  } else if ( parser->value_kind == unknown_value ) {
    parser->value_kind = ( strchr( QUOTES, parser->current_symbol )) ? as_string_value : as_raw_value;
    return ( parser->value_kind == as_raw_value ) ? PUSH_OR_FAIL() : JSON_REPEAT;
  } else if (( strchr( QUOTES, parser->current_symbol ) && parser->value_kind == as_string_value ) ||
             ( !strchr( VALUE_SYMBOLS, parser->current_symbol ) && parser->value_kind == as_raw_value )) {
    json_element_t node = {
        .kind = JSON_VALUE,
        .name = parser->last_element
    };
    parser->on_element_value( &node, parser->string_buffer );
    parser->on_parse_event(ELEMENT_END);
    return JSON_OK;
  }
  return PUSH_OR_FAIL();
}

json_ret_codes_t close_state( json_parser_t * parser ) {
  if ( parser->current_symbol == ',' || parser->current_symbol == '}' ) {
    flush_buffer(parser);
    parser->on_parse_event(OBJECT_END);
    return JSON_OK;
  } else if ( parser->current_symbol == ']' ) {
    flush_buffer(parser);
    parser->on_parse_event(ARRAY_END);
    return JSON_OK;
  }{
    return JSON_REPEAT;
  }
}

json_ret_codes_t error_state( json_parser_t * parser ) {
  parser->on_error( parser->last_rc, parser->position );
  return JSON_OK;
}*/
