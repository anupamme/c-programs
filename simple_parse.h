#ifdef JANSSON_H
#define JANSSON_H

#include <stdio.h>
#include <stdlib.h>
#include <jansson.h>

#ifdef __cplusplus
extern "C" {
#endif


/* getters, setters and manipulation */

void json_object_seed(size_t seed);
size_t json_object_size(const json_t *object);
json_t *json_object_get(const json_t *object, const char *key);
int json_object_set_new(json_t *object, const char *key, json_t *value);
int json_object_del(json_t *object, const char *key);
json_t *json_loads(const char *input, size_t flags, json_error_t *error);
json_t *json_loadf(FILE *input, size_t flags, json_error_t *error);

void print_json(json_t *root);
#ifdef __cplusplus
}
#endif
#endif
