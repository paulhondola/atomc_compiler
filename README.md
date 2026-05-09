# AtomC-Compiler

**AtomC-Compiler** is an educational compiler project designed to translate and execute **AtomC**, a simplified subset
of the C programming language. The project follows the fundamental stages of compiler construction, culminating in a
stack-based virtual machine (VM) that executes the generated code.

---

## Project Structure

The codebase is organized into modular components for clarity and maintainability:

- **`src/` & `include/`**:
    - **`frontend/`**: Lexical analysis (lexer) and syntactic analysis (parser).
    - **`analyzer/`**: Semantic analysis, including domain/scope management and type checking.
    - **`token/`**: Token definitions and token stream management.
    - **`vm/`**: Virtual machine implementation and instruction set.
    - **`utils/`**: Common utility functions (memory management, error handling).
    - **`main.c`**: Compiler entry point.
- **`tests/`**:
    - **`src/`**: Unit tests for the lexer, parser, and domain analyzer.
    - **`lexer/`, `parser/`**: Test data and expected outputs.
- **`docs/`**: Technical documentation and activity guides.

## Compiler Architecture

The compilation process is divided into several distinct stages:

### 1. Lexical Analysis (ALEX)

* **Purpose**: Groups individual characters from the source code into atomic units called **tokens**.
* **Implementation**: Uses a `tokenize` function to iterate through the source code and build a linked list of `Token` structures.

### 2. Syntactic Analysis (ASDR)

* **Algorithm**: Implements **Recursive Descent Analysis** (ASDR).
* **Process**: Transposes the formal grammar into a series of boolean functions that consume tokens based on language rules.

### 3. Domain & Type Analysis (AD/AT)

* **Symbol Table**: Organized as a stack of domains to handle nested scopes.
* **Semantic Rules**: Verifies that expressions adhere to AtomC constraints (L-values, type synthesis, compatibility).

### 4. Code Generation

* **Process**: Injects semantic actions into syntactic rules to generate VM instructions.
* **Stack Logic**: Focuses on address loading, dereferencing, and value storage.

## The Virtual Machine (VM)

The execution environment is a **stack-based virtual machine**.

* **Instruction Set**: Operations like `OP_PUSH`, `OP_POP`, `OP_CALL`, `OP_HALT`, and arithmetic comparisons.
* **Universal Values**: Uses a `Val` structure for dynamic data typing on the stack.

## Build and Run

This project uses [Meson](https://mesonbuild.com/) as its build system.

### Prerequisites

* A **C23** compatible compiler (e.g., GCC 14+ or Clang 18+)
* [Meson](https://mesonbuild.com/getting-started.html)
* [Ninja](https://ninja-build.org/)

### Building the Project

#### Initialize the build directory

```bash
meson setup target
```

#### Compile the source code

```bash
meson compile -C target
```

### Running the Compiler

After compiling, the `atomcc` executable will be available in `buildDir`:

```bash
./target/atomcc [input_file] --tokens [token_output_file] --domain [domain_output_file]
```

### Testing

Run the entire test suite via Meson:

```bash
meson test -C target
```

### Linting & Static Analysis

#### clang-format

Check formatting without modifying files:

```bash
clang-format --dry-run --Werror src/**/*.c include/**/*.h tests/src/*.c
```

Apply formatting in-place:

```bash
clang-format -i src/**/*.c include/**/*.h tests/src/*.c
```

#### clang-tidy

```bash
clang-tidy src/**/*.c tests/src/*.c -- -std=c23 -Iinclude
```

#### cppcheck

```bash
cppcheck --enable=all --std=c23 --inconclusive --suppress=missingIncludeSystem -Iinclude src/ tests/src/
```

### Memory Analysis

#### Valgrind (Linux)

```bash
valgrind --leak-check=full --track-origins=yes ./target/atomcc
valgrind --leak-check=full --track-origins=yes ./target/test_lexer
valgrind --leak-check=full --track-origins=yes ./target/test_parser
```

#### leaks (macOS)

```bash
leaks --atExit -- ./target/atomcc
leaks --atExit -- ./target/test_lexer
leaks --atExit -- ./target/test_parser
```
