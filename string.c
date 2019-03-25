typedef unsigned long long size_t;
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);

#include "string.h"

String *string_new() {
  String *string = calloc(1, sizeof(String));

  int capacity = 64;
  string->capacity = capacity;
  string->length = 0;
  string->buffer = calloc(capacity, sizeof(char));
  string->buffer[0] = '\0';

  return string;
}

void string_push(String *string, char c) {
  string->buffer[string->length++] = c;

  if (string->length >= string->capacity) {
    string->capacity *= 2;
    string->buffer = realloc(string->buffer, sizeof(char) * string->capacity);
  }

  string->buffer[string->length] = '\0';
}

void string_write(String *string, char *s) {
  for (int i = 0; s[i]; i++) {
    string_push(string, s[i]);
  }
}
