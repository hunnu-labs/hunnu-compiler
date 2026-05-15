# Support being built both standalone (as compiler-core root) and
# as a subdirectory of hunnu-lang via add_subdirectory(compiler-core/tests)
get_filename_component(HUNNU_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/.. ABSOLUTE)

# Test executable
add_executable(hunnu_tests
    main.c
    test_lexer.c
    test_parser.c
    test_value.c
    test_scope.c
    test_interpreter.c
    ${HUNNU_ROOT}/compiler/lexer/lexer.c
    ${HUNNU_ROOT}/compiler/lexer/token.c
    ${HUNNU_ROOT}/compiler/parser/parser.c
    ${HUNNU_ROOT}/compiler/ast/ast.c
    ${HUNNU_ROOT}/compiler/ast/ast_free.c
    ${HUNNU_ROOT}/compiler/ast/ast_print.c
    ${HUNNU_ROOT}/compiler/interpreter/interpreter.c
    ${HUNNU_ROOT}/compiler/interpreter/builtins.c
    ${HUNNU_ROOT}/compiler/value.c
    ${HUNNU_ROOT}/compiler/scope.c
    ${HUNNU_ROOT}/compiler/i18n/i18n.c
)

target_include_directories(hunnu_tests PRIVATE ${HUNNU_ROOT})
target_compile_options(hunnu_tests PRIVATE -Wall -Wextra -Wpedantic)
target_link_libraries(hunnu_tests dl)

add_test(NAME hunnu_c_unit_tests COMMAND hunnu_tests)
