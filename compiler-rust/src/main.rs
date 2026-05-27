//! Hunnu compiler CLI binary.
//!
//! Accepts a `.hn` source file, tokenizes and parses it,
//! then optionally emits LLVM IR, object files, assembly, or executables.

use std::env;
use std::fs;
use std::process;

fn print_usage(program: &str) {
    eprintln!("Usage:");
    eprintln!("  {} <source.hn>                       # Debug: print tokens and AST", program);
    #[cfg(feature = "llvm-codegen")]
    {
        eprintln!("  {} <source.hn> --emit-llvm [file.bc]  # Emit LLVM bitcode", program);
        eprintln!("  {} <source.hn> --emit-obj <file.o>    # Emit native object file", program);
        eprintln!("  {} <source.hn> --emit-asm <file.s>    # Emit assembly text", program);
        eprintln!("  {} <source.hn> --compile <output>     # Compile to executable via gcc", program);
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();

    if args.len() < 2 {
        print_usage(&args[0]);
        process::exit(1);
    }

    let filename = &args[1];
    let source = match fs::read_to_string(filename) {
        Ok(s) => s,
        Err(e) => {
            eprintln!("Error reading file '{}': {}", filename, e);
            process::exit(1);
        }
    };

    #[cfg(feature = "llvm-codegen")]
    {
        let mode = if args.len() > 2 { args[2].as_str() } else { "" };
        match mode {
            "--emit-llvm" | "-emit-llvm" => {
                let output = if args.len() > 3 { Some(args[3].as_str()) } else { None };
                match hunnu_compiler::parse_source(&source) {
                    Ok(program) => {
                        if let Err(e) = hunnu_compiler::compile_to_ir(&program, output) {
                            eprintln!("Codegen error: {}", e);
                            process::exit(1);
                        }
                    }
                    Err(e) => {
                        eprintln!("Parse error: {}", e);
                        process::exit(1);
                    }
                }
                return;
            }
            "--emit-obj" | "-emit-obj" => {
                if args.len() < 4 {
                    eprintln!("Usage: {} <source.hn> --emit-obj <file.o>", args[0]);
                    process::exit(1);
                }
                let output = &args[3];
                match hunnu_compiler::parse_source(&source) {
                    Ok(program) => {
                        if let Err(e) = hunnu_compiler::compile_to_object(&program, output) {
                            eprintln!("Codegen error: {}", e);
                            process::exit(1);
                        }
                        println!("Wrote object file: {}", output);
                    }
                    Err(e) => {
                        eprintln!("Parse error: {}", e);
                        process::exit(1);
                    }
                }
                return;
            }
            "--emit-asm" | "-emit-asm" => {
                if args.len() < 4 {
                    eprintln!("Usage: {} <source.hn> --emit-asm <file.s>", args[0]);
                    process::exit(1);
                }
                let output = &args[3];
                match hunnu_compiler::parse_source(&source) {
                    Ok(program) => {
                        if let Err(e) = hunnu_compiler::compile_to_assembly(&program, output) {
                            eprintln!("Codegen error: {}", e);
                            process::exit(1);
                        }
                        println!("Wrote assembly file: {}", output);
                    }
                    Err(e) => {
                        eprintln!("Parse error: {}", e);
                        process::exit(1);
                    }
                }
                return;
            }
            "--compile" | "-compile" => {
                if args.len() < 4 {
                    eprintln!("Usage: {} <source.hn> --compile <output>", args[0]);
                    process::exit(1);
                }
                let output = &args[3];
                match hunnu_compiler::compile_source_to_exe(&source, output) {
                    Ok(()) => {
                        println!("Compiled executable: {}", output);
                    }
                    Err(e) => {
                        eprintln!("Compilation error: {}", e);
                        process::exit(1);
                    }
                }
                return;
            }
            _ => {}
        }
    }

    // Default: debug mode - print tokens and AST
    println!("=== Tokens ===");
    let tokens = hunnu_compiler::lex_source(&source);
    for token in &tokens {
        println!("{:?} '{}' [{}:{}]", token.token_type, token.lexeme, token.line, token.column);
    }

    match hunnu_compiler::parse_source(&source) {
        Ok(program) => {
            println!("\n=== AST ===");
            println!("{:#?}", program);

            #[cfg(feature = "llvm-codegen")]
            {
                println!("\n=== LLVM IR ===");
                if let Err(e) = hunnu_compiler::compile_to_ir(&program, None) {
                    eprintln!("Codegen error: {}", e);
                }
            }
        }
        Err(e) => {
            eprintln!("Parse error: {}", e);
            process::exit(1);
        }
    }
}
