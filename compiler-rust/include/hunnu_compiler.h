#ifndef HUNNU_RUST_COMPILER_H
#define HUNNU_RUST_COMPILER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Lex source code and return token debug output as a C string */
char* hunnu_rust_lex(const char* source);

/* Parse source code and return AST debug output as a C string */
char* hunnu_rust_parse(const char* source);

/* Compile Hunnu source to a native object file.
 * Returns 0 on success, non-zero on error. */
int hunnu_rust_compile_to_object(const char* source, const char* output);

/* Emit LLVM IR for Hunnu source as a C string.
 * Returns a string (caller must free via hunnu_rust_free_string). */
char* hunnu_rust_emit_llvm(const char* source);

/* Free a string returned by hunnu_rust_lex, hunnu_rust_parse, or hunnu_rust_emit_llvm */
void hunnu_rust_free_string(char* s);

#ifdef __cplusplus
}
#endif

#endif /* HUNNU_RUST_COMPILER_H */
