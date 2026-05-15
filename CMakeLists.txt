cmake_minimum_required(VERSION 3.15)
project(hunnu-compiler VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
option(BUILD_RUST_VM "Build Rust VM component" ON)
option(BUILD_RUST_COMPILER "Build Rust compiler frontend" ON)

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/compiler)
include_directories(${CMAKE_SOURCE_DIR}/vm-rust/include)
include_directories(${CMAKE_SOURCE_DIR}/compiler-rust/include)

set(HUNNU_SOURCES
    cli/main.c
    cli/cli.c
    compiler/lexer/lexer.c
    compiler/lexer/token.c
    compiler/parser/parser.c
    compiler/parser/parse_decl.c
    compiler/parser/parse_stmt.c
    compiler/parser/parse_expr.c
    compiler/ast/ast.c
    compiler/ast/ast_free.c
    compiler/ast/ast_print.c
    compiler/interpreter/interpreter.c
    compiler/interpreter/eval.c
    compiler/interpreter/exec.c
    compiler/interpreter/call.c
    compiler/interpreter/builtins.c
    compiler/value.c
    compiler/scope.c
    compiler/import.c
    compiler/i18n/i18n.c
    compiler/version.c
    compiler/vm/compiler.c
    compiler/vm/vm.c
    compiler/transpile/transpile.c
)

if(BUILD_RUST_VM)
    find_program(CARGO_EXECUTABLE cargo)
    if(CARGO_EXECUTABLE)
        message(STATUS "Found cargo: ${CARGO_EXECUTABLE}")

        set(RUST_VM_LIB ${CMAKE_SOURCE_DIR}/vm-rust/target/release/libhunnu_vm.a)

        add_custom_command(
            OUTPUT ${RUST_VM_LIB}
            COMMAND ${CARGO_EXECUTABLE} build --release
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vm-rust
            COMMENT "Building Rust VM library"
        )

        add_custom_target(hunnu_vm_rust DEPENDS ${RUST_VM_LIB})

        add_executable(hunnu ${HUNNU_SOURCES})

        target_link_libraries(hunnu ${RUST_VM_LIB})
        target_link_libraries(hunnu pthread dl)

        add_dependencies(hunnu hunnu_vm_rust)

        install(PROGRAMS ${CMAKE_SOURCE_DIR}/vm-rust/target/release/hunnu-vm
                DESTINATION bin
                RENAME hunnu-vm
                OPTIONAL)

        message(STATUS "Rust VM will be linked with hunnu binary")
    else()
        message(WARNING "cargo not found - building without Rust VM support")
        add_executable(hunnu ${HUNNU_SOURCES})
    endif()
else()
    add_executable(hunnu ${HUNNU_SOURCES})
endif()

target_compile_options(hunnu PRIVATE -Wall -Wextra -pedantic)

install(TARGETS hunnu DESTINATION bin)

enable_testing()

add_subdirectory(tests)

add_test(NAME hunnu_version COMMAND hunnu -v)

add_test(NAME hunnu_tests
    COMMAND ${CMAKE_SOURCE_DIR}/run_tests.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

if(BUILD_RUST_VM AND CARGO_EXECUTABLE)
    add_test(
        NAME rust_vm_tests
        COMMAND ${CARGO_EXECUTABLE} test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/vm-rust
    )
endif()

if(BUILD_RUST_COMPILER AND CARGO_EXECUTABLE)
    add_test(
        NAME rust_compiler_tests
        COMMAND ${CARGO_EXECUTABLE} test
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/compiler-rust
    )
endif()

message(STATUS "Build configuration:")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
if(BUILD_RUST_VM AND CARGO_EXECUTABLE)
    message(STATUS "  Rust VM: enabled")
else()
    message(STATUS "  Rust VM: disabled")
endif()

if(BUILD_RUST_COMPILER)
    if(CARGO_EXECUTABLE)
        set(RUST_COMPILER_LIB ${CMAKE_SOURCE_DIR}/compiler-rust/target/release/libhunnu_compiler.a)

        add_custom_command(
            OUTPUT ${RUST_COMPILER_LIB}
            COMMAND ${CARGO_EXECUTABLE} build --release
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/compiler-rust
            COMMENT "Building Rust compiler library"
        )

        add_custom_target(hunnu_compiler_rust DEPENDS ${RUST_COMPILER_LIB})

        if(TARGET hunnu)
            target_link_libraries(hunnu ${RUST_COMPILER_LIB})
            add_dependencies(hunnu hunnu_compiler_rust)
        endif()

        install(PROGRAMS ${CMAKE_SOURCE_DIR}/compiler-rust/target/release/hunnu-compiler
                DESTINATION bin
                OPTIONAL)

        if(TARGET hunnu)
            target_compile_definitions(hunnu PRIVATE HUNNU_HAVE_RUST_COMPILER=1)
        endif()
        message(STATUS "  Rust compiler: enabled")
    else()
        message(STATUS "  Rust compiler: disabled (cargo not found)")
    endif()
endif()
