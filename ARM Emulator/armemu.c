#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "armemu.h"

#define NREGS 16
#define SP 13
#define LR 14
#define PC 15

// Project04: Analysis struct init
void analysis_init(struct analysis_st *ap) {
    ap->i_count = 0;
    ap->dp_count = 0;
    ap->mem_count = 0;
    ap->b_count = 0;
    ap->b_taken = 0;
    ap->b_not_taken = 0;
}

// Project04: Print results of dynamic analysis
void analysis_print(struct analysis_st *ap) {
    printf("=== Analysis\n");
    printf("I_count       = %d\n", ap->i_count);
    printf("DP_count      = %d (%.2f%%)\n", ap->dp_count,
           ((double) ap->dp_count / (double) ap->i_count) * 100.0);
    printf("SDT_count     = %d (%.2f%%)\n", ap->mem_count,
           ((double) ap->mem_count / (double) ap->i_count) * 100.0);    
    printf("B_count       = %d (%.2f%%)\n", ap->b_count,
           ((double) ap->b_count / (double) ap->i_count) * 100.0);
    printf("B_taken       = %d (%.2f%%)\n", ap->b_taken,
               ((double) ap->b_taken / (double) ap->b_count) * 100.0);
    printf("B_not_taken   = %d (%.2f%%)\n", ap->b_not_taken,
               ((double) ap->b_not_taken / (double) ap->b_count) * 100.0);
}

// Project04: Print results of dynamic analysis and cache sim
void armemu_print(struct arm_state *asp) {
    if (asp->analyze) {
        analysis_print(&(asp->analysis));
    }
    if (asp->cache_sim) {
        cache_print((&asp->cache));
    }
}

void armemu_init(struct arm_state *asp, uint32_t *func,
                 uint32_t a0, uint32_t a1,
                 uint32_t a2, uint32_t a3) {
    int i;

    /* Zero out registers */
    for (i = 0; i < NREGS; i++) {
        asp->regs[i] = 0;
    }

    /* Zero out CPSR */
    asp->cpsr = 0;

    /* Zero out the stack */
    for (i = 0; i < STACK_SIZE; i++) {
        asp->stack[i] = 0;
    }

    /* Set PC to point to address of function to emulate */
    asp->regs[PC] = (uint32_t) func;

    /* Set LR to 0 to know when we are done emulating */
    asp->regs[LR] = 0;

    /* Set SP to ??? */
    asp->regs[SP] = (uint32_t) &asp->stack[STACK_SIZE];

    /* Initialize the first 4 arguments */
    asp->regs[0] = a0;
    asp->regs[1] = a1;
    asp->regs[2] = a2;
    asp->regs[3] = a3;   

    // Project04: Initialize dynamic analysis
    analysis_init(&asp->analysis);
    
    // Project04: Initialize cache simulator
    cache_init(&asp->cache);      
}

void arm_state_print(struct arm_state *asp) {
    int i;

    for (i = 0; i < NREGS; i++) {
        printf("regs[%2d] = %8X (%d)\n", i, asp->regs[i],
               asp->regs[i]);
    }
    printf("cpsr     = %8X\n", asp->cpsr);
}

// ARMEMU CPSR HELPER FUNCTIONS
void get_cpsr_bits(uint32_t cpsr, uint32_t *n, uint32_t *z, uint32_t *c, uint32_t *v) {
    *n = (cpsr >> 31);
    *z = (cpsr >> 30) & 0b1;
    *c = (cpsr >> 29) & 0b1;
    *v = (cpsr >> 28) & 0b1;
}

void set_cpsr_bits(struct arm_state *asp, uint32_t n, uint32_t z, uint32_t c, uint32_t v) {
    uint32_t cpsr = 0;
    cpsr = cpsr | (n << 31);
    cpsr = cpsr | (z << 30);
    cpsr = cpsr | (c << 29);
    cpsr = cpsr | (v << 28);
    asp->cpsr = cpsr;
}

// ARMEMU BRANCHING
bool armemu_is_bx(uint32_t iw) {
    uint32_t bxcode = 0b000100101111111111110001;
    return ((iw >> 4) & 0xFFFFFF) == bxcode;
}

void armemu_bx(struct arm_state *asp, uint32_t iw) {
    uint32_t rn = iw & 0b1111;

    // Project04: increment dynamic analysis
    asp->analysis.b_count += 1;
    asp->analysis.b_taken += 1;    

    asp->regs[PC] = asp->regs[rn];
}

bool armemu_is_branch(uint32_t iw) {
    return ((iw >> 25) & 0b111) == 0b101;
}

bool armemu_take_branch(struct arm_state *asp, uint32_t cond) {
    bool update = false;
    uint32_t n, z, c, v;
    get_cpsr_bits(asp->cpsr, &n, &z, &c, &v);
    if (cond == 0b0000) {
        // beq
        update = z;
    } else if (cond == 0b0001) {
        // bne
        update = !z;
    } else if (cond == 0b1010) {
        //bge
        update = (n == v);
    } else if (cond == 0b1011) {
        // blt
        update = (n != v);
    } else if (cond == 0b1100) {
        // bgt
        update = (n == v) && !z;
    } else if (cond == 0b1101) {
        // ble
        update = z | (n != v);
    } else if (cond == 0b1110) {
        // b or bal
        update = 1;
    } else {
        printf("conditon not supported %d\n", cond);
        exit(-1);
    }
    return update;
}

void armemu_branch(struct arm_state *asp, uint32_t iw) {
    uint32_t link_bit, cond;
    cond = iw >> 28;
    link_bit = (iw >> 24) & 0b1;
    int32_t offset = iw & 0xFFFFFF;
    offset = (offset << 8) >> 8;
    offset = offset * 4;
    offset = offset + 8;
    bool update = armemu_take_branch(asp, cond);

    if (update) {
        // update PC
        if (link_bit) {
            asp->regs[LR] = asp->regs[PC] + 4;
        }        
        asp->regs[PC] = asp->regs[PC] + offset;
        asp->analysis.b_taken += 1;  
    } else {
        // don't take branch, increment PC normally
        asp->regs[PC] = asp->regs[PC] + 4;
        asp->analysis.b_not_taken += 1;
    }
    asp->analysis.b_count += 1;
}

// ARMEMU SINGLE DATA TRANSFER
bool armemu_is_sdt(uint32_t iw) {
    uint32_t dp_bits = (iw >> 26) & 0b11;

    return (dp_bits == 0b01);
}

uint32_t armemu_get_shift_amount(struct arm_state *asp, uint32_t iw) {
    uint32_t  shift_bit, shift_amount;
    shift_bit = (iw >> 4) & 0b1;
    shift_amount = (iw >> 7) & 0b11111;
    if (shift_bit) {
        uint32_t shift_reg = (iw >> 8) & 0b1111;
        shift_amount = asp->regs[shift_reg];
    }
    return shift_amount;
}

void armemu_sdt(struct arm_state *asp, uint32_t iw) {
    uint32_t i_bit, up_down, byte_word, load_store, rn, rm, rd, offset, target_address;
    i_bit = (iw >> 25) & 0b1;
    up_down = (iw >> 23) & 0b1;
    byte_word = (iw >> 22) & 0b1;
    load_store = (iw >> 20) & 0b1;
    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;
    rm = iw & 0b1111;
    offset = iw & 0xFFF;
    
    if (i_bit) {
        // left shift rm by shift amount
        offset = asp->regs[rm] << armemu_get_shift_amount(asp, iw);
    }
    
    if (up_down) {
        // add offset from base
        target_address = asp->regs[rn] + offset;
    } else {
        // subtract offset from base
        target_address = asp->regs[rn] - offset;
    }
    
    if (load_store) {
        // load from memory
        if (byte_word) {
            asp->regs[rd] = *((uint8_t *) target_address);
        } else {
            asp->regs[rd] = *((uint32_t *) target_address);
        }
    } else {
        // store to memory
        if (byte_word) {
            *((uint8_t *) target_address) = asp->regs[rd];
        } else {
            *((uint32_t *) target_address) = asp->regs[rd];   
        }
    }

    asp->regs[PC] = asp->regs[PC] + 4;
    asp->analysis.mem_count += 1;
}

// ARMEMU DATA PROCESSING
bool armemu_is_dp(uint32_t iw) {
    uint32_t dp_bits = (iw >> 26) & 0b11;

    return (dp_bits == 0);
}

void armemu_mov_shift(struct arm_state *asp, uint32_t iw, uint32_t i_bit, uint32_t rd, uint32_t rm, uint32_t oper2) {
    uint32_t shift_type, shift_amount;
    shift_type = (iw >> 5) & 0b11;
    shift_amount = armemu_get_shift_amount(asp, iw);
        
    if (i_bit) { 
        // mov with immediate
        asp->regs[rd] = oper2;
    } else if (shift_type == 0b00) {
        // lsl
        asp->regs[rd] = asp->regs[rm] << shift_amount;
    } else if (shift_type == 0b01) {
        // lsr
        asp->regs[rd] = asp->regs[rm] >> shift_amount;
    } else if (shift_type == 0b10) {
        // asr
        asp->regs[rd] = ((int32_t) asp->regs[rm]) >> shift_amount;
    } else {           
         // rotate
        printf("rotate has not been implemented\n");
        exit(-1);
    }
}

void armemu_cmp(struct arm_state *asp, uint32_t iw, uint32_t rn, uint32_t oper2) {
    uint32_t a, b;
    a = asp->regs[rn];
    b = oper2;
    int32_t as, bs;
    as = a;
    bs = b;
    // calculate n z c v
    int32_t result = as - bs;
    uint32_t n = (result < 0);
    uint32_t z = (result == 0);
    uint32_t c = (b > a);
    uint32_t v = 0;
    if ((as > 0) && (bs < 0)) {
        if (result < 0) {
            v = 1;
        }
    } else if ((as < 0) && (bs > 0)){
        if (result > 0) {
            v = 1;
        }
    }
    set_cpsr_bits(asp, n, z, c, v);    
}

void armemu_dp(struct arm_state *asp, uint32_t iw) {
    asp->analysis.dp_count += 1;
    uint32_t opcode, i_bit, rn, rd, rm, imm, mult;

    opcode = (iw >> 21) & 0b1111;
    i_bit = (iw >> 25) & 0b1;
    rn = (iw >> 16) & 0b1111;
    rd = (iw >> 12) & 0b1111;
    rm = iw & 0b1111;
    imm = iw & 0b11111111;
    mult = (iw >> 4) & 0b1111;
    
    uint32_t oper2;

    if (i_bit == 0) {
        oper2 = asp->regs[rm];
    } else {
        oper2 = imm;
    }

    if (opcode == 0b0010) {
        // sub
        asp->regs[rd] = asp->regs[rn] - oper2;
        
    } else if (opcode == 0b0011) {
        // rsb
        asp->regs[rd] = oper2 - asp->regs[rn];
        
    } else if (opcode == 0b0100) {
        // add
        asp->regs[rd] = asp->regs[rn] + oper2; 
               
    } else if (opcode == 0b1101) {
        // mov & shifts
        armemu_mov_shift(asp, iw, i_bit, rd, rm, oper2);
        
    } else if (opcode == 0b0000 && mult == 0b1001) {
        // mult (rd = rn)
        uint32_t rs = (iw >> 8) & 0b1111;
        asp->regs[rn] = asp->regs[rm] * asp->regs[rs];
        
    } else if (opcode == 0b0000) {
        // and
        asp->regs[rd] = asp->regs[rn] & oper2;
        
    } else if (opcode == 0b1010) {
        //cmp
        armemu_cmp(asp, iw, rn, oper2);
        
    } else {
        printf("armemu_dp() invalid opcode: %d\n", opcode);
        exit(-1);
    }

    if (rd != PC) {
        asp->regs[PC] = asp->regs[PC] + 4;   
    }
}

void armemu_one(struct arm_state *asp) {
    asp->analysis.i_count += 1;
    
    uint32_t iw = cache_lookup(&asp->cache, asp->regs[PC]);

    if (armemu_is_bx(iw)) {
        armemu_bx(asp, iw);
    } else if (armemu_is_dp(iw)) {
        armemu_dp(asp, iw);
    } else if (armemu_is_branch(iw)) {
        armemu_branch(asp, iw);
    } else if (armemu_is_sdt(iw)) {
        armemu_sdt(asp, iw);
    } else {
        printf("armemu_one() invalid instruction\n");
        exit(-1);
    }
}

int armemu(struct arm_state *asp) {

    while (asp->regs[PC] != 0) {
        armemu_one(asp);
    }

    return (int) asp->regs[0];
}
