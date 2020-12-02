#include <stdio.h>
#include <stdlib.h>

#include "8080_disasm.c"
#include "8080_emu.c"

/*
 * read_file_to_buf: Reads file into memory buffer at given offset.
 *
 * Arguments:
 *   filename - name of file to load into buffer
 *   buf      - buffer to load file contents into
 *   offset   - offset at which to load the file contents
 *
 * Returns:
 *   number of bytes read into buffer.
 */
int read_file_to_buf(char *filename, uint8_t *buf, uint32_t offset) {
    int rc, fsize;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        printf("error: Couldn't open %s\n", filename);
        exit(1);
    }

    // Get file size
    fseek(fp, 0L, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // Read file info buffer
    rc = fread(buf + offset, 1, fsize, fp);
    fclose(fp);

    return rc;
}

/*
 * write_port: Device handler for writing data to I/O ports.
 *
 * Arguments:
 *   port   - addr of port to write data to
 *   data   - data to write to port
 */
void write_port(uint8_t port, uint8_t data) {
    printf("::: Wrote to port %d: 0x%02x\n", port, data);
}

/*
 * read_port: Device handler for reading data from I/O ports.
 *
 * Arguments:
 *   port   - addr of port to read data from
 *
 * Returns:
 *   data read from port
 */
uint8_t read_port(uint8_t port) {
    uint8_t data = 0x00;
    printf("::: Read from port %d: 0x%02x\n", port, data);

    return data;
}

int main(int argc, char **argv) {
    unsigned int stop_at = 0;
    if (argc > 1) stop_at = atoi(argv[1]);

    int psize = 0;
    emu_state_t state = {0};
    state.interrupts_enabled = 1;  // Enable interrupts by default
    state.write_port = write_port;
    state.read_port = read_port;
    state.mem = malloc(0x8000);

    // Memory map information from:
    // http://www.emutalk.net/threads/38177-Space-Invaders
    psize += read_file_to_buf("ROM/invaders.h", state.mem, 0x0000);
    psize += read_file_to_buf("ROM/invaders.g", state.mem, 0x0800);
    psize += read_file_to_buf("ROM/invaders.f", state.mem, 0x1000);
    psize += read_file_to_buf("ROM/invaders.e", state.mem, 0x1800);

    unsigned int opcode;
    unsigned int instr_cnt = 0;
    while (state.pc < psize) {
        if (state.halted) continue;

        opcode = state.mem[state.pc];

        printf("%012d ", instr_cnt);  // Print instruction count
        print_flags(&state);
        printf("%*c", 12, ' ');  // Pad spacing
        (*disasm_handlers[opcode])(state.mem, state.pc);

        // Emulate instuction
        state.pc += (*emu_handlers[opcode])(&state);

        instr_cnt++;
        if (stop_at > 0 && instr_cnt > stop_at) {
            dump_state(&state);
            break;
        }
    }

    free(state.mem);
    return 0;
}
