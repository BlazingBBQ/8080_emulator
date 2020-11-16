#include <stdint.h>

#define EMU_UNIMPLEMENTED(name)    \
    int name(emu_state_t *state) { \
        (void)(state);             \
        exit(1);                   \
    }

/* Define register pair high-order and low-order registers */
#define RP_BC_RH (state->b)
#define RP_BC_RL (state->c)
#define RP_DE_RH (state->d)
#define RP_DE_RL (state->e)
#define RP_HL_RH (state->h)
#define RP_HL_RL (state->l)
#define RP_SP_RH (state->sp_h)
#define RP_SP_RL (state->sp_l)

#define SP ((RP_SP_RH << sizeof(uint8_t)) + RP_SP_RL)

#define LOW_ORDER_DATA (state->mem[state->pc + 1])
#define HIGH_ORDER_DATA (state->mem[state->pc + 2])
#define DATA (LOW_ORDER_DATA)

/* The content of the memory location, whose address is in registers B and C. */
#define MEM_BC (*(state->mem + (RP_BC_RH << sizeof(uint8_t)) + RP_BC_RL))
/* The content of the memory location, whose address is in registers D and E. */
#define MEM_DE (*(state->mem + (RP_DE_RH << sizeof(uint8_t)) + RP_DE_RL))
/* The content of the memory location, whose address is in registers H and L. */
#define MEM_HL (*(state->mem + (RP_HL_RH << sizeof(uint8_t)) + RP_HL_RL))
/* The content of the memory location, whose address is in SP registers. */
#define MEM_SP (*(state->mem + SP))

/* The content of the memory location, whose address is specified in byte 2 and
 * byte 3 of the instruction. */
#define MEM_ADDR \
    (*(state->mem + (HIGH_ORDER_DATA << sizeof(uint8_t)) + LOW_ORDER_DATA))
/* The content of the memory location at the succeeding address. */
#define MEM_ADDR_1 \
    (*(state->mem + (HIGH_ORDER_DATA << sizeof(uint8_t)) + LOW_ORDER_DATA + 1))

typedef struct {
    uint8_t z : 1;    // Zero
    uint8_t s : 1;    // Sign
    uint8_t p : 1;    // Parity
    uint8_t cy : 1;   // Carry
    uint8_t ac : 1;   // Auxilliary Carry
    uint8_t pad : 3;  // Pad remain bits
} condition_flags_t;

typedef struct {
    uint8_t a;
    uint8_t b;
    uint8_t c;
    uint8_t d;
    uint8_t e;
    uint8_t h;
    uint8_t l;
    uint16_t pc;   // Program counter
    uint8_t sp_h;  // Stack pointer (high & low)
    uint8_t sp_l;
    condition_flags_t cf;
    uint8_t *mem;
} emu_state_t;

/*
 * parity: Calculates the module 2 sum of the bits of the given value
 *
 * Arguments:
 *   val - value of which to calculate parity
 *
 * Returns:
 *   1 for even parity, 0 for odd parity.
 */
int parity(uint8_t val) {
    int bit_sum = 0;
    for (int i = 0; i < 8; i++) {
        bit_sum += (val >> i) & 0x01;
    }

    return bit_sum % 2 == 0;
}

/*
 * carry: Checks if there is a carry-out from the addition of given operands and
 *        carry-in
 *
 * Arguments:
 *   op1    - first operand
 *   op2    - second operand
 *   bit_no - bit to check for carry-out
 *   cy     - carry-in
 *
 * Returns:
 *   1 for carry-out, 0 for no carry-out.
 */
int carry(uint8_t op1, uint8_t op2, uint32_t bit_no, uint8_t cy) {
    int16_t result = op1 + op2 + cy;
    /* The bit at target offset will only be changed in resulting value if there
     * is a carry-out from lower bits.
     *
     * https://stackoverflow.com/questions/8034566/overflow-and-carry-flags-on-z80/8037485#8037485
     */
    return ((result ^ op1 ^ op2) & (1 << bit_no)) > 0;
}

/*
 * emu_update_zsp: Updates the Zero, Sign and Parity flags based on the result
 *                 of the operation
 *
 * Arguments:
 *   state  - emulator state to update
 *   result - result of the operation for which to update the flags
 *
 * Returns:
 *   None.
 */
void emu_update_zsp(emu_state_t *state, uint8_t result) {
    state->cf.z = (result == 0);
    state->cf.s = (result >> 7 == 1);
    state->cf.p = parity(result);
}

/*
 * emu_add: Add given value to the target register and update flags accordingly
 *
 * Arguments:
 *   state  - emulator state
 *   reg    - pointer to register on which to perform addition
 *   val    - value to add to register
 *   cy     - carry-in
 *
 * Returns:
 *   None.
 */
void emu_add(emu_state_t *state, uint8_t *reg, uint8_t val, uint8_t cy) {
    uint8_t result = *reg + val + cy;
    emu_update_zsp(state, result);
    state->cf.cy = carry(*reg, val, 8, cy);
    state->cf.ac = carry(*reg, val, 4, cy);
    *reg = result;
}

/*
 * emu_sub: Sub given value from the target register and update flags
 *          accordingly
 *
 * Arguments:
 *   state  - emulator state
 *   reg    - pointer to register on which to perform subtraction
 *   val    - value to sub from register
 *   cy     - carry-in
 *
 * Returns:
 *   None.
 */
void emu_sub(emu_state_t *state, uint8_t *reg, uint8_t val, uint8_t cy) {
    /* See https://en.wikipedia.org/wiki/Carry_flag#Vs._borrow_flag */
    emu_add(state, reg, ~val, !cy);
    state->cf.cy = !state->cf.cy;
}

/*
 * emu_inr: Increment value of register
 *          Does not affect cy flag
 *
 * Arguments:
 *   state  - emulator state
 *   reg    - pointer to register to increment
 *
 * Returns:
 *   None.
 */
void emu_inr(emu_state_t *state, uint8_t *reg) {
    *reg += 1;
    emu_update_zsp(state, *reg);
    state->cf.ac = (*reg == 0x0);
}

/*
 * emu_dcr: Decrement value of register
 *          Does not affect cy flag
 *
 * Arguments:
 *   state  - emulator state
 *   reg    - pointer to register to decrement
 *
 * Returns:
 *   None.
 */
void emu_dcr(emu_state_t *state, uint8_t *reg) {
    *reg -= 1;
    emu_update_zsp(state, *reg);
    state->cf.ac = (*reg == 0xF);
}

EMU_UNIMPLEMENTED(emu_unimplemented)

/* --- 8080 Instructions --- */

int emu_NOP(emu_state_t *state) {
    (void)(state);
    return 1;
}

int emu_LXI_B(emu_state_t *state) {
    RP_BC_RH = HIGH_ORDER_DATA;
    RP_BC_RL = LOW_ORDER_DATA;
    return 3;
}

int emu_STAX_B(emu_state_t *state) {
    MEM_BC = state->a;
    return 1;
}

int emu_INX_B(emu_state_t *state) {
    MEM_BC += 1;
    return 1;
}

int emu_INR_B(emu_state_t *state) {
    emu_inr(state, &state->b);
    return 1;
}

int emu_DCR_B(emu_state_t *state) {
    emu_dcr(state, &state->b);
    return 1;
}

int emu_MVI_B(emu_state_t *state) {
    state->b = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_RLC)
// 0x08 --
EMU_UNIMPLEMENTED(emu_DAD_B)

int emu_LDAX_B(emu_state_t *state) {
    state->a = MEM_BC;
    return 1;
}

int emu_DCX_B(emu_state_t *state) {
    MEM_BC -= 1;
    return 1;
}

int emu_INR_C(emu_state_t *state) {
    emu_inr(state, &state->c);
    return 1;
}

int emu_DCR_C(emu_state_t *state) {
    emu_dcr(state, &state->c);
    return 1;
}

int emu_MVI_C(emu_state_t *state) {
    state->c = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_RRC)
// 0x10 --

int emu_LXI_D(emu_state_t *state) {
    RP_DE_RH = HIGH_ORDER_DATA;
    RP_DE_RL = LOW_ORDER_DATA;
    return 3;
}

int emu_STAX_D(emu_state_t *state) {
    MEM_DE = state->a;
    return 1;
}

int emu_INX_D(emu_state_t *state) {
    MEM_DE += 1;
    return 1;
}

int emu_INR_D(emu_state_t *state) {
    emu_inr(state, &state->d);
    return 1;
}

int emu_DCR_D(emu_state_t *state) {
    emu_dcr(state, &state->d);
    return 1;
}

int emu_MVI_D(emu_state_t *state) {
    state->d = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_RAL)
// 0x18 --
EMU_UNIMPLEMENTED(emu_DAD_D)

int emu_LDAX_D(emu_state_t *state) {
    state->a = MEM_DE;
    return 1;
}

int emu_DCX_D(emu_state_t *state) {
    MEM_DE -= 1;
    return 1;
}

int emu_INR_E(emu_state_t *state) {
    emu_inr(state, &state->e);
    return 1;
}

int emu_DCR_E(emu_state_t *state) {
    emu_dcr(state, &state->e);
    return 1;
}

int emu_MVI_E(emu_state_t *state) {
    state->e = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_RAR)
EMU_UNIMPLEMENTED(emu_RIM)

int emu_LXI_H(emu_state_t *state) {
    RP_HL_RH = HIGH_ORDER_DATA;
    RP_HL_RL = LOW_ORDER_DATA;
    return 3;
}

int emu_SHLD(emu_state_t *state) {
    MEM_ADDR = state->l;
    MEM_ADDR_1 = state->h;
    return 3;
}

int emu_INX_H(emu_state_t *state) {
    MEM_HL += 1;
    return 1;
}

int emu_INR_H(emu_state_t *state) {
    emu_inr(state, &state->h);
    return 1;
}

int emu_DCR_H(emu_state_t *state) {
    emu_dcr(state, &state->h);
    return 1;
}

int emu_MVI_H(emu_state_t *state) {
    state->h = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_DAA)
// 0x28 --
EMU_UNIMPLEMENTED(emu_DAD_H)

int emu_LHLD(emu_state_t *state) {
    state->l = MEM_ADDR;
    state->h = MEM_ADDR_1;
    return 3;
}

int emu_DCX_H(emu_state_t *state) {
    MEM_HL -= 1;
    return 1;
}

int emu_INR_L(emu_state_t *state) {
    emu_inr(state, &state->l);
    return 1;
}

int emu_DCR_L(emu_state_t *state) {
    emu_dcr(state, &state->l);
    return 1;
}

int emu_MVI_L(emu_state_t *state) {
    state->l = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_CMA)
EMU_UNIMPLEMENTED(emu_SIM)

int emu_LXI_SP(emu_state_t *state) {
    RP_SP_RH = HIGH_ORDER_DATA;
    RP_SP_RL = LOW_ORDER_DATA;
    return 3;
}

int emu_STA(emu_state_t *state) {
    MEM_ADDR = state->a;
    return 3;
}

int emu_INX_SP(emu_state_t *state) {
    MEM_SP += 1;
    return 1;
}

int emu_INR_M(emu_state_t *state) {
    emu_inr(state, MEM_HL);
    return 1;
}

int emu_DCR_M(emu_state_t *state) {
    emu_dcr(state, MEM_HL);
    return 1;
}

int emu_MVI_M(emu_state_t *state) {
    MEM_HL = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_STC)
// 0x38 --
EMU_UNIMPLEMENTED(emu_DAD_SP)

int emu_LDA(emu_state_t *state) {
    state->a = MEM_ADDR;
    return 3;
}

int emu_DCX_SP(emu_state_t *state) {
    MEM_SP -= 1;
    return 1;
}

int emu_INR_A(emu_state_t *state) {
    emu_inr(state, &state->a);
    return 1;
}

int emu_DCR_A(emu_state_t *state) {
    emu_dcr(state, &state->a);
    return 1;
}

int emu_MVI_A(emu_state_t *state) {
    state->a = DATA;
    return 2;
}

EMU_UNIMPLEMENTED(emu_CMC)

int emu_MOV_B_B(emu_state_t *state) {
    state->b = state->b;
    return 1;
}

int emu_MOV_B_C(emu_state_t *state) {
    state->b = state->c;
    return 1;
}

int emu_MOV_B_D(emu_state_t *state) {
    state->b = state->d;
    return 1;
}

int emu_MOV_B_E(emu_state_t *state) {
    state->b = state->e;
    return 1;
}

int emu_MOV_B_H(emu_state_t *state) {
    state->b = state->h;
    return 1;
}

int emu_MOV_B_L(emu_state_t *state) {
    state->b = state->l;
    return 1;
}

int emu_MOV_B_M(emu_state_t *state) {
    state->b = MEM_HL;
    return 1;
}

int emu_MOV_B_A(emu_state_t *state) {
    state->b = state->a;
    return 1;
}

int emu_MOV_C_B(emu_state_t *state) {
    state->c = state->b;
    return 1;
}

int emu_MOV_C_C(emu_state_t *state) {
    state->c = state->c;
    return 1;
}

int emu_MOV_C_D(emu_state_t *state) {
    state->c = state->d;
    return 1;
}

int emu_MOV_C_E(emu_state_t *state) {
    state->c = state->e;
    return 1;
}

int emu_MOV_C_H(emu_state_t *state) {
    state->c = state->h;
    return 1;
}

int emu_MOV_C_L(emu_state_t *state) {
    state->c = state->l;
    return 1;
}

int emu_MOV_C_M(emu_state_t *state) {
    state->c = MEM_HL;
    return 1;
}

int emu_MOV_C_A(emu_state_t *state) {
    state->c = state->a;
    return 1;
}

int emu_MOV_D_B(emu_state_t *state) {
    state->d = state->b;
    return 1;
}

int emu_MOV_D_C(emu_state_t *state) {
    state->d = state->c;
    return 1;
}

int emu_MOV_D_D(emu_state_t *state) {
    state->d = state->d;
    return 1;
}

int emu_MOV_D_E(emu_state_t *state) {
    state->d = state->e;
    return 1;
}

int emu_MOV_D_H(emu_state_t *state) {
    state->d = state->h;
    return 1;
}

int emu_MOV_D_L(emu_state_t *state) {
    state->d = state->l;
    return 1;
}

int emu_MOV_D_M(emu_state_t *state) {
    state->d = MEM_HL;
    return 1;
}

int emu_MOV_D_A(emu_state_t *state) {
    state->d = state->a;
    return 1;
}

int emu_MOV_E_B(emu_state_t *state) {
    state->e = state->b;
    return 1;
}

int emu_MOV_E_C(emu_state_t *state) {
    state->e = state->c;
    return 1;
}

int emu_MOV_E_D(emu_state_t *state) {
    state->e = state->d;
    return 1;
}

int emu_MOV_E_E(emu_state_t *state) {
    state->e = state->e;
    return 1;
}

int emu_MOV_E_H(emu_state_t *state) {
    state->e = state->h;
    return 1;
}

int emu_MOV_E_L(emu_state_t *state) {
    state->e = state->l;
    return 1;
}

int emu_MOV_E_M(emu_state_t *state) {
    state->e = MEM_HL;
    return 1;
}

int emu_MOV_E_A(emu_state_t *state) {
    state->e = state->a;
    return 1;
}

int emu_MOV_H_B(emu_state_t *state) {
    state->h = state->b;
    return 1;
}

int emu_MOV_H_C(emu_state_t *state) {
    state->h = state->c;
    return 1;
}

int emu_MOV_H_D(emu_state_t *state) {
    state->h = state->d;
    return 1;
}

int emu_MOV_H_E(emu_state_t *state) {
    state->h = state->e;
    return 1;
}

int emu_MOV_H_H(emu_state_t *state) {
    state->h = state->h;
    return 1;
}

int emu_MOV_H_L(emu_state_t *state) {
    state->h = state->l;
    return 1;
}

int emu_MOV_H_M(emu_state_t *state) {
    state->h = MEM_HL;
    return 1;
}

int emu_MOV_H_A(emu_state_t *state) {
    state->h = state->a;
    return 1;
}

int emu_MOV_L_B(emu_state_t *state) {
    state->l = state->b;
    return 1;
}

int emu_MOV_L_C(emu_state_t *state) {
    state->l = state->c;
    return 1;
}

int emu_MOV_L_D(emu_state_t *state) {
    state->l = state->d;
    return 1;
}

int emu_MOV_L_E(emu_state_t *state) {
    state->l = state->e;
    return 1;
}

int emu_MOV_L_H(emu_state_t *state) {
    state->l = state->h;
    return 1;
}

int emu_MOV_L_L(emu_state_t *state) {
    state->l = state->l;
    return 1;
}

int emu_MOV_L_M(emu_state_t *state) {
    state->l = MEM_HL;
    return 1;
}

int emu_MOV_L_A(emu_state_t *state) {
    state->l = state->a;
    return 1;
}

int emu_MOV_M_B(emu_state_t *state) {
    MEM_HL = state->b;
    return 1;
}

int emu_MOV_M_C(emu_state_t *state) {
    MEM_HL = state->c;
    return 1;
}

int emu_MOV_M_D(emu_state_t *state) {
    MEM_HL = state->d;
    return 1;
}

int emu_MOV_M_E(emu_state_t *state) {
    MEM_HL = state->e;
    return 1;
}

int emu_MOV_M_H(emu_state_t *state) {
    MEM_HL = state->h;
    return 1;
}

int emu_MOV_M_L(emu_state_t *state) {
    MEM_HL = state->l;
    return 1;
}

EMU_UNIMPLEMENTED(emu_HLT)

int emu_MOV_M_A(emu_state_t *state) {
    MEM_HL = state->a;
    return 1;
}

int emu_MOV_A_B(emu_state_t *state) {
    state->a = state->b;
    return 1;
}

int emu_MOV_A_C(emu_state_t *state) {
    state->a = state->c;
    return 1;
}

int emu_MOV_A_D(emu_state_t *state) {
    state->a = state->d;
    return 1;
}

int emu_MOV_A_E(emu_state_t *state) {
    state->a = state->e;
    return 1;
}

int emu_MOV_A_H(emu_state_t *state) {
    state->a = state->h;
    return 1;
}

int emu_MOV_A_L(emu_state_t *state) {
    state->a = state->l;
    return 1;
}

int emu_MOV_A_M(emu_state_t *state) {
    state->a = MEM_HL;
    return 1;
}

int emu_MOV_A_A(emu_state_t *state) {
    state->a = state->a;
    return 1;
}

int emu_ADD_B(emu_state_t *state) {
    emu_add(state, &state->a, state->b, 0);
    return 1;
}

int emu_ADD_C(emu_state_t *state) {
    emu_add(state, &state->a, state->c, 0);
    return 1;
}

int emu_ADD_D(emu_state_t *state) {
    emu_add(state, &state->a, state->d, 0);
    return 1;
}

int emu_ADD_E(emu_state_t *state) {
    emu_add(state, &state->a, state->e, 0);
    return 1;
}

int emu_ADD_H(emu_state_t *state) {
    emu_add(state, &state->a, state->h, 0);
    return 1;
}

int emu_ADD_L(emu_state_t *state) {
    emu_add(state, &state->a, state->l, 0);
    return 1;
}

int emu_ADD_M(emu_state_t *state) {
    emu_add(state, &state->a, MEM_HL, 0);
    return 1;
}

int emu_ADD_A(emu_state_t *state) {
    emu_add(state, &state->a, state->a, 0);
    return 1;
}

int emu_ADC_B(emu_state_t *state) {
    emu_add(state, &state->a, state->b, state->cf.cy);
    return 1;
}

int emu_ADC_C(emu_state_t *state) {
    emu_add(state, &state->a, state->c, state->cf.cy);
    return 1;
}

int emu_ADC_D(emu_state_t *state) {
    emu_add(state, &state->a, state->d, state->cf.cy);
    return 1;
}

int emu_ADC_E(emu_state_t *state) {
    emu_add(state, &state->a, state->e, state->cf.cy);
    return 1;
}

int emu_ADC_H(emu_state_t *state) {
    emu_add(state, &state->a, state->h, state->cf.cy);
    return 1;
}

int emu_ADC_L(emu_state_t *state) {
    emu_add(state, &state->a, state->l, state->cf.cy);
    return 1;
}

int emu_ADC_M(emu_state_t *state) {
    emu_add(state, &state->a, MEM_HL, state->cf.cy);
    return 1;
}

int emu_ADC_A(emu_state_t *state) {
    emu_add(state, &state->a, state->a, state->cf.cy);
    return 1;
}

int emu_SUB_B(emu_state_t *state) {
    emu_sub(state, &state->a, state->b, 0);
    return 1;
}

int emu_SUB_C(emu_state_t *state) {
    emu_sub(state, &state->a, state->c, 0);
    return 1;
}

int emu_SUB_D(emu_state_t *state) {
    emu_sub(state, &state->a, state->d, 0);
    return 1;
}

int emu_SUB_E(emu_state_t *state) {
    emu_sub(state, &state->a, state->e, 0);
    return 1;
}

int emu_SUB_H(emu_state_t *state) {
    emu_sub(state, &state->a, state->h, 0);
    return 1;
}

int emu_SUB_L(emu_state_t *state) {
    emu_sub(state, &state->a, state->l, 0);
    return 1;
}

int emu_SUB_M(emu_state_t *state) {
    emu_sub(state, &state->a, MEM_HL, 0);
    return 1;
}

int emu_SUB_A(emu_state_t *state) {
    emu_sub(state, &state->a, state->a, 0);
    return 1;
}

int emu_SBB_B(emu_state_t *state) {
    emu_sub(state, &state->a, state->b, state->cf.cy);
    return 1;
}

int emu_SBB_C(emu_state_t *state) {
    emu_sub(state, &state->a, state->c, state->cf.cy);
    return 1;
}

int emu_SBB_D(emu_state_t *state) {
    emu_sub(state, &state->a, state->d, state->cf.cy);
    return 1;
}

int emu_SBB_E(emu_state_t *state) {
    emu_sub(state, &state->a, state->e, state->cf.cy);
    return 1;
}

int emu_SBB_H(emu_state_t *state) {
    emu_sub(state, &state->a, state->h, state->cf.cy);
    return 1;
}

int emu_SBB_L(emu_state_t *state) {
    emu_sub(state, &state->a, state->l, state->cf.cy);
    return 1;
}

int emu_SBB_M(emu_state_t *state) {
    emu_sub(state, &state->a, MEM_HL, state->cf.cy);
    return 1;
}

int emu_SBB_A(emu_state_t *state) {
    emu_sub(state, &state->a, state->a, state->cf.cy);
    return 1;
}

EMU_UNIMPLEMENTED(emu_ANA_B)
EMU_UNIMPLEMENTED(emu_ANA_C)
EMU_UNIMPLEMENTED(emu_ANA_D)
EMU_UNIMPLEMENTED(emu_ANA_E)
EMU_UNIMPLEMENTED(emu_ANA_H)
EMU_UNIMPLEMENTED(emu_ANA_L)
EMU_UNIMPLEMENTED(emu_ANA_M)
EMU_UNIMPLEMENTED(emu_ANA_A)
EMU_UNIMPLEMENTED(emu_XRA_B)
EMU_UNIMPLEMENTED(emu_XRA_C)
EMU_UNIMPLEMENTED(emu_XRA_D)
EMU_UNIMPLEMENTED(emu_XRA_E)
EMU_UNIMPLEMENTED(emu_XRA_H)
EMU_UNIMPLEMENTED(emu_XRA_L)
EMU_UNIMPLEMENTED(emu_XRA_M)
EMU_UNIMPLEMENTED(emu_XRA_A)
EMU_UNIMPLEMENTED(emu_ORA_B)
EMU_UNIMPLEMENTED(emu_ORA_C)
EMU_UNIMPLEMENTED(emu_ORA_D)
EMU_UNIMPLEMENTED(emu_ORA_E)
EMU_UNIMPLEMENTED(emu_ORA_H)
EMU_UNIMPLEMENTED(emu_ORA_L)
EMU_UNIMPLEMENTED(emu_ORA_M)
EMU_UNIMPLEMENTED(emu_ORA_A)
EMU_UNIMPLEMENTED(emu_CMP_B)
EMU_UNIMPLEMENTED(emu_CMP_C)
EMU_UNIMPLEMENTED(emu_CMP_D)
EMU_UNIMPLEMENTED(emu_CMP_E)
EMU_UNIMPLEMENTED(emu_CMP_H)
EMU_UNIMPLEMENTED(emu_CMP_L)
EMU_UNIMPLEMENTED(emu_CMP_M)
EMU_UNIMPLEMENTED(emu_CMP_A)
EMU_UNIMPLEMENTED(emu_RNZ)
EMU_UNIMPLEMENTED(emu_POP_B)
EMU_UNIMPLEMENTED(emu_JNZ)
EMU_UNIMPLEMENTED(emu_JMP)
EMU_UNIMPLEMENTED(emu_CNZ)
EMU_UNIMPLEMENTED(emu_PUSH_B)

int emu_ADI(emu_state_t *state) {
    emu_add(state, &state->a, DATA, 0);
    return 2;
}

EMU_UNIMPLEMENTED(emu_RST_0)
EMU_UNIMPLEMENTED(emu_RZ)
EMU_UNIMPLEMENTED(emu_RET)
EMU_UNIMPLEMENTED(emu_JZ)
// 0xcb --
EMU_UNIMPLEMENTED(emu_CZ)
EMU_UNIMPLEMENTED(emu_CALL)

int emu_ACI(emu_state_t *state) {
    emu_add(state, &state->a, DATA, state->cf.cy);
    return 2;
}

EMU_UNIMPLEMENTED(emu_RST_1)
EMU_UNIMPLEMENTED(emu_RNC)
EMU_UNIMPLEMENTED(emu_POP_D)
EMU_UNIMPLEMENTED(emu_JNC)
EMU_UNIMPLEMENTED(emu_OUT)
EMU_UNIMPLEMENTED(emu_CNC)
EMU_UNIMPLEMENTED(emu_PUSH_D)

int emu_SUI(emu_state_t *state) {
    emu_sub(state, &state->a, DATA, 0);
    return 2;
}

EMU_UNIMPLEMENTED(emu_RST_2)
EMU_UNIMPLEMENTED(emu_RC)
// 0xd9 --
EMU_UNIMPLEMENTED(emu_JC)
EMU_UNIMPLEMENTED(emu_IN)
EMU_UNIMPLEMENTED(emu_CC)
// 0xdd --

int emu_SBI(emu_state_t *state) {
    emu_sub(state, &state->a, DATA, state->cf.cy);
    return 2;
}

EMU_UNIMPLEMENTED(emu_RST_3)
EMU_UNIMPLEMENTED(emu_RPO)
EMU_UNIMPLEMENTED(emu_POP_H)
EMU_UNIMPLEMENTED(emu_JPO)
EMU_UNIMPLEMENTED(emu_XTHL)
EMU_UNIMPLEMENTED(emu_CPO)
EMU_UNIMPLEMENTED(emu_PUSH_H)
EMU_UNIMPLEMENTED(emu_ANI)
EMU_UNIMPLEMENTED(emu_RST_4)
EMU_UNIMPLEMENTED(emu_RPE)
EMU_UNIMPLEMENTED(emu_PCHL)
EMU_UNIMPLEMENTED(emu_JPE)

int emu_XCHG(emu_state_t *state) {
    uint8_t tmp_h = state->h;
    uint8_t tmp_l = state->l;

    state->h = state->d;
    state->l = state->e;
    state->d = tmp_h;
    state->e = tmp_l;
    return 1;
}

EMU_UNIMPLEMENTED(emu_CPE)
// 0xed --
EMU_UNIMPLEMENTED(emu_XRI)
EMU_UNIMPLEMENTED(emu_RST_5)
EMU_UNIMPLEMENTED(emu_RP)
EMU_UNIMPLEMENTED(emu_POP_PSW)
EMU_UNIMPLEMENTED(emu_JP)
EMU_UNIMPLEMENTED(emu_DI)
EMU_UNIMPLEMENTED(emu_CP)
EMU_UNIMPLEMENTED(emu_PUSH_PSW)
EMU_UNIMPLEMENTED(emu_ORI)
EMU_UNIMPLEMENTED(emu_RST_6)
EMU_UNIMPLEMENTED(emu_RM)
EMU_UNIMPLEMENTED(emu_SPHL)
EMU_UNIMPLEMENTED(emu_JM)
EMU_UNIMPLEMENTED(emu_EI)
EMU_UNIMPLEMENTED(emu_CM)
// 0xfc --
EMU_UNIMPLEMENTED(emu_CPI)
EMU_UNIMPLEMENTED(emu_RST_7)

/*
 * emu_handlers: Emulation handler functions, indexed by opcode.
 *
 * Returns:
 *      Number of bytes to advance pc.
 */
int (*emu_handlers[0x100])(emu_state_t *state) = {
    emu_NOP,            // 0x00
    emu_LXI_B,          // 0x01
    emu_STAX_B,         // 0x02
    emu_INX_B,          // 0x03
    emu_INR_B,          // 0x04
    emu_DCR_B,          // 0x05
    emu_MVI_B,          // 0x06
    emu_RLC,            // 0x07
    emu_unimplemented,  // 0x08
    emu_DAD_B,          // 0x09
    emu_LDAX_B,         // 0x0a
    emu_DCX_B,          // 0x0b
    emu_INR_C,          // 0x0c
    emu_DCR_C,          // 0x0d
    emu_MVI_C,          // 0x0e
    emu_RRC,            // 0x0f
    emu_unimplemented,  // 0x10
    emu_LXI_D,          // 0x11
    emu_STAX_D,         // 0x12
    emu_INX_D,          // 0x13
    emu_INR_D,          // 0x14
    emu_DCR_D,          // 0x15
    emu_MVI_D,          // 0x16
    emu_RAL,            // 0x17
    emu_unimplemented,  // 0x18
    emu_DAD_D,          // 0x19
    emu_LDAX_D,         // 0x1a
    emu_DCX_D,          // 0x1b
    emu_INR_E,          // 0x1c
    emu_DCR_E,          // 0x1d
    emu_MVI_E,          // 0x1e
    emu_RAR,            // 0x1f
    emu_RIM,            // 0x20
    emu_LXI_H,          // 0x21
    emu_SHLD,           // 0x22
    emu_INX_H,          // 0x23
    emu_INR_H,          // 0x24
    emu_DCR_H,          // 0x25
    emu_MVI_H,          // 0x26
    emu_DAA,            // 0x27
    emu_unimplemented,  // 0x28
    emu_DAD_H,          // 0x29
    emu_LHLD,           // 0x2a
    emu_DCX_H,          // 0x2b
    emu_INR_L,          // 0x2c
    emu_DCR_L,          // 0x2d
    emu_MVI_L,          // 0x2e
    emu_CMA,            // 0x2f
    emu_SIM,            // 0x30
    emu_LXI_SP,         // 0x31
    emu_STA,            // 0x32
    emu_INX_SP,         // 0x33
    emu_INR_M,          // 0x34
    emu_DCR_M,          // 0x35
    emu_MVI_M,          // 0x36
    emu_STC,            // 0x37
    emu_unimplemented,  // 0x38
    emu_DAD_SP,         // 0x39
    emu_LDA,            // 0x3a
    emu_DCX_SP,         // 0x3b
    emu_INR_A,          // 0x3c
    emu_DCR_A,          // 0x3d
    emu_MVI_A,          // 0x3e
    emu_CMC,            // 0x3f
    emu_MOV_B_B,        // 0x40
    emu_MOV_B_C,        // 0x41
    emu_MOV_B_D,        // 0x42
    emu_MOV_B_E,        // 0x43
    emu_MOV_B_H,        // 0x44
    emu_MOV_B_L,        // 0x45
    emu_MOV_B_M,        // 0x46
    emu_MOV_B_A,        // 0x47
    emu_MOV_C_B,        // 0x48
    emu_MOV_C_C,        // 0x49
    emu_MOV_C_D,        // 0x4a
    emu_MOV_C_E,        // 0x4b
    emu_MOV_C_H,        // 0x4c
    emu_MOV_C_L,        // 0x4d
    emu_MOV_C_M,        // 0x4e
    emu_MOV_C_A,        // 0x4f
    emu_MOV_D_B,        // 0x50
    emu_MOV_D_C,        // 0x51
    emu_MOV_D_D,        // 0x52
    emu_MOV_D_E,        // 0x53
    emu_MOV_D_H,        // 0x54
    emu_MOV_D_L,        // 0x55
    emu_MOV_D_M,        // 0x56
    emu_MOV_D_A,        // 0x57
    emu_MOV_E_B,        // 0x58
    emu_MOV_E_C,        // 0x59
    emu_MOV_E_D,        // 0x5a
    emu_MOV_E_E,        // 0x5b
    emu_MOV_E_H,        // 0x5c
    emu_MOV_E_L,        // 0x5d
    emu_MOV_E_M,        // 0x5e
    emu_MOV_E_A,        // 0x5f
    emu_MOV_H_B,        // 0x60
    emu_MOV_H_C,        // 0x61
    emu_MOV_H_D,        // 0x62
    emu_MOV_H_E,        // 0x63
    emu_MOV_H_H,        // 0x64
    emu_MOV_H_L,        // 0x65
    emu_MOV_H_M,        // 0x66
    emu_MOV_H_A,        // 0x67
    emu_MOV_L_B,        // 0x68
    emu_MOV_L_C,        // 0x69
    emu_MOV_L_D,        // 0x6a
    emu_MOV_L_E,        // 0x6b
    emu_MOV_L_H,        // 0x6c
    emu_MOV_L_L,        // 0x6d
    emu_MOV_L_M,        // 0x6e
    emu_MOV_L_A,        // 0x6f
    emu_MOV_M_B,        // 0x70
    emu_MOV_M_C,        // 0x71
    emu_MOV_M_D,        // 0x72
    emu_MOV_M_E,        // 0x73
    emu_MOV_M_H,        // 0x74
    emu_MOV_M_L,        // 0x75
    emu_HLT,            // 0x76
    emu_MOV_M_A,        // 0x77
    emu_MOV_A_B,        // 0x78
    emu_MOV_A_C,        // 0x79
    emu_MOV_A_D,        // 0x7a
    emu_MOV_A_E,        // 0x7b
    emu_MOV_A_H,        // 0x7c
    emu_MOV_A_L,        // 0x7d
    emu_MOV_A_M,        // 0x7e
    emu_MOV_A_A,        // 0x7f
    emu_ADD_B,          // 0x80
    emu_ADD_C,          // 0x81
    emu_ADD_D,          // 0x82
    emu_ADD_E,          // 0x83
    emu_ADD_H,          // 0x84
    emu_ADD_L,          // 0x85
    emu_ADD_M,          // 0x86
    emu_ADD_A,          // 0x87
    emu_ADC_B,          // 0x88
    emu_ADC_C,          // 0x89
    emu_ADC_D,          // 0x8a
    emu_ADC_E,          // 0x8b
    emu_ADC_H,          // 0x8c
    emu_ADC_L,          // 0x8d
    emu_ADC_M,          // 0x8e
    emu_ADC_A,          // 0x8f
    emu_SUB_B,          // 0x90
    emu_SUB_C,          // 0x91
    emu_SUB_D,          // 0x92
    emu_SUB_E,          // 0x93
    emu_SUB_H,          // 0x94
    emu_SUB_L,          // 0x95
    emu_SUB_M,          // 0x96
    emu_SUB_A,          // 0x97
    emu_SBB_B,          // 0x98
    emu_SBB_C,          // 0x99
    emu_SBB_D,          // 0x9a
    emu_SBB_E,          // 0x9b
    emu_SBB_H,          // 0x9c
    emu_SBB_L,          // 0x9d
    emu_SBB_M,          // 0x9e
    emu_SBB_A,          // 0x9f
    emu_ANA_B,          // 0xa0
    emu_ANA_C,          // 0xa1
    emu_ANA_D,          // 0xa2
    emu_ANA_E,          // 0xa3
    emu_ANA_H,          // 0xa4
    emu_ANA_L,          // 0xa5
    emu_ANA_M,          // 0xa6
    emu_ANA_A,          // 0xa7
    emu_XRA_B,          // 0xa8
    emu_XRA_C,          // 0xa9
    emu_XRA_D,          // 0xaa
    emu_XRA_E,          // 0xab
    emu_XRA_H,          // 0xac
    emu_XRA_L,          // 0xad
    emu_XRA_M,          // 0xae
    emu_XRA_A,          // 0xaf
    emu_ORA_B,          // 0xb0
    emu_ORA_C,          // 0xb1
    emu_ORA_D,          // 0xb2
    emu_ORA_E,          // 0xb3
    emu_ORA_H,          // 0xb4
    emu_ORA_L,          // 0xb5
    emu_ORA_M,          // 0xb6
    emu_ORA_A,          // 0xb7
    emu_CMP_B,          // 0xb8
    emu_CMP_C,          // 0xb9
    emu_CMP_D,          // 0xba
    emu_CMP_E,          // 0xbb
    emu_CMP_H,          // 0xbc
    emu_CMP_L,          // 0xbd
    emu_CMP_M,          // 0xbe
    emu_CMP_A,          // 0xbf
    emu_RNZ,            // 0xc0
    emu_POP_B,          // 0xc1
    emu_JNZ,            // 0xc2
    emu_JMP,            // 0xc3
    emu_CNZ,            // 0xc4
    emu_PUSH_B,         // 0xc5
    emu_ADI,            // 0xc6
    emu_RST_0,          // 0xc7
    emu_RZ,             // 0xc8
    emu_RET,            // 0xc9
    emu_JZ,             // 0xca
    emu_unimplemented,  // 0xcb
    emu_CZ,             // 0xcc
    emu_CALL,           // 0xcd
    emu_ACI,            // 0xce
    emu_RST_1,          // 0xcf
    emu_RNC,            // 0xd0
    emu_POP_D,          // 0xd1
    emu_JNC,            // 0xd2
    emu_OUT,            // 0xd3
    emu_CNC,            // 0xd4
    emu_PUSH_D,         // 0xd5
    emu_SUI,            // 0xd6
    emu_RST_2,          // 0xd7
    emu_RC,             // 0xd8
    emu_unimplemented,  // 0xd9
    emu_JC,             // 0xda
    emu_IN,             // 0xdb
    emu_CC,             // 0xdc
    emu_unimplemented,  // 0xdd
    emu_SBI,            // 0xde
    emu_RST_3,          // 0xdf
    emu_RPO,            // 0xe0
    emu_POP_H,          // 0xe1
    emu_JPO,            // 0xe2
    emu_XTHL,           // 0xe3
    emu_CPO,            // 0xe4
    emu_PUSH_H,         // 0xe5
    emu_ANI,            // 0xe6
    emu_RST_4,          // 0xe7
    emu_RPE,            // 0xe8
    emu_PCHL,           // 0xe9
    emu_JPE,            // 0xea
    emu_XCHG,           // 0xeb
    emu_CPE,            // 0xec
    emu_unimplemented,  // 0xed
    emu_XRI,            // 0xee
    emu_RST_5,          // 0xef
    emu_RP,             // 0xf0
    emu_POP_PSW,        // 0xf1
    emu_JP,             // 0xf2
    emu_DI,             // 0xf3
    emu_CP,             // 0xf4
    emu_PUSH_PSW,       // 0xf5
    emu_ORI,            // 0xf6
    emu_RST_6,          // 0xf7
    emu_RM,             // 0xf8
    emu_SPHL,           // 0xf9
    emu_JM,             // 0xfa
    emu_EI,             // 0xfb
    emu_CM,             // 0xfc
    emu_unimplemented,  // 0xfd
    emu_CPI,            // 0xfe
    emu_RST_7           // 0xff
};
