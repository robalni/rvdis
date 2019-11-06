#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

static const char *regs[] = {
    "zero", "ra", "sp",  "gp",  "tp", "t0", "t1", "t2",
    "s0",   "s1", "a0",  "a1",  "a2", "a3", "a4", "a5",
    "a6",   "a7", "s2",  "s3",  "s4", "s5", "s6", "s7",
    "s8",   "s9", "s10", "s11", "t3", "t4", "t5", "t6",
};

static const char *
csr_name(int n)
{
    switch (n) {
    case 0x000: return "ustatus";
    case 0x004: return "uie";
    case 0x005: return "utvec";
    case 0x040: return "uscratch";
    case 0x041: return "uepc";
    case 0x042: return "ucause";
    case 0x043: return "utval";
    case 0x044: return "uip";
    case 0x001: return "fflags";
    case 0x002: return "frm";
    case 0x003: return "fcsr";
    case 0xC00: return "cycle";
    case 0xC01: return "time";
    case 0xC02: return "instret";
    case 0xC03: return "hpmcounter3";
    case 0xC04: return "hpmcounter4";
    case 0xC1F: return "hpmcounter31";
    case 0xC80: return "cycleh";
    case 0xC81: return "timeh";
    case 0xC82: return "instreth";
    case 0xC83: return "hpmcounter3h";
    case 0xC84: return "hpmcounter4h";
    case 0xC9F: return "hpmcounter31h";
    case 0x100: return "sstatus";
    case 0x102: return "sedeleg";
    case 0x103: return "sideleg";
    case 0x104: return "sie";
    case 0x105: return "stvec";
    case 0x106: return "scounteren";
    case 0x140: return "sscratch";
    case 0x141: return "sepc";
    case 0x142: return "scause";
    case 0x143: return "stval";
    case 0x144: return "sip";
    case 0x180: return "satp";
    case 0x600: return "hstatus";
    case 0x602: return "hedeleg";
    case 0x603: return "hideleg";
    case 0x606: return "hcounteren";
    case 0x680: return "hgatp";
    case 0x605: return "htimedelta";
    case 0x615: return "htimedeltah";
    case 0x200: return "vsstatus";
    case 0x204: return "vsie";
    case 0x205: return "vstvec";
    case 0x240: return "vsscratch";
    case 0x241: return "vsepc";
    case 0x242: return "vscause";
    case 0x243: return "vstval";
    case 0x244: return "vsip";
    case 0x280: return "vsatp";
    case 0xF11: return "mvendorid";
    case 0xF12: return "marchid";
    case 0xF13: return "mimpid";
    case 0xF14: return "mhartid";
    case 0x300: return "mstatus";
    case 0x301: return "misa";
    case 0x302: return "medeleg";
    case 0x303: return "mideleg";
    case 0x304: return "mie";
    case 0x305: return "mtvec";
    case 0x306: return "mcounteren";
    case 0x310: return "mstatush";
    case 0x340: return "mscratch";
    case 0x341: return "mepc";
    case 0x342: return "mcause";
    case 0x343: return "mtval";
    case 0x344: return "mip";
    case 0x3A0: return "pmpcfg0";
    case 0x3A1: return "pmpcfg1";
    case 0x3A2: return "pmpcfg2";
    case 0x3A3: return "pmpcfg3";
    case 0x3B0: return "pmpaddr0";
    case 0x3B1: return "pmpaddr1";
    case 0x3BF: return "pmpaddr15";
    case 0xB00: return "mcycle";
    case 0xB02: return "minstret";
    case 0xB03: return "mhpmcounter3";
    case 0xB04: return "mhpmcounter4";
    case 0xB1F: return "mhpmcounter31";
    case 0xB80: return "mcycleh";
    case 0xB82: return "minstreth";
    case 0xB83: return "mhpmcounter3h";
    case 0xB84: return "mhpmcounter4h";
    case 0xB9F: return "mhpmcounter31h";
    case 0x320: return "mcountinhibit";
    case 0x323: return "mhpmevent3";
    case 0x324: return "mhpmevent4";
    case 0x33F: return "mhpmevent31";
    case 0x7A0: return "tselect";
    case 0x7A1: return "tdata1";
    case 0x7A2: return "tdata2";
    case 0x7A3: return "tdata3";
    case 0x7B0: return "dcsr";
    case 0x7B1: return "dpc";
    case 0x7B2: return "dscratch0";
    case 0x7B3: return "dscratch1";
    default: return "(unknown)";
    }
}

static uint32_t
bits(uint32_t n, uint32_t hi, uint32_t lo)
{
    assert(hi < 32);
    assert(lo < 32);
    assert(hi >= lo);
    return n >> lo & ((1 << (hi - lo + 1)) - 1);
}

static int32_t
sign_extend(int32_t n, int bits)
{
    int32_t m = n & ((1 << (bits - 1)) - 1);
    if (n >> (bits - 1) & 1) {
        return m - (1 << (bits - 1));
    } else {
        return m;
    }
}

int main()
{
    char data[1024];
    int s = read(0, data, sizeof data);
    if (s < 0) {
        perror("read");
        return 1;
    }

    for (size_t i = 0; i < s;) {
        char *p0 = &data[i];
        if ((*p0 & 0b11) != 0b11) {
            // 16 bit
            uint16_t instr = *(uint16_t*)p0;
            printf("    %04x  ", instr);
            uint32_t funct = bits(instr, 15, 13);
            uint32_t op = bits(instr, 2, 0);
            uint32_t r1 = bits(instr, 6, 2);
            uint32_t r2 = bits(instr, 11, 7);
            uint32_t b = bits(instr, 12, 12);
            int32_t imm = r1 + (b << 5);
            if (op == 0b01) {
                if (funct == 0) {
                    printf("c.addi ...");
                } else if (funct == 0b010 && r2) {
                    printf("c.li %s, %d", regs[r2], imm);
                }
            } else if (op == 0b10) {
                if (funct == 0b100 && r1 && r2 && !b) {
                    printf("c.mv %s, %s", regs[r2], regs[r1]);
                }
            }
            printf("\n");
            i += 2;
        } else if ((*p0 & 0b11100) != 0b11100 && (*p0 & 0b11) == 0b11) {
            // 32 bit
            uint32_t instr = *(uint32_t*)p0;
            printf("%08x  ", instr);
            uint32_t funct = bits(instr, 14, 12);
            uint32_t op = bits(instr, 6, 0);
            int32_t imm = bits(instr, 31, 20);
            uint32_t uimm = imm;
            imm = sign_extend(imm, 12);
            uint32_t r1 = bits(instr, 11, 7);
            uint32_t r2 = bits(instr, 19, 15);
            if (op == 0b10011) {
                if (funct == 0) {
                    printf("addi %s, %s, %d", regs[r1], regs[r2], imm);
                }
            } else if (op == 0b0100011) {
                uint32_t s_imm = bits(instr, 11, 7) | bits(instr, 31, 25) << 5;
                s_imm = sign_extend(s_imm, 12);
                uint32_t s_r1 = bits(instr, 19, 15);
                uint32_t s_r2 = bits(instr, 24, 20);
                if (funct == 0b010) {
                    printf("sw %s, %d(%s)", regs[s_r2], s_imm, regs[s_r1]);
                }
            } else if (op == 0b0000011) {
                if (funct == 0b010) {
                    printf("lw %s %d(%s)", regs[r1], imm, regs[r2]);
                }
            } else if (op == 0b1100011) {
                uint32_t b_rs1 = bits(instr, 19, 15);
                uint32_t b_rs2 = bits(instr, 24, 20);
                int32_t b_imm = bits(instr, 31, 31) << 12
                    | bits(instr, 30, 25) << 5
                    | bits(instr, 11, 8) << 1
                    | bits(instr, 7, 7) << 11;
                b_imm = sign_extend(b_imm, 13);
                if (funct == 1) {
                    printf("bne %s, %s, %d", regs[b_rs1], regs[b_rs2], b_imm);
                } else if (funct == 0b100) {
                    printf("blt %s, %s, %d", regs[b_rs1], regs[b_rs2], b_imm);
                }
            } else if (op == 0b1101111) {
                uint32_t j_rd = bits(instr, 11, 7);
                int32_t j_imm = bits(instr, 31, 31) << 20
                    | bits(instr, 30, 21) << 1
                    | bits(instr, 20, 20) << 11
                    | bits(instr, 19, 12) << 12;
                j_imm = sign_extend(j_imm, 21);
                printf("jal %s, %d", regs[j_rd], j_imm);
            } else if (op == 0b0110111) {
                uint32_t u_rd = bits(instr, 11, 7);
                int32_t u_imm = bits(instr, 31, 12);
                u_imm = sign_extend(u_imm, 20);
                printf("lui %s, 0x%02x", regs[u_rd], u_imm);
            } else if (op == 0b1110011) {
                if (funct == 0b010) {
                    printf("csrrs %s, %s, %s", regs[r1], csr_name(uimm), regs[r2]);
                } else if (instr == 0b10000010100000000000001110011) {
                    printf("wfi");
                } else if (instr == 0b1110011) {
                    printf("ecall");
                }
            }
            printf("\n");
            i += 4;
        } else {
            fprintf(stderr, "Bad instruction length %02x\n", *p0);
            i += 2;
        }
    }
}
