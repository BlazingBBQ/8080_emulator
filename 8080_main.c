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

int main(int argc, char **argv) {
    int psize = 0;
    emu_state_t state = {0};
    state.mem = malloc(0x8000);

    // Memory map information from:
    // http://www.emutalk.net/threads/38177-Space-Invaders
    psize += read_file_to_buf("ROM/invaders.h", state.mem, 0x0000);
    psize += read_file_to_buf("ROM/invaders.g", state.mem, 0x0800);
    psize += read_file_to_buf("ROM/invaders.f", state.mem, 0x1000);
    psize += read_file_to_buf("ROM/invaders.e", state.mem, 0x1800);

    unsigned int opcode;
    while (state.pc < psize) {
        opcode = state.mem[state.pc];
        (*disasm_handlers[opcode])(state.mem, state.pc);
        state.pc += (*emu_handlers[opcode])(&state);
    }

    free(state.mem);
    return 0;
}
