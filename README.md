# 8080_emulator

A simple 8080 emulator made with the goal of emulating space invaders. Based on and inspired by the guide at [emulator101](http://emulator101.com/).

## Notes

### Disassembler

I _really_ wanted a way to define each instruction in a single line. I had initially wanted something along the lines of:

```
#define D8(...)
#define D16(...)
#define REG(...)
#define ADDR(...)

#define INSTR(instr_name, format) ...
```

Where the `format` could then be define using the `D8`, `D16`, `REG` and `ADDR` macros like:

```
INSTR(NOP)
INSTR(LXI, REG(B), D16)
...
INSTR(SHLD, ADDR)
```

The solution I ended on defines an `INSTR_BASE` macro which is used to define macros for each instruction format, allowing them to pass in their own print statement. Then, each instruction handler is added to `disasm_handlers`, at their respective opcode index.

### Emulator

#### Flags

- Zero: if result of instruction has the value 0, flag is set; otherwise it is reset.
- Sign: if the most significant bit of the result of the operation has value 1, this flag is set; otherwise it is reset.
- Parity: if the module 2 sum of the bits of the result of the operation is 0, (i.e. result has even parity), this flag is set; otherwise it is reset.
- Carry: If the instruction results in a carry (from addition), or a borrow  (from subtraction or comparison) out of the high-order bit, this flag is set, otherwise it is reset.
- Auxiliary Carry: If the instruction caused a carry out of bit 3 and into bit 4 of the resulting value, the auxiliary carry is set; otherwise it is reset. This flag is affected by single precision additions, subtractions, increments, decrements, comparisons, and logical operations, but is principally used with additions and increments preceding a DAA (Decimal Adjust Accumulator) instruction.
