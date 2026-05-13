# iklang

A compiled, stack-based programming language targeting x86-64 Linux. Source files (`.ikl`) are compiled to native ELF binaries via NASM.

> **Platform:** Linux x86-64 only. NASM and GNU ld must be installed.

---

## Table of Contents

- [Requirements](#requirements)
- [Building](#building)
- [Installing](#installing)
- [Quick Start](#quick-start)
- [Language Reference](#language-reference)
  - [The Stack Model](#the-stack-model)
  - [Type Checking](#type-checking)
  - [Comments](#comments)
  - [Integer Literals](#integer-literals)
  - [String Literals](#string-literals)
  - [Arithmetic](#arithmetic)
  - [Comparison](#comparison)
  - [Logic](#logic)
  - [Stack Operations](#stack-operations)
  - [Bindings](#bindings)
  - [Control Flow](#control-flow)
  - [Loops](#loops)
  - [Memory](#memory)
  - [Macros](#macros)
  - [Includes](#includes)
  - [Syscalls](#syscalls)
  - [Command-Line Arguments](#command-line-arguments)
- [Standard Library](#standard-library)
- [Running Tests](#running-tests)
- [Contributing](#contributing)
- [Acknowledgements](#acknowledgements)

---

## Requirements

- GCC
- NASM
- GNU ld (`binutils`)

On Debian/Ubuntu:

```sh
sudo apt install gcc nasm binutils
```

---

## Building

```sh
git clone <repo>
cd iklang
make
```

The compiler binary is placed at `build/iklc`. The `PREFIX` variable controls the install path (default `/usr/local`).

```sh
make clean       # remove build/
make run         # run build/iklc with no arguments
```

---

## Installing

```sh
# Install to /usr/local/bin and /usr/local/lib/iklang (requires sudo)
sudo make install

# Install to a custom prefix (no sudo needed for ~/.local)
make install PREFIX=~/.local

# Uninstall
sudo make uninstall
```

`make install` places:
- `iklc` → `$(PREFIX)/bin/iklc`
- `lib/*.ikl` → `$(PREFIX)/lib/iklang/`

The compiler is rebuilt as part of `make install` so that the library path baked into the binary matches `PREFIX`.

---

## Quick Start

Create `hello.ikl`:

```
"std.ikl" include

"Hello, World!\n" print
```

Compile and run:

```sh
iklc hello.ikl
./out
```

> `write` expects an explicit `(len, ptr)` pair on the stack. For string literals use `print`, which computes the length automatically.

Without the standard library:

```sh
echo '1 2 + dump' > calc.ikl
iklc calc.ikl
./out
# 3
```

---

## Language Reference

### The Stack Model

iklang is stack-based. Every value is a 64-bit integer on the hardware stack. All operations consume values from the top and push results back.

Tokens are separated by whitespace. Execution is left-to-right.

---

### Type Checking

The compiler performs static type checking after parsing, before code generation. Two types exist:

| Type           | Produced by                                 |
|----------------|---------------------------------------------|
| `T_INT`        | integer literals, arithmetic results        |
| `T_STRING_LIT` | string literals                             |

`T_STRING_LIT` is integer-compatible: it can be used wherever an integer is expected (arithmetic, comparisons, `dump`, syscall arguments, etc.).

Type errors are reported at compile time:

- **Stack underflow** — an operation needs more values than are present.
- **Type mismatch** — an operation receives a value of the wrong type.
- **Branch mismatch** — `if`/`else` branches or a `while` body leave the stack in different states.
- **Non-empty stack** — values remain on the stack when execution ends.

---

### Comments

Comments are sections of the code that are stripped of at the tokenizing step. They begin by writing a ';' character and end at the next newline. 

```
; This is a comment!
12 12 + ; This is also a comment!
```

---

### Integer Literals

An integer literal pushes its value onto the stack.

```
42       -- stack: [42]
10 3     -- stack: [10, 3]   (3 on top)
```

---

### String Literals

A quoted string `"..."` pushes the pointer to the string data onto the stack. The pushed value has type `T_STRING_LIT`.

```
"Hello\n"   -- stack: [ptr]
```

Supported escape sequences: `\n` `\t` `\r` `\0` `\\` `\"`

The pointer references a static read-only data segment. Strings cannot span multiple lines.

---

### Arithmetic

All arithmetic operations pop two values (right operand on top) and push one result.

| Token | Operation          | Stack before → after         |
|-------|--------------------|------------------------------|
| `+`   | addition           | `a b +` → `a+b`              |
| `-`   | subtraction        | `a b -` → `a-b`              |
| `*`   | multiplication     | `a b *` → `a*b`              |
| `/`   | integer division   | `a b /` → `a/b`              |
| `%`   | modulo             | `a b %` → `a%b`              |

Division or modulo by zero prints an error to stderr and exits with code 1.

```
10 3 - dump    -- prints 7
4 5 * dump     -- prints 20
```

---

### Comparison

Comparison operators pop two values and push `1` (true) or `0` (false). The right operand is on top.

| Token | Condition         | Example         | Result |
|-------|-------------------|-----------------|--------|
| `=`   | equal             | `3 3 =`         | `1`    |
| `!=`  | not equal         | `3 4 !=`        | `1`    |
| `>`   | greater than      | `5 3 >`         | `1`    |
| `>=`  | greater or equal  | `3 3 >=`        | `1`    |
| `<`   | less than         | `2 5 <`         | `1`    |
| `<=`  | less or equal     | `4 5 <=`        | `1`    |

```
5 3 > dump     -- prints 1
1 2 = dump     -- prints 0
```

---

### Logic

| Token | Operation    | Stack before → after                          |
|-------|--------------|-----------------------------------------------|
| `!`   | boolean NOT  | `0 !` → `1`;  any non-zero `!` → `0`         |

```
0 ! dump    -- prints 1
5 ! dump    -- prints 0
```

---

### Stack Operations

| Token  | Stack effect        | Description                                      |
|--------|---------------------|--------------------------------------------------|
| `dump` | `( a -- )`          | Pop and print top of stack as decimal            |
| `dup`  | `( a -- a a )`      | Duplicate the top of stack                       |
| `drop` | `( a -- )`          | Discard the top of stack                         |
| `swap` | `( a b -- b a )`    | Swap the top two elements                        |
| `over` | `( a b -- a b a )`  | Copy the second element onto the top             |
| `rot`  | `( a b c -- b c a )`| Rotate the top three: third comes to the top     |
| `nip`  | `( a b -- b )`      | Discard the second element                       |
| `tuck` | `( a b -- b a b )`  | Copy the top element below the second            |

```
7 dup dump dump        -- prints 7 twice
1 2 swap dump dump     -- prints 1 then 2  (swap: top comes second)
1 2 3 rot dump dump dump   -- prints 1, 3, 2  (1 rotated to top)
```

---

### Bindings

Bindings are named 64-bit slots stored in the BSS segment.

#### Declaration — `let`

Write the name before `let`. Pops the top of the stack and stores it.

```
42 x let       -- x = 42; stack is now empty
x dump         -- prints 42
```

A name can only be declared once per program. Re-declaration is a compile-time error.

#### Mutation — `set`

`set` mutates an existing binding. Syntax is the same as `let`.

```
10 n let
20 n set
n dump    -- prints 20
```

#### Reading a binding

Writing a bare name pushes its current value onto the stack.

```
5 a let
3 b let
a b + dump    -- prints 8
```

---

### Control Flow

#### `if … end`

Pops the top of the stack. Executes the block if the value is non-zero.

```
5 3 > if
    10 dump
end
```

#### `if … else … end`

```
0 flag let

flag if
    1 dump
else
    0 dump    -- this branch runs
end
```

Blocks can be nested arbitrarily.

---

### Loops

#### `while … do … end`

The condition block before `do` is evaluated each iteration. `do` pops the result; the loop continues while it is non-zero.

```
1 i let
while i 5 <= do
    i dump
    i 1 + i set
end
```

Prints `1` through `5`.

Sum of 1..10:

```
1 i let
0 sum let
while i 10 <= do
    i sum + sum set
    i 1 + i set
end
sum dump    -- prints 55
```

---

### Memory

`mem` allocates a typed array using `brk`. Each array stores its stride and size in a 16-byte header immediately before the returned pointer, which is used for bounds checking.

#### Allocation

```
<size> <stride> mem
```

Pushes a pointer to `size` elements of `stride` bytes each.

```
4 8 mem arr let    -- 4 elements, 8 bytes each (64-bit)
```

Valid strides: `1`, `2`, `4`, `8`.

#### Writing an element

```
<value> <index> <name> load set
```

```
99 2 arr load set    -- arr[2] = 99
```

#### Reading an element

```
<index> <name> load
```

Pushes the value at that index.

```
2 arr load dump    -- prints arr[2]
```

Out-of-bounds access prints an error to stderr and exits with code 1.

**Full example:**

```
3 8 mem nums let

10 0 nums load set
20 1 nums load set
30 2 nums load set

0 nums load dump    -- 10
1 nums load dump    -- 20
2 nums load dump    -- 30
```

#### Operating on raw pointers

If you try to load memory from a raw pointer, you have to specify the stride of the memory.

This is done by using the `l8`, `l16`, `l32` and `l32` keywords.

```
"Hello, World!" str let
str     l8 dump  -- 72
str 1 + l8 dump  -- 101
```

---

### Macros

Macros are inlined at parse time. Define a macro with `<name> macro … end`, invoke it by writing the name.

```
double macro
    2 *
end

5 double dump     -- 10
```

Macros can call other macros:

```
double macro
    2 *
end

quadruple macro
    double double
end

3 quadruple dump    -- 12
```

Macros can contain `if`/`while` blocks and reference bindings. Macro definitions must appear before their first use. Nested macro definitions are not allowed.

---

### Includes

```
"filename.ikl" include
```

Includes are resolved at parse time. Search order:

1. Path as given, relative to the current working directory.
2. The installed library directory (`/usr/local/lib/iklang` by default, or whatever `PREFIX` was set to at build time).

Circular and duplicate includes are compile-time errors.

```
"std.ikl" include          -- found in lib dir after install
"./utils/helpers.ikl" include   -- relative path
```

---

### Syscalls

#### `syscall3`

Calls a Linux syscall with three arguments. Pops four values in order:

| Pop order | Register | Meaning         |
|-----------|----------|-----------------|
| 1st       | `rax`    | syscall number  |
| 2nd       | `rdi`    | argument 1      |
| 3rd       | `rsi`    | argument 2      |
| 4th       | `rdx`    | argument 3      |

Pushes the return value (`rax`).

Push operands in reverse (last arg first):

```
-- write(1, ptr, len)  →  syscall 1, rdi=1, rsi=ptr, rdx=len
-- push order: rdx(len), rsi(ptr), rdi(fd), rax(syscall_nr)

len ptr 1 1 syscall3 drop
```

String literals push only the pointer, so the length must be provided separately. The easiest way to write a string to stdout is the `print` macro from `std.ikl`, which calls `strlen` internally:

```
"std.ikl" include
"Hello\n" print
```

To call `syscall3` directly you need the length on the stack:

```
"Hello\n" strlen swap 1 1 syscall3 drop
--                     ^ fd=1   ^ syscall 1 (write)
```

---

### Command-Line Arguments

Two bindings are available in every program without declaration:

| Binding | Type    | Value                                         |
|---------|---------|-----------------------------------------------|
| `argc`  | integer | number of command-line arguments (including the program name) |
| `argv`  | integer | pointer to the array of argument string pointers |

`argv` is a flat array of 8-byte pointers. To get the string pointer for argument `i`, read 8 bytes at `argv + 8 * i`:

```
argv 8 i * + l64
```

Full example (`examples/arguments.ikl`) — print every argument on its own line:

```
"std.ikl" include

0 i let

while i argc < do
    argv 8 i * + l64 print
    "\n" print

    i 1 + i set
end
```

```sh
iklc examples/arguments.ikl
./out hello world
# ./out
# hello
# world
```

---

## Standard Library

The file `lib/std.ikl` is installed to `$(PREFIX)/lib/iklang/std.ikl`. Include it with:

```
"std.ikl" include
```

### `write`

Writes bytes to stdout. Expects `len` below `ptr` on the stack (ptr on top), and leaves the syscall return value on the stack.

```
write macro 1 1 syscall3 end
```

**Usage:** push `len` then `ptr`, then call `write`. Drop the return value if unneeded.

```
14 "Hello, World!\n" write drop
```

For string literals, prefer `print`, which computes the length automatically.

### `read`

Read stdin into a buffer.

```
read macro 
    dup 8 - load 1 -
    over
    0 0 syscall3
    over over + 0 swap set
    over
end
```

**Usage:** place an allocated ptr+len pair on the stack, then call `read`.

```
128 1 mem buf let
buf read 
```

### `strlen`

Pushes the length (in bytes) of a null-terminated string onto the stack, leaving the original pointer below.

```
strlen macro
    0
    while over over + l8 0 != do
        1 +
    end
end
```

**Usage:** place a string pointer on the stack, then call `strlen`. Stack effect: `( ptr -- ptr len )`.

```
"Hello, World!" strlen   -- stack: [ptr, 13]
```

### `print`

Writes a null-terminated string to stdout. Computes the length with `strlen`, then calls `write`. Discards the syscall return value.

```
print macro
    strlen
    swap
    1 1 syscall3
    drop
end
```

**Usage:** place a string pointer on the stack, then call `print`.

```
"Hello, World!\n" print
```

---

## Running Tests

Build the test runner and run all tests:

```sh
gcc -o tests/ikl_test_tool tests/ikl_test_tool.c
./tests/ikl_test_tool
```

Each test is a pair `tests/<name>.ikl` + `tests/<name>.expected`. The runner compiles each `.ikl` with `build/iklc`, executes the binary, and diffs stdout against the expected output. Tests without a `.expected` file are skipped.

---

## Contributing

Pull requests and forks are welcome. Please mention this project if you distribute derived work.

---

## Acknowledgements

This project is inspired by the work of [Alexey Kutepov](https://rexim.github.io/). Parallels in design and paradigms are intentional.

This project uses AI-assisted static analysis and refactoring, specifically for bug-finding and code consistency. All design decisions and language semantics are by the author.
