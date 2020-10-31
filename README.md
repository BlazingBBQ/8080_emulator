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
