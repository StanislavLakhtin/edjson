#include "edJSON/edJSON.h"

#define FILENAME "../example.json"

FILE *fp = NULL;

static edjson_err_t open_file( void ) {
  fp = fopen(FILENAME, "r");
  if (fp == NULL)
    return EDJSON_ERR_RESOURCE_NOT_FOUND;
  return EDJSON_OK;
}

static edjson_err_t read_from_file( char* buffer ) {
  *buffer = fgetc(fp);
  return (*buffer == EOF) ? EDJSON_EOF : EDJSON_OK;
}

static edjson_err_t error_handler( edjson_err_t code,  uint32_t position ) {
  printf("\nThere is an error (code %d) is JSON file in pos: %d", code, position);
  return EDJSON_OK;
}

static edjson_err_t on_object ( void) {
  printf("\nobject head");
  return EDJSON_OK;
}

edjson_err_t on_start_element_name_handler(const char* node_name) {
  printf("\nelement name: %s", node_name);
  return EDJSON_OK;
}

edjson_err_t on_element_value_handler(const json_element_t * node, const char * value) {
  printf("\nvalue detected: ");
  switch (node->kind){
    case JSON_OBJECT:
      printf("OBJECT"); break;
    case  JSON_ARRAY:
      printf("ARRAY"); break;
    case JSON_VALUE:
      printf("name: %s, value: %s", node->name, value); break;
  }
  return EDJSON_OK;
}

int main() {
  json_parser_t parser = {
      .init = open_file,
      .read = read_from_file,
      .on_error = error_handler,

      .on_start_object = on_object,
      .on_element_name = on_start_element_name_handler,
      .on_element_value = on_element_value_handler,
  };
  parse(&parser);
}
