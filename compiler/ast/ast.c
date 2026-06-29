/**
 * @file ast.c
 * @brief AST node creation and operations
 */

#include "ast.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/** AST node type names for debugging */
static const char* ast_type_names[] = {
    "PROGRAM",
    "VAR_DECL",
    "FN_DECL",
    "BLOCK",
    "IF_STMT",
    "WHILE_STMT",
    "FOR_STMT",
    "BREAK_STMT",
    "CONTINUE_STMT",
    "RETURN_STMT",
    "PRINT_STMT",
    "EXPR_STMT",
    "BINARY_EXPR",
    "UNARY_EXPR",
    "LITERAL",
    "IDENTIFIER",
    "CALL_EXPR",
    "ASSIGN",
    "ARRAY_EXPR",
    "INDEX_EXPR",
    "INDEX_ASSIGN",
    "STRING_CONCAT",
    "MATCH_EXPR",
    "EXTERN_FN",
    "TRY_STMT",
    "TYPE_DECL",
    "FIELD_ACCESS",
    "ADDRESS_OF",
    "DEREFERENCE",
    "STRUCT_INSTANCE",
    "METHOD_CALL",
    "CLASS_DECL",
    "NEW_EXPR",
    "FIELD_ASSIGN",
    "TRAIT_DECL",
    "IMPL_DECL",
    "UNSAFE_BLOCK",
    "ENUM_DECL",
    "ENUM_VARIANT",
    "LAMBDA",
    "RANGE_PATTERN",
    "ARRAY_PATTERN"
};

/**
 * @brief Converts AST node type to string
 * @param type AST node type
 * @return String name
 */
const char* ast_node_type_to_string(ASTNodeType type) {
    if (type >= 0 && type < (int)(sizeof(ast_type_names) / sizeof(ast_type_names[0]))) {
        return ast_type_names[type];
    }
    return "UNKNOWN";
}

/**
 * @brief Creates a program node
 * @param statements Array of statements
 * @param count Number of statements
 * @return New AST node
 */
ASTNode* ast_program_create(ASTNode** statements, size_t count) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_PROGRAM;
    node->line = 1;
    node->column = 1;
    node->data.program.statements = statements;
    node->data.program.count = count;
    return node;
}

/**
 * @brief Creates a variable declaration node
 * @param name Variable name
 * @param initializer Initializer expression
 * @param line Line number
 * @param column Column number
 * @return New AST node
 */
ASTNode* ast_var_decl_create(const char* name, ASTNode* initializer, int32_t line, int32_t column, int is_mut) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_VAR_DECL;
    node->line = line;
    node->column = column;
    node->data.var_decl.name = strdup(name);
    node->data.var_decl.initializer = initializer;
    node->data.var_decl.is_mut = is_mut;
    return node;
}

/**
 * @brief Creates a function declaration node
 * @param name Function name
 * @param params Parameter names
 * @param param_count Number of parameters
 * @param body Function body
 * @param line Line number
 * @param column Column number
 * @return New AST node
 */
ASTNode* ast_fn_decl_create(const char* name, char** params, size_t param_count, ASTNode* body, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FN_DECL;
    node->line = line;
    node->column = column;
    node->data.fn_decl.name = strdup(name);
    node->data.fn_decl.params = params;
    node->data.fn_decl.param_count = param_count;
    node->data.fn_decl.body = body;
    node->data.fn_decl.type_params = NULL;
    node->data.fn_decl.type_param_count = 0;
    return node;
}

ASTNode* ast_block_create(ASTNode** statements, size_t count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->line = line;
    node->column = column;
    node->data.block.statements = statements;
    node->data.block.count = count;
    return node;
}

ASTNode* ast_if_stmt_create(ASTNode* condition, ASTNode* then_branch, ASTNode* else_branch, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_IF_STMT;
    node->line = line;
    node->column = column;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_branch = then_branch;
    node->data.if_stmt.else_branch = else_branch;
    return node;
}

ASTNode* ast_while_stmt_create(ASTNode* condition, ASTNode* body, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_WHILE_STMT;
    node->line = line;
    node->column = column;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode* ast_for_stmt_create(ASTNode* initializer, ASTNode* condition, ASTNode* update, ASTNode* body, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FOR_STMT;
    node->line = line;
    node->column = column;
    node->data.for_stmt.initializer = initializer;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.update = update;
    node->data.for_stmt.body = body;
    return node;
}

ASTNode* ast_break_stmt_create(int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_BREAK_STMT;
    node->line = line;
    node->column = column;
    return node;
}

ASTNode* ast_continue_stmt_create(int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_CONTINUE_STMT;
    node->line = line;
    node->column = column;
    return node;
}

ASTNode* ast_return_stmt_create(ASTNode* value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_RETURN_STMT;
    node->line = line;
    node->column = column;
    node->data.return_stmt.value = value;
    return node;
}

ASTNode* ast_print_stmt_create(ASTNode* argument, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_PRINT_STMT;
    node->line = line;
    node->column = column;
    node->data.print_stmt.argument = argument;
    return node;
}

ASTNode* ast_expr_stmt_create(ASTNode* expression, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_EXPR_STMT;
    node->line = line;
    node->column = column;
    node->data.expr_stmt.expression = expression;
    return node;
}

ASTNode* ast_binary_expr_create(TokenType operator, ASTNode* left, ASTNode* right, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_BINARY_EXPR;
    node->line = line;
    node->column = column;
    node->data.binary_expr.operator = operator;
    node->data.binary_expr.left = left;
    node->data.binary_expr.right = right;
    return node;
}

ASTNode* ast_unary_expr_create(TokenType operator, ASTNode* operand, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_UNARY_EXPR;
    node->line = line;
    node->column = column;
    node->data.unary_expr.operator = operator;
    node->data.unary_expr.operand = operand;
    return node;
}

ASTNode* ast_literal_create_int(int64_t value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LITERAL;
    node->line = line;
    node->column = column;
    node->data.literal.literal_type = TOKEN_INT_LITERAL;
    node->data.literal.value.int_value = value;
    return node;
}

ASTNode* ast_literal_create_float(double value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LITERAL;
    node->line = line;
    node->column = column;
    node->data.literal.literal_type = TOKEN_FLOAT_LITERAL;
    node->data.literal.value.float_value = value;
    return node;
}

ASTNode* ast_literal_create_string(const char* value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LITERAL;
    node->line = line;
    node->column = column;
    node->data.literal.literal_type = TOKEN_STRING_LITERAL;
    node->data.literal.value.string_value = strdup(value);
    return node;
}

ASTNode* ast_literal_create_bool(int value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LITERAL;
    node->line = line;
    node->column = column;
    node->data.literal.literal_type = TOKEN_BOOL_LITERAL;
    node->data.literal.value.bool_value = value;
    return node;
}

ASTNode* ast_identifier_create(const char* name, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->line = line;
    node->column = column;
    node->data.identifier.name = strdup(name);
    return node;
}

ASTNode* ast_call_expr_create(const char* name, ASTNode** args, size_t arg_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_CALL_EXPR;
    node->line = line;
    node->column = column;
    node->data.call_expr.name = strdup(name);
    node->data.call_expr.args = args;
    node->data.call_expr.arg_count = arg_count;
    return node;
}

ASTNode* ast_assign_create(const char* name, ASTNode* value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->line = line;
    node->column = column;
    node->data.assign.name = strdup(name);
    node->data.assign.value = value;
    return node;
}

ASTNode* ast_array_expr_create(ASTNode** elements, size_t count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ARRAY_EXPR;
    node->line = line;
    node->column = column;
    node->data.array_expr.elements = elements;
    node->data.array_expr.count = count;
    return node;
}

ASTNode* ast_index_expr_create(ASTNode* array, ASTNode* index, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_INDEX_EXPR;
    node->line = line;
    node->column = column;
    node->data.index_expr.array = array;
    node->data.index_expr.index = index;
    return node;
}

ASTNode* ast_index_assign_create(ASTNode* array, ASTNode* index, ASTNode* value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_INDEX_ASSIGN;
    node->line = line;
    node->column = column;
    node->data.index_assign.array = array;
    node->data.index_assign.index = index;
    node->data.index_assign.value = value;
    return node;
}

ASTNode* ast_string_concat_create(ASTNode* left, ASTNode* right, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_STRING_CONCAT;
    node->line = line;
    node->column = column;
    node->data.string_concat.left = left;
    node->data.string_concat.right = right;
    return node;
}

ASTNode* ast_match_expr_create(ASTNode* value, ASTNode** patterns, ASTNode** bodies,
                               size_t case_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_MATCH_EXPR;
    node->line = line;
    node->column = column;
    node->data.match_expr.value = value;
    node->data.match_expr.patterns = patterns;
    node->data.match_expr.bodies = bodies;
    node->data.match_expr.case_count = case_count;
    return node;
}

ASTNode* ast_extern_fn_create(const char* name, const char* lib_name, const char* symbol_name,
                               char** param_names, size_t param_count, int returns_int,
                               int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_EXTERN_FN;
    node->line = line;
    node->column = column;
    node->data.extern_fn.name = strdup(name);
    node->data.extern_fn.lib_name = lib_name ? strdup(lib_name) : NULL;
    node->data.extern_fn.symbol_name = strdup(symbol_name);
    node->data.extern_fn.param_names = param_names;
    node->data.extern_fn.param_count = param_count;
    node->data.extern_fn.returns_int = returns_int;
    return node;
}

ASTNode* ast_try_stmt_create(ASTNode* try_block, const char* catch_var,
                              ASTNode* catch_block, ASTNode* finally_block,
                              int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_TRY_STMT;
    node->line = line;
    node->column = column;
    node->data.try_stmt.try_block = try_block;
    node->data.try_stmt.catch_var = catch_var ? strdup(catch_var) : NULL;
    node->data.try_stmt.catch_block = catch_block;
    node->data.try_stmt.finally_block = finally_block;
    return node;
}

ASTNode* ast_type_decl_create(const char* name, char** fields, int* is_pub, size_t field_count,
                                int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_TYPE_DECL;
    node->line = line;
    node->column = column;
    node->data.type_decl.name = strdup(name);
    node->data.type_decl.fields = fields;
    node->data.type_decl.is_pub = is_pub;
    node->data.type_decl.field_count = field_count;
    node->data.type_decl.type_params = NULL;
    node->data.type_decl.type_param_count = 0;
    return node;
}

ASTNode* ast_field_access_create(ASTNode* object, const char* field,
                                  int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FIELD_ACCESS;
    node->line = line;
    node->column = column;
    node->data.field_access.object = object;
    node->data.field_access.field = strdup(field);
    return node;
}

ASTNode* ast_address_of_create(ASTNode* operand, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ADDRESS_OF;
    node->line = line;
    node->column = column;
    node->data.address_of.operand = operand;
    return node;
}

ASTNode* ast_dereference_create(ASTNode* operand, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_DEREFERENCE;
    node->line = line;
    node->column = column;
    node->data.dereference.operand = operand;
    return node;
}

ASTNode* ast_struct_instance_create(const char* type_name, char** field_names,
                                     ASTNode** field_values, size_t field_count,
                                     int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_STRUCT_INSTANCE;
    node->line = line;
    node->column = column;
    node->data.struct_instance.type_name = strdup(type_name);
    node->data.struct_instance.field_names = field_names;
    node->data.struct_instance.field_values = field_values;
    node->data.struct_instance.field_count = field_count;
    return node;
}

ASTNode* ast_method_call_create(ASTNode* object, const char* method,
                                 ASTNode** args, size_t arg_count,
                                 int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_METHOD_CALL;
    node->line = line;
    node->column = column;
    node->data.method_call.object = object;
    node->data.method_call.method = strdup(method);
    node->data.method_call.args = args;
    node->data.method_call.arg_count = arg_count;
    return node;
}

ASTNode* ast_class_decl_create(const char* name, const char* parent_name, char** fields, int* is_pub,
                                 size_t field_count, ASTNode* constructor,
                                 ASTNode** methods, size_t method_count,
                                 int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_CLASS_DECL;
    node->line = line;
    node->column = column;
    node->data.class_decl.name = strdup(name);
    node->data.class_decl.parent_name = parent_name ? strdup(parent_name) : NULL;
    node->data.class_decl.fields = fields;
    node->data.class_decl.is_pub = is_pub;
    node->data.class_decl.field_count = field_count;
    node->data.class_decl.constructor = constructor;
    node->data.class_decl.methods = methods;
    node->data.class_decl.method_count = method_count;
    node->data.class_decl.type_params = NULL;
    node->data.class_decl.type_param_count = 0;
    return node;
}

ASTNode* ast_new_expr_create(const char* class_name, ASTNode** args,
                              size_t arg_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_NEW_EXPR;
    node->line = line;
    node->column = column;
    node->data.new_expr.class_name = strdup(class_name);
    node->data.new_expr.args = args;
    node->data.new_expr.arg_count = arg_count;
    return node;
}

ASTNode* ast_field_assign_create(ASTNode* object, const char* field,
                                   ASTNode* value, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_FIELD_ASSIGN;
    node->line = line;
    node->column = column;
    node->data.field_assign.object = object;
    node->data.field_assign.field = strdup(field);
    node->data.field_assign.value = value;
    return node;
}

ASTNode* ast_trait_decl_create(const char* name, char** method_names, size_t* method_param_counts, size_t method_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_TRAIT_DECL;
    node->line = line;
    node->column = column;
    node->data.trait_decl.name = strdup(name);
    node->data.trait_decl.method_names = method_names;
    node->data.trait_decl.method_param_counts = method_param_counts;
    node->data.trait_decl.method_count = method_count;
    return node;
}

ASTNode* ast_impl_decl_create(const char* trait_name, const char* type_name, ASTNode** methods, size_t method_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_IMPL_DECL;
    node->line = line;
    node->column = column;
    node->data.impl_decl.trait_name = strdup(trait_name);
    node->data.impl_decl.type_name = strdup(type_name);
    node->data.impl_decl.methods = methods;
    node->data.impl_decl.method_count = method_count;
    return node;
}

ASTNode* ast_unsafe_block_create(ASTNode* body, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_UNSAFE_BLOCK;
    node->line = line;
    node->column = column;
    node->data.unsafe_block.body = body;
    return node;
}

ASTNode* ast_enum_decl_create(const char* name, char** variant_names, size_t* variant_field_counts, char*** variant_field_names, size_t variant_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ENUM_DECL;
    node->line = line;
    node->column = column;
    node->data.enum_decl.name = strdup(name);
    node->data.enum_decl.variant_names = variant_names;
    node->data.enum_decl.variant_field_counts = variant_field_counts;
    node->data.enum_decl.variant_field_names = variant_field_names;
    node->data.enum_decl.variant_count = variant_count;
    node->data.enum_decl.type_params = NULL;
    node->data.enum_decl.type_param_count = 0;
    return node;
}

ASTNode* ast_enum_variant_create(const char* enum_name, const char* variant_name, ASTNode** args, size_t arg_count, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ENUM_VARIANT;
    node->line = line;
    node->column = column;
    node->data.enum_variant.enum_name = strdup(enum_name);
    node->data.enum_variant.variant_name = strdup(variant_name);
    node->data.enum_variant.args = args;
    node->data.enum_variant.arg_count = arg_count;
    return node;
}

ASTNode* ast_lambda_create(char** params, size_t param_count, ASTNode* body, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_LAMBDA;
    node->line = line;
    node->column = column;
    node->data.lambda.params = params;
    node->data.lambda.param_count = param_count;
    node->data.lambda.body = body;
    return node;
}

ASTNode* ast_range_pattern_create(ASTNode* start, ASTNode* end, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_RANGE_PATTERN;
    node->line = line;
    node->column = column;
    node->data.range_pattern.start = start;
    node->data.range_pattern.end = end;
    return node;
}

ASTNode* ast_array_pattern_create(char** names, size_t count, char* rest_name, int32_t line, int32_t column) {
    ASTNode* node = (ASTNode*)malloc(sizeof(ASTNode));
    node->type = AST_ARRAY_PATTERN;
    node->line = line;
    node->column = column;
    node->data.array_pattern.names = names;
    node->data.array_pattern.count = count;
    node->data.array_pattern.rest_name = rest_name ? strdup(rest_name) : NULL;
    return node;
}

static void ast_free_node(ASTNode* node) {
    if (!node) return;

    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.program.count; i++) {
                ast_free_node(node->data.program.statements[i]);
            }
            free(node->data.program.statements);
            break;

        case AST_VAR_DECL:
            free(node->data.var_decl.name);
            ast_free_node(node->data.var_decl.initializer);
            break;

        case AST_FN_DECL:
            free(node->data.fn_decl.name);
            for (size_t i = 0; i < node->data.fn_decl.param_count; i++) {
                free(node->data.fn_decl.params[i]);
            }
            free(node->data.fn_decl.params);
            for (size_t i = 0; i < node->data.fn_decl.type_param_count; i++) {
                free(node->data.fn_decl.type_params[i]);
            }
            free(node->data.fn_decl.type_params);
            ast_free_node(node->data.fn_decl.body);
            break;

        case AST_BLOCK:
            for (size_t i = 0; i < node->data.block.count; i++) {
                ast_free_node(node->data.block.statements[i]);
            }
            free(node->data.block.statements);
            break;

        case AST_IF_STMT:
            ast_free_node(node->data.if_stmt.condition);
            ast_free_node(node->data.if_stmt.then_branch);
            ast_free_node(node->data.if_stmt.else_branch);
            break;

        case AST_PRINT_STMT:
            ast_free_node(node->data.print_stmt.argument);
            break;

        case AST_EXPR_STMT:
            ast_free_node(node->data.expr_stmt.expression);
            break;

        case AST_BINARY_EXPR:
            ast_free_node(node->data.binary_expr.left);
            ast_free_node(node->data.binary_expr.right);
            break;

        case AST_UNARY_EXPR:
            ast_free_node(node->data.unary_expr.operand);
            break;

        case AST_LITERAL:
            if (node->data.literal.literal_type == TOKEN_STRING_LITERAL ||
                node->data.literal.literal_type == TOKEN_SYMBOL) {
                free(node->data.literal.value.string_value);
            }
            break;

        case AST_IDENTIFIER:
            free(node->data.identifier.name);
            break;

        case AST_CALL_EXPR:
            free(node->data.call_expr.name);
            for (size_t i = 0; i < node->data.call_expr.arg_count; i++) {
                ast_free_node(node->data.call_expr.args[i]);
            }
            free(node->data.call_expr.args);
            break;

        case AST_ASSIGN:
            free(node->data.assign.name);
            ast_free_node(node->data.assign.value);
            break;

        case AST_ARRAY_EXPR:
            for (size_t i = 0; i < node->data.array_expr.count; i++) {
                ast_free_node(node->data.array_expr.elements[i]);
            }
            free(node->data.array_expr.elements);
            break;

        case AST_INDEX_EXPR:
            ast_free_node(node->data.index_expr.array);
            ast_free_node(node->data.index_expr.index);
            break;

        case AST_INDEX_ASSIGN:
            ast_free_node(node->data.index_assign.array);
            ast_free_node(node->data.index_assign.index);
            ast_free_node(node->data.index_assign.value);
            break;

        case AST_STRING_CONCAT:
            ast_free_node(node->data.string_concat.left);
            ast_free_node(node->data.string_concat.right);
            break;

        case AST_MATCH_EXPR:
            ast_free_node(node->data.match_expr.value);
            for (size_t i = 0; i < node->data.match_expr.case_count; i++) {
                ast_free_node(node->data.match_expr.patterns[i]);
                ast_free_node(node->data.match_expr.bodies[i]);
            }
            free(node->data.match_expr.patterns);
            free(node->data.match_expr.bodies);
            break;

        case AST_EXTERN_FN:
            free(node->data.extern_fn.name);
            if (node->data.extern_fn.lib_name) free(node->data.extern_fn.lib_name);
            free(node->data.extern_fn.symbol_name);
            for (size_t i = 0; i < node->data.extern_fn.param_count; i++) {
                free(node->data.extern_fn.param_names[i]);
            }
            if (node->data.extern_fn.param_count > 0) {
                free(node->data.extern_fn.param_names);
            }
            break;

        case AST_TYPE_DECL:
            free(node->data.type_decl.name);
            for (size_t i = 0; i < node->data.type_decl.field_count; i++) {
                free(node->data.type_decl.fields[i]);
            }
            if (node->data.type_decl.field_count > 0) {
                free(node->data.type_decl.fields);
                free(node->data.type_decl.is_pub);
            }
            for (size_t i = 0; i < node->data.type_decl.type_param_count; i++) {
                free(node->data.type_decl.type_params[i]);
            }
            free(node->data.type_decl.type_params);
            break;

        case AST_FIELD_ACCESS:
            ast_free_node(node->data.field_access.object);
            free(node->data.field_access.field);
            break;

        case AST_ADDRESS_OF:
            ast_free_node(node->data.address_of.operand);
            break;

        case AST_DEREFERENCE:
            ast_free_node(node->data.dereference.operand);
            break;

        case AST_STRUCT_INSTANCE:
            free(node->data.struct_instance.type_name);
            for (size_t i = 0; i < node->data.struct_instance.field_count; i++) {
                free(node->data.struct_instance.field_names[i]);
                ast_free_node(node->data.struct_instance.field_values[i]);
            }
            if (node->data.struct_instance.field_count > 0) {
                free(node->data.struct_instance.field_names);
                free(node->data.struct_instance.field_values);
            }
            break;

        case AST_METHOD_CALL:
            ast_free_node(node->data.method_call.object);
            free(node->data.method_call.method);
            for (size_t i = 0; i < node->data.method_call.arg_count; i++) {
                ast_free_node(node->data.method_call.args[i]);
            }
            if (node->data.method_call.arg_count > 0) {
                free(node->data.method_call.args);
            }
            break;

        case AST_CLASS_DECL:
            free(node->data.class_decl.name);
            free(node->data.class_decl.parent_name);
            for (size_t i = 0; i < node->data.class_decl.field_count; i++) {
                free(node->data.class_decl.fields[i]);
            }
            if (node->data.class_decl.field_count > 0) {
                free(node->data.class_decl.fields);
                free(node->data.class_decl.is_pub);
            }
            for (size_t i = 0; i < node->data.class_decl.type_param_count; i++) {
                free(node->data.class_decl.type_params[i]);
            }
            free(node->data.class_decl.type_params);
            ast_free_node(node->data.class_decl.constructor);
            for (size_t i = 0; i < node->data.class_decl.method_count; i++) {
                ast_free_node(node->data.class_decl.methods[i]);
            }
            free(node->data.class_decl.methods);
            break;

        case AST_NEW_EXPR:
            free(node->data.new_expr.class_name);
            for (size_t i = 0; i < node->data.new_expr.arg_count; i++) {
                ast_free_node(node->data.new_expr.args[i]);
            }
            if (node->data.new_expr.arg_count > 0) {
                free(node->data.new_expr.args);
            }
            break;

        case AST_FIELD_ASSIGN:
            ast_free_node(node->data.field_assign.object);
            free(node->data.field_assign.field);
            ast_free_node(node->data.field_assign.value);
            break;

        case AST_TRAIT_DECL:
            free(node->data.trait_decl.name);
            for (size_t i = 0; i < node->data.trait_decl.method_count; i++) {
                free(node->data.trait_decl.method_names[i]);
            }
            free(node->data.trait_decl.method_names);
            free(node->data.trait_decl.method_param_counts);
            break;

        case AST_IMPL_DECL:
            free(node->data.impl_decl.trait_name);
            free(node->data.impl_decl.type_name);
            for (size_t i = 0; i < node->data.impl_decl.method_count; i++) {
                ast_free_node(node->data.impl_decl.methods[i]);
            }
            free(node->data.impl_decl.methods);
            break;

        case AST_UNSAFE_BLOCK:
            ast_free_node(node->data.unsafe_block.body);
            break;

        case AST_ENUM_DECL:
            free(node->data.enum_decl.name);
            for (size_t i = 0; i < node->data.enum_decl.type_param_count; i++) {
                free(node->data.enum_decl.type_params[i]);
            }
            free(node->data.enum_decl.type_params);
            for (size_t i = 0; i < node->data.enum_decl.variant_count; i++) {
                free(node->data.enum_decl.variant_names[i]);
                if (node->data.enum_decl.variant_field_names && node->data.enum_decl.variant_field_names[i]) {
                    for (size_t j = 0; j < node->data.enum_decl.variant_field_counts[i]; j++) {
                        free(node->data.enum_decl.variant_field_names[i][j]);
                    }
                    free(node->data.enum_decl.variant_field_names[i]);
                }
            }
            free(node->data.enum_decl.variant_names);
            free(node->data.enum_decl.variant_field_counts);
            free(node->data.enum_decl.variant_field_names);
            break;

        case AST_ENUM_VARIANT:
            free(node->data.enum_variant.enum_name);
            free(node->data.enum_variant.variant_name);
            for (size_t i = 0; i < node->data.enum_variant.arg_count; i++) {
                ast_free_node(node->data.enum_variant.args[i]);
            }
            free(node->data.enum_variant.args);
            break;

        case AST_LAMBDA:
            for (size_t i = 0; i < node->data.lambda.param_count; i++) {
                free(node->data.lambda.params[i]);
            }
            free(node->data.lambda.params);
            ast_free_node(node->data.lambda.body);
            break;

        case AST_RANGE_PATTERN:
            ast_free_node(node->data.range_pattern.start);
            ast_free_node(node->data.range_pattern.end);
            break;

        case AST_ARRAY_PATTERN:
            for (size_t i = 0; i < node->data.array_pattern.count; i++) {
                free(node->data.array_pattern.names[i]);
            }
            free(node->data.array_pattern.names);
            free(node->data.array_pattern.rest_name);
            break;

        case AST_WHILE_STMT:
        case AST_FOR_STMT:
        case AST_BREAK_STMT:
        case AST_CONTINUE_STMT:
        case AST_RETURN_STMT:
        case AST_TRY_STMT:
            break;

        default:
            break;
    }

    free(node);
}

void ast_free(ASTNode* node) {
    ast_free_node(node);
}

static void indent_print(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

static void ast_print_node(ASTNode* node, int indent) {
    if (!node) {
        indent_print(indent);
        printf("NULL\n");
        return;
    }

    indent_print(indent);
    printf("%s", ast_node_type_to_string(node->type));

    switch (node->type) {
        case AST_VAR_DECL:
            printf(" (%s)", node->data.var_decl.name);
            break;

        case AST_FN_DECL:
            printf(" (%s", node->data.fn_decl.name);
            if (node->data.fn_decl.type_param_count > 0) {
                printf("<");
                for (size_t i = 0; i < node->data.fn_decl.type_param_count; i++) {
                    if (i > 0) printf(", ");
                    printf("%s", node->data.fn_decl.type_params[i]);
                }
                printf(">");
            }
            printf(")");
            break;

        case AST_IDENTIFIER:
            printf(" (%s)", node->data.identifier.name);
            break;

        case AST_LITERAL:
            if (node->data.literal.literal_type == TOKEN_INT_LITERAL) {
                printf(" (%ld)", (long)node->data.literal.value.int_value);
            } else if (node->data.literal.literal_type == TOKEN_STRING_LITERAL) {
                printf(" (\"%s\")", node->data.literal.value.string_value);
            } else if (node->data.literal.literal_type == TOKEN_BOOL_LITERAL) {
                printf(" (%s)", node->data.literal.value.bool_value ? "true" : "false");
            }
            break;

        case AST_BINARY_EXPR:
            printf(" (op)");
            break;

        case AST_CALL_EXPR:
            printf(" (%s)", node->data.call_expr.name);
            break;

        case AST_ASSIGN:
            printf(" (%s)", node->data.assign.name);
            break;

        case AST_EXTERN_FN:
            printf(" (%s -> %s", node->data.extern_fn.name, node->data.extern_fn.symbol_name);
            if (node->data.extern_fn.lib_name) {
                printf(" from %s", node->data.extern_fn.lib_name);
            }
            printf(")");
            break;

        case AST_MATCH_EXPR:
            printf(" (value)");
            break;

        case AST_TRAIT_DECL:
        case AST_IMPL_DECL:
        case AST_UNSAFE_BLOCK:
        case AST_ENUM_DECL:
        case AST_ENUM_VARIANT:
        case AST_LAMBDA:
        case AST_RANGE_PATTERN:
        case AST_ARRAY_PATTERN:
            break;

        default:
            break;
    }

    printf("\n");

    switch (node->type) {
        case AST_PROGRAM:
            for (size_t i = 0; i < node->data.program.count; i++) {
                ast_print_node(node->data.program.statements[i], indent + 1);
            }
            break;

        case AST_VAR_DECL:
            ast_print_node(node->data.var_decl.initializer, indent + 1);
            break;

        case AST_FN_DECL:
            ast_print_node(node->data.fn_decl.body, indent + 1);
            break;

        case AST_BLOCK:
            for (size_t i = 0; i < node->data.block.count; i++) {
                ast_print_node(node->data.block.statements[i], indent + 1);
            }
            break;

        case AST_IF_STMT:
            indent_print(indent + 1);
            printf("condition:\n");
            ast_print_node(node->data.if_stmt.condition, indent + 2);
            indent_print(indent + 1);
            printf("then:\n");
            ast_print_node(node->data.if_stmt.then_branch, indent + 2);
            if (node->data.if_stmt.else_branch) {
                indent_print(indent + 1);
                printf("else:\n");
                ast_print_node(node->data.if_stmt.else_branch, indent + 2);
            }
            break;

        case AST_PRINT_STMT:
            ast_print_node(node->data.print_stmt.argument, indent + 1);
            break;

        case AST_EXPR_STMT:
            ast_print_node(node->data.expr_stmt.expression, indent + 1);
            break;

        case AST_BINARY_EXPR:
            ast_print_node(node->data.binary_expr.left, indent + 1);
            ast_print_node(node->data.binary_expr.right, indent + 1);
            break;

        case AST_UNARY_EXPR:
            ast_print_node(node->data.unary_expr.operand, indent + 1);
            break;

        case AST_CALL_EXPR:
            for (size_t i = 0; i < node->data.call_expr.arg_count; i++) {
                ast_print_node(node->data.call_expr.args[i], indent + 1);
            }
            break;

        case AST_ASSIGN:
            ast_print_node(node->data.assign.value, indent + 1);
            break;

        case AST_ARRAY_EXPR:
            for (size_t i = 0; i < node->data.array_expr.count; i++) {
                ast_print_node(node->data.array_expr.elements[i], indent + 1);
            }
            break;

        case AST_INDEX_EXPR:
            ast_print_node(node->data.index_expr.array, indent + 1);
            ast_print_node(node->data.index_expr.index, indent + 1);
            break;

        case AST_INDEX_ASSIGN:
            ast_print_node(node->data.index_assign.array, indent + 1);
            ast_print_node(node->data.index_assign.index, indent + 1);
            ast_print_node(node->data.index_assign.value, indent + 1);
            break;

        case AST_STRING_CONCAT:
            ast_print_node(node->data.string_concat.left, indent + 1);
            ast_print_node(node->data.string_concat.right, indent + 1);
            break;

        case AST_MATCH_EXPR:
            ast_print_node(node->data.match_expr.value, indent + 1);
            printf(" match cases:\n");
            for (size_t i = 0; i < node->data.match_expr.case_count; i++) {
                indent_print(indent + 1);
                printf("pattern: ");
                ast_print_node(node->data.match_expr.patterns[i], 0);
                printf("\n");
                indent_print(indent + 1);
                printf("body: ");
                ast_print_node(node->data.match_expr.bodies[i], 0);
                printf("\n");
            }
            break;

        case AST_EXTERN_FN:
            break;

        case AST_TRY_STMT:
            indent_print(indent);
            printf("TRY_STMT:\n");
            indent_print(indent + 1);
            printf("try_block:\n");
            ast_print_node(node->data.try_stmt.try_block, indent + 2);
            printf("\n");
            if (node->data.try_stmt.catch_var) {
                indent_print(indent + 1);
                printf("catch_var: %s\n", node->data.try_stmt.catch_var);
            }
            if (node->data.try_stmt.catch_block) {
                indent_print(indent + 1);
                printf("catch_block:\n");
                ast_print_node(node->data.try_stmt.catch_block, indent + 2);
                printf("\n");
            }
            if (node->data.try_stmt.finally_block) {
                indent_print(indent + 1);
                printf("finally_block:\n");
                ast_print_node(node->data.try_stmt.finally_block, indent + 2);
                printf("\n");
            }
            break;

        case AST_TYPE_DECL:
            indent_print(indent);
            printf("TYPE_DECL (%s) with %zu fields\n", node->data.type_decl.name, node->data.type_decl.field_count);
            for (size_t i = 0; i < node->data.type_decl.field_count; i++) {
                indent_print(indent + 1);
                printf("field: %s %s\n",
                       node->data.type_decl.is_pub && node->data.type_decl.is_pub[i] ? "pub" : "priv",
                       node->data.type_decl.fields[i]);
            }
            break;

        case AST_FIELD_ACCESS:
            indent_print(indent);
            printf("FIELD_ACCESS (.%s)\n", node->data.field_access.field);
            ast_print_node(node->data.field_access.object, indent + 1);
            break;

        case AST_ADDRESS_OF:
            indent_print(indent);
            printf("ADDRESS_OF (&)\n");
            ast_print_node(node->data.address_of.operand, indent + 1);
            break;

        case AST_DEREFERENCE:
            indent_print(indent);
            printf("DEREFERENCE (*)\n");
            ast_print_node(node->data.dereference.operand, indent + 1);
            break;

        case AST_CLASS_DECL:
            indent_print(indent);
            if (node->data.class_decl.parent_name) {
                printf("CLASS_DECL (%s : %s) with %zu fields, %zu methods\n",
                       node->data.class_decl.name,
                       node->data.class_decl.parent_name,
                       node->data.class_decl.field_count,
                       node->data.class_decl.method_count);
            } else {
                printf("CLASS_DECL (%s) with %zu fields, %zu methods\n",
                       node->data.class_decl.name,
                       node->data.class_decl.field_count,
                       node->data.class_decl.method_count);
            }
            for (size_t i = 0; i < node->data.class_decl.field_count; i++) {
                indent_print(indent + 1);
                printf("field: %s %s\n",
                       node->data.class_decl.is_pub && node->data.class_decl.is_pub[i] ? "pub" : "priv",
                       node->data.class_decl.fields[i]);
            }
            if (node->data.class_decl.constructor) {
                indent_print(indent + 1);
                printf("constructor (new):\n");
                ast_print_node(node->data.class_decl.constructor, indent + 2);
            }
            for (size_t i = 0; i < node->data.class_decl.method_count; i++) {
                indent_print(indent + 1);
                printf("method %zu:\n", i);
                ast_print_node(node->data.class_decl.methods[i], indent + 2);
            }
            break;

        case AST_NEW_EXPR:
            indent_print(indent);
            printf("NEW_EXPR (%s, %zu args)\n", node->data.new_expr.class_name, node->data.new_expr.arg_count);
            for (size_t i = 0; i < node->data.new_expr.arg_count; i++) {
                ast_print_node(node->data.new_expr.args[i], indent + 1);
            }
            break;

        case AST_FIELD_ASSIGN:
            indent_print(indent);
            printf("FIELD_ASSIGN (.%s)\n", node->data.field_assign.field);
            indent_print(indent + 1);
            printf("object:\n");
            ast_print_node(node->data.field_assign.object, indent + 2);
            indent_print(indent + 1);
            printf("value:\n");
            ast_print_node(node->data.field_assign.value, indent + 2);
            break;

        case AST_TRAIT_DECL:
            indent_print(indent);
            printf("TRAIT_DECL (%s) with %zu methods\n",
                   node->data.trait_decl.name,
                   node->data.trait_decl.method_count);
            for (size_t i = 0; i < node->data.trait_decl.method_count; i++) {
                indent_print(indent + 1);
                printf("method: %s (params: %zu)\n",
                       node->data.trait_decl.method_names[i],
                       node->data.trait_decl.method_param_counts[i]);
            }
            break;

        case AST_IMPL_DECL:
            indent_print(indent);
            printf("IMPL_DECL (impl %s for %s with %zu methods)\n",
                   node->data.impl_decl.trait_name,
                   node->data.impl_decl.type_name,
                   node->data.impl_decl.method_count);
            for (size_t i = 0; i < node->data.impl_decl.method_count; i++) {
                indent_print(indent + 1);
                printf("method %zu:\n", i);
                ast_print_node(node->data.impl_decl.methods[i], indent + 2);
            }
            break;

        case AST_UNSAFE_BLOCK:
            indent_print(indent);
            printf("UNSAFE_BLOCK\n");
            ast_print_node(node->data.unsafe_block.body, indent + 1);
            break;

        case AST_ENUM_DECL:
            indent_print(indent);
            printf("ENUM_DECL (%s) with %zu variants\n",
                   node->data.enum_decl.name,
                   node->data.enum_decl.variant_count);
            for (size_t i = 0; i < node->data.enum_decl.variant_count; i++) {
                indent_print(indent + 1);
                printf("variant: %s", node->data.enum_decl.variant_names[i]);
                if (node->data.enum_decl.variant_field_counts &&
                    node->data.enum_decl.variant_field_counts[i] > 0) {
                    printf(" (fields: ");
                    for (size_t j = 0; j < node->data.enum_decl.variant_field_counts[i]; j++) {
                        if (j > 0) printf(", ");
                        printf("%s", node->data.enum_decl.variant_field_names[i][j]);
                    }
                    printf(")");
                }
                printf("\n");
            }
            break;

        case AST_ARRAY_PATTERN:
            indent_print(indent);
            printf("ARRAY_PATTERN (%zu elements, rest: %s)\n",
                   node->data.array_pattern.count,
                   node->data.array_pattern.rest_name ? node->data.array_pattern.rest_name : "none");
            break;

        case AST_RANGE_PATTERN:
            indent_print(indent);
            printf("RANGE_PATTERN\n");
            break;

        case AST_ENUM_VARIANT:
            indent_print(indent);
            printf("ENUM_VARIANT (%s::%s, %zu args)\n",
                   node->data.enum_variant.enum_name,
                   node->data.enum_variant.variant_name,
                   node->data.enum_variant.arg_count);
            for (size_t i = 0; i < node->data.enum_variant.arg_count; i++) {
                ast_print_node(node->data.enum_variant.args[i], indent + 1);
            }
            break;

        default:
            break;
    }
}

void ast_print(ASTNode* node, int indent) {
    ast_print_node(node, indent);
}

