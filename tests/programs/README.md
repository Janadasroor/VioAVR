# AVR Test Programs

Real C programs compiled with `avr-gcc` and converted to simulator tests.

## Build All (use -j8 for parallel)

```bash
cd tests/programs
make clean
make -j8       # Compile all programs
make hex -j8   # Generate Intel HEX files
```

## Generate C++ Tests

```bash
cd project_root
python3 tests/make_tests.py --all
```

## Run Generated Tests

```bash
cd build
cmake ..
make -j8
ctest -R generated
```

## Programs

| Program | Description | Words |
|---------|-------------|-------|
| `smoke.c` | 4 NOPs + infinite loop (bare metal) | 5 |
| `alu.c` | ADD, SUB, AND, OR, XOR, INC, DEC, MUL | 30 |
| `alu_bare.c` | Naked assembly for precise opcodes | 50 |
| `branches.c` | BREQ, BRNE, RJMP, loops | 19 |
| `compare.c` | CP, CPC, CPI, CPSE | 11 |
| `io_ports.c` | DDRx, PORTx, PINx operations | 20 |
| `load_store.c` | LD, ST, LDS, STS, indirect | 11 |
| `small_ops.c` | SWAP, ASR, LSR, NEG, COM, TST | 18 |
| `stack.c` | PUSH, POP, function calls | 7 |
| `word_ops.c` | ADIW, SBIW, MOVW, 16-bit math | 20 |

## Adding New Programs

1. Write a C program in `tests/programs/`
2. If it should run without C runtime, add its name (without .c) to `BARE_PROGS` in Makefile
3. Run `make -j8` to compile
4. Run `python3 tests/make_tests.py program_name` to generate test
5. Run `ctest -R program_name` to verify

## Makefile Targets

```bash
make -j8        # Compile all programs
make hex -j8    # Generate HEX files
make clean      # Remove build artifacts
make info-<name> # Show disassembly for specific program
```
