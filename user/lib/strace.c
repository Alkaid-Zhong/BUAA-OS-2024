#include <lib.h>

void strace_barrier(u_int env_id) {
	int straced_bak = straced;
	straced = 0;
	while (envs[ENVX(env_id)].env_status == ENV_RUNNABLE) {
		syscall_yield();
	}
	straced = straced_bak;
}


void strace_send(int sysno) {
	if (!((SYS_putchar <= sysno && sysno <= SYS_set_tlb_mod_entry) ||
	      (SYS_exofork <= sysno && sysno <= SYS_panic)) ||
	    sysno == SYS_set_trapframe) {
		return;
	}

	// Your code here. (1/2)
	
	if (straced != 0) {
		int r = straced;
		straced = 0;
		syscall_ipc_try_send(env->env_parent_id, sysno, 0, 0);
		syscall_set_env_status(env->env_id, ENV_NOT_RUNNABLE);
		straced = r;
	}
}

void strace_recv() {
	// Your code here. (2/2)

	while(1) {
		syscall_ipc_recv(0);
		u_int sysno = env->env_ipc_value;
		u_int child_env_id = env->env_ipc_from;
		strace_barrier(child_env_id);
		recv_sysno(child_env_id, sysno);
		syscall_set_env_status(child_env_id, ENV_RUNNABLE);
		if (sysno == SYS_env_destroy) {
			break;
		}
	}

}
