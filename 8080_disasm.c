#include <stdio.h>
#include <string.h>

#define ARG_INDENT 8

#define INSTR_BASE(name, size, format...)                           \
    int disasm_##name(unsigned char *codebuffer, unsigned int pc) { \
        unsigned char *codebyte = &codebuffer[pc];                  \
        printf(format);                                             \
        printf("\n");                                               \
        return size;                                                \
    }
#define INSTR(name) INSTR_BASE(name, 1, #name)
#define INSTR_RST(num)       \
    INSTR_BASE(RST_##num, 1, \
               "RST"         \
               "%*s%s",      \
               ARG_INDENT - (int)strlen("RST"), "", #num)
#define INSTR_R(name, r)                                                      \
    INSTR_BASE(name##_##r, 1, #name "%*s%s", ARG_INDENT - (int)strlen(#name), \
               "", #r)
#define INSTR_R_R(name, r1, r2)                          \
    INSTR_BASE(name##_##r1##_##r2, 1, #name "%*s%s, %s", \
               ARG_INDENT - (int)strlen(#name), "", #r1, #r2)
#define INSTR_R_D8(name, r)                          \
    INSTR_BASE(name##_##r, 2, #name "%*s%s, 0x%02x", \
               ARG_INDENT - (int)strlen(#name), "", #r, codebyte[1])
#define INSTR_R_D16(name, r)                                         \
    INSTR_BASE(name##_##r, 3, #name "%*s%s, 0x%02x%02x",             \
               ARG_INDENT - (int)strlen(#name), "", #r, codebyte[2], \
               codebyte[1])
#define INSTR_D8(name)                                                      \
    INSTR_BASE(name, 2, #name "%*s0x%02x", ARG_INDENT - (int)strlen(#name), \
               "", codebyte[1])
#define INSTR_D16(name)                        \
    INSTR_BASE(name, 3, #name "%*s0x%02x%02x", \
               ARG_INDENT - (int)strlen(#name), "", codebyte[2], codebyte[1])
#define INSTR_ADDR(name)                       \
    INSTR_BASE(name, 3, #name "%*s0x%02x%02x", \
               ARG_INDENT - (int)strlen(#name), "", codebyte[2], codebyte[1])

INSTR_BASE(unimplemented, 1, "Unimplemented opcode <%02x> at addr: %08x",
           codebyte[0], pc)

/* --- 8080 Instructions --- */

INSTR(NOP)
INSTR_R_D16(LXI, B)
INSTR_R(STAX, B)
INSTR_R(INX, B)
INSTR_R(INR, B)
INSTR_R(DCR, B)
INSTR_R_D8(MVI, B)
INSTR(RLC)
// 0x08 --
INSTR_R(DAD, B)
INSTR_R(LDAX, B)
INSTR_R(DCX, B)
INSTR_R(INR, C)
INSTR_R(DCR, C)
INSTR_R_D8(MVI, C)
INSTR(RRC)
// 0x10 --
INSTR_R_D16(LXI, D)
INSTR_R(STAX, D)
INSTR_R(INX, D)
INSTR_R(INR, D)
INSTR_R(DCR, D)
INSTR_R_D8(MVI, D)
INSTR(RAL)
// 0x18 --
INSTR_R(DAD, D)
INSTR_R(LDAX, D)
INSTR_R(DCX, D)
INSTR_R(INR, E)
INSTR_R(DCR, E)
INSTR_R_D8(MVI, E)
INSTR(RAR)
INSTR(RIM)
INSTR_R_D16(LXI, H)
INSTR_ADDR(SHLD)
INSTR_R(INX, H)
INSTR_R(INR, H)
INSTR_R(DCR, H)
INSTR_R_D8(MVI, H)
INSTR(DAA)
// 0x28 --
INSTR_R(DAD, H)
INSTR_ADDR(LHLD)
INSTR_R(DCX, H)
INSTR_R(INR, L)
INSTR_R(DCR, L)
INSTR_R_D8(MVI, L)
INSTR(CMA)
INSTR(SIM)
INSTR_R_D16(LXI, SP)
INSTR_ADDR(STA)
INSTR_R(INX, SP)
INSTR_R(INR, M)
INSTR_R(DCR, M)
INSTR_R_D8(MVI, M)
INSTR(STC)
// 0x38 --
INSTR_R(DAD, SP)
INSTR_ADDR(LDA)
INSTR_R(DCX, SP)
INSTR_R(INR, A)
INSTR_R(DCR, A)
INSTR_R_D8(MVI, A)
INSTR(CMC)
INSTR_R_R(MOV, B, B)
INSTR_R_R(MOV, B, C)
INSTR_R_R(MOV, B, D)
INSTR_R_R(MOV, B, E)
INSTR_R_R(MOV, B, H)
INSTR_R_R(MOV, B, L)
INSTR_R_R(MOV, B, M)
INSTR_R_R(MOV, B, A)
INSTR_R_R(MOV, C, B)
INSTR_R_R(MOV, C, C)
INSTR_R_R(MOV, C, D)
INSTR_R_R(MOV, C, E)
INSTR_R_R(MOV, C, H)
INSTR_R_R(MOV, C, L)
INSTR_R_R(MOV, C, M)
INSTR_R_R(MOV, C, A)
INSTR_R_R(MOV, D, B)
INSTR_R_R(MOV, D, C)
INSTR_R_R(MOV, D, D)
INSTR_R_R(MOV, D, E)
INSTR_R_R(MOV, D, H)
INSTR_R_R(MOV, D, L)
INSTR_R_R(MOV, D, M)
INSTR_R_R(MOV, D, A)
INSTR_R_R(MOV, E, B)
INSTR_R_R(MOV, E, C)
INSTR_R_R(MOV, E, D)
INSTR_R_R(MOV, E, E)
INSTR_R_R(MOV, E, H)
INSTR_R_R(MOV, E, L)
INSTR_R_R(MOV, E, M)
INSTR_R_R(MOV, E, A)
INSTR_R_R(MOV, H, B)
INSTR_R_R(MOV, H, C)
INSTR_R_R(MOV, H, D)
INSTR_R_R(MOV, H, E)
INSTR_R_R(MOV, H, H)
INSTR_R_R(MOV, H, L)
INSTR_R_R(MOV, H, M)
INSTR_R_R(MOV, H, A)
INSTR_R_R(MOV, L, B)
INSTR_R_R(MOV, L, C)
INSTR_R_R(MOV, L, D)
INSTR_R_R(MOV, L, E)
INSTR_R_R(MOV, L, H)
INSTR_R_R(MOV, L, L)
INSTR_R_R(MOV, L, M)
INSTR_R_R(MOV, L, A)
INSTR_R_R(MOV, M, B)
INSTR_R_R(MOV, M, C)
INSTR_R_R(MOV, M, D)
INSTR_R_R(MOV, M, E)
INSTR_R_R(MOV, M, H)
INSTR_R_R(MOV, M, L)
INSTR(HLT)
INSTR_R_R(MOV, M, A)
INSTR_R_R(MOV, A, B)
INSTR_R_R(MOV, A, C)
INSTR_R_R(MOV, A, D)
INSTR_R_R(MOV, A, E)
INSTR_R_R(MOV, A, H)
INSTR_R_R(MOV, A, L)
INSTR_R_R(MOV, A, M)
INSTR_R_R(MOV, A, A)
INSTR_R(ADD, B)
INSTR_R(ADD, C)
INSTR_R(ADD, D)
INSTR_R(ADD, E)
INSTR_R(ADD, H)
INSTR_R(ADD, L)
INSTR_R(ADD, M)
INSTR_R(ADD, A)
INSTR_R(ADC, B)
INSTR_R(ADC, C)
INSTR_R(ADC, D)
INSTR_R(ADC, E)
INSTR_R(ADC, H)
INSTR_R(ADC, L)
INSTR_R(ADC, M)
INSTR_R(ADC, A)
INSTR_R(SUB, B)
INSTR_R(SUB, C)
INSTR_R(SUB, D)
INSTR_R(SUB, E)
INSTR_R(SUB, H)
INSTR_R(SUB, L)
INSTR_R(SUB, M)
INSTR_R(SUB, A)
INSTR_R(SBB, B)
INSTR_R(SBB, C)
INSTR_R(SBB, D)
INSTR_R(SBB, E)
INSTR_R(SBB, H)
INSTR_R(SBB, L)
INSTR_R(SBB, M)
INSTR_R(SBB, A)
INSTR_R(ANA, B)
INSTR_R(ANA, C)
INSTR_R(ANA, D)
INSTR_R(ANA, E)
INSTR_R(ANA, H)
INSTR_R(ANA, L)
INSTR_R(ANA, M)
INSTR_R(ANA, A)
INSTR_R(XRA, B)
INSTR_R(XRA, C)
INSTR_R(XRA, D)
INSTR_R(XRA, E)
INSTR_R(XRA, H)
INSTR_R(XRA, L)
INSTR_R(XRA, M)
INSTR_R(XRA, A)
INSTR_R(ORA, B)
INSTR_R(ORA, C)
INSTR_R(ORA, D)
INSTR_R(ORA, E)
INSTR_R(ORA, H)
INSTR_R(ORA, L)
INSTR_R(ORA, M)
INSTR_R(ORA, A)
INSTR_R(CMP, B)
INSTR_R(CMP, C)
INSTR_R(CMP, D)
INSTR_R(CMP, E)
INSTR_R(CMP, H)
INSTR_R(CMP, L)
INSTR_R(CMP, M)
INSTR_R(CMP, A)
INSTR(RNZ)
INSTR_R(POP, B)
INSTR_ADDR(JNZ)
INSTR_ADDR(JMP)
INSTR_ADDR(CNZ)
INSTR_R(PUSH, B)
INSTR_D8(ADI)
INSTR_RST(0)
INSTR(RZ)
INSTR(RET)
INSTR_ADDR(JZ)
// 0xcb --
INSTR_ADDR(CZ)
INSTR_ADDR(CALL)
INSTR_D8(ACI)
INSTR_RST(1)
INSTR(RNC)
INSTR_R(POP, D)
INSTR_ADDR(JNC)
INSTR_D8(OUT)
INSTR_ADDR(CNC)
INSTR_R(PUSH, D)
INSTR_D8(SUI)
INSTR_RST(2)
INSTR(RC)
// 0xd9 --
INSTR_ADDR(JC)
INSTR_D8(IN)
INSTR_ADDR(CC)
// 0xdd --
INSTR_D8(SBI)
INSTR_RST(3)
INSTR(RPO)
INSTR_R(POP, H)
INSTR_ADDR(JPO)
INSTR(XTHL)
INSTR_ADDR(CPO)
INSTR_R(PUSH, H)
INSTR_D8(ANI)
INSTR_RST(4)
INSTR(RPE)
INSTR(PCHL)
INSTR_ADDR(JPE)
INSTR(XCHG)
INSTR_ADDR(CPE)
// 0xed --
INSTR_D8(XRI)
INSTR_RST(5)
INSTR(RP)
INSTR_R(POP, PSW)
INSTR_ADDR(JP)
INSTR(DI)
INSTR_ADDR(CP)
INSTR_R(PUSH, PSW)
INSTR_D8(ORI)
INSTR_RST(6)
INSTR(RM)
INSTR(SPHL)
INSTR_ADDR(JM)
INSTR(EI)
INSTR_ADDR(CM)
// 0xfc --
INSTR_D8(CPI)
INSTR_RST(7)

/*
 * disasm_handlers: Disassembly handler functions, indexed by opcode.
 *
 * Returns:
 *      Number of bytes to advance pc.
 */
int (*disasm_handlers[0x100])(unsigned char *codebuffer,
                              unsigned int pc) = {
    disasm_NOP,            // 0x00
    disasm_LXI_B,          // 0x01
    disasm_STAX_B,         // 0x02
    disasm_INX_B,          // 0x03
    disasm_INR_B,          // 0x04
    disasm_DCR_B,          // 0x05
    disasm_MVI_B,          // 0x06
    disasm_RLC,            // 0x07
    disasm_unimplemented,  // 0x08
    disasm_DAD_B,          // 0x09
    disasm_LDAX_B,         // 0x0a
    disasm_DCX_B,          // 0x0b
    disasm_INR_C,          // 0x0c
    disasm_DCR_C,          // 0x0d
    disasm_MVI_C,          // 0x0e
    disasm_RRC,            // 0x0f
    disasm_unimplemented,  // 0x10
    disasm_LXI_D,          // 0x11
    disasm_STAX_D,         // 0x12
    disasm_INX_D,          // 0x13
    disasm_INR_D,          // 0x14
    disasm_DCR_D,          // 0x15
    disasm_MVI_D,          // 0x16
    disasm_RAL,            // 0x17
    disasm_unimplemented,  // 0x18
    disasm_DAD_D,          // 0x19
    disasm_LDAX_D,         // 0x1a
    disasm_DCX_D,          // 0x1b
    disasm_INR_E,          // 0x1c
    disasm_DCR_E,          // 0x1d
    disasm_MVI_E,          // 0x1e
    disasm_RAR,            // 0x1f
    disasm_RIM,            // 0x20
    disasm_LXI_H,          // 0x21
    disasm_SHLD,           // 0x22
    disasm_INX_H,          // 0x23
    disasm_INR_H,          // 0x24
    disasm_DCR_H,          // 0x25
    disasm_MVI_H,          // 0x26
    disasm_DAA,            // 0x27
    disasm_unimplemented,  // 0x28
    disasm_DAD_H,          // 0x29
    disasm_LHLD,           // 0x2a
    disasm_DCX_H,          // 0x2b
    disasm_INR_L,          // 0x2c
    disasm_DCR_L,          // 0x2d
    disasm_MVI_L,          // 0x2e
    disasm_CMA,            // 0x2f
    disasm_SIM,            // 0x30
    disasm_LXI_SP,         // 0x31
    disasm_STA,            // 0x32
    disasm_INX_SP,         // 0x33
    disasm_INR_M,          // 0x34
    disasm_DCR_M,          // 0x35
    disasm_MVI_M,          // 0x36
    disasm_STC,            // 0x37
    disasm_unimplemented,  // 0x38
    disasm_DAD_SP,         // 0x39
    disasm_LDA,            // 0x3a
    disasm_DCX_SP,         // 0x3b
    disasm_INR_A,          // 0x3c
    disasm_DCR_A,          // 0x3d
    disasm_MVI_A,          // 0x3e
    disasm_CMC,            // 0x3f
    disasm_MOV_B_B,        // 0x40
    disasm_MOV_B_C,        // 0x41
    disasm_MOV_B_D,        // 0x42
    disasm_MOV_B_E,        // 0x43
    disasm_MOV_B_H,        // 0x44
    disasm_MOV_B_L,        // 0x45
    disasm_MOV_B_M,        // 0x46
    disasm_MOV_B_A,        // 0x47
    disasm_MOV_C_B,        // 0x48
    disasm_MOV_C_C,        // 0x49
    disasm_MOV_C_D,        // 0x4a
    disasm_MOV_C_E,        // 0x4b
    disasm_MOV_C_H,        // 0x4c
    disasm_MOV_C_L,        // 0x4d
    disasm_MOV_C_M,        // 0x4e
    disasm_MOV_C_A,        // 0x4f
    disasm_MOV_D_B,        // 0x50
    disasm_MOV_D_C,        // 0x51
    disasm_MOV_D_D,        // 0x52
    disasm_MOV_D_E,        // 0x53
    disasm_MOV_D_H,        // 0x54
    disasm_MOV_D_L,        // 0x55
    disasm_MOV_D_M,        // 0x56
    disasm_MOV_D_A,        // 0x57
    disasm_MOV_E_B,        // 0x58
    disasm_MOV_E_C,        // 0x59
    disasm_MOV_E_D,        // 0x5a
    disasm_MOV_E_E,        // 0x5b
    disasm_MOV_E_H,        // 0x5c
    disasm_MOV_E_L,        // 0x5d
    disasm_MOV_E_M,        // 0x5e
    disasm_MOV_E_A,        // 0x5f
    disasm_MOV_H_B,        // 0x60
    disasm_MOV_H_C,        // 0x61
    disasm_MOV_H_D,        // 0x62
    disasm_MOV_H_E,        // 0x63
    disasm_MOV_H_H,        // 0x64
    disasm_MOV_H_L,        // 0x65
    disasm_MOV_H_M,        // 0x66
    disasm_MOV_H_A,        // 0x67
    disasm_MOV_L_B,        // 0x68
    disasm_MOV_L_C,        // 0x69
    disasm_MOV_L_D,        // 0x6a
    disasm_MOV_L_E,        // 0x6b
    disasm_MOV_L_H,        // 0x6c
    disasm_MOV_L_L,        // 0x6d
    disasm_MOV_L_M,        // 0x6e
    disasm_MOV_L_A,        // 0x6f
    disasm_MOV_M_B,        // 0x70
    disasm_MOV_M_C,        // 0x71
    disasm_MOV_M_D,        // 0x72
    disasm_MOV_M_E,        // 0x73
    disasm_MOV_M_H,        // 0x74
    disasm_MOV_M_L,        // 0x75
    disasm_HLT,            // 0x76
    disasm_MOV_M_A,        // 0x77
    disasm_MOV_A_B,        // 0x78
    disasm_MOV_A_C,        // 0x79
    disasm_MOV_A_D,        // 0x7a
    disasm_MOV_A_E,        // 0x7b
    disasm_MOV_A_H,        // 0x7c
    disasm_MOV_A_L,        // 0x7d
    disasm_MOV_A_M,        // 0x7e
    disasm_MOV_A_A,        // 0x7f
    disasm_ADD_B,          // 0x80
    disasm_ADD_C,          // 0x81
    disasm_ADD_D,          // 0x82
    disasm_ADD_E,          // 0x83
    disasm_ADD_H,          // 0x84
    disasm_ADD_L,          // 0x85
    disasm_ADD_M,          // 0x86
    disasm_ADD_A,          // 0x87
    disasm_ADC_B,          // 0x88
    disasm_ADC_C,          // 0x89
    disasm_ADC_D,          // 0x8a
    disasm_ADC_E,          // 0x8b
    disasm_ADC_H,          // 0x8c
    disasm_ADC_L,          // 0x8d
    disasm_ADC_M,          // 0x8e
    disasm_ADC_A,          // 0x8f
    disasm_SUB_B,          // 0x90
    disasm_SUB_C,          // 0x91
    disasm_SUB_D,          // 0x92
    disasm_SUB_E,          // 0x93
    disasm_SUB_H,          // 0x94
    disasm_SUB_L,          // 0x95
    disasm_SUB_M,          // 0x96
    disasm_SUB_A,          // 0x97
    disasm_SBB_B,          // 0x98
    disasm_SBB_C,          // 0x99
    disasm_SBB_D,          // 0x9a
    disasm_SBB_E,          // 0x9b
    disasm_SBB_H,          // 0x9c
    disasm_SBB_L,          // 0x9d
    disasm_SBB_M,          // 0x9e
    disasm_SBB_A,          // 0x9f
    disasm_ANA_B,          // 0xa0
    disasm_ANA_C,          // 0xa1
    disasm_ANA_D,          // 0xa2
    disasm_ANA_E,          // 0xa3
    disasm_ANA_H,          // 0xa4
    disasm_ANA_L,          // 0xa5
    disasm_ANA_M,          // 0xa6
    disasm_ANA_A,          // 0xa7
    disasm_XRA_B,          // 0xa8
    disasm_XRA_C,          // 0xa9
    disasm_XRA_D,          // 0xaa
    disasm_XRA_E,          // 0xab
    disasm_XRA_H,          // 0xac
    disasm_XRA_L,          // 0xad
    disasm_XRA_M,          // 0xae
    disasm_XRA_A,          // 0xaf
    disasm_ORA_B,          // 0xb0
    disasm_ORA_C,          // 0xb1
    disasm_ORA_D,          // 0xb2
    disasm_ORA_E,          // 0xb3
    disasm_ORA_H,          // 0xb4
    disasm_ORA_L,          // 0xb5
    disasm_ORA_M,          // 0xb6
    disasm_ORA_A,          // 0xb7
    disasm_CMP_B,          // 0xb8
    disasm_CMP_C,          // 0xb9
    disasm_CMP_D,          // 0xba
    disasm_CMP_E,          // 0xbb
    disasm_CMP_H,          // 0xbc
    disasm_CMP_L,          // 0xbd
    disasm_CMP_M,          // 0xbe
    disasm_CMP_A,          // 0xbf
    disasm_RNZ,            // 0xc0
    disasm_POP_B,          // 0xc1
    disasm_JNZ,            // 0xc2
    disasm_JMP,            // 0xc3
    disasm_CNZ,            // 0xc4
    disasm_PUSH_B,         // 0xc5
    disasm_ADI,            // 0xc6
    disasm_RST_0,          // 0xc7
    disasm_RZ,             // 0xc8
    disasm_RET,            // 0xc9
    disasm_JZ,             // 0xca
    disasm_unimplemented,  // 0xcb
    disasm_CZ,             // 0xcc
    disasm_CALL,           // 0xcd
    disasm_ACI,            // 0xce
    disasm_RST_1,          // 0xcf
    disasm_RNC,            // 0xd0
    disasm_POP_D,          // 0xd1
    disasm_JNC,            // 0xd2
    disasm_OUT,            // 0xd3
    disasm_CNC,            // 0xd4
    disasm_PUSH_D,         // 0xd5
    disasm_SUI,            // 0xd6
    disasm_RST_2,          // 0xd7
    disasm_RC,             // 0xd8
    disasm_unimplemented,  // 0xd9
    disasm_JC,             // 0xda
    disasm_IN,             // 0xdb
    disasm_CC,             // 0xdc
    disasm_unimplemented,  // 0xdd
    disasm_SBI,            // 0xde
    disasm_RST_3,          // 0xdf
    disasm_RPO,            // 0xe0
    disasm_POP_H,          // 0xe1
    disasm_JPO,            // 0xe2
    disasm_XTHL,           // 0xe3
    disasm_CPO,            // 0xe4
    disasm_PUSH_H,         // 0xe5
    disasm_ANI,            // 0xe6
    disasm_RST_4,          // 0xe7
    disasm_RPE,            // 0xe8
    disasm_PCHL,           // 0xe9
    disasm_JPE,            // 0xea
    disasm_XCHG,           // 0xeb
    disasm_CPE,            // 0xec
    disasm_unimplemented,  // 0xed
    disasm_XRI,            // 0xee
    disasm_RST_5,          // 0xef
    disasm_RP,             // 0xf0
    disasm_POP_PSW,        // 0xf1
    disasm_JP,             // 0xf2
    disasm_DI,             // 0xf3
    disasm_CP,             // 0xf4
    disasm_PUSH_PSW,       // 0xf5
    disasm_ORI,            // 0xf6
    disasm_RST_6,          // 0xf7
    disasm_RM,             // 0xf8
    disasm_SPHL,           // 0xf9
    disasm_JM,             // 0xfa
    disasm_EI,             // 0xfb
    disasm_CM,             // 0xfc
    disasm_unimplemented,  // 0xfd
    disasm_CPI,            // 0xfe
    disasm_RST_7           // 0xff
};
