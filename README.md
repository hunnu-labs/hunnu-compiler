# hunnu-compiler

Compiler infrastructure for [hunnu-lang](https://github.com/hunnu-labs/hunnu-lang) -- a lightweight, expression-oriented programming language.

This repository contains the compiler frontend and runtime components extracted from the main hunnu-lang project:

- **C Interpreter** -- Tree-walk interpreter with lexer, parser, AST, and runtime
- **C Bytecode VM** -- Stack-based bytecode compiler and virtual machine
- **C Transpiler** -- AOT compilation via C transpilation + gcc
- **Rust Compiler** -- Rust-based lexer, parser, AST, and LLVM codegen (experimental)
- **Rust VM** -- Rust-based stack VM (FFI-callable from C)

## Building

```bash
mkdir build && cd build
cmake ..
make
```

The `hunnu` binary will be built at `build/hunnu`.

### Options

- `-DBUILD_RUST_VM=ON` (default) -- Build and link Rust VM
- `-DBUILD_RUST_COMPILER=ON` (default) -- Build and link Rust compiler frontend

## Running

```bash
./build/hunnu run program.hn
./build/hunnu run program.hn --vm     # Use C bytecode VM
```

## Testing

```bash
cd build && make && ctest
```

## Project Structure

```
hunnu-compiler/
├── cli/                   # CLI entry point (main.c, cli.c)
├── compiler/
│   ├── ast/               # AST node definitions and operations
│   ├── interpreter/       # Tree-walk interpreter (eval, exec, call, builtins)
│   ├── lexer/             # Lexical analyzer (tokenizer)
│   ├── parser/            # Recursive descent parser
│   ├── transpile/         # C transpiler backend
│   ├── vm/                # Bytecode compiler and VM
│   ├── i18n/              # Internationalization (Mongolian/English errors)
│   ├── value.c/h          # Runtime value system
│   ├── scope.c/h          # Variable scope management
│   ├── import.c/h         # Module import resolver
│   └── version.c/h        # Version information
├── compiler-rust/         # Rust compiler frontend (experimental)
├── vm-rust/               # Rust VM implementation
├── tests/                 # C unit tests
└── CMakeLists.txt         # Build configuration
```

## License

MIT
