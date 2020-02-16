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
  printf("\n%04d > ", ptr->position);
  switch ( event_kind ) {
    case OBJECT_START:
      printf("{"); break;
    case OBJECT_END:
      printf("}"); break;
    case ARRAY_START:
      printf("["); break;
    case ARRAY_END:
      printf("]"); break;
    case FIELD_START:
      printf("."); break;
    case FIELD_NAME:
      printf("Attr name: \"%s\"", ptr->last_element); break;
    case FIELD_END:
      printf(";"); break;
    case VALUE_START:
      printf("="); break;
    case VALUE_END:
      printf("%s", ptr->string_buffer); break;
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
  return parse(&parser);
}
