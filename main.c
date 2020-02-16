#include "edJSON/edJSON.h"
#include <stdarg.h>

#define DEFAULT_FILENAME "../example.json"

static char * filename;
FILE *fp = NULL;

static json_ret_codes_t open_file() {
  fp = fopen(filename, "r");
  if (fp == NULL) {
    printf("\nERROR: Can't open file\n");
    return EDJSON_ERR_RESOURCE_NOT_FOUND;
  }
  return EDJSON_OK;
}

static json_ret_codes_t close_file(void) {
  if (fp != NULL)
    fclose(fp);
  return EDJSON_OK;
}

static json_ret_codes_t read_from_file(char *buffer) {
  *buffer = fgetc(fp);
  return (*buffer == EOF) ? EDJSON_EOF : EDJSON_OK;
}

static json_ret_codes_t error_handler(json_ret_codes_t code, uint32_t position) {
  printf("\nThere is an error (code %d) is JSON file in pos: %d", code, position);
  return EDJSON_OK;
}

static char *val_kind[] = {
    "object",
    "string",
    "number",
    "array",
    "string_constant",
    "unknown",
};

static json_ret_codes_t on_object(int num, ... ) {

  va_list valist;
  va_start(valist, num);
  if (num <2) {
    printf("Error in parse event method");
    va_end(valist);
    return -1;
  }
  edjson_event_kind_t event_kind = va_arg(valist, edjson_event_kind_t);
  json_parser_t *ptr = va_arg(valist, json_parser_t*);
  printf("\n%04d > ", ptr->position);
  switch (event_kind) {
    case OBJECT_START:
      printf("{");
      break;
    case OBJECT_END:
      printf("}");
      break;
    case ARRAY_START:
      printf("[");
      break;
    case ARRAY_END:
      printf("]");
      break;
    case FIELD_START:
      printf(".");
      break;
    case FIELD_NAME:
      printf("Attr name: %s", ptr->string_buffer); // todo improve events
      break;
    case FIELD_END:
      printf(";");
      break;
    case VALUE_START:
      printf("=");
      break;
    case VALUE_END:
      if (num != 3) {
        printf("Get value event error");
        break;
      }
      parse_value_state_t kind = va_arg(valist, parse_value_state_t);
      printf("%s, \"%s\"", val_kind[kind], ptr->string_buffer);
      break;
  }
  va_end(valist);
  return EDJSON_OK;
}

json_ret_codes_t on_start_element_name_handler(const char *node_name) {
  printf("\nfield name: %s", node_name);
  return EDJSON_OK;
}

json_ret_codes_t on_element_value_handler(const json_element_t *node) {
  printf("\nfield ");
  switch (node->kind) {
    case JSON_OBJECT:
      printf("OBJECT");
      break;
    case JSON_ARRAY:
      printf("ARRAY");
      break;
    case JSON_ATTRIBUTE:
      printf("name: %s, value: %s", node->name, node->value);
      break;
  }
  return EDJSON_OK;
}

int main(int argc, char* argv[]) {
  json_parser_t parser = {
      .init = open_file,
      .finish = close_file,
      .read = read_from_file,
      .on_error = error_handler,

      .emit_event = on_object,
      .on_element_name = on_start_element_name_handler,
      .on_element_value = on_element_value_handler,
  };
  if( argc > 2) {
    printf("Expected only one argument: filename of json file");
  } else if ( argc == 1 ) {
    filename = DEFAULT_FILENAME;
  } else
    filename = argv[1];
  printf("\nUsing input json filename: %s", filename);
  json_ret_codes_t err_code = parse(&parser);
  printf("\nProcess finished with exit code %d\n", err_code);
  return err_code;
}
