#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <wchar.h>

#include "8080_disasm.c"
#include "8080_emu.c"

#define SCREEN_WIDTH (256)
#define SCREEN_HEIGHT (224)
#define VRAM_START (0x2400)

uint16_t shift_reg;
uint8_t shift_reg_offset;

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
    switch (port) {
        case 2:
            /* Shift register offset */
            shift_reg_offset = (data & 0b111);
            break;
        case 3:
            /* Sounds */
            break;
        case 4:
            /* Shift register */
            shift_reg = (shift_reg >> 8) + (data << 8);
            break;
        case 5:
            /* Sounds */
            break;
        case 6:
            /* Watchdog */
            break;
        default:
            printf("::: Wrote to port %d: 0x%02x\n", port, data);
    }
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
    switch (port) {
        case 0:
            /* Hardware mapped P1 inputs, never used in code */
            // Fall-through to default case
            // break;
        case 1:
            /* Inputs
             * bit 0 = CREDIT (1 if deposit)
             * bit 1 = 2P start (1 if pressed)
             * bit 2 = 1P start (1 if pressed)
             * bit 3 = Always 1
             * bit 4 = 1P shot (1 if pressed)
             * bit 5 = 1P left (1 if pressed)
             * bit 6 = 1P right (1 if pressed)
             * bit 7 = Not connected
             */
            data = (0 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (1 << 3) |
                   (0 << 2) | (0 << 1) | (0 << 0);
            break;
        case 2:
            /* Inputs
             * bit 0 = DIP3 00 = 3 ships  10 = 5 ships
             * bit 1 = DIP5 01 = 4 ships  11 = 6 ships
             * bit 2 = Tilt
             * bit 3 = DIP6 0 = extra ship at 1500, 1 = extra ship at 1000
             * bit 4 = P2 shot (1 if pressed)
             * bit 5 = P2 left (1 if pressed)
             * bit 6 = P2 right (1 if pressed)
             * bit 7 = DIP7 Coin info displayed in demo screen 0=ON
             */
            data = (0 << 7) | (0 << 6) | (0 << 5) | (0 << 4) | (1 << 3) |
                   (0 << 2) | (0 << 1) | (0 << 0);
            break;
        case 3:
            /* Shift register result */
            data = (shift_reg >> (8 - shift_reg_offset)) & 0xff;
            break;
        default:
            printf("::: Read from port %d: 0x%02x\n", port, data);
    }

    return data;
}

/*
 * bit0: lower right
 * bit1: lower left
 * bit2: upper right
 * bit3: upper left
 */
wint_t box[16] = {0x0020, 0x2597, 0x2596, 0x2584, 0x259D, 0x2590,
                  0x259E, 0x259F, 0x2598, 0x259A, 0x258C, 0x2599,
                  0x2580, 0x259C, 0x259B, 0x2588};
/*
 * bit0: lower half
 * bit1: upper half
 */
wint_t h_box[4] = {0x0020, 0x2584, 0x2580, 0x2588};

void print_screen(emu_state_t *state) {
    int addr = VRAM_START + SCREEN_WIDTH / 8;

    printf("\e[1;1H\e[2J");  // Clear screen (ANSI terminals)
    wprintf(L"%lc", 0x250C);
    for (int i = 0; i < 224; i++) wprintf(L"%lc", 0x2500);
    wprintf(L"%lc\n", 0x2510);
    do {
#if 0
        /* Print using boxes split in 4 */
        addr -= 0x1;
        for (int i = 0; i < 4; i++) {
            printf("|");
            for (int j = 0; j < SCREEN_HEIGHT / 2; j++) {
                wprintf(L"%lc ", box[(!!(state->mem[addr + 0x20 * (j * 2)] &
                                         (0x1 << (7 - i * 2)))
                                      << 3) +
                                     (!!(state->mem[addr + 0x20 * (j * 2 +
                                     1)] &
                                         (0x1 << (7 - i * 2)))
                                      << 2) +
                                     (!!(state->mem[addr + 0x20 * (j * 2)] &
                                         (0x1 << (6 - i * 2)))
                                      << 1) +
                                     (!!(state->mem[addr + 0x20 * (j * 2 +
                                     1)] &
                                         (0x1 << (6 - i * 2))))]);
            }
            printf("|\n");
        }
#endif
        /* Print using boxes split in 2 */
        addr -= 0x1;
        for (int i = 0; i < 4; i++) {
            wprintf(L"%lc", 0x2502);
            for (int j = 0; j < SCREEN_HEIGHT; j++) {
                wprintf(L"%lc", h_box[(!!(state->mem[addr + 0x10 * j * 2] &
                                          (0x1 << (7 - i * 2)))
                                       << 1) +
                                      (!!(state->mem[addr + 0x10 * j * 2] &
                                          (0x1 << (6 - i * 2))))]);
            }
            wprintf(L"%lc\n", 0x2502);
        }
    } while (addr > VRAM_START);
    wprintf(L"%lc", 0x2514);
    for (int i = 0; i < 224; i++) wprintf(L"%lc", 0x2500);
    wprintf(L"%lc\n", 0x2518);
}

int main(int argc, char **argv) {
    setlocale(LC_CTYPE, "");

    unsigned int verbose = 0;
    unsigned int stop_at = 0;
    if (argc > 1) verbose = atoi(argv[1]);
    if (argc > 2) stop_at = atoi(argv[2]);

    shift_reg = 0x0000;
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
    unsigned int last_rst = 0;
    while (state.pc < psize) {
        if (state.halted) continue;

        opcode = state.mem[state.pc];

        if (verbose && instr_cnt % verbose == 0) {
            printf("%012d ", instr_cnt);  // Print instruction count
            print_flags(&state);
            printf("%*c", 12, ' ');  // Pad spacing
            (*disasm_handlers[opcode])(state.mem, state.pc);
        }

        // Emulate instuction
        state.pc += (*emu_handlers[opcode])(&state);

        // TODO: Use correct screen print interval
        if (instr_cnt % 400000 == 0) {
            print_screen(&state);
        }

        // Vertical sync interrupts
        // TODO: Use proper timing here too...
        // FIXME: Check if interrupts are enabled?
        if (instr_cnt % 100000 == 0) {
            emu_rst(&state, last_rst + 1);
            last_rst = !last_rst;
        }

        instr_cnt++;
        if (stop_at > 0 && instr_cnt > stop_at) break;
    }

    dump_state(&state);
    free(state.mem);
    return 0;
}
