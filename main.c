#include "edJSON/edJSON.h"

#define FILENAME "../example.json"

FILE *fp = NULL;

static json_ret_codes_t open_file( void ) {
  fp = fopen(FILENAME, "r");
  if (fp == NULL)
    return EDJSON_ERR_RESOURCE_NOT_FOUND;
  return EDJSON_OK;
}

static json_ret_codes_t read_from_file( char* buffer ) {
  *buffer = fgetc(fp);
  return (*buffer == EOF) ? EDJSON_EOF : EDJSON_OK;
}

static json_ret_codes_t error_handler( json_ret_codes_t code,  uint32_t position ) {
  printf("\nThere is an error (code %d) is JSON file in pos: %d", code, position);
  return EDJSON_OK;
}

static json_ret_codes_t on_object ( edjson_event_kind_t event_kind, void * _ptr ) {
  json_parser_t * ptr = _ptr;
  printf("\nevent > ");
  switch ( event_kind ) {
    case OBJECT_START:
      printf("OBJECT_START"); break;
    case OBJECT_END:
      printf("OBJECT_END"); break;
    case ARRAY_START:
      printf("ARRAY_START"); break;
    case ARRAY_END:
      printf("ARRAY_END"); break;
    case FIELD_START:
      printf("FIELD_START"); break;
    case FIELD_END:
      printf("FIELD_END"); break;
    case VALUE_START:
      printf("VALUE_START"); break;
    case VALUE_END:
      printf("VALUE_END"); break;
  }
  return EDJSON_OK;
}

json_ret_codes_t on_start_element_name_handler(const char* node_name) {
  printf("\nfield name: %s", node_name);
  return EDJSON_OK;
}

json_ret_codes_t on_element_value_handler(const json_element_t * node) {
  printf("\nfield ");
  switch (node->kind){
    case JSON_OBJECT:
      printf("OBJECT"); break;
    case  JSON_ARRAY:
      printf("ARRAY"); break;
    case JSON_ATTRIBUTE:
      printf("name: %s, value: %s", node->name, node->value); break;
  }
  return EDJSON_OK;
}

int main() {
  json_parser_t parser = {
      .init = open_file,
      .read = read_from_file,
      .on_error = error_handler,

      .emit_event = on_object,
      .on_element_name = on_start_element_name_handler,
      .on_element_value = on_element_value_handler,
  };
  parse(&parser);
}
