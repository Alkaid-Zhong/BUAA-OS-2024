#include <env.h>
#include <pmap.h>
#include <printk.h>
#include <trap.h>

extern void handle_int(void);
extern void handle_tlb(void);
extern void handle_sys(void);
extern void handle_mod(void);
extern void handle_reserved(void);
extern void handle_ri(void);

void (*exception_handlers[32])(void) = {
    [0 ... 31] = handle_reserved,
    [0] = handle_int,
    [2 ... 3] = handle_tlb,
    [10] = handle_ri,
#if !defined(LAB) || LAB >= 4
    [1] = handle_mod,
    [8] = handle_sys,
#endif
};

/* Overview:
 *   The fallback handler when an unknown exception code is encountered.
 *   'genex.S' wraps this function in 'handle_reserved'.
 */
void do_reserved(struct Trapframe *tf) {
	print_tf(tf);
	panic("Unknown ExcCode %2d", (tf->cp0_cause >> 2) & 0x1f);
}

void do_ri(struct Trapframe *tf) {
    u_long va = tf->cp0_epc;
    Pte *pte;
    page_lookup(curenv->env_pgdir, va, &pte);
    u_long pa = PTE_ADDR(*pte) | (va & 0xfff);
    u_long kva = KADDR(pa);

    int *instr = (int*) kva;
    int opCode = (*instr) >> 26;
    int _rs = ((*instr) >> 21) & 0x1f;
    int _rt = ((*instr) >> 16) & 0x1f;
    int _rd = ((*instr) >> 11) & 0x1f;
    int spc = ((*instr) >> 6) & 0x1f;
    int funcCode = (*instr) & 0x3f;

    printk("op:%u %d %d %d funx:%u\n", opCode, _rs, _rt, _rd, funcCode);

    if (opCode == 0 && funcCode == 0x3f) { //pmaxud
        u_int rs = tf->regs[_rs];
        u_int rt = tf->regs[_rt];
        u_int rd = 0;
        int i;
        for(i = 0; i < 32; i += 8) {
            u_int rs_i = rs & (0xff << i);
            u_int rt_i = rt & (0xff << i);
            if (rs_i < rt_i) {
                rd = rd | rt_i;
            } else {
                rd = rd | rs_i;
            }
        }
        tf->regs[_rd] = rd;
    } else if (opCode == 0 && funcCode == 0x3e) { //cas
        u_long *rs = tf->regs[_rs];
        u_long *tmp = tf->regs[_rs];
        if (*rs == tf->regs[_rt]) {
            *rs = tf->regs[_rd];
        }
        tf->regs[_rd] = tmp;
    }

    tf->cp0_epc += 4;
    
    // printk("instr:%x func: %x\n", *instr, funcCode);
}