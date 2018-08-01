#include "cc.h"

Type *type_new() {
  Type *type = (Type *) malloc(sizeof(Type));
  return type;
}

Type *type_char() {
  Type *char_type = type_new();
  char_type->type = CHAR;
  char_type->size = 1;
  return char_type;
}

Type *type_int() {
  Type *int_type = type_new();
  int_type->type = INT;
  int_type->size = 4;
  return int_type;
}

Type *type_pointer_to(Type *type) {
  Type *pointer = type_new();
  pointer->type = POINTER;
  pointer->pointer_to = type;
  pointer->size = 8;
  return pointer;
}

Type *type_array_of(Type *type, int array_size) {
  Type *array = type_new();
  array->type = ARRAY;
  array->array_of = type;
  array->array_size = array_size;
  array->size = type->size * array_size;
  return array;
}

Type *type_convert(Type *type) {
  if (type->type == ARRAY) {
    return type_pointer_to(type->array_of);
  }
  return type;
}
