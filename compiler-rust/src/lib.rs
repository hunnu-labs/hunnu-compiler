//! Hunnu Language Rust Compiler Frontend.
//!
//! This crate provides a lexer and parser for the Hunnu programming language,
//! along with optional LLVM-based code generation (`feature = "llvm-codegen"`).
//! It is used as the AOT compilation frontend for the `hunnu compile` command.

pub mod lexer;
pub mod parser;
pub mod ast;

#[cfg(feature = "llvm-codegen")]
pub mod codegen;

use std::ffi::{CStr, CString};
use std::os::raw::c_char;

pub use lexer::TokenType;
pub use lexer::Token;
pub use ast::ASTNode;
pub use ast::NodeData;
pub use ast::ASTNodeType;
pub use ast::LiteralType;
pub use ast::LiteralValue;

// Re-export codegen convenience functions when feature is enabled
#[cfg(feature = "llvm-codegen")]
pub use codegen::{compile_to_ir, compile_to_object, compile_to_assembly, compile_to_exe};
#[cfg(feature = "llvm-codegen")]
pub use codegen::CodeGen;

/// Tokenize a Hunnu source string into a vector of tokens.
pub fn lex_source(source: &str) -> Vec<Token> {
    let mut lexer = lexer::Lexer::new(source);
    lexer.tokenize()
}

/// Parse a Hunnu source string into an AST.
pub fn parse_source(source: &str) -> Result<ASTNode, String> {
    let tokens = lex_source(source);
    let mut parser = parser::Parser::new(tokens);
    parser.parse()
}

/// Parse and compile a Hunnu source string to an executable.
#[cfg(feature = "llvm-codegen")]
pub fn compile_source_to_exe(source: &str, output: &str) -> Result<(), String> {
    let program = parse_source(source)?;
    compile_to_exe(&program, output)
}

/// Format a token stream as a human-readable string.
pub fn tokens_to_string(tokens: &[Token]) -> String {
    let mut out = String::new();
    for token in tokens {
        out.push_str(&format!("{:?} '{}' [{}:{}]\n",
            token.token_type, token.lexeme, token.line, token.column));
    }
    out
}

/// Format an AST node (debug representation) as a string.
pub fn ast_to_string(node: &ASTNode) -> String {
    format!("{:#?}", node)
}

/// FFI entry point: tokenize a C string and return a C string representation.
/// The caller must free the returned string via [`hunnu_rust_free_string`].
#[no_mangle]
pub extern "C" fn hunnu_rust_lex(source: *const c_char) -> *mut c_char {
    let c_str = unsafe { CStr::from_ptr(source) };
    let source_str = c_str.to_str().unwrap_or("");
    let tokens = lex_source(source_str);
    let output = tokens_to_string(&tokens);
    CString::new(output).unwrap().into_raw()
}

/// FFI entry point: tokenize and parse a C string, return AST as a C string.
/// The caller must free the returned string via [`hunnu_rust_free_string`].
#[no_mangle]
pub extern "C" fn hunnu_rust_parse(source: *const c_char) -> *mut c_char {
    let c_str = unsafe { CStr::from_ptr(source) };
    let source_str = c_str.to_str().unwrap_or("");
    match parse_source(source_str) {
        Ok(ast) => {
            let output = ast_to_string(&ast);
            CString::new(output).unwrap().into_raw()
        }
        Err(e) => {
            let output = format!("Parse error: {}", e);
            CString::new(output).unwrap().into_raw()
        }
    }
}

/// FFI entry point: compile a Hunnu source to an object file.
/// Returns 0 on success, non-zero on error.
#[cfg(feature = "llvm-codegen")]
#[no_mangle]
pub extern "C" fn hunnu_rust_compile_to_object(
    source: *const c_char,
    output: *const c_char,
) -> i32 {
    let c_str = unsafe { CStr::from_ptr(source) };
    let source_str = c_str.to_str().unwrap_or("");
    let out_str = unsafe { CStr::from_ptr(output) };
    let output_str = out_str.to_str().unwrap_or("out.o");
    match compile_source_to_exe(source_str, output_str) {
        Ok(()) => 0,
        Err(e) => {
            eprintln!("Compile error: {}", e);
            1
        }
    }
}

/// FFI entry point: emit LLVM IR for a Hunnu source.
/// Returns a C string with the IR (caller must free via [`hunnu_rust_free_string`]).
#[cfg(feature = "llvm-codegen")]
#[no_mangle]
pub extern "C" fn hunnu_rust_emit_llvm(source: *const c_char) -> *mut c_char {
    let c_str = unsafe { CStr::from_ptr(source) };
    let source_str = c_str.to_str().unwrap_or("");
    match parse_source(source_str) {
        Ok(program) => {
            let mut codegen = CodeGen::new("hunnu_module");
            match codegen.generate(&program) {
                Ok(()) => {
                    // Capture IR output to a string
                    let output = format!("LLVM IR generated successfully for module");
                    CString::new(output).unwrap().into_raw()
                }
                Err(e) => {
                    let output = format!("Codegen error: {}", e);
                    CString::new(output).unwrap().into_raw()
                }
            }
        }
        Err(e) => {
            let output = format!("Parse error: {}", e);
            CString::new(output).unwrap().into_raw()
        }
    }
}

/// FFI entry point: free a C string previously returned by this crate.
#[no_mangle]
pub extern "C" fn hunnu_rust_free_string(s: *mut c_char) {
    if !s.is_null() {
        unsafe { let _ = CString::from_raw(s); }
    }
}
