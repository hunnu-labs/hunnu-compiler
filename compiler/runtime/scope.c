/**
 * @file scope.c
 * @brief Scope management for variable lookups
 */

#include "scope.h"
#include <stdlib.h>
#include <string.h>

struct Scope {
    char** names;
    unsigned long* hashes;
    Value* values;
    int* is_mut;
    size_t capacity;
    size_t count;
    struct Scope* parent;
};

/* djb2 — cheap string hash so lookups skip strcmp on the common mismatch. */
static unsigned long scope_hash(const char* s) {
    unsigned long h = 5381;
    unsigned char c;
    while ((c = (unsigned char)*s++)) {
        h = ((h << 5) + h) + c;
    }
    return h;
}

Scope* scope_create(size_t capacity, Scope* parent) {
    Scope* scope = (Scope*)malloc(sizeof(Scope));
    scope->capacity = capacity;
    scope->count = 0;
    scope->parent = parent;
    scope->names = (char**)malloc(sizeof(char*) * capacity);
    scope->hashes = (unsigned long*)malloc(sizeof(unsigned long) * capacity);
    scope->values = (Value*)malloc(sizeof(Value) * capacity);
    scope->is_mut = (int*)malloc(sizeof(int) * capacity);
    return scope;
}

void scope_free(Scope* scope) {
    if (!scope) return;
    for (size_t i = 0; i < scope->count; i++) {
        free(scope->names[i]);
        value_free(&scope->values[i]);
    }
    free(scope->names);
    free(scope->hashes);
    free(scope->values);
    free(scope->is_mut);
    free(scope);
}

Value* scope_lookup(Scope* scope, const char* name) {
    unsigned long h = scope_hash(name);
    Scope* current = scope;
    while (current) {
        for (size_t i = 0; i < current->count; i++) {
            if (current->hashes[i] == h && strcmp(current->names[i], name) == 0) {
                return &current->values[i];
            }
        }
        current = current->parent;
    }
    return NULL;
}

Value* scope_lookup_local(Scope* scope, const char* name) {
    unsigned long h = scope_hash(name);
    for (size_t i = 0; i < scope->count; i++) {
        if (scope->hashes[i] == h && strcmp(scope->names[i], name) == 0) {
            return &scope->values[i];
        }
    }
    return NULL;
}

void scope_define(Scope* scope, const char* name, Value value, int is_mut) {
    unsigned long h = scope_hash(name);
    for (size_t i = 0; i < scope->count; i++) {
        if (scope->hashes[i] == h && strcmp(scope->names[i], name) == 0) {
            value_free(&scope->values[i]);
            scope->values[i] = value_copy(&value);
            scope->is_mut[i] = is_mut;
            return;
        }
    }

    if (scope->count >= scope->capacity) {
        scope->capacity *= 2;
        scope->names = (char**)realloc(scope->names, sizeof(char*) * scope->capacity);
        scope->hashes =
            (unsigned long*)realloc(scope->hashes, sizeof(unsigned long) * scope->capacity);
        scope->values = (Value*)realloc(scope->values, sizeof(Value) * scope->capacity);
        scope->is_mut = (int*)realloc(scope->is_mut, sizeof(int) * scope->capacity);
    }
    scope->names[scope->count] = strdup(name);
    scope->hashes[scope->count] = h;
    scope->values[scope->count] = value_copy(&value);
    scope->is_mut[scope->count] = is_mut;
    scope->count++;
}

int scope_is_mutable(Scope* scope, const char* name) {
    unsigned long h = scope_hash(name);
    Scope* current = scope;
    while (current) {
        for (size_t i = 0; i < current->count; i++) {
            if (current->hashes[i] == h && strcmp(current->names[i], name) == 0) {
                return current->is_mut[i];
            }
        }
        current = current->parent;
    }
    return 0;
}
