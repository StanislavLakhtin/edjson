//
// Created by Stanislav Lakhtin on 10/02/2020.
//

#include "edJSON.h"

static const char * VALUE_SYMBOLS = "0123456789truefalse.";
static const char * QUOTES = "\"";
static const char SPACES[] = { ' ', '\n', '\r', '\t', 0x00 };
static const char VALUE_END[] = { ',', '}' };

edjson_err_t as_int( const char * data, int * value ) {
  int sz = strlen( data );
  char * str = malloc( sz );
  memcpy( str, data, sz );
  for ( int i = 0 ; i < sz ; i++ ) {
    if ( strchr( QUOTES, str[ i ] ))
      str[ i ] = ' ';
  }
  *value = atoi( str );
  free( str );
  return EDJSON_OK;
}

edjson_err_t as_string( const char * data, char * buffer ) {
  int sz = strlen( data );
  buffer = malloc( sz );
  memcpy( buffer, data, sz );
  for ( int i = 0 ; i < sz ; i++ ) {
    if ( strchr( QUOTES, buffer[ i ] ))
      buffer[ i ] = ' ';
  }
  return EDJSON_OK;
}

// ------------------- fsm -----------------------
#define TRANSITION_COUNT 12    // всего N правил. Следует синхронизировать число с нижеследующей инициализацие правил
static const struct json_transition state_transitions[] = {
    { idle,       JSON_OK,     object },
    { idle,       JSON_REPEAT, idle },
    { object,     JSON_REPEAT, object },
    { object,     JSON_OK,     node },
    { node,       JSON_REPEAT, node },
    { node,       JSON_OK,     definition },
    { definition, JSON_REPEAT, definition },
    { definition, JSON_OK,     value },
    { definition, JSON_ALT,    idle },
    { value,      JSON_OK,     close },
    { close,      JSON_REPEAT,     close },
    { close,      JSON_OK,     idle },
};

static const json_state_fptr_t states_fn[] = { idle_state,
                                               object_state,
                                               error_state,
                                               node_state,
                                               definition_state,
                                               value_state,
                                               close_state,
};

#define PUSH_OR_FAIL()  (push_to_buffer(parser, parser->current_symbol)) ? JSON_FAIL : JSON_REPEAT

enum json_states_t lookup_transitions( enum json_states_t state, enum json_ret_codes_t code ) {
  for ( int i = 0 ; i < TRANSITION_COUNT ; i++ ) {
    if ( state_transitions[ i ].src == state && state_transitions[ i ].ret_codes == code )
      return state_transitions[ i ].dst;
  }
  return UNKNOWN_STATE;
}

static void flush_buffer( json_parser_t * parser ) {
  memset( parser->string_buffer, 0x00, EDJSON_BUFFER_DEPTH );
  parser->buffer_head = 0x00;
}

static edjson_err_t push_to_buffer( json_parser_t * parser, char symbol ) {
  if ( parser->buffer_head < EDJSON_BUFFER_DEPTH ) {
    parser->string_buffer[ parser->buffer_head ] = symbol;
    parser->buffer_head += 1;
    return EDJSON_OK;
  }
  return EDJSON_ERR_MEMORY_OVERFLOW;
}

edjson_err_t parse( json_parser_t * parser ) {
  edjson_err_t reading_state = parser->init();
  if ( reading_state != EDJSON_OK )
    return reading_state;
  flush_buffer( parser );
  parser->position = 0x00;
  enum json_states_t cur_state = ENTRY_PARSER_STATE;
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

enum json_ret_codes_t idle_state( json_parser_t * parser ) {
  if ( parser->current_symbol == '{' )
    return JSON_OK;
  else if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  else
    return JSON_FAIL;
}

enum json_ret_codes_t object_state( json_parser_t * parser ) {
  if ( strchr( SPACES, parser->current_symbol ))
    return JSON_REPEAT;
  parser->on_start_object();
  return JSON_OK;
}

enum json_ret_codes_t node_state( json_parser_t * parser ) {
  if ( strchr( QUOTES, parser->current_symbol )) {
    memcpy( parser->last_element, parser->string_buffer, EDJSON_BUFFER_DEPTH );
    parser->on_element_name( parser->last_element );
    return JSON_OK;
  }
  return PUSH_OR_FAIL();
}

enum json_ret_codes_t definition_state( json_parser_t * parser ) {
  if ( strchr( SPACES, parser->current_symbol ) || parser->current_symbol == ':' )
    return JSON_REPEAT;
  if ( strchr( QUOTES, parser->current_symbol ))
    parser->value_kind = as_string_value;
  else if ( parser->current_symbol == '[' ) {
    flush_buffer( parser );
    json_element_t node = {
        .kind = JSON_ARRAY,
        .name = parser->last_element
    };
    parser->on_element_value( &node, "" );
    return JSON_ALT;
  } else parser->value_kind = as_raw_value;
  flush_buffer( parser );
  return JSON_OK;
}

enum json_ret_codes_t value_state( json_parser_t * parser ) {
  if (( strchr( QUOTES, parser->current_symbol ) && parser->value_kind == as_string_value ) ||
      ( strchr( SPACES, parser->current_symbol ) && parser->value_kind == as_raw_value )) {
    json_element_t node = {
        .kind = JSON_VALUE,
        .name = parser->last_element
    };
    parser->on_element_value( &node, parser->string_buffer );
    return JSON_OK;
  }
  return PUSH_OR_FAIL();
}

enum json_ret_codes_t close_state( json_parser_t * parser ) {
  return (parser->current_symbol == ',' || parser->current_symbol == '}') ? JSON_OK : JSON_REPEAT;
}

enum json_ret_codes_t error_state( json_parser_t * parser ) {
  parser->on_error( parser->last_rc, parser->position );
  return JSON_OK;
}
