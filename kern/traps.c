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
    int rs = ((*instr) >> 21) & 0x1f;
    int rt = ((*instr) >> 16) & 0x1f;
    int rd = ((*instr) >> 11) & 0x1f;
    int spc = ((*instr) >> 6) & 0x1f;
    int funcCode = (*instr) & 0x3f;

    tf->cp0_epc += 4;
    
    // printk("instr:%x func: %x\n", *instr, funcCode);
}