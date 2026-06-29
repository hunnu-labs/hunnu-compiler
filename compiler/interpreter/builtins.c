#include "builtins.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

Value builtin_str_to_upper(const char* s) {
    size_t len = strlen(s);
    char* result = malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        result[i] = (char)toupper((unsigned char)s[i]);
    }
    result[len] = '\0';
    Value v = value_create_string(result);
    free(result);
    return v;
}

Value builtin_str_to_lower(const char* s) {
    size_t len = strlen(s);
    char* result = malloc(len + 1);
    for (size_t i = 0; i < len; i++) {
        result[i] = (char)tolower((unsigned char)s[i]);
    }
    result[len] = '\0';
    Value v = value_create_string(result);
    free(result);
    return v;
}

int builtin_str_contains(const char* s, const char* sub) {
    return strstr(s, sub) != NULL;
}

Value builtin_str_trim(const char* s) {
    const char* start = s;
    while (*start && (unsigned char)*start <= ' ') {
        start++;
    }
    if (*start == '\0') {
        return value_create_string("");
    }
    const char* end = start + strlen(start) - 1;
    while (end > start && (unsigned char)*end <= ' ') {
        end--;
    }
    size_t len = (size_t)(end - start + 1);
    char* result = malloc(len + 1);
    memcpy(result, start, len);
    result[len] = '\0';
    Value v = value_create_string(result);
    free(result);
    return v;
}

Value builtin_str_split(const char* s, const char* delim) {
    size_t capacity = 8;
    size_t count = 0;
    Value** elements = malloc(sizeof(Value*) * capacity);

    const char* remaining = s;
    size_t delim_len = strlen(delim);

    while (*remaining) {
        const char* found = strstr(remaining, delim);
        size_t part_len;
        if (found) {
            part_len = (size_t)(found - remaining);
        } else {
            part_len = strlen(remaining);
        }

        if (count >= capacity) {
            capacity *= 2;
            elements = realloc(elements, sizeof(Value*) * capacity);
        }

        char* part = malloc(part_len + 1);
        memcpy(part, remaining, part_len);
        part[part_len] = '\0';
        elements[count] = malloc(sizeof(Value));
        *elements[count] = value_create_string(part);
        free(part);
        count++;

        if (!found) break;
        remaining = found + delim_len;
    }

    if (count == 0) {
        free(elements);
        return value_create_array(0);
    }

    Value result = value_create_array_val(elements, count);
    return result;
}

Value builtin_str_join(Value arr, const char* delim) {
    if (arr.type != VALUE_ARRAY || arr.array_length == 0) {
        return value_create_string("");
    }

    size_t total_len = 0;
    size_t delim_len = strlen(delim);

    for (size_t i = 0; i < arr.array_length; i++) {
        if (arr.array_elements[i]->type == VALUE_STRING) {
            total_len += strlen(arr.array_elements[i]->value.string_value);
        }
        if (i < arr.array_length - 1) {
            total_len += delim_len;
        }
    }

    char* result = malloc(total_len + 1);
    result[0] = '\0';
    char* pos = result;

    for (size_t i = 0; i < arr.array_length; i++) {
        if (arr.array_elements[i]->type == VALUE_STRING) {
            size_t part_len = strlen(arr.array_elements[i]->value.string_value);
            memcpy(pos, arr.array_elements[i]->value.string_value, part_len);
            pos += part_len;
        }
        if (i < arr.array_length - 1) {
            memcpy(pos, delim, delim_len);
            pos += delim_len;
        }
    }
    *pos = '\0';

    Value v = value_create_string(result);
    free(result);
    return v;
}

int builtin_fs_exists(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

Value builtin_fs_read_file(const char* path) {
    FILE* f = fopen(path, "rb");
    if (!f) {
        return value_create_option(1, NULL);
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size < 0) {
        fclose(f);
        return value_create_string("");
    }
    rewind(f);

    char* buffer = malloc((size_t)file_size + 1);
    if (!buffer) {
        fclose(f);
        return value_create_string("");
    }

    size_t read_size = fread(buffer, 1, (size_t)file_size, f);
    buffer[read_size] = '\0';
    fclose(f);

    Value v = value_create_string(buffer);
    free(buffer);
    return v;
}

int builtin_fs_write_file(const char* path, const char* content) {
    FILE* f = fopen(path, "wb");
    if (!f) return 0;

    size_t len = strlen(content);
    size_t written = fwrite(content, 1, len, f);
    fclose(f);

    return written == len;
}

Value builtin_arr_push(Value arr, Value val) {
    size_t new_len = arr.array_length + 1;
    Value** new_elements = malloc(sizeof(Value*) * new_len);
    for (size_t i = 0; i < arr.array_length; i++) {
        new_elements[i] = malloc(sizeof(Value));
        *new_elements[i] = value_copy(arr.array_elements[i]);
    }
    new_elements[arr.array_length] = malloc(sizeof(Value));
    *new_elements[arr.array_length] = value_copy(&val);

    Value result = value_create_array(0);
    result.array_elements = new_elements;
    result.array_length = new_len;
    return result;
}

Value builtin_arr_pop(Value arr) {
    if (arr.array_length == 0) {
        return value_create_option(1, NULL);
    }
    return value_copy(arr.array_elements[arr.array_length - 1]);
}

Value builtin_time_format(int64_t epoch) {
    time_t t = (time_t)epoch;
    struct tm* tm_info = localtime(&t);
    if (!tm_info) {
        return value_create_string("Invalid time");
    }
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);
    return value_create_string(buf);
}

Value builtin_dispatch(const char* name, Value* args, int arg_count) {
    if (strcmp(name, "__hn_str_to_upper") == 0 && arg_count == 1) {
        if (args[0].type != VALUE_STRING) return value_create_string("");
        return builtin_str_to_upper(args[0].value.string_value);
    }
    if (strcmp(name, "__hn_str_to_lower") == 0 && arg_count == 1) {
        if (args[0].type != VALUE_STRING) return value_create_string("");
        return builtin_str_to_lower(args[0].value.string_value);
    }
    if (strcmp(name, "__hn_str_contains") == 0 && arg_count == 2) {
        int found = 0;
        if (args[0].type == VALUE_STRING && args[1].type == VALUE_STRING) {
            found = builtin_str_contains(args[0].value.string_value, args[1].value.string_value);
        }
        return value_create_bool(found);
    }
    if (strcmp(name, "__hn_str_trim") == 0 && arg_count == 1) {
        if (args[0].type != VALUE_STRING) return value_create_string("");
        return builtin_str_trim(args[0].value.string_value);
    }
    if (strcmp(name, "__hn_str_split") == 0 && arg_count == 2) {
        if (args[0].type == VALUE_STRING && args[1].type == VALUE_STRING) {
            return builtin_str_split(args[0].value.string_value, args[1].value.string_value);
        }
        return value_create_none();
    }
    if (strcmp(name, "__hn_str_join") == 0 && arg_count == 2) {
        if (args[0].type == VALUE_ARRAY && args[1].type == VALUE_STRING) {
            return builtin_str_join(args[0], args[1].value.string_value);
        }
        return value_create_string("");
    }
    if (strcmp(name, "__hn_time_format") == 0 && arg_count == 1) {
        return builtin_time_format(args[0].value.int_value);
    }
    if (strcmp(name, "__hn_fs_exists") == 0 && arg_count == 1) {
        int exists = 0;
        if (args[0].type == VALUE_STRING) {
            exists = builtin_fs_exists(args[0].value.string_value);
        }
        return value_create_bool(exists);
    }
    if (strcmp(name, "__hn_fs_read_file") == 0 && arg_count == 1) {
        if (args[0].type != VALUE_STRING) return value_create_string("");
        return builtin_fs_read_file(args[0].value.string_value);
    }
    if (strcmp(name, "__hn_fs_write_file") == 0 && arg_count == 2) {
        int ok = 0;
        if (args[0].type == VALUE_STRING && args[1].type == VALUE_STRING) {
            ok = builtin_fs_write_file(args[0].value.string_value, args[1].value.string_value);
        }
        return value_create_bool(ok);
    }
    if (strcmp(name, "__hn_arr_push") == 0 && arg_count == 2) {
        if (args[0].type == VALUE_ARRAY) {
            return builtin_arr_push(args[0], args[1]);
        }
        return args[0];
    }
    if (strcmp(name, "__hn_arr_pop") == 0 && arg_count == 1) {
        if (args[0].type != VALUE_ARRAY) return value_create_none();
        return builtin_arr_pop(args[0]);
    }
    return value_create_none();
}
