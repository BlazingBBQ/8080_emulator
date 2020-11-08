#include <stdio.h>
#include <stdlib.h>

#include "8080_disasm.c"
#include "8080_emu.c"

int main(int argc, char **argv) {
    FILE *fp = fopen(argv[1], "rb");
    if (fp == NULL) {
        printf("error: Couldn't open %s\n", argv[1]);
        return 1;
    }

    // Get file size
    fseek(fp, 0L, SEEK_END);
    int fsize = ftell(fp);
    fseek(fp, 0L, SEEK_SET);

    // Read file info buffer
    unsigned char *codebuffer = malloc(fsize);
    fread(codebuffer, fsize, 1, fp);
    fclose(fp);

    unsigned int pc = 0;
    while (pc < fsize) pc += (*disasm_handlers[codebuffer[pc]])(codebuffer, pc);

    free(codebuffer);
    return 0;
}
